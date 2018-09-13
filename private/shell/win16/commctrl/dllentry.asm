PAGE,132
;***************************************************************************
;*
;*   DLLENTRY.ASM
;*
;*	VER.DLL Entry code
;*
;*	This module generates a code segment called _INIT.
;*	It initializes the local heap if one exists and then calls
;*	the C routine LibMain() which should have the form:
;*	BOOL FAR PASCAL LibMain(HANDLE hInstance,
;*				WORD   wDataSeg,
;*				WORD   cbHeap,
;*				LPSTR  lpszCmdLine);
;*
;*	The result of the call to LibMain is returned to Windows.
;*	The C routine should return TRUE if it completes initialization
;*	successfully, FALSE if some error occurs.
;*
;**************************************************************************

	INCLUDE CMACROS.INC

externFP <LIBMAIN>               ;The C routine to be called

ifndef SEGNAME
    SEGNAME equ <_INIT>         ; default seg name
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE


sBegin	CodeSeg          ; this defines what seg this goes in
assumes cs,CodeSeg

?PLM=0                           ;'C'naming
externA  <_acrtused>             ;Ensures that Win DLL startup code is linked

?PLM=1                           ;'PASCAL' naming
externFP <LOCALINIT>             ;Windows heap init routine

cProc   LibEntry, <PUBLIC,FAR>   ;Entry point into DLL

cBegin
        push    di               ;Handle of the module instance
        push    ds               ;Library data segment
        push    cx               ;Heap size
        push    es               ;Command line segment
        push    si               ;Command line offset

        ;** If we have some heap then initialize it
        jcxz    callc            ;Jump if no heap specified

        ;** Call the Windows function LocalInit() to set up the heap
        ;**	LocalInit((LPSTR)start, WORD cbHeap);

        xor     ax,ax
        cCall   LOCALINIT <ds, ax, cx>
        or      ax,ax            ;Did it do it ok ?
        jz      error            ;Quit if it failed

        ;** Invoke the C routine to do any special initialization

callc:
        call    LIBMAIN          ;Invoke the 'C' routine (result in AX)
        jmp short exit           ;LibMain is responsible for stack clean up

error:
	pop	si		 ;Clean up stack on a LocalInit error
        pop     es
        pop     cx
        pop     ds
        pop     di
exit:

cEnd

sEnd

;;
;;  Thunks for all the Window procedures.
;;
;;  USER.EXE RegisterClass tries to verify that the WndProc is valid
;;  this causes the segment that contains the WndProc to be broght in
;;  we Register all our classes at startup and we dont want them to come in
;;
WNDPROC macro Name
externFP &Name

sBegin	CodeSeg          ; this defines what seg this goes in
assumes cs,CodeSeg

public _&Name
_&Name&:
        jmp Name
sEnd
        endm

ifndef WIN31

WNDPROC TV_WndProc
WNDPROC ListView_WndProc
ifdef WANT_SUCKY_HEADER
WNDPROC HeaderWndProc
endif
WNDPROC Header_WndProc
WNDPROC HotKeyWndProc
WNDPROC ProgressWndProc
WNDPROC ToolbarWndProc
WNDPROC StatusWndProc
WNDPROC TrackBarWndProc
WNDPROC ToolTipsWndProc
WNDPROC ButtonListBoxProc

else
ifdef IEWIN31_25

WNDPROC TV_WndProc
WNDPROC ListView_WndProc
ifdef WANT_SUCKY_HEADER
WNDPROC HeaderWndProc
endif
WNDPROC Header_WndProc

endif ;IEWIN31_25
endif ; !WIN31

WNDPROC UpDownWndProc
WNDPROC Tab_WndProc

END LibEntry
