        title  "LPC Move Message Support"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    lpcmove.asm
;
; Abstract:
;
;    This module implements functions to support the efficient movement of
;    LPC Message blocks
;
; Author:
;
;    Steven R. Wood (stevewo) 30-Jun-1989
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;   6-Mar-90 bryanwi
;       Ported to the 386.
;
;--

.386p

include callconv.inc            ; calling convention macros

        page ,132
        subttl  "Update System Time"

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
	ASSUME	DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; LpcpMoveMessage (
;    OUT PPORT_MESSAGE DstMsg
;    IN PPORT_MESSAGE SrcMsg
;    IN ULONG MsgType OPTIONAL,
;    IN PCLIENT_ID ClientId OPTIONAL
;    )
;
; Routine Description:
;
;    This function moves an LPC message block.
;
; Arguments:
;
;    DstMsg (TOS) - Supplies pointer to where to move the message block to.
;
;    SrcMsg (TOS+4) - Supplies a pointer to the message to move.
;
;    MsgType (TOS+8) - If non-zero, then store in Type field of DstMsg
;
;    ClientId (TOS+12) - If non-NULL, then points to a ClientId to copy to dst
;
; Return Value:
;
;    None
;
;--

DstMsg      equ [esp + 12]
SrcMsg      equ [esp + 16]
SrcMsgData  equ [esp + 20]
MsgType     equ [esp + 24]
ClientId    equ [esp + 28]

cPublicProc _LpcpMoveMessage    ,5

        push    esi                     ; Save non-volatile registers
        push    edi

        mov     edi,DstMsg              ; (edi)->Destination
        cld
        mov     esi,SrcMsg              ; (esi)->Source

        lodsd                           ; (eax)=length
        stosd
        lea     ecx,3[eax]
        and     ecx,0FFFCH              ; (ecx)=length rounded up to 4
        shr     ecx,2                   ; (ecx)=length in dwords

        lodsd                           ; (eax)=DataInfoOffset U Type
        mov     edx,MsgType             ; (edx)=MsgType
        or      edx,edx
        jz      lmm10                   ; No MsgType, go do straight copy
        mov     ax,dx                   ; (eax low 16)=MsgType
lmm10:  stosd

        mov     edx,ClientId            ; (edx)=ClientId
        or      edx,edx
        jz      lmm20                   ; No Clientid to set, go do copy
        mov     eax,[edx]               ; Get new ClientId
        stosd                           ; and store in DstMsg->ClientId
        mov     eax,[edx+4]
        stosd
        add     esi,8                   ; and skip over SrcMsg->ClientId
        jmp     short lmm30

lmm20:  movsd                           ; Copy ClientId
        movsd

;
;   At this point, all of the control structures are copied, all we
;   need to copy is the data.
;

lmm30:
        movsd                           ; Copy MessageId
        movsd                           ; Copy ClientViewSize

        mov     esi,SrcMsgData          ; Copy data directly from user buffer
        rep     movsd                   ; Copy the data portion of message

        pop     edi
        pop     esi                     ; Restore non-volatile registers

        stdRET    _LpcpMoveMessage

stdENDP _LpcpMoveMessage

_TEXT$00   ends
        end

