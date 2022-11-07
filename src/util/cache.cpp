// #include "cache.h"

// namespace yubindb {

// void LruCache::Put(const string& key, string& val) {
//   std::lock_guard<std::mutex> lck(mut);
//   auto s = cacheMap.find(key);
//   if (s != cacheMap.end()) {
//     cacheList.emplace_front(std::move(make_pair(key, val)));
//     cacheMap[key] = cacheList.begin();
//     if (cacheList.size() > size) {
//       cacheMap.erase(key);
//       cacheList.pop_back();
//     }
//   } else {
//     val.copy(((s->second)->second).data(), val.size());
//     cacheList.splice(cacheList.begin(), cacheList, s->second);
//     s->second = cacheList.begin();
//   }
// }
// bool LruCache::Get(const string& key, string* val) {
//   std::lock_guard<std::mutex> lck(mut);
//   auto s = cacheMap.find(key);
//   if (s != cacheMap.end()) {
//     cacheList.splice(cacheList.begin(), cacheList, s->second);
//     s->second = cacheList.begin();
//     string val_((s->second)->second);
//     val = &val_;
//     return true;
//   }
//   return false;
// }

// void ShareCache::Put(const string& key, string& val) {
//   uint32_t part = Hash(key);
//   LruMap[part]->Put(key, val);
// }
// bool ShareCache::Get(const string& key, string* val) {
//   uint32_t part = Hash(key);
//   return LruMap[part]->Get(key, val);
// }
// uint32_t ShareCache::Hash(const string& key) {
//   return (folly::hash::fnv32(key.data())) & 0x000f;
// }
// }  // namespace yubindb