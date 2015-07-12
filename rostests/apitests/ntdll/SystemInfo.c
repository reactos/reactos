/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtQuery/SetSystemInformation
 * PROGRAMMERS:     Timo Kreuzer
 *                  Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/exfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>

#define ntv6(x) (LOBYTE(LOWORD(GetVersion())) >= 6 ? (x) : 0)

static
void
Test_Flags(void)
{
    NTSTATUS Status;
    ULONG ReturnLength;
    ULONG Flags;
    ULONG Buffer[2];
    ULONG Buffer2[2];
    PSYSTEM_FLAGS_INFORMATION FlagsInfo = (PVOID)Buffer;
    BOOLEAN PrivilegeEnabled;

    /* Query */
    ReturnLength = 0x55555555;
    Status = NtQuerySystemInformation(SystemFlagsInformation, NULL, 0, &ReturnLength);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0 ||
       ntv6(ReturnLength == sizeof(SYSTEM_FLAGS_INFORMATION)), "ReturnLength = %lu\n", ReturnLength);

    ReturnLength = 0x55555555;
    Status = NtQuerySystemInformation(SystemFlagsInformation, NULL, sizeof(SYSTEM_FLAGS_INFORMATION) - 1, &ReturnLength);
    ok(Status == STATUS_ACCESS_VIOLATION, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0x55555555, "ReturnLength = %lu\n", ReturnLength);

    ReturnLength = 0x55555555;
    Status = NtQuerySystemInformation(SystemFlagsInformation, (PVOID)2, 0, &ReturnLength);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0 ||
       ntv6(ReturnLength == sizeof(SYSTEM_FLAGS_INFORMATION)), "ReturnLength = %lu\n", ReturnLength);

    ReturnLength = 0x55555555;
    Status = NtQuerySystemInformation(SystemFlagsInformation, (PVOID)4, 0, &ReturnLength);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0 ||
       ntv6(ReturnLength == sizeof(SYSTEM_FLAGS_INFORMATION)), "ReturnLength = %lu\n", ReturnLength);

    ReturnLength = 0x55555555;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, (PUCHAR)FlagsInfo + 2, sizeof(SYSTEM_FLAGS_INFORMATION), &ReturnLength);
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0x55555555, "ReturnLength = %lu\n", ReturnLength);
    ok(Buffer[0] == 0x55555555, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    ReturnLength = 0x55555555;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, (PUCHAR)FlagsInfo + 2, sizeof(SYSTEM_FLAGS_INFORMATION) - 1, &ReturnLength);
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0x55555555, "ReturnLength = %lu\n", ReturnLength);
    ok(Buffer[0] == 0x55555555, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    ReturnLength = 0x55555555;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION) - 1, &ReturnLength);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0 ||
       ntv6(ReturnLength == sizeof(SYSTEM_FLAGS_INFORMATION)), "ReturnLength = %lu\n", ReturnLength);
    ok(Buffer[0] == 0x55555555, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    ReturnLength = 0x55555555;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION), &ReturnLength);
    ok(Status == STATUS_SUCCESS, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == sizeof(SYSTEM_FLAGS_INFORMATION), "ReturnLength = %lu\n", ReturnLength);
    ok(FlagsInfo->Flags != 0x55555555, "Flags = %lx\n", FlagsInfo->Flags);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);
    Flags = FlagsInfo->Flags;

    ReturnLength = 0x55555555;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION) + 1, &ReturnLength);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0 ||
       ntv6(ReturnLength == sizeof(SYSTEM_FLAGS_INFORMATION)), "ReturnLength = %lu\n", ReturnLength);
    ok(Buffer[0] == 0x55555555, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION), NULL);
    ok(Status == STATUS_SUCCESS, "NtQuerySystemInformation returned %lx\n", Status);
    ok(FlagsInfo->Flags == Flags, "Flags = %lx, expected %lx\n", FlagsInfo->Flags, Flags);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION), (PVOID)4);
    ok(Status == STATUS_ACCESS_VIOLATION, "NtQuerySystemInformation returned %lx\n", Status);
    ok(Buffer[0] == 0x55555555, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION), (PVOID)2);
    ok(Status == STATUS_ACCESS_VIOLATION, "NtQuerySystemInformation returned %lx\n", Status);
    ok(Buffer[0] == 0x55555555, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION), (PVOID)1);
    ok(Status == STATUS_ACCESS_VIOLATION, "NtQuerySystemInformation returned %lx\n", Status);
    ok(Buffer[0] == 0x55555555, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    RtlFillMemory(Buffer2, sizeof(Buffer2), 0x55);
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION), (PULONG)((PUCHAR)Buffer2 + 1));
    ok(Status == STATUS_SUCCESS, "NtQuerySystemInformation returned %lx\n", Status);
    ok(FlagsInfo->Flags == Flags, "Flags = %lx, expected %lx\n", FlagsInfo->Flags, Flags);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);
    ok(Buffer2[0] == 0x00000455, "Buffer2[0] = %lx\n", Buffer2[0]);
    ok(Buffer2[1] == 0x55555500, "Buffer2[1] = %lx\n", Buffer2[1]);

    /* Set */
    Status = NtSetSystemInformation(SystemFlagsInformation, NULL, 0);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, NULL, sizeof(SYSTEM_FLAGS_INFORMATION) - 1);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, NULL, sizeof(SYSTEM_FLAGS_INFORMATION));
    ok(Status == STATUS_ACCESS_DENIED, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, (PVOID)2, sizeof(SYSTEM_FLAGS_INFORMATION));
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "NtSetSystemInformation returned %lx\n", Status);

    Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &PrivilegeEnabled);
    if (!NT_SUCCESS(Status))
    {
        skip("Cannot acquire SeDebugPrivilege\n");
        return;
    }

    Status = NtSetSystemInformation(SystemFlagsInformation, NULL, sizeof(SYSTEM_FLAGS_INFORMATION));
    ok(Status == STATUS_ACCESS_VIOLATION, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, (PVOID)2, sizeof(SYSTEM_FLAGS_INFORMATION));
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, (PVOID)4, sizeof(SYSTEM_FLAGS_INFORMATION));
    ok(Status == STATUS_ACCESS_VIOLATION, "NtSetSystemInformation returned %lx\n", Status);

    FlagsInfo->Flags = Flags;
    Status = NtSetSystemInformation(SystemFlagsInformation, (PUCHAR)FlagsInfo + 2, sizeof(SYSTEM_FLAGS_INFORMATION));
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, (PUCHAR)FlagsInfo + 2, sizeof(SYSTEM_FLAGS_INFORMATION) - 1);
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION) - 1);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION) + 1);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtSetSystemInformation returned %lx\n", Status);

    Status = NtSetSystemInformation(SystemFlagsInformation, FlagsInfo, sizeof(SYSTEM_FLAGS_INFORMATION));
    ok(Status == STATUS_SUCCESS, "NtSetSystemInformation returned %lx\n", Status);

    ok(FlagsInfo->Flags == Flags, "Flags = %lx, expected %lu\n", FlagsInfo->Flags, Flags);

    Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, PrivilegeEnabled, FALSE, &PrivilegeEnabled);
    ok(Status == STATUS_SUCCESS, "RtlAdjustPrivilege returned %lx\n", Status);
}

