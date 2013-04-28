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
#include "stlport_prefix.h"

#include <locale>
#include <istream>
#include <algorithm>

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

// __valid_grouping compares two strings, one representing the
// group sizes encountered when reading an integer, and the other
// representing the valid group sizes as returned by the numpunct
// grouping() member function.  Both are interpreted right-to-left.
// The grouping string is treated as if it were extended indefinitely
// with its last value.  For a grouping to be valid, each term in
// the first string must be equal to the corresponding term in the
// second, except for the last, which must be less than or equal.

// boris : this takes reversed first string !
bool  _STLP_CALL
__valid_grouping(const char * first1, const char * last1,
                 const char * first2, const char * last2) {
  if (first1 == last1 || first2 == last2) return true;

  --last1; --last2;

  while (first1 != last1) {
    if (*last1 != *first2)
      return false;
    --last1;
    if (first2 != last2) ++first2;
  }

  return *last1 <= *first2;
}

_STLP_DECLSPEC unsigned char _STLP_CALL __digit_val_table(unsigned __index) {
  static const unsigned char __val_table[128] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,10,11,12,13,14,15,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,10,11,12,13,14,15,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
  };

  return __val_table[__index];
}

_STLP_DECLSPEC const char* _STLP_CALL __narrow_atoms()
{ return "+-0xX"; }

// index is actually a char

#if !defined (_STLP_NO_WCHAR_T)

// Similar, except return the character itself instead of the numeric
// value.  Used for floating-point input.
bool _STLP_CALL __get_fdigit(wchar_t& c, const wchar_t* digits) {
  const wchar_t* p = find(digits, digits + 10, c);
  if (p != digits + 10) {
    c = (char)('0' + (p - digits));
    return true;
  }
  else
    return false;
}

bool _STLP_CALL __get_fdigit_or_sep(wchar_t& c, wchar_t sep,
                                    const wchar_t * digits) {
  if (c == sep) {
    c = (char)',';
    return true;
  }
  else
    return __get_fdigit(c, digits);
}

#endif

_STLP_MOVE_TO_STD_NAMESPACE

#if !defined(_STLP_NO_FORCE_INSTANTIATE)
//----------------------------------------------------------------------
// Force instantiation of num_get<>
template class _STLP_CLASS_DECLSPEC istreambuf_iterator<char, char_traits<char> >;
// template class num_get<char, const char*>;
template class num_get<char, istreambuf_iterator<char, char_traits<char> > >;

#  if !defined (_STLP_NO_WCHAR_T)
template class _STLP_CLASS_DECLSPEC  istreambuf_iterator<wchar_t, char_traits<wchar_t> >;
template class num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
// template class num_get<wchar_t, const wchar_t*>;
#  endif
#endif

_STLP_END_NAMESPACE

// Local Variables:
// mode:C++
// End:
