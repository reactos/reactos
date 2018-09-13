	subttl  emfdiv.asm - Division
	page
;*******************************************************************************
;	 Copyright (c) Microsoft Corporation 1991
;	 All Rights Reserved
;
;emfdiv.asm - long double divide
;	by Tim Paterson
;
;Purpose:
;	Long double division.
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


;Dispatch tables for division
;
;One operand has been loaded into ecx:ebx:esi ("source"), the other is
;pointed to by edi ("dest").  edi points to dividend for fdiv,
;to divisor for fdivr.  
;
;Tag of source is shifted.  Tag values are as follows:
;
.erre	TAG_SNGL	eq	0	;SINGLE: low 32 bits are zero
.erre	TAG_VALID	eq	1
.erre	TAG_ZERO	eq	2
.erre	TAG_SPCL	eq	3	;NAN, Infinity, Denormal, Empty

;dest = dest / source
tFdivDisp	label	dword		;Source (reg)	Dest (*[di])
	dd	DivSingle		;single		single
	dd	DivSingle		;single		double
	dd	XorDestSign		;single		zero
	dd	DivSpclDest		;single		special
	dd	DivDouble		;double		single
	dd	DivDouble		;double		double
	dd	XorDestSign		;double		zero
	dd	DivSpclDest		;double		special
	dd	DivideByZero		;zero		single
	dd	DivideByZero		;zero		double
	dd	ReturnIndefinite	;zero		zero
	dd	DivSpclDest		;zero		special
	dd	DivSpclSource		;special	single
	dd	DivSpclSource		;special	double
	dd	DivSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	ReturnIndefinite	;Two infinities

;dest = source / dest
tFdivrDisp	label	dword		;Source (reg)	Dest (*[di])
	dd	DivrSingle		;single		single
	dd	DivrDouble		;single		double
	dd	DivideByZero		;single		zero
	dd	DivrSpclDest		;single		special
	dd	DivrSingle		;double		single
	dd	DivrDouble		;double		double
	dd	DivideByZero		;double		zero
	dd	DivrSpclDest		;double		special
	dd	XorSourceSign		;zero		single
	dd	XorSourceSign		;zero		double
	dd	ReturnIndefinite	;zero		zero
	dd	DivrSpclDest		;zero		special
	dd	DivrSpclSource		;special	single
	dd	DivrSpclSource		;special	double
	dd	DivrSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	ReturnIndefinite	;Two infinities


EM_ENTRY eFIDIV16
eFIDIV16:
	push	offset DivSetResult
	jmp	Load16Int		;Returns to DivSetResult

EM_ENTRY eFIDIVR16
eFIDIVR16:
	push	offset DivrSetResult
	jmp	Load16Int

EM_ENTRY eFIDIV32
eFIDIV32:
	push	offset DivSetResult
	jmp	Load32Int

EM_ENTRY eFIDIVR32
eFIDIVR32:
	push	offset DivrSetResult
	jmp	Load32Int

EM_ENTRY eFDIV32
eFDIV32:
	push	offset DivSetResult
	jmp	Load32Real			;Returns to DivSetResult

EM_ENTRY eFDIVR32
eFDIVR32:
	push	offset DivrSetResult		;Returns to DivrSetResult
	jmp	Load32Real

EM_ENTRY eFDIV64
eFDIV64:
	push	offset DivSetResult
	jmp	Load64Real			;Returns to DivSetResult

EM_ENTRY eFDIVR64
eFDIVR64:
	push	offset DivrSetResult
	jmp	Load64Real			;Returns to DivrSetResult


EM_ENTRY eFDIVRPreg
eFDIVRPreg:
	push	offset PopWhenDone

EM_ENTRY eFDIVRreg
eFDIVRreg:
	xchg	esi,edi

EM_ENTRY eFDIVRtop
eFDIVRtop:
	mov	ecx,EMSEG:[esi].ExpSgn
	mov	ebx,EMSEG:[esi].lManHi
	mov	esi,EMSEG:[esi].lManLo
DivrSetResult:
;cl has tag of dividend
	mov     ebp,offset tFdivrDisp
	mov	EMSEG:[Result],edi		;Save result pointer
	mov	ah,cl
	mov     al,EMSEG:[edi].bTag
	and	ah,not 1		;Ignore single vs. double on dividend
	cmp	ax,1
.erre	bTAG_VALID	eq	1
.erre	bTAG_SNGL	eq	0
	jz	DivrDouble		;Divisor was double
	ja	TwoOpResultSet
;.erre	DivrSingle eq $			;Fall into DivrSingle

