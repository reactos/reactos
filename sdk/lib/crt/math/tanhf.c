
#include <math.h>

#ifdef _MSC_VER
#pragma function(tanhf)
#endif

_Check_return_
float
__cdecl
tanhf(
    _In_ float x)
{
    return (float)tanh((double)x);
}
