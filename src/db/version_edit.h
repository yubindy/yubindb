#ifndef YUBINDB_VERSION_EDIT_H_
#define YUBINDB_VERSION_EDIT_H_

#include "../util/key.h"
namespace yubindb {
class FileMate {  // file mate
  int re;
  bool seek;  // if compaction is not
  uint64_t num;
  uint64_t file_size;
  InternalKey smallest;
  InternalKey largeest;
};
}  // namespace yubindb
#endif