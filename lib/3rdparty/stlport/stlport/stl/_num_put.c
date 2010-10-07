/*
 * Copyright (c) 1999
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
 *
 */
#ifndef _STLP_NUM_PUT_C
#define _STLP_NUM_PUT_C

#ifndef _STLP_INTERNAL_NUM_PUT_H
#  include <stl/_num_put.h>
#endif

#ifndef _STLP_INTERNAL_LIMITS
#  include <stl/_limits.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

// __do_put_float and its helper functions.  Strategy: write the output
// to a buffer of char, transform the buffer to _CharT, and then copy
// it to the output.

//----------------------------------------------------------------------
// num_put facet

template <class _CharT, class _OutputIter>
_OutputIter  _STLP_CALL
__copy_float_and_fill(const _CharT* __first, const _CharT* __last,
                      _OutputIter __oi,
                      ios_base::fmtflags __flags,
                      streamsize __width, _CharT __fill,
                      _CharT __xplus, _CharT __xminus) {
  if (__width <= __last - __first)
    return _STLP_STD::copy(__first, __last, __oi);
  else {
    streamsize __pad = __width - (__last - __first);
    ios_base::fmtflags __dir = __flags & ios_base::adjustfield;

    if (__dir == ios_base::left) {
      __oi = _STLP_STD::copy(__first, __last, __oi);
      return _STLP_PRIV __fill_n(__oi, __pad, __fill);
    }
    else if (__dir == ios_base::internal && __first != __last &&
             (*__first == __xplus || *__first == __xminus)) {
      *__oi++ = *__first++;
      __oi = _STLP_PRIV __fill_n(__oi, __pad, __fill);
      return _STLP_STD::copy(__first, __last, __oi);
    }
    else {
      __oi = _STLP_PRIV __fill_n(__oi, __pad, __fill);
      return _STLP_STD::copy(__first, __last, __oi);
    }
  }
}

#if !defined (_STLP_NO_WCHAR_T)
// Helper routine for wchar_t
template <class _OutputIter>
_OutputIter  _STLP_CALL
__put_float(__iostring &__str, _OutputIter __oi,
            ios_base& __f, wchar_t __fill,
            wchar_t __decimal_point, wchar_t __sep,
            size_t __group_pos, const string& __grouping) {
  const ctype<wchar_t>& __ct = use_facet<ctype<wchar_t> >(__f.getloc());

  __iowstring __wbuf;
  __convert_float_buffer(__str, __wbuf, __ct, __decimal_point);

  if (!__grouping.empty()) {
    __insert_grouping(__wbuf, __group_pos, __grouping,
                      __sep, __ct.widen('+'), __ct.widen('-'), 0);
  }

  return __copy_float_and_fill(__wbuf.data(), __wbuf.data() + __wbuf.size(), __oi,
                               __f.flags(), __f.width(0), __fill, __ct.widen('+'), __ct.widen('-'));
}
#endif /* WCHAR_T */

// Helper routine for char
template <class _OutputIter>
_OutputIter  _STLP_CALL
__put_float(__iostring &__str, _OutputIter __oi,
            ios_base& __f, char __fill,
            char __decimal_point, char __sep,
            size_t __group_pos, const string& __grouping) {
  if ((__group_pos < __str.size()) && (__str[__group_pos] == '.')) {
    __str[__group_pos] = __decimal_point;
  }

  if (!__grouping.empty()) {
    __insert_grouping(__str, __group_pos,
                      __grouping, __sep, '+', '-', 0);
  }

  return __copy_float_and_fill(__str.data(), __str.data() + __str.size(), __oi,
                               __f.flags(), __f.width(0), __fill, '+', '-');
}

template <class _CharT, class _OutputIter, class _Float>
_OutputIter _STLP_CALL
__do_put_float(_OutputIter __s, ios_base& __f,
                _CharT __fill, _Float __x) {
  __iostring __buf;

  size_t __group_pos = __write_float(__buf, __f.flags(), (int)__f.precision(), __x);

  const numpunct<_CharT>& __np = use_facet<numpunct<_CharT> >(__f.getloc());
  return __put_float(__buf, __s, __f, __fill,
                     __np.decimal_point(), __np.thousands_sep(),
                     __group_pos, __np.grouping());
}

