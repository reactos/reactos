
#include <math.h>

#if (_MSC_VER >= 1920)
#pragma function(_hypotf)
#endif

_Check_return_
float
__cdecl
_hypotf(
    _In_ float x,
    _In_ float y)
{
    return (float)_hypot((double)x, (double)y);
}
