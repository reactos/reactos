#include <string.h>

extern const unsigned short wine_wctype_table[];

/*
 * @implemented
 */
int __cdecl iswctype(wint_t wc, wctype_t wctypeFlags)
{
    return (wine_wctype_table[wine_wctype_table[wc >> 8] + (wc & 0xff)] & wctypeFlags);
}

