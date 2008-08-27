#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchLengthW _StringCchLengthW
#include <strsafe.h>

#undef StringCchLengthW
HRESULT __stdcall
StringCchLengthW(
    STRSAFE_LPCWSTR psz,
    size_t cbMax,
    size_t *pcb)
{
    /* Use the inlined version */
    return _StringCchLengthW(psz, cbMax, pcb);
}
