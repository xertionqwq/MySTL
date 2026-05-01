# 生成可执行文件
add_executable(test_allocator test_allocator.cpp)
add_executable(test_vector test_vector.cpp)
add_executable(test_skip_list test_skip_list.cpp)

# 链接接口库
target_link_libraries(test_allocator PRIVATE my_stl)
target_link_libraries(test_vector PRIVATE my_stl)
target_link_libraries(test_skip_list PRIVATE my_stl)

# 线程支持 (std::thread)
find_package(Threads REQUIRED)
target_link_libraries(test_skip_list PRIVATE Threads::Threads)

# 添加测试
add_test(NAME test_allocator COMMAND test_allocator)