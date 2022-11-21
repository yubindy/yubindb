#ifndef YUBINDB_TABLE_H_
#define YUBINDB_TABLE_H_
#include <string_view>

#include "../util/common.h"
#include "../util/iterator.h"
#include "../util/options.h"
#include "block.h"
#include "filterblock.h"
namespace yubindb {
class Table {
 public:
  static State Open(const Options& options,
                    std::shared_ptr<RandomAccessFile> file, uint64_t file_size,
                    std::shared_ptr<Table>& table);
  Table(const Table&) = delete;
  Table& operator=(const Table&) = delete;

  ~Table();

 private:
  friend class TableCache;

  static Iterator* BlockReader(void*, const ReadOptions&, std::string_view);

  explicit Table(Rep* rep) : rep_(rep) {}

  State InternalGet(const ReadOptions&, std::string_view key, void* arg,
                    void (*handle_result)(void* arg, std::string_view k,
                                          std::string_view v));
  void ReadMeta(std::string_view footer);
  void ReadFilter(std::string_view filter_handle_value);
  Options options;
  State status;
  std::shared_ptr<RandomAccessFile> file;
  uint64_t cache_id;  // block cache的ID，用于组建block cache结点的key
  std::shared_ptr<FilterBlockReader> filter;
  const char* filter_data;

  BlockHandle metaindex_handle;  // Handle to metaindex_block: saved from footer
  Block* index_block;
};
}  // namespace yubindb
#endif