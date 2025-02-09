
#ifndef __base_bind_internal_h__
#define __base_bind_internal_h__

#pragma once

#include "base/bind_helpers.h"
#include "base/bind_internal_win.h"
#include "base/callback_internal.h"
#include "base/memory/weak_ptr.h"
#include "base/template_util.h"

namespace base
{
namespace internal
{

// The method by which a function is invoked is determined by 3 different
// dimensions:
//
//   1) The type of function (normal or method).
//   2) The arity of the function.
//   3) The number of bound parameters.
//
// The templates below handle the determination of each of these dimensions.
// In brief:
//
//   FunctionTraits<> -- Provides a normalied signature, and other traits.
//   InvokerN<> -- Provides a DoInvoke() function that actually executes
//                 a calback.
//   InvokerStorageN<> -- Provides storage for the bound parameters, and
//                        typedefs to the above.
//   IsWeakMethod<> -- Determines if we are binding a method to a WeakPtr<>.
//
// More details about the design of each class is included in a comment closer
// to their defition.


// IsWeakMethod determines if we are binding a method to a WeakPtr<> for an
// object.  It is used to select an InvokerN that will no-op itself in the
// event the WeakPtr<> for the target object is invalidated.
template <bool IsMethod, typename T>
struct IsWeakMethod : public false_type {};

template <typename T>
struct IsWeakMethod<true, WeakPtr<T> > : public true_type {};

// FunctionTraits<>
//
// The FunctionTraits<> template determines the type of function, and also
// creates a NormalizedType used to select the InvokerN classes.  It turns out
// that syntactically, you only really have 2 variations when invoking a
// funciton pointer: normal, and method.  One is invoked func_ptr(arg1). The
// other is invoked (*obj_->method_ptr(arg1)).
//
// However, in the type system, there are many more distinctions. In standard
// C++, there's all variations of const, and volatile on the function pointer.
// In Windows, there are additional calling conventions (eg., __stdcall,
// __fastcall, etc.). FunctionTraits<> handles categorizing each of these into
// a normalized signature.
//
// Having a NormalizedSignature signature, reduces the combinatoric
// complexity of defintions for the InvokerN<> later.  Even though there are
// only 2 syntactic variations on invoking a function, without normalizing the
// signature, there would need to be one specialization of InvokerN for each
// unique (function_type, bound_arg, unbound_args) tuple in order to match all
// function signatures.
//
// By normalizing the function signature, we reduce function_type to exactly 2.

template <typename Sig>
struct FunctionTraits;

// Function: Arity 0.
template <typename R>
struct FunctionTraits<R(*)()>
{
    typedef R (*NormalizedSig)();
    typedef false_type IsMethod;

