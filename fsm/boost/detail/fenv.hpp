/*=============================================================================
    Copyright (c) 2010      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/config.hpp>

#if defined(BOOST_NO_FENV_H)
#error This platform does not have a floating point environment
#endif

#if !defined(BOOST_DETAIL_FENV_HPP)
#define BOOST_DETAIL_FENV_HPP

/* If we're using clang + glibc, we have to get hacky.
 * See http://llvm.org/bugs/show_bug.cgi?id=6907 */
#if defined(__clang__)       &&  (__clang_major__ < 3) &&    \
    defined(__GNU_LIBRARY__) && /* up to version 5 */ \
    defined(__GLIBC__) &&         /* version 6 + */ \
    !defined(_FENV_H)
#define _FENV_H

#include <features.h>
#include <bits/fenv.h>

extern "C" {
    extern int fegetexceptflag (fexcept_t*, int) __THROW;
    extern int fesetexceptflag (__const fexcept_t*, int) __THROW;
    extern int feclearexcept (int) __THROW;
    extern int feraiseexcept (int) __THROW;
    extern int fetestexcept (int) __THROW;
    extern int fegetround (void) __THROW;
    extern int fesetround (int) __THROW;
    extern int fegetenv (fenv_t*) __THROW;
    extern int fesetenv (__const fenv_t*) __THROW;
    extern int feupdateenv (__const fenv_t*) __THROW;
    extern int feholdexcept (fenv_t*) __THROW;

#ifdef __USE_GNU
    extern int feenableexcept (int) __THROW;
    extern int fedisableexcept (int) __THROW;
    extern int fegetexcept (void) __THROW;
#endif
}

namespace std
{
namespace tr1
{
using ::fenv_t;
using ::fexcept_t;
using ::fegetexceptflag;
using ::fesetexceptflag;
using ::feclearexcept;
using ::feraiseexcept;
using ::fetestexcept;
using ::fegetround;
using ::fesetround;
using ::fegetenv;
using ::fesetenv;
using ::feupdateenv;
using ::feholdexcept;
}
}

#else /* if we're not using GNU's C stdlib, fenv.h should work with clang */
#if defined(__SUNPRO_CC) /* lol suncc */
#include <stdio.h>
#endif

#include <fenv.h>

#endif

#endif /* BOOST_DETAIL_FENV_HPP */

