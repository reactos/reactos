
#define STANDALONE
#include <apitest.h>

extern void func__mbsncmp(void);
extern void func__mbsstr(void);
extern void func__snprintf(void);
extern void func__snwprintf(void);
extern void func__vsnprintf(void);
extern void func__vsnwprintf(void);
extern void func_mbstowcs(void);
extern void func_mbtowc(void);
extern void func_setjmp(void);
extern void func_sprintf(void);
extern void func_strcpy(void);
extern void func_strlen(void);
extern void func_strtoul(void);
extern void func_system(void);
extern void func_wcstombs(void);
extern void func_wcstoul(void);
extern void func_wctomb(void);

const struct test winetest_testlist[] =
{
    { "_mbsncmp", func__mbsncmp },
    { "_mbsstr", func__mbsstr },
    { "_snprintf", func__snprintf },
    { "_snwprintf", func__snwprintf },
    { "_vsnprintf", func__vsnprintf },
    { "_vsnwprintf", func__vsnwprintf },
    { "mbstowcs", func_mbstowcs },
    { "mbtowc", func_mbtowc },
    { "setjmp", func_setjmp },
    { "sprintf", func_sprintf },
    { "strcpy", func_strcpy },
    { "strlen", func_strlen },
    { "strtoul", func_strtoul },
    { "system", func_system },
    { "wcstoul", func_wcstoul },
    { "wctomb", func_wctomb },
    { "wcstombs", func_wcstombs },

    { 0, 0 }
};

