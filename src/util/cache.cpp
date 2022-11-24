#include "cache.h"

#include <memory>
#include <string_view>

#include "filename.h"
namespace yubindb {
struct TableAndFile {
 public:
  TableAndFile(TableAndFile& ptr) : file(ptr.file), table(ptr.table) {}
  std::shared_ptr<RandomAccessFile> file;
  std::shared_ptr<Table> table;
};
void LruCache::Erase(std::string_view ekey) {
  std::lock_guard<std::mutex> lk(mutex);
  auto s = cacheMap.find(ekey);
  if (s != cacheMap.end()) {
    cacheList.erase(s->second);
    cacheMap.erase(s);
  }
}
CacheHandle* LruCache::Insert(kvpair& pair) {
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
  return &pair.second;
}
CacheHandle* LruCache::Lookup(std::string_view& key) {
  std::lock_guard<std::mutex> lk(mutex);
  auto s = cacheMap.find(key);
  if (s != cacheMap.end()) {
    cacheList.splice(cacheList.begin(), cacheList, s->second);
    s->second = cacheList.begin();
    return &((*s->second)->second);
  }
  return nullptr;
}
CacheHandle* ShareCache::Insert(std::string_view& key,
                                std::shared_ptr<void> value) {
  int index = std::hash<std::string_view>{}(key) % 16;
  CacheHandle handle;
  handle.str = value;
  kvpair pair(key, handle);
  return sharecache[index]->Insert(pair);
}
CacheHandle* ShareCache::Lookup(std::string_view& key) {
  int index = std::hash<std::string_view>{}(key) % 16;
  return sharecache[index]->Lookup(key);
}
void ShareCache::Erase(std::string_view ekey) {
  int index = std::hash<std::string_view>{}(ekey) % 16;
  sharecache[index]->Erase(ekey);
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
      cache(std::make_unique<ShareCache>(opt->max_open_files -
                                         kNumNonTableCacheFiles)) {}
State TableCache::Get(const ReadOptions& readopt, uint64_t file_num,
                      uint64_t file_size, std::string_view key, void* arg,
                      void (*handle_rul)(void*, std::string_view a,
                                         std::string_view b)) {
  CacheHandle* handle = nullptr;
  State s = FindTable(file_num, file_size, &handle);
  if (s.ok()) {
    std::shared_ptr<Table> t =
        (*std::static_pointer_cast<std::shared_ptr<TableAndFile>>(handle->str))
            ->table;
    s = t->InternalGet(readopt, key, arg, handle_rul);
  }
  return s;
}
void TableCache::Evict(uint64_t file_number) {
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  cache->Erase(std::string_view(buf, sizeof(buf)));
}
State TableCache::FindTable(
    uint64_t file_num,
    uint64_t file_size,  //将ldb或sst文件映射为Table类,实现文件的操作
    CacheHandle** handle) {
  State s;
  char buf[sizeof(file_num)];
  EncodeFixed64(buf, file_num);
  std::string_view key(buf, sizeof(buf));
  *handle = cache->Lookup(key);
  std::string dbname_(dbname);
  if (*handle == nullptr) {
    std::string fname = TableFileName(dbname_, file_num);
    std::shared_ptr<RandomAccessFile> file = nullptr;
    std::shared_ptr<Table> table = nullptr;
    s = env->NewRandomAccessFile(fname, file);
    if (!s.ok()) {
      std::string old_fname = SSTTableFileName(dbname_, file_num);
      if (env->NewRandomAccessFile(old_fname, file).ok()) {
        s = State::Ok();
      }
    }
    if (s.ok()) {
      s = Table::Open(*opt, file, file_size, table);
    }
    if (s.ok()) {
      std::shared_ptr<TableAndFile> tf = std::shared_ptr<TableAndFile>();
      tf->file = file;
      tf->table = table;
      *handle =
          cache->Insert(key, static_pointer_cast<std::shared_ptr<void>>(tf));
    }
  }
  return s;
}
std::shared_ptr<Iterator> TableCache::NewIterator(
    const ReadOptions& options, uint64_t file_number, uint64_t file_size,
    std::shared_ptr<Table> tableptr) {
  tableptr = nullptr;
  CacheHandle* handle = nullptr;
  State s = FindTable(file_number, file_size, &handle);
  if (!s.ok()) {
    spdlog::error("dont find table Number{} {}", file_number, file_size);
  }
  std::shared_ptr<Table> table =
      (*std::static_pointer_cast<std::shared_ptr<TableAndFile>>(handle->str))
          ->table;
  std::shared_ptr<Iterator> rul = table->NewIterator(options);
  tableptr = table;
  return rul;
}
}  // namespace yubindb