	 page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  RLEA.ASM - helper routines for RLE stuff
;
; Created: Thu 27-Jun-1991
; Author:  Todd Laney [ToddLa]
;
; Copyright (c) 1991 Microsoft Corporation
;
; Exported Functions:   none
;
; Public Functions:     DecodeRle386
;
; Public Data:          none
;
; General Description:
;
; Restrictions:
;
; History:
;       Thu 15-Aug-1991 13:45:58 -by-  Todd Laney [ToddLa]
;       Created.
;
;-----------------------------------------------------------------------;

	.xlist
	include cmacros.inc
        include windows.inc
        .list

RLE_ESCAPE  equ 0
RLE_EOL     equ 0
RLE_EOF     equ 1
RLE_JMP     equ 2

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG	struc
lo	dw	?
hi	dw	?
LONG	ends

FARPOINTER	struc
off	dw	?
sel	dw	?
FARPOINTER      ends

wptr    equ     <word ptr>
bptr    equ     <byte ptr>

min_ax  macro   REG
        sub     ax,REG
	cwd
	and	ax,dx
        add     ax,REG
	endm

max_ax  macro   REG
        sub     ax,REG
	cwd
	not	dx
        and     ax,dx
        add     ax,REG
	endm

; Manually perform "push" dword register instruction to remove warning
PUSHD macro reg
	db	66h
	push	reg
endm

; Manually perform "pop" dword register instruction to remove warning
POPD macro reg
	db	66h
	pop	reg
endm

; -------------------------------------------------------
;		DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

sBegin  Data

sEnd  Data

; -------------------------------------------------------
;               CODE SEGMENT DECLARATIONS
; -------------------------------------------------------

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
        assumes cs,CodeSeg
        assumes ds,nothing
        assumes es,nothing

;---------------------------Macro---------------------------------------;
; NOP32 - 32 bit NOP
;
;   put after all string instructions that use esi or edi to fix a wierd
;   386 stepping bug
;
;-----------------------------------------------------------------------;
NOP32   macro
        db      67h
        nop
        endm

;---------------------------Macro---------------------------------------;
; ReadRLE
;
;   read a WORD from rle data
;
; Entry:
;	DS:ESI --> rle data
; Returns:
;	AX - word at DS:[ESI]
;	DS:ESI advanced
; History:
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
ReadRLE macro
if 0    ; lets work on the wierd 386 step
        lods    wptr ds:[esi]
else
        mov     ax, wptr ds:[esi]
        add     esi,2
endif
        endm

;---------------------------Public-Routine------------------------------;
; DecodeRle386
;
;   copy a rle bitmap to a DIB
;
; Entry:
;       pBits       - pointer to rle bits
; Returns:
;       none
; Error Returns:
;	None
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;	INT 10h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
	assumes ds,nothing
        assumes es,nothing

cProc   DecodeRle386, <NEAR, PASCAL, PUBLIC>, <ds>
	ParmD	lpbi
	ParmD	pDest
	ParmD	pBits
cBegin
.386
	PUSHD	di	; push edi
	PUSHD	si	; push esi

        xor     edi,edi
        xor     esi,esi
        xor     eax,eax
        xor     ecx,ecx

	lds	si,lpbi

	mov	ax,wptr [si].biWidth
	add	ax,3
	and	ax,not 3
	movzx	ebx,ax		    ; ebx is next_scan

	les	di,pDest
        lds     si,pBits
        assumes ds,nothing

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; Start of RLE decoding
;
;   DS:SI   --> RLE bits
;   ES:DI   --> screen output (points to start of scan)
;
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
	align	4
RleBltStart:
        mov     edx,edi             ; save start of scan

	align	4
RleBltAlign:
        inc     esi                 ; !!! re-align source
        and     si,not 1

	align	4
RleBltNext:
        ReadRLE                     ; al=count ah=color

        or      al,al               ; is it a escape?
	jz	short RleBltEscape

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a encoded run (al != 0)
;
;   al - run length
;   ah - run color
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
RleBltEncodedRun:
        mov     cl,al
	mov	al,ah

	shr	cx,1
        rep     stos wptr es:[edi]
        NOP32
	jnc	short RleBltNext

        stos    bptr es:[edi]
        NOP32

        jmp     short RleBltNext

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a RLE escape code (al=0)
; Possibilities are:
;       . End of Line            -  ah = 0
;       . End of RLE             -  ah = 1
;       . Delta                  -  ah = 2
;       . Unencoded run          -  ah = 3 or more
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
	align	4
RleBltEscape:
        cmp     ah,al
	je	short RleBltEOL

        inc     al
        cmp     ah,al
	je	short RleBltEOF

        inc     al
        cmp     ah,al
	je	short RleBltDelta
        errn$   RleBltUnencodedRun

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a un-encoded run (ah >= 3)
;
;   ah          is pixel count
;   DS:SI   --> pixels
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
RleBltUnencodedRun:
	mov	cl,ah

	shr	cx,1
        rep     movs wptr es:[edi], wptr ds:[esi]
        NOP32
        jnc     short RleBltAlign

	movs	bptr es:[edi], bptr ds:[esi]
        NOP32
	jmp	short RleBltAlign

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a delta jump, the next two bytes contain the jump values
; note the the jump values are unsigned bytes, x first then y
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
	align	4
