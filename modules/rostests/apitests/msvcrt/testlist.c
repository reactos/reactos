#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func__finite(void);
extern void func__isnan(void);
extern void func__mbsncmp(void);
extern void func__mbsstr(void);
extern void func__snprintf(void);
extern void func__snwprintf(void);
extern void func__vscprintf(void);
extern void func__vscwprintf(void);
extern void func__vsnprintf(void);
extern void func__vsnwprintf(void);
extern void func__wsystem(void);
extern void func_acos(void);
extern void func_asin(void);
extern void func_atan(void);
extern void func_atexit(void);
extern void func_ceil(void);
extern void func_cos(void);
extern void func_crtdata(void);
extern void func_exp(void);
extern void func_fabs(void);
extern void func_floor(void);
extern void func_fpcontrol(void);
extern void func_log(void);
extern void func_log10(void);
extern void func_mbstowcs(void);
extern void func_mbtowc(void);
extern void func_rand_s(void);
extern void func_setjmp(void);
extern void func_sin(void);
extern void func_sqrt(void);
extern void func_sprintf(void);
extern void func_static_construct(void);
extern void func_static_init(void);
extern void func_strcpy(void);
extern void func_strlen(void);
extern void func_strtoul(void);
extern void func_system(void);
extern void func_tan(void);
extern void func_wcstombs(void);
extern void func_wcstoul(void);
extern void func_wctomb(void);
#if defined(_M_IX86)
extern void func___getmainargs(void);
#elif defined(_M_ARM)
extern void func___rt_div(void);
extern void func___fto64(void);
extern void func___64tof(void);
#endif

extern void func_CommandLine(void);
extern void func_ieee(void);
extern void func_popen(void);
extern void func_splitpath(void);

const struct test winetest_testlist[] =
{
    { "_finite", func__finite },
    { "_isnan", func__isnan },
    { "_mbsncmp", func__mbsncmp },
    { "_mbsstr", func__mbsstr },
    { "_snprintf", func__snprintf },
    { "_snwprintf", func__snwprintf },
    { "_vscprintf", func__vscprintf },
    { "_vscwprintf", func__vscwprintf },
    { "_vsnprintf", func__vsnprintf },
    { "_vsnwprintf", func__vsnwprintf },
    { "_wsystem", func__wsystem },
    { "acos", func_acos },
    { "asin", func_asin },
    { "atan", func_atan },
    { "atexit", func_atexit },
    { "ceil", func_ceil },
    { "cos", func_cos },
    { "crtdata", func_crtdata },
    { "exp", func_exp },
    { "fabs", func_fabs },
    { "floor", func_floor },
    { "log", func_log },
    { "log10", func_log10 },
    { "mbstowcs", func_mbstowcs },
    { "mbtowc", func_mbtowc },
    { "rand_s", func_rand_s },
    { "setjmp", func_setjmp },
    { "sin", func_sin },
    { "sqrt", func_sqrt },
    { "sprintf", func_sprintf },
    { "static_construct", func_static_construct },
    { "static_init", func_static_init },
    { "strcpy", func_strcpy },
    { "strlen", func_strlen },
    { "strtoul", func_strtoul },
    { "system", func_system },
    { "tan", func_tan },
    { "wcstombs", func_wcstombs },
    { "wcstoul", func_wcstoul },
    { "wctomb", func_wctomb },

#if defined(_M_IX86)
    { "__getmainargs", func___getmainargs },
#elif defined(_M_AMD64)
    { "fpcontrol", func_fpcontrol },  // x86 / arm need fixing
#elif defined(_M_ARM)
    { "__rt_div", func___rt_div },
    { "__fto64", func___fto64 },
    { "__64tof", func___64tof },
#endif

    { "CommandLine", func_CommandLine },
    { "ieee", func_ieee },
    { "popen", func_popen },
    { "splitpath", func_splitpath },

    { 0, 0 }
};
