        title  "NtGetTickCount"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    tickcnt.asm
;
; Abstract:
;
;
;    This module contains the implementation for the fast NtGetTickCount service
;
;
; Author:
;
;    Mark Lucovsky (markl) 19-Oct-1996
;
; Environment:
;
;    Kernel mode.
;
; Revision History:
;
;
;--
.386p
;        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
;        .list

extrn   _KeTickCount:DWORD
extrn   _ExpTickCountMultiplier:DWORD

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132

;++
;
; Routine Description:
;
;     This function returns number of milliseconds since the system
;     booted.  This function is designed to support the Win32 GetTicKCount
;     API.
;
; Arguments:
;
;     NONE
;
; Return Value:
;
;     Returns the number of milliseconds that have transpired since boot
;
;--

cPublicProc _NtGetTickCount, 0
cPublicFpo 0, 0

        mov     eax,dword ptr [_KeTickCount]
        mul     dword ptr [_ExpTickCountMultiplier]
        shrd    eax,edx,24                  ; compute resultant tick count

        stdRET  _NtGetTickCount
stdENDP _NtGetTickCount

_TEXT   ends
        end
