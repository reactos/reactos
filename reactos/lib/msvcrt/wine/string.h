/*
 * String definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_STRING_H
#define __WINE_STRING_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

#ifndef MSVCRT_NLSCMP_DEFINED
#define _NLSCMPERROR               ((unsigned int)0x7fffffff)
#define MSVCRT_NLSCMP_DEFINED
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

void*       _memccpy(void*,const void*,int,MSVCRT(size_t));
int         _memicmp(const void*,const void*,MSVCRT(size_t));
int         _strcmpi(const char*,const char*);
char*       _strdup(const char*);
char*       _strerror(const char*);
int         _stricmp(const char*,const char*);
int         _stricoll(const char*,const char*);
char*       _strlwr(char*);
int         _strnicmp(const char*,const char*,MSVCRT(size_t));
char*       _strnset(char*,int,MSVCRT(size_t));
char*       _strrev(char*);
char*       _strset(char*,int);
char*       _strupr(char*);

void*       MSVCRT(memchr)(const void*,int,MSVCRT(size_t));
int         MSVCRT(memcmp)(const void*,const void*,MSVCRT(size_t));
void*       MSVCRT(memcpy)(void*,const void*,MSVCRT(size_t));
void*       MSVCRT(memmove)(void*,const void*,MSVCRT(size_t));
void*       MSVCRT(memset)(void*,int,MSVCRT(size_t));
char*       MSVCRT(strcat)(char*,const char*);
char*       MSVCRT(strchr)(const char*,int);
int         MSVCRT(strcmp)(const char*,const char*);
int         MSVCRT(strcoll)(const char*,const char*);
char*       MSVCRT(strcpy)(char*,const char*);
MSVCRT(size_t) MSVCRT(strcspn)(const char*,const char*);
char*       MSVCRT(strerror)(int);
MSVCRT(size_t) MSVCRT(strlen)(const char*);
char*       MSVCRT(strncat)(char*,const char*,MSVCRT(size_t));
int         MSVCRT(strncmp)(const char*,const char*,MSVCRT(size_t));
char*       MSVCRT(strncpy)(char*,const char*,MSVCRT(size_t));
char*       MSVCRT(strpbrk)(const char*,const char*);
char*       MSVCRT(strrchr)(const char*,int);
MSVCRT(size_t) MSVCRT(strspn)(const char*,const char*);
char*       MSVCRT(strstr)(const char*,const char*);
char*       MSVCRT(strtok)(char*,const char*);
MSVCRT(size_t) MSVCRT(strxfrm)(char*,const char*,MSVCRT(size_t));

#ifndef MSVCRT_WSTRING_DEFINED
#define MSVCRT_WSTRING_DEFINED
MSVCRT(wchar_t)*_wcsdup(const MSVCRT(wchar_t)*);
int             _wcsicmp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int             _wcsicoll(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wcslwr(MSVCRT(wchar_t)*);
int             _wcsnicmp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*_wcsnset(MSVCRT(wchar_t)*,MSVCRT(wchar_t),MSVCRT(size_t));
MSVCRT(wchar_t)*_wcsrev(MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wcsset(MSVCRT(wchar_t)*,MSVCRT(wchar_t));
MSVCRT(wchar_t)*_wcsupr(MSVCRT(wchar_t)*);

MSVCRT(wchar_t)*MSVCRT(wcscat)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcschr)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t));
int             MSVCRT(wcscmp)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
int             MSVCRT(wcscoll)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcscpy)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcscspn)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcslen)(const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsncat)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
int             MSVCRT(wcsncmp)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*MSVCRT(wcsncpy)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
MSVCRT(wchar_t)*MSVCRT(wcspbrk)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsrchr)(const MSVCRT(wchar_t)*,MSVCRT(wchar_t) wcFor);
MSVCRT(size_t)  MSVCRT(wcsspn)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcsstr)(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*MSVCRT(wcstok)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*);
MSVCRT(size_t)  MSVCRT(wcsxfrm)(MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,MSVCRT(size_t));
#endif /* MSVCRT_WSTRING_DEFINED */

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
static inline void* memccpy(void *s1, const void *s2, int c, MSVCRT(size_t) n) { return _memccpy(s1, s2, c, n); }
static inline int memicmp(const void* s1, const void* s2, MSVCRT(size_t) len) { return _memicmp(s1, s2, len); }
static inline int strcasecmp(const char* s1, const char* s2) { return _stricmp(s1, s2); }
static inline int strcmpi(const char* s1, const char* s2) { return _strcmpi(s1, s2); }
static inline char* strdup(const char* buf) { return _strdup(buf); }
static inline int stricmp(const char* s1, const char* s2) { return _stricmp(s1, s2); }
static inline int stricoll(const char* s1, const char* s2) { return _stricoll(s1, s2); }
static inline char* strlwr(char* str) { return _strlwr(str); }
static inline int strncasecmp(const char *str1, const char *str2, size_t n) { return _strnicmp(str1, str2, n); }
static inline int strnicmp(const char* s1, const char* s2, MSVCRT(size_t) n) { return _strnicmp(s1, s2, n); }
static inline char* strnset(char* str, int value, unsigned int len) { return _strnset(str, value, len); }
static inline char* strrev(char* str) { return _strrev(str); }
static inline char* strset(char* str, int value) { return _strset(str, value); }
static inline char* strupr(char* str) { return _strupr(str); }

static inline MSVCRT(wchar_t)* wcsdup(const MSVCRT(wchar_t)* str) { return _wcsdup(str); }
static inline int wcsicoll(const MSVCRT(wchar_t)* str1, const MSVCRT(wchar_t)* str2) { return _wcsicoll(str1, str2); }
static inline MSVCRT(wchar_t)* wcslwr(MSVCRT(wchar_t)* str) { return _wcslwr(str); }
static inline int wcsnicmp(const MSVCRT(wchar_t)* str1, const MSVCRT(wchar_t)* str2, MSVCRT(size_t) n) { return _wcsnicmp(str1, str2, n); }
static inline MSVCRT(wchar_t)* wcsnset(MSVCRT(wchar_t)* str, MSVCRT(wchar_t) c, MSVCRT(size_t) n) { return _wcsnset(str, c, n); }
static inline MSVCRT(wchar_t)* wcsrev(MSVCRT(wchar_t)* str) { return _wcsrev(str); }
static inline MSVCRT(wchar_t)* wcsset(MSVCRT(wchar_t)* str, MSVCRT(wchar_t) c) { return _wcsset(str, c); }
static inline MSVCRT(wchar_t)* wcsupr(MSVCRT(wchar_t)* str) { return _wcsupr(str); }
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_STRING_H */
