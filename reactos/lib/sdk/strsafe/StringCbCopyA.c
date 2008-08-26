#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCopyA _StringCbCopyA
#include <strsafe.h>

#undef StringCbCopyA
HRESULT __stdcall
StringCbCopyA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc)
{
    /* Use the inlined version */
    return _StringCbCopyA(pszDest, cbDest, pszSrc);
}
