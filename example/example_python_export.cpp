//
// Created by xinyang on 2020/11/22.
//

#include "umt/umt.hpp"
#include <thread>
#include <chrono>
#include <iostream>

using namespace std::chrono;
namespace bpy = boost::python;

struct foo {
    foo() = default;

    foo(int x, int y) : x(x), y(y) {}

    int x, y;

    bool operator==(const foo &other) const { return x == other.x && y == other.y; }
};

/// 消息订阅函数，接收到消息之后输出收到的消息
void subscribe() {
    foo f{};
    umt::Subscriber<foo> sub("msg-foo-0");
    while (true) {
        try {
            f = sub.pop();
        } catch (const umt::MessageError &) {
            continue;
        }
        std::cout << "get: f={.x=" << f.x << ", .y= " << f.y << "}" << std::endl;
    }
}

/// 等待同步sync值为77，当同步sync值为77时，每隔1s打印共享对象的值
void sync_print() {
    auto p_foo = umt::ObjManager<foo>::find_or_create("obj-foo-0");
    umt::Sync<int> sync("sync-0");
    while (true) {
        sync.wait(77);
        std::cout << "p_foo={.x=" << p_foo->x << ", .y=" << p_foo->y << "}" << std::endl;
        std::this_thread::sleep_for(1s);
    }
}

/**
 * 用malloc申请了空间，需要在外面释放
 */
wchar_t *to_wchar(const char *p_src) {
    wchar_t *p_dest;
    int len = 0;

    len = strlen(p_src) + 1;
    if (len <= 1) return nullptr;

    p_dest = new wchar_t[len];

    /*这里的len应该为宽字符长度，而非源字符串的字节长度，但字节长度肯定大于宽字符长度，因此暂且用之*/
    mbstowcs(p_dest, p_src, len);

    return p_dest;
}

/// 启动上面两个线程，同时导出相应函数和类到python。
int main(int argc, char *argv[]) {
    std::thread(subscribe).detach();
    std::thread(sync_print).detach();

    wchar_t *w_argv[16];
    for (int i = 0; i < argc; i++) {
        w_argv[i] = to_wchar(argv[i]);
    }

    Py_Initialize();
    try {
        auto main_module = bpy::import("__main__");
        bpy::scope main_scope(main_module);
        bpy::class_<foo>("foo", bpy::init<>())
                .def(bpy::init<int, int>())
                .def_readwrite("x", &foo::x)
                .def_readwrite("y", &foo::y);
        UMT_EXPORT_PYTHON_OBJ_MANAGER(foo);
        UMT_EXPORT_PYTHON_MASSAGE(foo);
        UMT_EXPORT_PYTHON_SYNC(int);
    } catch (const bpy::error_already_set &) {
        PyErr_Print();
        return -1;
    }

    return Py_Main(argc, w_argv);
}
