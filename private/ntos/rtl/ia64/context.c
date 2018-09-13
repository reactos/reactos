/*++

Module Name:

    context.c

Abstract:

    This module implements user-mode callable context manipulation routines.
    The interfaces exported from this module are portable, but they must
    be re-implemented for each architecture.

Author:


Revision History:

    Ported to the IA64

    27-Feb-1996   Revised to pass arguments to target thread by injecting
                  arguments into the backing store.

--*/

#include "ntrtlp.h"
#include "kxia64.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlInitializeContext)
#pragma alloc_text(PAGE,RtlRemoteCall)
#pragma alloc_text(PAGE,RtlSetIaToEmDebugReg)
#pragma alloc_text(PAGE,RtlIaToEmDebugContext)
#pragma alloc_text(PAGE,RtlEmToIaDebugContext)
#endif


VOID
RtlInitializeContext(
    IN HANDLE Process,
    OUT PCONTEXT Context,
    IN PVOID Parameter OPTIONAL,
    IN PVOID InitialPc OPTIONAL,
    IN PVOID InitialSp OPTIONAL
    )

/*++

Routine Description:

    This function initializes a context structure so that it can
    be used in a subsequent call to NtCreateThread.

Arguments:

    Context - Supplies a context buffer to be initialized by this routine.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

Return Value:

    Raises STATUS_BAD_INITIAL_STACK if the value of InitialSp is not properly
           aligned.

    Raises STATUS_BAD_INITIAL_PC if the value of InitialPc is not properly
           aligned.

--*/

{
    ULONGLONG Argument;

    RTL_PAGED_CODE();

    //
    // Check for proper initial stack (0 mod 16).
    //

    if (((ULONG_PTR)InitialSp & 0xf) != 0) {
        RtlRaiseStatus(STATUS_BAD_INITIAL_STACK);
    }

    //
    // Check for proper plabel address alignment.
    // Assumes InitialPc points to a plabel that must be 8-byte aligned.
    //

    if (((ULONG_PTR)InitialPc & 0x7) != 0) {
        RtlRaiseStatus(STATUS_BAD_INITIAL_PC);
    }

    //
    // Initialize the integer and floating registers to contain zeroes.
    //

    RtlZeroMemory(Context, sizeof(CONTEXT));

    //
    // Setup integer and control context.
    //

    Context->ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;

    Context->RsBSPSTORE = Context->IntSp = (ULONG_PTR)InitialSp;
    Context->IntSp -= STACK_SCRATCH_AREA;

    //
    // InitialPc is the module entry point which is a function pointer
    // in IA64. StIIP and IntGp are initiailized with actual IP and GP
    // from the plabel in LdrInitializeThunk after the loader runs.
    //

    Context->IntS0 = Context->StIIP = (ULONG_PTR)InitialPc;
    Context->IntGp = 0;

    //
    // Setup FPSR, PSR, and DCR
    // N.B. Values to be determined.
    //

    Context->StFPSR = USER_FPSR_INITIAL;
    Context->StIPSR = USER_PSR_INITIAL;
    Context->ApDCR = USER_DCR_INITIAL;

    //
    // Set the initial context of the thread in a machine specific way.
    // ie, pass the initial parameter to the RSE by saving it at the
    // bottom of the backing store.
    //
    //  Setup Frame Marker after RFI
    //  And other RSE states.
    //

    Argument = (ULONGLONG)Parameter;
    ZwWriteVirtualMemory(Process,
             (PVOID)((ULONG_PTR)Context->RsBSPSTORE),
             (PVOID)&Argument,
             sizeof(Argument),
             NULL);
//
// N.b. The IFS must be reinitialized in LdrInitializeThunk
//

    Context->StIFS = 0x8000000000000081ULL;            // Valid, 1 local register, 0 output register
    Context->RsBSP = Context->RsBSPSTORE;
    Context->RsRSC = USER_RSC_INITIAL;
    Context->ApUNAT = 0xFFFFFFFFFFFFFFFF;
}


NTSTATUS
RtlRemoteCall(
    HANDLE Process,
    HANDLE Thread,
    PVOID CallSite,
    ULONG ArgumentCount,
    PULONG_PTR Arguments,
    BOOLEAN PassContext,
    BOOLEAN AlreadySuspended
    )

