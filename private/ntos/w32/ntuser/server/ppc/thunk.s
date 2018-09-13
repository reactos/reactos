//       TITLE("Win32 Thunks")
//++
//
// Copyright (c) 1993  IBM Corporation
//
// Module Name:
//
//    thunk.s
//
// Abstract:
//
//    This module implements Win32 functions that must be written in
//    macro.
//
// Author:
//
//    Curt Fawcett (crf)  22-Sept-1993
//
// Revision History:
//
//    Curt Fawcett (crf)  19-Jan-1994           Removed Register names
//                                              as requested
//--
//
// Parameter Register Usage:
//
// r.3 - Current time low part
// r.4 - Current time high part
// r.5 - Boot time low part
// r.6 - Boot time high part
//
// r.4 - New stack address
// r.5 - Exit code
//
// Local Register Usage:
//
// r.7 - Result low part
// r.8 - Result high part
// r.9 - Temporary high part
// r.10 - Temporary low part
// r.11 - Temporary low part
// r.12 - Divide multiplier value
//

#include "ksppc.h"
//
// Define external entry points
//
        .globl ..FreeStackAndTerminate
//
//++
//
// VOID
// SwitchStackThenTerminate(
//     IN PVOID StackLimit,
//     IN PVOID NewStack,
//     IN DWORD ExitCode
//     )
//
//
// Routine Description:
//
//     This API is called during thread termination to delete a
//     thread's stack, switch to a stack in the thread's TEB, and then
//     terminate.
//
// Arguments:
//
//     StackLimit (r.3) - Supplies address of the stack to be freed.
//
//     NewStack (r.4) - Supplies an address within the terminating
//                      threads TEB that is to be used as its
//                      temporary stack while exiting.
//
//     ExitCode (r.5) - Supplies the termination status that the
//                      thread is to exit with.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(SwitchStackThenTerminate)

        mr      r.sp,r.4                // Set new stack address
        mr      r.4,r.5                 // Move exit code
        b       ..FreeStackAndTerminate // Jump to finish

        LEAF_EXIT(SwitchStackThenTerminate)


