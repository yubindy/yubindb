#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
extern int p;
void func(std::string& p) { 
  std::string_view pp(p.data(),pp.size());
  printf("%s",pp.data());
  return;
}
#include "stdio.h"
int main() {
  std::string p("12345");
  func(p);
}