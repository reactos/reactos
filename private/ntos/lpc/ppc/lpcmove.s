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
//    Chuck Bauman (cbauman@vnet.ibm.com) 20-Mar-1993
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
// July 5th, 1994    plj@vnet.ibm.com    slight performance optimization
//                                       and updated C code to be correct
//                                       again (I think).
//
// N.B. Structures of type PPORT_MESSAGE are assumed to be 8 byte aligned.
//
//--

#include "ksppc.h"

//              SBTTL("Move Message")
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
//    DstMsg (r.3) - Supplies a pointer to the destination message.
//
//    SrcMsg (r.4) - Supplies a pointer to the source message.
//
//    SrcMsgData (r.5) - Supplies a pointer to the source message data to
//                       copy to destination.
//
//    MsgType (r.6) - If non-zero, then store in type field of the
//                    destination message.
//
//    ClientId (r.7) - If non-NULL, then points to a ClientId to copy to
//       the destination message.
//
// Return Value:
//
//    None
//
//--
// The following is a possible C implementation of this routine, shown for
// (hopefully) clarity.
//--
// VOID
// LpcpMoveMessage (
//    OUT PULONG DstMsg                   PPORT_MESSAGE
//    IN PULONG SrcMsg                    PPORT_MESSAGE
//    IN PULONG SrcMsgData                PUCHAR
//    IN ULONG MsgType OPTIONAL,
//    IN PULONG ClientId OPTIONAL         PCLIENT_ID
//    )
// {
//      ULONG NumberWords;
//
//      *DstMsg++ = NumberWords = *SrcMsg++;
//      NumberWords = (NumberWords + 3) >> 2;
//      if (MsgType != 0) {
//         *DstMsg++ = (*SrcMsg++ & 0xffff0000) | (MsgType & 0xffff);
//      } else {
//         *DstMsg++ = *SrcMsg++;
//      }
//      if (ClientId == NULL) {
//         *DstMsg++ = *SrcMsg++;
//         *DstMsg++ = *SrcMsg++;
//      } else {
//         *DstMsg++ = *ClientId++;
//         *DstMsg++ = *ClientId;
//         SrcMsg += 2;
//      }
//      *DstMsg++ = *SrcMsg++;
//      *DstMsg++ = *SrcMsg++;
//      while (NumberWords--) {
//         *DstMsg++ = *SrcMsgData++;
//      }
// }

        LEAF_ENTRY(LpcpMoveMessage)
        lwz     r.10,0(r.4)    //  load first longword of source
        cmpwi   cr.1,r.6,0     //  message type specified?
        cmpwi   cr.6,r.7,0     //  client id specified?
        subi    r.5,r.5,4      //  Prepare for lwzu in copymsg loop
        lwz     r.8,4(r.4)     //  load second longword of source
        addi    r.9,r.10,3     //  round length to 4-byte multiple
        rlwinm. r.11,r.9,29,0x1fff // get number of quadwords
        lfd     f.0,16(r.4)    //  get message id and view size
        beq     cr.1,nomsgtype //  if eq, message type not specified
        rlwimi  r.8,r.6, 0, 0xffff // insert specified message type

nomsgtype:
        stw     r.10,0(r.3)    //  store first longword of destination
        stw     r.8,4(r.3)     //  store second longword of destination
        beq     cr.6,noclientid//  if eq, client id not specified
        lwz     r.10,0(r.7)    //  load first longword of client id
        lwz     r.8,4(r.7)     //  load second longword if client id
        b       storeclient    //

noclientid:
        lwz     r.10,8(r.4)    //  get third longword of source
        lwz     r.8,12(r.4)    //  get fourth longword of source

storeclient:
        mtcrf   0x01,r.9       //  set cr bit 29 if odd number longwords
        stw     r.10,8(r.3)    //  store client id (first half)
        stw     r.8,12(r.3)    //                  (second half)
        stfdu   f.0,16(r.3)    //  store message id and view size

//
//  At this point, the next byte to be stored in the destination (if any)
//  goes at (r.3) + 8.
//
        beq     cr.0,noquad    //  if 0 length, no quadwords
        mtctr   r.11           //  set count register with loop count

copymsg:
        lwz     r.10,4(r.5)    //  get longword of source
        lwzu    r.11,8(r.5)    //  get longword of source
        stwu    r.10,8(r.3)    //  store next longword of destination
        stw     r.11,4(r.3)    //  store next longword of destination
        bdnz    copymsg        //  if nz, more longwords to move

noquad:
        bflr    29             //  return if no odd longword to move

        lwz     r.10,4(r.5)    //  get last longword
        stw     r.10,8(r.3)    //  store last longword

        LEAF_EXIT(LpcpMoveMessage)      //  return

