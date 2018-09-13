        subttl  emstore.asm - FST, FSTP, FIST, FISTP instructions
        page
;*******************************************************************************
;emstore.asm - FST, FSTP, FIST, FISTP instructions
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Purpose:
;       FST, FSTP, FIST, FISTP instructions
;Inputs:
;	edi = [CURstk]
;	dseg:esi = pointer to memory destination
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


;******
EM_ENTRY eFSTP
eFSTP:
;******
;	edi = [CURstk]
;	esi = pointer to st(i) from instruction field

	cmp	EMSEG:[edi].bTag,bTAG_EMPTY
        jz      short efstp_StackError
;UNDONE: temporary hack to preserve condition codes
        mov     ax,[esp+4].OldStatus
        mov     EMSEG:[StatusWord],ax
;UNDONE: end of hack

;A common use of this instruction is FSTP st(0) just to pop the stack.
;We check for this case and optimize it.
        cmp     esi,edi
        jz      short JustPop
;Copy the register
        mov     eax,EMSEG:[edi].ExpSgn
        mov     EMSEG:[esi].ExpSgn,eax
        mov     eax,EMSEG:[edi].lManHi
        mov     EMSEG:[esi].lManHi,eax
        mov     eax,EMSEG:[edi].lManLo
        mov     EMSEG:[esi].lManLo,eax
JustPop:
	POPSTret	edi

efstp_StackError:
	mov	EMSEG:[CURerr],Invalid+StackFlag
	ret


;******
EM_ENTRY eFST
eFST:
;******
;	edi = [CURstk]
;	esi = pointer to st(i) from instruction field

	cmp	EMSEG:[edi].bTag,bTAG_EMPTY
	jz	StackError		;In emarith.asm
;Copy the register
        mov     eax,EMSEG:[edi].ExpSgn
        mov     EMSEG:[esi].ExpSgn,eax
        mov     eax,EMSEG:[edi].lManHi
        mov     EMSEG:[esi].lManHi,eax
        mov     eax,EMSEG:[edi].lManLo
        mov     EMSEG:[esi].lManLo,eax
DontPop:
	ret


;Come here if the instruction wants to pop the stack

PopStackChk:
	jc	DontPop			;Get unmasked error?
PopStack:
	mov	edi,EMSEG:[CURstk]
	POPSTret	edi


StoreSpcl64:
	cmp	cl,bTAG_DEN
	jz	Denorm64
.erre	bTAG_NAN lt bTAG_EMPTY
.erre	bTAG_NAN gt bTAG_INF
	cmp	cl,bTAG_NAN
	mov	ecx,DexpMax shl 16	;Insert special exponent for NAN/Inf.
	jb	StoreIEEE64		;Go handle infinity
	ja	Empty64
;Have a NAN.
	test	ebx,1 shl 30		;Check for SNAN
	jnz	StoreIEEE64		;Go store QNAN
	or	ebx,1 shl 30		;Make SNAN into a QNAN
	mov	EMSEG:[CURerr],Invalid	;Flag the exception
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jnz	StoreIEEE64		;If so, update with masked response
	stc				;Don't pop stack
	ret

Empty64:
;It's empty--signal invalid operation
	mov	EMSEG:[CURerr],StackFlag+Invalid
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	DoNothing64		;No--leave memory unchanged
;Store Indefinite
;For FSTP, we go ahead and do the pop even though it's empty
	mov	dword ptr dseg:[esi],0
	mov	dword ptr dseg:[esi+4],0FFF80000H	;64-bit IEEE indefinite
	ret				;CY clear

Denorm64:
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is it masked?
	jnz	NormStore64		;If so, ignore denormalization
DoNothing64:
	stc				;Don't pop stack
	ret

;*****************
;Store Double Real
;*****************

EM_ENTRY eFSTP64
eFSTP64:
	push	offset PopStackChk	;Return here after store

EM_ENTRY eFST64
eFST64:
        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	ebx,EMSEG:[edi].lManHi
	mov	ecx,EMSEG:[edi].ExpSgn
	mov	edi,EMSEG:[edi].lManLo
;mantissa in ebx:edi, exponent in high ecx, sign in ch bit 7, tag in cl
;memory destination is dseg:esi
	mov	al,ch			;Save sign bit
	cmp	cl,bTAG_ZERO
