#ifndef YUBINDB_CACHE_H_
#define YUBINDB_CACHE_H_
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdio.h>
#include <string_view>

#include "../util/common.h"
#include "../util/options.h"
#include "writebatch.h"

namespace yubindb {

class State;
struct Options;
struct ReadOptions;
struct WriteOptions;
struct Writer;

class DB {
 public:
  explicit DB() {}
  virtual ~DB() = 0;
  virtual State Open(const Options& options, std::string_view name,
                     DB** dbptr) = 0;
  virtual State Put(const WriteOptions& options, std::string_view key,
                    std::string_view value) = 0;
  virtual State Delete(const WriteOptions& options, std::string_view key) = 0;
  virtual State Write(const WriteOptions& options, WriteBatch* updates) = 0;

 private:
};
class DBImpl : public DB {
 public:
  explicit DBImpl(const Options opt, const std::string dbname);
  DBImpl(const DBImpl&) = delete;
  DBImpl& operator=(const DBImpl&) = delete;
  ~DBImpl();
  State Open(const Options& options, std::string_view name,
             DB** dbptr) override;
  State Put(const WriteOptions& options, std::string_view key,
            std::string_view value) override;
  State Delete(const WriteOptions& options, std::string_view key) override;
  State Write(const WriteOptions& options, WriteBatch* updates) override;

 private:
  struct Writer {
    explicit Writer(std::mutex* mutex_, bool sync_, bool done_,
                    WriteBatch* updates)
        : mutex(mutex_),
          sync(sync_),
          done(done_),
          state(state_),
          batch(updates) {}
    std::condition_variable cond;
    std::mutex* mutex;
    bool done;
    bool sync;
    State state;
    WriteBatch* batch;
  };
  std::mutex mutex;
  std::atomic<bool> shutting_down;
  std::deque<Writer*> writerque;
};
}  // namespace yubindb

#endif