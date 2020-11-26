#include "umt/umt.hpp"
#include <iostream>

/// 查找一个已经存在的命名共享对象并修改其值
void func() {
    auto str = umt::ObjManager<std::string>::find("str-0");
    if (str) *str = "666";
}

/// 创建一个已经存在的命名共享对象，创建失败
void error0() {
    auto str = umt::ObjManager<std::string>::create("str-0");
    if (!str) std::cout << "create fail!" << std::endl;
}

/// 查找一个不存在的命名共享对象，查找失败
void error1() {
    auto str = umt::ObjManager<std::string>::find("str-1");
    if (!str) std::cout << "find fail!" << std::endl;
}

/// 遍历某个类型下的所有命名共享对象
void for_each() {
    for (const auto &name: umt::ObjManager<std::string>::names()) {
        auto str = umt::ObjManager<std::string>::find(name);
        // 由于多线程读写，命名对象不一定存在，需要判断find()函数返回值。
        if (str) {
            std::cout << name << ": " << *str << std::endl;
        }
    }
}

int main() {
    auto str = umt::ObjManager<std::string>::create("str-0", "Hello,World!");
    std::cout << "after create: [" << *str << "]" << std::endl;
    func();
    std::cout << "after func: [" << *str << "]" << std::endl;
    for_each();
    error0();
    error1();
    return 0;
}