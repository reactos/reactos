	subttl	emxtract - FXTRACT and FSCALE instructions
        page
;*******************************************************************************
;emxtract - FXTRACT and FSCALE instructions
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Inputs:
;	edi = [CURstk]
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


XtractStackOver:
	mov	EMSEG:[SWcc],C1		;Flag stack overflow
XtractEmpty:
;Result is two Indefinites (if exception masked)
	call	StackError		;Put first indefinite at [edi] = ST(0)
	jz	XtractExit		;Error was unmasked--just exit
	mov	EMSEG:[CURstk],edi
        mov     eax,EMSEG:[edi].ExpSgn
        mov     EMSEG:[esi].ExpSgn,eax
        mov     eax,EMSEG:[edi].lManHi
        mov     EMSEG:[esi].lManHi,eax
        mov     eax,EMSEG:[edi].lManLo
        mov     EMSEG:[esi].lManLo,eax
	ret

	PrevStackWrap	edi,Xtract

EM_ENTRY eFXTRACT
eFXTRACT:
;edi = [CURstk]
	mov	esi,edi			;Save current ST
	PrevStackElem	edi,Xtract
;edi = ST(0)
;esi = ST(1) (operand)
	mov	eax,EMSEG:[esi].ExpSgn
;Exception priority requires reporting stack underflow (i.e., using an EMPTY)
;before stack overflow (i.e., no place for result).  Yes, both can happen
;together if they've screwed with the stack! (ST empty when ST(-1) isn't).
	cmp	al,bTAG_EMPTY		;Is operand empty?
	jz	XtractEmpty
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY	;Is there an empty spot?
	jnz	XtractStackOver
	cmp	al,bTAG_ZERO		;Is it special?
	jae	XtractSpclOrZero
XtractNormal:
	mov	EMSEG:[CURstk],edi
.erre   TexpBias eq 0
        movzx   ebx,ax                  ;Zero exponent
;Save mantissa in ST(0)
        mov     EMSEG:[edi].ExpSgn,ebx
        mov     ebx,EMSEG:[esi].lManHi
        mov     EMSEG:[edi].lManHi,ebx
        mov     ebx,EMSEG:[esi].lManLo
        mov     EMSEG:[edi].lManLo,ebx
	mov	edi,esi			;Save ST(1) pointer in edi
	shr	eax,16			;Move exponent down
	call	NormInt16		;in emload.asm
;mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl
	mov	EMSEG:[edi].lManLo,esi
	mov	EMSEG:[edi].lManHi,ebx
	mov	EMSEG:[edi].ExpSgn,ecx
XtractExit:
	ret

XtractSpcl:
	cmp	al,bTAG_INF
	jz	XtractInf
	cmp	al,bTAG_NAN
	jz	XtractNAN
;Must be denormal.  Change tag to VALID or SNGL.
	cmp	EMSEG:[esi].lManLo,0		;Any bits in low half?
.erre	bTAG_VALID eq 1
.erre	bTAG_SNGL eq 0
	setnz	al			;if low half==0 then al=0 else al=1
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is it masked?
	jnz	XtractNormal		;If so, ignore denormalization
	ret

XtractSpclOrZero:
	ja	XtractSpcl
;Operand is zero.  Result is ST(0) = 0 (same sign), ST(1) = -infinity
	mov	EMSEG:[CURerr],ZeroDivide
	test	EMSEG:[CWmask],ZeroDivide	;Exception masked?
	jz	XtractExit
	mov	EMSEG:[CURstk],edi
        mov     EMSEG:[edi].ExpSgn,eax
        mov     eax,EMSEG:[esi].lManHi
        mov     EMSEG:[edi].lManHi,eax
        mov     eax,EMSEG:[esi].lManLo
        mov     EMSEG:[edi].lManLo,eax
	mov	EMSEG:[esi].ExpSgn,(IexpMax-IexpBias+TexpBias) shl 16 + bSign shl 8 + bTAG_INF
	mov	EMSEG:[esi].bMan7,80H	;Change zero to infinity
	ret

