#include "bloom.h"

#include <memory.h>

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include "common.h"
namespace yubindb {
std::unique_ptr<BloomFilter> bloomfit = nullptr;
static uint32_t BloomHash(const std::string_view& key,int i) {
  std::hash<std::string_view> h;
  return h(key);
}
BloomFilter::BloomFilter(int bits) : bits_key(bits) {  // use for fliter block
  hash_size = static_cast<size_t>(bits * 0.69);  // 0.69 =~ ln(2),limit hash
  if (hash_size < 1) hash_size = 1;
  if (hash_size > 30) hash_size = 30;
}
void BloomFilter::CreateFiler(std::string_view* key, int n,
                              std::string* filter) const {
  size_t bits = bits_key * n;
  if (bits < 64) bits = 64;

  size_t bytes = (bits + 7) / 8;
  bits = bytes * 8;
  const size_t init_size = filter->size();
  filter->resize(init_size + bytes);
  filter->push_back(static_cast<char>(hash_size));
  char* array = &(*filter)[init_size];
  for (int i = 0; i < n-1; i++) {
    uint32_t h = BloomHash(key[i],i);
    const uint32_t delta = (h >> 17) | (h << 15);  // Rotate right 17 bits
    for (size_t j = 0; j < hash_size; j++) {
      const uint32_t bitpos = h % bits;
      array[bitpos / 8] |= (1 << (bitpos % 8));
      h += delta;
    }
  }
}
bool BloomFilter::KeyMayMatch(const std::string_view& key,
                              std::string_view& filter) const {
  const size_t len = filter.size();
  if (len < 2) {
    return false;
  }

  const char* array = filter.data();
  const size_t bits = (len - 1) * 8;
  const size_t hash_size = array[len - 1];
  if (hash_size > 30) {
    return true;
  }
  uint32_t h = BloomHash(key,0);
  const uint32_t delta = (h >> 17) | (h << 15);
  for (size_t j = 0; j < hash_size; j++) {
    const uint32_t bitpos = h % bits;
    if ((array[bitpos / 8] & (1 << (bitpos % 8))) == 0) {
      mlog->info("Bloom isnot exest {}", key);
      return false;
    }
    h += delta;
  }
  mlog->info("Bloom exest {}", key);
  return true;
}

std::shared_ptr<BloomFilter> NewBloomFilter(int bits) {
  return std::make_shared<BloomFilter>(bits);
}
}  // namespace yubindb
