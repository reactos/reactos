/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for NtQuerySystemInformationEx
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer (thomas.faber@reactos.org)
 */

#include "precomp.h"
#include <versionhelpers.h>

#undef KernelMode
#undef UserMode

/////////////////////////////////////////////
// FIXME: Move this to testutils lib
//#include <windows.h>
#include <intrin.h>

// CPUID Leaves
#define CPUID_EXTENDED_TOPOLOGY_V2 0x1F
#define CPUID_EXTENDED_TOPOLOGY    0x0B

// Topology level types (returned in ECX bits [15:8])
#define TOPOLOGY_LEVEL_INVALID     0
#define TOPOLOGY_LEVEL_SMT         1
#define TOPOLOGY_LEVEL_CORE        2

ULONG GetNumberOfCpuCores(void)
{
    int cpuInfo[4] = { 0 };
    ULONG leaf = 0;
    ULONG level = 0;
    ULONG threadsPerCore = 1; // Default fallback
    ULONG threadsPerSocket = 1;
    BOOL physicalCoreCountFound = FALSE;

    // 1. Check maximum supported standard CPUID leaf
    __cpuid(cpuInfo, 0);
    ULONG maxBasicLeaf = (ULONG)cpuInfo[0];

    // 2. Select the best available topology leaf (0x1F is preferred over 0x0B)
    if (maxBasicLeaf >= CPUID_EXTENDED_TOPOLOGY_V2)
    {
        leaf = CPUID_EXTENDED_TOPOLOGY_V2;
    }
    else if (maxBasicLeaf >= CPUID_EXTENDED_TOPOLOGY)
    {
        leaf = CPUID_EXTENDED_TOPOLOGY;
    }
    else
    {
        // Fallback for ancient CPUs that don't support topology leaves
        return 1;
    }

    // 3. Iterate through topology levels
    // Level types: 1 = SMT (Thread), 2 = Core.
    // Sub-leaf index is passed via ECX (the second argument of __cpuidex).
    for (level = 0; ; level++)
    {
        __cpuidex(cpuInfo, leaf, level);

        // If EBX is 0 or level type is INVALID, we reached the end of the topology hierarchy
        ULONG levelType = (cpuInfo[2] >> 8) & 0xFF;
        ULONG logicalProcessorsAtLevel = cpuInfo[1] & 0xFFFF;

        if (logicalProcessorsAtLevel == 0 || levelType == TOPOLOGY_LEVEL_INVALID)
        {
            break;
        }

        if (levelType == TOPOLOGY_LEVEL_SMT)
        {
            // Number of logical threads bundled into a single physical core
            threadsPerCore = logicalProcessorsAtLevel;
        }
        else if (levelType == TOPOLOGY_LEVEL_CORE)
        {
            // Total number of logical threads across the entire processor package/socket
            threadsPerSocket = logicalProcessorsAtLevel;
            physicalCoreCountFound = TRUE;
        }
    }

    // 4. Calculate physical cores based on parsing results
    if (physicalCoreCountFound && threadsPerCore > 0)
    {
        return threadsPerSocket / threadsPerCore;
    }

    // Secondary fallback: If the Core level wasn't explicitly enumerated in the loop,
    // then the total threads returned at the SMT level represents the whole package.
    return threadsPerSocket / threadsPerCore;
}

static BOOL IsUserAdmin(void)
{
    BOOL IsAdmin = FALSE;
    PSID AdministratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  &AdministratorsGroup))
    {
        return FALSE;
    }
    CheckTokenMembership(NULL, AdministratorsGroup, &IsAdmin);
    FreeSid(AdministratorsGroup);
    return IsAdmin;
}

/////////////////////////////////////////////

// For documentation see https://geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/sysinfo/queryex.htm
// And https://ntdoc.m417z.com/system_information_class

//
// TODO:
// - Validate alignment requirements for each info class
// - Check if the full provided size is probed (or only what's needed) for input/output/ReturnLength buffers
// - Way too large output buffer length
// - unaligned output buffer
//

typedef
NTSTATUS
NTAPI
FN_NtQuerySystemInformationEx(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength);

static FN_NtQuerySystemInformationEx* pNtQuerySystemInformationEx;

static ULONG g_NumberOfProcessorGroups;
static ULONG g_NumberOfProcessors;
static ULONG g_NumberOfProcessorsGroup0;
static ULONG g_NumberOfNumaNodes;
static ULONG64 g_CurrentSystemTime;
//static ULONG64 g_ProcessorFrequency;
static ULONG g_NumberOfCpuCores;
static BOOL g_IsWow64;

#define MINTIME 100000000 /* 10 seconds */
#define MAXTIME 315360000000000ull /* 365 days */
#define MINSYSTIME (g_CurrentSystemTime - 60 * 10000000ull) /* 1 minute ago */
#define MAXSYSTIME (g_CurrentSystemTime + 60 * 10000000ull) /* 1 minute from now */
#define MINCYCLETIME 1000000 /* 1 second */
#define MAXCTXSWITCHES (g_CurrentSystemTime / 10000000ull) /* 10 context switches per second per processor */


static void Test_AllInfoClasses(void)
{
    NTSTATUS Status, ExpectedStatus;
    ULONG ReturnLength, ExpectedReturnLength;
    USHORT GroupNumber = 0;
    ULONG64 Buffer[4] = { 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull };
    SYSTEM_INFORMATION_CLASS InfoClass;

    // Test all info classes
    for (InfoClass = (SYSTEM_INFORMATION_CLASS)0; InfoClass < (SYSTEM_INFORMATION_CLASS)185; InfoClass++)
    {
        UCHAR InputBuffer[1024] = { 0 };
        ULONG InputBufferSize = sizeof(USHORT);

        ExpectedStatus = STATUS_INVALID_INFO_CLASS;
        ExpectedReturnLength = 0xdeadbeef;
        switch (InfoClass)
        {
            case SystemProcessorPerformanceInformation:
            case SystemInterruptInformation:
            case SystemProcessorIdleInformation:
            case SystemProcessorPowerInformation:
            case SystemLogicalProcessorInformation:
            case SystemProcessorPerformanceDistribution:
                ExpectedStatus = STATUS_INFO_LENGTH_MISMATCH;
                break;
            case SystemNodeDistanceInformation:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN7)
                {
                    ExpectedStatus = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            case SystemProcessorPerformanceInformationEx:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN8)
                {
                    ExpectedStatus = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            case SystemFeatureConfigurationInformation:
            case SystemFeatureConfigurationSectionInformation:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN10)
                {
                    ExpectedStatus = STATUS_INFO_LENGTH_MISMATCH;
                    ExpectedReturnLength = 0;
                }
                break;

            case SystemProcessorIdleCycleTimeInformation:
                ExpectedStatus = STATUS_BUFFER_TOO_SMALL;
                break;
            case SystemProcessorCycleTimeInformation:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN7)
                {
                    ExpectedStatus = STATUS_BUFFER_TOO_SMALL;
                }
                break;
            case SystemProcessorCycleStatsInformation:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN10)
                {
                    ExpectedStatus = STATUS_BUFFER_TOO_SMALL;
                }
                break;

            case SystemWatchdogTimerInformation:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN10)
                {
                    ExpectedStatus = STATUS_INVALID_PARAMETER;
                }
                break;

            case SystemLogicalProcessorAndGroupInformation:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN7)
                {
                    ExpectedStatus = STATUS_INVALID_PARAMETER;
                }
                break;

            case SystemIsolatedUserModeInformation:
            case SystemCpuSetInformation:
            case SystemSecureKernelProfileInformation:
            case SystemSupportedProcessorArchitectures:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN10)
                {
                    ExpectedStatus = STATUS_INVALID_PARAMETER;
                }
                break;

            case SystemInterruptSteeringInformation:
            case SystemCodeIntegritySyntheticCacheInformation:
                if (is_reactos() || GetNTDDIVersion() >= NTDDI_WIN10)
                {
                    ExpectedStatus = STATUS_ACCESS_DENIED;
                    ExpectedReturnLength = 0;
                }
                break;

            default:
                break;
        }

        ReturnLength = 0xdeadbeef;
        Status = pNtQuerySystemInformationEx(InfoClass,
                                             InputBuffer,
                                             InputBufferSize,
                                             NULL,
                                             0,
                                             &ReturnLength);

        if (is_reactos() && (Status == STATUS_NOT_IMPLEMENTED))
        {
            continue;
        }

        ok(Status == ExpectedStatus, "[%u] Unexpected Status 0x%lx, expected 0x%lx\n", InfoClass, Status, ExpectedStatus);

        if ((ExpectedStatus == STATUS_INVALID_INFO_CLASS) || (ExpectedStatus == STATUS_INVALID_PARAMETER))
        {
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);
        }
        else if (ExpectedReturnLength == 0)
        {
            ok(ReturnLength == 0, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);
        }
        else
        {
            ok((ReturnLength > 0) && (ReturnLength < 64 * 1024), "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);
        }

        if (ExpectedStatus == STATUS_INVALID_INFO_CLASS)
        {
            // Null InputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 &GroupNumber,
                                                 0,
                                                 NULL,
                                                 0,
                                                 &ReturnLength);
            ok(Status == g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_INVALID_PARAMETER, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Too small input buffer
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 InputBuffer,
                                                 1,
                                                 NULL,
                                                 0,
                                                 &ReturnLength);
            ok(Status == STATUS_INVALID_INFO_CLASS, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Null InputBuffer with null InputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 0,
                                                 &ReturnLength);
            ok(Status == g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_INVALID_PARAMETER, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Null InputBuffer with non-null InputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 NULL,
                                                 sizeof(GroupNumber),
                                                 NULL,
                                                 0,
                                                 &ReturnLength);
            ok(Status == g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_INVALID_PARAMETER, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Unaligned InputBuffer with null InputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 (PUCHAR)Buffer + 1,
                                                 0,
                                                 NULL,
                                                 0,
                                                 &ReturnLength);
            ok(Status == g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_INVALID_PARAMETER, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Unaligned InputBuffer with non-null InputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 (PUCHAR)Buffer + 1,
                                                 sizeof(GroupNumber),
                                                 NULL,
                                                 0,
                                                 &ReturnLength);
            ok(Status == STATUS_INVALID_INFO_CLASS, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Invalid InputBuffer with null InputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                                 0,
                                                 NULL,
                                                 0,
                                                 &ReturnLength);
            ok(Status == g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_INVALID_PARAMETER, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Invalid InputBuffer with non-null InputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                                 sizeof(GroupNumber),
                                                 NULL,
                                                 0,
                                                 &ReturnLength);
            ok(Status == STATUS_INVALID_INFO_CLASS, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // NULL OutputBuffer with non-null OutputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 &GroupNumber,
                                                 sizeof(GroupNumber),
                                                 NULL,
                                                 PAGE_SIZE,
                                                 &ReturnLength);
            ok(Status == STATUS_INVALID_INFO_CLASS, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Invalid OutputBuffer with null OutputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 &GroupNumber,
                                                 sizeof(GroupNumber),
                                                 (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                                 0,
                                                 &ReturnLength);
            ok(Status == STATUS_INVALID_INFO_CLASS, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Invalid OutputBuffer with non-null OutputBufferLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 &GroupNumber,
                                                 sizeof(GroupNumber),
                                                 (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                                 PAGE_SIZE,
                                                 &ReturnLength);
            ok(Status == STATUS_INVALID_INFO_CLASS, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength 0x%lx\n", InfoClass, ReturnLength);

            // Null RetunLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 &GroupNumber,
                                                 sizeof(GroupNumber),
                                                 NULL,
                                                 0,
                                                 NULL);
            ok(Status == STATUS_INVALID_INFO_CLASS, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);

            // Invalid RetunLength
            ReturnLength = 0xdeadbeef;
            Status = pNtQuerySystemInformationEx(InfoClass,
                                                 &GroupNumber,
                                                 sizeof(GroupNumber),
                                                 NULL,
                                                 0,
                                                 (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull);
            ok(Status == STATUS_INVALID_INFO_CLASS, "[%u] Unexpected Status 0x%lx\n", InfoClass, Status);
        }
    }
}

static void Test_Common(SYSTEM_INFORMATION_CLASS InfoClass, ULONG InputBufferLength)
{
    NTSTATUS Status;
    ULONG ReturnLength;
    ULONG64 Input = 0;
    ULONG64 Buffer[4] = { 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull };

    // Null InputBuffer with null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         &Input,
                                         0,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Too small input buffer
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         &Input,
                                         InputBufferLength - 1,
                                         NULL,
                                         0,
                                         &ReturnLength);
    if (InfoClass == SystemCodeIntegritySyntheticCacheInformation)
    {
        ok(Status == STATUS_ACCESS_DENIED, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
        ok(ReturnLength == 0 || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);
    }
    else
    {
        ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
        ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);
    }

    // Null InputBuffer with non-null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         NULL,
                                         InputBufferLength,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Misaligned InputBuffer with null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         (PUCHAR)Buffer + 1,
                                         0,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Misaligned InputBuffer with non-null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         (PUCHAR)Buffer + 1,
                                         InputBufferLength,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Invalid InputBuffer with null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                         0,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Invalid InputBuffer with non-null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                         InputBufferLength,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == STATUS_ACCESS_VIOLATION, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Invalid OutputBuffer with non-null OutputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         &Buffer,
                                         InputBufferLength,
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                         PAGE_SIZE,
                                         &ReturnLength);
    ok(Status == STATUS_ACCESS_VIOLATION || broken(g_IsWow64), "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Invalid OutputBuffer with non-null OutputBufferLength, misaligned InputBuffer and non-null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         (PUCHAR)Buffer + 1,
                                         InputBufferLength,
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                         PAGE_SIZE,
                                         &ReturnLength);
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef || broken(g_IsWow64), "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // too large OutputBuffer with non-null OutputBufferLength
    PVOID Buffer2 = malloc(PAGE_SIZE);
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         &Buffer2,
                                         InputBufferLength,
                                         Buffer,
                                         0xFFFFFF00,
                                         &ReturnLength);
    ok(Status == STATUS_ACCESS_VIOLATION || broken(g_IsWow64), "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);
    free(Buffer2);

    // Invalid ReturnLength with misaligned InputBuffer and non-null InputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         (PUCHAR)Buffer + 1,
                                         InputBufferLength,
                                         NULL,
                                         0,
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull);
    ok(Status == STATUS_DATATYPE_MISALIGNMENT, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

}

