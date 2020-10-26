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

    template<class T>
    class ExportPublicConstructor : public T {
    public:
        template<class ...Ts>
        explicit ExportPublicConstructor(Ts &&...args): T(std::forward<Ts>(args)...) {}
    };

    class NamedObjException : public std::logic_error {
    public:
        using std::logic_error::logic_error;
    };

    class NamedObjCreateException : public NamedObjException {
    public:
        explicit NamedObjCreateException(const std::string &name)
                : NamedObjException(fmt::format("Name '{}' already exist.", name)) {}
    };

    class NamedObjFindException : public NamedObjException {
    public:
        explicit NamedObjFindException(const std::string &name)
                : NamedObjException(fmt::format("Name '{}' do not exist.", name)) {}
    };

    template<class T>
    class NamedObj : public T {
    public:
        using ObjType = T;

        using sPtr = std::shared_ptr<ObjType>;
        using wPtr = std::weak_ptr<ObjType>;

        template<class ...Ts>
        static sPtr create(const std::string &name, Ts &&...args) {
            std::unique_lock lock(mtx_);
            if (map_.find(name) != map_.end()) throw NamedObjCreateException(name);
            sPtr obj = std::make_shared<ExportPublicConstructor<NamedObj>>(name, std::forward<Ts>(args)...);
            map_.emplace(name, obj);
            return obj;
        }

        static sPtr find(const std::string &name) {
            std::unique_lock lock(mtx_);
            auto iter = map_.find(name);
            if (iter == map_.end()) throw NamedObjFindException(name);
            const auto &wptr = iter->second;
            if (wptr.expired()) throw NamedObjException("Unexpected error: empty weak_ptr. This is an internal bug!");
            return wptr.lock();
        }

        template<class ...Ts>
        static sPtr find_or_create(const std::string &name, Ts &&...args) {
            try {
                return find(name);
            } catch (const NamedObjFindException &e) {
                return create(name, std::forward<Ts>(args)...);
            }
        }

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
