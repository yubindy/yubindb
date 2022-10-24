#include <iostream>
#include <string>
#include <string_view>

int main() {

  const char *cstr = "yangxunwu";
  std::string_view stringView1(cstr);
  std::string_view stringView2(cstr, 4);
  std::cout << "stringView1: " << stringView1
            << ", stringView2: " << stringView2 << std::endl;

  std::string str = "yangxunwu";
  std::string_view stringView3(str.c_str());
  std::string_view stringView4(str.c_str(), 4);
  std::cout << "stringView3: " << stringView1
            << ", stringView4: " << stringView2 << std::endl;
}