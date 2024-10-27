/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for support of implicit Thread Local Storage (TLS)
 * PROGRAMMER:  Shane Fournier
 */

#include "precomp.h"

#include <pseh/pseh2.h>

#define TLS_VECTOR_MAX_SIZE 512

WCHAR dllpath_implicit_tls[MAX_PATH];

HANDLE ThreadEvent;

ULONG ExtantTlsEntryCount = 0;

DWORD WINAPI AuxThread0Proc(
  _In_ LPVOID pParameter
)
{
    PVOID* TlsVector;
    PTEB Teb = NtCurrentTeb();
    int i = 0;
    WaitForSingleObject(ThreadEvent, INFINITE);
    TlsVector = Teb->ThreadLocalStoragePointer;
    while(1)
    {
        if(TlsVector[i] && !((ULONG_PTR)TlsVector[i] % 4))
            ++i;
        else
            break;
    }
    WaitForSingleObject(ThreadEvent, INFINITE);
    ok(i == TLS_VECTOR_MAX_SIZE + ExtantTlsEntryCount, "ThreadLocalStoragePointer length is %d, expected length %lu\n", i, TLS_VECTOR_MAX_SIZE + ExtantTlsEntryCount);
    return 0;
}

DWORD WINAPI AuxThread1Proc(
  _In_ LPVOID pParameter
)
{
    PVOID* TlsVector;
    PTEB Teb = NtCurrentTeb();
    TlsVector = Teb->ThreadLocalStoragePointer;
    ok(TlsVector[1] != NULL, "ThreadLocalStoragePointer index 1 is NULL; failed to initialize other thread with TLS entries present\n");
    return 0;
}

START_TEST(implicit_tls)
{
    PVOID* TlsVector;
    WCHAR workdir[MAX_PATH];
    WCHAR duplicatepath[MAX_PATH];
	WCHAR basedllname[MAX_PATH];
    HMODULE DllAddr[TLS_VECTOR_MAX_SIZE];
    BOOL IsSuccess;
    DWORD Length;
    HANDLE AuxThread0, AuxThread1;
    int i;
    PTEB Teb = NtCurrentTeb();
    ULONG TlsIdx0Value;
    ULONG TlsIdx0ValueAfter;

    IsSuccess = GetTempPathW(_countof(workdir), workdir);
    ok(IsSuccess, "GetTempPathW error: %lu\n", GetLastError());

    Length = GetTempFileNameW(workdir, L"implicit_tls", 0, dllpath_implicit_tls);
    ok(Length != 0, "GetTempFileNameW failed with %lu\n", GetLastError());

    IsSuccess = CopyFileW(L"implicit_tls.dll", dllpath_implicit_tls, FALSE);
    ok(IsSuccess, "CopyFileW failed with %lu\n", GetLastError());

    ThreadEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(ThreadEvent != INVALID_HANDLE_VALUE, "CreateEventA failed with %lu\n", GetLastError());

    AuxThread0 = CreateThread(NULL, 0, AuxThread0Proc, NULL, 0, NULL);
    ok(AuxThread0 != NULL, "CreateThread failed with %lu\n", GetLastError());

    TlsVector = Teb->ThreadLocalStoragePointer;
    PULONG ModuleHandle = TlsVector[0];
    #if defined(_MSC_VER)
    #pragma warning( disable : 4311)
    #endif
    *ModuleHandle = (ULONG)GetModuleHandleA(NULL) + 3;

    TlsIdx0Value = *(PULONG)TlsVector[0];
    while (TlsVector[ExtantTlsEntryCount] && !((ULONG_PTR)TlsVector[ExtantTlsEntryCount] % 4))
    {
        ++ExtantTlsEntryCount;
    }
    for (i = 0; i < TLS_VECTOR_MAX_SIZE; i++)
    {
        StringCchPrintfW(basedllname, MAX_PATH, L"basedllname_%d", i);
        Length = GetTempFileNameW(workdir, basedllname, 0, duplicatepath);
        ok(Length != 0, "GetTempFileNameW failed with %lu\n", GetLastError());
        IsSuccess = CopyFileW(dllpath_implicit_tls, duplicatepath, FALSE);
        ok(IsSuccess, "CopyFileW failed with %lu\n", GetLastError());
        DllAddr[i] = LoadLibraryW(duplicatepath);
        ok(DllAddr[i] != NULL, "LoadLibraryW failed with %lu\n", GetLastError());

        if (i == TLS_VECTOR_MAX_SIZE / 2)
        {
            AuxThread1 = CreateThread(NULL, 0, AuxThread1Proc, NULL, 0, NULL);
		    ok(AuxThread1 != NULL, "CreateThread failed with %lu\n", GetLastError());
        }
    }

    TlsVector = Teb->ThreadLocalStoragePointer;
    ok(TlsVector[1] != NULL, "ThreadLocalStoragePointer index 1 is NULL; Implicit TLS unavailable\n");

    SetEvent(ThreadEvent);

    TlsIdx0ValueAfter = *(PULONG)TlsVector[0];

    ok(TlsIdx0Value == TlsIdx0ValueAfter, "Value in TLS index 0 corrupted by DLL loads; expected %lu and got %lu\n", TlsIdx0Value, TlsIdx0ValueAfter);

    i = 0;

    while(1)
    {
        if(TlsVector[i] && !((ULONG_PTR)TlsVector[i] % 4))
             ++i;
        else
            break;
    }
    ok(i == TLS_VECTOR_MAX_SIZE + ExtantTlsEntryCount, "ThreadLocalStoragePointer length is %d, expected length %lu\n", i, TLS_VECTOR_MAX_SIZE + ExtantTlsEntryCount);

    SetEvent(ThreadEvent);

    for (i = 0; i < TLS_VECTOR_MAX_SIZE; i++)
    {
        StringCchPrintfW(basedllname, MAX_PATH, L"basedllname_%d", i);
        Length = GetTempFileNameW(workdir, basedllname, 0, duplicatepath);
        ok(Length != 0, "GetTempFileNameW failed with %lu\n", GetLastError());
        IsSuccess = DeleteFileW(duplicatepath);
        ok(IsSuccess, "DeleteFileW failed with %lu\n", GetLastError());
        FreeLibrary(DllAddr[i]);
    }

    DeleteFileW(dllpath_implicit_tls);
}