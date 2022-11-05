#ifndef YUBINDB_VERSION_SET_H_
#define YUBINDB_VERSION_SET_H_
#include <memory.h>

#include "../util/key.h"
#include "version_edit.h"
namespace yubindb {
class Version {
 public:
  struct Stats {
    std::unique_ptr<FileMate> seek_file;
    int seek_file_level;
  };
  Version() = default;
  ~Version() = default;

 private:
};
class VersionSet {
 public:
  VersionSet() = default;
  ~VersionSet() = default;
  std::shared_ptr<Version> Current() { return nowversion; }
  SequenceNum LastSequence() { return last_sequence; }
  void SetLastSequence(SequenceNum seq) { last_sequence = seq; }

 private:
  std::shared_ptr<Version> nowversion;
  uint64_t last_sequence;
};
}  // namespace yubindb
#endif