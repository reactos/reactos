	subttl	emfsqrt.asm - FSQRT instruction
	page
;*******************************************************************************
;emfsqrt.asm - FSQRT instruction
;	by Tim Paterson
;
;	 Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;	 All Rights Reserved
;
;Inputs:
;	edi = [CURstk]
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


;A linear approximation of the square root function is used to get the
;intial value for Newton-Raphson iteration.  This approximation gives
;nearly 5-bit accuracy over the required input interval, [1,4).  The
;equation for the linear approximation of y = sqrt(x) is y = mx + b,
;where m is the slope (named SQRT_COEF) and b is the y-intercept (named
;SQRT_INTERCEPT).
;
;(The values for m and b were computed with Excel Solver in two passes: 
;the first pass computed them full precision, minimizing absolute error;
;the second computed only b after m was rounded to an 8-bit value.)
;
;The resulting values have the following maximum error:
;
;inp. value -->		1		 2.18972	3.82505
;----------------------------------------------------------------
;abs. err., full prec.	0.04544		-0.03233	0.04423
;abs. err., truncated	0.04544		-0.04546	0.04423
;
;The three input values shown represent the left end point, the maximum 
;error (derivative of absolute error == 0), and the right end point.  
;The right end point is not 4 because the approximation reaches 2.000
;at the value given--we abandon the linear approximation at that point
;and use that same value for all greater input values.	This linear
;approximation is computed with 8-bit operations, so truncations can
;add a negative error.  This increases maximum error only when it is
;already negative, as shown in the table.
;
;Each iteration of Newton-Raphson approximation more than doubles the
;number of bits of accuracy.  Suppose the current guess is A, and it has
;an absolute error of e (i.e., A+e or A-e is the root).  Then the absolute
;error after the next iteration is e^2/2A.  This error is always positive.
;However, the divide instruction truncates, which introduces an error
;that is always negative.  Sometimes a constant or rounding bit is added
;to balance the positive and negative errors.  The maximum possible error 
;is given in comments below for each iteration.  (Note that when we compute 
;the error from e^2/2A, A could be in the range 1 to 2--we use 1 to get
;max error.)  Remember that the binary point is to the RIGHT of the MSB
;when looking at these error numbers.


;SQRT_INTERCEPT is used when the binary point is to the right of the MSB.
;Multiplying it by 64K would put the binary point to the left of the MSB,
;so it must be divided by two to be aligned.
SQRT_INTERCEPT	equ	23185		; 0.70755 * 65536 / 2

;SQRT_COEF would have the binary point to the left of the MSB if multiplied
;by 256.  However, this would leave it with a leading zero, so we multiply
;it by two more to normalize it.
SQRT_COEF	equ	173		; 0.33789 * 256 * 2

SqrtSpcl:
	cmp	al,bTAG_DEN
	jz	SqrtDen
	cmp	al,bTAG_INF
	jnz	SpclDestNotDen
;Have infinity
	or	ah,ah			;Is it negative?
	js	ReturnIndefinite
SqrtRet:
	ret


MaxStartRoot:
;The first iteration is calculated as  (ax / bh) * 100H + bx.  The first 
;trial root in bx should be 10000H (which is too big).  But it's very
;easy to calculate (ax / 100H) * 100H + 10000H = ax.
	mov	bx,ax
	cmp	ax,-1			;Would subsequent DIV overflow?
	jb	FirstTrialRoot
