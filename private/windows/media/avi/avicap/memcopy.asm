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
; MemCopy
;
;   copy memory
;
; Entry:
;	lpSrc	HPSTR to copy from
;	lpDst	HPSTR to copy to
;	cbMem	DWORD count of bytes to move
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
;-----------------------------------------------------------------------;

cProc MemCopy,<NEAR,PASCAL,PUBLIC>,<ds>
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
        les     di,lpDst

        mov     ebx,ecx
	shr	ecx,2		; get count in DWORDs
	rep	movs dword ptr es:[edi], dword ptr ds:[esi]
        mov     ecx,ebx
	and	ecx,3
	rep	movs byte ptr es:[edi], byte ptr ds:[esi]

mc386_exit:
	cld
	pop	esi
	pop	edi
	.286
cEnd

sEnd CodeSeg
end