/*++

Routine Description:

    This function calls a procedure in another thread/process, using
    NtGetContext and NtSetContext.  Parameters are passed to the
    target procedure via its stack.

Arguments:

    Process - Handle of the target process

    Thread - Handle of the target thread within that process

    CallSite - Address of the procedure to call in the target process.

    ArgumentCount - Number of parameters to pass to the target
                    procedure.

    Arguments - Pointer to the array of parameters to pass.

    PassContext - TRUE if an additional parameter is to be passed that
        points to a context record.

    AlreadySuspended - TRUE if the target thread is already in a suspended
                       or waiting state.

Return Value:

    Status - Status value

--*/

{
    NTSTATUS Status;
    CONTEXT Context;
    ULONG_PTR ContextAddress;
    ULONG_PTR NewSp;
    ULONG_PTR NewBspStore;
    ULONGLONG ArgumentsCopy[10];
    PVOID ptr;
    ULONG Count = 0;
    ULONG ShiftCount;
    BOOLEAN RnatSaved = FALSE;


    RTL_PAGED_CODE();

    if (ArgumentCount > 8)
        return STATUS_INVALID_PARAMETER;

    //
    // If necessary, suspend the guy before with we mess with his stack.
    //

    if (AlreadySuspended == FALSE) {
        Status = NtSuspendThread(Thread, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            return(Status);
        }
    }

    //
    // Get the context record of the target thread.
    //

    Context.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(Thread, &Context);
    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }
        return(Status);
    }

    if (AlreadySuspended) {
        Context.IntV0 = STATUS_ALERTED;
    }

    //
    // Pass the parameters to the other thread via the backing store (r32-r39).
    // The context record is passed on the stack of the target thread.
    // N.B. Align the context record address, stack pointer, and allocate
    //      stack scratch area.
    //

    ContextAddress = (((ULONG_PTR)Context.IntSp + 0xf) & 0xf) - sizeof(CONTEXT);
    NewSp = ContextAddress - STACK_SCRATCH_AREA;
    Status = NtWriteVirtualMemory(Process, (PVOID)ContextAddress, &Context,
                  sizeof(CONTEXT), NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }
        return(Status);
    }

    RtlZeroMemory((PVOID)ArgumentsCopy, sizeof(ArgumentsCopy));
    NewBspStore = (ULONG_PTR) Context.RsBSPSTORE;

    if (PassContext) {
        if ( (NewBspStore & 0x1F8) == 0x1F8 ) {
            ArgumentsCopy[Count++] = Context.RsRNAT;
            NewBspStore += sizeof(ULONGLONG);
            RnatSaved = TRUE;
        }
        ShiftCount = (ULONG) (NewBspStore & 0x1F8) >> 3;
        Context.RsRNAT &= ~(0x1 << ShiftCount);
        ArgumentsCopy[Count++] = ContextAddress;
        NewBspStore += sizeof(ULONGLONG);
    }

    for (; ArgumentCount != 0 ; ArgumentCount--) {
        if ( (RnatSaved == FALSE) && ((NewBspStore & 0x1F8) == 0x1F8) ) {
            ArgumentsCopy[Count++] = Context.RsRNAT;
            NewBspStore += sizeof(ULONGLONG);
            RnatSaved = TRUE;
        }
        ShiftCount = (ULONG)(NewBspStore & 0x1F8) >> 3;
        Context.RsRNAT &= ~(0x1 << ShiftCount);
        ArgumentsCopy[Count++] = (ULONGLONG)(*Arguments++);
        NewBspStore += sizeof(ULONGLONG);
    }

    if ( (RnatSaved == FALSE) && ((NewBspStore & 0x1F8) == 0x1F8) ) {
        ArgumentsCopy[Count++] = Context.RsRNAT;
        NewBspStore += sizeof(ULONGLONG);
    }

    //
    //  Copy the arguments onto the target backing store.
    //

    if (Count) {
        Status = NtWriteVirtualMemory(Process,
                                      (PVOID)Context.RsBSPSTORE,
                                      ArgumentsCopy,
                                      Count * sizeof(ULONGLONG),
                                      NULL
                                      );

        if (NT_SUCCESS(Status) == FALSE) {
            if (AlreadySuspended == FALSE) {
                NtResumeThread(Thread, NULL);
            }
            return(Status);
        }
    }

    //
    // zero out loadrs, adjust RseStack
    //

    Context.RsRSC = (RSC_MODE_LY<<RSC_MODE)
                   | (RSC_BE_LITTLE<<RSC_BE)
                   | (0x3<<RSC_PL);

    //
    //  Setup argument numbers
    //

    Context.StIFS = (ULONGLONG)Count;       // # of arguments/size of frame
    Context.RsBSP = Context.RsBSPSTORE;

    //
    // Set the address of the target code into IIP, the new target stack
    // into sp, setup ap, and reload context to make it happen.
    //

    Context.IntSp = (ULONG_PTR)NewSp;

    //
    // We expect NtSetContext to get the real IP and GP for us
    //

    Context.StIIP = (ULONG_PTR)CallSite;

    //
    // sanitize the floating pointer status register
    //

    SANITIZE_FSR(Context.StFPSR, UserMode);

    Status = NtSetContextThread(Thread, &Context);
    if (!AlreadySuspended) {
        NtResumeThread(Thread, NULL);
    }

    return( Status );
}