;The reduced argument is so close to 4.0 that the 16-bit DIV instruction
;used in the next iteration would overflow.  If the argument is 4-A 
;then a guess of 2.0 is in error by approximately A/4.  [This is not
;an upper bound.  The error is a little by more than this by an
;addition with the magnitude of A^2.  This is an insignificant amount
;when A is small.]  This means that the first guess of 2.0 is quite
;accurate, and we'll use it to bypass some of the iteration steps. 
;This will eliminate the DIV overflow by skipping the DIV.
;
;One iteration is performed by: (Arg/Guess + Guess)/2.  When Guess = 2,
;this becomes (Arg/2 + 2)/2 = Arg/4 + 1.  We get Arg/2 just by assuming
;the binary point is one bit further left; then a single right shift is
;needed to get Arg/4.  By shifting in a 1 bit on the left, we account for
;adding 1 at the same time.  [Note that if Arg = 4 - A, then Arg/4 + 1
; = (4 - A)/4 + 1 = 1 - A/4 + 1 = 2 - A/4.  In other words, we just
;subtract out exactly what we estimate our error to be, A/4.]
;
;Since the upper 16 bits are 0FFFFH, A <= 2^-14, so error <= 2^-16 =
; +0.00001526, -0.
	mov	ebx,esi			;Return root in ebx
	sar	ebx,1			;Trial root = arg/2
	cmp	esi,ebx			;Will 32-bit division overflow?
	jb	StartThirdIteration	;No, our 32-bit guess is good
;Argument is really, really close to 4.0: with an initial trial root of
;2.0, max absolute error is 2^-32 = +2.328E-10, -0.  One trivial
;iteration will get us 65-bit accuracy, max abs. error = +2.71E-20, -0.
	mov	ebx,esi
	mov	eax,ecx			;65-bit root*2 in ebx:eax (MSB implied)
	shl	ecx,2			;ecx = low half*4
	jmp	RoundRoot

SqrtDen:
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal ;Is denormal exception masked?
	jnz	SqrtRet			;If not, quit

;******
EM_ENTRY eFSQRT
eFSQRT:
;******
	mov	eax,EMSEG:[edi].ExpSgn
	cmp	al,bTAG_ZERO
	jz	SqrtRet
	ja	SqrtSpcl
	or	ah,ah
	js	ReturnIndefinite
	mov	esi,EMSEG:[edi].lManHi
	mov	ecx,EMSEG:[edi].lManLo
	sar	EMSEG:[edi].wExp,1	;Divide exponent by two
	mov	edi,0			;Extend mantissa
	jc	RootAligned		;If odd exponent, leave it normalized
	shrd	edi,ecx,1
	shrd	ecx,esi,1
	shr	esi,1			;Denormalize, extending into edi
RootAligned:
;esi:ecx:edi has mantissa, 2 MSBs are left of binary point. Range is [1,4).
	shld	eax,esi,16		;Get high word of mantissa
	movzx	ebx,ah			;High byte to bl
;UNDONE:  MASM 6 bug!!
;UNDONE:  SQRT_COEF (=0AEH) get sign extended!!
	mov	dx,SQRT_COEF		;UNDONE
	imul	bx,dx			;UNDONE
;UNDONE imul	bx,SQRT_COEF		;Product in bx
;Multiply by SQRT_COEF causes binary point to shift left 1 bit.
	add	bx,SQRT_INTERCEPT	;5-bit approx. square root in bh
	jc	MaxStartRoot
;Max absolute error is +/- 0.04546
	div	bh			;See how close we are
	add	bh,al			;quotient + divisor (always sets CY)
FirstTrialRoot:
;Avoid RCR because it takes 9 clocks on 386.  Use SHRD (3 clocks) instead.
	mov	dl,1			;Need bit set
	shrd	bx,dx,1			;(quotient + divisor)/2
;bx has 9-bit approx. square root, normalized
;Max absolute error is +0.001033, -0.003906
	movzx	eax,si
	shld	edx,esi,16		;dx:ax has high half mantissa
	div	bx			;Test our approximation
	add	ebx,eax			;quotient + divisor
	shl	ebx,15			;Normalize (quotient + divisor)/2
;ebx has 17-bit approx. square root, normalized
;Max absolute error is +0.000007629, -0.00001526
;Add adjustment factor to center the error range at +/-0.00001144
	or	bh,20H			;Add in 0.000003815
StartThirdIteration:
	mov	edx,esi
	mov	eax,ecx
	div	ebx			;Test approximation
	stc				;Set bit for rounding (= 2.328E-10)
	adc	ebx,eax			;quotient + divisor + round bit
;Avoid RCR because it takes 9 clocks on 386.  Use SHRD (3 clocks) instead.
	mov	dl,1			;Need bit set
	shrd	ebx,edx,1		;(quotient + divisor)/2, rounded
