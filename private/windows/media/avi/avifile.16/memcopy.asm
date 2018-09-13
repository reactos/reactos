; mem.asm:
;
; masm -Mx -Zi -DSEGNAME=????? asm.asm
;
	TITLE MEM.ASM

;****************************************************************
;* MEM.ASM - Assembly mem-copy routines				*
;*		for 80286 and 80386				*
;****************************************************************
;

?PLM=1	    ; PASCAL Calling convention is DEFAULT
?WIN=0      ; Windows calling convention
PMODE=1

.xlist
include cmacros.inc
include windows.inc
.list

	externA	    __WinFlags	    ; in KERNEL
	externA	    __AHINCR	    ; in KERNEL
	externA	    __AHSHIFT	    ; in KERNEL

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG	struc
lo	dw	?
hi	dw	?
LONG	ends

FARPOINTER	struc
off	dw	?
sel	dw	?
FARPOINTER	ends

; -------------------------------------------------------
;		DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin Data
sEnd Data

sBegin CodeSeg
        assumes cs,CodeSeg
        assumes ds,DATA

;---------------------------Public-Routine------------------------------;
; hmemcpy
;
;   copy memory
;
; Entry:
;	lpSrc	HPSTR to copy from
;	lpDst	HPSTR to copy to
;	cbMem	DWORD count of bytes to move
;
;	NOTE: overlapped copies will work iff lpSrc.sel == lpDst.sel
;		[This is a lie.	 They will always work.]
;
; Returns:
;	destination pointer
; Error Returns:
;	None
; Registers Preserved:
;	BP,DS,SI,DI
; Registers Destroyed:
;	AX,BX,CX,DX,FLAGS
; Calls:
;	nothing
; History:
;
;	Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;	Tue 16-Oct-1990 16:41:00 -by-  David Maymudes [DavidMay]
;	Modified 286 case to work correctly with cbMem >= 64K.
;	Changed name to hmemcpy.
;	Changed 386 case to copy by longwords
;-----------------------------------------------------------------------;

cProc MemCopy,<FAR,PASCAL,PUBLIC,NODATA>,<>
;	 ParmD	 lpDst
;	 ParmD	 lpSrc
;	 ParmD	 cbMem
cBegin	<nogen>
        mov     ax,__WinFlags
        test    ax,WF_CPU286
        jz      fmemcpy386
	jmp	FAR PTR	fmemcpy286
cEnd <nogen>

cProc fmemcpy386,<FAR,PASCAL,PUBLIC,NODATA>,<ds>
	ParmD	lpDst
	ParmD	lpSrc
	ParmD	cbMem
cBegin
	.386
	push	edi
	push	esi
	cld

	mov	ecx,cbMem
	jecxz	mc386_exit

	movzx	edi,di
	movzx	esi,si
	lds	si,lpSrc
	les	di,lpDst
;
; calculate differance of pointers in "selector" space
;
	mov	ax,si		; DX:AX = lpSrc
	mov	dx,ds

	mov	bx,es		; BX = selector of ptr B

	mov	cx,__AHSHIFT	; number of selector bits per 64K 'segment'
	shr	dx,cl		; linearize ptr A
	shr	bx,cl		; linearize ptr B
;
; DX and BX contain normalized selectors
;
        mov     ecx,cbMem

        sub     ax,di
        sbb     dx,bx              ; do long subtraction.
        jnc     mc_copy_forward

	add	ax,cx
	adc	dx,cbMem.hi
        jnc     mc_copy_forward    ; carry, so >0, thus they do hit.

	std
	add	edi,ecx
	add	esi,ecx

	sub	edi,4
	sub	esi,4

	push	ecx
	shr	ecx,2		; get count in DWORDs
	rep	movs dword ptr es:[edi], dword ptr ds:[esi]
	db	67H		; Fix strange 386 bug
	add	edi,3
	add	esi,3
	pop	ecx
	and	ecx,3
	rep	movs byte ptr es:[edi], byte ptr ds:[esi]
	db	67H		; Fix strange 386 bug
	jmp	mc386_exit

mc_copy_forward:
	push	ecx
	shr	ecx,2		; get count in DWORDs
	rep	movs dword ptr es:[edi], dword ptr ds:[esi]
	db	67H
	pop	ecx
	and	ecx,3
	rep	movs byte ptr es:[edi], byte ptr ds:[esi]
	db	67H
	nop
mc386_exit:
	cld
	pop	esi
	pop	edi
	mov	dx,lpDst.sel	; return destination address
	mov	ax,lpDst.off
	.286
cEnd

cProc fmemcpy286,<FAR,PASCAL,PUBLIC,NODATA>,<ds,si,di>
	ParmD	lpDst
	ParmD	lpSrc
	ParmD	cbMem
cBegin
	mov	cx,cbMem.lo	; CX holds count
	or	cx,cbMem.hi	; or with high word
	jnz	@f
	jmp	empty_copy
@@:
	lds	si,lpSrc	  ; DS:SI = src
	les	di,lpDst	  ; ES:DI = dst
;
; calculate differance of pointers in "selector" space
;
	mov	ax,si		; DX:AX = lpSrc
	mov	dx,ds

	mov	bx,es		; BX = selector of ptr B

	mov	cx,__AHSHIFT	; number of selector bits per 64K 'segment'
	shr	dx,cl		; linearize ptr A
	shr	bx,cl		; linearize ptr B
;
; DX and BX contain normalized selectors
;
        mov     cx,cbMem.lo

	sub	ax,di
	sbb	dx,bx		; do long subtraction.
        jnc     forward_copy    ; difference is positive, so copy forward

