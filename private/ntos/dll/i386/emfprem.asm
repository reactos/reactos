	subttl emfprem.asm - FPREM and FPREM1 instructions
	page
;*******************************************************************************
;emfprem.asm - FPREM and FPREM1 instructions
;	by Tim Paterson
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;	 All Rights Reserved
;
;Inputs:
;	edi = [CURstk]
;	ST(1) loaded into ebx:esi & ecx
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************

;Dispatch table for remainder
;
;One operand has been loaded into ecx:ebx:esi ("source"), the other is
;pointed to by edi ("dest").  
;
;Tag of source is shifted.  Tag values are as follows:

.erre   TAG_SNGL        eq      0       ;SINGLE: low 32 bits are zero
.erre   TAG_VALID       eq      1
.erre   TAG_ZERO        eq      2
.erre   TAG_SPCL        eq      3       ;NAN, Infinity, Denormal, Empty

;Any special case routines not found in this file are in emarith.asm

					;Divisor	Dividend
tFpremDisp	label	dword		;Source(ST(1))	Dest (ST(0))
	dd	PremDouble		;single		single
	dd	PremDouble		;single		double
	dd	PremX			;single		zero
	dd	PremSpclDest		;single		special
	dd	PremDouble		;double		single
	dd	PremDouble		;double		double
	dd	PremX			;double		zero
	dd	PremSpclDest		;double		special
	dd	ReturnIndefinite	;zero		single
	dd	ReturnIndefinite	;zero		double
	dd	ReturnIndefinite	;zero		zero
	dd	PremSpclDest		;zero		special
	dd	PremSpclSource		;special	single
	dd	PremSpclSource		;special	double
	dd	PremSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	ReturnIndefinite	;Two infinites


PremSpclDone:
	add	sp,4			;Clean off return address for normal
	ret

;***
PremSpclDest:
	mov	al,EMSEG:[edi].bTag		;Pick up tag
	cmp	al,bTAG_INF		;Dividing infinity?
	jz	ReturnIndefinite	;Invalid operation if so
	jmp	SpclDest		;In emarith.asm

;***
PremSpclSource:
	cmp	cl,bTAG_INF		;Dividing by infinity?
	jnz	SpclSource		;in emarith.asm
PremX:
;Return Dest unchanged, quotient = 0
	mov     EMSEG:[SWcc],0
	ret
;*******************************************************************************

;Map quotient bits to condition codes

Q0	equ	C1
Q1	equ	C3
Q2	equ	C0

MapQuo	label	byte
	db	0
	db	Q0
	db	Q1
	db	Q1+Q0
	db	Q2
	db	Q2+Q0
	db	Q2+Q1
	db	Q2+Q1+Q0

Prem1Cont:

;edx:eax = remainder, normalized
;ebx:esi = divisor
;ebp = quotient
;edi = exponent difference, zero or less
;ecx = 0 (positive sign)
;
;At this point, 0 <= remainder < divisor.  However, for FPREM1 we need
; -divisor/2 <= remainder <= divisor/2.  If remainder = divisor/2, whether
;we choose + or - is dependent on whichever gives us an even quotient
;(the usual IEEE rounding rule).  Quotient must be incremented if we
;use negative remainder.

	cmp	edi,-1
	jl	PremCont		;Remainder < divisor/2
	jg	NegRemainExp0		;Remainder > divisor/2
;Exponent is -1
	cmp	edx,ebx
	jl	PremCont		;Remainder < divisor/2
	jg	NegRemain		;Remainder > divisor/2
	cmp	eax,esi
	jl	PremCont		;Remainder < divisor/2
	jg	NegRemain		;Remainder > divisor/2
;Remainder = divisor/2.  Ensure quotient is even
	test	ebp,1			;Even?
	jz	PremCont
NegRemain:
;Theoretically we subtract divisor from remainder once more, leaving us
;with a negative remainder.  But since we use sign/magnitude representation,
;we want the abs() of that with sign bit set--so subtract remainder from
;(larger) divisor.  Note that exponent difference is -1, so we must align
;binary points first.
	add	esi,esi
	adc	ebx,ebx			;Double divisor to align binary points
NegRemainExp0:
	sub	esi,eax
	sbb	ebx,edx			;Subtract remainder
	mov	eax,esi
	mov	edx,ebx			;Result in edx:eax
	mov	ch,bSign		;Flip sign of remainder
	inc	ebp			;Increase quotient
;Must normalize result of subtraction
	bsr	ecx,edx			;Look for 1 bit
	jnz	@F
	sub	edi,32
	xchg	edx,eax			;Shift left 32 bits
	bsr	ecx,edx
@@:
	lea     edi,[edi+ecx-31]        ;Fix up exponent for normalization
	not     cl
	shld	edx,eax,cl
	shl	eax,cl
	mov	ch,bSign		;Flip sign of remainder

PremCont:
;edx:eax = remainder, normalized
;ebp = quotient
;edi = exponent difference, zero or less
;ch = sign
	or	eax,eax			;Low bits zero?
