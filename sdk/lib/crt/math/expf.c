
#include <math.h>

_Check_return_
float
__cdecl
expf(
    _In_ float x)
{
    return (float)exp((double)x);
}
