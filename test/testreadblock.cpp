#include <string>

#include "gtest/gtest.h"
#include "src/db/db.h"
#include "src/util/common.h"
#include "src/util/loger.h"
using namespace yubindb;
TEST(testReadBlock, base) {
  yubindb::DB* db;
  yubindb::Options opt;
  yubindb::State s = yubindb::DBImpl::Open(opt, "/tmp/testdb", &db);
  assert(s.ok());

  // write key1,value1
  std::string key;
  std::string value;
  size_t p = 0;
  for (int i = 0; i < 200; i++) {
    key = "key";
    value = "value";
    key.append(std::to_string(i));
    value.append(std::to_string(i));
    s = db->Put(yubindb::WriteOptions(), key, value);
    EXPECT_TRUE(s.ok());
  }
  sleep(1);
  std::string value1;
  for (int i = 0; i < 200; i += 10) { //前面从imm get,然后从sstable get,最后从memtable读
    key = "key";
    value = "value";
    key.append(std::to_string(i));
    value.append(std::to_string(i));
    s = db->Get(yubindb::ReadOptions(), key, &value1);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(value, value1);
  }
  delete db;
  // std::string p;
  // PutFixed32(&p,32);
  // auto s=DecodeFixed32(p.data());
  // PutFixed32(&p,16);
}
// TEST(testReadTable, base) {}