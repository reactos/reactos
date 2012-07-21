#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

#if defined(TEST_MSVCRT)
extern void func__vscprintf(void);
extern void func__vscwprintf(void);
#endif
#if defined(TEST_NTDLL)
extern void func__vscwprintf(void);
#endif
extern void func__vsnprintf(void);
extern void func__vsnwprintf(void);
extern void func_sprintf(void);
extern void func_strcpy(void);

const struct test winetest_testlist[] =
{
    { "_vsnprintf", func__vsnprintf },
    { "_vsnwprintf", func__vsnwprintf },
    { "sprintf", func_sprintf },
    { "strcpy", func_strcpy },
#if defined(TEST_CRTDLL) || defined(TEST_MSVCRT) || defined(TEST_STATIC_CRT)
    // ...
#endif
#if defined(TEST_STATIC_CRT) || defined(TEST_MSVCRT)
    // ...
#endif
#if defined(TEST_STATIC_CRT)
#elif defined(TEST_MSVCRT)
    { "_vscprintf", func__vscprintf },
    { "_vscwprintf", func__vscwprintf },
#elif defined(TEST_NTDLL)
    { "_vscwprintf", func__vscwprintf },
#elif defined(TEST_CRTDLL)
#endif
    { 0, 0 }
};

