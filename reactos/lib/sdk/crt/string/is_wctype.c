#include <string.h>

extern const unsigned short wine_wctype_table[];

/*
 * obsolete
 *
 * @implemented
 */
int is_wctype(wint_t wc, wctype_t wctypeFlags)
{
   return iswctype(wc, wctypeFlags);
}
