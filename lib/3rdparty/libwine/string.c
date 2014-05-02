#include <ctype.h>
#include "wine/config.h"
#include "wine/port.h"

#ifndef HAVE_STRCASECMP

#ifdef _stricmp
# undef _stricmp
#endif

int _stricmp( const char *str1, const char *str2 )
{
    const unsigned char *ustr1 = (const unsigned char *)str1;
    const unsigned char *ustr2 = (const unsigned char *)str2;

    while (*ustr1 && toupper(*ustr1) == toupper(*ustr2)) {
        ustr1++;
        ustr2++;
    }
    return toupper(*ustr1) - toupper(*ustr2);
}
#endif
