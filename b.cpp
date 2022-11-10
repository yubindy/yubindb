#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdio.h>
#include <string>
#include <string_view>
#include <vector>

int main() {
  std::unique_ptr<int> p = std::make_unique<int>(13);
  std::set<std::unique_ptr<int>> pp;
  pp.insert(std::move(p));
  for (auto& i : pp) {
    printf("%d", *i);
  }
  return 0;
}