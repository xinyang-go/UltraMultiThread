//
// Created by xinyang on 2020/11/22.
//

#ifndef _UMT_SYNC_HPP_
#define _UMT_SYNC_HPP_

#include "ObjManager.hpp"
#include <mutex>
#include <chrono>
#include <condition_variable>

namespace umt {

    class SyncError : public std::runtime_error {
    protected:
        using std::runtime_error::runtime_error;
    };

    class SyncError_Empty : public SyncError {
    public:
        SyncError_Empty() : SyncError("empty sync. maybe uninitialized or moved.") {}
    };

    template<class T>
    class Sync {
    public:
        Sync() = default;

        explicit Sync(const std::string &name) {
            bind(name);
        }

        void bind(const std::string &name) {
            p_src = ObjManager<sync_src>::find_or_create(name);
        }

        void reset() {
            p_src.reset();
        }

        void set(const T &val) {
            if (!p_src) throw SyncError_Empty();
            std::unique_lock lock(p_src->mtx);
            p_src->val = val;
            p_src->cv.notify_all();
        }

        T get() {
            if (!p_src) throw SyncError_Empty();
            return p_src->val;
        }

        void wait(const T &v) const {
            if (!p_src) throw SyncError_Empty();
            std::unique_lock lock(p_src->mtx);
            p_src->cv.wait(lock, [&]() { return v == p_src->val; });
        }

        bool wait_for(const T &v, size_t ms) const {
            if (!p_src) throw SyncError_Empty();
            std::unique_lock lock(p_src->mtx);
            return p_src->cv.wait_for(lock, std::chrono::milliseconds(ms), [&]() { return v == p_src->val; });
        }

        template<class P>
        void wait_until(const T &v, P pt) const {
            if (!p_src) throw SyncError_Empty();
            std::unique_lock lock(p_src->mtx);
            return p_src->cv.wait_for(lock, pt, [&]() { return v == p_src->val; });
        }

    private:
        struct sync_src {
            T val;
            mutable std::mutex mtx;
            mutable std::condition_variable cv;
        };
        typename ObjManager<sync_src>::sptr p_src;
    };
}

#ifdef _UMT_WITH_BOOST_PYTHON_
#include <boost/python.hpp>


#define UMT_EXPORT_PYTHON_SYNC(type) do{        \
    using namespace umt;                        \
    using namespace boost::python;              \
    class_<Sync<type>>("Sync_"#type, init<>())  \
        .def(init<std::string>())               \
        .def("set", &Sync<type>::set)           \
        .def("get", &Sync<type>::get)           \
        .def("wait", &Sync<type>::wait)         \
        .def("wait_for", &Sync<type>::wait_for);\
}while(0)

#endif /* _UMT_WITH_BOOST_PYTHON_ */

#endif /* _UMT_SYNC_HPP_ */
