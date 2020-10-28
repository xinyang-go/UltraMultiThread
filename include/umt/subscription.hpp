//
// Created by xinyang on 2020/10/26.
//

#ifndef _UMT_SUBSCRIPTION_HPP_
#define _UMT_SUBSCRIPTION_HPP_

#include "named_obj.hpp"
#include <condition_variable>
#include <list>
#include <queue>

namespace umt {

    /**
     * @brief 订阅状态枚举
     */
    enum SubscriptionStatus {
        OK, TIMEOUT, STOPPED
    };

    template<class T>
    class Publisher;

    template<class T>
    class Subscriber;

    /**
     * @details 订阅器异常类型
     */
    class SubscriberException : public std::logic_error {
    public:
        using std::logic_error::logic_error;
    };

    /**
     * @details 发布器异常类型
     */
    class PublisherException : public std::logic_error {
    public:
        using std::logic_error::logic_error;
    };

    /**
     * @brief 消息管道类型
     * @details 关联一个消息上的发布者和订阅者
     * @tparam T 消息数据类型
     */
    template<class T>
    class MessagePipe {
        friend class Publisher<T>;

        friend class Subscriber<T>;

    private:
        std::mutex subs_mtx_;
        std::list<Subscriber<T> *> subs_;
        std::mutex pubs_mtx_;
        std::list<Publisher<T> *> pubs_;
    };

    /**
     * @brief 订阅器类型基类接口类
     * @details 订阅器通过消息类型和消息名称唯一确定一个消息对象。
     * @tparam T
     */
    template<class T>
    class Subscriber {
        friend class Publisher<T>;

    protected:
        Subscriber() = default;

        explicit Subscriber(const std::string &name) {
            Subscriber::bind(name);
        }

        Subscriber(const Subscriber &sub) = delete;

        Subscriber &operator=(const Subscriber &sub) = delete;

        Subscriber(Subscriber &&other) noexcept {
            if (!other.pipe_) return;
            {
                std::unique_lock lock(other.pipe_->subs_mtx_);
                *std::find(other.pipe_->subs_.begin(), other.pipe_->subs_.end(), &other) = this;
            }
            pipe_ = std::move(other.pipe_);
        }

        Subscriber &operator=(Subscriber &&other) noexcept {
            Subscriber::reset();
            if (!other.pipe_) return *this;
            {
                std::unique_lock lock(other.pipe_->subs_mtx_);
                *std::find(other.pipe_->subs_.begin(), other.pipe_->subs_.end(), &other) = this;
            }
            pipe_ = std::move(other.pipe_);
            return *this;
        }

    public:
        using DataType = T;

        virtual ~Subscriber() {
            Subscriber::reset();
        }

        /**
         * @brief 消息订阅器绑定到一个消息对象
         * @param name 消息名称
         */
        virtual void bind(const std::string &name) {
            reset();
            pipe_ = NamedObj<MessagePipe<T>>::find(name);
            std::unique_lock lock(pipe_->subs_mtx_);
            pipe_->subs_.emplace_back(this);
        }

        /**
         * @brief 消息订阅器从一个消息对象上解绑
         */
        virtual void reset() {
            if (!pipe_) return;
            std::unique_lock lock(pipe_->subs_mtx_);
            pipe_->subs_.remove(this);
            pipe_.reset();
        }

        /**
         * @brief 读取一条消息
         * @details 尝试读取一条消息，如果当前没有已缓存的消息，则线程永久休眠。如果该消息上没有发布者，将直接返回错误。
         * @param[out] obj 保存读取到的数据
         * @return 读取状态
         */
        SubscriptionStatus pop(DataType &obj) {
            if (!pipe_) throw SubscriberException("empty subscriber");
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [&]() { return available() || pipe_->pubs_.empty(); });
            if (!available()) return STOPPED;
            read(obj);
            return OK;
        }

        /**
         * @brief 读取一条消息
         * @details 尝试读取一条消息，如果当前没有已缓存的消息，则线程休眠，可指定最大休眠时间。如果该消息上没有发布者，将直接返回错误。
         * @tparam Duration 时间间隔类型
         * @param[out] obj 保存读取到的数据
         * @param dt 最大休眠时间
         * @return 读取状态
         */
        template<class Duration>
        SubscriptionStatus pop_for(DataType &obj, const Duration &dt) {
            if (!pipe_) throw SubscriberException("empty subscriber");
            std::unique_lock lock(mtx_);
            auto r = cv_.wait_for(lock, dt, [&]() { return available() || pipe_->pubs_.empty(); });
            if (!r) return TIMEOUT;
            if (!available()) return STOPPED;
            read(obj);
            return OK;
        }

        /**
         * @brief 读取一条消息
         * @details 尝试读取一条消息，如果当前没有已缓存的消息，则线程休眠，可指定休眠唤醒时间点。如果该消息上没有发布者，将直接返回错误。
         * @tparam TimePoint 时间点类型
         * @param[out] obj 保存读取到的数据
         * @param tp 休眠唤醒时间点
         * @return 读取状态
         */
        template<class TimePoint>
        SubscriptionStatus pop_until(DataType &obj, const TimePoint &tp) {
            if (!pipe_) throw SubscriberException("empty subscriber");
            std::unique_lock lock(mtx_);
            auto r = cv_.wait_until(lock, tp, [&]() { return available() || pipe_->pubs_.empty(); });
            if (!r) return TIMEOUT;
            if (!available()) return STOPPED;
            read(obj);
            return OK;
        }

