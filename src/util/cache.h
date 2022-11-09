#ifndef YUBINDB_CACHE_H_
#define YUBINDB_CACHE_H_
#include <list>
#include <map>
#include <memory.h>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "env.h"
#include "options.h"
namespace yubindb {
using kvpair = std::pair<std::string_view, std::string>;
class LruCache {
 public:
  LruCache() : size(0), use(0) {}
  ~LruCache() = default;
  void SetCapacity(size_t capacity) { size = capacity; }

  std::shared_ptr<kvpair> Insert(kvpair& pair);
  std::shared_ptr<kvpair> Lookup(std::string_view& key);
  size_t Getsize() const { return cacheList.size() * (sizeof(kvpair) + 8); };

 private:
  uint64_t size;
  uint64_t use;
  std::mutex mutex;
  std::list<std::shared_ptr<kvpair>> cacheList;
  std::unordered_map<std::string_view,
                     std::list<std::shared_ptr<kvpair>>::iterator>
      cacheMap;
};
class ShareCache {
 public:
  explicit ShareCache(uint64_t size) : last_id(0) {
    for (int s = 0; s < 16; s++) {
      sharecache[s] = std::make_unique<LruCache>();
      sharecache[s]->SetCapacity(size / 16);
    }
  }
  ~ShareCache() {}
  std::shared_ptr<kvpair> Insert(std::string_view& key, std::string& value);
  std::shared_ptr<kvpair> Lookup(std::string_view& key);
  uint64_t NewId();
  size_t Getsize();

 private:
  std::unique_ptr<LruCache> sharecache[16];
  std::mutex mutex;
  int last_id;
};
class TableCache {
 public:
  explicit TableCache(std::string_view dbname, const Options* opt);
  ~TableCache() = default;
  State Get(const ReadOptions& readopt, uint64_t file_num, uint64_t file_size,
            std::string_view key, void* arg,
            void (*handle_rul)(void*, std::string_view a, std::string_view b));
  void Evict(uint64_t file_number);

 private:
  State FindTable(uint64_t file_num, uint64_t file_size, void**);

  const PosixEnv* env;
  std::string_view dbname;
  const Options* const opt;
  std::unique_ptr<ShareCache> cache;
};
}  // namespace yubindb
#endif
