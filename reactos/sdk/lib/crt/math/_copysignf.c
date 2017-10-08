
#include <math.h>

_Check_return_
float
__cdecl
_copysignf(
    _In_ float x,
    _In_ float y)
{
    return (float)_copysign((double)x, (double)y);
}
