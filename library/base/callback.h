
#ifndef __base_callback_h__
#define __base_callback_h__

#pragma once

#include "callback_internal.h"
#include "template_util.h"

// New, super-duper, unified Callback system.  This will eventually replace
// NewRunnableMethod, NewRunnableFunction, CreateFunctor, and CreateCallback
// systems currently in the Chromium code base.
//
// WHAT IS THIS:
//
// The templated Callback class is a generalized function object. Together
// with the Bind() function in bind.h, they provide a type-safe method for
// performing currying of arguments, and creating a "closure."
//
// In programing languages, a closure is a first-class function where all its
// parameters have been bound (usually via currying).  Closures are well
// suited for representing, and passing around a unit of delayed execution.
// They are used in Chromium code to schedule tasks on different MessageLoops.
//
//
// MEMORY MANAGEMENT AND PASSING
//
// The Callback objects themselves should be passed by const-reference, and
// stored by copy. They internally store their state via a refcounted class
// and thus do not need to be deleted.
//
// The reason to pass via a const-reference is to avoid unnecessary
// AddRef/Release pairs to the internal state.
//
//
// EXAMPLE USAGE:
//
// /* Binding a normal function. */
// int Return5() { return 5; }
// base::Callback<int(int)> func_cb = base::Bind(&Return5);
// LOG(INFO) << func_cb.Run(5);  // Prints 5.
//
// void PrintHi() { LOG(INFO) << "hi."; }
// base::Closure void_func_cb = base::Bind(&PrintHi);
// LOG(INFO) << void_func_cb.Run();  // Prints: hi.
//
// /* Binding a class method. */
// class Ref : public RefCountedThreadSafe<Ref> {
//  public:
//   int Foo() { return 3; }
//   void PrintBye() { LOG(INFO) << "bye."; }
// };
// scoped_refptr<Ref> ref = new Ref();
// base::Callback<int(void)> ref_cb = base::Bind(&Ref::Foo, ref.get());
// LOG(INFO) << ref_cb.Run();  // Prints out 3.
//
// base::Closure void_ref_cb = base::Bind(&Ref::PrintBye, ref.get());
// void_ref_cb.Run();  // Prints: bye.
//
// /* Binding a class method in a non-refcounted class.
//  *
//  * WARNING: You must be sure the referee outlives the callback!
//  *          This is particularly important if you post a closure to a
//  *          MessageLoop because then it becomes hard to know what the
//  *          lifetime of the referee needs to be.
//  */
// class NoRef {
//  public:
//   int Foo() { return 4; }
//   void PrintWhy() { LOG(INFO) << "why???"; }
// };
// NoRef no_ref;
// base::Callback<int(void)> base::no_ref_cb =
//     base::Bind(&NoRef::Foo, base::Unretained(&no_ref));
// LOG(INFO) << ref_cb.Run();  // Prints out 4.
//
// base::Closure void_no_ref_cb =
//     base::Bind(&NoRef::PrintWhy, base::Unretained(no_ref));
// void_no_ref_cb.Run();  // Prints: why???
//
// /* Binding a reference. */
// int Identity(int n) { return n; }
// int value = 1;
// base::Callback<int(void)> bound_copy_cb = base::Bind(&Identity, value);
// base::Callback<int(void)> bound_ref_cb =
//     base::Bind(&Identity, base::ConstRef(value));
// LOG(INFO) << bound_copy_cb.Run();  // Prints 1.
// LOG(INFO) << bound_ref_cb.Run();  // Prints 1.
// value = 2;
// LOG(INFO) << bound_copy_cb.Run();  // Prints 1.
// LOG(INFO) << bound_ref_cb.Run();  // Prints 2.
//
//
// WHERE IS THIS DESIGN FROM:
//
// The design Callback and Bind is heavily influenced by C++'s
// tr1::function/tr1::bind, and by the "Google Callback" system used inside
// Google.
//
//
// HOW THE IMPLEMENTATION WORKS:
//
// There are three main components to the system:
//   1) The Callback classes.
//   2) The Bind() functions.
//   3) The arguments wrappers (eg., Unretained() and ConstRef()).
//
// The Callback classes represent a generic function pointer. Internally,
// it stores a refcounted piece of state that represents the target function
// and all its bound parameters.  Each Callback specialization has a templated
// constructor that takes an InvokerStorageHolder<> object.  In the context of
// the constructor, the static type of this InvokerStorageHolder<> object
// uniquely identifies the function it is representing, all its bound
// parameters, and a DoInvoke() that is capable of invoking the target.
//
// Callback's constructor is takes the InvokerStorageHolder<> that has the
// full static type and erases the target function type, and the bound
// parameters.  It does this by storing a pointer to the specific DoInvoke()
// function, and upcasting the state of InvokerStorageHolder<> to a
// InvokerStorageBase. This is safe as long as this InvokerStorageBase pointer
// is only used with the stored DoInvoke() pointer.
//
// To create InvokerStorageHolder<> objects, we use the Bind() functions.
// These functions, along with a set of internal templates, are reponsible for
//
//  - Unwrapping the function signature into return type, and parameters
//  - Determining the number of parameters that are bound
//  - Creating the storage for the bound parameters
//  - Performing compile-time asserts to avoid error-prone behavior
//  - Returning an InvokerStorageHolder<> with an DoInvoke() that has an arity
//    matching the number of unbound parameters, and knows the correct
//    refcounting semantics for the target object if we are binding a class
//    method.
//
// The Bind functions do the above using type-inference, and template
// specializations.
//
// By default Bind() will store copies of all bound parameters, and attempt
// to refcount a target object if the function being bound is a class method.
//
// To change this behavior, we introduce a set of argument wrappers
// (eg. Unretained(), and ConstRef()).  These are simple container templates
// that are passed by value, and wrap a pointer to argument.  See the
// file-level comment in base/bind_helpers.h for more info.
//
// These types are passed to the Unwrap() functions, and the MaybeRefcount()
// functions respectively to modify the behavior of Bind().  The Unwrap()
// and MaybeRefcount() functions change behavior by doing partial
// specialization based on whether or not a parameter is a wrapper type.
//
// ConstRef() is similar to tr1::cref.  Unretained() is specific to Chromium.
//
//
// WHY NOT TR1 FUNCTION/BIND?
//
// Direct use of tr1::function and tr1::bind was considered, but ultimately
// rejected because of the number of copy constructors invocations involved
// in the binding of arguments during construction, and the forwarding of
// arguments during invocation.  These copies will no longer be an issue in
// C++0x because C++0x will support rvalue reference allowing for the compiler
// to avoid these copies.  However, waiting for C++0x is not an option.
//
// Measured with valgrind on gcc version 4.4.3 (Ubuntu 4.4.3-4ubuntu5), the
// tr1::bind call itself will invoke a non-trivial copy constructor three times
// for each bound parameter.  Also, each when passing a tr1::function, each
// bound argument will be copied again.
//
// In addition to the copies taken at binding and invocation, copying a
// tr1::function causes a copy to be made of all the bound parameters and
// state.
//
// Furthermore, in Chromium, it is desirable for the Callback to take a
// reference on a target object when representing a class method call.  This
// is not supported by tr1.
//
// Lastly, tr1::function and tr1::bind has a more general and flexible API.
// This includes things like argument reordering by use of
// tr1::bind::placeholder, support for non-const reference parameters, and some
// limited amount of subtyping of the tr1::function object (eg.,
// tr1::function<int(int)> is convertible to tr1::function<void(int)>).
//
// These are not features that are required in Chromium. Some of them, such as
// allowing for reference parameters, and subtyping of functions, may actually
// because a source of errors. Removing support for these features actually
// allows for a simpler implementation, and a terser Currying API.
//
//
// WHY NOT GOOGLE CALLBACKS?
//
// The Google callback system also does not support refcounting.  Furthermore,
// its implementation has a number of strange edge cases with respect to type
// conversion of its arguments.  In particular, the argument's constness must
// at times match exactly the function signature, or the type-inference might
// break.  Given the above, writing a custom solution was easier.
//
//
// MISSING FUNCTIONALITY
//  - Invoking the return of Bind.  Bind(&foo).Run() does not work;
//  - Binding arrays to functions that take a non-const pointer.
//    Example:
//      void Foo(const char* ptr);
//      void Bar(char* ptr);
//      Bind(&Foo, "test");
//      Bind(&Bar, "test");  // This fails because ptr is not const.

