
#include <math.h>

_Check_return_
float
_chgsignf(_In_ float x)
{
    return (float)_chgsign((double)x);
}

