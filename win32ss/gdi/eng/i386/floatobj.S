/*
 * COPYRIGHT:         LGPL, see LGPL.txt in the top level directory
 * PROJECT:           ReactOS Win32 subsystem
 * PURPOSE:           FLOATOBJ floating point emulation functions for x86
 * FILE:              subsystems/win32/win32k/objects/i386/floatobj.S
 * PROGRAMMER:        Timo Kreuzer
 */

#include <asm.inc>

.code

/*******************************************************************************
 * IEEE 754-1985 single precision floating point
 *
 *    | 31 | 30...23  | 22...0   |
 *    |sign| exponent | fraction |
 *
 *  mantissa = 1 + (fraction / 2^23)
 *  f = (-1)^sign * mantissa * 2 ^ (exponent - bias)
 *  bias = 127
 *
 *******************************************************************************
 * win32k x86 floating point emulation
 *
 * struct _EFLOAT
 = {
 *    LONG lMant;
 *    LONG lExp;
 * };
 *
 * f = (lMant / 0x40000000) * 2 ^ (lExp - 2)
 *   = lMant * 2 ^ (lExp - 32)
 *
 *******************************************************************************
 * Optimization notes:
 *
 * - shld is slow (4 cycles) and not pairable, mov + shl is faster
 * - esp is used, because it's available earlier
 * - bsr is very slow on old cpus (up to 72 cycles on a p1) while being much
 *   faster on modern cpus (2-11 cycles). Workarounds using branch trees or
 *   table lookups are of no use nowadays.
 *******************************************************************************
 * Compatibility notes:
 * - There are issues with very large size values near integer overflow.
 *   Floating point values are behaving different there. This behavior isn't
 *   simulated yet. Difference is < 10^-5 %
 * - The result of a multiplication can differ from Windows result in the
 *   least significant bit, that is a difference of 1 / 2^30 or ~10^-9
 *******************************************************************************
 * Implementation status:
 *
 * FLOATOBJ_SetFloat - implemented, tested
 * FLOATOBJ_SetLong - implemented, tested
 * FLOATOBJ_GetFloat - implemented, tested
 * FLOATOBJ_GetLong - implemented, tested
 * FLOATOBJ_Equal - implemented, tested
 * FLOATOBJ_EqualLong - implemented
 * FLOATOBJ_GreaterThan - implemented
 * FLOATOBJ_GreaterThanLong - wrapper
 * FLOATOBJ_LessThan - implemented
 * FLOATOBJ_LessThanLong - wrapper
 * FLOATOBJ_Neg - implemented
 * FLOATOBJ_Mul - implemented, tested, optimized
 * FLOATOBJ_MulFloat - wrapper
 * FLOATOBJ_MulLong - wrapper, could really need optimization
 * FLOATOBJ_Div - implemented
 * FLOATOBJ_DivFloat - wrapper
 * FLOATOBJ_DivLong - wrapper
 * FLOATOBJ_Add - implemented, tested
 * FLOATOBJ_AddFloat - wrapper
 * FLOATOBJ_AddLong - wrapper
 * FLOATOBJ_Sub - implemented, tested
 * FLOATOBJ_SubFloat - wrapper
 * FLOATOBJ_SubLong - wrapper
 */

#define lMant 0
#define lExp 4

#define PARAM1 8
#define PARAM2 12

/** Globals **/
/* extern const FLOATOBJ gef0; */
PUBLIC _gef0
_gef0:
    .long 0, 0

/* extern const FLOATOBJ gef1; */
PUBLIC _gef1
_gef1:
    .long HEX(40000000), HEX(00000002)

/* extern const FLOATOBJ gef16; */
PUBLIC _gef16
_gef16:
    .long HEX(40000000), HEX(00000006)

/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_SetFloat(IN OUT PFLOATOBJ pf, IN FLOATL f);
 */
