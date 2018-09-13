PAGE,132
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       LIBENTRY.ASM
;
;       Windows dynamic link library entry routine
;
;   This module generates a code segment called INIT_TEXT.
;   It initializes the local heap if one exists and then calls
;   the C routine LibMain() which should have the form:
;   BOOL FAR PASCAL LibMain(HANDLE hInstance,
;                           WORD   wDataSeg,
;                           WORD   cbHeap,
;                           DWORD  ignore);     /* Always NULL - ignore */
;
;   The result of the call to LibMain is returned to Windows.
;   The C routine should return TRUE if it completes initialization
;   successfully, FALSE if some error occurs.
;
;   Note - The last parameter to LibMain is included for compatibility
;   reasons.  Applications that wish to modify this file and remove the
;   parameter from LibMain may do so by simply removing the two
;   "push" instructions below marked with "****".
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

include cmacros.inc

externFP <LibMain>               ; the C routine to be called

createSeg INIT_TEXT, INIT_TEXT, BYTE, PUBLIC, CODE
sBegin	INIT_TEXT
assumes CS,INIT_TEXT

?PLM=0                           ; 'C'naming
externA  <_acrtused>             ; ensures that Win DLL startup code is linked

?PLM=1                           ; 'PASCAL' naming
externFP <LocalInit>             ; Windows heap init routine

cProc   LibEntry, <PUBLIC,FAR>   ; entry point into DLL

;include CONVDLL.INC

cBegin
        push    di               ; handle of the module instance
        push    ds               ; library data segment
        push    cx               ; heap size
        push    es               ; Always NULL  ****  May remove (see above)
        push    si               ; Always NULL  ****  May remove (see above)

        ; if we have some heap then initialize it
        jcxz    callc            ; jump if no heap specified

        ; call the Windows function LocalInit() to set up the heap
        ; LocalInit((LPSTR)start, WORD cbHeap);

        xor     ax,ax
        cCall   LocalInit <ds, ax, cx>
        or      ax,ax            ; did it do it ok ?
        jz      error            ; quit if it failed

        ; invoke the C routine to do any special initialization

callc:
        call    LibMain          ; invoke the 'C' routine (result in AX)
        jmp short exit           ; LibMain is responsible for stack clean up

error:
	pop	si		 ; clean up stack on a LocalInit error
        pop     es
        pop     cx
        pop     ds
        pop     di

exit:

cEnd

sEnd	INIT_TEXT

end LibEntry

