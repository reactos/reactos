        subttl  emload.asm - FLD and FILD instructions
        page
;*******************************************************************************
;emload.asm - FLD and FILD instructions
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Purpose:
;       FLD and FILD instructions
;Inputs:
;	edi = [CURstk]
;	dseg:esi = pointer to memory operand
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


	PrevStackWrap	edi,LdStk	;Tied to PrevStackElem below

;*******
EM_ENTRY eFLDreg
eFLDreg:
;*******
;	edi = [CURstk]
;	esi = pointer to st(i) from instruction field

	PrevStackElem	edi,LdStk	;Point to receiving location
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY	;Is it empty?
	jnz	FldErr
	mov	ecx,EMSEG:[esi].ExpSgn
	cmp	cl,bTAG_EMPTY
	jz	FldErr
	mov	ebx,EMSEG:[esi].lManHi
	mov	esi,EMSEG:[esi].lManLo
	mov	EMSEG:[CURstk],edi
	mov	EMSEG:[edi].lManLo,esi
	mov	EMSEG:[edi].lManHi,ebx
	mov	EMSEG:[edi].ExpSgn,ecx
	ret


;This is common code that stores a value into the stack after being loaded
;into registers by the appropriate routine.

	PrevStackWrap	edi,Load	;Tied to PrevStackElem below

FldCont:
;mantissa in ebx:esi, exp/sign in ecx
;edi = [CURstk]
	PrevStackElem	edi,Load	;Point to receiving location
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY	;Is it empty?
	jnz	FldErr
	cmp	cl,bTAG_NAN		;Returning a NAN?
	jz	FldNAN
SaveStack:
	mov	EMSEG:[CURstk],edi
	mov	EMSEG:[edi].lManLo,esi
	mov	EMSEG:[edi].lManHi,ebx
	mov	EMSEG:[edi].ExpSgn,ecx
	ret

FldErr:
	or	EMSEG:[SWcc],C1		;Signal overflow
	mov	EMSEG:[CURerr],StackFlag;Kills possible denormal exception
Unsupported:
	call	ReturnIndefinite	;in emarith.asm
	jz	FldExit			;Unmasked, do nothing
	mov	EMSEG:[CURstk],edi	;Update top of stack
FldExit:
	ret

FldNAN:
;Is it a signaling NAN?
	test	ebx,1 shl 30		;Check for SNAN
	jnz	SaveStack		;If QNAN, just use it as result
	or	EMSEG:[CURerr],Invalid	;Flag the error
	or	ebx,1 shl 30		;Make it into a QNAN
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jnz	SaveStack		;If so, update with masked response
	ret


;****************
;Load Single Real
;****************

EM_ENTRY eFLD32
eFLD32:
	push	offset FldCont		;Return address
					;Fall into Load32Real
Load32Real:
;dseg:esi points to IEEE 32-bit real number
;On exit:
;	mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl
;preserves edi.

        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	ecx,dseg:[esi]		;Get number
	mov	ebx,ecx			;Save copy of mantissa
	shl	ebx,8			;Normalize
	shr	ecx,7			;Bring exponent down
	and	ecx,0FFH shl 16		;Look at just exponent
	mov	ch,dseg:[esi+3]		;Get sign again
	jz	short ZeroOrDenorm32	;Exponent is zero
	xor	esi,esi			;Zero out the low bits
	or	ebx,1 shl 31		;Set implied bit
	cmp	ecx,SexpMax shl 16
	jge	NANorInf		;Max exp., must be NAN or Infinity
	add	ecx,(TexpBias-SexpBias) shl 16	;Change to extended format bias
	mov	cl,bTAG_SNGL
	ret

ZeroOrDenorm32:
;Exponent is zero. Number is either zero or denormalized
	xor	esi,esi			;Zero out the low bits
	and	ebx,not (1 shl 31)	;Keep just mantissa
	jnz	Norm32
	mov	cl,bTAG_ZERO
	ret

Norm32:
	add	ecx,(TexpBias-SexpBias+1-31) shl 16	;Fix up bias
	jmp	FixDenorm


NANorInf:
;Shared by single and double real
	and	ecx,bSign shl 8		;Save only sign in ch
	or	ecx,TexpMax shl 16 + bTAG_NAN	;Max exp.
	cmp	ebx,1 shl 31		;Only 1 bit set means infinity
	jnz	@F
	or	esi,esi
	jnz	@F
	mov	cl,bTAG_INF
@@:
	ret

;****************
;Load Double Real
;****************

EM_ENTRY eFLD64
eFLD64:
	push	offset FldCont		;Return address
					;Fall into Load64Real
