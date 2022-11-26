#include <string>
#include <string_view>
#include <vector>
#include "src/util/bloom.h"
#include "gtest/gtest.h"

TEST(testbloom, test0) {
  std::vector<int> start_{0, 1, 3, 7, 9, 10};
  std::string k{"asdfghjklzggt"};
  std::string kk{"fresgrtesgvsdv"};
  start_.emplace_back(k.size());
  std::vector<std::string_view> pp;
  yubindb::BloomFilter sp(10);
  for (int i = 0; i < start_.size() - 1; i++) {
    const char* p = k.data() + start_[i];
    size_t length = start_[i + 1] - start_[i];
    pp.emplace_back(std::string_view(p, length));
  }
  std::string rul;
  sp.CreateFiler(pp.data(), pp.size(), &rul);
  std::string_view ruls(rul.c_str(), rul.size());
  for (int i = 0; i < start_.size() - 1; i++) {
    const char* p = k.data() + start_[i];
    size_t length = start_[i + 1] - start_[i];
    EXPECT_TRUE(sp.KeyMayMatch(std::string_view(p, length), ruls));
    p = kk.data() + start_[i];
    EXPECT_FALSE(sp.KeyMayMatch(std::string_view(p, length), ruls));
  }
}