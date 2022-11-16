#include "src/util/bloom.h"
#include <string>
#include <string_view>
#include <vector>
int main(){
    std::vector<std::string_view> p;
    BloomFilter sp;
    for(int i=10;i<20;i++){
        p.emplace_back(std::to_string(i));
    }
    std::string rul;
    sp.CreateFiler(p.begin(),p.size(),rul);
    return 0;
}