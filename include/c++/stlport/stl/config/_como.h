
// STLport configuration file
// It is internal STLport header - DO NOT include it directly

#define _STLP_COMPILER "Comeau"

#include <stl/config/_native_headers.h>

#define _STLP_UINT32_T unsigned int

#define _STLP_HAS_NO_NEW_C_HEADERS
// #define _STLP_VENDOR_GLOBAL_EXCEPT_STD
#define _STLP_LONG_LONG long long


//
// ADDITIONS FOR COMEAU C++, made by Comeau Computing.
// We can be reached through comeau@comeaucomputing.com
// You shouldn't need to change anything below here for Comeau C++.
// If you do, please tell us at comeau@comeaucomputing.com
//
// Changes made here, AND THROUGH ALL FILES, based upon the __COMO__ macro
// (and SIMILAR NAMES INVOLVING COMO).... no doubt some of this will
// change as SGI integrates the changes into their code base since
// some changes are not really Comeau C++ specific, but required to
// make the SGI code compliant with Standard C++).
//
// Testing was done with Comeau C++ 4.2.44 and 4.2.45.2.  Changes were made for
// both Comeau relaxed mode and Comeau strict mode, especially for end user code
// (that is, some of the .cxx files cannot compile in strict mode, because they
// contain extensions to Standard C++, however their object code forms can
// be used once compiled in relaxed mode, even if the end user code uses
// strict mode).
//
// These changes may also work for some earlier versions of Comeau C++,
// though we have not tested them.
//
// Actual mods made under RedHat 6.1 LINUX, should be ok with SuSE too and
// other LINUX's, and older Caldera LINUX, Solaris/SPARC, SunOS, SCO UNIX,
// and NetBSD. Other platforms may be added.  Comeau will also perform
// custom ports for you.
//
// Check libcomo details at http://www.comeaucomputing.com/libcomo and
// http://www.comeaucomputing.com
//
// History of Comeau changes (this is rough, as work was often going on in parallel):
// BETA1 July 14, 2000, Initial port for RedHat 6.1 INTEL/ELF
// BETA2 Aug   4, 2000, Stronger RedHat support
//                      Support for Comeau strict mode for end user code
// BETA3 Aug  22, 2000, Support for other LINUX/INTEL/ELF's, including older ones
// BETA4 Sept  2, 2000, Initial support for SCO UNIX + other UNIX x86 SVR3's
//                      Stronger support for end user Comeau strict mode
// BETA5 Oct   5, 2000, Initial support for Solaris/SPARC
//                      More SCO support (though still incomplete)
// BETA6 Feb   5, 2001, Minor mods to accomodate Comeau C++ 4.2.45.1
// BETA7 Mar  13, 2001, Verified with Comeau C++ 4.2.45.2
//                      Minor NetBSD support
// BETA8 Apr   1. 2001, Initial support for SunOS/SPARC
// BETA9 Apr   7, 2001, Stronger SCO support + other UNIX x86 SVR3's
//                      Mods for an fpos_t problem for some LINUXes
//                      Mods since Destroy did not work in strict mode
// BETA10 Apr  12. 2001, Stronger NetBSD support
//
// PLANNED:
// BETAx TBA  TBA, 2001, NetBSD, UNIXWARE, and Windows support expected
//


#ifdef __linux__

#   define _STLP_NO_NATIVE_MBSTATE_T      1
#   define _STLP_NO_NATIVE_WIDE_FUNCTIONS 1
#   define _STLP_NO_NATIVE_WIDE_STREAMS   1
#   define _STLP_NO_LONG_DOUBLE   1

// Comeau C++ under LINUX/INTEL/ELF
// Preprocess away "long long" routines for now, even in relaxed mode
# define __wcstoull_internal_defined  1
# define __wcstoll_internal_defined  1

#endif /* __COMO__ under __linux__ */

