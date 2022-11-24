#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "stdio.h"
int main() {
        std::shared_ptr<int> p=std::static_pointer_cast<std::shared_ptr<int>>(std::make_shared<uint64_t>(14));
        return
}