static void Test_Generic_Input(
    SYSTEM_INFORMATION_CLASS InfoClass,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    BOOL TooLargeInputAllowed,
    ULONG ExpectedInfoLength,
    NTSTATUS BufferSizeStatus)
{
    NTSTATUS Status;
    ULONG ReturnLength;

    Test_Common(InfoClass, InputBufferLength);

    // Too large input buffer
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         InputBuffer,
                                         InputBufferLength + 1,
                                         NULL,
                                         0,
                                         &ReturnLength);
    if (TooLargeInputAllowed)
    {
        ok(Status == BufferSizeStatus, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
        ok(ReturnLength == ExpectedInfoLength,
           "[%u] Unexpected ReturnLength: got 0x%lx, expected 0x%lx\n",
           InfoClass, ReturnLength, ExpectedInfoLength);
    }
    else
    {
        ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
        ok(ReturnLength == 0xdeadbeef,
           "[%u] Unexpected ReturnLength: got 0x%lx, expected 0xdeadbeef\n",
           InfoClass, ReturnLength);
    }

    // Invalid OutputBuffer with null OutputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         InputBuffer,
                                         InputBufferLength,
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                         0,
                                         &ReturnLength);
    ok(Status == BufferSizeStatus, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == ExpectedInfoLength, "[%u] Unexpected ReturnLength: got 0x%lx, expected 0x%lx\n",
       InfoClass, ReturnLength, ExpectedInfoLength);

    // Null RetunLength with NULL output buffer
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         InputBuffer,
                                         InputBufferLength,
                                         NULL,
                                         0,
                                         NULL);
    ok(Status == BufferSizeStatus, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);

    // Invalid RetunLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         InputBuffer,
                                         InputBufferLength,
                                         NULL,
                                         0,
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull);
    ok(Status == STATUS_ACCESS_VIOLATION, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);

    // Unaligned RetunLength
    ULONG Buffer[4];
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         InputBuffer,
                                         InputBufferLength,
                                         NULL,
                                         0,
                                         (PULONG)((PUCHAR)Buffer + 1));
    ok(Status == BufferSizeStatus, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(*(PULONG)((PUCHAR)Buffer + 1) == ExpectedInfoLength,
       "[%u] Unexpected ReturnLength: 0x%lx\n",
       InfoClass, *(PULONG)((PUCHAR)Buffer + 1));

    // NULL OutputBuffer with 0 OutputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         InputBuffer,
                                         InputBufferLength,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == BufferSizeStatus, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == ExpectedInfoLength, "[%u] Unexpected ReturnLength: got 0x%lx, expected 0x%lx\n",
       InfoClass, ReturnLength, ExpectedInfoLength);

    if (ReturnLength == 0)
    {
        return;
    }

    // Allocate a buffer
    PVOID InfoBuffer = malloc(ReturnLength);
    ok(InfoBuffer != NULL, "\n");
    if (InfoBuffer == NULL)
    {
        skip("[%u] Failed to allocate buffer of 0x%lx bytes. Skipping rest of test.\n", InfoClass, ReturnLength);
        return;
    }

    memset(InfoBuffer, 0xAB, ReturnLength);

    // Too small output buffer
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         InputBuffer,
                                         InputBufferLength,
                                         InfoBuffer,
                                         ReturnLength - 1,
                                         &ReturnLength);
    if ((InfoClass == SystemSupportedProcessorArchitectures) ||
        (InfoClass == SystemCpuSetInformation))
    {
        ok(Status == STATUS_BUFFER_TOO_SMALL, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    }
    else
    {
        ok(Status == STATUS_INFO_LENGTH_MISMATCH, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    }
    ok(ReturnLength == ExpectedInfoLength, "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Check if the buffer is untouched (except for some info classes, that return data if there is enough space)
    if (((InfoClass == SystemLogicalProcessorInformation) && (ReturnLength > sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION))) ||
        ((InfoClass == SystemProcessorIdleCycleTimeInformation) && (ReturnLength > sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION))) ||
        ((InfoClass == SystemLogicalProcessorAndGroupInformation) && (ReturnLength > sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX))) ||
        ((InfoClass == SystemProcessorCycleTimeInformation) && (ReturnLength > sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION))) ||
        ((InfoClass == SystemProcessorCycleStatsInformation) && (ReturnLength > sizeof(SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION))) ||
        ((InfoClass == SystemNodeDistanceInformation) && (ReturnLength > sizeof(USHORT))) ||
        ((InfoClass == SystemFeatureConfigurationInformation) && (ReturnLength > sizeof(SYSTEM_FEATURE_CONFIGURATION_INFORMATION))) ||
         (InfoClass == SystemFeatureConfigurationSectionInformation))
    {
        ok((*(PULONG64)InfoBuffer) != 0xABABABABABABABABull, "[%u] Buffer was not changed: 0x%llx\n", InfoClass, *(PULONG64)InfoBuffer);
    }
    else
    {
        ok((*(PULONG64)InfoBuffer) == 0xABABABABABABABABull, "[%u] Buffer was changed: 0x%llx\n", InfoClass, *(PULONG64)InfoBuffer);
    }

    // Too huge output buffer
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         InputBuffer,
                                         InputBufferLength,
                                         InfoBuffer,
                                         MAXULONG,
                                         &ReturnLength);
    ok(Status == STATUS_ACCESS_VIOLATION, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == ExpectedInfoLength, "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Check if the buffer is untouched (except for some info classes, that return data if there is enough space)
    if (((InfoClass == SystemLogicalProcessorInformation) && (ReturnLength > sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION))) ||
        ((InfoClass == SystemProcessorIdleCycleTimeInformation) && (ReturnLength > sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION))) ||
        ((InfoClass == SystemLogicalProcessorAndGroupInformation) && (ReturnLength > sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX))) ||
        ((InfoClass == SystemProcessorCycleTimeInformation) && (ReturnLength > sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION))) ||
        ((InfoClass == SystemProcessorCycleStatsInformation) && (ReturnLength > sizeof(SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION))) ||
        ((InfoClass == SystemNodeDistanceInformation) && (ReturnLength > sizeof(USHORT))) ||
        ((InfoClass == SystemFeatureConfigurationInformation) && (ReturnLength > sizeof(SYSTEM_FEATURE_CONFIGURATION_INFORMATION))) ||
         (InfoClass == SystemFeatureConfigurationSectionInformation))
    {
        ok((*(PULONG64)InfoBuffer) != 0xABABABABABABABABull, "[%u] Buffer was not changed: 0x%llx\n", InfoClass, *(PULONG64)InfoBuffer);
    }
    else
    {
        ok((*(PULONG64)InfoBuffer) == 0xABABABABABABABABull, "[%u] Buffer was changed: 0x%llx\n", InfoClass, *(PULONG64)InfoBuffer);
    }

    free(InfoBuffer);
}

