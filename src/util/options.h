#ifndef YUBINDB_OPTIONS_H_
#define YUBINDB_OPTIONS_H_
#include "../db/snapshot.h"
#include "env.h"
namespace yubindb {
const static int kNumLevels = 7;
struct Options {
  // Create an Options object with default values for all fields.
  Options() = default;
  ~Options() = default;
  bool paranoid_checks = false;

  int max_open_files = 1000;

  size_t block_size = 4 * 1024;

  int block_restart_interval = 16;

  uint64_t write_buffer_size = 4 * 1024 * 1024;  // 4M
  // todo immtable queue

  size_t max_file_size = 2 * 1024 * 1024;

  bool reuse_logs = false;

  PosixEnv env;
};

struct WriteOptions {
  WriteOptions() { sync = false; }
  bool sync;
};
// Options that control read operations
struct ReadOptions {
  ReadOptions() = default;
  //是否校验
  bool verify_checksums = false;

  //是否缓存内存，批量操作建议false
  bool fill_cache = true;
  const Snapshot* snapshot = nullptr;
};
}  // namespace yubindb
#endif