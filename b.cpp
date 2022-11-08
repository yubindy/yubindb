#include <functional>
#include <iostream>
#include <map>
#include <memory_resource>
#include <stdio.h>
#include <string>
#include <string_view>
using namespace std::literals;

int main() {
  std::pair<int, int> p(6, 7);
  printf("%d %d", p.first, p.second);
  p = std::make_pair<int, int>(1, 23);
  printf("%d %d", p.first, p.second);
  return 0;
}