static void Test_GroupNumber_Input(SYSTEM_INFORMATION_CLASS InfoClass, ULONG ExpectedInfoLength, NTSTATUS BufferSizeStatus)
{
    NTSTATUS Status;
    ULONG ReturnLength;
    USHORT GroupNumber = 0;

    Test_Generic_Input(InfoClass, &GroupNumber, sizeof(GroupNumber), TRUE, ExpectedInfoLength, BufferSizeStatus);

    // Invalid group number
    GroupNumber = 0xFFFF;
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);

    // Invalid group number and too small buffer
    ULONG64 Buffer[4] = { 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull, 0xCCCCCCCCCCCCCCCCull };
    GroupNumber = 0xFFFF;
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(InfoClass,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         Buffer,
                                         2,
                                         &ReturnLength);
    ok(Status == STATUS_INVALID_PARAMETER, "[%u] Unexpected Status: 0x%lx\n", InfoClass, Status);
    ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength: 0x%lx\n", InfoClass, ReturnLength);
}

static void Test_SystemProcessorPerformanceInformation(void) // 8
{
    NTSTATUS Status;
    ULONG ReturnLength;
    USHORT GroupNumber = 0;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PerfInfo;

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[]
    ULONG ExpectedInfoLength = g_NumberOfProcessorsGroup0 * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
    Test_GroupNumber_Input(SystemProcessorPerformanceInformation,
                           ExpectedInfoLength,
                           STATUS_INFO_LENGTH_MISMATCH);

    // Allocate a buffer
    PerfInfo = malloc(ExpectedInfoLength);
    ok(PerfInfo != NULL, "\n");
    if (PerfInfo == NULL)
    {
        return;
    }

    memset(PerfInfo, 0xAB, ExpectedInfoLength);

    // Proper call with output buffer
    Status = pNtQuerySystemInformationEx(SystemProcessorPerformanceInformation,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         PerfInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ULONG64 TotalTime = PerfInfo[0].KernelTime.QuadPart + PerfInfo[0].UserTime.QuadPart;

    ULONG NumberOfEntries = ReturnLength / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        /* Check if the values are somewhat reasonable */
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION Info = PerfInfo[i];
        ok((Info.IdleTime.QuadPart > 0) && (Info.IdleTime.QuadPart < MAXSYSTIME), "[%lu] Unexpected IdleTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info.IdleTime.QuadPart, MAXSYSTIME);
        ok((Info.KernelTime.QuadPart > 0) && (Info.KernelTime.QuadPart < MAXSYSTIME), "[%lu] Unexpected KernelTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info.KernelTime.QuadPart, MAXSYSTIME);
        ok((Info.UserTime.QuadPart > 0) && (Info.UserTime.QuadPart < MAXSYSTIME), "[%lu] Unexpected UserTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info.UserTime.QuadPart, MAXSYSTIME);
        ok((Info.DpcTime.QuadPart > 0) && (Info.DpcTime.QuadPart < MAXSYSTIME / 100), "[%lu] Unexpected DpcTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info.DpcTime.QuadPart, MAXSYSTIME / 100);
        ok((Info.InterruptTime.QuadPart > 0) && (Info.InterruptTime.QuadPart < MAXSYSTIME / 100), "[%lu] Unexpected InterruptTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info.InterruptTime.QuadPart, MAXSYSTIME / 100);
        ok(Info.InterruptCount > 10, "[%lu] Unexpected InterruptCount: 0x%lx\n", i, Info.InterruptCount);
        ok(*(PULONG)(&Info.InterruptCount + 1) == 0xABABABAB, "[%lu] Unexpected data at end of buffer: 0x%lx\n", i, *(PULONG)(&Info.InterruptCount + 1));
        ULONG64 Sum = Info.KernelTime.QuadPart + Info.UserTime.QuadPart;
        ULONG64 DiffTime = _abs64(Sum - TotalTime);
        ok(DiffTime < 2000000 /* 200 ms */, "[%lu] DiffTime too large: %llu\n", i, DiffTime);
    }

    free(PerfInfo);

    // Test ALL_PROCESSOR_GROUPS
    GroupNumber = ALL_PROCESSOR_GROUPS;
    Status = pNtQuerySystemInformationEx(SystemProcessorPerformanceInformation,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
}

static void Test_SystemInterruptInformation(void) // 23
{
    NTSTATUS Status;
    ULONG ReturnLength;
    USHORT GroupNumber = 0;
    PSYSTEM_INTERRUPT_INFORMATION IntInfo;

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_INTERRUPT_INFORMATION[]
    ULONG ExpectedInfoLength = g_NumberOfProcessorsGroup0 * sizeof(SYSTEM_INTERRUPT_INFORMATION);
    Test_GroupNumber_Input(SystemInterruptInformation,
                           ExpectedInfoLength,
                           STATUS_INFO_LENGTH_MISMATCH);

    // Allocate a buffer
    IntInfo = malloc(ExpectedInfoLength);
    ok(IntInfo != NULL, "\n");
    if (IntInfo == NULL)
    {
        return;
    }

    memset(IntInfo, 0xAB, ExpectedInfoLength);

    // Proper call with output buffer
    Status = pNtQuerySystemInformationEx(SystemInterruptInformation,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         IntInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ULONG NumberOfEntries = ReturnLength / sizeof(SYSTEM_INTERRUPT_INFORMATION);
    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        PSYSTEM_INTERRUPT_INFORMATION Info = &IntInfo[i];
        ok((Info->ContextSwitches > 10) && (Info->ContextSwitches < 0xF0000000), "[%lu] Unexpected ContextSwitches: 0x%lx\n", i, Info->ContextSwitches);
        ok((Info->DpcCount > 10) && (Info->DpcCount < 0xF0000000), "[%lu] Unexpected DpcCount: 0x%lx\n", i, Info->DpcCount);
        ok(Info->DpcRate < 1000, "[%lu] Unexpected DpcRate: 0x%lx\n", i, Info->DpcRate);
        ok((Info->TimeIncrement > 0) && (Info->TimeIncrement < 0xF0000000), "[%lu] Unexpected TimeIncrement: 0x%lx\n", i, Info->TimeIncrement);
        ok(Info->DpcBypassCount < 0xF00, "[%lu] Unexpected DpcBypassCount: 0x%lx\n", i, Info->DpcBypassCount);
        ok(Info->ApcBypassCount < 0xF00, "[%lu] Unexpected ApcBypassCount: 0x%lx\n", i, Info->ApcBypassCount);
    }

    free(IntInfo);
}

static void Test_SystemProcessorIdleInformation(void) // 42
{
    NTSTATUS Status;
    ULONG ReturnLength;
    USHORT GroupNumber = 0;
    PSYSTEM_PROCESSOR_IDLE_INFORMATION IdleInfo;

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_IDLE_INFORMATION[]
    ULONG ExpectedInfoLength = g_NumberOfProcessorsGroup0 * sizeof(SYSTEM_PROCESSOR_IDLE_INFORMATION);
    Test_GroupNumber_Input(SystemProcessorIdleInformation,
                           ExpectedInfoLength,
                           STATUS_INFO_LENGTH_MISMATCH);

    // Allocate a buffer
    IdleInfo = malloc(ExpectedInfoLength);
    ok(IdleInfo != NULL, "\n");
    if (IdleInfo == NULL)
    {
        return;
    }

    memset(IdleInfo, 0xAB, ExpectedInfoLength);

    // Proper call with output buffer
    Status = pNtQuerySystemInformationEx(SystemProcessorIdleInformation,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         IdleInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ULONG NumberOfEntries = ReturnLength / sizeof(SYSTEM_PROCESSOR_IDLE_INFORMATION);
    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        PSYSTEM_PROCESSOR_IDLE_INFORMATION Info = &IdleInfo[i];
        ok((Info->IdleTime > MINTIME) && (Info->IdleTime < MAXSYSTIME), "[%lu] Unexpected IdleTime: 0x%llx\n", i, Info->IdleTime);
        ok((Info->C1Time >= 0) && (Info->C1Time < MAXSYSTIME), "[%lu] Unexpected C1Time: 0x%llx\n", i, Info->C1Time);
        ok((Info->C2Time >= 0) && (Info->C2Time < MAXSYSTIME), "[%lu] Unexpected C2Time: 0x%llx\n", i, Info->C2Time);
        ok((Info->C3Time >= 0) && (Info->C3Time < MAXSYSTIME), "[%lu] Unexpected C3Time: 0x%llx\n", i, Info->C3Time);
        ok((Info->C1Transitions >= 0) && (Info->C1Transitions < MAXCTXSWITCHES), "[%lu] Unexpected C1Transitions: 0x%lx\n", i, Info->C1Transitions);
        ok((Info->C2Transitions >= 0) && (Info->C2Transitions < MAXCTXSWITCHES), "[%lu] Unexpected C2Transitions: 0x%lx\n", i, Info->C2Transitions);
        ok((Info->C3Transitions >= 0) && (Info->C3Transitions < MAXCTXSWITCHES), "[%lu] Unexpected C3Transitions: 0x%lx\n", i, Info->C3Transitions);
        ok_eq_ulong(Info->Padding, 0);
    }

    free(IdleInfo);
}

static void Test_SystemProcessorPowerInformation(void) // 61
{
    NTSTATUS Status;
    ULONG ReturnLength;
    USHORT GroupNumber = 0;
    PSYSTEM_PROCESSOR_POWER_INFORMATION PowerInfo;

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_POWER_INFORMATION[]
    ULONG ExpectedInfoLength = g_NumberOfProcessorsGroup0 * sizeof(SYSTEM_PROCESSOR_POWER_INFORMATION);
    Test_GroupNumber_Input(SystemProcessorPowerInformation,
                           ExpectedInfoLength,
                           STATUS_INFO_LENGTH_MISMATCH);

    // Allocate a buffer
    PowerInfo = malloc(ExpectedInfoLength);
    ok(PowerInfo != NULL, "\n");
    if (PowerInfo == NULL)
    {
        return;
    }

    memset(PowerInfo, 0xAB, ExpectedInfoLength);

    // Proper call with output buffer
    Status = pNtQuerySystemInformationEx(SystemProcessorPowerInformation,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         PowerInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ULONG NumberOfEntries = ReturnLength / sizeof(SYSTEM_PROCESSOR_POWER_INFORMATION);
    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        PSYSTEM_PROCESSOR_POWER_INFORMATION Info = &PowerInfo[i];
        /* These are percentages */
        ok((Info->CurrentFrequency > 0) && (Info->CurrentFrequency <= 100), "[%lu] Unexpected CurrentFrequency: 0x%x\n", i, Info->CurrentFrequency);
        ok((Info->ThermalLimitFrequency >= 0) && (Info->ThermalLimitFrequency <= 100), "[%lu] Unexpected ThermalLimitFrequency: 0x%x\n", i, Info->ThermalLimitFrequency);
        ok((Info->ConstantThrottleFrequency >= 0) && (Info->ConstantThrottleFrequency <= 100), "[%lu] Unexpected ConstantThrottleFrequency: 0x%x\n", i, Info->ConstantThrottleFrequency);
        ok((Info->DegradedThrottleFrequency >= 0) && (Info->DegradedThrottleFrequency <= 100), "[%lu] Unexpected DegradedThrottleFrequency: 0x%x\n", i, Info->DegradedThrottleFrequency);
        ok((Info->LastBusyFrequency >= 0) && (Info->LastBusyFrequency <= 100), "[%lu] Unexpected LastBusyFrequency: 0x%x\n", i, Info->LastBusyFrequency);
        ok((Info->LastC3Frequency >= 0) && (Info->LastC3Frequency <= 100), "[%lu] Unexpected LastC3Frequency: 0x%x\n", i, Info->LastC3Frequency);
        ok((Info->LastAdjustedBusyFrequency >= 0) && (Info->LastAdjustedBusyFrequency <= 100), "[%lu] Unexpected LastAdjustedBusyFrequency: 0x%x\n", i, Info->LastAdjustedBusyFrequency);
        ok((Info->ProcessorMinThrottle >= 0) && (Info->ProcessorMinThrottle <= 100), "[%lu] Unexpected ProcessorMinThrottle: 0x%x\n", i, Info->ProcessorMinThrottle);
        ok((Info->ProcessorMaxThrottle >= 0) && (Info->ProcessorMaxThrottle <= 100), "[%lu] Unexpected ProcessorMaxThrottle: 0x%x\n", i, Info->ProcessorMaxThrottle);

        ok((Info->NumberOfFrequencies >= 0) && (Info->NumberOfFrequencies <= 100), "[%lu] Unexpected NumberOfFrequencies: 0x%lx\n", i, Info->NumberOfFrequencies);
        //ok((Info->PromotionCount >= 0) && (Info->PromotionCount <= 1000), "[%lu] Unexpected PromotionCount: 0x%lx\n", i, Info->PromotionCount);
        ok((Info->DemotionCount >= 0) && (Info->DemotionCount <= 1000), "[%lu] Unexpected DemotionCount: 0x%lx\n", i, Info->DemotionCount);
        //ok((Info->ErrorCount >= 0) && (Info->ErrorCount <= 100), "[%lu] Unexpected ErrorCount: 0x%lx\n", i, Info->ErrorCount);
        //ok((Info->RetryCount >= 0) && (Info->RetryCount <= 100), "[%lu] Unexpected RetryCount: 0x%lx\n", i, Info->RetryCount);
        //ok((Info->CurrentFrequencyTime >= 0) && (Info->CurrentFrequencyTime <= 100), "[%lu] Unexpected CurrentFrequencyTime: 0x%llx\n", i, Info->CurrentFrequencyTime);
        //ok((Info->CurrentProcessorTime >= MINTIME) && (Info->CurrentProcessorTime <= MAXSYSTIME), "[%lu] Unexpected CurrentProcessorTime: 0x%llx\n", i, Info->CurrentProcessorTime);
        //ok((Info->CurrentProcessorIdleTime >= MINTIME) && (Info->CurrentProcessorIdleTime <= MAXSYSTIME), "[%lu] Unexpected CurrentProcessorIdleTime: 0x%llx\n", i, Info->CurrentProcessorIdleTime);
        //ok((Info->LastProcessorTime >= 0) && (Info->LastProcessorTime <= MAXSYSTIME), "[%lu] Unexpected LastProcessorTime: 0x%llx\n", i, Info->LastProcessorTime);
        ok((Info->LastProcessorIdleTime >= 0) && (Info->LastProcessorIdleTime <= MAXSYSTIME), "[%lu] Unexpected LastProcessorIdleTime: 0x%llx\n", i, Info->LastProcessorIdleTime);
        ok((Info->Energy >= 0) /* && (Info->Energy <= 0x200000000000ull) */, "[%lu] Unexpected Energy: 0x%llx\n", i,
           Info->Energy);
    }

    free(PowerInfo);
}

static void Test_SystemWatchdogTimerInformation(void) // 72
{
    NTSTATUS Status;
    ULONG ReturnLength;
    WATCHDOG_INFORMATION_CLASS WdInfoClass = WdInfoNop;
    SYSTEM_WATCHDOG_TIMER_INFORMATION WdInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemWatchdogTimerInformation is not supported on this OS version\n");
        return;
    }

    // In: ULONG WATCHDOG_INFORMATION_CLASS
    // Out: SYSTEM_WATCHDOG_TIMER_INFORMATION (Must be provided, otherwise the function fails with STATUS_INVALID_PARAMETER)
    Test_Common(SystemWatchdogTimerInformation, sizeof(WATCHDOG_INFORMATION_CLASS));

    /* test all watchdog information classes */
    for (WdInfoClass = -1; WdInfoClass <= WdInfoInvalid + 1; WdInfoClass++)
    {
        ReturnLength = 0xdeadbeef;
        WdInfo.WdInfoClass = 0xABABABAB;
        WdInfo.DataValue = 0xABABABAB;
        Status = pNtQuerySystemInformationEx(SystemWatchdogTimerInformation,
                                             &WdInfoClass,
                                             sizeof(WdInfoClass), // 1: STATUS_INVALID_PARAMETER
                                             &WdInfo,
                                             sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION),
                                             &ReturnLength);
        if (WdInfoClass == WdInfoNop)
        {
            ok_ntstatus(Status, STATUS_SUCCESS);
            ok_eq_hex(ReturnLength, sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION));
            ok_eq_int(WdInfo.WdInfoClass, WdInfoNop);
            ok_eq_hex(WdInfo.DataValue, 0);
        }
        else if (WdInfoClass == WdInfoGeneratedLastReset)
        {
            ok_ntstatus(Status, STATUS_SUCCESS);
            ok_eq_hex(ReturnLength, sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION));
            ok_eq_int(WdInfo.WdInfoClass, WdInfoGeneratedLastReset);
            ok_eq_hex(WdInfo.DataValue, 0);
        }
        else
        {
            ok(Status == STATUS_NOT_SUPPORTED, "[%u] Unexpected Status: 0x%lx\n", WdInfoClass, Status);
            ok(ReturnLength == 0xdeadbeef, "[%u] Unexpected ReturnLength: 0x%lx\n", WdInfoClass, ReturnLength);
            ok(WdInfo.WdInfoClass == 0xABABABAB, "[%u] Unexpected WdInfoClass: 0x%lx\n", WdInfoClass, WdInfo.WdInfoClass);
            ok(WdInfo.DataValue == 0xABABABAB, "[%u] Unexpected DataValue: 0x%llx\n", WdInfoClass, WdInfo.DataValue);
        }
    }

    ReturnLength = 0xdeadbeef;
    WdInfoClass = WdInfoNop;
    Status = pNtQuerySystemInformationEx(SystemWatchdogTimerInformation,
                                         &WdInfoClass,
                                         sizeof(WdInfoClass),
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok_ntstatus(Status, g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_INVALID_PARAMETER);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    Status = pNtQuerySystemInformationEx(SystemWatchdogTimerInformation,
                                         &WdInfoClass,
                                         sizeof(WdInfoClass),
                                         NULL,
                                         sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION),
                                         &ReturnLength);
    ok_ntstatus(Status, g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_ACCESS_VIOLATION);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    Status = pNtQuerySystemInformationEx(SystemWatchdogTimerInformation,
                                         &WdInfoClass,
                                         sizeof(WdInfoClass),
                                         &WdInfo,
                                         sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION) - 1,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    Status = pNtQuerySystemInformationEx(SystemWatchdogTimerInformation,
                                         &WdInfoClass,
                                         sizeof(WdInfoClass),
                                         &WdInfo,
                                         sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION) + 1,
                                         &ReturnLength);
    ok_ntstatus(Status, g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_INVALID_PARAMETER);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    Status = pNtQuerySystemInformationEx(SystemWatchdogTimerInformation,
                                         &WdInfoClass,
                                         sizeof(WdInfoClass),
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                         sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION),
                                         &ReturnLength);
    ok_ntstatus(Status, g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_ACCESS_VIOLATION);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    WdInfoClass = -1;
    Status = pNtQuerySystemInformationEx(SystemWatchdogTimerInformation,
                                         &WdInfoClass,
                                         sizeof(WdInfoClass),
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                         sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION),
                                         &ReturnLength);
    ok_ntstatus(Status, g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_ACCESS_VIOLATION);
    ok_eq_hex(ReturnLength, 0xdeadbeef);
}

