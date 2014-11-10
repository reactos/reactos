/*
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

#ifndef _STLP_INTERNAL_NEW
#define _STLP_INTERNAL_NEW

#ifndef _STLP_INTERNAL_CSTDDEF
// size_t
#  include <stl/_cstddef.h>
#endif

#if defined (__BORLANDC__) && (__BORLANDC__ < 0x570)
// new.h uses ::malloc ;(
#  include _STLP_NATIVE_CPP_C_HEADER(cstdlib)
using _STLP_VENDOR_CSTD::malloc;
#endif

#if !defined (_STLP_NO_NEW_NEW_HEADER)
// eMbedded Visual C++ .NET unfortunately uses _INC_NEW for both <new.h> and <new>
// we undefine the symbol to get the stuff in the SDK's <new>
#  if defined (_STLP_WCE_NET) && defined (_INC_NEW)
#    undef _INC_NEW
#  endif

#  if defined (new)
/* STLport cannot replace native Std library new header if new is a macro,
 * please define new macro after <new> header inclusion.
 */
#    error Cannot include native new header as new is a macro.
#  endif

#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <new>
#  else
#    include _STLP_NATIVE_CPP_RUNTIME_HEADER(new)
#  endif
#else
#  include <new.h>
#endif

#if defined (_STLP_NO_BAD_ALLOC) && !defined (_STLP_NEW_DONT_THROW_BAD_ALLOC)
#  define _STLP_NEW_DONT_THROW_BAD_ALLOC 1
#endif

#if defined (_STLP_USE_EXCEPTIONS) && defined (_STLP_NEW_DONT_THROW_BAD_ALLOC)

#  ifndef _STLP_INTERNAL_EXCEPTION
#    include <stl/_exception.h>
#  endif

_STLP_BEGIN_NAMESPACE

#  if defined (_STLP_NO_BAD_ALLOC)
struct nothrow_t {};
#    define nothrow nothrow_t()
#  endif

/*
 * STLport own bad_alloc exception to be used if the native C++ library
 * do not define it or when the new operator do not throw it to avoid
 * a useless library dependency.
 */
class bad_alloc : public exception {
public:
  bad_alloc () _STLP_NOTHROW_INHERENTLY { }
  bad_alloc(const bad_alloc&) _STLP_NOTHROW_INHERENTLY { }
  bad_alloc& operator=(const bad_alloc&) _STLP_NOTHROW_INHERENTLY {return *this;}
  ~bad_alloc () _STLP_NOTHROW_INHERENTLY { }
  const char* what() const _STLP_NOTHROW_INHERENTLY { return "bad alloc"; }
};

_STLP_END_NAMESPACE

#endif /* _STLP_USE_EXCEPTIONS && (_STLP_NO_BAD_ALLOC || _STLP_NEW_DONT_THROW_BAD_ALLOC) */

#if defined (_STLP_USE_OWN_NAMESPACE)

_STLP_BEGIN_NAMESPACE

#  if !defined (_STLP_NEW_DONT_THROW_BAD_ALLOC)
using _STLP_VENDOR_EXCEPT_STD::bad_alloc;
#  endif

#  if !defined (_STLP_NO_BAD_ALLOC)
using _STLP_VENDOR_EXCEPT_STD::nothrow_t;
using _STLP_VENDOR_EXCEPT_STD::nothrow;
#    if defined (_STLP_GLOBAL_NEW_HANDLER)
using ::new_handler;
using ::set_new_handler;
#    else
using _STLP_VENDOR_EXCEPT_STD::new_handler;
using _STLP_VENDOR_EXCEPT_STD::set_new_handler;
#    endif
#  endif /* !_STLP_NO_BAD_ALLOC */

_STLP_END_NAMESPACE
#endif /* _STLP_USE_OWN_NAMESPACE */

#ifndef _STLP_THROW_BAD_ALLOC
#  if !defined (_STLP_USE_EXCEPTIONS)
#    ifndef _STLP_INTERNAL_CSTDIO
#      include <stl/_cstdio.h>
#    endif
#    define _STLP_THROW_BAD_ALLOC puts("out of memory\n"); exit(1)
#  else
#    define _STLP_THROW_BAD_ALLOC _STLP_THROW(_STLP_STD::bad_alloc())
#  endif
#endif

#if defined (_STLP_NO_NEW_NEW_HEADER) || defined (_STLP_NEW_DONT_THROW_BAD_ALLOC)
#  define _STLP_CHECK_NULL_ALLOC(__x) void* __y = __x; if (__y == 0) { _STLP_THROW_BAD_ALLOC; } return __y
#else
#  define _STLP_CHECK_NULL_ALLOC(__x) return __x
#endif

_STLP_BEGIN_NAMESPACE

#if ((defined (__IBMCPP__) || defined (__OS400__) || defined (__xlC__) || defined (qTidyHeap)) && defined (_STLP_DEBUG_ALLOC))
inline void* _STLP_CALL __stl_new(size_t __n)   { _STLP_CHECK_NULL_ALLOC(::operator new(__n, __FILE__, __LINE__)); }
inline void  _STLP_CALL __stl_delete(void* __p) { ::operator delete(__p, __FILE__, __LINE__); }
#else
inline void* _STLP_CALL __stl_new(size_t __n)   { _STLP_CHECK_NULL_ALLOC(::operator new(__n)); }
inline void  _STLP_CALL __stl_delete(void* __p) { ::operator delete(__p); }
#endif
_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_NEW */

/*
 * Local Variables:
 * mode:C++
 * End:
 */
