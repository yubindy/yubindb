#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "stdio.h"
struct pub {
  pub() { printf("init\n"); }
  ~pub() { printf("desondy\n"); }
}; int main(int argc, char* argv[]) {
	std::shared_ptr<pub> pb=std::make_shared<pub>();
	std::shared_ptr<pub> pb1=pb;
	pb=nullptr;
	printf("end\n");
	return 0;
}