static VOID
RtlSetIaToEmDebugReg(
    IN ULONG          Dr7_RW,
    IN ULONG          Dr7_Len,
    IN ULONG          Addr,
    IN OUT ULONGLONG *DbrAddr1,
    IN OUT ULONGLONG *DbrAddr2,
    IN OUT ULONGLONG *IbrAddr1,
    IN OUT ULONGLONG *IbrAddr2
    )

/*++

Routine Description:

    This function constructs the EM mode Debug Register Context
    given iA mode debug Context as input.

Arguments:

    Dr7_RW     - Supplies iA mode context buffer including iA mode debug registers.

    Dr7_Len    - Supplies the value of the iA mode CR4.

    Addr       - Supplies a context buffer to be initialized by this routine.

    DbrAddr1   - Address of first associated data        breakpoint register in set.

    IbrAddr1   - Address of first associated instruction breakpoint register in set.

Return Value:

    None.

    N.B. Currently EM Debug Registers are only stored on a thread switch. Activating the
         EM debug context generated by this routine requires the EM debug registers to
         be loaded in the context of the target thread.

--*/

{
    ULONGLONG  mask;

    // If length is undefined value, set it to zero.
    if (Dr7_Len == 2)
        Dr7_Len = 0;

    mask = (ULONGLONG) DBG_REG_MASK(Dr7_Len);

    switch(Dr7_RW) {
        case DR7_RW_IX:
            *DbrAddr1 = 0;
            *IbrAddr1 = (ULONGLONG)Addr;
            *DbrAddr2 = 0;
            *IbrAddr2 = (IBR_EX | DBG_REG_PLM_USER | DBG_REG_MASK(0));
            break;
        case DR7_RW_DW:
            *DbrAddr1 = (ULONGLONG)Addr;
            *IbrAddr1 = 0;
            *DbrAddr2 = (DBR_WR | DBG_REG_PLM_USER | mask);
            *IbrAddr2 = 0;
            break;
        case DR7_RW_IORW:
            *DbrAddr1 = (ULONGLONG)Addr;
            *IbrAddr1 = 0;
            *DbrAddr2 = (DBR_RDWR | DBG_REG_PLM_USER | mask);
            *IbrAddr2 = 0;
            break;
        case DR7_RW_DWR:
            *DbrAddr1 = (ULONGLONG)Addr;
            *IbrAddr1 = 0;
            *DbrAddr2 = (DBR_RDWR | DBG_REG_PLM_USER | mask);
            *IbrAddr2 = 0;
            break;
        case DR7_RW_DISABLE:
        default:
            *DbrAddr1 = 0;
            *IbrAddr1 = 0;
            *DbrAddr2 = 0;
            *IbrAddr2 = 0;
    }

    return;
}


BOOLEAN
RtlIaToEmDebugContext(
    IN PCONTEXT86   ContextX86,
    IN OUT PCONTEXT ContextEM
    )

/*++

Routine Description:

    This function constructs the EM mode Debug Register Context given iA mode
    debug Context as input. The EM debug control bits in the PSR are set based
    on the iA mode context.

Arguments:

    ContextX86 - Supplies iA mode context buffer including iA mode debug registers.

    ContextEM  - Supplies a EM mode context buffer to be initialized by this routine.

Return Value:

    FALSE   - No context returned, based on input debugging is not active.
    TRUE    - EM Debug Context returned.

    N.B. Currently EM Debug Registers are only stored on a thread switch. Activating the
         EM debug context generated by this routine requires the EM debug registers to
         be loaded in the context of the target thread.

--*/

