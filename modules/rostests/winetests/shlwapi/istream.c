/* Unit test suite for SHLWAPI ShCreateStreamOnFile functions.
 *
 * Copyright 2008 Reece H. Dunn
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
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "shlwapi.h"

static void test_IStream_invalid_operations(IStream * stream, DWORD mode)
{
    HRESULT ret;
    IStream * clone;
    ULONG refcount;
    ULARGE_INTEGER uzero;
    ULARGE_INTEGER uret;
    LARGE_INTEGER zero;
    ULONG count;
    char data[256];

    uzero.HighPart = 0;
    uzero.LowPart = 0;
    uret.HighPart = 0;
    uret.LowPart = 0;
    zero.HighPart = 0;
    zero.LowPart = 0;

    /* IStream::Read */

    /* IStream_Read from the COBJMACROS is undefined by shlwapi.h, replaced by the IStream_Read helper function. */

    ret = stream->lpVtbl->Read(stream, NULL, 0, &count);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    ret = stream->lpVtbl->Read(stream, data, 5, NULL);
    ok(ret == S_FALSE || ret == S_OK, "expected S_FALSE or S_OK, got 0x%08lx\n", ret);

    ret = stream->lpVtbl->Read(stream, data, 0, NULL);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    ret = stream->lpVtbl->Read(stream, data, 3, &count);
    ok(ret == S_FALSE || ret == S_OK, "expected S_FALSE or S_OK, got 0x%08lx\n", ret);

    /* IStream::Write */

    /* IStream_Write from the COBJMACROS is undefined by shlwapi.h, replaced by the IStream_Write helper function. */

    ret = stream->lpVtbl->Write(stream, NULL, 0, &count);
    if (mode == STGM_READ)
    {
        ok(ret == STG_E_ACCESSDENIED /* XP */ || broken(ret == S_OK) /* Win2000 + IE5 */,
           "expected STG_E_ACCESSDENIED, got 0x%08lx\n", ret);
    }
    else
        ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    strcpy(data, "Hello");
    ret = stream->lpVtbl->Write(stream, data, 5, NULL);
    if (mode == STGM_READ)
        ok(ret == STG_E_ACCESSDENIED,
           "expected STG_E_ACCESSDENIED, got 0x%08lx\n", ret);
    else
        ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    strcpy(data, "Hello");
    ret = stream->lpVtbl->Write(stream, data, 0, NULL);
    if (mode == STGM_READ)
        ok(ret == STG_E_ACCESSDENIED,
           "expected STG_E_ACCESSDENIED, got 0x%08lx\n", ret);
    else
        ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    strcpy(data, "Hello");
    ret = stream->lpVtbl->Write(stream, data, 0, &count);
    if (mode == STGM_READ)
        ok(ret == STG_E_ACCESSDENIED,
           "expected STG_E_ACCESSDENIED, got 0x%08lx\n", ret);
    else
        ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    strcpy(data, "Hello");
    ret = stream->lpVtbl->Write(stream, data, 3, &count);
    if (mode == STGM_READ)
        ok(ret == STG_E_ACCESSDENIED,
           "expected STG_E_ACCESSDENIED, got 0x%08lx\n", ret);
    else
        ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    /* IStream::Seek */

    ret = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    ret = IStream_Seek(stream, zero, 20, NULL);
    ok(ret == E_INVALIDARG,
       "expected E_INVALIDARG, got 0x%08lx\n", ret);

    /* IStream::CopyTo */

    ret = IStream_CopyTo(stream, NULL, uzero, &uret, &uret);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    clone = NULL;
    ret = IStream_CopyTo(stream, clone, uzero, &uret, &uret);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    ret = IStream_CopyTo(stream, stream, uzero, &uret, &uret);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    ret = IStream_CopyTo(stream, stream, uzero, &uret, NULL);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    ret = IStream_CopyTo(stream, stream, uzero, NULL, &uret);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    /* IStream::Commit */

    ret = IStream_Commit(stream, STGC_DEFAULT);
    ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

    /* IStream::Revert */

    ret = IStream_Revert(stream);
    ok(ret == E_NOTIMPL, "expected E_NOTIMPL, got 0x%08lx\n", ret);

    /* IStream::LockRegion */

    ret = IStream_LockRegion(stream, uzero, uzero, 0);
    ok(ret == E_NOTIMPL /* XP */ || ret == S_OK /* Vista */,
      "expected E_NOTIMPL or S_OK, got 0x%08lx\n", ret);

    /* IStream::UnlockRegion */

    if (ret == E_NOTIMPL) /* XP */ {
        ret = IStream_UnlockRegion(stream, uzero, uzero, 0);
        ok(ret == E_NOTIMPL, "expected E_NOTIMPL, got 0x%08lx\n", ret);
    } else /* Vista */ {
        ret = IStream_UnlockRegion(stream, uzero, uzero, 0);
        ok(ret == S_OK, "expected S_OK, got 0x%08lx\n", ret);

        ret = IStream_UnlockRegion(stream, uzero, uzero, 0);
        ok(ret == STG_E_LOCKVIOLATION, "expected STG_E_LOCKVIOLATION, got 0x%08lx\n", ret);
    }

    /* IStream::Stat */

    ret = IStream_Stat(stream, NULL, 0);
    ok(ret == STG_E_INVALIDPOINTER,
       "expected STG_E_INVALIDPOINTER or E_NOTIMPL, got 0x%08lx\n", ret);

    /* IStream::Clone */

    /* Passing a NULL pointer for the second IStream::Clone param crashes on Win7 */

    clone = NULL;
    ret = IStream_Clone(stream, &clone);
    ok(ret == E_NOTIMPL, "expected E_NOTIMPL, got 0x%08lx\n", ret);
    ok(clone == NULL, "expected a NULL IStream object, got %p\n", stream);

    if (clone) {
        refcount = IStream_Release(clone);
        ok(refcount == 0, "expected 0, got %ld\n", refcount);
    }
}


