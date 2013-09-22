#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

#if defined(TEST_MSVCRT)
extern void func_mbstowcs_s(void);
extern void func_wcstombs_s(void);
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
    { "mbstowcs_s", func_mbstowcs_s },
    { "wcstombs_s", func_wcstombs_s },
    { "_vscprintf", func__vscprintf },
    { "_vscwprintf", func__vscwprintf },
#elif defined(TEST_NTDLL)
    { "_vscwprintf", func__vscwprintf },
#elif defined(TEST_CRTDLL)
#endif
    { 0, 0 }
};

