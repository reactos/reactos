#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCatA _StringCbCatA
#include <strsafe.h>

#undef StringCbCatA
HRESULT __stdcall
StringCbCatA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc)
{
    /* Use the inlined version */
    return _StringCbCatA(pszDest, cbDest, pszSrc);
}
