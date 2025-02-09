
// 元组(Tuple)是一种概念上与std::pair类似的通用模板容器.
// Tuple0到Tuple6对应容器中元素个数. 带有0-6个参数的MakeTuple()函数能够方便
// 的构造对应的元组对象. DispatchToMethod和DispatchToFunction带有函数指针或
// 者对象实例和成员函数指针, 解包元组为参数进行函数调用.
//
// 元组的元素通过值拷贝存储.
//
// 用法示例:
//     // These two methods of creating a Tuple are identical.
//     Tuple2<int, const char*> tuple_a(1, "wee");
//     Tuple2<int, const char*> tuple_b = MakeTuple(1, "wee");
//
//     void SomeFunc(int a, const char* b) {}
//     DispatchToFunction(&SomeFunc, tuple_a); // SomeFunc(1, "wee")
//     DispatchToFunction(
//       &SomeFunc, MakeTuple(10, "foo")); // SomeFunc(10, "foo")
//
//     struct { void SomeMeth(int a, int b, int c) {} } foo;
//     DispatchToMethod(&foo, &Foo::SomeMeth, MakeTuple(1, 2, 3));
//     //foo->SomeMeth(1, 2, 3);

#ifndef __base_tuple_h__
#define __base_tuple_h__

#pragma once

// Traits ----------------------------------------------------------------------
//
// A simple traits class for tuple arguments.
//
// ValueType: the bare, nonref version of a type (same as the type for nonrefs).
// RefType: the ref version of a type (same as the type for refs).
// ParamType: what type to pass to functions (refs should not be constified).
template<class P>
struct TupleTraits
{
    typedef P ValueType;
    typedef P& RefType;
    typedef const P& ParamType;
};

template<class P>
struct TupleTraits<P&>
{
    typedef P ValueType;
    typedef P& RefType;
    typedef P& ParamType;
};

template<class P>
struct TupleTypes {};

// Tuple -----------------------------------------------------------------------
//
// This set of classes is useful for bundling 0 or more heterogeneous data types
// into a single variable.  The advantage of this is that it greatly simplifies
// function objects that need to take an arbitrary number of parameters; see
// RunnableMethod and IPC::MessageWithTuple.
//
// Tuple0 is supplied to act as a 'void' type.  It can be used, for example,
// when dispatching to a function that accepts no arguments (see the
// Dispatchers below).
// Tuple1<A> is rarely useful.  One such use is when A is non-const ref that you
// want filled by the dispatchee, and the tuple is merely a container for that
// output (a "tier").  See MakeRefTuple and its usages.
struct Tuple0
{
    typedef Tuple0 ValueTuple;
    typedef Tuple0 RefTuple;
    typedef Tuple0 ParamTuple;
};

template<class A>
struct Tuple1
{
public:
    typedef A TypeA;

    Tuple1() {}
    explicit Tuple1(typename TupleTraits<A>::ParamType a) : a(a) {}

    A a;
};

template<class A, class B>
struct Tuple2
{
public:
    typedef A TypeA;
    typedef B TypeB;

    Tuple2() {}
    Tuple2(typename TupleTraits<A>::ParamType a,
           typename TupleTraits<B>::ParamType b)
        : a(a), b(b) {}

    A a;
    B b;
};

template<class A, class B, class C>
struct Tuple3
{
public:
    typedef A TypeA;
    typedef B TypeB;
    typedef C TypeC;

    Tuple3() {}
    Tuple3(typename TupleTraits<A>::ParamType a,
           typename TupleTraits<B>::ParamType b,
           typename TupleTraits<C>::ParamType c)
        : a(a), b(b), c(c) {}

    A a;
    B b;
    C c;
};

template<class A, class B, class C, class D>
struct Tuple4
{
public:
    typedef A TypeA;
    typedef B TypeB;
    typedef C TypeC;
    typedef D TypeD;

    Tuple4() {}
    Tuple4(typename TupleTraits<A>::ParamType a,
           typename TupleTraits<B>::ParamType b,
           typename TupleTraits<C>::ParamType c,
           typename TupleTraits<D>::ParamType d)
        : a(a), b(b), c(c), d(d) {}

    A a;
    B b;
    C c;
    D d;
};

template<class A, class B, class C, class D, class E>
struct Tuple5
{
public:
    typedef A TypeA;
    typedef B TypeB;
    typedef C TypeC;
    typedef D TypeD;
    typedef E TypeE;

