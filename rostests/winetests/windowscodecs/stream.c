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

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&pFactory);
    if(FAILED(hr)) {
        skip("CoCreateInstance returned with %#x, expected %#x\n", hr, S_OK);
        return;
    }

    hr = IWICImagingFactory_CreateStream(pFactory, &pStream);
    ok(hr == S_OK, "CreateStream returned with %#x, expected %#x\n", hr, S_OK);
    if(FAILED(hr)) {
        skip("Failed to create stream\n");
        return;
    }

    /* InitializeFromMemory */
    hr = IWICStream_InitializeFromMemory(pStream, NULL, sizeof(Memory));   /* memory = NULL */
    ok(hr == E_INVALIDARG, "InitializeFromMemory returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, 0);   /* size = 0 */
    ok(hr == S_OK, "InitializeFromMemory returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, sizeof(Memory));   /* stream already initialized */
    ok(hr == WINCODEC_ERR_WRONGSTATE, "InitializeFromMemory returned with %#x, expected %#x\n", hr, WINCODEC_ERR_WRONGSTATE);

    /* recreate stream */
    IWICStream_Release(pStream);
    hr = IWICImagingFactory_CreateStream(pFactory, &pStream);
    ok(hr == S_OK, "CreateStream failed with %#x\n", hr);

    hr = IWICStream_InitializeFromMemory(pStream, Memory, sizeof(Memory));
    ok(hr == S_OK, "InitializeFromMemory returned with %#x, expected %#x\n", hr, S_OK);

    /* IWICStream does not maintain an independent copy of the backing memory buffer. */
    memcpy(Memory, ZeroMem, sizeof(ZeroMem));
    hr = IWICStream_Read(pStream, MemBuf, sizeof(ZeroMem), &uBytesRead);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == sizeof(ZeroMem), "Read %u bytes\n", uBytesRead);
        ok(memcmp(MemBuf, ZeroMem, sizeof(ZeroMem)) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Write(pStream, CmpMem, sizeof(CmpMem), &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    /* Seek */
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);

    LargeInt.u.HighPart = 1;
    LargeInt.u.LowPart = 0;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "Seek returned with %#x, expected %#x\n", hr, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    LargeInt.QuadPart = sizeof(Memory) + 10;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == E_INVALIDARG, "Seek returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    LargeInt.QuadPart = 1;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == E_INVALIDARG, "Seek returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    LargeInt.QuadPart = -1;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == sizeof(Memory) - 1, "bSeek cursor moved to position (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart);

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, &uNewPos); /* reset seek pointer */
    LargeInt.QuadPart = -(LONGLONG)sizeof(Memory) - 5;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW),
       "Seek returned with %#x, expected %#x\n", hr, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0); /* remains unchanged */
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Read */
    hr = IWICStream_Read(pStream, MemBuf, 12, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 12, "Read %u bytes, expected %u\n", uBytesRead, 12);
        ok(memcmp(MemBuf, CmpMem, 12) == 0, "Read returned invalid data!\n");

        /* check whether the seek pointer has moved correctly */
        IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
        ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == uBytesRead, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, uBytesRead);
    }

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pStream, Memory, 10, &uBytesRead);   /* source = dest */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 10, "Read %u bytes, expected %u\n", uBytesRead, 10);
        ok(memcmp(Memory, CmpMem, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pStream, SeekPos, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pStream, Memory, 10, &uBytesRead);   /* source and dest overlap */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 10, "Read %u bytes, expected %u\n", uBytesRead, 10);
        ok(memcmp(Memory, CmpMemOverlap, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    memcpy(Memory, CmpMem, sizeof(CmpMem));

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pStream, Memory, sizeof(Memory) + 10, &uBytesRead);   /* request too many bytes */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == sizeof(Memory), "Read %u bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    hr = IWICStream_Read(pStream, NULL, 1, &uBytesRead);    /* destination buffer = NULL */
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, MemBuf, 0, &uBytesRead);    /* read 0 bytes */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICStream_Read(pStream, NULL, 0, &uBytesRead);
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pStream, MemBuf, 1, NULL);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);
    ZeroMemory(MemBuf, sizeof(MemBuf));
    hr = IWICStream_Read(pStream, MemBuf, sizeof(Memory) + 10, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == sizeof(Memory), "Read %u bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem, 64) == 0, "Read returned invalid data!\n");
    }
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Write */
    MemBuf[0] = CmpMem[0] + 1;
    MemBuf[1] = CmpMem[1] + 1;
    MemBuf[2] = CmpMem[2] + 1;
    hr = IWICStream_Write(pStream, MemBuf, 3, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesWritten == 3, "Wrote %u bytes, expected %u\n", uBytesWritten, 3);
        ok(memcmp(MemBuf, Memory, 3) == 0, "Wrote returned invalid data!\n"); /* make sure we're writing directly */

        /* check whether the seek pointer has moved correctly */
        IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
        ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == uBytesWritten, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, uBytesWritten);
    }
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Write(pStream, MemBuf, 0, &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    /* Restore the original contents of the memory stream. */
    hr = IWICStream_Write(pStream, CmpMem, sizeof(CmpMem), &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    /* Source and destination overlap. */
    hr = IWICStream_Write(pStream, Memory + 5, 10, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesWritten == 10, "Wrote %u bytes, expected %u\n", uBytesWritten, 10);
        ok(memcmp(CmpMemOverlap, Memory, sizeof(CmpMemOverlap)) == 0, "Wrote returned invalid data!\n");
    }

    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pStream, NULL, 3, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %u\n", uBytesWritten);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pStream, NULL, 0, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %u\n", uBytesWritten);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pStream, CmpMem, sizeof(Memory) + 10, &uBytesWritten);
    ok(hr == STG_E_MEDIUMFULL, "Write returned with %#x, expected %#x\n", hr, STG_E_MEDIUMFULL);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %u\n", uBytesWritten);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);
    IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* SetSize */
    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory) + 10;
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory);
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory) - 10;
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = 0;
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);

    uNewPos.QuadPart = -10;
    hr = IWICStream_SetSize(pStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);


    /* CopyTo */
    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = 5;
    hr = IWICStream_CopyTo(pStream, NULL, uNewPos, NULL, NULL);
    ok(hr == E_NOTIMPL, "CopyTo returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICImagingFactory_CreateStream(pFactory, &pBufStream);
    ok(hr == S_OK, "CreateStream failed with %#x\n", hr);

    hr = IWICStream_InitializeFromMemory(pBufStream, Memory, sizeof(Memory));
    ok(hr == S_OK, "InitializeFromMemory returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICStream_CopyTo(pStream, (IStream*)pBufStream, uNewPos, NULL, NULL);
    ok(hr == E_NOTIMPL, "CopyTo returned %#x, expected %#x\n", hr, E_NOTIMPL);
    IWICStream_Release(pBufStream);


    /* Commit */
    hr = IWICStream_Commit(pStream, STGC_DEFAULT);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_Commit(pStream, STGC_OVERWRITE);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_Commit(pStream, STGC_ONLYIFCURRENT);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_Commit(pStream, STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_Commit(pStream, STGC_CONSOLIDATE);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);


    /* Revert */
    IWICStream_Write(pStream, &MemBuf[5], 6, NULL);
    hr = IWICStream_Revert(pStream);
    ok(hr == E_NOTIMPL, "Revert returned %#x, expected %#x\n", hr, E_NOTIMPL);
    memcpy(Memory, CmpMem, sizeof(Memory));


    /* LockRegion/UnlockRegion */
    hr = IWICStream_LockRegion(pStream, uLargeNull, uLargeNull, 0);
    ok(hr == E_NOTIMPL, "LockRegion returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_UnlockRegion(pStream, uLargeNull, uLargeNull, 0);
    ok(hr == E_NOTIMPL, "UnlockRegion returned %#x, expected %#x\n", hr, E_NOTIMPL);


    /* Stat */
    hr = IWICStream_Stat(pStream, NULL, 0);
    ok(hr == E_INVALIDARG, "Stat returned %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Stat(pStream, &Stats, 0);
    ok(hr == S_OK, "Stat returned %#x, expected %#x\n", hr, S_OK);
    ok(Stats.pwcsName == NULL, "Stat returned name %p, expected %p\n", Stats.pwcsName, NULL);
    ok(Stats.type == STGTY_STREAM, "Stat returned type %d, expected %d\n", Stats.type, STGTY_STREAM);
    ok(Stats.cbSize.u.HighPart == 0 && Stats.cbSize.u.LowPart == sizeof(Memory), "Stat returned size (%u;%u)\n", Stats.cbSize.u.HighPart, Stats.cbSize.u.LowPart);
    ok(Stats.mtime.dwHighDateTime == 0 && Stats.mtime.dwLowDateTime == 0, "Stat returned mtime (%u;%u), expected (%u;%u)\n", Stats.mtime.dwHighDateTime, Stats.mtime.dwLowDateTime, 0, 0);
    ok(Stats.ctime.dwHighDateTime == 0 && Stats.ctime.dwLowDateTime == 0, "Stat returned ctime (%u;%u), expected (%u;%u)\n", Stats.ctime.dwHighDateTime, Stats.ctime.dwLowDateTime, 0, 0);
    ok(Stats.atime.dwHighDateTime == 0 && Stats.atime.dwLowDateTime == 0, "Stat returned atime (%u;%u), expected (%u;%u)\n", Stats.atime.dwHighDateTime, Stats.atime.dwLowDateTime, 0, 0);
    ok(Stats.grfMode == 0, "Stat returned access mode %d, expected %d\n", Stats.grfMode, 0);
    ok(Stats.grfLocksSupported == 0, "Stat returned supported locks %#x, expected %#x\n", Stats.grfLocksSupported, 0);
    ok(Stats.grfStateBits == 0, "Stat returned state bits %#x, expected %#x\n", Stats.grfStateBits, 0);


    /* Clone */
    hr = IWICStream_Clone(pStream, (IStream**)&pBufStream);
    ok(hr == E_NOTIMPL, "UnlockRegion returned %#x, expected %#x\n", hr, E_NOTIMPL);


    IWICStream_Release(pStream);
    IWICImagingFactory_Release(pFactory);
    CoUninitialize();
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
        skip("CoCreateInstance returned with %#x, expected %#x\n", hr, S_OK);
        return;
    }

    hr = IWICImagingFactory_CreateStream(pFactory, &pStream);
    ok(hr == S_OK, "CreateStream returned with %#x, expected %#x\n", hr, S_OK);
    if(FAILED(hr)) {
        skip("Failed to create stream\n");
        return;
    }

    hr = IWICStream_InitializeFromMemory(pStream, Memory, sizeof(Memory));
    ok(hr == S_OK, "InitializeFromMemory returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICImagingFactory_CreateStream(pFactory, &pSubStream);
    ok(hr == S_OK, "CreateStream returned with %#x, expected %#x\n", hr, S_OK);

    uNewPos.QuadPart = 20;
    uSize.QuadPart = 20;
    hr = IWICStream_InitializeFromIStreamRegion(pSubStream, (IStream*)pStream, uNewPos, uSize);
    ok(hr == S_OK, "InitializeFromIStreamRegion returned with %#x, expected %#x\n", hr, S_OK);
    if(FAILED(hr)) {
        skip("InitializeFromIStreamRegion unimplemented\n");
        IWICStream_Release(pSubStream);
        IWICStream_Release(pStream);
        IWICImagingFactory_Release(pFactory);
        CoUninitialize();
        return;
    }

    /* Seek */
    hr = IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_END, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 20, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 20);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    hr = IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);

    LargeInt.u.HighPart = 1;
    LargeInt.u.LowPart = 0;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "Seek returned with %#x, expected %#x\n", hr, WINCODEC_ERR_VALUEOUTOFRANGE);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    LargeInt.QuadPart = 30;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_SET, &uNewPos);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "Seek returned with %#x, expected %#x\n", hr, WINCODEC_ERR_VALUEOUTOFRANGE);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    LargeInt.QuadPart = 1;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE, "Seek returned with %#x, expected %#x\n", hr, WINCODEC_ERR_VALUEOUTOFRANGE);
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    LargeInt.QuadPart = -1;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 19, "bSeek cursor moved to position (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart);

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, &uNewPos); /* reset seek pointer */
    LargeInt.QuadPart = -25;
    uNewPos.u.HighPart = 0xdeadbeef;
    uNewPos.u.LowPart = 0xdeadbeef;
    hr = IWICStream_Seek(pSubStream, LargeInt, STREAM_SEEK_END, &uNewPos);
    ok(hr == WINCODEC_ERR_VALUEOUTOFRANGE,
       "Seek returned with %#x, expected %#x\n", hr, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
    ok(uNewPos.u.HighPart == 0xdeadbeef && uNewPos.u.LowPart == 0xdeadbeef, "Seek cursor initialized to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0xdeadbeef, 0xdeadbeef);
    hr = IWICStream_Seek(pStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0); /* remains unchanged */
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Read */
    hr = IWICStream_Read(pSubStream, MemBuf, 12, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 12, "Read %u bytes, expected %u\n", uBytesRead, 12);
        ok(memcmp(MemBuf, CmpMem+20, 12) == 0, "Read returned invalid data!\n");

        /* check whether the seek pointer has moved correctly */
        IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
        ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == uBytesRead, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, uBytesRead);
    }

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pSubStream, Memory, 10, &uBytesRead);   /* source = dest */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 10, "Read %u bytes, expected %u\n", uBytesRead, 10);
        ok(memcmp(Memory, CmpMem+20, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Read(pSubStream, Memory, 30, &uBytesRead);   /* request too many bytes */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 20, "Read %u bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem+20, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    uBytesRead = 0xdeadbeef;
    hr = IWICStream_Read(pSubStream, NULL, 1, &uBytesRead);    /* destination buffer = NULL */
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uBytesRead == 0xdeadbeef, "Expected uBytesRead to be unchanged, got %u\n", uBytesRead);

    hr = IWICStream_Read(pSubStream, MemBuf, 0, &uBytesRead);    /* read 0 bytes */
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    uBytesRead = 0xdeadbeef;
    hr = IWICStream_Read(pSubStream, NULL, 0, &uBytesRead);
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uBytesRead == 0xdeadbeef, "Expected uBytesRead to be unchanged, got %u\n", uBytesRead);

    hr = IWICStream_Read(pSubStream, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Read returned with %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Read(pSubStream, MemBuf, 1, NULL);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    ZeroMemory(MemBuf, sizeof(MemBuf));
    hr = IWICStream_Read(pSubStream, MemBuf, 30, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 20, "Read %u bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem+20, 20) == 0, "Read returned invalid data!\n");
    }
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* Write */
    MemBuf[0] = CmpMem[0] + 1;
    MemBuf[1] = CmpMem[1] + 1;
    MemBuf[2] = CmpMem[2] + 1;
    hr = IWICStream_Write(pSubStream, MemBuf, 3, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesWritten == 3, "Wrote %u bytes, expected %u\n", uBytesWritten, 3);
        ok(memcmp(MemBuf, Memory+20, 3) == 0, "Wrote returned invalid data!\n"); /* make sure we're writing directly */

        /* check whether the seek pointer has moved correctly */
        IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
        ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == uBytesWritten, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, uBytesWritten);
    }
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);

    hr = IWICStream_Write(pSubStream, MemBuf, 0, &uBytesWritten);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pSubStream, NULL, 3, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %u\n", uBytesWritten);
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pSubStream, NULL, 0, &uBytesWritten);
    ok(hr == E_INVALIDARG, "Write returned with %#x, expected %#x\n", hr, E_INVALIDARG);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %u\n", uBytesWritten);
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);

    hr = IWICStream_Write(pSubStream, CmpMem, 30, &uBytesWritten);
    ok(hr == S_OK, "Write returned with %#x, expected %#x\n", hr, STG_E_MEDIUMFULL);
    ok(uBytesWritten == 20, "Wrote %u bytes, expected %u\n", uBytesWritten, 0);
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == uBytesWritten, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, uBytesWritten);
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);


    /* SetSize */
    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory) + 10;
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory);
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = sizeof(Memory) - 10;
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);

    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = 0;
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);

    uNewPos.QuadPart = -10;
    hr = IWICStream_SetSize(pSubStream, uNewPos);
    ok(hr == E_NOTIMPL, "SetSize returned %#x, expected %#x\n", hr, E_NOTIMPL);


    /* CopyTo */
    uNewPos.u.HighPart = 0;
    uNewPos.u.LowPart = 30;
    hr = IWICStream_CopyTo(pSubStream, NULL, uNewPos, NULL, NULL);
    ok(hr == E_NOTIMPL, "CopyTo returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &CopyStream);
    ok(hr == S_OK, "CreateStream failed with %#x\n", hr);

    hr = IWICStream_CopyTo(pSubStream, CopyStream, uNewPos, NULL, NULL);
    ok(hr == E_NOTIMPL, "CopyTo returned %#x, expected %#x\n", hr, E_NOTIMPL);
    IStream_Release(CopyStream);


    /* Commit */
    hr = IWICStream_Commit(pSubStream, STGC_DEFAULT);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_Commit(pSubStream, STGC_OVERWRITE);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_Commit(pSubStream, STGC_ONLYIFCURRENT);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_Commit(pSubStream, STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_Commit(pSubStream, STGC_CONSOLIDATE);
    ok(hr == E_NOTIMPL, "Commit returned %#x, expected %#x\n", hr, E_NOTIMPL);


    /* Revert */
    IWICStream_Write(pSubStream, &MemBuf[5], 6, NULL);
    hr = IWICStream_Revert(pSubStream);
    ok(hr == E_NOTIMPL, "Revert returned %#x, expected %#x\n", hr, E_NOTIMPL);
    memcpy(Memory, CmpMem, sizeof(Memory));


    /* LockRegion/UnlockRegion */
    hr = IWICStream_LockRegion(pSubStream, uLargeNull, uLargeNull, 0);
    ok(hr == E_NOTIMPL, "LockRegion returned %#x, expected %#x\n", hr, E_NOTIMPL);

    hr = IWICStream_UnlockRegion(pSubStream, uLargeNull, uLargeNull, 0);
    ok(hr == E_NOTIMPL, "UnlockRegion returned %#x, expected %#x\n", hr, E_NOTIMPL);


    /* Stat */
    hr = IWICStream_Stat(pSubStream, NULL, 0);
    ok(hr == E_INVALIDARG, "Stat returned %#x, expected %#x\n", hr, E_INVALIDARG);

    hr = IWICStream_Stat(pSubStream, &Stats, 0);
    ok(hr == S_OK, "Stat returned %#x, expected %#x\n", hr, S_OK);
    ok(Stats.pwcsName == NULL, "Stat returned name %p, expected %p\n", Stats.pwcsName, NULL);
    ok(Stats.type == STGTY_STREAM, "Stat returned type %d, expected %d\n", Stats.type, STGTY_STREAM);
    ok(Stats.cbSize.u.HighPart == 0 && Stats.cbSize.u.LowPart == 20, "Stat returned size (%u;%u)\n", Stats.cbSize.u.HighPart, Stats.cbSize.u.LowPart);
    ok(Stats.mtime.dwHighDateTime == 0 && Stats.mtime.dwLowDateTime == 0, "Stat returned mtime (%u;%u), expected (%u;%u)\n", Stats.mtime.dwHighDateTime, Stats.mtime.dwLowDateTime, 0, 0);
    ok(Stats.ctime.dwHighDateTime == 0 && Stats.ctime.dwLowDateTime == 0, "Stat returned ctime (%u;%u), expected (%u;%u)\n", Stats.ctime.dwHighDateTime, Stats.ctime.dwLowDateTime, 0, 0);
    ok(Stats.atime.dwHighDateTime == 0 && Stats.atime.dwLowDateTime == 0, "Stat returned atime (%u;%u), expected (%u;%u)\n", Stats.atime.dwHighDateTime, Stats.atime.dwLowDateTime, 0, 0);
    ok(Stats.grfMode == 0, "Stat returned access mode %d, expected %d\n", Stats.grfMode, 0);
    ok(Stats.grfLocksSupported == 0, "Stat returned supported locks %#x, expected %#x\n", Stats.grfLocksSupported, 0);
    ok(Stats.grfStateBits == 0, "Stat returned state bits %#x, expected %#x\n", Stats.grfStateBits, 0);


    /* Clone */
    hr = IWICStream_Clone(pSubStream, &CopyStream);
    ok(hr == E_NOTIMPL, "Clone returned %#x, expected %#x\n", hr, E_NOTIMPL);


    IWICStream_Release(pSubStream);


    /* Recreate, this time larger than the original. */
    hr = IWICImagingFactory_CreateStream(pFactory, &pSubStream);
    ok(hr == S_OK, "CreateStream returned with %#x, expected %#x\n", hr, S_OK);

    uNewPos.QuadPart = 48;
    uSize.QuadPart = 32;
    hr = IWICStream_InitializeFromIStreamRegion(pSubStream, (IStream*)pStream, uNewPos, uSize);
    ok(hr == S_OK, "InitializeFromMemory returned with %#x, expected %#x\n", hr, S_OK);

    hr = IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_END, &uNewPos);
    ok(hr == S_OK, "Seek returned with %#x, expected %#x\n", hr, S_OK);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 16, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 16);

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    hr = IWICStream_Read(pSubStream, Memory, 48, &uBytesRead);
    ok(hr == S_OK, "Read returned with %#x, expected %#x\n", hr, S_OK);
    if(SUCCEEDED(hr)) {
        ok(uBytesRead == 16, "Read %u bytes\n", uBytesRead);
        ok(memcmp(Memory, CmpMem+48, uBytesRead) == 0, "Read returned invalid data!\n");
    }

    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_SET, NULL);
    uBytesWritten = 0xdeadbeef;
    hr = IWICStream_Write(pSubStream, CmpMem, 32, &uBytesWritten);
    ok(hr == STG_E_MEDIUMFULL, "Write returned with %#x, expected %#x\n", hr, STG_E_MEDIUMFULL);
    ok(uBytesWritten == 0xdeadbeef, "Expected uBytesWritten to be unchanged, got %u\n", uBytesWritten);
    IWICStream_Seek(pSubStream, LargeNull, STREAM_SEEK_CUR, &uNewPos);
    ok(uNewPos.u.HighPart == 0 && uNewPos.u.LowPart == 0, "Seek cursor moved to position (%u;%u), expected (%u;%u)\n", uNewPos.u.HighPart, uNewPos.u.LowPart, 0, 0);


    IWICStream_Release(pSubStream);
    IWICStream_Release(pStream);
    IWICImagingFactory_Release(pFactory);
    CoUninitialize();
}

START_TEST(stream)
{
    test_StreamOnMemory();
    test_StreamOnStreamRange();
}