    typedef R Return;

};

// Method: Arity 0.
template <typename R, typename T>
struct FunctionTraits<R(T::*)()>
{
    typedef R (T::*NormalizedSig)();
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;

};

// Const Method: Arity 0.
template <typename R, typename T>
struct FunctionTraits<R(T::*)() const>
{
    typedef R (T::*NormalizedSig)();
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;

};

// Function: Arity 1.
template <typename R, typename X1>
struct FunctionTraits<R(*)(X1)>
{
    typedef R (*NormalizedSig)(X1);
    typedef false_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef X1 B1;

};

// Method: Arity 1.
template <typename R, typename T, typename X1>
struct FunctionTraits<R(T::*)(X1)>
{
    typedef R (T::*NormalizedSig)(X1);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;

};

// Const Method: Arity 1.
template <typename R, typename T, typename X1>
struct FunctionTraits<R(T::*)(X1) const>
{
    typedef R (T::*NormalizedSig)(X1);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;

};

// Function: Arity 2.
template <typename R, typename X1, typename X2>
struct FunctionTraits<R(*)(X1, X2)>
{
    typedef R (*NormalizedSig)(X1, X2);
    typedef false_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef X1 B1;
    typedef X2 B2;

};

// Method: Arity 2.
template <typename R, typename T, typename X1, typename X2>
struct FunctionTraits<R(T::*)(X1, X2)>
{
    typedef R (T::*NormalizedSig)(X1, X2);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;

};

// Const Method: Arity 2.
template <typename R, typename T, typename X1, typename X2>
struct FunctionTraits<R(T::*)(X1, X2) const>
{
    typedef R (T::*NormalizedSig)(X1, X2);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;

};

// Function: Arity 3.
template <typename R, typename X1, typename X2, typename X3>
struct FunctionTraits<R(*)(X1, X2, X3)>
{
    typedef R (*NormalizedSig)(X1, X2, X3);
    typedef false_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef X1 B1;
    typedef X2 B2;
    typedef X3 B3;

};

// Method: Arity 3.
template <typename R, typename T, typename X1, typename X2, typename X3>
struct FunctionTraits<R(T::*)(X1, X2, X3)>
{
    typedef R (T::*NormalizedSig)(X1, X2, X3);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;
    typedef X3 B4;

};

// Const Method: Arity 3.
template <typename R, typename T, typename X1, typename X2, typename X3>
struct FunctionTraits<R(T::*)(X1, X2, X3) const>
{
    typedef R (T::*NormalizedSig)(X1, X2, X3);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;
    typedef X3 B4;

};

// Function: Arity 4.
template <typename R, typename X1, typename X2, typename X3, typename X4>
struct FunctionTraits<R(*)(X1, X2, X3, X4)>
{
    typedef R (*NormalizedSig)(X1, X2, X3, X4);
    typedef false_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef X1 B1;
    typedef X2 B2;
    typedef X3 B3;
    typedef X4 B4;

};

// Method: Arity 4.
template <typename R, typename T, typename X1, typename X2, typename X3,
          typename X4>
struct FunctionTraits<R(T::*)(X1, X2, X3, X4)>
{
    typedef R (T::*NormalizedSig)(X1, X2, X3, X4);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;
    typedef X3 B4;
    typedef X4 B5;

};

// Const Method: Arity 4.
template <typename R, typename T, typename X1, typename X2, typename X3,
          typename X4>
struct FunctionTraits<R(T::*)(X1, X2, X3, X4) const>
{
    typedef R (T::*NormalizedSig)(X1, X2, X3, X4);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;
    typedef X3 B4;
    typedef X4 B5;

};

// Function: Arity 5.
template <typename R, typename X1, typename X2, typename X3, typename X4,
          typename X5>
struct FunctionTraits<R(*)(X1, X2, X3, X4, X5)>
{
    typedef R (*NormalizedSig)(X1, X2, X3, X4, X5);
    typedef false_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef X1 B1;
    typedef X2 B2;
    typedef X3 B3;
    typedef X4 B4;
    typedef X5 B5;

};

// Method: Arity 5.
template <typename R, typename T, typename X1, typename X2, typename X3,
          typename X4, typename X5>
struct FunctionTraits<R(T::*)(X1, X2, X3, X4, X5)>
{
    typedef R (T::*NormalizedSig)(X1, X2, X3, X4, X5);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;
    typedef X3 B4;
    typedef X4 B5;
    typedef X5 B6;

};

// Const Method: Arity 5.
template <typename R, typename T, typename X1, typename X2, typename X3,
          typename X4, typename X5>
struct FunctionTraits<R(T::*)(X1, X2, X3, X4, X5) const>
{
    typedef R (T::*NormalizedSig)(X1, X2, X3, X4, X5);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;
    typedef X3 B4;
    typedef X4 B5;
    typedef X5 B6;

};

// Function: Arity 6.
template <typename R, typename X1, typename X2, typename X3, typename X4,
          typename X5, typename X6>
struct FunctionTraits<R(*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R (*NormalizedSig)(X1, X2, X3, X4, X5, X6);
    typedef false_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef X1 B1;
    typedef X2 B2;
    typedef X3 B3;
    typedef X4 B4;
    typedef X5 B5;
    typedef X6 B6;

};

// Method: Arity 6.
template <typename R, typename T, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
struct FunctionTraits<R(T::*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R (T::*NormalizedSig)(X1, X2, X3, X4, X5, X6);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;
    typedef X3 B4;
    typedef X4 B5;
    typedef X5 B6;
    typedef X6 B7;

};

// Const Method: Arity 6.
template <typename R, typename T, typename X1, typename X2, typename X3,
          typename X4, typename X5, typename X6>
struct FunctionTraits<R(T::*)(X1, X2, X3, X4, X5, X6) const>
{
    typedef R (T::*NormalizedSig)(X1, X2, X3, X4, X5, X6);
    typedef true_type IsMethod;

    typedef R Return;

    // Target type for each bound parameter.
    typedef T B1;
    typedef X1 B2;
    typedef X2 B3;
    typedef X3 B4;
    typedef X4 B5;
    typedef X5 B6;
    typedef X6 B7;

};

// InvokerN<>
//
// The InvokerN templates contain a static DoInvoke() function that is the key
// to implementing type erasure in the Callback() classes.
//
// DoInvoke() is a static function with a fixed signature that is independent
// of StorageType; its first argument is a pointer to the non-templated common
// baseclass of StorageType. This lets us store pointer to DoInvoke() in a
// function pointer that has knowledge of the specific StorageType, and thus
// no knowledge of the bound function and bound parameter types.
//
// As long as we ensure that DoInvoke() is only used with pointers there were
// upcasted from the correct StorageType, we can be sure that execution is
// safe.
//
// The InvokerN templates are the only point that knows the number of bound
// and unbound arguments.  This is intentional because it allows the other
// templates classes in the system to only have as many specializations as
// the max arity of function we wish to support.

template <bool IsWeak, typename StorageType, typename NormalizedSig>
struct Invoker0;

// Function: Arity 0 -> 0.
template <typename StorageType, typename R>
struct Invoker0<false, StorageType, R(*)()>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_();
}
};

// Function: Arity 1 -> 1.
template <typename StorageType, typename R,typename X1>
struct Invoker0<false, StorageType, R(*)(X1)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(x1);
}
};

// Function: Arity 2 -> 2.
template <typename StorageType, typename R,typename X1, typename X2>
struct Invoker0<false, StorageType, R(*)(X1, X2)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(x1, x2);
}
};

// Function: Arity 3 -> 3.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3>
struct Invoker0<false, StorageType, R(*)(X1, X2, X3)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(x1, x2, x3);
}
};

// Function: Arity 4 -> 4.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4>
struct Invoker0<false, StorageType, R(*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(x1, x2, x3, x4);
}
};

// Function: Arity 5 -> 5.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker0<false, StorageType, R(*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(x1, x2, x3, x4, x5);
}
};

// Function: Arity 6 -> 6.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
struct Invoker0<false, StorageType, R(*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType,
        typename internal::ParamTraits<X6>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5,
                      typename internal::ParamTraits<X6>::ForwardType x6)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(x1, x2, x3, x4, x5, x6);
}
};

template <bool IsWeak, typename StorageType, typename NormalizedSig>
struct Invoker1;

// Function: Arity 1 -> 0.
template <typename StorageType, typename R,typename X1>
struct Invoker1<false, StorageType, R(*)(X1)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_));
}
};

