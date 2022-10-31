#ifndef YUBINDB_KEY_H_
#define YUBINDB_KEY_H_
#include <cstddef>
#include <cstdint>
#include <string.h>
#include <string>
#include <string_view>

#include "common.h"
#include "key.h"
#include "skiplist.h"
namespace yubindb {
typedef uint64_t SequenceNum;
enum Valuetype {
  kTypeDeletion = 0X0,
  kTypeValue = 0x1,
};
using keycomper = std::function<bool(std::string_view, std::string_view)>;
static const SequenceNum kMaxSequenceNumber = ((0x1ull << 56) - 1);  // max seq
static std::string_view ExtractUserKey(std::string_view internal_key) {
  assert(internal_key.size() >= 8);
  return std::string_view(internal_key.data(), internal_key.size() - 8);
}
inline int cmp(std::string_view a, std::string_view b) {  // inernalkey cmp
  int r = strcmp(ExtractUserKey(a).data(), ExtractUserKey(b).data());
  if (r == 0) {
    const uint64_t anum = DecodeFixed64(a.data() + a.size() - 8);
    const uint64_t bnum = DecodeFixed64(b.data() + b.size() - 8);
    if (anum > bnum) {
      r = -1;
    } else if (anum < bnum) {
      r = +1;
    }
  }
  return r;
}
class InternalKey {
 public:
  explicit InternalKey(std::string_view str, SequenceNum num, Valuetype type) {
    Key.append(str.data(), str.size());
    PutFixed64(&Key, parser(num, type));
  }
  ~InternalKey();
  std::string_view getview() { return std::string_view(Key); }
  uint64_t parser(SequenceNum num, Valuetype type);

 private:
  std::string Key;
};
// entry format is:
//    klength  size_t
//    userkey  char[klength]
//    tag      uint64 ->seq+type
//    vlength  varint32
//    value    char[vlength]
class SkiplistKey {  // for skiplist
 public:
  explicit SkiplistKey(char* p, size_t intersizelen_)
      : str(p), interlen(intersizelen_) {}
  ~SkiplistKey() = default;
  std::string_view getview() {
    return std::string_view(str + sizeof(interlen), interlen);
  }
  std::string_view gettrueview() { return std::string_view(str); }
  SkiplistKey& operator=(const SkiplistKey& a) { str = a.str; }

 private:
  char* str;
  size_t interlen;
};
static bool compar(const InternalKey& a, const InternalKey& b) {}
class LookupKey {
 public:
 private:
};
}  // namespace yubindb
#endif