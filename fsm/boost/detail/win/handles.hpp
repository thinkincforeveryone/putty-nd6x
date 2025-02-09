//  memory.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_HANDLES_HPP
#define BOOST_DETAIL_WIN_HANDLES_HPP

#include <boost/detail/win/basic_types.hpp>


namespace boost
{
namespace detail
{
namespace win32
{
#if defined( BOOST_USE_WINDOWS_H )
using ::CloseHandle;
using ::DuplicateHandle;
#else
extern "C" {
    __declspec(dllimport) int __stdcall
    CloseHandle(void*);
    __declspec(dllimport) int __stdcall
    DuplicateHandle(void*,void*,void*,void**,unsigned long,int,unsigned long);
}

#endif
}
}
}

#endif // BOOST_DETAIL_WIN_HANDLES_HPP
