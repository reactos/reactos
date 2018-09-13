	subttl	emlsenv.asm - Emulator Save/Restore
	page
;***
;emlsenv.asm - Emulator Save/Restore
;
;
;	 Copyright (c) Microsoft Corporation 1991
;
;	 All Rights Reserved
;
;Purpose:
;	FLDCW, FSTCW, FSTSW, FSTENV, FLDENV, FSAVE, FRSTOR instructions
;
;Revision History:
;
; []	09/05/91  TP	Initial 32-bit version.
;
;
;*******************************************************************************


;When setting the control word, the [RoundMode] vector must be set
;according to the rounding and precision modes.

tRoundMode	label	dword
	irp	RC,<near,down,up,chop>
	irp	PC,<24,24,53,64>
	dd	Round&&PC&&RC
	endm
	endm


EM_ENTRY eFLDCW
eFLDCW:
;Uses only eax and ebx
	mov	ax, dseg:[esi]		; Fetch control word from user memory
SetControlWord:
	and	ax,0F3FH		; Limit to valid values
	mov	EMSEG:[ControlWord], ax	; Store in the emulated control word
	not	al			;Flip mask bits for fast compare
        and     al,3FH                  ;Limit to valid mask bits
	mov	EMSEG:[ErrMask],al
	and	eax,(RoundControl + PrecisionControl) shl 8
.erre	RoundControl eq 1100B
.erre	PrecisionControl eq 0011B
	shr	eax,6			;Put PC and RC in bits 2-5
	mov	ebx,tRoundMode[eax]	;Get correct RoundMode vector
	mov	EMSEG:[RoundMode],ebx
	mov	EMSEG:[SavedRoundMode],ebx
	and	eax,RoundControl shl (8-6)	;Mask off precision control
	mov	ebx,tRoundMode[eax+PC64 shl (8-6)];Get correct RoundMode vector
	mov	EMSEG:[TransRound],ebx	;Round mode w/o precision
	ret


EM_ENTRY eFSTCW
eFSTCW:
;Uses only eax 
	mov	ax, EMSEG:[ControlWord]	; Fetch user control word
	mov	dseg:[esi], ax		; Store into user memory
	ret


EM_ENTRY eFSTSW
eFSTSW:
;Uses only eax and ebx
	call	GetStatusWord		; Fetch emulated Status word
	mov	dseg:[esi], ax		; Store into user memory
	ret


eFSTSWax:
;Uses only eax and ebx
	call	GetStatusWord		; Fetch emulated Status word
	mov	[esp+4].regAX,ax
	ret


EM_ENTRY eFDECSTP
eFDECSTP:
;edi = [CURstk]
	cmp	edi,BEGstk
	jbe	DecWrap
	sub	EMSEG:[CURstk],Reg87Len
	ret

DecWrap:
	mov	EMSEG:[CURstk],INITstk
	ret


EM_ENTRY eFINCSTP
eFINCSTP:
;edi = [CURstk]
	cmp	edi,INITstk
	jae	IncWrap
	add	EMSEG:[CURstk],Reg87Len
	ret

IncWrap:
	mov	EMSEG:[CURstk],BEGstk
	ret


eFCLEX:
	mov	EMSEG:[SWerr],0
	and	[esp+4].OldLongStatus,0FFFF00FFH		; clear saved SWerr
	ret


;*** eFSTENV - emulate FSTENV	[address]
;
;   ARGUMENTS
;	    dseg:esi  = where to store environment
;
;
;   DESCRIPTION
;	    This routine emulates an 80387 FSTENV (store environment)
;

EM_ENTRY eFSTENV
eFSTENV:
	mov	ax,[esp+4].OldStatus
	mov	EMSEG:[StatusWord],ax
