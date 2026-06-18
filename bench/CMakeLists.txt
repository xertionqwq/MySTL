add_executable(bench_allocator bench_allocator.cpp)
target_link_libraries(bench_allocator PRIVATE my_stl)
if(WIN32)
    target_link_libraries(bench_allocator PRIVATE psapi)
endif()

# Benchmark: optimisation on, sanitizers off (last-wins overrides -fsanitize=address
# from parent CMAKE_CXX_FLAGS, both for compile and link)
target_compile_options(bench_allocator PRIVATE -O2 -fno-sanitize=all)
target_link_options(bench_allocator    PRIVATE -fno-sanitize=all)

# --- skip_list concurrency benchmark ---
add_executable(bench_skip_list bench_skip_list.cpp)
target_link_libraries(bench_skip_list PRIVATE my_stl)
target_compile_options(bench_skip_list PRIVATE -O2 -fno-sanitize=all)
target_link_options(bench_skip_list    PRIVATE -fno-sanitize=all)

# Link threads (std::thread, std::shared_mutex)
find_package(Threads REQUIRED)
target_link_libraries(bench_skip_list PRIVATE Threads::Threads)
