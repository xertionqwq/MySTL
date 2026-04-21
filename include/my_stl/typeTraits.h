#ifndef _TYPE_TRAITS_H_
#define _TYPE_TRAITS_H_
#pragma once

namespace MySTL{

    // Partial Specialization
    // 重写if-else选择, 实现编译期决定变量类型
    /*
    ** std::conditional<bool, T, F>::type (C++11)
    ** std::conditional_t<bool, T, F> (C++14，去掉了繁琐的 ::type 或 ::result)
    */
    namespace {
        template <bool, class Ta, class Tb>
        struct IfThenElse;

        template<class Ta, class Tb>
        struct IfThenElse < true, Ta, Tb > {
            using result = Ta;
        };

        template<class Ta, class Tb>
        struct IfThenElse < false, Ta, Tb > {
            using result = Tb;
        };
    } 

    // 编译期标签
    struct _true_type{
    };
    struct _false_type {
    };
    
    /*
    ** 萃取传入T类型的类型特征
    */
    template<class T>
    // 泛型版本, 悲观假设所有类型都复杂
    struct _type_traits {
        using has_trivial_default_constructor = _false_type;
        using has_trivial_copy_constructor = _false_type;
        using has_trivial_assignment_operator = _false_type;
        using has_trivial_destructor = _false_type;
        using is_POD_type = _false_type;
    };


    /*
    ** 往后逐个偏特化
    */
    template<>
    struct _type_traits<bool>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    };

    template<>
    struct _type_traits<char>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<signed char>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<wchar_t>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<short>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    };  

    template<>
    struct _type_traits<unsigned short>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    };  

    template<>
    struct _type_traits<int>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    };  

    template<>
    struct _type_traits<unsigned int>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 
    
    template<>
    struct _type_traits<long>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 


    template<>
    struct _type_traits<long long>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<unsigned long long>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<double>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<long double>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 


    template<class T>
    struct _type_traits<T*>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    };

    template<class T>
    struct _type_traits<const T*>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    };

    template<>
    struct _type_traits<char*>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    };

    template<>
    struct _type_traits<unsigned char*>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    };

    template<>
    struct _type_traits<signed char*>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<const char*>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<const unsigned char*>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 

    template<>
    struct _type_traits<const signed char*>{
        using has_trivial_default_constructor = _true_type;
        using has_trivial_copy_constructor = _true_type;
        using has_trivial_assignment_operator = _true_type;
        using has_trivial_destructor = _true_type;
        using is_POD_type = _true_type;
    }; 
};

#endif