#include <stdio.h>
#include <stdint.h>
int main(){
  uint64_t num = 0xdb4775248b80fb57ull;
  uint32_t a,b;
  a=num>>32;
  b=num;
  uint64_t c=a;
  c=c<<32;
  c|=b;
  if(c==num){
    printf("true");
  }else{
    printf("false");
  }
  long ll;
  printf("%d",sizeof(ll));
  return 0;
}