;ebx has 32-bit approx. square root, normalized
;Max absolute error is +2.983E-10, -2.328E-10
	mov	edx,esi			;Last time we need high half
	mov	eax,ecx
	shld	ecx,edi,2		;ecx = low half*4, w/extension back in
	div	ebx			;Test approximation
	xchg	edi,eax			;Save 1st quotient, get extension
	mov	esi,eax
	or	esi,edx			;Any remainder?
	jz	HaveRoot		;Result is ebx:esi
	div	ebx			;edi:eax is 64-bit quotient
	add	ebx,edi			;quotient + divisor (always sets CY)
RoundRoot:
	mov	esi,eax			;Save low half root*2

;We have 65-bit root*2 in ebx:esi (eax==esi) (MSB is implied one).
;Max absolute error is +4.450E-20, -5.421E-20.	This maximum error 
;corresponds to just less than +/- 1 in the last (65th) bit.  
;	
;We have to determine if this error is positive or negative so
;we can tell if we rounded up or down (and set the status bit
;accordingly).	This is done by squaring the root and comparing the
;that result with the input.
;
;Squaring the sample root requires summing partial products:
; lo*lo + lo*hi + hi*lo + hi*hi.  lo*hi == hi*lo, so only one multiply
;is needed there.  The low half of lo*lo isn't relevant, we know it
;is non-zero.  Only the low few bits of hi*hi are needed, so we can use
;an 8-bit multiply there.  Since the MSB is implied, we need to add in
;two 1*lo products (shifted up 64 bits).  We only need bits 64 - 71 of
;the 130-bit product (the action happens near bit 65).	What we're 
;squaring is root*2, so the result is square*4.  ecx already has arg*4.

	mul	eax			;Low partial product of square
	mov	edi,edx			;Only high half counts
	mov	eax,ebx
	mul	esi			;Middle partial product of square
	add	eax,eax			;There are two of these
	adc	edx,edx
	add	edi,eax
	adc	edx,0			;edx:edi = lo*lo + lo*hi + hi*lo
	add	edx,esi			;lo*implied msb
	add	edx,esi			;lo*implied msb again
	mov	al,bl
	mul	al			;hi*hi - only low 8 bits are valid
	add	al,dl			;Bits 64 - 71 of product
	or	al,1			;Account for sticky bits 0 - 63
	sub	cl,al			;Compare product with argument
;Sign flag set if product is larger.  In this case, subtract 1 from root.
	add	cl,cl			;Set CY if sign is set
SubOneFromRoot:
	sbb	esi,0			;Reduce root if product was too big
	sbb	ebx,0
ShiftRoot:
;ebx:esi = root*2
;Absolute error is in the range (0, -5.421E-20).  This is equivalent to
;less than +1, -0 in last bit.	Thus LSB is correct rounding bit as 
;long as we set a sticky bit below it.
;
;Now divide root*2 by 2, preserving LSB as rounding bit and filling
;eax with 1's as sticky bits.
;
;Avoid RCR because it takes 9 clocks on 386.  Use SHRD (3 clocks) instead.
	mov	eax,-1
	shrd	eax,esi,1		;Move round bit to MSB of eax
	shrd	esi,ebx,1
	shrd	ebx,eax,1		;Shift 1 into MSB of ebx
StoreRoot:
	mov	edi,EMSEG:[CURstk]
	mov	EMSEG:[Result],edi
	mov	ecx,EMSEG:[edi].ExpSgn
;mantissa in ebx:esi:eax, exponent in high ebx, sign in bh bit 7
	jmp	EMSEG:[RoundMode]

HaveRoot:
;esi = eax = edx = 0
	cmp	edi,ebx			;Does quotient == divisor?
	jz	StoreRoot		;If so, we're done
;Quotient != divisor, so answer is not exact.  Since remainder is zero,
;the division was exact.  The only error in the result is e^2/2A, which
;is always positive.  We need the error to be only negative so that
;the rounding routine can properly tell if it rounded up.
	add	ebx,edi			;quotient + divisor (always sets CY)
	jmp	SubOneFromRoot		;Reduce root to ensure negative error