    protected:
        /**
         * @brief 判断当前是否有可读的消息
         * @details 此函数不能为纯虚函数。因为该对象析构时，需要等子类析构完毕后才会调用当前类的析构函数，并将该订阅器解绑。
         *          导致可能出现发布器调用析构到一半的订阅器的函数的情况，如果此函数为纯虚函数，则调用会出错。
         * @return true则可读
         */
        virtual bool available() const { return false; };

        /**
         * @brief 读出一条消息
         * @details 此函数不能为纯虚函数。因为该对象析构时，需要等子类析构完毕后才会调用当前类的析构函数，并将该订阅器解绑。
         *          导致可能出现发布器调用析构到一半的订阅器的函数的情况，如果此函数为纯虚函数，则调用会出错。
         * @param[out] obj 保存读取到的消息
         */
        virtual void read(DataType &obj) {};

        /**
         * @brief 写入一条消息
         * @details 此函数不能为纯虚函数。因为该对象析构时，需要等子类析构完毕后才会调用当前类的析构函数，并将该订阅器解绑。
         *          导致可能出现发布器调用析构到一半的订阅器的函数的情况，如果此函数为纯虚函数，则调用会出错。
         * @param obj 待写入的消息
         */
        virtual void write(const DataType &obj) {};

    protected:
        /**
         * @brief 发布一条消息给订阅器
         * @param obj 待发布的消息
         */
        void notify(const DataType &obj) {
            std::unique_lock lock(mtx_);
            write(obj);
            cv_.notify_one();
        }

        std::mutex mtx_;
        std::condition_variable cv_;
        typename NamedObj<MessagePipe<DataType>>::sPtr pipe_;
    };

    /**
     * @brief 常规订阅器
     * @details 使用容器缓存接受到的消息，有最大缓存上限。
     * @tparam T 消息数据类型
     * @tparam SIZE 最大缓存上限，为0则无上限
     * @tparam C 容器类型
     */
    template<class T, size_t SIZE = 0, class C=std::queue<T>>
    class NormalSub : public Subscriber<T> {
    public:
        using DataType = T;
        using ContainerType = C;
        static constexpr size_t CacheSize = SIZE;

        using Subscriber<DataType>::Subscriber;

        void bind(const std::string &name) override {
            {
                std::unique_lock lock(this->mtx_);
                cache_ = ContainerType();
            }
            Subscriber<DataType>::bind(name);
        }

        void reset() override {
            Subscriber<DataType>::reset();
            {
                std::unique_lock lock(this->mtx_);
                cache_ = ContainerType();
            }
        }

    protected:
        bool available() const override {
            return !cache_.empty();
        }

        void read(DataType &obj) override {
            obj = std::move(cache_.front());
            cache_.pop();
        }

        void write(const DataType &obj) override {
            cache_.push(obj);
            if constexpr (CacheSize == 0) return;
            if (cache_.size() > CacheSize) {
                cache_.pop();
            }
        }

    private:
        ContainerType cache_;
    };

    template<class T>
    class Publisher {
    public:
        using DataType = T;

        Publisher() = default;

        explicit Publisher(const std::string &name) {
            bind(name);
        }

        Publisher(const Publisher &sub) = delete;

        Publisher &operator=(const Publisher &sub) = delete;

        Publisher(Publisher &&other) noexcept {
            if (!other.pipe_) return;
            std::unique_lock lock(other.pipe_->pubs_mtx_);
            *std::find(other.pipe_->pubs_.begin(), other.pipe_->pubs_.end(), &other) = this;
            pipe_ = std::move(other.pipe_);
        }

        Publisher &operator=(Publisher &&other) noexcept {
            reset();
            if (!other.pipe_) return *this;
            std::unique_lock lock(other.pipe_->pubs_mtx_);
            *std::find(other.pipe_->pubs_.begin(), other.pipe_->pubs_.end(), &other) = this;
            pipe_ = std::move(other.pipe_);
            return *this;
        }

        ~Publisher() {
            reset();
        }

        /**
         * @brief 消息发布器绑定到一个消息对象
         * @param name 消息名称
         */
        void bind(const std::string &name) {
            reset();
            pipe_ = NamedObj<MessagePipe<T>>::find_or_create(name);
            std::unique_lock lock(pipe_->pubs_mtx_);
            pipe_->pubs_.emplace_back(this);
        }

        /**
         * @brief 消息发布器从一个消息对象上解绑
         */
        void reset() {
            if (!pipe_) return;
            {
                std::unique_lock lock(pipe_->pubs_mtx_);
                pipe_->pubs_.remove(this);
            }
            if (pipe_->pubs_.empty()) {
                std::unique_lock lock(pipe_->subs_mtx_);
                for (auto *sub : pipe_->subs_) {
                    sub->cv_.notify_all();
                }
            }
            pipe_.reset();
        }

        /**
         * @brief 发布一条消息
         * @param obj 待发布的消息对象
         */
        void push(const DataType &obj) {
            if (!pipe_) throw PublisherException("empty publisher");
            std::unique_lock lock(pipe_->subs_mtx_);
            for (auto *p_sub: pipe_->subs_) {
                p_sub->notify(obj);
            }
        }

    private:
        typename NamedObj<MessagePipe<DataType>>::sPtr pipe_;
    };
}
#endif /* _UMT_SUBSCRIPTION_HPP_ */
