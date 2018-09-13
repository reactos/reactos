//      TITLE("LPC Move Message Support")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    lpcmove.s
//
// Abstract:
//
//    This module implements functions to support the efficient movement of
//    LPC Message blocks.
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
//    Thomas Van Baak (tvb) 19-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

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
//    ClientId (a4) - If non-NULL, then points to a ClientId to copy to
//       the destination message.
//
//    N.B. The messages are assumed to be quadword aligned.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(LpcpMoveMessage)

        ldq     t0, PmLength(a1)        // load first quadword of source

//
// The message length is in the low word of the first quadword.
//

        addq    t0, 3, t1               // round length to
        bic     t1, 3, t0               // nearest 4-byte multiple

//
// The message type is in the low half of the high longword. If a message
// type was specified, use it instead of the message type in the source
// message.
//

        sll     a3, 32, t2              // shift message type into position
        zap     t0, 0x30, t3            // clear message type field
        or      t3, t2, t1              // merge into new field
        cmovne  a3, t1, t0              // if a3!=0 use new message field
        stq     t0, PmLength(a0)        // store first quadword of destination

//
// The client id is the third and fourth items. If a client id was
// specified, use it instead of the client id in the source message.
//

        lda     t3, PmClientId(a1)      // get address of source client id
        cmovne  a4, a4, t3              // if a4!=0, use client id address in a4

//
//	Move the process and thread ids into place.  Note that for axp32, (t3) isn't
// necessarily quadword aligned.
// 

        LDP     t2, CidUniqueProcess(t3)// load low part of client id
        LDP     t1, CidUniqueThread(t3) // load high part of client id
        STP     t2, PmProcess(a0)       // store third longword of destination
        STP     t1, PmThread(a0)        // store fourth longword of destination
		  
#if defined(_AXP64_)

//
// Copy MessageId and ClientViewSize.  
//

        ldl     t2,PmMessageId(a1)
		  stl	    t2,PmMessageId(a0)
		  ldq     t2,PmClientViewSize(a1)
		  stq     t2,PmClientViewSize(a0)

#else

//
// PmClientViewSize is adjacent to PmMessageId, both can be moved at once
// with a single quadword move.
//

        ldq     t2,PmMessageId(a1)      // get next quadword of source
        stq     t2,PmMessageId(a0)      // set next quadword of destination

#endif

        and     t0, 0xfff8, t3          // isolate quadword move count
        beq     t3,20f                  // if eq, no quadwords to move
        and     a2, 7, t1               // check if source is quadword aligned
        bne     t1, UnalignedSource     // if ne, not quadword aligned

//
// Source and destination are both quadword aligned, use ldq/stq.  Use of
// the constant "PortMessageLength-8" is used in lieu of an additional
// instruction to increment a0 by that amount before entering the copy
// loop.
//

5:      ldq     t1, 0(a2)               // get source qword
        ADDP    a0, 8, a0               // advance destination address
        ADDP    a2, 8, a2               // advance source address
        subq    t3, 8, t3               // decrement number of bytes remaining
        stq     t1, PortMessageLength-8(a0) // store destination qword
        bne     t3, 5b                  // if ne, more quadwords to store
        br      zero, 20f               // move remaining longword

//
// We know that the destination is quadword aligned, but the source is
// not.  Use ldq_u to load the low and high parts of the source quadword,
// merge them with EXTQx and store them as one quadword.
//
// By reusing the result of the second ldq_u as the source for the
// next quadword's EXTQL we end up doing one ldq_u/stq for each quadword,
// regardless of the source's alignment.
//

UnalignedSource:                        //
        ldq_u   t1, 0(a2)               // prime t1 with low half of qword
10:     extql   t1, a2, t2              // t2 is aligned low part
        ADDP    a0, 8, a0               // advance destination address
        subq    t3, 8, t3               // reduce number of bytes remaining
        ldq_u   t1, 7(a2)               // t1 has high part
        extqh   t1, a2, t4              // t4 is aligned high part
        ADDP    a2, 8, a2               // advance source address
        bis     t2, t4, t5              // merge high and low parts
        stq     t5, PortMessageLength-8(a0) // store result
        bne     t3, 10b                 // if ne, more quadwords to move

//
// Move remaining longword (if any)
//

20:     and     t0, 4, t0               // check if longword to move
        beq     t0, 50f                 // if eq, no longword to move
        ldl     t1, 0(a2)               // move last longword to move
        stl     t1, PortMessageLength(a0) //
50:     ret     zero, (ra)              // return

        .end    LpcpMoveMessage
