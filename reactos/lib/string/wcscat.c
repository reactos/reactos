/*
 * $Id: wcscat.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

wchar_t* wcscat(wchar_t* s, const wchar_t* append)
{
    wchar_t* save = s;

    for (; *s; ++s);
        while ((*s++ = *append++));
    return save;
}
