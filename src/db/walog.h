#ifndef YUBINDB_WALOG_H_
#define YUBINDB_WALOG_H_
#include <string_view>

#include "../util/common.h"
#include "../util/env.h"

namespace yubindb {

enum RecordType {
  // zero is reserved for preallocated files
  kzerotype = 0,

  kfulltype = 1,
  kfirsttype = 2,
  kmiddletype = 3,
  klasttype = 4
};
class Record {
 public:
  Record();
  ~Record();

 private:
};
class walWriter {
 public:
  explicit walWriter(WritableFile* file_) : file(file_), block_offset(0) {
    for (int i = 0; i <= klasttype; i++) {
      // crc32
    }
  }
  ~walWriter();
  State Append(std::string_view str);

 private:
  State Flushphyrecord(RecordType type, const char* buf_, size_t size);
  WritableFile* file;
  int block_offset;
  uint32_t type_crc[klasttype + 1];
};
class walReader {
 public:
  explicit walReader();
  ~walReader();

 private:
};
}  // namespace yubindb
#endif