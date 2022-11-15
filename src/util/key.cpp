#include "key.h"
namespace yubindb {
uint64_t InternalKey::parser(SequenceNum num, Valuetype type) {
  assert(num <= kMaxSequenceNumber);
  assert(type <= kTypeValue);
  return (num << 8) | type;
}
//| Size| User key (string) | sequence number (7 bytes) | value type
//(1 byte) |

Lookey::Lookey(std::string_view key_, SequenceNum seq_) { //ISerror should fix by internal key
  size_t usize = key_.size();
  size_t needed = usize + 13;
  char* str;

  if (needed <= sizeof(key_)) {
    str = space;
  } else {
    str = new char[needed];
  }
  start = str;
  start = str;
  str = EncodeVarint32(str, usize + 8);
  kstart = str;
  memcpy(str, key_.data(), usize);
  str += usize;
  EncodeFixed64(str, PackSequenceAndType(seq_, kTypeValue));
  str += 8;
  end = str;
}

}  // namespace yubindb