#ifndef YUBINDB_VERSION_SET_H_
#define YUBINDB_VERSION_SET_H_
#include <memory.h>
#include <string>
#include <vector>

#include "../util/cache.h"
#include "../util/env.h"
#include "../util/key.h"
#include "../util/options.h"
#include "version_edit.h"
namespace yubindb {
class VersionSet;
class Version {
 public:
  struct Stats {
    std::unique_ptr<FileMate> seek_file;
    int seek_file_level;
  };
  Version() = default;
  ~Version() = default;

 private:
  friend VersionSet;
  std::vector<std::unique_ptr<FileMate>>
      files[kNumLevels];  // 每个级别的文件列表
};
class VersionSet {
 public:
  VersionSet(const std::string& dbname_, const Options* options,
             TableCache* table_cache);
  ~VersionSet() = default;
  State Recover(bool save_manifest);
  State LogAndApply(VersionEdit* edit, std::mutex* mu);
  std::shared_ptr<Version> Current() { return nowversion; }
  uint64_t MainifsetFileNum() { return manifest_file_number; }
  SequenceNum LastSequence() { return last_sequence; }
  void SetLastSequence(SequenceNum seq) { last_sequence = seq; }

  uint64_t NewFileNumber();
  int NumLevelFiles(int level) const;
  int64_t NumLevelBytes(int level) const;

 private:
  PosixEnv* env_;
  const std::string dbname;
  const Options* ops;
  uint64_t next_file_number;
  uint64_t manifest_file_number;
  uint64_t last_sequence;
  uint64_t log_number;

  std::shared_ptr<Version> nowversion;
  std::string
      compact_pointer[kNumLevels];  //记录每个层级下次compation启动的key。
};
}  // namespace yubindb
#endif