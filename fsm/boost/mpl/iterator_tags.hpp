
#ifndef BOOST_MPL_ITERATOR_TAG_HPP_INCLUDED
#define BOOST_MPL_ITERATOR_TAG_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: iterator_tags.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <boost/mpl/int.hpp>

namespace boost
{
namespace mpl
{

struct forward_iterator_tag       : int_<0>
{
    typedef forward_iterator_tag type;
};
struct bidirectional_iterator_tag : int_<1>
{
    typedef bidirectional_iterator_tag type;
};
struct random_access_iterator_tag : int_<2>
{
    typedef random_access_iterator_tag type;
};

}
}

#endif // BOOST_MPL_ITERATOR_TAG_HPP_INCLUDED
