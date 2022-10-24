#ifndef YUBINDB_CACHE_H_
#define YUBINDB_CACHE_H_
#include "spdlog/spdlog.h"
#include <cstddef>
#include <memory.h>
namespace yubindb {

class State {
public:
  State() : state_(nullptr) {}
  ~State();

  State(const State &rhs);
  State &operator=(const State &rhs);

  State(State &&rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  static State Ok() { return State(); }
  static State Notfound {return }
private:
  enum Code {
    kok = 0,
    knotfound = 1,
    kcorruption = 2,
    knotsupported = 3,
    kinvalidargument = 4,
    kioerror = 5
  };
  Code code() const {
    return (state_ == nullptr) ? kok : static_cast<Code>(state_[4]);
  }
  const char *state_;
}

} // namespace yubindb
#endif