    Tuple5() {}
    Tuple5(typename TupleTraits<A>::ParamType a,
           typename TupleTraits<B>::ParamType b,
           typename TupleTraits<C>::ParamType c,
           typename TupleTraits<D>::ParamType d,
           typename TupleTraits<E>::ParamType e)
        : a(a), b(b), c(c), d(d), e(e) {}

    A a;
    B b;
    C c;
    D d;
    E e;
};

template<class A, class B, class C, class D, class E, class F>
struct Tuple6
{
public:
    typedef A TypeA;
    typedef B TypeB;
    typedef C TypeC;
    typedef D TypeD;
    typedef E TypeE;
    typedef F TypeF;

    Tuple6() {}
    Tuple6(typename TupleTraits<A>::ParamType a,
           typename TupleTraits<B>::ParamType b,
           typename TupleTraits<C>::ParamType c,
           typename TupleTraits<D>::ParamType d,
           typename TupleTraits<E>::ParamType e,
           typename TupleTraits<F>::ParamType f)
        : a(a), b(b), c(c), d(d), e(e), f(f) {}

    A a;
    B b;
    C c;
    D d;
    E e;
    F f;
};

template<class A, class B, class C, class D, class E, class F, class G>
struct Tuple7
{
public:
    typedef A TypeA;
    typedef B TypeB;
    typedef C TypeC;
    typedef D TypeD;
    typedef E TypeE;
    typedef F TypeF;
    typedef G TypeG;

    Tuple7() {}
    Tuple7(typename TupleTraits<A>::ParamType a,
           typename TupleTraits<B>::ParamType b,
           typename TupleTraits<C>::ParamType c,
           typename TupleTraits<D>::ParamType d,
           typename TupleTraits<E>::ParamType e,
           typename TupleTraits<F>::ParamType f,
           typename TupleTraits<G>::ParamType g)
        : a(a), b(b), c(c), d(d), e(e), f(f), g(g) {}

    A a;
    B b;
    C c;
    D d;
    E e;
    F f;
    G g;
};

template<class A, class B, class C, class D, class E, class F, class G,
         class H>
struct Tuple8
{
public:
    typedef A TypeA;
    typedef B TypeB;
    typedef C TypeC;
    typedef D TypeD;
    typedef E TypeE;
    typedef F TypeF;
    typedef G TypeG;
    typedef H TypeH;

    Tuple8() {}
    Tuple8(typename TupleTraits<A>::ParamType a,
           typename TupleTraits<B>::ParamType b,
           typename TupleTraits<C>::ParamType c,
           typename TupleTraits<D>::ParamType d,
           typename TupleTraits<E>::ParamType e,
           typename TupleTraits<F>::ParamType f,
           typename TupleTraits<G>::ParamType g,
           typename TupleTraits<H>::ParamType h)
        : a(a), b(b), c(c), d(d), e(e), f(f), g(g), h(h) {}

    A a;
    B b;
    C c;
    D d;
    E e;
    F f;
    G g;
    H h;
};

// Tuple types ----------------------------------------------------------------
//
// Allows for selection of ValueTuple/RefTuple/ParamTuple without needing the
// definitions of class types the tuple takes as parameters.
template<>
struct TupleTypes< Tuple0 >
{
    typedef Tuple0 ValueTuple;
    typedef Tuple0 RefTuple;
    typedef Tuple0 ParamTuple;
};

template<class A>
struct TupleTypes< Tuple1<A> >
{
    typedef Tuple1<typename TupleTraits<A>::ValueType> ValueTuple;
    typedef Tuple1<typename TupleTraits<A>::RefType> RefTuple;
    typedef Tuple1<typename TupleTraits<A>::ParamType> ParamTuple;
};

template<class A, class B>
struct TupleTypes< Tuple2<A, B> >
{
    typedef Tuple2<typename TupleTraits<A>::ValueType,
            typename TupleTraits<B>::ValueType> ValueTuple;
    typedef Tuple2<typename TupleTraits<A>::RefType,
            typename TupleTraits<B>::RefType> RefTuple;
    typedef Tuple2<typename TupleTraits<A>::ParamType,
            typename TupleTraits<B>::ParamType> ParamTuple;
};