static
void
Test_TimeAdjustment(void)
{
    SYSTEM_QUERY_TIME_ADJUST_INFORMATION TimeInfoOrg, GetTimeInfo;
    SYSTEM_SET_TIME_ADJUST_INFORMATION SetTimeInfo;
    NTSTATUS Status;
    ULONG ReturnLength;
    BOOLEAN PrivilegeEnabled;

    SetTimeInfo.TimeAdjustment = 0;
    SetTimeInfo.Enable = 0;

    /* Query original values */
    Status = NtQuerySystemInformation(SystemTimeAdjustmentInformation,
                                      &TimeInfoOrg,
                                      sizeof(TimeInfoOrg),
                                      &ReturnLength);

    /* Test without privilege */
    Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
                                    &SetTimeInfo,
                                    sizeof(SetTimeInfo));
    ok_ntstatus(Status, STATUS_PRIVILEGE_NOT_HELD);

    /* Get the required privilege */
    Status = RtlAdjustPrivilege(SE_SYSTEMTIME_PRIVILEGE, TRUE, FALSE, &PrivilegeEnabled);
    if (!NT_SUCCESS(Status))
    {
        skip("Cannot acquire SeSystemTimePrivilege\n");
        return;
    }

    /* Test wrong length */
    Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
                                    &SetTimeInfo,
                                    sizeof(SetTimeInfo) + 1);
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

    /* Test both members 0 */
    Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
                                    &SetTimeInfo,
                                    sizeof(SetTimeInfo));
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

    /* Set huge value */
    SetTimeInfo.TimeAdjustment = -1;
    SetTimeInfo.Enable = 0;
    Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
                                    &SetTimeInfo,
                                    sizeof(SetTimeInfo));
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Query the result */
    Status = NtQuerySystemInformation(SystemTimeAdjustmentInformation,
                                      &GetTimeInfo,
                                      sizeof(GetTimeInfo),
                                      &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_long(GetTimeInfo.TimeAdjustment, -1);
    ok_long(GetTimeInfo.Enable, 0);

    /* set Enable to 1 */
    SetTimeInfo.TimeAdjustment = -1;
    SetTimeInfo.Enable = 1;
    Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
                                    &SetTimeInfo,
                                    sizeof(SetTimeInfo));
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Query the result */
    Status = NtQuerySystemInformation(SystemTimeAdjustmentInformation,
                                      &GetTimeInfo,
                                      sizeof(GetTimeInfo),
                                      &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_long(GetTimeInfo.TimeAdjustment, GetTimeInfo.TimeIncrement);
    ok_long(GetTimeInfo.Enable, 1);

    /* Restore original values */
    SetTimeInfo.TimeAdjustment = TimeInfoOrg.TimeAdjustment;
    SetTimeInfo.Enable = TimeInfoOrg.Enable;;
    Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
                                    &SetTimeInfo,
                                    sizeof(SetTimeInfo));
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = RtlAdjustPrivilege(SE_SYSTEMTIME_PRIVILEGE, PrivilegeEnabled, FALSE, &PrivilegeEnabled);
    ok(Status == STATUS_SUCCESS, "RtlAdjustPrivilege returned %lx\n", Status);
}

