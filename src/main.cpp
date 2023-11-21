//
// Created by damdi on 19.11.2023.
//
#include "Promise.h"
#include <iostream>
Task<int> foo();

Task<int> foo() {
    co_return 42;
}

[[maybe_unused]] Task<int> bar(){
    const auto result = foo();
    const int i =  co_await result;
    std::cout<<i<<std::endl;
    co_return i+23;
}

int main(){
    auto result = bar();
    return 0;
}