{
    ULONG Dr7 = ContextX86->Dr7;

    RtlSetIaToEmDebugReg(DR7_DB0_RW(Dr7), 
                         DR7_DB0_LEN(Dr7), 
                         DR_ADDR_L0(ContextX86->Dr0),
                         &ContextEM->DbD0, 
                         &ContextEM->DbD1, 
                         &ContextEM->DbI0, 
                         &ContextEM->DbI1);

    RtlSetIaToEmDebugReg(DR7_DB1_RW(Dr7), 
                         DR7_DB1_LEN(Dr7), 
                         DR_ADDR_L1(ContextX86->Dr1),
                         &ContextEM->DbD2, 
                         &ContextEM->DbD3, 
                         &ContextEM->DbI2, 
                         &ContextEM->DbI3);

    RtlSetIaToEmDebugReg(DR7_DB2_RW(Dr7), 
                         DR7_DB2_LEN(Dr7), 
                         DR_ADDR_L2(ContextX86->Dr2),
                         &ContextEM->DbD4, 
                         &ContextEM->DbD5, 
                         &ContextEM->DbI4, 
                         &ContextEM->DbI5);

    RtlSetIaToEmDebugReg(DR7_DB3_RW(Dr7), 
                         DR7_DB3_LEN(Dr7), 
                         DR_ADDR_L3(ContextX86->Dr3),
                         &ContextEM->DbD6, 
                         &ContextEM->DbD7, 
                         &ContextEM->DbI6, 
                         &ContextEM->DbI7);

    //
    // Check to see if Debugging is enabled or disabled.
    //
    if (DR7_L0(Dr7) || DR7_L1(Dr7) || DR7_L2(Dr7) || DR7_L3(Dr7))
    {
        ContextEM->StIPSR &= ~(PSR_DEBUG_SET);
        ContextEM->StIPSR |=  (1 << PSR_DB);
    } else {
        ContextEM->StIPSR &= ~(PSR_DEBUG_SET);
        return(FALSE);
    }

    return(TRUE);
}


BOOLEAN
RtlEmToIaDebugContext(
    IN PCONTEXT       ContextEM,
    IN OUT PCONTEXT86 ContextX86
    )

/*++

Routine Description:

    This function constructs the iA mode Debug Register Context
    given EM mode debug Context as input.

Arguments:

    ContextEM  - Supplies EM mode context buffer including EM mode debug registers.

    ContextX86 - Supplies a iA mode context buffer to be initialized by this routine.

Return Value:


    FALSE   - No context returned, based on input debugging is not active.
    TRUE    - EM Debug Context returned.

    N.B. Assumes that the thread's virtual iA mode Debug Registers reflects
         the current debug context. The kernel must keep the field DebugIaDr6 and
         DebugIaDr7 up to date.

    TBD How do we set DR6? From Status, what status? Probably should be set by exception
        handler.

--*/

{
    //
    // Check to see if Debugging is enabled or disabled.
    //

    // Translate Dr0 from either DbD0-1 or DbI0-1
    SET_DR7_L0(ContextX86->Dr7);

    SET_DR7_DB0_RW(ContextX86->Dr7, DBG_EM_ENABLE_TO_IA_RW(ContextEM->DbD1, ContextEM->DbI1));
    SET_DR7_DB0_LEN(ContextX86->Dr7, DBG_EM_MASK_TO_IA_LEN(ContextEM->DbD1, ContextEM->DbI1));
    ContextX86->Dr0 = DBG_EM_ADDR_TO_IA_ADDR(ContextEM->DbD0, ContextEM->DbI0);

    // Translate Dr1 from either DbD2-3 or DbI2-3
    SET_DR7_L1(ContextX86->Dr7);
    SET_DR7_DB1_RW(ContextX86->Dr7, DBG_EM_ENABLE_TO_IA_RW(ContextEM->DbD3, ContextEM->DbI3));
    SET_DR7_DB1_LEN(ContextX86->Dr7, DBG_EM_MASK_TO_IA_LEN(ContextEM->DbD3, ContextEM->DbI3));
    ContextX86->Dr1 = DBG_EM_ADDR_TO_IA_ADDR(ContextEM->DbD2, ContextEM->DbI2);

    // Translate Dr2 from either DbD4-5 or DbI4-5
    SET_DR7_L2(ContextX86->Dr7);
    SET_DR7_DB2_RW(ContextX86->Dr7, DBG_EM_ENABLE_TO_IA_RW(ContextEM->DbD5, ContextEM->DbI5));
    SET_DR7_DB2_LEN(ContextX86->Dr7, DBG_EM_MASK_TO_IA_LEN(ContextEM->DbD5, ContextEM->DbI5));
    ContextX86->Dr2 = DBG_EM_ADDR_TO_IA_ADDR(ContextEM->DbD4, ContextEM->DbI4);

    // Translate Dr3 from either DbD6-7 or DbI6-7
    SET_DR7_L3(ContextX86->Dr7);
    SET_DR7_DB3_RW(ContextX86->Dr7, DBG_EM_ENABLE_TO_IA_RW(ContextEM->DbD7, ContextEM->DbI7));
    SET_DR7_DB3_LEN(ContextX86->Dr7, DBG_EM_MASK_TO_IA_LEN(ContextEM->DbD7, ContextEM->DbI7));
    ContextX86->Dr3 = DBG_EM_ADDR_TO_IA_ADDR(ContextEM->DbD6, ContextEM->DbI6);

    if (!(DR7_L0(ContextX86->Dr7) || DR7_L1(ContextX86->Dr7) 
            || DR7_L2(ContextX86->Dr7) || DR7_L3(ContextX86->Dr7)))
        return(FALSE);
    else
        return(TRUE);
}


