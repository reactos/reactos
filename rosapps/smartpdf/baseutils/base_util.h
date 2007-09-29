/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#ifndef BASE_UTIL_H_
#define BASE_UTIL_H_

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

/* It seems that Visual C defines WIN32 for Windows code but _WINDOWS for WINCE projects,
   so I'll make sure to set WIN32 always*/
#ifdef _WINDOWS
 #ifndef WIN32
  #define WIN32 1
 #endif
 #ifndef _WIN32
  #define _WIN32 1
 #endif
#endif

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#else
#include <sys/time.h> // for timeval
#endif

/* Few most common includes for C stdlib */
#include <assert.h>
#include <stdio.h>

#include <wchar.h>
#include <string.h>

#include <stdlib.h>
#include <malloc.h>

// TODO: does it need to be __GNUC__ only? I think it was done for mingw
#ifdef __GNUC__
#include <stdarg.h>
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifdef _WIN32
  #ifndef __GNUC__
  typedef unsigned int uint32_t;
  #endif
  #ifndef _T
    #define _T TEXT
  #endif
#else
  #define _T(x) x
  /* TODO: if _UNICODE, it should be different */
  #define TEXT(x) x
#endif

/* compile-time assert */
#ifndef CASSERT
  #define CASSERT( exp, name ) typedef int dummy##name [ (exp ) ? 1 : -1 ];
#endif

/* Ugly name but the whole point is to make things shorter. */
#define SA(struct_name) (struct_name *)malloc(sizeof(struct_name))
#define SAZ(struct_name) (struct_name *)zmalloc(sizeof(struct_name))

typedef long long int64;

CASSERT( sizeof(int64)==8, int64_is_8_bytes )

#define dimof(X)    (sizeof(X)/sizeof((X)[0]))

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ms_timer {
#ifdef _WIN32
    LARGE_INTEGER   start;
    LARGE_INTEGER   end;
#else
    struct timeval  start;
    struct timeval  end;
#endif
} ms_timer;

#ifdef _WIN32
  void    win32_dbg_out(const char *format, ...);
  void    win32_dbg_out_hex(const char *dsc, const unsigned char *data, int dataLen);
#endif

/* TODO: consider using standard C macros for SWAP and MIN */
void        swap_int(int *one, int *two);
void        swap_double(double *one, double *two);
int         MinInt(int one, int two);

void        memzero(void *data, size_t len);
void *      zmalloc(size_t size);

void        sleep_milliseconds(int milliseconds);

void        ms_timer_start(ms_timer *timer);
void        ms_timer_stop(ms_timer *timer);
double      ms_timer_time_in_ms(ms_timer *timer);

#define LIST_REVERSE_FUNC_PROTO(func_name, TYPE) \
void func_name(TYPE **root)

#define LIST_REVERSE_FUNC(func_name, TYPE) \
void func_name(TYPE **root) \
{ \
    TYPE * cur; \
    TYPE * next; \
    TYPE * new_first = NULL; \
\
    if (!root) \
        return; \
\
    cur = *root; \
    while (cur) { \
        next = cur->next; \
        cur->next = new_first; \
        new_first = cur; \
        cur = next; \
    } \
    *root = new_first; \
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class MsTimer {
public:
    MsTimer() { ms_timer_start(&timer); }
    void start(void) { ms_timer_start(&timer); }
    void stop(void) { ms_timer_stop(&timer); }
    double timeInMs(void) { return ms_timer_time_in_ms(&timer); }
private:
    ms_timer timer;
};
#endif

#endif
