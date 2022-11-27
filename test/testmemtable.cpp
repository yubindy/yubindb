#include <assert.h>

#include <iostream>
#include <string>

#include "src/db/db.h"
#include "src/util/common.h"
int main() {
  yubindb::DB* db;
  yubindb::Options opt;
  yubindb::State s = yubindb::DBImpl::Open(opt, "/tmp/testdb", &db);
  assert(s.ok());

  // write key1,value1
  std::string key = "key";
  std::string value = "fdsfsd";
  for (int i = 0; i < 10; i++) {
    key.append(std::to_string(i));
    value.append(std::to_string(i));
    s = db->Put(yubindb::WriteOptions(), key, value);
    spdlog::info("K: {} V:{}", key, value);
  }
  s = db->Put(yubindb::WriteOptions(), "key0", "issb");
  assert(s.ok());
  std::string key1 = "key";
  for (int i = 0; i < 10; i++) {
    key1.append(std::to_string(i));
    s = db->Get(yubindb::ReadOptions(), key1, &value);
    assert(status.ok());
    std::cout << value << std::endl;
  }
  db->Delete(yubindb::WriteOptions(), "key0"); //iserror
  s = db->Get(yubindb::ReadOptions(), "key0", &value);
  assert(status.ok());
  std::cout << value << std::endl;
  delete db;
  return 0;
}
