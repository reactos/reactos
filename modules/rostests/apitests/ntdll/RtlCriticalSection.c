/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for Rtl Critical Section API
 * COPYRIGHT:   Copyright 2023 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <pseh/pseh2.h>

SYSTEM_INFO g_SysInfo;
OSVERSIONINFOEXA g_VerInfo;
ULONG g_Version;
ULONG g_DefaultSpinCount;

typedef
NTSTATUS
NTAPI
FN_RtlInitializeCriticalSectionEx(
    _Out_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount,
    _In_ ULONG Flags);

FN_RtlInitializeCriticalSectionEx* pfnRtlInitializeCriticalSectionEx;
RTL_CRITICAL_SECTION CritSect;
HANDLE hEventThread1Ready, hEventThread1Cont;
HANDLE hEventThread2Ready, hEventThread2Cont;

static
void
Test_Init(void)
{
    NTSTATUS Status;
    BOOL HasDebugInfo = (g_Version <= _WIN32_WINNT_VISTA);

    _SEH2_TRY
    {
        RtlInitializeCriticalSection(NULL);
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    Status = RtlInitializeCriticalSection(&CritSect);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_long(CritSect.LockCount, -1);
    ok_long(CritSect.RecursionCount, 0);
    ok_ptr(CritSect.OwningThread, NULL);
    ok_ptr(CritSect.LockSemaphore, NULL);
    ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);
    if (HasDebugInfo)
    {
        ok(CritSect.DebugInfo != NULL, "DebugInfo is %p\n", CritSect.DebugInfo);
        ok(CritSect.DebugInfo != LongToPtr(-1), "DebugInfo is %p\n", CritSect.DebugInfo);
    }
    else
    {
        ok(CritSect.DebugInfo == LongToPtr(-1), "DebugInfo is %p\n", CritSect.DebugInfo);
    }

    Status = RtlInitializeCriticalSectionAndSpinCount(&CritSect, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_long(CritSect.LockCount, -1);
    ok_long(CritSect.RecursionCount, 0);
    ok_ptr(CritSect.OwningThread, NULL);
    ok_ptr(CritSect.LockSemaphore, NULL);
    ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);
    if (HasDebugInfo)
    {
        ok(CritSect.DebugInfo != NULL, "DebugInfo is %p\n", CritSect.DebugInfo);
        ok(CritSect.DebugInfo != LongToPtr(-1), "DebugInfo is %p\n", CritSect.DebugInfo);
    }
    else
    {
        ok(CritSect.DebugInfo == LongToPtr(-1), "DebugInfo is %p\n", CritSect.DebugInfo);
    }

    Status = RtlInitializeCriticalSectionAndSpinCount(&CritSect, 0xFF000000);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);

    Status = RtlInitializeCriticalSectionAndSpinCount(&CritSect, 0x1234);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_size_t(CritSect.SpinCount, (g_SysInfo.dwNumberOfProcessors > 1) ? 0x1234 : 0);

    if (pfnRtlInitializeCriticalSectionEx != NULL)
    {
        _SEH2_TRY
        {
            pfnRtlInitializeCriticalSectionEx(NULL, 0, 0);
            Status = STATUS_SUCCESS;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

        Status = pfnRtlInitializeCriticalSectionEx(&CritSect, 0, 0);
        ok_ntstatus(Status, STATUS_SUCCESS);
        ok_long(CritSect.LockCount, -1);
        ok_long(CritSect.RecursionCount, 0);
        ok_ptr(CritSect.OwningThread, NULL);
        ok_ptr(CritSect.LockSemaphore, NULL);
        ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);
        if (HasDebugInfo)
        {
            ok(CritSect.DebugInfo != NULL, "DebugInfo is %p\n", CritSect.DebugInfo);
            ok(CritSect.DebugInfo != LongToPtr(-1), "DebugInfo is %p\n", CritSect.DebugInfo);
            if ((CritSect.DebugInfo != NULL) && (CritSect.DebugInfo != LongToPtr(-1)))
            {
                ok_int(CritSect.DebugInfo->Type, 0);
                ok_int(CritSect.DebugInfo->CreatorBackTraceIndex, 0);
                ok_int(CritSect.DebugInfo->CreatorBackTraceIndexHigh, 0);
                ok_ptr(CritSect.DebugInfo->CriticalSection, &CritSect);
                ok(CritSect.DebugInfo->ProcessLocksList.Flink != NULL, "Flink is NULL\n");
                ok(CritSect.DebugInfo->ProcessLocksList.Blink != NULL, "Blink is NULL\n");
                if ((CritSect.DebugInfo->ProcessLocksList.Flink != NULL) &&
                    (CritSect.DebugInfo->ProcessLocksList.Blink != NULL))
                {
                    ok_ptr(CritSect.DebugInfo->ProcessLocksList.Flink->Blink,
                        &CritSect.DebugInfo->ProcessLocksList);
                    ok_ptr(CritSect.DebugInfo->ProcessLocksList.Blink->Flink,
                        &CritSect.DebugInfo->ProcessLocksList);
                }
                ok_long(CritSect.DebugInfo->EntryCount, 0);
                ok_long(CritSect.DebugInfo->ContentionCount, 0);
                ok_long(CritSect.DebugInfo->Flags, 0);
                ok_int(CritSect.DebugInfo->SpareWORD, 0);
            }
        }
        else
        {
            ok(CritSect.DebugInfo == LongToPtr(-1), "DebugInfo is %p\n", CritSect.DebugInfo);
        }

        Status = pfnRtlInitializeCriticalSectionEx(&CritSect, 0x00FFFFFF, 0);
        ok_size_t(CritSect.SpinCount, (g_SysInfo.dwNumberOfProcessors > 1) ? 0x00FFFFFF : 0);

        Status = pfnRtlInitializeCriticalSectionEx(&CritSect, 0x01000000, 0);
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

        Status = pfnRtlInitializeCriticalSectionEx(&CritSect, 0x80000000, 0);
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

        _SEH2_TRY
        {
            Status = pfnRtlInitializeCriticalSectionEx(NULL, 0x12345678, 0);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

        for (ULONG i = 0; i < 32; i++)
        {
            ULONG Flags = 1UL << i;
            ULONG AllowedFlags = 0x07FFFFFF;
            if (g_Version >= _WIN32_WINNT_WIN7) AllowedFlags |= 0x18000000;
            NTSTATUS ExpectedStatus = (Flags & ~AllowedFlags) ?
                STATUS_INVALID_PARAMETER_3 : STATUS_SUCCESS;
            Status = pfnRtlInitializeCriticalSectionEx(&CritSect, 0, Flags);
            ok(Status == ExpectedStatus, "Wrong Status (0x%lx) for Flags 0x%lx\n",
                Status, Flags);
            if (NT_SUCCESS(Status))
            {
                ULONG SetFlags = Flags & 0x08000000;
                if (g_Version >= _WIN32_WINNT_WIN7) SetFlags |= (Flags & 0x03000000);
                ok_size_t(CritSect.SpinCount, (g_SysInfo.dwNumberOfProcessors > 1) ? g_DefaultSpinCount | SetFlags : SetFlags);
            }
        }

        Status = pfnRtlInitializeCriticalSectionEx(&CritSect, 0, 0xFFFFFFFF);
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);
        _SEH2_TRY
        {
            Status = pfnRtlInitializeCriticalSectionEx(NULL, 0, 0xFFFFFFFF);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);

        Status = pfnRtlInitializeCriticalSectionEx(&CritSect, 0x12345678, 0xFFFFFFFF);
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);

        Status = pfnRtlInitializeCriticalSectionEx(&CritSect, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
        if (g_Version >= _WIN32_WINNT_WIN7)
        {
            ok_ntstatus(Status, STATUS_SUCCESS);
            ok(CritSect.DebugInfo != NULL, "DebugInfo is %p\n", CritSect.DebugInfo);
            ok(CritSect.DebugInfo != LongToPtr(-1), "DebugInfo is %p\n", CritSect.DebugInfo);
            if ((CritSect.DebugInfo != NULL) && (CritSect.DebugInfo != LongToPtr(-1)))
            {
                ok_int(CritSect.DebugInfo->Type, 0);
                ok_int(CritSect.DebugInfo->CreatorBackTraceIndex, 0);
                ok_int(CritSect.DebugInfo->CreatorBackTraceIndexHigh, 0);
                ok_ptr(CritSect.DebugInfo->CriticalSection, &CritSect);
                ok(CritSect.DebugInfo->ProcessLocksList.Flink != NULL, "Flink is NULL\n");
                ok(CritSect.DebugInfo->ProcessLocksList.Blink != NULL, "Blink is NULL\n");
                if ((CritSect.DebugInfo->ProcessLocksList.Flink != NULL) &&
                    (CritSect.DebugInfo->ProcessLocksList.Blink != NULL))
                {
                    ok_ptr(CritSect.DebugInfo->ProcessLocksList.Flink->Blink,
                        &CritSect.DebugInfo->ProcessLocksList);
                    ok_ptr(CritSect.DebugInfo->ProcessLocksList.Blink->Flink,
                        &CritSect.DebugInfo->ProcessLocksList);
                }
                ok_long(CritSect.DebugInfo->EntryCount, 0);
                ok_long(CritSect.DebugInfo->ContentionCount, 0);
                ok_long(CritSect.DebugInfo->Flags, 0);
                ok_int(CritSect.DebugInfo->SpareWORD, 0);
            }
        }
        else
        {
            ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);
        }
    }
    else
    {
        skip("RtlInitializeCriticalSectionEx not available.\n");
    }
}

