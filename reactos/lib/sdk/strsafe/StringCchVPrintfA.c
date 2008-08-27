#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchVPrintfA _StringCchVPrintfA
#include <strsafe.h>

#undef StringCchVPrintfA
HRESULT __stdcall
StringCchVPrintfA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszFormat,
    va_list args)
{
    /* Use the inlined version */
    return _StringCchVPrintfA(pszDest, cbDest, pszFormat, args);
}
