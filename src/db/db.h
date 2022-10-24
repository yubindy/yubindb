#ifndef YUBINDB_CACHE_H_
#define YUBINDB_CACHE_H_
#include "../util/common.h"
#include "../util/options.h"
#include <stdio.h>

class DB {
public:
  explicit DB() {}
  virtual ~DB() = 0;
  virtual Statue open() = 0;

private:
};
Class DBimpl : public DB {}

#endif