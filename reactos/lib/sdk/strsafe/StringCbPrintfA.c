#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbPrintfA _StringCbPrintfA
#include <strsafe.h>

#undef StringCbPrintfA
HRESULT __stdcall
StringCbPrintfA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszFormat,
    ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCbVPrintfA(pszDest, cbDest, pszFormat, args);
    va_end(args);
    return result;
}
