	subttl  emfadd.asm - Addition and Subtraction
	page
;*******************************************************************************
;	 Copyright (c) Microsoft Corporation 1991
;	 All Rights Reserved
;
;emfadd.asm - long double add and subtract
;	by Tim Paterson
;
;Purpose:
;	Long double add/subtract.
;Outputs:
;	Jumps to [RoundMode] to round and store result.
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************

;*******************************************************************************
; Dispatch for Add/Sub/Subr
;
; Signs are passed in dx:
;       xor source sign with dl
;       xor dest sign with dh
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
tFaddDisp	label	dword		;Source (reg)	Dest (*[di])
	dd	AddDouble		;single		single
	dd	AddDouble		;single		double
	dd	AddSourceSign		;single		zero
	dd	AddSpclDest		;single		special
	dd	AddDouble		;double		single
	dd	AddDouble		;double		double
	dd	AddSourceSign		;double		zero
	dd	AddSpclDest		;double		special
	dd	AddDestSign		;zero		single
	dd	AddDestSign		;zero		double
	dd	AddZeroZero		;zero		zero
	dd	AddSpclDest		;zero		special
	dd	AddSpclSource		;special	single
	dd	AddSpclSource		;special	double
	dd	AddSpclSource		;special	zero
	dd	TwoOpBothSpcl		;special	special
	dd	AddTwoInf		;Two infinities

EM_ENTRY eFISUB16
eFISUB16:
        call    Load16Int
        mov     dx,bSign                ;Change sign of source
        jmp     AddSetResult

EM_ENTRY eFISUBR16
eFISUBR16:
        call    Load16Int
        mov     dx,bSign shl 8          ;Change sign of dest
        jmp     AddSetResult

EM_ENTRY eFIADD16
eFIADD16:
        call    Load16Int
        xor     edx,edx                 ;Both signs positive
        jmp     AddSetResult

EM_ENTRY eFISUB32
eFISUB32:
        call    Load32Int
        mov     dx,bSign                ;Change sign of source
        jmp     AddSetResult

EM_ENTRY eFISUBR32
eFISUBR32:
        call    Load32Int
        mov     dx,bSign shl 8          ;Change sign of dest
        jmp     AddSetResult

EM_ENTRY eFIADD32
eFIADD32:
        call    Load32Int
        xor     edx,edx                 ;Both signs positive
        jmp     AddSetResult

EM_ENTRY eFSUB32
eFSUB32:
        call    Load32Real
        mov     dx,bSign                ;Change sign of source
        jmp     AddSetResult

EM_ENTRY eFSUBR32
eFSUBR32:
        call    Load32Real
        mov     dx,bSign shl 8          ;Change sign of dest
        jmp     AddSetResult

EM_ENTRY eFADD32
eFADD32:
        call    Load32Real
        xor     edx,edx                 ;Both signs positive
        jmp     AddSetResult

EM_ENTRY eFSUB64
eFSUB64:
        call    Load64Real
        mov     dx,bSign                ;Change sign of source
        jmp     AddSetResult

EM_ENTRY eFSUBR64
eFSUBR64:
        call    Load64Real
        mov     dx,bSign shl 8          ;Change sign of dest
        jmp     AddSetResult

EM_ENTRY eFADD64
eFADD64:
        call    Load64Real
        xor     edx,edx                 ;Both signs positive
        jmp     AddSetResult


PolyAddDouble:
;This entry point is used by polynomial evaluator.
;It checks the operand in registers for zero, and doesn't require
;signs to be set up in dx.
;
;op1 mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7, tag in cl
;edi = pointer to op2 in ds
	xor	edx,edx			;Addition
	cmp	cl,bTAG_ZERO		;Adding to zero?
        jnz     AddDouble
;Number in registers is zero, so just return value from memory.
        mov     ecx,EMSEG:[edi].ExpSgn
        mov     ebx,EMSEG:[edi].lManHi
        mov     esi,EMSEG:[edi].lManLo
        ret

EM_ENTRY eFSUBPreg
eFSUBPreg:
        push    offset PopWhenDone

EM_ENTRY eFSUBreg
eFSUBreg:
        xchg    esi,edi

EM_ENTRY eFSUBtop
eFSUBtop:
        mov     dx,bSign                ;Change sign of source
        jmp     AddHaveSgn

EM_ENTRY eFSUBRPreg
eFSUBRPreg:
        push    offset PopWhenDone

