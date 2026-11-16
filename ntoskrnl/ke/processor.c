/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        ntoskrnl/ke/processor.c
 * PURPOSE:     Portable processor related routines
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * output/netkvm-20261115T235709Z.log names KeQueryActiveProcessorCountEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-kequeryactiveprocessorcountex
 * The active count is KeNumberProcessors in this file. Group 0 and
 * ALL_PROCESSOR_GROUPS from sdk/include/xdk/ntbasedef.h return that count.
 * Any other group returns 0, as KeQueryMaximumProcessorCountEx does here.
 * output/netkvm-20261116T001204Z.log names KeGetProcessorNumberFromIndex.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-kegetprocessornumberfromindex
 * An index below KeNumberProcessors is group 0 and that index. Any other
 * index returns STATUS_INVALID_PARAMETER from sdk/include/psdk/ntstatus.h.
 * output/netkvm-20261116T001444Z.log names KeGetCurrentProcessorNumberEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-kegetcurrentprocessornumberex
 * The return value is KeGetCurrentProcessorNumber. A non-null output
 * receives group 0 and that number.
 * output/netkvm-20261116T002118Z.log names KeGetProcessorIndexFromNumber.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-kegetprocessorindexfromnumber
 * Group 0 and a number below KeNumberProcessors return that number.
 * Any other value returns INVALID_PROCESSOR_INDEX from
 * sdk/include/xdk/ketypes.h.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

KAFFINITY KeActiveProcessors = 0;

/* Number of processors */
CCHAR KeNumberProcessors = 0;

#ifdef CONFIG_SMP

/* Theoretical maximum number of processors that can be handled.
 * Set once at run-time. Returned by KeQueryMaximumProcessorCount(). */
ULONG KeMaximumProcessors = MAXIMUM_PROCESSORS;

/* Maximum number of logical processors that can be started
 * (including dynamically) at run-time. If 0: do not perform checks. */
ULONG KeNumprocSpecified = 0;

/* Maximum number of logical processors that can be started
 * at boot-time. If 0: do not perform checks. */
ULONG KeBootprocSpecified = 0;

#endif // CONFIG_SMP

/* FUNCTIONS *****************************************************************/

KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID)
{
    return KeActiveProcessors;
}

/*
 * One processor group. The maximum is the number the kernel can name,
 * which stays constant for the boot. A group other than 0 is empty.
 */
ULONG
NTAPI
KeQueryMaximumProcessorCount(VOID)
{
#ifdef CONFIG_SMP
    return KeMaximumProcessors;
#else
    return 1;
#endif
}

ULONG
NTAPI
KeQueryMaximumProcessorCountEx(
    _In_ USHORT GroupNumber)
{
    if (GroupNumber == ALL_PROCESSOR_GROUPS || GroupNumber == 0)
    {
        return KeQueryMaximumProcessorCount();
    }

    return 0;
}

ULONG
NTAPI
KeQueryActiveProcessorCountEx(
    _In_ USHORT GroupNumber)
{
    if (GroupNumber == ALL_PROCESSOR_GROUPS || GroupNumber == 0)
    {
        return (ULONG)KeNumberProcessors;
    }

    return 0;
}

NTSTATUS
NTAPI
KeGetProcessorNumberFromIndex(
    _In_ ULONG ProcIndex,
    _Out_ PPROCESSOR_NUMBER ProcNumber)
{
    if (ProcIndex >= (ULONG)KeNumberProcessors)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ProcNumber->Group = 0;
    ProcNumber->Number = (UCHAR)ProcIndex;
    ProcNumber->Reserved = 0;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
KeGetCurrentProcessorNumberEx(
    _Out_opt_ PPROCESSOR_NUMBER ProcNumber)
{
    ULONG Number = KeGetCurrentProcessorNumber();

    if (ProcNumber != NULL)
    {
        ProcNumber->Group = 0;
        ProcNumber->Number = (UCHAR)Number;
        ProcNumber->Reserved = 0;
    }

    return Number;
}

ULONG
NTAPI
KeGetProcessorIndexFromNumber(
    _In_ PPROCESSOR_NUMBER ProcNumber)
{
    if (ProcNumber->Group != 0 || ProcNumber->Number >= (UCHAR)KeNumberProcessors)
    {
        return INVALID_PROCESSOR_INDEX;
    }

    return ProcNumber->Number;
}

/**
 * Retrieves the number of the current processor.
 *
 * \param ProcessorNumber Pointer to a PROCESSOR_NUMBER structure that receives the processor number.
 *
 * \return NTSTATUS The status of the operation.
 */
NTSTATUS
NTAPI
NtGetCurrentProcessorNumberEx(
    _Out_ PPROCESSOR_NUMBER ProcessorNumber)
{
    _SEH2_TRY
    {
        ProbeForWrite(ProcessorNumber, sizeof(PROCESSOR_NUMBER), __alignof(PROCESSOR_NUMBER));
        ProcessorNumber->Group = 0; // TODO: Support processor groups
        ProcessorNumber->Number = (UCHAR)KeGetCurrentProcessorNumber();
        ProcessorNumber->Reserved = 0;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}