inline void __get_money_digits_aux (__iostring &__buf, ios_base &, _STLP_LONGEST_FLOAT_TYPE __x)
{ __get_floor_digits(__buf, __x); }

#if !defined (_STLP_NO_WCHAR_T)
inline void __get_money_digits_aux (__iowstring &__wbuf, ios_base &__f, _STLP_LONGEST_FLOAT_TYPE __x) {
  __iostring __buf;
  __get_floor_digits(__buf, __x);

  const ctype<wchar_t>& __ct = use_facet<ctype<wchar_t> >(__f.getloc());
  __convert_float_buffer(__buf, __wbuf, __ct, wchar_t(0), false);
}
#endif

template <class _CharT>
void _STLP_CALL __get_money_digits(_STLP_BASIC_IOSTRING(_CharT) &__buf, ios_base& __f, _STLP_LONGEST_FLOAT_TYPE __x)
{ __get_money_digits_aux(__buf, __f, __x); }

// _M_do_put_integer and its helper functions.

template <class _CharT, class _OutputIter>
_OutputIter _STLP_CALL
__copy_integer_and_fill(const _CharT* __buf, ptrdiff_t __len,
                        _OutputIter __oi,
                        ios_base::fmtflags __flg, streamsize __wid, _CharT __fill,
                        _CharT __xplus, _CharT __xminus) {
  if (__len >= __wid)
    return _STLP_STD::copy(__buf, __buf + __len, __oi);
  else {
    //casting numeric_limits<ptrdiff_t>::max to streamsize only works is ptrdiff_t is signed or streamsize representation
    //is larger than ptrdiff_t one.
    _STLP_STATIC_ASSERT((sizeof(streamsize) > sizeof(ptrdiff_t)) ||
                        ((sizeof(streamsize) == sizeof(ptrdiff_t)) && numeric_limits<ptrdiff_t>::is_signed))
    ptrdiff_t __pad = __STATIC_CAST(ptrdiff_t, (min) (__STATIC_CAST(streamsize, (numeric_limits<ptrdiff_t>::max)()),
                                                      __STATIC_CAST(streamsize, __wid - __len)));
    ios_base::fmtflags __dir = __flg & ios_base::adjustfield;

    if (__dir == ios_base::left) {
      __oi = _STLP_STD::copy(__buf, __buf + __len, __oi);
      return _STLP_PRIV __fill_n(__oi, __pad, __fill);
    }
    else if (__dir == ios_base::internal && __len != 0 &&
             (__buf[0] == __xplus || __buf[0] == __xminus)) {
      *__oi++ = __buf[0];
      __oi = __fill_n(__oi, __pad, __fill);
      return _STLP_STD::copy(__buf + 1, __buf + __len, __oi);
    }
    else if (__dir == ios_base::internal && __len >= 2 &&
             (__flg & ios_base::showbase) &&
             (__flg & ios_base::basefield) == ios_base::hex) {
      *__oi++ = __buf[0];
      *__oi++ = __buf[1];
      __oi = __fill_n(__oi, __pad, __fill);
      return _STLP_STD::copy(__buf + 2, __buf + __len, __oi);
    }
    else {
      __oi = __fill_n(__oi, __pad, __fill);
      return _STLP_STD::copy(__buf, __buf + __len, __oi);
    }
  }
}

#if !defined (_STLP_NO_WCHAR_T)
// Helper function for wchar_t
template <class _OutputIter>
_OutputIter _STLP_CALL
__put_integer(char* __buf, char* __iend, _OutputIter __s,
              ios_base& __f,
              ios_base::fmtflags __flags, wchar_t __fill) {
  locale __loc = __f.getloc();
  const ctype<wchar_t>& __ct = use_facet<ctype<wchar_t> >(__loc);

  wchar_t __xplus  = __ct.widen('+');
  wchar_t __xminus = __ct.widen('-');

  wchar_t __wbuf[64];
  __ct.widen(__buf, __iend, __wbuf);
  ptrdiff_t __len = __iend - __buf;
  wchar_t* __eend = __wbuf + __len;

  const numpunct<wchar_t>& __np = use_facet<numpunct<wchar_t> >(__loc);
  const string& __grouping = __np.grouping();

  if (!__grouping.empty()) {
    int __basechars;
    if (__flags & ios_base::showbase)
      switch (__flags & ios_base::basefield) {
        case ios_base::hex: __basechars = 2; break;
        case ios_base::oct: __basechars = 1; break;
        default: __basechars = 0;
      }
    else
      __basechars = 0;

    __len = __insert_grouping(__wbuf, __eend, __grouping, __np.thousands_sep(),
                              __xplus, __xminus, __basechars);
  }

  return __copy_integer_and_fill((wchar_t*)__wbuf, __len, __s,
                                 __flags, __f.width(0), __fill, __xplus, __xminus);
}
#endif

