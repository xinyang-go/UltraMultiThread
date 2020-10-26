#include <iostream>
#include <thread>
#include <chrono>
#include "umt/umt.hpp"

using namespace std::chrono;

// 多线程cout互斥量
std::mutex mtx;

// 定义消息数据类型
using time_point_t = decltype(high_resolution_clock::now());
using msg_t = std::tuple<int, time_point_t>;

// 发布者线程
void publisher() {
    // 创建一个发布器，消息数据类型为msg_t
    umt::Publisher<msg_t> p_tmp;
    // 发布器绑定到名称为"time-0"的消息对象，如果对象不存在则会创建对象
    p_tmp.bind("time-0");

    // 发布器类型支持移动构造，但不支持拷贝构造
    umt::Publisher<msg_t> p0 = std::move(p_tmp);

    // 循环发布100次消息
    for (int i = 0; i < 100; i++) {
        std::this_thread::sleep_for(1ms);
        auto t1 = high_resolution_clock::now();
        // 发布一条消息
        p0.push(msg_t{i, t1});
        auto t2 = high_resolution_clock::now();
        {
            std::unique_lock lock(mtx);
            std::cout << i << " push cost: " << duration_cast<microseconds>(t2 - t1).count() << "us" << std::endl;
            std::cout.flush();
        }
    }
    {
        std::unique_lock lock(mtx);
        std::cout << "end publisher" << std::endl;
        std::cout.flush();
    }
}

// 订阅者线程
void subscriber() {
    // sleep 10ms 确保发布者线程已经开始运行
    std::this_thread::sleep_for(10ms);

    // 创建一个订阅器，消息数据类型为msg_t
    umt::NormalSub<msg_t> s_tmp;
    // 订阅器绑定到名称为"time-0"的消息对象，如果该对象不存在则会抛出异常
    s_tmp.bind("time-0");

    // 订阅器类型支持移动构造，但不支持拷贝构造
    umt::NormalSub<msg_t> s0 = std::move(s_tmp);

    msg_t msg;
    time_point_t t2;
    for (int i = 0; i < 100; i++) {
        // 读取一条消息，当前无消息则线程休眠。返回值表示是否读取成功
        auto r = s0.pop_for(msg, 0.1s);
        if (r == umt::TIMEOUT) {
            std::unique_lock lock(mtx);
            std::cout << "timeout" << std::endl;
            std::cout.flush();
        } else if (r == umt::STOPPED) {
            std::unique_lock lock(mtx);
            std::cout << "stopped" << std::endl;
            std::cout.flush();
        } else {
            auto[id, t1] = msg;
            t2 = high_resolution_clock::now();
            std::unique_lock lock(mtx);
            std::cout << id << " delay: " << duration_cast<microseconds>(t2 - t1).count() << "us" << std::endl;
            std::cout.flush();
        }
    }
    {
        std::unique_lock lock(mtx);
        std::cout << "end subscriber" << std::endl;
        std::cout.flush();
    }
}

int main() {
    std::thread p(publisher);
    std::thread s[4];
    for (auto &i : s) {
        i = std::thread(subscriber);
    }
    p.join();
    for (auto &i:s) {
        i.join();
    }

    return 0;
}
