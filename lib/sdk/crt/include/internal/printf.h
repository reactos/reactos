#ifndef _CRT_PRINTF_H
#define _CRT_PRINTF_H


/* Implementation of a printf appropriated from Linux kernel */

int lnx_sprintf(char *str, const char *fmt, ...);
int lnx__vsprintf(char *str, const char *fmt, va_list ap);
int lnx__vswprintf(wchar_t *str, const wchar_t *fmt, va_list ap);
int lnx__vsnprintf(char *str, size_t maxlen, const char *fmt, va_list ap);
int lnx__vsnwprintf(wchar_t *str, size_t maxlen, const char *fmt, va_list ap);
int lnx_vfprintf(FILE* f, const char* fmt, va_list ap);
int lnx_vfwprintf(FILE *f, const wchar_t *fmt, va_list ap);
#ifdef _UNICODE
#define lnx_vftprintf lnx_vfwprintf
#define lnx__vstprintf lnx__vswprintf
#else
#define lnx_vftprintf lnx_vfprintf
#define lnx__vstprintf lnx__vsprintf
#endif

#endif /* _CRT_PRINTF_H */
