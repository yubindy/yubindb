#include <iostream>
#include <functional>
#include <string>
#include <string_view>
#include <string_view>

int main(int argc, char *argv[])
{
	std::hash<std::string_view> h;
	std::string_view p("12344");
	uint64_t n = h(p[2]);
	std::cout << n << std::endl;

	return 0;
}
