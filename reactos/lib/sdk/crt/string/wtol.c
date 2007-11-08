#include <string.h>
#include <ctype.h>
#include <basetsd.h>

/* Implementation comes from wine/dlls/ntdll/wcstring.c */

/*
 * @implemented
 */
long
_wtol(const wchar_t *str)
{
    unsigned long RunningTotal = 0;
    char bMinus = 0;

    while (iswctype(*str, _SPACE) ) {
        str++;
    } /* while */

    if (*str == L'+') {
        str++;
    } else if (*str == L'-') {
        bMinus = 1;
        str++;
    } /* if */

    while (*str >= L'0' && *str <= L'9') {
        RunningTotal = RunningTotal * 10 + *str - L'0';
        str++;
    } /* while */

    return bMinus ? -RunningTotal : RunningTotal;
}


