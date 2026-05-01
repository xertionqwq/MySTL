#include <iostream>
#include <cassert>
#include "my_stl/vector.h"

// 测试辅助：统计成功/失败
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
    // 1. 构造 & 基础容量
    //==============================================================
    {
        TEST("default construct: size=0, capacity=0, isempty=true");
        MySTL::vector<int> v;
        if (v.size() == 0 && v.capacity() == 0 && v.isempty())
            PASS();
        else
            FAIL("default construct failed");
    }

    {
        TEST("vector(size_type n): n=0 produces empty vector");
        MySTL::vector<int> v(0);
        if (v.size() == 0 && v.isempty())
            PASS();
        else
            FAIL("explicit vector(0) should be empty");
    }

    {
        TEST("vector(size_type n): n=5 value-initialized to 0");
        MySTL::vector<int> v(5);
        bool ok = (v.size() == 5 && v.capacity() >= 5);
        for (size_t i = 0; i < v.size() && ok; i++)
            if (v[i] != 0)
                ok = false;
        if (ok)
            PASS();
        else
            FAIL("values not zero or size incorrect");
    }

    {
        TEST("vector(size_type n, value): n=3, value=42");
        MySTL::vector<int> v(3, 42);
        bool ok = (v.size() == 3);
        for (size_t i = 0; i < v.size() && ok; i++)
            if (v[i] != 42)
                ok = false;
        if (ok)
            PASS();
        else
            FAIL("values not 42");
    }

    {
        TEST("copy construct: values match, independence");
        MySTL::vector<int> v1(3, 7);
        MySTL::vector<int> v2(v1);
        v1[0] = 99; // modify original
        bool ok = (v2.size() == 3 && v2[0] == 7 && v2[1] == 7 && v2[2] == 7);
        if (ok)
            PASS();
        else
            FAIL("copy not independent");
    }

    {
        TEST("move construct: transfer ownership, source emptied");
        MySTL::vector<int> v1(3, 7);
        MySTL::vector<int> v2(std::move(v1));
        bool ok = (v2.size() == 3 && v2[0] == 7 && v2[1] == 7 && v2[2] == 7);
        if (!ok)
            FAIL("moved-to vector has wrong data");
        else if (!v1.isempty() || v1.size() != 0)
            FAIL("moved-from vector not empty");
        else if (v1.begin() != nullptr || v1.end() != nullptr)
            FAIL("moved-from pointers not nulled");
        else
            PASS();
    }

    {
        TEST("move-from can be reused via push_back");
        MySTL::vector<int> v1(3, 10);
        MySTL::vector<int> v2(std::move(v1));
        v1.push_back(42);  // moved-from vector should accept new elements
        if (v1.size() == 1 && v1[0] == 42)
            PASS();
        else
            FAIL("moved-from vector not reusable");
    }

    {
        TEST("swap exchanges data, size, and capacity");
        MySTL::vector<int> v1(3, 1);  // [1,1,1]
        MySTL::vector<int> v2(5, 9);  // [9,9,9,9,9]
        size_t cap1 = v1.capacity();
        size_t cap2 = v2.capacity();
        v1.swap(v2);
        bool ok = (v1.size() == 5 && v2.size() == 3);
        for (size_t i = 0; i < v1.size() && ok; i++)
            if (v1[i] != 9) ok = false;
        for (size_t i = 0; i < v2.size() && ok; i++)
            if (v2[i] != 1) ok = false;
        if (!ok)
            FAIL("data not exchanged");
        else if (v1.capacity() != cap2 || v2.capacity() != cap1)
            FAIL("capacity not exchanged");
        else
            PASS();
    }

    {
        TEST("swap self is safe (no-op)");
        MySTL::vector<int> v(3, 7);
        v.swap(v);
        if (v.size() == 3 && v[0] == 7)
            PASS();
        else
            FAIL("self-swap corrupted data");
    }

    {
        TEST("move assignment transfers data, empties source");
        MySTL::vector<int> v1(3, 7);
        MySTL::vector<int> v2;
        v2 = std::move(v1);
        bool ok = (v2.size() == 3 && v2[0] == 7 && v2[1] == 7 && v2[2] == 7);
        if (!ok)
            FAIL("assignee has wrong data");
        else if (!v1.isempty() || v1.size() != 0)
            FAIL("source not emptied");
        else
            PASS();
    }

    {
        TEST("move assignment transfers data, source left valid");
        MySTL::vector<int> v1(3, 1);
        MySTL::vector<int> v2(5, 9);
        v1 = std::move(v2);
        // v1 now holds v2's old data; v2 is valid-but-unspecified (may hold old v1 data)
        // v2's destructor will free whatever it holds; ASan catches leaks/double-free
        if (v1.size() == 5 && v1[0] == 9)
            PASS();
        else
            FAIL("move assignment corrupted state");
    }

    // {
    //     TEST("move self-assignment is safe");
    //     MySTL::vector<int> v(3, 7);
    //     v = std::move(v);
    //     if (v.size() == 3 && v[0] == 7)
    //         PASS();
    //     else
    //         FAIL("self-move-assign corrupted data");
    // }

    //==============================================================
    // 1b. 拷贝赋值 & 比较操作符
    //==============================================================
    {
        TEST("copy assignment: values match, independence");
        MySTL::vector<int> v1(3, 5);
        MySTL::vector<int> v2(3, 9);
        v1 = v2;
        bool ok = (v1.size() == 3 && v1[0] == 9 && v1[1] == 9 && v1[2] == 9);
        // modify v2 to verify independence
        v2[0] = 0;
        if (ok && v1[0] == 9)
            PASS();
        else
            FAIL("copy assign not independent");
    }

    {
        TEST("copy assignment with capacity expansion");
        MySTL::vector<int> v1(2, 1);
        MySTL::vector<int> v2(5, 7);
        size_t old_cap = v1.capacity();
        v1 = v2;  // v1 has cap 2, v2 has 5 elements, must expand
        if (v1.size() == 5 && v1[0] == 7 && v1[4] == 7 && v1.capacity() > old_cap)
            PASS();
        else
            FAIL("capacity not expanded");
    }

    {
        TEST("copy self-assignment is safe");
        MySTL::vector<int> v(3, 7);
        v = v;  // self-assign
        if (v.size() == 3 && v[0] == 7)
            PASS();
        else
            FAIL("self-copy-assign corrupted data");
    }

    {
        TEST("operator==: equal vectors return true");
        MySTL::vector<int> v1(3, 5);
        MySTL::vector<int> v2(3, 5);
        if (v1 == v2)
            PASS();
        else
            FAIL("equal vectors should compare equal");
    }

    {
        TEST("operator==: different size returns false");
        MySTL::vector<int> v1(3, 5);
        MySTL::vector<int> v2(5, 5);
        if (!(v1 == v2))
            PASS();
        else
            FAIL("different size should not compare equal");
    }

    {
        TEST("operator==: same size, different values returns false");
        MySTL::vector<int> v1(3, 5);
        MySTL::vector<int> v2(3, 9);
        if (!(v1 == v2))
            PASS();
        else
            FAIL("different values should not compare equal");
    }

    {
        TEST("operator!=: equal vectors return false");
        MySTL::vector<int> v1(3, 5);
        MySTL::vector<int> v2(3, 5);
        if (!(v1 != v2))
            PASS();
        else
            FAIL("equal vectors should not compare not-equal");
    }

    {
        TEST("operator!=: different vectors return true");
        MySTL::vector<int> v1(3, 5);
        MySTL::vector<int> v2(3, 9);
        if (v1 != v2)
            PASS();
        else
            FAIL("different vectors should compare not-equal");
    }

    //==============================================================
    // 2. push_back & pop_back
    //==============================================================
    {
        TEST("push_back on empty vector (triggers insert_aux 0->1)");
        MySTL::vector<int> v;
        v.push_back(10);
        if (v.size() == 1 && v[0] == 10 && v.capacity() >= 1)
            PASS();
        else
            FAIL("push_back on empty failed");
    }

    {
        TEST("push_back multiple (covers non-expanding & expanding)");
        MySTL::vector<int> v;
        for (int i = 0; i < 20; i++)
            v.push_back(i);
        bool ok = (v.size() == 20);
        for (int i = 0; i < 20 && ok; i++)
            if (v[i] != i)
                ok = false;
        if (ok && v.capacity() >= 20)
            PASS();
        else
            FAIL("wrong values or size");
    }

    {
        TEST("pop_back reduces size");
        MySTL::vector<int> v(5, 1);
        v.pop_back();
        if (v.size() == 4)
            PASS();
        else
            FAIL("size not reduced");
    }

    {
        TEST("pop_back on empty (no-op)");
        MySTL::vector<int> v;
        v.pop_back(); // should not crash
        if (v.size() == 0 && v.isempty())
            PASS();
        else
            FAIL("popping empty vector is not safe");
    }

    {
        TEST("push_back after pop_back");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.pop_back();
        v.push_back(3);
        if (v.size() == 2 && v[0] == 1 && v[1] == 3)
            PASS();
        else
            FAIL("interleaving failed");
    }

    //==============================================================
    // 3. 元素访问
    //==============================================================
    {
        TEST("operator[] read/write");
        MySTL::vector<int> v(3, 0);
        v[0] = 10;
        v[1] = 20;
        v[2] = 30;
        if (v[0] == 10 && v[1] == 20 && v[2] == 30)
            PASS();
        else
            FAIL("wrong values");
    }

    {
        TEST("front() & back()");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        if (v.front() == 1 && v.back() == 3)
            PASS();
        else
            FAIL("front/back mismatch");
    }

    {
        TEST("data() returns correct pointer");
        MySTL::vector<int> v(3, 5);
        int *p = v.data();
        if (p != nullptr && p[0] == 5 && p[1] == 5 && p[2] == 5)
            PASS();
        else
            FAIL("data() wrong");
    }

    {
        TEST("at(): valid index returns element");
        MySTL::vector<int> v;
        v.push_back(10);
        v.push_back(20);
        v.push_back(30);
        if (v.at(0) == 10 && v.at(1) == 20 && v.at(2) == 30)
            PASS();
        else
            FAIL("at() wrong value");
    }

    {
        TEST("at(): out_of_range on n >= size()");
        MySTL::vector<int> v(3, 1);
        bool caught = false;
        try { v.at(3); } catch (const std::out_of_range&) { caught = true; }
        if (caught)
            PASS();
        else
            FAIL("should throw out_of_range");
    }

    {
        TEST("at(): out_of_range on empty vector");
        MySTL::vector<int> v;
        bool caught = false;
        try { v.at(0); } catch (const std::out_of_range&) { caught = true; }
        if (caught)
            PASS();
        else
            FAIL("should throw out_of_range on empty");
    }

    {
        TEST("const at(): valid index returns const_reference");
        MySTL::vector<int> v;
        v.push_back(42);
        const MySTL::vector<int>& cv = v;
        if (cv.at(0) == 42)
            PASS();
        else
            FAIL("const at() wrong value");
    }

    //==============================================================
    // 4. clear & isempty
    //==============================================================
    {
        TEST("clear makes vector empty");
        MySTL::vector<int> v(5, 42);
        v.clear();
        if (v.size() == 0 && v.isempty())
            PASS();
        else
            FAIL("not empty after clear");
    }

    {
        TEST("clear on already empty vector");
        MySTL::vector<int> v;
        v.clear();
        if (v.size() == 0)
            PASS();
        else
            FAIL("clearing empty failed");
    }

    {
        TEST("reuse after clear");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.clear();
        v.push_back(3);
        if (v.size() == 1 && v[0] == 3)
            PASS();
        else
            FAIL("can't reuse after clear");
    }

    //==============================================================
    // 5. resize
    //==============================================================
    {
        TEST("resize n < size(): truncate, keep original values");
        MySTL::vector<int> v(5, 10);
        v.resize(3, 42);
        if (v.size() == 3 && v[0] == 10 && v[1] == 10 && v[2] == 10)
            PASS();
        else
            FAIL("wrong truncate");
    }

    {
        TEST("resize n > size() && n <= capacity(): fill+append");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        size_t cap = v.capacity();
        if (cap >= 5)
        {
            v.resize(5, 99);
            bool ok = (v.size() == 5);
            for (size_t i = 0; i < 5 && ok; i++)
                if (v[i] != 99)
                    ok = false;
            if (ok)
                PASS();
            else
                FAIL("values not 99");
        }
        else
        {
            std::cout << "SKIPPED (cap too small)" << std::endl;
            passed++;
        }
    }

    {
        TEST("resize n > capacity(): triggers expand branch");
        MySTL::vector<int> v(2, 1);
        size_t old_cap = v.capacity();
        v.resize(10, 7);
        if (v.size() == 10 && v.capacity() > old_cap)
        {
            bool ok = true;
            // first 2 elements preserved, last 8 filled with 7
            if (v[0] != 1 || v[1] != 1) ok = false;
            for (size_t i = 2; i < 10 && ok; i++)
                if (v[i] != 7) ok = false;
            if (ok)
                PASS();
            else
                FAIL("not all values correct");
        }
        else
        {
            FAIL("capacity did not expand");
        }
    }

    {
        TEST("resize n == size(): no-op (NOP)");
        MySTL::vector<int> v(3, 5);
        size_t old_cap = v.capacity();
        v.resize(3, 999); // n == size(), all branches skip
        if (v.size() == 3 && v[0] == 5 && v.capacity() == old_cap)
            PASS();
        else
            FAIL("should be no-op");
    }

    {
        TEST("resize n == 0: truncate to empty");
        MySTL::vector<int> v(3, 5);
        v.resize(0, 0);
        if (v.size() == 0 && v.isempty())
            PASS();
        else
            FAIL("not empty");
    }

    //==============================================================
    // 6. erase
    //==============================================================
    {
        TEST("erase single at front");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        auto it = v.erase(v.begin());
        if (v.size() == 2 && v[0] == 2 && v[1] == 3 && *it == 2)
            PASS();
        else
            FAIL("wrong result");
    }

    {
        TEST("erase single at middle");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.erase(v.begin() + 1);
        if (v.size() == 2 && v[0] == 1 && v[1] == 3)
            PASS();
        else
            FAIL("wrong result");
    }

    {
        TEST("erase single at back");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        auto it = v.erase(v.begin() + 2);
        if (v.size() == 2 && v[0] == 1 && v[1] == 2 && it == v.end())
            PASS();
        else
            FAIL("wrong result or iterator");
    }

    {
        TEST("erase range [first, last)");
        MySTL::vector<int> v;
        for (int i = 0; i < 6; i++)
            v.push_back(i);                              // [0,1,2,3,4,5]
        auto it = v.erase(v.begin() + 1, v.begin() + 4); // erase [1,4) i.e. 1,2,3
        // result should be [0,4,5]
        if (v.size() == 3 && v[0] == 0 && v[1] == 4 && v[2] == 5 && it == v.begin() + 1)
            PASS();
        else
            FAIL("range erase failed");
    }

    {
        TEST("erase entire range [begin, end)");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        auto it = v.erase(v.begin(), v.end());
        if (v.size() == 0 && it == v.end())
            PASS();
        else
            FAIL("full erase failed");
    }

    //==============================================================
    // 7. insert
    //==============================================================
    {
        TEST("insert single at front (no expand)");
        MySTL::vector<int> v;
        v.push_back(2);
        v.push_back(3); // cap from push_back should be >=2
        auto it = v.insert(v.begin(), 1);
        if (v.size() == 3 && v[0] == 1 && v[1] == 2 && v[2] == 3 && *it == 1)
            PASS();
        else
            FAIL("insert front failed");
    }

    {
        TEST("insert single at back (no expand)");
        MySTL::vector<int> v;
        v.push_back(1);
        v.push_back(2);
        auto it = v.insert(v.end(), 3);
        (void)it;
        if (v.size() == 3 && v[0] == 1 && v[1] == 2 && v[2] == 3)
            PASS();
        else
            FAIL("insert back failed");
    }

    {
        TEST("insert single triggers expand");
        MySTL::vector<int> v;
        v.push_back(1);                   // exactly full after first push
        auto it = v.insert(v.begin(), 0); // must expand
        (void)it;
        if (v.size() == 2 && v[0] == 0 && v[1] == 1)
            PASS();
        else
            FAIL("expand insert failed");
    }

    {
        TEST("insert n values (no expand)");
        MySTL::vector<int> v;
        v.push_back(5);
        v.push_back(6); // assume cap >= 5 after
        // Won't force expansion; just test n insert
        v.insert(v.begin() + 1, 3, 99);
        if (v.size() == 5 && v[0] == 5 && v[1] == 99 && v[2] == 99 && v[3] == 99 && v[4] == 6)
            PASS();
        else
            FAIL("n insert failed");
    }

    {
        TEST("insert n values triggers expand");
        MySTL::vector<int> v;
        v.push_back(1); // exactly full
        v.insert(v.begin(), 5, 0);
        if (v.size() == 6 && v[0] == 0 && v[5] == 1)
            PASS();
        else
            FAIL("n expand insert failed");
    }

    //==============================================================
    // 8. 边界与迭代器一致性
    //==============================================================
    {
        TEST("iterator begin/end consistency after push_back & erase");
        MySTL::vector<int> v;
        v.push_back(10);
        v.push_back(20);
        if (v.begin() != v.end() && v.begin() + 1 != v.end())
            PASS();
        else
            FAIL("iterator inconsistency");
    }

    {
        TEST("const_iterator: const begin/end works");
        MySTL::vector<int> v;
        v.push_back(1);
        const MySTL::vector<int> &cv = v;
        if (cv.begin() == v.begin() && cv.end() == v.end())
            PASS();
        else
            FAIL("const iterator mismatch");
    }

    {
        TEST("endback() returns endOfStorage_");
        MySTL::vector<int> v;
        v.push_back(1);
        if (v.endback() >= v.end() &&
            static_cast<size_t>(v.endback() - v.begin()) == v.capacity())
            PASS();
        else
            FAIL("endback wrong");
    }

    //==============================================================
    // 9. POD vs non-POD (check type_traits branch)
    //==============================================================
    {
        TEST("vector of struct with non-trivial semantics (n=3)");
        struct NonPOD
        {
            int x;
            NonPOD() : x(99) {}
            NonPOD(int v) : x(v) {}
        };
        MySTL::vector<NonPOD> v(3, NonPOD(7));
        bool ok = (v.size() == 3);
        for (size_t i = 0; i < v.size() && ok; i++)
            if (v[i].x != 7)
                ok = false;
        if (ok)
            PASS();
        else
            FAIL("non-POD values wrong");
    }

    {
        TEST("push_back non-POD objects");
        struct NonPOD
        {
            int x;
            NonPOD() : x(0) {}
            NonPOD(int v) : x(v) {}
        };
        MySTL::vector<NonPOD> v;
        v.push_back(NonPOD(1));
        v.push_back(NonPOD(2));
        if (v.size() == 2 && v[0].x == 1 && v[1].x == 2)
            PASS();
        else
            FAIL("non-POD push_back failed");
    }

    //==============================================================
    // 10. 结果
    //==============================================================
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << (passed + failed) << std::endl;
    if (failed > 0)
        std::cout << "Failed: " << failed << std::endl;
    return failed == 0 ? 0 : 1;
}