SaveEnv:
	xor	ax,ax
	mov	dseg:[esi.reserved1],ax
	mov	dseg:[esi.reserved2],ax
	mov	dseg:[esi.reserved3],ax
	mov	dseg:[esi.reserved4],ax
	mov	dseg:[esi.reserved5],ax
	mov	ax,EMSEG:[ControlWord]
	mov	dseg:[esi.E32_ControlWord],ax
	call	GetEMSEGStatusWord
	mov	dseg:[esi.E32_StatusWord],ax
	call	GetTagWord
	mov	dseg:[esi.E32_TagWord],ax
	mov	ax,cs
	mov	dseg:[esi.E32_CodeSeg],ax
	mov	ax,ss
	mov	dseg:[esi.E32_DataSeg],ax
	mov	eax,EMSEG:[PrevCodeOff]
	mov	dseg:[esi.E32_CodeOff],eax
	mov	eax,EMSEG:[PrevDataOff]
	mov	dseg:[esi.E32_DataOff],eax
        mov     EMSEG:[CWmask],03FH        ;Set all mask bits
	mov	EMSEG:[ErrMask],0
	ret


;*** eFSAVE - emulate FSAVE   [address]
;
;   ARGUMENTS
;	    dseg:esi  = where to store environment
;
;
;   DESCRIPTION
;	    This routine emulates an 80387 FSAVE (store environment)
;	    Once the data is stored an finit is executed.
;
;   REGISTERS
;	destroys ALL.

EM_ENTRY eFSAVE
eFSAVE:
	mov	ax,[esp+4].OldStatus
	mov	EMSEG:[StatusWord],ax
        mov     eax,[esp+4].OldCodeOff
        mov     EMSEG:[PrevCodeOff],eax
	push	offset eFINIT		; After fsave we must do a finit
SaveState:				; Enter here for debugger save state
	call	SaveEnv
	add	esi,size Env80x87_32	;Skip over environment
	mov	ebp,NumLev		;Save entire stack
	mov	edi,EMSEG:[CURstk]
FsaveStoreLoop:
	mov	eax,EMSEG:[edi].ExpSgn
	call	StoreTempReal		;in emstore.asm
        add     esi,10

        mov     edi,EMSEG:[CURstk]
        NextStackElem   edi,FSave
        mov     EMSEG:[CURstk],edi

        dec     ebp
	jnz	FsaveStoreLoop
	ret

WrapFSave:                              ; tied to NextStackElem above
        mov     edi, BEGstk
        mov     EMSEG:[CURstk],edi
        dec     ebp
        jnz     FsaveStoreLoop
        ret


;*** eFRSTOR - emulate FRSTOR  [address]
;
;   ARGUMENTS
;	    dseg:esi  = where to get the environment
;
;   DESCRIPTION
;	    This routine emulates an 80387 FRSTOR (restore state)

	NextStackWrap	edi,Frstor

EM_ENTRY eFRSTOR
eFRSTOR:
;First we set up the status word so that [CURstk] is initialized.
;The floating-point registers are stored in logical ST(0) - ST(7) order,
;not physical register order.  We don't do a full load of the environment
;because we're not ready to use the tag word yet.

    and		[esp+4].[OldLongStatus], NOT(LongSavedFlags)	;clear saved codes, errs
	mov	ax, dseg:[esi.E32_StatusWord]
	call	SetEmStatusWord		;Initialize [CURstk]
	add	esi,size Env80x87_32	;Skip over environment

;Load of temp real has one difference from real math chip: it is an invalid
;operation to load an unsupported format.  By ensuring the exception is
;masked, we will convert unsupported format to Indefinite.  Note that the
;mask and [CURerr] will be completely restored by the FLDENV at the end.

        mov     EMSEG:[CWmask],3FH              ;Mask off invalid operation exception
	mov	edi,EMSEG:[CURstk]
	mov	ebp,NumLev
