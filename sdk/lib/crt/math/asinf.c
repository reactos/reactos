
#include <math.h>

#ifdef _MSC_VER
#pragma function(asinf)
#endif

_Check_return_
float
__cdecl
asinf(
    _In_ float x)
{
    return (float)asin((double)x);
}
