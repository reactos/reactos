#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbPrintfW _StringCbPrintfW
#include <strsafe.h>

#undef StringCbPrintfW
HRESULT __stdcall
StringCbPrintfW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszFormat,
    ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCbVPrintfW(pszDest, cbDest, pszFormat, args);
    va_end(args);
    return result;
}
