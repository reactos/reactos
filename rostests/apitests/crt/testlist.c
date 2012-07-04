#define WIN32_LEAN_AND_MEAN
#define __ROS_LONG64__
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_sprintf(void);
extern void func_strcpy(void);

const struct test winetest_testlist[] =
{
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
#elif defined(TEST_NTDLL)
#elif defined(TEST_CRTDLL)
#endif
    { 0, 0 }
};

