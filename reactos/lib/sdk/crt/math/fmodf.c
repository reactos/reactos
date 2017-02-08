
#include <math.h>

_Check_return_
float
__cdecl
fmodf(
    _In_ float x,
    _In_ float y)
{
    return (float)fmod((double)x,(double)y);
}