static
DWORD
WINAPI
ThreadProc1(
    _In_ LPVOID lpParameter)
{
    printf("ThreadProc1 starting\n");
    RtlEnterCriticalSection(&CritSect);

    SetEvent(hEventThread1Ready);

    printf("ThreadProc1 waiting\n");
    WaitForSingleObject(hEventThread1Cont, INFINITE);
    printf("ThreadProc1 returned from wait\n");

    RtlLeaveCriticalSection(&CritSect);

    return 0;
}

static
DWORD
WINAPI
ThreadProc2(
    _In_ LPVOID lpParameter)
{
    printf("ThreadProc2 starting\n");
    RtlEnterCriticalSection(&CritSect);

    SetEvent(hEventThread2Ready);

    printf("ThreadProc2 waiting\n");
    WaitForSingleObject(hEventThread2Cont, INFINITE);
    printf("ThreadProc2 returned from wait\n");

    RtlLeaveCriticalSection(&CritSect);

    return 0;
}

static
void
Test_Acquire(void)
{
    DWORD dwThreadId1, dwThreadId2;
    HANDLE hThread1, hThread2;

    RtlInitializeCriticalSection(&CritSect);

    // Acquire once
    RtlEnterCriticalSection(&CritSect);
    ok_long(CritSect.LockCount, -2);
    ok_long(CritSect.RecursionCount, 1);
    ok_ptr(CritSect.OwningThread, UlongToHandle(GetCurrentThreadId()));
    ok_ptr(CritSect.LockSemaphore, NULL);
    ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);

    // Acquire recursively
    RtlEnterCriticalSection(&CritSect);
    ok_long(CritSect.LockCount, -2);
    ok_long(CritSect.RecursionCount, 2);
    ok_ptr(CritSect.OwningThread, UlongToHandle(GetCurrentThreadId()));
    ok_ptr(CritSect.LockSemaphore, NULL);
    ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);

    hEventThread1Ready = CreateEvent(NULL, TRUE, FALSE, NULL);
    hEventThread1Cont = CreateEvent(NULL, TRUE, FALSE, NULL);
    hEventThread2Ready = CreateEvent(NULL, TRUE, FALSE, NULL);
    hEventThread2Cont = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Create thread 1 and wait to it time to try to acquire the critical section
    hThread1 = CreateThread(NULL, 0, ThreadProc1, NULL, 0, &dwThreadId1);

    // Wait up to 10 s
    for (ULONG i = 0; (CritSect.LockCount == -2) && (i < 1000); i++)
    {
        Sleep(10);
    }

    ok_long(CritSect.LockCount, -6);
    ok_long(CritSect.RecursionCount, 2);
    ok_ptr(CritSect.OwningThread, UlongToHandle(GetCurrentThreadId()));
    //ok_ptr(CritSect.LockSemaphore, LongToPtr(-1)); // TODO: this behaves differently on different OS versions
    ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);

    // Create thread 2 and wait to it time to try to acquire the critical section
    hThread2 = CreateThread(NULL, 0, ThreadProc2, NULL, 0, &dwThreadId2);

    // Wait up to 10 s
    for (ULONG i = 0; (CritSect.LockCount == -6) && (i < 1000); i++)
    {
        Sleep(10);
    }

    ok_long(CritSect.LockCount, -10);
    ok_long(CritSect.RecursionCount, 2);
    ok_ptr(CritSect.OwningThread, UlongToHandle(GetCurrentThreadId()));
    //ok_ptr(CritSect.LockSemaphore, LongToPtr(-1));
    ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);

    RtlLeaveCriticalSection(&CritSect);
    ok_long(CritSect.LockCount, -10);
    ok_long(CritSect.RecursionCount, 1);
    RtlLeaveCriticalSection(&CritSect);

    // Wait until thread 1 has acquired the critical section
    WaitForSingleObject(hEventThread1Ready, INFINITE);

    ok_long(CritSect.LockCount, -6);
    ok_long(CritSect.RecursionCount, 1);
    ok_ptr(CritSect.OwningThread, UlongToHandle(dwThreadId1));
    //ok_ptr(CritSect.LockSemaphore, LongToPtr(-1));
    if (g_DefaultSpinCount != 0)
    {
        ok(CritSect.SpinCount <= g_DefaultSpinCount, "SpinCount increased\n");
    }
    else
    {
        ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);
    }

    ok_size_t(CritSect.SpinCount, g_DefaultSpinCount ? g_DefaultSpinCount - 1 : 0);

    // Release thread 1, wait for thread 2 to acquire the critical section
    SetEvent(hEventThread1Cont);
    WaitForSingleObject(hEventThread2Ready, INFINITE);

    ok_long(CritSect.LockCount, -2);
    ok_long(CritSect.RecursionCount, 1);
    ok_ptr(CritSect.OwningThread, UlongToHandle(dwThreadId2));
    //ok_ptr(CritSect.LockSemaphore, LongToPtr(-1));
    if (g_DefaultSpinCount != 0)
    {
        ok(CritSect.SpinCount <= g_DefaultSpinCount, "SpinCount increased\n");
    }
    else
    {
        ok_size_t(CritSect.SpinCount, g_DefaultSpinCount);
    }

    // Release thread 2
    SetEvent(hEventThread2Cont);

    // To make Thomas happy :)
    WaitForSingleObject(hThread1, INFINITE);
    WaitForSingleObject(hThread2, INFINITE);
}

