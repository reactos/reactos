#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCopyA _StringCchCopyA
#include <strsafe.h>

#undef StringCchCopyA
HRESULT __stdcall
StringCbCopyA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc)
{
    /* Use the inlined version */
    return _StringCchCopyA(pszDest, cbDest, pszSrc);
}
