#include "sstable.h"

#include <spdlog/spdlog.h>

#include <string_view>

#include "src/db/memtable.h"
#include "src/util/options.h"
namespace yubindb {
State Table::Open(const Options& options,
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

  // read index block
  std::string_view index_blocks;
  ReadOptions read;
  s = ReadBlock
}
}  // namespace yubindb