RleBltDelta:
        ReadRLE                     ; al = deltaX, ah = deltaY

        or      ah,ah
        jnz     short RleBltDeltaXY

RleBltDeltaX:
	add	edi,eax
        jmp     short RleBltNext

	align	4
RleBltDeltaXY:
        add     edi,ebx
        add     edx,ebx
        dec     ah
        jnz     RleBltDeltaXY

	add	edi,eax
        jmp     short RleBltNext

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a end of line marker, point ES:DI to the begining of the
; next scan
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
RleBltEOL:
        mov     edi,edx             ; go back to start of scan
        add     edi,ebx             ; advance to next scan
        jmp     short RleBltStart   ; go get some more

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a end of rle marker, clean up and exit.
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
RleBltEOF:
        errn$   RleBltExit

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
RleBltExit:
	POPD	si	; pop esi
	POPD	di	; pop edi
.286
cEnd

;---------------------------Macro---------------------------------------;
; ReadRle286
;
;   read a WORD from rle data
;
; Entry:
;       DS:SI --> rle data
; Returns:
;       AX - word at DS:[SI]
;       DS:SI advanced
; History:
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
ReadRle286 macro
        lods    wptr ds:[si]
        endm

;---------------------------Public-Routine------------------------------;
; DecodeRle286
;
;   copy a rle bitmap to a DIB
;
; Entry:
;       pBits       - pointer to rle bits
; Returns:
;       none
; Error Returns:
;	None
; Registers Preserved:
;       BP,DS,SI,DI
; Registers Destroyed:
;       AX,BX,CX,DX,FLAGS
; Calls:
;	INT 10h
; History:
;
;       Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
	assumes ds,nothing
        assumes es,nothing

cProc   DecodeRle286, <NEAR, PASCAL, PUBLIC>, <ds>
	ParmD	lpbi
	ParmD	pDest
	ParmD	pBits
cBegin
	push	di
	push	si

        xor     di,di
        xor     si,si
        xor     ax,ax
        xor     cx,cx

	lds	si,lpbi

	mov	ax,wptr [si].biWidth
	add	ax,3
	and	ax,not 3
	mov	bx,ax		    ; bx is next_scan

	les	di,pDest
        lds     si,pBits
        assumes ds,nothing

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; Start of RLE decoding
;
;   DS:SI   --> RLE bits
;   ES:DI   --> screen output (points to start of scan)
;
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
Rle286Start:
	mov	dx,di		    ; save start of scan

Rle286Next:
        ReadRLE286                  ; al=count ah=color

        or      al,al               ; is it a escape?
        jz      Rle286Escape

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a encoded run (al != 0)
;
;   al - run length
;   ah - run color
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
Rle286EncodedRun:
        mov     cl,al
	mov	al,ah

	shr	cx,1
	rep	stos wptr es:[di]
	adc	cl,cl
	rep	stos bptr es:[di]

        jmp     short Rle286Next

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a RLE escape code (al=0)
; Possibilities are:
;       . End of Line            -  ah = 0
;       . End of RLE             -  ah = 1
;       . Delta                  -  ah = 2
;       . Unencoded run          -  ah = 3 or more
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
Rle286Escape:
        cmp     ah,al
        je      Rle286EOL

        inc     al
        cmp     ah,al
        je      Rle286EOF

        inc     al
        cmp     ah,al
        je      Rle286Delta
        errn$   Rle286UnencodedRun

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a un-encoded run (ah >= 3)
;
;   ah          is pixel count
;   DS:SI   --> pixels
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
Rle286UnencodedRun:
	mov	cl,ah

	shr	cx,1
	rep	movs wptr es:[di], wptr ds:[si]
	adc	cl,cl
        rep     movs bptr es:[di], bptr ds:[si]

	inc	si			  ; !!! re-align source
	and	si,not 1
        jmp     short Rle286Next

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a delta jump, the next two bytes contain the jump values
; note the the jump values are unsigned bytes, x first then y
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
Rle286Delta:
        ReadRLE286                  ; al = deltaX, ah = deltaY

        or      ah,ah
        jnz     Rle286DeltaXY

Rle286DeltaX:
	add	di,ax
        jmp     short Rle286Next

Rle286DeltaXY:
        add     di,bx
        add     dx,bx
        dec     ah
        jnz     Rle286DeltaXY

	add	di,ax
        jmp     short Rle286Next

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a end of line marker, point ES:DI to the begining of the
; next scan
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
Rle286EOL:
        mov     di,dx             ; go back to start of scan
        add     di,bx             ; advance to next scan
        jmp     short Rle286Start ; go get some more

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; We have found a end of rle marker, clean up and exit.
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
Rle286EOF:
        errn$   Rle286Exit

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
Rle286Exit:
	pop	si
	pop	di
cEnd

sEnd

sEnd

end
