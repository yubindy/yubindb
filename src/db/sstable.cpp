#include "sstable.h"


#include <memory>
#include <string_view>

#include "cache.h"
#include "src/db/block.h"
#include "src/db/memtable.h"
#include "src/util/options.h"
namespace yubindb {
State Table::Open(const Options& options_,
                  std::shared_ptr<RandomAccessFile> file, uint64_t file_size,
                  std::shared_ptr<Table>& table) {
  table = nullptr;
  if (file_size < Footer::kEncodedLength) {
    mlog->error("file is too smart an sstable");
    return State::Corruption();
  }
  // char foot_buf[Footer::kEncodedLength];
  std::string_view foot_input;
  State s = file->Read(file_size - Footer::kEncodedLength,
                       Footer::kEncodedLength, &foot_input);
  if (!s.ok()) return s;

  Footer footer;
  s = footer.DecodeFrom(&foot_input);
  if (!s.ok()) return s;
  // read index block
  std::string_view index_blocks;
  ReadOptions read;
  mlog->debug("read index_blocks block form offset {} size {}",footer.index_handle().offset(),footer.index_handle().size());
  s = ReadBlock(file.get(), read, footer.index_handle(), &index_blocks);

  if (s.ok()) {  // read filter
    std::unique_ptr<Tablestpl> pls = std::make_unique<Tablestpl>();
    pls->options = options_;
    pls->file = file;
    pls->metaindex_handle = footer.metaindex_handle();
    pls->index_block = std::make_shared<Block>(index_blocks);
    pls->filter_data = nullptr;
    pls->filter = nullptr;
    table = std::make_shared<Table>(pls);
    table->ReadMeta(footer);
  }

  return s;
}
void Table::ReadMeta(const Footer& footer) {
  ReadOptions opt;
  std::string_view rul;
  State s = ReadBlock(pl->file.get(), opt, footer.metaindex_handle(), &rul);
  if (!s.ok()) {
    return;
  }
  std::unique_ptr<Block> meta = std::make_unique<Block>(rul);
  std::shared_ptr<Iterator> iter = meta->NewIterator();
  std::string key = "filter.bloom";
  iter->Seek(key);
  if (iter->Valid() && iter->key() == std::string_view(key)) {
    ReadFilter(iter->value());
  }
}
void Table::ReadFilter(std::string_view filter_handle_value) {
  std::string_view v = filter_handle_value;
  BlockHandle filter_handle;
  if (!filter_handle.DecodeFrom(&v).ok()) {
    return;
  }

  // We might want to unify with ReadBlock() if we start
  // requiring checksum verification in Table::Open.
  ReadOptions opt;
  if (pl->options.paranoid_checks) {
    opt.verify_checksums = true;
  }
  std::string_view block;
  if (!ReadBlock(pl->file.get(), opt, filter_handle, &block).ok()) {
    return;
  }
  pl->filter_data = block.data();  // Will need to delete later

  pl->filter = std::make_shared<FilterBlockReader>(block);
}
std::shared_ptr<Iterator> Table::BlockReader(void* args, const ReadOptions& opt,
                                             std::string_view index_value) {
  Table* table = reinterpret_cast<Table*>(args);
  std::shared_ptr<Block> block = nullptr;
  CacheHandle* cache_handle = nullptr;

  BlockHandle handle;
  std::string_view input = index_value;
  State s = handle.DecodeFrom(&input);
  if (s.ok()) {
    std::string_view contents;
    s = ReadBlock(table->pl->file.get(), opt, handle, &contents);
    if (s.ok()) {
      block = std::make_shared<Block>(contents);
    }
  }
  std::shared_ptr<Iterator> iter;
  if (block != nullptr) {
    iter = block->NewIterator();
  } else {
    mlog->error("new iterator blockreader error");
  }
  return iter;
}
State Table::InternalGet(const ReadOptions& options, std::string_view k,
                         void* arg,
                         void (*handle_result)(void*, std::string_view,
                                               std::string_view)) {
  State s;
  std::shared_ptr<Iterator> iiter = pl->index_block->NewIterator();
  iiter->Seek(k);
  if (iiter->Valid()) {
    std::string_view handle_value = iiter->value();
    std::shared_ptr<FilterBlockReader> filter = pl->filter;
    BlockHandle handle;
    if (filter != nullptr && handle.DecodeFrom(&handle_value).ok() &&
        !filter->KeyMayMatch(handle.offset(), k)) {
      // Not found
    } else {
      std::shared_ptr<Iterator> block_iter =
          BlockReader(this, options, iiter->value());
      block_iter->Seek(k);
      if (block_iter->Valid()) {
        (*handle_result)(arg, block_iter->key(), block_iter->value());
      }
      s = block_iter->state();
    }
  }
  if (s.ok()) {
    s = iiter->state();
  }
  return s;
}
std::shared_ptr<Iterator> Table::NewIterator(const ReadOptions& opt) {
  return NewTwoLevelIterator(pl->index_block->NewIterator(),
                             &Table::BlockReader, const_cast<Table*>(this),
                             opt);
}
}  // namespace yubindb