;*********
DivrSingle:
;*********
;Computes op1/op2
;Op1 is double, op2 is single (low 32 bits are zero)
	mov	edx,ebx
	mov	eax,esi			;Mantissa in edx:eax
	mov	ebx,EMSEG:[edi].ExpSgn
	mov	edi,EMSEG:[edi].lManHi
	jmp	DivSingleReg


SDivBigUnderflow:
;Overflow flag set could only occur with denormals (true exp < -32768)
	or	EMSEG:[CURerr],Underflow
	test	EMSEG:[CWmask],Underflow	;Is exception masked?
	jnz	UnderflowZero		;Yes, return zero (in emfmul.asm)
	add	ecx,Underbias shl 16	;Fix up exponent
	jmp	ContSdiv		;Continue with multiply


EM_ENTRY eFDIVPreg
eFDIVPreg:
	push	offset PopWhenDone

EM_ENTRY eFDIVreg
eFDIVreg:
	xchg	esi,edi

EM_ENTRY eFDIVtop
eFDIVtop:
	mov	ecx,EMSEG:[esi].ExpSgn
	mov	ebx,EMSEG:[esi].lManHi
	mov	esi,EMSEG:[esi].lManLo
DivSetResult:
;cl has tag of divisor
	mov     ebp,offset tFdivDisp
	mov	EMSEG:[Result],edi		;Save result pointer
	mov	al,cl
	mov     ah,EMSEG:[edi].bTag
	and	ah,not 1		;Ignore single vs. double on dividend
	cmp	ax,1
.erre	bTAG_VALID	eq	1
.erre	bTAG_SNGL	eq	0
	jz	DivDouble		;Divisor was double
	ja	TwoOpResultSet
;.erre	DivSingle eq $			;Fall into DivSingle

;*********
DivSingle:
;*********
;Computes op2/op1
;Op2 is double, op1 is single (low 32 bits are zero)
	xchg	edi,ebx			;Mantissa in edi, op2 ptr to ebx
	xchg	ebx,ecx			;ExpSgn to ebx, op2 ptr to ecx
	mov	edx,EMSEG:[ecx].lManHi
	mov	eax,EMSEG:[ecx].lManLo
	mov	ecx,EMSEG:[ecx].ExpSgn	;Op2 loaded

DivSingleReg:
;dividend mantissa in edx:eax, exponent in high ecx, sign in ch bit 7
;divisor mantissa in edi, exponent in high ebx, sign in bh bit 7

	xor	ch,bh			;Compute result sign
	xor	bx,bx			;Clear out sign and tag
	sub	ecx,1 shl 16		;Exponent adjustment needed
	sub	ecx,ebx			;Compute result exponent
.erre	TexpBias eq 0			;Exponents not biased
	jo	SDivBigUnderflow	;Dividing denormal by large number
ContSdiv:

;If dividend >= divisor, the DIV instruction will overflow.  Check for
;this condition and shift the dividend right one bit if necessary.
;
;In previous versions of this algorithm for 24-bit and 53-bit mantissas,
;this shift was always performed without a test.  This meant that a 1-bit
;normalization might be required at the end.  This worked fine because
;32 or 64 bits were calculated, so extra precision was available for
;normalization.  However, this version needs all 64 bits that are calculated, 
;so we can't afford a normalization shift at the end.  This test tells us
;up front how to align so we'll be normalized.
	xor	ebx,ebx			;Extend dividend
	cmp	edi,edx			;Will DIV overflow?
	ja	DoSdiv			;No, we're safe
	shrd	ebx,eax,1
	shrd	eax,edx,1
	shr	edx,1
	add	ecx,1 shl 16		;Bump exponent to account for shift
DoSdiv:
	div	edi
	xchg	ebx,eax			;Save quotient in ebx, extend remainder
	div	edi
	mov	esi,eax
;We have a 64-bit quotient in ebx:esi.  Now compare remainder*2 with divisor
;to compute round and sticky bits.
	mov	eax,-1			;Set round and sticky bits
	shl	edx,1			;Double remainder
	jc	RoundJmp		;If too big, round & sticky set
	cmp	edx,edi			;Is remainder*2 > divisor?
	ja	RoundJmp

;Observe, oh wondering one, how you can assume the result of this last
;compare is not equality.  Use the following notation: n=numerator,
;d=denominator,q=quotient,r=remainder,b=base(2^64 here).  If
;initially we had n < d then there was no shift and we will find q and r
;so that q*d+r=n*b, if initially we had n >= d then there was a shift and
;we will find q and r so that q*d+r=n*b/2.  If we have equality here
;then r=d/2  ==>  n={possibly 2*}(2*q+1)*d/(2*b), since this can only
;be integral if d is a multiple of b, but by definition b/2 <= d < b, we
;have a contradiction.	Equality is thus impossible at this point.

	cmp	edx,1			;Check for zero remainder
	sbb	eax,-2			;eax==0 if CY, ==1 if NC (was -1)
