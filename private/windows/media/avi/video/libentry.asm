;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   DLLENTRY.ASM
;
;   simulates the NT DllEntryPoint call for a Win16 DLL
;
;   Copyright (c) Microsoft Corporation 1989, 1990. All rights reserved.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        PMODE = 1
        ?PLM=1  ; pascal call convention
        ?WIN=0  ; NO! Windows prolog/epilog code
        .286

        .xlist
        include cmacros.inc
        .list

        externFP DllEntryPoint
        externFP LocalInit

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

;-----------------------------------------------------------------------;
;
; Stuff needed to avoid the C runtime coming in, and init the windows
; reserved parameter block at the base of DGROUP
;
; NOTE if you need the 'C' startup dont use this file.
;
;-----------------------------------------------------------------------;

if 1;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sBegin  Data
        assumes ds,Data

        DD  0           ; So null pointers get 0
        DW  5           ; number of reserved ptrs
globalW pLocalHeap,0    ; Local heap pointer
globalW pAtomTable,0    ; Atom table pointer
globalW pStackTop,0     ; top of stack
globalW pStackMin,0     ; minimum value of SP
globalW pStackBot,0     ; bottom of stack

public  __acrtused
	__acrtused = 1

sEnd        Data

endif;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sBegin  CodeSeg
	assumes cs,CodeSeg
        assumes ds,Data
        assumes es,nothing

;--------------------------Private-Routine-----------------------------;
;
; LibEntry - called when DLL is loaded
;
; Entry:
;       CX    = size of heap
;       DI    = module handle
;       DS    = automatic data segment
;       ES:SI = address of command line (not used)
; Returns:
;       AX = TRUE if success
; History:
;       06-27-89 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
	assumes ds,nothing
	assumes es,nothing

cProc   LibEntry,<FAR,PUBLIC,NODATA>,<>
cBegin
        jcxz    @f
        cCall   LocalInit,<0,0,cx>
@@:
ifdef DAYTONA
	cCall   DllEntryPoint, <1, di, ds, cx, 0, 0, 0>
endif
cEnd

;--------------------------Private-Routine-----------------------------;
;
; WEP - called when DLL is unloaded
;
; History:
;       06-27-89 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;
	assumes ds,nothing
	assumes es,nothing

cProc   WEP, <FAR, PUBLIC, PASCAL>, <ds>
        ParmW  fSystemExit
cBegin
        ;
        ;   HEY dont cleanup if windows is going down.
        ;
        mov     ax,fSystemExit
        or      ax,ax
;;;     jnz     just_exit

        mov     ax,DataBASE
        mov     ds,ax
        assumes ds,Data

ifdef DAYTONA
	cCall   DllEntryPoint, <0, di, ds, cx, 0, 0, 0>
endif
just_exit:
cEnd

sEnd CodeSeg

        end LibEntry
