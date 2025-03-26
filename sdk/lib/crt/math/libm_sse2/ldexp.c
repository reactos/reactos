/*
 * Copyright (C) 2008-2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "libm.h"
#include "libm_util.h"
#include "libm_new.h"

#define FN_PROTOTYPE_REF FN_PROTOTYPE
#define __amd_handle_error _handle_error
#define __amd_ldexp OP_LDEXP

//#include "fn_macros.h"
//#include "libm_util.h"
//#include "libm_special.h"

double FN_PROTOTYPE_REF(ldexp)(double x, int n)
{
    UT64 val,val_x;
    unsigned int sign;
    int exponent;
    val.f64 = x;
	val_x.f64 = x;
    sign = val.u32[1] & 0x80000000;
    val.u32[1] = val.u32[1] & 0x7fffffff; /* remove the sign bit */

    if (val.u64 > 0x7ff0000000000000)     /* x is NaN */
        #ifdef WINDOWS
        return __amd_handle_error("ldexp", __amd_ldexp, val_x.u64|0x0008000000000000, _DOMAIN, 0, EDOM, x, n, 2);
        #else
        {
        if(!(val.u64 & 0x0008000000000000))// x is snan
           return __amd_handle_error("ldexp", __amd_ldexp, val_x.u64|0x0008000000000000, _DOMAIN, AMD_F_INVALID, EDOM, x, n, 2);
        else
           return x;
	    }
        #endif

    if(val.u64 == 0x7ff0000000000000)/* x = +-inf*/
        return x;

    if((val.u64 == 0x0000000000000000) || (n==0))
        return x; /* x= +-0 or n= 0*/

    exponent = val.u32[1] >> 20; /* get the exponent */

    if(exponent == 0)/*x is denormal*/
    {
		val.f64 = val.f64 * VAL_2PMULTIPLIER_DP;/*multiply by 2^53 to bring it to the normal range*/
        exponent = val.u32[1] >> 20; /* get the exponent */
		exponent = exponent + n - MULTIPLIER_DP;
		if(exponent < -MULTIPLIER_DP)/*underflow*/
		{
			val.u32[1] = sign | 0x00000000;
			val.u32[0] = 0x00000000;
            return __amd_handle_error("ldexp", __amd_ldexp, val.u64, _UNDERFLOW, AMD_F_INEXACT|AMD_F_UNDERFLOW, ERANGE, x, (double)n, 2);
		}
		if(exponent > 2046)/*overflow*/
		{
			val.u32[1] = sign | 0x7ff00000;
			val.u32[0] = 0x00000000;
			return __amd_handle_error("ldexp", __amd_ldexp, val.u64, _OVERFLOW, AMD_F_INEXACT|AMD_F_OVERFLOW, ERANGE ,x, (double)n, 2);
		}

		exponent += MULTIPLIER_DP;
		val.u32[1] = sign | (exponent << 20) | (val.u32[1] & 0x000fffff);
		val.f64 = val.f64 * VAL_2PMMULTIPLIER_DP;
        return val.f64;
    }

    exponent += n;

    if(exponent < -MULTIPLIER_DP)/*underflow*/
	{
		val.u32[1] = sign | 0x00000000;
		val.u32[0] = 0x00000000;
        return __amd_handle_error("ldexp", __amd_ldexp, val.u64, _UNDERFLOW, AMD_F_INEXACT|AMD_F_UNDERFLOW, ERANGE, x, (double)n, 2);
	}

    if(exponent < 1)/*x is normal but output is debnormal*/
    {
		exponent += MULTIPLIER_DP;
		val.u32[1] = sign | (exponent << 20) | (val.u32[1] & 0x000fffff);
		val.f64 = val.f64 * VAL_2PMMULTIPLIER_DP;
        return val.f64;
    }

    if(exponent > 2046)/*overflow*/
	{
		val.u32[1] = sign | 0x7ff00000;
		val.u32[0] = 0x00000000;
	return __amd_handle_error("ldexp", __amd_ldexp, val.u64, _OVERFLOW, AMD_F_INEXACT|AMD_F_OVERFLOW, ERANGE ,x, (double)n, 2);
	}

    val.u32[1] = sign | (exponent << 20) | (val.u32[1] & 0x000fffff);
    return val.f64;
}




