/*
 * User Marshaling Tests
 *
 * Copyright 2004-2006 Robert Shearman for CodeWeavers
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
#define CONST_VTABLE
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "objidl.h"

#include "wine/test.h"

ULONG __RPC_USER HMETAFILE_UserSize(ULONG *, ULONG, HMETAFILE *);
unsigned char * __RPC_USER HMETAFILE_UserMarshal(ULONG *, unsigned char *, HMETAFILE *);
unsigned char * __RPC_USER HMETAFILE_UserUnmarshal(ULONG *, unsigned char *, HMETAFILE *);
void __RPC_USER HMETAFILE_UserFree(ULONG *, HMETAFILE *);

ULONG __RPC_USER HENHMETAFILE_UserSize(ULONG *, ULONG, HENHMETAFILE *);
unsigned char * __RPC_USER HENHMETAFILE_UserMarshal  (ULONG *, unsigned char *, HENHMETAFILE *);
unsigned char * __RPC_USER HENHMETAFILE_UserUnmarshal(ULONG *, unsigned char *, HENHMETAFILE *);
void  __RPC_USER HENHMETAFILE_UserFree(ULONG *, HENHMETAFILE *);

ULONG __RPC_USER HMETAFILEPICT_UserSize(ULONG *, ULONG, HMETAFILEPICT *);
unsigned char * __RPC_USER HMETAFILEPICT_UserMarshal  (ULONG *, unsigned char *, HMETAFILEPICT *);
unsigned char * __RPC_USER HMETAFILEPICT_UserUnmarshal(ULONG *, unsigned char *, HMETAFILEPICT *);
void __RPC_USER HMETAFILEPICT_UserFree(ULONG *, HMETAFILEPICT *);

ULONG __RPC_USER HBRUSH_UserSize(ULONG *, ULONG, HBRUSH *);
unsigned char * __RPC_USER HBRUSH_UserMarshal(ULONG *, unsigned char *, HBRUSH *);
unsigned char * __RPC_USER HBRUSH_UserUnmarshal(ULONG *, unsigned char *, HBRUSH *);
void __RPC_USER HBRUSH_UserFree(ULONG *, HBRUSH *);

static BOOL g_expect_user_alloc;
static void * WINAPI user_allocate(SIZE_T size)
{
    ok(g_expect_user_alloc, "unexpected user_allocate call\n");
    return CoTaskMemAlloc(size);
}

static BOOL g_expect_user_free;
static void WINAPI user_free(void *p)
{
    ok(g_expect_user_free, "unexpected user_free call\n");
    CoTaskMemFree(p);
}

static void init_user_marshal_cb(USER_MARSHAL_CB *umcb,
                                 PMIDL_STUB_MESSAGE stub_msg,
                                 PRPC_MESSAGE rpc_msg, unsigned char *buffer,
                                 unsigned int size, MSHCTX context)
{
    memset(rpc_msg, 0, sizeof(*rpc_msg));
    rpc_msg->Buffer = buffer;
    rpc_msg->BufferLength = size;

    memset(stub_msg, 0, sizeof(*stub_msg));
    stub_msg->RpcMsg = rpc_msg;
    stub_msg->Buffer = buffer;
    stub_msg->pfnAllocate = user_allocate;
    stub_msg->pfnFree = user_free;

    memset(umcb, 0, sizeof(*umcb));
    umcb->Flags = MAKELONG(context, NDR_LOCAL_DATA_REPRESENTATION);
    umcb->pStubMsg = stub_msg;
    umcb->Signature = USER_MARSHAL_CB_SIGNATURE;
    umcb->CBType = buffer ? USER_MARSHAL_CB_UNMARSHALL : USER_MARSHAL_CB_BUFFER_SIZE;
}

#define RELEASEMARSHALDATA WM_USER

struct host_object_data
{
    IStream *stream;
    IID iid;
    IUnknown *object;
    MSHLFLAGS marshal_flags;
    HANDLE marshal_event;
    IMessageFilter *filter;
};

static DWORD CALLBACK host_object_proc(LPVOID p)
{
    struct host_object_data *data = p;
    HRESULT hr;
    MSG msg;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (data->filter)
    {
        IMessageFilter * prev_filter = NULL;
        hr = CoRegisterMessageFilter(data->filter, &prev_filter);
        if (prev_filter) IMessageFilter_Release(prev_filter);
        ok(hr == S_OK, "got %08lx\n", hr);
    }

    hr = CoMarshalInterface(data->stream, &data->iid, data->object, MSHCTX_INPROC, NULL, data->marshal_flags);
    ok(hr == S_OK, "got %08lx\n", hr);

    /* force the message queue to be created before signaling parent thread */
    PeekMessageA(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(data->marshal_event);

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == RELEASEMARSHALDATA)
        {
            CoReleaseMarshalData(data->stream);
            SetEvent((HANDLE)msg.lParam);
        }
        else
            DispatchMessageA(&msg);
    }

    free(data);

    CoUninitialize();

    return hr;
}

static DWORD start_host_object2(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, IMessageFilter *filter, HANDLE *thread)
{
    DWORD tid = 0;
    HANDLE marshal_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    struct host_object_data *data = malloc(sizeof(*data));

    data->stream = stream;
    data->iid = *riid;
    data->object = object;
    data->marshal_flags = marshal_flags;
    data->marshal_event = marshal_event;
    data->filter = filter;

    *thread = CreateThread(NULL, 0, host_object_proc, data, 0, &tid);

    /* wait for marshaling to complete before returning */
    ok( !WaitForSingleObject(marshal_event, 10000), "wait timed out\n" );
    CloseHandle(marshal_event);

    return tid;
}

static void end_host_object(DWORD tid, HANDLE thread)
{
    BOOL ret = PostThreadMessageA(tid, WM_QUIT, 0, 0);
    ok(ret, "PostThreadMessage failed with error %ld\n", GetLastError());
    /* be careful of races - don't return until hosting thread has terminated */
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);
}

static const char cf_marshaled[] =
{
    0x9, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0,
    0x9, 0x0, 0x0, 0x0,
    'M', 0x0, 'y', 0x0,
    'F', 0x0, 'o', 0x0,
    'r', 0x0, 'm', 0x0,
    'a', 0x0, 't', 0x0,
    0x0, 0x0
};

