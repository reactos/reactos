/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel-Mode Test Suite for KdSystemDebugControl (kernel-mode)
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <kmt_test.h>
#include <ndk/kdfuncs.h>

#define ok_eq_print_test(testid, value, expected, spec) \
    ok((value) == (expected), "In test %lu: " #value " = " spec ", expected " spec "\n", testid, value, expected)

#define ok_eq_hex_test(testid, value, expected) \
    ok_eq_print_test(testid, value, expected, "0x%08lx")

#define ok_neq_print_test(testid, value, expected, spec) \
    ok((value) != (expected), "In test %lu: " #value " = " spec ", expected != " spec "\n", testid, value, expected)

#define ok_neq_hex_test(testid, value, expected) \
    ok_neq_print_test(testid, value, expected, "0x%08lx")


BOOLEAN
(NTAPI *pKdRefreshDebuggerNotPresent)(VOID);

NTSTATUS
(NTAPI *pKdSystemDebugControl)(
    _In_ SYSDBG_COMMAND Command,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Inout_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE PreviousMode);


static
NTSTATUS
TestSystemDebugControl(
    _In_ SYSDBG_COMMAND Command)
{
    return pKdSystemDebugControl(Command,
                                 NULL, // _In_ PVOID InputBuffer,
                                 0,    // _In_ ULONG InputBufferLength,
                                 NULL, // _Out_ PVOID OutputBuffer,
                                 0,    // _In_ ULONG OutputBufferLength,
                                 NULL,
                                 KernelMode);
}

START_TEST(KdSystemDebugControl)
{
    NTSTATUS Status;
    ULONG Command;
    RTL_OSVERSIONINFOEXW verInfo;
    UNICODE_STRING fnName;
    BOOLEAN IsNT52SP1OrHigher;
    BOOLEAN IsVistaOrHigher;
    BOOLEAN IsDebuggerActive;

    /* Test for OS version: KdSystemDebugControl()
     * exists only on NT 5.2 SP1 and higher */
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&verInfo);
    if (skip(NT_SUCCESS(Status), "RtlGetVersion() returned 0x%08lx\n", Status))
        return;

    // IsWindowsVersionOrGreater(5, 2, 1);
    IsNT52SP1OrHigher =
        (verInfo.dwMajorVersion > 5) ||
        (verInfo.dwMajorVersion == 5 && verInfo.dwMinorVersion > 2) ||
        (verInfo.dwMajorVersion == 5 && verInfo.dwMinorVersion == 2 && verInfo.wServicePackMajor >= 1);

    if (skip(IsNT52SP1OrHigher, "KdSystemDebugControl() only exists on NT 5.2 SP1 and higher\n"))
        return;

    // IsWindowsVersionOrGreater(6, 0, 0);
    IsVistaOrHigher = (verInfo.dwMajorVersion >= 6);


    /* Load the Kd routines at runtime */

    /* Note: KdRefreshDebuggerNotPresent() is NT 5.2+ */
    RtlInitUnicodeString(&fnName, L"KdRefreshDebuggerNotPresent");
    pKdRefreshDebuggerNotPresent = MmGetSystemRoutineAddress(&fnName);
    ok(!!pKdRefreshDebuggerNotPresent,
       "KdRefreshDebuggerNotPresent() unavailable but OS is NT 5.2 SP1 or higher?\n");

    RtlInitUnicodeString(&fnName, L"KdSystemDebugControl");
    pKdSystemDebugControl = MmGetSystemRoutineAddress(&fnName);
    if (skip(!!pKdSystemDebugControl, "KdSystemDebugControl() unavailable but OS is NT 5.2 SP1 or higher?\n"))
        return;


    /* Check whether the kernel debugger is present or not */
    IsDebuggerActive = (pKdRefreshDebuggerNotPresent
                     ? !pKdRefreshDebuggerNotPresent()
                     : (/*KD_DEBUGGER_ENABLED &&*/ !KD_DEBUGGER_NOT_PRESENT));

    trace("Debugger is %s\n", IsDebuggerActive ? "active" : "inactive");

    /* Unsupported commands */
    for (Command = 0; Command <= 6; ++Command)
    {
        Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
        if (!IsVistaOrHigher || IsDebuggerActive)
            ok_eq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
        else
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }

    /*
     * Supported commands:
     *
     * SysDbgQueryVersion = 7,
     * SysDbgReadVirtual  = 8,
     * SysDbgWriteVirtual = 9,
     * SysDbgReadPhysical  = 10,
     * SysDbgWritePhysical = 11,
     * SysDbgReadControlSpace  = 12,
     * SysDbgWriteControlSpace = 13,
     * SysDbgReadIoSpace  = 14,
     * SysDbgWriteIoSpace = 15,
     * SysDbgReadMsr  = 16,
     * SysDbgWriteMsr = 17,
     * SysDbgReadBusData  = 18,
     * SysDbgWriteBusData = 19,
     * SysDbgCheckLowMemory = 20
     */
    for (Command = 7; Command <= 20; ++Command)
    {
        Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
        if (!IsVistaOrHigher || IsDebuggerActive)
            ok_neq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS); // Status must be != STATUS_INVALID_INFO_CLASS
        else
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }

    /* Unsupported commands */
    for (Command = 21; Command <= 40; ++Command)
    {
        Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
        if (!IsVistaOrHigher || IsDebuggerActive)
            ok_eq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
        else
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }
}

/* EOF */