.erre	bTAG_VALID lt bTAG_ZERO
.erre	bTAG_SNGL lt bTAG_ZERO
        jz      short SignAndStore64    ;Just set sign and exit
        ja      StoreSpcl64
NormStore64:
;Note that we could have a denormal exception at this point.
;Thus any additional exceptions must OR into [CURerr], not MOV.
	xor	cx,cx
	add	ecx,(DexpBias-TexpBias) shl 16	;Correct bias
        jl      short Under64
        cmp     ecx,DexpMax shl 16      ;Exponent too big?
        jge     Over64
	test	edi,(1 shl 11) - 1	;Any bits to round?
        jz      short StoreIEEE64

Round64:
	or	EMSEG:[CURerr],Precision 	;Set flag on inexact result
	test	EMSEG:[CWcntl],RoundControl	;Check rounding control bits
.erre	RCnear eq 0
        jnz     NotNearest64            ;Not just round-to-nearest
	test	edi,1 shl 10		;Check rounding bit
        jz      short StoreIEEE64       ;If zero, don't round up
	test	edi,(3 shl 10)-1	;Test LSB and sticky bits
        jnz     RoundUp64b

StoreIEEE64:
        or      ecx, ecx                ;now that value is rounded,
        je      short Under64           ;check exponent for underflow

StoreIEEE64Continue:
	and	ebx,not (1 shl 31)	;Clear MSB--it's implied in IEEE64
	shrd	edi,ebx,11
	shr	ebx,11			;Move mantissa down
	shl	ecx,4			;Exponent up to position
	or	ebx,ecx			;Combine exponent
SignAndStore64:
	and	al,bSign		;Just sign bit
	shl	eax,24			;Sign to MSB
	or	ebx,eax			;Combine sign
	mov	dseg:[esi],edi
	mov	dseg:[esi+4],ebx
;CY clear indicate no error
	ret

SetUnderflow:
	or	EMSEG:[CURerr],Underflow	;Unmasked underflow--do nothing
DoNothing:
	stc				;Indicate nothing was done
	ret

Under64:
        dec     cl                      ; Is cx == 1?
        jz      short StoreIEEE64Continue   ; Yes, we've alread been here

	test	EMSEG:[CWmask],Underflow	;Is underflow masked?
	jz	SetUnderflow		;No, do nothing more
;Produce masked underflow response
;Note that the underflow exception does not occur if the number can be
;represented exactly as a denormal.

	sar	ecx,16			;Bring exponent down
	cmp	ecx,DexpMin-52	;Allow for shift down to rounding bit
	jl	BigUnder64		;Too small, just make it zero
.erre	DexpMin eq 0
	neg	ecx			;Use as shift count
	inc	ecx			;Shift by at least one
	xor	edx,edx			;Place for sticky bits
	cmp	cl,32			;Long shift?
	jb	ShortDenorm
	neg	edi			;CY set if non-zero
	sbb	edx,edx			;-1 if bits shifted off, else zero
	mov	edi,ebx
	xor	ebx,ebx			;32-bit right shift
ShortDenorm:
;Shift count is modulo-32
	shrd	edx,edi,cl
	shrd	edi,ebx,cl
	shr	ebx,cl
	cmp	edx,1			;CY set if zero, else clear
	sbb	edx,edx			;Zero if bits shifted off, else -1
	inc	edx			;1 if bits shifted off, else zero
	or	edi,edx			;Collapse sticky bits into edi

        mov     ecx, 1                  ;Biased exponent is zero, put 1 into CL (noticed by Under64)
	test	edi,(1 shl 11) - 1	;Any bits to round?
	jz	StoreIEEE64		;If not, no exception
	or	EMSEG:[CURerr],Underflow
	jmp	Round64

Over64:
	test	EMSEG:[CWmask],Overflow	;Is overflow masked?
	jz	SetOverflow		;No, do nothing more
;Produce masked overflow response
	or	EMSEG:[CURerr],Overflow+Precision
	mov	ebx,DexpMax shl 20
	xor	edi,edi			;ebx:edi = positive infinity
	mov	ah,EMSEG:[CWcntl]	;Get rounding control
