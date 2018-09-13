;****************************Module*Header*******************************
; Copyright (c) 1987 - 1991  Microsoft Corporation                      *
;************************************************************************
;
;  Stub.asm  library stub to do local init for us
;
?WIN=1
?PLM=1
?SMALL equ 1
.xlist
include cmacros.inc
.list

ExternW <__acrtused>

assumes cs,CODE

sBegin  CODE

        extrn   DOS3Call : far

ExternFP    <LocalInit, GlobalNotify>

; CX = size of heap
; DI = module handle
; DS = automatic data segment
; ES:SI = address of command line (not used)
;
cProc   LibInit,<FAR,PUBLIC,NODATA>,<si,di>
include convdll.inc
cBegin
        xor     ax,ax
        jcxz    done                    ; Fail if no heap

        push    ax                      ; LocalInit((LPSTR)NULL, cbHeap);
        push    ax
        push    cx
        call    LocalInit

        or      ax, ax                  ; skip GlobalNotify if LocalInit
        jz      done                    ; failed

        mov     ax, 1

done:
cEnd

cProc   WEP, <FAR,PUBLIC,NODATA>
cBegin  
        mov     ax, 1
cEnd


cProc   _lunlink,<PUBLIC,NEAR>
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

sEnd    CODE

        end     LibInit
