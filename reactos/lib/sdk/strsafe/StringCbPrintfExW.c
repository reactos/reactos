#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbPrintfExW _StringCbPrintfExW
#include <strsafe.h>

#undef StringCbPrintfExW
HRESULT __stdcall
StringCbPrintfExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags,
    STRSAFE_LPCWSTR pszFormat,
    ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCbVPrintfExW(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
    va_end(args);
    return result;
}