START_TEST(RtlCriticalSection)
{
    HMODULE hmodNtDll = GetModuleHandleA("ntdll.dll");
    pfnRtlInitializeCriticalSectionEx = (FN_RtlInitializeCriticalSectionEx*)
        GetProcAddress(hmodNtDll, "RtlInitializeCriticalSectionEx");

    g_VerInfo.dwOSVersionInfoSize = sizeof(g_VerInfo);
    GetVersionExA((LPOSVERSIONINFOA)&g_VerInfo);
    g_Version = g_VerInfo.dwMajorVersion << 8 | g_VerInfo.dwMinorVersion;
    printf("g_VerInfo: %lu.%lu.%lu ('%s')\n ",
        g_VerInfo.dwMajorVersion,
        g_VerInfo.dwMinorVersion,
        g_VerInfo.dwBuildNumber,
        g_VerInfo.szCSDVersion);
    GetSystemInfo(&g_SysInfo);
    printf("g_SysInfo.dwNumberOfProcessors = %lu\n", g_SysInfo.dwNumberOfProcessors);

    if ((g_Version >= _WIN32_WINNT_VISTA) && (g_SysInfo.dwNumberOfProcessors > 1))
    {
        g_DefaultSpinCount = 0x20007d0;
    }
    else
    {
        g_DefaultSpinCount = 0;
    }

    Test_Init();
    Test_Acquire();
}
