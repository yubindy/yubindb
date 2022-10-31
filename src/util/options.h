#ifndef YUBINDB_OPTIONS_H_
#define YUBINDB_OPTIONS_H_
#include "../db/snapshot.h"
namespace yubindb {

struct WriteOptions {
  WriteOptions() { sync = false; }
  bool sync;
};
// Options that control read operations
struct ReadOptions {
  ReadOptions() = default;
  //是否校验
  bool verify_checksums = false;

  //是否缓存内存，批量操作建议
  bool fill_cache = true;
  const Snapshot* snapshot = nullptr;
};
}  // namespace yubindb
#endif