#include <deque>
#include <memory.h>
#include <stdio.h>
#include <string_view>
#include <thread>
#include <unistd.h>
class P {
 public:
  P() {
    int b = 17;
    a = &b;
  }
  ~P() = default;
  void fun() {
    int s = *(int*)a;
    while (1) {
      printf("s");
    }
  }

 private:
  void* a;
};
int main() {
  int s = 14;
  P ap;
  std::thread p(&P::fun, &ap);
  p.detach();
  ::sleep(10);
  return 0;
}