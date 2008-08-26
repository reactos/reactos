#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbVPrintfW _StringCbVPrintfW
#include <strsafe.h>

#undef StringCbVPrintfW
HRESULT __stdcall
StringCbVPrintfW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszFormat,
    va_list args)
{
    /* Use the inlined version */
    return _StringCbVPrintfW(pszDest, cbDest, pszFormat, args);
}
