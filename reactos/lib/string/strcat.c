/*
 * $Id: strcat.c,v 1.1 2003/05/27 18:56:15 hbirr Exp $
 */

#include <string.h>

char* strcat(char* s, const char* append)
{
    char* save = s;

    for (; *s; ++s);
        while ((*s++ = *append++));
    return save;
}
