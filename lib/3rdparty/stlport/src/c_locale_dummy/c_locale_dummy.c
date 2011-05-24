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

/* This is a "stub" implementation of the "c_locale.h" interface,
   intended for operating systems where we have not yet written
   a real implementation.  A C++ library using this stub implementation
   is still standard-conforming, since the C++ standard does not require
   that any locales other than "C" be supported.
*/

#include <string.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>
#include <limits.h>

#if defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
#  define _STLP_STRNCPY(D, DS, S, C) strncpy_s(D, DS, S, C)
#  if !defined (_STLP_NO_WCHAR_T)
#    define _STLP_WCSNCPY(D, DS, S, C) wcsncpy_s(D, DS, S, C)
#  endif
#else
#  define _STLP_STRNCPY(D, DS, S, C) strncpy(D, S, C)
#  if !defined (_STLP_NO_WCHAR_T)
#    define _STLP_WCSNCPY(D, DS, S, C) wcsncpy(D, S, C)
#  endif
#endif

static const char *_C_name = "C";
static const char *_empty_str = "";
#ifndef _STLP_NO_WCHAR_T
static const wchar_t *_empty_wstr = L"";
#endif

static _Locale_mask_t ctable[256];

/* Framework functions */

void _Locale_init(void) {
  /* Ctype table for the ASCII character set. */
  char c;
  /* We might never reach 128 when char is signed. */
  for (c = 0; /* c != 128 */; ++c) {
    if (isalpha(c)) ctable[(unsigned char)c] |= _Locale_ALPHA;
    if (iscntrl(c)) ctable[(unsigned char)c] |= _Locale_CNTRL;
    if (isdigit(c)) ctable[(unsigned char)c] |= _Locale_DIGIT;
    if (isprint(c)) ctable[(unsigned char)c] |= _Locale_PRINT;
    if (ispunct(c)) ctable[(unsigned char)c] |= _Locale_PUNCT;
    if (isspace(c)) ctable[(unsigned char)c] |= _Locale_SPACE;
    if (isxdigit(c)) ctable[(unsigned char)c] |= _Locale_XDIGIT;
    if (isupper(c)) ctable[(unsigned char)c] |= _Locale_UPPER;
    if (islower(c)) ctable[(unsigned char)c] |= _Locale_LOWER;
    if (c == 127) break;
  }

  /* ASCII is a 7-bit code, so everything else is non-ASCII. */
  memset(&(ctable[128]), 0, 128 * sizeof(_Locale_mask_t));
}

void _Locale_final(void)
{}

void* _Locale_create(const char* name, int *__err_code) {
  if (name[0] == 'C' && name[1] == 0)
  { return (void*)0x1; }
  *__err_code = _STLP_LOC_NO_PLATFORM_SUPPORT; return 0;
}

struct _Locale_ctype* _Locale_ctype_create(const char *name,
                                           struct _Locale_name_hint* hint, int *__err_code)
{ return (struct _Locale_ctype*)_Locale_create(name, __err_code); }

struct _Locale_codecvt* _Locale_codecvt_create(const char *name,
                                               struct _Locale_name_hint* hint, int *__err_code)
{ return (struct _Locale_codecvt*)_Locale_create(name, __err_code); }

struct _Locale_numeric* _Locale_numeric_create(const char *name,
                                               struct _Locale_name_hint* hint, int *__err_code)
{ return (struct _Locale_numeric*)_Locale_create(name, __err_code); }

struct _Locale_time* _Locale_time_create(const char *name,
                                         struct _Locale_name_hint* hint, int *__err_code)
{ return (struct _Locale_time*)_Locale_create(name, __err_code); }

struct _Locale_collate* _Locale_collate_create(const char *name,
                                               struct _Locale_name_hint* hint, int *__err_code)
{ return (struct _Locale_collate*)_Locale_create(name, __err_code); }

struct _Locale_monetary* _Locale_monetary_create(const char *name,
                                                 struct _Locale_name_hint* hint, int *__err_code)
{ return (struct _Locale_monetary*)_Locale_create(name, __err_code); }

struct _Locale_messages* _Locale_messages_create(const char *name,
                                                 struct _Locale_name_hint* hint, int *__err_code)
{ return (struct _Locale_messages*)_Locale_create(name, __err_code); }

