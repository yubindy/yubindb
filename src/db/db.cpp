#include "db.h"

#include <memory.h>

#include "../util/filename.h"
#include "snapshot.h"
#include "spdlog/spdlog.h"
#include "src/util/common.h"
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
    : env(new PosixEnv),
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
      versions_(std::make_unique<VersionSet>(dbname, opt, table_cache)) {}
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

  // sumState statue = MakeRoomForwrite(updates == nullptr);  // is true,start
  // merage
  State statue;
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
    MaybeCompaction();     // TODO doing compaction ?
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
  // TODO
  bool allow_delay = !force;
  State s;
  while (true) {
    if (!bg_error.ok()) {
      s = bg_error;
      break;
    } else if (allow_delay &&
               versions_->NumLevelFiles(0) >= config::kL0_SlowdownWrites) {
      // level0的文件数限制超过8,睡眠1ms,等待后台任务执行。写入writer线程向压缩线程转让cpu Todobetter?
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
  std::set<uint64_t> live = pending_file;
  versions_->AddLiveFiles(&live);
  std::vector<std::string> filenames;
  env->GetChildren(dbname, &filenames);

  mutex.unlock();
  for (uint32_t i = 0; i < filenames.size(); i++) {
    // TODO 删除过时的文件，思考添加kv分离的gc
  }
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
void DBImpl::BackgroundCompaction() {  // TODO doing compaction
  if (imm_ != nullptr) {
    // CompactMemTable();
    return;
  }
}
}  // namespace yubindb
