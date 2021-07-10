
#include <math.h>

#ifdef _MSC_VER
#pragma function(tanf)
#endif

_Check_return_
float
__cdecl
tanf(
    _In_ float x)
{
    return (float)tan((double)x);
}
