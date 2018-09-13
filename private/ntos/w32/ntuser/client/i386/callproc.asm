        title   "Callback Routines"
;++
;
;  Copyright (c) 1985 - 1999, Microsoft Corporation
;
;  Module Name:
;
;     callproc.asm
;
;  Abstract:
;
;     This module implements stack cleanup to gaurd against cdecl
;     declared wndprocs.
;     Bug 234292
;
;  Author:
;
;     Joseph Jones (joejo) 12/4/98
;
;
;  Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
        .list

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page , 132


;++
;
;LRESULT
;UserCallWinProc(
;    WNDPROC winproc,
;    HWND hwnd,
;    UINT message,
;    WPARAM wParam,
;    LPARAM lParam
;    )
;
; Routine Description:
;
;    this function cals an x86 window procedure. It protects against
;    window procedures that don't preserve EBX or fail to clean up
;    enough stack.
;
;    The only register that the window proc cannot trash is ESI.
;
; Arguments:
;
;    winproc - x86 Procedure to call
;
;    hwnd - window handle that sent the message
;
;    message - message being sent
;
;    wParam - wParam argument to window procedure
;
;    lParam - lParam argument to window proc
;
; Return Value:
;
;    LRESULT return value from called window procedure
;
;--

cPublicProc _UserCallWinProc , 5

winproc         equ [ebp + 8]
hwnd            equ [ebp + 12]
message         equ [ebp + 16]
wParam          equ [ebp + 20]
lParam          equ [ebp + 24]

StackGuard      equ 0DCBAABCDh

stdENDP _UserCallWinProc
        
        push    ebp
        mov     ebp, esp
        
        push    StackGuard  ; push guard on the stack
        push    esi         ; push another DWORD on the stack
                            ; so that bogus apps that treat &lParam
                            ; as an LPPOINT don't corrupt the StackGuard

        push    lParam
        push    wParam
        push    message
        push    hwnd
        call    winproc
        
        cmp     DWORD PTR [esp+4], StackGuard
        je      goodCalling
        
;--
;       Bug 386625: fix for Corel Presentation 9.0 that restores the stack
;                   for 5 parameters instead of 4
;--
        
        cmp     DWORD PTR [esp], StackGuard
        jne     fixupTheStack
        sub     esp, 04h
        jmp     goodCalling

fixupTheStack:
        add     esp, 010h   ; fix up the stack

goodCalling:        
        add     esp, 08h    ; pop the extra DWORD
                            ; pop the second StackGuard
        
        pop     ebp
        stdRET  _UserCallWinProc

    

;/*
; * Bug 246472 - joejo
; * fixup all DDE Callbacks since some apps make their callbacks
; * C-Style instead of PASCAL.
; */

;++
;
;HDDEDATA 
;UserCallDDECallback(
;    UINT wType, 
;    UINT wFmt, 
;    HCONV hConv,
;    HSZ hsz1, 
;    HSZ hsz2, 
;    HDDEDATA hData, 
;    ULONG_PTR dwData1, 
;    ULONG_PTR dwData2
;    )
;
; Routine Description:
;
;    this function cals an x86 DDE Callback procedure. It protects against
;    callback procedures that don't preserve EBX or fail to clean up
;    enough stack.
;
;    The only register that the window proc cannot trash is ESI.
;
; Arguments:
;
;       pfnDDECallback - DDE Callback function pointer
;       wType
;       wFmt
;       hConv
;       hsz1
;       hsz2
;       hData
;       dwData1
;       dwData2
;
; Return Value:
;
;    HDDEDATA   - Handle to a returnded DDE Data object
;
;--

cPublicProc _UserCallDDECallback , 9

pfnDDECallback  equ [ebp + 8]
wType           equ [ebp + 12]
wFmt            equ [ebp + 16]
hConv           equ [ebp + 20]
hsz1            equ [ebp + 24]
hsz2            equ [ebp + 28]
hData           equ [ebp + 32]
dwData1         equ [ebp + 36]
dwData2         equ [ebp + 40]


stdENDP _UserCallDDECallback
        push    ebp
        mov     ebp, esp
        push    esi         ; save esi across the call
        push    edi         ; save edi across the call
        push    ebx         ; save ebx on the stack across the call
        mov     esi,esp     ; save the stack pointer in esi across the call
        push    dwData2
        push    dwData1
        push    hData
        push    hsz2
        push    hsz1
        push    hConv
        push    wFmt
        push    wType
        call    pfnDDECallback
        mov     esp,esi     ; restore the stack pointer in case callee forgot to clean up
        pop     ebx         ; restore ebx
        pop     edi         ; restore edi
        pop     esi         ; restore esi
        pop     ebp
        stdRET  _UserCallDDECallback



_TEXT   ends

       
        end
