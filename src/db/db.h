#ifndef YUBINDB_DB_H_
#define YUBINDB_DB_H_
#include <memory.h>
#include <stdio.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>

#include "cache.h"
#include "memtable.h"
#include "src/util/common.h"
#include "src/util/env.h"
#include "src/util/options.h"
#include "version_set.h"
#include "walog.h"
#include "writebatch.h"

namespace yubindb {

struct ReadOptions;
struct WriteOptions;
struct Writer;
class VersionSet;

const static uint32_t MaxBatchSize = 1024;
class DB {
 public:
  explicit DB() {}
  virtual ~DB(){};
  virtual State Put(const WriteOptions& options, std::string_view key,
                    std::string_view value) = 0;
  virtual State Get(const ReadOptions& options, const std::string_view& key,
                    std::string* value) = 0;
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
  State Get(const ReadOptions& options, const std::string_view& key,
            std::string* value);
  State InsertInto(WriteBatch* batch);

 private:
  struct Writer {
    explicit Writer(std::mutex* mutex_, bool sync_, bool done_,
                    WriteBatch* updates)
        : mutex(mutex_), sync(sync_), done(done_), batch(updates) {}
    void wait() {
      std::unique_lock<std::mutex> p(*mutex);
      cond.wait(p);
    }
    void signal() { cond.notify_one(); }

    std::condition_variable cond;
    std::mutex* mutex;  /// safe
    bool done;
    bool sync;
    State state;
    WriteBatch* batch;
  };
  struct CompactionState {
   public:
    explicit CompactionState(Compaction* c)
        : comp(c),
          small_snap(0),
          outfile(nullptr),
          builder(nullptr),
          total_bytes(0) {}
    ~CompactionState() { delete builder; }
    struct Output {
      uint64_t number;
      uint64_t file_size;
      InternalKey smallest, largest;
    };
    Output* current_output() { return &oupts[oupts.size() - 1]; }
    Compaction* comp;
    SequenceNum small_snap;
    std::vector<Output> oupts;
    std::shared_ptr<WritableFile> outfile;
    Tablebuilder* builder;

    uint64_t total_bytes;
  };
  State NewDB();
  WriteBatch* BuildBatch(Writer** rul);
  WriteBatch* BuildBatchGroup(DBImpl::Writer** last_writer);
  State Recover(VersionEdit* edit, bool* save_manifest);
  State MakeRoomForwrite(bool force);
  void DeleteObsoleteFiles();
  void MaybeCompaction();
  std::shared_ptr<const Snapshot> GetSnapshot();
  void ReleaseSnapshot(std::shared_ptr<const Snapshot>& snapshot);
  void BackgroundCall();
  void BackgroundCompaction();
  void CompactMemTable();
  State WriteLevel0Table(std::shared_ptr<Memtable>& mem, VersionEdit& edit,
                         std::shared_ptr<Version>& base);
  State BuildTable(std::shared_ptr<Memtable>& mem, FileMate& meta);
  State DoCompactionWork(std::unique_ptr<CompactionState>& compact);
  State FinishCompactionOutputFile(CompactionState* compact,
                                   std::shared_ptr<Iterator>& input);
  State OpenCompactionOutputFile(CompactionState* compact);
  State InstallCompactionResults(std::unique_ptr<CompactionState>& compact);
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
  std::deque<Writer*> writerque;
  std::shared_ptr<WriteBatch> batch;

  std::list<std::shared_ptr<const Snapshot>> snapshots;
  std::shared_ptr<TableCache> table_cache;
  std::set<uint64_t> pending_file;

  std::condition_variable background_work_finished_signal;
  std::set<uint64_t> pending_outputs_;
  std::atomic<bool> shutting_down_;
  bool background_compaction_;
  std::shared_ptr<PosixEnv> env;
  std::unique_ptr<VersionSet> versions_;
  State bg_error;
  // State stats_[kNumLevels];
};
}  // namespace yubindb
#endif
