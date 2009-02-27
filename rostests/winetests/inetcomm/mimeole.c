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
#define NONAMELESSUNION

#include "initguid.h"
#include "windows.h"
#include "ole2.h"
#include "ocidl.h"

#include "mimeole.h"

#include <stdio.h>
#include <assert.h>

#include "wine/test.h"

static char msg1[] =
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

static void test_CreateVirtualStream(void)
{
    HRESULT hr;
    IStream *pstm;

    hr = MimeOleCreateVirtualStream(&pstm);
    ok(hr == S_OK, "ret %08x\n", hr);

    IStream_Release(pstm);
}

static void test_CreateSecurity(void)
{
    HRESULT hr;
    IMimeSecurity *sec;

    hr = MimeOleCreateSecurity(&sec);
    ok(hr == S_OK, "ret %08x\n", hr);

    IMimeSecurity_Release(sec);
}

static void test_CreateBody(void)
{
    HRESULT hr;
    IMimeBody *body;
    HBODY handle = (void *)0xdeadbeef;
    IStream *in;
    LARGE_INTEGER off;
    ULARGE_INTEGER pos;
    ENCODINGTYPE enc;
    ULONG count, found_param, i;
    MIMEPARAMINFO *param_info;
    IMimeAllocator *alloc;
    BODYOFFSETS offsets;

    hr = CoCreateInstance(&CLSID_IMimeBody, NULL, CLSCTX_INPROC_SERVER, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeBody_GetHandle(body, &handle);
    ok(hr == MIME_E_NO_DATA, "ret %08x\n", hr);
    ok(handle == NULL, "handle %p\n", handle);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &in);
    ok(hr == S_OK, "ret %08x\n", hr);
    IStream_Write(in, msg1, sizeof(msg1) - 1, NULL);
    off.QuadPart = 0;
    IStream_Seek(in, off, STREAM_SEEK_SET, NULL);

    /* Need to call InitNew before Load otherwise Load crashes with native inetcomm */
    hr = IMimeBody_InitNew(body);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeBody_GetCurrentEncoding(body, &enc);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(enc == IET_7BIT, "encoding %d\n", enc);

    hr = IMimeBody_Load(body, in);
    ok(hr == S_OK, "ret %08x\n", hr);
    off.QuadPart = 0;
    IStream_Seek(in, off, STREAM_SEEK_CUR, &pos);
    ok(pos.u.LowPart == 359, "pos %u\n", pos.u.LowPart);

    hr = IMimeBody_IsContentType(body, "multipart", "mixed");
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_IsContentType(body, "text", "plain");
    ok(hr == S_FALSE, "ret %08x\n", hr);
    hr = IMimeBody_IsContentType(body, NULL, "mixed");
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_IsType(body, IBT_EMPTY);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = IMimeBody_SetData(body, IET_8BIT, "text", "plain", &IID_IStream, in);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_IsContentType(body, "text", "plain");
    todo_wine
        ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_GetCurrentEncoding(body, &enc);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(enc == IET_8BIT, "encoding %d\n", enc);

    memset(&offsets, 0xcc, sizeof(offsets));
    hr = IMimeBody_GetOffsets(body, &offsets);
    ok(hr == MIME_E_NO_DATA, "ret %08x\n", hr);
    ok(offsets.cbBoundaryStart == 0, "got %d\n", offsets.cbBoundaryStart);
    ok(offsets.cbHeaderStart == 0, "got %d\n", offsets.cbHeaderStart);
    ok(offsets.cbBodyStart == 0, "got %d\n", offsets.cbBodyStart);
    ok(offsets.cbBodyEnd == 0, "got %d\n", offsets.cbBodyEnd);

    hr = IMimeBody_IsType(body, IBT_EMPTY);
    ok(hr == S_FALSE, "got %08x\n", hr);

    hr = MimeOleGetAllocator(&alloc);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeBody_GetParameters(body, "nothere", &count, &param_info);
    ok(hr == MIME_E_NOT_FOUND, "ret %08x\n", hr);
    ok(count == 0, "got %d\n", count);
    ok(!param_info, "got %p\n", param_info);

    hr = IMimeBody_GetParameters(body, "bar", &count, &param_info);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(count == 0, "got %d\n", count);
    ok(!param_info, "got %p\n", param_info);

    hr = IMimeBody_GetParameters(body, "Content-Type", &count, &param_info);
    ok(hr == S_OK, "ret %08x\n", hr);
    todo_wine  /* native adds a charset parameter */
        ok(count == 4, "got %d\n", count);
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
    ok(found_param == 2, "matched %d params\n", found_param);

    hr = IMimeAllocator_FreeParamInfoArray(alloc, count, param_info, TRUE);
    ok(hr == S_OK, "ret %08x\n", hr);
    IMimeAllocator_Release(alloc);

    IStream_Release(in);
    IMimeBody_Release(body);
}

static void test_Allocator(void)
{
    HRESULT hr;
    IMimeAllocator *alloc;

    hr = MimeOleGetAllocator(&alloc);
    ok(hr == S_OK, "ret %08x\n", hr);
    IMimeAllocator_Release(alloc);
}

