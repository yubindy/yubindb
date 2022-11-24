#ifndef YUBINDB_KEY_H_
#define YUBINDB_KEY_H_
#include <string.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "common.h"
#include "skiplist.h"
namespace yubindb {
typedef uint64_t SequenceNum;
enum Valuetype {
  kTypeDeletion = 0X0,
  kTypeValue = 0x1,
};
class InternalKey;
static const Valuetype kValueTypeForSeek = kTypeValue;
using keycomper = std::function<bool(std::string_view, std::string_view)>;
static const SequenceNum kMaxSequenceNumber = ((0x1ull << 56) - 1);  // max seq
static uint64_t PackSequenceAndType(uint64_t seq, Valuetype t) {
  return (seq << 8) | t;
}
static void getsize(const char* ptr, uint32_t& p) {
  GetVarint32Ptr(ptr, ptr + 5, &p);
}
//    userkey  char[klength]
//    tag      uint64 ->seq+type
class InternalKey {
 public:
  explicit InternalKey(std::string_view str, SequenceNum num, Valuetype type) {
    Key.append(str.data(), str.size());
    PutFixed64(&Key, parser(num, type));
  }
  explicit InternalKey(std::string_view Key_) : Key(Key_.data()) {}
  explicit InternalKey(std::pair<const char*, const char*> pairs) {
    Key.resize(pairs.second - pairs.first);
    memcpy(Key.data(), pairs.first, pairs.second - pairs.first);
  }
  InternalKey(InternalKey& key_) { Key = key_.Key; }
  InternalKey(const InternalKey& key_) { Key = key_.Key; }
  InternalKey() {}
  InternalKey(InternalKey&& str) { Key = str.Key; }
  InternalKey& operator=(const InternalKey& ptr) {
    Key = ptr.Key;
    return *this;
  }
  ~InternalKey() = default;
  void clear() { return Key.clear(); }
  const std::string_view getview() const { return std::string_view(Key); }
  const std::string getString() { return Key; }
  uint64_t parser(SequenceNum num, Valuetype type);
  bool DecodeFrom(std::string_view s) {
    Key.assign(s.data(), s.size());
    return !Key.empty();
  }
  uint64_t Getag() { return DecodeFixed64(Key.data() + Key.size() - 8); }
  std::string_view ExtractUserKey() const {
    assert(Key.size() >= 8);
    return std::string_view(Key.data(), Key.size() - 8);
  }
  size_t size() { return Key.size(); }

 private:
  friend class SkiplistKey;
  std::string Key;
};

static inline int cmp(const InternalKey& a_,
                      const InternalKey& b_) {  // inernalkey cmp
  std::string_view a = a_.getview();
  std::string_view b = b_.getview();

  int r = a_.ExtractUserKey().compare(b_.ExtractUserKey());
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
struct ParsedInternalKey {
 public:
  std::string_view user_key;
  SequenceNum sequence;
  Valuetype type;

  ParsedInternalKey() {}  // Intentionally left uninitialized (for speed)
  ParsedInternalKey(std::string_view u, const SequenceNum& seq, Valuetype t)
      : user_key(u), sequence(seq), type(t) {}
  // std::string DebugString() const;
};
inline bool ParseInternalKey(std::string_view internal_key,
                             ParsedInternalKey* result) {
  const size_t n = internal_key.size();
  if (n < 8) return false;
  uint64_t num = DecodeFixed64(internal_key.data() + n - 8);
  uint8_t c = num & 0xff;
  result->sequence = num >> 8;
  result->type = static_cast<Valuetype>(c);
  result->user_key = std::string_view(internal_key.data(), n - 8);
  return (c <= static_cast<uint8_t>(kTypeValue));
}

// We construct a char array of the form:
//    klength  varint32               <-- start_
//    userkey  char[klength]          <-- kstart_
//    tag      uint64

class Lookey {
 public:
  Lookey(std::string_view key_, SequenceNum seq_);
  std::string_view usrkey();

  ~Lookey() = default;
  std::string_view Internalkey() const {
    return std::string_view(kstart, end - kstart);
  }

  std::pair<const char*, const char*> internal_key() const {
    std::pair<const char*, const char*> spt(kstart, end);
    return spt;
  }
  std::string_view inter_key() const {
    return std::string_view(kstart, end-kstart);
  }
  // Return the user key
  std::string_view key() const {
    return std::string_view(kstart, end - kstart - 8);
  }

 private:
  const char* start;   // all start
  const char* kstart;  // userkey start
  const char* end;
  char space[200];
};

// entry format is:
//    klength  varint32
//    userkey  char[klength]
//    tag      uint64 ->seq+type
//    vlength  varint32
//    value    char[vlength]
class SkiplistKey {  // for skiplist
 public:
  explicit SkiplistKey(const char* p) : str(p) {}
  ~SkiplistKey() = default;
  // InternalKey&& Key() const {
  //   uint32_t key_size;
  //   getsize(str, key_size);
  //   std::string p;
  //   p.resize(VarintLength(key_size));
  //   memcpy(p.data(),str + VarintLength(key_size), key_size);  //TODO '\0'
  //   return std::move(InternalKey(p));
  // }
  void Key(InternalKey& p) const {
    uint32_t key_size;
    getsize(str, key_size);
    p.Key.resize(key_size);
    memcpy(p.Key.data(), str + VarintLength(key_size), key_size);
  }
  void Val(std::string& value) const {
    uint32_t key_size;
    uint32_t val_size;
    getsize(str, key_size);
    getsize(str + key_size + VarintLength(key_size), val_size);
    value.resize(val_size);
    memcpy(value.data(), str + VarintLength(key_size) + key_size,
           val_size + VarintLength(val_size));
  }
  // void getInternalKey(std::string* key) const {
  //   uint32_t key_size;
  //   getsize(key_size);
  //   key->assign(str, VarintLength(key_size), key_size + 8);
  // }
  // void getValue(std::string* value) const {
  //   uint32_t key_size;
  //   getsize(key_size);
  //   uint32_t val_size;
  //   getsize(val_size);
  //   value->assign(str,
  //                 VarintLength(key_size) + key_size + VarintLength(val_size),
  //                 val_size);
  // }
  std::string_view gettrueview() { return std::string_view(str); }
  SkiplistKey& operator=(const SkiplistKey& a) {
    str = a.str;
    return *this;
  }

 private:
  const char* str;
};
}  // namespace yubindb
#endif