static void test_stream_read_write(IStream *stream, DWORD mode)
{
    static const LARGE_INTEGER start;
    HRESULT ret;
    unsigned char buf[16];
    DWORD written, count;
    STATSTG statstg;

    /* IStream_Read/Write from the COBJMACROS is undefined by shlwapi.h */

    written = 0xdeadbeaf;
    ret = stream->lpVtbl->Write(stream, "\x5e\xa7", 2, &written);
    if (mode == STGM_WRITE || mode == STGM_READWRITE)
    {
        ok(ret == S_OK, "IStream_Write error %#lx (access %#lx)\n", ret, mode);
        ok(written == 2, "expected 2, got %lu\n", written);
    }
    else
    {
        ok(ret == STG_E_ACCESSDENIED || broken(ret == S_OK) /* win2000 */, "expected STG_E_ACCESSDENIED, got %#lx (access %#lx)\n", ret, mode);
        ok(written == 0xdeadbeaf || broken(written == 2) /* win2000 */, "expected 0xdeadbeaf, got %#lx\n", written);
        written = 0;
        if (ret == S_OK) return; /* no point in further testing */
    }

    ret = stream->lpVtbl->Seek(stream, start, STREAM_SEEK_SET, NULL);
    ok(ret == S_OK, "Seek error %#lx\n", ret);

    count = 0xdeadbeaf;
    ret = stream->lpVtbl->Read(stream, buf, 2, &count);
    if (written != 0)
    {
        ok(ret == S_OK || broken(ret == S_FALSE) /* win2000 */, "IStream_Read error %#lx (access %#lx, written %lu)\n", ret, mode, written);
        if (ret == S_OK && (mode == STGM_WRITE || mode == STGM_READWRITE))
        {
            ok(count == 2, "expected 2, got %lu\n", count);
            ok(buf[0] == 0x5e && buf[1] == 0xa7, "expected 5ea7, got %02x%02x\n", buf[0], buf[1]);
        }
        else
            ok(count == 0, "expected 0, got %lu\n", count);
    }
    else
    {
        ok(ret == S_FALSE, "expected S_FALSE, got %#lx (access %#lx, written %lu)\n", ret, mode, written);
        ok(count == 0, "expected 0, got %lu\n", count);
    }

    ret = stream->lpVtbl->Seek(stream, start, STREAM_SEEK_SET, NULL);
    ok(ret == S_OK, "Seek error %#lx\n", ret);

    count = 0xdeadbeaf;
    ret = stream->lpVtbl->Read(stream, buf, 0, &count);
    ok(ret == S_OK, "IStream_Read error %#lx (access %#lx, written %lu)\n", ret, mode, written);
    ok(count == 0, "expected 0, got %lu\n", count);

    count = 0xdeadbeaf;
    ret = stream->lpVtbl->Read(stream, buf, sizeof(buf), &count);
    ok(ret == S_FALSE, "expected S_FALSE, got %#lx (access %#lx, written %lu)\n", ret, mode, written);
    ok(count == written, "expected %lu, got %lu\n", written, count);
    if (count)
        ok(buf[0] == 0x5e && buf[1] == 0xa7, "expected 5ea7, got %02x%02x\n", buf[0], buf[1]);

    memset(&statstg, 0xff, sizeof(statstg));
    ret = IStream_Stat(stream, &statstg, 0);
    ok(ret == S_OK, "Stat failed, hr %#lx.\n", ret);
    ok(statstg.pwcsName != NULL, "Unexpected name %s.\n", wine_dbgstr_w(statstg.pwcsName));
    CoTaskMemFree(statstg.pwcsName);

    memset(&statstg, 0xff, sizeof(statstg));
    ret = IStream_Stat(stream, &statstg, STATFLAG_NONAME);
    ok(ret == S_OK, "Stat failed, hr %#lx.\n", ret);
    ok(statstg.pwcsName == NULL, "Unexpected name %s.\n", wine_dbgstr_w(statstg.pwcsName));
}

