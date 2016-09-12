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

#include <cstdio>
#include <locale>
#include <istream>

#include "c_locale.h"
#include "acquire_release.h"

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

// default "C" values for month and day names

const char default_dayname[][14] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
  "Friday", "Saturday"};

const char default_monthname[][24] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December"};

#ifndef _STLP_NO_WCHAR_T
const wchar_t default_wdayname[][14] = {
  L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat",
  L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday",
  L"Friday", L"Saturday"};

const wchar_t default_wmonthname[][24] = {
  L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
  L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec",
  L"January", L"February", L"March", L"April", L"May", L"June",
  L"July", L"August", L"September", L"October", L"November", L"December"};
#endif

#if defined (__BORLANDC__)
_Time_Info time_init<char>::_M_timeinfo;
#  ifndef _STLP_NO_WCHAR_T
_WTime_Info time_init<wchar_t>::_M_timeinfo;
#  endif
#endif

// _Init_time_info: initialize table with
// "C" values (note these are not defined in the C standard, so this
// is somewhat arbitrary).

static void _Init_timeinfo_base(_Time_Info_Base& table) {
  table._M_time_format = "%H:%M:%S";
  table._M_date_format = "%m/%d/%y";
  table._M_date_time_format = "%m/%d/%y";
}

static void _Init_timeinfo(_Time_Info& table) {
  int i;
  for (i = 0; i < 14; ++i)
    table._M_dayname[i] = default_dayname[i];
  for (i = 0; i < 24; ++i)
    table._M_monthname[i] = default_monthname[i];
  table._M_am_pm[0] = "AM";
  table._M_am_pm[1] = "PM";
  _Init_timeinfo_base(table);
}

#ifndef _STLP_NO_WCHAR_T
static void _Init_timeinfo(_WTime_Info& table) {
  int i;
  for (i = 0; i < 14; ++i)
    table._M_dayname[i] = default_wdayname[i];
  for (i = 0; i < 24; ++i)
    table._M_monthname[i] = default_wmonthname[i];
  table._M_am_pm[0] = L"AM";
  table._M_am_pm[1] = L"PM";
  _Init_timeinfo_base(table);
}
#endif

static void _Init_timeinfo_base(_Time_Info_Base& table, _Locale_time * time) {
  table._M_time_format = _Locale_t_fmt(time);
  if ( table._M_time_format == "%T" ) {
    table._M_time_format = "%H:%M:%S";
  } else if ( table._M_time_format == "%r" ) {
    table._M_time_format = "%I:%M:%S %p";
  } else if ( table._M_time_format == "%R" ) {
    table._M_time_format = "%H:%M";
  }
  table._M_date_format = _Locale_d_fmt(time);
  table._M_date_time_format = _Locale_d_t_fmt(time);
  table._M_long_date_format = _Locale_long_d_fmt(time);
  table._M_long_date_time_format = _Locale_long_d_t_fmt(time);
}

static void _Init_timeinfo(_Time_Info& table, _Locale_time * time) {
  int i;
  for (i = 0; i < 7; ++i)
    table._M_dayname[i] = _Locale_abbrev_dayofweek(time, i);
  for (i = 0; i < 7; ++i)
    table._M_dayname[i+7] = _Locale_full_dayofweek(time, i);
  for (i = 0; i < 12; ++i)
    table._M_monthname[i] = _Locale_abbrev_monthname(time, i);
  for (i = 0; i < 12; ++i)
    table._M_monthname[i+12] = _Locale_full_monthname(time, i);
  table._M_am_pm[0] = _Locale_am_str(time);
  table._M_am_pm[1] = _Locale_pm_str(time);
  _Init_timeinfo_base(table, time);
}

