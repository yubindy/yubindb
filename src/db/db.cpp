#include "db.h"

#include <memory.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string_view>

#include "../util/bloom.h"
#include "../util/filename.h"
#include "snapshot.h"
#include "spdlog/spdlog.h"
#include "src/db/filterblock.h"
#include "src/db/memtable.h"
#include "src/util/common.h"
#include "src/util/env.h"
#include "src/util/options.h"
#include "version_edit.h"
class Version;
class VersionSet;

namespace yubindb {
State DBImpl::NewDB() {
  VersionEdit new_db;
  new_db.SetLogNumber(0);
  new_db.SetNextFile(2);
  new_db.SetLastSequence(0);

  const std::string manifest = DescriptorFileName(dbname, 1);
  std::shared_ptr<WritableFile> file;
  State s = env->NewWritableFile(manifest, file);
  if (!s.ok()) {
    return s;
  }
  {
    walWriter loger(file);
    std::string record;
    new_db.EncodeTo(&record);
    spdlog::info("wal write record {}", record);
    s = loger.Appendrecord(record);
    if (s.ok()) {
      s = file->Close();
    }
  }
  if (s.ok()) {
    // Make "CURRENT" file that points to the new manifest file.
    s = SetCurrentFile(env.get(), dbname, 1);
  } else {
    env->DeleteFile(manifest);
  }
  return s;
}
DBImpl::DBImpl(const Options* opt, const std::string& dbname)
    : env(std::make_shared<PosixEnv>()),
      opts(opt),
      dbname(dbname),
      table_cache(new TableCache(dbname, opt)),
      db_lock(nullptr),
      shutting_down_(false),
      mem_(nullptr),
      imm_(nullptr),
      has_imm_(false),
      logfile(nullptr),
      logfilenum(0),
      logwrite(nullptr),
      batch(std::make_shared<WriteBatch>()),
      background_compaction_(false),
      versions_(std::make_unique<VersionSet>(dbname, opt, table_cache,env)) {
  if (!bloomfit) {
    bloomfit = std::make_unique<BloomFilter>(10);
  }
}
DBImpl::~DBImpl() {
  std::unique_lock<std::mutex> lk(mutex);
  shutting_down_.store(true, std::memory_order_release);
  while (background_compaction_) {
    background_work_finished_signal.wait(lk);
  }
  lk.unlock();
  if (db_lock != nullptr) {
    env->UnlockFile(db_lock);
  }
}
//该方法会检查Lock文件是否被占用（LevelDB通过名为LOCK的文件避免多个LevelDB进程同时访问一个数据库）、
//目录是否存在、Current文件是否存在等。然后主要通过VersionSet::Recover与DBImpl::RecoverLogFile
//两个方法，分别恢复其VersionSet的状态与MemTable的状态。
State DBImpl::Recover(VersionEdit* edit, bool* save_manifest) {
  env->CreateDir(dbname);
  State s = env->LockFile(LockFileName(dbname), db_lock);
  if (!s.ok()) {
    return s;
  }
  if (!env->FileExists(CurrentFileName(dbname))) {
    s = NewDB();
    if (!s.ok()) {
      return s;
    }
  }
  s = versions_->Recover(save_manifest);
  if (!s.ok()) {
    return s;
  }
  SequenceNum max_sequence(0);
  // TODO
  return State::Ok();
}
State DBImpl::Open(const Options& options, std::string name, DB** dbptr) {
  *dbptr = nullptr;

  DBImpl* impl = new DBImpl(&options, name);
  impl->mutex.lock();
  VersionEdit edit;
  // Recover handles create_if_missing, error_if_exists
  bool save_manifest = false;
  State s = impl->Recover(&edit, &save_manifest);  //恢复自身状态。
  if (s.ok() && impl->mem_ == nullptr) {
    // Create newlog and a corresponding memtable.
    uint64_t new_log_number = impl->versions_->NewFileNumber();
    std::shared_ptr<WritableFile> lfile;
    s = impl->env->NewWritableFile(LogFileName(name, new_log_number), lfile);
    if (s.ok()) {
      edit.SetLogNumber(new_log_number);
      impl->logfile = lfile;
      impl->logfilenum = new_log_number;
      impl->logwrite = std::make_unique<walWriter>(lfile);
      impl->mem_ = std::make_shared<Memtable>();
    }
  }
  if (s.ok() && save_manifest) {
    edit.SetLogNumber(impl->logfilenum);
    s = impl->versions_->LogAndApply(&edit, &impl->mutex);
  }
  if (s.ok()) {
    impl->DeleteObsoleteFiles();  //删除过期文件
    impl->MaybeCompaction();
  }
  impl->mutex.unlock();
  if (s.ok()) {
    assert(impl->mem_ != nullptr);
    *dbptr = impl;
  } else {
    delete impl;
  }
  return s;
}
State DBImpl::Put(const WriteOptions& opt, std::string_view key,
                  std::string_view value) {
  WriteBatch batch;
  batch.Put(key, value);
  return Write(opt, &batch);
}
State DBImpl::Delete(const WriteOptions& opt, std::string_view key) {
  WriteBatch batch;
  batch.Delete(key);
  return Write(opt, &batch);
}
State DBImpl::Write(const WriteOptions& opt, WriteBatch* updates) {
  Writer w(&mutex, opt.sync, false, updates);
  std::unique_lock<std::mutex> ptr(mutex);  // mutex write
  writerque.emplace_back(&w);
  while (!w.done && &w != writerque.front()) {
    w.wait();
  }
  if (w.done) {
    return w.state;
  }
  // 写入前的各种检查为写入的数据留出足够的buffer,
  // MakeRoomForWrite方法会根据当前MemTable的使用率来选择是否触发Minor
  // Compaction

  State statue = MakeRoomForwrite(updates == nullptr);  // is true,start
  // merage
  uint64_t last_sequence = versions_->LastSequence();
  Writer* now_writer = &w;
  if (statue.ok() && updates != nullptr) {
    WriteBatch* upt = BuildBatchGroup(&now_writer);
    updates->SetSequence(last_sequence + 1);
    last_sequence += updates->Count();

    //将数据写入到yubindb
    mutex.unlock();
    statue = logwrite->Appendrecord(updates->Contents());  // add wallog
    bool sync_error = false;
    if (statue.ok() && opt.sync) {  // flush
      statue = logfile->Sync();
      if (!statue.ok()) {
        sync_error = true;
      }
    }
    if (statue.ok()) {
      statue = upt->InsertInto(mem_);
    }
    mutex.lock();
    if (sync_error) {
      spdlog::error("logfile sync error int {}", logfile->Name());
    }
  }
  versions_->SetLastSequence(last_sequence);
  while (true) {
    Writer* ready = writerque.front();
    writerque.pop_front();
    if (ready != &w) {
      ready->state = statue;
      ready->done = true;
      ready->signal();
    }
    if (ready == now_writer) break;
  }  //直到last_writer通知为止。
  return State::Ok();
}
State DBImpl::Get(const ReadOptions& options, const std::string_view& key,
                  std::string* value) {
  State s;
  std::unique_lock<std::mutex> rlock(mutex);
  SequenceNum snapshot;
  if (options.snapshot != nullptr) {
    snapshot = options.snapshot->sequence();
  } else {
    snapshot = versions_->LastSequence();
  }
  // 增加引用计数，避免在读取过程中被后台线程进行 Compaction 时“垃圾回收”了。
  std::shared_ptr<Memtable> mem = mem_;
  std::shared_ptr<Memtable> imm = imm_;
  std::shared_ptr<Version> current = versions_->Current();

  bool have_stat_update = false;
  Version::Stats stats;
  {
    rlock.unlock();
    const Lookey lk(key, snapshot);
    if (mem->Get(lk, value, &s)) {
      // done
    } else if (imm != nullptr && imm->Get(lk, value, &s)) {
      // done
    } else {
      // s = current->Get(options, lk, value, &s); jntm
      have_stat_update = true;
    }
    rlock.lock();
  }
  if (have_stat_update) {  //&& current->UpdateStats(stats)) {  jntm
    MaybeCompaction();
  }
  return s;
}

WriteBatch* DBImpl::BuildBatchGroup(DBImpl::Writer** last_writer) {
  Writer* fnt = writerque.front();
  WriteBatch* fntbatch = fnt->batch;
  uint64_t size = fntbatch->ByteSize();
  for (auto it = ++writerque.begin(); it != writerque.end(); it++) {
    if ((*it)->sync != fnt->sync) {
      last_writer = &(*it);
      break;
    }
    if (size + (*it)->batch->ByteSize() > MaxBatchSize) {
      last_writer = &(*it);
      break;
    }
    fntbatch->Append((*it)->batch);
  }
  return fntbatch;
}
State DBImpl::MakeRoomForwrite(bool force) {
  bool allow_delay = !force;
  State s;
  std::unique_lock<std::mutex> lks(mutex);
  while (true) {
    if (!bg_error.ok()) {
      s = bg_error;
      break;
    } else if (allow_delay &&
               versions_->NumLevelFiles(0) >= config::kL0_SlowdownWrites) {
      // level0的文件数限制超过8,睡眠1ms,等待后台任务执行。写入writer线程向压缩线程转让cpu
      // Todobetter?
      mutex.unlock();
      ::sleep(1000);
      allow_delay = false;  // sleep too large
      lks.unlock();
    } else if (!force &&
               (mem_->ApproximateMemoryUsage() <= opts->write_buffer_size)) {
      break;
    } else if (versions_->NumLevelFiles(0) >= config::kL0_StopWrites) {
      spdlog::warn("Too many L0 files; waiting...");
      background_work_finished_signal.wait(lks);
    } else if (imm_ != nullptr) {
      spdlog::info("Current memtable full; waiting...");
      background_work_finished_signal.wait(lks);
    } else {
      //到新的memtable并触发旧的进行compaction
      uint64_t new_log_number = versions_->NewFileNumber();
      std::unique_ptr<WritableFile> lfile = nullptr;
      s = env->NewWritableFile(LogFileName(dbname, new_log_number),
                               lfile);  //生成新的log文件
      if (!s.ok()) {
        // Avoid chewing through file number space in a tight loop.
        // versions_->ReuseFileNumber(new_log_number); TODO
        break;
      }
      logfile.reset(lfile.release());
      logwrite = std::make_unique<walWriter>(logfile);
      logfilenum = new_log_number;
      imm_ = mem_;
      has_imm_.store(true, std::memory_order_release);
      mem_ = std::make_shared<Memtable>();
      force = false;
      MaybeCompaction();
    }
  }
  return State::Ok();
}
void DBImpl::MaybeCompaction() {
  if (background_compaction_) {
    // Already scheduled
    spdlog::debug("background is work");
  } else if (shutting_down_.load(std::memory_order_acquire)) {
  } else if (!bg_error.ok()) {
  } else if (imm_ == nullptr && !versions_->NeedsCompaction()) {
    // No work to be done
  } else {
    background_compaction_ = true;
    env->Schedule(std::bind(&DBImpl::BackgroundCall, this));
    spdlog::info("dbimpl start schedule backgroundCall");
  }
}
void DBImpl::DeleteObsoleteFiles() {  // delete outtime file
  if (!bg_error.ok()) {
    return;
  }
  const uint64_t log_number = versions_->LogNumber();
  const uint64_t manifest_file_number = versions_->ManifestFileNumber();
  std::set<uint64_t> live = pending_file;
  versions_->AddLiveFiles(&live);
  std::vector<std::string> filenames;
  env->GetChildren(dbname, &filenames);

  mutex.unlock();
  uint64_t num;
  FileType type;
  for (uint32_t i = 0; i < filenames.size(); i++) {
    // TODO 删除过时的文件，思考添加kv分离的gc
    if (ParsefileName(filenames[i], &num, &type)) {
      bool keep = true;
      switch (type) {
        case kCurrentFile:
        case kDBLockFile:
        case kInfoLogFile:
          keep = true;
          break;
        case kDescriptorFile:
          keep = (num >= manifest_file_number);
          break;
        case kLogFile:
          keep = (num >= log_number);
          break;
        case kTableFile:
        case kTempFile:
          keep = (live.find(num) != live.end());
          break;
      }
      if (!keep) {
        if (type == kTableFile) {
          table_cache->Evict(num);  // clear deletedfile chache
        }
        spdlog::info("Delete type={} #{}", type, num);
        env->DeleteFile(dbname + "/" + filenames[i]);
      }
    }
  }
  mutex.lock();
}
std::shared_ptr<const Snapshot> DBImpl::GetSnapshot() {
  std::lock_guard<std::mutex> lk(mutex);
  std::shared_ptr<const Snapshot> snapshot_ =
      std::make_shared<const Snapshot>(versions_->LastSequence());
  snapshots.emplace_back(snapshot_);
  return snapshot_;
}
void DBImpl::ReleaseSnapshot(std::shared_ptr<const Snapshot>& snapshot) {
  std::lock_guard<std::mutex> lk(mutex);
  snapshots.remove(snapshot);
}
void DBImpl::BackgroundCall() {
  std::unique_lock<std::mutex> lk(mutex);
  if (shutting_down_.load(std::memory_order_acquire)) {
  } else if (!bg_error.ok()) {
  } else {
    BackgroundCompaction();
  }

  background_compaction_ = false;

  // Previous compaction may have produced too many files in a level,
  // so reschedule another compaction if needed.
  MaybeCompaction();
  background_work_finished_signal.notify_all();
}
void DBImpl::BackgroundCompaction() {  // doing compaction
  if (imm_ != nullptr) {
    CompactMemTable();
    return;
  }
  std::unique_ptr<Compaction> c;
  c = versions_->PickCompaction();
}
void DBImpl::CompactMemTable() {
  assert(imm != nullptr);
  VersionEdit edit;
  std::shared_ptr<Version> base = versions_->Current();
  State s = WriteLevel0Table(imm_, edit, base);

  if (s.ok() && shutting_down_.load(std::memory_order_acquire)) {
    spdlog::info("Deleting DB during memtable compaction");
    s = State::IoError();
  }
  if (s.ok()) {
    edit.SetLogNumber(logfilenum);
    s = versions_->LogAndApply(&edit, &mutex);  // updat version
  }

  if (s.ok()) {
    imm_ = nullptr;
    has_imm_.store(false, std::memory_order_release);
    DeleteObsoleteFiles();
  } else {
    spdlog::error("CompactMemTable is error in {}", logfilenum);
  }
}
State DBImpl::WriteLevel0Table(std::shared_ptr<Memtable>& mem,
                               VersionEdit& edit,
                               std::shared_ptr<Version>& base) {
  FileMate meta;
  meta.num = versions_->NewFileNumber();
  pending_outputs_.insert(meta.num);
  spdlog::info("Level-0 table filenumber{}: started", meta.num);
  State s;
  {
    mutex.unlock();
    s = BuildTable(mem, meta);
    mutex.lock();
  }
  spdlog::info("Level 0 table {} byytes {}", meta.num, meta.file_size);
  pending_outputs_.erase(meta.num);
  int level = 0;
  if (s.ok() && meta.file_size > 0) {
    edit.AddFile(0, meta.num, meta.file_size, meta.smallest, meta.largest);
  }
  return s;
}
State DBImpl::BuildTable(std::shared_ptr<Memtable>& mem, FileMate& meta) {
  State s;
  meta.file_size = 0;
  std::string fname = TableFileName(dbname, meta.num);
  std::shared_ptr<WritableFile> file;
  s = env->NewWritableFile(fname, file);
  if (!s.ok()) {
    return s;
  }
  std::unique_ptr<Tablebuilder> builder =
      std::make_unique<Tablebuilder>(opts, file);
  s = mem->Flushlevel0fromskip(meta, builder);
  if (s.ok()) {
    meta.file_size = builder->Size();
  }
  if (s.ok()) {
    s = file->Sync();
  }
  if (s.ok()) {
    s = file->Close();
  }
  if (s.ok() && meta.file_size > 0) {
  } else {
    env->DeleteFile(fname);
  }
  return s;
}
}  // namespace yubindb