static void test_stream_qi(IStream *stream)
{
    IUnknown *unk;
    HRESULT hr;

    hr = IStream_QueryInterface(stream, &IID_IStream, (void **)&unk);
    ok(SUCCEEDED(hr), "Failed to get IStream interface, hr %#lx.\n", hr);
    IUnknown_Release(unk);

    unk = NULL;
    hr = IStream_QueryInterface(stream, &IID_ISequentialStream, (void **)&unk);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* XP */, "Failed to get ISequentialStream interface, hr %#lx.\n", hr);
    if (unk)
        IUnknown_Release(unk);

    hr = IStream_QueryInterface(stream, &IID_IUnknown, (void **)&unk);
    ok(SUCCEEDED(hr), "Failed to get IUnknown interface, hr %#lx.\n", hr);
    IUnknown_Release(unk);
}

static void test_SHCreateStreamOnFileA(DWORD mode, DWORD stgm)
{
    IStream * stream;
    HRESULT ret;
    ULONG refcount;
    char test_file[MAX_PATH];
    static const CHAR testA_txt[] = "\\testA.txt";

    trace("SHCreateStreamOnFileA: testing mode %ld, STGM flags %08lx\n", mode, stgm);

    /* Don't used a fixed path for the testA.txt file */
    GetTempPathA(MAX_PATH, test_file);
    lstrcatA(test_file, testA_txt);

    /* invalid arguments */

    stream = NULL;
    ret = SHCreateStreamOnFileA(NULL, mode | stgm, &stream);
    if (ret == E_INVALIDARG) /* Win98 SE */ {
        win_skip("Not supported\n");
        return;
    }

    ok(ret == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) /* NT */ ||
       ret == HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME) /* 9x */,
       "SHCreateStreamOnFileA: expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) "
       "or HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME), got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