static void test_marshal_CLIPFORMAT(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    ULONG i, size;
    CLIPFORMAT cf = RegisterClipboardFormatA("MyFormat");
    CLIPFORMAT cf2;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = CLIPFORMAT_UserSize(&umcb.Flags, 1, &cf);
    ok(size == 12 + sizeof(cf_marshaled) ||
       broken(size == 16 + sizeof(cf_marshaled)),  /* win64 adds 4 extra (unused) bytes */
              "CLIPFORMAT: Wrong size %ld\n", size);

    buffer = malloc(size);
    memset( buffer, 0xcc, size );
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = CLIPFORMAT_UserMarshal(&umcb.Flags, buffer + 1, &cf);
    ok(buffer_end == buffer + 12 + sizeof(cf_marshaled), "got %p buffer %p\n", buffer_end, buffer);
    ok(*(LONG *)(buffer + 4) == WDT_REMOTE_CALL, "CLIPFORMAT: Context should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(LONG *)(buffer + 0));
    ok(*(DWORD *)(buffer + 8) == cf, "CLIPFORMAT: Marshaled value should be 0x%04x instead of 0x%04lx\n", cf, *(DWORD *)(buffer + 4));
    ok(!memcmp(buffer + 12, cf_marshaled, min( sizeof(cf_marshaled), size-12 )), "Marshaled data differs\n");
    if (size > sizeof(cf_marshaled) + 12)  /* make sure the extra bytes are not used */
        for (i = sizeof(cf_marshaled) + 12; i < size; i++)
            ok( buffer[i] == 0xcc, "buffer offset %lu has been set to %x\n", i, buffer[i] );

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = CLIPFORMAT_UserUnmarshal(&umcb.Flags, buffer + 1, &cf2);
    ok(buffer_end == buffer + 12 + sizeof(cf_marshaled), "got %p buffer %p\n", buffer_end, buffer);
    ok(cf == cf2, "CLIPFORMAT: Didn't unmarshal properly\n");
    free(buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    CLIPFORMAT_UserFree(&umcb.Flags, &cf2);
}

static void test_marshal_HWND(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    ULONG size;
    HWND hwnd = GetDesktopWindow();
    HWND hwnd2;
    wireHWND wirehwnd;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HWND_UserSize(&umcb.Flags, 1, &hwnd);
    ok(size == 4 + sizeof(*wirehwnd), "Wrong size %ld\n", size);

    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HWND_UserMarshal(&umcb.Flags, buffer + 1, &hwnd);
    ok(buffer_end == buffer + size, "got %p buffer %p\n", buffer_end, buffer);
    wirehwnd = (wireHWND)(buffer + 4);
    ok(wirehwnd->fContext == WDT_INPROC_CALL, "Context should be WDT_INPROC_CALL instead of 0x%08lx\n", wirehwnd->fContext);
    ok(wirehwnd->u.hInproc == (LONG_PTR)hwnd, "Marshaled value should be %p instead of %lx\n", hwnd, wirehwnd->u.hRemote);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HWND_UserUnmarshal(&umcb.Flags, buffer + 1, &hwnd2);
    ok(buffer_end == buffer + size, "got %p buffer %p\n", buffer_end, buffer);
    ok(hwnd == hwnd2, "Didn't unmarshal properly\n");
    free(buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HWND_UserFree(&umcb.Flags, &hwnd2);
}

static void test_marshal_HGLOBAL(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer;
    ULONG size, block_size;
    HGLOBAL hglobal;
    HGLOBAL hglobal2;
    unsigned char *wirehglobal;
    int i;

    hglobal = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HGLOBAL_UserSize(&umcb.Flags, 0, &hglobal);
    /* native is poorly programmed and allocates 4/8 bytes more than it needs to
     * here - Wine doesn't have to emulate that */
    ok((size == 8) || broken(size == 12) || broken(size == 16), "Size should be 8, instead of %ld\n", size);
    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    HGLOBAL_UserMarshal(&umcb.Flags, buffer, &hglobal);
    wirehglobal = buffer;
    ok(*(ULONG *)wirehglobal == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(ULONG *)wirehglobal);
    wirehglobal += sizeof(ULONG);
    ok(*(ULONG *)wirehglobal == 0, "buffer+4 should be HGLOBAL\n");
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    hglobal2 = NULL;
    HGLOBAL_UserUnmarshal(&umcb.Flags, buffer, &hglobal2);
    ok(hglobal2 == hglobal, "Didn't unmarshal properly\n");
    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HGLOBAL_UserFree(&umcb.Flags, &hglobal2);


    for(block_size = 0; block_size <= 17; block_size++)
    {
        ULONG actual_size, expected_size;

        hglobal = GlobalAlloc(0, block_size);
        buffer = GlobalLock(hglobal);
        for (i = 0; i < block_size; i++)
            buffer[i] = i;
        GlobalUnlock(hglobal);
        actual_size = GlobalSize(hglobal);
        expected_size = actual_size + 5 * sizeof(DWORD);
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
        size = HGLOBAL_UserSize(&umcb.Flags, 0, &hglobal);
        /* native is poorly programmed and allocates 4/8 bytes more than it needs to
         * here - Wine doesn't have to emulate that */
        ok(size == expected_size ||
           broken(size == expected_size + 4) ||
           broken(size == expected_size + 8),
           "%ld: got size %ld\n", block_size, size);
        buffer = malloc(size);
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
        HGLOBAL_UserMarshal(&umcb.Flags, buffer, &hglobal);
        wirehglobal = buffer;
        ok(*(ULONG *)wirehglobal == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(ULONG *)wirehglobal);
        wirehglobal += sizeof(ULONG);
        ok(*(ULONG *)wirehglobal == (ULONG)(ULONG_PTR)hglobal, "buffer+0x4 should be HGLOBAL\n");
        wirehglobal += sizeof(ULONG);
        ok(*(ULONG *)wirehglobal == actual_size, "%ld: buffer+0x8 %08lx\n", block_size, *(ULONG *)wirehglobal);
        wirehglobal += sizeof(ULONG);
        ok(*(ULONG *)wirehglobal == (ULONG)(ULONG_PTR)hglobal, "buffer+0xc should be HGLOBAL\n");
        wirehglobal += sizeof(ULONG);
        ok(*(ULONG *)wirehglobal == actual_size, "%ld: buffer+0x10 %08lx\n", block_size, *(ULONG *)wirehglobal);
        wirehglobal += sizeof(ULONG);
        for (i = 0; i < block_size; i++)
            ok(wirehglobal[i] == i, "buffer+0x%x should be %d\n", 0x10 + i, i);

        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
        hglobal2 = NULL;
        HGLOBAL_UserUnmarshal(&umcb.Flags, buffer, &hglobal2);
        ok(hglobal2 != NULL, "Didn't unmarshal properly\n");
        free(buffer);
        init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
        HGLOBAL_UserFree(&umcb.Flags, &hglobal2);
        GlobalFree(hglobal);
    }
}

static HENHMETAFILE create_emf(void)
{
    const RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateEnhMetaFileA(NULL, NULL, &rect, "HENHMETAFILE Marshaling Test\0Test\0\0");
    ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL);
    return CloseEnhMetaFile(hdc);
}

static void test_marshal_HENHMETAFILE(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    ULONG size;
    HENHMETAFILE hemf;
    HENHMETAFILE hemf2 = NULL;
    unsigned char *wirehemf;

    hemf = create_emf();

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HENHMETAFILE_UserSize(&umcb.Flags, 1, &hemf);
    ok(size > 24, "size should be at least 24 bytes, not %ld\n", size);
    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = HENHMETAFILE_UserMarshal(&umcb.Flags, buffer + 1, &hemf);
    ok(buffer_end == buffer + size, "got %p buffer %p\n", buffer_end, buffer);
    wirehemf = buffer + 4;
    ok(*(DWORD *)wirehemf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (DWORD)(DWORD_PTR)hemf, "wirestgm + 0x4 should be hemf instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (size - 0x14), "wirestgm + 0x8 should be size - 0x14 instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (size - 0x14), "wirestgm + 0xc should be size - 0x14 instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == EMR_HEADER, "wirestgm + 0x10 should be EMR_HEADER instead of %ld\n", *(DWORD *)wirehemf);
    /* ... rest of data not tested - refer to tests for GetEnhMetaFileBits
     * at this point */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = HENHMETAFILE_UserUnmarshal(&umcb.Flags, buffer + 1, &hemf2);
    ok(buffer_end == buffer + size, "got %p buffer %p\n", buffer_end, buffer);
    ok(hemf2 != NULL, "HENHMETAFILE didn't unmarshal\n");
    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    HENHMETAFILE_UserFree(&umcb.Flags, &hemf2);
    DeleteEnhMetaFile(hemf);

    /* test NULL emf */
    hemf = NULL;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HENHMETAFILE_UserSize(&umcb.Flags, 1, &hemf);
    ok(size == 12, "size should be 12 bytes, not %ld\n", size);
    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = HENHMETAFILE_UserMarshal(&umcb.Flags, buffer + 1, &hemf);
    ok(buffer_end == buffer + size, "got %p buffer %p\n", buffer_end, buffer);
    wirehemf = buffer + 4;
    ok(*(DWORD *)wirehemf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehemf);
    wirehemf += sizeof(DWORD);
    ok(*(DWORD *)wirehemf == (DWORD)(DWORD_PTR)hemf, "wirestgm + 0x4 should be hemf instead of 0x%08lx\n", *(DWORD *)wirehemf);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = HENHMETAFILE_UserUnmarshal(&umcb.Flags, buffer + 1, &hemf2);
    ok(buffer_end == buffer + size, "got %p buffer %p\n", buffer_end, buffer);
    ok(hemf2 == NULL, "NULL HENHMETAFILE didn't unmarshal\n");
    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    HENHMETAFILE_UserFree(&umcb.Flags, &hemf2);
}

static HMETAFILE create_mf(void)
{
    RECT rect = {0, 0, 100, 100};
    HDC hdc = CreateMetaFileA(NULL);
    ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rect, "Test String", strlen("Test String"), NULL);
    return CloseMetaFile(hdc);
}

static void test_marshal_HMETAFILE(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer;
    ULONG size;
    HMETAFILE hmf;
    HMETAFILE hmf2 = NULL;
    unsigned char *wirehmf;

    hmf = create_mf();

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HMETAFILE_UserSize(&umcb.Flags, 0, &hmf);
    ok(size > 20, "size should be at least 20 bytes, not %ld\n", size);
    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILE_UserMarshal(&umcb.Flags, buffer, &hmf);
    wirehmf = buffer;
    ok(*(DWORD *)wirehmf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(DWORD *)wirehmf == (DWORD)(DWORD_PTR)hmf, "wirestgm + 0x4 should be hmf instead of 0x%08lx\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(DWORD *)wirehmf == (size - 0x10), "wirestgm + 0x8 should be size - 0x10 instead of 0x%08lx\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(DWORD *)wirehmf == (size - 0x10), "wirestgm + 0xc should be size - 0x10 instead of 0x%08lx\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(WORD *)wirehmf == 1, "wirestgm + 0x10 should be 1 instead of 0x%08lx\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    /* ... rest of data not tested - refer to tests for GetMetaFileBits
     * at this point */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILE_UserUnmarshal(&umcb.Flags, buffer, &hmf2);
    ok(hmf2 != NULL, "HMETAFILE didn't unmarshal\n");
    free(buffer);
    HMETAFILE_UserFree(&umcb.Flags, &hmf2);
    DeleteMetaFile(hmf);

    /* test NULL emf */
    hmf = NULL;

    size = HMETAFILE_UserSize(&umcb.Flags, 0, &hmf);
    ok(size == 8, "size should be 8 bytes, not %ld\n", size);
    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILE_UserMarshal(&umcb.Flags, buffer, &hmf);
    wirehmf = buffer;
    ok(*(DWORD *)wirehmf == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);
    ok(*(DWORD *)wirehmf == (DWORD)(DWORD_PTR)hmf, "wirestgm + 0x4 should be hmf instead of 0x%08lx\n", *(DWORD *)wirehmf);
    wirehmf += sizeof(DWORD);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILE_UserUnmarshal(&umcb.Flags, buffer, &hmf2);
    ok(hmf2 == NULL, "NULL HMETAFILE didn't unmarshal\n");
    free(buffer);
    HMETAFILE_UserFree(&umcb.Flags, &hmf2);
}

#define USER_MARSHAL_PTR_PREFIX \
  ( (DWORD)'U'         | ( (DWORD)'s' << 8 ) | \
  ( (DWORD)'e' << 16 ) | ( (DWORD)'r' << 24 ) )

static void test_marshal_HMETAFILEPICT(void)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    ULONG size;
    HMETAFILEPICT hmfp;
    HMETAFILEPICT hmfp2 = NULL;
    METAFILEPICT *pmfp;
    unsigned char *wirehmfp;

    hmfp = GlobalAlloc(GMEM_MOVEABLE, sizeof(*pmfp));
    pmfp = GlobalLock(hmfp);
    pmfp->mm = MM_ISOTROPIC;
    pmfp->xExt = 1;
    pmfp->yExt = 2;
    pmfp->hMF = create_mf();
    GlobalUnlock(hmfp);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HMETAFILEPICT_UserSize(&umcb.Flags, 1, &hmfp);
    ok(size > 24, "size should be at least 24 bytes, not %ld\n", size);
    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = HMETAFILEPICT_UserMarshal(&umcb.Flags, buffer + 1, &hmfp);
    wirehmfp = buffer + 4;
    ok(*(DWORD *)wirehmfp == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == (DWORD)(DWORD_PTR)hmfp, "wirestgm + 0x4 should be hmf instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == MM_ISOTROPIC, "wirestgm + 0x8 should be MM_ISOTROPIC instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == 1, "wirestgm + 0xc should be 1 instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == 2, "wirestgm + 0x10 should be 2 instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == USER_MARSHAL_PTR_PREFIX, "wirestgm + 0x14 should be \"User\" instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == WDT_REMOTE_CALL, "wirestgm + 0x18 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    pmfp = GlobalLock(hmfp);
    ok(*(DWORD *)wirehmfp == (DWORD)(DWORD_PTR)pmfp->hMF, "wirestgm + 0x1c should be pmfp->hMF instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    GlobalUnlock(hmfp);
    wirehmfp += sizeof(DWORD);
    /* Note use (buffer_end - buffer) instead of size here, because size is an
     * overestimate with native */
    ok(*(DWORD *)wirehmfp == (buffer_end - buffer - 0x2c), "wirestgm + 0x20 should be size - 0x34 instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == (buffer_end - buffer - 0x2c), "wirestgm + 0x24 should be size - 0x34 instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(WORD *)wirehmfp == 1, "wirehmfp + 0x28 should be 1 instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    /* ... rest of data not tested - refer to tests for GetMetaFileBits
     * at this point */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    HMETAFILEPICT_UserUnmarshal(&umcb.Flags, buffer + 1, &hmfp2);
    ok(hmfp2 != NULL, "HMETAFILEPICT didn't unmarshal\n");
    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    HMETAFILEPICT_UserFree(&umcb.Flags, &hmfp2);
    pmfp = GlobalLock(hmfp);
    DeleteMetaFile(pmfp->hMF);
    GlobalUnlock(hmfp);
    GlobalFree(hmfp);

    /* test NULL emf */
    hmfp = NULL;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = HMETAFILEPICT_UserSize(&umcb.Flags, 1, &hmfp);
    ok(size == 12, "size should be 12 bytes, not %ld\n", size);
    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = HMETAFILEPICT_UserMarshal(&umcb.Flags, buffer + 1, &hmfp);
    ok(buffer_end == buffer + size, "got %p buffer %p\n", buffer_end, buffer);
    wirehmfp = buffer + 4;
    ok(*(DWORD *)wirehmfp == WDT_REMOTE_CALL, "wirestgm + 0x0 should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);
    ok(*(DWORD *)wirehmfp == (DWORD)(DWORD_PTR)hmfp, "wirestgm + 0x4 should be hmf instead of 0x%08lx\n", *(DWORD *)wirehmfp);
    wirehmfp += sizeof(DWORD);

    hmfp2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = HMETAFILEPICT_UserUnmarshal(&umcb.Flags, buffer + 1, &hmfp2);
    ok(buffer_end == buffer + size, "got %p buffer %p\n", buffer_end, buffer);
    ok(hmfp2 == NULL, "NULL HMETAFILE didn't unmarshal\n");
    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    HMETAFILEPICT_UserFree(&umcb.Flags, &hmfp2);
}

typedef struct
{
    IUnknown IUnknown_iface;
    LONG refs;
} TestUnknown;

static inline TestUnknown *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, TestUnknown, IUnknown_iface);
}

static HRESULT WINAPI Test_IUnknown_QueryInterface(
                                                   LPUNKNOWN iface,
                                                   REFIID riid,
                                                   LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IUnknown_AddRef(LPUNKNOWN iface)
{
    TestUnknown *This = impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI Test_IUnknown_Release(LPUNKNOWN iface)
{
    TestUnknown *This = impl_from_IUnknown(iface);
    return InterlockedDecrement(&This->refs);
}

static const IUnknownVtbl TestUnknown_Vtbl =
{
    Test_IUnknown_QueryInterface,
    Test_IUnknown_AddRef,
    Test_IUnknown_Release,
};

struct test_stream
{
    IStream IStream_iface;
    LONG refs;
};

static inline struct test_stream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct test_stream, IStream_iface);
}

static HRESULT WINAPI Test_IStream_QueryInterface(IStream *iface,
                                                  REFIID riid, LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IStream))
    {
        *ppvObj = iface;
        IStream_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IStream_AddRef(IStream *iface)
{
    struct test_stream *This = impl_from_IStream(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI Test_IStream_Release(IStream *iface)
{
    struct test_stream *This = impl_from_IStream(iface);
    return InterlockedDecrement(&This->refs);
}

static const IStreamVtbl TestStream_Vtbl =
{
    Test_IStream_QueryInterface,
    Test_IStream_AddRef,
    Test_IStream_Release
    /* the rest can be NULLs */
};

static TestUnknown Test_Unknown = { {&TestUnknown_Vtbl}, 1 };
static TestUnknown Test_Unknown2 = { {&TestUnknown_Vtbl}, 1 };
static struct test_stream Test_Stream = { {&TestStream_Vtbl}, 1 };
static struct test_stream Test_Stream2 = { {&TestStream_Vtbl}, 1 };

ULONG __RPC_USER WdtpInterfacePointer_UserSize(ULONG *, ULONG, ULONG, IUnknown *, REFIID);
unsigned char * __RPC_USER WdtpInterfacePointer_UserMarshal(ULONG *, ULONG, unsigned char *, IUnknown *, REFIID);
unsigned char * __RPC_USER WdtpInterfacePointer_UserUnmarshal(ULONG *, unsigned char *, IUnknown **, REFIID);

static void marshal_WdtpInterfacePointer(DWORD umcb_ctx, DWORD ctx, BOOL client, BOOL in, BOOL out)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    ULONG size;
    IUnknown *unk;
    IUnknown *unk2;
    unsigned char *wireip;
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, 0);
    IStream *stm;
    void *marshal_data;
    LARGE_INTEGER zero;
    ULARGE_INTEGER pos;
    DWORD marshal_size;

    /* shows that the WdtpInterfacePointer functions don't marshal anything for
     * NULL pointers, so code using these functions must handle that case
     * itself */

    unk = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, umcb_ctx);
    size = WdtpInterfacePointer_UserSize(&umcb.Flags, ctx, 0, unk, &IID_IUnknown);
    ok(size == 0, "size should be 0 bytes, not %ld\n", size);
    buffer = malloc(size);
    buffer_end = WdtpInterfacePointer_UserMarshal(&umcb.Flags, ctx, buffer, unk, &IID_IUnknown);
    ok(buffer_end == buffer, "buffer_end %p buffer %p\n", buffer_end, buffer);
    free(buffer);

    /* Now for a non-NULL pointer. The marshalled data are two size DWORDS and then
       the result of CoMarshalInterface called with the LOWORD of the ctx */

    unk = &Test_Unknown.IUnknown_iface;
    Test_Unknown.refs = 1;

    CreateStreamOnHGlobal(h, TRUE, &stm);
    CoMarshalInterface(stm, &IID_IUnknown, unk, LOWORD(ctx), NULL, MSHLFLAGS_NORMAL);
    zero.QuadPart = 0;
    IStream_Seek(stm, zero, STREAM_SEEK_CUR, &pos);
    marshal_size = pos.u.LowPart;
    marshal_data = GlobalLock(h);
    todo_wine
    ok(Test_Unknown.refs == 2, "got %ld\n", Test_Unknown.refs);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, umcb_ctx);
    size = WdtpInterfacePointer_UserSize(&umcb.Flags, ctx, 0, unk, &IID_IUnknown);
    ok(size >= marshal_size + 2 * sizeof(DWORD), "marshal size %lx got %lx\n", marshal_size, size);
    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, umcb_ctx);
    buffer_end = WdtpInterfacePointer_UserMarshal(&umcb.Flags, ctx, buffer, unk, &IID_IUnknown);
    todo_wine
    ok(Test_Unknown.refs == 2, "got %ld\n", Test_Unknown.refs);
    wireip = buffer;

    ok(buffer_end == buffer + marshal_size + 2 * sizeof(DWORD), "buffer_end %p buffer %p\n", buffer_end, buffer);

    ok(*(DWORD *)wireip == marshal_size, "wireip + 0x0 should be %lx instead of %lx\n", marshal_size, *(DWORD *)wireip);
    wireip += sizeof(DWORD);
    ok(*(DWORD *)wireip == marshal_size, "wireip + 0x4 should be %lx instead of %lx\n", marshal_size, *(DWORD *)wireip);
    wireip += sizeof(DWORD);

    ok(!memcmp(marshal_data, wireip, marshal_size), "buffer mismatch\n");
    GlobalUnlock(h);
    zero.QuadPart = 0;
    IStream_Seek(stm, zero, STREAM_SEEK_SET, NULL);
    CoReleaseMarshalData(stm);
    IStream_Release(stm);

    Test_Unknown2.refs = 1;
    unk2 = &Test_Unknown2.IUnknown_iface;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, umcb_ctx);
    umcb.pStubMsg->IsClient = client;
    umcb.pStubMsg->fIsIn = in;
    umcb.pStubMsg->fIsOut = out;

    WdtpInterfacePointer_UserUnmarshal(&umcb.Flags, buffer, &unk2, &IID_IUnknown);
    ok(unk2 != NULL, "IUnknown object didn't unmarshal properly\n");
    ok(Test_Unknown.refs == 2, "got %ld\n", Test_Unknown.refs);
    ok(Test_Unknown2.refs == 0, "got %ld\n", Test_Unknown2.refs);
    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_INPROC);
    IUnknown_Release(unk2);
}

