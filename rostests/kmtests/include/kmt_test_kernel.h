/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite test framework declarations
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _KMTEST_TEST_KERNEL_H_
#define _KMTEST_TEST_KERNEL_H_

#if !defined _KMTEST_TEST_H_
#error include kmt_test.h instead of including kmt_test_kernel.h
#endif /* !defined _KMTEST_TEST_H_ */

BOOLEAN KmtIsCheckedBuild;
BOOLEAN KmtIsMultiProcessorBuild;
PCSTR KmtMajorFunctionNames[] =
{
    "Create",
    "CreateNamedPipe",
    "Close",
    "Read",
    "Write",
    "QueryInformation",
    "SetInformation",
    "QueryEa",
    "SetEa",
    "FlushBuffers",
    "QueryVolumeInformation",
    "SetVolumeInformation",
    "DirectoryControl",
    "FileSystemControl",
    "DeviceControl",
    "InternalDeviceControl/Scsi",
    "Shutdown",
    "LockControl",
    "Cleanup",
    "CreateMailslot",
    "QuerySecurity",
    "SetSecurity",
    "Power",
    "SystemControl",
    "DeviceChange",
    "QueryQuota",
    "SetQuota",
    "Pnp/PnpPower"
};

VOID KmtSetIrql(IN KIRQL NewIrql)
{
    KIRQL Irql = KeGetCurrentIrql();
    if (Irql > NewIrql)
        KeLowerIrql(NewIrql);
    else if (Irql < NewIrql)
        KeRaiseIrql(NewIrql, &Irql);
}

BOOLEAN KmtAreInterruptsEnabled(VOID)
{
    return (__readeflags() & (1 << 9)) != 0;
}

typedef struct _POOL_HEADER
{
    union
    {
        struct
        {
#ifdef _M_AMD64
            USHORT PreviousSize:8;
            USHORT PoolIndex:8;
            USHORT BlockSize:8;
            USHORT PoolType:8;
#else
            USHORT PreviousSize:9;
            USHORT PoolIndex:7;
            USHORT BlockSize:9;
            USHORT PoolType:7;
#endif
        };
        ULONG Ulong1;
    };
#ifdef _M_AMD64
    ULONG PoolTag;
#endif
    union
    {
#ifdef _M_AMD64
        PEPROCESS ProcessBilled;
#else
        ULONG PoolTag;
#endif
        struct
        {
            USHORT AllocatorBackTraceIndex;
            USHORT PoolTagHash;
        };
    };
} POOL_HEADER, *PPOOL_HEADER;

ULONG KmtGetPoolTag(PVOID Memory)
{
    PPOOL_HEADER Header;

    /* it's not so easy for allocations of PAGE_SIZE */
    if (((ULONG_PTR)Memory & (PAGE_SIZE - 1)) == 0)
        return 'TooL';

    Header = Memory;
    Header--;

    return Header->PoolTag;
}

USHORT KmtGetPoolType(PVOID Memory)
{
    PPOOL_HEADER Header;

    /* it's not so easy for allocations of PAGE_SIZE */
    if (((ULONG_PTR)Memory & (PAGE_SIZE - 1)) == 0)
        return 0;

    Header = Memory;
    Header--;

    return Header->PoolType;
}

PVOID KmtGetSystemRoutineAddress(IN PCWSTR RoutineName)
{
    UNICODE_STRING RoutineNameString;
    RtlInitUnicodeString(&RoutineNameString, (PWSTR)RoutineName);
    return MmGetSystemRoutineAddress(&RoutineNameString);
}

PKTHREAD KmtStartThread(IN PKSTART_ROUTINE StartRoutine, IN PVOID StartContext OPTIONAL)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;
    PVOID ThreadObject = NULL;

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    ThreadHandle = INVALID_HANDLE_VALUE;
    Status = PsCreateSystemThread(&ThreadHandle,
                                  SYNCHRONIZE,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  StartRoutine,
                                  StartContext);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status) && ThreadHandle != NULL && ThreadHandle != INVALID_HANDLE_VALUE, "No thread\n"))
    {
        Status = ObReferenceObjectByHandle(ThreadHandle,
                                           SYNCHRONIZE,
                                           *PsThreadType,
                                           KernelMode,
                                           &ThreadObject,
                                           NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ObCloseHandle(ThreadHandle, KernelMode);
    }
    return ThreadObject;
}

VOID KmtFinishThread(IN PKTHREAD Thread OPTIONAL, IN PKEVENT Event OPTIONAL)
{
    NTSTATUS Status;

    if (skip(Thread != NULL, "No thread\n"))
        return;

    if (Event)
        KeSetEvent(Event, IO_NO_INCREMENT, TRUE);
    Status = KeWaitForSingleObject(Thread,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ObDereferenceObject(Thread);
}

INT __cdecl KmtVSNPrintF(PSTR Buffer, SIZE_T BufferMaxLength, PCSTR Format, va_list Arguments) KMT_FORMAT(ms_printf, 3, 0);

#endif /* !defined _KMTEST_TEST_KERNEL_H_ */
