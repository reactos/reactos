#include <string.h>
#include <stdlib.h>
#include <windows.h>

/*
 * @implemented
 */
long
_wtol(const wchar_t *str)
{
     ULONG RunningTotal = 0;
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


