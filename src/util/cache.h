// #ifndef YUBINDB_CACHE_H_
// #define YUBINDB_CACHE_H_
// #include <array>
// #include <list>
// #include <map>
// #include <memory.h>
// #include <mutex>
// #include <string_view>
// #include <unordered_map>
// namespace yubindb {

// class Cache {
//  public:
//   explicit Cache() = default;
//   virtual ~Cache() = 0;
//   struct Handle {};
//   Cache(const Cache&) = delete;
//   Cache& operator=(const Cache&) = delete;
//   virtual Handle* Insert(std::string_view key, void* value, size_t charge,
//                          void (*deleter)(std::string_view key,
//                                          void* value)) = 0;
//   virtual bool Get(std::string_view key, std::string* val) = 0;
//   virtual void Release(Handle* handle) = 0;
// };

// class LruCache : public Cache {
//  public:
//   explicit LruCache(uint64_t size_) : size(size_){};
//   ~LruCache() = default;
//   void SetCapacity(size_t capacity) { size = capacity; }

//   // Like Cache methods, but with an extra "hash" parameter.
//   Cache::Handle* Insert(std::string_view key, uint32_t hash, void* value,
//                         size_t charge,
//                         void (*deleter)(std::string_view key, void* value));
//   Cache::Handle* Lookup(std::string_view key, uint32_t hash);
//   void Release(Cache::Handle* handle);
//   void Erase(std::string_view key, uint32_t hash);

//  private:
//   // void (*delete)(cosnt string &key, string &val);
//   uint64_t size;
//   std::mutex mut;
//   using kvpair = std::pair<string, string>;
//   std::list<kvpair> cacheList;
//   std::unordered_map<string, std::list<kvpair>::iterator> cacheMap;
// };
// class ShareCache {
//  public:
//   explicit ShareCache();
//   ~ShareCache() = default;
//   void Put(const string& key, string& val);
//   bool Get(const string& key, string* val);
//   uint32_t Hash(const string& key);

//  private:
//   using Lrucacheptr = std::shared_ptr<LruCache>;
//   std::array<Lrucacheptr, 16> LruMap;
// };

// }  // namespace yubindb
// #endif