#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchGetsA _StringCchGetsA
#include <strsafe.h>

#undef StringCchGetsA
HRESULT __stdcall
StringCchGetsA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest)
{
    /* Use the inlined version */
    return _StringCchGetsA(pszDest, cbDest);
}
