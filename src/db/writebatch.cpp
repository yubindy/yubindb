#include "writebatch.h"
namespace yubindb {
//前8字节是该WriteBatch的SequenceNumber，后4字节是该WriteBatch中Entry的数量
static const size_t Headsize = 12;
void WriteBatch::Clear() {
  mate.clear();
  mate.resize(Headsize);
}
void WriteBatch::Put(std::string_view key, std::string_view value) {
  SetCount(Count() + 1);
}

void WriteBatch::Delete(std::string_view key) {}

void WriteBatch::Append(WriteBatch& source) {
  SetCount(source.Count() + Count());
  mate.append(source.mate.data() + Headsize);
}

int WriteBatch::Count() { return DecodeFixed32(mate.data() + 8); }

void WriteBatch::SetSequence(SequenceNum seq) {
  return EncodeFixed64(mate, seq);
}

void WriteBatch::SetContents(const std::string_view& contents) {
  assert(contents.size() > Headsize);
  mate.assign(mate, contents);
}

State WriteBatch::InsertInto(Memtable* memtable) {}
}  // namespace yubindb