template<class A, class B, class C>
struct TupleTypes< Tuple3<A, B, C> >
{
    typedef Tuple3<typename TupleTraits<A>::ValueType,
            typename TupleTraits<B>::ValueType,
            typename TupleTraits<C>::ValueType> ValueTuple;
    typedef Tuple3<typename TupleTraits<A>::RefType,
            typename TupleTraits<B>::RefType,
            typename TupleTraits<C>::RefType> RefTuple;
    typedef Tuple3<typename TupleTraits<A>::ParamType,
            typename TupleTraits<B>::ParamType,
            typename TupleTraits<C>::ParamType> ParamTuple;
};

template<class A, class B, class C, class D>
struct TupleTypes< Tuple4<A, B, C, D> >
{
    typedef Tuple4<typename TupleTraits<A>::ValueType,
            typename TupleTraits<B>::ValueType,
            typename TupleTraits<C>::ValueType,
            typename TupleTraits<D>::ValueType> ValueTuple;
    typedef Tuple4<typename TupleTraits<A>::RefType,
            typename TupleTraits<B>::RefType,
            typename TupleTraits<C>::RefType,
            typename TupleTraits<D>::RefType> RefTuple;
    typedef Tuple4<typename TupleTraits<A>::ParamType,
            typename TupleTraits<B>::ParamType,
            typename TupleTraits<C>::ParamType,
            typename TupleTraits<D>::ParamType> ParamTuple;
};

template<class A, class B, class C, class D, class E>
struct TupleTypes< Tuple5<A, B, C, D, E> >
{
    typedef Tuple5<typename TupleTraits<A>::ValueType,
            typename TupleTraits<B>::ValueType,
            typename TupleTraits<C>::ValueType,
            typename TupleTraits<D>::ValueType,
            typename TupleTraits<E>::ValueType> ValueTuple;
    typedef Tuple5<typename TupleTraits<A>::RefType,
            typename TupleTraits<B>::RefType,
            typename TupleTraits<C>::RefType,
            typename TupleTraits<D>::RefType,
            typename TupleTraits<E>::RefType> RefTuple;
    typedef Tuple5<typename TupleTraits<A>::ParamType,
            typename TupleTraits<B>::ParamType,
            typename TupleTraits<C>::ParamType,
            typename TupleTraits<D>::ParamType,
            typename TupleTraits<E>::ParamType> ParamTuple;
};

template<class A, class B, class C, class D, class E, class F>
struct TupleTypes< Tuple6<A, B, C, D, E, F> >
{
    typedef Tuple6<typename TupleTraits<A>::ValueType,
            typename TupleTraits<B>::ValueType,
            typename TupleTraits<C>::ValueType,
            typename TupleTraits<D>::ValueType,
            typename TupleTraits<E>::ValueType,
            typename TupleTraits<F>::ValueType> ValueTuple;
    typedef Tuple6<typename TupleTraits<A>::RefType,
            typename TupleTraits<B>::RefType,
            typename TupleTraits<C>::RefType,
            typename TupleTraits<D>::RefType,
            typename TupleTraits<E>::RefType,
            typename TupleTraits<F>::RefType> RefTuple;
    typedef Tuple6<typename TupleTraits<A>::ParamType,
            typename TupleTraits<B>::ParamType,
            typename TupleTraits<C>::ParamType,
            typename TupleTraits<D>::ParamType,
            typename TupleTraits<E>::ParamType,
            typename TupleTraits<F>::ParamType> ParamTuple;
};

template<class A, class B, class C, class D, class E, class F, class G>
struct TupleTypes< Tuple7<A, B, C, D, E, F, G> >
{
    typedef Tuple7<typename TupleTraits<A>::ValueType,
            typename TupleTraits<B>::ValueType,
            typename TupleTraits<C>::ValueType,
            typename TupleTraits<D>::ValueType,
            typename TupleTraits<E>::ValueType,
            typename TupleTraits<F>::ValueType,
            typename TupleTraits<G>::ValueType> ValueTuple;
    typedef Tuple7<typename TupleTraits<A>::RefType,
            typename TupleTraits<B>::RefType,
            typename TupleTraits<C>::RefType,
            typename TupleTraits<D>::RefType,
            typename TupleTraits<E>::RefType,
            typename TupleTraits<F>::RefType,
            typename TupleTraits<G>::RefType> RefTuple;
    typedef Tuple7<typename TupleTraits<A>::ParamType,
            typename TupleTraits<B>::ParamType,
            typename TupleTraits<C>::ParamType,
            typename TupleTraits<D>::ParamType,
            typename TupleTraits<E>::ParamType,
            typename TupleTraits<F>::ParamType,
            typename TupleTraits<G>::ParamType> ParamTuple;
};

