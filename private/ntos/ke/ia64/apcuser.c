/*++

Module Name:

    apcuser.c

Abstract:

    This module implements the machine dependent code necessary to initialize
    a user mode APC.

Author:

    William K. Cheung  26-Oct-1995

    based on MIPS version by David N. Cutler (davec) 23-Apr-1990

Environment:

    Kernel mode only, IRQL APC_LEVEL.

Revision History:

--*/

#include "ki.h"
#include "kxia64.h"

VOID
KiInitializeUserApc (
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is called to initialize the context for a user mode APC.

Arguments:

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

    NormalRoutine - Supplies a pointer to the user mode APC routine.

    NormalContext - Supplies a pointer to the user context for the APC
        routine.

    SystemArgument1 - Supplies the first system supplied value.

    SystemArgument2 - Supplies the second system supplied value.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord;
    EXCEPTION_RECORD ExceptionRecord;
    LONG Length;
    ULONGLONG UserStack;
    ULONGLONG OriginalBSP;
    PULONGLONG Arguments;

    //
    // Move the user mode state from the trap and exception frames to the
    // context frame.
    //

    ContextRecord.ContextFlags = CONTEXT_FULL;
    OriginalBSP = TrapFrame->RsBSP;

    if (TRAP_FRAME_TYPE(TrapFrame) == SYSCALL_FRAME) {

        //
        // Make the necessary adjustment to the BSP value in the trap frame
        // if the user APC is dispatched in the context of a system call
        //

        SHORT RNatSaveIndex, Temp;
        SHORT OutputFrameSize;

        OutputFrameSize = (SHORT)((TrapFrame->StIFS & PFS_SIZE_MASK) - ((TrapFrame->StIFS >> PFS_SIZE_SHIFT) & PFS_SIZE_MASK));
        RNatSaveIndex = (SHORT)((TrapFrame->RsBSP>>3) & NAT_BITS_PER_RNAT_REG);

        Temp = RNatSaveIndex + OutputFrameSize - NAT_BITS_PER_RNAT_REG;
        while (Temp >= 0) {
            OutputFrameSize++;
            Temp -= NAT_BITS_PER_RNAT_REG;
        }
        TrapFrame->RsBSP += OutputFrameSize * sizeof(ULONGLONG);

    }

    KeContextFromKframes(TrapFrame, ExceptionFrame, &ContextRecord);
    TrapFrame->RsBSP = OriginalBSP;

    //
    // Transfer the context information to the user stack, initialize the
    // APC routine parameters, and modify the trap frame so execution will
    // continue in user mode at the user mode APC dispatch routine.
    //
    // We build the following structure on the user stack:
    //
    //             |                               |  
    //             |-------------------------------|
    //             |                               |
    //             |   Interrupted user's          |
    //             |   stack frame                 |
    //             |                               |
    //             |                               |
    //             |-------------------------------|
    //             |   Slack Space due to the      |
    //             |   16-byte stack alignment     |
    //             | - - - - - - - - - - - - - - - |
    //             |     NormalRoutine             |
    //             |     SystemArgument2           |
    //             |     SystemArgument1           |
    //             |     NormalContext             |
    //             | - - - - - - - - - - - - - - - |
    //             |   Context Frame               |
    //             |      Filled in with state     |
    //             |      of interrupted user      |
    //             |      program                  |
    //             | - - - - - - - - - - - - - - - |
    //             |   Stack Scratch Area          |
    //             |-------------------------------|
    //             |                               |

    try {

    USHORT LocalFrameSize;
    PPLABEL_DESCRIPTOR Plabel = (PPLABEL_DESCRIPTOR) KeUserApcDispatcher;

    //
    // Compute total length of 4 arguments, context record, and
    // stack scratch area. 
    //
    // Compute the new 16-byte aligned user stack pointer.
    //

    Length = (4 * sizeof(ULONGLONG) + CONTEXT_LENGTH +
              STACK_SCRATCH_AREA + 15) & (~15);
    UserStack = (ContextRecord.IntSp & (~15)) - Length;
    Arguments = (PULONGLONG)(UserStack + STACK_SCRATCH_AREA + CONTEXT_LENGTH);

    //
    // Probe user stack area for writeability and then transfer the
    // context record to the user stack.
    //

    ProbeForWrite((PCHAR)UserStack, Length, sizeof(QUAD));
    RtlCopyMemory((PVOID)(UserStack+STACK_SCRATCH_AREA), 
                  &ContextRecord, sizeof(CONTEXT));

    //
    // Set the address of the user APC routine, the APC parameters, the
    // interrupt frame set, the new global pointer, and the new stack 
    // pointer in the current trap frame.  The four APC parameters are 
    // passed via the scratch registers t0 thru t3. 
    // Set the continuation address so control will be transfered to 
    // the user APC dispatcher.
    //

    *Arguments++ = (ULONGLONG)NormalContext;     // 1st argument
    *Arguments++ = (ULONGLONG)SystemArgument1;   // 2nd argument
    *Arguments++ = (ULONGLONG)SystemArgument2;   // 3rd argument
    *Arguments++ = (ULONGLONG)NormalRoutine;     // 4th argument
    *(PULONGLONG)UserStack = Plabel->GlobalPointer;  // user apc dispatcher gp

    TrapFrame->IntNats = 0;                      // sanitize integer Nats
    TrapFrame->IntSp = UserStack;                // stack pointer

    TrapFrame->StIIP = Plabel->EntryPoint;       // entry point from plabel
    TrapFrame->StIPSR &= ~(0x3ULL << PSR_RI);    // start at bundle boundary
    TrapFrame->StIFS &= 0xffffffc000000000;      // set the initial frame
                                                 // size of KeUserApcDispatcher
                                                 // to be zero.

    //
    // If an exception occurs, then copy the exception information to an
    // exception record and handle the exception.
    //

    } except (KiCopyInformation(&ExceptionRecord,
                                (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Set the address of the exception to the current program address
        // and raise the exception by calling the exception dispatcher.
        //

        ExceptionRecord.ExceptionAddress = (PVOID)(TrapFrame->StIIP);
        KiDispatchException(&ExceptionRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }

    return;
}
