/* $Id: getname.c,v 1.1 2002/12/06 13:14:14 robd Exp $
 *
 */
#include <windows.h>


WINBOOL
STDCALL
GetComputerNameW(LPWSTR lpBuffer, LPDWORD nSize)
{
    WCHAR Name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD Size = 0;

    /*
     * FIXME: get the computer's name from the registry.
     */
    lstrcpyW(Name, L"ROSHost"); /* <-- FIXME -- */
    Size = lstrlenW(Name) + 1;
    if (Size > *nSize) {
        *nSize = Size;
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }
    lstrcpyW(lpBuffer, Name);
    return TRUE;
}


WINBOOL
STDCALL
GetComputerNameA(LPSTR lpBuffer, LPDWORD nSize)
{
    WCHAR Name[MAX_COMPUTERNAME_LENGTH + 1];
    int i;

    if (FALSE == GetComputerNameW(Name, nSize)) {
        return FALSE;
    }

/* FIXME --> */
/* Use UNICODE to ANSI */
    for (i = 0; Name[i]; ++i) {
        lpBuffer[i] = (CHAR)Name[i];
    }
    lpBuffer[i] = '\0';
/* FIXME <-- */

    return TRUE;
}