if (0) /* This test crashes on WinXP SP2 */
{
    ret = SHCreateStreamOnFileA(test_file, mode | stgm, NULL);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileA: expected E_INVALIDARG, got 0x%08lx\n", ret);
}

    stream = NULL;
    ret = SHCreateStreamOnFileA(test_file, mode | STGM_CONVERT | stgm, &stream);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileA: expected E_INVALIDARG, got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = SHCreateStreamOnFileA(test_file, mode | STGM_DELETEONRELEASE | stgm, &stream);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileA: expected E_INVALIDARG, got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = SHCreateStreamOnFileA(test_file, mode | STGM_TRANSACTED | stgm, &stream);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileA: expected E_INVALIDARG, got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

    /* file does not exist */

    stream = NULL;
    ret = SHCreateStreamOnFileA(test_file, mode | STGM_FAILIFTHERE | stgm, &stream);
    ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "SHCreateStreamOnFileA: expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileA: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = SHCreateStreamOnFileA(test_file, mode | STGM_CREATE | stgm, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileA: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileA: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_stream_qi(stream);
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileA: expected 0, got %ld\n", refcount);
    }

    /* NOTE: don't delete the file, as it will be used for the file exists tests. */

    /* file exists */

    stream = NULL;
    ret = SHCreateStreamOnFileA(test_file, mode | STGM_FAILIFTHERE | stgm, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileA: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileA: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileA: expected 0, got %ld\n", refcount);
    }

    stream = NULL;
    ret = SHCreateStreamOnFileA(test_file, mode | STGM_CREATE | stgm, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileA: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileA: expected a valid IStream object, got NULL\n");

    if (stream) {
        BOOL delret;

        test_stream_read_write(stream, mode);
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileA: expected 0, got %ld\n", refcount);

        delret = DeleteFileA(test_file);
        ok(delret, "SHCreateStreamOnFileA: could not delete file '%s', got error %ld\n",
           test_file, GetLastError());
    }
}


static void test_SHCreateStreamOnFileW(DWORD mode, DWORD stgm)
{
    IStream * stream;
    HRESULT ret;
    ULONG refcount;
    WCHAR test_file[MAX_PATH];
    CHAR  test_fileA[MAX_PATH];
    static const CHAR testW_txt[] = "\\testW.txt";

    trace("SHCreateStreamOnFileW: testing mode %ld, STGM flags %08lx\n", mode, stgm);

    /* Don't used a fixed path for the testW.txt file */
    GetTempPathA(MAX_PATH, test_fileA);
    lstrcatA(test_fileA, testW_txt);
    MultiByteToWideChar(CP_ACP, 0, test_fileA, -1, test_file, MAX_PATH);

    /* invalid arguments */

    if (0)
    {
        /* Crashes on NT4 */
        stream = NULL;
        ret = SHCreateStreamOnFileW(NULL, mode | stgm, &stream);
        ok(ret == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || /* XP */
           ret == E_INVALIDARG /* Vista */,
          "SHCreateStreamOnFileW: expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) or E_INVALIDARG, got 0x%08lx\n", ret);
        ok(stream == NULL, "SHCreateStreamOnFileW: expected a NULL IStream object, got %p\n", stream);
    }

    if (0)
    {
        /* This test crashes on WinXP SP2 */
            ret = SHCreateStreamOnFileW(test_file, mode | stgm, NULL);
            ok(ret == E_INVALIDARG, "SHCreateStreamOnFileW: expected E_INVALIDARG, got 0x%08lx\n", ret);
    }

    stream = NULL;
    ret = SHCreateStreamOnFileW(test_file, mode | STGM_CONVERT | stgm, &stream);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileW: expected E_INVALIDARG, got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileW: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = SHCreateStreamOnFileW(test_file, mode | STGM_DELETEONRELEASE | stgm, &stream);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileW: expected E_INVALIDARG, got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileW: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = SHCreateStreamOnFileW(test_file, mode | STGM_TRANSACTED | stgm, &stream);
    ok(ret == E_INVALIDARG, "SHCreateStreamOnFileW: expected E_INVALIDARG, got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileW: expected a NULL IStream object, got %p\n", stream);

    /* file does not exist */

    stream = NULL;
    ret = SHCreateStreamOnFileW(test_file, mode | STGM_FAILIFTHERE | stgm, &stream);
    if (ret == E_INVALIDARG) /* Win98 SE */ {
        win_skip("Not supported\n");
        return;
    }

    ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "SHCreateStreamOnFileW: expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileW: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = SHCreateStreamOnFileW(test_file, mode | STGM_CREATE | stgm, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileW: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileW: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_stream_qi(stream);
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileW: expected 0, got %ld\n", refcount);
    }

    /* NOTE: don't delete the file, as it will be used for the file exists tests. */

    /* file exists */

    stream = NULL;
    ret = SHCreateStreamOnFileW(test_file, mode | STGM_FAILIFTHERE | stgm, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileW: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileW: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileW: expected 0, got %ld\n", refcount);
    }

    stream = NULL;
    ret = SHCreateStreamOnFileW(test_file, mode | STGM_CREATE | stgm, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileW: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileW: expected a valid IStream object, got NULL\n");

    if (stream) {
        BOOL delret;

        test_stream_read_write(stream, mode);
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileW: expected 0, got %ld\n", refcount);

        delret = DeleteFileA(test_fileA);
        ok(delret, "SHCreateStreamOnFileW: could not delete the test file, got error %ld\n",
           GetLastError());
    }
}


