#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCatNW _StringCchCatNW
#include <strsafe.h>

#undef StringCchCatNW
HRESULT __stdcall
StringCchCatNW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    size_t cbMaxAppend)
{
    /* Use the inlined version */
    return _StringCchCatNW(pszDest, cbDest, pszSrc, cbMaxAppend);
}