// Method: Arity 0 -> 0.
template <typename StorageType, typename R, typename T>
struct Invoker1<false, StorageType, R(T::*)()>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)();
}
};

// WeakPtr Method: Arity 0 -> 0.
template <typename StorageType, typename T>
struct Invoker1<true, StorageType, void(T::*)()>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static void DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)();
}
};

// Function: Arity 2 -> 1.
template <typename StorageType, typename R,typename X1, typename X2>
struct Invoker1<false, StorageType, R(*)(X1, X2)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), x2);
}
};

// Method: Arity 1 -> 1.
template <typename StorageType, typename R, typename T, typename X1>
struct Invoker1<false, StorageType, R(T::*)(X1)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(x1);
}
};

// WeakPtr Method: Arity 1 -> 1.
template <typename StorageType, typename T, typename X1>
struct Invoker1<true, StorageType, void(T::*)(X1)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X1>::ForwardType x1)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(x1);
}
};

// Function: Arity 3 -> 2.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3>
struct Invoker1<false, StorageType, R(*)(X1, X2, X3)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), x2, x3);
}
};

// Method: Arity 2 -> 2.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2>
struct Invoker1<false, StorageType, R(T::*)(X1, X2)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(x1, x2);
}
};

// WeakPtr Method: Arity 2 -> 2.
template <typename StorageType, typename T, typename X1, typename X2>
struct Invoker1<true, StorageType, void(T::*)(X1, X2)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X1>::ForwardType x1,
                         typename internal::ParamTraits<X2>::ForwardType x2)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(x1, x2);
}
};

// Function: Arity 4 -> 3.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4>
struct Invoker1<false, StorageType, R(*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), x2, x3, x4);
}
};

// Method: Arity 3 -> 3.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3>
struct Invoker1<false, StorageType, R(T::*)(X1, X2, X3)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(x1, x2, x3);
}
};

// WeakPtr Method: Arity 3 -> 3.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3>
struct Invoker1<true, StorageType, void(T::*)(X1, X2, X3)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X1>::ForwardType x1,
                         typename internal::ParamTraits<X2>::ForwardType x2,
                         typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(x1, x2, x3);
}
};

// Function: Arity 5 -> 4.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker1<false, StorageType, R(*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), x2, x3, x4, x5);
}
};

// Method: Arity 4 -> 4.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4>
struct Invoker1<false, StorageType, R(T::*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(x1, x2, x3, x4);
}
};

// WeakPtr Method: Arity 4 -> 4.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4>
struct Invoker1<true, StorageType, void(T::*)(X1, X2, X3, X4)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X1>::ForwardType x1,
                         typename internal::ParamTraits<X2>::ForwardType x2,
                         typename internal::ParamTraits<X3>::ForwardType x3,
                         typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(x1, x2, x3, x4);
}
};

// Function: Arity 6 -> 5.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
struct Invoker1<false, StorageType, R(*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType,
        typename internal::ParamTraits<X6>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5,
                      typename internal::ParamTraits<X6>::ForwardType x6)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), x2, x3, x4, x5, x6);
}
};

// Method: Arity 5 -> 5.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4, typename X5>
struct Invoker1<false, StorageType, R(T::*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X1>::ForwardType x1,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(x1, x2, x3, x4, x5);
}
};

// WeakPtr Method: Arity 5 -> 5.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker1<true, StorageType, void(T::*)(X1, X2, X3, X4, X5)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X1>::ForwardType,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X1>::ForwardType x1,
                         typename internal::ParamTraits<X2>::ForwardType x2,
                         typename internal::ParamTraits<X3>::ForwardType x3,
                         typename internal::ParamTraits<X4>::ForwardType x4,
                         typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(x1, x2, x3, x4, x5);
}
};

template <bool IsWeak, typename StorageType, typename NormalizedSig>
struct Invoker2;

// Function: Arity 2 -> 0.
template <typename StorageType, typename R,typename X1, typename X2>
struct Invoker2<false, StorageType, R(*)(X1, X2)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_));
}
};

// Method: Arity 1 -> 0.
template <typename StorageType, typename R, typename T, typename X1>
struct Invoker2<false, StorageType, R(T::*)(X1)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_));
}
};

// WeakPtr Method: Arity 1 -> 0.
template <typename StorageType, typename T, typename X1>
struct Invoker2<true, StorageType, void(T::*)(X1)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static void DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_));
}
};

// Function: Arity 3 -> 1.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3>
struct Invoker2<false, StorageType, R(*)(X1, X2, X3)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_), x3);
}
};

// Method: Arity 2 -> 1.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2>
struct Invoker2<false, StorageType, R(T::*)(X1, X2)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_), x2);
}
};

// WeakPtr Method: Arity 2 -> 1.
template <typename StorageType, typename T, typename X1, typename X2>
struct Invoker2<true, StorageType, void(T::*)(X1, X2)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X2>::ForwardType x2)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), x2);
}
};

// Function: Arity 4 -> 2.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4>
struct Invoker2<false, StorageType, R(*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_), x3, x4);
}
};

// Method: Arity 3 -> 2.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3>
struct Invoker2<false, StorageType, R(T::*)(X1, X2, X3)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_), x2, x3);
}
};

// WeakPtr Method: Arity 3 -> 2.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3>
struct Invoker2<true, StorageType, void(T::*)(X1, X2, X3)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X2>::ForwardType x2,
                         typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), x2, x3);
}
};

// Function: Arity 5 -> 3.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker2<false, StorageType, R(*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_), x3, x4, x5);
}
};