static void Test_SystemLogicalProcessorInformation(void) // 73
{
    NTSTATUS Status;
    ULONG ExpectedInfoLength;
    ULONG ReturnLength;
    USHORT RelationshipType;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION LogProcInfo;

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION

    //
    // The size of the info depends on the machine's hardware (number of cores,
    // threads and cache levels). So we determine it first, so we can call
    // Test_Generic_Input.
    //
    RelationshipType = RelationProcessorCore;
    Status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformation,
                                         &RelationshipType,
                                         sizeof(RelationshipType),
                                         NULL,
                                         0,
                                         &ExpectedInfoLength);
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);
    ok(ExpectedInfoLength % sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) == 0,
       "Unexpected info length: %lu\n", ExpectedInfoLength);

    // In: USHORT (LOGICAL_PROCESSOR_RELATIONSHIP) RelationshipType
    // Out: SYSTEM_LOGICAL_PROCESSOR_INFORMATION[]
    Test_Generic_Input(SystemLogicalProcessorInformation,
                       &RelationshipType,
                       sizeof(RelationshipType),
                       TRUE,
                       ExpectedInfoLength,
                       STATUS_INFO_LENGTH_MISMATCH);

    /* Test all relationship types */
    for (INT i = -1; i <= RelationProcessorModule + 1; i++)
    {
        RelationshipType = i;
        ReturnLength = 0xdeadbeef;
        Status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformation,
                                             &RelationshipType,
                                             sizeof(RelationshipType),
                                             NULL,
                                             0,
                                             &ReturnLength);
        if (RelationshipType == RelationProcessorCore)
        {
            ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);
            ok_eq_hex(ReturnLength, ExpectedInfoLength);
        }
        else
        {
            ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
            ok_eq_hex(ReturnLength, 0xdeadbeef);
        }
    }

    // Allocate a buffer
    LogProcInfo = malloc(ExpectedInfoLength);
    ok(LogProcInfo != NULL, "\n");
    if (LogProcInfo == NULL)
    {
        return;
    }

    memset(LogProcInfo, 0xAB, ExpectedInfoLength);

    RelationshipType = RelationProcessorCore;
    Status = pNtQuerySystemInformationEx(SystemLogicalProcessorInformation,
                                         &RelationshipType,
                                         sizeof(RelationshipType),
                                         LogProcInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, ExpectedInfoLength);

    ULONG_PTR Group0Mask = ((SIZE_T)1 << g_NumberOfProcessorsGroup0) - 1;
    ULONG NumberOfEntries = ReturnLength / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Info = &LogProcInfo[i];
        ok(Info->ProcessorMask != 0, "[%lu] ProcessorMask is 0\n", i);
        ok((Info->ProcessorMask & ~Group0Mask) == 0, "[%lu] ProcessorMask is invalid: %lx\n", i, Info->ProcessorMask);
        switch (Info->Relationship)
        {
            case RelationProcessorCore:
                ok((Info->ProcessorCore.Flags & (UCHAR)~1) == 0, "[%lu] Unexpected ProcessorCore.Flags: 0x%lx\n", i, Info->ProcessorCore.Flags);
                break;
            case RelationNumaNode:
                ok(Info->NumaNode.NodeNumber < g_NumberOfNumaNodes, "[%lu] Unexpected NumaNodeNumber: %u\n", i, Info->NumaNode.NodeNumber);
                break;
            case RelationCache:
                ok((Info->Cache.Level >= 1) && (Info->Cache.Level <= 3), "[%lu] Unexpected Cache Level: %u\n", i, Info->Cache.Level);
                ok((Info->Cache.Type >= CacheUnified) && (Info->Cache.Type <= CacheData), "[%lu] Unexpected Cache Type: %u\n", i, Info->Cache.Type);
                break;
            case RelationProcessorPackage:
                ok(Info->ProcessorCore.Flags == 0, "[%lu] Unexpected ProcessorCore.Flags: 0x%lx\n", i, Info->ProcessorCore.Flags);
            case RelationGroup:
            case RelationProcessorDie:
            case RelationNumaNodeEx:
            case RelationProcessorModule:
            case RelationAll:
                break;
        }
    }
}

