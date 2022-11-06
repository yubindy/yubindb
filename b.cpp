#include <deque>
#include <memory.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>

void fun(void* p) {
  int s = *(int*)p;
  while (1) {
    printf("p");
  }
}
int main() {
  int s = 14;
  std::thread p(fun, &s);
  p.detach();
  ::sleep(10);
  return 0;
}