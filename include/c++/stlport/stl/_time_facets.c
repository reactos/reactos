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
#ifndef _STLP_TIME_FACETS_C
#define _STLP_TIME_FACETS_C

#ifndef _STLP_INTERNAL_TIME_FACETS_H
#  include <stl/_time_facets.h>
#endif

#ifndef _STLP_INTERNAL_NUM_PUT_H
#  include <stl/_num_put.h>
#endif

#ifndef _STLP_INTERNAL_NUM_GET_H
#  include <stl/_num_get.h>
#endif

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// Declarations of static template members.

template <class _CharT, class _InputIterator>
locale::id time_get<_CharT, _InputIterator>::id;

template <class _CharT, class _OutputIterator>
locale::id time_put<_CharT, _OutputIterator>::id;

_STLP_MOVE_TO_PRIV_NAMESPACE

/* Matching input against a list of names

 * Alphabetic input of the names of months and the names
 * of weekdays requires matching input against a list of names.
 * We use a simple generic algorithm to accomplish this.  This
 * algorithm is not very efficient, especially for longer lists
 * of names, but it probably does not matter for the initial
 * implementation and it may never matter, since we do not expect
 * this kind of input to be used very often.  The algorithm
 * could be improved fairly simply by creating a new list of
 * names still in the running at each iteration.  A more sophisticated
 * approach would be to build a tree to do the matching.
 *
 * We compare each character of the input to the corresponding
 * character of each name on the list that has not been eliminated,
 * either because every character in the name has already been
 * matched, or because some character has not been matched.  We
 * continue only as long as there are some names that have not been
 * eliminated.

 * We do not really need a random access iterator (a forward iterator
 * would do), but the extra generality makes the notation clumsier,
 * and we don't really need it.

 * We can recognize a failed match by the fact that the return value
 * will be __name_end.
 */

#define _MAXNAMES        24

template <class _InIt, class _NameIt>
size_t _STLP_CALL
__match(_InIt& __first, _InIt& __last, _NameIt __name, _NameIt __name_end) {
  typedef ptrdiff_t difference_type;
  difference_type __n = __name_end - __name;
  difference_type __i, __start = 0;
  size_t __pos = 0;
  difference_type __check_count = __n;
  bool __do_not_check[_MAXNAMES];
  size_t __matching_name_index = __n;

  memset(__do_not_check, 0, sizeof(__do_not_check));

  while (__first != __last) {
    difference_type __new_n = __n;
    for (__i = __start; __i < __n; ++__i) {
      if (!__do_not_check[__i]) {
        if (*__first == __name[__i][__pos]) {
          if (__pos == (__name[__i].size() - 1)) {
            __matching_name_index = __i;
            __do_not_check[__i] = true;
            if (__i == __start) ++__start;
            --__check_count;
            if (__check_count == 0) {
              ++__first;
              return __matching_name_index;
            }
          }
          __new_n = __i + 1;
        }
        else {
          __do_not_check[__i] = true;
          if (__i == __start) ++__start;
          --__check_count;
          if (__check_count == 0)
            return __matching_name_index;
        }
      }
      else {
        if (__i == __start) ++ __start;
      }
    }

    __n = __new_n;
    ++__first; ++__pos;
  }

  return __matching_name_index;
}

// __get_formatted_time reads input that is assumed to be formatted
// according to the rules for the C strftime function (C standard,
// 7.12.3.5).  This function is used to implement the do_get_time
// and do_get_date virtual functions, which depend on the locale
// specifications for the time and day formats respectively.
// Note the catchall default case, intended mainly for the '%Z'
// format designator, which does not make sense here since the
// representation of timezones is not part of the locale.
//
// The case branches are implemented either by doing a match using
// the appopriate name table or by doing a __get_integer_nogroup.
//
// 'y' format is assumed to mean that the input represents years
// since 1900.  That is, 2002 should be represented as 102.  There
// is no century-guessing.
//
// The match is successful if and only if the second component of the
// return value is format_end.