static void test_marshal_WdtpInterfacePointer(void)
{
    /*
     * There are two places where we can pass the marshalling ctx: as
     * part of the umcb and as a separate flag.  The loword of that
     * separate flag field is what matters.
     */

    /* All three are marshalled as inproc */
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_INPROC, 0,0,0);
    marshal_WdtpInterfacePointer(MSHCTX_DIFFERENTMACHINE, MSHCTX_INPROC,0,0,0);
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MAKELONG(MSHCTX_INPROC, 0xffff),0,0,0);

    /* All three are marshalled as remote */
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_DIFFERENTMACHINE,0,0,0);
    marshal_WdtpInterfacePointer(MSHCTX_DIFFERENTMACHINE, MSHCTX_DIFFERENTMACHINE,0,0,0);
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MAKELONG(MSHCTX_DIFFERENTMACHINE, 0xffff),0,0,0);

    /* Test different combinations of client, in and out */
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_DIFFERENTMACHINE,0,0,1);
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_DIFFERENTMACHINE,0,1,0);
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_DIFFERENTMACHINE,0,1,1);
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_DIFFERENTMACHINE,1,0,0);
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_DIFFERENTMACHINE,1,0,1);
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_DIFFERENTMACHINE,1,1,0);
    marshal_WdtpInterfacePointer(MSHCTX_INPROC, MSHCTX_DIFFERENTMACHINE,1,1,1);
}