_FLOATOBJ_SetFloat@8:
PUBLIC _FLOATOBJ_SetFloat@8
	push ebp
	mov ebp, esp

	mov ecx, [esp + PARAM2]		/* Load the float into ecx */
	mov eax, ecx				/* Copy float to eax for later */

	test ecx, HEX(7f800000)		/* Check for zero exponent - 0 or denormal */
	jz SetFloat0				/* If it's all zero, ... */

	shl ecx, 7					/* Put the bits for the mantissa in place */

	cdq							/* Fill edx with the sign from the FLOATL in eax */
	and ecx, HEX(7fffffff)			/* Mask out invalid field in the mantissa */

	shr eax, 23					/* Shift the exponent in eax in place */
	or ecx, HEX(40000000)		/* Set bit for 1 in the mantissa */
	and eax, HEX(0ff)			/* Mask out invalid fields in the exponent in eax */

	xor ecx, edx				/* Make use of the sign bit expanded to full edx */

	sub eax, 125				/* Adjust exonent bias */

	sub ecx, edx				/* Substract -1 or add 1 if sign was set */

	mov edx, [esp + PARAM1]		/* Load pf into edx */
	mov [edx + lMant], ecx		/* Save back mantissa */
	mov [edx + lExp], eax		/* Save back exponent */

	pop ebp						/* Return */
	ret 8

SetFloat0:
	mov edx, [esp + PARAM1]		/* Load pf into edx */

	mov dword ptr [edx + lMant], 0	/* Set mantissa and exponent to 0 */
	mov dword ptr [edx + lExp], 0

	pop ebp						/* Return */
	ret 8


/*******************************************************************************
 * LONG
 * APIENTRY
 * FLOATOBJ_GetFloat(IN PFLOATOBJ pf);
 *
 */
_FLOATOBJ_GetFloat@4:
PUBLIC _FLOATOBJ_GetFloat@4
	push ebp
	mov ebp, esp

	mov edx, [esp + PARAM1]		/* Load pf into edx */
	mov eax, [edx + lMant]		/* Load mantissa into eax */
	mov ecx, [edx + lExp]		/* Load exponent into ecx */

	cdq							/* Calculate abs(mantissa) */
	xor eax, edx

	add ecx, 125

	sub eax, edx
	jz GetFloatRet

	and ecx, HEX(0ff)			/* Mask out invalid fields in the exponent */
	and eax, HEX(3fffffff)		/* Mask out invalid fields in mantissa */

	shl ecx, 23					/* Shift exponent in place */
	shr eax, 7					/* Shift mantissa in place */

	and edx, HEX(80000000)			/* Reduce edx to sign bit only */

	or eax, ecx					/* Set exponent in result */
	or eax, edx					/* Set sign bit in result */

GetFloatRet:
	/* Return */
	pop ebp
	ret 4



/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_SetLong(OUT PFLOATOBJ pf, IN LONG l);
 *
 * Instead of using abs(l), which is 3 + 2 instructions, use a branch.
 */
_FLOATOBJ_SetLong@8:
PUBLIC _FLOATOBJ_SetLong@8
	push ebp
	mov ebp, esp

	mov eax, [esp + PARAM2]		/* Load l into eax */
	mov edx, [esp + PARAM1]		/* Load pf into edx */

	test eax, eax				/* different handling for <0, =0 and >0 */
	js SetLongNeg
	jz SetLong0

	bsr ecx, eax				/* Get number of most significant bit aka log2(l) */
	mov [edx + lExp], ecx		/* Safe log2(l) into exponent */

	neg ecx						/* Calculate necessary shift */
	add ecx, 30

	add dword ptr [edx + lExp], 2	/* Adjust exponent */

	shl eax, cl					/* Shift mantissa in place */
	mov [edx + lMant], eax		/* Save mantissa */

	pop ebp						/* Return */
	ret 8

SetLongNeg:
	neg eax						/* Get absolute value of l */
	bsr ecx, eax				/* Get number of most significant bit aka log2(l) */
	neg eax						/* Back to negative */

	mov [edx + lExp], ecx		/* Safe log2(-l) into exponent */

	neg ecx						/* Calculate necessary shift */
	add ecx, 30

	add dword ptr [edx + lExp], 2	/* Adjust exponent */

	shl eax, cl					/* Shift mantissa in place */
	mov [edx + lMant], eax		/* Save mantissa */

	pop ebp						/* Return */
	ret 8

SetLong0:
	mov dword ptr [edx + lMant], 0	/* Set mantissa and exponent to 0 */
	mov dword ptr [edx + lExp], 0

	pop ebp						/* Return */
	ret 8


/*******************************************************************************
 * LONG
 * APIENTRY
 * FLOATOBJ_GetLong(IN PFLOATOBJ pf);
 *
 */
