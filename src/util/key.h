#ifndef YUBINDB_KEY_H_
#define YUBINDB_KEY_H_
#include <string.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

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
  explicit InternalKey(std::string&& Key_) : Key(Key_) {}
  InternalKey(InternalKey& key_) { Key = key_.Key; }
  InternalKey(const InternalKey& key_) { Key = key_.Key; }
  InternalKey() {}
  InternalKey(InternalKey&& str) { Key = str.Key; }
  InternalKey& operator=(const InternalKey& ptr) {
    Key = ptr.Key;
    return *this;
  }
  ~InternalKey() = default;
  const std::string_view getview() const { return std::string_view(Key); }
  const std::string& getString() { return Key; }
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

 private:
 friend class SkiplistKey;
  std::string Key;
};

inline int cmp(const InternalKey& a_,
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
    return std::string_view(start, end - start);
  }

  // Return an internal key (suitable for passing to an internal iterator)
  std::string&& internal_key() const {
    std::string ptr;
    ptr.resize(end - start);
    memcpy(ptr.data(), start, end - start);
    return std::move(ptr);
  }
  // Return the user key
  std::string_view key() const {
    return std::string_view(start, end - start - 8);
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
  void Getvalsize(uint32_t& val_size){
   uint32_t key_size;
    getsize(str, key_size);
    getsize(str + key_size + VarintLength(key_size), val_size);
  }
  void Val(std::string& value) const {
    uint32_t key_size;
    uint32_t val_size;
    getsize(str, key_size);
    getsize(str + key_size + VarintLength(key_size), val_size);
    memcpy(value.data(),
           str + VarintLength(key_size) + key_size + VarintLength(val_size),
           val_size);
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