static void Test_SystemProcessorIdleCycleTimeInformation(void) // 83
{
    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION[]
    ULONG ExpectedInfoLength = g_NumberOfProcessorsGroup0 * sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION);
    Test_GroupNumber_Input(SystemProcessorIdleCycleTimeInformation,
                           ExpectedInfoLength,
                           STATUS_BUFFER_TOO_SMALL);
}

static void Test_SystemProcessorPerformanceDistribution(void) // 100
{
    USHORT GroupNumber = 0;
    ULONG ExpectedInfoLength;
    NTSTATUS Status;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_VISTA)
    {
        skip("SystemProcessorPerformanceDistribution is not supported on this OS version\n");
        return;
    }

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION

    //
    // The size of the info can generally be calculazed as:
    // HitCountSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT); // Different between Win8 and Win8.1+
    // StateCount = <Number of supported P-States>
    // DistributionSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION) + (StateCount - 1) * HitCountSize; // Variable size struct
    // HeaderSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION) + (ProcessorCount -1) * sizeof(ULONG) + // Offsets array
    // StateStartOffset = ALIGN_UP_BY(HeaderSize, sizeof(ULONG64))
    // InfoSize = StateStartOffset + ProcessorCount * DistributionSize
    // 
    // To calculate the expected info length, we would need to know the number of states in the distribution.
    // But there is no simple way to get that. Instead we just do a normal call to get the required buffer size,
    // so we can can call the common function to test the input validation.
    //
    // Proper call without output buffer
    Status = pNtQuerySystemInformationEx(SystemProcessorPerformanceDistribution,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         NULL,
                                         0,
                                         &ExpectedInfoLength);
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

    Test_GroupNumber_Input(SystemProcessorPerformanceDistribution,
                           ExpectedInfoLength,
                           STATUS_INFO_LENGTH_MISMATCH);
}

static void Test_SystemLogicalProcessorAndGroupInformation(void) // 107
{
    NTSTATUS Status;
    ULONG ExpectedInfoLength;
    ULONG ReturnLength;
    LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX LogProcInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN7)
    {
        skip("SystemLogicalProcessorAndGroupInformation is not supported on this OS version\n");
        return;
    }

    //
    // The size of the info depends on the machine's hardware (number of cores,
    // threads and cache levels). So we determine it first, so we can call
    // Test_Generic_Input.
    //
    Relationship = RelationProcessorCore;
    Status = pNtQuerySystemInformationEx(SystemLogicalProcessorAndGroupInformation,
                                         &Relationship,
                                         sizeof(Relationship),
                                         NULL,
                                         0,
                                         &ExpectedInfoLength);
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);
    //ok(ExpectedInfoLength % sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) == 0,
    //   "Unexpected info length: %lu\n", ExpectedInfoLength);

    // In: LOGICAL_PROCESSOR_RELATIONSHIP Relationship
    // Out: SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX[]
    Test_Generic_Input(SystemLogicalProcessorAndGroupInformation,
                       &Relationship,
                       sizeof(Relationship),
                       TRUE,
                       ExpectedInfoLength,
                       STATUS_INFO_LENGTH_MISMATCH);

    /* Test all relationship types */
    for (INT i = -2; i <= RelationProcessorModule + 1; i++)
    {
        winetest_push_context("Relationship=%d", i);

        Relationship = i;
        ReturnLength = 0xdeadbeef;
        Status = pNtQuerySystemInformationEx(SystemLogicalProcessorAndGroupInformation,
                                             &Relationship,
                                             sizeof(Relationship),
                                             NULL,
                                             0,
                                             &ReturnLength);
        if ((Relationship >= RelationProcessorCore) && (Relationship <= RelationProcessorModule))
        {
            ok(Status == STATUS_SUCCESS || Status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected Status: 0x%lx\n", Status);
            ok(ReturnLength > 0 && ReturnLength <= 0x2000, "Unexpected ReturnLength: %lu\n", ReturnLength);
        }
        else
        {
            ok_ntstatus(Status, STATUS_UNSUCCESSFUL);
            ok_eq_hex(ReturnLength, 0);
        }
        winetest_pop_context();
    }

    /* Properly test all valid relationship types */
    for (Relationship = RelationProcessorCore; Relationship <= RelationGroup; Relationship++)
    {
        /* Query the size */
        Status = pNtQuerySystemInformationEx(SystemLogicalProcessorAndGroupInformation,
                                             &Relationship,
                                             sizeof(Relationship),
                                             NULL,
                                             0,
                                             &ExpectedInfoLength);
        ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

        // Allocate a buffer
        LogProcInfo = malloc(ExpectedInfoLength);
        ok(LogProcInfo != NULL, "\n");
        if (LogProcInfo == NULL)
        {
            return;
        }

        memset(LogProcInfo, 0xAB, ExpectedInfoLength);

        /* Query the information */
        Status = pNtQuerySystemInformationEx(SystemLogicalProcessorAndGroupInformation,
                                             &Relationship,
                                             sizeof(Relationship),
                                             LogProcInfo,
                                             ExpectedInfoLength,
                                             &ReturnLength);
        ok_ntstatus(Status, STATUS_SUCCESS);
        ok_eq_hex(ReturnLength, ExpectedInfoLength);

        //ULONG_PTR Group0Mask = ((SIZE_T)1 << g_NumberOfProcessorsGroup0) - 1;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Info = LogProcInfo;
        PVOID BufferEnd = (PBYTE)LogProcInfo + ReturnLength;
        while ((PBYTE)Info < (PBYTE)BufferEnd)
        {
            ok(Info->Relationship == Relationship, "Unexpected Relationship: %u\n", Info->Relationship);
            ok(Info->Size > 0, "Unexpected Size: %lu\n", Info->Size);

            switch (Info->Relationship)
            {
                case RelationProcessorCore:
                case RelationProcessorPackage:
                    ok((Info->Processor.Flags & (UCHAR)~1) == 0, "Unexpected Processor.Flags: 0x%lx\n", Info->Processor.Flags);
                    ok(Info->Processor.EfficiencyClass <= 1, "Unexpected Processor.EfficiencyClass: %u\n", Info->Processor.EfficiencyClass);
                    ok(Info->Processor.GroupCount == 1, "Unexpected Processor.GroupCount: %u\n", Info->Processor.GroupCount);
                    break;
                case RelationNumaNode:
                    ok(Info->NumaNode.NodeNumber < g_NumberOfNumaNodes, "Unexpected NumaNodeNumber: %u\n", Info->NumaNode.NodeNumber);
                    break;
                case RelationCache:
                    ok((Info->Cache.Level >= 1) && (Info->Cache.Level <= 3), "Unexpected Cache Level: %u\n", Info->Cache.Level);
                    ok((Info->Cache.Type >= CacheUnified) && (Info->Cache.Type <= CacheData), "Unexpected Cache Type: %u\n", Info->Cache.Type);
                    break;
                case RelationGroup:
                    ok(Info->Group.MaximumGroupCount >= g_NumberOfProcessorGroups, "Unexpected MaximumGroupCount: %u\n", Info->Group.MaximumGroupCount);
                    ok(Info->Group.MaximumGroupCount <= 64, "Unexpected MaximumGroupCount: %u\n", Info->Group.MaximumGroupCount);
                    ok(Info->Group.ActiveGroupCount == g_NumberOfProcessorGroups, "Unexpected ActiveGroupCount: %u\n", Info->Group.ActiveGroupCount);
                    ok(Info->Group.GroupInfo[0].ActiveProcessorCount == g_NumberOfProcessorsGroup0, "Unexpected ActiveProcessorCount for Group 0: %u\n", Info->Group.GroupInfo[0].ActiveProcessorCount);
                    // TODO: validate additional groups
                    break;

                case RelationProcessorDie:
                case RelationNumaNodeEx:
                case RelationProcessorModule:
                case RelationAll:
                    ok(FALSE, "Unexpected Relationship: %u\n", Info->Relationship);
                    break;
            }

            Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((PBYTE)Info + Info->Size);
        }


        free(LogProcInfo);
    }

}

