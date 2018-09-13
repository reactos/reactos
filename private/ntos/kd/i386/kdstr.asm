;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    kdstr.asm
;
; Abstract:
;
; Author:
;
; Environment:
;
; Revision History:
;
;--

.386p
include mac386.inc
include callconv.inc                    ; calling convention macros

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; PUCHAR
;   KdpCopyDataToStack(
;      IN PUCHAR Data,
;      IN ULONG  Length
;   )
;
; Routine Description:
;
;    This routine moves the specified amount of data bytes onto the callers
;    stack.  The callers ESP is adjust to account for the storage.
;
; Arguments:
;
;    (esp)          - return address
;
;    Data   (esp+4) - supplies point to the string to copy
;
;    Length (esp+8) - Length of data to copy
;
;    MaximumLength (esp+
;
; Return Value:
;
;    Pointer to copied data
;
;--
cPublicProc _KdpCopyDataToStack ,2
        pop     eax             ; (eax) = return address
        pop     edx             ; (edx) = data pointer
        pop     ecx             ; (ecx) = length

        sub     esp, ecx        ; make space on callers stack
        sub     esp, 4
        and     esp, not 3      ; round stack to a  DWORD boundry

        push    ecx             ; restore parameters onto new stack location
        push    edx
        push    eax

        push    edi
        push    esi             ; save C registers

        mov     esi, edx        ; (esi) = source
        mov     eax, esp
        add     eax, 5*4        ; adjust for local stack
        mov     edi, eax        ; (edi) = dest stack address
        mov     edx, ecx        ; (edx) = number of bytes
        shr     ecx, 2          ; # of dwords
        rep     movsd           ; move dwords
        mov     ecx, edx
        and     ecx, 3          ; # of remaining bytes
        rep     movsb           ; move bytes

        pop     esi
        pop     edi             ; restore C registers
        stdRET    _KdpCopyDataToStack                     ; (eax) = dest stack address

stdENDP _KdpCopyDataToStack

_TEXT   ends
        end
