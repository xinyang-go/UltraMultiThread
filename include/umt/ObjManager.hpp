//
// Created by xinyang on 2020/11/22.
//

#ifndef _UMT_OBJ_MANAGER_HPP_
#define _UMT_OBJ_MANAGER_HPP_

#include <mutex>
#include <memory>
#include <vector>
#include <unordered_map>


namespace umt {

    /**
     * @brief 用于导出protect构造函数为public，方便使用std::make_shared创建共享对象
     */
    template<class T>
    class ExportPublicConstructor : public T {
    public:
        template<class ...Ts>
        explicit ExportPublicConstructor(Ts &&...args): T(std::forward<Ts>(args)...) {}
    };

    /**
     * @brief 命名共享对象管理器
     * @details 通过对象类型和对象名称唯一确定一个共享对象（即std::shared_ptr）
     *          此对象管理器不可用于非class的基本类型（即无法用于int，double等类型）
     *          当导出对象管理器至python时，要求被管理对象类型必须有默认构造函数
     *          该对象下的所有函数满足线程安全性
     * @tparam T 被管理的对象类型
     */
    template<class T>
    class ObjManager : public T {
    public:
        using sptr = std::shared_ptr<T>;
        using wptr = std::weak_ptr<T>;

        /**
         * @brief 创建一个命名共享对象
         * @tparam Ts 对象的构造函数参数类型
         * @param name 对象的名称
         * @param args 对象的构造函数参数
         * @return 创建出的共享对象，如果该名称下已经存在一个共享对象，则返回nullptr
         */
        template<class ...Ts>
        static sptr create(const std::string &name, Ts &&...args) {
            std::unique_lock lock(_mtx);
            if (_map.find(name) != _map.end()) return nullptr;
            sptr p_obj = std::make_shared<ExportPublicConstructor<ObjManager<T>>>(name, std::forward<Ts>(args)...);
            _map.emplace(name, p_obj);
            return p_obj;
        }

        /**
         * @brief 查找一个命名共享对象
         * @param name 对象的名称
         * @return 查找到的共享对象，如果该名称下不存在一个共享对象，则返回nullptr
         */
        static sptr find(const std::string &name) {
            std::unique_lock lock(_mtx);
            auto iter = _map.find(name);
            if (iter == _map.end()) return nullptr;
            else return iter->second.lock();
        }

        /**
         * @brief 查找一个命名共享对象，如果不存在则将其创建
         * @tparam Ts 对象的构造函数参数类型
         * @param name 对象的名称
         * @param args 对象的构造函数参数
         * @return 查找或创建出的共享对象
         */
        template<class ...Ts>
        static sptr find_or_create(const std::string &name, Ts &&...args) {
            std::unique_lock lock(_mtx);
            auto iter = _map.find(name);
            if (iter != _map.end()) return iter->second.lock();
            sptr p_obj = std::make_shared<ExportPublicConstructor<ObjManager<T>>>(name, std::forward<Ts>(args)...);
            _map.emplace(name, p_obj);
            return p_obj;
        }

        static std::vector<std::string> names() {
            std::unique_lock lock(_mtx);
            std::vector<std::string> _names;
            for (const auto[n, w]: _map) {
                _names.emplace_back(n);
            }
            return _names;
        }

        /**
         * @brief 析构函数中，将该对象从map中删除
         */
        ~ObjManager() {
            std::unique_lock lock(_mtx);
            _map.erase(_name);
        }

    protected:
        /**
         * @brief protect构造函数，无法直接创建该类型的对象
         * @tparam Ts 对象的构造函数参数类型
         * @param name 对象的名称
         * @param args 对象的构造函数参数
         */
        template<class ...Ts>
        explicit ObjManager(std::string name, Ts &&...args): _name(std::move(name)), T(std::forward<Ts>(args)...) {}

    private:
        /// 当前对象名称
        std::string _name;

        /// 全局map互斥锁
        static std::mutex _mtx;
        /// 对象map，用于查找命名对象
        static std::unordered_map<std::string, wptr> _map;
    };

    template<class T>
    inline std::mutex ObjManager<T>::_mtx;

    template<class T>
    inline std::unordered_map<std::string, typename ObjManager<T>::wptr> ObjManager<T>::_map;
}

#ifdef _UMT_WITH_BOOST_PYTHON_

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

/// 导出一个类型的对象管理器至python（被管理的对象类型本身需要另外手动导出至python）。
#define UMT_EXPORT_PYTHON_OBJ_MANAGER(type) do{                                         \
    static_assert(std::is_default_constructible_v<type>,                                \
        "UMT_EXPORT_PYTHON_OBJ_MANAGER: type must has a default constructor");          \
    using namespace boost::python;                                                      \
    using namespace umt;                                                                \
    class_<ObjManager<type>>("ObjManager_"#type, no_init)                               \
        .def("create", &ObjManager<type>::create<>)                                     \
        .def("find", &ObjManager<type>::find)                                           \
        .def("find_or_create", &ObjManager<type>::find_or_create<>)                     \
        .def("names", &ObjManager<type>::names);                                        \
    register_ptr_to_python<std::shared_ptr<type>>();                                    \
    class_<std::vector<std::string>>("vector_string")                                   \
        .def(vector_indexing_suite<std::vector<std::string>>());                                                  \
}while(0)

#endif /* _UMT_WITH_BOOST_PYTHON_ */

#endif /* _UMT_OBJ_MANAGER_HPP_ */
