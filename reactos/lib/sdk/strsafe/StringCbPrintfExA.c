#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbPrintfExA _StringCbPrintfExA
#include <strsafe.h>

#undef StringCbPrintfExA
HRESULT __stdcall
StringCbPrintfExA(
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
    result = StringCbVPrintfExA(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
    va_end(args);
    return result;
}
