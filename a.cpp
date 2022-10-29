#include <iostream>
#include <string>
#include <string_view>

int main() {
  uint32_t t = 85;
  uint32_t p = 85;
  t *= p / 100;
  printf("%d", t);
  return 0;
}