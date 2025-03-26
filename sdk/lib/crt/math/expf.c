
#include <math.h>

#ifdef _MSC_VER
#pragma function(expf)
#endif

_Check_return_
float
__cdecl
expf(
    _In_ float x)
{
    return (float)exp((double)x);
}
