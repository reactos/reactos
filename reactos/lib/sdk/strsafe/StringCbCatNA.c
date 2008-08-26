#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCatNA _StringCbCatNA
#include <strsafe.h>

#undef StringCbCatNA
HRESULT __stdcall
StringCbCatNA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    size_t cbMaxAppend)
{
    /* Use the inlined version */
    return _StringCbCatNA(pszDest, cbDest, pszSrc, cbMaxAppend);
}
