        page    ,132
        title   OLE2STUB.ASM
;***********************************************************************
;*								       *
;*  MODULE      : OLE2STUB.ASM                                          *
;*								       *
;*  DESCRIPTION : Provide functions to allow run-time linking to       *
;*							   the ACM.    *
;*								       *
;*  COPYRIGHT   : Copyright 1991, Microsoft Corp.  All Rights Reserved.*
;*								       *
;*	Author: David Maymudes					       *
;*	Based on similar code for MMSYSTEM by: 			       *
;*               Todd Laney and Matt Saettler - Multimedia Systems     *
;*								       *
;***********************************************************************

;-----------------------------------------------------------------------
;
; Documentation (such as it is)
;
;------------------------------------------------------------------------
;
; Call function as you normally would.  Include OLE2.H normally.  
; However, instead of linking to OLE2.LIB, link to OLE2STUB.OBJ
;
; All functions will return error conditions if OLE2.DLL is not present.
; 
; Because I'm lazy, the calling routine has to load the module into
; memory before calling any of this.
;------------------------------------------------------------------------

page

        .286
	?PLM=1	    ; PASCAL Calling convention is DEFAULT
        ?WIN=0      ; Windows calling convention

        .xlist
	include cmacros.inc
	.list

;*********************************************************************
;               CONSTANT DECLARATIONS
;*********************************************************************

ifndef FALSE
FALSE	 	equ	0
endif
ifndef NULL
NULL	 	equ	0
endif
ifndef MMSYSERR_ERROR
MMSYSERR_ERROR 	equ	1
endif

;*********************************************************************
;               EXTERN DECLARATIONS
;*********************************************************************

	externFP   OutputDebugString
	externFP   _wsprintf
	externFP   GetProcAddress
        externFP   GetModuleHandle

;ifdef DEBUG
;        externFP    __dprintf               ; in DPRINTF.C
;endif

;*********************************************************************
;               STRUCTURE DECLARATIONS
;*********************************************************************

LONG    struc
	lo      dw      ?
	hi      dw      ?
LONG    ends

FARPOINTER      struc
	off     dw      ?
	sel     dw      ?
FARPOINTER      ends

PROCENTRY	struc
	curproc		dd	?	; see parameters to macros, below
        ordinal         dw      ?
	numparms	dw	?
        errret          dd      ?

ifdef DEBUG
	szProc		db	?
endif
PROCENTRY	ends

MODENTRY	struc
	hModule		dw	?
	szModule	db	?
MODENTRY	ends

;*********************************************************************
;               DATA SEGMENT DECLARATIONS
;*********************************************************************

ifndef SEGNAME
        SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

page

;*********************************************************************
;                  MACRO DECLARATIONS
;*********************************************************************

; 
;------------------------------------------------------------------------------
;
; MACRO DOUT
;
; Parms:
;
;  text		Text to output using OutputDebugString when DEBUG is defined
;		Text is automatically appended with CR/LF
;

DOUT macro text
        local   string_buffer

ifdef DEBUG		; only do output if DEBUG is defined

_DATA segment
string_buffer label byte
        db      "&text&",13,10,0
_DATA ends
        pusha
        push    DataBASE
        push    DataOFFSET string_buffer
        call    OutputDebugString
        popa
endif
        endm

; 
;------------------------------------------------------------------------------
;
; MACRO Begin_Module_Table
;
; Parms:
;
; Module_Name 	Name of Module to Run-Time-Link
;
; defines <Module_Name>_Proc Macro
;
; Use End_Module_Table to close

Begin_Module_Table MACRO Module_Name

sBegin DATA

ifdef DEBUG
	public Module_Name&_Module_Table
endif

Module_Name&_Module_Table label word
	dw	-1		; hModule
	db	"&Module_Name&",0

sEnd Data

sBegin CodeSeg
        assumes cs,CodeSeg
        assumes ds,Data
        assumes es,nothing

