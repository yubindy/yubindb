// #include "bloom.h"

// #include <cstdint>
// #include <functional>
// #include <memory.h>
// #include <string>

// namespace yubindb {
// static uint32_t BloomHash(std::string_view& key) {
//   // return std::hash<char>()(key);
// }
// BloomFilter::BloomFilter(int bits) : bits_key(bits) {
//   hash_size = static_cast<size_t>(bits * 0.69);  // 0.69 =~ ln(2),limit hash
//   if (hash_size < 1) hash_size = 1;
//   if (hash_size > 30) hash_size = 30;
// }
// void BloomFilter::CreateFiler(std::string_view& key,
//                               std::string* filter) const {
//   size_t bits = bits_key * key.size();
//   if (bits < 64) bits = 64;
//   bits = ((bits + 7) * 8) / 8;
//   const size_t init_size = filter->size();
//   filter->resize(init_size);
//   filter->push_back(static_cast<char>(hash_size));
//   char* array = &(*filter)[init_size];
//   for (int i = 0; i < key.size(); i++) {
//     uint32_t h = BloomHash(key[i]);
//     const uint32_t delta = (h >> 17) | (h << 15);  // Rotate right 17 bits
//     for (size_t j = 0; j < hash_size; j++) {
//       const uint32_t bitpos = h % bits;
//       array[bitpos / 8] |= (1 << (bitpos % 8));
//       h += delta;
//     }
//   }
// }
// bool BloomFilter::KeyMayMatch(std::string_view& key,
//                               std::string_view& filter) const {
//   const size_t len = filter.size();
//   if (len < 2) {
//     return false;
//   }
//   const char* array = filter.data();
//   const size_t bits = (len - 1) * 8;
//   const size_t k = array[len - 1];
//   uint32_t h = BloomHash(key);
//   const uint32_t delta = (h >> 17) | (h << 15);
//   for (size_t j = 0; j < k; j++) {
//     const uint32_t bitpos = h % bits;
//     if ((array[bitpos / 8] & (1 << (bitpos % 8))) == 0) {
//       spdlog::info("Bloom isnot exest");
//       return false;
//     }
//     h += delta;
//   }
//   spdlog::info("Bloom exest");
//   return true;
// }

// std::shared_ptr<Filter> NewBloomFilter(int bits) {
//   return std::make_shared<BloomFilter>(bits);
// }
// }  // namespace yubindb
