
#include <math.h>

_Check_return_
float
__cdecl
exp2f(
    _In_ float x)
{
    /* This below avoids clang to optimize our pow call to exp2 */
    static const float TWO = 2.0f;
    return powf(TWO, x);
}