// Method: Arity 4 -> 3.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4>
struct Invoker2<false, StorageType, R(T::*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_), x2, x3,
            x4);
}
};

// WeakPtr Method: Arity 4 -> 3.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4>
struct Invoker2<true, StorageType, void(T::*)(X1, X2, X3, X4)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X2>::ForwardType x2,
                         typename internal::ParamTraits<X3>::ForwardType x3,
                         typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), x2, x3, x4);
}
};

// Function: Arity 6 -> 4.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
struct Invoker2<false, StorageType, R(*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType,
        typename internal::ParamTraits<X6>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5,
                      typename internal::ParamTraits<X6>::ForwardType x6)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_), x3, x4, x5,
                       x6);
}
};

// Method: Arity 5 -> 4.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4, typename X5>
struct Invoker2<false, StorageType, R(T::*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X2>::ForwardType x2,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_), x2, x3,
            x4, x5);
}
};

// WeakPtr Method: Arity 5 -> 4.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker2<true, StorageType, void(T::*)(X1, X2, X3, X4, X5)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X2>::ForwardType,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X2>::ForwardType x2,
                         typename internal::ParamTraits<X3>::ForwardType x3,
                         typename internal::ParamTraits<X4>::ForwardType x4,
                         typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), x2, x3, x4, x5);
}
};

template <bool IsWeak, typename StorageType, typename NormalizedSig>
struct Invoker3;

// Function: Arity 3 -> 0.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3>
struct Invoker3<false, StorageType, R(*)(X1, X2, X3)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_));
}
};

// Method: Arity 2 -> 0.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2>
struct Invoker3<false, StorageType, R(T::*)(X1, X2)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_));
}
};

// WeakPtr Method: Arity 2 -> 0.
template <typename StorageType, typename T, typename X1, typename X2>
struct Invoker3<true, StorageType, void(T::*)(X1, X2)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static void DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_));
}
};

// Function: Arity 4 -> 1.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4>
struct Invoker3<false, StorageType, R(*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X4>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), x4);
}
};

// Method: Arity 3 -> 1.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3>
struct Invoker3<false, StorageType, R(T::*)(X1, X2, X3)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), x3);
}
};

// WeakPtr Method: Arity 3 -> 1.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3>
struct Invoker3<true, StorageType, void(T::*)(X1, X2, X3)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X3>::ForwardType x3)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_), x3);
}
};

// Function: Arity 5 -> 2.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker3<false, StorageType, R(*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), x4, x5);
}
};

// Method: Arity 4 -> 2.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4>
struct Invoker3<false, StorageType, R(T::*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), x3, x4);
}
};

// WeakPtr Method: Arity 4 -> 2.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4>
struct Invoker3<true, StorageType, void(T::*)(X1, X2, X3, X4)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X3>::ForwardType x3,
                         typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_), x3,
                             x4);
}
};

// Function: Arity 6 -> 3.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
struct Invoker3<false, StorageType, R(*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType,
        typename internal::ParamTraits<X6>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5,
                      typename internal::ParamTraits<X6>::ForwardType x6)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), x4, x5, x6);
}
};

// Method: Arity 5 -> 3.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4, typename X5>
struct Invoker3<false, StorageType, R(T::*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X3>::ForwardType x3,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), x3, x4, x5);
}
};

// WeakPtr Method: Arity 5 -> 3.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker3<true, StorageType, void(T::*)(X1, X2, X3, X4, X5)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X3>::ForwardType,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X3>::ForwardType x3,
                         typename internal::ParamTraits<X4>::ForwardType x4,
                         typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_), x3,
                             x4, x5);
}
};

template <bool IsWeak, typename StorageType, typename NormalizedSig>
struct Invoker4;

// Function: Arity 4 -> 0.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4>
struct Invoker4<false, StorageType, R(*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), Unwrap(invoker->p4_));
}
};

// Method: Arity 3 -> 0.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3>
struct Invoker4<false, StorageType, R(T::*)(X1, X2, X3)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), Unwrap(invoker->p4_));
}
};

// WeakPtr Method: Arity 3 -> 0.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3>
struct Invoker4<true, StorageType, void(T::*)(X1, X2, X3)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static void DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_),
                             Unwrap(invoker->p4_));
}
};

// Function: Arity 5 -> 1.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker4<false, StorageType, R(*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), Unwrap(invoker->p4_), x5);
}
};

// Method: Arity 4 -> 1.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4>
struct Invoker4<false, StorageType, R(T::*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X4>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), Unwrap(invoker->p4_), x4);
}
};

// WeakPtr Method: Arity 4 -> 1.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4>
struct Invoker4<true, StorageType, void(T::*)(X1, X2, X3, X4)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X4>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X4>::ForwardType x4)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_),
                             Unwrap(invoker->p4_), x4);
}
};

// Function: Arity 6 -> 2.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
struct Invoker4<false, StorageType, R(*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X5>::ForwardType,
        typename internal::ParamTraits<X6>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X5>::ForwardType x5,
                      typename internal::ParamTraits<X6>::ForwardType x6)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), Unwrap(invoker->p4_), x5, x6);
}
};

// Method: Arity 5 -> 2.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4, typename X5>
struct Invoker4<false, StorageType, R(T::*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X4>::ForwardType x4,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), Unwrap(invoker->p4_), x4, x5);
}
};

// WeakPtr Method: Arity 5 -> 2.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker4<true, StorageType, void(T::*)(X1, X2, X3, X4, X5)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X4>::ForwardType,
        typename internal::ParamTraits<X5>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X4>::ForwardType x4,
                         typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_),
                             Unwrap(invoker->p4_), x4, x5);
}
};

