#include <assert.h>
#include <iostream>
#include <string>

#include "../util/common.h"
#include "db.h"
#include "writebatch.h"
int main() {
  yubindb::DB* db;
  yubindb::Options opt;
  yubindb::State s = yubindb::DBImpl::Open(opt, "/tmp/testdb", &db);
  assert(s.ok());

  // write key1,value1
  std::string key = "key";
  std::string value = "value";

  s = db->Put(yubindb::WriteOptions(), key, value);
  assert(s.ok());
  return 0;
}
