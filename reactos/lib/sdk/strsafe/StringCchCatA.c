#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCatA _StringCchCatA
#include <strsafe.h>

#undef StringCchCatA
HRESULT __stdcall
StringCchCatA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc)
{
    /* Use the inlined version */
    return _StringCchCatA(pszDest, cbDest, pszSrc);
}