_FLOATOBJ_GetLong@4:
PUBLIC _FLOATOBJ_GetLong@4
	push ebp
	mov ebp, esp

	mov edx, [ebp + PARAM1]		/* Load pf into edx */
	mov ecx, 32					/* Load (32 - lExp) into ecx */
	sub ecx, [edx + lExp]
	jle short GetLong2			/* Check for Overflow */

	mov eax, [edx + lMant]		/* Load mantissa into eax */
	sar eax, cl					/* Signed shift mantissa according to exponent */

	pop ebp						/* Return */
	ret 4

GetLong2:
	xor eax, eax				/* Overflow, return 0 */
	pop ebp
	ret 4


/******************************************************************************
 * BOOL
 * APIENTRY
 * FLOATOBJ_Equal(IN PFLOATOBJ pf1, IN PFLOATOBJ pf2);
 */
_FLOATOBJ_Equal@8:
PUBLIC _FLOATOBJ_Equal@8
	push ebp
	mov ebp, esp

	mov ecx, [esp + PARAM1]		/* Load pf1 into ecx */
	mov eax, [esp + PARAM2]		/* Load pf2 into ecx */

	mov edx, [ecx + lExp]		/* Get float1 in ecx, edx */
	mov ecx, [ecx + lMant]

	sub edx, [eax + lExp]		/* Calculate diference to float2 */
	sub ecx, [eax + lMant]

	or edx, ecx					/* Combine */

	mov eax, 0					/* Set eax if combination is 0 */
	setz al

	pop ebp						/* Return */
	ret 8


/******************************************************************************
 * BOOL
 * APIENTRY
 * FLOATOBJ_EqualLong(IN PFLOATOBJ pf, IN LONG l);
 */
_FLOATOBJ_EqualLong@8:
PUBLIC _FLOATOBJ_EqualLong@8
	push ebp
	mov ebp, esp

	mov eax, [esp + PARAM1]		/* Load pf into eax */
	mov ecx, 32					/* Load (32 - lExp) into ecx */
	sub ecx, [eax + lExp]
	mov edx, [eax + lMant]		/* Load mantissa into edx */
	sar edx, cl					/* Signed shift mantissa according to exponent */
	shl edx, cl					/* Shift the mantissa back */
	cmp edx, [eax + lMant]		/* Check whether bits were killed by shifting */
	jnz EqualLongFalse			/* We have truncated the mantissa, return 0 */

	sar edx, cl					/* Shift the mantissa again */
	xor eax, eax				/* Set return value ... */
	cmp edx, [esp + PARAM2]		/* TRUE if shifted mantissa equals the LONG */
	setz al

	pop ebp						/* Return */
	ret 8

EqualLongFalse:
	xor eax, eax				/* Return FALSE */
	pop ebp
	ret 8


/******************************************************************************
 * BOOL
 * APIENTRY
 * FLOATOBJ_GreaterThan(IN PFLOATOBJ pf, IN PFLOATOBJ pf1);
 *
 */
_FLOATOBJ_GreaterThan@8:
PUBLIC _FLOATOBJ_GreaterThan@8
	push ebp
	mov ebp, esp

	mov eax, [ebp + PARAM1]		/* Load pointer to efloat1 in eax */
	mov edx, [ebp + PARAM2]		/* Load pointer to efloat2 in edx */

	mov ecx, [eax + lMant]		/* Load mantissa1 in ecx */
	mov edx, [edx + lMant]		/* Load mantissa2 in edx */

	sar ecx, 31					/* Calculate sign(lMant1) in ecx */
	sar edx, 31					/* Calculate sign(lMant2) in edx */

	cmp ecx, edx				/* Branch if both have the same sign */
	je GreaterThan_2

	/* Mantissae have different sign */
	mov eax, 0					/* Return (sign(lMant1) > sign(lMant2)) */
	setg al
	pop ebp
	ret 8

GreaterThan_2:
	/* Mantissae have the same sign */

	mov edx, [ebp + PARAM2]		/* Reload pointer to float2 in edx */
	test ecx, ecx				/* Branch if sign is negative */
	js GreaterThan_neg

	/* Both mantissae are positive or 0 */

	or ecx, [edx + lMant]		/* Branch if one mantissa is 0 */
	jz GreaterThan_pos2

	/* Both mantissae are positive */

	mov ecx, [eax + lExp]		/* Branch if exponents are equal */
	cmp ecx, [edx + lExp]
	je GreaterThan_pos2

	mov eax, 0					/* Return (lExp1 > lExp2) */
	setg al
	pop ebp
	ret 8

