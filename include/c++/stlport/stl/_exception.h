/*
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
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
 */

// The header <exception> contains low-level functions that interact
// with a compiler's exception-handling mechanism.  It is assumed to
// be supplied with the compiler, rather than with the library, because
// it is inherently tied very closely to the compiler itself.

// On platforms where <exception> does not exist, this header defines
// an exception base class.  This is *not* a substitute for everything
// in <exception>, but it suffices to support a bare minimum of STL
// functionality.

#ifndef _STLP_INTERNAL_EXCEPTION
#define _STLP_INTERNAL_EXCEPTION

#if !defined (_STLP_NO_EXCEPTION_HEADER)

#  if defined ( _UNCAUGHT_EXCEPTION )
#    undef _STLP_NO_UNCAUGHT_EXCEPT_SUPPORT
#  endif

#  if defined (_STLP_BROKEN_EXCEPTION_CLASS)
#    define exception     _STLP_NULLIFIED_BROKEN_EXCEPTION_CLASS
#    define bad_exception _STLP_NULLIFIED_BROKEN_BAD_EXCEPTION_CLASS
#    if defined (_STLP_NO_NEW_NEW_HEADER)
#      include _STLP_NATIVE_CPP_RUNTIME_HEADER(Exception.h)
#    else
#      include _STLP_NATIVE_CPP_RUNTIME_HEADER(Exception)
#    endif
#    undef exception
#    undef bad_exception
#  else
#    if defined (_STLP_NO_NEW_NEW_HEADER)
#      if defined (_STLP_HAS_INCLUDE_NEXT)
#        include_next <exception.h>
#      else
#        include _STLP_NATIVE_CPP_RUNTIME_HEADER(exception.h)
#      endif
#    else
#      if defined (_STLP_HAS_INCLUDE_NEXT)
#        include_next <exception>
#      else
#        include _STLP_NATIVE_CPP_RUNTIME_HEADER(exception)
#      endif
#    endif
#  endif

#  if defined (_STLP_HAS_SPECIFIC_PROLOG_EPILOG) && defined (_STLP_MSVC_LIB) && (_STLP_MSVC_LIB < 1300)
// dwa 02/04/00
// The header <yvals.h> which ships with vc6 and is included by its native <exception>
// actually turns on warnings, so we have to turn them back off.
#    include <stl/config/_warnings_off.h>
#  endif

#  if defined (_STLP_USE_OWN_NAMESPACE)

_STLP_BEGIN_NAMESPACE
#    if !defined (_STLP_BROKEN_EXCEPTION_CLASS)
#      if !defined (_STLP_USING_PLATFORM_SDK_COMPILER) || !defined (_WIN64)
using _STLP_VENDOR_EXCEPT_STD::exception;
#      else
using ::exception;
#      endif
using _STLP_VENDOR_EXCEPT_STD::bad_exception;
#    endif

#    if !defined (_STLP_NO_USING_FOR_GLOBAL_FUNCTIONS)
// fbp : many platforms present strange mix of
// those in various namespaces
#      if !defined (_STLP_VENDOR_UNEXPECTED_STD)
#        define _STLP_VENDOR_UNEXPECTED_STD _STLP_VENDOR_EXCEPT_STD
#      else
/* The following definitions are for backward compatibility as _STLP_VENDOR_TERMINATE_STD
 * and _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD has been introduce after _STLP_VENDOR_UNEXPECTED_STD
 * and _STLP_VENDOR_UNEXPECTED_STD was the macro used in their place before that introduction.
 */
