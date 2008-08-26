#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbGetsW _StringCbGetsW
#include <strsafe.h>

#undef StringCbGetsW
HRESULT __stdcall
StringCbGetsW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest)
{
    /* Use the inlined version */
    return _StringCbGetsW(pszDest, cbDest);
}