GreaterThan_pos2:
	/* Exponents are equal or one mantissa is 0 */

	mov ecx, [eax + lMant]		/* Return (lMant1 > lMant2) */
	cmp ecx, [edx + lMant]
	mov eax, 0
	setg al
	pop ebp
	ret 8

GreaterThan_neg:
	/* Both mantissae are negative */

	mov ecx, [eax + lExp]		/* Branch if exponents are equal */
	cmp ecx, [edx + lExp]
	je GreaterThan_neg2

	/* Both mantissae negative, exponents are different */

	mov eax, 0					/* Return (lExp1 < lExp2) */
	setl al
	pop ebp
	ret 8

GreaterThan_neg2:
	/* Both mantissae negative, exponents are equal */

	mov ecx, [eax + lMant]		/* Return (lMant1 < lMant2) */
	cmp ecx, [edx + lMant]
	mov eax, 0
	setl al
	pop ebp
	ret 8



/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_GreaterThanLong(IN OUT PFLOATOBJ pf, IN LONG l);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetLong and
 * LOATOBJ_GreaterThan
 */
_FLOATOBJ_GreaterThanLong@8:
PUBLIC _FLOATOBJ_GreaterThanLong@8
	push ebp
	mov ebp, esp

	sub esp, 8					/* Make room for a FLOATOBJ on the stack */
	mov eax, [ebp + PARAM2]		/* Load LONG into eax */

	lea ecx, [ebp -8]			/* Load pointer to local FLOATOBJ into ecx */

	push eax					/* Push LONG on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetLong@8	/* Set the local FLOATOBJ */

	lea ecx, [ebp -8]			/* Push pointer to the local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_GreaterThan@8	/* Compare */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/******************************************************************************
 * BOOL
 * APIENTRY
 * FLOATOBJ_LessThan(IN PFLOATOBJ pf, IN PFLOATOBJ pf1);
 *
 */
_FLOATOBJ_LessThan@8:
PUBLIC _FLOATOBJ_LessThan@8
	push ebp
	mov ebp, esp

	mov eax, [ebp + PARAM1]		/* Load pointer to floats in eax and edx */
	mov edx, [ebp + PARAM2]

	mov ecx, [eax + lMant]		/* Load mantissae in ecx and edx */
	mov edx, [edx + lMant]

	sar ecx, 31					/* Calculate sign(lMant1) and sign(lMant2) */
	sar edx, 31

	cmp ecx, edx				/* Branch if both have the same sign */
	je LessThan_2

	/* Mantissae have different sign */

	mov eax, 0					/* Return (sign(lMant1) < sign(lMant2)) */
	setl al
	pop ebp
	ret 8

LessThan_2:
	/* Mantissae have the same sign */


	mov edx, [ebp + PARAM2]		/* Reload pointer to float2 in edx */

	test ecx, ecx				/* Branch if sign is negative */
	js LessThan_neg

	/* Both mantissae are positive or 0 */

	or ecx, [edx + lMant]		/* Branch if one mantissa is 0 */
	jz LessThan_pos2

	/* Both mantissae are positive */

	mov ecx, [eax + lExp]		/* Branch if exponents are equal */
	cmp ecx, [edx + lExp]
	je LessThan_pos2

	mov eax, 0					/* Return (lExp1 < lExp2) */
	setl al
	pop ebp
	ret 8

LessThan_pos2:
	/* Exponents are equal or one mantissa is 0 */

	mov ecx, [eax + lMant]		/* Return (lMant1 < lMant2) */
	cmp ecx, [edx + lMant]
	mov eax, 0
	setl al
	pop ebp
	ret 8

LessThan_neg:
	/* Both mantissae are negative */

	mov ecx, [eax + lExp]		/* Branch if exponents are equal */
	cmp ecx, [edx + lExp]
	je LessThan_neg2

	/* Both mantissae negative, exponents are different */

	mov eax, 0					/* Return (lExp1 > lExp2) */
	setg al
	pop ebp
	ret 8

LessThan_neg2:
	/* Both mantissae negative, exponents are equal */

	mov ecx, [eax + lMant]		/* Return (lMant1 > lMant2) */
	cmp ecx, [edx + lMant]
	mov eax, 0
	setg al
	pop ebp
	ret 8


/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_LessThanLong(IN OUT PFLOATOBJ pf, IN LONG l);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetLong and FLOATOBJ_LessThan
 */
