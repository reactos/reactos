// 
// Pei-Hwa Lin (peiwhal), July 17, 1997
//

#include "urltrk.h"

#ifdef unix
extern "C"
#endif /* unix */
BOOL WINAPI
IsLoggingEnabledW
(
    IN LPCWSTR  pwszUrl
)
{
    DWORD       cbSize;
    LPSTR       lpUrl = NULL;
    BOOL        bRet = FALSE;

    if (pwszUrl == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return bRet;
    }

    cbSize = lstrlenW(pwszUrl) + sizeof(WCHAR);
    lpUrl = (LPSTR)LocalAlloc(LPTR, cbSize);
    if (!lpUrl)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return bRet;
    }

    int i=WideCharToMultiByte(CP_ACP, 0, pwszUrl, -1, lpUrl,
                    cbSize, NULL, NULL);
    if (!i)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        LocalFree(lpUrl);
        return bRet;
    }

    bRet = IsLoggingEnabledA(lpUrl);
    LocalFree(lpUrl);
    return bRet;
}

