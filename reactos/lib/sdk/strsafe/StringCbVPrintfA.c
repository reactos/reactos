#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbVPrintfA _StringCbVPrintfA
#include <strsafe.h>

#undef StringCbVPrintfA
HRESULT __stdcall
StringCbVPrintfA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszFormat,
    va_list args)
{
    /* Use the inlined version */
    return _StringCbVPrintfA(pszDest, cbDest, pszFormat, args);
}
