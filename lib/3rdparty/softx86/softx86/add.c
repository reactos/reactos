/*
 * add.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 *
 * Decompiler and executioneer for ADD/SUB/ADC/SBB/CMP instructions.
 *
 ***********************************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 ************************************************************************************/

#include <softx86.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include "optable.h"
#include "add.h"

sx86_ubyte op_add8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;

/* peform the addition */
	ret = src + val;

/* if carry */
        if ((ret < src) || (ret < val)) ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
        else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* if overflow */
	if (((src & 0x80) == (val & 0x80)) && ((src & 0x80) != (ret & 0x80)))
        {
            ctx->state->reg_flags.val |= SX86_CPUFLAG_OVERFLOW;
        }
	else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if (((val&0xF)+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_add16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;

/* peform the addition */
	ret = src + val;

/* if carry */
        if ((ret < src) || (ret < val)) ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
        else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* if overflow */
	if (((src & 0x8000) == (val & 0x8000)) && ((src & 0x8000) != (ret & 0x8000)))
        {
            ctx->state->reg_flags.val |= SX86_CPUFLAG_OVERFLOW;
        }
	else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x8000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if (((val&0xF)+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else			
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_add32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;

/* peform the addition */
	ret = src + val;

/* if carry */
        if ((ret < src) || (ret < val)) ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
        else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* if overflow */
	if (((src & 0x80000000) == (val & 0x80000000))
            && ((src & 0x80000000) != (ret & 0x80000000)))
        {
            ctx->state->reg_flags.val |= SX86_CPUFLAG_OVERFLOW;
        }
	else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x80000000)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if (((val&0xF)+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else			
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_adc8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret,carry;

	carry = ((sx86_ubyte)(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY))?1:0;

/* special case: x + 0xFF + (carry=1) == x */
	if (carry && val == 0xFF) {
/* peform the addition */
		ret = val;

/* it's a carry over */
		ctx->state->reg_flags.val |=
			 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}
	else {
/* peform the addition */
		ret = src + val + carry;

/* if carry/overflow */
		if (ret < src)
			ctx->state->reg_flags.val |=
				 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
		else
			ctx->state->reg_flags.val &=
				~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if (((val&0xF)+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_adc16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret,carry;

	carry = ((sx86_ubyte)(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY))?1:0;

/* special case: x + 0xFFFF + (carry=1) == x */
	if (carry && val == 0xFFFF) {
/* peform the addition */
		ret = val;

/* it's a carry over */
		ctx->state->reg_flags.val |=
			 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}
	else {
/* peform the addition */
		ret = src + val + carry;

/* if carry/overflow */
		if (ret < src)
			ctx->state->reg_flags.val |=
				 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
		else
			ctx->state->reg_flags.val &=
				~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if (((val&0xF)+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_adc32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret,carry;

	carry = ((sx86_ubyte)(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY))?1:0;

/* special case: x + 0xFFFFFFFF + (carry=1) == x */
	if (carry && val == 0xFFFFFFFF) {
/* peform the addition */
		ret = val;

/* it's a carry over */
		ctx->state->reg_flags.val |=
			 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}
	else {
/* peform the addition */
		ret = src + val + carry;

/* if carry/overflow */
		if (ret < src)
			ctx->state->reg_flags.val |=
				 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
		else
			ctx->state->reg_flags.val &=
				~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a + b = c, ((b AND 0Fh) + (a AND 0Fh) >= 0x10).
   in other words, if the lowest nibbles added together overflow. */
	if (((val&0xF)+(src&0xF)) >= 0x10)
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_sub8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret;

/* peform the addition */
	ret = src - val;

/* if carry */
        if (val > src) ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
        else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* if overflow */
	if (((src & 0x80) != (val & 0x80)) && ((src & 0x80) != (ret & 0x80)))
        {
            ctx->state->reg_flags.val |= SX86_CPUFLAG_OVERFLOW;
        }
	else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause a carry. */
	if ((val&0xF) > (src&0xF))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_sub16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret;

/* peform the addition */
	ret = src - val;

/* if carry */
        if (val > src) ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
        else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* if overflow */
	if (((src & 0x8000) != (val & 0x8000)) && ((src & 0x8000) != (ret & 0x8000)))
        {
            ctx->state->reg_flags.val |= SX86_CPUFLAG_OVERFLOW;
        }
	else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause a carry. */
	if ((val&0xF) > (src&0xF))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_sub32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret;

/* peform the addition */
	ret = src - val;

/* if carry */
        if (val > src) ctx->state->reg_flags.val |= SX86_CPUFLAG_CARRY;
        else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_CARRY;

/* if overflow */
	if (((src & 0x80000000) != (val & 0x80000000))
            && ((src & 0x80000000) != (ret & 0x80000000)))
        {
            ctx->state->reg_flags.val |= SX86_CPUFLAG_OVERFLOW;
        }
	else ctx->state->reg_flags.val &= ~SX86_CPUFLAG_OVERFLOW;

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause a carry. */
	if ((val&0xF) > (src&0xF))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_ubyte op_sbb8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val)
{
	sx86_ubyte ret,carry;

	carry = ((sx86_ubyte)(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY))?1:0;

/* special case: x - 0xFF - (carry=1) == x */
	if (carry && val == 0xFF) {
/* peform the addition */
		ret = val;

/* it's a carry over */
		ctx->state->reg_flags.val |=
			 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}
	else {
/* peform the addition */
		ret = src - (val + carry);

/* if carry/overflow */
		if (ret > src)
			ctx->state->reg_flags.val |=
				 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
		else
			ctx->state->reg_flags.val &=
				~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}

/* if result treated as signed value is negative */
	if (ret & 0x80)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause a carry. */
	if ((val&0xF) > (src&0xF))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_uword op_sbb16(softx86_ctx* ctx,sx86_uword src,sx86_uword val)
{
	sx86_uword ret,carry;

	carry = ((sx86_ubyte)(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY))?1:0;

/* special case: x + 0xFFFF + (carry=1) == x */
	if (carry && val == 0xFFFF) {
/* peform the addition */
		ret = val;

/* it's a carry over */
		ctx->state->reg_flags.val |=
			 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}
	else {
/* peform the addition */
		ret = src - (val + carry);

/* if carry/overflow */
		if (ret > src)
			ctx->state->reg_flags.val |=
				 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
		else
			ctx->state->reg_flags.val &=
				~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}

/* if result treated as signed value is negative */
	if (ret & 0x8000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause a carry. */
	if ((val&0xF) > (src&0xF))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity16(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

sx86_udword op_sbb32(softx86_ctx* ctx,sx86_udword src,sx86_udword val)
{
	sx86_udword ret,carry;

	carry = ((sx86_ubyte)(ctx->state->reg_flags.val & SX86_CPUFLAG_CARRY))?1:0;

/* special case: x + 0xFFFFFFFF + (carry=1) == x */
	if (carry && val == 0xFFFFFFFF) {
/* peform the addition */
		ret = val;

/* it's a carry over */
		ctx->state->reg_flags.val |=
			 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}
	else {
/* peform the addition */
		ret = src - (val + carry);

/* if carry/overflow */
		if (ret > src)
			ctx->state->reg_flags.val |=
				 (SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
		else
			ctx->state->reg_flags.val &=
				~(SX86_CPUFLAG_CARRY | SX86_CPUFLAG_OVERFLOW);
	}

/* if result treated as signed value is negative */
	if (ret & 0x80000000)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_SIGN;
	else			ctx->state->reg_flags.val &= ~SX86_CPUFLAG_SIGN;

/* if result is zero */
	if (!ret)	ctx->state->reg_flags.val |=  SX86_CPUFLAG_ZERO;
	else		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_ZERO;

/* auxiliary flag emulation (not defined by Intel's documentation, but observed
   from many trials in DEBUG on an old Pentium Pro 166MHz I have kickin' around
   as a web server :)
   
   apparently aux is set if:
   given operation a - b = c, ((a AND 0Fh) - (b AND 0Fh) < 0).
   in other words, if the lowest nibbles subtracted cause a carry. */
	if ((val&0xF) > (src&0xF))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_AUX;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_AUX;

/* finally, compute parity for parity bit */
	if (softx86_parity8(ret))
		ctx->state->reg_flags.val |=  SX86_CPUFLAG_PARITY;
	else	
		ctx->state->reg_flags.val &= ~SX86_CPUFLAG_PARITY;

	return ret;
}

int Sfx86OpcodeExec_add(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x00) {					// ADD reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_rw(ctx,w16,0,mod,reg,
			rm,opswap,op_add8,op_add16,op_add32);

		return 1;
	}
	if (opcode == 0x04) {						// ADD AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		x = op_add8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		ctx->state->general_reg[SX86_REG_AX].b.lo = x;
		return 1;
	}
	if (opcode == 0x05) {						// ADD AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		x  = op_add16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		ctx->state->general_reg[SX86_REG_AX].w.lo = x;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_add(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x00) {					// ADD reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"ADD %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"ADD %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0x04) {						// ADD AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"ADD AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0x05) {						// ADD AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"ADD AX,%04Xh",x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_adc(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x10) {					// ADC reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_rw(ctx,w16,0,mod,reg,
			rm,opswap,op_adc8,op_adc16,op_adc32);

		return 1;
	}
	if (opcode == 0x14) {						// ADC AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		x = op_adc8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		ctx->state->general_reg[SX86_REG_AX].b.lo = x;
		return 1;
	}
	if (opcode == 0x15) {						// ADC AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		x  = op_adc16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		ctx->state->general_reg[SX86_REG_AX].w.lo = x;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_adc(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x10) {					// ADC reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"ADC %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"ADC %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0x14) {						// ADC AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"ADC AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0x15) {						// ADC AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"ADC AX,%04Xh",x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_sub(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x28) {					// SUB reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_rw(ctx,w16,0,mod,reg,
			rm,opswap,op_sub8,op_sub16,op_sub32);

		return 1;
	}
	if (opcode == 0x2C) {						// SUB AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		x = op_sub8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		ctx->state->general_reg[SX86_REG_AX].b.lo = x;
		return 1;
	}
	if (opcode == 0x2D) {						// SUB AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		x  = op_sub16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		ctx->state->general_reg[SX86_REG_AX].w.lo = x;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_sub(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x28) {					// SUB reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"SUB %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"SUB %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0x2C) {						// SUB AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"SUB AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0x2D) {						// SUB AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"SUB AX,%04Xh",x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_sbb(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x18) {					// SBB reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_rw(ctx,w16,0,mod,reg,
			rm,opswap,op_sbb8,op_sbb16,op_sbb32);

		return 1;
	}
	if (opcode == 0x1C) {						// SBB AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		x = op_sbb8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		ctx->state->general_reg[SX86_REG_AX].b.lo = x;
		return 1;
	}
	if (opcode == 0x1D) {						// SBB AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		x  = op_sbb16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		ctx->state->general_reg[SX86_REG_AX].w.lo = x;
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_sbb(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x18) {					// SBB reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"SBB %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"SBB %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0x1C) {						// SBB AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"SBB AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0x1D) {						// SBB AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"SBB AX,%04Xh",x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeExec_cmp(sx86_ubyte opcode,softx86_ctx* ctx)
{
	if ((opcode&0xFC) == 0x38) {					// CMP reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_exec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_exec_full_modregrm_ro(ctx,w16,0,mod,reg,		// we use the variant
			rm,opswap,op_sub8,op_sub16,op_sub32);		// that DOES NOT modify
									// the destination
									// register, since CMP
									// is documented to
									// perform subtraction
									// in a temporary register

		return 1;
	}
	if (opcode == 0x3C) {						// CMP AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_exec_byte(ctx);
		op_sub8(ctx,ctx->state->general_reg[SX86_REG_AX].b.lo,x);
		return 1;
	}
	if (opcode == 0x3D) {						// CMP AX,imm16
		sx86_uword x;

		x  = softx86_fetch_exec_byte(ctx);
		x |= softx86_fetch_exec_byte(ctx) << 8;
		op_sub16(ctx,ctx->state->general_reg[SX86_REG_AX].w.lo,x);
		return 1;
	}

	return 0;
}

int Sfx86OpcodeDec_cmp(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128])
{
	if ((opcode&0xFC) == 0x38) {					// CMP reg,reg/mem or reg/mem,reg
		sx86_ubyte x;
		sx86_ubyte w16,opswap;
		sx86_ubyte mod,reg,rm;

		w16    = (opcode&1);
		opswap = (opcode&2)>>1;					// add from reg to r/m UNLESS this bit it set (then it's the other way around)
		x      = softx86_fetch_dec_byte(ctx);			// fetch mod/reg/rm
		sx86_modregrm_unpack(x,mod,reg,rm);
		sx86_dec_full_modregrm(ctx,w16,0,mod,reg,rm,op1_tmp,op2_tmp);

		if (opswap)	sprintf(buf,"CMP %s,%s",op2_tmp,op1_tmp);
		else		sprintf(buf,"CMP %s,%s",op1_tmp,op2_tmp);
		return 1;
	}
	if (opcode == 0x3C) {						// CMP AL,imm8
		sx86_ubyte x;

		x = softx86_fetch_dec_byte(ctx);
		sprintf(buf,"CMP AL,%02Xh",x);
		return 1;
	}
	if (opcode == 0x3D) {						// CMP AX,imm16
		sx86_uword x;

		x  = softx86_fetch_dec_byte(ctx);
		x |= softx86_fetch_dec_byte(ctx) << 8;
		sprintf(buf,"CMP AX,%04Xh",x);
		return 1;
	}

	return 0;
}
