/*
 * MimeOle tests
 *
 * Copyright 2007 Huw Davies
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
#include "initguid.h"
#include "windows.h"
#include "ole2.h"
#include "ocidl.h"

#include "mimeole.h"
#include "wininet.h"

#include <stdio.h>

#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        expect_ ## func = FALSE; \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func  "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(Stream_Read);
DEFINE_EXPECT(Stream_Stat);
DEFINE_EXPECT(Stream_Seek);
DEFINE_EXPECT(Stream_Seek_END);
DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportResult);

static const char msg1[] =
    "MIME-Version: 1.0\r\n"
    "Content-Type: multipart/mixed;\r\n"
    " boundary=\"------------1.5.0.6\";\r\n"
    " stuff=\"du;nno\";\r\n"
    " morestuff=\"so\\\\me\\\"thing\\\"\"\r\n"
    "foo: bar\r\n"
    "From: Huw Davies <huw@codeweavers.com>\r\n"
    "From: Me <xxx@codeweavers.com>\r\n"
    "To: wine-patches <wine-patches@winehq.org>\r\n"
    "Cc: Huw Davies <huw@codeweavers.com>,\r\n"
    "    \"Fred Bloggs\"   <fred@bloggs.com>\r\n"
    "foo: baz\r\n"
    "bar: fum\r\n"
    "\r\n"
    "This is a multi-part message in MIME format.\r\n"
    "--------------1.5.0.6\r\n"
    "Content-Type: text/plain; format=fixed; charset=UTF-8\r\n"
    "Content-Transfer-Encoding: 8bit\r\n"
    "\r\n"
    "Stuff\r\n"
    "--------------1.5.0.6\r\n"
    "Content-Type: text/plain; charset=\"us-ascii\"\r\n"
    "Content-Transfer-Encoding: 7bit\r\n"
    "\r\n"
    "More stuff\r\n"
    "--------------1.5.0.6--\r\n";

static const char mhtml_page1[] =
    "MIME-Version: 1.0\r\n"
    "Content-Type: multipart/related; type:=\"text/html\"; boundary=\"----=_NextPart_000_00\"\r\n"
    "\r\n"
    "------=_NextPart_000_00\r\n"
    "Content-Type: text/html; charset=\"Windows-1252\"\r\n"
    "Content-Transfer-Encoding: quoted-printable\r\n"
    "\r\n"
    "<HTML></HTML>\r\n"
    "------=_NextPart_000_00\r\n"
    "Content-Type: Image/Jpeg\r\n"
    "Content-Transfer-Encoding: base64\r\n"
    "Content-Location: http://winehq.org/mhtmltest.html\r\n"
    "\r\n\t\t\t\tVGVzdA==\r\n\r\n"
    "------=_NextPart_000_00--";

static void test_CreateVirtualStream(void)
{
    HRESULT hr;
    IStream *pstm;

    hr = MimeOleCreateVirtualStream(&pstm);
    ok(hr == S_OK, "ret %08lx\n", hr);

    IStream_Release(pstm);
}

static void test_CreateSecurity(void)
{
    HRESULT hr;
    IMimeSecurity *sec;

    hr = MimeOleCreateSecurity(&sec);
    ok(hr == S_OK, "ret %08lx\n", hr);

    IMimeSecurity_Release(sec);
}

static IStream *create_stream_from_string(const char *data)
{
    LARGE_INTEGER off;
    IStream *stream;
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IStream_Write(stream, data, strlen(data), NULL);
    ok(hr == S_OK, "Write failed: %08lx\n", hr);

    off.QuadPart = 0;
    hr = IStream_Seek(stream, off, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Seek failed: %08lx\n", hr);

    return stream;
}

#define test_current_encoding(a,b) _test_current_encoding(__LINE__,a,b)
static void _test_current_encoding(unsigned line, IMimeBody *mime_body, ENCODINGTYPE encoding)
{
    ENCODINGTYPE current_encoding;
    HRESULT hres;

    hres = IMimeBody_GetCurrentEncoding(mime_body, &current_encoding);
    ok_(__FILE__,line)(hres == S_OK, "GetCurrentEncoding failed: %08lx\n", hres);
    ok_(__FILE__,line)(current_encoding == encoding, "encoding = %d, expected %d\n", current_encoding, encoding);
}

static void test_CreateBody(void)
{
    HRESULT hr;
    IMimeBody *body;
    HBODY handle = (void *)0xdeadbeef;
    IStream *in;
    LARGE_INTEGER off;
    ULARGE_INTEGER pos;
    ULONG count, found_param, i;
    MIMEPARAMINFO *param_info;
    IMimeAllocator *alloc;
    BODYOFFSETS offsets;
    CLSID clsid;

    hr = CoCreateInstance(&CLSID_IMimeBody, NULL, CLSCTX_INPROC_SERVER, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_GetClassID(body, NULL);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    hr = IMimeBody_GetClassID(body, &clsid);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(IsEqualGUID(&clsid, &IID_IMimeBody), "got %s\n", wine_dbgstr_guid(&clsid));

    hr = IMimeBody_GetHandle(body, &handle);
    ok(hr == MIME_E_NO_DATA, "ret %08lx\n", hr);
    ok(handle == NULL, "handle %p\n", handle);

    in = create_stream_from_string(msg1);

    /* Need to call InitNew before Load otherwise Load crashes with native inetcomm */
    hr = IMimeBody_InitNew(body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    test_current_encoding(body, IET_7BIT);

    hr = IMimeBody_Load(body, in);
    ok(hr == S_OK, "ret %08lx\n", hr);
    off.QuadPart = 0;
    IStream_Seek(in, off, STREAM_SEEK_CUR, &pos);
    ok(pos.LowPart == 359, "pos %lu\n", pos.LowPart);

    hr = IMimeBody_IsContentType(body, "multipart", "mixed");
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeBody_IsContentType(body, "text", "plain");
    ok(hr == S_FALSE, "ret %08lx\n", hr);
    hr = IMimeBody_IsContentType(body, NULL, "mixed");
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeBody_IsType(body, IBT_EMPTY);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = IMimeBody_SetData(body, IET_8BIT, "text", "plain", &IID_IStream, in);
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeBody_IsContentType(body, "text", "plain");
    todo_wine
        ok(hr == S_OK, "ret %08lx\n", hr);
    test_current_encoding(body, IET_8BIT);

    memset(&offsets, 0xcc, sizeof(offsets));
    hr = IMimeBody_GetOffsets(body, &offsets);
    ok(hr == MIME_E_NO_DATA, "ret %08lx\n", hr);
    ok(offsets.cbBoundaryStart == 0, "got %ld\n", offsets.cbBoundaryStart);
    ok(offsets.cbHeaderStart == 0, "got %ld\n", offsets.cbHeaderStart);
    ok(offsets.cbBodyStart == 0, "got %ld\n", offsets.cbBodyStart);
    ok(offsets.cbBodyEnd == 0, "got %ld\n", offsets.cbBodyEnd);

    hr = IMimeBody_IsType(body, IBT_EMPTY);
    ok(hr == S_FALSE, "got %08lx\n", hr);

    hr = MimeOleGetAllocator(&alloc);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_GetParameters(body, "nothere", &count, &param_info);
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);
    ok(count == 0, "got %ld\n", count);
    ok(!param_info, "got %p\n", param_info);

    hr = IMimeBody_GetParameters(body, "bar", &count, &param_info);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(count == 0, "got %ld\n", count);
    ok(!param_info, "got %p\n", param_info);

    hr = IMimeBody_GetParameters(body, "Content-Type", &count, &param_info);
    ok(hr == S_OK, "ret %08lx\n", hr);
    todo_wine  /* native adds a charset parameter */
        ok(count == 4, "got %ld\n", count);
    ok(param_info != NULL, "got %p\n", param_info);

    found_param = 0;
    for(i = 0; i < count; i++)
    {
        if(!strcmp(param_info[i].pszName, "morestuff"))
        {
            found_param++;
            ok(!strcmp(param_info[i].pszData, "so\\me\"thing\""),
               "got %s\n", param_info[i].pszData);
        }
        else if(!strcmp(param_info[i].pszName, "stuff"))
        {
            found_param++;
            ok(!strcmp(param_info[i].pszData, "du;nno"),
               "got %s\n", param_info[i].pszData);
        }
    }
    ok(found_param == 2, "matched %ld params\n", found_param);

    hr = IMimeAllocator_FreeParamInfoArray(alloc, count, param_info, TRUE);
    ok(hr == S_OK, "ret %08lx\n", hr);
    IMimeAllocator_Release(alloc);

    IStream_Release(in);
    IMimeBody_Release(body);
}