const char *_Locale_ctype_default(char* buf)    { return _C_name; }
const char *_Locale_numeric_default(char * buf) { return _C_name; }
const char *_Locale_time_default(char* buf)     { return _C_name; }
const char *_Locale_collate_default(char* buf)  { return _C_name; }
const char *_Locale_monetary_default(char* buf) { return _C_name; }
const char *_Locale_messages_default(char* buf) { return _C_name; }

char const* _Locale_ctype_name(const struct _Locale_ctype *lctype, char* buf)
{ return _C_name; }

char const* _Locale_codecvt_name(const struct _Locale_codecvt *lcodecvt, char* buf)
{ return _C_name; }

char const* _Locale_numeric_name(const struct _Locale_numeric *lnum, char* buf)
{ return _C_name; }

char const* _Locale_time_name(const struct _Locale_time *ltime, char* buf)
{ return _C_name; }

char const* _Locale_collate_name(const struct _Locale_collate *lcol, char* buf)
{ return _C_name; }

char const* _Locale_monetary_name(const struct _Locale_monetary *lmon, char* buf)
{ return _C_name; }

char const* _Locale_messages_name(const struct _Locale_messages *lmes, char* buf)
{ return _C_name; }

void _Locale_ctype_destroy(struct _Locale_ctype *lctype)     {}
void _Locale_codecvt_destroy(struct _Locale_codecvt *lcodecvt)   {}
void _Locale_numeric_destroy(struct _Locale_numeric *lnum)   {}
void _Locale_time_destroy(struct _Locale_time *ltime)        {}
void _Locale_collate_destroy(struct _Locale_collate *lcol)   {}
void _Locale_monetary_destroy(struct _Locale_monetary *lmon) {}
void _Locale_messages_destroy(struct _Locale_messages *lmes) {}

static char const* _Locale_extract_name(const char* name, int *__err_code) {
  // When the request is the default locale or the "C" locale we answer "C".
  if (name[0] == 0 ||
      (name[0] == 'C' && name[1] == 0))
  {  return _C_name; }
  *__err_code = _STLP_LOC_NO_PLATFORM_SUPPORT; return 0;
}

char const* _Locale_extract_ctype_name(const char *name, char *buf,
                                       struct _Locale_name_hint* hint, int *__err_code)
{ return _Locale_extract_name(name, __err_code); }

char const* _Locale_extract_numeric_name(const char *name, char *buf,
                                         struct _Locale_name_hint* hint, int *__err_code)
{ return _Locale_extract_name(name, __err_code); }

char const* _Locale_extract_time_name(const char*name, char *buf,
                                      struct _Locale_name_hint* hint, int *__err_code)
{ return _Locale_extract_name(name, __err_code); }

char const* _Locale_extract_collate_name(const char *name, char *buf,
                                         struct _Locale_name_hint* hint, int *__err_code)
{ return _Locale_extract_name(name, __err_code); }

char const* _Locale_extract_monetary_name(const char *name, char *buf,
                                          struct _Locale_name_hint* hint, int *__err_code)
{ return _Locale_extract_name(name, __err_code); }

char const* _Locale_extract_messages_name(const char *name, char *buf,
                                          struct _Locale_name_hint* hint, int *__err_code)
{ return _Locale_extract_name(name, __err_code); }

struct _Locale_name_hint* _Locale_get_ctype_hint(struct _Locale_ctype* ctype)
{ return 0; }
struct _Locale_name_hint* _Locale_get_numeric_hint(struct _Locale_numeric* numeric)
{ return 0; }
struct _Locale_name_hint* _Locale_get_time_hint(struct _Locale_time* time)
{ return 0; }
struct _Locale_name_hint* _Locale_get_collate_hint(struct _Locale_collate* collate)
{ return 0; }
struct _Locale_name_hint* _Locale_get_monetary_hint(struct _Locale_monetary* monetary)
{ return 0; }
struct _Locale_name_hint* _Locale_get_messages_hint(struct _Locale_messages* messages)
{ return 0; }

/* ctype */
const _Locale_mask_t* _Locale_ctype_table(struct _Locale_ctype* lctype) {
  _STLP_MARK_PARAMETER_AS_UNUSED(lctype)
  return ctable;
}

