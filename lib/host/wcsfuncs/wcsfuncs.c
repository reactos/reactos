/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       lib/host/wcsfuncs/wcsfuncs.c
  PURPOSE:    Reimplemented wide-character string functions for host tools (to be independent of the host wchar_t size)
  COPYRIGHT:  Copyright 2008 Colin Finck <mail@colinfinck.de>
*/

#include <host/typedefs.h>

/* Function implementations */
SIZE_T utf16_wcslen(PCWSTR str)
{
    SIZE_T i;

    for(i = 0; str[i]; i++);

    return i;
}

PWSTR utf16_wcschr(PWSTR str, WCHAR c)
{
    SIZE_T i;

    for(i = 0; str[i] && str[i] != c; i++);

    if(str[i])
        return &str[i];
    else
        return NULL;
}

INT utf16_wcsncmp(PCWSTR string1, PCWSTR string2, size_t count)
{
    while(count--)
    {
        if(*string1 != *string2)
            return 1;

        if(*string1 == 0)
            return 0;

        string1++;
        string2++;
    }

    return 0;
}
