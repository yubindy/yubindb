#include "cache.h"
namespace yubindb {
std::shared_ptr<kvpair> LruCache::Insert(kvpair& pair) {
  std::lock_guard<std::mutex> lk(mutex);
  std::shared_ptr<kvpair> hand = std::make_shared<kvpair>(pair);
  auto s = cacheMap.find(pair.first);
  if (s != cacheMap.end()) {
    cacheList.emplace_front(std::move(hand));
    cacheMap[pair.first] = cacheList.begin();
    if (cacheList.size() > size) {
      std::string_view back = cacheList.back()->first;
      s = cacheMap.find(back);
      cacheMap.erase(back);
      cacheList.pop_back();
    }
  } else {
    cacheList.splice(cacheList.begin(), cacheList, s->second);
    s->second = cacheList.begin();
  }
  return hand;
}
std::shared_ptr<kvpair> LruCache::Lookup(std::string_view& key) {
  std::lock_guard<std::mutex> lk(mutex);
  auto s = cacheMap.find(key);
  if (s != cacheMap.end()) {
    cacheList.splice(cacheList.begin(), cacheList, s->second);
    s->second = cacheList.begin();
    return *(s->second);
  }
  return nullptr;
}
std::shared_ptr<kvpair> ShareCache::Insert(std::string_view& key,
                                           std::string& value) {
  int index = std::hash<std::string_view>{}(key) % 16;
  kvpair pair(key, value);
  return sharecache[index]->Insert(pair);
}
std::shared_ptr<kvpair> ShareCache::Lookup(std::string_view& key) {
  int index = std::hash<std::string_view>{}(key) % 16;
  return sharecache[index]->Lookup(key);
}
uint64_t ShareCache::NewId() {
  std::lock_guard<std::mutex> lk(mutex);
  return ++last_id;
}
size_t ShareCache::Getsize() {
  size_t all = 0;
  for (int i = 0; i < 16; i++) {
    all += sharecache[i]->Getsize();
  }
  return all;
}
TableCache::TableCache(std::string_view dbname, const Options* opt)
    : env(new PosixEnv),
      dbname(dbname),
      opt(opt),
      cache(new ShareCache(opt->max_open_files - kNumNonTableCacheFiles)) {}
State TableCache::Get(const ReadOptions& readopt, uint64_t file_num,
                      uint64_t file_size, std::string_view key, void* arg,
                      void (*handle_rul)(void*, std::string_view a,
                                         std::string_view b)) {}
void TableCache::Evict(uint64_t file_number) {}
State TableCache::FindTable(uint64_t file_num, uint64_t file_size, void**) {}
}  // namespace yubindb