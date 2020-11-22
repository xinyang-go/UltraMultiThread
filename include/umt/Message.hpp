//
// Created by xinyang on 2020/11/22.
//

#ifndef _UMT_MESSAGE_HPP_
#define _UMT_MESSAGE_HPP_

#include "ObjManager.hpp"
#include <condition_variable>
#include <chrono>
#include <queue>
#include <list>

namespace umt {

    class MessageError : public std::runtime_error {
    protected:
        using std::runtime_error::runtime_error;
    };

    class MessageError_Stopped : public MessageError {
    public:
        MessageError_Stopped() : MessageError("no publisher on this message!") {}
    };

    class MessageError_Timeout : public MessageError {
    public:
        MessageError_Timeout() : MessageError("message read timeout!") {}
    };

    class MessageError_Empty : public MessageError {
    public:
        MessageError_Empty() : MessageError("empty message. maybe uninitailized or moved!") {}
    };

    template<class T>
    class Publisher;

    template<class T>
    class Subscriber;

    template<class T>
    class MessagePipe {
        friend class Publisher<T>;

        friend class Subscriber<T>;

    public:
        using MsgType = T;
    private:
        std::mutex pubs_mtx;
        std::list<Publisher<T> *> pubs;
        std::mutex subs_mtx;
        std::list<Subscriber<T> *> subs;
    };

    template<class T>
    class Subscriber {
        friend Publisher<T>;
    private:
        using MsgManager = ObjManager<MessagePipe<T>>;
    public:
        using MsgType = T;

        Subscriber() = default;

        explicit Subscriber(const std::string &msg_name, size_t max_fifo_size = 0) {
            bind(msg_name, max_fifo_size);
        }

        Subscriber(const Subscriber &other) : fifo_size(other.fifo_size), fifo(other.fifo), p_msg(other.p_msg) {
            std::unique_lock subs_lock(p_msg->subs_mtx);
            p_msg->subs.emplace_front(this);
        }

        Subscriber(Subscriber &&other) noexcept: fifo_size(other.fifo_size), fifo(std::move(other.fifo)),
                                                 p_msg(other.p_msg) {
            other.reset();
            std::unique_lock subs_lock(p_msg->subs_mtx);
            p_msg->subs.emplace_front(this);
        }

        ~Subscriber() { reset(); }

        void reset() {
            if (!fifo.empty()) fifo = std::queue<T>();
            if (!p_msg) return;
            std::unique_lock subs_lock(p_msg->subs_mtx);
            p_msg->subs.remove(this);
            p_msg.reset();
        }

        void bind(const std::string &msg_name, size_t max_fifo_size = 0) {
            reset();
            fifo_size = max_fifo_size;
            p_msg = MsgManager::find_or_create(msg_name);
            std::unique_lock subs_lock(p_msg->subs_mtx);
            p_msg->subs.emplace_front(this);
        }

        T pop() {
            if (!p_msg) throw MessageError_Empty();
            std::unique_lock lock(mtx);
            cv.wait(lock, [this]() { return p_msg->pubs.empty() || !fifo.empty(); });
            if (p_msg->pubs.empty()) throw MessageError_Stopped();
            T tmp = std::move(fifo.front());
            fifo.pop();
            return tmp;
        }

        T pop_for(size_t ms) {
            if (!p_msg) throw MessageError_Empty();
            using namespace std::chrono;
            std::unique_lock lock(mtx);
            if (!cv.wait_for(lock, milliseconds(ms), [this]() { return p_msg->pubs.empty() || !fifo.empty(); })) {
                throw MessageError_Timeout();
            }
            if (p_msg->pubs.empty()) throw MessageError_Stopped();
            T tmp = std::move(fifo.front());
            fifo.pop();
            return tmp;
        }

        template<class P>
        T pop_until(P pt) {
            if (!p_msg) throw MessageError_Empty();
            std::unique_lock lock(mtx);
            if (!cv.wait_until(lock, pt, [this]() { return p_msg->pubs.empty() || !fifo.empty(); })) {
                throw MessageError_Timeout();
            }
            if (p_msg->pubs.empty()) throw MessageError_Stopped();
            T tmp = std::move(fifo.front());
            fifo.pop();
            return tmp;
        }

    private:
        void write_obj(const T &obj) {
            std::unique_lock lock(mtx);
            if (fifo_size > 0 && fifo.size() >= fifo_size) {
                fifo.pop();
            }
            fifo.push(obj);
        }

        void notify() const {
            cv.notify_one();
        }

    private:
        mutable std::mutex mtx;
        mutable std::condition_variable cv;
        size_t fifo_size{};
        std::queue<T> fifo;
        typename MsgManager::sptr p_msg;
    };

    template<class T>
    class Publisher {
    private:
        using MsgManager = ObjManager<MessagePipe<T>>;
    public:
        using MsgType = T;

        Publisher() = default;

        explicit Publisher(const std::string &msg_name) {
            bind(msg_name);
        }

        Publisher(const Publisher &other) : p_msg(other.p_msg) {
            std::unique_lock pubs_lock(p_msg->pubs_mtx);
            p_msg->pubs.emplace_front(this);
        }

        Publisher(Publisher &&other) noexcept: p_msg(other.p_msg) {
            other.reset();
            std::unique_lock pubs_lock(p_msg->pubs_mtx);
            p_msg->pubs.emplace_front(this);
        }

        ~Publisher() { reset(); }

        void reset() {
            if (!p_msg) return;
            std::unique_lock pubs_lock(p_msg->pubs_mtx);
            p_msg->pubs.remove(this);
            if (p_msg->pubs.empty()) {
                std::unique_lock subs_lock(p_msg->subs_mtx);
                for (const auto &sub: p_msg->subs) {
                    sub->notify();
                }
            }
            p_msg.reset();
        }

        void bind(const std::string &msg_name) {
            reset();
            p_msg = MsgManager::find_or_create(msg_name);
            std::unique_lock pubs_lock(p_msg->pubs_mtx);
            p_msg->pubs.emplace_front(this);
        }

        void push(const T &obj) {
            if (!p_msg) throw MessageError_Empty();
            std::unique_lock subs_lock(p_msg->subs_mtx);
            for (auto &sub: p_msg->subs) {
                sub->write_obj(obj);
                sub->notify();
            }
        }

    private:
        typename MsgManager::sptr p_msg;
    };
}

#ifdef _UMT_WITH_BOOST_PYTHON_

#include <boost/python.hpp>

#define UMT_EXPORT_PYTHON_MASSAGE(type) do{                 \
    using namespace umt;                                    \
    using namespace boost::python;                          \
    class_<Subscriber<type>>("Subscriber_"#type, init<>())  \
        .def(init<std::string>())                           \
        .def(init<std::string, size_t>())                   \
        .def("reset", &Subscriber<type>::reset)             \
        .def("bind", &Subscriber<type>::bind)               \
        .def("pop", &Subscriber<type>::pop)                 \
        .def("pop_for", &Subscriber<type>::pop_for);        \
    class_<Publisher<type>>("Publisher_"#type, init<>())    \
        .def(init<std::string>())                           \
        .def("reset", &Publisher<type>::reset)              \
        .def("bind", &Publisher<type>::bind)                \
        .def("push", &Publisher<type>::push);               \
}while(0)

#endif /* _UMT_WITH_BOOST_PYTHON_ */

#endif /* _UMT_MESSAGE_HPP_ */