FrstorLoadLoop:
	push	esi
	call	LoadTempReal		;In emload.asm
	pop	esi
	add	esi,10		;Point to next temp real
	NextStackElem	edi,Frstor
	dec	ebp
	jnz	FrstorLoadLoop
	sub	esi,NumLev*10+size Env80x87_32	;Point to start of env.
        jmp     eFLDENV                 ;Fall into eFLDENV


;***	eFLDENV - emulate FLDENV   [address]
;
;	ARGUMENTS
;	       dseg:si	= where to store environment
;
;	       This routine emulates an 80387 FLDENV (load environment)

EM_ENTRY eFLDENV
eFLDENV:
    and		[esp+4].[OldLongStatus], NOT(LongSavedFlags)	;clear saved codes, errs
	mov		ax, dseg:[esi.E32_StatusWord]
	call	SetEmStatusWord			; set up status word
	mov		ax, dseg:[esi.E32_ControlWord]
	call	SetControlWord
	mov		ax, dseg:[esi.E32_TagWord]
	call	UseTagWord
	mov		eax, dseg:[esi.E32_CodeOff]
	mov     EMSEG:[PrevCodeOff], eax
	mov		eax, dseg:[esi.E32_DataOff]
	mov     EMSEG:[PrevDataOff], eax
	ret


;***	GetTagWord - figures out what the tag word is from the numeric stack
;		   and returns the value of the tag word in ax.
;

GetTagWord:
	push	esi
	xor	eax, eax
	mov	ecx, NumLev		; get tags for regs. 0, 7 - 1
	mov	esi,INITstk
GetTagLoop:
	mov	bh, EMSEG:[esi.bTag]	; The top 2 bits of Tag are the X87 tag bits.
	shld	ax, bx, 2
	sub	esi, Reg87Len
	loop	GetTagLoop
	rol	ax, 2			; This moves Tag(0) into the low 2 bits
	pop	esi
	ret


;***	UseTagWord - Set up tags using tag word from environment
;
;	ARGUMENTS
;	       ax - should contain the tag word
;
;	Destroys ax,bx,cx,dx,di

UseTagWord:
	ror	ax, 2			; mov Tag(0) into top bits of ax
	mov	edi,INITstk
	mov	ecx, NumLev
UseTagLoop:
	mov	dl,bTAG_EMPTY
	cmp	ah, 0c0h		;Is register to be tagged Empty?
	jae	SetTag			;Yes, go mark it
	mov	dl,EMSEG:[edi].bTag	;Get current tag
	cmp	dl,bTAG_EMPTY		;Is register currently Empty?
	je	SetTagNotEmpty		;If so, go figure out tag for it
SetTag:
	mov	EMSEG:[edi].bTag,dl
UseTagLoopCheck:
	sub	edi, Reg87Len
	shl	eax, 2
	loop	UseTagLoop
	ret

SetTagEmpty:
	mov	EMSEG:[edi.bTag], bTAG_EMPTY
	jmp	UseTagLoopCheck

SetTagNotEmpty:
;Register is currently tagged empty, but new tag word says it is not empty.
;Figure out a new tag for it.  The rules are:
;
;1. Everything is either normalized or zero--unnormalized formats cannot
;get in.  So if the high half mantissa is zero, the number is zero.
;
;2. Although the exponent bias is different, NANs and Infinities are in
;standard IEEE format - exponent is TexpMax, mantissa indicates NAN vs.
;infinity (mantissa for infinity is 800..000H).
;
;3. Denormals have an exponent less than TexpMin.
;
;4. If the low half of the mantissa is zero, it is tagged bTAG_SNGL
;
;5. Everything else is bTAG_VALID

	mov	ebx,EMSEG:[edi].lManHi
	mov	dl,bTAG_ZERO		;Try zero first
	or	ebx,ebx			;Is mantissa zero?
	jz	SetTag
	mov	edx,EMSEG:[edi].ExpSgn
	mov	dl,bTAG_DEN
	cmp	edx,TexpMin shl 16	;Is it denormal?
	jl	SetTag
	cmp	EMSEG:[edi].lManLo,0	;Is low half zero?
