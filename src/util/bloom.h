#ifndef YUBINDB_FILTER_H_
#define YUBINDB_FILTER_H_
#include <memory>
#include <string_view>

#include "spdlog/spdlog.h"

namespace yubindb {

class Filter {
 public:
  virtual ~Filter() = 0;
  virtual void CreateFiler(std::string_view& key,
                           std::string* filter) const = 0;
  virtual bool KeyMayMatch(std::string_view& key,
                           std::string_view& filter) const = 0;
};

class BloomFilter : public Filter {
 public:
  explicit BloomFilter(int bits);
  ~BloomFilter() = default;
  //给长度为 n 的 key,创建一个过滤策略，并将策略序列化为 string ，追加到 dst
  void CreateFiler(std::string_view& key,
                   std::string* filter) const override;  // d
  bool KeyMayMatch(std::string_view& key,
                   std::string_view& filter) const override;

 private:
  char bits_key;
  int hash_size;
};
std::shared_ptr<Filter> NewBloomFilter(int bits);
}  // namespace yubindb
#endif