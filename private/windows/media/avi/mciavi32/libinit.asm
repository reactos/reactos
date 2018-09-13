	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  LIBINIT.ASM
;
; library stub to do local init for a Dynamic linked library
;
; Created: 06-27-89
; Author:  Todd Laney [ToddLa]
;
; Exported Functions:   none
;
; Public Functions:     none
;
; Public Data:		none
;
; General Description:
;
; Restrictions:
;
;   This must be the first object file in the LINK line, this assures
;   that the reserved parameter block is at the *base* of DGROUP
;
;-----------------------------------------------------------------------;

?PLM=1      ; PASCAL Calling convention is DEFAULT
?WIN=1	    ; Windows calling convention

        .286
	.xlist
	include cmacros.inc
;       include windows.inc
        .list

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

;-----------------------------------------------------------------------;
;
;   externs from KERNEL
;
        externFP    <LocalInit>
        externFP    <FatalAppExit>

;-----------------------------------------------------------------------;
;
;   LibMain is the function in C code we will call on a DLL load.
;   it is assumed in the same segment as we are.
;
;;;;;;;;externNP    <LibMain>
        externFP    <LibMain>  ;; Use this line if LibMain is far call

;-----------------------------------------------------------------------;
;
; Stuff needed to avoid the C runtime coming in, and init the windows
; reserved parameter block at the base of DGROUP
;
sBegin  Data
assumes DS,Data
            org 0               ; base of DATA segment!

            DD  0               ; So null pointers get 0
maxRsrvPtrs = 5
            DW  maxRsrvPtrs
usedRsrvPtrs = 0
labelDP     <PUBLIC,rsrvptrs>

DefRsrvPtr  MACRO   name
globalW     name,0
usedRsrvPtrs = usedRsrvPtrs + 1
ENDM

DefRsrvPtr  pLocalHeap          ; Local heap pointer
DefRsrvPtr  pAtomTable          ; Atom table pointer
DefRsrvPtr  pStackTop           ; top of stack
DefRsrvPtr  pStackMin           ; minimum value of SP
DefRsrvPtr  pStackBot           ; bottom of stack

if maxRsrvPtrs-usedRsrvPtrs
            DW maxRsrvPtrs-usedRsrvPtrs DUP (0)
endif

public  __acrtused
	__acrtused = 1

sEnd        Data

;-----------------------------------------------------------------------;

sBegin  CodeSeg
        assumes cs,CodeSeg

;--------------------------Private-Routine-----------------------------;
;
; LibEntry - called when DLL is loaded
;
; Entry:
;       CX    = size of heap
;       DI    = module handle
;       DS    = automatic data segment
;       ES:SI = address of command line (not used by a DLL)
;
; Returns:
;       AX = TRUE if success
; Error Returns:
;       AX = FALSE if error (ie fail load process)
; Registers Preserved:
;	SI,DI,DS,BP
; Registers Destroyed:
;       AX,BX,CX,DX,ES,FLAGS
; Calls:
;	None
; History:
;
;       06-27-89 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
        assumes ds,Data
        assumes es,nothing

cProc   LibEntry,<FAR,PUBLIC,NODATA>,<>
cBegin
ifdef DEBUG
        ;
        ; if this module is not linked first the reserved parameter block
        ; will not be initialized correctly, check for this and
        ;
        lea     ax,pLocalHeap
        cmp     ax,6
        je      RsrvPtrsOk

RsrvPtrsHosed:
        int     3

        lea     ax,RsrvPtrsMsg
        cCall   FatalAppExit,<0,cs,ax>
        jmp     short RsrvPtrsOk

RsrvPtrsMsg:
        db      'RsrvPtrs hosed!',0

RsrvPtrsOk:
endif
	;
        ; Push frame for LibMain (hModule,cbHeap,lpszCmdLine)
	;
	push	di
	push	cx
	push	es
	push	si

        ;
        ; Init the local heap (if one is declared in the .def file)
        ;
        jcxz no_heap

        cCall   LocalInit,<0,0,cx>

no_heap:
        cCall   LibMain
cEnd

;--------------------------Exported-Routine-----------------------------;
;
;   WEP()
;
;   called when the DLL is unloaded, it is passed 1 WORD parameter that
;   is TRUE if the system is going down, or zero if the app is
;
;   WARNING:
;
;       This function is basicly useless, you cant can any kernel function
;       that may cause the LoadModule() code to be reentered..
;
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   WEP,<FAR,PUBLIC,NODATA>,<>
        ParmW  WhyIsThisParamBogusDave?
cBegin
cEnd

sEnd    CodeSeg

        end     LibEntry
