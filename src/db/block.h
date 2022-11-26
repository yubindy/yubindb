#ifndef YUBINDB_BLOCK_H_
#define YUBINDB_BLOCK_H_
#include <cassert>
#include <string_view>

#include "../util/options.h"
#include "iterator.h"
#include "src/util/common.h"
#include "src/util/key.h"
namespace yubindb {
static const uint64_t kTableMagicNumber = 0xdb4775248b80fb57ull;
// 1-byte type + 32-bit crc
static const size_t kBlockBackSize = 5;
class BlockHandle {
 public:
  enum { kMaxEncodedLength = 10 + 10 };

  BlockHandle()=default;
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
      log->error("bad block handle");
      return State::Corruption();
    }
  }

 private:
  uint64_t offset_;
  uint64_t size_;
};
State ReadBlock(RandomAccessFile* file, const ReadOptions& options,
                const BlockHandle& handle, std::string_view* result);
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
class Block {  // stack alloc
 private:
  class Iter;

 public:
  explicit Block(std::string_view data)
      : data_(data.data()), size_(data.size()) {
    if (size_ < sizeof(uint32_t)) {
      size_ = 0;
    } else {
      size_t max_restarts_allowed =
          (size_ - sizeof(uint32_t)) / sizeof(uint32_t);
      if (NumRestarts() > max_restarts_allowed) {
        size_ = 0;
      } else {
        restart_offset_ = size_ - (1 + NumRestarts()) * sizeof(uint32_t);
      }
    }
  }
  Block(const Block&) = delete;
  Block& operator=(const Block&) = delete;

  ~Block()=default;
  static inline const char* DecodeEntry(const char* p, const char* limit,
                                        uint32_t* shared, uint32_t* non_shared,
                                        uint32_t* value_length) {
    if (limit - p < 3) return nullptr;
    *shared = reinterpret_cast<const uint8_t*>(p)[0];
    *non_shared = reinterpret_cast<const uint8_t*>(p)[1];
    *value_length = reinterpret_cast<const uint8_t*>(p)[2];
    if ((*shared | *non_shared | *value_length) < 128) {
      p += 3;
    } else {
      if ((p = GetVarint32Ptr(p, limit, shared)) == nullptr) return nullptr;
      if ((p = GetVarint32Ptr(p, limit, non_shared)) == nullptr) return nullptr;
      if ((p = GetVarint32Ptr(p, limit, value_length)) == nullptr)
        return nullptr;
    }

    if (static_cast<uint32_t>(limit - p) < (*non_shared + *value_length)) {
      return nullptr;
    }
    return p;
  }
  size_t size() const { return size_; }
  std::shared_ptr<Iterator> NewIterator() {
    if (size_ < sizeof(uint32_t)) {
      log->error("size is small");
      return nullptr;
    }
    if (NumRestarts() > 0) {
      return static_pointer_cast<Iterator>(
          std::make_shared<Iter>(data_, restart_offset_, NumRestarts()));
    } else {
      return nullptr;
    }
  }

 private:
  inline uint32_t NumRestarts() const {
    assert(size_ >= sizeof(uint32_t));
    return DecodeFixed32(data_ + size_ -
                         sizeof(uint32_t));  //最后存储restart num
  }

  const char* data_;
  size_t size_;
  uint32_t restart_offset_;  // Offset in data_ of restart array
};
class Block::Iter : public Iterator {
 private:
  inline uint32_t NextEntryOffset() const {
    return (value_.data() + value_.size()) - data_;
  }
  uint32_t GetRestartPoint(uint32_t index) {
    assert(index < num_restarts_);
    return DecodeFixed32(data_ + restarts_ + index * sizeof(uint32_t));
  }
  void SeekToRestartPoint(uint32_t index) {
    key_.clear();
    restart_index_ = index;
    uint32_t offset = GetRestartPoint(index);
    value_ = std::string_view(data_ + offset, 0);  // value offset，用于后面解析
  }
  inline uint32_t NextEntryoffset() const {
    return (value_.data() + value_.size() - data_);
  }
  bool ParseNextKey() {
    current_ = NextEntryoffset();
    const char* p = data_ + current_;
    const char* limit = data_ + restarts_;
    if (p >= limit) {
      current_ = restarts_;
      restart_index_ = num_restarts_;
      return false;
    }

    uint32_t shared, non_shared, value_len;
    p = DecodeEntry(p, limit, &shared, &non_shared, &value_len);
    if (p == nullptr || key_.size() < shared) {
      key_.clear();
      log->error("bad entry in block nowoffset {} key {}", current_, key_);
      return false;
    } else {
      key_.resize(shared);
      key_.append(p, non_shared);
      value_ = std::string_view(p + non_shared, value_len);
      while (restart_index_ + 1 < num_restarts_ &&
             GetRestartPoint(restart_index_ + 1) < current_) {
        ++restart_index_;
      }
      return true;
    }
  }
  const char* data_;
  uint32_t const restarts_;
  uint32_t const num_restarts_;

  uint32_t current_;  // now offset
  uint32_t restart_index_;
  std::string key_;
  std::string_view value_;
  State state_;

 public:
  Iter(const char* data, uint32_t restarts, uint32_t num_restarts)
      : data_(data),
        restarts_(restarts),
        num_restarts_(num_restarts),
        current_(restarts_),
        restart_index_(num_restarts_) {
    assert(num_restarts_ > 0);
  }
  bool Valid() const override { return current_ < restarts_; }
  State state() { return state_; }
  std::string_view key() const override {
    Valid();
    return key_;
  }
  std::string_view value() const override {
    Valid();
    return value_;
  }
  void SeekToFirst() override {
    SeekToRestartPoint(0);
    ParseNextKey();
  }
  void SeekToLast() override {
    SeekToRestartPoint(num_restarts_ - 1);
    while (ParseNextKey() && NextEntryOffset() < restarts_) {
    }
  }
  void Seek(std::string_view target) override {  //二分查找 by re_start
    uint32_t left = 0;
    uint32_t right = num_restarts_ - 1;
    while (left < right) {
      uint32_t mid = (left + right + 1) / 2;
      uint32_t region_offset = GetRestartPoint(mid);
      uint32_t shared, non_shared, value_length;
      const char* key_ptr =
          DecodeEntry(data_ + region_offset, data_ + restarts_, &shared,
                      &non_shared, &value_length);
      if (key_ptr == nullptr || (shared != 0)) {
        log->error("doing error in this");
        return;
      }
      std::string_view mid_key(key_ptr, non_shared);
      if (cmp(mid_key, target) < 0) {
        left = mid;
      } else {
        right = mid - 1;
      }
    }
    SeekToRestartPoint(left);
    while (1) {
      if (!ParseNextKey()) {
        return;
      }
      if (cmp(key_, target) >= 0) {
        return;
      }
    }
  }
  void Next() override {
    assert(Vaild());
    ParseNextKey();
  }
  void Prev() override {
    assert(Valid());
    const uint32_t now_offset = current_;
    while (GetRestartPoint(restart_index_) >= now_offset) {
      if (restart_index_ == 0) {
        // No more entries
        current_ = restarts_;
        restart_index_ = num_restarts_;
        return;
      }
      restart_index_--;
    }
    SeekToRestartPoint(restart_index_);
    do {
    } while (ParseNextKey() && NextEntryOffset() < now_offset);
  }
};
}  // namespace yubindb
#endif