static void Test_SystemProcessorCycleTimeInformation(void) // 108
{
    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN7)
    {
        skip("SystemProcessorCycleTimeInformation is not supported on this OS version\n");
        return;
    }

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION[]
    ULONG ExpectedInfoLength = g_NumberOfProcessorsGroup0 * sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION);
    Test_GroupNumber_Input(SystemProcessorCycleTimeInformation,
                           ExpectedInfoLength,
                           STATUS_BUFFER_TOO_SMALL);

}

static void Test_SystemNodeDistanceInformation(void) // 121
{
    NTSTATUS Status;
    ULONG ExpectedInfoLength;
    ULONG ReturnLength;
    USHORT NodeNumber = 0;
    PUSHORT NodeDistInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN7)
    {
        skip("SystemNodeDistanceInformation is not supported on this OS version\n");
        return;
    }

    // IN: in: USHORT NodeNumber
    // OUT: ULONG[NumberOfNumaNodes]
    ExpectedInfoLength = g_NumberOfNumaNodes * sizeof(ULONG);
    Test_GroupNumber_Input(SystemNodeDistanceInformation,
                           ExpectedInfoLength,
                           STATUS_INFO_LENGTH_MISMATCH);

    // Allocate a buffer
    NodeDistInfo = malloc(ExpectedInfoLength);
    ok(NodeDistInfo != NULL, "\n");
    if (NodeDistInfo == NULL)
    {
        return;
    }

    memset(NodeDistInfo, 0xAB, ExpectedInfoLength);

    Status = pNtQuerySystemInformationEx(SystemNodeDistanceInformation,
                                         &NodeNumber,
                                         sizeof(NodeNumber),
                                         NodeDistInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, ExpectedInfoLength);

    for (ULONG i = 0; i < g_NumberOfNumaNodes; i++)
    {
        if (i == NodeNumber)
            ok(NodeDistInfo[i] == 0, "[%lu] Unexpected distance for self node: %lu\n", i, NodeDistInfo[i]);
        else
            ok(NodeDistInfo[i] > 0, "[%lu] Unexpected distance: %lu\n", i, NodeDistInfo[i]);
    }

    free(NodeDistInfo);
}

static void Test_SystemProcessorPerformanceInformationEx(void) // 141
{
    NTSTATUS Status;
    ULONG ReturnLength;
    USHORT GroupNumber = 0;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX PerfInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN8)
    {
        skip("SystemProcessorPerformanceInformationEx is not supported on this OS version\n");
        return;
    }

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX[]
    ULONG ExpectedInfoLength = g_NumberOfProcessorsGroup0 * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX);
    Test_GroupNumber_Input(SystemProcessorPerformanceInformationEx,
                           ExpectedInfoLength,
                           STATUS_INFO_LENGTH_MISMATCH);

    // Allocate a buffer
    PerfInfo = malloc(ExpectedInfoLength);
    ok(PerfInfo != NULL, "\n");
    if (PerfInfo == NULL)
    {
        return;
    }

    memset(PerfInfo, 0xAB, ExpectedInfoLength);

    // Proper call with output buffer
    Status = pNtQuerySystemInformationEx(SystemProcessorPerformanceInformationEx,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         PerfInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ULONG64 TotalTime = PerfInfo[0].KernelTime.QuadPart + PerfInfo[0].UserTime.QuadPart;

    ULONG NumberOfEntries = ReturnLength / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX);
    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        /* Check if the values are somewhat reasonable */
        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX Info = &PerfInfo[i];
        ok((Info->IdleTime.QuadPart > 0) && (Info->IdleTime.QuadPart < MAXSYSTIME), "[%lu] Unexpected IdleTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info->IdleTime.QuadPart, MAXSYSTIME);
        ok((Info->KernelTime.QuadPart > 0) && (Info->KernelTime.QuadPart < MAXSYSTIME), "[%lu] Unexpected KernelTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info->KernelTime.QuadPart, MAXSYSTIME);
        ok((Info->UserTime.QuadPart > 0) && (Info->UserTime.QuadPart < MAXSYSTIME), "[%lu] Unexpected UserTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info->UserTime.QuadPart, MAXSYSTIME);
        ok((Info->DpcTime.QuadPart > 0) && (Info->DpcTime.QuadPart < MAXSYSTIME / 100), "[%lu] Unexpected DpcTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info->DpcTime.QuadPart, MAXSYSTIME / 100);
        ok((Info->InterruptTime.QuadPart > 0) && (Info->InterruptTime.QuadPart < MAXSYSTIME / 100), "[%lu] Unexpected InterruptTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info->InterruptTime.QuadPart, MAXSYSTIME / 100);
        ok(Info->InterruptCount > 10, "[%lu] Unexpected InterruptCount: 0x%lx\n", i, Info->InterruptCount);
        ok(Info->Spare0 == 0, "[%lu] Unexpected Spare0: 0x%lx\n", i, Info->Spare0);
        ok((Info->AvailableTime.QuadPart >= 0) && (Info->AvailableTime.QuadPart < MAXTIME), "[%lu] Unexpected AvailableTime: 0x%llx, expected [0 - 0x%llx]\n", i, Info->AvailableTime.QuadPart, MAXTIME);
        ok(Info->Spare1.QuadPart == 0, "[%lu] Unexpected Spare1: 0x%llx\n", i, Info->Spare1.QuadPart);
        ok(Info->Spare2.QuadPart == 0, "[%lu] Unexpected Spare2: 0x%llx\n", i, Info->Spare2.QuadPart);
        ULONG64 Sum = Info->KernelTime.QuadPart + Info->UserTime.QuadPart;
        ULONG64 DiffTime = _abs64(Sum - TotalTime);
        ok(DiffTime < 2000000 /* 200 ms */, "[%lu] DiffTime too large: %llu\n", i, DiffTime);
    }

    free(PerfInfo);
}

static void Test_SystemProcessorCycleStatsInformation(void) // 160
{
    NTSTATUS Status;
    ULONG ReturnLength;
    USHORT GroupNumber = 0;
    PSYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION CycleStatsInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemProcessorCycleStatsInformation is not supported on this OS version\n");
        return;
    }

    // In: USHORT ProcessorGroup
    // Out: SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION[]
    ULONG ExpectedInfoLength = g_NumberOfProcessorsGroup0 * sizeof(SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION);
    Test_GroupNumber_Input(SystemProcessorCycleStatsInformation,
                           ExpectedInfoLength,
                           STATUS_BUFFER_TOO_SMALL);

    // Allocate a buffer
    CycleStatsInfo = malloc(ExpectedInfoLength);
    ok(CycleStatsInfo != NULL, "\n");
    if (CycleStatsInfo == NULL)
    {
        return;
    }

    memset(CycleStatsInfo, 0xAB, ExpectedInfoLength);

    // Proper call with output buffer
    Status = pNtQuerySystemInformationEx(SystemProcessorCycleStatsInformation,
                                         &GroupNumber,
                                         sizeof(GroupNumber),
                                         CycleStatsInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, g_IsWow64 ? STATUS_INVALID_INFO_CLASS : STATUS_SUCCESS);
    if (g_IsWow64)
    {
        free(CycleStatsInfo);
        return;
    }

    ULONG NumberOfEntries = ReturnLength / sizeof(SYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION);
    for (ULONG i = 0; i < NumberOfEntries; i++)
    {
        /* Check if the values are somewhat reasonable */
        PSYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION Info = &CycleStatsInfo[i];
        for (ULONG bucket = 0; bucket < SystemProcessorCycleStatsBucketMax; bucket++)
        {
            ULONG64 PrimaryCycles = Info->Cycles[bucket][SystemProcessorCycleStatsEfficiencyClassPrimary];
            ok((PrimaryCycles >= 0) && (PrimaryCycles < __rdtsc()),
                "[%lu][%lu][%lu] Unexpected PrimaryCycles: 0x%llx\n", i, bucket, SystemProcessorCycleStatsEfficiencyClassPrimary, PrimaryCycles);
            ULONG64 SecondaryCycles = Info->Cycles[bucket][SystemProcessorCycleStatsEfficiencyClassSecondary];
            ok((PrimaryCycles >= 0) && (PrimaryCycles < __rdtsc()),
                "[%lu][%lu][%lu] Unexpected SecondaryCycles: 0x%llx\n", i, bucket, SystemProcessorCycleStatsEfficiencyClassSecondary, SecondaryCycles);
        }
    }

    free(CycleStatsInfo);
}

static void Test_SystemIsolatedUserModeInformation(void) // 165
{
    NTSTATUS Status;
    HANDLE InputHandle = (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEF;
    ULONG ReturnLength;
    SYSTEM_ISOLATED_USER_MODE_INFORMATION IsolatedUmInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemIsolatedUserModeInformation is not supported on this OS version\n");
        return;
    }

    // In: HANDLE
    // Out: SYSTEM_ISOLATED_USER_MODE_INFORMATION
    Test_Generic_Input(SystemIsolatedUserModeInformation,
                       &InputHandle,
                       sizeof(InputHandle),
                       FALSE,
                       sizeof(SYSTEM_ISOLATED_USER_MODE_INFORMATION),
                       STATUS_INFO_LENGTH_MISMATCH);

    // Test with current process
    InputHandle = GetCurrentProcess();
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemIsolatedUserModeInformation,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         &IsolatedUmInfo,
                                         sizeof(IsolatedUmInfo),
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, sizeof(IsolatedUmInfo));

    ok_eq_char(IsolatedUmInfo.Spare0[0], 0);
    ok_eq_char(IsolatedUmInfo.Spare0[1], 0);
    ok_eq_char(IsolatedUmInfo.Spare0[2], 0);
    ok_eq_char(IsolatedUmInfo.Spare0[3], 0);
    ok_eq_char(IsolatedUmInfo.Spare0[4], 0);
    ok_eq_ulonglong(IsolatedUmInfo.Spare1, 0);

}

static void Test_SystemCpuSetInformation(void) // 175
{
    NTSTATUS Status;
    ULONG ReturnLength;
    HANDLE InputHandle = NULL;
    PSYSTEM_CPU_SET_INFORMATION CpuSetInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemCpuSetInformation is not supported on this OS version\n");
        return;
    }

    // In: HANDLE
    // Out: SYSTEM_CPU_SET_INFORMATION
    ULONG ExpectedInfoLength = g_NumberOfProcessors * sizeof(SYSTEM_CPU_SET_INFORMATION);
    Test_Generic_Input(SystemCpuSetInformation,
                       &InputHandle,
                       sizeof(InputHandle),
                       FALSE,
                       ExpectedInfoLength,
                       STATUS_BUFFER_TOO_SMALL);

    // Allocate a buffer
    CpuSetInfo = malloc(ExpectedInfoLength);
    ok(CpuSetInfo != NULL, "\n");
    if (CpuSetInfo == NULL)
    {
        return;
    }

    memset(CpuSetInfo, 0xAB, ExpectedInfoLength);

    // Proper call with output buffer
    Status = pNtQuerySystemInformationEx(SystemCpuSetInformation,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         CpuSetInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ULONG LastCoreIndex = 0;
    for (ULONG i = 0; i < g_NumberOfProcessors; i++)
    {
        PSYSTEM_CPU_SET_INFORMATION Info = &CpuSetInfo[i];
        ok(Info->Size == sizeof(SYSTEM_CPU_SET_INFORMATION), "[%lu] Unexpected Size: %lu\n", i, Info->Size);
        ok(Info->CpuSet.Id == (0x100 | i), "[%lu] Unexpected Id: %u\n", i, Info->CpuSet.Id);
        ok(Info->CpuSet.Group == 0, "[%lu] Unexpected Group: %u\n", i, Info->CpuSet.Group);
        ok(Info->CpuSet.LogicalProcessorIndex == i, "[%lu] Unexpected LogicalProcessorIndex: %u\n", i, Info->CpuSet.LogicalProcessorIndex);
        ok((Info->CpuSet.CoreIndex == i) || (Info->CpuSet.CoreIndex == LastCoreIndex), "[%lu] Unexpected CoreIndex: %u\n", i, Info->CpuSet.CoreIndex);
        if (Info->CpuSet.CoreIndex == i)
            LastCoreIndex = i;
        ok(Info->CpuSet.LastLevelCacheIndex <= 4, "[%lu] Unexpected LastLevelCacheIndex: %u\n", i, Info->CpuSet.LastLevelCacheIndex);
        ok(Info->CpuSet.NumaNodeIndex < g_NumberOfNumaNodes, "[%lu] Unexpected NumaNodeIndex: %u\n", i, Info->CpuSet.NumaNodeIndex);
        ok(Info->CpuSet.EfficiencyClass <= 1, "[%lu] Unexpected EfficiencyClass: %u\n", i, Info->CpuSet.EfficiencyClass);
        ok((Info->CpuSet.AllFlags & ~1) == 0, "[%lu] Unexpected AllFlags: 0x%lx\n", i, Info->CpuSet.AllFlags);
        ok(Info->CpuSet.SchedulingClass < 128, "[%lu] Unexpected SchedulingClass: %u\n", i, Info->CpuSet.SchedulingClass);
        ok(Info->CpuSet.AllocationTag == 0, "[%lu] Unexpected AllocationTag: %u\n", i, Info->CpuSet.AllocationTag);
    }

    // Now test with a non-zero handle, which should return the same info
    InputHandle = GetCurrentProcess();
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemCpuSetInformation,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         CpuSetInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, ExpectedInfoLength);

    // TODO: Test remote process handle of WOW64 process

    // Thread is not allowed!
    InputHandle = GetCurrentThread();
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemCpuSetInformation,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         CpuSetInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_OBJECT_TYPE_MISMATCH);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    free(CpuSetInfo);
}

