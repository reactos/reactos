	externFP   GetProcAddress
        externFP   LoadLibrary
        externFP   GetModuleHandle
        externFP   FatalAppExit

ifndef CodeSeg
        CodeSeg equ <Code>
endif

;------------------------------------------------------------------------------
;
; MACRO RTL_PROC
;
; USAGE:
;   RTL_PROC ProcName, ModuleName, Ordinal, loadds?
;
;   ProcName    - api name to stub
;   ModuleName  - module to run-time-link to.
;   Ordinal     - export ordinal
;   loadds?     - should we loadds
;
; EXAMPLE:
;
;   RTL_PROC    ShellAbout, SHELL, 1
;
;------------------------------------------------------------------------------
RTL_PROC macro ProcName, ModuleName, Ordinal, LoadProc, loadds?

sBegin Data

;;public ModuleName&ProcName&
ModuleName&ProcName& label dword
        dw offset Load&ModuleName&ProcName&
        dw seg    Load&ModuleName&ProcName&
ifdef DEBUG
        db  "&ProcName&", 0
endif

sEnd    Data

sBegin CodeSeg
        assumes cs,CodeSeg
        assumes ds,nothing
        assumes es,nothing

public &ProcName&
&ProcName& proc far
        call    LoadProc

ifnb <loadds?>
	mov	bx,DataBASE
        mov     es,bx
        assumes es,Data
else
        assumes ds,Data
endif
        jmp     dword ptr [ModuleName&ProcName&]

&ProcName& endp

;;public Load&ModuleName&ProcName&
Load&ModuleName&ProcName& proc near
ifdef DEBUG
        mov     cx,Ordinal
        lea     bx,ModuleName&ProcName&+4
        call    rtl_debug
endif
ifnb <loadds?>
        Save    <es>
endif
        cCall   GetProcAddress,<ax,0,Ordinal>
        mov     word ptr [ModuleName&ProcName&][0],ax
        mov     word ptr [ModuleName&ProcName&][2],dx
        jmp     short &ProcName&
Load&ModuleName&ProcName& endp

sEnd    CodeSeg

        endm

sBegin CodeSeg
        assumes cs,CodeSeg
	assumes ds,nothing
	assumes es,nothing

;---------------------------------------------------------------------------;
;       cx --> Ordinal
;       es:bx --> szProc
;---------------------------------------------------------------------------;

ifdef DEBUG
	externFP   OutputDebugString
	externFP   _wsprintf

format_string:
        db      "SHELL: RTL: %ls@%d",13,10,0

rtl_debug proc near

        push    ds
        push    es
	pusha
	sub	sp,128
	mov	si,sp

        push    cx                      ; %d
        push    es                      ; %ls
        push    bx

        lea     ax,format_string        ; format string
	push	cs
        push    ax

        push    ss                      ; buffer
	push	si
	call	_wsprintf
        add     sp,7*2                  ; clear 9 words

        cCall   OutputDebugString,<ss,si>

        add     sp,128
        popa
        pop     es
        pop     ds
        ret

rtl_debug endp

endif

sEnd CodeSeg