typedef struct {
    IStream IStream_iface;
    LONG ref;
    unsigned pos;
} TestStream;

static inline TestStream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, TestStream, IStream_iface);
}

static HRESULT WINAPI Stream_QueryInterface(IStream *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_ISequentialStream, riid) || IsEqualGUID(&IID_IStream, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Stream_AddRef(IStream *iface)
{
    TestStream *This = impl_from_IStream(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI Stream_Release(IStream *iface)
{
    TestStream *This = impl_from_IStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI Stream_Read(IStream *iface, void *pv, ULONG cb, ULONG *pcbRead)
{
    TestStream *This = impl_from_IStream(iface);
    BYTE *output = pv;
    unsigned i;

    CHECK_EXPECT(Stream_Read);

    for(i = 0; i < cb; i++)
        output[i] = '0' + This->pos++;
    *pcbRead = i;
    return S_OK;
}

static HRESULT WINAPI Stream_Write(IStream *iface, const void *pv, ULONG cb, ULONG *pcbWritten)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static DWORD expect_seek_pos;

static HRESULT WINAPI Stream_Seek(IStream *iface, LARGE_INTEGER dlibMove, DWORD dwOrigin,
                                  ULARGE_INTEGER *plibNewPosition)
{
    TestStream *This = impl_from_IStream(iface);

    if(dwOrigin == STREAM_SEEK_END) {
        CHECK_EXPECT(Stream_Seek_END);
        ok(dlibMove.QuadPart == expect_seek_pos, "unexpected seek pos %lu\n", dlibMove.LowPart);
        if(plibNewPosition)
            plibNewPosition->QuadPart = 10;
        return S_OK;
    }

    CHECK_EXPECT(Stream_Seek);

    ok(dlibMove.QuadPart == expect_seek_pos, "unexpected seek pos %lu\n", dlibMove.LowPart);
    ok(dwOrigin == STREAM_SEEK_SET, "dwOrigin = %ld\n", dwOrigin);
    This->pos = dlibMove.QuadPart;
    if(plibNewPosition)
        plibNewPosition->QuadPart = This->pos;
    return S_OK;
}

static HRESULT WINAPI Stream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_CopyTo(IStream *iface, IStream *pstm, ULARGE_INTEGER cb,
                                    ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Revert(IStream *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                        ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_UnlockRegion(IStream *iface,
        ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Stat(IStream *iface, STATSTG *pstatstg, DWORD dwStatFlag)
{
    CHECK_EXPECT(Stream_Stat);
    ok(dwStatFlag == STATFLAG_NONAME, "dwStatFlag = %lx\n", dwStatFlag);
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Clone(IStream *iface, IStream **ppstm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IStreamVtbl StreamVtbl = {
    Stream_QueryInterface,
    Stream_AddRef,
    Stream_Release,
    Stream_Read,
    Stream_Write,
    Stream_Seek,
    Stream_SetSize,
    Stream_CopyTo,
    Stream_Commit,
    Stream_Revert,
    Stream_LockRegion,
    Stream_UnlockRegion,
    Stream_Stat,
    Stream_Clone
};

static IStream *create_test_stream(void)
{
    TestStream *stream;
    stream = HeapAlloc(GetProcessHeap(), 0, sizeof(*stream));
    stream->IStream_iface.lpVtbl = &StreamVtbl;
    stream->ref = 1;
    stream->pos = 0;
    return &stream->IStream_iface;
}

#define test_stream_read(a,b,c,d) _test_stream_read(__LINE__,a,b,c,d)
static void _test_stream_read(unsigned line, IStream *stream, HRESULT exhres, const char *exdata, unsigned read_size)
{
    ULONG read = 0xdeadbeed, exread = strlen(exdata);
    char buf[1024];
    HRESULT hres;

    if(read_size == -1)
        read_size = sizeof(buf)-1;

    hres = IStream_Read(stream, buf, read_size, &read);
    ok_(__FILE__,line)(hres == exhres, "Read returned %08lx, expected %08lx\n", hres, exhres);
    ok_(__FILE__,line)(read == exread, "unexpected read size %lu, expected %lu\n", read, exread);
    buf[read] = 0;
    ok_(__FILE__,line)(read == exread && !memcmp(buf, exdata, read), "unexpected data %s\n", buf);
}

static void test_SetData(void)
{
    IStream *stream, *stream2, *test_stream;
    IMimeBody *body;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_IMimeBody, NULL, CLSCTX_INPROC_SERVER, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    /* Need to call InitNew before Load otherwise Load crashes with native inetcomm */
    hr = IMimeBody_InitNew(body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    stream = create_stream_from_string(msg1);
    hr = IMimeBody_Load(body, stream);
    ok(hr == S_OK, "ret %08lx\n", hr);
    IStream_Release(stream);

    test_stream = create_test_stream();
    hr = IMimeBody_SetData(body, IET_BINARY, "text", "plain", &IID_IStream, test_stream);

    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeBody_IsContentType(body, "text", "plain");
    todo_wine
    ok(hr == S_OK, "ret %08lx\n", hr);

    test_current_encoding(body, IET_BINARY);

    SET_EXPECT(Stream_Stat);
    SET_EXPECT(Stream_Seek_END);
    hr = IMimeBody_GetData(body, IET_BINARY, &stream);
    CHECK_CALLED(Stream_Stat);
    CHECK_CALLED(Stream_Seek_END);
    ok(hr == S_OK, "GetData failed %08lx\n", hr);
    ok(stream != test_stream, "unexpected stream\n");

    SET_EXPECT(Stream_Seek);
    SET_EXPECT(Stream_Read);
    test_stream_read(stream, S_OK, "012", 3);
    CHECK_CALLED(Stream_Seek);
    CHECK_CALLED(Stream_Read);

    SET_EXPECT(Stream_Stat);
    SET_EXPECT(Stream_Seek_END);
    hr = IMimeBody_GetData(body, IET_BINARY, &stream2);
    CHECK_CALLED(Stream_Stat);
    CHECK_CALLED(Stream_Seek_END);
    ok(hr == S_OK, "GetData failed %08lx\n", hr);
    ok(stream2 != stream, "unexpected stream\n");

    SET_EXPECT(Stream_Seek);
    SET_EXPECT(Stream_Read);
    test_stream_read(stream2, S_OK, "01", 2);
    CHECK_CALLED(Stream_Seek);
    CHECK_CALLED(Stream_Read);

    expect_seek_pos = 3;
    SET_EXPECT(Stream_Seek);
    SET_EXPECT(Stream_Read);
    test_stream_read(stream, S_OK, "345", 3);
    CHECK_CALLED(Stream_Seek);
    CHECK_CALLED(Stream_Read);

    IStream_Release(stream);
    IStream_Release(stream2);
    IStream_Release(test_stream);

    stream = create_stream_from_string(" \t\r\n|}~YWJj ZGV|}~mZw== \t"); /* "abcdefg" in base64 obscured by invalid chars */
    hr = IMimeBody_SetData(body, IET_BASE64, "text", "plain", &IID_IStream, stream);
    IStream_Release(stream);
    ok(hr == S_OK, "SetData failed: %08lx\n", hr);

    test_current_encoding(body, IET_BASE64);

    hr = IMimeBody_GetData(body, IET_BINARY, &stream);
    ok(hr == S_OK, "GetData failed %08lx\n", hr);

    test_stream_read(stream, S_OK, "abc", 3);
    test_stream_read(stream, S_OK, "defg", -1);

    IStream_Release(stream);

    hr = IMimeBody_GetData(body, IET_BASE64, &stream);
    ok(hr == S_OK, "GetData failed %08lx\n", hr);

    test_stream_read(stream, S_OK, " \t\r", 3);
    IStream_Release(stream);

    stream = create_stream_from_string(" =3d=3D\"one\" \t=\r\ntw=  o=\nx3\n=34\r\n5");
    hr = IMimeBody_SetData(body, IET_QP, "text", "plain", &IID_IStream, stream);
    IStream_Release(stream);
    ok(hr == S_OK, "SetData failed: %08lx\n", hr);

    test_current_encoding(body, IET_QP);

    hr = IMimeBody_GetData(body, IET_BINARY, &stream);
    ok(hr == S_OK, "GetData failed %08lx\n", hr);

    test_stream_read(stream, S_OK, " ==\"one\" \ttw=o=3\n4\r\n5", -1);

    IStream_Release(stream);

    IMimeBody_Release(body);
}

static void test_Allocator(void)
{
    HRESULT hr;
    IMimeAllocator *alloc;

    hr = MimeOleGetAllocator(&alloc);
    ok(hr == S_OK, "ret %08lx\n", hr);
    IMimeAllocator_Release(alloc);
}

static void test_CreateMessage(void)
{
    HRESULT hr;
    IMimeMessage *msg;
    IStream *stream;
    LONG ref;
    HBODY hbody, hbody2;
    IMimeBody *body;
    BODYOFFSETS offsets;
    ULONG count;
    FINDBODY find_struct;
    HCHARSET hcs;
    HBODY handle = NULL;

    char text[] = "text";
    HBODY *body_list;
    PROPVARIANT prop;
    static const char att_pritype[] = "att:pri-content-type";

    hr = MimeOleCreateMessage(NULL, &msg);
    ok(hr == S_OK, "ret %08lx\n", hr);

    stream = create_stream_from_string(msg1);

    hr = IMimeMessage_Load(msg, stream);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeMessage_CountBodies(msg, HBODY_ROOT, TRUE, &count);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(count == 3, "got %ld\n", count);

    hr = IMimeMessage_CountBodies(msg, HBODY_ROOT, FALSE, &count);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(count == 3, "got %ld\n", count);

    hr = IMimeMessage_BindToObject(msg, HBODY_ROOT, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeBody_GetOffsets(body, &offsets);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(offsets.cbBoundaryStart == 0, "got %ld\n", offsets.cbBoundaryStart);
    ok(offsets.cbHeaderStart == 0, "got %ld\n", offsets.cbHeaderStart);
    ok(offsets.cbBodyStart == 359, "got %ld\n", offsets.cbBodyStart);
    ok(offsets.cbBodyEnd == 666, "got %ld\n", offsets.cbBodyEnd);
    IMimeBody_Release(body);

    hr = IMimeMessage_GetBody(msg, IBL_ROOT, NULL, &hbody);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_GetHandle(body, NULL);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    hr = IMimeBody_GetHandle(body, &handle);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(handle != NULL, "handle %p\n", handle);

    hr = IMimeMessage_GetBody(msg, IBL_PARENT, hbody, NULL);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    hbody2 = (HBODY)0xdeadbeef;
    hr = IMimeMessage_GetBody(msg, IBL_PARENT, hbody, &hbody2);
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);
    ok(hbody2 == NULL, "hbody2 %p\n", hbody2);

    PropVariantInit(&prop);
    hr = IMimeMessage_GetBodyProp(msg, hbody, att_pritype, 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(prop.vt == VT_LPSTR, "vt %08x\n", prop.vt);
    ok(!strcasecmp(prop.pszVal, "multipart"), "got %s\n", prop.pszVal);
    PropVariantClear(&prop);

    hr = IMimeMessage_GetBody(msg, IBL_FIRST, hbody, &hbody);
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeMessage_BindToObject(msg, hbody, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_GetHandle(body, &handle);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(handle == hbody, "handle %p\n", handle);

    hr = IMimeBody_GetOffsets(body, &offsets);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(offsets.cbBoundaryStart == 405, "got %ld\n", offsets.cbBoundaryStart);
    ok(offsets.cbHeaderStart == 428, "got %ld\n", offsets.cbHeaderStart);
    ok(offsets.cbBodyStart == 518, "got %ld\n", offsets.cbBodyStart);
    ok(offsets.cbBodyEnd == 523, "got %ld\n", offsets.cbBodyEnd);

    hr = IMimeBody_GetCharset(body, &hcs);
    ok(hr == S_OK, "ret %08lx\n", hr);
    todo_wine
    {
        ok(hcs != NULL, "Expected non-NULL charset\n");
    }

    IMimeBody_Release(body);

    hr = IMimeMessage_GetBody(msg, IBL_NEXT, hbody, &hbody);
    ok(hr == S_OK, "ret %08lx\n", hr);
    hr = IMimeMessage_BindToObject(msg, hbody, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_GetHandle(body, &handle);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(handle == hbody, "handle %p\n", handle);

    hr = IMimeBody_GetOffsets(body, &offsets);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(offsets.cbBoundaryStart == 525, "got %ld\n", offsets.cbBoundaryStart);
    ok(offsets.cbHeaderStart == 548, "got %ld\n", offsets.cbHeaderStart);
    ok(offsets.cbBodyStart == 629, "got %ld\n", offsets.cbBodyStart);
    ok(offsets.cbBodyEnd == 639, "got %ld\n", offsets.cbBodyEnd);
    IMimeBody_Release(body);

    find_struct.pszPriType = text;
    find_struct.pszSubType = NULL;

    hr = IMimeMessage_FindFirst(msg, &find_struct, &hbody);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeMessage_FindNext(msg, &find_struct, &hbody);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeMessage_FindNext(msg, &find_struct, &hbody);
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);

    hr = IMimeMessage_GetAttachments(msg, &count, &body_list);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(count == 2, "got %ld\n", count);
    if(count == 2)
    {
        IMimeBody *attachment;
        PROPVARIANT prop;

        PropVariantInit(&prop);

        hr = IMimeMessage_BindToObject(msg, body_list[0], &IID_IMimeBody, (void**)&attachment);
        ok(hr == S_OK, "ret %08lx\n", hr);

        hr = IMimeBody_IsContentType(attachment, "multipart", NULL);
        ok(hr == S_FALSE, "ret %08lx\n", hr);

        test_current_encoding(attachment, IET_8BIT);

        prop.vt = VT_LPSTR;
        hr = IMimeBody_GetProp(attachment, "Content-Transfer-Encoding", 0, &prop);
        ok(hr == S_OK, "ret %08lx\n", hr);

        ok(prop.vt == VT_LPSTR, "type %d\n", prop.vt);
        ok(!strcmp(prop.pszVal, "8bit"), "got  %s\n", prop.pszVal);
        PropVariantClear(&prop);

        hr = IMimeBody_IsType(attachment, IBT_ATTACHMENT);
        todo_wine ok(hr == S_FALSE, "ret %08lx\n", hr);

        IMimeBody_Release(attachment);

        hr = IMimeMessage_BindToObject(msg, body_list[1], &IID_IMimeBody, (void**)&attachment);
        ok(hr == S_OK, "ret %08lx\n", hr);

        hr = IMimeBody_IsContentType(attachment, "multipart", NULL);
        ok(hr == S_FALSE, "ret %08lx\n", hr);

        test_current_encoding(attachment, IET_7BIT);

        prop.vt = VT_LPSTR;
        hr = IMimeBody_GetProp(attachment, "Content-Transfer-Encoding", 0, &prop);
        ok(hr == S_OK, "ret %08lx\n", hr);
        ok(prop.vt == VT_LPSTR, "type %d\n", prop.vt);
        ok(!strcmp(prop.pszVal, "7bit"), "got  %s\n", prop.pszVal);
        PropVariantClear(&prop);

        hr = IMimeBody_IsType(attachment, IBT_ATTACHMENT);
        ok(hr == S_OK, "ret %08lx\n", hr);

        IMimeBody_Release(attachment);
    }
    CoTaskMemFree(body_list);

    hr = IMimeBody_GetCharset(body, &hcs);
    ok(hr == S_OK, "ret %08lx\n", hr);
    todo_wine
    {
        ok(hcs != NULL, "Expected non-NULL charset\n");
    }

    IMimeMessage_Release(msg);

    ref = IStream_AddRef(stream);
    ok(ref == 2 ||
       broken(ref == 1), /* win95 */
       "ref %ld\n", ref);
    IStream_Release(stream);

    IStream_Release(stream);
}

static void test_mhtml_message(void)
{
    IMimeMessage *mime_message;
    IMimeBody *mime_body;
    HBODY *body_list;
    IStream *stream;
    ULONG count;
    HRESULT hres;

    hres = MimeOleCreateMessage(NULL, &mime_message);
    ok(hres == S_OK, "MimeOleCreateMessage failed: %08lx\n", hres);

    stream = create_stream_from_string(mhtml_page1);
    hres = IMimeMessage_Load(mime_message, stream);
    IStream_Release(stream);
    ok(hres == S_OK, "Load failed: %08lx\n", hres);

    hres = IMimeMessage_CountBodies(mime_message, HBODY_ROOT, TRUE, &count);
    ok(hres == S_OK, "CountBodies failed: %08lx\n", hres);
    ok(count == 3, "got %ld\n", count);

    hres = IMimeMessage_GetAttachments(mime_message, &count, &body_list);
    ok(hres == S_OK, "GetAttachments failed: %08lx\n", hres);
    ok(count == 2, "count = %lu\n", count);

    hres = IMimeMessage_BindToObject(mime_message, body_list[0], &IID_IMimeBody, (void**)&mime_body);
    ok(hres == S_OK, "BindToObject failed: %08lx\n", hres);

    hres = IMimeBody_GetData(mime_body, IET_BINARY, &stream);
    ok(hres == S_OK, "GetData failed: %08lx\n", hres);
    test_stream_read(stream, S_OK, "<HTML></HTML>", -1);
    IStream_Release(stream);

    test_current_encoding(mime_body, IET_QP);

    IMimeBody_Release(mime_body);

    hres = IMimeMessage_BindToObject(mime_message, body_list[1], &IID_IMimeBody, (void**)&mime_body);
    ok(hres == S_OK, "BindToObject failed: %08lx\n", hres);

    test_current_encoding(mime_body, IET_BASE64);

    hres = IMimeBody_GetData(mime_body, IET_BINARY, &stream);
    ok(hres == S_OK, "GetData failed: %08lx\n", hres);
    test_stream_read(stream, S_OK, "Test", -1);
    IStream_Release(stream);

    IMimeBody_Release(mime_body);

    CoTaskMemFree(body_list);

    IMimeMessage_Release(mime_message);
}

static void test_MessageSetProp(void)
{
    static const char topic[] = "wine topic";
    HRESULT hr;
    IMimeMessage *msg;
    IMimeBody *body;
    PROPVARIANT prop;

    hr = MimeOleCreateMessage(NULL, &msg);
    ok(hr == S_OK, "ret %08lx\n", hr);

    PropVariantInit(&prop);

    hr = IMimeMessage_BindToObject(msg, HBODY_ROOT, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_SetProp(body, NULL, 0, &prop);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    hr = IMimeBody_SetProp(body, "Thread-Topic", 0, NULL);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.pszVal, topic);
    hr = IMimeBody_SetProp(body, "Thread-Topic", 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    hr = IMimeBody_GetProp(body, NULL, 0, &prop);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    hr = IMimeBody_GetProp(body, "Thread-Topic", 0, NULL);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    hr = IMimeBody_GetProp(body, "Wine-Topic", 0, &prop);
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);

    prop.vt = VT_LPSTR;
    hr = IMimeBody_GetProp(body, "Thread-Topic", 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    if(hr == S_OK)
    {
        ok(prop.vt == VT_LPSTR, "type %d\n", prop.vt);
        ok(!strcmp(prop.pszVal, topic), "got  %s\n", prop.pszVal);
        PropVariantClear(&prop);
    }

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.pszVal, topic);
    hr = IMimeBody_SetProp(body, PIDTOSTR(PID_HDR_SUBJECT), 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    prop.vt = VT_LPSTR;
    hr = IMimeBody_GetProp(body, PIDTOSTR(PID_HDR_SUBJECT), 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    if(hr == S_OK)
    {
        ok(prop.vt == VT_LPSTR, "type %d\n", prop.vt);
        ok(!strcmp(prop.pszVal, topic), "got  %s\n", prop.pszVal);
        PropVariantClear(&prop);
    }

    /* Using the name or PID returns the same result. */
    prop.vt = VT_LPSTR;
    hr = IMimeBody_GetProp(body, "Subject", 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    if(hr == S_OK)
    {
        ok(prop.vt == VT_LPSTR, "type %d\n", prop.vt);
        ok(!strcmp(prop.pszVal, topic), "got  %s\n", prop.pszVal);
        PropVariantClear(&prop);
    }

    prop.vt = VT_LPWSTR;
    hr = IMimeBody_GetProp(body, "Subject", 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    if(hr == S_OK)
    {
        ok(prop.vt == VT_LPWSTR, "type %d\n", prop.vt);
        ok(!lstrcmpW(prop.pwszVal, L"wine topic"), "got %s\n", wine_dbgstr_w(prop.pwszVal));
        PropVariantClear(&prop);
    }

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.pszVal, topic);
    hr = IMimeBody_SetProp(body, PIDTOSTR(PID_HDR_TO), 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    /* Out of Range PID */
    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.pszVal, topic);
    hr = IMimeBody_SetProp(body, PIDTOSTR(124), 0, &prop);
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    IMimeBody_Release(body);
    IMimeMessage_Release(msg);
}

static void test_MessageGetPropInfo(void)
{
    static const char topic[] = "wine topic";
    static const char subject[] = "wine testing";
    HRESULT hr;
    IMimeMessage *msg;
    IMimeBody *body;
    PROPVARIANT prop;
    MIMEPROPINFO info;

    hr = MimeOleCreateMessage(NULL, &msg);
    ok(hr == S_OK, "ret %08lx\n", hr);

    PropVariantInit(&prop);

    hr = IMimeMessage_BindToObject(msg, HBODY_ROOT, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.pszVal, topic);
    hr = IMimeBody_SetProp(body, "Thread-Topic", 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(subject)+1);
    strcpy(prop.pszVal, subject);
    hr = IMimeBody_SetProp(body, PIDTOSTR(PID_HDR_SUBJECT), 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    memset(&info, 0, sizeof(info));
    info.dwMask = PIM_ENCODINGTYPE | PIM_FLAGS | PIM_PROPID;
    hr = IMimeBody_GetPropInfo(body, NULL, &info);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    memset(&info, 0, sizeof(info));
    info.dwMask = PIM_ENCODINGTYPE | PIM_FLAGS | PIM_PROPID;
    hr = IMimeBody_GetPropInfo(body, "Subject", NULL);
    ok(hr == E_INVALIDARG, "ret %08lx\n", hr);

    memset(&info, 0xfe, sizeof(info));
    info.dwMask = PIM_ENCODINGTYPE | PIM_FLAGS | PIM_PROPID;
    hr = IMimeBody_GetPropInfo(body, "Subject", &info);
    ok(hr == S_OK, "ret %08lx\n", hr);
    if(hr == S_OK)
    {
       ok(info.dwMask & (PIM_ENCODINGTYPE | PIM_FLAGS| PIM_PROPID), "Invalid mask 0x%08lx\n", info.dwFlags);
       todo_wine ok(info.dwFlags & 0x10000000, "Invalid flags 0x%08lx\n", info.dwFlags);
       ok(info.ietEncoding == 0, "Invalid encoding %d\n", info.ietEncoding);
       ok(info.dwPropId == PID_HDR_SUBJECT, "Invalid propid %ld\n", info.dwPropId);
       ok(info.cValues == 0xfefefefe, "Invalid cValues %ld\n", info.cValues);
    }

    memset(&info, 0xfe, sizeof(info));
    info.dwMask = 0;
    hr = IMimeBody_GetPropInfo(body, "Subject", &info);
    ok(hr == S_OK, "ret %08lx\n", hr);
    if(hr == S_OK)
    {
       ok(info.dwMask == 0, "Invalid mask 0x%08lx\n", info.dwFlags);
       ok(info.dwFlags == 0xfefefefe, "Invalid flags 0x%08lx\n", info.dwFlags);
       ok(info.ietEncoding == -16843010, "Invalid encoding %d\n", info.ietEncoding);
       ok(info.dwPropId == -16843010, "Invalid propid %ld\n", info.dwPropId);
    }

    memset(&info, 0xfe, sizeof(info));
    info.dwMask = 0;
    info.dwPropId = 1024;
    info.ietEncoding = 99;
    hr = IMimeBody_GetPropInfo(body, "Subject", &info);
    ok(hr == S_OK, "ret %08lx\n", hr);
    if(hr == S_OK)
    {
       ok(info.dwMask == 0, "Invalid mask 0x%08lx\n", info.dwFlags);
       ok(info.dwFlags == 0xfefefefe, "Invalid flags 0x%08lx\n", info.dwFlags);
       ok(info.ietEncoding == 99, "Invalid encoding %d\n", info.ietEncoding);
       ok(info.dwPropId == 1024, "Invalid propid %ld\n", info.dwPropId);
    }

    memset(&info, 0, sizeof(info));
    info.dwMask = PIM_ENCODINGTYPE | PIM_FLAGS | PIM_PROPID;
    hr = IMimeBody_GetPropInfo(body, "Invalid Property", &info);
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);

    IMimeBody_Release(body);
    IMimeMessage_Release(msg);
}

static void test_MessageOptions(void)
{
    static const char string[] = "XXXXX";
    static const char zero[] =   "0";
    HRESULT hr;
    IMimeMessage *msg;
    PROPVARIANT prop;

    hr = MimeOleCreateMessage(NULL, &msg);
    ok(hr == S_OK, "ret %08lx\n", hr);

    PropVariantInit(&prop);

    prop.vt = VT_BOOL;
    prop.boolVal = TRUE;
    hr = IMimeMessage_SetOption(msg, OID_HIDE_TNEF_ATTACHMENTS, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    hr = IMimeMessage_GetOption(msg, OID_HIDE_TNEF_ATTACHMENTS, &prop);
    todo_wine ok(hr == S_OK, "ret %08lx\n", hr);
    todo_wine ok(prop.vt == VT_BOOL, "vt %08x\n", prop.vt);
    todo_wine ok(prop.boolVal == TRUE, "Hide Attachments got %d\n", prop.boolVal);
    PropVariantClear(&prop);

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(string)+1);
    strcpy(prop.pszVal, string);
    hr = IMimeMessage_SetOption(msg, OID_HIDE_TNEF_ATTACHMENTS, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    hr = IMimeMessage_GetOption(msg, OID_HIDE_TNEF_ATTACHMENTS, &prop);
    todo_wine ok(hr == S_OK, "ret %08lx\n", hr);
    todo_wine ok(prop.vt == VT_BOOL, "vt %08x\n", prop.vt);
    todo_wine ok(prop.boolVal == TRUE, "Hide Attachments got %d\n", prop.boolVal);
    PropVariantClear(&prop);

    /* Invalid property type doesn't change the value */
    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(zero)+1);
    strcpy(prop.pszVal, zero);
    hr = IMimeMessage_SetOption(msg, OID_HIDE_TNEF_ATTACHMENTS, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    hr = IMimeMessage_GetOption(msg, OID_HIDE_TNEF_ATTACHMENTS, &prop);
    todo_wine ok(hr == S_OK, "ret %08lx\n", hr);
    todo_wine ok(prop.vt == VT_BOOL, "vt %08x\n", prop.vt);
    todo_wine ok(prop.boolVal == TRUE, "Hide Attachments got %d\n", prop.boolVal);
    PropVariantClear(&prop);

    /* Invalid OID */
    prop.vt = VT_BOOL;
    prop.boolVal = TRUE;
    hr = IMimeMessage_SetOption(msg, 0xff00000a, &prop);
    ok(hr == MIME_E_INVALID_OPTION_ID, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    /* Out of range before type. */
    prop.vt = VT_I4;
    prop.lVal = 1;
    hr = IMimeMessage_SetOption(msg, 0xff00000a, &prop);
    ok(hr == MIME_E_INVALID_OPTION_ID, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    IMimeMessage_Release(msg);
}

static void test_BindToObject(void)
{
    HRESULT hr;
    IMimeMessage *msg;
    IMimeBody *body;
    ULONG count;

    hr = MimeOleCreateMessage(NULL, &msg);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeMessage_CountBodies(msg, HBODY_ROOT, TRUE, &count);
    ok(hr == S_OK, "ret %08lx\n", hr);
    ok(count == 1, "got %ld\n", count);

    hr = IMimeMessage_BindToObject(msg, HBODY_ROOT, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);
    IMimeBody_Release(body);

    IMimeMessage_Release(msg);
}

static void test_BodyDeleteProp(void)
{
    static const char topic[] = "wine topic";
    HRESULT hr;
    IMimeMessage *msg;
    IMimeBody *body;
    PROPVARIANT prop;

    hr = MimeOleCreateMessage(NULL, &msg);
    ok(hr == S_OK, "ret %08lx\n", hr);

    PropVariantInit(&prop);

    hr = IMimeMessage_BindToObject(msg, HBODY_ROOT, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_DeleteProp(body, "Subject");
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);

    hr = IMimeBody_DeleteProp(body, PIDTOSTR(PID_HDR_SUBJECT));
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.pszVal, topic);
    hr = IMimeBody_SetProp(body, "Subject", 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    hr = IMimeBody_DeleteProp(body, "Subject");
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_GetProp(body, "Subject", 0, &prop);
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);

    prop.vt = VT_LPSTR;
    prop.pszVal = CoTaskMemAlloc(strlen(topic)+1);
    strcpy(prop.pszVal, topic);
    hr = IMimeBody_SetProp(body, PIDTOSTR(PID_HDR_SUBJECT), 0, &prop);
    ok(hr == S_OK, "ret %08lx\n", hr);
    PropVariantClear(&prop);

    hr = IMimeBody_DeleteProp(body, PIDTOSTR(PID_HDR_SUBJECT));
    ok(hr == S_OK, "ret %08lx\n", hr);

    hr = IMimeBody_GetProp(body, PIDTOSTR(PID_HDR_SUBJECT), 0, &prop);
    ok(hr == MIME_E_NOT_FOUND, "ret %08lx\n", hr);

    IMimeBody_Release(body);
    IMimeMessage_Release(msg);
}

static void test_MimeOleGetPropertySchema(void)
{
    HRESULT hr;
    IMimePropertySchema *schema = NULL;

    hr = MimeOleGetPropertySchema(&schema);
    ok(hr == S_OK, "ret %08lx\n", hr);

    IMimePropertySchema_Release(schema);
}

typedef struct {
    const char *url;
    const char *content;
    const WCHAR *mime;
    const char *data;
} mhtml_binding_test_t;

static const mhtml_binding_test_t binding_tests[] = {
    {
        "mhtml:file://%s",
        mhtml_page1,
        L"text/html",
        "<HTML></HTML>"
    },
    {
        "mhtml:file://%s!http://winehq.org/mhtmltest.html",
        mhtml_page1,
        L"Image/Jpeg",
        "Test"
    }
};

static const mhtml_binding_test_t *current_binding_test;
static IInternetProtocol *current_binding_protocol;

static HRESULT WINAPI BindInfo_QueryInterface(IInternetBindInfo *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BindInfo_AddRef(IInternetBindInfo *iface)
{
    return 2;
}

static ULONG WINAPI BindInfo_Release(IInternetBindInfo *iface)
{
    return 1;
}

static HRESULT WINAPI BindInfo_GetBindInfo(IInternetBindInfo *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    CHECK_EXPECT(GetBindInfo);

    ok(grfBINDF != NULL, "grfBINDF == NULL\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %ld\n", pbindinfo->cbSize);

    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON | BINDF_NEEDFILE;
    return S_OK;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface, ULONG ulStringType, LPOLESTR *ppwzStr,
        ULONG cEl, ULONG *pcElFetched)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IInternetBindInfoVtbl InternetBindInfoVtbl = {
    BindInfo_QueryInterface,
    BindInfo_AddRef,
    BindInfo_Release,
    BindInfo_GetBindInfo,
    BindInfo_GetBindString
};

static IInternetBindInfo bind_info = {
    &InternetBindInfoVtbl
};

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&CLSID_MimeEdit, guidService)) {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ok(0, "unexpected service %s\n", wine_dbgstr_guid(guidService));
    return E_FAIL;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider service_provider = { &ServiceProviderVtbl };

static HRESULT WINAPI ProtocolSink_QueryInterface(IInternetProtocolSink *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        *ppv = &service_provider;
        return S_OK;
    }

    *ppv = NULL;
    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    return 2;
}

static ULONG WINAPI ProtocolSink_Release(IInternetProtocolSink *iface)
{
    return 1;
}

static HRESULT WINAPI ProtocolSink_Switch(IInternetProtocolSink *iface, PROTOCOLDATA *pProtocolData)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolSink_ReportProgress(IInternetProtocolSink *iface, ULONG ulStatusCode,
        const WCHAR *szStatusText)
{
    switch(ulStatusCode) {
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        ok(!lstrcmpW(szStatusText, current_binding_test->mime), "status text %s\n", wine_dbgstr_w(szStatusText));
        return S_OK;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        CHECK_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        return S_OK;
    default:
        ok(0, "unexpected call %lu %s\n", ulStatusCode, wine_dbgstr_w(szStatusText));
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF, ULONG ulProgress,
        ULONG ulProgressMax)
{
    char buf[1024];
    DWORD read;
    HRESULT hres;

    CHECK_EXPECT(ReportData);

    ok(!ulProgress, "ulProgress = %lu\n", ulProgress);
    ok(ulProgress == ulProgressMax, "ulProgress != ulProgressMax\n");
    ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_INTERMEDIATEDATANOTIFICATION
                   | BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE | BSCF_AVAILABLEDATASIZEUNKNOWN),
            "grcf = %08lx\n", grfBSCF);

    hres = IInternetProtocol_Read(current_binding_protocol, buf, sizeof(buf), &read);
    ok(hres == S_OK, "Read failed: %08lx\n", hres);
    buf[read] = 0;
    ok(!strcmp(buf, current_binding_test->data), "unexpected data: %s\n", buf);

    hres = IInternetProtocol_Read(current_binding_protocol, buf, sizeof(buf), &read);
    ok(hres == S_FALSE, "Read failed: %08lx\n", hres);
    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult, DWORD dwError,
        LPCWSTR szResult)
{
    CHECK_EXPECT(ReportResult);
    ok(hrResult == S_OK, "hrResult = %08lx\n", hrResult);
    ok(!dwError, "dwError = %lu\n", dwError);
    ok(!szResult, "szResult = %s\n", wine_dbgstr_w(szResult));
    return S_OK;
}

static IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    ProtocolSink_QueryInterface,
    ProtocolSink_AddRef,
    ProtocolSink_Release,
    ProtocolSink_Switch,
    ProtocolSink_ReportProgress,
    ProtocolSink_ReportData,
    ProtocolSink_ReportResult
};

static IInternetProtocolSink protocol_sink = { &InternetProtocolSinkVtbl };

static void test_mhtml_protocol_binding(const mhtml_binding_test_t *test)
{
    char file_name[MAX_PATH+32], *p, urla[INTERNET_MAX_URL_LENGTH];
    WCHAR test_url[INTERNET_MAX_URL_LENGTH];
    IInternetProtocol *protocol;
    IUnknown *unk;
    HRESULT hres;
    HANDLE file;
    DWORD size;
    BOOL ret;

    p = file_name + GetCurrentDirectoryA(sizeof(file_name), file_name);
    *p++ = '\\';
    strcpy(p, "winetest.mht");

    file = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");

    WriteFile(file, test->content, strlen(test->content), &size, NULL);
    CloseHandle(file);

    sprintf(urla, test->url, file_name);
    MultiByteToWideChar(CP_ACP, 0, urla, -1, test_url, ARRAY_SIZE(test_url));

    hres = CoCreateInstance(&CLSID_IMimeHtmlProtocol, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not create protocol handler: %08lx\n", hres);

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetProtocolEx, (void**)&unk);
    ok(hres == E_NOINTERFACE, "Could get IInternetProtocolEx\n");

    current_binding_test = test;
    current_binding_protocol = protocol;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
    SET_EXPECT(ReportData);
    SET_EXPECT(ReportResult);
    hres = IInternetProtocol_Start(protocol, test_url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08lx\n", hres);
    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
    todo_wine CHECK_CALLED(ReportProgress_CACHEFILENAMEAVAILABLE);
    CHECK_CALLED(ReportData);
    CHECK_CALLED(ReportResult);

    IInternetProtocol_Release(protocol);
    ret = DeleteFileA("winetest.mht");
    ok(ret, "DeleteFile failed: %lu\n", GetLastError());
}

static const struct {
    const WCHAR *base_url;
    const WCHAR *relative_url;
    const WCHAR *expected_result;
    BOOL todo;
} combine_tests[] = {
    {
        L"mhtml:file:///c:/dir/test.mht", L"http://test.org",
        L"mhtml:file:///c:/dir/test.mht!x-usc:http://test.org"
    }, {
        L"mhtml:file:///c:/dir/test.mht", L"3D\"http://test.org\"",
        L"mhtml:file:///c:/dir/test.mht!x-usc:3D\"http://test.org\""
    }, {
        L"mhtml:file:///c:/dir/test.mht", L"123abc",
        L"mhtml:file:///c:/dir/test.mht!x-usc:123abc"
    }, {
        L"mhtml:file:///c:/dir/test.mht!x-usc:http://test.org", L"123abc",
        L"mhtml:file:///c:/dir/test.mht!x-usc:123abc"
    }, {
        L"MhtMl:file:///c:/dir/test.mht!x-usc:http://test.org/dir/dir2/file.html", L"../..",
        L"mhtml:file:///c:/dir/test.mht!x-usc:../.."
    }, {
        L"mhtml:file:///c:/dir/test.mht!x-usc:file:///c:/dir/dir2/file.html", L"../..",
        L"mhtml:file:///c:/dir/test.mht!x-usc:../.."
    }, {
        L"mhtml:file:///c:/dir/test.mht!x-usc:http://test.org", L"",
        L"mhtml:file:///c:/dir/test.mht"
    }, {
        L"mhtml:file:///c:/dir/test.mht!x-usc:http://test.org", L"mhtml:file:///d:/file.html",
        L"file:///d:/file.html", TRUE
    }, {
        L"mhtml:file:///c:/dir/test.mht!x-usc:http://test.org", L"mhtml:file:///c:/dir2/test.mht!x-usc:http://test.org",
        L"mhtml:file:///c:/dir2/test.mht!x-usc:http://test.org", TRUE
    }, {
        L"mhtml:file:///c:/dir/test.mht!http://test.org", L"123abc",
        L"mhtml:file:///c:/dir/test.mht!x-usc:123abc"
    }, {
        L"mhtml:file:///c:/dir/test.mht!http://test.org", L"",
        L"mhtml:file:///c:/dir/test.mht"
    }
};

static void test_mhtml_protocol_info(void)
{
    WCHAR combined_url[INTERNET_MAX_URL_LENGTH];
    IInternetProtocolInfo *protocol_info;
    DWORD combined_len;
    unsigned i, exlen;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_IMimeHtmlProtocol, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == S_OK, "Could not create protocol info: %08lx\n", hres);

    for(i = 0; i < ARRAY_SIZE(combine_tests); i++) {
        combined_len = 0xdeadbeef;
        hres = IInternetProtocolInfo_CombineUrl(protocol_info, combine_tests[i].base_url,
                                                combine_tests[i].relative_url, ICU_BROWSER_MODE,
                                                combined_url, ARRAY_SIZE(combined_url), &combined_len, 0);
        todo_wine_if(combine_tests[i].todo)
        ok(hres == S_OK, "[%u] CombineUrl failed: %08lx\n", i, hres);
        if(SUCCEEDED(hres)) {
            exlen = lstrlenW(combine_tests[i].expected_result);
            ok(combined_len == exlen, "[%u] combined len is %lu, expected %u\n", i, combined_len, exlen);
            ok(!lstrcmpW(combined_url, combine_tests[i].expected_result), "[%u] combined URL is %s, expected %s\n",
               i, wine_dbgstr_w(combined_url), wine_dbgstr_w(combine_tests[i].expected_result));

            combined_len = 0xdeadbeef;
            hres = IInternetProtocolInfo_CombineUrl(protocol_info, combine_tests[i].base_url,
                                                    combine_tests[i].relative_url, ICU_BROWSER_MODE,
                                                    combined_url, exlen, &combined_len, 0);
            ok(hres == E_FAIL, "[%u] CombineUrl returned: %08lx\n", i, hres);
            ok(!combined_len, "[%u] combined_len = %lu\n", i, combined_len);
        }
    }

    hres = IInternetProtocolInfo_CombineUrl(protocol_info, L"http://test.org", L"http://test.org",
                                            ICU_BROWSER_MODE, combined_url, ARRAY_SIZE(combined_url),
                                            &combined_len, 0);
    ok(hres == E_FAIL, "CombineUrl failed: %08lx\n", hres);

    IInternetProtocolInfo_Release(protocol_info);
}

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl outer_vtbl = {
    outer_QueryInterface,
    outer_AddRef,
    outer_Release
};

static BOOL broken_mhtml_resolver;

static void test_mhtml_protocol(void)
{
    IUnknown outer = { &outer_vtbl };
    IClassFactory *class_factory;
    IUnknown *unk, *unk2;
    unsigned i;
    HRESULT hres;

    /* test class factory */
    hres = CoGetClassObject(&CLSID_IMimeHtmlProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&unk2);
    ok(hres == E_NOINTERFACE, "IInternetProtocolInfo supported\n");

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&class_factory);
    ok(hres == S_OK, "Could not get IClassFactory iface: %08lx\n", hres);
    IUnknown_Release(unk);

    hres = IClassFactory_CreateInstance(class_factory, &outer, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CreateInstance returned: %08lx\n", hres);
    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocol, (void**)&unk2);
    ok(hres == S_OK, "Could not get IInternetProtocol iface: %08lx\n", hres);
    IUnknown_Release(unk2);
    IUnknown_Release(unk);

    hres = IClassFactory_CreateInstance(class_factory, (IUnknown*)0xdeadbeef, &IID_IInternetProtocol, (void**)&unk2);
    ok(hres == CLASS_E_NOAGGREGATION, "CreateInstance returned: %08lx\n", hres);

    IClassFactory_Release(class_factory);

    if(!broken_mhtml_resolver)
        test_mhtml_protocol_info();

    for(i = 0; i < ARRAY_SIZE(binding_tests); i++)
        test_mhtml_protocol_binding(binding_tests + i);
}

static void test_MimeOleObjectFromMoniker(void)
{
    IMoniker *mon, *new_mon;
    WCHAR *mhtml_url;
    IBindCtx *bind_ctx;
    IUnknown *unk;
    unsigned i;
    HRESULT hres;

    static const struct {
        const WCHAR *url;
        const WCHAR *mhtml_url;
    } tests[] = {
        {L"file:///x:\\dir\\file.mht", L"mhtml:file://x:\\dir\\file.mht"},
        {L"file:///x:/dir/file.mht", L"mhtml:file://x:\\dir\\file.mht"},
        {L"http://www.winehq.org/index.html?query#hash", L"mhtml:http://www.winehq.org/index.html?query#hash"},
        {L"../test.mht", L"mhtml:../test.mht"}
    };

    for(i = 0; i < ARRAY_SIZE(tests); i++) {
        hres = CreateURLMoniker(NULL, tests[i].url, &mon);
        ok(hres == S_OK, "CreateURLMoniker failed: %08lx\n", hres);

        hres = CreateBindCtx(0, &bind_ctx);
        ok(hres == S_OK, "CreateBindCtx failed: %08lx\n", hres);

        hres = MimeOleObjectFromMoniker(0, mon, bind_ctx, &IID_IUnknown, (void**)&unk, &new_mon);
        ok(hres == S_OK || broken(!i && hres == INET_E_RESOURCE_NOT_FOUND), "MimeOleObjectFromMoniker failed: %08lx\n", hres);
        IBindCtx_Release(bind_ctx);
        if(hres == INET_E_RESOURCE_NOT_FOUND) { /* winxp */
            win_skip("Broken MHTML behaviour found. Skipping some tests.\n");
            broken_mhtml_resolver = TRUE;
            return;
        }

        hres = IMoniker_GetDisplayName(new_mon, NULL, NULL, &mhtml_url);
        ok(hres == S_OK, "GetDisplayName failed: %08lx\n", hres);
        ok(!lstrcmpW(mhtml_url, tests[i].mhtml_url), "[%d] unexpected mhtml URL: %s\n", i, wine_dbgstr_w(mhtml_url));
        CoTaskMemFree(mhtml_url);

        IUnknown_Release(unk);
        IMoniker_Release(new_mon);
        IMoniker_Release(mon);
    }
}

START_TEST(mimeole)
{
    OleInitialize(NULL);
    test_CreateVirtualStream();
    test_CreateSecurity();
    test_CreateBody();
    test_SetData();
    test_Allocator();
    test_CreateMessage();
    test_MessageSetProp();
    test_MessageGetPropInfo();
    test_MessageOptions();
    test_BindToObject();
    test_BodyDeleteProp();
    test_MimeOleGetPropertySchema();
    test_mhtml_message();
    test_MimeOleObjectFromMoniker();
    test_mhtml_protocol();
    OleUninitialize();
}
