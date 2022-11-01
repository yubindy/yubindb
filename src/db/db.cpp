#include "db.h"

#include "spdlog/spdlog.h"

class SnapshotImpl;
class Version;
class VersionSet;

namespace yubindb {
State DBImpl::Open(const Options& options, std::string_view name, DB** dbptr) {
  return State();
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
  uint64_t last_sequence = versions_->LastSqruence();
  std::shared_ptr<Writer>* now_writer = &w;
  if (statue.ok() && updates != nullptr) {
    WriteBatch* upt = BuildBatchGroup(now_writer);
    updates->SetSequence(last_sequence + 1);
    last_sequence = updates->Count();

    //将数据写入到LevelDB
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
                  std::string* value) {
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
      s = current->Get(options, lk, value, &s);
      have_stat_update = true;
    }
    rlock.lock();
  }
  if (have_stat_update && current->UpdateStats(stats)) {
    MaybeCompaction();  // doing compaction ?
  }
  return s;
}
WriteBatch* DBImpl::BuildBatchGroup(std::shared_ptr<Writer>* last_writer) {
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
}  // namespace yubindb