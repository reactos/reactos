	subttl	emlsbcd.asm - FBSTP and FBLD instructions
        page
;*******************************************************************************
;emlsbcd.asm - FBSTP and FBLD instructions
;
;        Microsoft Confidential
;
;	 Copyright (c) Microsoft Corporation 1991
;        All Rights Reserved
;
;Purpose:
;	FBSTP and FBLD instructions.
;
;	These routines convert between 64-bit integer and 18-digit packed BCD
;	format.  They work by splitting the number being converted in half
;	and converting the two halves separately.  This works well because
;	9 decimal digits fit nicely within 30 binary bits, so converion of
;	each half is strictly a 32-bit operation.
;
;Inputs:
;	edi = [CURstk]
;	dseg:esi = pointer to memory operand
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;*******************************************************************************


;******
eFBLD:
;******
	mov	eax,dseg:[esi+5]		;Get high 8 digits
	or	eax,eax			;Anything there?
	jz	HighDigitsZero
	mov	ecx,8
	call	ReadDigits		;Convert first 8 digits to binary
	mov	eax,dseg:[esi+1]		;Get next 8 digits
	xor	edi,edi
	shld	edi,eax,4		;Shift ninth digit into edi
	imul	ebx,10
	add	edi,ebx			;Accumulate ninth digit
SecondNineDigits:
	xor	ebx,ebx			;In case eax==0
	shl	eax,4			;Keep digits left justified
	jz	LastTwoDigits
	mov	ecx,7
	call	ReadDigits		;Convert next 7 digits to binary
LastTwoDigits:
	mov	al,dseg:[esi]		;Get last two digits
	shl	eax,24			;Left justify
	mov	ecx,2
	call	InDigitLoop		;Accumulate last two digits
;edi = binary value of high 9 digits
;ebx = binary value of low 9 digits
	mov	eax,1000000000		;One billion: shift nine digits left
	mul	edi			;Left shift 9 digits. 9 cl. if edi==0
	add	ebx,eax			;Add in low digits
	adc	edx,0
BcdReadyToNorm:
;edx:ebx = integer converted to binary
	mov	eax,dseg:[esi+6]		;Get sign to high bit of eax
	mov	esi,ebx
	mov	ebx,edx
	mov     edi,EMSEG:[CURstk]
;mantissa in ebx:esi, sign in high bit of eax
;edi = [CURstk]
	jmp	NormQuadInt		;in emload.asm

HighDigitsZero:
	mov	eax,dseg:[esi+1]		;Get next 8 digits
	or	eax,eax			;Anything there?
	jz	CheckLastTwo
	xor	edi,edi
	shld	edi,eax,4		;Shift ninth digit into edi
	jmp	SecondNineDigits
       
CheckLastTwo:
	mov	bl,dseg:[esi]		;Get last two digits
	or	bl,bl
	jz	ZeroBCD
	mov	al,bl
	shr	al,4			;Bring down upper digit
	imul	eax,10
	and	ebx,0FH			;Keep lowest digit only
	add	ebx,eax
	xor	edx,edx
	jmp	BcdReadyToNorm
	
ZeroBCD:
	mov	ecx,bTAG_ZERO		;Exponent is zero
	mov	ch,dseg:[esi+9]		;Get sign byte to ch
	xor	ebx,ebx
	mov	esi,ebx
;mantissa in ebx:esi, exp/sign in ecx
;edi = [CURstk]
	jmp	FldCont			;in emload.asm

			
;*** ReadDigits
;
;Inputs:
;	eax = packed BCD digits, left justified, non-zero
;	ecx = no. of digits, 7 or 8
;Outputs:
;	ebx = number

SkipZeroDigits:
        sub     ecx,3
        shl     eax,12
ReadDigits:
;We start by scanning off leading zeros.  This costs 16 cl./nybble in
;the ScanZero loop.  To reduce this cost for many leading zeros, we
;check for three leading zeros at a time.  Adding this test saves
;26 cl. for 3 leading zeros, 57 cl. for 6 leading zeros, at a cost
;of only 5 cl. if less than 3 zeros.  We choose 3 at a time so we
;can repeat it once (there are never more than 7 zeros).
	test    eax,0FFF00000H          ;Check first 3 nybbles for zero
	jz      SkipZeroDigits
	xor	ebx,ebx
ScanZero:
;Note that bsr is 3 cl/bit, or 12 cl/nybble.  Add in the overhead and
;this loop of 16 cl/nybble is cheaper for the 1 - 3 digits it does.
	dec	ecx
	shld	ebx,eax,4		;Shift digit into ebx
	rol	eax,4			;Left justify **Doesn't affect ZF!**
	jz	ScanZero		;Skip to next digit if zero
	jecxz	ReadDigitsX
InDigitLoop:
;eax = digits to convert, left justified
;ebx = result accumulation
;ecx = number of digits to convert
	xor	edx,edx
	shld	edx,eax,4		;Shift digit into edx
	shl	eax,4			;Keep digits left justified
	imul	ebx,10			;Only 10 clocks on 386!
	add	ebx,edx			;Accumulate number
	dec	ecx
	jnz	InDigitLoop