.erre	bTAG_VALID eq 1
.erre	bTAG_SNGL eq 0
	setnz	dl			;if low half==0 then dl=0 else dl=1
	cmp	edx,TexpMax shl 16	;Is it NAN or Infinity?
	jl	SetTag			;If not, it's valid
.erre	(bTAG_VALID - bTAG_SNGL) shl TAG_SHIFT eq (bTAG_NAN - bTAG_INF)
	shl	dl,TAG_SHIFT
	add	dl,bTAG_INF - bTAG_SNGL
;If the low bits were zero we have just changed bTAG_SNGL to bTAG_INF
;If the low bits weren't zero, we changed bTAG_VALID to bTAG_NAN
;See if infinity is really possible: is high half 80..00H?
	cmp	ebx,1 shl 31		;Is it infinity?
	jz	SetTag			;Store tag for infinity or NAN
	mov	dl,bTAG_NAN
	jmp	SetTag


;***	GetStatusWord -
;
; User status word returned in ax.
; Destroys ebx only.

GetStatusWord:
	mov	eax, EMSEG:[CURstk]
	sub	eax, BEGstk
	mov	bl,Reg87Len
	div	bl
        inc     eax                     ; adjust for emulator's stack layout
	and	eax, 7			; eax is now the stack number
	shl	ax, 11
	or	ax,[esp+8].OldStatus	; or in the rest of the status word.
	ret


;***	GetEMSEGStatusWord -
;
; User status word returned in ax.
; Destroys ebx only.
; Uses status word in per-thread data area, otherwise
;   identical to GetStatusWord

EM_ENTRY eGetStatusWord
GetEMSEGStatusWord:
	mov	eax, EMSEG:[CURstk]
	sub	eax, BEGstk
	mov	bl,Reg87Len
	div	bl
        inc     eax                     ; adjust for emulator's stack layout
	and	eax, 7			; eax is now the stack number
	shl	ax, 11
	or	ax, EMSEG:[StatusWord]	; or in the rest of the status word.
	ret


;***	SetEmStatusWord -
;
; Given user status word in ax, set into emulator.
; Destroys ebx only.


SetEmStatusWord:
	and	ax,7F7FH
	mov	bx,ax
        and     bx,3FH                  ; set up CURerr in case user
	mov	EMSEG:[CURerr],bl	; wants to force an exception
	mov	ebx, eax
	and	ebx, not (7 shl 11)	; remove stack field.
	mov	EMSEG:[StatusWord], bx

	sub	ah, 8  			; adjust for emulator's stack layout
	and	ah, 7 shl 3
	mov	al, ah
	shr	ah, 1
	add	al, ah			; stack field * 3 * 4
.erre	Reg87Len eq 12	
	and	eax, 255	   	; eax is now 12*stack number
        add     eax, BEGstk
	mov	EMSEG:[CURstk], eax
	ret


public _SaveEm87Context
_SaveEm87Context PROC

	push	ebp
	mov	ebp, esp
	push	ebx
	push	edi
	push	esi
	mov	esi, [ebp+8]
	call	SaveState
	test	EMSEG:[CURErr], Summary
	jne	RetSaveEmIdle
	mov	eax, Em87Busy
	jmp	RetSaveEm
RetSaveEmIdle:
	mov	eax, Em87Idle
RetSaveEm:
	pop	esi
	pop	edi
	pop	ebx
	pop	ebp
	ret
_SaveEm87Context ENDP


public _RestoreEm87Context
_RestoreEm87Context PROC
	push	ebp
	mov	ebp, esp
	push	ebx
	push	edi
	push	esi
	mov	esi, [ebp+8]
	call	eFRSTOR
	pop	esi
	pop	edi
	pop	ebx
	pop	ebp
	ret
_RestoreEm87Context  ENDP
