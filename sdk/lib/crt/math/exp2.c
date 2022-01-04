
#include <math.h>

_Check_return_
double
__cdecl
exp2(
    _In_ double x)
{
    /* This below avoids clang to optimize our pow call to exp2 */
    static const double TWO = 2.0;
    return pow(TWO, x);
}
