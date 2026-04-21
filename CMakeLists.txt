# 最低版本要求
cmake_minimum_required(VERSION 3.15)

project(My_STL)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 禁用编译器优化

# 全局编译警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -fsanitize=address")

# 接口库
add_library(my_stl INTERFACE)

# 添加include目录
target_include_directories(my_stl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# 添加测试
enable_testing(
)
add_subdirectory(tests)