// Note that the antepenultimate parameter is being used only to determine
// the correct overloading for the calls to __get_integer_nogroup.
template <class _InIt1, class _Ch, class _TimeInfo>
string::const_iterator _STLP_CALL
__get_formatted_time _STLP_WEAK (_InIt1 __first,  _InIt1 __last,
                                 string::const_iterator __format, string::const_iterator __format_end,
                                 _Ch*, const _TimeInfo& __table,
                                 const ios_base& __s, ios_base::iostate& __err, tm* __t) {
  const ctype<_Ch>& __ct = use_facet<ctype<_Ch> >(__s.getloc());
  typedef basic_string<_Ch, char_traits<_Ch>, allocator<_Ch> > string_type;
  size_t offset;

  while (__first != __last && __format != __format_end) {
    offset = 0;
    if (*__format == '%') {
      ++__format;
      char __c = *__format;
      if (__c == '#') { //MS extension
        ++__format;
        __c = *__format;
      }

      switch (__c) {
        case 'A':
          offset = 7;
        case 'a': {
          size_t __index = __match(__first, __last,
                                   __table._M_dayname + offset, __table._M_dayname + offset + 7);
          if (__index == 7)
            return __format;
          __t->tm_wday = __STATIC_CAST(int, __index);
          break;
        }

        case 'B':
          offset = 12;
        case 'b': {
          size_t __index = __match(__first, __last,
                                   __table._M_monthname + offset, __table._M_monthname + offset + 12);
          if (__index == 12)
            return __format;
          __t->tm_mon = __STATIC_CAST(int, __index);
          break;
        }

        case 'd': {
          bool __pr = __get_decimal_integer(__first, __last, __t->tm_mday, __STATIC_CAST(_Ch*, 0));
          if (!__pr || __t->tm_mday < 1 || __t->tm_mday > 31) {
            __err |= ios_base::failbit;
            return __format;
          }
          break;
        }

        case 'H': case 'I': {
          bool __pr = __get_decimal_integer(__first, __last, __t->tm_hour, __STATIC_CAST(_Ch*, 0));
          if (!__pr)
            return __format;
          break;
        }

        case 'j': {
          bool __pr = __get_decimal_integer(__first, __last, __t->tm_yday, __STATIC_CAST(_Ch*, 0));
          if (!__pr)
            return __format;
          break;
        }

        case 'm': {
          bool __pr = __get_decimal_integer(__first, __last, __t->tm_mon, __STATIC_CAST(_Ch*, 0));
          --__t->tm_mon;
          if (!__pr || __t->tm_mon < 0 || __t->tm_mon > 11) {
            __err |= ios_base::failbit;
            return __format;
          }
          break;
        }

        case 'M': {
          bool __pr = __get_decimal_integer(__first, __last, __t->tm_min, __STATIC_CAST(_Ch*, 0));
          if (!__pr)
            return __format;
          break;
        }

        case 'p': {
          size_t __index = __match(__first, __last,
                                   __table._M_am_pm + 0, __table._M_am_pm + 2);
          if (__index == 2)
            return __format;
          // 12:00 PM <=> 12:00, 12:00 AM <=> 00:00
          if (__index == 1 && __t->tm_hour != 12 )
            __t->tm_hour += 12;
          if (__index == 0 && __t->tm_hour == 12 )
            __t->tm_hour = 0;
          break;
        }

        case 'S': {
          bool __pr = __get_decimal_integer(__first, __last, __t->tm_sec, __STATIC_CAST(_Ch*, 0));
          if (!__pr)
            return __format;
          break;
        }

        case 'y': {
          bool __pr = __get_decimal_integer(__first, __last, __t->tm_year, __STATIC_CAST(_Ch*, 0));
          if (!__pr)
            return __format;
          break;
        }

        case 'Y': {
          bool __pr = __get_decimal_integer(__first, __last, __t->tm_year, __STATIC_CAST(_Ch*, 0));
          __t->tm_year -= 1900;
          if (!__pr)
            return __format;
          break;
        }

        default:
          break;
      }
    }
    else {
      if (*__first++ != __ct.widen(*__format)) break;
    }

    ++__format;
  }

  return __format;
}

template <class _InIt, class _TimeInfo>
bool _STLP_CALL
__get_short_or_long_dayname(_InIt& __first, _InIt& __last, const _TimeInfo& __table, tm* __t) {
  size_t __index = __match(__first, __last, __table._M_dayname + 0, __table._M_dayname + 14);
  if (__index != 14) {
    __t->tm_wday = __STATIC_CAST(int, __index % 7);
    return true;
  }
  return false;
}