ifdef DEBUG			; make public so debugger is aware of it
        public load&Module_Name
endif

;
; entry:
;	DS:BX	--> ProcEntry for API being called
;
load&Module_Name& proc far
	; stack frame is not modified or copied
	; vars are still in place

	mov	ax,DataOFFSET Module_Name&_Module_Table
        jmp     LoadModuleStub

load&Module_Name& endp

sEnd CodeSeg

page

; 
;------------------------------------------------------------------------------
;
; MACRO <Module_Name>_Proc
;
; Parms:
;
; Name of procedure	Name of procedure to emulate
; ordinal of exported proc
; # of stack parms	use 0 for CDECL routines
; error return value	default error value for use by FailAPIStub
; fail proc		defaults to FailAPIStub if not specified
;			use custom 'fail' proc to replace functionlity
;			if specified module/proc not found in system
;
Module_Name&_Proc macro ProcName, Ordinal, sizestack, errret, failproc

sBegin Data

ifdef DEBUG
        public Module_Name&&Ordinal&
endif

Module_Name&&Ordinal& label word
ifb     <failproc>
        dd      load&Module_Name
        dw      &Ordinal
        dw      &sizestack
        dd      &errret
else
        dd      load&Module_Name
        dw      &Ordinal
        dw      -1
        dd      &failproc
endif

ifdef DEBUG
	db	"&ProcName&",0
endif

sEnd    Data

sBegin CodeSeg
        assumes cs,CodeSeg
        assumes ds,Data
        assumes es,nothing

public &ProcName&

&ProcName& proc far
	; stack frame is not modified or copied
	; vars are still in place

        mov     bx,DataOFFSET Module_Name&&Ordinal&
        jmp     [bx].curproc                            ; current proc

&ProcName& endp
	
sEnd    CodeSeg

        endm
        endm

page
;------------------------------------------------------------------------------
;
; MACRO End_Module_Table
;
; Parms
; Module_Name	Must be the same as in Begin_Module_Table
;

End_Module_Table macro Module_Name

        purge   Module_Name&_Proc

        endm


;-----------------------------------------------------------------------------
;
;  Helper routines for SHELL
;
;-----------------------------------------------------------------------------
sBegin CodeSeg
        assumes cs,CodeSeg
	assumes ds,Data
	assumes es,nothing

;-----------------------------------------------------------------------------
;
; FailApiStub
;
;  Default handler if Module or Proc Address is not found.
;
;  returns default error code
;
;  entry:
;	DS:BX	--> PROCENTRY
;

FailApiStub proc far

	pop	dx		        ; get return addr
	pop	ax

	add	sp,[bx].numparms	; remove params from stack

	push	ax			; restore return addr
	push	dx
	mov	ax,[bx].errret.lo	; return fail code
	mov	dx,[bx].errret.hi
        retf

FailApiStub endp

;-----------------------------------------------------------------------------
;
; LoadModuleStub
;
;  Initial handler for all procs.  Attempts to load module (if not already
;  loaded) and then gets proc address.  If any errors, sets curproc to
;  failproc for 'unavailable' processing.
;
;  If successful, then sets curproc to imported function and calls it.
;
; entry:
;	DS:BX --> PROCENTRY
;	DS:AX --> MODENTRY
;
; NOTE:  Assumes module is already loaded 
;
;     To be totally general:
;       if can't GetModuleHandle(),
;	needs to do a OpenFile(OF_EXIST,...) + LoadLibrary()
;	needs to FreeLibrary() all DLLs at end/exit
;
LoadModuleStub proc far

ifdef DEBUG
	pusha
	sub	sp,128
	mov	si,sp

	mov	di,ax			; DS:DI --> MODENTRY
	
        push    [bx].ordinal            ; %d

	lea	ax,[bx].szProc		; %ls
	push	ds
	push	ax

	lea	ax,[di].szModule	; %ls
	push	ds
        push    ax

        lea     ax,format_string        ; format string
	push	cs
        push    ax

        push    ss                      ; buffer
	push	si
	call	_wsprintf
        add     sp,9*2                  ; clear 9 words

        cCall   OutputDebugString,<ss,si>

	add	sp,128
	popa
        jmp     short @f
