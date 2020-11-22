#include "umt/umt.hpp"
#include <chrono>
#include <thread>
#include <iostream>

using namespace std::chrono;

struct msg_t {
    decltype(high_resolution_clock::now()) timestamp;
    std::string val;
};

/// 消息发布函数，每1ms发布一条消息
void publish() {
    msg_t msg;
    umt::Publisher<msg_t> pub("msg-0");
    for (int i = 0; i < 1000; i++) {
        msg.val = std::to_string(i);
        msg.timestamp = high_resolution_clock::now();
        pub.push(msg);
        std::this_thread::sleep_for(1ms);
    }
}

/// 消息订阅函数，收到消息后会打印当前消息从发布到接收之间的延迟时间
void subscribe() {
    msg_t msg;
    umt::Subscriber<msg_t> sub("msg-0", 1);
    while (true) {
        try {
            msg = sub.pop();
        } catch (const umt::MessageError_Stopped &) {
            break;
        }
        auto t = high_resolution_clock::now();
        auto dt = duration_cast<microseconds>(t - msg.timestamp).count();
        std::cout << "dt: " << dt << "us, val: " << msg.val << std::endl;
    }
}

/// 启动两个订阅线程，和一个发布线程
int main() {
    std::thread sub0(subscribe);
    std::thread sub1(subscribe);
    publish();
    sub0.join();
    sub1.join();
    return 0;
}