_FLOATOBJ_LessThanLong@8:
PUBLIC _FLOATOBJ_LessThanLong@8
	push ebp
	mov ebp, esp

	sub esp, 8					/* Make room for a FLOATOBJ on the stack */
	mov eax, [ebp + PARAM2]		/* Load LONG into eax */

	lea ecx, [ebp -8]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push LONG on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetLong@8	/* Set the local FLOATOBJ */

	lea ecx, [ebp -8]			/* Push pointer to the local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_LessThan@8	/* Compare */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8



/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_Mul(IN OUT PFLOATOBJ pf1, IN PFLOATOBJ pf2);
 *
 *  (mant1 * 2^exp1) * (mant2 * 2^exp2) = (mant1 * mant2) * 2^(exp1 + exp2)
 *  or mant = mant1 * mant2 and exp = exp1 + exp2
 *  No special handling for 0, where mantissa is 0
 */
_FLOATOBJ_Mul@8:
PUBLIC _FLOATOBJ_Mul@8
	push ebp
	mov ebp, esp

	mov edx, [esp + PARAM1]		/* Load pf1 into edx */
	mov ecx, [esp + PARAM2]		/* Load pf2 into ecx */
	mov eax, [ecx + lMant]		/* Load mantissa2 into eax */
	mov ecx, [ecx + lExp]		/* Load exponent2 into ecx */

	imul dword ptr [edx + lMant]	/* Multiply eax with mantissa 1 */

	test edx, edx				/* Special handling for result < 0 */
	js MulNeg

	shl edx, 2					/* Get new mantissa from bits 30 to 62 */
	shr eax, 30					/* of edx:eax into edx */
	or eax, edx

	mov edx, ecx				/* Need ecx for the shift, safe exp2 to free edx */
	mov ecx, 0					/* Check for highest bit */
	sets cl
	shr eax, cl					/* Normalize mantissa in eax */

	jz Mul0						/* All 0? */

	lea edx, [edx + ecx -2]		/* Normalize exponent in edx */


	mov ecx, [esp + PARAM1]		/* Load pf1 into ecx */
	mov [ecx + lMant], eax		/* Save back mantissa */
	add [ecx + lExp], edx		/* Save back exponent */

	pop ebp						/* Return */
	ret 8

MulNeg:

	shl edx, 2					/* Get new mantissa from bits 30 to 62 */
	shr eax, 30					/* of edx:eax into edx */
	or eax, edx

	mov edx, ecx				/* Need ecx for the shift, safe exp2 to free edx */

	mov ecx, 0					/* Check for highest bit */
	setns cl
	shr eax, cl					/* Normalize mantissa in eax */

	jz Mul0						/* All 0? */


	lea edx, [edx + ecx -2]		/* Normalize exponent in edx */
	or eax, HEX(80000000)			/* Set sign bit */

	mov ecx, [esp + PARAM1]		/* Load pf1 into ecx */
	mov [ecx + lMant], eax		/* Save back mantissa */
	add [ecx + lExp], edx		/* Save back exponent */

	pop ebp						/* Return */
	ret 8

Mul0:
	mov ecx, [esp + PARAM1]		/* Load pf1 into ecx */
	mov [ecx + lMant], eax		/* Store 0 in mantissa */
	mov [ecx + lExp], eax		/* Store 0 in exponent */

	pop ebp						/* Return */
	ret 8


/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_MulFloat(IN OUT PFLOATOBJ pf, IN FLOATL f);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetFloat and FLOATOBJ_Mul
 */
_FLOATOBJ_MulFloat@8:
PUBLIC _FLOATOBJ_MulFloat@8
	push ebp
	mov ebp, esp

	sub esp, 8					/* Make room for a FLOATOBJ on the stack */
	mov eax, [ebp + PARAM2]		/* Load f into eax */
	lea ecx, [ebp -4]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push f on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetFloat@8	/* Set the FLOATOBJ */

	lea ecx, [ebp -4]			/* Push pointer to local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_Mul@8		/* Multiply */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_MulLong(IN OUT PFLOATOBJ pf, IN LONG l);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetLong and FLOATOBJ_Mul
 */