// Helper function for char
template <class _OutputIter>
_OutputIter _STLP_CALL
__put_integer(char* __buf, char* __iend, _OutputIter __s,
              ios_base& __f, ios_base::fmtflags __flags, char __fill) {
  char __grpbuf[64];
  ptrdiff_t __len = __iend - __buf;

  const numpunct<char>& __np = use_facet<numpunct<char> >(__f.getloc());
  const string& __grouping = __np.grouping();

  if (!__grouping.empty()) {
    int __basechars;
    if (__flags & ios_base::showbase)
      switch (__flags & ios_base::basefield) {
        case ios_base::hex: __basechars = 2; break;
        case ios_base::oct: __basechars = 1; break;
        default: __basechars = 0;
      }
    else
      __basechars = 0;

     // make sure there is room at the end of the buffer
     // we pass to __insert_grouping
    _STLP_STD::copy(__buf, __iend, (char *) __grpbuf);
    __buf = __grpbuf;
    __iend = __grpbuf + __len;
    __len = __insert_grouping(__buf, __iend, __grouping, __np.thousands_sep(),
                              '+', '-', __basechars);
  }

  return __copy_integer_and_fill(__buf, __len, __s, __flags, __f.width(0), __fill, '+', '-');
}

#if defined (_STLP_LONG_LONG)
typedef _STLP_LONG_LONG __max_int_t;
typedef unsigned _STLP_LONG_LONG __umax_int_t;
#else
typedef long __max_int_t;
typedef unsigned long __umax_int_t;
#endif

_STLP_DECLSPEC const char* _STLP_CALL __hex_char_table_lo();
_STLP_DECLSPEC const char* _STLP_CALL __hex_char_table_hi();

template <class _Integer>
inline char* _STLP_CALL
__write_decimal_backward(char* __ptr, _Integer __x, ios_base::fmtflags __flags, const __true_type& /* is_signed */) {
  const bool __negative = __x < 0 ;
  __max_int_t __temp = __x;
  __umax_int_t __utemp = __negative?-__temp:__temp;

  for (; __utemp != 0; __utemp /= 10)
    *--__ptr = (char)((int)(__utemp % 10) + '0');
  // put sign if needed or requested
  if (__negative)
    *--__ptr = '-';
  else if (__flags & ios_base::showpos)
    *--__ptr = '+';
  return __ptr;
}

template <class _Integer>
inline char* _STLP_CALL
__write_decimal_backward(char* __ptr, _Integer __x, ios_base::fmtflags __flags, const __false_type& /* is_signed */) {
  for (; __x != 0; __x /= 10)
    *--__ptr = (char)((int)(__x % 10) + '0');
  // put sign if requested
  if (__flags & ios_base::showpos)
    *--__ptr = '+';
  return __ptr;
}