XtractInf:
;Result is ST(0) = infinity (same sign), ST(1) = +infinity
        mov     EMSEG:[esi].bSgn,0            ;Ensure ST(1) is positive
XtractQNAN:
        mov     EMSEG:[CURstk],edi
        mov     EMSEG:[edi].ExpSgn,eax
        mov     eax,EMSEG:[esi].lManHi
        mov     EMSEG:[edi].lManHi,eax
        mov     eax,EMSEG:[esi].lManLo
        mov     EMSEG:[edi].lManLo,eax
        ret

XtractNAN:
;Result is two QNANs, signal Invalid Operation if SNAN
	test	EMSEG:[esi].bMan7,40H		;Is it SNAN?
	jnz	XtractQNAN
	mov	EMSEG:[CURerr],Invalid
	test	EMSEG:[CWmask],Invalid
	jz	XtractExit
	or	EMSEG:[esi].bMan7,40H		;Change to QNAN
        jmp     XtractQNAN

;*******************************************************************************
;
;FSCALE instruction

;Actual instruction entry point is in emarith.asm

;Dispatch table for scale
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

tFscaleDisp	label	dword		;Source (reg)	Dest (*[di] = ST)
	dd	ScaleDouble		;single		single
	dd	ScaleDouble		;single		double
	dd	ScaleX			;single		zero
	dd	ScaleSpclDest		;single		special
	dd	ScaleDouble		;double		single
	dd	ScaleDouble		;double		double
	dd	ScaleX			;double		zero
	dd	ScaleSpclDest		;double		special
	dd	ScaleX			;zero		single
	dd	ScaleX			;zero		double
	dd	ScaleX			;zero		zero
	dd	ScaleSpclDest		;zero		special
	dd	ScaleSpclSource		;special	single
	dd	ScaleSpclSource		;special	double
	dd	ScaleSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	ScaleTwoInf		;Two infinites


;The unmasked response to overflow and underflow with FSCALE is complicated 
;by the extreme range it can generate.  Normally, the exponent is biased
;by 24,576 in the appropriate direction to bring it back into range.
;This may not be enough, however.  If it isn't, a result of infinity
;(with the correct sign) is returned for overflow, regardless of the 
;rounding mode.  For underflow, zero (with the correct sign) is returned,
;even if it could be represented as a denormal.  This may be the only 
;operation in which the unmasked response destroys the operands beyond 
;recovery.

BigScale:
;Scale factor is much too big.  Just shift mantissa right two bits to get
;MSB out of sign bit and ensure no overflow when we add.
	mov	cl,2			;Always shift 2 bits if it's big
	jmp	ScaleCont

ScaleDouble:
;ebx:esi = ST(1) mantissa
;ecx = ST(1) sign in bit 15, exponent in high half
;edi = pointer to ST(0)
	rol	ecx,16			;Bring exponent down, sign to top
	or	ch,ch			;Check sign of exponent
	js	ScaleX			;No work if less than zero
	cmp	cx,30			;Scale factor exceed 30 bits?
	jge	BigScale
	not	cl			;cl = amount to shift right (mod 32)
ScaleCont:
	shr	ebx,cl			;ebx = exponent adjustment for ST(0)
;Use two's complement if negative (complement and increment)
	mov     eax,ecx
	cdq				;Extend sign through edx
	xor	ebx,edx			;Complement if negative
	sub	ebx,edx			;Increment if negative
;Scale exponent
	movsx	eax,EMSEG:[edi].wExp		;Get exponent to adjust
	add	eax,ebx			;Can't overflow
	cmp	eax,IexpMax-IexpBias	;Within normal range?
	jge	ScaleOverflow
	cmp	eax,IexpMin-IexpBias
	jle	ScaleUnderflow
SaveScaledExp:
;Result fit withing normal range
	mov	EMSEG:[edi].wExp,ax		;Update exponent of ST(0)
ScaleX:
	ret

ScaleOverflow:
;eax = exponent that's too big
	mov	EMSEG:[CURerr],Overflow
	test	EMSEG:[CWmask],Overflow	;Is exception unmasked?
	jz	UnmaskedScaleOvfl
