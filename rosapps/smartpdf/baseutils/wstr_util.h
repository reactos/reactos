/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef WSTR_UTIL_H_
#define WSTR_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define wstrlen wcslen
int     wstr_ieq(const WCHAR *str1, const WCHAR *str2);
int     wstr_startswith(const WCHAR *str, const WCHAR *txt);
int     wstr_startswithi(const WCHAR *str, const WCHAR *txt);
int     wstr_empty(const WCHAR *str);
int     wstr_copy(WCHAR *dst, int dst_cch_size, const WCHAR *src);
int     wstr_copyn(WCHAR *dst, int dst_cch_size, const WCHAR *src, int src_cch_size);
WCHAR * wstr_dup(const WCHAR *str);
WCHAR * wstr_dupn(const WCHAR *str, int str_len_cch);
WCHAR * wstr_cat(const WCHAR *str1, const WCHAR *str2);
WCHAR * wstr_cat3(const WCHAR *str1, const WCHAR *str2, const WCHAR *str3);
WCHAR * wstr_cat4(const WCHAR *str1, const WCHAR *str2, const WCHAR *str3, const WCHAR *str4);
WCHAR * wstr_url_encode(const WCHAR *str);
WCHAR   wchar_needs_url_escape(WCHAR c);
int     wstr_contains(const WCHAR *str, WCHAR c);
WCHAR * wstr_printf(const WCHAR *format, ...);

#ifdef DEBUG
void wstr_util_test(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