int _Locale_toupper(struct _Locale_ctype*lctype, int c)
{ return toupper(c); }

int _Locale_tolower(struct _Locale_ctype*lctype, int c)
{ return tolower(c); }

#ifndef _STLP_NO_WCHAR_T
_Locale_mask_t _WLocale_ctype(struct _Locale_ctype *lctype, wint_t wc, _Locale_mask_t mask) {
  _Locale_mask_t ret = 0;
  if ((mask & _Locale_ALPHA) != 0 && iswalpha(wc))
    ret |= _Locale_ALPHA;
  
  if ((mask & _Locale_CNTRL) != 0 && iswcntrl(wc))
    ret |= _Locale_CNTRL;

  if ((mask & _Locale_DIGIT) != 0 && iswdigit(wc))
    ret |= _Locale_DIGIT;

  if ((mask & _Locale_PRINT) != 0 && iswprint(wc)) 
    ret |= _Locale_PRINT;

  if ((mask & _Locale_PUNCT) != 0 && iswpunct(wc))
    ret |= _Locale_PUNCT;

  if ((mask & _Locale_SPACE) != 0 && iswspace(wc))
    ret |= _Locale_SPACE;

  if ((mask & _Locale_XDIGIT) != 0 && iswxdigit(wc))
    ret |= _Locale_XDIGIT;

  if ((mask & _Locale_UPPER) != 0 && iswupper(wc))
    ret |= _Locale_UPPER;

  if ((mask & _Locale_LOWER) != 0 && iswlower(wc))
    ret |= _Locale_LOWER;

  return ret;
}

wint_t _WLocale_tolower(struct _Locale_ctype *lctype, wint_t wc)
{ return towlower(wc); }

wint_t _WLocale_toupper(struct _Locale_ctype *lctype, wint_t wc)
{ return towupper(wc); }

int _WLocale_mb_cur_max (struct _Locale_codecvt *lcodecvt) { return 1; }
int _WLocale_mb_cur_min (struct _Locale_codecvt *lcodecvt) { return 1; }
int _WLocale_is_stateless (struct _Locale_codecvt *lcodecvt) { return 1; }

size_t _WLocale_mbtowc(struct _Locale_codecvt *lcodecvt,
                       wchar_t *to,
                       const char *from, size_t n,
                       mbstate_t *st)
{ *to = *from; return 1; }

size_t _WLocale_wctomb(struct _Locale_codecvt *lcodecvt,
                       char *to, size_t n,
                       const wchar_t c,
                       mbstate_t *st)
{ *to = (char)c; return 1; }

size_t _WLocale_unshift(struct _Locale_codecvt *lcodecvt,
                        mbstate_t *st,
                        char *buf, size_t n, char ** next)
{ *next = buf; return 0; }
#endif

/* Collate */
 int _Locale_strcmp(struct _Locale_collate* lcol,
                    const char* s1, size_t n1, const char* s2, size_t n2) {
  int ret = 0;
  char buf1[64], buf2[64];
  while (n1 > 0 || n2 > 0) {
    size_t bufsize1 = n1 < 63 ? n1 : 63;
    size_t bufsize2 = n2 < 63 ? n2 : 63;
    _STLP_STRNCPY(buf1, 64, s1, bufsize1); buf1[bufsize1] = 0;
    _STLP_STRNCPY(buf2, 64, s2, bufsize2); buf2[bufsize2] = 0;

    ret = strcmp(buf1, buf2);
    if (ret != 0) return ret < 0 ? -1 : 1;
    s1 += bufsize1; n1 -= bufsize1;
    s2 += bufsize2; n2 -= bufsize2;
  }
  return ret == 0 ? 0 : (ret < 0 ? -1 : 1);
}

#ifndef _STLP_NO_WCHAR_T

