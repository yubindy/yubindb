#include<stdio.h>
#include<string.h>
#include<string>
#include<iostream>
int main() {
  char* b="\0\0\012345";
  std::string p;
  p.resize(8);
  memcpy(p.data(),b,8);
  printf("%s %d",p.data(),p.size());
  std::cout<<p<<std::endl;
  return 0;
}