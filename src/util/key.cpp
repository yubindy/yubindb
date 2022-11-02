#include "key.h"
namespace yubindb {
uint64_t InternalKey::parser(SequenceNum num, Valuetype type) {
  assert(num <= kMaxSequenceNumber);
  assert(type <= kTypeValue);
  return (num << 8) | type;
}
//| Size| User key (string) | sequence number (7 bytes) | value type
//(1 byte) |
Lookey::Lookey(std::string_view key_, SequenceNum seq_) {
  size_t usize = key_.size();
  size_t needed = usize + 13;
  char* str;

  if (needed <= sizeof(key_)) {
    str = space;
  } else {
    str = new char[needed];
  }
  start = str;
  interlen = usize + 8;
  // str = EncodeVarint32(str, usize + 8);
  std::memcpy(str, (char*)(&interlen), sizeof(interlen));
  str += sizeof(interlen);
  std::memcpy(str, key_.data(), usize);
  str += usize;
  EncodeFixed64(str,
                PackSequenceAndType(seq_, kValueTypeForSeek));  // 定长8字节
  str += 8;
  end = str;
}
uint64_t SkiplistKey::Getag() {
  const char* p = str + sizeof(interlen) + interlen;
  return DecodeFixed64(p);
}
}  // namespace yubindb