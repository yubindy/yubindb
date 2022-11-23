#ifndef YUBINDB_OPTIONS_H_
#define YUBINDB_OPTIONS_H_
#include "env.h"
namespace yubindb {
class Snapshot;
const static int kNumLevels = 7;
enum CompressionType { kNoCompression = 0x0, kSnappyCompression = 0x1 };
struct Options {
  // Create an Options object with default values for all fields.
  Options() = default;
  ~Options() = default;
  Options(const Options& opt)
      : paranoid_checks(opt.paranoid_checks),
        max_open_files(opt.max_open_files),
        block_size(opt.block_size),
        block_restart_interval(opt.block_restart_interval),
        write_buffer_size(opt.write_buffer_size),
        max_file_size(opt.max_file_size),
        reuse_logs(opt.reuse_logs),
        compression(kSnappyCompression) {}
  bool paranoid_checks = false;

  int max_open_files = 1000;

  uint32_t block_size = 4 * 1024;

  int block_restart_interval = 16;

  uint64_t write_buffer_size = 4 * 1024 * 1024;  // 4M
  // todo immtable queue

  uint32_t max_file_size = 2 * 1024 * 1024;

  bool reuse_logs = false;
  CompressionType compression = kSnappyCompression;
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
namespace config {
static const int kNumLevels = 7;

// Level-0 compaction is started when we hit this many files.
static const int kL0_Compaction = 4;

// Soft limit on number of level-0 files.  We slow down writes at this point.
static const int kL0_SlowdownWrites = 8;

// Maximum number of level-0 files.  We stop writes at this point.
static const int kL0_StopWrites = 12;

// Maximum level to which a new compacted memtable is pushed if it
// does not create overlap.  We try to push to level 2 to avoid the
// relatively expensive level 0=>1 compactions and to avoid some
// expensive manifest file operations.  We do not push all the way to
// the largest level since that can generate a lot of wasted disk
// space if the same key space is being repeatedly overwritten.
static const int kMaxMemCompactLevel = 2;

// Approximate gap in bytes between samples of data read during iteration.
static const int kReadBytesPeriod = 1048576;

}  // namespace config
static inline int cmp(const std::string_view& a,
                      const std::string_view& b) {  // inernalkey cmp

  int r = std::string_view(a.data(), a.size() - 8)
              .compare(std::string_view(b.data(), b.size() - 8));
  if (r == 0) {
    const uint64_t anum = DecodeFixed64(a.data() + a.size() - 8);
    const uint64_t bnum = DecodeFixed64(b.data() + b.size() - 8);
    if (anum > bnum) {
      r = -1;
    } else if (anum < bnum) {
      r = +1;
    }
  }
  return r;
}
}  // namespace yubindb
#endif