_FLOATOBJ_MulLong@8:
PUBLIC _FLOATOBJ_MulLong@8
	push ebp
	mov ebp, esp

	sub esp, 8					/* Make room for a FLOATOBJ on the stack */
	mov eax, [ebp + PARAM2]		/* Load l into eax */
	lea ecx, [ebp -8]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push l on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetLong@8	/* Set the local FLOATOBJ */

	lea ecx, [ebp -8]			/* Push pointer to local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_Mul@8		/* Multiply */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/*******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_Div(IN OUT PFLOATOBJ pf1, IN PFLOATOBJ pf2);
 *
 */
_FLOATOBJ_Div@8:
PUBLIC _FLOATOBJ_Div@8
	push ebp
	mov ebp, esp
	push ebx

	mov eax, [ebp + PARAM2]		/* Load lMant2 into eax */
	mov eax, [eax + lMant]

	cdq							/* Calculate abs(lMant2) */
	xor eax, edx
	sub eax, edx
	jz DivError					/* Divide by zero error! */

	mov ebx, edx				/* Copy sign(lMant2) to ebx */
	mov ecx, eax				/* Copy abs(lMant2) to ecx */

	mov eax, [ebp + PARAM1]		/* Load lMant1 into eax */
	mov eax, [eax + lMant]

	cdq							/* Calculate abs(lMant1) */
	xor eax, edx
	sub eax, edx

	jz Div0						/* Dividend is 0? */

	xor ebx, edx				/* combine both signs in ebx */

	mov edx, eax				/* Prepare edx:eax for integer divide */
	xor eax, eax
	shr edx, 1
	div ecx						/* Do an unsigned divide */

	xor ecx, ecx				/* Adjust result */
	test eax, HEX(80000000)
	setnz cl
	shr eax, cl

	xor eax, ebx				/* Correct the result's sign */
	sub eax, ebx

	mov edx, [ebp + PARAM1]		/* Load pf1 into edx */
	mov [edx + lMant], eax		/* Safe back the mantissa */
	mov ebx, [ebp + PARAM2]		/* Load pf2 into ebx */
	sub ecx, [ebx + lExp]		/* Calculate exponent offset */
	inc ecx
	add [edx + lExp], ecx		/* Safe back exponent */

	pop ebx						/* Return */
	pop ebp
	ret 8

DivError:
Div0:
	mov edx, [ebp + PARAM1]		/* Load pf into edx */
	mov [edx + lMant], eax		/* Store 0 in mantissa */
	mov [edx + lExp], eax		/* Store 0 in exponent */

	pop ebx						/* Return */
	pop ebp
	ret 8


/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_DivFloat(IN OUT PFLOATOBJ pf, IN FLOATL f);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetFloat and FLOATOBJ_Div
 */
_FLOATOBJ_DivFloat@8:
PUBLIC _FLOATOBJ_DivFloat@8
	push ebp
	mov ebp, esp
	sub esp, 8					/* Make room for a FLOATOBJ on the stack */

	mov eax, [ebp + PARAM2]		/* Load f into eax */
	lea ecx, [ebp -4]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push f on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetFloat@8	/* Set the FLOATOBJ */

	lea ecx, [ebp -4]			/* Push pointer to local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_Div@8		/* Divide */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_DivLong(IN OUT PFLOATOBJ pf, IN LONG l);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetLong and FLOATOBJ_Div
 */
_FLOATOBJ_DivLong@8:
PUBLIC _FLOATOBJ_DivLong@8
	push ebp
	mov ebp, esp
	sub esp, 8					/* Make room for a FLOATOBJ on the stack */

	mov eax, [ebp + PARAM2]		/* Load l into eax */
	lea ecx, [ebp -8]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push l on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetLong@8	/* Set the local FLOATOBJ */

	lea ecx, [ebp -8]			/* Push pointer to the local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_Div@8		/* Divide */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/*******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_Add(IN OUT PFLOATOBJ pf1, IN PFLOATOBJ pf2);
 *
 */
