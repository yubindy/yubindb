#include <spdlog/spdlog.h>

#include <string>
#include <string_view>

#include "src/util/loger.h"
using namespace yubindb;
int main() {
  std::string p = "sdf";
  mlogger.Setlog("logs");
  mlog->error("sdfsf {} dasdds {}", p.data(),p);
  return 0;
}