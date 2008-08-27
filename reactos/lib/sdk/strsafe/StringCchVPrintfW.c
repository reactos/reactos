#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchVPrintfW _StringCchVPrintfW
#include <strsafe.h>

#undef StringCchVPrintfW
HRESULT __stdcall
StringCchVPrintfW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszFormat,
    va_list args)
{
    /* Use the inlined version */
    return _StringCchVPrintfW(pszDest, cbDest, pszFormat, args);
}
