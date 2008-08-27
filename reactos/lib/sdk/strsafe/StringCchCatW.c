#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCatW _StringCchCatW
#include <strsafe.h>

#undef StringCchCatW
HRESULT __stdcall
StringCchCatW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc)
{
    /* Use the inlined version */
    return _StringCchCatW(pszDest, cbDest, pszSrc);
}
