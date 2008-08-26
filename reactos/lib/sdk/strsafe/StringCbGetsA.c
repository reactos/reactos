#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbGetsA _StringCbGetsA
#include <strsafe.h>

#undef StringCbGetsA
HRESULT __stdcall
StringCbGetsA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest)
{
    /* Use the inlined version */
    return _StringCbGetsA(pszDest, cbDest);
}
