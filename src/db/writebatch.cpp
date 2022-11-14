#include "writebatch.h"

#include "../util/key.h"
#include "memtable.h"
#include "spdlog/spdlog.h"
namespace yubindb {
//前8字节是该WriteBatch的SequenceNumber，后4字节是该WriteBatch中Entry的数量
static const uint32_t Headsize = 12;
void WriteBatch::Clear() {
  mate.clear();
  mate.resize(Headsize);
}
void WriteBatch::Put(std::string_view key, std::string_view value) {
  SetCount(Count() + 1);
  mate.push_back(static_cast<char>(kTypeValue));
  PutLengthPrefixedview(&mate, key);
  PutLengthPrefixedview(&mate, value);
}

void WriteBatch::Delete(std::string_view key) {}

void WriteBatch::Append(WriteBatch* source) {
  SetCount(source->Count() + Count());
  mate.append(source->mate.data() + Headsize);
}

int WriteBatch::Count() { return DecodeFixed32(mate.data() + 8); }

void WriteBatch::SetCount(int n) { EncodeFixed32(&mate[8], n); }
void WriteBatch::SetSequence(SequenceNum seq) {
  EncodeFixed64(mate.data(), seq);
}

State WriteBatch::InsertInto(std::shared_ptr<Memtable> memtable) {
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
          spdlog::debug("memtable add Seq:{} Type:{} Key:{} Value:{}", now_seq,
                        kTypeValue, key, value);
        } else {
          spdlog::error("bad WriteBatch Put");
          return State::Corruption("bad WriteBatch Put");
        }
      case kTypeDeletion:
        if (GetLengthPrefixedview(&ptr, &key)) {
          memtable->Add(now_seq, kTypeValue, key, std::string_view());
          spdlog::debug("memtable add Seq:{} Type:{} Key:{} Value:{}", now_seq,
                        kTypeDeletion, key, value);
        } else {
          spdlog::error("bad WriteBatch Del");
          return State::Corruption("bad WriteBatch Del");
        }
        break;
      default:
        spdlog::error("unknown WriteBatch type");
        return State::Corruption("unknown WriteBatch type");
    }
    seq++;
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