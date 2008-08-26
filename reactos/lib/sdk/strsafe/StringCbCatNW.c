#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCatNW _StringCbCatNW
#include <strsafe.h>

#undef StringCbCatNW
HRESULT __stdcall
StringCbCatNW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    size_t cbMaxAppend)
{
    /* Use the inlined version */
    return _StringCbCatNW(pszDest, cbDest, pszSrc, cbMaxAppend);
}
