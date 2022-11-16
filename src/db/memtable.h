#ifndef YUBINDB_MEMTABLE_H_
#define YUBINDB_MEMTABLE_H_
#include <memory.h>

#include <string_view>

#include "../util/arena.h"
#include "../util/cache.h"
#include "../util/key.h"
#include "../util/skiplistpg.h"
#include "block.h"
namespace yubindb {
class Tablebuilder;
class Memtable {
 public:
  Memtable();
  ~Memtable() = default;

  Memtable(const Memtable&) = delete;
  Memtable& operator=(const Memtable&) = delete;
  void FindShortestSeparator(std::string* start, std::string_view limit);
  void FindShortSuccessor(std::string* key);
  uint32_t ApproximateMemoryUsage();
  // Iterator* NewIterator(); //TODO?
  void Add(SequenceNum seq, Valuetype type, std::string_view key,
           std::string_view value);
  bool Get(const Lookey& key, std::string* value, State* s);
  void Flushlevel0fromskip(FileMate& meta);

 private:
  friend class Tablebuilder;
  std::string tmp;
  std::shared_ptr<Arena> arena;
  std::unique_ptr<Skiplist> table;
  class Tablebuilder {
   public:
   private:
    Options options;
    Options index_block_options;
    WritableFile* file;  //要生成的.sst文件
    uint64_t offset;     //累加每个Data Block的偏移量
    State state;
    Blockbuilder data_block;
    Blockbuilder index_block;
    std::string
        last_key;  //上一个插入的key值，新插入的key必须比它大，保证.sst文件中的key是从小到大排列的
    int64_t num_entries;  //.sst文件中存储的所有记录总数。
    bool closed;          // Either Finish() or Abandon() has been called.
    FilterBlockBuilder* filter_block;

    bool pending_index_entry;  //当一个Data Block被写入到.sst文件时，为true
    BlockHandle pending_handle;  // Handle to add to index block

    std::string compressed_output;  //Data Block的block_data字段压缩后的结果
  };
};

}  // namespace yubindb
#endif