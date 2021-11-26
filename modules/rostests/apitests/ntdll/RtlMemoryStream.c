/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the Rtl*MemoryStream* series of functions
 * PROGRAMMER:      David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

#define COBJMACROS
#include <ole2.h>
#include <wtypes.h>

ULONG finalReleaseCallCount = 0;

VOID
NTAPI
CustomFinalReleaseMemoryStream(PRTL_MEMORY_STREAM stream)
{
    finalReleaseCallCount++;
    trace("FinalRelease CALLED.\n");
}

VOID
NTAPI
CustomFinalReleaseOutOfProcessMemoryStream(PRTL_MEMORY_STREAM stream)
{
    finalReleaseCallCount++;
    trace("FinalRelease CALLED.\n");
    RtlFinalReleaseOutOfProcessMemoryStream(stream);
}

BOOL CompareStructsAndSaveForLater(PRTL_MEMORY_STREAM pold, PRTL_MEMORY_STREAM pnew, PSTR at)
{
    BOOL equal = TRUE;

    // Compare
    if (pold->Vtbl != pnew->Vtbl) {  if (equal) { trace("%s:\n", at); equal = FALSE; } trace("Vtbl changed from %p to %p\n", pold->Vtbl, pnew->Vtbl);}
    if (pold->RefCount != pnew->RefCount) {  if (equal) { trace("%s:\n", at); equal = FALSE; } trace("RefCount changed from %ld to %ld\n", pold->RefCount, pnew->RefCount); }
    if (pold->Unk1 != pnew->Unk1) {  if (equal) { trace("%s:\n", at); equal = FALSE; } trace("Unk1 changed from %lu to %lu\n", pold->Unk1, pnew->Unk1); }
    if (pold->Current != pnew->Current) {  if (equal) { trace("%s:\n", at); equal = FALSE; } trace("Current changed from %p to %p\n", pold->Current, pnew->Current); }
    if (pold->Start != pnew->Start) {  if (equal) { trace("%s:\n", at); equal = FALSE; } trace("Start changed from %p to %p\n", pold->Start, pnew->Start); }
    if (pold->End != pnew->End) {  if (equal) { trace("%s:\n", at); equal = FALSE; } trace("End changed from %p to %p\n", pold->End, pnew->End); }
    if (pold->FinalRelease != pnew->FinalRelease) {  if (equal) { trace("%s:\n", at); equal = FALSE; } trace("FinalRelease changed from %p to %p\n", pold->FinalRelease, pnew->FinalRelease); }
    if (pold->ProcessHandle != pnew->ProcessHandle) {  if (equal) { trace("%s:\n", at); equal = FALSE; } trace("ProcessHandle changed from %p to %p\n", pold->ProcessHandle, pnew->ProcessHandle); }

    // Save
    pold->Vtbl = pnew->Vtbl;
    pold->RefCount = pnew->RefCount;
    pold->Unk1 = pnew->Unk1;
    pold->Current = pnew->Current;
    pold->Start = pnew->Start;
    pold->End = pnew->End;
    pold->FinalRelease = pnew->FinalRelease;
    pold->ProcessHandle = pnew->ProcessHandle;

    return equal;
}