template<class A, class B, class C, class D, class E, class F, class G,
         class H>
struct TupleTypes< Tuple8<A, B, C, D, E, F, G, H> >
{
    typedef Tuple8<typename TupleTraits<A>::ValueType,
            typename TupleTraits<B>::ValueType,
            typename TupleTraits<C>::ValueType,
            typename TupleTraits<D>::ValueType,
            typename TupleTraits<E>::ValueType,
            typename TupleTraits<F>::ValueType,
            typename TupleTraits<G>::ValueType,
            typename TupleTraits<H>::ValueType> ValueTuple;
    typedef Tuple8<typename TupleTraits<A>::RefType,
            typename TupleTraits<B>::RefType,
            typename TupleTraits<C>::RefType,
            typename TupleTraits<D>::RefType,
            typename TupleTraits<E>::RefType,
            typename TupleTraits<F>::RefType,
            typename TupleTraits<G>::RefType,
            typename TupleTraits<H>::RefType> RefTuple;
    typedef Tuple8<typename TupleTraits<A>::ParamType,
            typename TupleTraits<B>::ParamType,
            typename TupleTraits<C>::ParamType,
            typename TupleTraits<D>::ParamType,
            typename TupleTraits<E>::ParamType,
            typename TupleTraits<F>::ParamType,
            typename TupleTraits<G>::ParamType,
            typename TupleTraits<H>::ParamType> ParamTuple;
};

// Tuple creators -------------------------------------------------------------
//
// Helper functions for constructing tuples while inferring the template
// argument types.
inline Tuple0 MakeTuple()
{
    return Tuple0();
}

template<class A>
inline Tuple1<A> MakeTuple(const A& a)
{
    return Tuple1<A>(a);
}

template<class A, class B>
inline Tuple2<A, B> MakeTuple(const A& a, const B& b)
{
    return Tuple2<A, B>(a, b);
}

template<class A, class B, class C>
inline Tuple3<A, B, C> MakeTuple(const A& a, const B& b, const C& c)
{
    return Tuple3<A, B, C>(a, b, c);
}

template<class A, class B, class C, class D>
inline Tuple4<A, B, C, D> MakeTuple(const A& a, const B& b, const C& c,
                                    const D& d)
{
    return Tuple4<A, B, C, D>(a, b, c, d);
}

template<class A, class B, class C, class D, class E>
inline Tuple5<A, B, C, D, E> MakeTuple(const A& a, const B& b, const C& c,
                                       const D& d, const E& e)
{
    return Tuple5<A, B, C, D, E>(a, b, c, d, e);
}

template<class A, class B, class C, class D, class E, class F>
inline Tuple6<A, B, C, D, E, F> MakeTuple(const A& a, const B& b, const C& c,
        const D& d, const E& e, const F& f)
{
    return Tuple6<A, B, C, D, E, F>(a, b, c, d, e, f);
}

template<class A, class B, class C, class D, class E, class F, class G>
inline Tuple7<A, B, C, D, E, F, G> MakeTuple(const A& a, const B& b, const C& c,
        const D& d, const E& e, const F& f,
        const G& g)
{
    return Tuple7<A, B, C, D, E, F, G>(a, b, c, d, e, f, g);
}

template<class A, class B, class C, class D, class E, class F, class G,
         class H>
inline Tuple8<A, B, C, D, E, F, G, H> MakeTuple(const A& a, const B& b,
        const C& c, const D& d,
        const E& e, const F& f,
        const G& g, const H& h)
{
    return Tuple8<A, B, C, D, E, F, G, H>(a, b, c, d, e, f, g, h);
}

// The following set of helpers make what Boost refers to as "Tiers" - a tuple
// of references.
template<class A>
inline Tuple1<A&> MakeRefTuple(A& a)
{
    return Tuple1<A&>(a);
}

template<class A, class B>
inline Tuple2<A&, B&> MakeRefTuple(A& a, B& b)
{
    return Tuple2<A&, B&>(a, b);
}

template<class A, class B, class C>
inline Tuple3<A&, B&, C&> MakeRefTuple(A& a, B& b, C& c)
{
    return Tuple3<A&, B&, C&>(a, b, c);
}

