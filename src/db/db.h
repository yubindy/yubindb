#ifndef YUBINDB_DB_H_
#define YUBINDB_DB_H_
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory.h>
#include <mutex>
#include <set>
#include <stdio.h>
#include <string>
#include <string_view>

#include "../util/cache.h"
#include "../util/common.h"
#include "../util/env.h"
#include "../util/options.h"
#include "memtable.h"
#include "version_set.h"
#include "walog.h"
#include "writebatch.h"

namespace yubindb {

struct ReadOptions;
struct WriteOptions;
struct Writer;
class VersionSet;
class SnapshotList;

const static size_t MaxBatchSize = 1024;
class DB {
 public:
  explicit DB() {}
  virtual ~DB() = 0;
  virtual State Put(const WriteOptions& options, std::string_view key,
                    std::string_view value) = 0;
  virtual State Delete(const WriteOptions& options, std::string_view key) = 0;
  virtual State Write(const WriteOptions& options, WriteBatch* updates) = 0;
};
class DBImpl : public DB {
 public:
  explicit DBImpl(const Options* opt, const std::string& dbname);
  DBImpl(const DBImpl&) = delete;
  DBImpl& operator=(const DBImpl&) = delete;
  ~DBImpl();
  static State Open(const Options& options, std::string name, DB** dbptr);
  State Put(const WriteOptions& options, std::string_view key,
            std::string_view value) override;
  State Delete(const WriteOptions& options, std::string_view key) override;
  State Write(const WriteOptions& options, WriteBatch* updates) override;
  State Get(const ReadOptions& options, std::string_view key,
            std::string_view* value);
  State InsertInto(WriteBatch* batch, Memtable* mem);
  const Snapshot* GetSnapshot();

 private:
  struct Writer {
    explicit Writer(std::mutex* mutex_, bool sync_, bool done_,
                    WriteBatch* updates)
        : mutex(mutex_), sync(sync_), done(done_), batch(updates) {}
    ~Writer() {
      delete[] batch;
      mutex->unlock();
    }
    void wait() {
      std::unique_lock<std::mutex> p(*mutex);
      cond.wait(p);
    }
    void signal() { cond.notify_one(); }

    std::condition_variable cond;
    std::mutex* mutex;
    bool done;
    bool sync;
    State state;
    WriteBatch* batch;
  };
  State NewDB();
  WriteBatch* BuildBatch(Writer** rul);
  WriteBatch* BuildBatchGroup(std::shared_ptr<DBImpl::Writer>* last_writer);
  State MakeRoomForwrite(bool force);
  void ReleaseSnapshot(const Snapshot* snapshot);
  State Recover(VersionEdit* edit, bool* save_manifest);
  void DeleteObsoleteFiles();
  void MaybeCompaction();
  void BackgroundCall();
  void BackgroundCompaction();

  const std::string dbname;
  const Options* opts;
  std::unique_ptr<FileLock> db_lock;

  std::mutex mutex;
  std::shared_ptr<Memtable> mem_;  // now memtable
  std::shared_ptr<Memtable> imm_;  // imemtable
  std::atomic<bool> has_imm_;

  uint64_t logfilenum;
  std::shared_ptr<WritableFile> logfile;
  std::unique_ptr<walWriter> logwrite;
  std::deque<std::shared_ptr<Writer>> writerque;
  std::unique_ptr<WriteBatch> batch;

  std::shared_ptr<TableCache> table_cache;

  std::condition_variable background_work_finished_signal;
  std::set<uint64_t> pending_outputs_;
  std::atomic<bool> shutting_down_;
  bool background_compaction_;
  std::unique_ptr<VersionSet> versions_;
  State bg_error;
  State stats_[kNumLevels];
  std::shared_ptr<PosixEnv> env;
};
}  // namespace yubindb
#endif
