#ifndef YUBINDB_FILTERBLOCK_H_
#define YUBINDB_FILTERBLOCK_H_
#include <memory.h>

#include <memory>
#include <string>
#include <string_view>

#include "../util/bloom.h"
namespace yubindb {
static const size_t kFilterBaseLg = 11;
static const size_t kFilterBase = 1 << kFilterBaseLg;

static std::unique_ptr<BloomFilter> bloomfit=nullptr;
class FilterBlockbuilder {
 public:
  FilterBlockbuilder() = default;
  void StartBlock(uint64_t block_offset);
  void AddKey(const std::string_view& key);
  std::string_view Finish();

 private:
  void CreateNewfilter();
  std::string keys_;
  std::vector<size_t> start_;
  std::string result_;  // bloom filter
  std::vector<std::string_view> tmp_keys_;
  std::vector<uint32_t> filter_offsets_;
};
class FilterBlockReader {
 public:
  explicit FilterBlockReader(const std::string_view& contents);
  bool KeyMayMatch(uint64_t block_offset, const std::string_view& key);

 private:
  const char* data;
  const char* offset;
  size_t num;
  size_t base_lg;  //每个filter size
};
};  // namespace yubindb
#endif