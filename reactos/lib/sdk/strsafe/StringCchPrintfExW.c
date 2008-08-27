#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchPrintfExW _StringCchPrintfExW
#include <strsafe.h>

#undef StringCchPrintfExW
HRESULT __stdcall
StringCchPrintfExW(
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
    result = StringCchVPrintfExW(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
    va_end(args);
    return result;
}
