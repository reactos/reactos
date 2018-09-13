/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995 Intel Corporation

Module Name:

    lpcmove.c

Abstract:

    This module implements functions to support the efficient movement
    of LPC message blocks.

    There is a corresponding .s version that is hand optimized.
    Need to evaluate and install one or the other.

Author:

    Roy D'Souza (rdsouza) 5-May-96

Revision History:

--*/

#include "lpcp.h"


VOID
LpcpMoveMessage (
    OUT PPORT_MESSAGE DstMsg,
    IN PPORT_MESSAGE SrcMsg,
    IN PUCHAR SrcMsgData,
    IN ULONG MsgType OPTIONAL,
    IN PCLIENT_ID ClientId OPTIONAL
    )

/*++

Routine Description:

    This function moves an LPC message block and optionally sets the message
    type and client id to the specified values.

Arguments:

    DstMsg     - Supplies a pointer to the destination message.

    SrcMsg     - Supplies a pointer to the source message.

    SrcMsgData - Supplies a pointer to the source message data to
                 copy to destination.

    MsgType    - If non-zero, then store in type field of the
                 destination message.

    ClientId   - If non-NULL, then points to a ClientId to copy to
                 the destination message.

Return Value:

    None

--*/

{
    ULONG NumberDwords;
    ULONGLONG Temp1, Temp2, Temp3;

    //
    // Extract the data length and copy over the first dword
    //

    *((PULONG)DstMsg)++ = NumberDwords = *((PULONG)SrcMsg)++;
    NumberDwords = ((0x0000FFFF & NumberDwords) + 3) >> 2;

    //
    // Set the message type properly and update the second dword
    //

    *((PULONG)DstMsg)++ = MsgType == 0 ? *((PULONG)SrcMsg)++ :
                         *((PULONG)SrcMsg)++ & 0xFFFF0000 | MsgType & 0xFFFF;

    //
    // Set the ClientId appropriately and update the third dword
    //

    *((PULONG_PTR)DstMsg)++ = ClientId == NULL ? *((PULONG_PTR)SrcMsg) :
            *((PULONG_PTR)ClientId)++;
    ((PULONG_PTR)SrcMsg)++;

    *((PULONG_PTR)DstMsg)++ = ClientId == NULL ? *((PULONG_PTR)SrcMsg) :
            *((PULONG_PTR)ClientId);
    ((PULONG_PTR)SrcMsg)++;

    //
    // Update the final two longwords in the header
    //

    *((PULONG_PTR)DstMsg)++ = *((PULONG_PTR)SrcMsg)++;
    *((PULONG_PTR)DstMsg)++ = *((PULONG_PTR)SrcMsg)++;

    //
    // Copy the data
    //

    if (NumberDwords > 0) {

        if ((ULONG_PTR) SrcMsgData & (sizeof(ULONGLONG)-1)) {

            //
            // SrcMsgData is 4-byte aligned while DstMsg is 8-byte aligned.
            //

            Temp1 = *((PULONG) SrcMsgData)++;

            while (NumberDwords >= 3) {

                Temp2 = *((PULONGLONG)SrcMsgData)++;
                *((PULONGLONG)DstMsg)++ = (Temp2 << 32) | Temp1;
                Temp1 = Temp2 & 0x0000FFFF;
                NumberDwords -= sizeof(ULONGLONG) / sizeof(ULONG);
            }

            *(PULONG)DstMsg = (ULONG)Temp1;
            NumberDwords--;

        } else {

            // 
            // Both DstMsg and SrcMsgData are 8-byte aligned;
            // copy 2 dwords (1 qword) at a time.
            // 

            while (NumberDwords >= sizeof(ULONGLONG) / sizeof(ULONG)) {

                *((PULONGLONG)DstMsg)++ = *((PULONGLONG)SrcMsgData)++;
                NumberDwords -= sizeof(ULONGLONG) / sizeof(ULONG);
            }
        }

        //
        // copy the remaining dwords, if any
        //
    
        while (NumberDwords--) {
            *((PULONG)DstMsg)++ = *((PULONG)SrcMsgData)++;
        }
    }
}