static void test_SHCreateStreamOnFileEx(DWORD mode, DWORD stgm)
{
    IStream * stream;
    IStream * template = NULL;
    HRESULT ret;
    ULONG refcount;
    WCHAR test_file[MAX_PATH];
    CHAR  test_fileA[MAX_PATH];
    static const CHAR testEx_txt[] = "\\testEx.txt";
    BOOL delret;

    if (winetest_debug > 1)
        trace("SHCreateStreamOnFileEx: testing mode %ld, STGM flags %08lx\n", mode, stgm);

    /* Don't used a fixed path for the testEx.txt file */
    GetTempPathA(MAX_PATH, test_fileA);
    lstrcatA(test_fileA, testEx_txt);
    MultiByteToWideChar(CP_ACP, 0, test_fileA, -1, test_file, MAX_PATH);

    /* invalid arguments */

    if (0)
    {
        /* Crashes on NT4 */
        stream = NULL;
        ret = SHCreateStreamOnFileEx(NULL, mode, 0, FALSE, NULL, &stream);
        ok(ret == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || /* XP */
           ret == E_INVALIDARG /* Vista */,
          "SHCreateStreamOnFileEx: expected HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) or E_INVALIDARG, got 0x%08lx\n", ret);
        ok(stream == NULL, "SHCreateStreamOnFileEx: expected a NULL IStream object, got %p\n", stream);
    }

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode, 0, FALSE, template, &stream);
    if (ret == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
        win_skip("File probably locked by Anti-Virus/Spam software, trying again\n");
        Sleep(1000);
        ret = SHCreateStreamOnFileEx(test_file, mode, 0, FALSE, template, &stream);
    }
    ok( ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
        ret == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
        "SHCreateStreamOnFileEx: expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) or "
        "HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), got 0x%08lx\n", ret);

    ok(stream == NULL, "SHCreateStreamOnFileEx: expected a NULL IStream object, got %p\n", stream);

    if (0)
    {
        /* This test crashes on WinXP SP2 */
        ret = SHCreateStreamOnFileEx(test_file, mode, 0, FALSE, NULL, NULL);
        ok(ret == E_INVALIDARG, "SHCreateStreamOnFileEx: expected E_INVALIDARG, got 0x%08lx\n", ret);
    }

    /* file does not exist */

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode | STGM_FAILIFTHERE | stgm, 0, FALSE, NULL, &stream);
    if ((stgm & STGM_TRANSACTED) == STGM_TRANSACTED && mode == STGM_READ) {
        ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* XP */ || ret == E_INVALIDARG /* Vista */,
          "SHCreateStreamOnFileEx: expected E_INVALIDARG or HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got 0x%08lx\n", ret);

        if (ret == E_INVALIDARG) {
            skip("SHCreateStreamOnFileEx: STGM_TRANSACTED not supported in this configuration.\n");
            return;
        }
    } else {
        ok( ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
            ret == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
            "SHCreateStreamOnFileEx: expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) or "
            "HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), got 0x%08lx\n", ret);
    }
    ok(stream == NULL, "SHCreateStreamOnFileEx: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode | STGM_FAILIFTHERE | stgm, 0, TRUE, NULL, &stream);
    /* not supported on win9x */
    if (broken(ret == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) && stream == NULL)) {
        skip("Not supported\n");
        DeleteFileA(test_fileA);
        return;
    }

    ok(ret == S_OK, "SHCreateStreamOnFileEx: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileEx: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_stream_qi(stream);
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileEx: expected 0, got %ld\n", refcount);

        delret = DeleteFileA(test_fileA);
        ok(delret, "SHCreateStreamOnFileEx: could not delete the test file, got error %ld\n",
           GetLastError());
    }

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode | STGM_CREATE | stgm, 0, FALSE, NULL, &stream);
    if (ret == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
        win_skip("File probably locked by Anti-Virus/Spam software, trying again\n");
        Sleep(1000);
        ret = SHCreateStreamOnFileEx(test_file, mode | STGM_CREATE | stgm, 0, FALSE, NULL, &stream);
    }
    ok(ret == S_OK, "SHCreateStreamOnFileEx: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileEx: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileEx: expected 0, got %ld\n", refcount);

        delret = DeleteFileA(test_fileA);
        ok(delret, "SHCreateStreamOnFileEx: could not delete the test file, got error %ld\n",
           GetLastError());
    }

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode | STGM_CREATE | stgm, 0, TRUE, NULL, &stream);
    if (ret == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
        win_skip("File probably locked by Anti-Virus/Spam software, trying again\n");
        Sleep(1000);
        ret = SHCreateStreamOnFileEx(test_file, mode | STGM_CREATE | stgm, 0, TRUE, NULL, &stream);
    }
    ok(ret == S_OK, "SHCreateStreamOnFileEx: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileEx: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileEx: expected 0, got %ld\n", refcount);
    }

    /* NOTE: don't delete the file, as it will be used for the file exists tests. */

    /* file exists */

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode | STGM_FAILIFTHERE | stgm, 0, FALSE, NULL, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileEx: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileEx: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileEx: expected 0, got %ld\n", refcount);
    }

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode | STGM_FAILIFTHERE | stgm, 0, TRUE, NULL, &stream);
    ok(ret == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "SHCreateStreamOnFileEx: expected HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), got 0x%08lx\n", ret);
    ok(stream == NULL, "SHCreateStreamOnFileEx: expected a NULL IStream object, got %p\n", stream);

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode | STGM_CREATE | stgm, 0, FALSE, NULL, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileEx: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileEx: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileEx: expected 0, got %ld\n", refcount);
    }

    stream = NULL;
    ret = SHCreateStreamOnFileEx(test_file, mode | STGM_CREATE | stgm, 0, TRUE, NULL, &stream);
    ok(ret == S_OK, "SHCreateStreamOnFileEx: expected S_OK, got 0x%08lx\n", ret);
    ok(stream != NULL, "SHCreateStreamOnFileEx: expected a valid IStream object, got NULL\n");

    if (stream) {
        test_IStream_invalid_operations(stream, mode);

        refcount = IStream_Release(stream);
        ok(refcount == 0, "SHCreateStreamOnFileEx: expected 0, got %ld\n", refcount);
    }

    delret = DeleteFileA(test_fileA);
    ok(delret, "SHCreateStreamOnFileEx: could not delete the test file, got error %ld\n",
       GetLastError());
}


