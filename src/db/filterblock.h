#ifndef YUBINDB_FILTERBLOCK_H_
#define YUBINDB_FILTERBLOCK_H_
#include <string>
#include <string_view>
#include "../util/bloom.h"
namespace yubindb {
class FilterBlockbuilder {
 public:
 explicit FilterBlockbuilder(BloomFilter& filter);
 private:
};
class FilterBlockreader {};
};  // namespace yubindb
#endif