#ifndef _STLP_NO_WCHAR_T
static void _Init_timeinfo(_WTime_Info& table, _Locale_time * time) {
  wchar_t buf[128];
  int i;
  for (i = 0; i < 7; ++i)
    table._M_dayname[i] = _WLocale_abbrev_dayofweek(time, i, _STLP_ARRAY_AND_SIZE(buf));
  for (i = 0; i < 7; ++i)
    table._M_dayname[i+7] = _WLocale_full_dayofweek(time, i, _STLP_ARRAY_AND_SIZE(buf));
  for (i = 0; i < 12; ++i)
    table._M_monthname[i] = _WLocale_abbrev_monthname(time, i, _STLP_ARRAY_AND_SIZE(buf));
  for (i = 0; i < 12; ++i)
    table._M_monthname[i+12] = _WLocale_full_monthname(time, i, _STLP_ARRAY_AND_SIZE(buf));
  table._M_am_pm[0] = _WLocale_am_str(time, _STLP_ARRAY_AND_SIZE(buf));
  table._M_am_pm[1] = _WLocale_pm_str(time, _STLP_ARRAY_AND_SIZE(buf));
  _Init_timeinfo_base(table, time);
}
#endif

template <class _Ch, class _TimeInfo>
void __subformat(_STLP_BASIC_IOSTRING(_Ch) &buf, const ctype<_Ch>& ct,
                 const string& format, const _TimeInfo& table, const tm* t) {
  const char * cp = format.data();
  const char * cp_end = cp + format.size();
  while (cp != cp_end) {
    if (*cp == '%') {
      char mod = 0;
      ++cp;
      if (*cp == '#') {
        mod = *cp; ++cp;
      }
      __write_formatted_timeT(buf, ct, *cp++, mod, table, t);
    } else
      buf.append(1, *cp++);
  }
}

static void __append(__iostring &buf, const string& name)
{ buf.append(name.data(), name.data() + name.size()); }

static void __append(__iowstring &buf, const wstring& name)
{ buf.append(name.data(), name.data() + name.size()); }

static void __append(__iostring &buf, char *first, char *last, const ctype<char>& /* ct */)
{ buf.append(first, last); }

static void __append(__iowstring &buf, char *first, char *last, const ctype<wchar_t>& ct) {
  wchar_t _wbuf[64];
  ct.widen(first, last, _wbuf);
  buf.append(_wbuf, _wbuf + (last - first));
}

#if defined (__GNUC__)
/* The number of days from the first day of the first ISO week of this
   year to the year day YDAY with week day WDAY.  ISO weeks start on
   Monday; the first ISO week has the year's first Thursday.  YDAY may
   be as small as YDAY_MINIMUM.  */
#  define __ISO_WEEK_START_WDAY 1 /* Monday */
#  define __ISO_WEEK1_WDAY 4 /* Thursday */
#  define __YDAY_MINIMUM (-366)
#  define __TM_YEAR_BASE 1900
static int
__iso_week_days(int yday, int wday) {
  /* Add enough to the first operand of % to make it nonnegative.  */
  int big_enough_multiple_of_7 = (-__YDAY_MINIMUM / 7 + 2) * 7;
  return (yday
          - (yday - wday + __ISO_WEEK1_WDAY + big_enough_multiple_of_7) % 7
          + __ISO_WEEK1_WDAY - __ISO_WEEK_START_WDAY);
}

#  define __is_leap(year)\
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

#endif

#define __hour12(hour) \
  (((hour) % 12 == 0) ? (12) : (hour) % 12)

#if !defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
#  define _STLP_SPRINTF sprintf
#else
#  define _STLP_SPRINTF sprintf_s
#endif

