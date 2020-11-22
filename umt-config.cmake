find_package(Threads REQUIRED)
set(umt_LIBS Threads::Threads)
set(umt_INCLUDE_DIR include)

find_package(Boost COMPONENTS python3)
find_package(Python3 COMPONENTS Interpreter Development)
if (Boost_FOUND AND Python3_FOUND)
    message("-- with boost.python")
    list(APPEND umt_LIBS Boost::python3 Python3::Python)
    add_compile_definitions(_UMT_WITH_BOOST_PYTHON_)
else ()
    message("-- without boost.python")
endif ()

