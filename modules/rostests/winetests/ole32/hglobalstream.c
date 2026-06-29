/*
 * Stream on HGLOBAL Tests
 *
 * Copyright 2006 Robert Shearman (for CodeWeavers)
 * Copyright 2016 Dmitry Timoshkov
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error %#08lx\n", hr)

static char const * const *expected_method_list;

#define CHECK_EXPECTED_METHOD(method_name) \
do { \
    ok(*expected_method_list != NULL, "Extra method %s called\n", method_name); \
        if (*expected_method_list) \
        { \
            ok(!strcmp(*expected_method_list, method_name), "Expected %s to be called instead of %s\n", \
               *expected_method_list, method_name); \
                   expected_method_list++; \
        } \
} while(0)

static void test_streamonhglobal(void)
{
    const char data[] = "Test String";
    ULARGE_INTEGER ull;
    IStream *pStream;
    LARGE_INTEGER ll;
    char buffer[128];
    ULONG read;
    STATSTG statstg;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok(hr == S_OK, "Failed to create a stream, hr %#lx.\n", hr);

    ull.QuadPart = sizeof(data);
    hr = IStream_SetSize(pStream, ull);
    ok_ole_success(hr, "IStream_SetSize");

    hr = IStream_Write(pStream, data, sizeof(data), NULL);
    ok_ole_success(hr, "IStream_Write");

    /* Seek beyond the end of the stream and read from it */
    ll.QuadPart = sizeof(data) + 16;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");

    hr = IStream_Read(pStream, buffer, sizeof(buffer), &read);
    ok_ole_success(hr, "IStream_Read");
    ok(read == 0, "IStream_Read returned read %ld\n", read);

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == sizeof(data) + 16, "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* Seek to the start of the stream and read from it */
    ll.QuadPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");

    /* should return S_OK, not S_FALSE */
    hr = IStream_Read(pStream, buffer, sizeof(buffer), &read);
    ok_ole_success(hr, "IStream_Read");
    ok(read == sizeof(data), "IStream_Read returned read %ld\n", read);

    /* ignores HighPart */
    ull.u.HighPart = -1;
    ull.u.LowPart = 0;
    hr = IStream_SetSize(pStream, ull);
    ok_ole_success(hr, "IStream_SetSize");

    /* IStream_Seek -- NULL position argument */
    ll.u.HighPart = 0;
    ll.u.LowPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, NULL);
    ok_ole_success(hr, "IStream_Seek");

    /* IStream_Seek -- valid position argument (seek from current position) */
    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == sizeof(data), "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- invalid seek argument */
    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 123;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_END+1, &ull);
    ok(hr == STG_E_SEEKERROR, "IStream_Seek should have returned STG_E_SEEKERROR instead of 0x%08lx\n", hr);
    ok(ull.u.LowPart == sizeof(data), "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should not have changed HighPart, got %ld\n", ull.u.HighPart);

    /* IStream_Seek -- valid position argument (seek to beginning) */
    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == 0, "should have set LowPart to 0 instead of %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- valid position argument (seek to end) */
    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_END, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == 0, "should have set LowPart to 0 instead of %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- ignore HighPart in the move value (seek from current position) */
    ll.u.HighPart = 0;
    ll.u.LowPart = sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = -1;
    ll.u.LowPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == sizeof(data), "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- ignore HighPart in the move value (seek to beginning) */
    ll.u.HighPart = 0;
    ll.u.LowPart = sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = -1;
    ll.u.LowPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == 0, "should have set LowPart to 0 instead of %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- invalid LowPart value (seek before start of stream) */
    ll.u.HighPart = 0;
    ll.u.LowPart = sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0x80000000;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok(hr == STG_E_SEEKERROR, "IStream_Seek should have returned STG_E_SEEKERROR instead of 0x%08lx\n", hr);
    ok(ull.u.LowPart == sizeof(data), "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- valid LowPart value (seek to start of stream) */
    ll.u.HighPart = 0;
    ll.u.LowPart = sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = -(DWORD)sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == 0, "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- invalid LowPart value (seek to start of stream-1) */
    ll.u.HighPart = 0;
    ll.u.LowPart = sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = -(DWORD)sizeof(data)-1;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok(hr == STG_E_SEEKERROR, "IStream_Seek should have returned STG_E_SEEKERROR instead of 0x%08lx\n", hr);
    ok(ull.u.LowPart == sizeof(data), "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- valid LowPart value (seek forward to 0x80000000) */
    ll.u.HighPart = 0;
    ll.u.LowPart = sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0x80000000 - sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == 0x80000000, "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- invalid LowPart value (seek to beginning) */
    ll.u.HighPart = 0;
    ll.u.LowPart = sizeof(data);
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0x80000000;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok(hr == STG_E_SEEKERROR, "IStream_Seek should have returned STG_E_SEEKERROR instead of 0x%08lx\n", hr);
    ok(ull.u.LowPart == sizeof(data), "LowPart set to %ld\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- valid LowPart value (seek to beginning) */
    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0x7FFFFFFF;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == 0x7FFFFFFF, "should have set LowPart to 0x7FFFFFFF instead of %08lx\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- valid LowPart value (seek from current position) */
    ll.u.HighPart = 0;
    ll.u.LowPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, &ull);
    ok_ole_success(hr, "IStream_Seek");

    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0x7FFFFFFF;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == 0x7FFFFFFF, "should have set LowPart to 0x7FFFFFFF instead of %08lx\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- second seek allows you to go past 0x7FFFFFFF size */
    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 9;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok_ole_success(hr, "IStream_Seek");
    ok(ull.u.LowPart == 0x80000008, "should have set LowPart to 0x80000008 instead of %08lx\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    /* IStream_Seek -- seek wraps position/size on integer overflow, but not on win8 */
    ull.u.HighPart = 0xCAFECAFE;
    ull.u.LowPart = 0xCAFECAFE;
    ll.u.HighPart = 0;
    ll.u.LowPart = 0x7FFFFFFF;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_CUR, &ull);
    ok(hr == S_OK || hr == STG_E_SEEKERROR /* win8 */, "IStream_Seek\n");
    if (SUCCEEDED(hr))
        ok(ull.u.LowPart == 0x00000007, "should have set LowPart to 0x00000007 instead of %08lx\n", ull.u.LowPart);
    else
        ok(ull.u.LowPart == 0x80000008, "should have set LowPart to 0x80000008 instead of %08lx\n", ull.u.LowPart);
    ok(ull.u.HighPart == 0, "should have set HighPart to 0 instead of %ld\n", ull.u.HighPart);

    hr = IStream_Commit(pStream, STGC_DEFAULT);
    ok_ole_success(hr, "IStream_Commit");

    hr = IStream_Revert(pStream);
    ok_ole_success(hr, "IStream_Revert");

    hr = IStream_LockRegion(pStream, ull, ull, LOCK_WRITE);
    ok(hr == STG_E_INVALIDFUNCTION, "IStream_LockRegion should have returned STG_E_INVALIDFUNCTION instead of 0x%08lx\n", hr);

    hr = IStream_Stat(pStream, &statstg, STATFLAG_DEFAULT);
    ok_ole_success(hr, "IStream_Stat");
    ok(statstg.type == STGTY_STREAM, "statstg.type should have been STGTY_STREAM instead of %ld\n", statstg.type);

    /* test OOM condition */
    ull.u.HighPart = -1;
    ull.u.LowPart = 0;
    hr = IStream_SetSize(pStream, ull);
    ok(hr == S_OK, "IStream_SetSize with large size should have returned S_OK instead of 0x%08lx\n", hr);

    IStream_Release(pStream);
}

static HRESULT WINAPI TestStream_QueryInterface(IStream *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_ISequentialStream) ||
        IsEqualIID(riid, &IID_IStream))
    {
        *ppv = iface;
        IStream_AddRef(iface);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TestStream_AddRef(IStream *iface)
{
    return 2;
}

static ULONG WINAPI TestStream_Release(IStream *iface)
{
    return 1;
}

static HRESULT WINAPI TestStream_Read(IStream *iface, void *pv, ULONG cb, ULONG *pcbRead)
{
    CHECK_EXPECTED_METHOD("TestStream_Read");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_Write(IStream *iface, const void *pv, ULONG cb, ULONG *pcbWritten)
{
    CHECK_EXPECTED_METHOD("TestStream_Write");
    *pcbWritten = 5;
    return S_OK;
}

static HRESULT WINAPI TestStream_Seek(IStream *iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    CHECK_EXPECTED_METHOD("TestStream_Seek");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    CHECK_EXPECTED_METHOD("TestStream_SetSize");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_CopyTo(IStream *iface, IStream *pStream, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    CHECK_EXPECTED_METHOD("TestStream_CopyTo");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    CHECK_EXPECTED_METHOD("TestStream_Commit");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_Revert(IStream *iface)
{
    CHECK_EXPECTED_METHOD("TestStream_Revert");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    CHECK_EXPECTED_METHOD("TestStream_LockRegion");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_UnlockRegion(IStream *iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    CHECK_EXPECTED_METHOD("TestStream_UnlockRegion");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_Stat(IStream *iface, STATSTG *pstatstg, DWORD grfStatFlag)
{
    CHECK_EXPECTED_METHOD("TestStream_Stat");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestStream_Clone(IStream *iface, IStream **pStream)
{
    CHECK_EXPECTED_METHOD("TestStream_Clone");
    return E_NOTIMPL;
}

static /*const*/ IStreamVtbl StreamVtbl =
{
    TestStream_QueryInterface,
    TestStream_AddRef,
    TestStream_Release,
    TestStream_Read,
    TestStream_Write,
    TestStream_Seek,
    TestStream_SetSize,
    TestStream_CopyTo,
    TestStream_Commit,
    TestStream_Revert,
    TestStream_LockRegion,
    TestStream_UnlockRegion,
    TestStream_Stat,
    TestStream_Clone
};

static IStream Test_Stream = { &StreamVtbl };

static void test_copyto(void)
{
    IStream *pStream, *pStream2;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    static const char szHello[] = "Hello";
    ULARGE_INTEGER cb;
    static const char *methods_copyto[] =
    {
        "TestStream_Write",
        NULL
    };
    ULONG written;
    ULARGE_INTEGER ullRead;
    ULARGE_INTEGER ullWritten;
    ULARGE_INTEGER libNewPosition;
    static const LARGE_INTEGER llZero;
    char buffer[15];

    ok_ole_success(hr, "CreateStreamOnHGlobal");

    expected_method_list = methods_copyto;

    hr = IStream_Write(pStream, szHello, sizeof(szHello), &written);
    ok_ole_success(hr, "IStream_Write");
    ok(written == sizeof(szHello), "only %ld bytes written\n", written);

    hr = IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");

    cb.QuadPart = sizeof(szHello);
    hr = IStream_CopyTo(pStream, &Test_Stream, cb, &ullRead, &ullWritten);
    ok(ullWritten.QuadPart == 5, "ullWritten was %ld instead\n", (ULONG)ullWritten.QuadPart);
    ok(ullRead.QuadPart == sizeof(szHello), "only %ld bytes read\n", (ULONG)ullRead.QuadPart);
    ok_ole_success(hr, "IStream_CopyTo");

    ok(!*expected_method_list, "Method sequence starting from %s not called\n", *expected_method_list);

    hr = IStream_Clone(pStream, &pStream2);
    ok_ole_success(hr, "IStream_Clone");

    hr = IStream_Seek(pStream2, llZero, STREAM_SEEK_CUR, &libNewPosition);
    ok_ole_success(hr, "IStream_Seek");
    ok(libNewPosition.QuadPart == sizeof(szHello), "libNewPosition wasn't set correctly for the cloned stream\n");

    hr = IStream_Seek(pStream2, llZero, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");

    hr = IStream_Read(pStream2, buffer, sizeof(buffer), NULL);
    ok_ole_success(hr, "IStream_Read");
    ok(!strcmp(buffer, szHello), "read data \"%s\" didn't match originally written data\n", buffer);

    IStream_Release(pStream2);
    IStream_Release(pStream);
}

static void test_freed_hglobal(void)
{
    static const char teststring[] = "this is a test string";
    HRESULT hr;
    IStream *pStream;
    HGLOBAL hglobal;
    char *p;
    char buffer[sizeof(teststring) + 8];
    ULARGE_INTEGER ull;
    ULONG read, written;

    hglobal = GlobalAlloc(GMEM_DDESHARE|GMEM_NODISCARD|GMEM_MOVEABLE, strlen(teststring) + 1);
    ok(hglobal != NULL, "GlobalAlloc failed with error %ld\n", GetLastError());
    p = GlobalLock(hglobal);
    strcpy(p, teststring);
    GlobalUnlock(hglobal);

    hr = CreateStreamOnHGlobal(hglobal, FALSE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = IStream_Read(pStream, buffer, sizeof(buffer), &read);
    ok_ole_success(hr, "IStream_Read");
    ok(!strcmp(buffer, teststring), "buffer data %s differs\n", buffer);
    ok(read == sizeof(teststring) ||
       broken(read == ((sizeof(teststring) + 3) & ~3)), /* win9x rounds the size */
       "read should be sizeof(teststring) instead of %ld\n", read);

    GlobalFree(hglobal);

    memset(buffer, 0, sizeof(buffer));
    read = -1;
    hr = IStream_Read(pStream, buffer, sizeof(buffer), &read);
    ok_ole_success(hr, "IStream_Read");
    ok(buffer[0] == 0, "buffer data should be untouched\n");
    ok(read == 0, "read should be 0 instead of %ld\n", read);

    ull.QuadPart = sizeof(buffer);
    hr = IStream_SetSize(pStream, ull);
    ok(hr == E_OUTOFMEMORY, "IStream_SetSize with invalid HGLOBAL should return E_OUTOFMEMORY instead of 0x%08lx\n", hr);

    hr = IStream_Write(pStream, buffer, sizeof(buffer), &written);
    ok(hr == E_OUTOFMEMORY, "IStream_Write with invalid HGLOBAL should return E_OUTOFMEMORY instead of 0x%08lx\n", hr);
    ok(written == 0, "written should be 0 instead of %ld\n", written);

    IStream_Release(pStream);
}

static void stream_info(IStream *stream, HGLOBAL *hmem, int *size, int *pos)
{
    HRESULT hr;
    STATSTG stat;
    LARGE_INTEGER offset;
    ULARGE_INTEGER newpos;

    *hmem = 0;
    *size = *pos = -1;

    hr = GetHGlobalFromStream(stream, hmem);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    memset(&stat, 0x55, sizeof(stat));
    hr = IStream_Stat(stream, &stat, STATFLAG_DEFAULT);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    ok(stat.type == STGTY_STREAM, "unexpected %#lx\n", stat.type);
    ok(!stat.pwcsName, "unexpected %p\n", stat.pwcsName);
    ok(IsEqualIID(&stat.clsid, &GUID_NULL), "unexpected %s\n", wine_dbgstr_guid(&stat.clsid));
    ok(!stat.cbSize.HighPart, "unexpected %#lx\n", stat.cbSize.HighPart);
    *size = stat.cbSize.LowPart;

    offset.QuadPart = 0;
    hr = IStream_Seek(stream, offset, STREAM_SEEK_CUR, &newpos);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    ok(!newpos.HighPart, "unexpected %#lx\n", newpos.HighPart);
    *pos = newpos.LowPart;
}

static void test_IStream_Clone(void)
{
    static const char hello[] = "Hello World!";
    char buf[32];
    HRESULT hr;
    IStream *stream, *clone;
    HGLOBAL orig_hmem, hmem, hmem_clone;
    ULARGE_INTEGER newsize;
    LARGE_INTEGER offset;
    int size, pos, ret;

    /* test simple case for Clone */
    orig_hmem = GlobalAlloc(GMEM_MOVEABLE, 0);
    ok(orig_hmem != 0, "unexpected %p\n", orig_hmem);
    hr = CreateStreamOnHGlobal(orig_hmem, TRUE, &stream);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    hr = GetHGlobalFromStream(stream, NULL);
    ok(hr == E_INVALIDARG, "unexpected %#lx\n", hr);

    hr = GetHGlobalFromStream(NULL, &hmem);
    ok(hr == E_INVALIDARG, "unexpected %#lx\n", hr);

    stream_info(stream, &hmem, &size, &pos);
    ok(hmem == orig_hmem, "handles should match\n");
    ok(size == 0, "unexpected %d\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    hr = IStream_Clone(stream, &clone);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    hr = IStream_Write(stream, hello, sizeof(hello), NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    stream_info(stream, &hmem, &size, &pos);
    ok(hmem == orig_hmem, "handles should match\n");
    ok(size == 13, "unexpected %d\n", size);
    ok(pos == 13, "unexpected %d\n", pos);

    stream_info(clone, &hmem_clone, &size, &pos);
    ok(hmem_clone == hmem, "handles should match\n");
    ok(size == 13, "unexpected %d\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    buf[0] = 0;
    hr = IStream_Read(clone, buf, sizeof(buf), NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    ok(!strcmp(buf, hello), "wrong stream contents\n");

    newsize.QuadPart = 0x8000;
    hr = IStream_SetSize(stream, newsize);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    stream_info(stream, &hmem, &size, &pos);
    ok(hmem == orig_hmem, "handles should match\n");
    ok(size == 0x8000, "unexpected %#x\n", size);
    ok(pos == 13, "unexpected %d\n", pos);

    stream_info(clone, &hmem_clone, &size, &pos);
    ok(hmem_clone == hmem, "handles should match\n");
    ok(size == 0x8000, "unexpected %#x\n", size);
    ok(pos == 13, "unexpected %d\n", pos);

    IStream_Release(clone);
    IStream_Release(stream);

    /* exploit GMEM_FIXED forced move for the same base streams */
    orig_hmem = GlobalAlloc(GMEM_FIXED, 1);
    ok(orig_hmem != 0, "unexpected %p\n", orig_hmem);
    hr = CreateStreamOnHGlobal(orig_hmem, TRUE, &stream);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    hr = IStream_Clone(stream, &clone);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    stream_info(stream, &hmem, &size, &pos);
    ok(hmem == orig_hmem, "handles should match\n");
    ok(size == 1, "unexpected %d\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    stream_info(clone, &hmem_clone, &size, &pos);
    ok(hmem_clone == hmem, "handles should match\n");
    ok(size == 1, "unexpected %d\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    newsize.QuadPart = 0x8000;
    hr = IStream_SetSize(stream, newsize);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    stream_info(stream, &hmem, &size, &pos);
    ok(hmem != 0, "unexpected %p\n", hmem);
    ok(hmem != orig_hmem, "unexpected %p\n", hmem);
    ok(size == 0x8000, "unexpected %#x\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    stream_info(clone, &hmem_clone, &size, &pos);
    ok(hmem_clone == hmem, "handles should match\n");
    ok(size == 0x8000, "unexpected %#x\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    IStream_Release(stream);
    IStream_Release(clone);

    /* test Release of cloned stream */
    hr = CreateStreamOnHGlobal(0, TRUE, &stream);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    hr = IStream_Clone(stream, &clone);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    stream_info(stream, &hmem, &size, &pos);
    ok(hmem != 0, "unexpected %p\n", hmem);
    ok(size == 0, "unexpected %d\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    stream_info(clone, &hmem_clone, &size, &pos);
    ok(hmem_clone == hmem, "handles should match\n");
    ok(size == 0, "unexpected %#x\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    ret = IStream_Release(stream);
    ok(ret == 0, "unexpected %d\n", ret);

    newsize.QuadPart = 0x8000;
    hr = IStream_SetSize(clone, newsize);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    stream_info(clone, &hmem_clone, &size, &pos);
    ok(hmem_clone == hmem, "handles should match\n");
    ok(size == 0x8000, "unexpected %#x\n", size);
    ok(pos == 0, "unexpected %d\n", pos);

    hr = IStream_Write(clone, hello, sizeof(hello), NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    stream_info(clone, &hmem_clone, &size, &pos);
    ok(hmem_clone == hmem, "handles should match\n");
    ok(size == 0x8000, "unexpected %#x\n", size);
    ok(pos == 13, "unexpected %d\n", pos);

    offset.QuadPart = 0;
    hr = IStream_Seek(clone, offset, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);

    buf[0] = 0;
    hr = IStream_Read(clone, buf, sizeof(buf), NULL);
    ok(hr == S_OK, "unexpected %#lx\n", hr);
    ok(!strcmp(buf, hello), "wrong stream contents\n");

    stream_info(clone, &hmem_clone, &size, &pos);
    ok(hmem_clone == hmem, "handles should match\n");
    ok(size == 0x8000, "unexpected %#x\n", size);
    ok(pos == 32, "unexpected %d\n", pos);

    ret = IStream_Release(clone);
    ok(ret == 0, "unexpected %d\n", ret);
}

START_TEST(hglobalstream)
{
    test_streamonhglobal();
    test_copyto();
    test_freed_hglobal();
    test_IStream_Clone();
}
