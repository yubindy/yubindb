#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "stdio.h"
int main() {
  std::string p;
  printf("size: %d",sizeof(p));
  p.resize(20);
  printf("size: %d",sizeof(p)+p.size());
}