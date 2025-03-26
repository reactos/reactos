
#include <math.h>

#ifdef _MSC_VER
#pragma function(floorf)
#endif

_Check_return_
float
__cdecl
floorf(
    _In_ float x)
{
    return (float)floor((double)x);
}