int _WLocale_strcmp(struct _Locale_collate* lcol,
                    const wchar_t* s1, size_t n1, const wchar_t* s2, size_t n2) {
  int ret = 0;
  wchar_t buf1[64], buf2[64];
  while (n1 > 0 || n2 > 0) {
    size_t bufsize1 = n1 < 63 ? n1 : 63;
    size_t bufsize2 = n2 < 63 ? n2 : 63;
    _STLP_WCSNCPY(buf1, 64, s1, bufsize1); buf1[bufsize1] = 0;
    _STLP_WCSNCPY(buf2, 64, s2, bufsize2); buf2[bufsize2] = 0;

    ret = wcscmp(buf1, buf2);
    if (ret != 0) return ret < 0 ? -1 : 1;
    s1 += bufsize1; n1 -= bufsize1;
    s2 += bufsize2; n2 -= bufsize2;
  }
  return ret == 0 ? 0 : (ret < 0 ? -1 : 1);
}

#endif

size_t _Locale_strxfrm(struct _Locale_collate* lcol,
                       char* dest, size_t dest_n,
                       const char* src, size_t src_n) {
  if (dest != 0) {
    _STLP_STRNCPY(dest, dest_n, src, dest_n - 1); dest[dest_n - 1] = 0;
  }
  return src_n;
}

#ifndef _STLP_NO_WCHAR_T

size_t _WLocale_strxfrm(struct _Locale_collate* lcol,
                        wchar_t* dest, size_t dest_n,
                        const wchar_t* src, size_t src_n) {
  if (dest != 0) {
    _STLP_WCSNCPY(dest, dest_n, src, dest_n - 1); dest[dest_n - 1] = 0;
  }
  return src_n;
}

#endif

/* Numeric */

char _Locale_decimal_point(struct _Locale_numeric* lnum)
{ return '.'; }
char _Locale_thousands_sep(struct _Locale_numeric* lnum)
{ return ','; }
const char* _Locale_grouping(struct _Locale_numeric * lnum)
{ return _empty_str; }
const char * _Locale_true(struct _Locale_numeric * lnum)
{ return "true"; }
const char * _Locale_false(struct _Locale_numeric * lnum)
{ return "false"; }

#ifndef _STLP_NO_WCHAR_T
wchar_t _WLocale_decimal_point(struct _Locale_numeric* lnum)
{ return L'.'; }
wchar_t _WLocale_thousands_sep(struct _Locale_numeric* lnum)
{ return L','; }
const wchar_t * _WLocale_true(struct _Locale_numeric* lnum, wchar_t* buf, size_t bufSize)
{ return L"true"; }
const wchar_t * _WLocale_false(struct _Locale_numeric* lnum, wchar_t* buf, size_t bufSize)
{ return L"false"; }
#endif

/* Monetary */

const char* _Locale_int_curr_symbol(struct _Locale_monetary * lmon)
{ return _empty_str; }
const char* _Locale_currency_symbol(struct _Locale_monetary * lmon)
{ return _empty_str; }
char        _Locale_mon_decimal_point(struct _Locale_monetary * lmon)
{ return '.'; }
char        _Locale_mon_thousands_sep(struct _Locale_monetary * lmon)
{ return ','; }
const char* _Locale_mon_grouping(struct _Locale_monetary * lmon)
{ return _empty_str; }
const char* _Locale_positive_sign(struct _Locale_monetary * lmon)
{ return _empty_str; }
const char* _Locale_negative_sign(struct _Locale_monetary * lmon)
{ return _empty_str; }
char        _Locale_int_frac_digits(struct _Locale_monetary * lmon)
{ return 0; }
char        _Locale_frac_digits(struct _Locale_monetary * lmon)
{ return 0; }
int         _Locale_p_cs_precedes(struct _Locale_monetary * lmon)
{ return CHAR_MAX; }
int         _Locale_p_sep_by_space(struct _Locale_monetary * lmon)
{ return CHAR_MAX; }
int         _Locale_p_sign_posn(struct _Locale_monetary * lmon)
{ return CHAR_MAX; }
int         _Locale_n_cs_precedes(struct _Locale_monetary * lmon)
{ return CHAR_MAX; }
int          _Locale_n_sep_by_space(struct _Locale_monetary * lmon)
{ return CHAR_MAX; }
int          _Locale_n_sign_posn(struct _Locale_monetary * lmon)
{ return CHAR_MAX; }

