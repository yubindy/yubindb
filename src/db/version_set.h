#ifndef YUBINDB_VERSION_SET_H_
#define YUBINDB_VERSION_SET_H_
#include <memory.h>

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "../util/cache.h"
#include "../util/env.h"
#include "../util/key.h"
#include "../util/options.h"
#include "version_edit.h"
#include "walog.h"
namespace yubindb {
class VersionSet;
class Compaction;
class Version {
 public:
  struct Stats {
    std::shared_ptr<FileMate> seek_file;
    int seek_file_level;
  };
  explicit Version(VersionSet* vset)
      : vset(vset), compaction_score(-1), compaction_level(-1) {}
  ~Version() = default;
  void GetOverlappFiles(int level, const InternalKey* begin,
                        const InternalKey* end,
                        std::vector<std::shared_ptr<FileMate>>* inputs);

 private:
  friend VersionSet;
  VersionSet* vset;
  std::vector<std::shared_ptr<FileMate>>
      files[kNumLevels];  // 每个级别的文件列表

  // 用于size_compation
  double compaction_score;
  int compaction_level;
};
class VersionSet {
 public:
  VersionSet(const std::string& dbname_, const Options* options,
             std::shared_ptr<TableCache>& table_cache_,
             std::shared_ptr<PosixEnv>& env);
  ~VersionSet() = default;
  State Recover(bool* save_manifest);
  State LogAndApply(VersionEdit* edit, std::mutex* mu);
  void Finalize(std::unique_ptr<Version>& v);
  bool NeedsCompaction();
  std::shared_ptr<Version> Current() { return nowversion; }
  SequenceNum LastSequence() const { return last_sequence; }
  void SetLastSequence(SequenceNum seq) { last_sequence = seq; }
  uint64_t NewFileNumber() {
    { return next_file_number++; }
  }
  State WriteSnapshot(std::unique_ptr<walWriter>& log);
  int NumLevelFiles(int level) const { return nowversion->files[level].size(); }
  int64_t NumLevelBytes(int level) const;
  void AddLiveFiles(std::set<uint64_t>* live);
  uint64_t LogNumber() { return log_number; }
  uint64_t ManifestFileNumber() { return manifest_file_number; }
  std::unique_ptr<Compaction> PickCompaction();
  void GetRange(const std::vector<std::shared_ptr<FileMate>>& inputs,
                InternalKey* smallest, InternalKey* largest);
  void GetRange2(const std::vector<std::shared_ptr<FileMate>>& inputs1,
                 const std::vector<std::shared_ptr<FileMate>>& inputs2,
                 InternalKey* smallest, InternalKey* largest);
  void SetupOtherInputs(std::unique_ptr<Compaction>& cop);

 private:
  class Builder;
  std::shared_ptr<PosixEnv> env_;
  const std::string dbname;
  const Options* ops;
  std::shared_ptr<TableCache> table_cache;
  uint64_t next_file_number;
  uint64_t manifest_file_number;
  uint64_t last_sequence;
  uint64_t log_number;

  // uint64_t prev_log_number;
  //  Opened lazily  about write to mainifset
  std::shared_ptr<WritableFile> descriptor_file;
  std::unique_ptr<walWriter> descriptor_log;
  std::list<std::shared_ptr<Version>> versionlist;  // ersion构成的双向链表
  std::shared_ptr<Version> nowversion;  //链表头指向当前最新的Version
  std::string compact_pointer[kNumLevels];  //记录每个层级下次compation启动的key
};
class Compaction {
 public:
  Compaction(const Options* options, int level);
  ~Compaction();

 private:
  friend class Version;
  friend class VersionSet;
  int level_;
  uint64_t max_output_file_size_;
  std::shared_ptr<Version> input_version_;
  VersionEdit edit_;
  std::vector<std::shared_ptr<FileMate>> inputs_[2];

  std::vector<std::shared_ptr<FileMate>> grandparents_;  // level+2
                                                         // 的overlop情况
  size_t grand_index;
  bool seen_key;
  int64_t overlapbytes;

  size_t level_ptrs[config::kNumLevels];
};
}  // namespace yubindb
#endif