/*++

Module Name:

    context.c

Abstract:

    This module contains the context management routines for
    Win32

Author:


Revision History:

--*/

#include "basedll.h"

#include "kxia64.h"

VOID
BaseInitializeContext(
    OUT PCONTEXT Context,
    IN PVOID Parameter OPTIONAL,
    IN PVOID InitialPc OPTIONAL,
    IN PVOID InitialSp OPTIONAL,
    IN BASE_CONTEXT_TYPE ContextType
    )

/*++

Routine Description:

    This function initializes a context structure so that it can
    be used in a subsequent call to NtCreateThread.

Arguments:

    Context - Supplies a context buffer to be initialized by this routine.

    Parameter - Supplies the thread's parameter.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

    NewThread - Supplies a flag that specifies that this is a new
        thread, or a new process.

Return Value:

    Raises STATUS_BAD_INITIAL_STACK if the value of InitialSp is not properly
           aligned.

    Raises STATUS_BAD_INITIAL_PC if the value of InitialPc is not properly
           aligned.

--*/

{
    ULONG ArgumentsCount;
    //
    // Initialize the Context 
    //
    RtlZeroMemory((PVOID)Context, sizeof(CONTEXT));

    Context->StIPSR = USER_PSR_INITIAL;
    Context->RsPFS = 0;                             // no PFS
    Context->RsBSPSTORE = Context->IntSp = (ULONG_PTR)InitialSp;
    Context->IntSp -= STACK_SCRATCH_AREA; // scratch area as per convention
    Context->IntS1 = (ULONG_PTR)InitialPc;
    Context->IntS2 = (ULONG_PTR)Parameter;

    //
    // Enable RSE engine
    //

    Context->RsRSC = (RSC_MODE_EA<<RSC_MODE)
                   | (RSC_BE_LITTLE<<RSC_BE)
                   | (0x3<<RSC_PL);

    if ( ContextType == BaseContextTypeThread ) {
        Context->IntS0 = Context->StIIP = (ULONG_PTR)BaseThreadStartThunk;
        }
    else if ( ContextType == BaseContextTypeFiber ) {
        Context->IntS0 = Context->StIIP = (ULONG_PTR)BaseFiberStart;
        //
        // set up the return pointer here..
        // when SwitchToFiber restores context and calls return, 
        // the contorl goes to this routine
        //
        Context->BrRp = *((ULONGLONG *)((PUCHAR)BaseFiberStart));

        //
        // set up sof = 96 in pfs. This will be used to set up CFM for above
        // routine
        //  
        Context->RsPFS |= 0x60;

        }
    else {
        Context->IntS0 = Context->StIIP = (ULONG_PTR)(LONG_PTR)BaseProcessStartThunk;
        }
    //
    // Note that we purposely set IntGp = 0ULL, to indicate special protocol 
    //
    Context->IntGp = 0ULL;

    Context->ContextFlags = CONTEXT_CONTROL| CONTEXT_INTEGER;

    // set nat bits for every thing except ap, gp, sp, also  T0 and T1
    Context->ApUNAT = 0xFFFFFFFFFFFFEDF1ULL;
    Context->Eflag = 0x00003002ULL;
}

VOID
BaseFiberStart(
    VOID
    )

/*++

Routine Description:

    This function is called to start a Win32 fiber. Its purpose
    is to call BaseThreadStart, getting the necessary arguments
    from the fiber context record.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PFIBER Fiber;

    Fiber = GetCurrentFiber();
    BaseThreadStart( (LPTHREAD_START_ROUTINE)Fiber->FiberContext.IntS1,
                     (LPVOID)Fiber->FiberContext.IntS2 );
}


VOID
BaseProcessStartupIA64(
   IN PTHREAD_START_ROUTINE StartRoutine,
   IN PVOID ThreadParameter
   )

/*++

Routine Description:

   This function calls to the portable thread starter after moving
   its arguments from registers to the argument registers.

Arguments:

   StartRoutine - User Target start program counter
   ThreadParameter 

Return Value:

   Never Returns

--*/
{
   (StartRoutine)(ThreadParameter);
}
