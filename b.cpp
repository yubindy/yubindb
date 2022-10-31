#include <bits/stdc++.h>

int main() {
  auto p = std::make_shared<int>(12);
  auto s = &p;
  printf("%d", s->get());
  return 0;
}