#include <string.h>

int iswctype(wint_t wc, wctype_t wctypeFlags);

/*
 * obsolete
 *
 * @implemented
 */
int is_wctype(wint_t wc, wctype_t wctypeFlags)
{
   return iswctype(wc, wctypeFlags);
}
