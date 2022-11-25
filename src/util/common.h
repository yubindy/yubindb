#ifndef YUBINDB_COMMON_H_
#define YUBINDB_COMMON_H_
#include <memory.h>

#include <cstddef>
#include <string_view>

#include "spdlog/spdlog.h"
#define SPDLOG_ACTIVE_LEVEL \
  SPDLOG_LEVEL_TRACE  //必须定义这个宏,才能输出文件名和行号
namespace yubindb {
typedef uint64_t SequenceNum;
typedef uint32_t uint32_t;
static std::shared_ptr<spdlog::logger> log; //log

//定长 fix
//非定长 varint
//编码
void PutFixed32(std::string* dst, uint32_t value);
void PutFixed64(std::string* dst, uint64_t value);
void PutVarint32(std::string* dst, uint32_t value);
void PutVarint64(std::string* dst, uint64_t value);
void PutLengthPrefixedview(
    std::string* dst,
    std::string_view value);  // dst： 长度(编码后) + value

//解码
bool GetVarint32(std::string_view* input, uint32_t* value);
bool GetVarint64(std::string_view* input, uint64_t* value);
bool GetLengthPrefixedview(std::string_view* input, std::string_view* result);

//解码变长整形最基础函数。
const char* GetVarint32Ptr(const char* p, const char* limit, uint32_t* v);
const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* v);

//获取编码后长度
int VarintLength(uint64_t v);

// 编码变长整形的最基础函数。
char* EncodeVarint32(char* dst, uint32_t value);
char* EncodeVarint64(char* dst, uint64_t value);

inline void EncodeFixed32(char* dst, uint32_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);
  std::memcpy(buffer, &value, sizeof(uint32_t));
  return;
}

inline void EncodeFixed64(char* dst, uint64_t value) {
  uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);
  std::memcpy(buffer, &value, sizeof(uint64_t));
  return;
}

inline uint32_t DecodeFixed32(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);

  uint32_t result;
  std::memcpy(&result, buffer, sizeof(uint32_t));
  return result;
}

inline uint64_t DecodeFixed64(const char* ptr) {
  const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);
  uint64_t result;
  std::memcpy(&result, buffer, sizeof(uint64_t));
  return result;
}

const char* GetVarint32PtrFallback(const char* p, const char* limit,
                                   uint32_t* value);
inline const char* GetVarint32Ptr(const char* p, const char* limit,
                                  uint32_t* value) {
  if (p < limit) {
    uint32_t result = *(reinterpret_cast<const uint8_t*>(p));
    if ((result & 128) == 0) {  //字节最高 bit 为0，说明编码结束。
      *value = result;
      return p + 1;
    }
  }
  return GetVarint32PtrFallback(p, limit, value);
}
bool GetLengthPrefixed(std::string_view* input, std::string_view* result);
const char* GetLengthPrefixed(const char* p, const char* limit,
                              std::string_view* result);
const char* GetLengthPrefixedview(const char* p, const char* limit,
                                  std::string_view* result);
bool GetLengthPrefixedview(std::string_view* input, std::string_view* result);
class State {
  enum Code {
    kok = 0,
    knotfound = 1,
    kcorruption = 2,
    knotsupported = 3,
    kinvalidargument = 4,
    kioerror = 5
  };

 public:
  State() : state_(kok) {}
  State(Code ptr) : state_(ptr) {}
  ~State() = default;

  State(const State& rhs) { state_ = rhs.code(); }
  State& operator=(const State& rhs) {
    state_ = rhs.code();
    return *this;
  }

  State(State&& rhs) noexcept : state_(rhs.state_) {}
  static State Ok() { return State(); }
  static State Notfound() { return State(knotfound); }
  static State Corruption() { return State(kcorruption); }
  static State NotSupported() { return State(knotsupported); }
  static State InvalidArgument() { return State(kinvalidargument); }
  static State IoError() { return State(kioerror); }
  bool ok() const { return (state_ == kok); }

  bool IsNotFound() const { return state_ == knotfound; }

  bool IsCorruption() const { return state_ == kcorruption; }

  bool IsIOError() const { return state_ == kioerror; }

  bool IsNotSupportedError() const { return state_ == knotsupported; }

  bool IsInvalidArgument() const { return state_ == kinvalidargument; }
  std::string ToString() const;

 private:
  Code code() const { return state_; }

  Code state_;
};
}  // namespace yubindb
#endif