VOID
RtlEmToX86Context(
    IN PCONTEXT ContextEm,
    IN OUT PCONTEXT86 ContextX86
    )
{
    ContextX86->ContextFlags = CONTEXT_FULL | CONTEXT_X86;

    //
    // TBD: How to transfer the remaining floating point states from EM
    //      context record to the floating save area in iA context record.
    //

    RtlEmFpToIaFpContext(&ContextEm->FltT2, &ContextX86->FloatSave);
    RtlEmToIaDebugContext(ContextEm, ContextX86);

    ContextX86->SegGs = (ULONG)ContextEm->IntT20;
    ContextX86->SegFs = (ULONG)ContextEm->IntT19;
    ContextX86->SegEs = (ULONG)ContextEm->IntT15;
    ContextX86->SegDs = (ULONG)ContextEm->IntT18;
    ContextX86->Edi = (ULONG)ContextEm->IntT6;
    ContextX86->Esi = (ULONG)ContextEm->IntT5;
    ContextX86->Ebx = (ULONG)ContextEm->IntT4;
    ContextX86->Edx = (ULONG)ContextEm->IntT3;
    ContextX86->Ecx = (ULONG)ContextEm->IntT2;
    ContextX86->Eax = (ULONG)ContextEm->IntV0;
    ContextX86->Ebp = (ULONG)ContextEm->IntTeb;
    ContextX86->Eip = (ULONG)ContextEm->StIIP;
    ContextX86->SegCs = (ULONG)ContextEm->SegCSD;      // ContextEm->IntT16;
    ContextX86->EFlags = (ULONG)ContextEm->Eflag;
    ContextX86->Esp = (ULONG)ContextEm->IntSp;
    ContextX86->SegSs = (ULONG)ContextEm->SegSSD;      // ContextEm->IntT17;
}


VOID
RtlX86ToEmContext(
    IN PCONTEXT86 ContextX86,
    IN OUT PCONTEXT ContextEm
    )
{
    FLOAT128 FpWorkArea;

    ContextEm->ContextFlags = CONTEXT_FULL | CONTEXT_IA64;

    RtlIaFpToEmFpContext((PVOID)&ContextX86->FloatSave,
                         &ContextEm->FltT2,
                         &FpWorkArea);
    RtlIaToEmDebugContext(ContextX86, ContextEm);

    ContextEm->IntT20 = ContextX86->SegGs;
    ContextEm->IntT19 = ContextX86->SegFs;
    ContextEm->IntT15 = ContextX86->SegEs;
    ContextEm->IntT18 = ContextX86->SegDs;
    ContextEm->IntT6 = ContextX86->Edi;
    ContextEm->IntT5 = ContextX86->Esi;
    ContextEm->IntT4 = ContextX86->Ebx;
    ContextEm->IntT3 = ContextX86->Edx;
    ContextEm->IntT2 = ContextX86->Ecx;
    ContextEm->IntV0 = ContextX86->Eax;
    ContextEm->IntTeb = ContextX86->Ebp;
    ContextEm->StIIP = ContextX86->Eip;
    ContextEm->SegCSD = ContextX86->SegCs;
    ContextEm->Eflag = ContextX86->EFlags;
    ContextEm->IntSp = ContextX86->Esp;
    ContextEm->SegSSD = ContextX86->SegSs;
}
