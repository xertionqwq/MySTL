#include <cstdlib>   // malloc / free
#include <iostream>
#include <stdexcept> // std::logic_error

#include "my_stl/core/construct.h"

using namespace std;

#define MEM_ALL cout << "mem has allocated" << endl;
#define MEM_DES cout << "mem has destoryed" << endl;

class A {
public:
    // A() = default;
    A(int val) : a(val) {
        cout << "A(int) constract -- val-> " << a << endl;
    }
    ~A() { cout << "~A() destoryed -- val-> " << a << endl; }

    int a = 10;
};

class B {
public:
    B(size_t size) : aa(91) , buf(nullptr) {
        if (size >= 1e5)
            throw std::logic_error("Buffer overflow!");
        buf = new int[size];
        cout << "B(size_t) construct" << endl;
    }
    ~B() { 
        delete buf;
        cout << "~B() destoryed" << endl;
    }

    A aa;
    int *buf; // 已由构造函数的成员初始化列表 buf(nullptr) 置空
};

// 实验一, 内存分配与对象构造分离
void test1()
{
    A aa(20);
    void *mem = malloc(sizeof(A)); // 分配内存
    MEM_ALL
    A *live = static_cast<A *>(mem);  // 指针转换

    MySTL::construct(live, aa);       // 对象构造
    cout << "mem->" << live << " --- a:" << live->a << endl;
    MySTL::destroy(live); // 显式析构, 此时内存尚未回收

    free(mem); // 释放内存
    MEM_DES
}

// 实验二, 多次构造与析构
void test2() {
    void *mem = malloc(sizeof(A));
    MEM_ALL
    A *live = static_cast<A *>(mem);

    // 多次构造与析构
    // 但是分配的内存地址始终不变
    for (int i = 0; i < 5; ++i) {
        // A(i) 是临时对象, 命中 construct(T*, T&&) 移动重载;
        // 但 A 有自定义析构函数, 隐式移动构造被抑制, 实际退化为拷贝构造
        MySTL::construct(live, A(i));       // 对象构造
        cout << "mem->" << live << " --- a:" << live->a << endl;
        MySTL::destroy(live); // 显式析构, 此时内存尚未回收
    }

    free(mem);
    MEM_DES
}

// 实验三, 构造函数抛异常, 同时验证栈展开
void test3() {
    try{
        A a1(99);
        B(1e6);
        // 栈展开, 先析构91a, 再析构99a
    } catch (const std::exception &e) { // 就地接住, 避免异常逃出 main 触发 std::terminate
        cout << "caught: " << e.what() << endl;
    }
}

int main() {
    test1();

    test2();

    test3();

    cout << "return done" << endl;
}