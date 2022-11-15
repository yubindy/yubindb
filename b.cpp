#include<stdio.h>
#include<string.h>
#include <algorithm>
#include <memory>
#include<string>
#include<iostream>
struct p{
  p()=default;
  ~p()=default;
  int a;
  std::string b;
};
int main() {
  
  std::string a,b;
  a="1234";
  std::string c=std::move(a);
  std::cout<< c<<std::endl;
}
// int main(){
//   std::string* p;
//   p=new std::string();
//   p->resize(5);
//   ::memcpy(p->data(),"1\0456",5);
//   std::cout<< 
// }