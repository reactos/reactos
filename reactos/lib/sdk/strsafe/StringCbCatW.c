#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCatW _StringCbCatW
#include <strsafe.h>

#undef StringCbCatW
HRESULT __stdcall
StringCbCatW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc)
{
    /* Use the inlined version */
    return _StringCbCatW(pszDest, cbDest, pszSrc);
}