static void test_CreateMessage(void)
{
    HRESULT hr;
    IMimeMessage *msg;
    IStream *stream;
    LARGE_INTEGER pos;
    LONG ref;
    HBODY hbody;
    IMimeBody *body;
    BODYOFFSETS offsets;
    ULONG count;
    FINDBODY find_struct;
    HCHARSET hcs;

    char text[] = "text";
    HBODY *body_list;
    PROPVARIANT prop;
    static char att_pritype[] = "att:pri-content-type";

    hr = MimeOleCreateMessage(NULL, &msg);
    ok(hr == S_OK, "ret %08x\n", hr);

    CreateStreamOnHGlobal(NULL, TRUE, &stream);
    IStream_Write(stream, msg1, sizeof(msg1) - 1, NULL);
    pos.QuadPart = 0;
    IStream_Seek(stream, pos, STREAM_SEEK_SET, NULL);

    hr = IMimeMessage_Load(msg, stream);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeMessage_CountBodies(msg, HBODY_ROOT, TRUE, &count);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(count == 3, "got %d\n", count);

    hr = IMimeMessage_CountBodies(msg, HBODY_ROOT, FALSE, &count);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(count == 3, "got %d\n", count);

    hr = IMimeMessage_BindToObject(msg, HBODY_ROOT, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_GetOffsets(body, &offsets);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(offsets.cbBoundaryStart == 0, "got %d\n", offsets.cbBoundaryStart);
    ok(offsets.cbHeaderStart == 0, "got %d\n", offsets.cbHeaderStart);
    ok(offsets.cbBodyStart == 359, "got %d\n", offsets.cbBodyStart);
    ok(offsets.cbBodyEnd == 666, "got %d\n", offsets.cbBodyEnd);
    IMimeBody_Release(body);

    hr = IMimeMessage_GetBody(msg, IBL_ROOT, NULL, &hbody);

    PropVariantInit(&prop);
    hr = IMimeMessage_GetBodyProp(msg, hbody, att_pritype, 0, &prop);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(prop.vt == VT_LPSTR, "vt %08x\n", prop.vt);
    ok(!strcasecmp(prop.u.pszVal, "multipart"), "got %s\n", prop.u.pszVal);
    PropVariantClear(&prop);

    hr = IMimeMessage_GetBody(msg, IBL_FIRST, hbody, &hbody);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeMessage_BindToObject(msg, hbody, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_GetOffsets(body, &offsets);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(offsets.cbBoundaryStart == 405, "got %d\n", offsets.cbBoundaryStart);
    ok(offsets.cbHeaderStart == 428, "got %d\n", offsets.cbHeaderStart);
    ok(offsets.cbBodyStart == 518, "got %d\n", offsets.cbBodyStart);
    ok(offsets.cbBodyEnd == 523, "got %d\n", offsets.cbBodyEnd);

    hr = IMimeBody_GetCharset(body, &hcs);
    ok(hr == S_OK, "ret %08x\n", hr);
    todo_wine
    {
        ok(hcs != NULL, "Expected non-NULL charset\n");
    }

    IMimeBody_Release(body);

    hr = IMimeMessage_GetBody(msg, IBL_NEXT, hbody, &hbody);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeMessage_BindToObject(msg, hbody, &IID_IMimeBody, (void**)&body);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeBody_GetOffsets(body, &offsets);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(offsets.cbBoundaryStart == 525, "got %d\n", offsets.cbBoundaryStart);
    ok(offsets.cbHeaderStart == 548, "got %d\n", offsets.cbHeaderStart);
    ok(offsets.cbBodyStart == 629, "got %d\n", offsets.cbBodyStart);
    ok(offsets.cbBodyEnd == 639, "got %d\n", offsets.cbBodyEnd);
    IMimeBody_Release(body);

    find_struct.pszPriType = text;
    find_struct.pszSubType = NULL;

    hr = IMimeMessage_FindFirst(msg, &find_struct, &hbody);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeMessage_FindNext(msg, &find_struct, &hbody);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeMessage_FindNext(msg, &find_struct, &hbody);
    ok(hr == MIME_E_NOT_FOUND, "ret %08x\n", hr);

    hr = IMimeMessage_GetAttachments(msg, &count, &body_list);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(count == 2, "got %d\n", count);
    CoTaskMemFree(body_list);

    hr = IMimeMessage_GetCharset(body, &hcs);
    ok(hr == S_OK, "ret %08x\n", hr);
    todo_wine
    {
        ok(hcs != NULL, "Expected non-NULL charset\n");
    }

    IMimeMessage_Release(msg);

    ref = IStream_AddRef(stream);
    ok(ref == 2 ||
       broken(ref == 1), /* win95 */
       "ref %d\n", ref);
    IStream_Release(stream);

    IStream_Release(stream);
}

START_TEST(mimeole)
{
    OleInitialize(NULL);
    test_CreateVirtualStream();
    test_CreateSecurity();
    test_CreateBody();
    test_Allocator();
    test_CreateMessage();
    OleUninitialize();
}
