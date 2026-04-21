# 生成可执行文件
add_executable(test_allocator test_allocator.cpp)

# 链接接口库
target_link_libraries(test_allocator PRIVATE my_stl)

# 添加测试
add_test(NAME test_allocator COMMAND test_allocator)