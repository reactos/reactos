#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbVPrintfExW _StringCbVPrintfExW
#include <strsafe.h>

#undef StringCbVPrintfExW
HRESULT __stdcall
StringCbVPrintfExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags, 
    STRSAFE_LPCWSTR pszFormat,
    va_list args)
{
    /* Use the inlined version */
    return _StringCbVPrintfExW(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
}
