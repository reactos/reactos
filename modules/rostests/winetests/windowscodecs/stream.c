/*
 * Copyright 2009 Tony Wasserka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/test.h"

#define COBJMACROS
#include "wincodec.h"

#define CHECK_CUR_POS(a, b) _check_cur_pos((IStream *)a, b, FALSE, __LINE__)
#define CHECK_CUR_POS_TODO(a, b) _check_cur_pos((IStream *)a, b, TRUE, __LINE__)
static void _check_cur_pos(IStream *stream, ULONGLONG expected_pos, BOOL todo, unsigned int line)
{
    LARGE_INTEGER offset;
    ULARGE_INTEGER pos;
    HRESULT hr;

    offset.QuadPart = 0;
    hr = IStream_Seek(stream, offset, STREAM_SEEK_CUR, &pos);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get current position, hr %#lx.\n", hr);
    todo_wine_if(todo)
    ok_(__FILE__, line)(pos.QuadPart == expected_pos, "Unexpected stream position %s.\n",
        wine_dbgstr_longlong(pos.QuadPart));
}

static void test_StreamOnMemory(void)
{
    IWICImagingFactory *pFactory;
    IWICStream *pStream, *pBufStream;
    const BYTE CmpMem[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    };
    const BYTE CmpMemOverlap[] = {
        0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    };
    const BYTE ZeroMem[10] = {0};
    BYTE Memory[64], MemBuf[64];
    LARGE_INTEGER LargeNull, LargeInt, SeekPos;
    ULARGE_INTEGER uLargeNull, uNewPos;
    ULONG uBytesRead, uBytesWritten;
    HRESULT hr;
    STATSTG Stats;

    LargeNull.QuadPart = 0;
    uLargeNull.QuadPart = 0;
    SeekPos.QuadPart = 5;

    memcpy(Memory, CmpMem, sizeof(CmpMem));

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&pFactory);
    if(FAILED(hr)) {
        skip("CoCreateInstance returned with %#lx, expected %#lx\n", hr, S_OK);
        return;
    }

    hr = IWICImagingFactory_CreateStream(pFactory, &pStream);
    ok(hr == S_OK, "CreateStream returned with %#lx, expected %#lx\n", hr, S_OK);
    if(FAILED(hr)) {
        skip("Failed to create stream\n");
        return;
    }

    /* InitializeFromMemory */
    hr = IWICStream_InitializeFromMemory(pStream, NULL, sizeof(Memory));   /* memory = NULL */
    ok(hr == E_INVALIDARG, "InitializeFromMemory returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, 0);   /* size = 0 */
    ok(hr == S_OK, "InitializeFromMemory returned with %#lx, expected %#lx\n", hr, S_OK);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, sizeof(Memory));   /* stream already initialized */
    ok(hr == WINCODEC_ERR_WRONGSTATE, "InitializeFromMemory returned with %#lx, expected %#lx\n", hr, WINCODEC_ERR_WRONGSTATE);

    /* recreate stream */
    IWICStream_Release(pStream);
    hr = IWICImagingFactory_CreateStream(pFactory, &pStream);
    ok(hr == S_OK, "CreateStream failed with %#lx\n", hr);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, sizeof(Memory));
    ok(hr == S_OK, "InitializeFromMemory returned with %#lx, expected %#lx\n", hr, S_OK);

    /* IWICStream does not maintain an independent copy of the backing memory buffer. */
    memcpy(Memory, ZeroMem, sizeof(ZeroMem));
    hr = IWICStream_Read(pStream, MemBuf, sizeof(ZeroMem), &uBytesRead);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == sizeof(ZeroMem), "Read %lu bytes\n", uBytesRead);
        ok(memcmp(MemBuf, ZeroMem, sizeof(ZeroMem)) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Write(pStream, CmpMem, sizeof(CmpMem), &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);

    /* Seek */
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#lx, expected %#lx\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Seek returned with %#lx, expected %#lx\n", hr, S_OK);

    LargeInt.u.HighPart = 1;
    LargeInt.u.LowPart = 0;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "Seek returned with %#lx, expected %#lx\n", hr, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    CHECK_CUR_POS(pStream, 0);

    LargeInt.QuadPart = sizeof(Memory) + 10;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == E_INVALIDARG, "Seek returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    CHECK_CUR_POS(pStream, 0);

    LargeInt.QuadPart = 1;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == E_INVALIDARG, "Seek returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    CHECK_CUR_POS(pStream, 0);

    LargeInt.QuadPart = -1;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#lx, expected %#lx\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == sizeof(Memory) - 1, "bSeek cursor moved to position (%lu;%lu)\n", uNewPos.u.HighPart, uNewPos.u.LowPart);

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, &uNewPos); /* reset seek pointer */
    LargeInt.QuadPart = -(LONGLONG)sizeof(Memory) - 5;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW),
       "Seek returned with %#lx, expected %#lx\n", hr, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    CHECK_CUR_POS(pStream, 0);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    /* Read */
    hr = IWICStream_Read(pStream, MemBuf, 12, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 12, "Read %lu bytes, expected %u\n", uBytesRead, 12);
        ok(memcmp(MemBuf, CmpMem, 12) == 0, "Read returned invalid data!\n");

        /* check whether the seek pointer has moved correctly */
        CHECK_CUR_POS(pStream, uBytesRead);
    }

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pStream, Memory, 10, &uBytesRead);   /* source = dest */
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 10, "Read %lu bytes, expected %u\n", uBytesRead, 10);
        ok(memcmp(Memory, CmpMem, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pStream, SeekPos, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pStream, Memory, 10, &uBytesRead);   /* source and dest overlap */
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 10, "Read %lu bytes, expected %u\n", uBytesRead, 10);
        ok(memcmp(Memory, CmpMemOverlap, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    memcpy(Memory, CmpMem, sizeof(CmpMem));

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pStream, Memory, sizeof(Memory) + 10, &uBytesRead);   /* request too many bytes */
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == sizeof(Memory), "Read %lu bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    hr = IWICStream_Read(pStream, NULL, 1, &uBytesRead);    /* destination buffer = NULL */
    ok(hr == E_INVALIDARG, "Read returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, MemBuf, 0, &uBytesRead);    /* read 0 bytes */
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);

    hr = IWICStream_Read(pStream, NULL, 0, &uBytesRead);
    ok(hr == E_INVALIDARG, "Read returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Read returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, MemBuf, 1, NULL);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);
    ZeroMemory(MemBuf, sizeof(MemBuf));
    hr = IWICStream_Read(pStream, MemBuf, sizeof(Memory) + 10, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == sizeof(Memory), "Read %lu bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem, 64) == 0, "Read returned invalid data!\n");
    }
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Write */
    MemBuf[0] = CmpMem[0] + 1;
    MemBuf[1] = CmpMem[1] + 1;
    MemBuf[2] = CmpMem[2] + 1;
    hr = IWICStream_Write(pStream, MemBuf, 3, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesWritten == 3, "Wrote %lu bytes, expected %u\n", uBytesWritten, 3);
        ok(memcmp(MemBuf, Memory, 3) == 0, "Wrote returned invalid data!\n"); /* make sure we're writing directly */

        /* check whether the seek pointer has moved correctly */
        CHECK_CUR_POS(pStream, uBytesWritten);
    }
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Write(pStream, MemBuf, 0, &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);

    /* Restore the original contents of the memory stream. */
    hr = IWICStream_Write(pStream, CmpMem, sizeof(CmpMem), &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    /* Source and destination overlap. */
    hr = IWICStream_Write(pStream, Memory + 5, 10, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesWritten == 10, "Wrote %lu bytes, expected %u\n", uBytesWritten, 10);
        ok(memcmp(CmpMemOverlap, Memory, sizeof(CmpMemOverlap)) == 0, "Wrote returned invalid data!\n");
    }

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pStream, NULL, 3, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %lu\n", uBytesWritten);
    CHECK_CUR_POS(pStream, 0);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pStream, NULL, 0, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %lu\n", uBytesWritten);
    CHECK_CUR_POS(pStream, 0);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pStream, CmpMem, sizeof(Memory) + 10, &uBytesWritten);
    ok(hr == STG_E_MEDIUMFULL, "Write returned with %#lx, expected %#lx\n", hr, STG_E_MEDIUMFULL);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %lu\n", uBytesWritten);
    CHECK_CUR_POS(pStream, 0);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* SetSize */
    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory) + 10;
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory);
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory) - 10;
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = 0;
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    uNewPos.QuadPart = -10;
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);


    /* CopyTo */
    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = 5;
    hr = IWICStream_CopyTo(pStream, NULL, uNewPos, NULL, NULL);
    ok(hr == E_NOTIMPL, "CopyTo returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    hr = IWICImagingFactory_CreateStream(pFactory, &pBufStream);
    ok(hr == S_OK, "CreateStream failed with %#lx\n", hr);

    hr = IWICStream_InitializeFromMemory(pBufStream, Memory, sizeof(Memory));
    ok(hr == S_OK, "InitializeFromMemory returned with %#lx, expected %#lx\n", hr, S_OK);

    hr = IWICStream_CopyTo(pStream, (IStream*)pBufStream, uNewPos, NULL, NULL);
    ok(hr == E_NOTIMPL, "CopyTo returned %#lx, expected %#lx\n", hr, E_NOTIMPL);
    IWICStream_Release(pBufStream);


    /* Commit */
    hr = IWICStream_Commit(pStream, STGC_DEFAULT);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(pStream, STGC_OVERWRITE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(pStream, STGC_ONLYIFCURRENT);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(pStream, STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(pStream, STGC_CONSOLIDATE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);


    /* Revert */
    IWICStream_Write(pStream, &MemBuf[5], 6, NULL);
    hr = IWICStream_Revert(pStream);
    ok(hr == E_NOTIMPL, "Revert returned %#lx, expected %#lx\n", hr, E_NOTIMPL);
    memcpy(Memory, CmpMem, sizeof(Memory));


    /* LockRegion/UnlockRegion */
    hr = IWICStream_LockRegion(pStream, uLargeNull, uLargeNull, 0);
    ok(hr == E_NOTIMPL, "LockRegion returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    hr = IWICStream_UnlockRegion(pStream, uLargeNull, uLargeNull, 0);
    ok(hr == E_NOTIMPL, "UnlockRegion returned %#lx, expected %#lx\n", hr, E_NOTIMPL);


    /* Stat */
    hr = IWICStream_Stat(pStream, NULL, 0);
    ok(hr == E_INVALIDARG, "Stat returned %#lx, expected %#lx\n", hr, E_INVALIDARG);

    hr = IWICStream_Stat(pStream, &Stats, 0);
    ok(hr == S_OK, "Stat returned %#lx, expected %#lx\n", hr, S_OK);
    ok(Stats.pwcsName == NULL, "Stat returned name %p, expected %p\n", Stats.pwcsName, NULL);
    ok(Stats.type == STGTY_STREAM, "Stat returned type %ld, expected %d\n", Stats.type, STGTY_STREAM);
    ok(Stats.cbSize.u.HighPart == 0 && Stats.cbSize.u.LowPart == sizeof(Memory), "Stat returned size (%lu;%lu)\n", Stats.cbSize.u.HighPart, Stats.cbSize.u.LowPart);
    ok(Stats.mtime.dwHighDateTime == 0 && Stats.mtime.dwLowDateTime == 0, "Stat returned mtime (%lu;%lu), expected (%u;%u)\n", Stats.mtime.dwHighDateTime, Stats.mtime.dwLowDateTime, 0, 0);
    ok(Stats.ctime.dwHighDateTime == 0 && Stats.ctime.dwLowDateTime == 0, "Stat returned ctime (%lu;%lu), expected (%u;%u)\n", Stats.ctime.dwHighDateTime, Stats.ctime.dwLowDateTime, 0, 0);
    ok(Stats.atime.dwHighDateTime == 0 && Stats.atime.dwLowDateTime == 0, "Stat returned atime (%lu;%lu), expected (%u;%u)\n", Stats.atime.dwHighDateTime, Stats.atime.dwLowDateTime, 0, 0);
    ok(Stats.grfMode == 0, "Stat returned access mode %ld, expected %d\n", Stats.grfMode, 0);
    ok(Stats.grfLocksSupported == 0, "Stat returned supported locks %#lx, expected %#x\n", Stats.grfLocksSupported, 0);
    ok(Stats.grfStateBits == 0, "Stat returned state bits %#lx, expected %#x\n", Stats.grfStateBits, 0);


    /* Clone */
    hr = IWICStream_Clone(pStream, (IStream**)&pBufStream);
    ok(hr == E_NOTIMPL, "UnlockRegion returned %#lx, expected %#lx\n", hr, E_NOTIMPL);


    IWICStream_Release(pStream);
    IWICImagingFactory_Release(pFactory);
}

static void test_StreamOnStreamRange(void)
{
    IWICImagingFactory *pFactory;
    IWICStream *pStream, *pSubStream;
    IStream *CopyStream;
    const BYTE CmpMem[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    };
    BYTE Memory[64], MemBuf[64];
    LARGE_INTEGER LargeNull, LargeInt;
    ULARGE_INTEGER uLargeNull, uNewPos, uSize;
    ULONG uBytesRead, uBytesWritten;
    HRESULT hr;
    STATSTG Stats;

    LargeNull.QuadPart = 0;
    uLargeNull.QuadPart = 0;

    memcpy(Memory, CmpMem, sizeof(CmpMem));

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&pFactory);
    if(FAILED(hr)) {
        skip("CoCreateInstance returned with %#lx, expected %#lx\n", hr, S_OK);
        return;
    }

    hr = IWICImagingFactory_CreateStream(pFactory, &pStream);
    ok(hr == S_OK, "CreateStream returned with %#lx, expected %#lx\n", hr, S_OK);
    if(FAILED(hr)) {
        skip("Failed to create stream\n");
        return;
    }

    hr = IWICStream_InitializeFromMemory(pStream, Memory, sizeof(Memory));
    ok(hr == S_OK, "InitializeFromMemory returned with %#lx, expected %#lx\n", hr, S_OK);

    hr = IWICImagingFactory_CreateStream(pFactory, &pSubStream);
    ok(hr == S_OK, "CreateStream returned with %#lx, expected %#lx\n", hr, S_OK);

    uNewPos.QuadPart = 20;
    uSize.QuadPart = 20;
    hr = IWICStream_InitializeFromIStreamRegion(pSubStream, (IStream*)pStream, uNewPos, uSize);
    ok(hr == S_OK, "InitializeFromIStreamRegion returned with %#lx, expected %#lx\n", hr, S_OK);
    if(FAILED(hr)) {
        skip("InitializeFromIStreamRegion unimplemented\n");
        IWICStream_Release(pSubStream);
        IWICStream_Release(pStream);
        IWICImagingFactory_Release(pFactory);
        CoUninitialize();
        return;
    }

    /* Seek */
    CHECK_CUR_POS(pStream, 0);
    hr = IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_END, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 20, "Seek cursor moved to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 20);
    ok(hr == S_OK, "Seek returned with %#lx, expected %#lx\n", hr, S_OK);
    CHECK_CUR_POS(pStream, 0);

    hr = IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#lx, expected %#lx\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);
    CHECK_CUR_POS(pStream, 0);

    hr = IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Seek returned with %#lx, expected %#lx\n", hr, S_OK);

    LargeInt.u.HighPart = 1;
    LargeInt.u.LowPart = 0;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "Seek returned with %#lx, expected %#lx\n", hr, WINCODEC_ERR_VALUEOUTOFRANGE);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    CHECK_CUR_POS(pStream, 0);

    LargeInt.QuadPart = 30;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "Seek returned with %#lx, expected %#lx\n", hr, WINCODEC_ERR_VALUEOUTOFRANGE);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    CHECK_CUR_POS(pStream, 0);

    LargeInt.QuadPart = 1;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "Seek returned with %#lx, expected %#lx\n", hr, WINCODEC_ERR_VALUEOUTOFRANGE);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    CHECK_CUR_POS(pStream, 0);

    LargeInt.QuadPart = -1;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#lx, expected %#lx\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 19, "bSeek cursor moved to position (%lu;%lu)\n", uNewPos.u.HighPart, uNewPos.u.LowPart);
    CHECK_CUR_POS(pStream, 0);

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, &uNewPos); /* reset seek pointer */
    LargeInt.QuadPart = -25;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE,
       "Seek returned with %#lx, expected %#lx\n", hr, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    CHECK_CUR_POS(pStream, 0);
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Read */
    hr = IWICStream_Read(pSubStream, MemBuf, 12, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 12, "Read %lu bytes, expected %u\n", uBytesRead, 12);
        ok(memcmp(MemBuf, CmpMem+20, 12) == 0, "Read returned invalid data!\n");

        /* check whether the seek pointer has moved correctly */
        CHECK_CUR_POS(pSubStream, uBytesRead);
        CHECK_CUR_POS(pStream, 0);
    }

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pSubStream, Memory, 10, &uBytesRead);   /* source = dest */
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 10, "Read %lu bytes, expected %u\n", uBytesRead, 10);
        ok(memcmp(Memory, CmpMem+20, uBytesRead) == 0, "Read returned invalid data!\n");
    }
    CHECK_CUR_POS(pStream, 0);

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pSubStream, Memory, 30, &uBytesRead);   /* request too many bytes */
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 20, "Read %lu bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem+20, uBytesRead) == 0, "Read returned invalid data!\n");
    }
    CHECK_CUR_POS(pStream, 0);

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    uBytesRead = 0xdeadbeef;
    hr = IWICStream_Read(pSubStream, NULL, 1, &uBytesRead);    /* destination buffer = NULL */
    ok(hr == E_INVALIDARG, "Read returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);
    ok(uBytesRead == 0xdeadbeef, "Expected uBytesRead to be unchanged, got %lu\n", uBytesRead);

    hr = IWICStream_Read(pSubStream, MemBuf, 0, &uBytesRead);    /* read 0 bytes */
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);

    uBytesRead = 0xdeadbeef;
    hr = IWICStream_Read(pSubStream, NULL, 0, &uBytesRead);
    ok(hr == E_INVALIDARG, "Read returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);
    ok(uBytesRead == 0xdeadbeef, "Expected uBytesRead to be unchanged, got %lu\n", uBytesRead);

    hr = IWICStream_Read(pSubStream, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Read returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pSubStream, MemBuf, 1, NULL);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    ZeroMemory(MemBuf, sizeof(MemBuf));
    hr = IWICStream_Read(pSubStream, MemBuf, 30, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 20, "Read %lu bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem+20, 20) == 0, "Read returned invalid data!\n");
    }
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Write */
    MemBuf[0] = CmpMem[0] + 1;
    MemBuf[1] = CmpMem[1] + 1;
    MemBuf[2] = CmpMem[2] + 1;
    hr = IWICStream_Write(pSubStream, MemBuf, 3, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesWritten == 3, "Wrote %lu bytes, expected %u\n", uBytesWritten, 3);
        ok(memcmp(MemBuf, Memory+20, 3) == 0, "Wrote returned invalid data!\n"); /* make sure we're writing directly */

        /* check whether the seek pointer has moved correctly */
        CHECK_CUR_POS(pSubStream, uBytesWritten);
        CHECK_CUR_POS(pStream, 0);
    }
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Write(pSubStream, MemBuf, 0, &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pSubStream, NULL, 3, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %lu\n", uBytesWritten);
    CHECK_CUR_POS(pSubStream, 0);
    CHECK_CUR_POS(pStream, 0);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pSubStream, NULL, 0, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#lx, expected %#lx\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %lu\n", uBytesWritten);
    CHECK_CUR_POS(pSubStream, 0);
    CHECK_CUR_POS(pStream, 0);

    hr = IWICStream_Write(pSubStream, CmpMem, 30, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#lx, expected %#lx\n", hr, STG_E_MEDIUMFULL);
    ok(uBytesWritten == 20, "Wrote %lu bytes, expected %u\n", uBytesWritten, 0);
    CHECK_CUR_POS(pSubStream, uBytesWritten);
    CHECK_CUR_POS(pStream, 0);
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* SetSize */
    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory) + 10;
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory);
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory) - 10;
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = 0;
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    uNewPos.QuadPart = -10;
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#lx, expected %#lx\n", hr, E_NOTIMPL);


    /* CopyTo */
    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = 30;
    hr = IWICStream_CopyTo(pSubStream, NULL, uNewPos, NULL, NULL);
    ok(hr == E_NOTIMPL, "CopyTo returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &CopyStream);
    ok(hr == S_OK, "CreateStream failed with %#lx\n", hr);

    hr = IWICStream_CopyTo(pSubStream, CopyStream, uNewPos, NULL, NULL);
    ok(hr == E_NOTIMPL, "CopyTo returned %#lx, expected %#lx\n", hr, E_NOTIMPL);
    IStream_Release(CopyStream);


    /* Commit */
    hr = IWICStream_Commit(pSubStream, STGC_DEFAULT);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(pSubStream, STGC_OVERWRITE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(pSubStream, STGC_ONLYIFCURRENT);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(pSubStream, STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(pSubStream, STGC_CONSOLIDATE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);


    /* Revert */
    IWICStream_Write(pSubStream, &MemBuf[5], 6, NULL);
    hr = IWICStream_Revert(pSubStream);
    ok(hr == E_NOTIMPL, "Revert returned %#lx, expected %#lx\n", hr, E_NOTIMPL);
    memcpy(Memory, CmpMem, sizeof(Memory));


    /* LockRegion/UnlockRegion */
    hr = IWICStream_LockRegion(pSubStream, uLargeNull, uLargeNull, 0);
    ok(hr == E_NOTIMPL, "LockRegion returned %#lx, expected %#lx\n", hr, E_NOTIMPL);

    hr = IWICStream_UnlockRegion(pSubStream, uLargeNull, uLargeNull, 0);
    ok(hr == E_NOTIMPL, "UnlockRegion returned %#lx, expected %#lx\n", hr, E_NOTIMPL);


    /* Stat */
    hr = IWICStream_Stat(pSubStream, NULL, 0);
    ok(hr == E_INVALIDARG, "Stat returned %#lx, expected %#lx\n", hr, E_INVALIDARG);

    hr = IWICStream_Stat(pSubStream, &Stats, 0);
    ok(hr == S_OK, "Stat returned %#lx, expected %#lx\n", hr, S_OK);
    ok(Stats.pwcsName == NULL, "Stat returned name %p, expected %p\n", Stats.pwcsName, NULL);
    ok(Stats.type == STGTY_STREAM, "Stat returned type %ld, expected %d\n", Stats.type, STGTY_STREAM);
    ok(Stats.cbSize.u.HighPart == 0 && Stats.cbSize.u.LowPart == 20, "Stat returned size (%lu;%lu)\n", Stats.cbSize.u.HighPart, Stats.cbSize.u.LowPart);
    ok(Stats.mtime.dwHighDateTime == 0 && Stats.mtime.dwLowDateTime == 0, "Stat returned mtime (%lu;%lu), expected (%u;%u)\n", Stats.mtime.dwHighDateTime, Stats.mtime.dwLowDateTime, 0, 0);
    ok(Stats.ctime.dwHighDateTime == 0 && Stats.ctime.dwLowDateTime == 0, "Stat returned ctime (%lu;%lu), expected (%u;%u)\n", Stats.ctime.dwHighDateTime, Stats.ctime.dwLowDateTime, 0, 0);
    ok(Stats.atime.dwHighDateTime == 0 && Stats.atime.dwLowDateTime == 0, "Stat returned atime (%lu;%lu), expected (%u;%u)\n", Stats.atime.dwHighDateTime, Stats.atime.dwLowDateTime, 0, 0);
    ok(Stats.grfMode == 0, "Stat returned access mode %ld, expected %d\n", Stats.grfMode, 0);
    ok(Stats.grfLocksSupported == 0, "Stat returned supported locks %#lx, expected %#x\n", Stats.grfLocksSupported, 0);
    ok(Stats.grfStateBits == 0, "Stat returned state bits %#lx, expected %#x\n", Stats.grfStateBits, 0);


    /* Clone */
    hr = IWICStream_Clone(pSubStream, &CopyStream);
    ok(hr == E_NOTIMPL, "Clone returned %#lx, expected %#lx\n", hr, E_NOTIMPL);


    IWICStream_Release(pSubStream);


    /* Recreate, this time larger than the original. */
    hr = IWICImagingFactory_CreateStream(pFactory, &pSubStream);
    ok(hr == S_OK, "CreateStream returned with %#lx, expected %#lx\n", hr, S_OK);

    uNewPos.QuadPart = 48;
    uSize.QuadPart = 32;
    hr = IWICStream_InitializeFromIStreamRegion(pSubStream, (IStream*)pStream, uNewPos, uSize);
    ok(hr == S_OK, "InitializeFromMemory returned with %#lx, expected %#lx\n", hr, S_OK);

    hr = IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_END, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#lx, expected %#lx\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 16, "Seek cursor moved to position (%lu;%lu), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 16);

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    hr = IWICStream_Read(pSubStream, Memory, 48, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#lx, expected %#lx\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 16, "Read %lu bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem+48, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pSubStream, CmpMem, 32, &uBytesWritten);
    ok(hr == STG_E_MEDIUMFULL, "Write returned with %#lx, expected %#lx\n", hr, STG_E_MEDIUMFULL);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %lu\n", uBytesWritten);
    CHECK_CUR_POS(pSubStream, 0);
    CHECK_CUR_POS(pStream, 0);

    IWICStream_Release(pSubStream);
    IWICStream_Release(pStream);
    IWICImagingFactory_Release(pFactory);
    CoUninitialize();
}

static void test_StreamOnIStream(void)
{
    static const BYTE data[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    };
    static const LARGE_INTEGER zero_pos;
    static const ULARGE_INTEGER uzero;
    IWICStream *stream, *substream;
    IWICImagingFactory *factory;
    BYTE memory[64], buff[64];
    ULONG read_len, written;
    ULARGE_INTEGER newpos;
    IStream *copy_stream;
    LARGE_INTEGER pos;
    unsigned int i;
    STATSTG stats;
    HRESULT hr;

    memcpy(memory, data, sizeof(data));

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "Failed to create a factory, hr %#lx.\n", hr);

    hr = IWICImagingFactory_CreateStream(factory, &stream);
    ok(hr == S_OK, "Failed to create a stream, hr %#lx.\n", hr);

    hr = IWICStream_InitializeFromMemory(stream, memory, sizeof(memory));
    ok(hr == S_OK, "Failed to initialize stream, hr %#lx.\n", hr);

    hr = IWICImagingFactory_CreateStream(factory, &substream);
    ok(hr == S_OK, "Failed to create a stream, hr %#lx.\n", hr);

    pos.QuadPart = 1;
    hr = IWICStream_Seek(stream, pos, STREAM_SEEK_SET, &newpos);
    ok(hr == S_OK, "Failed to set position, hr %#lx.\n", hr);
    CHECK_CUR_POS(stream, 1);

    hr = IWICStream_InitializeFromIStream(substream, (IStream *)stream);
    ok(hr == S_OK, "Failed to initialize stream, hr %#lx.\n", hr);
    CHECK_CUR_POS(substream, 1);

    /* Seek */
    CHECK_CUR_POS(stream, 1);
    hr = IWICStream_Seek(substream, zero_pos, STREAM_SEEK_END, &newpos);
    ok(hr == S_OK, "Failed to seek a stream, hr %#lx.\n", hr);
    ok(newpos.QuadPart == sizeof(memory), "Unexpected position %s.\n", wine_dbgstr_longlong(newpos.QuadPart));
    CHECK_CUR_POS(substream, sizeof(memory));
    CHECK_CUR_POS(stream, sizeof(memory));

    hr = IWICStream_Seek(substream, zero_pos, STREAM_SEEK_SET, &newpos);
    ok(hr == S_OK, "Failed to seek a stream, hr %#lx.\n", hr);
    ok(newpos.QuadPart == 0, "Unexpected position %s.\n", wine_dbgstr_longlong(newpos.QuadPart));
    CHECK_CUR_POS(stream, 0);
    CHECK_CUR_POS(substream, 0);

    hr = IWICStream_Seek(substream, zero_pos, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Failed to seek a stream, hr %#lx.\n", hr);

    pos.u.HighPart = 1;
    pos.u.LowPart = 0;
    newpos.u.HighPart = 0xdeadbeef;
    newpos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(substream, pos, STREAM_SEEK_SET, &newpos);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "Unexpected hr %#lx.\n", hr);
    ok(newpos.u.HighPart == 0xdeadbeef && newpos.u.LowPart == 0xdeadbeef, "Unexpected position %s.\n",
        wine_dbgstr_longlong(newpos.QuadPart));
    CHECK_CUR_POS(stream, 0);
    CHECK_CUR_POS(substream, 0);

    pos.QuadPart = sizeof(memory) + 1;
    newpos.u.HighPart = 0xdeadbeef;
    newpos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(substream, pos, STREAM_SEEK_SET, &newpos);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(newpos.u.HighPart == 0xdeadbeef && newpos.u.LowPart == 0xdeadbeef, "Unexpected position %s.\n",
        wine_dbgstr_longlong(newpos.QuadPart));
    CHECK_CUR_POS(stream, 0);
    CHECK_CUR_POS(substream, 0);

    pos.QuadPart = 1;
    newpos.u.HighPart = 0xdeadbeef;
    newpos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(substream, pos, STREAM_SEEK_END, &newpos);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(newpos.u.HighPart == 0xdeadbeef && newpos.u.LowPart == 0xdeadbeef, "Unexpected position %s.\n",
        wine_dbgstr_longlong(newpos.QuadPart));
    CHECK_CUR_POS(stream, 0);
    CHECK_CUR_POS(substream, 0);

    pos.QuadPart = -1;
    hr = IWICStream_Seek(substream, pos, STREAM_SEEK_END, &newpos);
    ok(hr == S_OK, "Failed to seek a stream, hr %#lx.\n", hr);
    ok(newpos.QuadPart == sizeof(memory) - 1, "Unexpected position %s.\n", wine_dbgstr_longlong(newpos.QuadPart));
    CHECK_CUR_POS(stream, sizeof(memory) - 1);
    CHECK_CUR_POS(substream, sizeof(memory) - 1);

    IWICStream_Seek(substream, zero_pos, STREAM_SEEK_SET, NULL);

    /* Read */
    hr = IWICStream_Read(substream, buff, 12, &read_len);
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
    ok(read_len == 12, "Unexpected read length %lu.\n", read_len);
    ok(!memcmp(buff, data, 12), "Unexpected data.\n");
    CHECK_CUR_POS(substream, read_len);
    CHECK_CUR_POS(stream, read_len);

    IWICStream_Seek(substream, zero_pos, STREAM_SEEK_SET, NULL);
    CHECK_CUR_POS(stream, 0);

    hr = IWICStream_Read(substream, memory, 10, &read_len);   /* source = dest */
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
    ok(read_len == 10, "Unexpected read length %lu.\n", read_len);
    ok(!memcmp(memory, data, read_len), "Unexpected data.\n");
    CHECK_CUR_POS(stream, 10);

    IWICStream_Seek(substream, zero_pos, STREAM_SEEK_SET, NULL);
    hr = IWICStream_Read(substream, memory, 2 * sizeof(data), &read_len);   /* request too many bytes */
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
    ok(read_len == 64, "Unexpected read length %lu.\n", read_len);
    ok(!memcmp(memory, data, read_len), "Unexpected data.\n");
    CHECK_CUR_POS(stream, sizeof(data));

    IWICStream_Seek(substream, zero_pos, STREAM_SEEK_SET, NULL);
    read_len = 0xdeadbeef;
    hr = IWICStream_Read(substream, NULL, 1, &read_len);    /* destination buffer = NULL */
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(read_len == 0xdeadbeef, "Unexpected read length %lu.\n", read_len);

    read_len = 1;
    hr = IWICStream_Read(substream, buff, 0, &read_len);    /* read 0 bytes */
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
    ok(read_len == 0, "Unexpected read length %lu.\n", read_len);

    read_len = 0xdeadbeef;
    hr = IWICStream_Read(substream, NULL, 0, &read_len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(read_len == 0xdeadbeef, "Unexpected read length %lu.\n", read_len);

    hr = IWICStream_Read(substream, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICStream_Read(substream, buff, 1, NULL);
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
    CHECK_CUR_POS(substream, 1);
    CHECK_CUR_POS(stream, 1);
    IWICStream_Seek(substream, zero_pos, STREAM_SEEK_SET, NULL);

    /* Write */
    for (i = 0; i < 3; ++i)
        buff[i] = data[i] + 1;

    hr = IWICStream_Write(substream, buff, 3, &written);
    ok(hr == S_OK, "Failed to write to stream, hr %#lx.\n", hr);
    ok(written == 3, "Unexpected written length %lu.\n", written);
    ok(!memcmp(buff, memory, 3), "Unexpected stream data.\n");
    CHECK_CUR_POS(substream, written);
    CHECK_CUR_POS(stream, written);
    IWICStream_Seek(substream, zero_pos, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Write(substream, buff, 0, &written);
    ok(hr == S_OK, "Failed to write to stream, hr %#lx.\n", hr);

    written = 0xdeadbeef;
    hr = IWICStream_Write(substream, NULL, 3, &written);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(written == 0xdeadbeef, "Unexpected written length %lu.\n", written);
    CHECK_CUR_POS(substream, 0);
    CHECK_CUR_POS(stream, 0);

    written = 0xdeadbeef;
    hr = IWICStream_Write(substream, NULL, 0, &written);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(written == 0xdeadbeef, "Unexpected written length %lu.\n", written);
    CHECK_CUR_POS(substream, 0);
    CHECK_CUR_POS(stream, 0);

    /* SetSize */
    newpos.u.HighPart = 0;
    newpos.u.LowPart = sizeof(memory) + 10;
    hr = IWICStream_SetSize(substream, newpos);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    newpos.u.HighPart = 0;
    newpos.u.LowPart = sizeof(memory);
    hr = IWICStream_SetSize(substream, newpos);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    newpos.u.HighPart = 0;
    newpos.u.LowPart = sizeof(memory) - 10;
    hr = IWICStream_SetSize(substream, newpos);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    newpos.QuadPart = 0;
    hr = IWICStream_SetSize(substream, newpos);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    newpos.QuadPart = -10;
    hr = IWICStream_SetSize(substream, newpos);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    /* CopyTo */
    newpos.u.HighPart = 0;
    newpos.u.LowPart = 30;
    hr = IWICStream_CopyTo(substream, NULL, newpos, NULL, NULL);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &copy_stream);
    ok(hr == S_OK, "Failed to create a stream, hr %#lx.\n", hr);

    hr = IWICStream_CopyTo(substream, copy_stream, newpos, NULL, NULL);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    IStream_Release(copy_stream);

    /* Commit */
    hr = IWICStream_Commit(substream, STGC_DEFAULT);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(substream, STGC_OVERWRITE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(substream, STGC_ONLYIFCURRENT);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(substream, STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    hr = IWICStream_Commit(substream, STGC_CONSOLIDATE);
    ok(broken(hr == E_NOTIMPL) || hr == S_OK, "Commit returned %#lx\n", hr);

    /* Revert */
    IWICStream_Write(substream, buff + 5, 6, NULL);
    hr = IWICStream_Revert(substream);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    memcpy(memory, data, sizeof(memory));

    /* LockRegion/UnlockRegion */
    hr = IWICStream_LockRegion(substream, uzero, uzero, 0);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IWICStream_UnlockRegion(substream, uzero, uzero, 0);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    /* Stat */
    hr = IWICStream_Stat(substream, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IWICStream_Stat(substream, &stats, 0);
    ok(hr == S_OK, "Failed to get stream stats, hr %#lx.\n", hr);
    ok(stats.pwcsName == NULL, "Unexpected name %p.\n", stats.pwcsName);
    ok(stats.type == STGTY_STREAM, "Unexpected type %ld.\n", stats.type);
    ok(stats.cbSize.QuadPart == sizeof(data), "Unexpected size %s.\n", wine_dbgstr_longlong(stats.cbSize.QuadPart));
    ok(stats.mtime.dwHighDateTime == 0 && stats.mtime.dwLowDateTime == 0, "Unexpected mtime (%lu;%lu).\n",
        stats.mtime.dwHighDateTime, stats.mtime.dwLowDateTime);
    ok(stats.ctime.dwHighDateTime == 0 && stats.ctime.dwLowDateTime == 0, "Unexpected ctime (%lu;%lu).\n",
        stats.ctime.dwHighDateTime, stats.ctime.dwLowDateTime);
    ok(stats.atime.dwHighDateTime == 0 && stats.atime.dwLowDateTime == 0, "Unexpected atime (%lu;%lu).\n",
        stats.atime.dwHighDateTime, stats.atime.dwLowDateTime);
    ok(stats.grfMode == 0, "Unexpected mode %ld.\n", stats.grfMode);
    ok(stats.grfLocksSupported == 0, "Unexpected locks support %#lx.\n", stats.grfLocksSupported);
    ok(stats.grfStateBits == 0, "Unexpected state bits %#lx.\n", stats.grfStateBits);

    /* Clone */
    hr = IWICStream_Clone(substream, &copy_stream);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    IWICStream_Release(substream);
    IWICStream_Release(stream);
    IWICImagingFactory_Release(factory);
}

START_TEST(stream)
{
    CoInitialize(NULL);

    test_StreamOnMemory();
    test_StreamOnStreamRange();
    test_StreamOnIStream();

    CoUninitialize();
}