template <class _Integer>
char* _STLP_CALL
__write_integer_backward(char* __buf, ios_base::fmtflags __flags, _Integer __x) {
  char* __ptr = __buf;

  if (__x == 0) {
    *--__ptr = '0';
    if ((__flags & ios_base::showpos) && ((__flags & (ios_base::oct | ios_base::hex)) == 0))
      *--__ptr = '+';
    // oct or hex base shall not be added to the 0 value (see '#' flag in C formating strings)
  }
  else {
    switch (__flags & ios_base::basefield) {
      case ios_base::oct:
        {
          __umax_int_t __temp = __x;
          // if the size of integer is less than 8, clear upper part
          if ( sizeof(__x) < 8  && sizeof(__umax_int_t) >= 8 )
            __temp &= 0xFFFFFFFF;

          for (; __temp != 0; __temp >>=3)
            *--__ptr = (char)((((unsigned)__temp)& 0x7) + '0');

          // put leading '0' if showbase is set
          if (__flags & ios_base::showbase)
            *--__ptr = '0';
        }
        break;
      case ios_base::hex:
        {
          const char* __table_ptr = (__flags & ios_base::uppercase) ?
            __hex_char_table_hi() : __hex_char_table_lo();
          __umax_int_t __temp = __x;
          // if the size of integer is less than 8, clear upper part
          if ( sizeof(__x) < 8  && sizeof(__umax_int_t) >= 8 )
            __temp &= 0xFFFFFFFF;

          for (; __temp != 0; __temp >>=4)
            *--__ptr = __table_ptr[((unsigned)__temp & 0xF)];

          if (__flags & ios_base::showbase) {
            *--__ptr = __table_ptr[16];
            *--__ptr = '0';
          }
        }
        break;
      //case ios_base::dec:
      default:
        {
#if defined(__HP_aCC) && (__HP_aCC == 1)
          bool _IsSigned = !((_Integer)-1 > 0);
          if (_IsSigned)
            __ptr = __write_decimal_backward(__ptr, __x, __flags, __true_type() );
          else
            __ptr = __write_decimal_backward(__ptr, __x, __flags, __false_type() );
#else
          typedef typename __bool2type<numeric_limits<_Integer>::is_signed>::_Ret _IsSigned;
          __ptr = __write_decimal_backward(__ptr, __x, __flags, _IsSigned());
#endif
        }
        break;
    }
  }

  // return pointer to beginning of the string
  return __ptr;
}

template <class _CharT, class _OutputIter, class _Integer>
_OutputIter _STLP_CALL
__do_put_integer(_OutputIter __s, ios_base& __f, _CharT __fill, _Integer __x) {
  // buffer size = number of bytes * number of digit necessary in the smallest Standard base (base 8, 3 digits/byte)
  //               plus the longest base representation '0x'
  // Do not use __buf_size to define __buf static buffer, some compilers (HP aCC) do not accept const variable as
  // the specification of a static buffer size.
  char __buf[sizeof(_Integer) * 3 + 2];
  const ptrdiff_t __buf_size = sizeof(__buf) / sizeof(char);
  ios_base::fmtflags __flags = __f.flags();
  char* __ibeg = __write_integer_backward((char*)__buf + __buf_size, __flags, __x);
  return __put_integer(__ibeg, (char*)__buf + __buf_size, __s, __f, __flags, __fill);
}

template <class _CharT, class _OutputIter>
_OutputIter _STLP_CALL
__do_put_bool(_OutputIter __s, ios_base& __f, _CharT __fill, bool __x) {
  const numpunct<_CharT>& __np = use_facet<numpunct<_CharT> >(__f.getloc());

  basic_string<_CharT, char_traits<_CharT>, allocator<_CharT> > __str = __x ? __np.truename() : __np.falsename();

  streamsize __wid = __f.width(0);
  if (__str.size() >= __STATIC_CAST(size_t, __wid))
    return _STLP_STD::copy(__str.begin(), __str.end(), __s);
  else {
    streamsize __pad = __wid - __str.size();
    ios_base::fmtflags __dir = __f.flags() & ios_base::adjustfield;

    if (__dir == ios_base::left) {
      __s = _STLP_STD::copy(__str.begin(), __str.end(), __s);
      return __fill_n(__s, __pad, __fill);
    }
    else /* covers right and internal padding */ {
      __s = __fill_n(__s, __pad, __fill);
      return _STLP_STD::copy(__str.begin(), __str.end(), __s);
    }
  }
}
_STLP_MOVE_TO_STD_NAMESPACE

//
// num_put<>
//

template <class _CharT, class _OutputIterator>
locale::id num_put<_CharT, _OutputIterator>::id;

#if !defined (_STLP_NO_BOOL)
template <class _CharT, class _OutputIter>
_OutputIter
num_put<_CharT, _OutputIter>::do_put(_OutputIter __s, ios_base& __f, _CharT __fill,
                                     bool __val) const {
  if (!(__f.flags() & ios_base::boolalpha))
    // 22.2.2.2.2.23: shall return do_put for int and not directly __do_put_integer.
    return do_put(__s, __f, __fill, __STATIC_CAST(long, __val));

  return _STLP_PRIV __do_put_bool(__s, __f, __fill, __val);
}
#endif

template <class _CharT, class _OutputIter>
_OutputIter
num_put<_CharT, _OutputIter>::do_put(_OutputIter __s, ios_base& __f, _CharT __fill,
                                     long __val) const
{ return _STLP_PRIV __do_put_integer(__s, __f, __fill, __val); }

