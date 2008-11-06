/*
 * Unit test suite for cstubs
 *
 * Copyright 2006 Huw Davies
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

#include <stdarg.h>

#define PROXY_DELEGATION
#define COBJMACROS

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>


#include "initguid.h"
#include "rpc.h"
#include "rpcdce.h"
#include "rpcproxy.h"

static CStdPSFactoryBuffer PSFactoryBuffer;

CSTDSTUBBUFFERRELEASE(&PSFactoryBuffer)
CSTDSTUBBUFFER2RELEASE(&PSFactoryBuffer)

static GUID IID_if1 = {0x12345678, 1234, 5678, {12,34,56,78,90,0xab,0xcd,0xef}};
static GUID IID_if2 = {0x12345679, 1234, 5678, {12,34,56,78,90,0xab,0xcd,0xef}};
static GUID IID_if3 = {0x1234567a, 1234, 5678, {12,34,56,78,90,0xab,0xcd,0xef}};
static GUID IID_if4 = {0x1234567b, 1234, 5678, {12,34,56,78,90,0xab,0xcd,0xef}};

static int my_alloc_called;
static int my_free_called;

static void * CALLBACK my_alloc(size_t size)
{
    my_alloc_called++;
    return NdrOleAllocate(size);
}

static void CALLBACK my_free(void *ptr)
{
    my_free_called++;
    NdrOleFree(ptr);
}

typedef struct _MIDL_PROC_FORMAT_STRING
{
    short          Pad;
    unsigned char  Format[ 2 ];
} MIDL_PROC_FORMAT_STRING;

typedef struct _MIDL_TYPE_FORMAT_STRING
{
    short          Pad;
    unsigned char  Format[ 2 ];
} MIDL_TYPE_FORMAT_STRING;


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
{
    0,
    {
        0, 0
    }
};

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
{
    0,
    {
        0, 0
    }
};

static const MIDL_STUB_DESC Object_StubDesc =
    {
    NULL,
    my_alloc,
    my_free,
    { 0 },
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x20000, /* Ndr library version */
    0,
    0x50100a4, /* MIDL Version 5.1.164 */
    0,
    NULL,
    0,  /* notify & notify_flag routine table */
    1,  /* Flags */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

static HRESULT WINAPI if1_fn1_Proxy(void *This)
{
    return S_OK;
}

static void __RPC_STUB if1_fn1_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    trace("fn1 stub\n");
}

static HRESULT WINAPI if1_fn2_Proxy(void *This)
{
    return S_OK;
}

static void __RPC_STUB if1_fn2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase)
{
    trace("fn2 stub\n");
}

static CINTERFACE_PROXY_VTABLE(5) if1_proxy_vtbl =
{
    { &IID_if1 },
    {   IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        if1_fn1_Proxy,
        if1_fn2_Proxy
    }
};


static const unsigned short if1_FormatStringOffsetTable[] =
    {
    0,
    0
    };

static const MIDL_SERVER_INFO if1_server_info =
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &if1_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};


static const PRPC_STUB_FUNCTION if1_table[] =
{
    if1_fn1_Stub,
    if1_fn2_Stub
};

static CInterfaceStubVtbl if1_stub_vtbl =
{
    {
        &IID_if1,
        &if1_server_info,
        5,
        &if1_table[-3]
    },
    { CStdStubBuffer_METHODS }
};

static CINTERFACE_PROXY_VTABLE(13) if2_proxy_vtbl =
{
    { &IID_if2 },
    {   IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }
};

static const unsigned short if2_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_SERVER_INFO if2_server_info =
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &if2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};


static const PRPC_STUB_FUNCTION if2_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

static CInterfaceStubVtbl if2_stub_vtbl =
{
    {
        &IID_if2,
        &if2_server_info,
        13,
        &if2_table[-3]
    },
    { CStdStubBuffer_DELEGATING_METHODS }
};

static CINTERFACE_PROXY_VTABLE(4) if3_proxy_vtbl =
{
    { &IID_if3 },
    {   IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        if1_fn1_Proxy
    }
};


static const unsigned short if3_FormatStringOffsetTable[] =
    {
    0,
    0
    };