static void test_SHCreateStreamOnFileEx_CopyTo(void)
{
    HRESULT ret;
    IStream *src, *dst;
    WCHAR tmpPath[MAX_PATH];
    WCHAR srcFileName[MAX_PATH];
    WCHAR dstFileName[MAX_PATH];
    ULARGE_INTEGER count, read, written;
    LARGE_INTEGER distance;
    static const char srcContents[1];
    static const WCHAR prefix[] = { 'T', 'S', 'T', 0 };

    GetTempPathW(MAX_PATH, tmpPath);
    ret = GetTempFileNameW(tmpPath, prefix, 0, srcFileName);
    ok(ret != 0, "GetTempFileName failed, got error %ld\n", GetLastError());
    ret = GetTempFileNameW(tmpPath, prefix, 0, dstFileName);
    ok(ret != 0, "GetTempFileName failed, got error %ld\n", GetLastError());

    ret = SHCreateStreamOnFileEx(srcFileName, STGM_CREATE | STGM_READWRITE | STGM_DELETEONRELEASE, FILE_ATTRIBUTE_TEMPORARY, FALSE, NULL, &src);
    ok(SUCCEEDED(ret), "SHCreateStreamOnFileEx failed with ret=0x%08lx\n", ret);

    written.QuadPart = 0;
    ret = IStream_Write(src, srcContents, sizeof(srcContents), &written.LowPart);
    ok(SUCCEEDED(ret), "ISequentialStream_Write failed with ret=0x%08lx\n", ret);

    distance.QuadPart = 0;
    ret = IStream_Seek(src, distance, STREAM_SEEK_SET, &written);
    ok(SUCCEEDED(ret), "ISequentialStream_Seek failed with ret=0x%08lx\n", ret);

    ret = SHCreateStreamOnFileEx(dstFileName, STGM_CREATE | STGM_READWRITE | STGM_DELETEONRELEASE, FILE_ATTRIBUTE_TEMPORARY, FALSE, NULL, &dst);
    ok(SUCCEEDED(ret), "SHCreateStreamOnFileEx failed with ret=0x%08lx\n", ret);

    /* Test using a count larger than the source file, so that the Read operation will fall short */
    count.QuadPart = 2;

    ret = IStream_CopyTo(src, dst, count, &read, &written);
    ok(SUCCEEDED(ret), "CopyTo failed with ret=0x%08lx\n", ret);

    ok(read.QuadPart == 1, "read does not match size: %ld != 1\n", read.LowPart);
    ok(written.QuadPart == 1, "written does not match size: %ld != 1\n", written.LowPart);

    IStream_Release(dst);
    IStream_Release(src);
    DeleteFileW( srcFileName );
    DeleteFileW( dstFileName );
}

