#include <assert.h>

#include <cstddef>
#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "src/db/db.h"
#include "src/util/common.h"
#include "src/util/loger.h"
using namespace yubindb;
TEST(testmemtable, test0) {
  yubindb::DB* db;
  yubindb::Options opt;
  yubindb::State s = yubindb::DBImpl::Open(opt, "/tmp/testdb", &db);
  assert(s.ok());

  // write key1,value1
  std::string key = "key0";
  std::string value = "value0";
  std::string value1 = "value0";
  size_t p = 0;
  for (int i = 0; i < 200; i++) {
    s = db->Put(yubindb::WriteOptions(), key, value);
    key.replace(3, std::to_string(i).size(), std::to_string(i));
    value.replace(5, std::to_string(i).size(), std::to_string(i));
    p += key.size() + value.size()+8+VarintLength(key.size())+VarintLength(value.size());
    EXPECT_TRUE(s.ok());
  }
  mlog->info("All size = {}", p);
  delete db;
}
