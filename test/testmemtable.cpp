#include <assert.h>
#include <benchmark/benchmark.h>

#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "src/db/db.h"
#include "src/util/common.h"

TEST(testmemtable, test0) {
  yubindb::DB* db;
  yubindb::Options opt;
  yubindb::State s = yubindb::DBImpl::Open(opt, "/tmp/testdb", &db);
  assert(s.ok());

  // write key1,value1
  std::string key = "key";
  std::string value = "fdsfsd";
  std::string value1 = "fdsfsd";
  for (int i = 0; i < 10; i++) {
    key.append(std::to_string(i));
    value.append(std::to_string(i));
    s = db->Put(yubindb::WriteOptions(), key, value);
    EXPECT_TRUE(s.ok());
  }
  assert(s.ok());
  std::string key1 = "key";
  for (int i = 0; i < 10; i++) {
    key1.append(std::to_string(i));
    s = db->Get(yubindb::ReadOptions(), key1, &value);
    EXPECT_TRUE(s.ok());
    value1.append(std::to_string(i));
    EXPECT_EQ(value, value1);
  }
  db->Delete(yubindb::WriteOptions(), "key0");  // iserror
  s = db->Get(yubindb::ReadOptions(), "key0", &value);
  EXPECT_TRUE(s.IsNotFound());
  delete db;
}
