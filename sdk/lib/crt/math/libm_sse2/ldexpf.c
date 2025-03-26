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
#define __amd_handle_errorf _handle_errorf
#define __amd_ldexp OP_LDEXP

//#include "fn_macros.h"
//#include "libm_util_amd.h"
//#include "libm_special.h"

float FN_PROTOTYPE_REF(ldexpf)(float x, int n)
{
    UT32 val,val_x;
    unsigned int sign;
    int exponent;
    val.f32 = x;
	val_x.f32 = x;
    sign = val.u32 & 0x80000000;
    val.u32 = val.u32 & 0x7fffffff;/* remove the sign bit */

    if(val.u32 > 0x7f800000)/* x= nan */
        #ifdef WINDOWS
        return __amd_handle_errorf("ldexpf", __amd_ldexp, val_x.u32|0x00400000, _DOMAIN, 0, EDOM, x, (float)n, 2);
        #else
        {
          if(!(val.u32 & 0x00400000)) //x is snan
              return __amd_handle_errorf("ldexpf", __amd_ldexp, val_x.u32|0x00400000, _DOMAIN, AMD_F_INVALID, EDOM, x, (float)n, 2);
          else
              return x;
		}
        #endif

    if(val.u32 == 0x7f800000)/* x = +-inf*/
        return x;

    if((val.u32 == 0x00000000) || (n==0))/* x= +-0 or n= 0*/
        return x;

    exponent = val.u32 >> 23; /* get the exponent */

	if(exponent == 0)/*x is denormal*/
	{
		val.f32 = val.f32 * VAL_2PMULTIPLIER_SP;/*multiply by 2^24 to bring it to the normal range*/
		exponent = (val.u32 >> 23); /* get the exponent */
		exponent = exponent + n - MULTIPLIER_SP;
		if(exponent < -MULTIPLIER_SP)/*underflow*/
		{
			val.u32 = sign | 0x00000000;
            return __amd_handle_errorf("ldexpf", __amd_ldexp, val.u32, _UNDERFLOW, AMD_F_INEXACT|AMD_F_UNDERFLOW, ERANGE, x, (float)n, 2);
		}
		if(exponent > 254)/*overflow*/
		{
			val.u32 = sign | 0x7f800000;
            return __amd_handle_errorf("ldexpf", __amd_ldexp, val.u32, _OVERFLOW, AMD_F_INEXACT|AMD_F_OVERFLOW, ERANGE, x, (float)n, 2);
		}

		exponent += MULTIPLIER_SP;
		val.u32 = sign | (exponent << 23) | (val.u32 & 0x007fffff);
		val.f32 = val.f32 * VAL_2PMMULTIPLIER_SP;
        return val.f32;
	}

    exponent += n;

    if(exponent < -MULTIPLIER_SP)/*underflow*/
	{
		val.u32 = sign | 0x00000000;
        return __amd_handle_errorf("ldexpf", __amd_ldexp, val.u32, _UNDERFLOW, AMD_F_INEXACT|AMD_F_UNDERFLOW, ERANGE, x, (float)n, 2);
	}

    if(exponent < 1)/*x is normal but output is debnormal*/
    {
		exponent += MULTIPLIER_SP;
		val.u32 = sign | (exponent << 23) | (val.u32 & 0x007fffff);
		val.f32 = val.f32 * VAL_2PMMULTIPLIER_SP;
        return val.f32;
    }

    if(exponent > 254)/*overflow*/
	{
        val.u32 = sign | 0x7f800000;
        return __amd_handle_errorf("ldexpf", __amd_ldexp, val.u32, _OVERFLOW, AMD_F_INEXACT|AMD_F_OVERFLOW, ERANGE, x, (float)n, 2);
	}

    val.u32 = sign | (exponent << 23) | (val.u32 & 0x007fffff);/*x is normal and output is normal*/
    return val.f32;
}