EM_ENTRY eFSUBRreg
eFSUBRreg:
        xchg    esi,edi

EM_ENTRY eFSUBRtop
eFSUBRtop:
        mov     dx,bSign shl 8          ;Change sign of dest
        jmp     AddHaveSgn


InsignifAdd:
	mov	eax,1			;Set sticky bit
	shl	ch,1			;Get sign, CY set IFF subtracting mant.
	jnc	ReturnOp1
	sub	esi,eax			;Subtract 1 from mantissa
	sbb	ebx,0
	neg	eax
ReturnOp1:
;ebx:esi:eax = normalized unrounded mantissa
;high half of ecx = exponent
;high bit of ch = sign
	jmp	EMSEG:[RoundMode]

EM_ENTRY eFADDPreg
eFADDPreg:
        push    offset PopWhenDone

EM_ENTRY eFADDreg
eFADDreg:
        xchg    esi,edi

EM_ENTRY eFADDtop
eFADDtop:
        xor     edx,edx                 ;Both signs positive
AddHaveSgn:
        mov     ecx,EMSEG:[esi].ExpSgn
        mov     ebx,EMSEG:[esi].lManHi
        mov     esi,EMSEG:[esi].lManLo
AddSetResult:
        mov     ebp,offset tFaddDisp
        mov     EMSEG:[Result],edi            ;Save result pointer
        mov     al,cl
        mov     ah,EMSEG:[edi].bTag
        test    ax,ZEROorSPCL * 100H + ZEROorSPCL
        jnz     TwoOpDispatch

;.erre   AddDouble eq $                  ;Fall into AddDouble

;*********
AddDouble:
;*********
;
;op1 mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7
;dl = sign change for op1
;dh = sign change for op2
;edi = pointer to op2

	xor	ch,dl			;Flip sign if subtracting
	mov	eax,EMSEG:[edi].ExpSgn
	xor	ah,dh			;Flip sign if subtracting
	mov	edx,EMSEG:[edi].lManHi
	mov	edi,EMSEG:[edi].lManLo

AddDoubleReg:
;op1 mantissa in ebx:esi, exponent in high ecx, sign in ch bit 7
;op2 mantissa in edx:edi, exponent in high eax, sign in ah bit 7

	cmp	eax,ecx			;Compare exponents
.erre	TexpBias eq 0			;Not biased, use signed jump
	jle	short HavLg		;op1 is larger, we have the right order
	xchg	esi,edi
	xchg	ebx,edx
	xchg	eax,ecx
HavLg:
;Larger in ebx:esi.  Note that if the exponents were equal, things like
;the sign bit or tag may have determined which is "larger".  It doesn't
;matter which is which if the exponents are equal, however.
	and	ah,80H			;Keep sign bit
	sar	ch,1			;Extend sign into bit 6 of byte
	xor	ch,ah			;See if signs are the same
	xor	ax,ax			;Clear out sign and tag
	neg	eax			;ax still 0
	add	eax,ecx			;Get exponent difference
	shr	eax,16			;Bring exp. difference down to low end
	jz	short Aligned
	cmp	eax,64+1		;Is difference in range?
;CONSIDER: tell me again why 1/4 LSB could have effect.  It seems like
;CONSIDER: 1/2 LSB is the limit.
	ja	short InsignifAdd	;  (Even 1/4 LSB could have effect)
	mov	cl,al			;Shift count to cl
;High half ecx = exponent
;ch bit 7 = sign difference
;ch bit 6 = sign
;cl = shift count
	xor	eax,eax			;Prepare to take bits shifted out
	cmp	cl,32			;More than a whole word?
	jb	short ShortShift
	xchg	eax,edx			;Save bits shifted out in eax
	xchg	edi,eax
	sub	cl,32
	cmp	cl,8			;Safe to shift this much
	jb	short ShortSticky
;Collapse all (sticky) bits of eax into LSB of edi
	neg	eax			;Sets CY if eax was not zero
	sbb	eax,eax			;-1 if CY was set, zero otherwise
	neg	eax			;Sticky bit in LSB only
	or	di,ax			;Move sticky bit up
	cmp	cl,32			;Less than another Dword?
	jb	short ShortShift
	mov	eax,edi
	xor	edi,edi			;edx = edi = 0
ShortSticky:
;Shift will not be more than 8 bits
	or	ah,al			;Move up sticky bits
