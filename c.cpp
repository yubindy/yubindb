#include <stdint.h>
#include <stdio.h>
#include<iostream>
#include <string>
#include <string_view>
int main() {
  std::string p = "12345678";
  std::string_view cp=p;
  cp.remove_prefix(4);
  std::cout<< cp;
}