#ifdef __USING_x86SVR3x_WITH_COMO /* SCO et al */
/* UNIX 386+ SVR3 mods made with __USING_x86SVR3x_WITH_COMO
   in other sources, not here */
#    define atan2l atan2
#    define cosl cos
#    define sinl sin
#    define sqrtl sqrt
#    include <math.h>
     inline long double expl(long double arg) { return exp(arg); }
     inline long double logl(long double arg) { return log(arg); }
#    define log10l log10

#    define sinhl sinh
#    define coshl cosh
#    define fabsl fabs
namespace std {
 inline int min(int a, int b) { return a>b ? b : a; }
}
#endif

#ifdef sun
// Comeau C++ under Solaris/SPARC or SunOS

#ifdef solarissparc
#define __USING_SOLARIS_SPARC_WITH_COMO /* show this in the source when grep'ing for COMO */
// Note comowchar.h for Solaris/SPARC wchar stuff

#include <math.h>
#    define sinf sin
#    define sinl sin
#    define sinhf sinh
#    define sinhl sinh
#    define cosf cos
#    define cosl cos
#    define coshf cosh
#    define coshl cosh
#    define atan2l atan2
#    define atan2f atan2
     inline float logf(float arg) { return log(arg); }
     inline long double logl(long double arg) { return log(arg); }
#    define log10f log10
#    define log10l log10
#    define expf exp
     inline long double expl(long double arg) { return exp(arg); }
#    define sqrtf sqrt
#    define sqrtl sqrt
#    define fabsf fabs
#    define fabsl fabs
#else
#define __USING_SUNOS_WITH_COMO

#define __unix 1
#define __EXTENSIONS__ /* This might create undue noise somewhere */
#endif
#endif /* sun */

#if defined(__NetBSD__)
// From non-como #ifdef __GNUC__ above
#undef _STLP_NO_FUNCTION_PTR_IN_CLASS_TEMPLATE
#define __unix 1

#include <sys/cdefs.h>
// Some joker #define'd __END_DECLS as };
#undef __END_DECLS
#define __END_DECLS }

// <sys/stat.h> prob
#include <sys/cdefs.h>
#undef __RENAME
#define __RENAME(x)

#define wchar_t __COMO_WCHAR_T
#include <stddef.h>
#undef wchar_t

#include <math.h>
# ifdef BORIS_DISABLED
#    define atan2l atan2
#    define cosl cos
#    define sinl sin
#    define sqrtl sqrt
     inline long double expl(long double arg) { return exp(arg); }
     inline long double logl(long double arg) { return log(arg); }
#    define log10l log10
#    define sinhl sinh
#    define coshl cosh
#    define fabsl fabs
# endif
#endif /* __NetBSD__ under __COMO__ */

// Shouldn't need to change anything below here for Comeau C++
// If so, tell us at comeau@comeaucomputing.com

#define _STLP_NO_DRAND48

#define _STLP_PARTIAL_SPECIALIZATION_SYNTAX
#define _STLP_NO_USING_CLAUSE_IN_CLASS

#if __COMO_VERSION__ < 4300
#if __COMO_VERSION__ >= 4245
#define _STLP_NO_EXCEPTION_HEADER /**/
    // Is this needed?
#   include <stdexcept.stdh>
#endif
#define _STLP_NO_BAD_ALLOC /**/
#define _STLP_USE_AUTO_PTR_CONVERSIONS /**/
#endif

// this one is true only with MS
# if defined (_MSC_VER)
#  define _STLP_WCHAR_T_IS_USHORT 1
#  if _MSC_VER <= 1200
#   define _STLP_VENDOR_GLOBAL_CSTD
#  endif
#  if _MSC_VER < 1100
#   define _STLP_NO_BAD_ALLOC 1
#   define _STLP_NO_EXCEPTION_HEADER 1
#   define _STLP_NO_NEW_NEW_HEADER 1
#   define _STLP_USE_NO_IOSTREAMS 1
#  endif
# endif

// # define __EDG_SWITCHES


