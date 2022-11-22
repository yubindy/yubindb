#ifndef YUBINDB_TABLE_H_
#define YUBINDB_TABLE_H_
#include <memory>
#include <string_view>

#include "../util/common.h"
#include "../util/iterator.h"
#include "../util/options.h"
#include "block.h"
#include "filterblock.h"
#include "src/db/memtable.h"
namespace yubindb {
struct Tablestpl {
  Options options;
  State status;
  std::shared_ptr<RandomAccessFile> file;
  uint64_t cache_id;  // block cache的ID，用于组建block cache结点的key
  std::shared_ptr<FilterBlockReader> filter;
  const char* filter_data;
  uint64_t ache_id;

  BlockHandle metaindex_handle;  // Handle to metaindex_block: saved from footer
  std::shared_ptr<Block> index_block;
};
class Table {
 public:
  static State Open(const Options& options_,
                    std::shared_ptr<RandomAccessFile> file, uint64_t file_size,
                    std::shared_ptr<Table>& table);
  Table(const Table&) = delete;
  Table& operator=(const Table&) = delete;
  Iterator* NewIterator(const ReadOptions&);
  ~Table();

 private:
  friend class TableCache;

  static Iterator* BlockReader(void*, const ReadOptions&, std::string_view);

  explicit Table(std::unique_ptr<Tablestpl>& pl_) { pl.reset(pl_.release()); }

  State InternalGet(const ReadOptions&, std::string_view key, void* arg,
                    void (*handle_result)(void* arg, std::string_view k,
                                          std::string_view v));
  void ReadMeta(const Footer& footer);
  void ReadFilter(std::string_view filter_handle_value);
  std::unique_ptr<Tablestpl> pl;
};
}  // namespace yubindb
#endif