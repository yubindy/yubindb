#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "stdio.h"
int main() {
  std::vector<int> start_{1, 2, 3, 4, 5, 6};
  std::string keys_{"asdfghjklz"};
  start_.emplace_back(keys_.size());
  std::vector<std::string_view> tmp_keys_;
  for (int i = 0; i < start_.size(); i++) {
    const char* p = keys_.data() + start_[i];
    size_t length = start_[i + 1] - start_[i];
    tmp_keys_.emplace_back(std::string_view(p, length));
  }
  int p=0;
  return 0;
}