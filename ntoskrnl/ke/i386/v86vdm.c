/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/v86vdm.c
 * PURPOSE:         Manages the Kernel's support for Virtual-8086 Mode (V86)
 *                  used by Video Drivers to access ROM BIOS functions, as well
 *                  as the kernel architecture part of generic VDM support.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG KeI386EFlagsAndMaskV86 = EFLAGS_USER_SANITIZE;
ULONG KeI386EFlagsOrMaskV86 = EFLAGS_INTERRUPT_MASK;
PVOID Ki386IopmSaveArea;
BOOLEAN KeI386VirtualIntExtensions = FALSE;

/* PRIVATE FUNCTIONS *********************************************************/

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
Ke386CallBios(IN ULONG Int,
              OUT PCONTEXT Context)
{
    PUCHAR Trampoline = (PUCHAR)TRAMPOLINE_BASE;
    PTEB VdmTeb = (PTEB)TRAMPOLINE_TEB;
    PVDM_TIB VdmTib = (PVDM_TIB)TRAMPOLINE_TIB;
    ULONG ContextSize = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
    PKTHREAD Thread = KeGetCurrentThread();
    PKTSS Tss = KeGetPcr()->TSS;
    PKPROCESS Process = Thread->ApcState.Process;
    PVDM_PROCESS_OBJECTS VdmProcessObjects;
    USHORT OldOffset, OldBase;

    /* Start with a clean TEB */
    RtlZeroMemory(VdmTeb, sizeof(TEB));

    /* Write the interrupt and bop */
    *Trampoline++ = 0xCD;
    *Trampoline++ = (UCHAR)Int;
    *(PULONG)Trampoline = TRAMPOLINE_BOP;

    /* Setup the VDM TEB and TIB */
    VdmTeb->Vdm = (PVOID)TRAMPOLINE_TIB;
    RtlZeroMemory(VdmTib, sizeof(VDM_TIB));
    VdmTib->Size = sizeof(VDM_TIB);

    /* Set a blank VDM state */
    *VdmState = 0;

    /* Copy the context */
    RtlCopyMemory(&VdmTib->VdmContext, Context, ContextSize);
    VdmTib->VdmContext.SegCs = (ULONG_PTR)Trampoline >> 4;
    VdmTib->VdmContext.SegSs = (ULONG_PTR)Trampoline >> 4;
    VdmTib->VdmContext.Eip = 0;
    VdmTib->VdmContext.Esp = 2 * PAGE_SIZE - sizeof(ULONG_PTR);
    VdmTib->VdmContext.EFlags |= EFLAGS_V86_MASK | EFLAGS_INTERRUPT_MASK;
    VdmTib->VdmContext.ContextFlags = CONTEXT_FULL;

    /* This can't be a real VDM process */
    ASSERT(PsGetCurrentProcess()->VdmObjects == NULL);

    /* Allocate VDM structure */
    VdmProcessObjects = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(VDM_PROCESS_OBJECTS),
                                              TAG('K', 'e', ' ', ' '));
    if (!VdmProcessObjects) return STATUS_NO_MEMORY;

    /* Set it up */
    RtlZeroMemory(VdmProcessObjects, sizeof(VDM_PROCESS_OBJECTS));
    VdmProcessObjects->VdmTib = VdmTib;
    PsGetCurrentProcess()->VdmObjects = VdmProcessObjects;

    /* Set the system affinity for the current thread */
    KeSetSystemAffinityThread(1);

    /* Make sure there's space for two IOPMs, then copy & clear the current */
    //ASSERT(((PKGDTENTRY)&KeGetPcr()->GDT[KGDT_TSS / 8])->LimitLow >=
    //        (0x2000 + IOPM_OFFSET - 1));
    RtlCopyMemory(Ki386IopmSaveArea, &Tss->IoMaps[0].IoMap, PAGE_SIZE * 2);
    RtlZeroMemory(&Tss->IoMaps[0].IoMap, PAGE_SIZE * 2);

    /* Save the old offset and base, and set the new ones */
    OldOffset = Process->IopmOffset;
    OldBase = Tss->IoMapBase;
    Process->IopmOffset = (USHORT)IOPM_OFFSET;
    Tss->IoMapBase = (USHORT)IOPM_OFFSET;

    /* Switch stacks and work the magic */
    Ki386SetupAndExitToV86Mode(VdmTeb);

    /* Restore IOPM */
    RtlCopyMemory(&Tss->IoMaps[0].IoMap, Ki386IopmSaveArea, PAGE_SIZE * 2);
    Process->IopmOffset = OldOffset;
    Tss->IoMapBase = OldBase;

    /* Restore affinity */
    KeRevertToUserAffinityThread();

    /* Restore context */
    RtlCopyMemory(Context, &VdmTib->VdmContext, ContextSize);
    Context->ContextFlags = CONTEXT_FULL;

    /* Free VDM objects */
    ExFreePool(PsGetCurrentProcess()->VdmObjects);
    PsGetCurrentProcess()->VdmObjects = NULL;

    /* Return status */
    return STATUS_SUCCESS;
}

/* EOF */
