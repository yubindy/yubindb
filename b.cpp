#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdio.h>
#include <string>
#include <string_view>

int main() {
  std::string_view p("123456");
  std::string_view pp(p.substr(0, 4));
  std::string_view sp("123");
  std::cout << p << std::endl;
  std::cout << pp.compare(sp) << std::endl;
  return 0;
}