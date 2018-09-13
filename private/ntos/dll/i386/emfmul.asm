	subttl  emfmul.asm - Multiplication
	page
;*******************************************************************************
;	 Copyright (c) Microsoft Corporation 1991
;	 All Rights Reserved
;
;emfmul.asm - long double multiply
;	by Tim Paterson
;
;Purpose:
;	Long double multiplication.
;Inputs:
;	ebx:esi = op1 mantissa
;	ecx = op1 sign in bit 15, exponent in high half
;	edi = pointer to op2 and result location
;	[Result] = edi
;
;	Exponents are unbiased.  Denormals have been normalized using
;	this expanded exponent range.  Neither operand is allowed to be zero.
;Outputs:
;	Jumps to [RoundMode] to round and store result.
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************

;Dispatch table for multiply
;
;One operand has been loaded into ecx:ebx:esi ("source"), the other is
;pointed to by edi ("dest").  
;
;Tag of source is shifted.  Tag values are as follows:

.erre	TAG_SNGL	eq	0	;SINGLE: low 32 bits are zero
.erre	TAG_VALID	eq	1
.erre	TAG_ZERO	eq	2
.erre	TAG_SPCL	eq	3	;NAN, Infinity, Denormal, Empty

;Any special case routines not found in this file are in emarith.asm

tFmulDisp	label	dword		;Source (reg)	Dest (*[di])
	dd	MulSingle		;single		single
	dd	MulDouble		;single		double
	dd	XorDestSign		;single		zero
	dd	MulSpclDest		;single		special
	dd	MulDouble		;double		single
	dd	MulDouble		;double		double
	dd	XorDestSign		;double		zero
	dd	MulSpclDest		;double		special
	dd	XorSourceSign		;zero		single
	dd	XorSourceSign		;zero		double
	dd	XorDestSign		;zero		zero
	dd	MulSpclDest		;zero		special
	dd	MulSpclSource		;special	single
	dd	MulSpclSource		;special	double
	dd	MulSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	XorDestSign		;Two infinities


EM_ENTRY eFIMUL16
eFIMUL16:
	push	offset MulSetResult
	jmp	Load16Int			;Returns to MulSetResult

EM_ENTRY eFIMUL32
eFIMUL32:
	push	offset MulSetResult
	jmp	Load32Int			;Returns to MulSetResult

EM_ENTRY eFMUL32
eFMUL32:
	push	offset MulSetResult
	jmp	Load32Real			;Returns to MulSetResult

EM_ENTRY eFMUL64
eFMUL64:
	push	offset MulSetResult
	jmp	Load64Real			;Returns to MulSetResult

EM_ENTRY eFMULPreg
eFMULPreg:
	push	offset PopWhenDone

EM_ENTRY eFMULreg
eFMULreg:
	xchg	esi,edi

EM_ENTRY eFMULtop
eFMULtop:
	mov	ecx,EMSEG:[esi].ExpSgn
	mov	ebx,EMSEG:[esi].lManHi
	mov	esi,EMSEG:[esi].lManLo
MulSetResult:
	mov     ebp,offset tFmulDisp
	mov	EMSEG:[Result],edi		;Save result pointer
	mov	al,cl
	or	al,EMSEG:[edi].bTag
	cmp	al,bTAG_VALID
.erre	bTAG_VALID	eq	1
.erre	bTAG_SNGL	eq	0
	jz	MulDouble
	ja	TwoOpResultSet
;.erre	MulSingle eq $			;Fall into MulSingle


;*********
MulSingle:
;*********

	mov	edx,EMSEG:[edi].ExpSgn
	mov	eax,EMSEG:[edi].lManHi

;op1 mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7
;op2 high mantissa in eax, exponent in high edx, sign in dh bit 7

	xor	ch,dh			;Compute result sign
	xor	dx,dx			;Clear out sign and tag
	add	ecx,edx			;Result exponent
.erre	TexpBias eq 0			;Exponents not biased
	jo	SMulBigUnderflow	;Multiplying two denormals
