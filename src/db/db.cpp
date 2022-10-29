#include "db.h"

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
  Writer w(&mutex, opt.sync, false, updates);

  std::unique_lock<std::mutex> ptr(mutex);
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
  State statue = MakeRoomForwrite(updates == nullptr);
  uint64_t last_sequence = version_->LastSqruence();
  Writer* now_writer = &w;
  if (State.ok() && update != nullptr) {
    WriteBatch* updates = BuildBatchGroup(&now_writer);
    updates->SetSequence(last_sequence + 1);
    last_sequence = updates->Count();

    //将数据写入到LevelDB
    mutex.unlock();
    State = logwrite->Appendrecord(updates->Contents());
  }
}
State DBImpl::MakeRoomForwrite(bool force) {}
}  // namespace yubindb