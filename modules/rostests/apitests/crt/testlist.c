#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

#if defined(TEST_MSVCRT)
extern void func__vscprintf(void);
extern void func__vscwprintf(void);
extern void func_atexit(void);
#endif
#if defined(TEST_STATIC_CRT) || defined(TEST_MSVCRT)
#if defined(_M_ARM)
extern void func___rt_div(void);
extern void func___fto64(void);
extern void func___64tof(void);
#endif
#endif
#if defined(TEST_NTDLL)
extern void func__vscwprintf(void);
#endif
extern void func_fputc(void);
extern void func_fputwc(void);
extern void func__snprintf(void);
extern void func__snwprintf(void);
extern void func__vsnprintf(void);
extern void func__vsnwprintf(void);
extern void func_mbstowcs(void);
extern void func_mbtowc(void);
extern void func_sprintf(void);
extern void func_strcpy(void);
extern void func_strlen(void);
extern void func_strnlen(void);
extern void func_strtoul(void);
extern void func_wcsnlen(void);
extern void func_wcstombs(void);
extern void func_wcstoul(void);
extern void func_wctomb(void);
extern void func___getmainargs(void);

extern void func_static_construct(void);
extern void func_static_init(void);
extern void func_crtdata(void);

const struct test winetest_testlist[] =
{
    { "_vsnprintf", func__vsnprintf },
    { "_vsnwprintf", func__vsnwprintf },
    { "mbstowcs", func_mbstowcs },
    { "mbtowc", func_mbtowc },
    { "_snprintf", func__snprintf },
    { "_snwprintf", func__snwprintf },
    { "sprintf", func_sprintf },
    { "strcpy", func_strcpy },
    { "strlen", func_strlen },
    { "strtoul", func_strtoul },
    { "wcstoul", func_wcstoul },
    { "wctomb", func_wctomb },
    { "wcstombs", func_wcstombs },
#if defined(TEST_CRTDLL) || defined(TEST_MSVCRT) || defined(TEST_STATIC_CRT)
    // ...
#endif
#if defined(TEST_STATIC_CRT) || defined(TEST_MSVCRT)
#if defined(_M_ARM)
    { "__rt_div", func___rt_div },
    { "__fto64", func___fto64 },
    { "__64tof", func___64tof },
#endif
#endif
#if defined(TEST_STATIC_CRT)
#elif defined(TEST_MSVCRT)
    { "atexit", func_atexit },
    { "crtdata", func_crtdata },
#if defined(_M_IX86)
    { "__getmainargs", func___getmainargs },
#endif
    { "_vscprintf", func__vscprintf },
    { "_vscwprintf", func__vscwprintf },

    { "static_construct", func_static_construct },
    { "static_init", func_static_init },
#elif defined(TEST_NTDLL)
    { "_vscwprintf", func__vscwprintf },
#elif defined(TEST_CRTDLL)
#endif
    { 0, 0 }
};

