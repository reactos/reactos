/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    abiosc.c

Abstract:

    This module implements ROM BIOS support C routines for i386 NT.

Author:

    Shie-Lin Tzong (shielint) 10-Sept-1992

Environment:

    Kernel mode.


Revision History:

--*/
#include "ki.h"
#pragma hdrstop
#include "vdmntos.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,Ke386CallBios)
#endif


//
// Never change these equates without checking biosa.asm
//

#define V86_CODE_ADDRESS    0x10000
#define INT_OPCODE          0xcd
#define V86_BOP_OPCODE      0xfec4c4
#define V86_STACK_POINTER   0x1ffe
#define IOPM_OFFSET         FIELD_OFFSET(KTSS, IoMaps[0].IoMap)
#define VDM_TIB_ADDRESS     0x12000
#define INT_10_TEB          0x13000

//
// External References
//

PVOID Ki386IopmSaveArea;
BOOLEAN BiosInitialized = FALSE;
VOID
Ki386SetupAndExitToV86Code (
   PVOID ExecutionAddress
   );


NTSTATUS
Ke386CallBios (
    IN ULONG BiosCommand,
    IN OUT PCONTEXT BiosArguments
    )

/*++

Routine Description:

    This function invokes specified ROM BIOS code by executing
    "INT BiosCommand."  Before executing the BIOS code, this function
    will setup VDM context, change stack pointer ...etc.  If for some reason
    the operation fails, a status code will be returned.  Otherwise, this
    function always returns success reguardless of the result of the BIOS
    call.

    N.B. This implementation relies on the fact that the direct
         I/O access operations between apps are serialized by win user.

Arguments:

    BiosCommand - specifies which ROM BIOS function to invoke.

    BiosArguments - specifies a pointer to the context which will be used
                  to invoke ROM BIOS.

Return Value:

    NTSTATUS code to specify the failure.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    PVDM_TIB VdmTib;
    PUCHAR BaseAddress = (PUCHAR)V86_CODE_ADDRESS;
    PTEB UserInt10Teb = (PTEB)INT_10_TEB;
    PKTSS Tss;
    PKPROCESS Process;
    PKTHREAD Thread;
    USHORT OldIopmOffset, OldIoMapBase;
    PVDM_PROCESS_OBJECTS VdmObjects;
    ULONG   ContextLength;

//  KIRQL OldIrql;
//#if DBG
//    PULONG IdtAddress;
//    ULONG RegionSize;
//    ULONG OldProtect;
//#endif

    //
    // Map in ROM BIOS area to perform the int 10 code
    //

    if (!BiosInitialized) {
        RtlZeroMemory(UserInt10Teb, sizeof(TEB));
    }

//#if DBG
//    IdtAddress = 0;
//    RegionSize = 0x1000;
//    ZwProtectVirtualMemory ( NtCurrentProcess(),
//                             &IdtAddress,
//                             &RegionSize,
//                             PAGE_READWRITE,
//                             &OldProtect
//                             );
//#endif

    try {

        //
        // Write "Int BiosCommand; bop" to reserved user space (0x1000).
        // Later control will transfer to the user space to execute
        // these two instructions.
        //

        *BaseAddress++ = INT_OPCODE;
        *BaseAddress++ = (UCHAR)BiosCommand;
        *(PULONG)BaseAddress = V86_BOP_OPCODE;

        //
        // Set up Vdm(v86) context to execute the int BiosCommand
        // instruction by copying user supplied context to VdmContext
        // and updating the control registers to predefined values.
        //

        //
        // We want to use a constant number for the int10.
        //
        //
        // Create a fake TEB so we can switch the thread to it while we
        // do an int10
        //

        UserInt10Teb->Vdm = (PVOID)VDM_TIB_ADDRESS;
        VdmTib = (PVDM_TIB)VDM_TIB_ADDRESS;
        RtlZeroMemory(VdmTib, sizeof(VDM_TIB));
        VdmTib->Size = sizeof(VDM_TIB);
        *pNtVDMState = 0;

        //
        // extended registers are never going to matter to
        //  an Int10 call, so only copy the old part of the
        //  context record.
        //
        ContextLength = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        RtlMoveMemory(&(VdmTib->VdmContext), BiosArguments,  ContextLength);
        VdmTib->VdmContext.SegCs = (ULONG)BaseAddress >> 4;
        VdmTib->VdmContext.SegSs = (ULONG)BaseAddress >> 4;
        VdmTib->VdmContext.Eip = 0;
        VdmTib->VdmContext.Esp = 2 * PAGE_SIZE - sizeof(ULONG);
        VdmTib->VdmContext.EFlags |= EFLAGS_V86_MASK | EFLAGS_INTERRUPT_MASK;
        VdmTib->VdmContext.ContextFlags = CONTEXT_FULL;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }

    //
    // The vdm kernel code finds the Tib by looking at a pointer cached in
    // kernel memory, which was probed at Vdm creation time.  Since the
    // creation semantics for this vdm are peculiar, we do something similar
    // here.
    //

    try {

        //
        // We never get here on a process that is a real vdm.  If we do,
        // bad things will happen  (pool leak, failure to execute dos and windows apps)
        //
        ASSERT(PsGetCurrentProcess()->VdmObjects == NULL);
        VdmObjects = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(VDM_PROCESS_OBJECTS),
            '  eK'
            );


        //
        // Since we are doing this on behalf of CSR not a user process, we aren't
        // charging quota.
        //
        if (VdmObjects == NULL) {
            Status = STATUS_NO_MEMORY;
        } else {

            //
            // We are only initializing the VdmTib pointer, because that's the only
            // part of the VdmObjects we use for ROM calls.  We aren't set up
            // to simulate interrupts, or any of the other stuff that would be done
            // in a conventional vdm
            //
            RtlZeroMemory( VdmObjects, sizeof(VDM_PROCESS_OBJECTS));

            VdmObjects->VdmTib = VdmTib;

            PsGetCurrentProcess()->VdmObjects = VdmObjects;
        }
    }  except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }

    if (Status == STATUS_SUCCESS) {

        //
        // Since we are going to v86 mode and accessing some I/O ports, we
        // need to make sure the IopmOffset is set correctly across context
        // swap and the I/O bit map has all the bits cleared.
        // N.B.  This implementation assumes that there is only one full
        //       screen DOS app and the io access between full screen DOS
        //       app and the server code is serialized by win user.  That
        //       means even we change the IOPM, the full screen dos app won't
        //       be able to run on this IOPM.
        //     * In another words, IF THERE IS
        //     * MORE THAN ONE FULL SCREEN DOS APPS, THIS CODE IS BROKEN.*
        //
        // NOTE This code works on the assumption that winuser serializes
        //      direct I/O access operations.
        //

        //
        // Call the bios from the processor which booted the machine.
        //

        Thread = KeGetCurrentThread();
        KeSetSystemAffinityThread(1);
        Tss = KeGetPcr()->TSS;

        //
        // Save away the original IOPM bit map and clear all the IOPM bits
        // to allow v86 int 10 code to access ALL the io ports.
        //

        //
        // Make sure there are at least 2 IOPM maps.
        //

        ASSERT(KeGetPcr()->GDT[KGDT_TSS / 8].LimitLow >= (0x2000 + IOPM_OFFSET - 1));
        RtlMoveMemory (Ki386IopmSaveArea,
                       (PVOID)&Tss->IoMaps[0].IoMap,
                       PAGE_SIZE * 2
                       );
        RtlZeroMemory ((PVOID)&Tss->IoMaps[0].IoMap, PAGE_SIZE * 2);

        Process = Thread->ApcState.Process;
        OldIopmOffset = Process->IopmOffset;
        OldIoMapBase = Tss->IoMapBase;
        Process->IopmOffset = (USHORT)(IOPM_OFFSET);      // Set Process IoPmOffset before
        Tss->IoMapBase = (USHORT)(IOPM_OFFSET);           //     updating Tss IoMapBase

        //
        // Call ASM routine to switch stack to exit to v86 mode to
        // run Int BiosCommand.
        //

        Ki386SetupAndExitToV86Code(UserInt10Teb);

        //
        // After we return from v86 mode, the control comes here.
        //
        // Restore old IOPM
        //

        RtlMoveMemory ((PVOID)&Tss->IoMaps[0].IoMap,
                       Ki386IopmSaveArea,
                       PAGE_SIZE * 2
                       );

        Process->IopmOffset = OldIopmOffset;
        Tss->IoMapBase = OldIoMapBase;

        //
        // Restore old affinity for current thread.
        //

        KeRevertToUserAffinityThread();

        //
        // Copy 16 bit vdm context back to caller.
        //
        // Extended register state is not going to matter,
        // so copy only the old part of the context record.
        //
        ContextLength = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        RtlMoveMemory(BiosArguments, &(VdmTib->VdmContext), ContextLength);
        BiosArguments->ContextFlags = CONTEXT_FULL;

        //
        // Free the pool used for the VdmTib pointer
        //
        ExFreePool(PsGetCurrentProcess()->VdmObjects);
        PsGetCurrentProcess()->VdmObjects = NULL;

    }

//#if DBG
//    IdtAddress = 0;
//    RegionSize = 0x1000;
//    ZwProtectVirtualMemory ( NtCurrentProcess(),
//                             &IdtAddress,
//                             &RegionSize,
//                             PAGE_NOACCESS,
//                             &OldProtect
//                             );
//#endif

    return(Status);
}