#ifndef _STLP_NO_WCHAR_T
const wchar_t* _WLocale_int_curr_symbol(struct _Locale_monetary * lmon,
                                        wchar_t* buf, size_t bufSize)
{ return _empty_wstr; }
const wchar_t* _WLocale_currency_symbol(struct _Locale_monetary * lmon,
                                        wchar_t* buf, size_t bufSize)
{ return _empty_wstr; }
wchar_t        _WLocale_mon_decimal_point(struct _Locale_monetary * lmon)
{ return L'.'; }
wchar_t        _WLocale_mon_thousands_sep(struct _Locale_monetary * lmon)
{ return L','; }
const wchar_t* _WLocale_positive_sign(struct _Locale_monetary * lmon,
                                      wchar_t* buf, size_t bufSize)
{ return _empty_wstr; }
const wchar_t* _WLocale_negative_sign(struct _Locale_monetary * lmon,
                                      wchar_t* buf, size_t bufSize)
{ return _empty_wstr; }
#endif

/* Time */
static const char* full_monthname[] =
{ "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December" };
const char * _Locale_full_monthname(struct _Locale_time * ltime, int n)
{ return full_monthname[n]; }

static const char* abbrev_monthname[] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
const char * _Locale_abbrev_monthname(struct _Locale_time * ltime, int n)
{ return abbrev_monthname[n]; }

static const char* full_dayname[] =
{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char * _Locale_full_dayofweek(struct _Locale_time * ltime, int n)
{ return full_dayname[n]; }

static const char* abbrev_dayname[] =
{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char * _Locale_abbrev_dayofweek(struct _Locale_time * ltime, int n)
{ return abbrev_dayname[n]; }

const char* _Locale_d_t_fmt(struct _Locale_time* ltime)
{ return "%m/%d/%y"; }
const char* _Locale_d_fmt(struct _Locale_time* ltime)
{ return "%m/%d/%y"; }
const char* _Locale_t_fmt(struct _Locale_time* ltime)
{ return "%H:%M:%S"; }
const char* _Locale_long_d_t_fmt(struct _Locale_time* ltime)
{ return _empty_str; }
const char* _Locale_long_d_fmt(struct _Locale_time* ltime)
{ return _empty_str; }
const char* _Locale_am_str(struct _Locale_time* ltime)
{ return "AM"; }
const char* _Locale_pm_str(struct _Locale_time* ltime)
{ return "PM"; }

#ifndef _STLP_NO_WCHAR_T
static const wchar_t* full_wmonthname[] =
{ L"January", L"February", L"March", L"April", L"May", L"June",
  L"July", L"August", L"September", L"October", L"November", L"December" };
const wchar_t * _WLocale_full_monthname(struct _Locale_time * ltime, int n,
                                        wchar_t* buf, size_t bufSize)
{ return full_wmonthname[n]; }

static const wchar_t* abbrev_wmonthname[] =
{ L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
  L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
const wchar_t * _WLocale_abbrev_monthname(struct _Locale_time * ltime, int n,
                                          wchar_t* buf, size_t bufSize)
{ return abbrev_wmonthname[n]; }

static const wchar_t* full_wdayname[] =
{ L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday" };
const wchar_t * _WLocale_full_dayofweek(struct _Locale_time * ltime, int n,
                                        wchar_t* buf, size_t bufSize)
{ return full_wdayname[n]; }

static const wchar_t* abbrev_wdayname[] =
{ L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
const wchar_t * _WLocale_abbrev_dayofweek(struct _Locale_time * ltime, int n,
                                          wchar_t* buf, size_t bufSize)
{ return abbrev_wdayname[n]; }

const wchar_t* _WLocale_am_str(struct _Locale_time* ltime,
                               wchar_t* buf, size_t bufSize)
{ return L"AM"; }
const wchar_t* _WLocale_pm_str(struct _Locale_time* ltime,
                               wchar_t* buf, size_t bufSize)
{ return L"PM"; }
#endif

/* Messages */

nl_catd_type _Locale_catopen(struct _Locale_messages* lmes, const char* name)
{ return -1; }
void _Locale_catclose(struct _Locale_messages* lmes, nl_catd_type cat) {}
const char* _Locale_catgets(struct _Locale_messages* lmes, nl_catd_type cat,
                            int setid, int msgid, const char *dfault)
{ return dfault; }
