        title  "Event Pair Support"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    evpair.asm
;
; Abstract:
;
;
;    This module contains the implementation for the fast event pair
;    system services that are used for client/server synchronization.
;    sethiwaitlo, setlowaithi.
;
;
; Author:
;
;    Mark Lucovsky (markl) 03-Feb-1992
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

EXTRNP  _KiSetServerWaitClientEvent,3
extrn   _KeTickCount:DWORD
extrn   _ExpTickCountMultiplier:DWORD

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "NtSetLowWaitHighThread"

;++
; Routine Description:
;
;     This function uses the prereferenced client/server event pair pointer
;     and sets the low event of the event pair and waits on the high event
;     of the event pair object.
;
;     N.B. This service assumes that it has been called from user mode.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     TBS
;
;--

cPublicProc _NtSetLowWaitHighThread, 0
cPublicFpo 0, 0

        mov     eax,fs:PcPrcbData+PbCurrentThread
        mov     eax,dword ptr [eax+EtEventPair]
        or      eax,eax
        jz      $BAIL
        mov     ecx,eax                     ; compute address of events
        add     eax,EpEventLow              ; server event
        add     ecx,EpEventHigh             ; client event
        stdCall _KiSetServerWaitClientEvent, <eax, ecx, 1>
        xor     eax,eax
        stdRET  _NtSetLowWaitHighThread
$BAIL:
        mov     eax,STATUS_NO_EVENT_PAIR
        stdRET  _NtSetLowWaitHighThread
stdENDP _NtSetLowWaitHighThread

        page ,132
        subttl  "NtSetHighWaitLowThread"

;++
; Routine Description:
;
;     This function uses the prereferenced client/server event pair pointer
;     and sets the low event of the event pair and waits on the high event
;     of the event pair object.
;
;     N.B. This service assumes that it has been called from user mode.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     TBS
;
;--
;PUBLIC _NtSetHighWaitLowThread, 0
cPublicProc _NtSetHighWaitLowThread, 0
cPublicFpo 0, 0

        mov     eax,fs:PcPrcbData+PbCurrentThread
        mov     eax,dword ptr [eax+EtEventPair]
        or      eax,eax
        jz      $BAIL1
        mov     ecx,eax                     ; compute address of events
        add     eax,EpEventHigh             ; server event
        add     ecx,EpEventLow              ; client event
        stdCall _KiSetServerWaitClientEvent, <eax, ecx, 1>
        xor     eax,eax
        stdRET  _NtSetHighWaitLowThread
$BAIL1:
        mov eax,STATUS_NO_EVENT_PAIR
        stdRET  _NtSetHighWaitLowThread
stdENDP _NtSetHighWaitLowThread

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
