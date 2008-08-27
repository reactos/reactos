#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchGetsW _StringCchGetsW
#include <strsafe.h>

#undef StringCchGetsW
HRESULT __stdcall
StringCchGetsW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest)
{
    /* Use the inlined version */
    return _StringCchGetsW(pszDest, cbDest);
}
