/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/string.h>

#pragma function(strcat)

char* strcat(char* s, const char* append)
{
    char* save = s;

    for (; *s; ++s);
        while ((*s++ = *append++));
    return save;
}
