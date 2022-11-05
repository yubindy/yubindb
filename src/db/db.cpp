#include "db.h"

#include "../util/filename.h"
#include "spdlog/spdlog.h"
#include "version_edit.h"
class SnapshotImpl;
class Version;
class VersionSet;

namespace yubindb {
//该方法会检查Lock文件是否被占用（LevelDB通过名为LOCK的文件避免多个LevelDB进程同时访问一个数据库）、
//目录是否存在、Current文件是否存在等。然后主要通过VersionSet::Recover与DBImpl::RecoverLogFile
//两个方法，分别恢复其VersionSet的状态与MemTable的状态。
State DBImpl::Recover(VersionEdit* edit, bool* save_manifest) {
  env->CreateDir(dbname);
  State s = env->LockFile(LockFileName(dbname), db_lock);
}
State DBImpl::Open(const Options& options, std::string name, DB** dbptr) {
  *dbptr = nullptr;

  DBImpl* impl = new DBImpl(options, name);
  impl->mutex.lock();
  VersionEdit edit;
  // Recover handles create_if_missing, error_if_exists
  bool save_manifest = false;
  State s = impl->Recover(&edit, &save_manifest);  //恢复自身状态。
  if (s.ok() && impl->mem_ == nullptr) {
    // Create new log and a corresponding memtable.
    uint64_t new_log_number = impl->versions_->NewFileNumber();
    WritableFile* lfile;
    s = impl->env->NewWritableFile(LogFileName(name, new_log_number), &lfile);
    if (s.ok()) {
      edit.SetLogNumber(new_log_number);
      impl->logfile = lfile;
      impl->logfile_number_ = new_log_number;
      impl->log_ = new log::Writer(lfile);
      impl->mem_ = new MemTable(impl->internal_comparator_);
      impl->mem_->Ref();
    }
  }
  if (s.ok() && save_manifest) {
    edit.SetPrevLogNumber(0);  // No older logs needed after recovery.
    edit.SetLogNumber(impl->logfile_number_);
    s = impl->versions_->LogAndApply(&edit, &impl->mutex_);
  }
  if (s.ok()) {
    impl->DeleteObsoleteFiles();
    impl->MaybeScheduleCompaction();
  }
  impl->mutex_.Unlock();
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
  std::shared_ptr<Writer> w =
      std::make_shared<Writer>(&mutex, opt.sync, false, updates);

  std::unique_lock<std::mutex> ptr(mutex);  // mutex write
  writerque.emplace_back(w);
  while (!w->done && w != writerque.front()) {
    w->wait();
  }
  if (w->done) {
    return w->state;
  }
  // 写入前的各种检查为写入的数据留出足够的buffer,
  // MakeRoomForWrite方法会根据当前MemTable的使用率来选择是否触发Minor
  // Compaction
  State statue = MakeRoomForwrite(updates == nullptr);  // is true,start merage
  uint64_t last_sequence = versions_->LastSequence();
  std::shared_ptr<Writer>* now_writer = &w;
  if (statue.ok() && updates != nullptr) {
    WriteBatch* upt = BuildBatchGroup(now_writer);
    updates->SetSequence(last_sequence + 1);
    last_sequence = updates->Count();

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
      statue = InsertInto(upt, mem_.get());
    }
    mutex.lock();
    if (sync_error) {
      spdlog::error("logfile sync error int {}", logfile->Name());
    }
  }
  versions_->SetLastSequence(last_sequence);
  while (true) {
    std::shared_ptr<Writer> ready = writerque.front();
    writerque.pop_front();
    if (ready.get() != w.get()) {
      ready->state = statue;
      ready->done = true;
      ready->signal();
    }
    if (ready == *now_writer) break;
  }  //直到last_writer通知为止。
  return State::Ok();
}
State DBImpl::Get(const ReadOptions& options, std::string_view key,
                  std::string_view* value) {
  State s;
  std::unique_lock<std::mutex> rlock(mutex);
  SequenceNum snapshot;
  if (options.snapshot != nullptr) {
    snapshot = static_cast<const SnapshotImpl*>(options.snapshot)->sequence();
  } else {
    snapshot = versions_->LastSequence();
  }
  std::shared_ptr<Memtable> mem = mem_;
  std::shared_ptr<Memtable> imm = imm_;
  std::shared_ptr<Version> current = versions_->Current();

  bool have_stat_update = false;
  Version::Stats stats;
  {
    rlock.unlock();
    Lookey lk(key, snapshot);
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
WriteBatch* DBImpl::BuildBatchGroup(
    std::shared_ptr<DBImpl::Writer>* last_writer) {
  std::shared_ptr<Writer> fnt = writerque.front();
  WriteBatch* fntbatch = fnt->batch;
  uint64_t size = fntbatch->ByteSize();
  for (auto it = ++writerque.begin(); it != writerque.end(); it++) {
    if (it->get()->sync != fnt->sync) {
      last_writer = &(*it);
      break;
    }
    if (size + it->get()->batch->ByteSize() > MaxBatchSize) {
      last_writer = &(*it);
      break;
    }
    fntbatch->Append(*it->get()->batch);
  }
  return fntbatch;
}
State DBImpl::MakeRoomForwrite(bool force) {
  // TODO
}
void DBImpl::MaybeCompaction() {}
}  // namespace yubindb