static void Test_SystemSecureKernelProfileInformation(void) // 178
{
    NTSTATUS Status;
    ULONG ReturnLength;
    HANDLE InputHandle = NULL;
    SYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION SecureKernelInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemSecureKernelProfileInformation is not supported on this OS version\n");
        return;
    }

    // In: HANDLE
    // Out: SYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION
    //ULONG ExpectedInfoLength = g_NumberOfProcessors * sizeof(SYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION);
    Test_Generic_Input(SystemSecureKernelProfileInformation,
                       &InputHandle,
                       sizeof(InputHandle),
                       FALSE,
                       0,
                       STATUS_BUFFER_TOO_SMALL);

    // Test with invalid handle
    InputHandle = (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEF;
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemSecureKernelProfileInformation,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         &SecureKernelInfo,
                                         sizeof(SecureKernelInfo),
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_PRIVILEGE_NOT_HELD);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    // Test with current process
    InputHandle = GetCurrentProcess();
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemSecureKernelProfileInformation,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         &SecureKernelInfo,
                                         sizeof(SecureKernelInfo),
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_PRIVILEGE_NOT_HELD);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    BOOLEAN WasEnabled;
    Status = RtlAdjustPrivilege(SE_SYSTEM_PROFILE_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        skip("Skipping SystemSecureKernelProfileInformation test, requires SE_SYSTEM_PROFILE_PRIVILEGE\n");
        return;
    }

    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemSecureKernelProfileInformation,
                                            &InputHandle,
                                            sizeof(InputHandle),
                                            &SecureKernelInfo,
                                            sizeof(SecureKernelInfo),
                                            &ReturnLength);
    ok_ntstatus(Status, STATUS_HV_FEATURE_UNAVAILABLE);
    ok_eq_hex(ReturnLength, 0);

    InputHandle = NULL;
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemSecureKernelProfileInformation,
                                            &InputHandle,
                                            sizeof(InputHandle),
                                            &SecureKernelInfo,
                                            sizeof(SecureKernelInfo),
                                            &ReturnLength);
    ok_ntstatus(Status, STATUS_HV_FEATURE_UNAVAILABLE);
    ok_eq_hex(ReturnLength, 0);

    InputHandle = (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEF;
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemSecureKernelProfileInformation,
                                            &InputHandle,
                                            sizeof(InputHandle),
                                            &SecureKernelInfo,
                                            sizeof(SecureKernelInfo),
                                            &ReturnLength);
    ok_ntstatus(Status, STATUS_HV_FEATURE_UNAVAILABLE);
    ok_eq_hex(ReturnLength, 0);

    RtlAdjustPrivilege(SE_SYSTEM_PROFILE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
}

static void Test_SystemInterruptSteeringInformation(void) // 180
{
    NTSTATUS Status;
    ULONG ReturnLength;
    SYSTEM_INTERRUPT_STEERING_INFORMATION_INPUT Input = { 0 };

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemInterruptSteeringInformation is not supported on this OS version\n");
        return;
    }

    // In: SYSTEM_INTERRUPT_STEERING_INFORMATION_INPUT
    // Out: SYSTEM_INTERRUPT_STEERING_INFORMATION_OUTPUT
    Test_Common(SystemInterruptSteeringInformation, 1);

    // Too large input buffer
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemInterruptSteeringInformation,
                                         &Input,
                                         sizeof(Input) + 1,
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);
    ok_eq_ulong(ReturnLength, 0);

    // Invalid OutputBuffer with null OutputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemInterruptSteeringInformation,
                                         &Input,
                                         sizeof(Input),
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull,
                                         0,
                                         &ReturnLength);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);
    ok_eq_ulong(ReturnLength, 0);

    // Null RetunLength with NULL output buffer
    Status = pNtQuerySystemInformationEx(SystemInterruptSteeringInformation,
                                         &Input,
                                         sizeof(Input),
                                         NULL,
                                         0,
                                         NULL);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);

    // Invalid RetunLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemInterruptSteeringInformation,
                                         &Input,
                                         sizeof(Input),
                                         NULL,
                                         0,
                                         (PVOID)(ULONG_PTR)0xdeadbeefdeadbee0ull);
    ok(Status == STATUS_ACCESS_VIOLATION, "[%u] Unexpected Status: 0x%lx\n", SystemInterruptSteeringInformation, Status);

    // Unaligned RetunLength
    ULONG Buffer[4] = { 0xCCCCCCCC, 0xCCCCCCCC, 0xCCCCCCCC, 0xCCCCCCCC };
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemInterruptSteeringInformation,
                                         &Input,
                                         sizeof(Input),
                                         NULL,
                                         0,
                                         (PULONG)((PUCHAR)Buffer + 1));
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);
    ok_eq_ulong(*(PULONG)((PUCHAR)Buffer + 1), 0);

    // NULL OutputBuffer with 0 OutputBufferLength
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemInterruptSteeringInformation,
                                         &Input,
                                         sizeof(Input),
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok_eq_hex(Status, STATUS_ACCESS_DENIED);
    ok_eq_ulong(ReturnLength, 0);
}

static void Test_SystemSupportedProcessorArchitectures(void) // 181
{
    NTSTATUS Status;
    ULONG ExpectedInfoLength;
    ULONG ReturnLength;
    HANDLE InputHandle = 0;
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION* ArchInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemSupportedProcessorArchitectures is not supported on this OS version\n");
        return;
    }

#ifdef _M_AMD64
    ExpectedInfoLength = 3 * sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION);
#else
    ExpectedInfoLength = 2 * sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION);
#endif

    // In: HANDLE ProcessHandle (the handle can be NULL)
    // Out: SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION[]
    Test_Generic_Input(SystemSupportedProcessorArchitectures,
                       &InputHandle,
                       sizeof(InputHandle),
                       FALSE,
                       ExpectedInfoLength,
                       STATUS_BUFFER_TOO_SMALL);

    // Allocate a buffer
    ArchInfo = malloc(ExpectedInfoLength);
    ok(ArchInfo != NULL, "\n");
    if (ArchInfo == NULL)
    {
        return;
    }

    memset(ArchInfo, 0xAB, ExpectedInfoLength);

    // Proper call with output buffer
    Status = pNtQuerySystemInformationEx(SystemSupportedProcessorArchitectures,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         ArchInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ULONG NumberOfEntries = ReturnLength / sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION);
#ifdef _M_AMD64
    ok_eq_ulong(NumberOfEntries, 3);
#else
    ok_eq_ulong(NumberOfEntries, 2);
#endif

    ok(ArchInfo[0].Machine == IMAGE_FILE_MACHINE_NATIVE, "[%lu] Unexpected Machine: 0x%x\n", 0, ArchInfo[0].Machine);
    ok(ArchInfo[0].KernelMode == 1, "[%lu] Unexpected KernelMode: 0x%x\n", 0, ArchInfo[0].KernelMode);
    ok(ArchInfo[0].UserMode == 1, "[%lu] Unexpected UserMode: 0x%x\n", 0, ArchInfo[0].UserMode);
    ok(ArchInfo[0].Native == 1, "[%lu] Unexpected Native: 0x%x\n", 0, ArchInfo[0].Native);
    ok(ArchInfo[0].Process == 0, "[%lu] Unexpected Process: 0x%x\n", 0, ArchInfo[0].Process);
    ok(ArchInfo[0].WoW64Container == 0, "[%lu] Unexpected WoW64Container: 0x%x\n", 0, ArchInfo[0].WoW64Container);
    ok(ArchInfo[0].ReservedZero0 == 0, "[%lu] Unexpected ReservedZero0: 0x%x\n", 0, ArchInfo[0].ReservedZero0);
