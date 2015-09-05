/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/arch/i386/arch.c
 * PURPOSE:         Boot Library Architectural Initialization for i386
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

BL_ARCH_CONTEXT FirmwareExecutionContext;
BL_ARCH_CONTEXT ApplicationExecutionContext;
PBL_ARCH_CONTEXT CurrentExecutionContext;

/* FUNCTIONS *****************************************************************/

VOID
//__declspec(naked) fixme: gcc
ArchTrapNoProcess (
    VOID
    )
{
    /* Do nothing, this is an unsupported debugging interrupt */
    // _asm { iret } FIXME: GCC
}

VOID
ArchSwitchContext (
    _In_ PBL_ARCH_CONTEXT NewContext,
    _In_ PBL_ARCH_CONTEXT OldContext
    )
{
    /* Are we switching to real mode? */
    if (NewContext->Mode == BlRealMode)
    {
        /* Disable paging */
        __writecr0(__readcr0() & ~CR0_PG);

        /* Are we coming from PAE mode? */
        if ((OldContext != NULL) && (OldContext->TranslationType == BlPae))
        {
            /* Turn off PAE */
            __writecr4(__readcr4() & ~CR4_PAE);
        }

        /* Enable interrupts */
        _enable();
    }
    else
    {
        /* Switching to protected mode -- disable interrupts if needed */
        if (!(NewContext->ContextFlags & BL_CONTEXT_INTERRUPTS_ON))
        {
            _disable();
        }

        /* Are we enabling paging for the first time? */
        if (NewContext->ContextFlags & BL_CONTEXT_PAGING_ON)
        {
            /* In PAE mode? */
            if (NewContext->TranslationType == BlPae)
            {
                /* Turn on PAE */
                __writecr4(__readcr4() | CR4_PAE);
            }

            /* Turn on paging */
            __writecr0(__readcr0() | CR0_PG);
        }
    }
}

NTSTATUS
ArchInitializeContext (
    _In_ PBL_ARCH_CONTEXT Context
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Are we initializing real mode? */
    if (Context->Mode == BlRealMode)
    {
        /* Disable paging, enable interrupts */
        Context->ContextFlags &= ~BL_CONTEXT_PAGING_ON;
        Context->ContextFlags |= BL_CONTEXT_INTERRUPTS_ON;
    }
    else if (!(BlpApplicationFlags & BL_APPLICATION_FLAG_CONVERTED_FROM_EFI) ||
             (BlpLibraryParameters.TranslationType != BlNone))
    {
        /* Read the current translation type */
        Context->TranslationType = BlpLibraryParameters.TranslationType;

        /* Disable paging (it's already on), enable interrupts */
        Context->ContextFlags &= ~BL_CONTEXT_PAGING_ON;
        Context->ContextFlags |= BL_CONTEXT_INTERRUPTS_ON;

        /* Enable FXSR support in the FPU */
        __writecr4(__readcr4() | CR4_FXSR);
    }
    else
    {
        /* Invalid context */
        Status = STATUS_NOT_SUPPORTED;
    }

    /* Return context status */
    return Status;
}

NTSTATUS
ArchInitializeContexts (
    VOID
    )
{
    PBL_ARCH_CONTEXT Context = NULL;
    NTSTATUS EfiStatus, AppStatus;

    /* No current context */
    CurrentExecutionContext = NULL;

    /* Setup the EFI and Application modes respectively */
    FirmwareExecutionContext.Mode = BlRealMode;
    ApplicationExecutionContext.Mode = BlProtectedMode;

    /* Initialize application mode */
    AppStatus = ArchInitializeContext(&ApplicationExecutionContext);
    if (NT_SUCCESS(AppStatus))
    {
        /* Set it as current if it worked */
        Context = &ApplicationExecutionContext;
        CurrentExecutionContext = &ApplicationExecutionContext;
    }

    /* Initialize EFI mode */
    EfiStatus = ArchInitializeContext(&FirmwareExecutionContext);
    if (NT_SUCCESS(EfiStatus))
    {
        /* Set it as current if application context failed */
        if (!NT_SUCCESS(AppStatus))
        {
            Context = &FirmwareExecutionContext;
            CurrentExecutionContext = &FirmwareExecutionContext;
        }

        /* Switch to application mode, or EFI if that one failed */
        ArchSwitchContext(Context, NULL);
        EfiStatus = STATUS_SUCCESS;
    }

    /* Return initialization state */
    return EfiStatus;
}

VOID
BlpArchSwitchContext (
    _In_ BL_ARCH_MODE NewMode
    )
{
    PBL_ARCH_CONTEXT Context;

    /* In real mode, use EFI, otherwise, use the application mode */
    Context = &FirmwareExecutionContext;
    if (NewMode != BlProtectedMode) Context = &ApplicationExecutionContext;

    /* Are we in a different mode? */
    if (CurrentExecutionContext->Mode != NewMode)
    {
        /* Switch to the new one */
        ArchSwitchContext(Context, CurrentExecutionContext);
        CurrentExecutionContext = Context;
    }
}

/*++
* @name BlpArchInitialize
*
*     The BlpArchInitialize function initializes the Boot Library.
*
* @param  Phase
*         Pointer to the Boot Application Parameter Block.
*
* @return NT_SUCCESS if the boot library was loaded correctly, relevant error
*         otherwise.
*
*--*/
NTSTATUS
BlpArchInitialize (
    _In_ ULONG Phase
    )
{
    KDESCRIPTOR Idtr;
    USHORT CodeSegment;
    NTSTATUS Status;
    PKIDTENTRY IdtBase;

    /* Assume success */
    Status = STATUS_SUCCESS;

    /* Is this phase 1? */
    if (Phase != 0)
    {
        /* Get the IDT */
        __sidt(&Idtr);
        IdtBase = (PKIDTENTRY)Idtr.Base;

        /* Get the Code Segment */
       // _asm { mov CodeSegment, cs } FIXME: GCC
        CodeSegment = 8; // fix fix

        /* Set up INT 3, ASSERT, and SECURITY_ASSERT to be no-op (for Rtl) */
        IdtBase[3].Offset = (USHORT)(ULONG_PTR)ArchTrapNoProcess;
        IdtBase[3].Selector = CodeSegment;
        IdtBase[3].Access = 0x8E00u;
        IdtBase[3].ExtendedOffset = (ULONG_PTR)ArchTrapNoProcess >> 16;
        IdtBase[0x2C].Offset = (USHORT)(ULONG_PTR)ArchTrapNoProcess;
        IdtBase[0x2C].Selector = CodeSegment;
        IdtBase[0x2C].Access = 0x8E00u;
        IdtBase[0x2C].ExtendedOffset = (ULONG_PTR)ArchTrapNoProcess >> 16;
        IdtBase[0x2D].Offset = (USHORT)(ULONG_PTR)ArchTrapNoProcess;
        IdtBase[0x2D].Selector = CodeSegment;
        IdtBase[0x2D].Access = 0x8E00u;
        IdtBase[0x2D].ExtendedOffset = (ULONG_PTR)ArchTrapNoProcess >> 16;

        /* Write the IDT back */
        Idtr.Base = (ULONG)IdtBase;
        __lidt(&Idtr);

        /* Reset FPU state */
       //  __asm { fninit } FIXME: GCC
    }
    else
    {
        /* Reset TSC if needed */
        if ((__readmsr(0x10) >> 32) & 0xFFC00000)
        {
            __writemsr(0x10, 0);
        }

        /* Initialize all the contexts */
        Status = ArchInitializeContexts();
    }

    /* Return initialization state */
    return Status;
}