void test_InProcess()
{
    LARGE_INTEGER move;
    ULARGE_INTEGER size;
    HRESULT res;
    ULONG i;

    RTL_MEMORY_STREAM stream;
    RTL_MEMORY_STREAM previous;

    IStream * istream;

    UCHAR buffer[80];
    UCHAR buffer2[180];
    ULONG bytesRead;

    STATSTG stat;

    finalReleaseCallCount = 0;

    for (i = 0; i < sizeof(buffer2); i++)
    {
        buffer2[i] = i % UCHAR_MAX;
    }

    memset(&stream, 0x90, sizeof(stream));
    memset(&previous, 0x00, sizeof(previous));

    StartSeh()
        RtlInitMemoryStream(NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        RtlInitMemoryStream(&stream);
    EndSeh(STATUS_SUCCESS);

    CompareStructsAndSaveForLater(&previous, &stream, "After init");

    ok(stream.RefCount == 0, "RefCount has a wrong value: %ld (expected %d).\n", stream.RefCount, 0);

    stream.Current = buffer2;
    stream.Start = buffer2;
    stream.End = buffer2 + sizeof(buffer2);
    stream.FinalRelease = CustomFinalReleaseMemoryStream;

    CompareStructsAndSaveForLater(&previous, &stream, "After assigning");

    StartSeh()
        IStream_QueryInterface((struct IStream*)&stream, NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        IStream_QueryInterface((struct IStream*)&stream, &IID_IStream, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        IStream_QueryInterface((struct IStream*)&stream, NULL, (void**)&istream);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        res = IStream_QueryInterface((struct IStream*)&stream, &IID_IStream, (void**)&istream);
        ok(res == S_OK, "QueryInterface to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(stream.RefCount == 2, "RefCount has a wrong value: %ld (expected %d).\n", stream.RefCount, 2);
    EndSeh(STATUS_SUCCESS);

    CompareStructsAndSaveForLater(&previous, &stream, "After QueryInterface");

    StartSeh()
        res = IStream_Stat(istream, NULL, 0);
        ok(res == STG_E_INVALIDPOINTER, "Stat to IStream returned wrong hResult: 0x%08lx.\n", res);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        res = IStream_Stat(istream, &stat, STATFLAG_NONAME);
        ok(res == S_OK, "Stat to IStream returned wrong hResult: 0x%08lx.\n", res);
    EndSeh(STATUS_SUCCESS);

    ok(stream.Current == buffer2,
       "stream.Current points to the wrong address 0x%p (expected 0x%p)\n",
       stream.Current, buffer2);
    ok(stream.Start == buffer2, "stream.Start was changed unexpectedly\n");
    ok(stream.End == buffer2 + sizeof(buffer2), "stream.End was changed unexpectedly\n");

    ok(stat.cbSize.QuadPart == ((PUCHAR)stream.End - (PUCHAR)stream.Start),
       "stat.cbSize has the wrong value %I64u (expected %d)\n",
       stat.cbSize.QuadPart, (PUCHAR)stream.End - (PUCHAR)stream.Start);

    CompareStructsAndSaveForLater(&previous, &stream, "After Stat");

    StartSeh()
        res = IStream_AddRef(istream);
        ok(res == 3, "AddRef to IStream returned wrong hResult: %ld.\n", res);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        res = IStream_AddRef(istream);
        ok(res == 4, "AddRef to IStream returned wrong hResult: %ld.\n", res);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        res = IStream_Release(istream);
        ok(res == 3, "Release to IStream returned wrong hResult: %ld.\n", res);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        res = IStream_AddRef(istream);
        ok(res == 4, "AddRef to IStream returned wrong hResult: %ld.\n", res);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        res = IStream_Release(istream);
        ok(res == 3, "Release to IStream returned wrong hResult: %ld.\n", res);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        res = IStream_Release(istream);
        ok(res == 2, "Release to IStream returned wrong hResult: %ld.\n", res);
    EndSeh(STATUS_SUCCESS);

    CompareStructsAndSaveForLater(&previous, &stream, "After AddRef");

    StartSeh()
        res = IStream_Read(istream, NULL, 0, &bytesRead);
        ok(res == S_OK, "Read to IStream returned wrong hResult: 0x%08lx.\n", res);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        res = IStream_Read(istream, buffer, 40, NULL);
        ok(res == S_OK, "Read to IStream returned wrong hResult: 0x%08lx.\n", res);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        res = IStream_Read(istream, buffer + 40, 39, &bytesRead);
        ok(res == S_OK, "Read to IStream returned wrong hResult: 0x%08lx.\n", res);
    EndSeh(STATUS_SUCCESS);

    if (SUCCEEDED(res))
    {
        bytesRead += 40;
        for (i = 0; i < bytesRead; i++)
        {
            ok(buffer[i] == i, "Buffer[%lu] contains a wrong number %u (expected %lu).\n", i, buffer[i], i);
        }
    }

    ok(stream.Current == buffer2 + 79,
       "stream.Current points to the wrong address 0x%p (expected 0x%p)\n",
       stream.Current, buffer2);
    ok(stream.Start == buffer2, "stream.Start was changed unexpectedly\n");
    ok(stream.End == buffer2 + sizeof(buffer2), "stream.End was changed unexpectedly\n");

    CompareStructsAndSaveForLater(&previous, &stream, "After Read 1");

    size.QuadPart = 0x9090909090909090ull;

    StartSeh()
        move.QuadPart = -1;
        res = IStream_Seek(istream, move, STREAM_SEEK_END, &size);
        ok(res == STG_E_INVALIDPOINTER, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0x9090909090909090ull, "Seek modified the new location in an error (0x%08lx,0x%08lx).\n", size.HighPart, size.LowPart);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        move.QuadPart = 0;
        res = IStream_Seek(istream, move, STREAM_SEEK_END, &size);
        ok(res == S_OK, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == (PUCHAR)stream.End - (PUCHAR)stream.Start, "Seek new location unexpected value: 0x%08lx.\n", size.LowPart);
    EndSeh(STATUS_SUCCESS);

    size.QuadPart = 0x9090909090909090ull;

    StartSeh()
        move.QuadPart = 1;
        res = IStream_Seek(istream, move, STREAM_SEEK_END, &size);
        ok(res == S_OK, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == (PUCHAR)stream.End - (PUCHAR)stream.Start - 1, "Seek new location unexpected value: 0x%08lx.\n", size.LowPart);
    EndSeh(STATUS_SUCCESS);

    size.QuadPart = 0x9090909090909090ull;

    StartSeh()
        move.QuadPart = 2;
        res = IStream_Seek(istream, move, STREAM_SEEK_END, &size);
        ok(res == S_OK, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == (PUCHAR)stream.End - (PUCHAR)stream.Start - 2, "Seek new location unexpected value: 0x%08lx.\n", size.LowPart);
    EndSeh(STATUS_SUCCESS);

    size.QuadPart = 0x9090909090909090ull;

    StartSeh()
        move.QuadPart = -20;
        res = IStream_Seek(istream, move, STREAM_SEEK_SET, &size);
        ok(res == STG_E_INVALIDPOINTER, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0x9090909090909090ull, "Seek modified the new location in an error.\n");
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        move.QuadPart = 4000;
        res = IStream_Seek(istream, move, STREAM_SEEK_SET, &size);
        ok(res == STG_E_INVALIDPOINTER, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0x9090909090909090ull, "Seek modified the new location in an error.\n");
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        move.QuadPart = 0x100000000ull;
        res = IStream_Seek(istream, move, STREAM_SEEK_SET, &size);
#ifdef _WIN64
        ok(res == STG_E_INVALIDPOINTER, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0x9090909090909090ull, "Seek modified the new location in an error (0x%08lx,0x%08lx).\n", size.HighPart, size.LowPart);
#else
        ok(res == S_OK, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0, "Seek new location unexpected value: 0x%08lx.\n", size.LowPart);
#endif
    EndSeh(STATUS_SUCCESS);

#ifdef _WIN64
    StartSeh()
        move.QuadPart = 0;
        res = IStream_Seek(istream, move, STREAM_SEEK_SET, &size);
        ok(res == S_OK, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0, "Seek new location unexpected value: 0x%08lx.\n", size.LowPart);
    EndSeh(STATUS_SUCCESS);
#endif

    size.QuadPart = 0x9090909090909090ull;

    StartSeh()
        move.QuadPart = -20;
        res = IStream_Seek(istream, move, STREAM_SEEK_CUR, &size);
        ok(res == STG_E_INVALIDPOINTER, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0x9090909090909090ull, "Seek modified the new location in an error (0x%08lx,0x%08lx).\n", size.HighPart, size.LowPart);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        move.QuadPart = 0x100000000ull;
        res = IStream_Seek(istream, move, STREAM_SEEK_CUR, &size);
#ifdef _WIN64
        ok(res == STG_E_INVALIDPOINTER, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0x9090909090909090ull, "Seek modified the new location in an error (0x%08lx,0x%08lx).\n", size.HighPart, size.LowPart);
#else
        ok(res == S_OK, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
        ok(size.QuadPart == 0, "Seek new location unexpected value: 0x%08lx.\n", size.LowPart);
#endif
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        move.QuadPart = 40;
        res = IStream_Seek(istream, move, STREAM_SEEK_SET, &size);
        ok(res == S_OK, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);
    EndSeh(STATUS_SUCCESS);

    ok(size.QuadPart == 40,
       "Seek returned wrong offset %I64u (expected %d)\n",
       size.QuadPart, 40);

    ok(stream.Current == buffer2 + 40,
       "stream.Current points to the wrong address 0x%p (expected 0x%p)\n",
       stream.Current, buffer2);
    ok(stream.Start == buffer2, "stream.Start was changed unexpectedly\n");
    ok(stream.End == buffer2 + sizeof(buffer2), "stream.End was changed unexpectedly\n");

    CompareStructsAndSaveForLater(&previous, &stream, "After Seek");

    res = IStream_Read(istream, buffer, sizeof(buffer), &bytesRead);

    ok(res == S_OK, "Read to IStream returned wrong hResult: 0x%08lx.\n", res);

    if (SUCCEEDED(res))
    {
        for (i = 0; i < bytesRead; i++)
        {
            ok(buffer[i] == (i + 40), "Buffer[%lu] contains a wrong number %u (expected %lu).\n", i, buffer[i], i + 40);
        }
    }

    ok(stream.Current == buffer2 + 40 + sizeof(buffer),
       "stream.Current points to the wrong address 0x%p (expected 0x%p)\n",
       stream.Current, buffer2);
    ok(stream.Start == buffer2, "stream.Start was changed unexpectedly\n");
    ok(stream.End == buffer2 + sizeof(buffer2), "stream.End was changed unexpectedly\n");

    CompareStructsAndSaveForLater(&previous, &stream, "After Read 2");

    res = IStream_Release(istream);

    ok(res == 1, "Release to IStream returned wrong hResult: 0x%08lx.\n", res);

    ok(stream.RefCount == 1, "RefCount has a wrong value: %ld (expected %d).\n", stream.RefCount, 1);

    res = IStream_Release(istream);

    ok(res == S_OK, "Release to IStream returned wrong hResult: 0x%08lx.\n", res);

    ok(stream.RefCount == 0, "RefCount has a wrong value: %ld (expected %d).\n", stream.RefCount, 0);

    ok(finalReleaseCallCount == 1, "FinalRelease was called %lu times instead of 1.\n", finalReleaseCallCount);
}

void test_OutOfProcess()
{
    LARGE_INTEGER move;
    ULARGE_INTEGER size;
    HRESULT res;
    HANDLE process;
    ULONG i;

    RTL_MEMORY_STREAM stream;
    RTL_MEMORY_STREAM previous;

    IStream * istream;

    UCHAR buffer[80];
    UCHAR buffer2[180];
    ULONG bytesRead;

    STATSTG stat;

    finalReleaseCallCount = 0;

    for (i = 0; i < sizeof(buffer2); i++)
    {
        buffer2[i] = i % UCHAR_MAX;
    }

    memset(&stream, 0x90, sizeof(stream));
    memset(&previous, 0x00, sizeof(previous));

    process = GetCurrentProcess();

    RtlInitOutOfProcessMemoryStream(&stream);

    ok(stream.FinalRelease == RtlFinalReleaseOutOfProcessMemoryStream,
       "stream.FinalRelease unexpected %p != %p.\n",
       stream.FinalRelease, RtlFinalReleaseOutOfProcessMemoryStream);

    ok(stream.RefCount == 0, "RefCount has a wrong value: %ld (expected %d).\n", stream.RefCount, 0);

    CompareStructsAndSaveForLater(&previous, &stream, "After init");

    stream.Current = buffer2;
    stream.Start = buffer2;
    stream.End = buffer2 + sizeof(buffer2);
    stream.ProcessHandle = process;
    stream.FinalRelease = CustomFinalReleaseOutOfProcessMemoryStream;

    CompareStructsAndSaveForLater(&previous, &stream, "After assigning");

    res = IStream_QueryInterface((struct IStream*)&stream, &IID_IStream, (void**)&istream);

    ok(res == S_OK, "QueryInterface to IStream returned wrong hResult: 0x%08lx.\n", res);

    ok(stream.RefCount == 1, "RefCount has a wrong value: %ld (expected %d).\n", stream.RefCount, 1);

    ok(stream.ProcessHandle == process,
       "ProcessHandle changed unexpectedly: 0x%p (expected 0x%p)\n",
       stream.ProcessHandle, process);

    CompareStructsAndSaveForLater(&previous, &stream, "After QueryInterface");

    res = IStream_Stat(istream, &stat, STATFLAG_NONAME);

    ok(res == S_OK, "Stat to IStream returned wrong hResult: 0x%08lx.\n", res);

    ok(stream.Current == buffer2,
       "stream.Current points to the wrong address 0x%p (expected 0x%p)\n",
       stream.Current, buffer2);
    ok(stream.Start == buffer2, "stream.Start was changed unexpectedly\n");
    ok(stream.End == buffer2 + sizeof(buffer2), "stream.End was changed unexpectedly\n");
    ok(stream.ProcessHandle == process,
       "ProcessHandle changed unexpectedly: 0x%p (expected 0x%p)\n",
       stream.ProcessHandle, process);

    ok(stat.cbSize.QuadPart == ((PUCHAR)stream.End - (PUCHAR)stream.Start),
       "stat.cbSize has the wrong value %I64u (expected %d)\n",
       stat.cbSize.QuadPart, (PUCHAR)stream.End - (PUCHAR)stream.Start);

    CompareStructsAndSaveForLater(&previous, &stream, "After Stat");

    res = IStream_Read(istream, buffer, sizeof(buffer), &bytesRead);

    ok(res == S_OK, "Read to IStream returned wrong hResult: 0x%08lx.\n", res);

    if (SUCCEEDED(res))
    {
        for (i = 0; i < bytesRead; i++)
        {
            ok(buffer[i] == i, "Buffer[%lu] contains a wrong number %u (expected %lu).\n", i, buffer[i], i);
        }
    }

    ok(stream.Current == buffer2 + sizeof(buffer),
       "stream.Current points to the wrong address 0x%p (expected 0x%p)\n",
       stream.Current, buffer2);
    ok(stream.Start == buffer2, "stream.Start was changed unexpectedly\n");
    ok(stream.End == buffer2 + sizeof(buffer2), "stream.End was changed unexpectedly\n");
    ok(stream.ProcessHandle == process,
       "ProcessHandle changed unexpectedly: 0x%p (expected 0x%p)\n",
       stream.ProcessHandle, process);

    CompareStructsAndSaveForLater(&previous, &stream, "After Read 1");

    move.QuadPart = 40;

    res = IStream_Seek(istream, move, STREAM_SEEK_SET, &size);

    ok(res == S_OK, "Seek to IStream returned wrong hResult: 0x%08lx.\n", res);

    ok(size.QuadPart == 40,
       "Seek returned wrong offset %I64u (expected %d)\n",
       size.QuadPart, 40);

    ok(stream.Current == buffer2 + 40,
       "stream.Current points to the wrong address 0x%p (expected 0x%p)\n",
       stream.Current, buffer2);
    ok(stream.Start == buffer2, "stream.Start was changed unexpectedly\n");
    ok(stream.End == buffer2 + sizeof(buffer2), "stream.End was changed unexpectedly\n");
    ok(stream.ProcessHandle == process,
       "ProcessHandle changed unexpectedly: 0x%p (expected 0x%p)\n",
       stream.ProcessHandle, process);

    CompareStructsAndSaveForLater(&previous, &stream, "After Seek");

    res = IStream_Read(istream, buffer, sizeof(buffer), &bytesRead);

    ok(res == S_OK, "Read to IStream returned wrong hResult: 0x%08lx.\n", res);

    if (SUCCEEDED(res))
    {
        for (i = 0; i < bytesRead; i++)
        {
            ok(buffer[i] == (i + 40), "Buffer[%lu] contains a wrong number %u (expected %lu).\n", i, buffer[i], i + 40);
        }
    }

    ok(stream.Current == buffer2 + 40 + sizeof(buffer),
       "stream.Current points to the wrong address 0x%p (expected 0x%p)\n",
       stream.Current, buffer2);
    ok(stream.Start == buffer2, "stream.Start was changed unexpectedly\n");
    ok(stream.End == buffer2 + sizeof(buffer2), "stream.End was changed unexpectedly\n");
    ok(stream.ProcessHandle == process,
       "ProcessHandle changed unexpectedly: 0x%p (expected 0x%p)\n",
       stream.ProcessHandle, process);

    CompareStructsAndSaveForLater(&previous, &stream, "After Read 2");

    res = IStream_Release(istream);

    ok(res == S_OK, "Release to IStream returned wrong hResult: 0x%08lx.\n", res);

    ok(stream.RefCount == 0, "RefCount has a wrong value: %ld (expected %d).\n", stream.RefCount, 0);

    ok(finalReleaseCallCount == 1, "FinalRelease was called %lu times instead of 1.\n", finalReleaseCallCount);
}

START_TEST(RtlMemoryStream)
{
    test_InProcess();
    test_OutOfProcess();
}