;Return max value if RCup bit = 1 and -, or RCdown bit = 1 and +
;i.e., RCup & sign OR RCdown & not sign
.erre	RCchop eq RCup + RCdown		;Always return max value
.erre	RCnear eq 0			;Never return max value
	sar	al,7			;Expand sign through whole byte
.erre	(RCdown and bSign) eq 0		;Don't want to change real sign
	xor	al,RCdown		;Flip sign for RCdown bit
	and	ah,al			;RCup & sign  OR  RCdown & not sign
	test	ah,RoundControl		;Look only at RC bits
	jz	SignAndStore64		;Return infinity
	dec	ebx
	dec	edi			;Max value == infinity-1
	jmp	SignAndStore64

SetOverflow:
	or	EMSEG:[CURerr],Overflow
	stc				;Indicate nothing was done
	ret

BigUnder64:
	or	EMSEG:[CURerr],Underflow+Precision
	xor	ebx,ebx
	mov	edi,ebx			;Set it to zero
	mov	ecx,ebx			;Including exponent
NotNearest64:
;We want to increase the magnitude if RCup and +, or RCdown and -
	mov	ah,EMSEG:[CWcntl]		;Get rounding control
	sar	al,7			;Expand sign through whole byte
.erre	(not RCup and RoundControl) eq RCdown
	xor	ah,al			;Flip rounding bits if negative
	and	ah,RoundControl
	cmp	ah,RCup
        jnz     StoreIEEE64             ;No, chop it

RoundUp64b:
        mov     EMSEG:[SWcc],RoundUp
	add	edi,1 shl 11		;Round up
	adc	ebx,0
        jnc     StoreIEEE64

	add	ecx,1 shl 16		;Mantissa overflowed, bump exponent
        cmp     ecx,DexpMax shl 16      ;Exponent too big?
        jge     Over64
        jmp     StoreIEEE64

;*******************************************************************************

StoreSpcl32:
	cmp	cl,bTAG_DEN
	jz	Denorm32
.erre	bTAG_NAN lt bTAG_EMPTY
.erre	bTAG_NAN gt bTAG_INF
	cmp	cl,bTAG_NAN
	mov	ecx,SexpMax shl 16	;Insert special exponent
	jb	StoreIEEE32
	ja	Empty64
;Have a NAN.
	test	ebx,1 shl 30		;Check for SNAN
	jnz	StoreIEEE32		;Go store QNAN
	or	ebx,1 shl 30		;Make SNAN into a QNAN
	mov	EMSEG:[CURerr],Invalid	;Flag the exception
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jnz	StoreIEEE32		;If so, update with masked response
	stc				;Don't pop stack
	ret

Empty32:
;It's empty--signal invalid operation
	mov	EMSEG:[CURerr],StackFlag+Invalid
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	DoNothing32		;No--leave memory unchanged
;Store Indefinite
;For FSTP, we go ahead and do the pop even though it's empty
	mov	dword ptr dseg:[esi],0FFC00000H	;32-bit IEEE indefinite
	ret				;CY clear

Denorm32:
	mov	EMSEG:[CURerr],Denormal
	test	EMSEG:[CWmask],Denormal	;Is it masked?
	jnz	NormStore32		;If so, ignore denormalization
DoNothing32:
	stc				;Don't pop stack
	ret

;*****************
;Store Single Real
;*****************

EM_ENTRY eFSTP32
eFSTP32:
	push	offset PopStackChk	;Return here after store

EM_ENTRY eFST32
eFST32:
        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	ebx,EMSEG:[edi].lManHi
	mov	ecx,EMSEG:[edi].ExpSgn
	mov	edi,EMSEG:[edi].lManLo
;mantissa in ebx:edi, exponent in high ecx, sign in ch bit 7, tag in cl
;memory destination is dseg:esi
	mov	al,ch			;Save sign bit
	cmp	cl,bTAG_ZERO
.erre	bTAG_VALID lt bTAG_ZERO
.erre	bTAG_SNGL lt bTAG_ZERO
	jz	SignAndStore32		;Just set sign and exit
	ja	StoreSpcl32
NormStore32:
;Note that we could have a denormal exception at this point.
;Thus any additional exceptions must OR into [CURerr], not MOV.
	xor	cx,cx
	add	ecx,(SexpBias-TexpBias) shl 16	;Correct bias
	jle	Under32
	cmp	ecx,SexpMax shl 16	;Exponent too big?
	jge	Over32
