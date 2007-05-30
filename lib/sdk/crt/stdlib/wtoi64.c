/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @implemented
 */
__int64 _wtoi64(const wchar_t* nptr)
{
    wchar_t* s = (wchar_t*)nptr;
    __int64 acc = 0;
    int neg = 0;

    while (iswspace((int)*s))
        s++;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    else if (*s == '+')
        s++;

    while (iswdigit((int)*s)) {
        acc = 10 * acc + ((int)*s - '0');
        s++;
    }

    if (neg)
        acc *= -1;
    return acc;
}
