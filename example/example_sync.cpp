//
// Created by xinyang on 2020/11/22.
//

#include "umt/umt.hpp"
#include <chrono>
#include <thread>
#include <iostream>

using namespace std::chrono;

constexpr int wait_val = 666;

/// 根据sync状态向终端输出字符串，等待sync值为666
void sync_print() {
    auto sync = umt::Sync<int>("sync-0");
    while (true) {
        sync.wait(666);
        std::cout << "sync == 666" << std::endl;
        std::this_thread::sleep_for(1s);
    }
}

/// 根据用户输入设置sync的值
int main() {
    int num;
    auto sync = umt::Sync<int>("sync-0");
    std::thread(sync_print).detach();
    for (int i = 0; i < 10; i++) {
        std::cout << "input 666 to enable the sync, other number to disable the sync: ";
        std::cin >> num;
        sync.set(num);
    }
    return 0;
}