;See if we need to round
	mov	edx,ebx			;Get low bits
	and	edx,(1 shl 8) - 1	;Mask to last 8 bits
	or	edx,edi			;Throwing away any bits?
	jz	StoreIEEE32
;Result will not be exact--check rounding mode
Round32:
	or	EMSEG:[CURerr],Precision 	;Set flag on inexact result
	test	EMSEG:[CWcntl],RoundControl	;Check rounding control bits
.erre	RCnear eq 0
	jnz	NotNearest32		;Not just round-to-nearest
	test	bl,1 shl 7		;Round bit set?
	jz	StoreIEEE32
	mov	edx,ebx
	and	edx,(3 shl 7)-1		;Mask to LSB and sticky bits
	or	edx,edi			;Combine with remaining sticky bits
	jz	StoreIEEE32
	mov	EMSEG:[SWcc],RoundUp
	add	ebx,1 shl 8		;Round up
	jc	AddOneExp32
StoreIEEE32:
	and	ebx,not (1 shl 31)	;Clear MSB--it's implied in IEEE32
	shr	ebx,8			;Move mantissa down
	shl	ecx,7			;Exponent up to position
	or	ebx,ecx			;Combine exponent
SignAndStore32:
	and	al,bSign		;Just sign bit
	shl	eax,24			;Sign to MSB
	or	ebx,eax			;Combine sign
	mov	dseg:[esi],ebx
;CY clear indicate no error
	ret

Under32:
	test	EMSEG:[CWmask],Underflow	;Is underflow masked?
	jz	SetUnderflow		;No, do nothing more
;Produce masked underflow response
;Note that the underflow exception does not occur if the number can be
;represented exactly as a denormal.
	sar	ecx,16			;Bring exponent down
	cmp	ecx,SexpMin-23	;Allow for shift down to rounding bit
	jl	BigUnder32		;Too small, just make it zero
.erre	SexpMin eq 0
	neg	ecx			;Use as shift count
	inc	ecx			;Shift by at least one
	xor	edx,edx			;Place for sticky bits
	shrd	edx,ebx,cl
	shr	ebx,cl
	xor	ecx,ecx			;Biased exponent is zero
	or	edi,edx			;Combine sticky bits
	mov	edx,ebx			;Get low bits
	and	edx,(1 shl 8) - 1	;Mask to last 8 bits
	or	edx,edi			;Throwing away any bits?
	jz	StoreIEEE32
	or	EMSEG:[CURerr],Underflow
	jmp	Round32

AddOneExp32:
	add	ecx,1 shl 16		;Mantissa overflowed, bump exponent
	cmp	ecx,SexpMax shl 16	;Exponent too big?
	jl	StoreIEEE32
Over32:
	test	EMSEG:[CWmask],Overflow	;Is overflow masked?
	jz	SetOverflow		;No, do nothing more
;Produce masked overflow response
	or	EMSEG:[CURerr],Overflow+Precision
	mov	ebx,SexpMax shl 23
	mov	ah,EMSEG:[CWcntl]		;Get rounding control
;Return max value if RCup bit = 1 and -, or RCdown bit = 1 and +
;i.e., RCup & sign OR RCdown & not sign
.erre	RCchop eq RCup + RCdown		;Always return max value
.erre	RCnear eq 0			;Never return max value
	sar	al,7			;Expand sign through whole byte
.erre	(RCdown and bSign) eq 0		;Don't want to change real sign
	xor	al,RCdown		;Flip sign for RCdown bit
	and	ah,al			;RCup & sign  OR  RCdown & not sign
	test	ah,RoundControl		;Look only at RC bits
	jz	SignAndStore32		;Return infinity
	dec	ebx			;Max value == infinity-1
	jmp	SignAndStore32

BigUnder32:
	or	EMSEG:[CURerr],Underflow+Precision
	xor	ebx,ebx			;Set it to zero
	xor	ecx,ecx			;Exponent too
NotNearest32:
;We want to increase the magnitude if RCup and +, or RCdown and -
	mov	ah,EMSEG:[CWcntl]		;Get rounding control
	sar	al,7			;Expand sign through whole byte
