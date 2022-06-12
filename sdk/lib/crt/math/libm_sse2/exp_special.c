
/*******************************************************************************
MIT License
-----------

Copyright (c) 2002-2019 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this Software and associated documentaon files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*******************************************************************************/

#include <fpieee.h>
#include <excpt.h>
#include <float.h>
#include <math.h>
#include <errno.h>

#include "libm_new.h"

// y = expf(x)
// y = exp(x)

// these codes and the ones in the related .asm files have to match
#define EXP_X_NAN       1
#define EXP_Y_ZERO      2
#define EXP_Y_INF       3

float _expf_special(float x, float y, U32 code)
{
    switch(code)
    {
    case EXP_X_NAN:
        {
            UT64 ym; ym.u64 = 0; ym.f32[0] = y;
            _handle_errorf("expf", _FpCodeExp, ym.u64, _DOMAIN, 0, EDOM, x, 0.0, 1);
        }
        break;

    case EXP_Y_ZERO:
        {
            UT64 ym; ym.u64 = 0; ym.f32[0] = y;
            _handle_errorf("expf", _FpCodeExp, ym.u64, _UNDERFLOW, AMD_F_INEXACT|AMD_F_UNDERFLOW, ERANGE, x, 0.0, 1);
        }
        break;

    case EXP_Y_INF:
        {
            UT64 ym; ym.u64 = 0; ym.f32[0] = y;
            _handle_errorf("expf", _FpCodeExp, ym.u64, _OVERFLOW, AMD_F_INEXACT|AMD_F_OVERFLOW, ERANGE, x, 0.0, 1);

        }
        break;
    }

    return y;
}

double _exp_special(double x, double y, U32 code)
{
    switch(code)
    {
    case EXP_X_NAN:
        {
            UT64 ym; ym.f64 = y;
            _handle_error("exp", _FpCodeExp, ym.u64, _DOMAIN, 0, EDOM, x, 0.0, 1);
        }
        break;

    case EXP_Y_ZERO:
        {
            UT64 ym; ym.f64 = y;
            _handle_error("exp", _FpCodeExp, ym.u64, _UNDERFLOW, AMD_F_INEXACT|AMD_F_UNDERFLOW, ERANGE, x, 0.0, 1);
        }
        break;

    case EXP_Y_INF:
        {
            UT64 ym; ym.f64 = y;
            _handle_error("exp", _FpCodeExp, ym.u64, _OVERFLOW, AMD_F_INEXACT|AMD_F_OVERFLOW, ERANGE, x, 0.0, 1);
        }
        break;
    }


    return y;
}