template<class A, class B, class C, class D>
inline Tuple4<A&, B&, C&, D&> MakeRefTuple(A& a, B& b, C& c, D& d)
{
    return Tuple4<A&, B&, C&, D&>(a, b, c, d);
}

template<class A, class B, class C, class D, class E>
inline Tuple5<A&, B&, C&, D&, E&> MakeRefTuple(A& a, B& b, C& c, D& d, E& e)
{
    return Tuple5<A&, B&, C&, D&, E&>(a, b, c, d, e);
}

template<class A, class B, class C, class D, class E, class F>
inline Tuple6<A&, B&, C&, D&, E&, F&> MakeRefTuple(A& a, B& b, C& c, D& d, E& e,
        F& f)
{
    return Tuple6<A&, B&, C&, D&, E&, F&>(a, b, c, d, e, f);
}

template<class A, class B, class C, class D, class E, class F, class G>
inline Tuple7<A&, B&, C&, D&, E&, F&, G&> MakeRefTuple(A& a, B& b, C& c, D& d,
        E& e, F& f, G& g)
{
    return Tuple7<A&, B&, C&, D&, E&, F&, G&>(a, b, c, d, e, f, g);
}

template<class A, class B, class C, class D, class E, class F, class G,
         class H>
inline Tuple8<A&, B&, C&, D&, E&, F&, G&, H&> MakeRefTuple(A& a, B& b, C& c,
        D& d, E& e, F& f,
        G& g, H& h)
{
    return Tuple8<A&, B&, C&, D&, E&, F&, G&, H&>(a, b, c, d, e, f, g, h);
}

// Dispatchers ----------------------------------------------------------------
//
// Helper functions that call the given method on an object, with the unpacked
// tuple arguments.  Notice that they all have the same number of arguments,
// so you need only write:
//   DispatchToMethod(object, &Object::method, args);
// This is very useful for templated dispatchers, since they don't need to know
// what type |args| is.

// Non-Static Dispatchers with no out params.
template<class ObjT, class Method>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple0& arg)
{
    (obj->*method)();
}

template<class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg)
{
    (obj->*method)(arg);
}

template<class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple1<A>& arg)
{
    (obj->*method)(arg.a);
}

template<class ObjT, class Method, class A, class B>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple2<A, B>& arg)
{
    (obj->*method)(arg.a, arg.b);
}

template<class ObjT, class Method, class A, class B, class C>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<A, B, C>& arg)
{
    (obj->*method)(arg.a, arg.b, arg.c);
}

template<class ObjT, class Method, class A, class B, class C, class D>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<A, B, C, D>& arg)
{
    (obj->*method)(arg.a, arg.b, arg.c, arg.d);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<A, B, C, D, E>& arg)
{
    (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<A, B, C, D, E, F>& arg)
{
    (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F, class G>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple7<A, B, C, D, E, F, G>& arg)
{
    (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f, arg.g);
}

// Static Dispatchers with no out params.
template<class Function>
inline void DispatchToFunction(Function function, const Tuple0& arg)
{
    (*function)();
}

template<class Function, class A>
inline void DispatchToFunction(Function function, const A& arg)
{
    (*function)(arg);
}

template<class Function, class A>
inline void DispatchToFunction(Function function, const Tuple1<A>& arg)
{
    (*function)(arg.a);
}

template<class Function, class A, class B>
inline void DispatchToFunction(Function function, const Tuple2<A, B>& arg)
{
    (*function)(arg.a, arg.b);
}

template<class Function, class A, class B, class C>
inline void DispatchToFunction(Function function, const Tuple3<A, B, C>& arg)
{
    (*function)(arg.a, arg.b, arg.c);
}

template<class Function, class A, class B, class C, class D>
inline void DispatchToFunction(Function function,
                               const Tuple4<A, B, C, D>& arg)
{
    (*function)(arg.a, arg.b, arg.c, arg.d);
}

template<class Function, class A, class B, class C, class D, class E>
inline void DispatchToFunction(Function function,
                               const Tuple5<A, B, C, D, E>& arg)
{
    (*function)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

template<class Function, class A, class B, class C, class D, class E, class F>
inline void DispatchToFunction(Function function,
                               const Tuple6<A, B, C, D, E, F>& arg)
{
    (*function)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f);
}

template<class Function, class A, class B, class C, class D, class E, class F,
         class G>
inline void DispatchToFunction(Function function,
                               const Tuple7<A, B, C, D, E, F, G>& arg)
{
    (*function)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f, arg.g);
}

template<class Function, class A, class B, class C, class D, class E, class F,
         class G, class H>
inline void DispatchToFunction(Function function,
                               const Tuple8<A, B, C, D, E, F, G, H>& arg)
{
    (*function)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f, arg.g, arg.h);
}

