;****************************Module*Header*******************************
; Copyright (c) 1987 - 1991  Microsoft Corporation                      *
;************************************************************************
;
;	nocrt.asm
;
;	linking with this object file will resolved all the WINSTART externals
;	without bringing in any of the C runtime code, chopping down the size
;	of _TEXT and DGROUP.
;
;	Needless to say, you cannot use any C runtime functions if you do so.
;
;	Created 2-1-89, craigc
;

?PLM = 1
?WIN = 1
memS = 1

.xlist
include cmacros.inc
.list

ExternW <__acrtused>

sBegin	data

__psp	dw	?	; winstart assigns this variable to the PSP segment

sEnd

sBegin  code

assumes CS,CODE

public __setargv,__setenvp,__nmalloc
public __psp,__cinit,_exit

dummy	proc	near
__nmalloc:		; called by myalloc (support for setargv and setenvp)
__setargv:		; called to set __argv and __argc
__setenvp:		; sets up environment (in DGROUP!)
__cinit:		; C runtime initialization
	xor	ax,ax
	ret
dummy	endp

;
;	_exit -
;
;	called by WINSTART after WinMain returns to exit the task
;	will assemble with a "possible invalid use of nogen" since there's
;	no point in the last half of the stack frame code...
;

cProc	_exit, <NEAR,PUBLIC>
	parmW status
cBegin
	mov	ax,status
	mov	ah,4Ch
	int	21h
cEnd	<nogen>

sEnd

end
