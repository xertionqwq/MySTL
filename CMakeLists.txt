# 最低版本要求
cmake_minimum_required(VERSION 3.15)

project(My_STL VERSION 1.0.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -fsanitize=address")
endif()

# 接口库
add_library(my_stl INTERFACE)

# 添加 include 目录（build tree + install tree）
include(GNUInstallDirs)
target_include_directories(my_stl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ---- 安装规则 ----
install(TARGETS my_stl EXPORT my_stlTargets)
install(DIRECTORY include/my_stl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT my_stlTargets
    NAMESPACE MySTL::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/my_stl
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/my_stlConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/my_stlConfig.cmake"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/my_stlTargets.cmake\")\n"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/my_stlConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/my_stlConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/my_stl"
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)
endif()