_FLOATOBJ_Add@8:
PUBLIC _FLOATOBJ_Add@8
	push ebp
	mov ebp, esp
	push ebx

	mov eax, [ebp + PARAM1]		/* Load pointer to pf1 in eax */
	mov ebx, [ebp + PARAM2]		/* Load pointer to pf2 in ebx */

	mov ecx, [eax + lExp]		/* Load float1 in (eax,ecx) */
	mov edx, [ebx + lExp]
	mov eax, [eax + lMant]		/* Load float2 in (ebx,edx) */
	mov ebx, [ebx + lMant]

	cmp ecx, edx				/* Check which one has the bigger lExp */
	jl Add2

	sub ecx, edx				/* Calculate lExp1 - lExp2 */
	sar eax, 1					/* Shift both mantissae 1 bit right */
	sar ebx, 1
	sar ebx, cl					/* Shift lMant2 according to exponent difference */

	add eax, ebx				/* Add the manrissae */
	jz AddIs0

	cdq							/* Calculate abs(mantissa) */
	xor eax, edx
	sub eax, edx

	bsr ecx, eax				/* Find most significant bit */
	neg ecx						/* and calculate needed normalize shift */
	add ecx, 30
	shl eax, cl
	dec ecx

	xor eax, edx				/* Go back to original sign */
	sub eax, edx

	mov edx, [ebp + PARAM1]		/* Reload pointer to float1 */

	pop ebx

	mov dword ptr [edx + lMant], eax	/* Safe mantissa */
	sub [edx + lExp], ecx		/* Adjust exponent */

	pop ebp						/* Return */
	ret 8

Add2:
	sub edx, ecx				/* Calculate lExp2 - lExp1 and put it into ecx */
	mov ecx, edx

	sar ebx, 1					/* Shift both mantissae 1 bit right */
	sar eax, 1
	sar eax, cl					/* Shift lMant2 according to exponent difference */

	add eax, ebx				/* Add the manrissae */
	jz AddIs0

	mov ebx, [ebp + PARAM1]		/* Reload pointer to float1 */
	add [ebx + lExp], ecx		/* Adjust exponent part 1 */

	cdq							/* Calculate abs(mantissa) */
	xor eax, edx
	sub eax, edx

	bsr ecx, eax				/* Find most significant bit */
	neg ecx						/* and calculate needed normalize shift */
	add ecx, 30
	shl eax, cl
	dec ecx

	xor eax, edx				/* Go back to original sign */
	sub eax, edx

	mov dword ptr [ebx + lMant], eax	/* Safe mantissa and adjust exponent */
	sub [ebx + lExp], ecx

	pop ebx						/* Return */
	pop ebp
	ret 8

AddIs0:
	/* Mantissa is 0, so float to (0,0) */
	mov eax, [ebp + PARAM1]
	pop ebx
	mov dword ptr [eax + lMant], 0
	mov dword ptr [eax + lExp], 0
	pop ebp
	ret 8


/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_AddFloat(IN OUT PFLOATOBJ pf, IN FLOATL f);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetFloat and FLOATOBJ_Add
 */
_FLOATOBJ_AddFloat@8:
PUBLIC _FLOATOBJ_AddFloat@8
	push ebp
	mov ebp, esp
	sub esp, 8					/* Make room for a FLOATOBJ on the stack */

	mov eax, [ebp + PARAM2]		/* Load f into eax */
	lea ecx, [ebp -4]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push f on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetFloat@8	/* Set the FLOATOBJ */

	lea ecx, [ebp -4]			/* Push pointer to local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_Add@8		/* Add */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_AddLong(IN OUT PFLOATOBJ pf, IN LONG l);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetLong and FLOATOBJ_Add
 */
_FLOATOBJ_AddLong@8:
PUBLIC _FLOATOBJ_AddLong@8
	push ebp
	mov ebp, esp
	sub esp, 8					/* Make room for a FLOATOBJ on the stack */

	mov eax, [ebp + PARAM2]		/* Load l into eax */
	lea ecx, [ebp -8]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push l on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetLong@8	/* Set the local FLOATOBJ */

	lea ecx, [ebp -8]			/* Push pointer to the local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_Add@8		/* Add */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/*******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_Sub(IN OUT PFLOATOBJ pf, IN PFLOATOBJ pf1);
 *
 */
_FLOATOBJ_Sub@8:
PUBLIC _FLOATOBJ_Sub@8
	push ebp
	mov ebp, esp
	push ebx

	mov eax, [ebp + PARAM1]		/* Load pointer to floats in eax and ebx */
	mov ebx, [ebp + PARAM2]

	mov ecx, [eax + lExp]		/* Load float1 in (eax,ecx) and float2 in (ebx,edx) */
	mov edx, [ebx + lExp]
	mov eax, [eax + lMant]
	mov ebx, [ebx + lMant]

	cmp ecx, edx				/* Check which one has the bigger lExp */
	jl Sub2

	sub ecx, edx				/* Calculate lExp1 - lExp2 */
	sar eax, 1					/* Shift both mantissae 1 bit right */
	sar ebx, 1
	sar ebx, cl					/* Shift lMant2 according to exponent difference */

	sub eax, ebx				/* Substract the manrissae */
	jz SubIs0

	cdq							/* Calculate abs(mantissa) */
	xor eax, edx
	sub eax, edx

	bsr ecx, eax				/* Find most significant bit */
	neg ecx						/* and calculate needed normalize shift */
	add ecx, 30
	shl eax, cl
	dec ecx

	xor eax, edx				/* Go back to original sign */
	sub eax, edx

	mov edx, [ebp + PARAM1]		/* Reload pointer to float1 */

	pop ebx

	mov dword ptr [edx + lMant], eax /* Safe mantissa and adjust exponent */
	sub [edx + lExp], ecx

	pop ebp
	ret 8