static void marshal_STGMEDIUM(BOOL client, BOOL in, BOOL out)
{
    USER_MARSHAL_CB umcb;
    MIDL_STUB_MESSAGE stub_msg;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end, *expect_buffer, *expect_buffer_end;
    ULONG size, expect_size;
    STGMEDIUM med, med2;
    IUnknown *unk = &Test_Unknown.IUnknown_iface;
    IStream *stm = &Test_Stream.IStream_iface;

    /* TYMED_NULL with pUnkForRelease */

    Test_Unknown.refs = 1;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    expect_size = WdtpInterfacePointer_UserSize(&umcb.Flags, umcb.Flags, 2 * sizeof(DWORD), unk, &IID_IUnknown);
    expect_buffer = malloc(expect_size);
    *(DWORD*)expect_buffer = TYMED_NULL;
    *((DWORD*)expect_buffer + 1) = 0xdeadbeef;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, expect_buffer, expect_size, MSHCTX_DIFFERENTMACHINE);
    expect_buffer_end = WdtpInterfacePointer_UserMarshal(&umcb.Flags, umcb.Flags, expect_buffer + 2 * sizeof(DWORD), unk, &IID_IUnknown);

    med.tymed = TYMED_NULL;
    med.pstg = NULL;
    med.pUnkForRelease = unk;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = STGMEDIUM_UserSize(&umcb.Flags, 0, &med);
    ok(size == expect_size, "size %ld should be %ld bytes\n", size, expect_size);

    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = STGMEDIUM_UserMarshal(&umcb.Flags, buffer, &med);
    ok(buffer_end - buffer == expect_buffer_end - expect_buffer, "buffer size mismatch\n");
    ok(*(DWORD*)buffer == TYMED_NULL, "got %08lx\n", *(DWORD*)buffer);
    ok(*((DWORD*)buffer+1) != 0, "got %08lx\n", *((DWORD*)buffer+1));
    ok(!memcmp(buffer+8, expect_buffer + 8, expect_buffer_end - expect_buffer - 8), "buffer mismatch\n");

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    umcb.pStubMsg->IsClient = client;
    umcb.pStubMsg->fIsIn = in;
    umcb.pStubMsg->fIsOut = out;

    Test_Unknown2.refs = 1;
    med2.tymed = TYMED_NULL;
    med2.pstm = NULL;
    med2.pUnkForRelease = &Test_Unknown2.IUnknown_iface;

    STGMEDIUM_UserUnmarshal(&umcb.Flags, buffer, &med2);

    ok(med2.tymed == TYMED_NULL, "got tymed %lx\n", med2.tymed);
    ok(med2.pUnkForRelease != NULL, "Incorrectly unmarshalled\n");
    ok(Test_Unknown2.refs == 0, "got %ld\n", Test_Unknown2.refs);

    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    STGMEDIUM_UserFree(&umcb.Flags, &med2);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, expect_buffer, expect_size, MSHCTX_DIFFERENTMACHINE);
    med2.tymed = TYMED_NULL;
    med2.pstm = NULL;
    med2.pUnkForRelease = NULL;
    STGMEDIUM_UserUnmarshal(&umcb.Flags, expect_buffer, &med2);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    STGMEDIUM_UserFree(&umcb.Flags, &med2);

    ok(Test_Unknown.refs == 1, "got %ld\n", Test_Unknown.refs);

    free(expect_buffer);

    /* TYMED_ISTREAM with pUnkForRelease */

    Test_Unknown.refs = 1;
    Test_Stream.refs = 1;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    expect_size = WdtpInterfacePointer_UserSize(&umcb.Flags, umcb.Flags, 3 * sizeof(DWORD), (IUnknown*)stm, &IID_IStream);
    expect_size = WdtpInterfacePointer_UserSize(&umcb.Flags, umcb.Flags, expect_size, unk, &IID_IUnknown);

    expect_buffer = malloc(expect_size);
    /* There may be a hole between the two interfaces so init the buffer to something */
    memset(expect_buffer, 0xcc, expect_size);
    *(DWORD*)expect_buffer = TYMED_ISTREAM;
    *((DWORD*)expect_buffer + 1) = 0xdeadbeef;
    *((DWORD*)expect_buffer + 2) = 0xcafe;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, expect_buffer, expect_size, MSHCTX_DIFFERENTMACHINE);
    expect_buffer_end = WdtpInterfacePointer_UserMarshal(&umcb.Flags, umcb.Flags, expect_buffer + 3 * sizeof(DWORD), (IUnknown*)stm, &IID_IStream);
    expect_buffer_end = WdtpInterfacePointer_UserMarshal(&umcb.Flags, umcb.Flags, expect_buffer_end, unk, &IID_IUnknown);

    med.tymed = TYMED_ISTREAM;
    med.pstm = stm;
    med.pUnkForRelease = unk;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = STGMEDIUM_UserSize(&umcb.Flags, 0, &med);
    ok(size == expect_size, "size %ld should be %ld bytes\n", size, expect_size);

    buffer = malloc(size);
    memset(buffer, 0xcc, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = STGMEDIUM_UserMarshal(&umcb.Flags, buffer, &med);
    ok(buffer_end - buffer == expect_buffer_end - expect_buffer, "buffer size mismatch\n");
    ok(*(DWORD*)buffer == TYMED_ISTREAM, "got %08lx\n", *(DWORD*)buffer);
    ok(*((DWORD*)buffer+1) != 0, "got %08lx\n", *((DWORD*)buffer+1));
    ok(*((DWORD*)buffer+2) != 0, "got %08lx\n", *((DWORD*)buffer+2));
    ok(!memcmp(buffer + 12, expect_buffer + 12, (buffer_end - buffer) - 12), "buffer mismatch\n");

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    umcb.pStubMsg->IsClient = client;
    umcb.pStubMsg->fIsIn = in;
    umcb.pStubMsg->fIsOut = out;

    Test_Stream2.refs = 1;
    Test_Unknown2.refs = 1;
    med2.tymed = TYMED_ISTREAM;
    med2.pstm = &Test_Stream2.IStream_iface;
    med2.pUnkForRelease = &Test_Unknown2.IUnknown_iface;

    STGMEDIUM_UserUnmarshal(&umcb.Flags, buffer, &med2);

    ok(med2.tymed == TYMED_ISTREAM, "got tymed %lx\n", med2.tymed);
    ok(med2.pstm != NULL, "Incorrectly unmarshalled\n");
    ok(med2.pUnkForRelease != NULL, "Incorrectly unmarshalled\n");
    ok(Test_Stream2.refs == 0, "got %ld\n", Test_Stream2.refs);
    ok(Test_Unknown2.refs == 0, "got %ld\n", Test_Unknown2.refs);

    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    STGMEDIUM_UserFree(&umcb.Flags, &med2);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, expect_buffer, expect_size, MSHCTX_DIFFERENTMACHINE);
    med2.tymed = TYMED_NULL;
    med2.pstm = NULL;
    med2.pUnkForRelease = NULL;
    STGMEDIUM_UserUnmarshal(&umcb.Flags, expect_buffer, &med2);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    STGMEDIUM_UserFree(&umcb.Flags, &med2);

    ok(Test_Unknown.refs == 1, "got %ld\n", Test_Unknown.refs);
    ok(Test_Stream.refs == 1, "got %ld\n", Test_Stream.refs);

    free(expect_buffer);

    /* TYMED_ISTREAM = NULL with pUnkForRelease = NULL */

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    expect_size = 3 * sizeof(DWORD);

    med.tymed = TYMED_ISTREAM;
    med.pstm = NULL;
    med.pUnkForRelease = NULL;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    size = STGMEDIUM_UserSize(&umcb.Flags, 0, &med);
    ok(size == expect_size, "size %ld should be %ld bytes\n", size, expect_size);

    buffer = malloc(size);
    memset(buffer, 0xcc, size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    buffer_end = STGMEDIUM_UserMarshal(&umcb.Flags, buffer, &med);
    ok(buffer_end - buffer == expect_size, "buffer size mismatch\n");
    ok(*(DWORD*)buffer == TYMED_ISTREAM, "got %08lx\n", *(DWORD*)buffer);
    ok(*((DWORD*)buffer+1) == 0, "got %08lx\n", *((DWORD*)buffer+1));
    ok(*((DWORD*)buffer+2) == 0, "got %08lx\n", *((DWORD*)buffer+2));

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_DIFFERENTMACHINE);
    umcb.pStubMsg->IsClient = client;
    umcb.pStubMsg->fIsIn = in;
    umcb.pStubMsg->fIsOut = out;

    Test_Stream2.refs = 1;
    Test_Unknown2.refs = 1;
    med2.tymed = TYMED_ISTREAM;
    med2.pstm = &Test_Stream2.IStream_iface;
    med2.pUnkForRelease = &Test_Unknown2.IUnknown_iface;

    STGMEDIUM_UserUnmarshal(&umcb.Flags, buffer, &med2);

    ok(med2.tymed == TYMED_ISTREAM, "got tymed %lx\n", med2.tymed);
    ok(med2.pstm == NULL, "Incorrectly unmarshalled\n");
    ok(med2.pUnkForRelease == &Test_Unknown2.IUnknown_iface, "Incorrectly unmarshalled\n");
    ok(Test_Stream2.refs == 0, "got %ld\n", Test_Stream2.refs);
    ok(Test_Unknown2.refs == 1, "got %ld\n", Test_Unknown2.refs);

    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_DIFFERENTMACHINE);
    STGMEDIUM_UserFree(&umcb.Flags, &med2);
}

static void test_marshal_STGMEDIUM(void)
{
    marshal_STGMEDIUM(0, 0, 0);
    marshal_STGMEDIUM(0, 0, 1);
    marshal_STGMEDIUM(0, 1, 0);
    marshal_STGMEDIUM(0, 1, 1);
    /* For Windows versions post 2003, client side, non-[in,out] STGMEDIUMs get zero-initialised.
       However since inline stubs don't set fIsIn or fIsOut this behaviour would break
       ref counting in GetDataHere_Proxy for example, as we'd end up not releasing the original
       interface.  For simplicity we don't test or implement this. */
    marshal_STGMEDIUM(1, 1, 1);
}

static void test_marshal_SNB(void)
{
    static const WCHAR str1W[] = {'s','t','r','i','n','g','1',0};
    static const WCHAR str2W[] = {'s','t','r','2',0};
    unsigned char *buffer, *src, *mbuf;
    MIDL_STUB_MESSAGE stub_msg;
    WCHAR **ptrW, *dataW;
    USER_MARSHAL_CB umcb;
    RPC_MESSAGE rpc_msg;
    RemSNB *wiresnb;
    SNB snb, snb2;
    ULONG size;

    /* 4 bytes alignment */
    snb = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = SNB_UserSize(&umcb.Flags, 3, &snb);
    ok(size == 16, "Size should be 16, instead of %ld\n", size);

    /* NULL block */
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = SNB_UserSize(&umcb.Flags, 0, &snb);
    ok(size == 12, "Size should be 12, instead of %ld\n", size);

    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    mbuf = SNB_UserMarshal(&umcb.Flags, buffer, &snb);
    ok(mbuf == buffer + size, "got %p, %p\n", mbuf, buffer + size);

    wiresnb = (RemSNB*)buffer;
    ok(wiresnb->ulCntStr == 0, "got %lu\n", wiresnb->ulCntStr);
    ok(wiresnb->ulCntChar == 0, "got %lu\n", wiresnb->ulCntChar);
    ok(*(ULONG*)wiresnb->rgString == 0, "got %lu\n", *(ULONG*)wiresnb->rgString);

    snb2 = NULL;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    SNB_UserUnmarshal(&umcb.Flags, buffer, &snb2);
    ok(snb2 == NULL, "got %p\n", snb2);

    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    SNB_UserFree(&umcb.Flags, &snb2);

    /* block with actual data */

    /* allocate source block, n+1 pointers first, then data */
    src = malloc(sizeof(WCHAR*) * 3 + sizeof(str1W) + sizeof(str2W));
    ptrW = (WCHAR**)src;
    dataW = *ptrW = (WCHAR*)(src + 3*sizeof(WCHAR*));
    ptrW++;
    *ptrW = (WCHAR*)(src + 3*sizeof(WCHAR*) + sizeof(str1W));
    ptrW++;
    *ptrW = NULL;
    lstrcpyW(dataW, str1W);
    dataW += lstrlenW(str1W) + 1;
    lstrcpyW(dataW, str2W);

    snb = (SNB)src;
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = SNB_UserSize(&umcb.Flags, 0, &snb);
    ok(size == 38, "Size should be 38, instead of %ld\n", size);

    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    SNB_UserMarshal(&umcb.Flags, buffer, &snb);

    wiresnb = (RemSNB*)buffer;
    ok(wiresnb->ulCntStr == 13, "got %lu\n", wiresnb->ulCntStr);
    ok(wiresnb->ulCntChar == 2, "got %lu\n", wiresnb->ulCntChar);
    /* payload length is stored one more time, as ULONG */
    ok(*(ULONG*)wiresnb->rgString == wiresnb->ulCntStr, "got %lu\n", *(ULONG*)wiresnb->rgString);
    dataW = &wiresnb->rgString[2];
    ok(!lstrcmpW(dataW, str1W), "marshalled string 0: %s\n", wine_dbgstr_w(dataW));
    dataW += ARRAY_SIZE(str1W);
    ok(!lstrcmpW(dataW, str2W), "marshalled string 1: %s\n", wine_dbgstr_w(dataW));

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);

    g_expect_user_alloc = TRUE;
    snb2 = NULL;
    SNB_UserUnmarshal(&umcb.Flags, buffer, &snb2);
    g_expect_user_alloc = FALSE;

    ptrW = snb2;
    ok(!lstrcmpW(*ptrW, str1W), "unmarshalled string 0: %s\n", wine_dbgstr_w(*ptrW));
    ptrW++;
    ok(!lstrcmpW(*ptrW, str2W), "unmarshalled string 1: %s\n", wine_dbgstr_w(*ptrW));
    ptrW++;
    ok(*ptrW == NULL, "expected terminating NULL ptr, got %p, start %p\n", *ptrW, snb2);

    free(buffer);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);

    g_expect_user_free = TRUE;
    SNB_UserFree(&umcb.Flags, &snb2);
    g_expect_user_free = FALSE;

    free(src);
}

static void test_marshal_HDC(void)
{
    MIDL_STUB_MESSAGE stub_msg;
    HDC hdc = GetDC(0), hdc2;
    USER_MARSHAL_CB umcb;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    wireHDC wirehdc;
    ULONG size;

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HDC_UserSize(&umcb.Flags, 1, &hdc);
    ok(size == 4 + sizeof(*wirehdc), "Wrong size %ld\n", size);

    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HDC_UserMarshal(&umcb.Flags, buffer + 1, &hdc);
    ok(buffer_end == buffer + 4 + sizeof(*wirehdc), "got %p buffer %p\n", buffer_end, buffer);
    wirehdc = (wireHDC)(buffer + 4);
    ok(wirehdc->fContext == WDT_INPROC_CALL, "Context should be WDT_INPROC_CALL instead of 0x%08lx\n", wirehdc->fContext);
    ok(wirehdc->u.hInproc == (LONG_PTR)hdc, "Marshaled value should be %p instead of %lx\n", hdc, wirehdc->u.hRemote);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HDC_UserUnmarshal(&umcb.Flags, buffer + 1, &hdc2);
    ok(buffer_end == buffer + 4 + sizeof(*wirehdc), "got %p buffer %p\n", buffer_end, buffer);
    ok(hdc == hdc2, "Didn't unmarshal properly\n");
    free(buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HDC_UserFree(&umcb.Flags, &hdc2);
    ReleaseDC(0, hdc);
}

static void test_marshal_HICON(void)
{
    static const BYTE bmp_bits[1024];
    MIDL_STUB_MESSAGE stub_msg;
    HICON hIcon, hIcon2;
    USER_MARSHAL_CB umcb;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    wireHICON wirehicon;
    ULONG size;

    hIcon = CreateIcon(0, 16, 16, 1, 1, bmp_bits, bmp_bits);
    ok(hIcon != 0, "CreateIcon failed\n");

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HICON_UserSize(&umcb.Flags, 1, &hIcon);
    ok(size == 4 + sizeof(*wirehicon), "Wrong size %ld\n", size);

    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HICON_UserMarshal(&umcb.Flags, buffer + 1, &hIcon);
    ok(buffer_end == buffer + 4 + sizeof(*wirehicon), "got %p buffer %p\n", buffer_end, buffer);
    wirehicon = (wireHICON)(buffer + 4);
    ok(wirehicon->fContext == WDT_INPROC_CALL, "Context should be WDT_INPROC_CALL instead of 0x%08lx\n", wirehicon->fContext);
    ok(wirehicon->u.hInproc == (LONG_PTR)hIcon, "Marshaled value should be %p instead of %lx\n", hIcon, wirehicon->u.hRemote);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HICON_UserUnmarshal(&umcb.Flags, buffer + 1, &hIcon2);
    ok(buffer_end == buffer + 4 + sizeof(*wirehicon), "got %p buffer %p\n", buffer_end, buffer);
    ok(hIcon == hIcon2, "Didn't unmarshal properly\n");
    free(buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HICON_UserFree(&umcb.Flags, &hIcon2);
    DestroyIcon(hIcon);
}

static void test_marshal_HBRUSH(void)
{
    MIDL_STUB_MESSAGE stub_msg;
    HBRUSH hBrush, hBrush2;
    USER_MARSHAL_CB umcb;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    LOGBRUSH logbrush;
    wireHBRUSH wirehbrush;
    ULONG size;

    logbrush.lbStyle = BS_SOLID;
    logbrush.lbColor = RGB(0, 0, 0);
    logbrush.lbHatch = 0;

    hBrush = CreateBrushIndirect(&logbrush);
    ok(hBrush != 0, "CreateBrushIndirect failed\n");

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HBRUSH_UserSize(&umcb.Flags, 1, &hBrush);
    ok(size == 4 + sizeof(*wirehbrush), "Wrong size %ld\n", size);

    buffer = malloc(size);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HBRUSH_UserMarshal(&umcb.Flags, buffer + 1, &hBrush);
    ok(buffer_end == buffer + 4 + sizeof(*wirehbrush), "got %p buffer %p\n", buffer_end, buffer);
    wirehbrush = (wireHBRUSH)(buffer + 4);
    ok(wirehbrush->fContext == WDT_INPROC_CALL, "Context should be WDT_INPROC_CALL instead of 0x%08lx\n", wirehbrush->fContext);
    ok(wirehbrush->u.hInproc == (LONG_PTR)hBrush, "Marshaled value should be %p instead of %lx\n", hBrush, wirehbrush->u.hRemote);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HBRUSH_UserUnmarshal(&umcb.Flags, buffer + 1, &hBrush2);
    ok(buffer_end == buffer + 4 + sizeof(*wirehbrush), "got %p buffer %p\n", buffer_end, buffer);
    ok(hBrush == hBrush2, "Didn't unmarshal properly\n");
    free(buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HBRUSH_UserFree(&umcb.Flags, &hBrush2);
    DeleteObject(hBrush);
}

static void test_marshal_HBITMAP(void)
{
    static const ULONG header_size = FIELD_OFFSET(userBITMAP, cbSize);
    static BYTE bmp_bits[1024];
    MIDL_STUB_MESSAGE stub_msg;
    HBITMAP hBitmap, hBitmap2;
    USER_MARSHAL_CB umcb;
    RPC_MESSAGE rpc_msg;
    unsigned char *buffer, *buffer_end;
    unsigned char bitmap[1024];
    ULONG size, bitmap_size;

    hBitmap = CreateBitmap(16, 16, 1, 1, bmp_bits);
    ok(hBitmap != 0, "CreateBitmap failed\n");
    size = GetObjectA(hBitmap, sizeof(bitmap), bitmap);
    ok(size != 0, "GetObject failed\n");
    bitmap_size = GetBitmapBits(hBitmap, 0, NULL);
    ok(bitmap_size != 0, "GetBitmapBits failed\n");

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_INPROC);
    size = HBITMAP_UserSize(&umcb.Flags, 1, &hBitmap);
    ok(size == 0xc, "Wrong size %ld\n", size);
    buffer = malloc(size + 4);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_INPROC);
    buffer_end = HBITMAP_UserMarshal(&umcb.Flags, buffer + 1, &hBitmap);
    ok(buffer_end == buffer + 0xc, "HBITMAP_UserMarshal() returned wrong size %ld\n", (LONG)(buffer_end - buffer));
    ok(*(ULONG *)(buffer + 0x4) == WDT_INPROC_CALL, "Context should be WDT_INPROC_CALL instead of 0x%08lx\n", *(ULONG *)(buffer + 0x4));
    ok(*(ULONG *)(buffer + 0x8) == (ULONG)(ULONG_PTR)hBitmap, "wirestgm + 0x4 should be bitmap handle instead of 0x%08lx\n", *(ULONG *)(buffer + 0x8));

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_INPROC);
    HBITMAP_UserUnmarshal(&umcb.Flags, buffer + 1, &hBitmap2);
    ok(hBitmap2 != NULL, "Didn't unmarshal properly\n");
    free(buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_INPROC);
    HBITMAP_UserFree(&umcb.Flags, &hBitmap2);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    size = HBITMAP_UserSize(&umcb.Flags, 1, &hBitmap);
    ok(size == 0x10 + header_size + bitmap_size ||
       broken(size == 0x14 + header_size + bitmap_size), /* Windows adds 4 extra (unused) bytes */
       "Wrong size %ld\n", size);

    buffer = malloc(size + 4);
    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    buffer_end = HBITMAP_UserMarshal(&umcb.Flags, buffer + 1, &hBitmap);
    ok(buffer_end == buffer + 0x10 + header_size + bitmap_size, "HBITMAP_UserMarshal() returned wrong size %ld\n", (LONG)(buffer_end - buffer));
    ok(*(ULONG *)(buffer + 0x4) == WDT_REMOTE_CALL, "Context should be WDT_REMOTE_CALL instead of 0x%08lx\n", *(ULONG *)buffer);
    ok(*(ULONG *)(buffer + 0x8) == (ULONG)(ULONG_PTR)hBitmap, "wirestgm + 0x4 should be bitmap handle instead of 0x%08lx\n", *(ULONG *)(buffer + 0x4));
    ok(*(ULONG *)(buffer + 0xc) == (ULONG)(ULONG_PTR)bitmap_size, "wirestgm + 0x8 should be bitmap size instead of 0x%08lx\n", *(ULONG *)(buffer + 0x4));
    ok(!memcmp(buffer + 0x10, bitmap, header_size), "buffer mismatch\n");
    ok(!memcmp(buffer + 0x10 + header_size, bmp_bits, bitmap_size), "buffer mismatch\n");

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, buffer, size, MSHCTX_LOCAL);
    HBITMAP_UserUnmarshal(&umcb.Flags, buffer + 1, &hBitmap2);
    ok(hBitmap2 != NULL, "Didn't unmarshal properly\n");
    free(buffer);

    init_user_marshal_cb(&umcb, &stub_msg, &rpc_msg, NULL, 0, MSHCTX_LOCAL);
    HBITMAP_UserFree(&umcb.Flags, &hBitmap2);
    DeleteObject(hBitmap);
}

