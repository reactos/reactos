#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbLengthA _StringCbLengthA
#include <strsafe.h>

#undef StringCbLengthA
HRESULT __stdcall
StringCbLengthA(
    STRSAFE_LPCSTR psz,
    size_t cbMax,
    size_t *pcb)
{
    /* Use the inlined version */
    return _StringCbLengthA(psz, cbMax, pcb);
}