static
void
Test_KernelDebugger(void)
{
    NTSTATUS Status;
    ULONG ReturnLength;
    ULONG Buffer[2];
    PSYSTEM_KERNEL_DEBUGGER_INFORMATION DebuggerInfo = (PVOID)Buffer;

    /* Query */
    ReturnLength = 0x55555555;
    Status = NtQuerySystemInformation(SystemKernelDebuggerInformation, NULL, 0, &ReturnLength);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0 ||
       ntv6(ReturnLength == sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION)), "ReturnLength = %lu\n", ReturnLength);

    ReturnLength = 0x55555555;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemKernelDebuggerInformation, DebuggerInfo, sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION) - 1, &ReturnLength);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0 ||
       ntv6(ReturnLength == sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION)), "ReturnLength = %lu\n", ReturnLength);
    ok(Buffer[0] == 0x55555555, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    ReturnLength = 0x55555555;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemKernelDebuggerInformation, (PUCHAR)DebuggerInfo + 1, sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION), &ReturnLength);
    ok(Status == STATUS_SUCCESS, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION), "ReturnLength = %lu\n", ReturnLength);
    ok((Buffer[0] & 0x55fefe55) == 0x55000055, "Buffer[0] = %lx\n", Buffer[0]);
    ok(Buffer[1] == 0x55555555, "Buffer[1] = %lx\n", Buffer[1]);

    ReturnLength = 0x55555555;
    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    Status = NtQuerySystemInformation(SystemKernelDebuggerInformation, DebuggerInfo, sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION), &ReturnLength);
    ok(Status == STATUS_SUCCESS, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION), "ReturnLength = %lu\n", ReturnLength);
    ok(DebuggerInfo->KernelDebuggerEnabled == FALSE ||
       DebuggerInfo->KernelDebuggerEnabled == TRUE, "KernelDebuggerEnabled = %u\n", DebuggerInfo->KernelDebuggerEnabled);
    ok(DebuggerInfo->KernelDebuggerNotPresent == FALSE ||
       DebuggerInfo->KernelDebuggerNotPresent == TRUE, "KernelDebuggerNotPresent = %u\n", DebuggerInfo->KernelDebuggerNotPresent);

    /* Set - not supported */
    DebuggerInfo->KernelDebuggerEnabled = FALSE;
    DebuggerInfo->KernelDebuggerNotPresent = TRUE;
    Status = NtSetSystemInformation(SystemKernelDebuggerInformation, DebuggerInfo, sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION));
    ok(Status == STATUS_INVALID_INFO_CLASS, "NtSetSystemInformation returned %lx\n", Status);
}

