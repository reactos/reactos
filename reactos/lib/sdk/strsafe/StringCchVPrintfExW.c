#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchVPrintfExW _StringCchVPrintfExW
#include <strsafe.h>

#undef StringCchVPrintfExW
HRESULT __stdcall
StringCchVPrintfExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags, 
    STRSAFE_LPCWSTR pszFormat,
    va_list args)
{
    /* Use the inlined version */
    return _StringCchVPrintfExW(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
}