Load64Real:
;dseg:esi points to IEEE 64-bit real number
;On exit:
;	mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl
;preserves edi.

        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	ecx,dseg:[esi+4]		;Get sign, exp., and high mantissa
	mov	ebx,ecx			;Save copy of mantissa
	shr	ecx,4			;Bring exponent down
	and	ecx,7FFH shl 16		;Look at just exponent
	mov	ch,dseg:[esi+7]		;Get sign again
	mov	esi,dseg:[esi]		;Get low 32 bits of op
	jz	short ZeroOrDenorm64	;Exponent is zero
	shld	ebx,esi,31-20
	shl	esi,31-20		;Normalize
	or	ebx,1 shl 31		;Set implied bit
	cmp	ecx,DexpMax shl 16
	jge	NANorInf		;Max exp., must be NAN or Infinity
	add	ecx,(TexpBias-DexpBias) shl 16	;Change to extended format bias
SetNormTag:
	or	esi,esi			;Any bits in low half?
.erre	bTAG_VALID eq 1
.erre	bTAG_SNGL eq 0
	setnz   cl                      ;if low half==0 then cl=0 else cl=1
	ret

ZeroOrDenorm64:
;Exponent is zero. Number is either zero or denormalized
	and	ebx,0FFFFFH		;Keep just mantissa
	jnz	ShortNorm64		;Are top 20 bits zero?
	or	esi,esi			;Are low 32 bits zero too?
	jnz	LongNorm64
	mov	cl,bTAG_ZERO
	ret

LongNorm64:
	xchg	ebx,esi			;Shift up 32 bits
	sub	ecx,32 shl 16		;Correct exponent
ShortNorm64:
	add	ecx,(TexpBias-DexpBias+12-31) shl 16	;Fix up bias
FixDenorm:
	or	EMSEG:[CURerr],Denormal	;Set Denormal Exception
	bsr	edx,ebx			;Scan for MSB
;Bit number in edx ranges from 0 to 31
	mov	cl,dl
	not	cl			;Convert bit number to shift count
	shld	ebx,esi,cl
	shl	esi,cl
	shl	edx,16			;Move exp. adjustment to high end
	add	ecx,edx			;Adjust exponent
	jmp	SetNormTag


;******************
;Load Short Integer
;******************

EM_ENTRY eFILD16
eFILD16:
	push	offset FldCont		;Return address
					;Fall into Load16Int
Load16Int:
;dseg:esi points to 16-bit integer
;On exit:
;	mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl
;preserves edi.

        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	ax,dseg:[esi]
NormInt16:
	xor	esi,esi			;Extend with zero
	cwd				;extend sign through dx
	xor	ax,dx
	sub	ax,dx			;Take ABS() of integer
	bsr	cx,ax			;Find MSB
	jz	ZeroInt
;Bit number in cx ranges from 0 to 15
	not	ecx			;Convert to shift count
	shl	eax,cl			;Normalize
	not	ecx
.erre	TexpBias eq 0
	shl	ecx,16			;Move exponent to high half
	mov	ch,dh			;Set sign
	mov	ebx,eax			;Mantissa to ebx
	mov	cl,bTAG_SNGL
	ret

ZeroInt:
	xor	ebx,ebx
	mov	ecx,ebx
	mov	cl,bTAG_ZERO
	ret


;******************
;Load Long Integer
;******************

EM_ENTRY eFILD32
eFILD32:
	push	offset FldCont		;Return address
					;Fall into Load32Int
Load32Int:
;dseg:esi points to 32-bit integer
;On exit:
;	mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl
;preserves edi.

        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	eax,dseg:[esi]
	xor	esi,esi			;Extend with zero
	or	eax,eax			;It it zero?
	jz	ZeroInt
	cdq				;extend sign through edx
	xor	eax,edx
	sub	eax,edx			;Take ABS() of integer
	mov	ebx,eax			;Mantissa to ebx
;BSR uses 3 clocks/bit, so speed it up by checking the top half
;This saves 36 clocks on 386 (42 on 486sx)
;Cost is 13 clocks on 386 if high word isn't zero (5 on 486sx)
.erre	TexpBias eq 0
	xor	eax,eax			;Initialize exponent
	cmp	ebx,0FFFFH		;Upper bits zero?
	ja	@F
	shl	ebx,16
	sub	eax,16
@@:
	bsr	ecx,ebx			;Find MSB
	add	eax,ecx			;Compute expoment
	not	cl			;Convert bit number to shift count
	shl	ebx,cl			;Normalize
	shrd	ecx,eax,16		;Move exponent to high half of ecx
	mov	ch,dh			;Set sign
	mov	cl,bTAG_SNGL
	ret


;*****************
;Load Quad Integer
;*****************