static const MIDL_SERVER_INFO if3_server_info =
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &if3_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};


static const PRPC_STUB_FUNCTION if3_table[] =
{
    if1_fn1_Stub
};

static CInterfaceStubVtbl if3_stub_vtbl =
{
    {
        &IID_if3,
        &if3_server_info,
        4,
        &if1_table[-3]
    },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static CINTERFACE_PROXY_VTABLE(7) if4_proxy_vtbl =
{
    { &IID_if4 },
    {   IUnknown_QueryInterface_Proxy,
        IUnknown_AddRef_Proxy,
        IUnknown_Release_Proxy ,
        0,
        0,
        0,
        0
    }
};

static const unsigned short if4_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_SERVER_INFO if4_server_info =
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &if4_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};


static const PRPC_STUB_FUNCTION if4_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
};

static CInterfaceStubVtbl if4_stub_vtbl =
{
    {
        &IID_if4,
        &if4_server_info,
        7,
        &if2_table[-3]
    },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static const CInterfaceProxyVtbl *cstub_ProxyVtblList[] =
{
    (const CInterfaceProxyVtbl *) &if1_proxy_vtbl,
    (const CInterfaceProxyVtbl *) &if2_proxy_vtbl,
    (const CInterfaceProxyVtbl *) &if3_proxy_vtbl,
    (const CInterfaceProxyVtbl *) &if4_proxy_vtbl,
    NULL
};

static const CInterfaceStubVtbl *cstub_StubVtblList[] =
{
    (const CInterfaceStubVtbl *) &if1_stub_vtbl,
    (const CInterfaceStubVtbl *) &if2_stub_vtbl,
    (const CInterfaceStubVtbl *) &if3_stub_vtbl,
    (const CInterfaceStubVtbl *) &if4_stub_vtbl,
    NULL
};

static PCInterfaceName const if_name_list[] =
{
    "if1",
    "if2",
    "if3",
    "if4",
    NULL
};

static const IID *base_iid_list[] =
{
    NULL,
    &IID_ITypeLib,
    NULL,
    &IID_IDispatch,
    NULL
};

#define cstub_CHECK_IID(n)     IID_GENERIC_CHECK_IID( cstub, pIID, n)

static int __stdcall iid_lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( cstub, 4, 4 )
    IID_BS_LOOKUP_NEXT_TEST( cstub, 2 )
    IID_BS_LOOKUP_NEXT_TEST( cstub, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( cstub, 4, *pIndex )

}


static const ExtendedProxyFileInfo my_proxy_file_info =
{
    (const PCInterfaceProxyVtblList *) &cstub_ProxyVtblList,
    (const PCInterfaceStubVtblList *) &cstub_StubVtblList,
    (const PCInterfaceName *) &if_name_list,
    (const IID **) &base_iid_list,
    &iid_lookup,
    4,
    1,
    NULL,
    0,
    0,
    0
};

static const ProxyFileInfo *proxy_file_list[] = {
    &my_proxy_file_info,
    NULL
};


static IPSFactoryBuffer *test_NdrDllGetClassObject(void)
{
    IPSFactoryBuffer *ppsf = NULL;
    const CLSID PSDispatch = {0x20420, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46}};
    const CLSID CLSID_Unknown = {0x45678, 0x1234, 0x6666, {0xff, 0x67, 0x45, 0x98, 0x76, 0x12, 0x34, 0x56}};
    HRESULT r;
    HMODULE hmod = LoadLibraryA("rpcrt4.dll");
    void *CStd_QueryInterface = GetProcAddress(hmod, "CStdStubBuffer_QueryInterface");
    void *CStd_AddRef = GetProcAddress(hmod, "CStdStubBuffer_AddRef");
    void *CStd_Release = GetProcAddress(hmod, "NdrCStdStubBuffer_Release");
    void *CStd_Connect = GetProcAddress(hmod, "CStdStubBuffer_Connect");
    void *CStd_Disconnect = GetProcAddress(hmod, "CStdStubBuffer_Disconnect");
    void *CStd_Invoke = GetProcAddress(hmod, "CStdStubBuffer_Invoke");
    void *CStd_IsIIDSupported = GetProcAddress(hmod, "CStdStubBuffer_IsIIDSupported");
    void *CStd_CountRefs = GetProcAddress(hmod, "CStdStubBuffer_CountRefs");
    void *CStd_DebugServerQueryInterface = GetProcAddress(hmod, "CStdStubBuffer_DebugServerQueryInterface");
    void *CStd_DebugServerRelease = GetProcAddress(hmod, "CStdStubBuffer_DebugServerRelease");

    r = NdrDllGetClassObject(&PSDispatch, &IID_IPSFactoryBuffer, (void**)&ppsf, proxy_file_list,
                             &CLSID_Unknown, &PSFactoryBuffer);
    ok(r == CLASS_E_CLASSNOTAVAILABLE, "NdrDllGetClassObject with unknown clsid should have returned CLASS_E_CLASSNOTAVAILABLE instead of 0x%x\n", r);
    ok(ppsf == NULL, "NdrDllGetClassObject should have set ppsf to NULL on failure\n");

    r = NdrDllGetClassObject(&PSDispatch, &IID_IPSFactoryBuffer, (void**)&ppsf, proxy_file_list,
                         &PSDispatch, &PSFactoryBuffer);

    ok(r == S_OK, "ret %08x\n", r);
    ok(ppsf != NULL, "ppsf == NULL\n");

    ok(PSFactoryBuffer.pProxyFileList == proxy_file_list, "pfl not the same\n");
    ok(PSFactoryBuffer.pProxyFileList[0]->pStubVtblList == (PCInterfaceStubVtblList *) &cstub_StubVtblList, "stub vtbllist not the same\n");

    /* if1 is non-delegating, if2 is delegating, if3 is non-delegating
       but I've zero'ed the vtbl entries, similarly if4 is delegating
       with zero'ed vtbl entries */

#define VTBL_TEST_NOT_CHANGE_TO(name, i)                                  \
    ok(PSFactoryBuffer.pProxyFileList[0]->pStubVtblList[i]->Vtbl.name != CStd_##name, #name "vtbl %d updated %p %p\n", \
       i, PSFactoryBuffer.pProxyFileList[0]->pStubVtblList[i]->Vtbl.name, CStd_##name );
#define VTBL_TEST_CHANGE_TO(name, i)                                  \
    ok(PSFactoryBuffer.pProxyFileList[0]->pStubVtblList[i]->Vtbl.name == CStd_##name, #name "vtbl %d not updated %p %p\n", \
       i, PSFactoryBuffer.pProxyFileList[0]->pStubVtblList[i]->Vtbl.name, CStd_##name );
#define VTBL_TEST_ZERO(name, i)                                  \
    ok(PSFactoryBuffer.pProxyFileList[0]->pStubVtblList[i]->Vtbl.name == NULL, #name "vtbl %d not null %p\n", \
       i, PSFactoryBuffer.pProxyFileList[0]->pStubVtblList[i]->Vtbl.name );
    VTBL_TEST_NOT_CHANGE_TO(QueryInterface, 0);
    VTBL_TEST_NOT_CHANGE_TO(AddRef, 0);
    VTBL_TEST_NOT_CHANGE_TO(Release, 0);
    VTBL_TEST_NOT_CHANGE_TO(Connect, 0);
    VTBL_TEST_NOT_CHANGE_TO(Disconnect, 0);
    VTBL_TEST_NOT_CHANGE_TO(Invoke, 0);
    VTBL_TEST_NOT_CHANGE_TO(IsIIDSupported, 0);
    VTBL_TEST_NOT_CHANGE_TO(CountRefs, 0);
    VTBL_TEST_NOT_CHANGE_TO(DebugServerQueryInterface, 0);
    VTBL_TEST_NOT_CHANGE_TO(DebugServerRelease, 0);

    VTBL_TEST_CHANGE_TO(QueryInterface, 1);
    VTBL_TEST_CHANGE_TO(AddRef, 1);
    VTBL_TEST_NOT_CHANGE_TO(Release, 1);
    VTBL_TEST_NOT_CHANGE_TO(Connect, 1);
    VTBL_TEST_NOT_CHANGE_TO(Disconnect, 1);
    VTBL_TEST_CHANGE_TO(Invoke, 1);
    VTBL_TEST_CHANGE_TO(IsIIDSupported, 1);
    VTBL_TEST_NOT_CHANGE_TO(CountRefs, 1);
    VTBL_TEST_CHANGE_TO(DebugServerQueryInterface, 1);
    VTBL_TEST_CHANGE_TO(DebugServerRelease, 1);

    VTBL_TEST_CHANGE_TO(QueryInterface, 2);
    VTBL_TEST_CHANGE_TO(AddRef, 2);
    VTBL_TEST_ZERO(Release, 2);
    VTBL_TEST_CHANGE_TO(Connect, 2);
    VTBL_TEST_CHANGE_TO(Disconnect, 2);
    VTBL_TEST_CHANGE_TO(Invoke, 2);
    VTBL_TEST_CHANGE_TO(IsIIDSupported, 2);
    VTBL_TEST_CHANGE_TO(CountRefs, 2);
    VTBL_TEST_CHANGE_TO(DebugServerQueryInterface, 2);
    VTBL_TEST_CHANGE_TO(DebugServerRelease, 2);

    VTBL_TEST_CHANGE_TO(QueryInterface, 3);
    VTBL_TEST_CHANGE_TO(AddRef, 3);
    VTBL_TEST_ZERO(Release, 3);
    VTBL_TEST_NOT_CHANGE_TO(Connect, 3);
    VTBL_TEST_NOT_CHANGE_TO(Disconnect, 3);
    VTBL_TEST_CHANGE_TO(Invoke, 3);
    VTBL_TEST_CHANGE_TO(IsIIDSupported, 3);
    VTBL_TEST_NOT_CHANGE_TO(CountRefs, 3);
    VTBL_TEST_CHANGE_TO(DebugServerQueryInterface, 3);
    VTBL_TEST_CHANGE_TO(DebugServerRelease, 3);



#undef VTBL_TEST_NOT_CHANGE_TO
#undef VTBL_TEST_CHANGE_TO
#undef VTBL_TEST_ZERO

    ok(PSFactoryBuffer.RefCount == 1, "ref count %d\n", PSFactoryBuffer.RefCount);
    IPSFactoryBuffer_Release(ppsf);

    r = NdrDllGetClassObject(&IID_if3, &IID_IPSFactoryBuffer, (void**)&ppsf, proxy_file_list,
                             NULL, &PSFactoryBuffer);
    ok(r == S_OK, "ret %08x\n", r);
    ok(ppsf != NULL, "ppsf == NULL\n");

    return ppsf;
}

static int base_buffer_invoke_called;
static HRESULT WINAPI base_buffer_Invoke(IRpcStubBuffer *This, RPCOLEMESSAGE *msg, IRpcChannelBuffer *channel)
{
    base_buffer_invoke_called++;
    ok(msg == (RPCOLEMESSAGE*)0xcafebabe, "msg ptr changed\n");
    ok(channel == (IRpcChannelBuffer*)0xdeadbeef, "channel ptr changed\n");
    return S_OK; /* returning any failure here results in an exception */
}

static IRpcStubBufferVtbl base_buffer_vtbl = {
    (void*)0xcafebab0,
    (void*)0xcafebab1,
    (void*)0xcafebab2,
    (void*)0xcafebab3,
    (void*)0xcafebab4,
    base_buffer_Invoke,
    (void*)0xcafebab6,
    (void*)0xcafebab7,
    (void*)0xcafebab8,
    (void*)0xcafebab9
};

static void test_NdrStubForwardingFunction(void)
{
    void *This[5];
    void *real_this;
    IRpcChannelBuffer *channel = (IRpcChannelBuffer*)0xdeadbeef;
    RPC_MESSAGE *msg = (RPC_MESSAGE*)0xcafebabe;
    DWORD *phase = (DWORD*)0x12345678;
    IRpcStubBufferVtbl *base_buffer_vtbl_ptr = &base_buffer_vtbl;
    IRpcStubBuffer *base_stub_buffer = (IRpcStubBuffer*)&base_buffer_vtbl_ptr;

    memset(This, 0xcc, sizeof(This));
    This[0] = base_stub_buffer;
    real_this = &This[1];

    NdrStubForwardingFunction( real_this, channel, msg, phase );
    ok(base_buffer_invoke_called == 1, "base_buffer_invoke called %d times\n", base_buffer_invoke_called);

}

static IRpcStubBuffer *create_stub(IPSFactoryBuffer *ppsf, REFIID iid, IUnknown *obj, HRESULT expected_result)
{
    IRpcStubBuffer *pstub = NULL;
    HRESULT r;

    r = IPSFactoryBuffer_CreateStub(ppsf, iid, obj, &pstub);
    ok(r == expected_result, "CreateStub returned %08x expected %08x\n", r, expected_result);
    return pstub;
}

static HRESULT WINAPI create_stub_test_QI(IUnknown *This, REFIID iid, void **ppv)
{
    ok(IsEqualIID(iid, &IID_if1), "incorrect iid\n");
    *ppv = (void*)0xdeadbeef;
    return S_OK;
}

static IUnknownVtbl create_stub_test_vtbl =
{
    create_stub_test_QI,
    NULL,
    NULL
};

static HRESULT WINAPI create_stub_test_fail_QI(IUnknown *This, REFIID iid, void **ppv)
{
    ok(IsEqualIID(iid, &IID_if1), "incorrect iid\n");
    *ppv = NULL;
    return E_NOINTERFACE;
}

static IUnknownVtbl create_stub_test_fail_vtbl =
{
    create_stub_test_fail_QI,
    NULL,
    NULL
};

static void test_CreateStub(IPSFactoryBuffer *ppsf)
{
    IUnknownVtbl *vtbl = &create_stub_test_vtbl;
    IUnknown *obj = (IUnknown*)&vtbl;
    IRpcStubBuffer *pstub = create_stub(ppsf, &IID_if1, obj, S_OK);
    CStdStubBuffer *cstd_stub = (CStdStubBuffer*)pstub;
    const CInterfaceStubHeader *header = ((const CInterfaceStubHeader *)cstd_stub->lpVtbl) - 1;

    ok(IsEqualIID(header->piid, &IID_if1), "header iid differs\n");
    ok(cstd_stub->RefCount == 1, "ref count %d\n", cstd_stub->RefCount);
    /* 0xdeadbeef returned from create_stub_test_QI */
    ok(cstd_stub->pvServerObject == (void*)0xdeadbeef, "pvServerObject %p\n", cstd_stub->pvServerObject);
    ok(cstd_stub->pPSFactory == ppsf, "pPSFactory %p\n", cstd_stub->pPSFactory);

    vtbl = &create_stub_test_fail_vtbl;
    pstub = create_stub(ppsf, &IID_if1, obj, E_NOINTERFACE);

}

static HRESULT WINAPI connect_test_orig_QI(IUnknown *This, REFIID iid, void **ppv)
{
    ok(IsEqualIID(iid, &IID_if1) ||
       IsEqualIID(iid, &IID_if2), "incorrect iid\n");
    *ppv = (void*)This;
    return S_OK;
}

static int connect_test_orig_release_called;
static ULONG WINAPI connect_test_orig_release(IUnknown *This)
{
    connect_test_orig_release_called++;
    return 0;
}

static IUnknownVtbl connect_test_orig_vtbl =
{
    connect_test_orig_QI,
    NULL,
    connect_test_orig_release
};

static HRESULT WINAPI connect_test_new_QI(IUnknown *This, REFIID iid, void **ppv)
{
    ok(IsEqualIID(iid, &IID_if1) ||
       IsEqualIID(iid, &IID_if2), "incorrect iid\n");
    *ppv = (void*)0xcafebabe;
    return S_OK;
}

static IUnknownVtbl connect_test_new_vtbl =
{
    connect_test_new_QI,
    NULL,
    NULL
};

static HRESULT WINAPI connect_test_new_fail_QI(IUnknown *This, REFIID iid, void **ppv)
{
    ok(IsEqualIID(iid, &IID_if1), "incorrect iid\n");
    *ppv = (void*)0xdeadbeef;
    return E_NOINTERFACE;
}

static IUnknownVtbl connect_test_new_fail_vtbl =
{
    connect_test_new_fail_QI,
    NULL,
    NULL
};

static int connect_test_base_Connect_called;
static HRESULT WINAPI connect_test_base_Connect(IRpcStubBuffer *pstub, IUnknown *obj)
{
    connect_test_base_Connect_called++;
    ok(*(void**)obj == (void*)0xbeefcafe, "unexpected obj %p\n", obj);
    return S_OK;
}

static IRpcStubBufferVtbl connect_test_base_stub_buffer_vtbl =
{
    (void*)0xcafebab0,
    (void*)0xcafebab1,
    (void*)0xcafebab2,
    connect_test_base_Connect,
    (void*)0xcafebab4,
    (void*)0xcafebab5,
    (void*)0xcafebab6,
    (void*)0xcafebab7,
    (void*)0xcafebab8,
    (void*)0xcafebab9
};

static void test_Connect(IPSFactoryBuffer *ppsf)
{
    IUnknownVtbl *orig_vtbl = &connect_test_orig_vtbl;
    IUnknownVtbl *new_vtbl = &connect_test_new_vtbl;
    IUnknownVtbl *new_fail_vtbl = &connect_test_new_fail_vtbl;
    IUnknown *obj = (IUnknown*)&orig_vtbl;
    IRpcStubBuffer *pstub = create_stub(ppsf, &IID_if1, obj, S_OK); 
    CStdStubBuffer *cstd_stub = (CStdStubBuffer*)pstub;
    IRpcStubBufferVtbl *base_stub_buf_vtbl = &connect_test_base_stub_buffer_vtbl;
    HRESULT r;

    obj = (IUnknown*)&new_vtbl;
    r = IRpcStubBuffer_Connect(pstub, obj);
    ok(r == S_OK, "r %08x\n", r);
    ok(connect_test_orig_release_called == 1, "release called %d\n", connect_test_orig_release_called);
    ok(cstd_stub->pvServerObject == (void*)0xcafebabe, "pvServerObject %p\n", cstd_stub->pvServerObject);

    cstd_stub->pvServerObject = (IUnknown*)&orig_vtbl;
    obj = (IUnknown*)&new_fail_vtbl;
    r = IRpcStubBuffer_Connect(pstub, obj);
    ok(r == E_NOINTERFACE, "r %08x\n", r);
    ok(cstd_stub->pvServerObject == (void*)0xdeadbeef, "pvServerObject %p\n", cstd_stub->pvServerObject);
    ok(connect_test_orig_release_called == 2, "release called %d\n", connect_test_orig_release_called);    

    /* Now use a delegated stub.

       We know from the NdrStubForwardFunction test that
       (void**)pstub-1 is the base interface stub buffer.  This shows
       that (void**)pstub-2 contains the address of a vtable that gets
       passed to the base interface's Connect method.  Note that
       (void**)pstub-2 itself gets passed to Connect and not
       *((void**)pstub-2), so it should contain the vtable ptr and not
       an interface ptr. */

    obj = (IUnknown*)&orig_vtbl;
    pstub = create_stub(ppsf, &IID_if2, obj, S_OK);
    *((void**)pstub-1) = &base_stub_buf_vtbl;
    *((void**)pstub-2) = (void*)0xbeefcafe;

    obj = (IUnknown*)&new_vtbl;
    r = IRpcStubBuffer_Connect(pstub, obj);
    ok(connect_test_base_Connect_called == 1, "connect_test_bsae_Connect called %d times\n",
       connect_test_base_Connect_called);
    ok(connect_test_orig_release_called == 3, "release called %d\n", connect_test_orig_release_called);
    cstd_stub = (CStdStubBuffer*)pstub;
    ok(cstd_stub->pvServerObject == (void*)0xcafebabe, "pvServerObject %p\n", cstd_stub->pvServerObject);
}

static void test_Disconnect(IPSFactoryBuffer *ppsf)
{
    IUnknownVtbl *orig_vtbl = &connect_test_orig_vtbl;
    IUnknown *obj = (IUnknown*)&orig_vtbl;
    IRpcStubBuffer *pstub = create_stub(ppsf, &IID_if1, obj, S_OK);
    CStdStubBuffer *cstd_stub = (CStdStubBuffer*)pstub;

    connect_test_orig_release_called = 0;
    IRpcStubBuffer_Disconnect(pstub);
    ok(connect_test_orig_release_called == 1, "release called %d\n", connect_test_orig_release_called);
    ok(cstd_stub->pvServerObject == NULL, "pvServerObject %p\n", cstd_stub->pvServerObject);
}


static int release_test_psfacbuf_release_called;
static ULONG WINAPI release_test_pretend_psfacbuf_release(IUnknown *pUnk)
{
    release_test_psfacbuf_release_called++;
    return 1;
}

static IUnknownVtbl release_test_pretend_psfacbuf_vtbl =
{
    NULL,
    NULL,
    release_test_pretend_psfacbuf_release
};

static void test_Release(IPSFactoryBuffer *ppsf)
{
    LONG facbuf_refs;
    IUnknownVtbl *orig_vtbl = &connect_test_orig_vtbl;
    IUnknown *obj = (IUnknown*)&orig_vtbl;
    IUnknownVtbl *pretend_psfacbuf_vtbl = &release_test_pretend_psfacbuf_vtbl;
    IUnknown *pretend_psfacbuf = (IUnknown *)&pretend_psfacbuf_vtbl;
    IRpcStubBuffer *pstub = create_stub(ppsf, &IID_if1, obj, S_OK);
    CStdStubBuffer *cstd_stub = (CStdStubBuffer*)pstub;

    facbuf_refs = PSFactoryBuffer.RefCount;

    /* This shows that NdrCStdStubBuffer_Release doesn't call Disconnect */
    ok(cstd_stub->RefCount == 1, "ref count %d\n", cstd_stub->RefCount);
    connect_test_orig_release_called = 0;
    IRpcStubBuffer_Release(pstub);
todo_wine {
    ok(connect_test_orig_release_called == 0, "release called %d\n", connect_test_orig_release_called);
}
    ok(PSFactoryBuffer.RefCount == facbuf_refs - 1, "factory buffer refs %d orig %d\n", PSFactoryBuffer.RefCount, facbuf_refs);

    /* This shows that NdrCStdStubBuffer_Release calls Release on its 2nd arg, rather than on This->pPSFactory
       (which are usually the same and indeed it's odd that _Release requires this 2nd arg). */
    pstub = create_stub(ppsf, &IID_if1, obj, S_OK);
    ok(PSFactoryBuffer.RefCount == facbuf_refs, "factory buffer refs %d orig %d\n", PSFactoryBuffer.RefCount, facbuf_refs);
    NdrCStdStubBuffer_Release(pstub, (IPSFactoryBuffer*)pretend_psfacbuf);
    ok(release_test_psfacbuf_release_called == 1, "pretend_psfacbuf_release called %d\n", release_test_psfacbuf_release_called);
    ok(PSFactoryBuffer.RefCount == facbuf_refs, "factory buffer refs %d orig %d\n", PSFactoryBuffer.RefCount, facbuf_refs);
}

static HRESULT WINAPI delegating_invoke_test_QI(ITypeLib *pUnk, REFIID iid, void** ppv)
{

    *ppv = pUnk;
    return S_OK;
}

static ULONG WINAPI delegating_invoke_test_addref(ITypeLib *pUnk)
{
    return 1;
}

static ULONG WINAPI delegating_invoke_test_release(ITypeLib *pUnk)
{
    return 1;
}

static UINT WINAPI delegating_invoke_test_get_type_info_count(ITypeLib *pUnk)
{
    return 0xabcdef;
}

static ITypeLibVtbl delegating_invoke_test_obj_vtbl =
{
    delegating_invoke_test_QI,
    delegating_invoke_test_addref,
    delegating_invoke_test_release,
    delegating_invoke_test_get_type_info_count,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static HRESULT WINAPI delegating_invoke_chan_query_interface(IRpcChannelBuffer *pchan,
                                                             REFIID iid,
                                                             void **ppv)
{
    ok(0, "call to QueryInterface not expected\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI delegating_invoke_chan_add_ref(IRpcChannelBuffer *pchan)
{
    return 2;
}

static ULONG WINAPI delegating_invoke_chan_release(IRpcChannelBuffer *pchan)
{
    return 1;
}

static HRESULT WINAPI delegating_invoke_chan_get_buffer(IRpcChannelBuffer *pchan,
                                                        RPCOLEMESSAGE *msg,
                                                        REFIID iid)
{
    msg->Buffer = HeapAlloc(GetProcessHeap(), 0, msg->cbBuffer);
    return S_OK;
}

static HRESULT WINAPI delegating_invoke_chan_send_receive(IRpcChannelBuffer *pchan,
                                                          RPCOLEMESSAGE *pMessage,
                                                          ULONG *pStatus)
{
    ok(0, "call to SendReceive not expected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI delegating_invoke_chan_free_buffer(IRpcChannelBuffer *pchan,
                                                         RPCOLEMESSAGE *pMessage)
{
    ok(0, "call to FreeBuffer not expected\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI delegating_invoke_chan_get_dest_ctx(IRpcChannelBuffer *pchan,
                                                          DWORD *pdwDestContext,
                                                          void **ppvDestContext)
{
    *pdwDestContext = MSHCTX_LOCAL;
    *ppvDestContext = NULL;
    return S_OK;
}

static HRESULT WINAPI delegating_invoke_chan_is_connected(IRpcChannelBuffer *pchan)
{
    ok(0, "call to IsConnected not expected\n");
    return E_NOTIMPL;
}

static IRpcChannelBufferVtbl delegating_invoke_test_rpc_chan_vtbl =
{
    delegating_invoke_chan_query_interface,
    delegating_invoke_chan_add_ref,
    delegating_invoke_chan_release,
    delegating_invoke_chan_get_buffer,
    delegating_invoke_chan_send_receive,
    delegating_invoke_chan_free_buffer,
    delegating_invoke_chan_get_dest_ctx,
    delegating_invoke_chan_is_connected
};

static void test_delegating_Invoke(IPSFactoryBuffer *ppsf)
{
    ITypeLibVtbl *obj_vtbl = &delegating_invoke_test_obj_vtbl;
    IUnknown *obj = (IUnknown*)&obj_vtbl;
    IRpcStubBuffer *pstub = create_stub(ppsf, &IID_if2, obj, S_OK);
    IRpcChannelBufferVtbl *pchan_vtbl = &delegating_invoke_test_rpc_chan_vtbl;
    IRpcChannelBuffer *pchan = (IRpcChannelBuffer *)&pchan_vtbl;
    HRESULT r = E_FAIL;
    RPCOLEMESSAGE msg;

    memset(&msg, 0, sizeof(msg));
    msg.dataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;
    msg.iMethod = 3;
    r = IRpcStubBuffer_Invoke(pstub, &msg, pchan);
    ok(r == S_OK, "ret %08x\n", r);
    if(r == S_OK)
    {
        ok(*(DWORD*)msg.Buffer == 0xabcdef, "buf[0] %08x\n", *(DWORD*)msg.Buffer);
        ok(*((DWORD*)msg.Buffer + 1) == S_OK, "buf[1] %08x\n", *((DWORD*)msg.Buffer + 1));
    }
    /* free the buffer allocated by delegating_invoke_chan_get_buffer */
    HeapFree(GetProcessHeap(), 0, msg.Buffer);
    IRpcStubBuffer_Release(pstub);
}

START_TEST( cstub )
{
    IPSFactoryBuffer *ppsf;

    OleInitialize(NULL);

    ppsf = test_NdrDllGetClassObject();
    test_NdrStubForwardingFunction();
    test_CreateStub(ppsf);
    test_Connect(ppsf);
    test_Disconnect(ppsf);
    test_Release(ppsf);
    test_delegating_Invoke(ppsf);

    OleUninitialize();
}