ReadDigitsX:
	ret
		
;*******************************************************************************

ChkInvalidBCD:
	ja	SetInvalidBCD
	cmp	edi,0A7640000H		;(1000000000*1000000000) and 0ffffffffh
	jb	ValidBCD
SetInvalidBCD:
	mov	EMSEG:[CURerr],Invalid
InvalidBCD:
	test	EMSEG:[CWmask],Invalid	;Is it masked?
	jz	ReadDigitsX		;No--leave memory unchanged
;Store Indefinite
	mov	dword ptr dseg:[esi],0
	mov	dword ptr dseg:[esi+4],0
	mov	word ptr dseg:[esi+8],-1	;0FF00000000H for packed BCD indefinite
	jmp	PopStack		;in emstore.asm

;******
eFBSTP:
;******
	call	RoundToInteger		;Get integer in ebx:edi, sign in ch
	jc	InvalidBCD
	cmp	ebx,0DE0B6B3H		;(1000000000*1000000000) shr 32
	jae	ChkInvalidBCD
ValidBCD:
	and	ch,bSign
	mov	dseg:[esi+9],ch		;Fill in sign byte
	mov	edx,ebx
	mov	eax,edi			;Get number to edx:eax for division
	mov	ebx,1000000000
	div	ebx			;Break into two 9-digit halves
	xor	ecx,ecx			;Initial digits
	mov	edi,eax			;Save quotient
	mov	eax,edx
	or	eax,eax
	jz	SaveLowBCD
	call	WriteDigits
	shrd	ecx,eax,4		;Pack 8th digit
	xor	al,al
	shl	eax,20			;Move digit in ah to high end
SaveLowBCD:
	mov	dseg:[esi],ecx		;Save low 8 digits
	mov	ecx,eax			;Get ready for next 8 digits
	mov	eax,edi
	or	eax,eax
	jz	ZeroHighBCD
	call	WriteDigits
	shl	ah,4			;Move digit to upper nybble
	or	al,ah			;Combine last two digits
SaveHighBCD:
	mov	dseg:[esi+4],ecx		;Save lower 8 digits
	mov	dseg:[esi+8],al
	jmp	PopStack

ZeroHighBCD:
	shr	ecx,28			;Position 9th digit
	jmp	SaveHighBCD


;*** WriteDigits
;
;Inputs:
;	eax = binary number < 1,000,000,000 and > 0
;	ecx = Zero or had one BCD digit left justified
;Purpose:
;	Convert binary integer to BCD.
;
;	The time required for the DIV instruction is dependent on operand
;	size, at 6 + (no. of bits) clocks for 386.  (In contrast, multiply
;	by 10 as used in FBLD/ReadDigits above takes the same amount of
;	time regardless of operand size--only 10 clocks.)
;
;	The easy way to do this conversion would be to repeatedly do a
;	32-bit division by 10 (at 38 clocks/divide).  Instead, the number
;	is broken down so that mostly 8-bit division is used (only 14 clocks).
;	AAM (17 clocks) is also used to save us from having to load the 
;	constant 10 and zero ah.  AAM is faster than DIV on the 486sx.
;
;Outputs:
;	ecx has seven more digits packed into it (from left)
;	ah:al = most significant two digits (unpacked)
;esi,edi preserved

WriteDigits:
;eax = binary number < 1,000,000,000
	cdq				;Zero edx
	mov	ebx,10000
	div	ebx			;Break into 4-digit and 5-digit pieces
	mov	bl,100
	or	edx,edx
	jz	ZeroLowDigits
	xchg	edx,eax			;Get 4-digit remainder to eax
;Compute low 4 digits
; 0 < eax < 10000
	div	bl			;Get two 2-digit pieces. 14cl on 386
	mov	bh,al			;Save high 2 digits
	mov	al,ah			;Get low digits
	aam
	shl	ah,4			;Move digit to upper nybble
	or	al,ah
	shrd	ecx,eax,8
	mov	al,bh			;Get high 2 digits
	aam
	shl	ah,4			;Move digit to upper nybble
	or	al,ah
	shrd	ecx,eax,8
;Compute high 5 digits
	mov	eax,edx			;5-digit quotient to eax
	or	eax,eax
	jz	ZeroHighDigits
ConvHigh5:
	cdq				;Zero edx
	shld	edx,eax,16		;Put quotient in dx:ax
	xor	bh,bh			;bx = 100
	div	bx			;Get 2- and 3-digit pieces. 22cl on 386
	xchg	edx,eax			;Save high 3 digits, get log 2 digits
	aam
	shl	ah,4			;Move digit to upper nybble
	or	al,ah
	shrd	ecx,eax,8
	mov	eax,edx			;Get high 3 digits
	mov	bl,10
	div	bl
	mov	bl,ah			;Remainder is next digit
	shrd	ecx,ebx,4
	aam				;Get last two digits
;Last two digits in ah:al
	ret

ZeroLowDigits:
	shr	ecx,16
	jmp	ConvHigh5

ZeroHighDigits:
	shr	ecx,12
	ret
