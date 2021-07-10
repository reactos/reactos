
#include <math.h>

#ifdef _MSC_VER
#pragma function(acosf)
#endif

_Check_return_
float
__cdecl
acosf(
    _In_ float x)
{
    return (float)acos((double)x);
}
