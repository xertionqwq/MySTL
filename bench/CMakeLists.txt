add_executable(bench_allocator bench_allocator.cpp)
target_link_libraries(bench_allocator PRIVATE my_stl)

# Benchmark: optimisation on, sanitizers off (last-wins overrides -fsanitize=address
# from parent CMAKE_CXX_FLAGS, both for compile and link)
target_compile_options(bench_allocator PRIVATE -O2 -fno-sanitize=all)
target_link_options(bench_allocator    PRIVATE -fno-sanitize=all)