ContSmul:

;Value in ecx is correct exponent if result is not normalized.
;If result comes out normalized, 1 will be added.

	mul	ebx			;Compute product
	mov	ebx,edx
	mov	esi,eax
	xor	eax,eax			;Extend with zero

;Result in ebx:esi:eax
;ecx = exponent minus one in high half, sign in ch
	or	ebx,ebx			;Check for normalization
	jns	ShiftOneBit		;In emfadd.asm
	add	ecx,1 shl 16		;Adjust exponent
	jmp	EMSEG:[RoundMode]

SMulBigUnderflow:
	or	EMSEG:[CURerr],Underflow
	add	ecx,Underbias shl 16	;Fix up exponent
	test	EMSEG:[CWmask],Underflow	;Is exception masked?
	jz	ContSmul		;No, continue with multiply
UnderflowZero:
	or	EMSEG:[CURerr],Precision
SignedZero:
	and	ecx,bSign shl 8		;Preserve sign bit
	xor	ebx,ebx
	mov	esi,ebx
	mov	cl,bTAG_ZERO
	jmp	EMSEG:[ZeroVector]

;*******************************************************************************

DMulBigUnderflow:
;Overflow flag set could only occur with denormals (true exp < -32768)
	or	EMSEG:[CURerr],Underflow
	test	EMSEG:[CWmask],Underflow	;Is exception masked?
	jnz	UnderflowZero		;Yes, return zero
	add	ecx,Underbias shl 16	;Fix up exponent
	jmp	ContDmul		;Continue with multiply

PolyMulToZero:
	ret				;Return the zero in registers

PolyMulDouble:
;This entry point is used by polynomial evaluator.
;It checks the operand in registers for zero.
	cmp	cl,bTAG_ZERO		;Adding to zero?
	jz	PolyMulToZero

;*********
MulDouble:
;*********

	mov	eax,EMSEG:[edi].ExpSgn
	mov	edx,EMSEG:[edi].lManHi
	mov	edi,EMSEG:[edi].lManLo

MulDoubleReg:				;Entry point used by transcendentals
;op1 mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7
;op2 mantissa in edx:edi, exponent in high eax, sign in ah bit 7

	xor	ch,ah			;Compute result sign
	xor	ax,ax			;Clear out sign and tag
	add	ecx,eax			;Result exponent
.erre	TexpBias eq 0			;Exponents not biased
	jo	DMulBigUnderflow	;Multiplying two denormals
ContDmul:

;Value in ecx is correct exponent if result is not normalized.
;If result comes out normalized, 1 will be added.

	mov	ebp,edx			;edx is used by MUL instruction

;Generate and sum partial products, from least to most significant

	mov	eax,edi
	mul	esi			;Lowest partial product
	add	eax,-1			;CY set IFF eax<>0
	sbb	cl,cl			;Sticky bit: 0 if zero, -1 if nz
	xchg	edi,edx			;Save high result

;First product: cl reflects low dword non-zero (sticky bit), edi has high dword

	mov	eax,ebx
	mul	edx
	add	edi,eax
	adc	edx,0			;Sum first results
	xchg	edx,esi			;High result to esi

;Second product: accumulated in esi:edi:cl

	mov	eax,ebp			;Next mult. to eax
	mul	edx
	add	edi,eax			;Sum low results
	adc	esi,edx			;Sum high results
	mov	eax,ebx
	mov	ebx,0			;Preserve CY flag
	adc	ebx,ebx			;Keep carry out of high sum

;Third product: accumulated in ebx:esi:edi:cl

	mul	ebp
	add	esi,eax
	adc	ebx,edx
	mov	eax,edi
	or	al,cl			;Collapse sticky bits into eax

;Result in ebx:esi:eax
;ecx = exponent minus one in high half, sign in ch
MulDivNorm:
	or	ebx,ebx			;Check for normalization
	jns	ShiftOneBit		;In emfadd.asm
	add	ecx,1 shl 16		;Adjust exponent
	jmp	EMSEG:[RoundMode]
