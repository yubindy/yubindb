#ifndef YUBINDB_VERSION_SET_H_
#define YUBINDB_VERSION_SET_H_
#include <memory.h>

#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "../util/cache.h"
#include "../util/env.h"
#include "../util/iterator.h"
#include "../util/key.h"
#include "../util/options.h"
#include "src/util/common.h"
#include "version_edit.h"
#include "walog.h"
namespace yubindb {
class VersionSet;
class Compaction;
static int64_t MaxGrandParentOverlapBytes(const Options* options) {
  return 10 * options->max_file_size;
}
int FindFile(const std::vector<std::shared_ptr<FileMate>>& files,
             std::string_view key);
enum SaverState {
  kNotFound,
  kFound,
  kDeleted,
  kCorrupt,
};
struct Saver {
  SaverState state;
  std::string_view user_key;
  std::string* value;
};
static void SaveValue(void* arg, std::string_view ikey, std::string_view v) {
  Saver* s = reinterpret_cast<Saver*>(arg);
  ParsedInternalKey parsed_key;
  if (!ParseInternalKey(ikey, &parsed_key)) {
    s->state = kCorrupt;  // data error
  } else {
    if (cmp(parsed_key.user_key, s->user_key) == 0) {
      s->state = (parsed_key.type == kTypeValue) ? kFound : kDeleted;
      if (s->state == kFound) {
        s->value->assign(v.data(), v.size());
      }
    }
  }
}
class Version {
 public:
  struct GetStats {
    std::shared_ptr<FileMate> seek_file;
    int seek_file_level;
  };
  explicit Version(VersionSet* vset)
      : vset(vset), compaction_score(-1), compaction_level(-1) {}
  ~Version() = default;
  State Get(const ReadOptions& op, const Lookey& key, std::string* val,
            GetStats* stats);
  void GetOverlappFiles(int level, const InternalKey* begin,
                        const InternalKey* end,
                        std::vector<std::shared_ptr<FileMate>>* inputs);

 private:
  friend VersionSet;
  friend class Compaction;
  VersionSet* vset;
  std::vector<std::shared_ptr<FileMate>>
      files[kNumLevels];  // 每个级别的文件列表

  // 用于size_compation
  double compaction_score;
  int compaction_level;
};
class LevelFileNumIterator : public Iterator {
 public:
  LevelFileNumIterator(const std::vector<std::shared_ptr<FileMate>>* flist)
      : flist_(flist), index_(flist->size()) {}
  bool Valid() const override { return index_ < flist_->size(); }
  void Seek(std::string_view target) override {
    index_ = FindFile(*flist_, target);
  }
  void SeekToFirst() override { index_ = 0; }
  void SeekToLast() override {
    index_ = flist_->empty() ? 0 : flist_->size() - 1;
  }
  void Next() override {
    assert(Valid());
    index_++;
  }
  void Prev() override {
    assert(Valid());
    if (index_ == 0) {
      index_ = flist_->size();  // Marks as invalid
    } else {
      index_--;
    }
  }
  std::string_view key() const override {
    assert(Valid());
    return (*flist_)[index_]->largest.getview();
  }
  std::string_view value() const override {
    assert(Valid());
    EncodeFixed64(value_buf_, (*flist_)[index_]->num);
    EncodeFixed64(value_buf_ + 8, (*flist_)[index_]->file_size);
    return std::string_view(value_buf_, sizeof(value_buf_));
  }
  State state() override { return State::Ok(); }

 private:
  const std::vector<std::shared_ptr<FileMate>>* const flist_;
  uint32_t index_;

  // Holds the file number and size.
  mutable char value_buf_[16];
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
  std::shared_ptr<Iterator> MakeInputIterator(Compaction* c);

 private:
  class Builder;

  friend class Compaction;
  friend class Version;

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
  bool IsTrivialMove();
  std::shared_ptr<FileMate>& Input(int n, int m) { return inputs_[n][m]; }
  int Inputsize(int n) { return inputs_[n].size(); }
  std::vector<std::shared_ptr<FileMate>> Input(int n) { return inputs_[n]; }
  VersionEdit* Edit() { return &edit_; }
  int Level() { return level_; }
  uint64_t MaxOutputFileSize() const { return max_output_file_size_; }
  void AddInputDeletions(VersionEdit* edit);  // for input 0 删除文件
  bool IsBaseLevelForKey(std::string_view user_key);
  bool ShouldStopBefore(std::string_view internal_key);
  void ReleaseInputs() { input_version_ = nullptr; }

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