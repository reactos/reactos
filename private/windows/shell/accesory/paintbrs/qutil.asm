;****************************Module*Header*******************************
; Copyright (c) 1987 - 1991  Microsoft Corporation                      *
;************************************************************************
        title   qutil.asm

;****************************************************************/
;*     assembly code utilities                            */
;****************************************************************/

.xlist
?MEDIUM equ 1
?WIN = 1
?PLM = 0
include cmacros.inc
.list

time	struc
	hour	 dw  ?
	hour12	 dw  ?
	hour24	 dw  ?
	minute	 dw  ?
	second	 dw  ?
	ampm	 dw  ?
time	ends

date	struc
	month	 dw  ?
	day	 dw  ?
	year	 dw  ?
        dayofweek dw ?
date	ends

sBegin  DATA
sEnd    DATA

sBegin  CODE

assumes CS,CODE
assumes DS,DATA

        extrn   DOS3Call : far

;---------------------------------------------
; RepeatMove - very fast repeat move with long args
;	Syntax   RepeatMove(char far *dest, char far *src, cnt)
;---------------------------------------------
cProc   RepeatMove,<PUBLIC,FAR>,<di,si>
	parmD   lpDest
	parmD   lpSrc
	parmW   cnt
cBegin
	push    ds
	les     di,lpDest
	lds     si,lpSrc
	mov     cx,cnt
	mov	BX,CX
;see which is bigger
	cmp	SI,DI		;is source bigger than dest?
	ja	repm1		;this is only a 16 bit guess, but...
				;if they differ in segments it should be
				;irrelevant
	std
	add     si,cx
	add     di,cx
	dec     si
	dec     di
	shr	BX,1
	jnc	repm0a
	movsb
repm0a:
	shr	CX,1
	jcxz	repm2
	dec	si
	dec	di
	rep	movsw
	jmp	short repm2
;move upwards
repm1:
	cld
	shr	CX,1
	jcxz	repm1a
	rep	movsw
repm1a:
	shr	BX,1
	jnc	repm2
	movsb
repm2:
	cld
	pop     ds
cEnd

;---------------------------------------------
; RepeatFill - very fast repeat fill with long dest
;	Syntax   RepeatFill(char far *dest, databyte, cnt)
;---------------------------------------------
cProc   RepeatFill,<PUBLIC,FAR>,<di>
	parmD   lpDest
	parmB   bData
	parmW   cnt
cBegin
	les     di,lpDest
	mov	CX,cnt
	mov	AL,bData
	cld
	mov	AH,AL
	mov	BX,CX
	shr	CX,1
	jcxz	rpf1
	rep	stosw
rpf1:
	shr	BX,1
	jnc	rpf2
	stosb
rpf2:
cEnd

;---------------------------------------------
; DeleteFile delete a file
; 	syntax: DeleteFile(LPSTR filename)
;   returns 0 or DOS error number
;---------------------------------------------

cProc   DeleteFile,<PUBLIC,FAR>,<si,di>
	parmD src
cBegin
        push    ds
        lds     dx,src
	mov     ah,41h
        call    DOS3Call
	jc      @F
	xor     ax,ax
@@:
        pop     ds
cEnd

;---------------------------------------------
; SetCurrentDrive set the current drive
; 	syntax: SetCurrentDrive(int drive_number)
;   returns 0 or DOS error number
;---------------------------------------------

cProc   SetCurrentDrive,<PUBLIC,FAR>,<si,di>
	parmW drive
cBegin
        mov     dl, byte ptr drive
	mov     ah,0Eh
        call    DOS3Call
	jc      @F
	xor     ax,ax
@@:
cEnd

;---------------------------------------------
; chdir set current directory
; 	syntax: chdir(LPSTR path)
;   returns 0 or DOS error number
;---------------------------------------------

cProc   chdir,<PUBLIC,FAR>,<si,di>
	parmD src
cBegin
        push    ds
        lds     dx,src
	mov     ah,3Bh
        call    DOS3Call
	jc      @F
	xor     ax,ax
@@:
        pop     ds
cEnd


;---------------------------------------------
; getcwd get current directory
; 	syntax: getcwd(LPSTR path, int path_length)
;   returns 0 or DOS error number
;---------------------------------------------

cProc   getcwd,<PUBLIC,FAR>,<si,di>
	parmD path
        parmW len

        localV temppath, 67
cBegin
        push    ds
        push    es

        cld                     ; we use forward string ops

        cmp     len, 4          ; will at least x:\ fit?
        jb      @F              ; nope

        mov     ax, ss          ; point ds & es to stack
        mov     ds, ax
        mov     es, ax

        lea     di, temppath    ; fill in drive
        mov     si, di
        mov     ah, 19h
        call    DOS3Call
        jc      @F
        add     al, 'A'         ; convert to ascii
        stosb
        mov     al, ':'
        stosb
        mov     al, '\'
        stosb

        mov     si, di          ; get current working directory
        xor     dl, dl          ; current drive
	mov     ah,47h
        call    DOS3Call
	jc      @F

        mov     byte ptr [di+63], 0     ; make sure string is null-terminated

        les     di, path        ; copy string to caller's buffer
        lea     si, temppath
        mov     cx, len
        rep     movsb

	xor     ax,ax
@@:
        pop     es
        pop     ds
cEnd


cProc	GetTime, <PUBLIC>,<si,di>
        parmW   pTime               ; pointer to the structure to fill

cBegin
        mov     ax, 2c00h           ; get time
        int     21h
        mov     bx, pTime

	xor	ax,ax
        mov     al,ch
	mov	[bx].hour24, ax
	mov	al,cl
        mov     [bx].minute, ax
        mov     al,dh
        mov     [bx].second, ax
cEnd

cProc	GetDate, <PUBLIC>,<si,di>
	parmW	pDate		    ; pointer to the structure to fill

cBegin
	mov	ax, 2a00h	    ; get date
        int     21h
	mov	bx, pDate

	mov	[bx].year, cx

        xor     ah, ah
        mov     [bx].dayofweek, ax

	mov	al,dh
	mov	[bx].month, ax
	mov	al,dl
	mov	[bx].day, ax
cEnd


sEnd    CODE

end

