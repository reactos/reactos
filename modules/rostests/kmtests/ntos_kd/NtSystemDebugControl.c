/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel-Mode Test Suite for NtSystemDebugControl (user-mode)
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <kmt_test.h>
#include <ndk/exfuncs.h>
#include <ndk/kdfuncs.h>
#include <ndk/setypes.h>

#define ok_eq_print_test(testid, value, expected, spec) \
    ok((value) == (expected), "In test %lu: " #value " = " spec ", expected " spec "\n", testid, value, expected)

#define ok_eq_hex_test(testid, value, expected) \
    ok_eq_print_test(testid, value, expected, "0x%08lx")

#define ok_neq_print_test(testid, value, expected, spec) \
    ok((value) != (expected), "In test %lu: " #value " = " spec ", expected != " spec "\n", testid, value, expected)

#define ok_neq_hex_test(testid, value, expected) \
    ok_neq_print_test(testid, value, expected, "0x%08lx")

ULONG
GetNtDdiVersion(VOID)
{
    RTL_OSVERSIONINFOEXW verInfo;
    NTSTATUS Status;
    ULONG Version;

    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&verInfo);
    if (!NT_SUCCESS(Status))
    {
        trace("RtlGetVersion() returned 0x%08lx\n", Status);
        return 0;
    }

    Version = ((((verInfo.dwMajorVersion & 0xFF)  << 8)  |
                 (verInfo.dwMinorVersion & 0xFF)) << 16) |
              (((verInfo.wServicePackMajor & 0xFF) << 8) |
                (verInfo.wServicePackMinor & 0xFF));

    return Version;
}

static
NTSTATUS
TestSystemDebugControl(
    _In_ SYSDBG_COMMAND Command)
{
    return NtSystemDebugControl(Command,
                                NULL, // _In_ PVOID InputBuffer,
                                0,    // _In_ ULONG InputBufferLength,
                                NULL, // _Out_ PVOID OutputBuffer,
                                0,    // _In_ ULONG OutputBufferLength,
                                NULL);
}

START_TEST(NtSystemDebugControl)
{
    NTSTATUS Status;
    ULONG Command;
    ULONG Version;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION DebuggerInfo = {0};
    BOOLEAN IsNT52SP1OrHigher;
    BOOLEAN IsVistaOrHigher;
    BOOLEAN IsDebuggerActive;
    BOOLEAN PrivilegeSet[2] = {FALSE};
    BOOLEAN WasDebuggerEnabled;

    /* Test for OS version: KdSystemDebugControl()
     * exists only on NT 5.2 SP1 and higher */
    Version = GetNtDdiVersion();
    if (skip(Version != 0, "GetNtDdiVersion() returned 0\n"))
        return;

    // IsWindowsVersionOrGreater(5, 2, 1);
    IsNT52SP1OrHigher = (Version >= NTDDI_WS03SP1);

    // IsWindowsVersionOrGreater(6, 0, 0);
    IsVistaOrHigher = (Version >= NTDDI_WIN6);


    /* Check whether the kernel debugger is present or not */
    Status = NtQuerySystemInformation(SystemKernelDebuggerInformation,
                                      &DebuggerInfo,
                                      sizeof(DebuggerInfo),
                                      NULL);

    IsDebuggerActive = NT_SUCCESS(Status) && !DebuggerInfo.KernelDebuggerNotPresent;
    // DebuggerInfo.KernelDebuggerEnabled; // SharedUserData->KdDebuggerEnabled;

    trace("Debugger is %s\n", IsDebuggerActive ? "active" : "inactive");

    /*
     * Explicitly disable the debug privilege so that we can test
     * that NtSystemDebugControl() fails when the privilege is absent.
     * Note that SysDbgGetTriageDump (29) is used for testing here,
     * because it doesn't require a debugger to be active in order
     * to proceed further (privilege check and actual functionality).
     */
    Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, FALSE, FALSE, &PrivilegeSet[0]);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = TestSystemDebugControl(SysDbgGetTriageDump /* 29 */);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);

    /* Now, enable the debug privilege for the rest of the tests */
    Status = RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &PrivilegeSet[1]);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Supported commands */
    for (Command = 0; Command <= 5; ++Command)
    {
        Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
        if (!IsVistaOrHigher || IsDebuggerActive)
            ok_neq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
        else
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }

    /* Test SysDbgBreakPoint (6) only when the debugger is inactive
     * or disabled; otherwise this call would trigger a breakpoint */
    if (!skip((IsVistaOrHigher && !IsDebuggerActive) || !SharedUserData->KdDebuggerEnabled,
        "NtSystemDebugControl(SysDbgBreakPoint) skipped because the debugger is active\n"))
    {
        Status = TestSystemDebugControl(SysDbgBreakPoint /* 6 */);
        if (!SharedUserData->KdDebuggerEnabled /*&& (!IsVistaOrHigher || IsDebuggerActive)*/)
        {
            ok_eq_hex_test(Command, Status, STATUS_UNSUCCESSFUL);
        }
        else
        {
            // ASSERT(IsVistaOrHigher && !IsDebuggerActive);
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
        }
    }

    /*
     * Commands handled by kernel-mode KdSystemDebugControl(),
     * and unsupported in user-mode:
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
    // TODO: Handle this differently if !IsNT52SP1OrHigher ?
    DBG_UNREFERENCED_PARAMETER(IsNT52SP1OrHigher);
    for (Command = 7; Command <= 20; ++Command)
    {
        Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
        if (!IsVistaOrHigher || IsDebuggerActive)
            ok_eq_hex_test(Command, Status, STATUS_NOT_IMPLEMENTED);
        else
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }


    /*
     * Separately test commands SysDbgEnableKernelDebugger (21)
     * and SysDbgDisableKernelDebugger (22), as they influence
     * the internal state of the debugger. The order of testing
     * matters, depending on whether the debugger was originally
     * enabled or disabled.
     */

    /* Save whether the debugger is currently enabled;
     * the next tests are going to change its state */
    WasDebuggerEnabled = SharedUserData->KdDebuggerEnabled;

