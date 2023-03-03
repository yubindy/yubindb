#include <assert.h>

#include <cstddef>
#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "src/db/db.h"
#include "src/util/common.h"

TEST(testrecover,recovermemtable) {
  yubindb::DB* db;
  yubindb::Options opt;
  yubindb::State s = yubindb::DBImpl::Open(opt, "/tmp/testdb", &db);
  assert(s.ok());

  // write key1,value1
  std::string key = "key";
  std::string value = "fdsfsd";
  std::string value1 = "fdsfsd";
  // recover
  std::string key2 = "key";
  s = db->Get(yubindb::ReadOptions(), "key0", &value);
  EXPECT_TRUE(s.IsNotFound());
  for (int i = 1; i < 10; i++) {
    key2.append(std::to_string(i));
    s = db->Get(yubindb::ReadOptions(), key2, &value);
    EXPECT_TRUE(s.ok());
    value1.append(std::to_string(i));
    EXPECT_EQ(value, value1);
  }
  EXPECT_TRUE(s.IsNotFound());
  delete db;
  assert(s.ok());
}
