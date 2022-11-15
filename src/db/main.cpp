#include <assert.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <string>

#include "../util/common.h"
#include "db.h"
#include "loginit.h"
#include "writebatch.h"
#define SPDLOG_ACTIVE_LEVEL \
  SPDLOG_LEVEL_TRACE  //必须定义这个宏,才能输出文件名和行号
int main() {
  yubindb::DB* db;
  yubindb::Options opt;
  spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][thread %t][%@,%!][%l] : %v");
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
  s = db->Put(yubindb::WriteOptions(),"key0","issb");
  assert(s.ok());
  std::string key1="key";
  for (int i = 0; i < 10; i++) {
    key1.append(std::to_string(i));
    s = db->Get(yubindb::ReadOptions(), key1, &value);
    assert(status.ok());
    std::cout << value << std::endl;
  }
  return 0;
}