template <bool IsWeak, typename StorageType, typename NormalizedSig>
struct Invoker5;

// Function: Arity 5 -> 0.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker5<false, StorageType, R(*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), Unwrap(invoker->p4_), Unwrap(invoker->p5_));
}
};

// Method: Arity 4 -> 0.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4>
struct Invoker5<false, StorageType, R(T::*)(X1, X2, X3, X4)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), Unwrap(invoker->p4_), Unwrap(invoker->p5_));
}
};

// WeakPtr Method: Arity 4 -> 0.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4>
struct Invoker5<true, StorageType, void(T::*)(X1, X2, X3, X4)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static void DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_),
                             Unwrap(invoker->p4_), Unwrap(invoker->p5_));
}
};

// Function: Arity 6 -> 1.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
struct Invoker5<false, StorageType, R(*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X6>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X6>::ForwardType x6)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), Unwrap(invoker->p4_), Unwrap(invoker->p5_), x6);
}
};

// Method: Arity 5 -> 1.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4, typename X5>
struct Invoker5<false, StorageType, R(T::*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X5>::ForwardType);

    static R DoInvoke(InvokerStorageBase* base,
                      typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), Unwrap(invoker->p4_), Unwrap(invoker->p5_), x5);
}
};

// WeakPtr Method: Arity 5 -> 1.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker5<true, StorageType, void(T::*)(X1, X2, X3, X4, X5)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<X5>::ForwardType);

    static void DoInvoke(InvokerStorageBase* base,
                         typename internal::ParamTraits<X5>::ForwardType x5)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_),
                             Unwrap(invoker->p4_), Unwrap(invoker->p5_), x5);
}
};

template <bool IsWeak, typename StorageType, typename NormalizedSig>
struct Invoker6;

// Function: Arity 6 -> 0.
template <typename StorageType, typename R,typename X1, typename X2,
          typename X3, typename X4, typename X5, typename X6>
struct Invoker6<false, StorageType, R(*)(X1, X2, X3, X4, X5, X6)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return invoker->f_(Unwrap(invoker->p1_), Unwrap(invoker->p2_),
                       Unwrap(invoker->p3_), Unwrap(invoker->p4_), Unwrap(invoker->p5_),
                       Unwrap(invoker->p6_));
}
};

// Method: Arity 5 -> 0.
template <typename StorageType, typename R, typename T, typename X1,
          typename X2, typename X3, typename X4, typename X5>
struct Invoker6<false, StorageType, R(T::*)(X1, X2, X3, X4, X5)>
{
    typedef R(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static R DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    return (Unwrap(invoker->p1_)->*invoker->f_)(Unwrap(invoker->p2_),
            Unwrap(invoker->p3_), Unwrap(invoker->p4_), Unwrap(invoker->p5_),
            Unwrap(invoker->p6_));
}
};

// WeakPtr Method: Arity 5 -> 0.
template <typename StorageType, typename T, typename X1, typename X2,
          typename X3, typename X4, typename X5>
struct Invoker6<true, StorageType, void(T::*)(X1, X2, X3, X4, X5)>
{
    typedef void(*DoInvokeType)(
        internal::InvokerStorageBase*);

    static void DoInvoke(InvokerStorageBase* base)
{
    StorageType* invoker = static_cast<StorageType*>(base);
    typename StorageType::P1Traits::StorageType& weak_ptr = invoker->p1_;
    if (!weak_ptr.get())
    {
        return;
    }
    (weak_ptr->*invoker->f_)(Unwrap(invoker->p2_), Unwrap(invoker->p3_),
                             Unwrap(invoker->p4_), Unwrap(invoker->p5_), Unwrap(invoker->p6_));
}
};

// InvokerStorageN<>
//
// These are the actual storage classes for the Invokers.
//
// Though these types are "classes", they are being used as structs with
// all member variable public.  We cannot make it a struct because it inherits
// from a class which causes a compiler warning.  We cannot add a "Run()" method
// that forwards the unbound arguments because that would require we unwrap the
// Sig type like in InvokerN above to know the return type, and the arity
// of Run().
//
// An alternate solution would be to merge InvokerN and InvokerStorageN,
// but the generated code seemed harder to read.

template <typename Sig>
class InvokerStorage0 : public InvokerStorageBase
{
public:
    typedef InvokerStorage0 StorageType;
    typedef FunctionTraits<Sig> TargetTraits;
    typedef typename TargetTraits::IsMethod IsMethod;
    typedef Sig Signature;
    typedef Invoker0<false, StorageType,
            typename TargetTraits::NormalizedSig> Invoker;



    InvokerStorage0(Sig f)
        : f_(f)
    {
    }

    virtual ~InvokerStorage0() {  }

    Sig f_;
};

template <typename Sig, typename P1>
class InvokerStorage1 : public InvokerStorageBase
{
public:
    typedef InvokerStorage1 StorageType;
    typedef FunctionTraits<Sig> TargetTraits;
    typedef typename TargetTraits::IsMethod IsMethod;
    typedef Sig Signature;
    typedef ParamTraits<P1> P1Traits;
    typedef Invoker1<IsWeakMethod<IsMethod::value, P1>::value, StorageType,
            typename TargetTraits::NormalizedSig> Invoker;
    COMPILE_ASSERT(!(IsWeakMethod<IsMethod::value, P1>::value) ||
                   is_void<typename TargetTraits::Return>::value,
                   weak_ptrs_can_only_bind_to_methods_without_return_values);

