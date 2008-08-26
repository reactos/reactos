#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbLengthW _StringCbLengthW
#include <strsafe.h>

#undef StringCbLengthW
HRESULT __stdcall
StringCbLengthW(
    STRSAFE_LPCWSTR psz,
    size_t cbMax,
    size_t *pcb)
{
    /* Use the inlined version */
    return _StringCbLengthW(psz, cbMax, pcb);
}
