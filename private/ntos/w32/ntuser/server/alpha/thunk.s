//       TITLE("Win32 Thunks")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    thunk.s
//
// Abstract:
//
//    This module implements Win32 functions that must be written in assembler.
//
// Author:
//
//    Mark Lucovsky (markl) 5-Oct-1990
//
// Revision History:
//
//    Jim Anderson (jima) 31-Oct-1994
//
//        Copied from base for worker thread cleanup
//
//--

#include "ksalpha.h"

        SBTTL("Switch Stack Then Terminate")
//++
//
// VOID
// SwitchStackThenTerminate (
//     IN PVOID StackLimit,
//     IN PVOID NewStack,
//     IN DWORD ExitCode
//     )
//
// Routine Description:
//
//     This API is called during thread termination to delete a thread's
//     stack, switch to a stack in the thread's TEB, and then terminate.
//
// Arguments:
//
//     StackLimit (a0) - Supplies the address of the stack to be freed.
//
//     NewStack (a1) - Supplies an address within the terminating thread's TE
//         that is to be used as its temporary stack while exiting.
//
//     ExitCode (a2) - Supplies the termination status that the thread
//         is to exit with.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(SwitchStackThenTerminate)

//
// Switch stacks and then jump to FreeStackAndTerminate.
//

        mov     a1, sp                  // set new stack pointer
        mov     a2, a1                  // set exit code argument
        br      zero, FreeStackAndTerminate // jump

        .end SwitchStackThenTerminate
