
#include <math.h>

#ifdef _MSC_VER
#pragma function(atanf)
#endif

_Check_return_
float
__cdecl
atanf(
    _In_ float x)
{
    return (float)atan((double)x);
}