    // For methods, we need to be careful for parameter 1.  We skip the
    // scoped_refptr check because the binder itself takes care of this. We also
    // disallow binding of an array as the method's target object.
    COMPILE_ASSERT(IsMethod::value ||
                   !internal::UnsafeBindtoRefCountedArg<P1>::value,
                   p1_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!IsMethod::value || !is_array<P1>::value,
                   first_bound_argument_to_method_cannot_be_array);

    // Do not allow binding a non-const reference parameter. Non-const reference
    // parameters are disallowed by the Google style guide.  Also, binding a
    // non-const reference parameter can make for subtle bugs because the
    // invoked function will receive a reference to the stored copy of the
    // argument and not the original.
    COMPILE_ASSERT(
        !( is_non_const_reference<typename TargetTraits::B1>::value ),
        do_not_bind_functions_with_nonconst_ref);


    InvokerStorage1(Sig f, const P1& p1)
        : f_(f), p1_(static_cast<typename ParamTraits<P1>::StorageType>(p1))
    {
        MaybeRefcount<IsMethod, P1>::AddRef(p1_);
    }

    virtual ~InvokerStorage1()
    {
        MaybeRefcount<IsMethod, P1>::Release(p1_);
    }

    Sig f_;
    typename ParamTraits<P1>::StorageType p1_;
};

template <typename Sig, typename P1, typename P2>
class InvokerStorage2 : public InvokerStorageBase
{
public:
    typedef InvokerStorage2 StorageType;
    typedef FunctionTraits<Sig> TargetTraits;
    typedef typename TargetTraits::IsMethod IsMethod;
    typedef Sig Signature;
    typedef ParamTraits<P1> P1Traits;
    typedef ParamTraits<P2> P2Traits;
    typedef Invoker2<IsWeakMethod<IsMethod::value, P1>::value, StorageType,
            typename TargetTraits::NormalizedSig> Invoker;
    COMPILE_ASSERT(!(IsWeakMethod<IsMethod::value, P1>::value) ||
                   is_void<typename TargetTraits::Return>::value,
                   weak_ptrs_can_only_bind_to_methods_without_return_values);

    // For methods, we need to be careful for parameter 1.  We skip the
    // scoped_refptr check because the binder itself takes care of this. We also
    // disallow binding of an array as the method's target object.
    COMPILE_ASSERT(IsMethod::value ||
                   !internal::UnsafeBindtoRefCountedArg<P1>::value,
                   p1_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!IsMethod::value || !is_array<P1>::value,
                   first_bound_argument_to_method_cannot_be_array);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P2>::value,
                   p2_is_refcounted_type_and_needs_scoped_refptr);

    // Do not allow binding a non-const reference parameter. Non-const reference
    // parameters are disallowed by the Google style guide.  Also, binding a
    // non-const reference parameter can make for subtle bugs because the
    // invoked function will receive a reference to the stored copy of the
    // argument and not the original.
    COMPILE_ASSERT(
        !( is_non_const_reference<typename TargetTraits::B1>::value ||
           is_non_const_reference<typename TargetTraits::B2>::value ),
        do_not_bind_functions_with_nonconst_ref);


    InvokerStorage2(Sig f, const P1& p1, const P2& p2)
        : f_(f), p1_(static_cast<typename ParamTraits<P1>::StorageType>(p1)),
          p2_(static_cast<typename ParamTraits<P2>::StorageType>(p2))
    {
        MaybeRefcount<IsMethod, P1>::AddRef(p1_);
    }

    virtual ~InvokerStorage2()
    {
        MaybeRefcount<IsMethod, P1>::Release(p1_);
    }

    Sig f_;
    typename ParamTraits<P1>::StorageType p1_;
    typename ParamTraits<P2>::StorageType p2_;
};

template <typename Sig, typename P1, typename P2, typename P3>
class InvokerStorage3 : public InvokerStorageBase
{
public:
    typedef InvokerStorage3 StorageType;
    typedef FunctionTraits<Sig> TargetTraits;
    typedef typename TargetTraits::IsMethod IsMethod;
    typedef Sig Signature;
    typedef ParamTraits<P1> P1Traits;
    typedef ParamTraits<P2> P2Traits;
    typedef ParamTraits<P3> P3Traits;
    typedef Invoker3<IsWeakMethod<IsMethod::value, P1>::value, StorageType,
            typename TargetTraits::NormalizedSig> Invoker;
    COMPILE_ASSERT(!(IsWeakMethod<IsMethod::value, P1>::value) ||
                   is_void<typename TargetTraits::Return>::value,
                   weak_ptrs_can_only_bind_to_methods_without_return_values);

    // For methods, we need to be careful for parameter 1.  We skip the
    // scoped_refptr check because the binder itself takes care of this. We also
    // disallow binding of an array as the method's target object.
    COMPILE_ASSERT(IsMethod::value ||
                   !internal::UnsafeBindtoRefCountedArg<P1>::value,
                   p1_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!IsMethod::value || !is_array<P1>::value,
                   first_bound_argument_to_method_cannot_be_array);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P2>::value,
                   p2_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P3>::value,
                   p3_is_refcounted_type_and_needs_scoped_refptr);

    // Do not allow binding a non-const reference parameter. Non-const reference
    // parameters are disallowed by the Google style guide.  Also, binding a
    // non-const reference parameter can make for subtle bugs because the
    // invoked function will receive a reference to the stored copy of the
    // argument and not the original.
    COMPILE_ASSERT(
        !( is_non_const_reference<typename TargetTraits::B1>::value ||
           is_non_const_reference<typename TargetTraits::B2>::value ||
           is_non_const_reference<typename TargetTraits::B3>::value ),
        do_not_bind_functions_with_nonconst_ref);


    InvokerStorage3(Sig f, const P1& p1, const P2& p2, const P3& p3)
        : f_(f), p1_(static_cast<typename ParamTraits<P1>::StorageType>(p1)),
          p2_(static_cast<typename ParamTraits<P2>::StorageType>(p2)),
          p3_(static_cast<typename ParamTraits<P3>::StorageType>(p3))
    {
        MaybeRefcount<IsMethod, P1>::AddRef(p1_);
    }

    virtual ~InvokerStorage3()
    {
        MaybeRefcount<IsMethod, P1>::Release(p1_);
    }

    Sig f_;
    typename ParamTraits<P1>::StorageType p1_;
    typename ParamTraits<P2>::StorageType p2_;
    typename ParamTraits<P3>::StorageType p3_;
};

