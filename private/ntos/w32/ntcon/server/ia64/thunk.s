//++
//
// Module Name:
//
//    thunk.s
//
// Abstract:
//
//   This module implements all Win32 thunks. This includes the
///   first level thread starter...
//
// Author:
//
//   12-Oct-1995
//
// Revision History:
//
//--
#include "ksia64.h"
    .file    "thunk.s"

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
//     This API is called during thread termination to delete a thread's
//     stack, switch to a stack in the thread's TEB, and then terminate.
//
// Arguments:
//
//     StackLimit (a0) - Supplies the address of the stack to be freed.
//
//     NewStack (a1) - Supplies an address within the terminating threads TEB
//         that is to be used as its temporary stack while exiting.
//         This is also used as the new RseStackBase
//
//     ExitCode (a2) - Supplies the termination status that the thread
//         is to exit with.
//
// Return Value:
//
//     None.
//
//--
        .global FreeStackAndTerminate
        .type   FreeStackAndTerminate, @function

RseSIZE   = 320         // reserve 320 bytes RseStack, RseStack should
                        //  be larger than memory stack
// Input arguments
StackBase = a0
NewStack  = a1
ExitCode  = a2

NewStackBase = t1

OldStackBase = t2
OldExitCode = t0
SavedRSC = t3
temp = t4

        LEAF_ENTRY(SwitchStackThenTerminate)

        mov     SavedRSC = ar.rsc
        mov     OldStackBase = StackBase      // save the arguments to pass down
        mov     t0 = 15                       // align base to 16 byte boundry
        ;;

        alloc   t22 = ar.pfs, 0, 0, 0, 0      // throw Stacked registers away
        add     NewStackBase = RseSIZE, NewStack
        mov     OldExitCode = ExitCode
        ;;

        andcm   NewStackBase = NewStackBase, t0
        ;;
        dep     temp = 0, SavedRSC, RSC_MODE, 2
        ;;
        
        mov     ar.rsc = temp
        mov     brp = zero                    // set brp to zero to indicate
        mov     ar.pfs = zero
        ;;

        loadrs
        ;;
        mov     ar.bspstore = NewStackBase
        mov     sp = NewStackBase             // setup new stack
        ;;

        alloc   t22 = ar.pfs, 0, 0, 2, 0      // allocate 2 output registers
        mov     ar.rsc = SavedRSC
        mov     out0 = OldStackBase           // re arrange args
        mov     out1 = OldExitCode
        br      FreeStackAndTerminate         // end of call chain.

        LEAF_EXIT(SwitchStackThenTerminate)
