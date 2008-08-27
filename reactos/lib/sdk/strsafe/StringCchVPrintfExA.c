#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchVPrintfExA _StringCchVPrintfExA
#include <strsafe.h>

#undef StringCchVPrintfExA
HRESULT __stdcall
StringCchVPrintfExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags, 
    STRSAFE_LPCSTR pszFormat,
    va_list args)
{
    /* Use the inlined version */
    return _StringCchVPrintfExA(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
}