// Dispatchers with 0 out param (as a Tuple0).
template<class ObjT, class Method>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple0& arg, Tuple0*)
{
    (obj->*method)();
}

template<class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg, Tuple0*)
{
    (obj->*method)(arg);
}

template<class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple1<A>& arg, Tuple0*)
{
    (obj->*method)(arg.a);
}

template<class ObjT, class Method, class A, class B>
inline void DispatchToMethod(ObjT* obj,
                             Method method,
                             const Tuple2<A, B>& arg, Tuple0*)
{
    (obj->*method)(arg.a, arg.b);
}

template<class ObjT, class Method, class A, class B, class C>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<A, B, C>& arg, Tuple0*)
{
    (obj->*method)(arg.a, arg.b, arg.c);
}

template<class ObjT, class Method, class A, class B, class C, class D>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<A, B, C, D>& arg, Tuple0*)
{
    (obj->*method)(arg.a, arg.b, arg.c, arg.d);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<A, B, C, D, E>& arg, Tuple0*)
{
    (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E,
         class F>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<A, B, C, D, E, F>& arg, Tuple0*)
{
    (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e, arg.f);
}

// Dispatchers with 1 out param.
template<class ObjT, class Method,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple1<OutA>* out)
{
    (obj->*method)(&out->a);
}

template<class ObjT, class Method, class InA,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple1<OutA>* out)
{
    (obj->*method)(in, &out->a);
}

template<class ObjT, class Method, class InA,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple1<OutA>* out)
{
    (obj->*method)(in.a, &out->a);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple1<OutA>* out)
{
    (obj->*method)(in.a, in.b, &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple1<OutA>* out)
{
    (obj->*method)(in.a, in.b, in.c, &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple1<OutA>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class InE, class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple1<OutA>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple1<OutA>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f, &out->a);
}

// Dispatchers with 2 out params.
template<class ObjT, class Method,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple2<OutA, OutB>* out)
{
    (obj->*method)(&out->a, &out->b);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple2<OutA, OutB>* out)
{
    (obj->*method)(in, &out->a, &out->b);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple2<OutA, OutB>* out)
{
    (obj->*method)(in.a, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple2<OutA, OutB>* out)
{
    (obj->*method)(in.a, in.b, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple2<OutA, OutB>* out)
{
    (obj->*method)(in.a, in.b, in.c, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple2<OutA, OutB>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple2<OutA, OutB>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a, &out->b);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple2<OutA, OutB>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f, &out->a, &out->b);
}

// Dispatchers with 3 out params.
template<class ObjT, class Method,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple3<OutA, OutB, OutC>* out)
{
    (obj->*method)(&out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple3<OutA, OutB, OutC>* out)
{
    (obj->*method)(in, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple3<OutA, OutB, OutC>* out)
{
    (obj->*method)(in.a, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple3<OutA, OutB, OutC>* out)
{
    (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple3<OutA, OutB, OutC>* out)
{
    (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple3<OutA, OutB, OutC>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple3<OutA, OutB, OutC>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple3<OutA, OutB, OutC>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f, &out->a, &out->b, &out->c);
}

// Dispatchers with 4 out params.
template<class ObjT, class Method,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out)
{
    (obj->*method)(&out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out)
{
    (obj->*method)(in, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out)
{
    (obj->*method)(in.a, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out)
{
    (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out)
{
    (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e,
                   &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f,
                   &out->a, &out->b, &out->c, &out->d);
}

// Dispatchers with 5 out params.
template<class ObjT, class Method,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out)
{
    (obj->*method)(&out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out)
{
    (obj->*method)(in, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out)
{
    (obj->*method)(in.a, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out)
{
    (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out)
{
    (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c, &out->d,
                   &out->e);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e,
                   &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE, class InF,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple6<InA, InB, InC, InD, InE, InF>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out)
{
    (obj->*method)(in.a, in.b, in.c, in.d, in.e, in.f,
                   &out->a, &out->b, &out->c, &out->d, &out->e);
}

#endif //__base_tuple_h__