// #ifndef YUBINDB_FILTER_H_
// #define YUBINDB_FILTER_H_
// #include <memory>

// #include "spdlog/spdlog.h"
// using string = folly::fbstring;

// namespace yubindb {

// class Filter {
//  public:
//   virtual ~Filter() = 0;
//   virtual void CreateFiler(const string* key, std::string* filter) const = 0;
//   virtual bool KeyMayMatch(const string& key, const string& filter) const = 0;
// };
// std::shared_ptr<Filter> NewBloomFilter(int bits);

// class BloomFilter : public Filter {
//  public:
//   explicit BloomFilter(int bits);
//   ~BloomFilter() = default;
//   //给长度为 n 的 key,创建一个过滤策略，并将策略序列化为 string ，追加到 dst
//   void CreateFiler(const string* key,
//                    std::string* filter) const override;  // d
//   bool KeyMayMatch(const string& key, const string& filter) const override;

//  private:
//   char bits_key;
//   int hash_size;
// };
// }  // namespace yubindb
// #endif