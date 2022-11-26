#include <spdlog/spdlog.h>

#include <string>
#include <string_view>
int main() {
  std::string p = "sdf";
  spdlog::error("sdfsf {}", p);
  return 0;
}