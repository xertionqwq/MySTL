#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <my_stl/core/alloc.h>

using namespace std;

// 使用裸内存分配器
using rawAlloc = MySTL::alloc<false, 0>;

void allocMemObj(size_t bytes) {
    cout << "allocating obj memory cost " << bytes << " bytes" << '\n';
}

// 打印地址
void printAddr(void *p) {
    cout << "raw mem : " << p << '\n';
}

// 实验一, 观察不同字节数是否来自同一地址
void test1() {
    rawAlloc alloc;
    // 分配地址
    for (int i = 1; i <= 16; i++) {
        size_t bytes = i * 8; // 以 8 字节为基础
        void *listHead = alloc.allocate(bytes);
        allocMemObj(bytes);
        printAddr(listHead);
        alloc.deAllocate(bytes, listHead);
    }
}

// 实验二, 验证地址复用
void test2() {
    rawAlloc alloc;
    // 重复分配 8 字节
    for (int i = 1; i <= 2; i++) {
        size_t bytes = 8; 
        void *listHead = alloc.allocate(bytes);
        allocMemObj(bytes);
        printAddr(listHead);
        alloc.deAllocate(bytes, listHead);
    }
}

// 实验三, 验证字节数强行上调至了 8 的倍数
void test3() {
    rawAlloc alloc;
    // 分配地址
    for (int i = 1; i <= 16; i++) {
        size_t bytes = i * 7; // 以 7 字节为基础
        void *listHead = alloc.allocate(bytes);
        allocMemObj(bytes);
        printAddr(listHead);
        alloc.deAllocate(bytes, listHead);
    }
}

// 实验四, 验证大字节对象地址差异
void test4() {
    rawAlloc alloc;
    for (int i = 1; i <= 2; i++) {
        void *listHead = alloc.allocate(i * 8);
        allocMemObj(i * 8);
        printAddr(listHead);
        alloc.deAllocate(i * 8, listHead);
    }
    void *biggerObj = alloc.allocate(129);
    allocMemObj(129);
    printAddr(biggerObj);
    alloc.deAllocate(129, biggerObj);
}

// 实验五, 统计一次批量分配实际调用了多少次 malloc
// 用法: 编译时不要加 -fsanitize=address (ASan 会拦截 malloc), 然后:
//   clang++ -std=c++17 -Wall -Wextra -g -I include experiment.cpp -o /tmp/exp02
//   LD_PRELOAD=bench/libmalloc_counter.so /tmp/exp02 malloc N
//
// 实测结果 (malloc_counter 的总数, 含 2 次进程/printf 基线):
//   N = 0        -> 2       (基线)
//   N = 1..40    -> 3       (+1: 首个 chunk 320B, 可切 40 块)
//   N = 41..83   -> 4       (+2: 第二个 chunk 344B, 可切 43 块)
//   N = 84..129  -> 5       (+3: 第三个 chunk 368B, 可切 46 块)
//   N = 130      -> 6
//   free 始终为 0: 小块 deAllocate 只挂回 free-list, 不归还系统
void test5(int n) {
    constexpr int kMax = 256;
    if (n < 0 || n > kMax) {
        std::printf("n must be in [0, %d]\n", kMax);
        return;
    }

    void *ptrs[kMax];
    for (int i = 0; i < n; ++i)
        ptrs[i] = rawAlloc::allocate(8); // 同尺寸, 走同一条 free-list

    std::printf("allocated %d blocks of 8 bytes\n", n);

    for (int i = 0; i < n; ++i)
        rawAlloc::deAllocate(8, ptrs[i]);
}

int main(int argc, char **argv) {
    // malloc 计数模式: ./experiment malloc [N]
    if (argc >= 2 && std::strcmp(argv[1], "malloc") == 0) {
        int n = (argc >= 3) ? std::atoi(argv[2]) : 41;
        test5(n);
        return 0;
    }

    /*
    allocating obj memory cost 8 bytes
    raw mem : 0x75bdce3e0040
    allocating obj memory cost 16 bytes
    raw mem : 0x75bdce3e00e0
    eo - 40 = 10 转十进制 -> 10 * 16 = 160b(十进制) = 8b * 20
    ** 我们只申请了一个对象, 但是却分配了 20 个对象
    */
    // test1();

    /*
    allocating obj memory cost 8 bytes
    raw mem : 0x6e39149e0040
    allocating obj memory cost 8 bytes
    raw mem : 0x6e39149e0040
    ** 可以看到再次申请同一字节数时, 地址完全一致, 说明自由链表得到复用
    */
    // test2();

    // test3();

    /*
    allocating obj memory cost 8 bytes
    raw mem : 0x6f841f7e0040
    allocating obj memory cost 16 bytes
    raw mem : 0x6f841f7e00e0
    allocating obj memory cost 129 bytes
    raw mem : 0x6f341f7e0040
    ** 第三条地址差异很大, 说明大对象不走自由链表分配
    */
    test4();

    cout << "return done" << endl;
}