namespace base
{

// First, we forward declare the Callback class template. This informs the
// compiler that the template only has 1 type parameter which is the function
// signature that the Callback is representing.
//
// After this, create template specializations for 0-6 parameters. Note that
// even though the template typelist grows, the specialization still
// only has one type: the function signature.
template<typename Sig>
class Callback;

template<typename R>
class Callback<R(void)> : public internal::CallbackBase
{
public:
    typedef R(*PolymorphicInvoke)(internal::InvokerStorageBase*);

    Callback() : CallbackBase(NULL, NULL) {}

    // We pass InvokerStorageHolder by const ref to avoid incurring an
    // unnecessary AddRef/Unref pair even though we will modify the object.
    // We cannot use a normal reference because the compiler will warn
    // since this is often used on a return value, which is a temporary.
    //
    // Note that this constructor CANNOT be explicit, and that Bind() CANNOT
    // return the exact Callback<> type.  See base/bind.h for details.
    template<typename T>
    Callback(const internal::InvokerStorageHolder<T>& invoker_holder)
        : CallbackBase(
            reinterpret_cast<InvokeFuncStorage>(&T::Invoker::DoInvoke),
            &invoker_holder.invoker_storage_)
    {
        COMPILE_ASSERT((is_same<PolymorphicInvoke,
                        typename T::Invoker::DoInvokeType>::value),
                       callback_type_does_not_match_bind_result);
    }

