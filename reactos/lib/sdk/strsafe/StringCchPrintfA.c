#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchPrintfA _StringCchPrintfA
#include <strsafe.h>

#undef StringCchPrintfA
HRESULT __stdcall
StringCchPrintfA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszFormat,
    ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCchVPrintfA(pszDest, cbDest, pszFormat, args);
    va_end(args);
    return result;
}
