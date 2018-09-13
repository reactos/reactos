        title  "Interlocked Support"
;++
;
; Copyright (c) 1996  Microsoft Corporation
;
; Module Name:
;
;    slist.asm
;
; Abstract:
;
;    This module implements functions to support interlocked S-List
;    operations.
;
; Author:
;
;    David N. Cutler (davec) 13-Mar-1996
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
        .list


_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page , 132
        subttl  "Interlocked Pop Entry Sequenced List"
;++
;
; PVOID
; FASTCALL
; RtlpInterlockedPopEntrySList (
;    IN PSLIST_HEADER ListHead
;    )
;
; Routine Description:
;
;    This function removes an entry from the front of a sequenced singly
;    linked list so that access to the list is synchronized in an MP system.
;    If there are no entries in the list, then a value of NULL is returned.
;    Otherwise, the address of the entry that is removed is returned as the
;    function value.
;
; Arguments:
;
;    (ecx) = ListHead - Supplies a pointer to the sequenced listhead from
;         which an entry is to be removed.
;
; Return Value:
;
;    The address of the entry removed from the list, or NULL if the list is
;    empty.
;
;--

cPublicFastCall RtlpInterlockedPopEntrySList, 1

cPublicFpo 0,2

;
; Save nonvolatile registers and read the listhead sequence number followed
; by the listhead next link.
;
; N.B. These two dwords MUST be read exactly in this order.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, ecx                ; save listhead address
        mov     edx,[ebp] + 4           ; get current sequence number
        mov     eax,[ebp] + 0           ; get current next link

;
; If the list is empty, then there is nothing that can be removed.
;

Epop10: or      eax, eax                ; check if list is empty
        jz      short Epop20            ; if z set, list is empty
        mov     ecx, edx                ; copy sequence number and depth
        add     ecx, 0FFFFH             ; adjust sequence number and depth

;
; N.B. It is possible for the following instruction to fault in the rare
;      case where the first entry in the list is allocated on another
;      processor and free between the time the free pointer is read above
;      and the following instruction. When this happens, the access fault
;      code continues execution by skipping the following instruction.
;      This results in the compare failing and the entire operation is
;      retried.
;

        mov     ebx, [eax]              ; get address of successor entry

.586

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

.386

        jnz     short Epop10            ; if z clear, exchange failed

;
; Restore nonvolatile registers and return result.
;

cPublicFpo 0,0

Epop20: pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET    RtlpInterlockedPopEntrySList

fstENDP RtlpInterlockedPopEntrySList

        page , 132
        subttl  "Interlocked Push Entry Sequenced List"
;++
;
; PVOID
; FASTCALL
; RtlpInterlockedPushEntrySList (
;    IN PSLIST_HEADER ListHead,
;    IN PVOID ListEntry
;    )
;
; Routine Description:
;
;    This function inserts an entry at the head of a sequenced singly linked
;    list so that access to the list is synchronized in an MP system.
;
; Arguments:
;
;    (ecx) ListHead - Supplies a pointer to the sequenced listhead into which
;          an entry is to be inserted.
;
;    (edx) ListEntry - Supplies a pointer to the entry to be inserted at the
;          head of the list.
;
; Return Value:
;
;    Previous contents of ListHead.  NULL implies list went from empty
;       to not empty.
;
;--

cPublicFastCall RtlpInterlockedPushEntrySList, 2

cPublicFpo 0,2

;
; Save nonvolatile registers and read the listhead sequence number followed
; by the listhead next link.
;
; N.B. These two dwords MUST be read exactly in this order.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, ecx                ; save listhead address
        mov     ebx, edx                ; save list entry address
        mov     edx,[ebp] + 4           ; get current sequence number
        mov     eax,[ebp] + 0           ; get current next link
Epsh10: mov     [ebx], eax              ; set next link in new first entry
        mov     ecx, edx                ; copy sequence number
        add     ecx, 010001H            ; increment sequence number and depth

.586

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

.386

        jnz     short Epsh10            ; if z clear, exchange failed

;
; Restore nonvolatile registers and return result.
;

cPublicFpo 0,0

        pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET  RtlpInterlockedPushEntrySList

fstENDP RtlpInterlockedPushEntrySList

_TEXT$00   ends
        end
