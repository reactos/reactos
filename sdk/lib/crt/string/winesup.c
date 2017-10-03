
#include "precomp.h"
#include "winesup.h"

#ifdef _LIBCNT_

static struct lconv _LIBCNT_lconv =
{
    ".", // char* decimal_point;
    ",", // char* thousands_sep;
    " ", // char* grouping;
    "$", // char* int_curr_symbol;
    "$", // char* currency_symbol;
    ".", // char* mon_decimal_point;
    "?", // char* mon_thousands_sep;
    "/", // char* mon_grouping;
    "+", // char* positive_sign;
    "-", // char* negative_sign;
    4, // char int_frac_digits;
    4, // char frac_digits;
    4, // char p_cs_precedes;
    1, // char p_sep_by_space;
    0, // char n_cs_precedes;
    1, // char n_sep_by_space;
    1, // char p_sign_posn;
    1, // char n_sign_posn;
};

threadlocinfo _LIBCNT_locinfo =
{
    2, // LONG refcount;
    0, // CP_ACP, // unsigned int lc_codepage;
    0, // unsigned int lc_collate_cp;
    {0}, // unsigned long lc_handle[6];
    {{0}}, // LC_ID lc_id[6];

    // struct {
    // char *locale;
    // wchar_t *wlocale;
    // int *refcount;
    // int *wrefcount;
    // } lc_category[6];
    {{0}},

    0, // int lc_clike;
    2, // int mb_cur_max;
    0, // int *lconv_intl_refcount;
    0, // int *lconv_num_refcount;
    0, // int *lconv_mon_refcount;
    &_LIBCNT_lconv, // struct MSVCRT_lconv *lconv;
    0, // int *ctype1_refcount;
    0, // unsigned short *ctype1;
    0, // const unsigned short *pctype;
    0, // unsigned char *pclmap;
    0, // unsigned char *pcumap;
    0, // struct __lc_time_data *lc_time_curr;
};

#define get_locinfo() (&_LIBCNT_locinfo)

#endif

#define _SET_NUMBER_(type) *va_arg((*ap), type*) = negative ? -cur : cur

void
__declspec(noinline)
_internal_handle_float(
    int negative,
    int exp,
    int suppress,
    ULONGLONG d,
    int l_or_L_prefix,
    va_list *ap)
{
    long double cur = 1, expcnt = 10;
    unsigned fpcontrol;
    BOOL negexp;
#ifdef _M_ARM
    DbgBreakPoint();
    fpcontrol = _controlfp(0, 0);
#else
    fpcontrol = _control87(0, 0);
    _control87(_EM_DENORMAL|_EM_INVALID|_EM_ZERODIVIDE
            |_EM_OVERFLOW|_EM_UNDERFLOW|_EM_INEXACT, 0xffffffff);
#endif
    negexp = (exp < 0);
    if(negexp)
        exp = -exp;
    /* update 'cur' with this exponent. */
    while(exp) {
        if(exp & 1)
            cur *= expcnt;
        exp /= 2;
        expcnt = expcnt*expcnt;
    }
    cur = (negexp ? d/cur : d*cur);

#ifdef _M_ARM
    DbgBreakPoint();
    _controlfp(fpcontrol, 0xffffffff);
#else
    _control87(fpcontrol, 0xffffffff);
#endif

    if (!suppress) {
        if (l_or_L_prefix) _SET_NUMBER_(double);
        else _SET_NUMBER_(float);
    }
}