.erre	(not RCup and RoundControl) eq RCdown
	xor	ah,al			;Flip rounding bits if negative
	and	ah,RoundControl
	cmp	ah,RCup
	jnz	StoreIEEE32		;No, chop it
	mov	EMSEG:[SWcc],RoundUp
	add	ebx,1 shl 8		;Round up
	jnc	StoreIEEE32
	jmp	AddOneExp32

;*******************************************************************************

StoreSpcl32Int:
	cmp	cl,bTAG_DEN
	jz	NormStore32Int		;Ignore denormal
	cmp	cl,bTAG_EMPTY
	jnz	Over32Int		;All other specials are invalid
	mov	EMSEG:[CURerr],StackFlag+Invalid
	jmp	Invalid32Int

DoNothing32Int:
	stc				;Don't pop stack
	ret

CheckMax32:
	ja	Over32Int
	test	al,bSign		;Is it negative?
	jnz	Store32Int		;If so, answer is OK
Over32Int:
;Overflow on integer store is invalid according to IEEE
	mov	EMSEG:[CURerr],Invalid	;Must remove precision exception
Invalid32Int:
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	DoNothing32Int		;No--leave memory unchanged
;Store Indefinite
;For FSTP, we go ahead and do the pop even though it's empty
	mov	dword ptr dseg:[esi],80000000H	;32-bit integer indefinite
	ret				;CY clear

;******************
;Store Long Integer
;******************

EM_ENTRY eFISTP32
eFISTP32:
	push	offset PopStackChk	;Return here after store

EM_ENTRY eFIST32
eFIST32:
        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	ebx,EMSEG:[edi].lManHi
	mov	ecx,EMSEG:[edi].ExpSgn
	mov	edi,EMSEG:[edi].lManLo
;mantissa in ebx:edi, exponent in high ecx, sign in ch bit 7, tag in cl
;memory destination is dseg:esi
	mov	al,ch			;Save sign bit
	cmp	cl,bTAG_ZERO
.erre	bTAG_VALID lt bTAG_ZERO
.erre	bTAG_SNGL lt bTAG_ZERO
	jz	Store32Int		;Just store zero and exit
	ja	StoreSpcl32Int
NormStore32Int:
	xor	edx,edx
	sar	ecx,16			;Bring exponent down
	cmp	ecx,-1			;Is it less than 1?
	jle	Under32Int
	cmp	ecx,31
	jg	Over32Int
	sub	ecx,31
	neg	ecx			;cl = amount to shift right
	shrd	edx,edi,cl
	shrd	edi,ebx,cl		;Collect round and sticky bits
	shr	ebx,cl			;Align integer
;See if we need to round
	mov	ecx,edi
	or	ecx,edx			;Throwing away any bits?
	jz	StoreIEEE32Int
;Result will not be exact--check rounding mode
Round32Int:
	mov	EMSEG:[CURerr],Precision 	;Set flag on inexact result
	test	EMSEG:[CWcntl],RoundControl	;Check rounding control bits
.erre	RCnear eq 0
	jnz	NotNearest32Int		;Not just round-to-nearest

;To perform "round even" when the round bit is set and the sticky bits
;are zero, we treat the LSB as if it were a sticky bit.  Thus if the LSB
;is set, that will always force a round up (to even) if the round bit is
;set.  If the LSB is zero, then the sticky bits remain zero and we always
;round down.

	bt	ebx,0			;Look at LSB (for round even)
	adc	edx,-1			;CY set if sticky bits <>0
	adc	edi,(1 shl 31)-1	;CY set if round up
	jnc	StoreIEEE32Int
	mov	EMSEG:[SWcc],RoundUp
	inc	ebx
	jz	Over32Int
StoreIEEE32Int:
	cmp	ebx,1 shl 31		;Check for max value
	jae	CheckMax32
SignAndStore32Int:
	shl	eax,24			;Sign to MSB
	cdq				;Extend sign through edx
	xor	ebx,edx			;Complement
	sub	ebx,edx			;  and increment if negative
	clc
Store32Int:
	mov	dseg:[esi],ebx
;CY clear indicates no error
	ret

Under32Int:
;ZF set if exponent is -1
	xchg	edx,edi			;32-bit right shift
	xchg	edi,ebx			;ebx = 0 now
	jz	Round32Int		;If exponent was -1, ready to round
	mov	EMSEG:[CURerr],Precision 	;Set flag on inexact result
