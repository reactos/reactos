/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for exception behavior in dll notifications
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

WCHAR dllpath[MAX_PATH];

LONG g_TlsCalled = 0;
LONG g_DllMainCalled = 0;

LONG g_TlsExcept = 0xffffff;
LONG g_DllMainExcept = 0xffffff;

ULONG g_BaseHandlers = 0;

DWORD g_dwWinVer = 0;

ULONG CountHandlers(VOID)
{
    EXCEPTION_REGISTRATION_RECORD* exc;
    ULONG Count = 0;

    exc = NtCurrentTeb()->NtTib.ExceptionList;

    while (exc && exc != (EXCEPTION_REGISTRATION_RECORD*)~0)
    {
        Count++;
        exc = exc->Next;
    }

    return Count;
}

int g_TLS_ATTACH = 4;
int g_TLS_DETACH = 3;

VOID WINAPI notify_TlsCallback(IN HINSTANCE hDllHandle, IN DWORD dwReason, IN LPVOID lpvReserved)
{
    ULONG handlers = CountHandlers() - g_BaseHandlers;

    InterlockedIncrement(&g_TlsCalled);
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        ok_int(handlers, g_TLS_ATTACH);
    }
    else
    {
        ok_int(handlers, g_TLS_DETACH);
    }

    if (InterlockedCompareExchange(&g_TlsExcept, 0xffffff, dwReason) == dwReason)
    {
        RaiseException(EXCEPTION_DATATYPE_MISALIGNMENT, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
}

int g_DLL_ATTACH = 3;
int g_DLL_DETACH = 2;

BOOL WINAPI notify_DllMain(IN HINSTANCE hDllHandle, IN DWORD dwReason, IN LPVOID lpvReserved)
{
    ULONG handlers = CountHandlers() - g_BaseHandlers;

    InterlockedIncrement(&g_DllMainCalled);
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        ok_int(handlers, g_DLL_ATTACH);
    }
    else
    {
        ok_int(handlers, g_DLL_DETACH); // For failures, see https://jira.reactos.org/browse/CORE-14857
    }

    if (InterlockedCompareExchange(&g_DllMainExcept, 0xffffff, dwReason) == dwReason)
    {
        RaiseException(EXCEPTION_DATATYPE_MISALIGNMENT, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }
    return TRUE;
}


static void execute_test(void)
{
    HMODULE mod;
    DWORD dwErr;
    _SEH2_TRY
    {
        g_TlsExcept = 0xffffff;
        g_DllMainExcept = 0xffffff;
        g_DllMainCalled = 0;
        g_TlsCalled = 0;
        g_BaseHandlers = CountHandlers();
        mod = LoadLibraryW(dllpath);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) != NULL, "Unable to load module (0x%lx)\n", dwErr);
        ok_hex(g_DllMainCalled, 1);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 1);
        if (g_TlsCalled == 0)
            trace("Tls not active\n");
        g_BaseHandlers = CountHandlers();
        FreeLibrary(mod);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) == NULL, "Unable to unload module (0x%lx)\n", dwErr);
        ok_hex(g_DllMainCalled, 2);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 2);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(0, "Unable to load it normally\n");
    }
    _SEH2_END;


    _SEH2_TRY
    {
        g_TlsExcept = 0xffffff;
        g_DllMainExcept = DLL_PROCESS_ATTACH;
        g_DllMainCalled = 0;
        g_TlsCalled = 0;
        g_BaseHandlers = CountHandlers();
        mod = LoadLibraryW(dllpath);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) == NULL, "Module loaded (0x%lx)\n", dwErr);
        if (g_dwWinVer <= _WIN32_WINNT_WIN7)
            ok_hex(dwErr, ERROR_NOACCESS);
        else
            ok_hex(dwErr, ERROR_DLL_INIT_FAILED);
        ok_hex(g_DllMainCalled, 1);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 1);
        if (mod)
        {
            FreeLibrary(mod);
            dwErr = GetLastError();
            ok(GetModuleHandleW(dllpath) == NULL, "Unable to unload module (0x%lx)\n", dwErr);
            ok_hex(g_DllMainCalled, 1);
            if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
                ok_hex(g_TlsCalled, 1);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(0, "Unable to execute test\n");
    }
    _SEH2_END;

    _SEH2_TRY
    {
        g_TlsExcept = 0xffffff;
        g_DllMainExcept = DLL_PROCESS_DETACH;
        g_DllMainCalled = 0;
        g_TlsCalled = 0;
        g_BaseHandlers = CountHandlers();
        mod = LoadLibraryW(dllpath);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) != NULL, "Unable to load module (0x%lx)\n", dwErr);
        ok_hex(g_DllMainCalled, 1);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 1);
        g_BaseHandlers = CountHandlers();
        FreeLibrary(mod);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) == NULL, "Unable to unload module (0x%lx)\n", dwErr);
        ok_hex(g_DllMainCalled, 2);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 2);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(0, "Unable to execute test\n");
    }
    _SEH2_END;

    _SEH2_TRY
    {
        g_TlsExcept = DLL_PROCESS_ATTACH;
        g_DllMainExcept = 0xffffff;
        g_DllMainCalled = 0;
        g_TlsCalled = 0;
        g_BaseHandlers = CountHandlers();
        mod = LoadLibraryW(dllpath);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) != NULL, "Unable to load module (0x%lx)\n", dwErr);
        ok_hex(g_DllMainCalled, 1);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 1);
        g_BaseHandlers = CountHandlers();
        FreeLibrary(mod);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) == NULL, "Unable to unload module (0x%lx)\n", dwErr);
        ok_hex(g_DllMainCalled, 2);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 2);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(0, "Unable to execute test\n");
    }
    _SEH2_END;

    _SEH2_TRY
    {
        g_TlsExcept = DLL_PROCESS_DETACH;
        g_DllMainExcept = 0xffffff;
        g_DllMainCalled = 0;
        g_TlsCalled = 0;
        g_BaseHandlers = CountHandlers();
        mod = LoadLibraryW(dllpath);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) != NULL, "Unable to load module (0x%lx)\n", dwErr);
        ok_hex(g_DllMainCalled, 1);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 1);
        g_BaseHandlers = CountHandlers();
        FreeLibrary(mod);
        dwErr = GetLastError();
        ok(GetModuleHandleW(dllpath) == NULL, "Unable to unload module (0x%lx)\n", dwErr);
        ok_hex(g_DllMainCalled, 2);
        if (g_dwWinVer > _WIN32_WINNT_WS03 || g_TlsCalled)
            ok_hex(g_TlsCalled, 2);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(0, "Unable to execute test\n");
    }
    _SEH2_END;

}