    bool Equals(const Callback& other) const
    {
        return CallbackBase::Equals(other);
    }

    R Run() const
    {
        PolymorphicInvoke f =
            reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

        return f(invoker_storage_.get());
    }
};

template<typename R, typename A1>
class Callback<R(A1)> : public internal::CallbackBase
{
public:
    typedef R(*PolymorphicInvoke)(internal::InvokerStorageBase*,
                                  typename internal::ParamTraits<A1>::ForwardType);

    Callback() : CallbackBase(NULL, NULL) {}

    // We pass InvokerStorageHolder by const ref to avoid incurring an
    // unnecessary AddRef/Unref pair even though we will modify the object.
    // We cannot use a normal reference because the compiler will warn
    // since this is often used on a return value, which is a temporary.
    //
    // Note that this constructor CANNOT be explicit, and that Bind() CANNOT
    // return the exact Callback<> type.  See base/bind.h for details.
    template<typename T>
    Callback(const internal::InvokerStorageHolder<T>& invoker_holder)
        : CallbackBase(
            reinterpret_cast<InvokeFuncStorage>(&T::Invoker::DoInvoke),
            &invoker_holder.invoker_storage_)
    {
        COMPILE_ASSERT((is_same<PolymorphicInvoke,
                        typename T::Invoker::DoInvokeType>::value),
                       callback_type_does_not_match_bind_result);
    }

    bool Equals(const Callback& other) const
    {
        return CallbackBase::Equals(other);
    }

    R Run(typename internal::ParamTraits<A1>::ForwardType a1) const
    {
        PolymorphicInvoke f =
            reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

        return f(invoker_storage_.get(), a1);
    }
};

template<typename R, typename A1, typename A2>
class Callback<R(A1, A2)> : public internal::CallbackBase
{
public:
    typedef R(*PolymorphicInvoke)(internal::InvokerStorageBase*,
                                  typename internal::ParamTraits<A1>::ForwardType,
                                  typename internal::ParamTraits<A2>::ForwardType);

    Callback() : CallbackBase(NULL, NULL) {}

    // We pass InvokerStorageHolder by const ref to avoid incurring an
    // unnecessary AddRef/Unref pair even though we will modify the object.
    // We cannot use a normal reference because the compiler will warn
    // since this is often used on a return value, which is a temporary.
    //
    // Note that this constructor CANNOT be explicit, and that Bind() CANNOT
    // return the exact Callback<> type.  See base/bind.h for details.
    template <typename T>
    Callback(const internal::InvokerStorageHolder<T>& invoker_holder)
        : CallbackBase(
            reinterpret_cast<InvokeFuncStorage>(&T::Invoker::DoInvoke),
            &invoker_holder.invoker_storage_)
    {
        COMPILE_ASSERT((is_same<PolymorphicInvoke,
                        typename T::Invoker::DoInvokeType>::value),
                       callback_type_does_not_match_bind_result);
    }

