#include <iostream>
#include <functional>
#include <string>
#include <string_view>
#include <string_view>

int main(int argc, char *argv[])
{
	std::string_view p("12345");
	if(p.starts_with("123")){
		printf("crazy\n");
	}
	size_t num;
	std::string sp(p);
	num = strtoull (p.data(), NULL, 0);
	if(num==12345){
		printf("sadsef");
	}
	if(p=="12345"){
	printf("%d",num);
	}
	return 0;
}
