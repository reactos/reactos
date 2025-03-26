
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

double _sincos_special(double x, char *name, unsigned int operation)
{
    UT64 xu;
	unsigned int is_snan;

	xu.f64 = x;

    if((xu.u64 & INF_POS_64) == INF_POS_64)
    {
        // x is Inf or NaN
        if((xu.u64 & MANTISSA_MASK_64) == 0x0)
        {
            // x is Inf
			xu.u64 = IND_64;
			_handle_error(name, operation, xu.u64, _DOMAIN, AMD_F_INVALID, EDOM, x, 0, 1);
		}
		else 
		{
			// x is NaN
            is_snan = (((xu.u64 & QNAN_MASK_64) == QNAN_MASK_64) ? 0 : 1);
			if(is_snan)
			{
				xu.u64 |= QNAN_MASK_64;
			}
			_handle_error(name, operation, xu.u64, _DOMAIN, 0, EDOM, x, 0, 1);
		}
	}

	return xu.f64;
}

float _sincosf_special(float x, char *name, unsigned int operation)
{
    UT64 xu;
	unsigned int is_snan;

	xu.u64    = 0;
	xu.f32[0] = x;

    if((xu.u32[0] & INF_POS_32) == INF_POS_32)
    {
        // x is Inf or NaN
        if((xu.u32[0] & MANTISSA_MASK_32) == 0x0)
        {
            // x is Inf	
			xu.u32[0] = IND_32; 
			_handle_errorf(name, operation, xu.u64, _DOMAIN, AMD_F_INVALID, EDOM, x, 0, 1);
		}
		else
		{
			// x is NaN
            is_snan = (((xu.u32[0] & QNAN_MASK_32) == QNAN_MASK_32) ? 0 : 1);
			if(is_snan) 
			{
				xu.u32[0] |= QNAN_SET_32;
				_handle_errorf(name, operation, xu.u64, _DOMAIN, AMD_F_INVALID, EDOM, x, 0, 1);
			}
			else 
			{
				_handle_errorf(name, operation, xu.u64, _DOMAIN, 0, EDOM, x, 0, 1);
			}
		}
	}

	return xu.f32[0];
}

float _sinf_special(float x)
{
	return _sincosf_special(x, "sinf", _FpCodeSin);
}

double _sin_special(double x)
{
	return _sincos_special(x, "sin", _FpCodeSin);
}

float _cosf_special(float x)
{
	return _sincosf_special(x, "cosf", _FpCodeCos);
}

double _cos_special(double x)
{
	return _sincos_special(x, "cos", _FpCodeCos);
}

double _tan_special(double x)
{
        return  _sincos_special(x, "tan",_FpCodeTan);
}

float _tanf_special(float x)
{
        return  _sincosf_special(x, "tanf",_FpCodeTan);
}