    bool Equals(const Callback& other) const
    {
        return CallbackBase::Equals(other);
    }

    R Run(typename internal::ParamTraits<A1>::ForwardType a1,
          typename internal::ParamTraits<A2>::ForwardType a2) const
    {
        PolymorphicInvoke f =
            reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

        return f(invoker_storage_.get(), a1, a2);
    }
};

template<typename R, typename A1, typename A2, typename A3>
class Callback<R(A1, A2, A3)> : public internal::CallbackBase
{
public:
    typedef R(*PolymorphicInvoke)(internal::InvokerStorageBase*,
                                  typename internal::ParamTraits<A1>::ForwardType,
                                  typename internal::ParamTraits<A2>::ForwardType,
                                  typename internal::ParamTraits<A3>::ForwardType);

    Callback() : CallbackBase(NULL, NULL) {}

    // We pass InvokerStorageHolder by const ref to avoid incurring an
    // unnecessary AddRef/Unref pair even though we will modify the object.
    // We cannot use a normal reference because the compiler will warn
    // since this is often used on a return value, which is a temporary.
    //
    // Note that this constructor CANNOT be explicit, and that Bind() CANNOT
    // return the exact Callback<> type.  See base/bind.h for details.
    template<typename T>
    Callback(const internal::InvokerStorageHolder<T>& invoker_holder)
        : CallbackBase(
            reinterpret_cast<InvokeFuncStorage>(&T::Invoker::DoInvoke),
            &invoker_holder.invoker_storage_)
    {
        COMPILE_ASSERT((is_same<PolymorphicInvoke,
                        typename T::Invoker::DoInvokeType>::value),
                       callback_type_does_not_match_bind_result);
    }

    bool Equals(const Callback& other) const
    {
        return CallbackBase::Equals(other);
    }

    R Run(typename internal::ParamTraits<A1>::ForwardType a1,
          typename internal::ParamTraits<A2>::ForwardType a2,
          typename internal::ParamTraits<A3>::ForwardType a3) const
    {
        PolymorphicInvoke f =
            reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

        return f(invoker_storage_.get(), a1, a2, a3);
    }
};

template<typename R, typename A1, typename A2, typename A3, typename A4>
class Callback<R(A1, A2, A3, A4)> : public internal::CallbackBase
{
public:
    typedef R(*PolymorphicInvoke)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<A1>::ForwardType,
        typename internal::ParamTraits<A2>::ForwardType,
        typename internal::ParamTraits<A3>::ForwardType,
        typename internal::ParamTraits<A4>::ForwardType);

    Callback() : CallbackBase(NULL, NULL) {}

    // We pass InvokerStorageHolder by const ref to avoid incurring an
    // unnecessary AddRef/Unref pair even though we will modify the object.
    // We cannot use a normal reference because the compiler will warn
    // since this is often used on a return value, which is a temporary.
    //
    // Note that this constructor CANNOT be explicit, and that Bind() CANNOT
    // return the exact Callback<> type.  See base/bind.h for details.
    template<typename T>
    Callback(const internal::InvokerStorageHolder<T>& invoker_holder)
        : CallbackBase(
            reinterpret_cast<InvokeFuncStorage>(&T::Invoker::DoInvoke),
            &invoker_holder.invoker_storage_)
    {
        COMPILE_ASSERT((is_same<PolymorphicInvoke,
                        typename T::Invoker::DoInvokeType>::value),
                       callback_type_does_not_match_bind_result);
    }

    bool Equals(const Callback& other) const
    {
        return CallbackBase::Equals(other);
    }

    R Run(typename internal::ParamTraits<A1>::ForwardType a1,
          typename internal::ParamTraits<A2>::ForwardType a2,
          typename internal::ParamTraits<A3>::ForwardType a3,
          typename internal::ParamTraits<A4>::ForwardType a4) const
    {
        PolymorphicInvoke f =
            reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

        return f(invoker_storage_.get(), a1, a2, a3, a4);
    }
};

template<typename R, typename A1, typename A2, typename A3, typename A4,
         typename A5>