NotNearest32Int:
;We want to increase the magnitude if RCup and +, or RCdown and -
	mov	ah,EMSEG:[CWcntl]		;Get rounding control
	sar	al,7			;Expand sign through whole byte
.erre	(not RCup and RoundControl) eq RCdown
	xor	ah,al			;Flip rounding bits if negative
	and	ah,RoundControl
	cmp	ah,RCup			;Rounding up?
	jnz	StoreIEEE32Int		;No, chop it
	mov	EMSEG:[SWcc],RoundUp
	inc	ebx
	jnc	StoreIEEE32Int
	jmp	Over32Int

;*******************************************************************************

StoreSpcl16Int:
	cmp	cl,bTAG_DEN
	jz	NormStore16Int		;Ignore denormal
	cmp	cl,bTAG_EMPTY
	jnz	Over16Int		;All other specials are invalid
	mov	EMSEG:[CURerr],StackFlag+Invalid
	jmp	Invalid16Int

DoNothing16Int:
	stc				;Don't pop stack
	ret

CheckMax16:
	ja	Over16Int
	test	al,bSign		;Is it negative?
	jnz	Store16Int		;If so, answer is OK
Over16Int:
;Overflow on integer store is invalid according to IEEE
	mov	EMSEG:[CURerr],Invalid
Invalid16Int:
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	DoNothing16Int		;No--leave memory unchanged
;Store Indefinite
;For FSTP, we go ahead and do the pop even though it's empty
	mov	word ptr dseg:[esi],8000H	;16-bit integer indefinite
	ret				;CY clear

;*******************
;Store Short Integer
;*******************

EM_ENTRY eFISTP16
eFISTP16:
	push	offset PopStackChk	;Return here after store

EM_ENTRY eFIST16
eFIST16:
        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	ebx,EMSEG:[edi].lManHi
	mov	ecx,EMSEG:[edi].ExpSgn
	mov	edi,EMSEG:[edi].lManLo
;mantissa in ebx:edi, exponent in high ecx, sign in ch bit 7, tag in cl
;memory destination is dseg:esi
	mov	al,ch			;Save sign bit
	cmp	cl,bTAG_ZERO
.erre	bTAG_VALID lt bTAG_ZERO
.erre	bTAG_SNGL lt bTAG_ZERO
	jz	Store16Int		;Just store zero and exit
	ja	StoreSpcl16Int
NormStore16Int:
	xor	edx,edx
	sar	ecx,16			;Bring exponent down
	cmp	ecx,-1			;Is it less than 1?
	jle	Under16Int
	cmp	ecx,15
	jg	Over16Int
	sub	ecx,31
	neg	ecx			;cl = amount to shift right
	shrd	edx,edi,cl
	shrd	edi,ebx,cl		;Collect round and sticky bits
	shr	ebx,cl			;Align integer
;See if we need to round
	mov	ecx,edi
	or	ecx,edx			;Throwing away any bits?
	jz	StoreIEEE16Int
;Result will not be exact--check rounding mode
Round16Int:
	mov	EMSEG:[CURerr],Precision 	;Set flag on inexact result
	test	EMSEG:[CWcntl],RoundControl	;Check rounding control bits
.erre	RCnear eq 0
	jnz	NotNearest16Int		;Not just round-to-nearest

;To perform "round even" when the round bit is set and the sticky bits
;are zero, we treat the LSB as if it were a sticky bit.  Thus if the LSB
;is set, that will always force a round up (to even) if the round bit is
;set.  If the LSB is zero, then the sticky bits remain zero and we always
;round down.

	bt	ebx,0			;Look at LSB (for round even)
	adc	edx,-1			;CY set if sticky bits <>0
	adc	edi,(1 shl 31)-1	;CY set if round up
	jnc	StoreIEEE16Int
	mov	EMSEG:[SWcc],RoundUp
	inc	ebx
StoreIEEE16Int:
	cmp	ebx,1 shl 15		;Check for max value
	jae	CheckMax16
SignAndStore16Int:
	shl	eax,24			;Sign to MSB
	cdq				;Extend sign through edx
	xor	ebx,edx			;Complement
	sub	ebx,edx			;  and increment if negative
	clc
Store16Int:
	mov	dseg:[esi],bx
;CY clear indicates no error
	ret