//
// FIXME: Re-enable ONCE our KDBG and KDCOM dlls support disabling and re-enabling.
//
    DBG_UNREFERENCED_LOCAL_VARIABLE(WasDebuggerEnabled);
#if 0
    /* Try to disable or enable the debugger, depending on its original state */
    if (WasDebuggerEnabled)
        Command = SysDbgDisableKernelDebugger; // 22
    else
        Command = SysDbgEnableKernelDebugger;  // 21
    Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
    if (!IsVistaOrHigher || IsDebuggerActive)
    {
        /*
         * KdEnableDebugger() (with lock enabled) wants a KdDisableDebugger()
         * first (i.e. that the debugger was previously explicitly disabled)
         * in order to return success; otherwise it'll return STATUS_INVALID_PARAMETER.
         */
        if (Command == SysDbgEnableKernelDebugger)
        {
            ok(Status == STATUS_SUCCESS || Status == STATUS_INVALID_PARAMETER,
               "In test %lu: Status = 0x%08lx, expected 0x%08lx or 0x%08lx\n",
               Command, Status, STATUS_SUCCESS, STATUS_INVALID_PARAMETER);
        }
        else
        {
            ok_eq_hex_test(Command, Status, STATUS_SUCCESS);
        }
    }
    else
    {
        ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }

    /* Re-enable or disable the debugger, depending on its original state */
    if (WasDebuggerEnabled)
        Command = SysDbgEnableKernelDebugger;  // 21
    else
        Command = SysDbgDisableKernelDebugger; // 22
    Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
    if (!IsVistaOrHigher || IsDebuggerActive)
        ok_eq_hex_test(Command, Status, STATUS_SUCCESS);
    else
        ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
#endif
// END FIXME


    /* Supported commands */
    for (Command = 23; Command <= 31; ++Command)
    {
        Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
        if (!IsVistaOrHigher || IsDebuggerActive)
            ok_neq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
        else
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }

    /* These are Vista+ and depend on the OS version */
    for (Command = 32; Command <= 36; ++Command)
    {
        Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
        if (!IsVistaOrHigher || IsDebuggerActive)
        {
            if (Version >= NTDDI_WIN6) // IsVistaOrHigher
                ok_neq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
            else
                ok_eq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
        }
        else
        {
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
        }
    }

    Command = 37; // SysDbgGetLiveKernelDump
    Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
    if (!IsVistaOrHigher || IsDebuggerActive)
    {
        if (Version >= NTDDI_WINBLUE)
            ok_neq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
        else
            ok_eq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
    }
    else
    {
        ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }

    Command = 38; // SysDbgKdPullRemoteFile
    Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
    if (!IsVistaOrHigher || IsDebuggerActive)
    {
        if (Version >= NTDDI_WIN10_VB)
            ok_neq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
        else
            ok_eq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
    }
    else
    {
        ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }

    /* Unsupported commands */
    for (Command = 39; Command <= 40; ++Command)
    {
        Status = TestSystemDebugControl((SYSDBG_COMMAND)Command);
        if (!IsVistaOrHigher || IsDebuggerActive)
            ok_eq_hex_test(Command, Status, STATUS_INVALID_INFO_CLASS);
        else
            ok_eq_hex_test(Command, Status, STATUS_DEBUGGER_INACTIVE);
    }

    /* Finally restore the original debug privilege state */
    RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, PrivilegeSet[0], FALSE, &PrivilegeSet[0]);
}

/* EOF */
