#include "filterblock.h"

#include <cstddef>
#include <string_view>

#include "src/util/common.h"
namespace yubindb {
extern std::unique_ptr<BloomFilter> bloomfit;
void FilterBlockbuilder::StartBlock(uint64_t block_offset) {
  uint64_t filter_index = (block_offset / kFilterBase);
  while (filter_index > filter_offsets_.size()) {  // new filter
    CreateNewfilter();
  }
}
void FilterBlockbuilder::AddKey(const std::string_view& key) {
  start_.emplace_back(key.size());
  keys_.append(key.data(), key.size());
}
void FilterBlockbuilder::CreateNewfilter() {
  const size_t num_key = start_.size();
  if (num_key == 0) {
    filter_offsets_.push_back(result_.size());  // empty filter
    return;
  }
  char* p=keys_.data();
  for (int i = 0; i < start_.size(); i++) {
    tmp_keys_.emplace_back(std::string_view(p, start_[i]));
    p+=start_[i];
  }
  filter_offsets_.emplace_back(result_.size());
  bloomfit->CreateFiler(tmp_keys_.data(), start_.size(), &result_);
  tmp_keys_.clear();
  keys_.clear();
  start_.clear();
}
std::string_view FilterBlockbuilder::Finish() {
  if (!start_.empty()) {
    CreateNewfilter();
  }
  const uint32_t bengoffset = result_.size();
  for (int i = 0; i < filter_offsets_.size(); i++) {
    PutFixed32(&result_, filter_offsets_[i]);
  }
  PutFixed32(&result_, bengoffset);
  result_.push_back(kFilterBaseLg);
  return result_;
}
FilterBlockReader::FilterBlockReader(const std::string_view& contents)
    : data(nullptr), offset(nullptr), num(0), base_lg(0) {
  size_t n = contents.size();
  if (n < 5) return;
  data = contents.data();
  base_lg = contents[n - 1];
  uint32_t index_offset = DecodeFixed32(contents.data() + n - 5);
  if (index_offset > n - 5) return;
  offset = data + index_offset;
  num = (n - 5 - index_offset) / 4;
}
bool FilterBlockReader::KeyMayMatch(uint64_t block_offset,
                                    const std::string_view& key) {
  uint64_t index = block_offset >> base_lg;
  if (index < num) {
    uint32_t start = DecodeFixed64(offset + index * 4);
    uint32_t limit = DecodeFixed64(offset + index * 4 + 4);
    if (start <= limit && limit <= static_cast<size_t>(offset - data)) {
      std::string_view filt = std::string_view(data + start, limit - start);
      return bloomfit->KeyMayMatch(key, filt);
    } else if (start == limit) {
      return false;
    }
  }
  return true;
}
};  // namespace yubindb