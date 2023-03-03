#ifndef YUBINDB_LOGER_H
#define YUBINDB_LOGER_H

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>

namespace yubindb {
class Loger {
 public:
  static Loger& Logger();
  Loger() = default;
  void Setlog(const std::string& dbname);
  std::shared_ptr<spdlog::logger> logger;
};
#define mlogger Loger::Logger()
#define mlog Loger::Logger().logger
}  // namespace yubindb
#endif