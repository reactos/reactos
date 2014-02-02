/*
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#define _STLP_COMPILER "CC"

// Mostly correct guess, change it for Alpha (and other environments
// that has 64-bit "long")
#  define _STLP_UINT32_T unsigned long

// Uncomment if long long is available
#  define _STLP_LONG_LONG long long

// Uncomment this if your compiler can't inline while(), for()
#  define _STLP_LOOP_INLINE_PROBLEMS 1

// Uncomment this if your compiler does not support exceptions
// Cray C++ supports exceptions when '-h exceptions' option is user;
// therefore '-D_STLP_HAS_NO_EXCEPTIONS' must be used when '-h exceptions'
// is NOT used.
//#  define _STLP_HAS_NO_EXCEPTIONS 1

// Delete?
// Define this if compiler lacks <exception> header
//#  define _STLP_NO_EXCEPTION_HEADER 1

// Uncomment this if your C library has lrand48() function
#  define _STLP_RAND48 1

// Uncomment if native new-style C library headers lile <cstddef>, etc are not available.
#   define _STLP_HAS_NO_NEW_C_HEADERS 1

// uncomment if new-style headers <new> is available
#   define _STLP_NO_NEW_NEW_HEADER 1

// uncomment this if <iostream> and other STD headers put their stuff in ::namespace,
// not std::
#  define _STLP_VENDOR_GLOBAL_STD

// uncomment this if <cstdio> and the like put stuff in ::namespace,
// not std::
#  define _STLP_VENDOR_GLOBAL_CSTD

# define _STLP_NATIVE_C_HEADER(__x) </usr/include/##__x>
// WARNING: Following is hardcoded to the system default C++ include files
# define _STLP_NATIVE_CPP_RUNTIME_HEADER(__x) </opt/ctl/CC/CC/include/##__x>


# define _STLP_NO_NATIVE_MBSTATE_T
# define _STLP_NO_USING_FOR_GLOBAL_FUNCTIONS
//# define _STLP_VENDOR_GLOBAL_EXCEPT_STD
