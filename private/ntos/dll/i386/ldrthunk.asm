        title   "LdrInitializeThunk"
;++
;
;  Copyright (c) 1989  Microsoft Corporation
;
;  Module Name:
;
;     ldrthunk.s
;
;  Abstract:
;
;     This module implements the thunk for the LdrpInitialize APC routine.
;
;  Author:
;
;     Steven R. Wood (stevewo) 27-Apr-1990
;
;  Environment:
;
;     Any mode.
;
;  Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
        .list

        EXTRNP  _LdrpInitialize,3

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page , 132

;++
;
; VOID
; LdrInitializeThunk(
;    IN PVOID NormalContext,
;    IN PVOID SystemArgument1,
;    IN PVOID SystemArgument2
;    )
;
; Routine Description:
;
;    This function computes a pointer to the context record on the stack
;    and jumps to the LdrpInitialize function with that pointer as its
;    parameter.
;
; Arguments:
;
;    NormalContext - User Mode APC context parameter (ignored).
;
;    SystemArgument1 - User Mode APC system argument 1 (ignored).
;
;    SystemArgument2 - User Mode APC system argument 2 (ignored).
;
; Return Value:
;
;    None.
;
;--

cPublicProc _LdrInitializeThunk , 4

NormalContext   equ [esp + 4]
SystemArgument1 equ [esp + 8]
SystemArgument2 equ [esp + 12]
Context         equ [esp + 16]

        lea     eax,Context             ; Calculate address of context record
        mov     NormalContext,eax       ; Pass as first parameter to
if DEVL
        xor     ebp,ebp                 ; Mark end of frame pointer list
endif
IFDEF STD_CALL
        jmp     _LdrpInitialize@12      ; LdrpInitialize
ELSE
        jmp     _LdrpInitialize         ; LdrpInitialize
ENDIF

stdENDP _LdrInitializeThunk

;++
;
; VOID
; LdrpCallInitRoutine(
;    IN PDLL_INIT_ROUTINE InitRoutine,
;    IN PVOID DllHandle,
;    IN ULONG Reason,
;    IN PCONTEXT Context OPTIONAL
;    )
;
; Routine Description:
;
;    This function calls an x86 DLL init routine.  It is robust
;    against DLLs that don't preserve EBX or fail to clean up
;    enough stack.
;
;    The only register that the DLL init routine cannot trash is ESI.
;
; Arguments:
;
;    InitRoutine - Address of init routine to call
;
;    DllHandle - Handle of DLL to call
;
;    Reason - one of the DLL_PROCESS_... or DLL_THREAD... values
;
;    Context - context pointer or NULL
;
; Return Value:
;
;    FALSE if the init routine fails, TRUE for success.
;
;--

cPublicProc _LdrpCallInitRoutine , 4

InitRoutine     equ [ebp + 8]
DllHandle       equ [ebp + 12]
Reason          equ [ebp + 16]
Context         equ [ebp + 20]

stdENDP _LdrpCallInitRoutine
        push    ebp
        mov     ebp, esp
        push    esi         ; save esi across the call
        push    edi         ; save edi across the call
        push    ebx         ; save ebx on the stack across the call
        mov     esi,esp     ; save the stack pointer in esi across the call
        push    Context
        push    Reason
        push    DllHandle
        call    InitRoutine
        mov     esp,esi     ; restore the stack pointer in case callee forgot to clean up
        pop     ebx         ; restore ebx
        pop     edi         ; restore edi
        pop     esi         ; restore esi
        pop     ebp
        stdRET  _LdrpCallInitRoutine

_TEXT   ends
        end
