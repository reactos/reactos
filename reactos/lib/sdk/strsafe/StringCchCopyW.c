#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCopyW _StringCchCopyW
#include <strsafe.h>

#undef StringCchCopyW
HRESULT __stdcall
StringCchCopyW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc)
{
    /* Use the inlined version */
    return _StringCchCopyW(pszDest, cbDest, pszSrc);
}