template <typename Sig, typename P1, typename P2, typename P3, typename P4>
class InvokerStorage4 : public InvokerStorageBase
{
public:
    typedef InvokerStorage4 StorageType;
    typedef FunctionTraits<Sig> TargetTraits;
    typedef typename TargetTraits::IsMethod IsMethod;
    typedef Sig Signature;
    typedef ParamTraits<P1> P1Traits;
    typedef ParamTraits<P2> P2Traits;
    typedef ParamTraits<P3> P3Traits;
    typedef ParamTraits<P4> P4Traits;
    typedef Invoker4<IsWeakMethod<IsMethod::value, P1>::value, StorageType,
            typename TargetTraits::NormalizedSig> Invoker;
    COMPILE_ASSERT(!(IsWeakMethod<IsMethod::value, P1>::value) ||
                   is_void<typename TargetTraits::Return>::value,
                   weak_ptrs_can_only_bind_to_methods_without_return_values);

    // For methods, we need to be careful for parameter 1.  We skip the
    // scoped_refptr check because the binder itself takes care of this. We also
    // disallow binding of an array as the method's target object.
    COMPILE_ASSERT(IsMethod::value ||
                   !internal::UnsafeBindtoRefCountedArg<P1>::value,
                   p1_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!IsMethod::value || !is_array<P1>::value,
                   first_bound_argument_to_method_cannot_be_array);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P2>::value,
                   p2_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P3>::value,
                   p3_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P4>::value,
                   p4_is_refcounted_type_and_needs_scoped_refptr);

    // Do not allow binding a non-const reference parameter. Non-const reference
    // parameters are disallowed by the Google style guide.  Also, binding a
    // non-const reference parameter can make for subtle bugs because the
    // invoked function will receive a reference to the stored copy of the
    // argument and not the original.
    COMPILE_ASSERT(
        !( is_non_const_reference<typename TargetTraits::B1>::value ||
           is_non_const_reference<typename TargetTraits::B2>::value ||
           is_non_const_reference<typename TargetTraits::B3>::value ||
           is_non_const_reference<typename TargetTraits::B4>::value ),
        do_not_bind_functions_with_nonconst_ref);


    InvokerStorage4(Sig f, const P1& p1, const P2& p2, const P3& p3, const P4& p4)
        : f_(f), p1_(static_cast<typename ParamTraits<P1>::StorageType>(p1)),
          p2_(static_cast<typename ParamTraits<P2>::StorageType>(p2)),
          p3_(static_cast<typename ParamTraits<P3>::StorageType>(p3)),
          p4_(static_cast<typename ParamTraits<P4>::StorageType>(p4))
    {
        MaybeRefcount<IsMethod, P1>::AddRef(p1_);
    }

    virtual ~InvokerStorage4()
    {
        MaybeRefcount<IsMethod, P1>::Release(p1_);
    }

    Sig f_;
    typename ParamTraits<P1>::StorageType p1_;
    typename ParamTraits<P2>::StorageType p2_;
    typename ParamTraits<P3>::StorageType p3_;
    typename ParamTraits<P4>::StorageType p4_;
};

template <typename Sig, typename P1, typename P2, typename P3, typename P4,
          typename P5>
class InvokerStorage5 : public InvokerStorageBase
{
public:
    typedef InvokerStorage5 StorageType;
    typedef FunctionTraits<Sig> TargetTraits;
    typedef typename TargetTraits::IsMethod IsMethod;
    typedef Sig Signature;
    typedef ParamTraits<P1> P1Traits;
    typedef ParamTraits<P2> P2Traits;
    typedef ParamTraits<P3> P3Traits;
    typedef ParamTraits<P4> P4Traits;
    typedef ParamTraits<P5> P5Traits;
    typedef Invoker5<IsWeakMethod<IsMethod::value, P1>::value, StorageType,
            typename TargetTraits::NormalizedSig> Invoker;
    COMPILE_ASSERT(!(IsWeakMethod<IsMethod::value, P1>::value) ||
                   is_void<typename TargetTraits::Return>::value,
                   weak_ptrs_can_only_bind_to_methods_without_return_values);

