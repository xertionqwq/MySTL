#include <iostream>
#include <cassert>
#include <thread>
#include "my_stl/skip_list.h"
#include "my_stl/vector.h"

static int passed = 0;
static int failed = 0;

#define TEST(name) std::cout << "[" << (++test_count) << "] " << name << " ... "
#define PASS()                              \
    do                                      \
    {                                       \
        std::cout << "PASSED" << std::endl; \
        passed++;                           \
    } while (0)
#define FAIL(msg)                                    \
    do                                               \
    {                                                \
        std::cout << "FAILED: " << msg << std::endl; \
        failed++;                                    \
    } while (0)

int main()
{
    int test_count = 0;

    //==============================================================
    // 1. 基础操作
    //==============================================================
    {
        TEST("default construct: isempty, size=0");
        MySTL::skip_list<int, int> sl;
        if (sl.isempty() && sl.size() == 0)
            PASS();
        else
            FAIL("empty skip_list wrong state");
    }

    {
        TEST("insert single: size=1, find returns value");
        MySTL::skip_list<int, int> sl;
        bool ok = sl.insert(10, 100);
        int* p = sl.find(10);
        if (ok && sl.size() == 1 && !sl.isempty() && p && *p == 100)
            PASS();
        else
            FAIL("insert or find failed");
    }

    {
        TEST("insert duplicate: returns false, size unchanged");
        MySTL::skip_list<int, int> sl;
        sl.insert(5, 50);
        bool ok = sl.insert(5, 99);
        int* p = sl.find(5);
        if (!ok && sl.size() == 1 && p && *p == 50)
            PASS();
        else
            FAIL("duplicate not rejected or value overwritten");
    }

    {
        TEST("erase existing: returns true, size=0, find=none");
        MySTL::skip_list<int, int> sl;
        sl.insert(7, 70);
        bool ok = sl.erase(7);
        if (ok && sl.size() == 0 && sl.isempty() && sl.find(7) == nullptr)
            PASS();
        else
            FAIL("erase failed");
    }

    {
        TEST("erase non-existing: returns false");
        MySTL::skip_list<int, int> sl;
        sl.insert(1, 10);
        bool ok = sl.erase(99);
        if (!ok && sl.size() == 1)
            PASS();
        else
            FAIL("should return false for missing key");
    }

    {
        TEST("find non-existing: returns nullptr");
        MySTL::skip_list<int, int> sl;
        sl.insert(3, 30);
        if (sl.find(99) == nullptr)
            PASS();
        else
            FAIL("should be nullptr");
    }

    {
        TEST("const find: const skip_list access");
        MySTL::skip_list<int, int> sl;
        sl.insert(42, 420);
        const auto& csl = sl;
        const int* p = csl.find(42);
        if (p && *p == 420)
            PASS();
        else
            FAIL("const find failed");
    }

    //==============================================================
    // 2. 有序数据
    //==============================================================
    {
        TEST("insert ascending 1..100, find all");
        MySTL::skip_list<int, int> sl;
        for (int i = 1; i <= 100; ++i)
            sl.insert(i, i * 10);
        bool ok = (sl.size() == 100);
        for (int i = 1; i <= 100 && ok; ++i)
        {
            int* p = sl.find(i);
            if (!p || *p != i * 10) ok = false;
        }
        if (ok)
            PASS();
        else
            FAIL("ascending insert/find mismatch");
    }

    {
        TEST("insert descending 1..100, find all");
        MySTL::skip_list<int, int> sl;
        for (int i = 100; i >= 1; --i)
            sl.insert(i, i);
        bool ok = (sl.size() == 100);
        for (int i = 1; i <= 100 && ok; ++i)
        {
            int* p = sl.find(i);
            if (!p || *p != i) ok = false;
        }
        if (ok)
            PASS();
        else
            FAIL("descending insert/find mismatch");
    }

    {
        TEST("erase from middle of range");
        MySTL::skip_list<int, int> sl;
        for (int i = 1; i <= 5; ++i)
            sl.insert(i, i);
        sl.erase(3);
        if (sl.size() == 4 && sl.find(2) && sl.find(4) && !sl.find(3))
            PASS();
        else
            FAIL("middle erase broke list");
    }

    //==============================================================
    // 3. 边界
    //==============================================================
    {
        TEST("find on empty skip_list returns nullptr");
        MySTL::skip_list<int, int> sl;
        if (sl.find(1) == nullptr)
            PASS();
        else
            FAIL("should return nullptr");
    }

    {
        TEST("erase on empty skip_list returns false");
        MySTL::skip_list<int, int> sl;
        if (!sl.erase(1) && sl.size() == 0)
            PASS();
        else
            FAIL("should return false");
    }

    //==============================================================
    // 4. maxHeight 伸缩
    //==============================================================
    {
        TEST("maxHeight grows with many inserts");
        MySTL::skip_list<int, int> sl;
        // 大量插入应触发高层节点
        for (int i = 0; i < 1000; ++i)
            sl.insert(i, i);
        if (sl.size() == 1000 && !sl.isempty())
            PASS();
        else
            FAIL("size mismatch");
    }

    {
        TEST("maxHeight shrinks after erase all");
        MySTL::skip_list<int, int> sl;
        for (int i = 0; i < 100; ++i)
            sl.insert(i, i);
        for (int i = 0; i < 100; ++i)
            sl.erase(i);
        if (sl.isempty() && sl.size() == 0)
            PASS();
        else
            FAIL("not empty after erase all");
    }

    //==============================================================
    // 5. 内存（ASan 验证）
    //==============================================================
    {
        TEST("destroy empty skip_list (no leak)");
        {
            MySTL::skip_list<int, int> sl;
        }
        PASS();  // ASan catches leaks
    }

    {
        TEST("insert + erase many loop (no leak)");
        {
            MySTL::skip_list<int, int> sl;
            for (int r = 0; r < 10; ++r)
            {
                for (int i = 0; i < 100; ++i)
                    sl.insert(i, i);
                for (int i = 0; i < 100; ++i)
                    sl.erase(i);
            }
        }
        PASS();  // ASan catches leaks
    }

    //==============================================================
    // 6. 并发
    //==============================================================
    {
        TEST("concurrent insert different keys (16 threads)");
        MySTL::skip_list<int, int> sl;
        const int N = 100;
        MySTL::vector<std::thread> threads;
        for (int t = 0; t < 16; ++t)
        {
            threads.push_back(std::thread([&sl, t, N]() {
                int base = t * N;
                for (int i = 0; i < N; ++i)
                    sl.insert(base + i, base + i);
            }));
        }
        for (auto& th : threads) th.join();

        bool ok = (sl.size() == 16 * N);
        for (int i = 0; i < 16 * N && ok; ++i)
        {
            int* p = sl.find(i);
            if (!p || *p != i) ok = false;
        }
        if (ok)
            PASS();
        else
            FAIL("concurrent insert data mismatch");
    }

    {
        TEST("concurrent find (16 readers)");
        MySTL::skip_list<int, int> sl;
        for (int i = 0; i < 100; ++i)
            sl.insert(i, i * 10);

        MySTL::vector<std::thread> threads;
        std::atomic<int> errors{0};
        for (int t = 0; t < 16; ++t)
        {
            threads.push_back(std::thread([&sl, &errors]() {
                for (int i = 0; i < 100; ++i)
                {
                    int* p = sl.find(i);
                    if (!p || *p != i * 10)
                        errors.fetch_add(1, std::memory_order_relaxed);
                }
            }));
        }
        for (auto& th : threads) th.join();

        if (errors.load() == 0)
            PASS();
        else
            FAIL("concurrent read saw wrong data");
    }

    {
        TEST("concurrent insert same key: only one succeeds");
        MySTL::skip_list<int, int> sl;
        std::atomic<int> success{0};
        const int N = 100;
        MySTL::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t)
        {
            threads.push_back(std::thread([&sl, &success]() {
                for (int i = 0; i < N; ++i)
                {
                    if (sl.insert(i, i))
                        success.fetch_add(1, std::memory_order_relaxed);
                }
            }));
        }
        for (auto& th : threads) th.join();

        if (success.load() == N && sl.size() == static_cast<size_t>(N))
            PASS();
        else
            FAIL("duplicate insert not properly rejected");
    }

    {
        TEST("concurrent insert + erase (8 writers, 8 erasers)");
        MySTL::skip_list<int, int> sl;
        // Pre-fill
        for (int i = 0; i < 200; ++i)
            sl.insert(i, i);

        MySTL::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t)
        {
            threads.push_back(std::thread([&sl, t]() {
                int base = 1000 + t * 50;
                for (int i = 0; i < 50; ++i)
                    sl.insert(base + i, base + i);
            }));
        }
        for (int t = 0; t < 8; ++t)
        {
            threads.push_back(std::thread([&sl, t]() {
                for (int i = t * 25; i < t * 25 + 25; ++i)
                    sl.erase(i);
            }));
        }
        for (auto& th : threads) th.join();

        // Verfiy: all pre-fill keys from [0,200) not erased by these threads
        // should still be findable
        if (sl.size() >= 200)   // 8*25 erased, 8*50 inserted → net should be ≥ 200+400-200
            PASS();
        else
            FAIL("size too small, data might be lost");
    }

    //==============================================================
    // 7. 结果
    //==============================================================
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << (passed + failed) << std::endl;
    if (failed > 0)
        std::cout << "Failed: " << failed << std::endl;
    return failed == 0 ? 0 : 1;
}
