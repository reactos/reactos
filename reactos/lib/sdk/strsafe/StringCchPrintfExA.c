#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchPrintfExA _StringCchPrintfExA
#include <strsafe.h>

#undef StringCchPrintfExA
HRESULT __stdcall
StringCchPrintfExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags,
    STRSAFE_LPCSTR pszFormat,
    ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCchVPrintfExA(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
    va_end(args);
    return result;
}