.erre	bTAG_VALID eq 1
.erre	bTAG_SNGL eq 0
	setnz   cl                      ;if low half==0 then cl=0 else cl=1
	mov	esi,EMSEG:[CURstk]
	mov     ebx,esi
	NextStackElem   ebx,Prem
	add	di,EMSEG:[ebx].wExp		;Compute result exponent
	cmp	di,IexpMin-IexpBias
	jle	PremUnderflow
SavePremResult:
	mov	EMSEG:[esi].lManLo,eax
	xor	EMSEG:[esi].bSgn,ch
	mov	EMSEG:[esi].lManHi,edx
	and	ebp,7			;Keep last 3 bits of quotient only
					;  and give write buffers a break
	mov	EMSEG:[esi].wExp,di
	mov	EMSEG:[esi].bTag,cl
	mov	al,MapQuo[ebp]		;Get cond. codes for this quotient
	mov	EMSEG:[SWcc],al
	ret

	NextStackWrap   ebx,Prem        ;Tied to NextStackElem above

PremUnderflow:
	test	EMSEG:[CWmask],Underflow	;Is exception unmasked?
	jz	UnmaskedPremUnder
	mov	cl,bTAG_DEN
	jmp	SavePremResult

UnmaskedPremUnder:
	add	edi,UnderBias		;Additional exp. bias for unmasked resp.
	or	EMSEG:[CURerr],Underflow
	jmp	SavePremResult

;*******************************************************************************

PremDouble:
;edi = [CURstk]
;ebx:esi = ST(1) mantissa, ecx = ExpSgn

	add	sp,4			;Clean off return address for special
	mov	eax,EMSEG:[edi].lManLo
	mov	edx,EMSEG:[edi].lManHi
	movsx	edi,EMSEG:[edi].wExp
	xor	ebp,ebp			;Quotient, in case we skip stage 1
	sar	ecx,16			;Bring exponent down
	sub	edi,ecx			;Get exponent difference
	jl	ExitPremLoop		;If dividend is smaller, return it.

;FPREM is performed in two stages.  The first stage is used only if the
;exponent difference is greater than 31.  It reduces the exponent difference
;by 32, and repeats until the difference is less than 32.  Note that
;unlike the hardware FPREM instruction, we are not limited to reducing
;the exponent by only 63--we just keep looping until it's done.
;
;The second stage performs ordinary 1-bit-at-a-time long division.
;It stops when the exponent difference is zero, meaning we have an
;integer quotient and the final remainder.
;
;edx:eax = dividend
;ebx:esi = divisor
;edi = exponent difference
;ebp = 0 (initial quotient)

	cmp	edi,32			;Do we need to do stage 1?
	jl	FitDivisor		;No, start stage 2

;FPREM stage 1
;
;Exponent difference is at least 32.  Use 32-bit division to compute
;quotient and exact remainder, reducing exponent difference by 32.

