	subttl	emtrig.asm - Trig functions sine, cosine, tangent
	page
;*******************************************************************************
;	 Copyright (c) Microsoft Corporation 1991
;	 All Rights Reserved
;
;emtrig.asm - Trig functions sine, cosine, tangent
;	by Tim Paterson
;
;Purpose:
;	FCOS, FPTAN, FSIN, FSINCOS instructions
;Inputs:
;	edi = [CURstk]
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


;XPi is the 66-bit value of Pi from the Intel manual
XPiHi		equ	0C90FDAA2H
XPiMid		equ	02168C234H
XPiLo		equ	0C0000000H	;Extension of pi
PiOver4exp	equ	-1		;Pi/4 ~= 3/4, so exponent is -1

TinyAngleExp	equ	-32		;Smallest angle we bother with
MaxAngleExp	equ	63		;Angle that's too big

Trig1Result:
;Trig function reduction routine used by functions returning 1 value
;(FSIN and FCOS)
;edi = [CURstk] = argument pointer
;Argument has already been checked for zero.
;ZF = (tag == bTAG_ZERO)
	jb	TrigPrem
;Tagged special
	mov	al,EMSEG:[edi].bTAG
	cmp	al,bTAG_DEN
	jz	TrigDenorm
	add	sp,4			;Don't return to caller
	cmp	al,bTAG_INF
	jnz	SpclDestNotDen		;Check for Empty or NAN
	mov	EMSEG:[SWcc],C2		;Can't reduce infinity
	jmp	ReturnIndefinite

TrigDenorm:
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is denormal exception masked?
	jnz	TrigPrem		;Yes, continue
	add	sp,4			;Don't return to caller
TrigRet:
	ret


Trig2Inf:
	mov	EMSEG:[SWcc],C2		;Can't reduce infinity
	jmp	Trig2Indefinite

Trig2StackOver:
	mov	EMSEG:[SWcc],C1		;Signal overflow
Trig2StackUnder:
	mov	EMSEG:[CURerr],Invalid+StackFlag
Trig2Indefinite:
	add	sp,4			;Don't return to caller
	call	ReturnIndefinite
	jz	TrigRet			;Unmasked, don't change registers
;Produce masked response
	mov	EMSEG:[CURstk],esi		;Push stack
	mov	edi,esi
	jmp	ReturnIndefinite

Trig2Special:
	cmp	al,bTAG_DEN
	jz	TrigDenorm
	cmp	al,bTAG_INF
	jz	Trig2Inf
;Must be a NAN
	add	sp,4			;Don't return to caller
	call	DestNAN
	jz	TrigRet			;Unmasked, don't change registers
;Produce masked response
	mov	EMSEG:[CURstk],esi		;Push stack
        mov     eax,EMSEG:[edi].ExpSgn
        mov     EMSEG:[esi].ExpSgn,eax
        mov     eax,EMSEG:[edi].lManHi
        mov     EMSEG:[esi].lManHi,eax
        mov     eax,EMSEG:[edi].lManLo
        mov     EMSEG:[esi].lManLo,eax
	ret

Trig2Zero:
	add	sp,4			;Don't return to caller
	mov	EMSEG:[CURstk],esi
	mov	edi,esi
;Amazing coincidence: both FSINCOS and FPTAN return the same result for
;a zero argument:
;	FSINCOS returns ST(0) = cos(0) = 1, ST(1) = sin(0) = 0.
;	FPTAN returns ST(0) = 1 always, ST(1) = tan(0) = 0.
;Return zero has same sign as argument zero, so we don't need to touch
;it -- just push +1.0.
	jmp	ReturnOne

TrigOutOfRange:
	mov	EMSEG:[SWcc],C2		;Signal argument not reduced
	add	sp,4
	ret

PrevStackWrap	esi,Trig2		;Tied to PrevStackElem below

Trig2Result:
;Trig function reduction routine used by functions returning 2 values
;(FSINCOS and FPTAN)
;edi = [CURstk] = argument pointer
	mov	esi,edi
	PrevStackElem	esi,Trig2	;esi points to second result location
	mov	al,EMSEG:[edi].bTAG	;Get tag
	cmp	al,bTAG_EMPTY		;Stack underflow if empty
	jz	Trig2StackUnder
	cmp	EMSEG:[esi].bTAG,bTAG_EMPTY	;Stack overflow if not empty
	jnz	Trig2StackOver
	cmp	al,bTAG_ZERO		;Is it Special?
	ja	Trig2Special
	jz	Trig2Zero
;Fall into TrigPrem

;****
;TrigPrem
;
;This routine reduces an angle in radians to the range [0, pi/4].
;Angles in odd-numbered octants have been subtracted from pi/4.
;It uses a 66-bit value for pi, as required by the 387.
;TrigPrem uses the same two-stage algorithm as FPREM (see 
;emfprem.asm).	However, it is limited to an argument < 2^63.
;
;Inputs:
;	edi = [CURstk]
;Outputs:
;	ebx:esi = remainder, normalized
;	high ecx = exponent, cl = tag
;	al = octant
;	edi = [CURstk]

TrigPrem:
	mov	EMSEG:[Result],edi
	mov	eax,EMSEG:[edi].lManLo
	mov	edx,EMSEG:[edi].lManHi
	movsx	ebx,EMSEG:[edi].wExp
	cmp	ebx,MaxAngleExp
	jge	TrigOutOfRange
	xor	edi,edi			;Extend dividend
	xor	esi,esi			;Quotient, in case we skip stage 1
.erre	PiOver4exp eq -1
	inc	ebx			;Subtract exponent of pi/4
	jl	ExitTrigPrem		;If dividend is smaller, return it.
;We now know that 0 <= ExpDif < 64, so it fits in bl.
	cmp	bl,31			;Do we need to do stage 1?
	jl	FitPi			;No, start stage 2

;FPREM stage 1
;
;Exponent difference is at least 31.  Use 32-bit division to compute
;quotient and exact remainder, reducing exponent difference by 31.
;
;edx:eax = dividend
;ebx = exponent difference

;Shift dividend right one bit to be sure DIV instruction won't overflow
;This means we'll be reducing the exponent difference by 31, not 32
	xor	ebp,ebp			;Dividend extension
	shrd	ebp,eax,1
	shrd	eax,edx,1
	shr	edx,1

	sub	bl,31			;Exponent reduced
	mov	ecx,XPiHi
	div	ecx			;Guess a quotient "digit"

;Check out our guess.  
;Currently, remainder in edx = (high dividend) - (quotient * high pi).
;(High dividend is the upper 64 bits--ebp has 1 bit.)  The definition 
;of remainder is (all dividend) - (quotient * all pi).  So if we
;subtract (quotient * low pi) from edx:ebp, we'll get the true 
;remainder.  If it's negative, our guess was too big.

	mov	esi,eax			;Save quotient
	mov	ecx,edx			;Save remainder

;The pi/4 we use has two bits set below the first 64 bits.  This means
;we must add another 3/4 of the quotient into the amount to subtract,
;which we'll compute by rounding the low 32 bits up 1, then subtracting 
;1/4 of quotient.  But since we're computing the amount to subtract from
;the remainder, we'll add the 1/4 of the quotient to the remainder instead
;of subtracting it from the amount to subtract.

.erre	XPiLo eq (3 shl 30)
	mov	eax,XPiMid+1
	mul	esi			;Quotient * low pi
;Note that ebp is either 0 or 800...00H
	shr	ebp,30			;Move down to low end
	shld	ebp,esi,30		;Move back up, adding 1/4 of quotient
	mov	edi,esi			;Another copy of quotient
	shl	edi,30			;Keep last two bits
;edx:eax has amount to subtract to get correct remainder from ecx:ebp:edi
	sub	ebp,eax
	sbb	ecx,edx			;Subtract from remainder
	mov	eax,ebp
	mov	edx,ecx			;Remainder back to edx:eax:edi
	jnc	TrigPremNorm		;Was quotient OK?
TrigCorrect:
	dec	esi			;Quotient was too big
	add	edi,XPiLo
	adc	eax,XPiMid		;Add divisor back into remainder
	adc	edx,XPiHi
	jnc	TrigCorrect		;Repeat if quotient is still too big
	jmp	TrigPremNorm

;FPREM stage 2
;
;Exponent difference is less than 32.  Use restoring long division to
;compute quotient bits until exponent difference is zero.  Note that we
;often get more than one bit/loop:  BSR is used to scan off leading
;zeros each time around.  Since the divisor is normalized, we can
;instantly compute a zero quotient bit for each leading zero bit.

TrigPremLoop:
;edx:eax:edi = dividend (remainder) minus pi/4
;esi = quotient
;ebx = exponent difference
;
;If D is current dividend and p is pi/4, then we have edx:eax:edi = D - p, 
;which is negative.  We want 2*D - p, which is positive.  
;2*D - p = 2*(D - p) + p.
	add	edi,edi			;2*(D - p)
	adc	eax,eax
	adc	edx,edx

	add	edi,XPiLo		;2*(D-p) + p = 2*D - p
	adc	eax,XPiMid
	adc	edx,XPiHi

	add	esi,esi			;Double quotient too
	dec	ebx			;Decrement exponent difference
PiFit:
	inc	esi
TrigPremNorm:
	bsr	ecx,edx			;Find first 1 bit
	jz	TrigPremZero
	not	cl
	and	cl,1FH			;Convert bit no. to shift count
	sub	ebx,ecx			;Reduce exponent difference
	jl	TrigTooFar
	shld	edx,eax,cl
	shld	eax,edi,cl
	shl	edi,cl			;Finish normalize shift
	shl	esi,cl			;Shift quotient
FitPi:
;Dividend could be larger or smaller than divisor
	sub	edi,XPiLo
	sbb	eax,XPiMid
	sbb	edx,XPiHi
	jnc	PiFit
;Couldn't subtract pi/2 from dividend.	
;edx:eax:edi = dividend - pi/4, which is negative
	or	ebx,ebx			;Is exponent difference zero?
	jg	TrigPremLoop
;If quotient (octant number) is odd, we have subtracted an odd number of
;pi/4's.  However, simple angle reductions work in multiples of pi/2.
;We will keep the extra pi/4 we just subtracted if the octant was odd.
;This will give a result range of [-pi/4, pi/4].  
	test	esi,1			;Is octant odd?
	jz	EvenOctant
NegPremResult:
;-pi/4 < dividend < 0.  Negate this since we use sign-magnitude representation.
	not	edx			;96-bit negation
	not	eax
	neg	edi
	sbb	eax,-1
	sbb	edx,-1
;May need to normalize
	bsr	ecx,edx
	jz	TrigNorm32
	lea	ebx,[ebx+ecx-31]	;Fix up exponent for normalization
	not	cl			;Convert bit no. to shift count
TrigShortNorm:
	shld	edx,eax,cl
	shld	eax,edi,cl
	shl	edi,cl			;Finish normalize shift
RoundPrem:
;Must round 66-bit result to 64 bits.
;To perform "round even" when the round bit is set and the sticky bits
;are zero, we treat the LSB as if it were a sticky bit.  Thus if the LSB
;is set, that will always force a round up (to even) if the round bit is
;set.  If the LSB is zero, then the sticky bits remain zero and we always
;round down.  This rounding rule is implemented by adding RoundBit-1
;(7F..FFH), setting CY if round up.  
	bt	eax,0			;Is mantissa even or odd? (set CY)
	adc	edi,(1 shl 31)-1	;Sum LSB & sticky bits--CY if round up
	adc	eax,0
	adc	edx,0
ExitTrigPrem:
;edx:eax = remainder, normalized
;esi = quotient
;ebx = exponent difference, zero or less
.erre	PiOver4exp eq -1
	dec	ebx			;True exponent
.erre	bTAG_SNGL eq 0
	shrd	ecx,ebx,16		;Exponent to high ecx
	mov	ebx,edx			;High mant. to ebx
	xchg	esi,eax			;Low mant. to esi, octant to eax
	or      esi,esi			;Any bits in low half?
.erre   bTAG_VALID eq 1
.erre   bTAG_SNGL eq 0
	setnz   cl			;if low half==0 then cl=0 else cl=1
	mov	edi,EMSEG:[CURstk]
	test	EMSEG:[edi].bSgn,bSign	;Was angle negative?
	jnz	FlipOct			;Yes, flip octant over
	ret

FlipOct:
;Angle was negative.  Subtract octant from 7.
	neg	al
	add	al,7
	ret

EvenOctant:
;Restore dividend
	add	edi,XPiLo
	adc	eax,XPiMid
	adc	edx,XPiHi
	jmp	RoundPrem

TrigTooFar:
;Exponent difference in ebx went negative when reduced by shift count in ecx.
;We need a quotient corresponding to exponent difference of zero.
	add	ecx,ebx			;Compute previous exponent difference
	shl	esi,cl			;Fix up quotient
	sub	ecx,ebx			;Restore shift count
	test	esi,1			;Is octant odd?
	jz	TrigShortNorm		;No, go normalize
	xor	ebx,ebx			;Restore old exponent difference (zero)
SubPiOver4:
;We are here if exponent difference was zero and octant is odd.
;As noted above, we need to reduce the angle by a multiple of pi/2,
;not pi/4.  We will subtract one more pi/4, which will make the
;result range [-pi/4, pi/4].
	sub	edi,XPiLo
	sbb	eax,XPiMid
	sbb	edx,XPiHi
	jmp	NegPremResult

TrigPremZero:
;High dword of remainder is all zero, so we've reduced exponent difference
;by 32 bits and overshot.  We need a quotient corresponding to exponent 
;difference of zero, so we just shift it by the original difference.  Then
;we need to normalize the rest of the remainder.
	mov	ecx,ebx			;Get exponent difference
	shl	esi,cl			;Fix up quotient
	test	esi,1			;Is octant odd?
	jnz	SubPiOver4		;Yes, go subtract another pi/4
TrigNorm32:
	bsr	ecx,eax
	jz	TinyTrig
	lea	ebx,[ebx+ecx-31-32]	;Fix up exponent for normalization
	mov	edx,eax
	mov	eax,edi			;Shift left by 32 bits
	not	cl			;Convert bit no. to shift count
	shld	edx,eax,cl		;Normalize remainder
	shl	eax,cl
	jmp	ExitTrigPrem

TinyTrig:
;Upper 64 bits of remainder are all zero.  We are assured that the extended
;remainder is never zero, though.
	mov	edx,edi			;Shift left 64 bits
	bsr	ecx,edi
	lea	ebx,[ebx+ecx-31-64]	;Fix up exponent for normalization
	not	cl			;Convert bit no. to shift count
	shl	edx,cl			;Normalize
	jmp	ExitTrigPrem

;*******************************************************************************

EM_ENTRY eFCOS
eFCOS:
    and		[esp].[OldLongStatus+4],NOT(C2 SHL 16)	;clear C2
	cmp	EMSEG:[edi].bTAG,bTAG_ZERO
	jz	ReturnOne
	call	Trig1Result
;ebx:esi,ecx = reduced argument
;eax = octant
	mov	ch,80H			;Assume negative
	test	al,110B			;Negative in octants 2 - 5
	jpo	@F			;Occurs when 1 of these bits are set
	xor	ch,ch			;Actually positve
@@:
	test	al,011B			;Look for octants 0,3,4,7
	jpo	TakeSine		;Use sine if not
TakeCosine:
	cmp	ecx,TinyAngleExp shl 16	;Is angle really small?
	jl	CosReturnOne		;cos(x) = 1 for tiny x
CosNotTiny:
	mov	edi,offset tCosPoly
;Note that argument needs to be saved in ArgTemp (by EvalPolySetup) in case 
;we were called from eFSINCOS and we'll need the arg for the sine.  Argument
;is not needed for cosine, however (just its square).
	call	EvalPolySetup		;In emftran.asm
	mov	ch,EMSEG:[ArgTemp].bSgn	;Get sign we already figured out
TransUnround:
;The last operation performed a simple round nearest, without setting the 
;C1 status bit if round up occured.  We reverse this last rounding now
;so we can do the user's selected rounding mode.  We also ensure that
;the answer is never exact.
	sub	eax,(1 shl 31)-1	;Sum LSB & sticky bits--CY if round up
	jz	UnroundExact		;Answer looks exact, but it's not
	sbb	esi,0
	sbb	ebx,0
	jns	PolyDropExponent	;We had rounded up exponent too
FinalTransRound:
;A jump through [TransRound] is only valid if the number is known not to
;underflow.  Unmasked underflow requires [RoundMode] be set.
	mov	edx,EMSEG:[TransRound]
	mov	EMSEG:[RoundMode],edx
	call	edx			;Perform user's rounding
RestoreRound:
;Restore rounding vectors
	mov	EMSEG:[ZeroVector],offset SaveResult
	mov	eax,EMSEG:[SavedRoundMode]
	mov	EMSEG:[RoundMode],eax
	ret

UnroundExact:
	inc	eax			;Let's say our answer is a bit small
	jmp	FinalTransRound

PolyDropExponent:
	sub	ecx,1 shl 16		;Decrement exponent
	or	ebx,1 shl 31		;Set MSB
	jmp	FinalTransRound


SinRet:
	ret

SaveTinySin:
;Argument in ebx:esi,ecx is small enough so that sin(x) = x, which happens
;when x - x^3/6 = x [or 1 - x^2/6 = 1].  Note that the infinitely precise
;result is slightly less than the argument.  To get the correct answer for
;any rounding mode, we decrement the argument and set up for rounding.
	mov	eax,-1			;Set up rounding bits
	sub	esi,1
	sbb	ebx,0			;Drop mantissa by one
	js	FinalTransRound		;Still normalized?
;mantissa must have been 800..000H, set it to 0FFF...FFFH and drop exponent
	mov	ebx,eax			;ebx = -1
	sub	ecx,1 shl 16		;Drop exponent by one
	jmp	FinalTransRound


EM_ENTRY eFSIN
eFSIN:
    and		[esp].[OldLongStatus+4],NOT(C2 SHL 16)	;clear C2
	cmp	EMSEG:[edi].bTAG,bTAG_ZERO
	jz	SinRet			;Return zero for zero argument
	call	Trig1Result
	mov	ch,al
	shl	ch,7-2			;Move bit 2 to bit 7 as sign bit
ReducedSine:
;ebx:esi,ecx = reduced argument
;ch = correct sign
;eax = octant
	test	al,011B			;Look for octants 0,3,4,7
	jpo	TakeCosine		;Use cosine if not
TakeSine:
	cmp	ecx,TinyAngleExp shl 16	;Is angle really small?
	jl	SaveTinySin		;sin(x) = x for tiny x

;The polynomial for sine is  sin(x) = x * P(x^2).  However, the degree zero
;coefficient of P() is 1, so  P() = R() + 1, where R() has no degree zero
;term.	Thus  sin(x) = x * [R(x^2) + 1] = x * R(x^2) + x.
;
;What's important here is that adding 1 to R(x^2) can blow away a lot of
;precision just before we do that last multiply by x.  Note that x < pi/4 < 1,
;so that x^2 is often << 1.  The precision is lost when R(x^2) is shifted
;right to align its binary point with 1.0.  This can cause a loss of at
;least 1 bit of precision after the final multiply by x in addition to 
;rounding errors.
;
;To avoid this precision loss, we use the alternate form given above,
;sin(x) = x * R(x^2) + x.  Instead of adding 1.0 and multiplying by x,
;we multiply by x and add x--exactly the same level of difficulty.  But
;the mulitply has all of R(x^2)'s precision available.
;
;Because the polynomial R() has no zero-degree term, we give EvalPoly
;one degree less (so we don't have to add zero as the last term).
;Then we have to multiply once more by x^2 since we left the loop early.

SineNotTiny:
	mov	edi,offset tSinPoly
	call	EvalPolySetup		;In emftran.asm
SineFinish:

ifdef NT386
        mov	edi,YFloatTemp
else
	mov	edi,offset edata:FloatTemp
endif
	call	PolyMulDouble		;Last coefficient in R(x^2)

ifdef NT386
	mov	edi,YArgTemp		;Point to original x
else
	mov	edi,offset edata:ArgTemp ;Point to original x
endif

	call	PolyMulDouble		;Compute x * R(x^2)

ifdef NT386
	mov	edi,YArgTemp		;Point to original x
else
	mov	edi,offset edata:ArgTemp ;Point to original x
endif

	push	offset TransUnround
	jmp	PolyAddDouble		;Compute x * R(x^2) + x


EM_ENTRY eFPTAN
eFPTAN:
    and		[esp].[OldLongStatus+4],NOT(C2 SHL 16)	;clear C2
	call	Trig2Result
	push	offset TanPushOne	; Push 1.0 when we're all done
;ebx:esi,ecx = reduced argument
;eax = octant
	mov	ch,al
	shl	ch,7-1			;Move bit 1 to bit 7 as sign bit
;Note that ch bit 6 now has even/odd octant, which we'll need when we're
;done to see if we should take reciprocal.
	cmp	ecx,TinyAngleExp shl 16	;Is angle really small?
	jl	TinyTan
	mov	edi,offset tTanPoly
	call	Eval2Poly		;In emftran.asm
	mov	edi,EMSEG:[CURstk]	;Point to first result
	push	offset TransUnround	;Return address of divide
	test	EMSEG:[ArgTemp].bSgn,0C0H	;Check low 2 bits of octant
;Given the reduced input range, the result can never overflow or underflow.
;It is must then be safe to assume neither operand is zero.
	jpe	DivDouble		;Tan() octants 0,3,4,7
	jmp	DivrDouble		;CoTan()

TinyTan:
	test	ch,0C0H			;Check low 2 bits of octant
	jpe	SaveTinySin		;Octants 0,3,4,7: tan(x) = x for tiny x
;Need reciprocal of reduced argument
	mov	edi,esi
	mov	esi,ebx			;Mantissa in esi:edi
	mov	ebx,ecx			;ExpSgn to ebx
	mov	edx,1 shl 31		;Load 1.0
	xor	eax,eax
.erre	TexpBias eq 0
	xor	ecx,ecx			;Sign and exponent are zero
;dividend mantissa in edx:eax, exponent in high ecx, sign in ch bit 7
;divisor mantissa in esi:edi, exponent in high ebx, sign in bh bit 7
	push	offset TransUnround	;Return address of divide
;Note that this can never overflow, because the reduced argument is never
;smaller than about 2^-65.
	jmp	DivDoubleReg


PrevStackWrap	edi,Tan			;Tied to PrevStackElem below

TanPushOne:
	PrevStackElem	edi,Tan		;edi points to second result location
	mov	EMSEG:[CURstk],edi
ReturnOne:
	mov	EMSEG:[edi].lManLo,0
	mov	EMSEG:[edi].lManHi,1 shl 31
	mov	EMSEG:[edi].ExpSgn,(0-TexpBias) shl 16 + bTAG_SNGL
	ret


PrevStackWrap	edi,SinCos		;Tied to PrevStackElem below

eFSINCOS:
    and		[esp].[OldLongStatus+4],NOT(C2 SHL 16)	;clear C2
	call	Trig2Result
;Figure out signs
	mov	ch,al			;Start with sign of sine
	shl	ch,7-2			;Move bit 2 to bit 7 as sign bit
	mov	ah,80H			;Assume sign of cosine is negative
	test	al,110B			;Negative in octants 2 - 5
	jpo	@F			;Occurs when 1 of these bits are set
	xor	ah,ah			;Actually positve
@@:
;ch = sign of sine
;ah = sign of cosine
	cmp	ecx,TinyAngleExp shl 16	;Is angle really small?
	jl	TinySinCos
	push	eax			;Save octant and sign of cosine
	call	ReducedSine		;On exit, edi = [CURstk]
	pop	eax
;The Sin() funcion restored the rounding vectors to normal.  Set them back.
	mov	EMSEG:[RoundMode],offset PolyRound
	mov	EMSEG:[ZeroVector],offset PolyZero
	PrevStackElem	edi,SinCos	;edi points to second result location
	mov	EMSEG:[CURstk],edi
	mov	EMSEG:[Result],edi
;Load x^2 back into registers
	mov	ecx,EMSEG:[FloatTemp].ExpSgn
	mov	ebx,EMSEG:[FloatTemp].lManHi
	mov	esi,EMSEG:[FloatTemp].lManLo
	mov	EMSEG:[ArgTemp].bSgn,ah	;Save sign
	test	al,011B			;Look for octants 0,3,4,7
	jpo	FastSine		;Use sine if not
	mov	edi,offset tCosPoly
	call	EvalPoly		;In emftran.asm
	mov	ch,EMSEG:[ArgTemp].bSgn	;Get sign we already figured out
	jmp	TransUnround

FastSine:
	mov	edi,offset tSinPoly
	push	offset SineFinish
	jmp	EvalPoly		;In emftran.asm

TinySinCos:
;ch = sign of sine
;ah = sign of cosine
;ebx:esi,high ecx = reduced argument
;edi = [CURstk]
	test	al,011B			;Look for octants 0,3,4,7
	jpo	TinyCosSin		;Take cosine first if not
	push	eax
	call	SaveTinySin		;For sine, arg is result
	pop	ecx
;edi = [CURstk]
;ch = sign of cosine
;Set cosine to 1.0
	PrevStackElem	edi,TinySinCos	;edi points to second result location
	mov	EMSEG:[CURstk],edi
	mov	EMSEG:[Result],edi
CosReturnOne:
;Cosine is nearly equal to 1.0.  Put in next smaller value and round it.
	mov	ebx,-1
	mov	esi,ebx			;Set mantissa to -1
	mov	eax,ebx			;Set up rounding bits
.erre	TexpBias eq 0
	and	ecx,bSign shl 8		;Keep only sign
	sub	ecx,1 shl 16		;Exponent of -1
;A jump through [TransRound] is only valid if the number is known not to
;underflow.  Unmasked underflow requires [RoundMode] be set.
	jmp	EMSEG:[TransRound]

	PrevStackWrap	edi,TinySinCos

	PrevStackWrap	edi,TinyCosSin

TinyCosSin:
;Sine is nearly 1.0, cosine is argument
;
;ch = sign of sine
;ah = sign of cosine
;ebx:esi,high ecx = reduced argument
;edi = [CURstk]
	xchg	ah,ch			;Cosine sign to ch, sine sign to ah
	push	edi			;Save place for sine
	PrevStackElem	edi,TinyCosSin	;edi points to second result location
	mov	EMSEG:[CURstk],edi
	mov	EMSEG:[Result],edi
	push	eax
	call	SaveTinySin		;For sine, arg is result
	pop	ecx
;ch = sign of sine
	pop	EMSEG:[Result]		;Set up location for sine
	jmp	CosReturnOne

;*******************************************************************************

;********************* Polynomial Coefficients *********************

;These polynomial coefficients were all taken from "Computer Approximations"
;by J.F. Hart (reprinted 1978 w/corrections).  All calculations and 
;conversions to hexadecimal were done with a character-string calculator
;written in Visual Basic with precision set to 30 digits.  Once the constants
;were typed into this file, all transfers were done with cut-and-paste
;operations to and from the calculator to help eliminate any typographical
;errors.


tCosPoly	label	word

;These constants are derived from Hart #3824: cos(x) = P(x^2),
;accurate to 19.45 digits over interval [0, pi/4].  The original 
;constants in Hart required that the argument x be divided by pi/4.  
;These constants have been scaled so this is no longer required.
;Scaling is done by multiplying the constant by a power of 4/pi.
;The power is given in the table.

	dd	7			;Degree seven

;  Original Hart constant	      power	Scaled constant
;
;-0.38577 62037 2		 E-12  14  -0.113521232057839395845871741043E-10
;Hex value:    0.C7B56AF786699CF1BD13FD290 HFFDC
	dq	0C7B56AF786699CF2H
	dw	(bSign shl 8)+bTAG_VALID,0FFDCH-1

;+0.11500 49702 4263		  E-9  12  +0.208755551456778828747793797596E-8
;Hex value:    0.8F74AA3CCE49E68D6F5444A18 HFFE4
	dq	08F74AA3CCE49E68DH
	dw	bTAG_VALID,0FFE4H-1

;-0.24611 36382 63700 5		  E-7  10  -0.275573128656960822243472872247E-6
;Hex value:    0.93F27B7F10CC8A1703EFC8A04 HFFEB
	dq	093F27B7F10CC8A17H
	dw	(bSign shl 8)+bTAG_VALID,0FFEBH-1

;+0.35908 60445 88581 953	  E-5	8  +0.248015872828994630247806807317E-4
;Hex value:    0.D00D00CD6BB3ECD17E10D5830 HFFF1
	dq	0D00D00CD6BB3ECD1H
	dw	bTAG_VALID,0FFF1H-1

;-0.32599 18869 26687 55044	  E-3	6  -0.138888888888589604343951947246E-2
;Hex value:    0.B60B60B609B165894CFE522AC HFFF7
	dq	0B60B60B609B16589H
	dw	(bSign shl 8)+bTAG_VALID,0FFF7H-1

;+0.15854 34424 38154 10897 54	  E-1	4  +0.416666666666664302573692446873E-1
;Hex value:    0.AAAAAAAAAAA99A1AF53042B08 HFFFC
	dq	0AAAAAAAAAAA99A1BH
	dw	bTAG_VALID,0FFFCH-1

;-0.30842 51375 34042 45242 414	  E0	2  -0.499999999999999992843582920899E0
;Hex value:    0.FFFFFFFFFFFFFEF7F98D3BFA8 HFFFF
	dq	0FFFFFFFFFFFFFEF8H
	dw	(bSign shl 8)+bTAG_VALID,0FFFFH-1

;+0.99999 99999 99999 99996 415	  E0	0  (no change)
;Hex value     0.FFFFFFFFFFFFFFFF56B402618 H0
	dq	0FFFFFFFFFFFFFFFFH
	dw	bTAG_VALID,00H-1


tSinPoly	label	word

;These constants are derived from Hart #3044: sin(x) = x * P(x^2),
;accurate to 20.73 digits over interval [0, pi/4].  The original 
;constants in Hart required that the argument x be divided by pi/4.  
;These constants have been scaled so this is no longer required.
;Scaling is done by multiplying the constant by a power of 4/pi.
;The power is given in the table.

	dd	7-1			;Degree seven, but the last coefficient
					;is 1.0 and is not listed here.

;  Original Hart constant	      power	Scaled constant
;
;-0.20225 31292 93		 E-13  15  -0.757786788401271156262125540409E-12
;Hex value:    0.D54C4AF2B524F0F2D6411C90A HFFD8
	dq	0D54C4AF2B524F0F3H
	dw	(bSign shl 8)+bTAG_VALID,0FFD8H-1

;+0.69481 52035 0522		 E-11  13  +0.160583476232246065559545749398E-9
;Hex value:    0.B0903AF085DA66030F16E43BC HFFE0
	dq	0B0903AF085DA6603H
	dw	bTAG_VALID,0FFE0H-1

;-0.17572 47417 61708 06	  E-8  11  -0.250521047382673309542092418731E-7
;Hex value:    0.D73229320D2AF05971AC96FF4 HFFE7
	dq	0D73229320D2AF059H
	dw	(bSign shl 8)+bTAG_VALID,0FFE7H-1

;+0.31336 16889 17325 348	  E-6	9  +0.275573192133901687156480447942E-5
;Hex value:    0.B8EF1D2984D2FBA28A9CC9DEE HFFEE
	dq	0B8EF1D2984D2FBA3H
	dw	bTAG_VALID,0FFEEH-1

;-0.36576 20418 21464 00052 9	  E-4	7  -0.198412698412531058609618529749E-3
;Hex value:    0.D00D00D00C3FDDD7916E5CB28 HFFF4
	dq	0D00D00D00C3FDDD8H
	dw	(bSign shl 8)+bTAG_VALID,0FFF4H-1

;+0.24903 94570 19271 62752 519	  E-2	5  +0.83333333333333203341753387264E-2
;Hex value:    0.8888888888884C95D619A0343 HFFFA
	dq	08888888888884C96H
	dw	bTAG_VALID,0FFFAH-1

;-0.80745 51218 82807 81520 2582  E-1	3  -0.166666666666666666281276062229E0
;Hex value:    0.AAAAAAAAAAAAAA8E3AD80EAB8 HFFFE
	dq	0AAAAAAAAAAAAAA8EH
	dw	(bSign shl 8)+bTAG_VALID,0FFFEH-1

;+0.78539 81633 97448 30961 41845 E0	1  +0.99999999999999999999812025812E0
;Hex value:    0.FFFFFFFFFFFFFFFFF71F88110 H0
;	dq	8000000000000000H	;This constant of 1.0 omitted here.
;	dw	bTAG_VALID,0		;   It is handled in code.


tTanPoly	label	word

;These constants are derived from Hart #4286: tan(x) = x * P(x^2) / Q(x^2),
;accurate to 19.94 digits over interval [0, pi/4].  The original 
;constants in Hart required that the argument x be divided by pi/4.  
;These constants have been scaled so this is no longer required.
;Scaling is done by multiplying the constant by the same power of 4/pi
;as the power of x the constant is used on.  However, the highest
;degree coefficient of Q() is 1, and after scaling this way it would
;become (4/pi)^8.  In order to keep this coefficient equal to one,
;we scale everything again by (pi/4)^8.  This scaling is partially
;canceled by the original scaling by powers of 4/pi, and the net
;resulting power of pi/4 is given in the table.


	dd	3			;First poly is degree 3

;  Original Hart constant	        power	Scaled constant
;
;-.45649 31943 86656 31873 96113 7    E2  1  -35.8528916474714232910463077546
;Hex value:    0.8F695C6D93AF6F97B6E022AB3 H6
        dq      08F695C6D93AF6F98H
        dw      (bSign shl 8)+bTAG_VALID,06H-1

;+.14189 85425 27617 78388 00394 831  E5  3  +6874.60229709782436592720603503
;Hex value:    0.D6D4D181240D0D08C88DF4AA6 HD
        dq      0D6D4D181240D0D09H
        dw      bTAG_VALID,0DH-1

;-.89588 84400 67680 41087 29639 541  E6  5  -267733.884797157298951145495276
;Hex value:    0.82BABC504220C62B1D0722684 H13
        dq      082BABC504220C62BH
        dw      (bSign shl 8)+bTAG_VALID,013H-1

;+.10888 60043 72816 87521 38857 983  E8  7  +2007248.9111748838841548144685
;Hex value:    0.F506874A160EB9C0994AADD6A H15
        dq      0F506874A160EB9C1H
        dw      bTAG_VALID,015H-1



	dd	4			;Second poly is degree 4
;NOTE: Eval2Poly assumes the first coefficient is 1.0, so it is omitted

;  Original Hart constant	        power	Scaled constant
;
;-.10146 56190 25288 53387 54401 947  E4  2  -625.890950057027419879480354834
;Hex value:    0.9C790553635355A95241A5324 HA
        dq      09C790553635355A9H
        dw      (bSign shl 8)+bTAG_VALID,0AH-1

;+.13538 27128 05119 09382 89294 872  E6  4  +51513.6992033752080924797647367
;Hex value:    0.C939B2FEFE0DC585E649870FE H10
        dq      0C939B2FEFE0DC586H
        dw      bTAG_VALID,010H-1

;-.39913 09518 03516 51504 43427 94   E7  6  -936816.855188785264866481436899
;Hex value:    0.E4B70DAEDA6F89E5A7CE626FA H14
        dq      0E4B70DAEDA6F89E6H
        dw      (bSign shl 8)+bTAG_VALID,014H-1

;+.13863 79666 35676 29165 33913 361  E8  8  +2007248.91117488388417770850458
;Hex value:    0.F506874A160EB9C0CCD8313BC H15
        dq      0F506874A160EB9C1H
        dw      bTAG_VALID,015H-1
