//
// Created by xinyang on 2020/10/26.
//

#ifndef _UMT_NAMED_OBJ_HPP_
#define _UMT_NAMED_OBJ_HPP_

#include <fmt/format.h>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace umt {

    /**
     * @brief 辅助类，导出public构造函数
     * @tparam T 需要导出导出public构造函数的类型
     */
    template<class T>
    class ExportPublicConstructor : public T {
    public:
        template<class ...Ts>
        explicit ExportPublicConstructor(Ts &&...args): T(std::forward<Ts>(args)...) {}
    };

    /**
     * @brief 命名对象异常类型
     */
    class NamedObjException : public std::logic_error {
    public:
        using std::logic_error::logic_error;
    };

    /**
     * @brief 命名对象创建异常类型
     */
    class NamedObjCreateException : public NamedObjException {
    public:
        explicit NamedObjCreateException(const std::string &name)
                : NamedObjException(fmt::format("Name '{}' already exist.", name)) {}
    };

    /**
     * @brief 命名对象查找异常类型
     */
    class NamedObjFindException : public NamedObjException {
    public:
        explicit NamedObjFindException(const std::string &name)
                : NamedObjException(fmt::format("Name '{}' do not exist.", name)) {}
    };

    /**
     * @brief 共享命名对象类型
     * @details 类型和名称共同唯一确定一个共享对象。这意味着不同类型之间可以出现重名对象。
     * @tparam T 对象数据类型
     */
    template<class T>
    class NamedObj : public T {
    public:
        using ObjType = T;

        using sPtr = std::shared_ptr<ObjType>;
        using wPtr = std::weak_ptr<ObjType>;

        /**
         * @brief 创建共享命名对象。该对象已存在则会抛出异常。
         * @tparam Ts 创建对象时，构造函数的参数类型
         * @param name 对象名称
         * @param args 创建对象时，构造函数的参数
         * @return 创建好的共享命名对象
         */
        template<class ...Ts>
        static sPtr create(const std::string &name, Ts &&...args) {
            std::unique_lock lock(mtx_);
            if (map_.find(name) != map_.end()) throw NamedObjCreateException(name);
            sPtr obj = std::make_shared<ExportPublicConstructor<NamedObj>>(name, std::forward<Ts>(args)...);
            map_.emplace(name, obj);
            return obj;
        }

        /**
         * @brief 查找共享命名对象。该对象不存在则会抛出异常。
         * @param name 对象名称
         * @return 查找到的共享命名对象
         */
        static sPtr find(const std::string &name) {
            std::unique_lock lock(mtx_);
            auto iter = map_.find(name);
            if (iter == map_.end()) throw NamedObjFindException(name);
            const auto &wptr = iter->second;
            if (wptr.expired()) throw NamedObjFindException(name);
            return wptr.lock();
        }

        /**
         * @brief 尝试查找一个共享命名对象，如果不存在则创建它
         * @tparam Ts 创建对象时，构造函数的参数类型
         * @param name 对象名称
         * @param args 创建对象时，构造函数的参数类型
         * @return 查找到或创建好的共享命名对象
         */
        template<class ...Ts>
        static sPtr find_or_create(const std::string &name, Ts &&...args) {
            std::unique_lock lock(mtx_);
            auto iter = map_.find(name);
            if (iter == map_.end()) {
                sPtr obj = std::make_shared<ExportPublicConstructor<NamedObj>>(name, std::forward<Ts>(args)...);
                map_.emplace(name, obj);
                return obj;
            } else {
                const auto &wptr = iter->second;
                if (wptr.expired()) throw NamedObjFindException(name);
                return wptr.lock();
            }
        }

        /**
         * @brief 析构函数，将自己从对象列表中删除
         */
        ~NamedObj() {
            std::unique_lock lock(mtx_);
            map_.erase(name_);
        }

    protected:
        template<class ...Ts>
        explicit NamedObj(std::string name, Ts &&...args):T(std::forward<Ts>(args)...), name_(std::move(name)) {}

    private:
        std::string name_;

        static std::mutex mtx_;
        static std::unordered_map<std::string, wPtr> map_;
    };

    template<class T>
    inline std::mutex NamedObj<T>::mtx_;

    template<class T>
    inline std::unordered_map<std::string, typename NamedObj<T>::wPtr> NamedObj<T>::map_;
}

#endif /* _UMT_NAMED_OBJ_HPP_ */
