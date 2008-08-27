#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCatNA _StringCchCatNA
#include <strsafe.h>

#undef StringCchCatNA
HRESULT __stdcall
StringCbCatNA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    size_t cbMaxAppend)
{
    /* Use the inlined version */
    return _StringCchCatNA(pszDest, cbDest, pszSrc, cbMaxAppend);
}
