
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

// y = log10f(x)
// y = log10(x)
// y = logf(x)
// y = log(x)

// these codes and the ones in the related .asm files have to match
#define LOG_X_ZERO      1
#define LOG_X_NEG       2
#define LOG_X_NAN       3

static float _logf_special_common(float x, float y, U32 code, unsigned int op, char *name)
{
    switch(code)
    {
    case LOG_X_ZERO:
        {
            UT64 ym; ym.u64 = 0; ym.f32[0] = y;
            _handle_errorf(name, op, ym.u64, _SING, AMD_F_DIVBYZERO, ERANGE, x, 0.0, 1);
        }
        break;

    case LOG_X_NEG:
        {
            UT64 ym; ym.u64 = 0; ym.f32[0] = y;
            _handle_errorf(name, op, ym.u64, _DOMAIN, AMD_F_INVALID, EDOM, x, 0.0, 1);
        }
        break;

    case LOG_X_NAN:
        {
            unsigned int is_snan;
            UT32 xm; UT64 ym;
            xm.f32 = x;
            is_snan = (((xm.u32 & QNAN_MASK_32) == QNAN_SET_32) ? 0 : 1);
            ym.u64 = 0; ym.f32[0] = y;

            if(is_snan)
            {
                _handle_errorf(name, op, ym.u64, _DOMAIN, AMD_F_INVALID, EDOM, x, 0.0, 1);
            }
            else
            {
                _handle_errorf(name, op, ym.u64, _DOMAIN, 0, EDOM, x, 0.0, 1);
            }
        }
        break;
    }

    return y;
}

float _logf_special(float x, float y, U32 code)
{
    return _logf_special_common(x, y, code, _FpCodeLog, "logf");
}

float _log10f_special(float x, float y, U32 code)
{
    return _logf_special_common(x, y, code, _FpCodeLog10, "log10f");
}

static double _log_special_common(double x, double y, U32 code, unsigned int op, char *name)
{
    switch(code)
    {
    case LOG_X_ZERO:
        {
            UT64 ym; ym.f64 = y;
            _handle_error(name, op, ym.u64, _SING, AMD_F_DIVBYZERO, ERANGE, x, 0.0, 1);
        }
        break;

    case LOG_X_NEG:
        {
            UT64 ym; ym.f64 = y;
            _handle_error(name, op, ym.u64, _DOMAIN, AMD_F_INVALID, EDOM, x, 0.0, 1);
        }
        break;

    case LOG_X_NAN:
        {
            UT64 ym; ym.f64 = y;
            _handle_error(name, op, ym.u64, _DOMAIN, 0, EDOM, x, 0.0, 1);
        }
        break;
    }

    return y;
}

double _log_special(double x, double y, U32 code)
{
    return _log_special_common(x, y, code, _FpCodeLog, "log");
}

double _log10_special(double x, double y, U32 code)
{
    return _log_special_common(x, y, code, _FpCodeLog10, "log10");
}
