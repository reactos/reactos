/*
 * PROJECT:    .inf file parser
 * LICENSE:    GPL - See COPYING in the top level directory
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "inflib.h"
#include "infhost.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

INT isspaceW(WCHAR c)
{
    return iswspace(c);
}

INT strlenW(PCWSTR s)
{
    return wcslen(s);
}

PWSTR strcpyW(PWSTR d, PCWSTR s)
{
    return wcscpy(d, s);
}

PWSTR strncpyW(PWSTR d, PCWSTR s, SIZE_T c)
{
    return wcsncpy(d, s, c);
}

INT strcmpiW(PCWSTR s1, PCWSTR s2)
{
    return wcsicmp(s1, s2);
}

LONG strtolW(PCWSTR s, PWSTR *e, INT r)
{
    return wcstol(s, e, r);
}

ULONG strtoulW(PCWSTR s, PWSTR *e, INT r)
{
    return wcstoul(s, e, r);
}