template <class _Ch, class _TimeInfo>
void _STLP_CALL __write_formatted_timeT(_STLP_BASIC_IOSTRING(_Ch) &buf,
                                        const ctype<_Ch>& ct,
                                        char format, char modifier,
                                        const _TimeInfo& table, const tm* t) {
  char _buf[64];
  char *_bend;

  switch (format) {
    case 'a':
      __append(buf, table._M_dayname[t->tm_wday]);
      break;

    case 'A':
      __append(buf, table._M_dayname[t->tm_wday + 7]);
      break;

    case 'b':
      __append(buf, table._M_monthname[t->tm_mon]);
      break;

    case 'B':
      __append(buf, table._M_monthname[t->tm_mon + 12]);
      break;

    case 'c':
      __subformat(buf, ct, (modifier != '#') ? table._M_date_time_format
                                             : table._M_long_date_time_format, table, t);
      break;

    case 'd':
      _STLP_SPRINTF(_buf, (modifier != '#') ? "%.2ld" : "%ld", (long)t->tm_mday);
      __append(buf, _buf, ((long)t->tm_mday < 10L && modifier == '#') ? _buf + 1 : _buf + 2, ct);
      break;

    case 'e':
      _STLP_SPRINTF(_buf, "%2ld", (long)t->tm_mday);
      __append(buf, _buf, _buf + 2, ct);
      break;

    case 'H':
      _STLP_SPRINTF(_buf, (modifier != '#') ? "%.2ld" : "%ld", (long)t->tm_hour);
      __append(buf, _buf, ((long)t->tm_hour < 10L && modifier == '#') ? _buf + 1 : _buf + 2, ct);
      break;

    case 'I':
      _STLP_SPRINTF(_buf, (modifier != '#') ? "%.2ld" : "%ld", (long)__hour12(t->tm_hour));
      __append(buf, _buf, ((long)__hour12(t->tm_hour) < 10L && modifier == '#') ? _buf + 1 : _buf + 2, ct);
      break;

    case 'j':
      _bend = __write_integer(_buf, 0, (long)((long)t->tm_yday + 1));
      __append(buf, _buf, _bend, ct);
      break;

    case 'm':
      _STLP_SPRINTF(_buf, (modifier != '#') ? "%.2ld" : "%ld", (long)t->tm_mon + 1);
      __append(buf, _buf, ((long)(t->tm_mon + 1) < 10L && modifier == '#') ? _buf + 1 : _buf + 2, ct);
      break;

    case 'M':
      _STLP_SPRINTF(_buf, (modifier != '#') ? "%.2ld" : "%ld", (long)t->tm_min);
      __append(buf, _buf, ((long)t->tm_min < 10L && modifier == '#') ? _buf + 1 : _buf + 2, ct);
      break;

    case 'p':
      __append(buf, table._M_am_pm[t->tm_hour / 12]);
      break;

    case 'S': // pad with zeros
       _STLP_SPRINTF(_buf, (modifier != '#') ? "%.2ld" : "%ld", (long)t->tm_sec);
       __append(buf, _buf, ((long)t->tm_sec < 10L && modifier == '#') ? _buf + 1 : _buf + 2, ct);
       break;

    case 'U':
      _bend = __write_integer(_buf, 0, long((t->tm_yday - t->tm_wday + 7) / 7));
      __append(buf, _buf, _bend, ct);
      break;

    case 'w':
      _bend = __write_integer(_buf, 0, (long)t->tm_wday);
      __append(buf, _buf, _bend, ct);
      break;

    case 'W':
      _bend = __write_integer(_buf, 0,
                             (long)(t->tm_wday == 0 ? (t->tm_yday + 1) / 7 :
                                                      (t->tm_yday + 8 - t->tm_wday) / 7));
      __append(buf, _buf, _bend, ct);
      break;

    case'x':
      __subformat(buf, ct, (modifier != '#') ? table._M_date_format
                                             : table._M_long_date_format, table, t);
      break;

    case 'X':
      __subformat(buf, ct, table._M_time_format, table, t);
      break;

    case 'y':
      _bend = __write_integer(_buf, 0, (long)((long)(t->tm_year + 1900) % 100));
      __append(buf, _buf, _bend, ct);
      break;

    case 'Y':
      _bend = __write_integer(_buf, 0, (long)((long)t->tm_year + 1900));
      __append(buf, _buf, _bend, ct);
      break;

    case '%':
      buf.append(1, ct.widen('%'));
      break;

#if defined (__GNUC__)
      // fbp : at least on SUN
#  if defined (_STLP_UNIX) && !defined (__linux__)
#    define __USE_BSD 1
#  endif

   /*********************************************
    *     JGS, handle various extensions        *
    *********************************************/

    case 'h': /* POSIX.2 extension */
      // same as 'b', abbrev month name
      __append(buf, table._M_monthname[t->tm_mon]);
      break;
    case 'C': /* POSIX.2 extension */
      // same as 'd', the day
      _STLP_SPRINTF(_buf, "%2ld", (long)t->tm_mday);
      __append(buf, _buf, _buf + 2, ct);
      break;

    case 'D': /* POSIX.2 extension */
      // same as 'x'
      __subformat(buf, ct, table._M_date_format, table, t);
      break;

    case 'k': /* GNU extension */
      _STLP_SPRINTF(_buf, "%2ld", (long)t->tm_hour);
      __append(buf, _buf, _buf + 2, ct);
      break;

    case 'l': /* GNU extension */
      _STLP_SPRINTF(_buf, "%2ld", (long)t->tm_hour % 12);
      __append(buf, _buf, _buf + 2, ct);
      break;

    case 'n': /* POSIX.2 extension */
      buf.append(1, ct.widen('\n'));
      break;

    case 'R': /* GNU extension */
      __subformat(buf, ct, "%H:%M", table, t);
      break;

    case 'r': /* POSIX.2 extension */
      __subformat(buf, ct, "%I:%M:%S %p", table, t);
      break;

    case 'T': /* POSIX.2 extension.  */
      __subformat(buf, ct, "%H:%M:%S", table, t);
      break;

    case 't': /* POSIX.2 extension.  */
      buf.append(1, ct.widen('\t'));

    case 'u': /* POSIX.2 extension.  */
      _bend = __write_integer(_buf, 0, long((t->tm_wday - 1 + 7)) % 7 + 1);
      __append(buf, _buf, _bend, ct);
      break;

    case 's': {
      time_t __t = mktime(__CONST_CAST(tm*, t));
      _bend = __write_integer(_buf, 0, (long)__t );
      __append(buf, _buf, _bend, ct);
      break;
    }
    case 'g': /* GNU extension */
    case 'G': {
      int year = t->tm_year + __TM_YEAR_BASE;
      int days = __iso_week_days (t->tm_yday, t->tm_wday);
      if (days < 0) {
        /* This ISO week belongs to the previous year.  */
        year--;
        days = __iso_week_days (t->tm_yday + (365 + __is_leap (year)), t->tm_wday);
      }
      else {
        int d = __iso_week_days (t->tm_yday - (365 + __is_leap (year)), t->tm_wday);
        if (0 <= d) {
          /* This ISO week belongs to the next year.  */
          ++year;
          days = d;
        }
      }
      long val;
      switch (format) {
      case 'g':
        val = (long)(year % 100 + 100) % 100;
        break;
      case 'G':
        val = (long)year;
        break;
      default:
        val = (long)days / 7 + 1;
        break;
      }
      _bend = __write_integer(_buf, 0, val);
      __append(buf, _buf, _bend, ct);
      break;
    }

#  if defined (_STLP_USE_GLIBC)
    case 'z':   /* GNU extension.  */
      if (t->tm_isdst < 0)
        break;
      {
        int diff;
#    if defined (__USE_BSD) || defined (__BEOS__)
        diff = t->tm_gmtoff;
#    else
        diff = t->__tm_gmtoff;
#    endif
        if (diff < 0) {
          buf.append(1, ct.widen('-'));
          diff = -diff;
        } else
          buf.append(1, ct.widen('+'));
        diff /= 60;
        _STLP_SPRINTF(_buf, "%.4d", (diff / 60) * 100 + diff % 60);
        __append(buf, _buf, _buf + 4, ct);
        break;
      }
#  endif /* __GLIBC__ */
#endif /* __GNUC__ */

    default:
      break;
  }
}

