
#include <math.h>

#ifdef _MSC_VER
#pragma function(log10f)
#endif

_Check_return_
float
__cdecl
log10f(
    _In_ float x)
{
    return (float)log10((double)x);
}