format_string:
        db      "Linking %ls!%ls@%d",13,10,0
@@:
endif
	pusha
	
	mov	si,ax			; ds:[si] --> MODENTRY
	mov	di,bx			; ds:[di] --> PROCENTRY

	mov	ax,[si].hModule		
	or	ax,ax
	jz	LoadModuleStubFail	; module does not exist

	cmp	ax,-1
	jne	LoadModuleStubGetProc	

	lea	ax,[si].szModule
	cCall	GetModuleHandle, <ds,ax>
	mov	[si].hModule,ax
	or	ax,ax
	jz	LoadModuleStubLoad

LoadModuleStubGetProc:
        cCall   GetProcAddress,<ax,0,[di].ordinal>
	or	dx,dx
	jz	LoadModuleStubFail

LoadModuleStubDone:
	mov	[di].curproc.lo,ax
	mov	[di].curproc.hi,dx

	popa
	jmp	[bx].curproc

LoadModuleStubLoad:
	
	;; call load library here after verifying with OpenFile()
	;

	; for now, fall through to error

LoadModuleStubFail:
        DOUT    <*** API not found! ***>

        mov     ax,CodeSegOFFSET FailApiStub
        mov     dx,cs

        cmp     [di].numparms,-1            ; do we have a fail proc?
        jne     LoadModuleStubDone          ; no...use FailApiStub

        mov     ax,[di].errret.lo           ; yes..it is stored in errret
        mov     dx,[di].errret.hi
        jmp     short LoadModuleStubDone    ; use it
	
LoadModuleStub endp

sEnd CodeSeg

page
;*********************************************************************
;                    CODE and DATA
;*********************************************************************


;
; Define OLE2 Run-Time-Load Table

Begin_Module_Table OLE2

OLE2_Proc OleInitialize           2,	4,	8000ffffh
OLE2_Proc OleUninitialize         3,	0,	8000ffffh
OLE2_Proc ReleaseStgMedium        32,	4,	8000ffffh
OLE2_Proc OleSetClipboard         49,	4,	8000ffffh
OLE2_Proc OleGetClipboard         50,	4,	8000ffffh
OLE2_Proc OleFlushClipboard       76,	0,	8000ffffh

;
; end the OLE2 R-T-L table

End_Module_Table OLE2

;*********************************************************************
;                    STUB ROUTINES
;*********************************************************************

; no stub routines for OLE2.


;*********************************************************************
;                    STUPID 'C' RUNTIME HACK
;*********************************************************************

sBegin Code
        assumes cs,Code
        assumes ds,nothing
        assumes es,nothing

        externFP    GlobalAlloc
        externFP    GlobalLock
        externFP    GlobalFree

.386

;
; stutpid hack to get GetDGOUP to work!!!
;
public ___ExportedStub
___ExportedStub:
        mov     ax,DataBASE



%out ***************** WHY IS THE C RUNTIME BROKEN???

GMEM_MOVEABLE equ  0002h

public __fmalloc

__fmalloc proc far

        pop     eax
        pop     bx
        push    bx
        push    eax

GMEM_SHARE    equ  2000h
GMEM_MOVEABLE equ  0002h

        cCall   GlobalAlloc, <GMEM_MOVEABLE+GMEM_SHARE, 0, bx>
        cCall   GlobalLock, <ax>

        retf

__fmalloc endp

public __ffree

__ffree proc far

        pop     eax     ; return addr
        pop     ebx     ; sel:off
        push    ebx
        push    eax

        shr     ebx,16
        cCall   GlobalFree, <bx>
        retf

__ffree endp

sEnd

end
