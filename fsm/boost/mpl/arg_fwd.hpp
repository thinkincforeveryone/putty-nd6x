
#ifndef BOOST_MPL_ARG_FWD_HPP_INCLUDED
#define BOOST_MPL_ARG_FWD_HPP_INCLUDED

// Copyright Peter Dimov 2001-2002
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: arg_fwd.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <boost/mpl/aux_/adl_barrier.hpp>
#include <boost/mpl/aux_/nttp_decl.hpp>

BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE_OPEN

template< BOOST_MPL_AUX_NTTP_DECL(int, N) > struct arg;

BOOST_MPL_AUX_ADL_BARRIER_NAMESPACE_CLOSE
BOOST_MPL_AUX_ADL_BARRIER_DECL(arg)

#endif // BOOST_MPL_ARG_FWD_HPP_INCLUDED
