

#include <stdio.h>
#include <stdlib.h>

#include <functional>
#include <string>
#include <string_view>

int main() {
  std::string p("12345");
  int s=100;
  auto sp = std::hash<int>()(s);
  printf("%d", sp);
  return 0;
}