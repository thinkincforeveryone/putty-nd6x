
#ifndef __base_bind_h__
#define __base_bind_h__

#pragma once

#include "bind_internal.h"
#include "callback_internal.h"

// See base/callback.h for how to use these functions.
//
// IMPLEMENTATION NOTE
// Though Bind()'s result is meant to be stored in a Callback<> type, it
// cannot actually return the exact type without requiring a large amount
// of extra template specializations. The problem is that in order to
// discern the correct specialization of Callback<>, Bind would need to
// unwrap the function signature to determine the signature's arity, and
// whether or not it is a method.
//
// Each unique combination of (arity, function_type, num_prebound) where
// function_type is one of {function, method, const_method} would require
// one specialization.  We eventually have to do a similar number of
// specializations anyways in the implementation (see the FunctionTraitsN,
// classes).  However, it is avoidable in Bind if we return the result
// via an indirection like we do below.

namespace base
{

template<typename Sig>
internal::InvokerStorageHolder<internal::InvokerStorage0<Sig> >
Bind(Sig f)
{
    return internal::MakeInvokerStorageHolder(
               new internal::InvokerStorage0<Sig>(f));
}

template<typename Sig, typename P1>
internal::InvokerStorageHolder<internal::InvokerStorage1<Sig,P1> >
Bind(Sig f, const P1& p1)
{
    return internal::MakeInvokerStorageHolder(
               new internal::InvokerStorage1<Sig, P1>(
                   f, p1));
}

template<typename Sig, typename P1, typename P2>
internal::InvokerStorageHolder<internal::InvokerStorage2<Sig,P1, P2> >
Bind(Sig f, const P1& p1, const P2& p2)
{
    return internal::MakeInvokerStorageHolder(
               new internal::InvokerStorage2<Sig, P1, P2>(
                   f, p1, p2));
}

template<typename Sig, typename P1, typename P2, typename P3>
internal::InvokerStorageHolder<internal::InvokerStorage3<Sig,P1, P2, P3> >
Bind(Sig f, const P1& p1, const P2& p2, const P3& p3)
{
    return internal::MakeInvokerStorageHolder(
               new internal::InvokerStorage3<Sig, P1, P2, P3>(
                   f, p1, p2, p3));
}

template<typename Sig, typename P1, typename P2, typename P3, typename P4>
internal::InvokerStorageHolder<internal::InvokerStorage4<Sig,P1, P2, P3, P4> >
Bind(Sig f, const P1& p1, const P2& p2, const P3& p3, const P4& p4)
{
    return internal::MakeInvokerStorageHolder(
               new internal::InvokerStorage4<Sig, P1, P2, P3, P4>(
                   f, p1, p2, p3, p4));
}

template<typename Sig, typename P1, typename P2, typename P3, typename P4,
         typename P5>
internal::InvokerStorageHolder<internal::InvokerStorage5<Sig,P1, P2, P3, P4,
         P5> >
         Bind(Sig f, const P1& p1, const P2& p2, const P3& p3, const P4& p4,
              const P5& p5)
{
    return internal::MakeInvokerStorageHolder(
               new internal::InvokerStorage5<Sig, P1, P2, P3, P4, P5>(
                   f, p1, p2, p3, p4, p5));
}

template<typename Sig, typename P1, typename P2, typename P3, typename P4,
         typename P5, typename P6>
internal::InvokerStorageHolder<internal::InvokerStorage6<Sig,P1, P2, P3, P4,
         P5, P6> >
         Bind(Sig f, const P1& p1, const P2& p2, const P3& p3, const P4& p4,
              const P5& p5, const P6& p6)
{
    return internal::MakeInvokerStorageHolder(
               new internal::InvokerStorage6<Sig, P1, P2, P3, P4, P5, P6>(
                   f, p1, p2, p3, p4, p5, p6));
}

} //namespace base

#endif //__base_bind_h__