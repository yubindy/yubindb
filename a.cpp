#include <iostream>
#include <std:: string_view>
#include <string>

int main() {
  std::string cstr("yangxunwu");
  std::std::string_view stringView1(cstr.data());
  std::std::string_view stringView2(cstr.data(), 4);
  std::cout << "stringView1: " << stringView1
            << ", stringView2: " << stringView2 << std::endl;
  std::cout << "stringView1: " << stringView1.size()
            << ", stringView2: " << stringView2.data() << std::endl;
}