BOOL extract_resource(const WCHAR* Filename, LPCWSTR ResourceName)
{
    BOOL Success;
    DWORD dwWritten, Size;
    HGLOBAL hGlobal;
    LPVOID pData;
    HANDLE Handle;
    HRSRC hRsrc = FindResourceW(GetModuleHandleW(NULL), ResourceName, (LPCWSTR)10);
    ok(!!hRsrc, "Unable to find %s\n", wine_dbgstr_w(ResourceName));
    if (!hRsrc)
        return FALSE;

    hGlobal = LoadResource(GetModuleHandleW(NULL), hRsrc);
    Size = SizeofResource(GetModuleHandleW(NULL), hRsrc);
    pData = LockResource(hGlobal);

    ok(Size && !!pData, "Unable to load %s\n", wine_dbgstr_w(ResourceName));
    if (!Size || !pData)
        return FALSE;

    Handle = CreateFileW(Filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (Handle == INVALID_HANDLE_VALUE)
    {
        skip("Failed to create temp file %ls, error %lu\n", Filename, GetLastError());
        return FALSE;
    }
    Success = WriteFile(Handle, pData, Size, &dwWritten, NULL);
    ok(Success == TRUE, "WriteFile failed with %lu\n", GetLastError());
    ok(dwWritten == Size, "WriteFile wrote %lu bytes instead of %lu\n", dwWritten, Size);
    CloseHandle(Handle);
    Success = Success && (dwWritten == Size);

    UnlockResource(pData);
    return Success;
}


START_TEST(load_notifications)
{
    WCHAR workdir[MAX_PATH];
    BOOL ret;
    UINT Length;
    PPEB Peb = NtCurrentPeb();

    g_dwWinVer = (DWORD)(Peb->OSMajorVersion << 8) | Peb->OSMinorVersion;
    trace("Winver: 0x%lx\n", g_dwWinVer);

    if (g_dwWinVer <= _WIN32_WINNT_WS03)
    {
        g_DLL_ATTACH = 4;
        g_DLL_DETACH = 1;
    }
    else if (g_dwWinVer <= _WIN32_WINNT_WS08)
    {
        g_TLS_ATTACH = 5;
        g_DLL_ATTACH = 4;
    }
    else if (g_dwWinVer <= _WIN32_WINNT_WIN7)
    {
        g_TLS_ATTACH = 3;
        g_DLL_ATTACH = 2;
    }
    else if (g_dwWinVer <= _WIN32_WINNT_WINBLUE)
    {
        g_TLS_DETACH = 5;
        g_DLL_DETACH = 4;
    }

    ret = GetTempPathW(_countof(workdir), workdir);
    ok(ret, "GetTempPathW error: %lu\n", GetLastError());

    Length = GetTempFileNameW(workdir, L"ntdll", 0, dllpath);
    ok(Length != 0, "GetTempFileNameW failed with %lu\n", GetLastError());

    if (extract_resource(dllpath, (LPCWSTR)101))
    {
        _SEH2_TRY
        {
            execute_test();
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ok(0, "Ldr didnt handle exception\n");
        }
        _SEH2_END;
    }
    else
    {
        ok(0, "Failed to extract resource\n");
    }

    DeleteFileW(dllpath);
}
