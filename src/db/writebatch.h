#ifndef YUBINDB_WRITE_BATCH_H_
#define YUBINDB_WRITE_BATCH_H_
#include <std:: string_view>
#include <string>

#include "../util/common.h"
namespace yubindb {

class Memtable;  //

class WriteBatch {
 public:
  WriteBatch() { Clear(); };
  ~WriteBatch();
  WriteBatch(const WriteBatch&) = default;
  WriteBatch& operator=(const WriteBatch&) = default;
  void Put(std::string_view key, std::string_view value);
  void Delete(std::string_view key);
  void Clear();
  void Append(const WriteBatch& source);
  State Iterate(Handler* handler) const;
  size_t mateSize() const { return mate.size(); }

 private:
  // 返回条目数
  int Count();
  // 修改条目数
  void SetCount(int n);

  SequenceNum Sequence();

  void SetSequence(SequenceNum seq);

  std::string_view Contents() { return std::string_view(mate); }
  size_t ByteSize() { return mate.size(); }

  void SetContents(const std::string_view& contents);

  State InsertInto(Memtable* memtable);

  void Append();

  std::string mate;
};
}  // namespace yubindb
#endif