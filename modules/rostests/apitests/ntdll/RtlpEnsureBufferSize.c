/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlpEnsureBufferSize
 * PROGRAMMER:      Mark Jansen
 */

#include "precomp.h"

#include <tlhelp32.h>


NTSTATUS (NTAPI *pRtlpEnsureBufferSize)(_In_ ULONG Flags, _Inout_ PRTL_BUFFER Buffer, _In_ SIZE_T RequiredSize);


static BOOL IsBlockFromHeap(HANDLE hHeap, PVOID ptr)
{
    /* Use when this is implemented */
#if 0
    PROCESS_HEAP_ENTRY Entry;
    BOOL ret = FALSE;
    if (!HeapLock(hHeap))
    {
        skip("Unable to lock heap\n");
        return FALSE;
    }

    Entry.lpData = NULL;
    while (!ret && HeapWalk(hHeap, &Entry))
    {
        if ((Entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) &&
            (Entry.lpData == ptr))
        {
            ret = TRUE;
        }
    }

    HeapUnlock(hHeap);
    return ret;
#else
    HEAPENTRY32 he;
    BOOL ret = FALSE;
    HANDLE hHeapSnap = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST, GetCurrentProcessId());

    if (hHeapSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    he.dwSize = sizeof(he);

    if (Heap32First(&he, GetCurrentProcessId(), (DWORD_PTR)hHeap))
    {
        do {
            if ((DWORD_PTR)ptr >= he.dwAddress && (DWORD_PTR)ptr <= (he.dwAddress + he.dwBlockSize))
                ret = TRUE;
        } while (!ret && Heap32Next(&he));
    }

    CloseHandle(hHeapSnap);

    return ret;
#endif
}


START_TEST(RtlpEnsureBufferSize)
{
    RTL_BUFFER Buffer = { 0 };
    ULONG Flag;
    UCHAR StaticBuf[4];
    PVOID tmp;
    BOOL SkipHeapCheck;

    HMODULE mod = GetModuleHandleW(L"ntdll.dll");
    pRtlpEnsureBufferSize = (void *)GetProcAddress(mod, "RtlpEnsureBufferSize");

    if (!pRtlpEnsureBufferSize)
    {
        skip("No RtlpEnsureBufferSize\n");
        return;
    }

    memset(StaticBuf, 0xba, sizeof(StaticBuf));
    RtlInitBuffer(&Buffer, StaticBuf, sizeof(StaticBuf));

    /* NULL buffer yields a failure */
    ok_ntstatus(pRtlpEnsureBufferSize(0, NULL, 0), STATUS_INVALID_PARAMETER);

    /* All flags other than '1' yield a failure */
    for (Flag = 2; Flag; Flag <<= 1)
    {
        ok_ntstatus(pRtlpEnsureBufferSize(Flag, &Buffer, 0), STATUS_INVALID_PARAMETER);
        ok_ptr(Buffer.Buffer, Buffer.StaticBuffer);
        ok_int(Buffer.Size, Buffer.StaticSize);
        ok_ptr(Buffer.StaticBuffer, StaticBuf);
        ok_int(Buffer.StaticSize, sizeof(StaticBuf));
        RtlFreeBuffer(&Buffer);
    }

    ok_ntstatus(pRtlpEnsureBufferSize(0, &Buffer, 0), STATUS_SUCCESS);
    ok_ptr(Buffer.Buffer, Buffer.StaticBuffer);
    ok_int(Buffer.Size, Buffer.StaticSize);
    ok_ptr(Buffer.StaticBuffer, StaticBuf);
    ok_int(Buffer.StaticSize, sizeof(StaticBuf));
    RtlFreeBuffer(&Buffer);

    ok_ntstatus(pRtlpEnsureBufferSize(0, &Buffer, 1), STATUS_SUCCESS);
    ok_ptr(Buffer.Buffer, Buffer.StaticBuffer);
    ok_int(Buffer.Size, Buffer.StaticSize);
    ok_ptr(Buffer.StaticBuffer, StaticBuf);
    ok_int(Buffer.StaticSize, sizeof(StaticBuf));
    RtlFreeBuffer(&Buffer);

    ok_ntstatus(pRtlpEnsureBufferSize(0, &Buffer, 2), STATUS_SUCCESS);
    ok_ptr(Buffer.Buffer, Buffer.StaticBuffer);
    ok_int(Buffer.Size, Buffer.StaticSize);
    ok_ptr(Buffer.StaticBuffer, StaticBuf);
    ok_int(Buffer.StaticSize, sizeof(StaticBuf));
    RtlFreeBuffer(&Buffer);

    ok_ntstatus(pRtlpEnsureBufferSize(0, &Buffer, 3), STATUS_SUCCESS);
    ok_ptr(Buffer.Buffer, Buffer.StaticBuffer);
    ok_int(Buffer.Size, Buffer.StaticSize);
    ok_ptr(Buffer.StaticBuffer, StaticBuf);
    ok_int(Buffer.StaticSize, sizeof(StaticBuf));
    RtlFreeBuffer(&Buffer);

    ok_ntstatus(pRtlpEnsureBufferSize(0, &Buffer, 4), STATUS_SUCCESS);
    ok_ptr(Buffer.Buffer, Buffer.StaticBuffer);
    ok_int(Buffer.Size, Buffer.StaticSize);
    ok_ptr(Buffer.StaticBuffer, StaticBuf);
    ok_int(Buffer.StaticSize, sizeof(StaticBuf));
    RtlFreeBuffer(&Buffer);

    /* Check that IsBlockFromHeap works! */
    tmp = RtlAllocateHeap(RtlGetProcessHeap(), 0, 5);
    SkipHeapCheck = (IsBlockFromHeap(RtlGetProcessHeap(), StaticBuf) != FALSE) ||
        (IsBlockFromHeap(RtlGetProcessHeap(), tmp) != TRUE);
    RtlFreeHeap(RtlGetProcessHeap(), 0, tmp);

    if (SkipHeapCheck)
        skip("Unable to verify the heap used\n");

    /* Allocated is exactly what is asked for, not rounded to nearest whatever */
    ok_ntstatus(pRtlpEnsureBufferSize(0, &Buffer, 5), STATUS_SUCCESS);
    if (!SkipHeapCheck)
        ok_int(IsBlockFromHeap(RtlGetProcessHeap(), Buffer.Buffer), TRUE);
    ok(!memcmp(Buffer.Buffer, StaticBuf, sizeof(StaticBuf)), "Expected First 4 bytes to be the same!\n");
    ok_int(Buffer.Size, 5);
    ok_ptr(Buffer.StaticBuffer, StaticBuf);
    ok_int(Buffer.StaticSize, sizeof(StaticBuf));
    RtlFreeBuffer(&Buffer);

    ok_ntstatus(pRtlpEnsureBufferSize(RTL_SKIP_BUFFER_COPY, &Buffer, 5), STATUS_SUCCESS);
    if (!SkipHeapCheck)
        ok_int(IsBlockFromHeap(RtlGetProcessHeap(), Buffer.Buffer), TRUE);
    ok(memcmp(Buffer.Buffer, StaticBuf, sizeof(StaticBuf)), "Expected First 4 bytes to be different!\n");
    ok_int(Buffer.Size, 5);
    ok_ptr(Buffer.StaticBuffer, StaticBuf);
    ok_int(Buffer.StaticSize, sizeof(StaticBuf));
    RtlFreeBuffer(&Buffer);

    /* Size is not limited to UNICODE_STRING sizes */
    ok_ntstatus(pRtlpEnsureBufferSize(RTL_SKIP_BUFFER_COPY, &Buffer, UNICODE_STRING_MAX_BYTES + 1), STATUS_SUCCESS);
    if (!SkipHeapCheck)
        ok_int(IsBlockFromHeap(RtlGetProcessHeap(), Buffer.Buffer), TRUE);
    ok(memcmp(Buffer.Buffer, StaticBuf, sizeof(StaticBuf)), "Expected First 4 bytes to be different!\n");
    ok_int(Buffer.Size, UNICODE_STRING_MAX_BYTES + 1);
    ok_ptr(Buffer.StaticBuffer, StaticBuf);
    ok_int(Buffer.StaticSize, sizeof(StaticBuf));
    RtlFreeBuffer(&Buffer);
}