void _STLP_CALL __write_formatted_time(__iostring &buf, const ctype<char>& ct,
                                       char format, char modifier,
                                       const _Time_Info& table, const tm* t)
{ __write_formatted_timeT(buf, ct, format, modifier, table, t); }

void _STLP_CALL __write_formatted_time(__iowstring &buf, const ctype<wchar_t>& ct,
                                       char format, char modifier,
                                       const _WTime_Info& table, const tm* t)
{ __write_formatted_timeT(buf, ct, format, modifier, table, t); }

static time_base::dateorder __get_date_order(_Locale_time* time) {
  const char * fmt = _Locale_d_fmt(time);
  char first, second, third;

  while (*fmt != 0 && *fmt != '%') ++fmt;
  if (*fmt == 0)
    return time_base::no_order;
  first = *++fmt;
  while (*fmt != 0 && *fmt != '%') ++fmt;
  if (*fmt == 0)
    return time_base::no_order;
  second = *++fmt;
  while (*fmt != 0 && *fmt != '%') ++fmt;
  if (*fmt == 0)
    return time_base::no_order;
  third = *++fmt;

  switch (first) {
    case 'd':
      return (second == 'm' && third == 'y') ? time_base::dmy
                                             : time_base::no_order;
    case 'm':
      return (second == 'd' && third == 'y') ? time_base::mdy
                                             : time_base::no_order;
    case 'y':
      switch (second) {
        case 'd':
          return third == 'm' ? time_base::ydm : time_base::no_order;
        case 'm':
          return third == 'd' ? time_base::ymd : time_base::no_order;
        default:
          return time_base::no_order;
      }
    default:
      return time_base::no_order;
  }
}