RoundJmp:
	jmp	EMSEG:[RoundMode]

;*******************************************************************************

DDivBigUnderflow:
;Overflow flag set could only occur with denormals (true exp < -32768)
	or	EMSEG:[CURerr],Underflow
	test	EMSEG:[CWmask],Underflow	;Is exception masked?
	jnz	UnderflowZero		;Yes, return zero (in emfmul.asm)
	add	ecx,Underbias shl 16	;Fix up exponent
	jmp	ContDdiv		;Continue with multiply

DivrDoubleSetFlag:
;Special entry point used by FPATAN to set bit 6 of flag dword pushed
;on stack before call.
	or	byte ptr [esp+4],40H
;*********
DivrDouble:
;*********
;Computes op1/op2
	mov	edx,ebx
	mov	eax,esi			;Mantissa in edx:eax
	mov	ebx,EMSEG:[edi].ExpSgn
	mov	esi,EMSEG:[edi].lManHi
	mov	edi,EMSEG:[edi].lManLo
	jmp	short DivDoubleReg

HighHalfEqual:
;edx:eax:ebp = dividend
;esi:edi = divisor
;ecx = exponent and sign of result
;
;High half of dividend is equal to high half of divisor.  This will cause
;the DIV instruction to overflow.  If whole dividend >= whole divisor, then
;we just shift the dividend right 1 bit.
	cmp	eax,edi			;Is dividend >= divisor?
	jae	ShiftDividend		;Yes, divide it by two
;DIV instruction would overflow, so skip it and calculate the effective
;result.  Assume a quotient of 2^32-1 and calculate the remainder.  See
;detailed comments under MaxQuo below--this is a copy of that code.
	push	ecx			;Save exp. and sign
	mov	ebx,-1			;Max quotient digit
	sub	eax,edi			;Calculate correct remainder
;Currently edx == esi, but the next instruction ensures that is no longer
;true, since eax != 0.  This will allow us to skip the MaxQuo check at
;DivFirstDigit.
	add	edx,eax			;Should set CY if quotient fit
	mov	eax,edi			;ecx:eax has new remainder
	jc	ComputeSecond		;Remainder was positive
;Quotient doesn't fit.  Note that we can no longer ensure that edx != esi
;after making a correction.
	mov	ecx,edx			;Need remainder in ecx:eax
	jmp	DivCorrect1

;*********
DivDouble:
;*********
;Computes op2/op1
	mov	eax,edi			;Move op2 pointer
	mov	edi,esi
	mov	esi,ebx			;Mantissa in esi:edi
	mov	ebx,ecx			;ExpSgn to ebx
	mov	ecx,EMSEG:[eax].ExpSgn	;Op2 loaded
	mov	edx,EMSEG:[eax].lManHi
	mov	eax,EMSEG:[eax].lManLo

DivDoubleReg:
;dividend mantissa in edx:eax, exponent in high ecx, sign in ch bit 7
;divisor mantissa in esi:edi, exponent in high ebx, sign in bh bit 7

	xor	ch,bh			;Compute result sign
	xor	bx,bx			;Clear out sign and tag
	sub	ecx,1 shl 16		;Exponent adjustment needed
	sub	ecx,ebx			;Compute result exponent
.erre	TexpBias eq 0			;Exponents not biased
	jo	DDivBigUnderflow	;Dividing denormal by large number
ContDdiv:

;If dividend >= divisor, we must shift the dividend right one bit.
;This will ensure the result is normalized.
;
;In previous versions of this algorithm for 24-bit and 53-bit mantissas,
;this shift was always performed without a test.  This meant that a 1-bit
;normalization might be required at the end.  This worked fine because
;32 or 64 bits were calculated, so extra precision was available for
;normalization.  However, this version needs all 64 bits that are calculated, 
;so we can't afford a normalization shift at the end.  This test tells us
;up front how to align so we'll be normalized.
	xor	ebp,ebp			;Extend dividend
	cmp	esi,edx			;Dividend > divisor
	ja	DoDdiv
	jz	HighHalfEqual		;Go compare low halves
ShiftDividend:
	shrd	ebp,eax,1
	shrd	eax,edx,1
	shr	edx,1
	add	ecx,1 shl 16		;Bump exponent to account for shift
DoDdiv:
	push	ecx			;Save exp. and sign

;edx:eax:ebp = dividend
;esi:edi = divisor
;
;Division algorithm from Knuth vol. 2, p. 237, using 32-bit "digits":
;Guess a quotient digit by dividing two MSDs of dividend by the MSD of
;divisor.  If divisor is >= 1/2 the radix (radix = 2^32 in this case), then
;this guess will be no more than 2 larger than the correct value of that
;quotient digit (and never smaller).  Divisor meets magnitude condition 
;because it's normalized.

	div	esi			;Guess first quotient "digit"