struct obj
{
    IDataObject IDataObject_iface;
};

static HRESULT WINAPI obj_QueryInterface(IDataObject *iface, REFIID iid, void **obj)
{
    *obj = NULL;

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IDataObject))
        *obj = iface;

    if (*obj)
    {
        IDataObject_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI obj_AddRef(IDataObject *iface)
{
    return 2;
}

static ULONG WINAPI obj_Release(IDataObject *iface)
{
    return 1;
}

static HRESULT WINAPI obj_DO_GetDataHere(IDataObject *iface, FORMATETC *fmt,
                                         STGMEDIUM *med)
{
    ok( med->pUnkForRelease == NULL, "got %p\n", med->pUnkForRelease );

    if (fmt->cfFormat == 2)
    {
        IStream_Release(med->pstm);
        med->pstm = &Test_Stream2.IStream_iface;
    }

    return S_OK;
}

static const IDataObjectVtbl obj_data_object_vtbl =
{
    obj_QueryInterface,
    obj_AddRef,
    obj_Release,
    NULL, /* GetData */
    obj_DO_GetDataHere,
    NULL, /* QueryGetData */
    NULL, /* GetCanonicalFormatEtc */
    NULL, /* SetData */
    NULL, /* EnumFormatEtc */
    NULL, /* DAdvise */
    NULL, /* DUnadvise */
    NULL  /* EnumDAdvise */
};

static struct obj obj =
{
    {&obj_data_object_vtbl}
};

static void test_GetDataHere_Proxy(void)
{
    HRESULT hr;
    IStream *stm;
    HANDLE thread;
    DWORD tid;
    static const LARGE_INTEGER zero;
    IDataObject *data;
    FORMATETC fmt;
    STGMEDIUM med;

    hr = CreateStreamOnHGlobal( NULL, TRUE, &stm );
    ok( hr == S_OK, "got %08lx\n", hr );
    tid = start_host_object2( stm, &IID_IDataObject, (IUnknown *)&obj.IDataObject_iface, MSHLFLAGS_NORMAL, NULL, &thread );

    IStream_Seek( stm, zero, STREAM_SEEK_SET, NULL );
    hr = CoUnmarshalInterface( stm, &IID_IDataObject, (void **)&data );
    ok( hr == S_OK, "got %08lx\n", hr );
    IStream_Release( stm );

    Test_Stream.refs = 1;
    Test_Stream2.refs = 1;
    Test_Unknown.refs = 1;

    fmt.cfFormat = 1;
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    med.pstm = NULL;
    med.pUnkForRelease = &Test_Unknown.IUnknown_iface;

    fmt.tymed = med.tymed = TYMED_NULL;
    hr = IDataObject_GetDataHere( data, &fmt, &med );
    ok( hr == DV_E_TYMED, "got %08lx\n", hr );

    for (fmt.tymed = TYMED_HGLOBAL; fmt.tymed <= TYMED_ENHMF; fmt.tymed <<= 1)
    {
        med.tymed = fmt.tymed;
        hr = IDataObject_GetDataHere( data, &fmt, &med );
        ok( hr == (fmt.tymed <= TYMED_ISTORAGE ? S_OK : DV_E_TYMED), "got %08lx for tymed %ld\n", hr, fmt.tymed );
        ok( Test_Unknown.refs == 1, "got %ld\n", Test_Unknown.refs );
    }

    fmt.tymed = TYMED_ISTREAM;
    med.tymed = TYMED_ISTORAGE;
    hr = IDataObject_GetDataHere( data, &fmt, &med );
    ok( hr == DV_E_TYMED, "got %08lx\n", hr );

    fmt.tymed = med.tymed = TYMED_ISTREAM;
    med.pstm = &Test_Stream.IStream_iface;
    med.pUnkForRelease = &Test_Unknown.IUnknown_iface;

    hr = IDataObject_GetDataHere( data, &fmt, &med );
    ok( hr == S_OK, "got %08lx\n", hr );

    ok( med.pstm == &Test_Stream.IStream_iface, "stm changed\n" );
    ok( med.pUnkForRelease == &Test_Unknown.IUnknown_iface, "punk changed\n" );

    ok( Test_Stream.refs == 1, "got %ld\n", Test_Stream.refs );
    ok( Test_Unknown.refs == 1, "got %ld\n", Test_Unknown.refs );

    fmt.cfFormat = 2;
    fmt.tymed = med.tymed = TYMED_ISTREAM;
    med.pstm = &Test_Stream.IStream_iface;
    med.pUnkForRelease = &Test_Unknown.IUnknown_iface;

    hr = IDataObject_GetDataHere( data, &fmt, &med );
    ok( hr == S_OK, "got %08lx\n", hr );

    ok( med.pstm == &Test_Stream.IStream_iface, "stm changed\n" );
    ok( med.pUnkForRelease == &Test_Unknown.IUnknown_iface, "punk changed\n" );

    ok( Test_Stream.refs == 1, "got %ld\n", Test_Stream.refs );
    ok( Test_Unknown.refs == 1, "got %ld\n", Test_Unknown.refs );
    ok( Test_Stream2.refs == 0, "got %ld\n", Test_Stream2.refs );

    IDataObject_Release( data );
    end_host_object( tid, thread );
}

START_TEST(usrmarshal)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_marshal_CLIPFORMAT();
    test_marshal_HWND();
    test_marshal_HGLOBAL();
    test_marshal_HENHMETAFILE();
    test_marshal_HMETAFILE();
    test_marshal_HMETAFILEPICT();
    test_marshal_WdtpInterfacePointer();
    test_marshal_STGMEDIUM();
    test_marshal_SNB();
    test_marshal_HDC();
    test_marshal_HICON();
    test_marshal_HBRUSH();
    test_marshal_HBITMAP();

    test_GetDataHere_Proxy();

    CoUninitialize();
}
