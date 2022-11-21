#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
void func(std::string_view *p){
	char buf[6]="fsefe";
	*p=std::string_view(buf,6);
	return;
}
int main(int argc, char *argv[])
{
	std::string_view p;
	func(&p);
	std::cout<<p<<std::endl;
	return 0;
}