template <class _CharT, class _OutputIter>
_OutputIter
num_put<_CharT, _OutputIter>::do_put(_OutputIter __s, ios_base& __f, _CharT __fill,
                                     unsigned long __val) const
{ return _STLP_PRIV __do_put_integer(__s, __f, __fill, __val); }

template <class _CharT, class _OutputIter>
_OutputIter
num_put<_CharT, _OutputIter>::do_put(_OutputIter __s, ios_base& __f, _CharT __fill,
                                     double __val) const
{ return _STLP_PRIV __do_put_float(__s, __f, __fill, __val); }

#if !defined (_STLP_NO_LONG_DOUBLE)
template <class _CharT, class _OutputIter>
_OutputIter
num_put<_CharT, _OutputIter>::do_put(_OutputIter __s, ios_base& __f, _CharT __fill,
                                     long double __val) const
{ return _STLP_PRIV __do_put_float(__s, __f, __fill, __val); }
#endif

#if defined (_STLP_LONG_LONG)
template <class _CharT, class _OutputIter>
_OutputIter
num_put<_CharT, _OutputIter>::do_put(_OutputIter __s, ios_base& __f, _CharT __fill,
                                     _STLP_LONG_LONG __val) const
{ return _STLP_PRIV __do_put_integer(__s, __f, __fill, __val); }

template <class _CharT, class _OutputIter>
_OutputIter
num_put<_CharT, _OutputIter>::do_put(_OutputIter __s, ios_base& __f, _CharT __fill,
                                     unsigned _STLP_LONG_LONG __val) const
{ return _STLP_PRIV __do_put_integer(__s, __f, __fill, __val); }
#endif /* _STLP_LONG_LONG */


// 22.2.2.2.2 Stage 1: "For conversion from void* the specifier is %p."
// This is not clear and I'm really don't follow this (below).
template <class _CharT, class _OutputIter>
_OutputIter
num_put<_CharT, _OutputIter>::do_put(_OutputIter __s, ios_base& __f, _CharT /*__fill*/,
                                     const void* __val) const {
  const ctype<_CharT>& __c_type = use_facet<ctype<_CharT> >(__f.getloc());
  ios_base::fmtflags __save_flags = __f.flags();

  __f.setf(ios_base::hex, ios_base::basefield);
  __f.setf(ios_base::showbase);
  __f.setf(ios_base::internal, ios_base::adjustfield);
  __f.width((sizeof(void*) * 2) + 2); // digits in pointer type plus '0x' prefix
  if ( __val == 0 ) {
    // base ('0x') not shown for null, but I really want to type it
    // for pointer. Print it first in this case.
    const char* __table_ptr = (__save_flags & ios_base::uppercase) ?
            _STLP_PRIV __hex_char_table_hi() : _STLP_PRIV __hex_char_table_lo();
    __s++ = __c_type.widen( '0' );
    __s++ = __c_type.widen( __table_ptr[16] );
    __f.width((sizeof(void*) * 2)); // digits in pointer type
  } else {
    __f.width((sizeof(void*) * 2) + 2); // digits in pointer type plus '0x' prefix
  }
#if defined (_STLP_MSVC)
#  pragma warning (push)
#  pragma warning (disable : 4311) //pointer truncation from 'const void*' to 'unsigned long'
#endif
  _OutputIter result =
#ifdef _STLP_LONG_LONG
    ( sizeof(void*) == sizeof(unsigned long) ) ?
#endif
    _STLP_PRIV __do_put_integer(__s, __f, __c_type.widen('0'), __REINTERPRET_CAST(unsigned long,__val))
#ifdef _STLP_LONG_LONG
      : /* ( sizeof(void*) == sizeof(unsigned _STLP_LONG_LONG) ) ? */
    _STLP_PRIV __do_put_integer(__s, __f, __c_type.widen('0'), __REINTERPRET_CAST(unsigned _STLP_LONG_LONG,__val))
#endif
        ;
#if defined (_STLP_MSVC)
#  pragma warning (pop)
#endif
  __f.flags(__save_flags);
  return result;
}

_STLP_END_NAMESPACE

#endif /* _STLP_NUM_PUT_C */

// Local Variables:
// mode:C++
// End:
