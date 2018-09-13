        subttl  emround.asm - Rounding and Precision Control and FRNDINT
        page
;*******************************************************************************
;emround.asm - Rounding and Precision Control
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Purpose:
;       Rounding and precision control.  The correct routine is jumped
;	to through the [RoundMode] vector.
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;	02/28/92  JWM   Minor bug fix in NotNearLow
;
;*******************************************************************************


RndIntSpcl:
	cmp	cl,bTAG_INF
	jz	RndIntX			;Leave infinity unchanged
	cmp	cl,bTAG_DEN
	jnz	SpclDestNotDen		;Handle NAN & empty - in emarith.asm
;Handle denormal
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is it masked?
	jnz	NormRndInt		;If so, ignore denormalization
RndIntX:
	ret

;********
EM_ENTRY eFRNDINT
eFRNDINT:
;********
;edi points to top of stack
	mov	ecx,EMSEG:[edi].ExpSgn
	cmp	cl,bTAG_ZERO
.erre	bTAG_VALID lt bTAG_ZERO
.erre	bTAG_SNGL lt bTAG_ZERO
	jz	RndIntX	
	ja	RndIntSpcl
	cmp	ecx,63 shl 16		;Is it already integer?
	jge	RndIntX
NormRndInt:
	mov	ebx,EMSEG:[edi].lManHi
	mov	esi,EMSEG:[edi].lManLo
	mov	EMSEG:[Result],edi	;Save result pointer
	xor	eax,eax			;Extend mantissa
	push	offset SaveResult
	jmp	RoundToBit

;*******************************************************************************

ResultOverflow:
;mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl.
;We were all ready to save the rounded result, but the exponent turned out
;to be too large.
	or	EMSEG:[CURerr],Overflow
	sub	ecx,UnderBias shl 16	;Unmasked response
	test	EMSEG:[CWmask],Overflow	;Is exception unmasked?
	jz	SaveResult		;If so, we're ready
;Produce masked overflow response
	mov	ebx,1 shl 31		;Assume infinity
	xor	esi,esi
	mov	cl,bTAG_INF
	mov	al,EMSEG:[CWcntl]	;Get rounding control
	mov	ah,al
	and	ah,RCchop			;Rounding control only
;Return max value if RCup bit = 1 and -, or RCdown bit = 1 and +
;i.e., RCup & sign OR RCdown & not sign
.erre	RCchop eq RCup + RCdown		;Always return max value
.erre	RCnear eq 0			;Never return max value
	sar	ch,7			;Expand sign through whole byte
.erre	(RCdown and bSign) eq 0		;Don't want to change real sign
	xor	ch,RCdown		;Flip sign for RCdown bit
	and	ah,ch			;RCup & sign  OR  RCdown & not sign
	jnz	SaveMax
	and	ecx,0FFFFH
	or	ecx,TexpMax shl 16
	jmp	SaveResult		;Save Infinity
SaveMax:
;Get max value for current precision
	mov	ebx,0FFFFFF00H		;Max value for 24 bits
	and	ecx,bSign shl 8		;Preserve only sign
	or	ecx,(IexpMax-IexpBias-1) shl 16 + bTAG_VALID ;Set up max value
	and	al,PrecisionControl
.erre	PC24 eq 0
	jz	SaveResult		;Save 24-bit max value
	dec	esi			;esi == -1
	mov	ebx,esi
	cmp	al,PC53
	jnz	SaveResult		;Save 64-bit max value
	mov	esi,0FFFFF800H
	jmp	SaveResult		;Save 53-bit max value

;*******************************************************************************
;
;64-bit rounding routines
;

;***********
Round64down:
;***********
	cmp	ecx,(IexpMin-IexpBias+1) shl 16	;Test for Underflow
	jl	RndDenorm64
	or	eax,eax			;Exact result?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision 	;Set flag on inexact result
;Chop if positive, increase mantissa if negative
	test	ch,bSign
	jz	SaveValidResult		;Positive, so chop
	jmp	RoundUp64		;Round up if negative

RndDenorm64:
	test	EMSEG:[CWmask],Underflow ;Is exception unmasked?
	jz	RndSetUnder
Denormalize:
;We don't really store in denormalized format, but we need the number 
;to be rounded as if we do.  If the exponent were -IexpBias, we would
;lose 1 bit of precision; as it gets more negative, we lose more bits.
;We'll do this by adjusting the exponent so that the bits we want to 
;keep look like integer bits, and performing round-to-integer.
	add	ecx,(IexpBias+62) shl 16 ;Adjust exponent so we're integer
	call	RoundToBit
;Set underflow exception if precision exception is set
	mov	al,EMSEG:[CURerr]
	and	al,Precision
	ror	al,Precision-Underflow	;Move Precision bit to Underflow pos.
	or	EMSEG:[CURerr],al	;Signal Underflow if inexact
	cmp	cl,bTAG_ZERO
	jz	SaveResult
	sub	ecx,(IexpBias+62) shl 16;Restore unbiased exponent
	cmp	ecx,TexpMin shl 16	;Did we round out of denorm?
	jae	SaveResult
	mov	cl,bTAG_DEN
	jmp	SaveResult

RndSetUnder:
;Underflow exception not masked.  Adjust exponent and try again.
	or	EMSEG:[CURerr],Underflow
	add	ecx,UnderBias shl 16
	jmp	EMSEG:[RoundMode]	;Try again with revised exponent

;***********
Round64near:
;***********
;mantissa in ebx:esi:eax, exponent in high ecx, sign in ch bit 7
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm64
	or	eax,eax			;Exact result?
	jz	short SaveValidResult
	or	EMSEG:[CURerr],Precision ;Set flag on inexact result

;To perform "round even" when the round bit is set and the sticky bits
;are zero, we treat the LSB as if it were a sticky bit.  Thus if the LSB
;is set, that will always force a round up (to even) if the round bit is
;set.  If the LSB is zero, then the sticky bits remain zero and we always
;round down.  This rounding rule is implemented by adding RoundBit-1
;(7F..FFH), setting CY if round up.  

	bt	esi,0			;Is mantissa even or odd? (set CY)
	adc	eax,(1 shl 31)-1	;Sum LSB & sticky bits--CY if round up
	jnc	SaveValidResult
RoundUp64:
	mov	EMSEG:[SWcc],RoundUp
	add	esi,1
	adc	ebx,0
	jc	BumpExponent		;Overflowed, increment exponent

SaveValidResult:			;A jump to here requires 9 clocks
	or	esi,esi			;Any bits in low half?
.erre	bTAG_VALID eq 1
.erre	bTAG_SNGL eq 0
	setnz   cl                      ;if low half==0 then cl=0 else cl=1
	cmp	ecx,TexpMax shl 16	;Test for overflow
	jge	ResultOverflow

SaveResult:				;A jump to here requires 10 clocks
;mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl
	mov	edi,EMSEG:[Result]
SaveResultEdi:
	mov	EMSEG:[edi].lManLo,esi
	mov	EMSEG:[edi].lManHi,ebx
SaveExpSgn:
	mov	EMSEG:[edi].ExpSgn,ecx
	ret

;***********
Round64up:
;***********
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm64
	or	eax,eax			;Exact result?
	jz	short SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
;Chop if negative, increase mantissa if positive
	cmp	ch,bSign		;No CY iff sign bit is set
	jc	RoundUp64		;Round up if positive
	jmp	short SaveValidResult

;***********
Round64chop:
;***********
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm64
	or	eax,eax			;Exact result?
	jz	short SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
	jmp	short SaveValidResult

;*******************************************************************************
;
;53-bit rounding routines
;

;***********
Round53down:
;***********
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm53
	mov	edx,esi			;Get low bits
	and	edx,(1 shl 11) - 1	;Mask to last 11 bits
	or	edx,eax			;Throwing away any bits?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
;Chop if positive, increase mantissa if negative
	and	esi,not ((1 shl 11)-1)	;Mask off low 11 bits
	test	ch,bSign
	jz	SaveValidResult		;Positive, go chop
	jmp	RoundUp53

RndDenorm53:
	test	EMSEG:[CWmask],Underflow;Is exception unmasked?
	jz	RndSetUnder
;We don't really store in denormalized format, but we need the number 
;to be rounded as if we do.  If the exponent were -IexpBias, we would
;lose 1 bit of precision; as it gets more negative, we lose more bits.
;We'll do this by adjusting the exponent so that the bits we want to 
;keep look like integer bits, and performing round-to-integer.
	add	ecx,(IexpBias+51) shl 16 ;Adjust exponent so we're integer
	call	RoundToBit
;Set underflow exception if precision exception is set
	mov	al,EMSEG:[CURerr]
	and	al,Precision
	ror	al,Precision-Underflow	;Move Precision bit to Underflow pos.
	or	EMSEG:[CURerr],al	;Signal Underflow if inexact
	cmp	cl,bTAG_ZERO
	jz	SaveResult
	sub	ecx,(IexpBias+51) shl 16;Restore unbiased exponent
	cmp	ecx,(IexpMin-IexpBias+1) shl 16	;Did we round out of denorm?
	jae	SaveResult
	mov	cl,bTAG_DEN
	jmp	SaveResult

;***********
Round53near:
;***********
;mantissa in ebx:esi:eax, exponent in high ecx, sign in ch bit 7
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm53
	mov	edx,esi			;Get low bits
	and	edx,(1 shl 11) - 1	;Mask to last 11 bits
	or	edx,eax			;Throwing away any bits?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result

;To perform "round even" when the round bit is set and the sticky bits
;are zero, we treat the LSB as if it were a sticky bit.  Thus if the LSB
;is set, that will always force a round up (to even) if the round bit is
;set.  If the LSB is zero, then the sticky bits remain zero and we always
;round down.

	mov	edx,esi
	and	esi,not ((1 shl 11)-1)	;Mask off low 11 bits
	test	edx,1 shl 10		;Is round bit set?
	jz	SaveValidResult
	and	edx,(3 shl 10)-1	;Keep only sticky bits and LSB
	or	eax,edx			;Combine with other sticky bits
	jz	SaveValidResult
RoundUp53:
	mov	EMSEG:[SWcc],RoundUp
	add	esi,1 shl 11		;Round
	adc	ebx,0
	jnc	SaveValidResult
BumpExponent:
	add	ecx,1 shl 16		;Mantissa overflowed, bump exponent
	or	ebx,1 shl 31		;Set MSB
	jmp	SaveValidResult

;***********
Round53up:
;***********
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm53
	mov	edx,esi			;Get low bits
	and	edx,(1 shl 11) - 1	;Mask to last 11 bits
	or	edx,eax			;Throwing away any bits?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
;Chop if negative, increase mantissa if positive
	and	esi,not ((1 shl 11)-1)	;Mask off low 11 bits
	test	ch,bSign
	jz	RoundUp53		;Round up if positive
	jmp	SaveValidResult

;***********
Round53chop:
;***********
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm53
	mov	edx,esi			;Get low bits
	and	edx,(1 shl 11) - 1	;Mask to last 11 bits
	or	edx,eax			;Throwing away any bits?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
	and	esi,not ((1 shl 11)-1)	;Mask off low 11 bits
	jmp	SaveValidResult

;*******************************************************************************
;
;24-bit rounding routines
;

;***********
Round24down:
;***********
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm24
	or	eax,esi			;Low dword is just sticky bits
	mov	edx,ebx			;Get low bits
	and	edx,(1 shl 8) - 1	;Mask to last 8 bits
	or	edx,eax			;Throwing away any bits?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
;Chop if positive, increase mantissa if negative
	xor	esi,esi
	and	ebx,not ((1 shl 8)-1)	;Mask off low 8 bits
	test	ch,bSign
	jz	SaveValidResult		;Chop if positive
	jmp	RoundUp24

RndDenorm24:
	test	EMSEG:[CWmask],Underflow;Is exception unmasked?
	jz	RndSetUnder
;We don't really store in denormalized format, but we need the number 
;to be rounded as if we do.  If the exponent were -IexpBias, we would
;lose 1 bit of precision; as it gets more negative, we lose more bits.
;We'll do this by adjusting the exponent so that the bits we want to 
;keep look like integer bits, and performing round-to-integer.
	add	ecx,(IexpBias+22) shl 16 ;Adjust exponent so we're integer
	call	RoundToBit
;Set underflow exception if precision exception is set
	mov	al,EMSEG:[CURerr]
	and	al,Precision
	ror	al,Precision-Underflow	;Move Precision bit to Underflow pos.
	or	EMSEG:[CURerr],al	;Signal Underflow if inexact
	cmp	cl,bTAG_ZERO
	jz	SaveResult
	sub	ecx,(IexpBias+22) shl 16;Restore unbiased exponent
	cmp	ecx,(IexpMin-IexpBias+1) shl 16	;Did we round out of denorm?
	jae	SaveResult
	mov	cl,bTAG_DEN
	jmp	SaveResult

;***********
Round24near:
;***********
;mantissa in ebx:esi:eax, exponent in high ecx, sign in ch bit 7
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm24
	or	eax,esi			;Low dword is just sticky bits
	mov	edx,ebx			;Get low bits
	and	edx,(1 shl 8) - 1	;Mask to last 8 bits
	or	edx,eax			;Throwing away any bits?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
	xor	esi,esi

;To perform "round even" when the round bit is set and the sticky bits
;are zero, we treat the LSB as if it were a sticky bit.  Thus if the LSB
;is set, that will always force a round up (to even) if the round bit is
;set.  If the LSB is zero, then the sticky bits remain zero and we always
;round down.  

	mov	edx,ebx
	and	ebx,not ((1 shl 8)-1)	;Mask off low 8 bits
	test	dl,1 shl 7		;Round bit set?
	jz	SaveValidResult
	and	edx,(3 shl 7)-1		;Mask to LSB and sticky bits
	or	eax,edx			;Combine all sticky bits
	jz	SaveValidResult
RoundUp24:
	mov	EMSEG:[SWcc],RoundUp
	add	ebx,1 shl 8
	jnc	SaveValidResult
	jmp	BumpExponent		;Overflowed, increment exponent

;***********
Round24up:
;***********
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm24
	or	eax,esi			;Low dword is just sticky bits
	mov	edx,ebx			;Get low bits
	and	edx,(1 shl 8) - 1	;Mask to last 8 bits
	or	edx,eax			;Throwing away any bits?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
;Chop if negative, increase mantissa if positive
	xor	esi,esi
	and	ebx,not ((1 shl 8)-1)	;Mask off low 8 bits
	test	ch,bSign
	jz	RoundUp24		;Round up if positive
	jmp	SaveValidResult

;***********
Round24chop:
;***********
	cmp	ecx,TexpMin shl 16	;Test for Underflow
	jl	RndDenorm24
	or	eax,esi			;Low dword is just sticky bits
	mov	edx,ebx			;Get low bits
	and	edx,(1 shl 8) - 1	;Mask to last 8 bits
	or	edx,eax			;Throwing away any bits?
	jz	SaveValidResult
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
	xor	esi,esi
	and	ebx,not ((1 shl 8)-1)	;Mask off low 8 bits
	jmp	SaveValidResult

;*******************************************************************************

;*** RoundToInteger
;
;This routine is used by FISTP Int64 and BSTP.  Unlike RoundToBit, this
;unnormalizes the number into a 64-bit integer.
;
;Inputs:
;	edi = pointer to number to round in stack
;Outputs:
;	CY set if invalid operation
;	ebx:edi = rounded integer if CY clear
;	ch = sign if CY clear
;Note:
;	FIST/FISTP/BSTP exception rules are used:  If the number is too big,
;	Invalid Operation occurs.  Denormals are ignored.
;
;esi preserved

RoundSpcl64Int:
	cmp	cl,bTAG_DEN
	jz	NormRound64Int		;Ignore denormal
	cmp	cl,bTAG_EMPTY
	jnz	RoundInvalid		;All other specials are invalid
	mov	EMSEG:[CURerr],StackFlag+Invalid
	stc				;Flag exception to caller
	ret

RoundInvalid:
;Overflow on integer store is invalid according to IEEE
	mov	EMSEG:[CURerr],Invalid
	stc				;Flag exception to caller
	ret

RoundToInteger:
	mov	ebx,EMSEG:[edi].lManHi
	mov	ecx,EMSEG:[edi].ExpSgn
	mov	edi,EMSEG:[edi].lManLo
;mantissa in ebx:edi, exponent in high ecx, sign in ch bit 7, tag in cl
	mov	al,ch			;Save sign bit
	cmp	cl,bTAG_ZERO
.erre	bTAG_VALID lt bTAG_ZERO
.erre	bTAG_SNGL lt bTAG_ZERO
	jz	RoundIntX		;Just return zero
	ja	RoundSpcl64Int
NormRound64Int:
	xor	edx,edx
	sar	ecx,16			;Bring exponent down
	cmp	ecx,-1			;Is it less than 1?
	jle	Under64Int
	cmp	ecx,63
	jg	RoundInvalid
	sub	ecx,63
	neg	ecx			;cl = amount to shift right
	mov	ch,al			;Get sign out of al
	xor	eax,eax
	cmp	cl,32			;Too big for one shift?
	jl	ShortShft64
;32-bit shift right
	xchg	edx,edi
	xchg	ebx,edi			;ebx=0 now
	shrd	eax,edx,cl
;Max total shift is 63 bits, so we know that the LSB of eax is still zero.
;We can rotate this zero to the MSB so the sticky bits in eax can be combined
;with those in edx without affecting the rounding bit in the MSB of edx.
	ror	eax,1			;MSB is now zero
ShortShft64:
;Shift count in cl is modulo-32
	shrd	edx,edi,cl
	shrd	edi,ebx,cl
	shr	ebx,cl
	or	edx,eax			;Collapse sticky bits into one dword
	jz	RoundIntX		;No sticky or round bits, so don't round
;Result will not be exact--check rounding mode
Round64Int:
	mov	EMSEG:[CURerr],Precision;Set flag on inexact result
	test	EMSEG:[CWcntl],RoundControl	;Check rounding control bits
.erre	RCnear eq 0
	jnz	NotNearest64Int		;Not just round-to-nearest

;To perform "round even" when the round bit is set and the sticky bits
;are zero, we treat the LSB as if it were a sticky bit.  Thus if the LSB
;is set, that will always force a round up (to even) if the round bit is
;set.  If the LSB is zero, then the sticky bits remain zero and we always
;round down.

	bt	edi,0			;Look at LSB (for round even)
	adc	edx,(1 shl 31)-1	;CY set if round up
	jnc	RoundIntX
	mov	EMSEG:[SWcc],RoundUp
	add	edi,1			;Round
	adc	ebx,0
	jc	RoundInvalid
RoundIntX:
	ret				;CY clear, no Invalid exception

Shift64Round:
	or	edi,edi
	setnz	dl			;Set sticky bit in edx
	xor	edi,edi			;Mantissa is all zero
	jmp	Round64Int

Under64Int:
;ZF set if exponent is -1
	xchg	ebx,edx			;64-bit right shift
	mov	ch,al			;Restore sign to ch
	jz	Shift64Round		;Exp. is -1, could need to round up
	xor	edi,edi			;Mantissa is all zero
	mov	EMSEG:[CURerr],Precision;Set flag on inexact result
NotNearest64Int:
;We want to increase the magnitude if RCup and +, or RCdown and -
	mov	al,EMSEG:[CWcntl]	;Get rounding control
.erre	(not RCup and RoundControl) eq RCdown
	sar	ch,7			;Expand sign through whole byte
	xor	al,ch			;Flip round mode if -
	and	al,RoundControl
	cmp	al,RCup			;Rounding up?
	jnz	RoundIntOk		;No, chop it
	mov	EMSEG:[SWcc],RoundUp
	add	edi,1
	adc	ebx,0
	jc	RoundInvalid
RoundIntOk:
	clc
	ret

;*******************************************************************************

;*** RoundToBit
;
;This is a relatively low performance routine used by FRNDINT and to
;generate internal-format denormals.  It can round to any bit position.
;
;Inputs:
;	mantissa in ebx:esi:eax, exponent in high ecx, sign in ch bit 7
;Purpose:
;	Round number to integer.  Zero exponent means number is in the
;	range [1,2), so only the MSB will survive (MSB-1 is round bit).  
;	Larger exponents keep more bits; 63 would mean no rounding.
;Outputs:
;	mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl
;
;Does NOT detect overflow.

NoSigBits:
;Exponent was negative: no integer part
	and	ecx,bSign shl 8		;Zero exponent, preserve sign
	mov	cl,bTAG_ZERO
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
	test	EMSEG:[CWcntl],RoundControl	;Check rounding control bits
.erre	RCnear eq 0
	jnz	NotNearNoSig		;Not just round-to-nearest
	cmp	edx,-1			;Exponent of -1 ==> range [.5,1)
	je	HalfBitRound
RndIntToZero:
	xor	ebx,ebx
	mov	esi,ebx			;Just return zero
	ret

NotNearNoSig:
;We want to increase the magnitude if RCup and +, or RCdown and -
	mov	al,EMSEG:[CWcntl]	;Get rounding control
	sar	ch,7			;Expand sign through whole byte
	xor	al,ch			;Flip rounding bits if negative
	and	al,RoundControl
	cmp	al,RCup			;Rounding up?
	jnz	RndIntToZero		;No, chop it
RndIntToOne:
	mov	ebx,1 shl 31
	xor	esi,esi
	mov	cl,bTAG_SNGL
	mov	EMSEG:[SWcc],RoundUp
	ret

HalfBitRound:
	add	ebx,ebx			;Shift off MSB (round bit)
	or	ebx,esi
	or	ebx,eax
	jnz	RndIntToOne
	ret				;Return zero

;**********
RoundToBit:
;**********
	mov	edx,ecx			;Make copy of exponent
	sar	edx,16			;Bring rounding exponent down
	jl	NoSigBits
	mov	cl,dl
	cmp	cl,32			;Rounding in low word?
	jae	RoundLow
;When cl = 31, the RoundBit is in the low half while the LSB is in the 
;high half.  We must preserve the RoundBit when we move it to eax.
	xchg    eax,esi                 ;Low half becomes sticky bits
	or      ah,al                   ;Preserve lowest bits in ah
	add     esi,-1                  ;Set CY if any original sticky bits
	sbb     al,al                   ;Put original sticky bits in al
	mov	esi,ebx
	xor	ebx,ebx			;Shift mantissa right 32 bits
RoundLow:
	mov	edx,(1 shl 31) - 1
	shr	edx,cl			;Make mask
;Note in the case of cl = 31, edx is now zero.
	mov	edi,esi
	and	edi,edx
	or	edi,eax			;Any bits being lost?
	jz	RndSetTag		;All done
	inc	edx			;Mask for LSB
	or	EMSEG:[CURerr],Precision;Set flag on inexact result
	test	EMSEG:[CWcntl],RoundControl	;Check rounding control bits
.erre	RCnear eq 0
	jnz	NotNearLow		;Not just round-to-nearest
	mov	edi,edx			;Save LSB mask
	shr	edi,1			;Mask for round bit
	jc	SplitRound		;Round bit in eax?
	test	esi,edi			;Round bit set?
	jz	MaskOffLow
	dec	edi			;Mask for sticky bits
	or	edi,edx			;Sticky bits + LSB
	and	edi,esi
	or	edi,eax			;Any sticky bits set?
	jz	MaskOffLow
RoundUpThenMask:
	mov	EMSEG:[SWcc],RoundUp
	add	esi,edx			;Round up
	adc	ebx,0
	jc	RoundBumpExp
MaskOffLow:
	dec	edx			;Mask for round & sticky bits
	not	edx
	and	esi,edx			;Zero out low bits
RndSetTag:
	or	ebx,ebx			;Is it normalized?
        jns     RoundedHighHalf
        or      esi,esi                 ;Any bits in low half?
.erre   bTAG_VALID eq 1
.erre   bTAG_SNGL eq 0
        setnz   cl                      ;if low half==0 then cl=0 else cl=1
        ret

SplitRound:
;Rounding high half in esi on rounding bit in eax
	bt	esi,0			;Look at LSB
	adc	eax,(1 shl 31) - 1	;Set CY if round up
	jc	RoundUpThenMask
        or      ebx,ebx                 ;Will set ZF for jnz below
RoundedHighHalf:
;Rounding occured in high half, which had been moved low.
;Move it back up high.
;
;ZF set here on content of ebx.  If not zero, rounding high half in esi
;rippled forward into zero in ebx.
        mov     cl,bTAG_SNGL
        jnz     RndIntNorm              ;Present high half should be zero
        xchg    ebx,esi                 ;Shift left 32 bits
        ret

RndIntNorm:
;Rounded up high half of mantissa, which rolled over to 0.
	add	ecx,1 shl 16		;Increase exponent
	mov	ebx,1 shl 31		;Restore MSB
	ret				;Tag already set to SNGL

RoundBumpExp:
;Mantissa was FFFFF... and rolled over to 0 when we rounded
	add	ecx,1 shl 16		;Increase exponent
	mov	ebx,1 shl 31		;Restore MSB
	jmp	MaskOffLow

NotNearLow:
;We want to increase the magnitude if RCup and +, or RCdown and -
	mov	al,EMSEG:[CWcntl]	;Get rounding control
	sar	ch,7			;Expand sign through whole byte
.erre	(not RCup and RoundControl) eq RCdown
	xor	al,ch			;Flip rounding bits if negative
	and	al,RoundControl
	cmp	al,RCup			;Rounding up?
	jz	RoundUpThenMask		;yes
	jmp	MaskOffLow		;No, chop it