#        if !defined (_STLP_VENDOR_TERMINATE_STD)
#          define _STLP_VENDOR_TERMINATE_STD _STLP_VENDOR_UNEXPECTED_STD
#        endif
#        if !defined (_STLP_VENDOR_UNCAUGHT_EXCEPTION_STD)
#          define _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD _STLP_VENDOR_UNEXPECTED_STD
#        endif
#      endif
#      if !defined (_STLP_VENDOR_TERMINATE_STD)
#        define _STLP_VENDOR_TERMINATE_STD _STLP_VENDOR_EXCEPT_STD
#      endif
#      if !defined (_STLP_VENDOR_UNCAUGHT_EXCEPTION_STD)
#        define _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD _STLP_VENDOR_EXCEPT_STD
#      endif
#      if !defined (_STLP_VENDOR_TERMINATE_STD)
#        define _STLP_VENDOR_TERMINATE_STD _STLP_VENDOR_EXCEPT_STD
#      endif
#      if !defined (_STLP_VENDOR_UNCAUGHT_EXCEPTION_STD)
#        define _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD _STLP_VENDOR_EXCEPT_STD
#      endif
// weird errors
#        if !defined (_STLP_NO_UNEXPECTED_EXCEPT_SUPPORT)
#          if defined (__ICL) && (__ICL >= 900) && (_STLP_MSVC_LIB < 1300)
//See config/_intel.h for reason about this workaround
using std::unexpected;
#          else
using _STLP_VENDOR_UNEXPECTED_STD::unexpected;
#          endif
using _STLP_VENDOR_UNEXPECTED_STD::unexpected_handler;
using _STLP_VENDOR_UNEXPECTED_STD::set_unexpected;
#        endif
using _STLP_VENDOR_TERMINATE_STD::terminate;
using _STLP_VENDOR_TERMINATE_STD::terminate_handler;
using _STLP_VENDOR_TERMINATE_STD::set_terminate;

#      if !defined (_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)
using _STLP_VENDOR_UNCAUGHT_EXCEPTION_STD::uncaught_exception;
#      endif
#    endif /* !_STLP_NO_USING_FOR_GLOBAL_FUNCTIONS */
_STLP_END_NAMESPACE
#  endif /* _STLP_OWN_NAMESPACE */
#else /* _STLP_NO_EXCEPTION_HEADER */

/* fbp : absence of <exception> usually means that those
 * functions are not going to be called by compiler.
 * Still, define them for the user.
 * dums: Policy modification, if the function do not behave like the Standard
 *       defined it we do not grant it in the STLport namespace. We will have
 *       compile time error rather than runtime error.
 */
#if 0
/*
typedef void (*unexpected_handler)();
unexpected_handler set_unexpected(unexpected_handler f) _STLP_NOTHROW_INHERENTLY;
void unexpected();

typedef void (*terminate_handler)();
terminate_handler set_terminate(terminate_handler f) _STLP_NOTHROW_INHERENTLY;
void terminate();

bool uncaught_exception(); // not implemented under mpw as of Jan/1999
*/
#endif

#endif /* _STLP_NO_EXCEPTION_HEADER */

#if defined (_STLP_NO_EXCEPTION_HEADER) || defined (_STLP_BROKEN_EXCEPTION_CLASS)
_STLP_BEGIN_NAMESPACE

// section 18.6.1
class _STLP_CLASS_DECLSPEC exception {
public:
#  ifndef _STLP_USE_NO_IOSTREAMS
  exception() _STLP_NOTHROW;
  virtual ~exception() _STLP_NOTHROW;
  virtual const char* what() const _STLP_NOTHROW;
#  else
  exception() _STLP_NOTHROW {}
  virtual ~exception() _STLP_NOTHROW {}
  virtual const char* what() const _STLP_NOTHROW {return "class exception";}
#  endif
};

// section 18.6.2.1
class _STLP_CLASS_DECLSPEC bad_exception : public exception {
public:
#  ifndef _STLP_USE_NO_IOSTREAMS
  bad_exception() _STLP_NOTHROW;
  ~bad_exception() _STLP_NOTHROW;
  const char* what() const _STLP_NOTHROW;
#  else
  bad_exception() _STLP_NOTHROW {}
  ~bad_exception() _STLP_NOTHROW {}
  const char* what() const _STLP_NOTHROW {return "class bad_exception";}
#  endif
};

// forward declaration
class __Named_exception;
_STLP_END_NAMESPACE
#endif

#endif /* _STLP_INTERNAL_EXCEPTION */