START_TEST(NtSystemInformation)
{
    NTSTATUS Status;
    ULONG ReturnLength;

    Status = NtQuerySystemInformation(9999, NULL, 0, NULL);
    ok(Status == STATUS_INVALID_INFO_CLASS, "NtQuerySystemInformation returned %lx\n", Status);

    Status = NtQuerySystemInformation(9999, NULL, 0, (PVOID)1);
    ok(Status == STATUS_ACCESS_VIOLATION ||
       ntv6(Status == STATUS_INVALID_INFO_CLASS), "NtQuerySystemInformation returned %lx\n", Status);

    ReturnLength = 0x55555555;
    Status = NtQuerySystemInformation(9999, NULL, 0, &ReturnLength);
    ok(Status == STATUS_INVALID_INFO_CLASS, "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0 ||
       ntv6(ReturnLength == 0x55555555), "ReturnLength = %lu\n", ReturnLength);

    ReturnLength = 0x55555555;
    Status = NtQuerySystemInformation(9999, NULL, 1, &ReturnLength);
    ok(Status == STATUS_ACCESS_VIOLATION ||
       ntv6(Status == STATUS_INVALID_INFO_CLASS), "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0x55555555, "ReturnLength = %lu\n", ReturnLength);

    ReturnLength = 0x55555555;
    Status = NtQuerySystemInformation(9999, (PVOID)1, 1, &ReturnLength);
    ok(Status == STATUS_DATATYPE_MISALIGNMENT ||
       ntv6(Status == STATUS_INVALID_INFO_CLASS), "NtQuerySystemInformation returned %lx\n", Status);
    ok(ReturnLength == 0x55555555, "ReturnLength = %lu\n", ReturnLength);

    Status = NtQuerySystemInformation(9999, NULL, 1, (PVOID)1);
    ok(Status == STATUS_ACCESS_VIOLATION ||
       ntv6(Status == STATUS_INVALID_INFO_CLASS), "NtQuerySystemInformation returned %lx\n", Status);

    Status = NtQuerySystemInformation(9999, (PVOID)1, 1, (PVOID)1);
    ok(Status == STATUS_DATATYPE_MISALIGNMENT ||
       ntv6(Status == STATUS_INVALID_INFO_CLASS), "NtQuerySystemInformation returned %lx\n", Status);

    Test_Flags();
    Test_TimeAdjustment();
    Test_KernelDebugger();
}
