#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
extern int p;
void func() { p = 13; }
#include "stdio.h"
int main() {
  static int p = 12;
  printf("size: %d", p);
  func();
  printf("size: %d", p);
}