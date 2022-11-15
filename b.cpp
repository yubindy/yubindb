#include<stdio.h>
#include<string.h>
#include <memory>
#include<string>
#include<iostream>
int main() {
  
  std::unique_ptr<std::string>p;
  p=std::make_unique<std::string>();
  ::memcpy(p->data(),"1\0 ab",5);
  std::cout<<p->data() <<std::endl;
  printf("%s",p->data());
  return 0;
}