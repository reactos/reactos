
#include <math.h>

#ifdef _MSC_VER
#pragma function(coshf)
#endif

_Check_return_
float
__cdecl
coshf(
    _In_ float x)
{
    return (float)cosh((double)x);
}
