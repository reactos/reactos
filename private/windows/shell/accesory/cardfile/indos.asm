        title   indos.asm

;************************************************************************/
;*                                                              	*/
;*  Windows Cardfile - Written by Mark Cliggett                 	*/
;*  (c) Copyright Microsoft Corp. 1985, 1991 - All Rights Reserved	*/
;*                                                              	*/
;************************************************************************/

.xlist
include cmacros.inc
.list

EXTRN	DOS3Call:FAR		    ; Replacement function for INT 21H

createSeg   _FILE,nrfile,byte,public,CODE

sBegin  DATA
sEnd    DATA

sBegin  NRFILE

assumes CS,NRFILE
assumes DS,DATA

cProc   Frename,<PUBLIC,FAR>,<di>
    parmW src
    parmW dst
cBegin
        mov     dx,src
        push    ds
        pop     es
        mov     di,dst
        mov     ah,56h
;	int	21h		    ; Original INT 21H
	call	DOS3Call	    ; Supports Windows 3.0 and WLO
	jc	renexit
        xor     ax,ax
renexit:
cEnd

;
; Fdelete (pch) - delete a file
;   returns 0 or -1
;

cProc   Fdelete,<PUBLIC,FAR>
    parmW src
cBegin
        mov     dx,src
        mov     ah,41h
;	int	21h		    ; Original INT 21H
	call	DOS3Call	    ; Supports Windows 3.0 and WLO
        jc      rmexit
        xor     ax,ax
rmexit:
cEnd

cProc mylread,<PUBLIC,FAR>
    parmW fh
    parmD buf
    parmW count
cBegin
    push    ds
    mov     bx,fh
    lds     dx,buf
    mov     ah,3fh
    mov     cx,count
;	int	21h		    ; Original INT 21H
	call	DOS3Call	    ; Supports Windows 3.0 and WLO
    jnc     lreaddone
    xor     ax,ax
lreaddone:
    pop     ds
cEnd

cProc myread,<PUBLIC,FAR>
    parmW fh
    parmW buf
    parmW count
cBegin
    mov     bx,fh
    mov     dx,buf
    mov     ah,3fh
    mov     cx,count
;	int	21h		    ; Original INT 21H
	call	DOS3Call	    ; Supports Windows 3.0 and WLO
    jnc     readdone
    xor     ax,ax
readdone:
cEnd

cProc mylwrite,<PUBLIC,FAR>
    parmW   fh
    parmD   buf
    parmW   count
cBegin
    push    ds
    mov     bx,fh
    lds     dx,buf
    mov     ah,40h
    mov     cx,count
;	int	21h		    ; Original INT 21H
	call	DOS3Call	    ; Supports Windows 3.0 and WLO
    jnc     lwritedone
    xor     ax,ax
lwritedone:
    pop     ds
cEnd

cProc mywrite,<PUBLIC,FAR>
    parmW   fh
    parmW   buf
    parmW   count
cBegin
    mov     bx,fh
    mov     dx,buf
    mov     ah,40h
    mov     cx,count
;	int	21h		    ; Original INT 21H
	call	DOS3Call	    ; Supports Windows 3.0 and WLO
    jnc     writedone
    xor     ax,ax
writedone:
cEnd

cProc mylmul,<PUBLIC,FAR>
    parmW   int1
    parmW   int2
cBegin
    mov     ax,int1
    mov     dx,int2
    mul     dx
cEnd

sEnd    NRFILE

end