template <class _InIt, class _TimeInfo>
bool _STLP_CALL
__get_short_or_long_monthname(_InIt& __first, _InIt& __last, const _TimeInfo& __table, tm* __t) {
  size_t __index = __match(__first, __last, __table._M_monthname + 0, __table._M_monthname + 24);
  if (__index != 24) {
    __t->tm_mon = __STATIC_CAST(int, __index % 12);
    return true;
  }
  return false;
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _Ch, class _InIt>
_InIt
time_get<_Ch, _InIt>::do_get_date(_InIt __s, _InIt  __end,
                                  ios_base& __str, ios_base::iostate&  __err,
                                  tm* __t) const {
  typedef string::const_iterator string_iterator;

  string_iterator __format = this->_M_timeinfo._M_date_format.begin();
  string_iterator __format_end = this->_M_timeinfo._M_date_format.end();

  string_iterator __result
    = _STLP_PRIV __get_formatted_time(__s, __end, __format, __format_end,
                                      __STATIC_CAST(_Ch*, 0), this->_M_timeinfo,
                                      __str, __err, __t);
  if (__result == __format_end)
    __err = ios_base::goodbit;
  else {
    __err = ios_base::failbit;
    if (__s == __end)
      __err |= ios_base::eofbit;
  }
  return __s;
}

template <class _Ch, class _InIt>
_InIt
time_get<_Ch, _InIt>::do_get_time(_InIt __s, _InIt  __end,
                                  ios_base& __str, ios_base::iostate&  __err,
                                  tm* __t) const {
  typedef string::const_iterator string_iterator;
  string_iterator __format = this->_M_timeinfo._M_time_format.begin();
  string_iterator __format_end = this->_M_timeinfo._M_time_format.end();

  string_iterator __result
    = _STLP_PRIV __get_formatted_time(__s, __end, __format, __format_end,
                                      __STATIC_CAST(_Ch*, 0), this->_M_timeinfo,
                                      __str, __err, __t);
  __err = __result == __format_end ? ios_base::goodbit
                                   : ios_base::failbit;
  if (__s == __end)
    __err |= ios_base::eofbit;
  return __s;
}

template <class _Ch, class _InIt>
_InIt
time_get<_Ch, _InIt>::do_get_year(_InIt __s, _InIt  __end,
                                  ios_base&, ios_base::iostate&  __err,
                                  tm* __t) const {
  if (__s == __end) {
    __err = ios_base::failbit | ios_base::eofbit;
    return __s;
  }

  bool __pr =  _STLP_PRIV __get_decimal_integer(__s, __end, __t->tm_year, __STATIC_CAST(_Ch*, 0));
  __t->tm_year -= 1900;
  __err = __pr ? ios_base::goodbit : ios_base::failbit;
  if (__s == __end)
    __err |= ios_base::eofbit;

  return __s;
}

template <class _Ch, class _InIt>
_InIt
time_get<_Ch, _InIt>::do_get_weekday(_InIt __s, _InIt  __end,
                                     ios_base &__str, ios_base::iostate &__err,
                                     tm *__t) const {
  bool __result =
    _STLP_PRIV __get_short_or_long_dayname(__s, __end, this->_M_timeinfo, __t);
  if (__result)
    __err = ios_base::goodbit;
  else {
    __err = ios_base::failbit;
    if (__s == __end)
      __err |= ios_base::eofbit;
  }
  return __s;
}

template <class _Ch, class _InIt>
_InIt
time_get<_Ch, _InIt>::do_get_monthname(_InIt __s, _InIt  __end,
                                       ios_base &__str, ios_base::iostate &__err,
                                       tm *__t) const {
  bool __result =
    _STLP_PRIV __get_short_or_long_monthname(__s, __end, this->_M_timeinfo, __t);
  if (__result)
    __err = ios_base::goodbit;
  else {
    __err = ios_base::failbit;
    if (__s == __end)
      __err |= ios_base::eofbit;
  }
  return __s;
}

template<class _Ch, class _OutputIter>
_OutputIter
time_put<_Ch,_OutputIter>::put(_OutputIter __s, ios_base& __f, _Ch __fill,
                               const tm* __tmb, const _Ch* __pat,
                               const _Ch* __pat_end) const {
  const ctype<_Ch>& _Ct = use_facet<ctype<_Ch> >(__f.getloc());
  while (__pat != __pat_end) {
    char __c = _Ct.narrow(*__pat, 0);
    if (__c == '%') {
      char __mod = 0;
      ++__pat;
      __c = _Ct.narrow(*__pat++, 0);
      if (__c == '#') { // MS extension
        __mod = __c;
        __c = _Ct.narrow(*__pat++, 0);
      }
      __s = do_put(__s, __f, __fill, __tmb, __c, __mod);
    }
    else
      *__s++ = *__pat++;
  }
  return __s;
}

template<class _Ch, class _OutputIter>
_OutputIter
time_put<_Ch,_OutputIter>::do_put(_OutputIter __s, ios_base& __f, _Ch /* __fill */,
                                  const tm* __tmb, char __format,
                                  char __modifier ) const {
  const ctype<_Ch>& __ct = use_facet<ctype<_Ch> >(__f.getloc());
  _STLP_BASIC_IOSTRING(_Ch) __buf;
  _STLP_PRIV __write_formatted_time(__buf, __ct, __format, __modifier, this->_M_timeinfo, __tmb);
  return copy(__buf.begin(), __buf.end(), __s);
}

_STLP_END_NAMESPACE

#endif /* _STLP_TIME_FACETS_C */

// Local Variables:
// mode:C++
// End:
