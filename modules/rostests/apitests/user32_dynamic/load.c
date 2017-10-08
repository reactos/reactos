/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for loading/unloading user32.dll
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <ndk/pstypes.h>
#include <ndk/rtlfuncs.h>

START_TEST(load)
{
    HMODULE hUser32;
    HMODULE hUser32_2;
    BOOL Ret;
    DWORD Error;
    PPEB Peb = NtCurrentPeb();
    PTEB Teb = NtCurrentTeb();

    /* Before init */
    hUser32 = GetModuleHandleW(L"user32");
    ok(hUser32 == NULL, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable == NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);

    SetLastError(12345);
    hUser32 = LoadLibraryW(L"user32");
    Error = GetLastError();
    ok(hUser32 != NULL, "LoadLibrary failed\n");
    ok(Error != 12345, "Error = %lu\n", Error);

    /* Initialized */
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);

    SetLastError(12345);
    hUser32_2 = LoadLibraryW(L"user32");
    Error = GetLastError();
    ok(hUser32_2 == hUser32, "LoadLibrary failed\n");
    ok(Error == 12345, "Error = %lu\n", Error);

    /* Still initialized */
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);

    SetLastError(12345);
    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == TRUE, "FreeLibrary returned %d\n", Ret);
    ok(Error == 12345, "Error = %lu\n", Error);

    /* Still initialized */
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);

    SetLastError(12345);
    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == TRUE, "FreeLibrary returned %d\n", Ret);
    ok(Error == 12345, "Error = %lu\n", Error);

    /* Somebody kept an extra reference! */
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);

    SetLastError(12345);
    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == TRUE, "FreeLibrary returned %d\n", Ret);
    ok(Error == 12345, "Error = %lu\n", Error);

    /* Uninitialized */
    hUser32 = GetModuleHandleW(L"user32");
    ok(hUser32 == NULL, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);

    SetLastError(12345);
    hUser32 = LoadLibraryW(L"user32");
    Error = GetLastError();
    ok(hUser32 != NULL, "LoadLibrary failed\n");
    ok(Error != 12345, "Error = %lu\n", Error);

    /* Initialized again */
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);

    SetLastError(12345);
    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == TRUE, "FreeLibrary returned %d\n", Ret);
    ok(Error == 12345, "Error = %lu\n", Error);

    /* Extra reference again */
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);

    SetLastError(12345);
    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == TRUE, "FreeLibrary returned %d\n", Ret);
    ok(Error == 12345, "Error = %lu\n", Error);

    /* Uninitialized again */
    hUser32 = GetModuleHandleW(L"user32");
    ok(hUser32 == NULL, "hUser32 = %p\n", hUser32);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    ok(Teb->Win32ClientInfo != NULL, "Win32ClientInfo = %p\n", Teb->Win32ClientInfo);
}
