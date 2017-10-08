/*
 * Copyright (c) 1999
 * Silicon Graphics
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#ifndef _STLP_RANGE_ERRORS_H
#define _STLP_RANGE_ERRORS_H

// A few places in the STL throw range errors, using standard exception
// classes defined in <stdexcept>.  This header file provides functions
// to throw those exception objects.

// _STLP_DONT_THROW_RANGE_ERRORS is a hook so that users can disable
// this exception throwing.
#if defined (_STLP_CAN_THROW_RANGE_ERRORS) && defined (_STLP_USE_EXCEPTIONS) && \
   !defined (_STLP_DONT_THROW_RANGE_ERRORS)
#  define _STLP_THROW_RANGE_ERRORS
#endif

// For the STLport iostreams, only declaration here, definition is in the lib
#if !defined (_STLP_USE_NO_IOSTREAMS) && !defined (_STLP_EXTERN_RANGE_ERRORS)
#  define _STLP_EXTERN_RANGE_ERRORS
#endif

_STLP_BEGIN_NAMESPACE
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_runtime_error(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_range_error(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_out_of_range(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_length_error(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_invalid_argument(const char* __msg);
void _STLP_FUNCTION_THROWS _STLP_DECLSPEC _STLP_CALL __stl_throw_overflow_error(const char* __msg);

#if defined (__DMC__) && !defined (_STLP_NO_EXCEPTIONS)
#   pragma noreturn(__stl_throw_runtime_error)
#   pragma noreturn(__stl_throw_range_error)
#   pragma noreturn(__stl_throw_out_of_range)
#   pragma noreturn(__stl_throw_length_error)
#   pragma noreturn(__stl_throw_invalid_argument)
#   pragma noreturn(__stl_throw_overflow_error)
#endif
_STLP_END_NAMESPACE

#if !defined (_STLP_EXTERN_RANGE_ERRORS)
#  include <stl/_range_errors.c>
#endif

#endif /* _STLP_RANGE_ERRORS_H */

// Local Variables:
// mode:C++
// End:
