
#include <math.h>

#ifdef _MSC_VER
#pragma function(sinhf)
#endif

_Check_return_
float
__cdecl
sinhf(
    _In_ float x)
{
    return (float)sinh((double)x);
}
