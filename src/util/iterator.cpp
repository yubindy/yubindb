#include "iterator.h"

#include <cstddef>
#include <string_view>

#include "cache.h"
namespace yubindb {
Iterator* GetFileIterator(void* arg, const ReadOptions& options,
                          std::string_view file_value) {
  TableCache* cache = reinterpret_cast<TableCache*>(arg);
  if (file_value.size() != 16) {
    spdlog::error("FileReader invoked with unexpected value");
    return nullptr;
  } else {
    return cache->NewIterator(options, DecodeFixed64(file_value.data()),
                              DecodeFixed64(file_value.data() + 8));
  }
}
}  // namespace yubindb