;Produce masked overflow response
	mov	al,EMSEG:[CWcntl]		;Get rounding control
	mov	ah,al
;Return max value if RCup bit = 1 and -, or RCdown bit = 1 and +
;i.e., RCup & sign OR RCdown & not sign
.erre	RCchop eq RCup + RCdown		;Always return max value
.erre	RCnear eq 0			;Never return max value
	sar	ch,7			;Expand sign through whole byte
.erre	(RCdown and bSign) eq 0		;Don't want to change real sign
	xor	ch,RCdown		;Flip sign for RCdown bit
	and	ah,ch			;RCup & sign  OR  RCdown & not sign
	jz	ScaleToInfinity		;Save Infinity
;Get max value
	sub	ecx,1 shl 16		;Drop exponent by 1
	xor	esi,esi
	dec	esi			;esi == -1
	mov	ebx,esi
SaveScaleMax:
	mov	EMSEG:[edi].lManLo,esi
	mov	EMSEG:[edi].lManHi,ebx
	mov	EMSEG:[edi].ExpSgn,ecx
	ret

UnmaskedScaleOvfl:
	sub	eax,UnderBias		;Unmasked response
	cmp	eax,IexpMax-IexpBias	;Within normal range now?
	jl	SaveScaledExp		;Use exponent biased by 24K
ScaleToInfinity:
	mov	ebx,1 shl 31
	xor	esi,esi
	mov	ecx,(IexpMax-IexpBias+TexpBias) shl 16 + bTAG_INF
	mov	ch,EMSEG:[edi].bSgn		;Give it same sign
	jmp	SaveScaleMax		;Use infinity

ScaleUnderflow:
;eax = exponent that's too big
	test	EMSEG:[CWmask],Underflow	;Is exception unmasked?
	jz	ScaleSetUnder
	cmp	eax,-32768		;Does exponent fit in 16 bits?
	jg	@F
	mov	ax,-32768		;Max value
@@:
;Set up for denormalizer
	mov	ebx,EMSEG:[edi].lManHi
	mov	esi,EMSEG:[edi].lManLo
	shrd	ecx,eax,16		;Move exponent to high end of ecx
	mov	ch,EMSEG:[edi].bSgn		;Keep sign
	xor	eax,eax			;No sticky bits
	mov	EMSEG:[Result],edi
	jmp	Denormalize		;In emround.asm

ScaleSetUnder:
;Underflow exception not masked.  Adjust exponent and try again.
	mov	EMSEG:[CURerr],Underflow
	add	eax,UnderBias		;Unmasked response
	cmp	eax,IexpMin-IexpBias	;Within normal range now?
	jg	SaveScaledExp		;Use exponent biased by 24K
	mov	EMSEG:[CURerr],Underflow
ScaleToZero:
	mov	ecx,bTAG_ZERO
	mov	ch,EMSEG:[edi].bSgn		;Give it same sign
	xor	ebx,ebx
	mov	esi,ebx
	jmp	SaveScaleMax		;Set to zero

;***
ScaleSpclDest:
	mov	al,EMSEG:[edi].bTag		;Pick up tag
	cmp	al,bTAG_INF		;Scaling infinity?
	jz	ScaleRet		;No change if so
	jmp	SpclDest		;In emarith.asm

ScaleRet:
	ret

;***
ScaleSpclSource:
	cmp	cl,bTAG_INF		;Scaling by infinity?
	jnz	SpclSource		;in emarith.asm
	or	ch,ch			;Scaling by -infinity?
	js	ScaleToZero
	cmp	EMSEG:[edi].bTag,bTAG_ZERO	;Zero scaled by +infinity?
	jnz	ScaleToInfinity
	jmp	ReturnIndefinite	;Invalid operation

;***
ScaleTwoInf:
	or	ch,ch			;Scaling by +infinity?
	jns	ScaleRet		;All done then
;Scaling infinity by -infinity
	jmp	ReturnIndefinite	;Invalid operation
