#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchLengthA _StringCchLengthA
#include <strsafe.h>

#undef StringCchLengthA
HRESULT __stdcall
StringCchLengthA(
    STRSAFE_LPCSTR psz,
    size_t cbMax,
    size_t *pcb)
{
    /* Use the inlined version */
    return _StringCchLengthA(psz, cbMax, pcb);
}
