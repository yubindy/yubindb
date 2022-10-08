#ifndef YUBINDB_CACHE_H_
#define YUBINDB_CACHE_H_
#include "folly/FBString.h"
#include <array>
#include <list>
#include <map>
#include <memory.h>
#include <mutex>
#include <unordered_map>
using string = folly::fbstring;
namespace yubindb {

class Cache {
public:
  explicit Cache(){};
  virtual ~Cache() = 0;
  virtual void Put(const string &key, string &val) = 0;
  virtual bool Get(const string &key, string *val) = 0;
};

class LruCache : public Cache {
public:
  explicit LruCache(uint64_t size_) : size(size_){};
  ~LruCache() = default;
  void Put(const string &key, string &val) override;
  bool Get(const string &key, string *val) override;

private:
  // void (*delete)(cosnt string &key, string &val);
  uint64_t size;
  std::mutex mut;
  using kvpair = std::pair<string, string>;
  std::list<kvpair> cacheList;
  std::unordered_map<string, std::list<kvpair>::iterator> cacheMap;
};
class ShareCache {
public:
  explicit ShareCache();
  ~ShareCache() = default;
  void Put(const string &key, string &val);
  bool Get(const string &key, string *val);
  uint32_t Hash(const string &key);

private:
  using Lrucacheptr = std::shared_ptr<LruCache>;
  std::array<Lrucacheptr, 16> LruMap;
};

} // namespace yubindb
#endif