time_init<char>::time_init()
  : _M_dateorder(time_base::no_order)
{ _Init_timeinfo(_M_timeinfo); }

time_init<char>::time_init(const char* __name) {
  if (!__name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _Locale_time *__time = __acquire_time(__name, buf, 0, &__err_code);
  if (!__time)
    locale::_M_throw_on_creation_failure(__err_code, __name, "time");

  _Init_timeinfo(this->_M_timeinfo, __time);
  _M_dateorder = __get_date_order(__time);
  __release_time(__time);
}

time_init<char>::time_init(_Locale_time *__time) {
  _Init_timeinfo(this->_M_timeinfo, __time);
  _M_dateorder = __get_date_order(__time);
}

#ifndef _STLP_NO_WCHAR_T
time_init<wchar_t>::time_init()
  : _M_dateorder(time_base::no_order)
{ _Init_timeinfo(_M_timeinfo); }

time_init<wchar_t>::time_init(const char* __name) {
  if (!__name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _Locale_time *__time = __acquire_time(__name, buf, 0, &__err_code);
  if (!__time)
    locale::_M_throw_on_creation_failure(__err_code, __name, "time");

  _Init_timeinfo(this->_M_timeinfo, __time);
  _M_dateorder = __get_date_order(__time);
  __release_time(__time);
}

time_init<wchar_t>::time_init(_Locale_time *__time) {
  _Init_timeinfo(this->_M_timeinfo, __time);
  _M_dateorder = __get_date_order(__time);
}
#endif

_STLP_MOVE_TO_STD_NAMESPACE

#if !defined (_STLP_NO_FORCE_INSTANTIATE)
template class time_get<char, istreambuf_iterator<char, char_traits<char> > >;
template class time_put<char, ostreambuf_iterator<char, char_traits<char> > >;

#  ifndef _STLP_NO_WCHAR_T
template class time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
template class time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >;
#  endif

#endif

_STLP_END_NAMESPACE
