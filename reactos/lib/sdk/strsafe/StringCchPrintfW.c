#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchPrintfW _StringCchPrintfW
#include <strsafe.h>

#undef StringCchPrintfW
HRESULT __stdcall
StringCchPrintfW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszFormat,
    ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCchVPrintfW(pszDest, cbDest, pszFormat, args);
    va_end(args);
    return result;
}
