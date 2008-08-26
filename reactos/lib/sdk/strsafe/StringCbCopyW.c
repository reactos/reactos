#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCopyW _StringCbCopyW
#include <strsafe.h>

#undef StringCbCopyW
HRESULT __stdcall
StringCbCopyW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc)
{
    /* Use the inlined version */
    return _StringCbCopyW(pszDest, cbDest, pszSrc);
}