Under16Int:
;ZF set if exponent is -1
	xchg	edx,edi			;16-bit right shift
	xchg	edi,ebx			;ebx = 0 now
	jz	Round16Int		;If exponent was -1, ready to round
	mov	EMSEG:[CURerr],Precision 	;Set flag on inexact result
NotNearest16Int:
;We want to increase the magnitude if RCup and +, or RCdown and -
	mov	ah,EMSEG:[CWcntl]		;Get rounding control
	sar	al,7			;Expand sign through whole byte
.erre	(not RCup and RoundControl) eq RCdown
	xor	ah,al			;Flip rounding bits if negative
	and	ah,RoundControl
	cmp	ah,RCup			;Rounding up?
	jnz	StoreIEEE16Int		;No, chop it
	mov	EMSEG:[SWcc],RoundUp
	inc	ebx
	jnc	StoreIEEE16Int
	jmp	Over16Int

;*******************************************************************************

;******************
;Store Quad Integer
;******************

EM_ENTRY eFISTP64
eFISTP64:
        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	call	RoundToInteger
	jc	Invalid64Int
;Have integer in ebx:edi
;Sign in ch
	cmp	ebx,1 shl 31		;Check for max value
	jae	CheckMax64
	or	ch,ch			;Check sign
	jns	Store64Int
;64-bit negation
	not	ebx
	neg	edi
	sbb	ebx,-1
Store64Int:
	mov	dseg:[esi],edi
	mov	dseg:[esi+4],ebx
	jmp	PopStack

CheckMax64:
	ja	Over64Int
	test	al,bSign		;Is it negative?
	jnz	Store64Int		;If so, answer is OK
Over64Int:
;Overflow on integer store is invalid according to IEEE
	mov	EMSEG:[CURerr],Invalid
Invalid64Int:
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	DoNothing80		;No--leave memory unchanged
;Store Indefinite
;For FSTP, we go ahead and do the pop even though it's empty
	mov	dword ptr dseg:[esi],0
	mov	dword ptr dseg:[esi+4],80000000H	;64-bit integer indefinite
	jmp	PopStack

;*******************************************************************************

Empty80:
;It's empty--signal invalid operation
	mov	EMSEG:[CURerr],StackFlag+Invalid
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	DoNothing80		;No--leave memory unchanged
;Store Indefinite
;For FSTP, we go ahead and do the pop even though it's empty
	mov	dword ptr dseg:[esi],0
	mov	dword ptr dseg:[esi+4],0C0000000H
	mov	word ptr dseg:[esi+8],0FFFFH	;80-bit IEEE indefinite
	jmp	PopStack

DoNothing80:
	ret

;***************
;Store Temp Real
;***************

EM_ENTRY eFSTP80
eFSTP80:
        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	eax,EMSEG:[edi].ExpSgn
	cmp	al,bTAG_EMPTY
	jz	Empty80

        push    offset PopStack

StoreTempReal:
	mov	ebx,EMSEG:[edi].lManHi
	mov	edi,EMSEG:[edi].lManLo
;mantissa in ebx:edi, exponent in high eax, sign in ah bit 7, tag in al
;memory destination is dseg:esi
	mov	ecx,eax			;get copy of sign and tag
	shr	ecx,16			;Bring exponent down
	cmp	al,bTAG_ZERO
	jz	StoreIEEE80		;Skip bias if zero
	add	ecx,IexpBias-TexpBias	;Correct bias
	cmp	al,bTAG_DEN
	jz	Denorm80
StoreIEEE80:
	and	eax,bSign shl 8
	or	ecx,eax			;Combine sign with exponent
	mov	dseg:[esi],edi
	mov	dseg:[esi+4],ebx
	mov	dseg:[esi+8],cx

;	jmp	PopStack
        ret

Denorm80:
;Must change it to a denormal
	dec	ecx
	neg	ecx			;Use as shift count
	cmp	cl,32			;Long shift?
	jae	LongDenorm
	shrd	edi,ebx,cl
	shr	ebx,cl
	xor	ecx,ecx			;Exponent is zero
	jmp	StoreIEEE80

LongDenorm:
;edi must be zero if we have 32 bits to shift
	xchg	ebx,edi			;32-bit right shift
	shr	edi,cl			;shift count is modulo-32
	xor	ecx,ecx			;Exponent is zero
	jmp	StoreIEEE80
