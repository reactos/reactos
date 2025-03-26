
#include <math.h>

#ifdef _MSC_VER
#pragma function(ceilf)
#endif

_Check_return_
float
__cdecl
ceilf(
    _In_ float x)
{
    return (float)ceil((double)x);
}