EM_ENTRY eFILD64
eFILD64:
        mov     EMSEG:[PrevDataOff],esi       ;Save operand pointer
	mov	ebx,dseg:[esi+4]		;Get high 32 bits
	mov	eax,ebx			;Make copy of sign
	mov	esi,dseg:[esi]		;Get low 32 bits
	mov	ecx,ebx
	or	ecx,esi			;Is it zero?
	jz	ZeroQuad
NormQuadInt:
;Entry point from eFBLD
;eax bit 31 = sign
;ebx:esi = integer
;edi = [CURstk]
.erre	TexpBias eq 0
	mov     ax,32                   ;Initialize exponent
	or	ebx,ebx			;Check sign
	jz	LongNormInt
	jns	FindBit
	not	ebx
	neg	esi			;CY set if non-zero
	sbb	ebx,-1			;Add one if esi == 0
	jnz	FindBit			;Check for high bits zero
LongNormInt:
	xchg	ebx,esi			;Normalize 32 bits
	xor     ax,ax                   ;Reduce exponent by 32
FindBit:
;BSR uses 3 clocks/bit, so speed it up by checking the top half
;This saves 35 clocks on 386 (41 on 486sx)
;Cost is 11 clocks on 386 if high word isn't zero (4 on 486sx)
	cmp	ebx,0FFFFH		;Upper bits zero?
	ja	@F
	shld	ebx,esi,16
	shl	esi,16
	sub	eax,16
@@:
	bsr	ecx,ebx			;Find MSB
	add	eax,ecx			;Compute expoment
	not	cl			;Convert bit number to shift count
	shld	ebx,esi,cl		;Normalize
	shl	esi,cl
	mov     ecx,eax                 ;Move sign and exponent to ecx
	rol     ecx,16                  ;Swap sign and exponent halves
	or	esi,esi			;Any bits in low half?
.erre	bTAG_VALID eq 1
.erre	bTAG_SNGL eq 0
	setnz   cl                      ;if low half==0 then cl=0 else cl=1
	jmp	FldCont

ZeroQuad:
	mov	cl,bTAG_ZERO
	jmp	FldCont


;****************
;Load Temp Real
;****************

	PrevStackWrap	edi,Ld80	;Tied to PrevStackElem below

EM_ENTRY eFLD80
eFLD80:
;This is not considered an "arithmetic" operation (like all the others are),
;so SNANs do NOT cause an exception.  However, unsupported formats do.
        mov     EMSEG:[PrevDataOff],esi	;Save operand pointer
	PrevStackElem	edi,Ld80	;Point to receiving location
	cmp	EMSEG:[edi].bTag,bTAG_EMPTY	;Is it empty?
	jnz	FldErr
LoadTempReal:
	mov	ebx,dseg:[esi+4]	;Get high half of mantissa
	mov	cx,dseg:[esi+8]		;Get exponent and sign
	mov	esi,dseg:[esi]		;Get low half of mantissa
	mov	eax,ecx	
	and	ch,7FH			;Mask off sign bit
	shl	ecx,16			;Move exponent to high end
	mov	ch,ah			;Restore sign
	jz	ZeroOrDenorm80
;Check for unsupported format: unnormals (MSB not set)
	or	ebx,ebx
	jns	Unsupported
	sub	ecx,(IexpBias-TexpBias) shl 16	;Correct the bias
	cmp	ecx,TexpMax shl 16
	jge	NANorInf80
SetupTag:
	or	esi,esi			;Any bits in low half?
.erre	bTAG_VALID eq 1
.erre	bTAG_SNGL eq 0
	setnz   cl                      ;if low half==0 then cl=0 else cl=1
	jmp	SaveStack

NANorInf80:
	mov	cl,bTAG_NAN
	cmp	ebx,1 shl 31		;Only 1 bit set means infinity
	jnz	SaveStack
	or	esi,esi
	jnz	SaveStack
	mov	cl,bTAG_INF
	jmp	SaveStack

ZeroOrDenorm80:
;Exponent is zero. Number is either zero or denormalized
	or	ebx,ebx
	jnz	ShortNorm80		;Are top 32 bits zero?
	or	esi,esi			;Are low 32 bits zero too?
	jnz	LongNorm80
	mov	cl,bTAG_ZERO
	jmp	SaveStack

;This code accepts and works correctly with pseudo-denormals (MSB already set)
LongNorm80:
	xchg	ebx,esi			;Shift up 32 bits
	sub	ecx,32 shl 16		;Correct exponent
ShortNorm80:
	add	ecx,(TexpBias-IexpBias+1-31) shl 16	;Fix up bias
	bsr	edx,ebx			;Scan for MSB
;Bit number in edx ranges from 0 to 31
	mov	cl,dl
	not	cl			;Convert bit number to shift count
	shld	ebx,esi,cl
	shl	esi,cl
	shl	edx,16			;Move exp. adjustment to high end
	add	ecx,edx			;Adjust exponent
	jmp	SetUpTag
