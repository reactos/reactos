
#define STANDALONE
#include <apitest.h>

extern void func__mbsncmp(void);
extern void func__mbsstr(void);
#if defined(_M_ARM)
extern void func___rt_div(void);
extern void func___fto64(void);
extern void func___64tof(void);
#endif
extern void func_ceil(void);
extern void func_fabs(void);
extern void func_floor(void);
extern void func_fpcontrol(void);
extern void func_fputc(void);
extern void func_fputwc(void);
extern void func_setjmp(void);
extern void func__snprintf(void);
extern void func__snwprintf(void);
extern void func__vsnprintf(void);
extern void func__vsnwprintf(void);
extern void func_mbstowcs(void);
extern void func_mbtowc(void);
extern void func_rand_s(void);
extern void func_sprintf(void);
extern void func_strcpy(void);
extern void func_strlen(void);
extern void func_strnlen(void);
extern void func_strtoul(void);
extern void func_system(void);
extern void func_wcsnlen(void);
extern void func_wcstombs(void);
extern void func_wcstoul(void);
extern void func_wctomb(void);
extern void func__wsystem(void);
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
    { "setjmp", func_setjmp },
    { "_snprintf", func__snprintf },
    { "_snwprintf", func__snwprintf },
    { "sprintf", func_sprintf },
    { "strcpy", func_strcpy },
    { "strlen", func_strlen },
    { "strtoul", func_strtoul },
    { "wcstoul", func_wcstoul },
    { "wctomb", func_wctomb },
    { "wcstombs", func_wcstombs },
    { "ceil", func_ceil },
    { "fabs", func_fabs },
    { "floor", func_floor },
    { "rand_s", func_rand_s },
#ifdef _M_AMD64 // x86 / arm need fixing
    { "fpcontrol", func_fpcontrol },
#endif
#if defined(_M_ARM)
    { "__rt_div", func___rt_div },
    { "__fto64", func___fto64 },
    { "__64tof", func___64tof },
#endif

    { 0, 0 }
};

