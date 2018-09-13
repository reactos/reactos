//      TITLE("LPC Move Message Support")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    lpcmove.s
//
// Abstract:
//
//    This module implements functions to support the efficient movement of
//    LPC Message blocks
//
// Author:
//
//    David N. Cutler (davec) 11-Apr-1990
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

                SBTTL("Move Message")
//++
//
// VOID
// LpcpMoveMessage (
//    OUT PPORT_MESSAGE DstMsg
//    IN PPORT_MESSAGE SrcMsg
//    IN PUCHAR SrcMsgData
//    IN ULONG MsgType OPTIONAL,
//    IN PCLIENT_ID ClientId OPTIONAL
//    )
//
// Routine Description:
//
//    This function moves an LPC message block and optionally sets the message
//    type and client id to the specified values.
//
//    N.B. The messages are assumed to be quadword aligned.
//
// Arguments:
//
//    DstMsg (a0) - Supplies a pointer to the destination message.
//
//    SrcMsg (a1) - Supplies a pointer to the source message.
//
//    SrcMsgData (a2) - Supplies a pointer to the source message data to
//       copy to destination.
//
//    MsgType (a3) - If non-zero, then store in type field of the destination
//       message.
//
//    ClientId (4 * 4(sp)) - If non-NULL, then points to a ClientId to copy to
//       the destination message.
//
// Return Value:
//
//    None
//
//--

        LEAF_ENTRY(LpcpMoveMessage)

        lw      v0,4 * 4(sp)            // get address of client id
        lw      t0,0(a1)                // load first longword of source
        lw      t1,4(a1)                // load second longword of source
        addu    t2,t0,0x3               // round length to 4-byte multiple
        and     t2,t2,0xfffc            //
        beq     zero,a3,10f             // if eq, message type not specified
        srl     t1,t1,16                // clear low half of second longword
        sll     t1,t1,16                //
        or      t1,t1,a3                // set message type to specified value
10:     sw      t0,0(a0)                // store first longword of destination
        sw      t1,4(a0)                // store second longword of destination
        ld      t3,8(a1)                // get client id from source
        beq     zero,v0,20f             // if eq, client id not specified
        ldr     t3,0(v0)                // get specified client id
        ldl     t3,7(v0)                //
20:     ld      t4,16(a1)               // move message id and view size
        sd      t3,8(a0)                // store client id
        and     t3,t2,0xfff8            // isolate quadword move count
        sd      t4,16(a0)               //
        beq     zero,t3,40f             // if eq, no quadwords to move
        addu    t3,t3,a2                // compute ending address of move
30:     ldr     t0,0(a2)                // get next quadword of source data
        ldl     t0,7(a2)                //
        addu    a2,a2,8                 // advance source address
        addu    a0,a0,8                 // advance message pointers
        sd      t0,24 - 8(a0)           // store next longword of destination
        bne     t3,a2,30b               // if ne, more longwords to move
40:     and     t2,t2,0x4               // check if final longword to move
        beq     zero,t2,50f             // if eq, no data part of message
        lw      t0,0(a2)                // get next longword of source data
        sw      t0,24(a0)               // store next longword of destination
50:     j       ra                      // return

        .end    LpcpMoveMessage