; see if the blocks intersect: is source + count > dest?
; equivalently, is source-dest + count > 0 ?
;	sub	ax,cx
;	sbb	dx,0
;	jnc	next		; This looks wrong.  Recheck!

	add	ax,cx
	adc	dx,cbMem.hi
        jc      reverse_copy    ; carry, so >0, thus they do hit.

forward_copy:
	jmp	next
	
reverse_copy:
; first, we have to set ds:si and es:di to the _ends_ of the blocks

        sub     cx,1
	sbb	cbMem.hi,0	; subtract 2 from (long) count
	
	xor	ax,ax		
	add	si,cx
	adc	ax,cbMem.hi

	push	cx
	mov	cx,__AHSHIFT
	shl	ax,cl
	pop	cx
	mov	bx,ds
	add	ax,bx		; advance DS
	mov	ds,ax

	xor	ax,ax
	add	di,cx
	adc	ax,cbMem.hi

	push	cx
	mov	cx,__AHSHIFT
	shl	ax,cl
	pop	cx
	mov	bx,es
	add	ax,bx		; advance ES
	mov	es,ax

        add     cx,1
	adc	cbMem.hi,0	; restore count
;
;	DS:SI += Count
;	ES:DI += Count
;	While Count != 0 Do
;		Num = MIN(Count,SI+1,DI+1)
;		Reverse Copy "Num" Bytes from DS:SI to ES:DI
;			(SI -= Num, DI -= Num)
;		Count -= Num
;		If Count == 0 Then
;			BREAK
;		If SI == 0xFFFF Then
;			DS -= __AHINCR
;		If DI == 0xFFFF Then
;			ES -= __AHINCR
;
next_r:
	mov	ax,si
	sub	ax,di
	sbb	bx,bx
	and	ax,bx
	add	ax,di		; AX = MIN(SI, DI)
	
	test	cbMem.hi,-1	; is high word not zero?
	jnz	@f		; at least 64k to go

        dec     cx
	sub	ax,cx
	sbb	bx,bx
	and	ax,bx
        add     ax,cx
        inc     cx
@@:
	xor	bx,bx
        add     ax,1            ; AX = Num = MIN(Count-1,SI,DI)+1
	adc	bx,0		; bx==1 if exactly 64k

	xchg	ax,cx
	sub	ax,cx		; Count -= Num
	sbb	cbMem.hi,bx

	std
	shr	bx,1
        rcr     cx,1            ; if bx==1, then cx ends up 0x8000
        dec     si
        dec     di
	rep	movsw
        inc     si              ; realign pointers
	inc	di
        adc     cl,cl
        rep     movsb           ; move last byte, if necessary
	cld

	mov	cx,ax		; restore cx
	or	ax,cbMem.hi

	jz	done		; If Count == 0 Then BREAK

	cmp	si,-1		; if SI wraps, update DS
        jne     @f
	mov	ax,ds
	sub	ax,__AHINCR
	mov	ds,ax		; update DS if appropriate
@@:
	cmp	di,-1		; if DI wraps, update ES
        jne     next_r
	mov	ax,es
	sub	ax,__AHINCR
	mov	es,ax		; update ES if appropriate
	jmp	next_r

;
;	While Count != 0 Do
;		If (Count + SI > 65536) OR (Count + DI > 65536) Then
;			Num = Min(65536-SI, 65536-DI)
;		Else
;			Num = Count
;		Copy "Num" Bytes from DS:SI to ES:DI (SI += Num, DI += Num)
;		Count -= Num
;		If Count == 0 Then
;			BREAK
;		If SI == 0 Then
;			DS += __AHINCR
;		If DI == 0 Then
;			ES += __AHINCR
;
next:
;;;;;;;;mov     ax,cx
;;;;;;;;dec     ax

	mov	ax,di
	not	ax		; AX = 65535-DI

	mov	dx,si
	not	dx		; DX = 65535-SI

	sub	ax,dx
	sbb	bx,bx
	and	ax,bx
	add	ax,dx		; AX = MIN(AX,DX) = MIN(65535-SI,65535-DI)

	; problem: ax might have wrapped to zero

	test	cbMem.hi,-1
	jnz	plentytogo	; at least 64k still to copy
	
	dec	cx		; this is ok, since high word is zero
	sub	ax,cx
	sbb	bx,bx
	and	ax,bx
	add	ax,cx		; AX = MIN(AX,CX)
	inc	cx

plentytogo:
	xor	bx,bx
	add	ax,1		; AX = Num = MIN(count,65536-SI,65536-DI)
				; we must check the carry here!
	adc	bx,0		; BX could be 1 here, if CX==0 indicating
				; exactly 64k to copy
	xchg	ax,cx
	sub	ax,cx		; Count -= Num
	sbb	cbMem.hi,bx

	shr	bx,1
	rcr	cx,1		; if bx==1, then cx ends up 0x8000
	rep	movsw
        adc     cl,cl
        rep     movsb           ; move last byte, if necessary

	mov	cx,ax		; put low word of count back in cx
	or	ax,cbMem.hi

	jz	done		; If Count == 0 Then BREAK

	or	si,si		; if SI wraps, update DS
	jnz	@f
	mov	ax,ds
	add	ax,__AHINCR
	mov	ds,ax		; update DS if appropriate
@@:
	or	di,di		; if DI wraps, update ES
	jnz	next
	mov	ax,es
	add	ax,__AHINCR
	mov	es,ax		; update ES if appropriate
	jmp	next
;
; Restore registers and return
;
done:
empty_copy:
	mov	dx,lpDst.sel	; return destination address
	mov	ax,lpDst.off
cEnd

sEnd

sEnd CodeSeg
end
