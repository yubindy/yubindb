#include "sstable.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <string_view>

#include "src/db/block.h"
#include "src/db/memtable.h"
#include "src/util/options.h"
namespace yubindb {
State Table::Open(const Options& options_,
                  std::shared_ptr<RandomAccessFile> file, uint64_t file_size,
                  std::shared_ptr<Table>& table) {
  table = nullptr;
  if (file_size < Footer::kEncodedLength) {
    spdlog::error("file is too smart an sstable");
    return State::Corruption();
  }
  char foot_buf[Footer::kEncodedLength];
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
}  // namespace yubindb