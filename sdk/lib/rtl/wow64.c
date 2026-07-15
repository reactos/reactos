/*
 * PROJECT:     ReactOS runtime library (RTL)
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of RtlWow64GetProcessMachines
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
RtlWow64GetProcessMachines(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessMachine,
    _Out_opt_ PUSHORT NativeMachine)
{
    NTSTATUS Status;
    ULONG_PTR Wow64Info;

    /* Check if the caller wants the current process */
    if (ProcessHandle == NtCurrentProcess())
    {
        /* Easy: process machine is current architecture */
        *ProcessMachine  = IMAGE_FILE_MACHINE_NATIVE;
    }
    else if (SharedUserData->ImageNumberLow == IMAGE_FILE_MACHINE_AMD64)
    {
        /* Kernel architecture is AMD64, query whether the process is WOW64 */
        Status = NtQueryInformationProcess(ProcessHandle,
                                           ProcessWow64Information,
                                           &Wow64Info,
                                           sizeof(Wow64Info),
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        *ProcessMachine = Wow64Info ? IMAGE_FILE_MACHINE_I386 : IMAGE_FILE_MACHINE_AMD64;
    }
    else
    {
        /* kernel is something else, assume no WOW64 */
        *ProcessMachine  = IMAGE_FILE_MACHINE_NATIVE;
    }

    if (NativeMachine != NULL)
    {
        /* The kernel stores it's native architecture in this field */
        *NativeMachine = SharedUserData->ImageNumberLow;
    }

    return STATUS_SUCCESS;
}
