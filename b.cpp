#include <bits/stdc++.h>
using namespace std;
int main() {
  std::string_view p("123456");
  std::string_view pp("asdfgg");
  pp = p;
  if (strcmp(p, (const char*)(pp.data()))) {
    printf("%s", pp.data());
  } else {
    printf("DDDD");
  }
  return 0;
}