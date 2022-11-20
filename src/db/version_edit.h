#ifndef YUBINDB_VERSION_EDIT_H_
#define YUBINDB_VERSION_EDIT_H_
#include <map>
#include <set>
#include <string_view>

#include "../util/key.h"
#include "src/db/block.h"
namespace yubindb {
struct FileMate {  // file mate
  FileMate() = default;
  ~FileMate() = default;
  FileMate(FileMate& ptr)
      : num(ptr.num),
        file_size(ptr.file_size),
        smallest(ptr.smallest),
        largest(ptr.largest) {}
  FileMate(FileMate&& ptr)
      : num(ptr.num),
        file_size(ptr.file_size),
        smallest(ptr.smallest),
        largest(ptr.largest) {}
  uint64_t num;
  uint64_t file_size;
  InternalKey smallest;
  InternalKey largest;
  BlockHandle nextbeg;  // next level 的开始 -> | 用于优化二分查找
  BlockHandle nexend;   // next level 的结束 -> |
};
enum Tag {
  kLogNumber = 0,
  kNextFileNumber = 1,
  kLastSequence = 2,
  kCompactPointer = 3,
  kDeletedFile = 4,
  kNewFile = 5,
};

class VersionEdit {
 public:
  VersionEdit() { Clear(); }
  ~VersionEdit() = default;

  void Clear();
  void SetLogNumber(uint64_t num) {
    has_log_number = true;
    log_number = num;
  }
  void SetNextFile(uint64_t num) {
    has_next_file_number = true;
    next_file_number = num;
  }
  void SetLastSequence(SequenceNum seq) {
    has_last_sequence = true;
    last_sequence = seq;
  }
  void SetCompactPointer(int level, const InternalKey& key) {
    compact_pointers.emplace_back(std::make_pair(level, key));
  }
  // 在指定编号处添加指定文件。
  // “smallest”和“largest”是文件中最小和最大的键
  void AddFile(int level, uint64_t file, uint64_t file_size,
               const InternalKey& smallest, const InternalKey& largest) {
    FileMate f;
    f.num = file;
    f.file_size = file_size;
    f.smallest = smallest;
    f.largest = largest;
    new_files.push_back(std::make_pair(level, f));
  }

  // Delete the specified "file" from the specified "level".
  void DeleteFile(int level, uint64_t file) {
    deleted_files.insert(std::make_pair(level, file));
  }

  void EncodeTo(std::string* dst) const;
  State DecodeFrom(std::string_view src);

 private:
  friend class VersionSet;

  typedef std::set<std::pair<int, uint64_t>> DeletedFileSet;

  uint64_t log_number;
  uint64_t next_file_number;
  SequenceNum last_sequence;
  bool has_log_number;
  bool has_next_file_number;
  bool has_last_sequence;

  std::vector<std::pair<int, InternalKey>>
      compact_pointers;  // <level, 对应level的下次compation启动的key>
  DeletedFileSet deleted_files;
  std::vector<std::pair<int, FileMate>> new_files;  // <level，文件描述信息>
};
}  // namespace yubindb
#endif