static void test_SHCreateMemStream(void)
{
    static const BYTE initial[10];
    IStream *stream, *stream2;
    LARGE_INTEGER off;
    IUnknown *unk;
    char buff[10];
    ULONG i, num;
    HRESULT hr;

    stream = SHCreateMemStream(initial, 0);
    ok(stream != NULL, "Failed to create a stream.\n");
    IStream_Release(stream);

    stream = SHCreateMemStream(NULL, 10);
    ok(stream != NULL, "Failed to create a stream.\n");
    IStream_Release(stream);

    stream = SHCreateMemStream(NULL, 0);
    ok(stream != NULL, "Failed to create a stream.\n");

    hr = IStream_QueryInterface(stream, &IID_ISequentialStream, (void **)&unk);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* WinXP */, "Failed to QI, hr %#lx.\n", hr);
    if (unk)
        IUnknown_Release(unk);

    num = ~0;
    hr = IStream_Read(stream, buff, sizeof(buff), &num);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(num == 0, "expected 0, got 0x%08lx\n", num);

    num = ~0;
    memset(buff, 0x55, sizeof(buff));
    hr = IStream_Write(stream, buff, sizeof(buff), &num);
    ok(hr == S_OK, "Failed to write, hr %#lx.\n", hr);
    ok(num == sizeof(buff), "expected %Iu, got %lu\n", sizeof(buff), num);

    off.QuadPart = 0;
    hr = IStream_Seek(stream, off, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Failed to seek, hr %#lx.\n", hr);

    num = ~0;
    memset(buff, 0, sizeof(buff));
    hr = IStream_Read(stream, buff, sizeof(buff), &num);
    ok(hr == S_OK, "Failed to read, hr %#lx.\n", hr);
    ok(num == sizeof(buff), "expected %Iu, got %lu\n", sizeof(buff), num);
    for (i = 0; i < ARRAY_SIZE(buff); i++)
        ok(buff[i] == 0x55, "expected 0x55, got 0x%02x at %lu\n", buff[i], i);

    num = ~0;
    hr = IStream_Read(stream, buff, sizeof(buff), &num);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(num == 0, "expected 0, got 0x%08lx\n", num);

    num = ~0;
    memset(buff, 0x11, sizeof(buff));
    hr = IStream_Write(stream, buff, sizeof(buff), &num);
    ok(hr == S_OK, "Failed to write, hr %#lx.\n", hr);
    ok(num == sizeof(buff), "expected %Iu, got %lu\n", sizeof(buff), num);

    off.QuadPart = -sizeof(buff);
    hr = IStream_Seek(stream, off, STREAM_SEEK_CUR, NULL);
    ok(hr == S_OK, "Failed to seek, hr %#lx.\n", hr);

    num = ~0;
    memset(buff, 0, sizeof(buff));
    hr = IStream_Read(stream, buff, sizeof(buff), &num);
    ok(hr == S_OK, "Failed to read, hr %#lx.\n", hr);
    ok(num == sizeof(buff), "expected %Iu, got %lu\n", sizeof(buff), num);
    for (i = 0; i < ARRAY_SIZE(buff); i++)
        ok(buff[i] == 0x11, "expected 0x11, got 0x%02x at %lu\n", buff[i], i);

    num = ~0;
    hr = IStream_Read(stream, buff, sizeof(buff), &num);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(num == 0, "expected 0, got 0x%08lx\n", num);

    hr = IStream_Clone(stream, &stream2);
    todo_wine
    ok(hr == S_OK || broken(hr == E_NOTIMPL) /* < Win8 */, "Failed to clone a stream, hr %#lx.\n", hr);
    if (hr == S_OK)
        IStream_Release(stream2);

    IStream_Release(stream);
}