class Callback<R(A1, A2, A3, A4, A5)> : public internal::CallbackBase
{
public:
    typedef R(*PolymorphicInvoke)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<A1>::ForwardType,
        typename internal::ParamTraits<A2>::ForwardType,
        typename internal::ParamTraits<A3>::ForwardType,
        typename internal::ParamTraits<A4>::ForwardType,
        typename internal::ParamTraits<A5>::ForwardType);

    Callback() : CallbackBase(NULL, NULL) {}

    // We pass InvokerStorageHolder by const ref to avoid incurring an
    // unnecessary AddRef/Unref pair even though we will modify the object.
    // We cannot use a normal reference because the compiler will warn
    // since this is often used on a return value, which is a temporary.
    //
    // Note that this constructor CANNOT be explicit, and that Bind() CANNOT
    // return the exact Callback<> type.  See base/bind.h for details.
    template <typename T>
    Callback(const internal::InvokerStorageHolder<T>& invoker_holder)
        : CallbackBase(
            reinterpret_cast<InvokeFuncStorage>(&T::Invoker::DoInvoke),
            &invoker_holder.invoker_storage_)
    {
        COMPILE_ASSERT((is_same<PolymorphicInvoke,
                        typename T::Invoker::DoInvokeType>::value),
                       callback_type_does_not_match_bind_result);
    }

    bool Equals(const Callback& other) const
    {
        return CallbackBase::Equals(other);
    }

    R Run(typename internal::ParamTraits<A1>::ForwardType a1,
          typename internal::ParamTraits<A2>::ForwardType a2,
          typename internal::ParamTraits<A3>::ForwardType a3,
          typename internal::ParamTraits<A4>::ForwardType a4,
          typename internal::ParamTraits<A5>::ForwardType a5) const
    {
        PolymorphicInvoke f =
            reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

        return f(invoker_storage_.get(), a1, a2, a3, a4, a5);
    }
};

template<typename R, typename A1, typename A2, typename A3, typename A4,
         typename A5, typename A6>
class Callback<R(A1, A2, A3, A4, A5, A6)> : public internal::CallbackBase
{
public:
    typedef R(*PolymorphicInvoke)(
        internal::InvokerStorageBase*,
        typename internal::ParamTraits<A1>::ForwardType,
        typename internal::ParamTraits<A2>::ForwardType,
        typename internal::ParamTraits<A3>::ForwardType,
        typename internal::ParamTraits<A4>::ForwardType,
        typename internal::ParamTraits<A5>::ForwardType,
        typename internal::ParamTraits<A6>::ForwardType);

    Callback() : CallbackBase(NULL, NULL) {}

    // We pass InvokerStorageHolder by const ref to avoid incurring an
    // unnecessary AddRef/Unref pair even though we will modify the object.
    // We cannot use a normal reference because the compiler will warn
    // since this is often used on a return value, which is a temporary.
    //
    // Note that this constructor CANNOT be explicit, and that Bind() CANNOT
    // return the exact Callback<> type.  See base/bind.h for details.
    template <typename T>
    Callback(const internal::InvokerStorageHolder<T>& invoker_holder)
        : CallbackBase(
            reinterpret_cast<InvokeFuncStorage>(&T::Invoker::DoInvoke),
            &invoker_holder.invoker_storage_)
    {
        COMPILE_ASSERT((is_same<PolymorphicInvoke,
                        typename T::Invoker::DoInvokeType>::value),
                       callback_type_does_not_match_bind_result);
    }

    bool Equals(const Callback& other) const
    {
        return CallbackBase::Equals(other);
    }

    R Run(typename internal::ParamTraits<A1>::ForwardType a1,
          typename internal::ParamTraits<A2>::ForwardType a2,
          typename internal::ParamTraits<A3>::ForwardType a3,
          typename internal::ParamTraits<A4>::ForwardType a4,
          typename internal::ParamTraits<A5>::ForwardType a5,
          typename internal::ParamTraits<A6>::ForwardType a6) const
    {
        PolymorphicInvoke f =
            reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

        return f(invoker_storage_.get(), a1, a2, a3, a4, a5, a6);
    }
};


// Syntactic sugar to make Callbacks<void(void)> easier to declare since it
// will be used in a lot of APIs with delayed execution.
typedef Callback<void(void)> Closure;

} //namespace base

#endif //__base_callback_h__