    // For methods, we need to be careful for parameter 1.  We skip the
    // scoped_refptr check because the binder itself takes care of this. We also
    // disallow binding of an array as the method's target object.
    COMPILE_ASSERT(IsMethod::value ||
                   !internal::UnsafeBindtoRefCountedArg<P1>::value,
                   p1_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!IsMethod::value || !is_array<P1>::value,
                   first_bound_argument_to_method_cannot_be_array);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P2>::value,
                   p2_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P3>::value,
                   p3_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P4>::value,
                   p4_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P5>::value,
                   p5_is_refcounted_type_and_needs_scoped_refptr);

    // Do not allow binding a non-const reference parameter. Non-const reference
    // parameters are disallowed by the Google style guide.  Also, binding a
    // non-const reference parameter can make for subtle bugs because the
    // invoked function will receive a reference to the stored copy of the
    // argument and not the original.
    COMPILE_ASSERT(
        !( is_non_const_reference<typename TargetTraits::B1>::value ||
           is_non_const_reference<typename TargetTraits::B2>::value ||
           is_non_const_reference<typename TargetTraits::B3>::value ||
           is_non_const_reference<typename TargetTraits::B4>::value ||
           is_non_const_reference<typename TargetTraits::B5>::value ),
        do_not_bind_functions_with_nonconst_ref);


    InvokerStorage5(Sig f, const P1& p1, const P2& p2, const P3& p3,
                    const P4& p4, const P5& p5)
        : f_(f), p1_(static_cast<typename ParamTraits<P1>::StorageType>(p1)),
          p2_(static_cast<typename ParamTraits<P2>::StorageType>(p2)),
          p3_(static_cast<typename ParamTraits<P3>::StorageType>(p3)),
          p4_(static_cast<typename ParamTraits<P4>::StorageType>(p4)),
          p5_(static_cast<typename ParamTraits<P5>::StorageType>(p5))
    {
        MaybeRefcount<IsMethod, P1>::AddRef(p1_);
    }

    virtual ~InvokerStorage5()
    {
        MaybeRefcount<IsMethod, P1>::Release(p1_);
    }

    Sig f_;
    typename ParamTraits<P1>::StorageType p1_;
    typename ParamTraits<P2>::StorageType p2_;
    typename ParamTraits<P3>::StorageType p3_;
    typename ParamTraits<P4>::StorageType p4_;
    typename ParamTraits<P5>::StorageType p5_;
};

template <typename Sig, typename P1, typename P2, typename P3, typename P4,
          typename P5, typename P6>
class InvokerStorage6 : public InvokerStorageBase
{
public:
    typedef InvokerStorage6 StorageType;
    typedef FunctionTraits<Sig> TargetTraits;
    typedef typename TargetTraits::IsMethod IsMethod;
    typedef Sig Signature;
    typedef ParamTraits<P1> P1Traits;
    typedef ParamTraits<P2> P2Traits;
    typedef ParamTraits<P3> P3Traits;
    typedef ParamTraits<P4> P4Traits;
    typedef ParamTraits<P5> P5Traits;
    typedef ParamTraits<P6> P6Traits;
    typedef Invoker6<IsWeakMethod<IsMethod::value, P1>::value, StorageType,
            typename TargetTraits::NormalizedSig> Invoker;
    COMPILE_ASSERT(!(IsWeakMethod<IsMethod::value, P1>::value) ||
                   is_void<typename TargetTraits::Return>::value,
                   weak_ptrs_can_only_bind_to_methods_without_return_values);

    // For methods, we need to be careful for parameter 1.  We skip the
    // scoped_refptr check because the binder itself takes care of this. We also
    // disallow binding of an array as the method's target object.
    COMPILE_ASSERT(IsMethod::value ||
                   !internal::UnsafeBindtoRefCountedArg<P1>::value,
                   p1_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!IsMethod::value || !is_array<P1>::value,
                   first_bound_argument_to_method_cannot_be_array);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P2>::value,
                   p2_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P3>::value,
                   p3_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P4>::value,
                   p4_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P5>::value,
                   p5_is_refcounted_type_and_needs_scoped_refptr);
    COMPILE_ASSERT(!internal::UnsafeBindtoRefCountedArg<P6>::value,
                   p6_is_refcounted_type_and_needs_scoped_refptr);

    // Do not allow binding a non-const reference parameter. Non-const reference
    // parameters are disallowed by the Google style guide.  Also, binding a
    // non-const reference parameter can make for subtle bugs because the
    // invoked function will receive a reference to the stored copy of the
    // argument and not the original.
    COMPILE_ASSERT(
        !( is_non_const_reference<typename TargetTraits::B1>::value ||
           is_non_const_reference<typename TargetTraits::B2>::value ||
           is_non_const_reference<typename TargetTraits::B3>::value ||
           is_non_const_reference<typename TargetTraits::B4>::value ||
           is_non_const_reference<typename TargetTraits::B5>::value ||
           is_non_const_reference<typename TargetTraits::B6>::value ),
        do_not_bind_functions_with_nonconst_ref);


    InvokerStorage6(Sig f, const P1& p1, const P2& p2, const P3& p3,
                    const P4& p4, const P5& p5, const P6& p6)
        : f_(f), p1_(static_cast<typename ParamTraits<P1>::StorageType>(p1)),
          p2_(static_cast<typename ParamTraits<P2>::StorageType>(p2)),
          p3_(static_cast<typename ParamTraits<P3>::StorageType>(p3)),
          p4_(static_cast<typename ParamTraits<P4>::StorageType>(p4)),
          p5_(static_cast<typename ParamTraits<P5>::StorageType>(p5)),
          p6_(static_cast<typename ParamTraits<P6>::StorageType>(p6))
    {
        MaybeRefcount<IsMethod, P1>::AddRef(p1_);
    }

    virtual ~InvokerStorage6()
    {
        MaybeRefcount<IsMethod, P1>::Release(p1_);
    }

    Sig f_;
    typename ParamTraits<P1>::StorageType p1_;
    typename ParamTraits<P2>::StorageType p2_;
    typename ParamTraits<P3>::StorageType p3_;
    typename ParamTraits<P4>::StorageType p4_;
    typename ParamTraits<P5>::StorageType p5_;
    typename ParamTraits<P6>::StorageType p6_;
};

} //namespace internal
} //namespace base

#endif //__base_bind_internal_h__