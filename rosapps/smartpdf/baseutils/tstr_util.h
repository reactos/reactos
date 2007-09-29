/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef TSTR_UTIL_H_
#define TSTR_UTIL_H_

#ifdef _UNICODE
  #include "wstr_util.h"
  #define tstr_len      wcslen
  #define tstr_dup      wstr_dup
  #define tstr_dupn     wstr_dupn
  #define tstr_cat      wstr_cat
  #define tstr_cat3     wstr_cat3
  #define tstr_cat4     wstr_cat4
  #define tstr_copy     wstr_copy
  #define tstr_copyn    wstr_copyn
  #define tstr_startswith wstr_startswith
  #define tstr_startswithi wstr_startswithi
  #define tstr_url_encode wstr_url_encode
  #define tchar_needs_url_escape wchar_needs_url_escape
  #define tstr_contains wstr_contains
  #define tstr_printf   wstr_printf
  #define tstr_ieq      wstr_ieq
  #define tstr_empty    wstr_empty
#else
  #include "str_util.h"
  #define tstr_len      strlen
  #define tstr_dup      str_dup
  #define tstr_dupn     str_dupn
  #define tstr_cat      str_cat
  #define tstr_cat3     str_cat3
  #define tstr_cat4     str_cat4
  #define tstr_copy     str_copy
  #define tstr_copyn    str_copyn
  #define tstr_startswith str_startswith
  #define tstr_startswithi str_startswithi
  #define tstr_url_encode str_url_encode
  #define tchar_needs_url_escape char_needs_url_escape
  #define tstr_contains str_contains
  #define tstr_printf   str_printf
  #define tstr_ieq      str_ieq
  #define tstr_empty    str_empty
#endif

#endif
