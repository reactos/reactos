
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

// these codes and the ones in the related .asm files have to match
#define POW_X_ONE_Y_SNAN            1
#define POW_X_ZERO_Z_INF            2
#define POW_X_NAN                   3
#define POW_Y_NAN                   4
#define POW_X_NAN_Y_NAN             5
#define POW_X_NEG_Y_NOTINT          6
#define POW_Z_ZERO                  7
#define POW_Z_DENORMAL              8
#define POW_Z_INF                   9

float _powf_special(float x, float y, float z, U32 code)
{
    switch(code)
    {
    case POW_X_ONE_Y_SNAN:
        {
            UT64 zm; zm.u64 = 0; zm.f32[0] = z;
            _handle_errorf("powf", _FpCodePow, zm.u64, 0, AMD_F_INVALID, 0, x, y, 2);
        }
        break;

    case POW_X_ZERO_Z_INF:
        {
            UT64 zm; zm.u64 = 0; zm.f32[0] = z;
            _handle_errorf("powf", _FpCodePow, zm.u64, _SING, AMD_F_DIVBYZERO, ERANGE, x, y, 2);
        }
        break;

    case POW_X_NAN:
    case POW_Y_NAN:
    case POW_X_NAN_Y_NAN:   
    case POW_X_NEG_Y_NOTINT:
        {
            UT64 zm; zm.u64 = 0; zm.f32[0] = z;
            _handle_errorf("powf", _FpCodePow, zm.u64, _DOMAIN, AMD_F_INVALID, EDOM, x, y, 2);
        }
        break;

    case POW_Z_ZERO:
        {
            UT64 zm; zm.u64 = 0; zm.f32[0] = z;
            _handle_errorf("powf", _FpCodePow, zm.u64, _UNDERFLOW, AMD_F_INEXACT|AMD_F_UNDERFLOW, ERANGE, x, y, 2);
        }
        break;

    case POW_Z_INF:
        {
            UT64 zm; zm.u64 = 0; zm.f32[0] = z;
            _handle_errorf("powf", _FpCodePow, zm.u64, _OVERFLOW, AMD_F_INEXACT|AMD_F_OVERFLOW, ERANGE, x, y, 2);
        }
        break;
    }

    return z;
}

double _pow_special(double x, double y, double z, U32 code)
{
    switch(code)
    {
    case POW_X_ZERO_Z_INF:
        {
            UT64 zm; zm.f64 = z;
            _handle_error("pow", _FpCodePow, zm.u64, _SING, AMD_F_DIVBYZERO, ERANGE, x, y, 2);
        }
        break;

    case POW_X_NAN:
    case POW_Y_NAN:
    case POW_X_NAN_Y_NAN:
    case POW_X_NEG_Y_NOTINT:
        {
            UT64 zm; zm.f64 = z;
            _handle_error("pow", _FpCodePow, zm.u64, _DOMAIN, AMD_F_INVALID, EDOM, x, y, 2);
        }
        break;

    case POW_Z_ZERO:
    case POW_Z_DENORMAL:
        {
            UT64 zm; zm.f64 = z;
            _handle_error("pow", _FpCodePow, zm.u64, _UNDERFLOW, AMD_F_INEXACT|AMD_F_UNDERFLOW, ERANGE, x, y, 2);
        }
        break;

    case POW_Z_INF:
        {
            UT64 zm; zm.f64 = z;
            _handle_error("pow", _FpCodePow, zm.u64, _OVERFLOW, AMD_F_INEXACT|AMD_F_OVERFLOW, ERANGE, x, y, 2);
        }
        break;
    }

    return z;
}