;DIV instruction will overflow if dividend >= divisor.  In this case,
;subtract divisor from dividend to ensure no overflow.  This will change
;the quotient, but that doesn't matter because we only need the last
;3 bits of the quotient (and we're about to calculate 32 quotient bits).
;This subtraction will not affect the remainder.

	sub	eax,esi
	sbb	edx,ebx	
	jnc	FpremReduce32		;Was dividend big?
	add	eax,esi			;Restore dividend, it was smaller
	adc	edx,ebx

;Division algorithm from Knuth vol. 2, p. 237, using 32-bit "digits":
;Guess a quotient digit by dividing two MSDs of dividend by the MSD of
;divisor.  If divisor is >= 1/2 the radix (radix = 2^32 in this case), then
;this guess will be no more than 2 larger than the correct value of that
;quotient digit (and never smaller).  Divisor meets magnitude condition 
;because it's normalized.
;
;This loop typically takes 117 clocks.

;edx:eax = dividend
;ebx:esi = divisor
;edi = exponent difference
;ebp = quotient (zero)

FpremReduce32:
;We know that dividend < divisor, but it is still possible that 
;high dividend == high divisor, which will cause the DIV instruction
;to overflow.
	cmp	edx,ebx			;Will DIV instruction overflow?
	jae	PremOvfl
	div	ebx			;Guess a quotient "digit"

;Currently, remainder in edx = dividend - (quotient * high half divisor).
;The definition of remainder is dividend - (quotient * all divisor).  So
;if we subtract (quotient * low half divisor) from edx, we'll get
;the true remainder.  If it's negative, our guess was too big.

	mov	ebp,eax			;Save quotient
	mov	ecx,edx			;Save remainder
	mul	esi			;Quotient * low half divisor
	neg	eax			;Subtract from dividend extended with 0
	sbb	ecx,edx			;Subtract from remainder
	mov	edx,ecx			;Remainder back to edx:eax
	jnc	HavPremQuo		;Was quotient OK?
FpremCorrect:
	dec	ebp			;Quotient was too big
	add	eax,esi			;Add divisor back into remainder
	adc	edx,ebx
	jnc	FpremCorrect		;Repeat if quotient is still too big
HavPremQuo:
	sub	edi,32			;Exponent reduced
	cmp	edi,32			;Exponent difference within 31?
	jl	PremNormalize		;Do it a bit a time
	or	edx,edx			;Check for zero remainder
	jnz	FpremReduce32
	or	eax,eax			;Remainder 0?
	jz	ExactPrem
	xchg	edx,eax			;Shift left 32 bits
	sub	edi,32			;Another 32 bits reduced
	cmp	edi,32
	jge	FpremReduce32
	xor	ebp,ebp			;No quotient bits are valid
	jmp	PremNormalize

PremOvfl:
;edx:eax = dividend
;ebx:esi = divisor
;On exit, ebp = second quotient "digit"
;
;Come here if divide instruction would overflow. This must mean that edx == ebx,
;i.e., the high halves of the dividend and divisor are equal. Assume a result
;of 2^32-1, thus remainder = dividend - ( divisor * (2^32-1) )
; = dividend - divisor * 2^32 + divisor. Since the high halves of the dividend
;and divisor are equal, dividend - divisor * 2^32 can be computed by
;subtracting only the low halves. When adding divisor (in ebx) to this, note
;that edx == ebx, and we want the result in edx anyway.
;
;Note also that since dividend < divisor, the
;dividend - divisor * 2^32 calculation must always be negative. Thus the 
;addition of divisor back to it should generate a carry if it goes positive.

	mov	ebp,-1			;Max quotient digit
	sub	eax,esi			;Calculate correct remainder
	add	edx,eax			;Should set CY if quotient fit
	mov	eax,esi			;edx:eax has new remainder
	jc	HavPremQuo		;Remainder was positive
	jmp	FpremCorrect

ExactPrem:
;eax = 0
	mov	esi,EMSEG:[CURstk]
	mov	EMSEG:[esi].lManLo,eax
	mov	EMSEG:[esi].lManHi,eax
	add	sp,4			;Clean off first return address
	mov	EMSEG:[esi].wExp,ax
	mov	EMSEG:[esi].bTag,bTAG_ZERO
	ret


;FPREM stage 2
;
;Exponent difference is less than 32.  Use restoring long division to
;compute quotient bits until exponent difference is zero.  Note that we
;often get more than one bit/loop:  BSR is used to scan off leading
;zeros each time around.  Since the divisor is normalized, we can
;instantly compute a zero quotient bit for each leading zero bit.
;
;For reductions of 1 to 31 bits per loop, this loop requires 41 or 59 clocks
;plus 3 clocks/bit (BSR time).  If we had to use this for 32-bit reductions
;(without stage 1), we could expect (50+6)*16 = 896 clocks typ (2 bits/loop)
;instead of the 112 required by stage 1!

FpremLoop:
;edx:eax = dividend (remainder) minus divisor
;ebx:esi = divisor
;ebp = quotient
;edi = exponent difference, less than 32
;
;If R is current remainder and d is divisor, then we have edx:eax = R - d, 
;which is negative.  We want 2*R - d, which is positive.  
;2*R - d = 2*(R - d) + d.
	add	eax,eax			;2*(R - d)
	adc	edx,edx
	add	eax,esi			;2*(R-d) + d = 2*R - d
	adc	edx,ebx	
	add	ebp,ebp			;Double quotient too
	dec	edi			;Decrement exponent difference
DivisorFit:
	inc	ebp			;Count one in quotient
PremNormalize:
	bsr	ecx,edx			;Find first 1 bit
	jz	PremHighZero
	not     cl
	and     cl,1FH                  ;Convert bit no. to shift count
	shld	edx,eax,cl		;Normalize
	shl	eax,cl
	sub	edi,ecx			;Reduce exponent difference
	jl	PremTooFar
	shl	ebp,cl			;Shift quotient
FitDivisor:
;Dividend could be larger or smaller than divisor
	sub	eax,esi
	sbb	edx,ebx
	jnc	DivisorFit
;Couldn't subtract divisor from dividend.
	or	edi,edi			;Is exponent difference zero or less?
	jg	FpremLoop
	add	eax,esi			;Restore dividend
	adc	edx,ebx
	xor	ecx,ecx			;Sign is positive
	ret

PremTooFar:
;Exponent difference in edi went negative when reduced by shift count in ecx.
;We need a quotient corresponding to exponent difference of zero.
	add	ecx,edi			;Restore exponent difference
	shl	ebp,cl			;Fix up quotient
ExitPremLoop:
;edx:eax = remainder, normalized
;ebp = quotient
;edi = exponent difference, zero or less
	xor	ecx,ecx			;Sign is positive
	ret

PremHighZero:
;High half of remainder is all zero, so we've reduced exponent difference
;by 32 bits and overshot.  We need a quotient corresponding to exponent 
;difference of zero, so we just shift it by the original difference.  Then
;we need to normalize the low half remainder.
	mov	ecx,edi
	shl	ebp,cl			;Fix up quotient
	bsr	ecx,eax
	jz	ExactPrem
	lea     edi,[edi+ecx-63]        ;Fix up exponent for normalization
	xchg	eax,edx			;Shift by 32 bits
	not     cl
        shl     edx,cl                  ;Normalize remainder
        xor     ecx,ecx                 ;Sign is positive
        ret
