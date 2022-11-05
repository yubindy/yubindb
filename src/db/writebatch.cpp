#include "writebatch.h"

#include "../util/key.h"
#include "memtable.h"
#include "spdlog/spdlog.h"
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

void WriteBatch::SetCount(int n) { EncodeFixed32(&mate[8], n); }
void WriteBatch::SetSequence(SequenceNum seq) {
  EncodeFixed64(mate.data(), seq);
}

State WriteBatch::InsertInto(Memtable* memtable) {
  std::string_view ptr(mate);
  SequenceNum now_seq = Sequence();
  int now_cnt = Count();
  if (ptr.size() < Headsize) {
    spdlog::error("malformed WriteBatch (too small)");
  }
  ptr.remove_prefix(Headsize);
  std::string_view key, value;
  char type;
  while (!ptr.empty()) {
    now_cnt++;
    type = ptr[0];
    ptr.remove_prefix(1);
    switch (type) {
      case kTypeValue:
        if (GetLengthPrefixedview(&ptr, &key) &&
            GetLengthPrefixedview(&ptr, &value)) {
          memtable->Add(now_seq, kTypeValue, key, value);
        } else {
          spdlog::error("bad WriteBatch Put");
          return State::Corruption("bad WriteBatch Put");
        }
      case kTypeDeletion:
        if (GetLengthPrefixedview(&ptr, &key)) {
          memtable->Add(now_seq, kTypeValue, key, std::string_view());
        } else {
          spdlog::error("bad WriteBatch Del");
          return State::Corruption("bad WriteBatch Del");
        }
        break;
      default:
        return State::Corruption("unknown WriteBatch type");
    }
  }
  if (now_cnt != Count()) {
    spdlog::error("WriteBatch has wrong count has {} should {}", now_cnt,
                  Count());
    return State::Corruption("WriteBatch has wrong count");
  } else {
    return State::Ok();
  }
}
}  // namespace yubindb