ShortShift:
	shrd	eax,edi,cl		;Save bits shifted out in eax
	shrd	edi,edx,cl
	shr	edx,cl
Aligned:
	shl	ch,1			;Were signs the same?
	jc	short SubMant		;No--go subtract mantissas
;Add mantissas
	add	esi,edi
	adc	ebx,edx
	jnc	short AddExit
;Addition of mantissas overflowed. Bump exponent and shift right
	shrd	eax,esi,1
	shrd	esi,ebx,1		;Faster than RCR
	sar	ebx,1
	or	ebx,1 shl 31		;Set MSB
	add	ecx,1 shl 16
AddExit:
;ebx:esi:eax = normalized unrounded mantissa
;high half of ecx = exponent
;high bit of ch = sign
	jmp	EMSEG:[RoundMode]

NegMant:
;To get here, exponents must have been equal and op2 was bigger than op1.
;Note that this means nothing ever got shifted into eax.
	not	ch			;Change sign of result
	not	ebx
	neg	esi
	sbb	ebx,-1
	js	short AddExit		;Already normalized?
	test	ebx,40000000H		;Only one bit out of normal?
	jz	short NormalizeAdd
	jmp	short NormOneBit

SubMant:
;Subtract mantissas
	neg	eax			;Pretend minuend is zero extended
	sbb	esi,edi
	sbb	ebx,edx
	jc	short NegMant
	js	short AddExit		;Already normalized?
NormChk:
	test	ebx,40000000H		;Only one bit out of normal?
	jz	short NormalizeAdd
;One bit normalization
NormOneBit:
	sub	ecx,1 shl 16		;Adjust exponent
ShiftOneBit:				;Entry point from emfmul.asm
	shld	ebx,esi,1
	shld	esi,eax,1
	shl	eax,1
	jmp	EMSEG:[RoundMode]

;***********
AddZeroZero:				;Entry point for adding two zeros
;***********
	mov	ah,EMSEG:[edi].bSgn	;Get sign of op
	xor	ch,dl			;Possibly subtracting source
	xor	ah,dh			;Possibly subtracting dest
	xor	ch,ah			;Do signs match?
	js	FindZeroSign		;No - use rounding mode to set sign
	mov	EMSEG:[edi].bSgn,ah	;Correct the sign if subtracting
	ret				;Result at [edi] is now correct

ZeroChk:
;Upper 64 bits were all zero, but there could be 1 bit in the MSB
;of eax.
	or	eax,eax
	jnz	short OneBitLeft
	mov	ebx,eax
	mov	esi,eax			;Zero mantissa
FindZeroSign:
;Round to -0 if "round down" mode, round to +0 otherwise
	xor	ecx,ecx			;Zero exponent, positive sign
	mov	dl,EMSEG:[CWcntl]	;Get control word
	and	dl,RoundControl
        cmp	dl,RCdown		;Rounding down?
	jnz	ZeroJmp
	mov	ch,80H			;Set sign bit
ZeroJmp:
	mov	cl,bTAG_ZERO
	jmp	EMSEG:[ZeroVector]

OneBitLeft:
	xchg	ebx,eax			;Bit now normalized
	sub	ecx,64 shl 16		;Adjust exponent
	jmp	EMSEG:[RoundMode]

NormalizeAdd:
;Inputs:
;	ebx:esi:eax = 65-bit number
;	ecx high half = exponent
;
;Since we are more than 1 bit out of normalization, exponents must have
;differed by 0 or 1.  Thus rounding will not be necessary for 64 bits.
	bsr	edx,ebx			;Scan for MSB
	jnz	short ShortNorm
	bsr	edx,esi
	jz	short ZeroChk
	sub	ecx,32 shl 16		;Adjust exponent
	mov	ebx,esi			;Push it up 32 bits
	mov	esi,eax
ShortNorm:
;Bit number in edx ranges from 0 to 31
	mov	cl,dl
	not	cl			;Convert bit number to shift count
	shld	ebx,esi,cl
	shld	esi,eax,cl
	shl	edx,16			;Move exp. adjustment to high end
	lea	ecx,[ecx+edx-(31 shl 16)] ;Adjust exponent
	xor	eax,eax			;No extra bits
	jmp	EMSEG:[RoundMode]

AddDestSign:
	xor	EMSEG:[edi].bSgn,dh
	ret

AddSourceSign:
	xor	ch,dl
	jmp	SaveResult
