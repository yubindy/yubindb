#include "db.h"

namespace yubindb {
State DBImpl::Open(const Options& options, std::string_view name, DB** dbptr) {
  return State();
}
State DBImpl::Put(const WriteOptions& opt, std::string_view key,
                  std::string_view value) {
  WriteBatch batch;
  batch.Put(key, value);
  return Write(opt, batch);
}
State DBImpl::Delete(const WriteOptions& opt, std::string_view key) {
  WriteBatch batch;
  batch.Delete(key);
  return Write(opt, batch);
}
State DBImpl::Write(const WriteOptions& opt, WriteBatch* updates) {
  Writer w(&mutex, opt.sync, false, updates);
  
  std::unique_lock<Mutex> ptr(mutex);
  writerque.emplace_back(&w);
  while(!w.done&& w!=writerque.front()){
    w.wait();
  }
  if(w.done){
    return w.state;
  }
  
}
}  // namespace yubindb