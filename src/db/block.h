#ifndef YUBINDB_BLOCK_H_
#define YUBINDB_BLOCK_H_
#include <string_view>

#include "../util/options.h"
#include "src/util/common.h"
namespace yubindb {
class BlockHandle {
 public:
  enum { kMaxEncodedLength = 10 + 10 };

  BlockHandle();
  uint64_t offset() const { return offset_; }
  void set_offset(uint64_t offset) { offset_ = offset; }
  uint64_t size() const { return size_; }
  void set_size(uint64_t size) { size_ = size; }

  void EncodeTo(std::string* dst) const {
    PutVarint64(dst, offset_);
    PutVarint64(dst, size_);
  }
  State DecodeFrom(std::string_view* input) {
    if (GetVarint64(input, &offset_) && GetVarint64(input, &size_)) {
      return State::Ok();
    } else {
      spdlog::error("bad block handle");
      return State::Corruption();
    }
  }

 private:
  uint64_t offset_;
  uint64_t size_;
};
class Blockbuilder {
 public:
  explicit Blockbuilder(const Options* options)
      : opt(options), restarts(), entrynum(0), finished(false) {
    assert(options->block_restart_interval >= 1);
    restarts.push_back(0);
  }
  void Reset();

  void Add(std::string_view key, std::string_view value);

  std::string_view Finish();

  size_t CurrentSizeEstimate() const;

  bool empty() const { return buffer.empty(); }
  Blockbuilder(const Blockbuilder&) = delete;
  Blockbuilder& operator=(const Blockbuilder&) = delete;

 private:
  const Options* opt;
  std::string buffer;
  std::vector<uint32_t> restarts;  // Restart points for 二分没有压缩的点
  int entrynum;
  bool finished;
  std::string last_key;
};
class block {
 public:
  block() = default;
  ~block() = default;
};
}  // namespace yubindb
#endif