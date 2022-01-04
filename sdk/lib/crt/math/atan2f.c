
#include <math.h>

#ifdef _MSC_VER
#pragma function(atan2f)
#endif

_Check_return_
float
__cdecl
atan2f(
    _In_ float x,
    _In_ float y)
{
    return (float)atan2((double)x,(double)y);
}
