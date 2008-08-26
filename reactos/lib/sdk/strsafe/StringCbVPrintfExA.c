#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbVPrintfExA _StringCbVPrintfExA
#include <strsafe.h>

#undef StringCbVPrintfExA
HRESULT __stdcall
StringCbVPrintfExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags, 
    STRSAFE_LPCSTR pszFormat,
    va_list args)
{
    /* Use the inlined version */
    return _StringCbVPrintfExA(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
}
