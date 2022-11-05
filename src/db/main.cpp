#include <assert.h>
#include <iostream>
#include <string.h>

#include "db.h"

int main() {
  yubindb::DB* db;
  yubindb::Options options;
  options.create_if_missing = true;
  yubindb::State s = yubindb::DB::Open(options, "/tmp/testdb", &db);
  assert(s.ok());

  // write key1,value1
  std::string key = "key";
  std::string value = "value";

  s = db->Put(yubindb::WriteOptions(), key, value);
  assert(s.ok());
  return 0; 
}