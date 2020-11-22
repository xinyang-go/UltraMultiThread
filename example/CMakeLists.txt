project(example)

add_executable(example_obj_manager example_obj_manager.cpp)
target_link_libraries(example_obj_manager ${umt_LIBS})

add_executable(example_message example_message.cpp)
target_link_libraries(example_message ${umt_LIBS})

add_executable(example_sync example_sync.cpp)
target_link_libraries(example_sync ${umt_LIBS})

if (Boost_FOUND AND Python3_FOUND)
    message("-- python example")
    add_executable(example_python_export example_python_export.cpp)
    target_link_libraries(example_python_export ${umt_LIBS})
endif ()