Sub2:
	sub edx, ecx				/* Calculate lExp2 - lExp1 and put it into ecx */
	mov ecx, edx

	sar ebx, 1					/* Shift both mantissae 1 bit right */
	sar eax, 1
	sar eax, cl					/* Shift lMant2 according to exponent difference */

	sub eax, ebx				/* Substract the manrissae */
	jz AddIs0

	mov ebx, [ebp + PARAM1]		/* Reload pointer to float1 */
	add [ebx + lExp], ecx		/* Adjust exponent part 1 */

	cdq							/* Calculate abs(mantissa) */
	xor eax, edx
	sub eax, edx

	bsr ecx, eax				/* Find most significant bit */
	neg ecx						/* and calculate needed normalize shift */
	add ecx, 30
	shl eax, cl
	dec ecx

	xor eax, edx				/* Go back to original sign */
	sub eax, edx

	mov dword ptr [ebx + lMant], eax	/* Safe mantissa */
	sub [ebx + lExp], ecx		/* Adjust exponent */

	pop ebx						/* Return */
	pop ebp
	ret 8

SubIs0:
	/* Mantissa is 0, so float to (0,0) */
	mov eax, [ebp + PARAM1]
	pop ebx
	mov dword ptr [eax + lMant], 0
	mov dword ptr [eax + lExp], 0
	pop ebp
	ret 8

/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_SubFloat(IN OUT PFLOATOBJ pf, IN FLOATL f);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetFloat and FLOATOBJ_Sub
 */
_FLOATOBJ_SubFloat@8:
PUBLIC _FLOATOBJ_SubFloat@8
	push ebp
	mov ebp, esp
	sub esp, 8					/* Make room for a FLOATOBJ on the stack */

	mov eax, [ebp + PARAM2]		/* Load f into eax */
	lea ecx, [ebp -4]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push f on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetFloat@8	/* Set the FLOATOBJ */

	lea ecx, [ebp -4]			/* Push pointer to local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_Sub@8		/* Substract */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_SubLong(IN OUT PFLOATOBJ pf, IN LONG l);
 *
 * Currently implemented as a wrapper around FLOATOBJ_SetLong and FLOATOBJ_Sub
 */
_FLOATOBJ_SubLong@8:
PUBLIC _FLOATOBJ_SubLong@8
	push ebp
	mov ebp, esp
	sub esp, 8					/* Make room for a FLOATOBJ on the stack */

	mov eax, [ebp + PARAM2]		/* Load l into eax */
	lea ecx, [ebp -8]			/* Load pointer to local FLOATOBJ into ecx */
	push eax					/* Push l on the stack */
	push ecx					/* Push pointer to local FLOATOBJ on the stack */
	call _FLOATOBJ_SetLong@8	/* Set the local FLOATOBJ */

	lea ecx, [ebp -8]			/* Push pointer to the local FLOATOBJ on the stack */
	push ecx
	push [ebp + PARAM1]			/* Push the FLOATOBJ param on the stack */
	call _FLOATOBJ_Sub@8		/* Substract */

	mov esp, ebp				/* Cleanup and return */
	pop ebp
	ret 8


/*******************************************************************************
 * VOID
 * APIENTRY
 * FLOATOBJ_Neg(IN OUT PFLOATOBJ pf);
 *
 */
_FLOATOBJ_Neg@4:
PUBLIC _FLOATOBJ_Neg@4
	push ebp
	mov ebp, esp

	mov ecx, [esp + PARAM1]		/* Load pf into ecx */
	neg dword ptr [ecx + lMant]	/* Negate lMant1 */

	pop ebp						/* Return */
	ret 4

END
/* EOF */