#ifdef _M_AMD64
    ok(ArchInfo[1].Machine == IMAGE_FILE_MACHINE_I386, "[%lu] Unexpected Machine: 0x%x\n", 1, ArchInfo[1].Machine);
    ok(ArchInfo[1].KernelMode == 0, "[%lu] Unexpected KernelMode: 0x%x\n", 1, ArchInfo[1].KernelMode);
    ok(ArchInfo[1].UserMode == 1, "[%lu] Unexpected UserMode: 0x%x\n", 1, ArchInfo[1].UserMode);
    ok(ArchInfo[1].Native == 0, "[%lu] Unexpected Native: 0x%x\n", 1, ArchInfo[1].Native);
    ok(ArchInfo[1].Process == 0, "[%lu] Unexpected Process: 0x%x\n", 1, ArchInfo[1].Process);
    ok(ArchInfo[1].WoW64Container == 0 || ArchInfo[1].WoW64Container == 1,
       "[%lu] Unexpected WoW64Container: 0x%x\n", 1, ArchInfo[1].WoW64Container);
    ok(ArchInfo[1].ReservedZero0 == 0, "[%lu] Unexpected ReservedZero0: 0x%x\n", 1, ArchInfo[1].ReservedZero0);
#endif

    ok(ArchInfo[NumberOfEntries - 1].Machine == IMAGE_FILE_MACHINE_UNKNOWN, "[%lu] Unexpected Machine: 0x%x\n", NumberOfEntries - 1, ArchInfo[NumberOfEntries - 1].Machine);

    // Now test with a non-zero handle, which should return the same info
    InputHandle = GetCurrentProcess();
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemSupportedProcessorArchitectures,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         ArchInfo,
                                         ExpectedInfoLength,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, ExpectedInfoLength);

    // TODO: Test remote process handle of WOW64 process

    // Thread is not allowed!
    InputHandle = GetCurrentThread();
    ReturnLength = 0xdeadbeef;
    Status = pNtQuerySystemInformationEx(SystemSupportedProcessorArchitectures,
                                         &InputHandle,
                                         sizeof(InputHandle),
                                         NULL,
                                         0,
                                         &ReturnLength);
    ok_ntstatus(Status, STATUS_OBJECT_TYPE_MISMATCH);
    ok_eq_hex(ReturnLength, 0xdeadbeef);

    free(ArchInfo);
}

// Class 209 - SystemCodeIntegritySyntheticCacheInformation
typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION
{
    ULONG Length;
    union
    {
        ULONG CodeIntegrityOptions;
        struct
        {
            ULONG Enabled : 1;                          // CODEINTEGRITY_OPTION_ENABLED
            ULONG TestSign : 1;                         // CODEINTEGRITY_OPTION_TESTSIGN
            ULONG UmciEnabled : 1;                      // CODEINTEGRITY_OPTION_UMCI_ENABLED
            ULONG UmciAuditModeEnabled : 1;             // CODEINTEGRITY_OPTION_UMCI_AUDITMODE_ENABLED
            ULONG UmciExclusionPathsEnabled : 1;        // CODEINTEGRITY_OPTION_UMCI_EXCLUSIONPATHS_ENABLED
            ULONG TestBuild : 1;                        // CODEINTEGRITY_OPTION_TEST_BUILD
            ULONG PreproductionBuild : 1;               // CODEINTEGRITY_OPTION_PREPRODUCTION_BUILD
            ULONG DebugModeEnabled : 1;                 // CODEINTEGRITY_OPTION_DEBUGMODE_ENABLE
            ULONG FlightBuild : 1;                      // CODEINTEGRITY_OPTION_FLIGHT_BUILD
            ULONG FlightingEnabled : 1;                 // CODEINTEGRITY_OPTION_FLIGHTING_ENABLED
            ULONG HvciKmciEnabled : 1;                  // CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED
            ULONG HvciKmciAuditModeEnabled : 1;         // CODEINTEGRITY_OPTION_HVCI_KMCI_AUDITMODE_ENABLED
            ULONG HvciKmciStrictModeEnabled : 1;        // CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED
            ULONG HvciIumEnabled : 1;                   // CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED
            ULONG WhqlEnforcementEnabled : 1;           // CODEINTEGRITY_OPTION_WHQL_ENFORCEMENT_ENABLED
            ULONG WhqlAuditModeEnabled : 1;             // CODEINTEGRITY_OPTION_WHQL_AUDITMODE_ENABLED
            ULONG Spare : 16;
        };
    };
} SYSTEM_CODEINTEGRITY_INFORMATION, *PSYSTEM_CODEINTEGRITY_INFORMATION;

static void Test_SystemCodeIntegritySyntheticCacheInformation(void) // 209
{
    NTSTATUS Status;
    ULONG ReturnLength;
    ULONG64 Input = 0;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemCodeIntegritySyntheticCacheInformation is not supported on this OS version\n");
        return;
    }

    // Check if we have admin rights, as this info class requires it
    if (!IsUserAdmin())
    {
        ReturnLength = 0xDEADBEEF;
        Status = pNtQuerySystemInformationEx(SystemCodeIntegritySyntheticCacheInformation,
                                             &Input,
                                             sizeof(Input),
                                             NULL,
                                             0,
                                             &ReturnLength);
        ok_ntstatus(Status, STATUS_ACCESS_DENIED);
        ok_eq_hex(ReturnLength, 0);

        skip("Skipping SystemCodeIntegritySyntheticCacheInformation test, requires admin rights\n");
        return;
    }

    // In: ???
    // Out: ???
    Test_Common(SystemCodeIntegritySyntheticCacheInformation, sizeof(Input));
}

//static
void Test_SystemFeatureConfigurationInformation(void) // 210, since 20H1
{
    //NTSTATUS Status;
    ULONG ExpectedInfoLength;
    //ULONG ReturnLength;
    SYSTEM_FEATURE_CONFIGURATION_QUERY FeatureQuery = { 0 };
    //PSYSTEM_FEATURE_CONFIGURATION_INFORMATION FeatureInfo;

    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemFeatureConfigurationInformation is not supported on this OS version\n");
        return;
    }

    // In: SYSTEM_FEATURE_CONFIGURATION_QUERY (only requires 1 byte)
    // Out: SYSTEM_FEATURE_CONFIGURATION_INFORMATION
    // Can be set with NtSetSystemInformation, passing SYSTEM_FEATURE_CONFIGURATION_UPDATE
    Test_Common(SystemFeatureConfigurationInformation, 1);

    ExpectedInfoLength = 0; // sizeof(*FeatureInfo);

    Test_Generic_Input(SystemFeatureConfigurationInformation,
                       &FeatureQuery,
                       1,
                       TRUE,
                       ExpectedInfoLength,
                       STATUS_INFO_LENGTH_MISMATCH);

}

//static
void Test_SystemFeatureConfigurationSectionInformation(void) // 211
{
    if (!is_reactos() && GetNTDDIVersion() < NTDDI_WIN10)
    {
        skip("SystemFeatureConfigurationSectionInformation is not supported on this OS version\n");
        return;
    }

    // In: SYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST (only requires 1 byte)
    // Out: SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION
    //Test_Common(SystemFeatureConfigurationSectionInformation, sizeof(SYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST));
    Test_Common(SystemFeatureConfigurationSectionInformation, 1);

    //NTSTATUS Status;
    ULONG ExpectedInfoLength;
    SYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST FeatureRequest = { 0 };
    //PSYSTEM_FEATURE_CONFIGURATION_INFORMATION FeatureInfo;

    ExpectedInfoLength = 0; // sizeof(*FeatureInfo);
    Test_Generic_Input(SystemFeatureConfigurationSectionInformation,
                       &FeatureRequest,
                       1,
                       TRUE,
                       ExpectedInfoLength,
                       STATUS_INFO_LENGTH_MISMATCH);
}

START_TEST(NtQuerySystemInformationEx)
{//__debugbreak();
    HINSTANCE hinst = LoadLibraryA("ntdll.dll");
    pNtQuerySystemInformationEx = (FN_NtQuerySystemInformationEx*)GetProcAddress(hinst, "NtQuerySystemInformationEx");
    if (pNtQuerySystemInformationEx == NULL)
    {
        hinst = LoadLibraryA("ntdll_vista.dll");
        pNtQuerySystemInformationEx = (FN_NtQuerySystemInformationEx*)GetProcAddress(hinst, "NtQuerySystemInformationEx");
    }

    if (pNtQuerySystemInformationEx == NULL)
    {
        skip("NtQuerySystemInformationEx is not available\n");
        return;
    }

    g_NumberOfProcessors = NtCurrentPeb()->NumberOfProcessors;
    g_NumberOfProcessorsGroup0 = min(g_NumberOfProcessors, 64);
    g_NumberOfProcessorGroups = (g_NumberOfProcessors + 63) / 64;
    g_NumberOfNumaNodes = 1;
    g_NumberOfCpuCores = GetNumberOfCpuCores();

    LARGE_INTEGER SystemTime;
    NtQuerySystemTime(&SystemTime);
    g_CurrentSystemTime = SystemTime.QuadPart;

    IsWow64Process(GetCurrentProcess(), &g_IsWow64);

    Test_AllInfoClasses();

    Test_SystemProcessorPerformanceInformation(); // 8
    Test_SystemInterruptInformation(); // 23
    Test_SystemProcessorIdleInformation(); // 42
    Test_SystemProcessorPowerInformation(); // 61
    Test_SystemWatchdogTimerInformation(); // 72
    Test_SystemLogicalProcessorInformation(); // 73
    Test_SystemProcessorIdleCycleTimeInformation(); // 83
    Test_SystemProcessorPerformanceDistribution(); // 100
    Test_SystemLogicalProcessorAndGroupInformation(); // 107
    Test_SystemProcessorCycleTimeInformation(); // 108
    Test_SystemNodeDistanceInformation(); // 121
    Test_SystemProcessorPerformanceInformationEx(); // 141
    Test_SystemProcessorCycleStatsInformation(); // 160
    Test_SystemIsolatedUserModeInformation(); // 165
    Test_SystemCpuSetInformation(); // 175
    Test_SystemSecureKernelProfileInformation(); // 178
    Test_SystemInterruptSteeringInformation(); // 180
    Test_SystemSupportedProcessorArchitectures(); // 181
    Test_SystemCodeIntegritySyntheticCacheInformation(); // 209
//Test_SystemFeatureConfigurationInformation(); // 210
//Test_SystemFeatureConfigurationSectionInformation(); // 211
}
