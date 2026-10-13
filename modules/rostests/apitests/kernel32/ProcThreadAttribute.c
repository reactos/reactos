/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for process/thread attribute lists
 */

#include "precomp.h"

typedef
BOOL
WINAPI
FN_InitializeProcThreadAttributeList(
    _Inout_opt_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    _In_ DWORD dwAttributeCount,
    _In_ DWORD dwFlags,
    _Inout_ PSIZE_T lpSize);

typedef
BOOL
WINAPI
FN_UpdateProcThreadAttribute(
    _In_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
    _In_ DWORD dwFlags,
    _In_ DWORD_PTR Attribute,
    _In_ PVOID lpValue,
    _In_ SIZE_T cbSize,
    _In_ PVOID lpPreviousValue,
    _In_ PSIZE_T lpReturnSize);

typedef
VOID
WINAPI
FN_DeleteProcThreadAttributeList(
    _In_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList);

START_TEST(ProcThreadAttribute)
{
    FN_InitializeProcThreadAttributeList *pInitialize;
    FN_UpdateProcThreadAttribute *pUpdate;
    FN_DeleteProcThreadAttributeList *pDelete;
    HANDLE hKernel32, hPipe;
    LPPROC_THREAD_ATTRIBUTE_LIST list;
    SIZE_T size;
    ULONG uFlags = 0;
    HANDLE hParent = NULL;
    BOOL ret;

    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32)
    {
        skip("kernel32 not loaded?!\n");
        return;
    }

    pInitialize = (FN_InitializeProcThreadAttributeList*)GetProcAddress(hKernel32, "InitializeProcThreadAttributeList");
    pUpdate = (FN_UpdateProcThreadAttribute*)GetProcAddress(hKernel32, "UpdateProcThreadAttribute");
    pDelete = (FN_DeleteProcThreadAttributeList*)GetProcAddress(hKernel32, "DeleteProcThreadAttributeList");

    if (!pInitialize || !pUpdate || !pDelete)
    {
        skip("ProcThreadAttribute functions not exported (pre-Vista / 0x502 builds)\n");
        return;
    }

    /* Query the required buffer size. Windows fails this first call with
     * ERROR_INSUFFICIENT_BUFFER so apps know how much to allocate. */
    size = 0;
    ret = pInitialize(NULL, 2, 0, &size);
    ok(size > 0, "InitializeProcThreadAttributeList returned invalid size %lu\n", (unsigned long)size);
    if (!ret)
    {
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
           "Size query: error %lu, expected ERROR_INSUFFICIENT_BUFFER (%lu)\n",
           GetLastError(), ERROR_INSUFFICIENT_BUFFER);
    }

    list = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, size);
    if (!list)
    {
        skip("HeapAlloc failed\n");
        return;
    }

    /* Now the real initialization */
    ret = pInitialize(list, 2, 0, &size);
    ok(ret, "InitializeProcThreadAttributeList failed with %lu\n", GetLastError());

    /* any handle value will do for the HANDLE_LIST test */
    hPipe = INVALID_HANDLE_VALUE;

    ret = pUpdate(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &hPipe, sizeof(hPipe), NULL, NULL);
    ok(ret, "UpdateProcThreadAttribute(HANDLE_LIST) failed with %lu\n", GetLastError());

    ret = pUpdate(list, 0, PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS, &uFlags, sizeof(uFlags), NULL, NULL);
    ok(ret, "UpdateProcThreadAttribute(EXTENDED_FLAGS) failed with %lu\n", GetLastError());

    /* Duplicating an already stored attribute must fail */
    ret = pUpdate(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &hPipe, sizeof(hPipe), NULL, NULL);
    ok(!ret && GetLastError() == ERROR_OBJECT_NAME_EXISTS,
       "Duplicate attribute: expected ERROR_OBJECT_NAME_EXISTS, got %lu\n", GetLastError());

    /* no room left now, capacity is reached */
    ret = pUpdate(list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hParent, sizeof(hParent), NULL, NULL);
    ok(!ret, "Expected update past capacity to fail, but it succeeded\n");
    ok(GetLastError() == ERROR_GEN_FAILURE, "Capacity error %lu, expected ERROR_GEN_FAILURE\n", GetLastError());

    /* reset the list, then an unknown attribute number is rejected */
    ret = pInitialize(list, 2, 0, &size);
    ok(ret, "Re-InitializeProcThreadAttributeList failed with %lu\n", GetLastError());

    ret = pUpdate(list, 0, (DWORD_PTR)0x1234, &hPipe, sizeof(hPipe), NULL, NULL);
    ok(!ret && GetLastError() == ERROR_NOT_SUPPORTED,
       "Unknown attribute: expected ERROR_NOT_SUPPORTED, got %lu\n", GetLastError());

    /* make sure the whole round trip does not explode */
    pDelete(list);
    pDelete(NULL);

    HeapFree(GetProcessHeap(), 0, list);
}