START_TEST(istream)
{
    static const DWORD stgm_access[] = {
        STGM_READ,
        STGM_WRITE,
        STGM_READWRITE
    };

    static const DWORD stgm_sharing[] = {
        0,
        STGM_SHARE_DENY_NONE,
        STGM_SHARE_DENY_READ,
        STGM_SHARE_DENY_WRITE,
        STGM_SHARE_EXCLUSIVE
    };

    static const DWORD stgm_flags[] = {
        0,
        STGM_CONVERT,
        STGM_DELETEONRELEASE,
        STGM_CONVERT | STGM_DELETEONRELEASE,
        STGM_TRANSACTED | STGM_CONVERT,
        STGM_TRANSACTED | STGM_DELETEONRELEASE,
        STGM_TRANSACTED | STGM_CONVERT | STGM_DELETEONRELEASE
    };

    int i, j, k;

    for (i = 0; i != ARRAY_SIZE(stgm_access); i++) {
        for (j = 0; j != ARRAY_SIZE(stgm_sharing); j ++) {
            test_SHCreateStreamOnFileA(stgm_access[i], stgm_sharing[j]);
            test_SHCreateStreamOnFileW(stgm_access[i], stgm_sharing[j]);

            for (k = 0; k != ARRAY_SIZE(stgm_flags); k++)
                test_SHCreateStreamOnFileEx(stgm_access[i], stgm_sharing[j] | stgm_flags[k]);
        }
    }

    test_SHCreateStreamOnFileEx_CopyTo();
    test_SHCreateMemStream();
}
