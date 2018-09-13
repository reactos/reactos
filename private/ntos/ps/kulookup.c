/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kulookup.c

Abstract:

    The module implements the code necessary to lookup user mode entry points
    in the system DLL for exception dispatching and APC delivery.

Author:

    David N. Cutler (davec) 8-Oct-90

Revision History:

--*/

#include "psp.h"
#pragma alloc_text(INIT, PspLookupKernelUserEntryPoints)

NTSTATUS
PspLookupKernelUserEntryPoints (
    VOID
    )

/*++

Routine Description:

    The function locates the address of the exception dispatch and user APC
    delivery routine in the system DLL and stores the respective addresses
    in the PCR.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{

    NTSTATUS Status;
    PSZ EntryName;

    //
    // Lookup the user mode "trampoline" code for exception dispatching
    //

    EntryName = "KiUserExceptionDispatcher";
    Status = PspLookupSystemDllEntryPoint(EntryName,
                                          (PVOID *)&KeUserExceptionDispatcher);
    if (NT_SUCCESS(Status) == FALSE) {
        KdPrint(("Ps: Cannot find user exception dispatcher address\n"));
        return Status;
    }

    //
    // Lookup the user mode "trampoline" code for APC dispatching
    //

    EntryName = "KiUserApcDispatcher";
    Status = PspLookupSystemDllEntryPoint(EntryName,
                                          (PVOID *)&KeUserApcDispatcher);
    if (NT_SUCCESS(Status) == FALSE) {
        KdPrint(("Ps: Cannot find user apc dispatcher address\n"));
        return Status;
    }

    //
    // Lookup the user mode "trampoline" code for callback dispatching.
    //

    EntryName = "KiUserCallbackDispatcher";
    Status = PspLookupSystemDllEntryPoint(EntryName,
                                          (PVOID *)&KeUserCallbackDispatcher);
    if (NT_SUCCESS(Status) == FALSE) {
        KdPrint(("Ps: Cannot find user callback dispatcher address\n"));
        return Status;
    }

    //
    // Lookup the user mode "trampoline" code for raising a usermode exception
    //

    EntryName = "KiRaiseUserExceptionDispatcher";
    Status = PspLookupSystemDllEntryPoint(EntryName,
                                          (PVOID *)&KeRaiseUserExceptionDispatcher);
    if (NT_SUCCESS(Status) == FALSE) {
        KdPrint(("Ps: Cannot find raise user exception dispatcher address\n"));
        return Status;
    }

    return Status;
}