;Check out our guess.  
;Currently, remainder in edx = dividend - (quotient * high half divisor).
;The definition of remainder is dividend - (quotient * all divisor).  So
;if we subtract (quotient * low half divisor) from edx, we'll get
;the true remainder.  If it's negative, our guess was too big.

	mov	ebx,eax			;Save quotient
	mov	ecx,edx			;Save remainder
	mul	edi			;Quotient * low half divisor
	sub	ebp,eax			;Subtract from dividend extension
	sbb	ecx,edx			;Subtract from remainder
	mov	eax,ebp			;Low remainder to eax
	jnc	DivFirstDigit		;Was quotient OK?
DivCorrect1:
	dec	ebx			;Quotient was too big
	add	eax,edi			;Add divisor back into remainder
	adc	ecx,esi
	jnc	DivCorrect1		;Repeat if quotient is still too big
DivFirstDigit:
	cmp	ecx,esi			;Would DIV instruction overflow?
	jae	short MaxQuo		;Yes, figure alternate quotient
	mov	edx,ecx			;Remainder back to edx:eax

;Compute 2nd quotient "digit"

ComputeSecond:
	div	esi			;Guess 2nd quotient "digit"
	mov	ebp,eax			;Save quotient
	mov	ecx,edx			;Save remainder
	mul	edi			;Quotient * low half divisor
	neg	eax			;Subtract from dividend extended with 0
	sbb	ecx,edx			;Subtract from remainder
	jnc	DivSecondDigit		;Was quotient OK?
DivCorrect2:
	dec	ebp			;Quotient was too big
	add	eax,edi			;Add divisor back into remainder
	adc	ecx,esi
	jnc	DivCorrect2		;Repeat if quotient is still too big
DivSecondDigit:
;ebx:ebp = quotient
;ecx:eax = remainder
;esi:edi = divisor
;Now compare remainder*2 with divisor to compute round and sticky bits.
	mov	edx,-1			;Set round and sticky bits
	shld	ecx,eax,1		;Double remainder
	jc	DDivEnd			;If too big, round & sticky set
	shl	eax,1
	sub	edi,eax
	sbb	esi,ecx			;Subtract remainder*2 from divisor
	jb	DDivEnd			;If <0, use round & sticky bits set

;Observe, oh wondering one, how you can assume the result of this last
;compare is not equality.  Use the following notation: n=numerator,
;d=denominator,q=quotient,r=remainder,b=base(2^64 here).  If
;initially we had n < d then there was no shift and we will find q and r
;so that q*d+r=n*b, if initially we had n >= d then there was a shift and
;we will find q and r so that q*d+r=n*b/2.  If we have equality here
;then r=d/2  ==>  n={possibly 2*}(2*q+1)*d/(2*b), since this can only
;be integral if d is a multiple of b, but by definition b/2 <= d < b, we
;have a contradiction.	Equality is thus impossible at this point.

;No round bit, but set sticky bit if remainder != 0.
	or	eax,ecx			;Is remainder zero?
	add	eax,-1			;Set CY if non-zero
	adc	edx,1			;edx==0 if NC, ==1 if CY (was -1)
DDivEnd:
	mov	esi,ebp			;Result in ebx:esi
	mov	eax,edx			;Round/sticky bits to eax
	pop	ecx			;Recover sign/exponent
	jmp	EMSEG:[RoundMode]


MaxQuo:
;ebx = first quotient "digit"
;ecx:eax = remainder
;esi:edi = divisor
;On exit, ebp = second quotient "digit"
;
;Come here if divide instruction would overflow. This must mean that ecx == esi,
;i.e., the high halves of the dividend and divisor are equal. Assume a result
;of 2^32-1, thus remainder = dividend - ( divisor * (2^32-1) )
; = dividend - divisor * 2^32 + divisor. Since the high halves of the dividend
;and divisor are equal, dividend - divisor * 2^32 can be computed by
;subtracting only the low halves. When adding divisor (in esi) to this, note
;that ecx == esi, and we want the result in ecx anyway.
;
;Note also that since the dividend is a previous remainder, the
;dividend - divisor * 2^32 calculation must always be negative. Thus the 
;addition of divisor back to it should generate a carry if it goes positive.

	mov	ebp,-1			;Max quotient digit
	sub	eax,edi			;Calculate correct remainder
	add	ecx,eax			;Should set CY if quotient fit
	mov	eax,edi			;ecx:eax has new remainder
	jc	DivSecondDigit		;Remainder was positive
	jmp	DivCorrect2
