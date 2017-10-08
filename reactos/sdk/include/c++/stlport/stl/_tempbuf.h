/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
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

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef _STLP_INTERNAL_TEMPBUF_H
#define _STLP_INTERNAL_TEMPBUF_H

#ifndef _STLP_CLIMITS
#  include <climits>
#endif

#ifndef _STLP_INTERNAL_CSTDLIB
#  include <stl/_cstdlib.h>
#endif

#ifndef _STLP_INTERNAL_UNINITIALIZED_H
#  include <stl/_uninitialized.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _Tp>
pair<_Tp*, ptrdiff_t>  _STLP_CALL
__get_temporary_buffer(ptrdiff_t __len, _Tp*);

#ifndef _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS

template <class _Tp>
inline pair<_Tp*, ptrdiff_t>  _STLP_CALL get_temporary_buffer(ptrdiff_t __len) {
  return __get_temporary_buffer(__len, (_Tp*) 0);
}

# if ! defined(_STLP_NO_EXTENSIONS)
// This overload is not required by the standard; it is an extension.
// It is supported for backward compatibility with the HP STL, and
// because not all compilers support the language feature (explicit
// function template arguments) that is required for the standard
// version of get_temporary_buffer.
template <class _Tp>
inline pair<_Tp*, ptrdiff_t>  _STLP_CALL
get_temporary_buffer(ptrdiff_t __len, _Tp*) {
  return __get_temporary_buffer(__len, (_Tp*) 0);
}
# endif
#endif

template <class _Tp>
inline void  _STLP_CALL return_temporary_buffer(_Tp* __p) {
// SunPro brain damage
  free((char*)__p);
}

template <class _ForwardIterator, class _Tp>
class _Temporary_buffer {
private:
  ptrdiff_t  _M_original_len;
  ptrdiff_t  _M_len;
  _Tp*       _M_buffer;

  void _M_allocate_buffer() {
    _M_original_len = _M_len;
    _M_buffer = 0;

    if (_M_len > (ptrdiff_t)(INT_MAX / sizeof(_Tp)))
      _M_len = INT_MAX / sizeof(_Tp);

    while (_M_len > 0) {
      _M_buffer = (_Tp*) malloc(_M_len * sizeof(_Tp));
      if (_M_buffer)
        break;
      _M_len /= 2;
    }
  }

  void _M_initialize_buffer(const _Tp&, const __true_type&) {}
  void _M_initialize_buffer(const _Tp& val, const __false_type&) {
    uninitialized_fill_n(_M_buffer, _M_len, val);
  }

public:
  ptrdiff_t size() const { return _M_len; }
  ptrdiff_t requested_size() const { return _M_original_len; }
  _Tp* begin() { return _M_buffer; }
  _Tp* end() { return _M_buffer + _M_len; }

  _Temporary_buffer(_ForwardIterator __first, _ForwardIterator __last) {
    // Workaround for a __type_traits bug in the pre-7.3 compiler.
#   if defined(__sgi) && !defined(__GNUC__) && _COMPILER_VERSION < 730
    typedef typename __type_traits<_Tp>::is_POD_type _Trivial;
#   else
    typedef typename __type_traits<_Tp>::has_trivial_default_constructor  _Trivial;
#   endif
    _STLP_TRY {
      _M_len = _STLP_STD::distance(__first, __last);
      _M_allocate_buffer();
      if (_M_len > 0)
        _M_initialize_buffer(*__first, _Trivial());
    }
    _STLP_UNWIND(free(_M_buffer); _M_buffer = 0; _M_len = 0)
  }

  ~_Temporary_buffer() {
    _STLP_STD::_Destroy_Range(_M_buffer, _M_buffer + _M_len);
    free(_M_buffer);
  }

private:
  // Disable copy constructor and assignment operator.
  _Temporary_buffer(const _Temporary_buffer<_ForwardIterator, _Tp>&) {}
  void operator=(const _Temporary_buffer<_ForwardIterator, _Tp>&) {}
};

# ifndef _STLP_NO_EXTENSIONS

// Class temporary_buffer is not part of the standard.  It is an extension.

template <class _ForwardIterator,
          class _Tp
#ifdef _STLP_CLASS_PARTIAL_SPECIALIZATION
                    = typename iterator_traits<_ForwardIterator>::value_type
#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */
         >
struct temporary_buffer : public _Temporary_buffer<_ForwardIterator, _Tp>
{
  temporary_buffer(_ForwardIterator __first, _ForwardIterator __last)
    : _Temporary_buffer<_ForwardIterator, _Tp>(__first, __last) {}
  ~temporary_buffer() {}
};

# endif /* _STLP_NO_EXTENSIONS */

_STLP_END_NAMESPACE

# ifndef _STLP_LINK_TIME_INSTANTIATION
#  include <stl/_tempbuf.c>
# endif

#endif /* _STLP_INTERNAL_TEMPBUF_H */

// Local Variables:
// mode:C++
// End:
