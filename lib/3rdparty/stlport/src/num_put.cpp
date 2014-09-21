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
#include <ostream>

_STLP_BEGIN_NAMESPACE

// Note that grouping[0] is the number of digits in the *rightmost* group.
// We assume, without checking, that *last is null and that there is enough
// space in the buffer to extend the number past [first, last).
template <class Char>
static ptrdiff_t
__insert_grouping_aux(Char* first, Char* last, const string& grouping,
                      Char separator, Char Plus, Char Minus,
                      int basechars) {
  typedef string::size_type str_size;

  if (first == last)
    return 0;

  int sign = 0;

  if (*first == Plus || *first == Minus) {
    sign = 1;
    ++first;
  }

  first += basechars;
  Char* cur_group = last; // Points immediately beyond the rightmost
                          // digit of the current group.
  int groupsize = 0; // Size of the current group (if grouping.size() == 0, size
                     // of group unlimited: we force condition (groupsize <= 0))

  for ( str_size n = 0; ; ) { // Index of the current group
    if ( n < grouping.size() ) {
      groupsize = __STATIC_CAST(int, grouping[n++] );
    }

    if ((groupsize <= 0) || (groupsize >= cur_group - first) || (groupsize == CHAR_MAX)) {
      break;
    }

    // Insert a separator character just before position cur_group - groupsize
    cur_group -= groupsize;
    ++last;
    copy_backward(cur_group, last, last + 1);
    *cur_group = separator;
  }

  return (last - first) + sign + basechars;
}

//Dynamic output buffer version.
template <class Char, class Str>
static void
__insert_grouping_aux( /* __basic_iostring<Char> */ Str& iostr, size_t __group_pos,
                      const string& grouping,
                      Char separator, Char Plus, Char Minus,
                      int basechars) {
  typedef string::size_type str_size;

  if (iostr.size() < __group_pos)
    return;

  int __first_pos = 0;
  Char __first = *iostr.begin();

  if (__first == Plus || __first == Minus) {
    ++__first_pos;
  }

  __first_pos += basechars;

  typename Str::iterator cur_group(iostr.begin() + __group_pos);    // Points immediately beyond the rightmost
                                                                    // digit of the current group.
  int groupsize = 0; // Size of the current group (if grouping.size() == 0, size
                     // of group unlimited: we force condition (groupsize <= 0))

  for ( str_size n = 0; ; ) { // Index of the current group
    if ( n < grouping.size() ) {
      groupsize = __STATIC_CAST( int, grouping[n++] );
    }

    if ( (groupsize <= 0) || (groupsize >= ((cur_group - iostr.begin()) - __first_pos)) ||
         (groupsize == CHAR_MAX)) {
      break;
    }

    // Insert a separator character just before position cur_group - groupsize
    cur_group -= groupsize;
    cur_group = iostr.insert(cur_group, separator);
  }
}

//----------------------------------------------------------------------
// num_put

_STLP_MOVE_TO_PRIV_NAMESPACE

_STLP_DECLSPEC const char* _STLP_CALL __hex_char_table_lo()
{ return "0123456789abcdefx"; }

_STLP_DECLSPEC const char* _STLP_CALL __hex_char_table_hi()
{ return "0123456789ABCDEFX"; }

char* _STLP_CALL
__write_integer(char* buf, ios_base::fmtflags flags, long x) {
  char tmp[64];
  char* bufend = tmp+64;
  char* beg = __write_integer_backward(bufend, flags, x);
  return copy(beg, bufend, buf);
}

///-------------------------------------

ptrdiff_t _STLP_CALL
__insert_grouping(char * first, char * last, const string& grouping,
                  char separator, char Plus, char Minus, int basechars) {
  return __insert_grouping_aux(first, last, grouping,
                               separator, Plus, Minus, basechars);
}

void _STLP_CALL
__insert_grouping(__iostring &str, size_t group_pos, const string& grouping,
                  char separator, char Plus, char Minus, int basechars) {
  __insert_grouping_aux(str, group_pos, grouping, separator, Plus, Minus, basechars);
}

#if !defined (_STLP_NO_WCHAR_T)
ptrdiff_t _STLP_CALL
__insert_grouping(wchar_t* first, wchar_t* last, const string& grouping,
                  wchar_t separator, wchar_t Plus, wchar_t Minus,
                  int basechars) {
  return __insert_grouping_aux(first, last, grouping, separator,
                               Plus, Minus, basechars);
}

void _STLP_CALL
__insert_grouping(__iowstring &str, size_t group_pos, const string& grouping,
                  wchar_t separator, wchar_t Plus, wchar_t Minus,
                  int basechars) {
  __insert_grouping_aux(str, group_pos, grouping, separator, Plus, Minus, basechars);
}
#endif

_STLP_MOVE_TO_STD_NAMESPACE

//----------------------------------------------------------------------
// Force instantiation of num_put<>
#if !defined(_STLP_NO_FORCE_INSTANTIATE)
template class _STLP_CLASS_DECLSPEC ostreambuf_iterator<char, char_traits<char> >;
// template class num_put<char, char*>;
template class num_put<char, ostreambuf_iterator<char, char_traits<char> > >;
# ifndef _STLP_NO_WCHAR_T
template class ostreambuf_iterator<wchar_t, char_traits<wchar_t> >;
template class num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
// template class num_put<wchar_t, wchar_t*>;
# endif /* INSTANTIATE_WIDE_STREAMS */
#endif

_STLP_END_NAMESPACE

// Local Variables:
// mode:C++
// End:
