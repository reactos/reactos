/*
 * Unit test suite for ndr marshalling functions
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

#define COBJMACROS

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>
#include <ole2.h>

#include "rpc.h"
#include "rpcdce.h"
#include "rpcproxy.h"
#include "midles.h"
#include "ndrtypes.h"

#include "wine/heap.h"
#include "wine/test.h"

static int my_alloc_called;
static int my_free_called;
static void * CALLBACK my_alloc(SIZE_T size)
{
    my_alloc_called++;
    return NdrOleAllocate(size);
}

static void CALLBACK my_free(void *ptr)
{
    my_free_called++;
    NdrOleFree(ptr);
}

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
    NULL, /* format string, filled in by tests */
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

static RPC_DISPATCH_FUNCTION IFoo_table[] =
{
    0
};

static RPC_DISPATCH_TABLE IFoo_v0_0_DispatchTable =
{
    0,
    IFoo_table
};

static const RPC_SERVER_INTERFACE IFoo___RpcServerInterface =
{
    sizeof(RPC_SERVER_INTERFACE),
    {{0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x12,0x34}},{0,0}},
    {{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},
    &IFoo_v0_0_DispatchTable,
    0,
    0,
    0,
    0,
    0,
};

static RPC_IF_HANDLE IFoo_v0_0_s_ifspec = (RPC_IF_HANDLE)& IFoo___RpcServerInterface;
static BOOL use_pointer_ids = FALSE;

static void determine_pointer_marshalling_style(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    char ch = 0xde;

    static const unsigned char fmtstr_up_char[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = NULL;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 8;
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    NdrPointerMarshall(&StubMsg, (unsigned char*)&ch, fmtstr_up_char);
    ok(StubMsg.Buffer == StubMsg.BufferStart + 5, "%p %p\n", StubMsg.Buffer, StubMsg.BufferStart);

    use_pointer_ids = (*(unsigned int *)StubMsg.BufferStart != (UINT_PTR)&ch);
    trace("Pointer marshalling using %s\n", use_pointer_ids ? "pointer ids" : "pointer value");

    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

static void test_ndr_simple_type(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    LONG l, l2 = 0;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = NULL;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 16;
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    l = 0xcafebabe;
    NdrSimpleTypeMarshall(&StubMsg, (unsigned char*)&l, FC_LONG);
    ok(StubMsg.Buffer == StubMsg.BufferStart + 4, "%p %p\n", StubMsg.Buffer, StubMsg.BufferStart);
    ok(*(LONG*)StubMsg.BufferStart == l, "%d\n", *(LONG*)StubMsg.BufferStart);

    StubMsg.Buffer = StubMsg.BufferStart + 1;
    NdrSimpleTypeMarshall(&StubMsg, (unsigned char*)&l, FC_LONG);
    ok(StubMsg.Buffer == StubMsg.BufferStart + 8, "%p %p\n", StubMsg.Buffer, StubMsg.BufferStart);
    ok(*(LONG*)(StubMsg.BufferStart + 4) == l, "%d\n", *(LONG*)StubMsg.BufferStart);

    StubMsg.Buffer = StubMsg.BufferStart + 1;
    NdrSimpleTypeUnmarshall(&StubMsg, (unsigned char*)&l2, FC_LONG);
    ok(StubMsg.Buffer == StubMsg.BufferStart + 8, "%p %p\n", StubMsg.Buffer, StubMsg.BufferStart);
    ok(l2 == l, "%d\n", l2);

    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

static void test_pointer_marshal(const unsigned char *formattypes,
                                 void *memsrc, DWORD srcsize,
                                 const void *wiredata,
                                 ULONG wiredatalen,
                                 int(*cmp)(const void*,const void*,size_t),
                                 int num_additional_allocs,
                                 const char *msgpfx)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    DWORD size;
    void *ptr;
    unsigned char *mem, *mem_orig;

    if(!cmp)
        cmp = memcmp;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = formattypes;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrPointerBufferSize( &StubMsg,
                          memsrc,
                          formattypes );
    ok(StubMsg.BufferLength >= wiredatalen, "%s: length %d\n", msgpfx, StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    memset(StubMsg.BufferStart, 0x0, StubMsg.BufferLength); /* This is a hack to clear the padding between the ptr and longlong/double */

    ptr = NdrPointerMarshall( &StubMsg,  memsrc, formattypes );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    if (srcsize == 8 && wiredatalen == 16 && StubMsg.Buffer - StubMsg.BufferStart == 12)
    {
        /* win9x doesn't align 8-byte types properly */
        wiredatalen = 12;
    }
    else
    {
        ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
        ok(!memcmp(StubMsg.BufferStart, wiredata, wiredatalen), "%s: incorrectly marshaled\n", msgpfx);
    }

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;

    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %u size %u\n", msgpfx, StubMsg.MemorySize, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    if (formattypes[1] & FC_POINTER_DEREF)
        ok(size == srcsize + sizeof(void *), "%s: mem size %u\n", msgpfx, size);
    else
        ok(size == srcsize, "%s: mem size %u\n", msgpfx, size);

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 16;
    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %u size %u\n", msgpfx, StubMsg.MemorySize, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    if (formattypes[1] & FC_POINTER_DEREF)
        ok(size == srcsize + sizeof(void *) + 16, "%s: mem size %u\n", msgpfx, size);
    else
        ok(size == srcsize + 16, "%s: mem size %u\n", msgpfx, size);

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 1;
    size = NdrPointerMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: mem size %u size %u\n", msgpfx, StubMsg.MemorySize, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    if (formattypes[1] & FC_POINTER_DEREF)
        ok(size == srcsize + sizeof(void *) + (srcsize == 8 ? 8 : sizeof(void *)), "%s: mem size %u\n", msgpfx, size);
    else
        ok(size == srcsize + (srcsize == 8 ? 8 : sizeof(void *)), "%s: mem size %u\n", msgpfx, size);

    size = srcsize;
    if (formattypes[1] & FC_POINTER_DEREF) size += 4;

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    /* Using my_alloc() here is necessary to prevent a crash in Windows 7+. */
    mem_orig = mem = my_alloc(size);
    memset(mem, 0, size);
    my_alloc_called = my_free_called = 0;
    if (formattypes[1] & FC_POINTER_DEREF)
        *(void**)mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    /* On Windows 7+ unmarshalling may involve calls to NdrFree, for unclear reasons. */
    my_free_called = 0;

    NdrPointerFree(&StubMsg, mem, formattypes);
    if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
    {
        /* In this case the top-level pointer is not freed. */
        ok(my_free_called == num_additional_allocs, "%s: my_free got called %d times\n", msgpfx, my_free_called);
        HeapFree(GetProcessHeap(), 0, mem);
    }
    else
        ok(my_free_called == 1 + num_additional_allocs, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    /* reset the buffer and call with must alloc */
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem_orig = mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (formattypes[1] & FC_POINTER_DEREF)
        *(void**)mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    /* doesn't allocate mem in this case */
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called); 
    ok(!my_free_called, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    NdrPointerFree(&StubMsg, mem, formattypes);
    if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
    {
        /* In this case the top-level pointer is not freed. */
        ok(my_free_called == num_additional_allocs, "%s: my_free got called %d times\n", msgpfx, my_free_called);
        HeapFree(GetProcessHeap(), 0, mem);
    }
    else
        ok(my_free_called == 1 + num_additional_allocs, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    if (formattypes[0] != FC_RP)
    {
        /* now pass the address of a NULL ptr */
        mem = NULL;
        my_alloc_called = my_free_called = 0;
        StubMsg.Buffer = StubMsg.BufferStart;
        ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
        ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
        ok(mem != StubMsg.BufferStart + wiredatalen - srcsize, "%s: mem points to buffer %p %p\n", msgpfx, mem, StubMsg.BufferStart);
        ok(!cmp(mem, memsrc, size), "%s: incorrectly unmarshaled\n", msgpfx);
        ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
        ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
        ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called); 
        my_alloc_called = 0;
        NdrPointerFree(&StubMsg, mem, formattypes);
 
        /* again pass address of NULL ptr, but pretend we're a server */
        if (0)  /* crashes on Win9x and NT4 */
        {
            mem = NULL;
            StubMsg.Buffer = StubMsg.BufferStart;
            StubMsg.IsClient = 0;
            ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
            ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
            if (formattypes[2] == FC_ENUM16)
                ok(mem != StubMsg.BufferStart + wiredatalen - srcsize, "%s: mem points to buffer %p %p\n", msgpfx, mem, StubMsg.BufferStart);
            else
                ok(mem == StubMsg.BufferStart + wiredatalen - srcsize, "%s: mem doesn't point to buffer %p %p\n", msgpfx, mem, StubMsg.BufferStart);
            ok(!cmp(mem, memsrc, size), "%s: incorrectly unmarshaled\n", msgpfx);
            ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
            ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
            if (formattypes[2] != FC_ENUM16)
            {
                ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
                my_alloc_called = 0;
            }
        }
    }

    /* Server */
    StubMsg.IsClient = 0;

    /* For most basetypes (but not enum16), memory will not be allocated but
     * instead point directly to the buffer. */
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(!!mem, "%s: mem was not allocated\n", msgpfx);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
    if (formattypes[2] == FC_ENUM16)
        ok(my_alloc_called == 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    else
        ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    ok(!my_free_called, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    NdrPointerFree(&StubMsg, mem, formattypes);
    if (formattypes[2] == FC_ENUM16)
        ok(my_free_called == 1, "%s: my_free got called %d times\n", msgpfx, my_free_called);
    else if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
    {
        /* In theory this should be freed to correspond with the allocation, but
         * FC_ALLOCED_ON_STACK is set, and NdrPointerFree() has no way of
         * knowing that the memory allocated by NdrPointerUnmarshall() isn't
         * stack memory. In practice it always *is* stack memory if ON_STACK is
         * set, so this leak isn't a concern. */
        ok(my_free_called == 0, "%s: my_free got called %d times\n", msgpfx, my_free_called);
        HeapFree(GetProcessHeap(), 0, mem);
    }
    else
        ok(my_free_called == num_additional_allocs, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    /* reset the buffer and call with must alloc */
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(!!mem, "%s: mem was not allocated\n", msgpfx);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
    if (formattypes[2] == FC_ENUM16)
        ok(my_alloc_called == 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    else
        ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    ok(!my_free_called, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    NdrPointerFree(&StubMsg, mem, formattypes);
    if (formattypes[2] == FC_ENUM16)
        ok(my_free_called == 1, "%s: my_free got called %d times\n", msgpfx, my_free_called);
    else if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
    {
        ok(my_free_called == 0, "%s: my_free got called %d times\n", msgpfx, my_free_called);
        HeapFree(GetProcessHeap(), 0, mem);
    }
    else
        ok(my_free_called == num_additional_allocs, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    /* Τest with an existing pointer. Unless it's a stack pointer (and deref'd)
     * a new pointer will be allocated anyway (in fact, an invalid pointer works
     * in every such case). */

    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem_orig = mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (formattypes[1] & FC_POINTER_DEREF)
        *(void**)mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
        ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    else
    {
        ok(mem != mem_orig, "%s: mem has not changed\n", msgpfx);
        HeapFree(GetProcessHeap(), 0, mem_orig);
    }
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
    if (formattypes[2] == FC_ENUM16)
        ok(my_alloc_called == 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    else if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
        ok(my_alloc_called == 0, "%s: my_alloc got called %d times\n", msgpfx, my_free_called);
    else
        ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    ok(!my_free_called, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    NdrPointerFree(&StubMsg, mem, formattypes);
    if (formattypes[2] == FC_ENUM16)
        ok(my_free_called == 1, "%s: my_free got called %d times\n", msgpfx, my_free_called);
    else if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
    {
        ok(my_free_called == 0, "%s: my_free got called %d times\n", msgpfx, my_free_called);
        HeapFree(GetProcessHeap(), 0, mem);
    }
    else
        ok(my_free_called == num_additional_allocs, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    /* reset the buffer and call with must alloc */
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem_orig = mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (formattypes[1] & FC_POINTER_DEREF)
        *(void**)mem = NULL;
    ptr = NdrPointerUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
        ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    else
    {
        ok(mem != mem_orig, "%s: mem has not changed\n", msgpfx);
        HeapFree(GetProcessHeap(), 0, mem_orig);
    }
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p len %d\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart, wiredatalen);
    ok(StubMsg.MemorySize == 0, "%s: memorysize %d\n", msgpfx, StubMsg.MemorySize);
    if (formattypes[2] == FC_ENUM16)
        ok(my_alloc_called == 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    else if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
        ok(my_alloc_called == 0, "%s: my_alloc got called %d times\n", msgpfx, my_free_called);
    else
        ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    ok(!my_free_called, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    NdrPointerFree(&StubMsg, mem, formattypes);
    if (formattypes[2] == FC_ENUM16)
        ok(my_free_called == 1, "%s: my_free got called %d times\n", msgpfx, my_free_called);
    else if ((formattypes[1] & FC_ALLOCED_ON_STACK) && (formattypes[1] & FC_POINTER_DEREF))
    {
        ok(my_free_called == 0, "%s: my_free got called %d times\n", msgpfx, my_free_called);
        HeapFree(GetProcessHeap(), 0, mem);
    }
    else
        ok(my_free_called == num_additional_allocs, "%s: my_free got called %d times\n", msgpfx, my_free_called);

    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

static int deref_cmp(const void *s1, const void *s2, size_t num)
{
    return memcmp(*(const void *const *)s1, *(const void *const *)s2, num);
}


static void test_simple_types(void)
{
    unsigned char wiredata[16];
    unsigned char ch;
    unsigned char *ch_ptr;
    unsigned short s;
    unsigned int i;
    ULONG l;
    ULONGLONG ll;
    float f;
    double d;

    static const unsigned char fmtstr_up_char[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_byte[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x1,            /* FC_BYTE */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_small[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x3,            /* FC_SMALL */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_usmall[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x4,            /* FC_USMALL */
        0x5c,           /* FC_PAD */
    };  
    static const unsigned char fmtstr_rp_char[] =
    {
        0x11, 0x8,      /* FC_RP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_rpup_char_onstack_deref[] =
    {
        0x11, 0x14,     /* FC_RP [alloced_on_stack] [pointer_deref] */
        NdrFcShort( 0x2 ),      /* Offset= 2 (4) */
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_rpup_char_onstack[] =
    {
        0x11, 0x04,     /* FC_RP [alloced_on_stack] */
        NdrFcShort( 0x2 ),      /* Offset= 2 (4) */
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_rpup_char_deref[] =
    {
        0x11, 0x10,     /* FC_RP [pointer_deref] */
        NdrFcShort( 0x2 ),      /* Offset= 2 (4) */
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x2,            /* FC_CHAR */
        0x5c,           /* FC_PAD */
    };

    static const unsigned char fmtstr_up_wchar[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x5,            /* FC_WCHAR */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_short[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x6,            /* FC_SHORT */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_ushort[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x7,            /* FC_USHORT */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_enum16[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xd,            /* FC_ENUM16 */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_long[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x8,            /* FC_LONG */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_ulong[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x9,            /* FC_ULONG */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_enum32[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xe,            /* FC_ENUM32 */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_errorstatus[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0x10,           /* FC_ERROR_STATUS_T */
        0x5c,           /* FC_PAD */
    };

    static const unsigned char fmtstr_up_longlong[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xb,            /* FC_HYPER */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_float[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xa,            /* FC_FLOAT */
        0x5c,           /* FC_PAD */
    };
    static const unsigned char fmtstr_up_double[] =
    {
        0x12, 0x8,      /* FC_UP [simple_pointer] */
        0xc,            /* FC_DOUBLE */
        0x5c,           /* FC_PAD */
    };

    ch = 0xa5;
    ch_ptr = &ch;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (UINT_PTR)ch_ptr;
    wiredata[4] = ch;
 
    test_pointer_marshal(fmtstr_up_char, ch_ptr, 1, wiredata, 5, NULL, 0, "up_char");
    test_pointer_marshal(fmtstr_up_byte, ch_ptr, 1, wiredata, 5, NULL, 0, "up_byte");
    test_pointer_marshal(fmtstr_up_small, ch_ptr, 1, wiredata, 5, NULL, 0,  "up_small");
    test_pointer_marshal(fmtstr_up_usmall, ch_ptr, 1, wiredata, 5, NULL, 0, "up_usmall");

    test_pointer_marshal(fmtstr_rp_char, ch_ptr, 1, &ch, 1, NULL, 0, "rp_char");

    test_pointer_marshal(fmtstr_rpup_char_onstack_deref, &ch_ptr, 1, wiredata, 5, deref_cmp, 1, "rpup_char_onstack_deref");
    test_pointer_marshal(fmtstr_rpup_char_onstack, ch_ptr, 1, wiredata, 5, NULL, 0, "rpup_char_onstack");
    test_pointer_marshal(fmtstr_rpup_char_deref, &ch_ptr, 1, wiredata, 5, deref_cmp, 1, "rpup_char_deref");

    s = 0xa597;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (UINT_PTR)&s;
    *(unsigned short*)(wiredata + 4) = s;

    test_pointer_marshal(fmtstr_up_wchar, &s, 2, wiredata, 6, NULL, 0, "up_wchar");
    test_pointer_marshal(fmtstr_up_short, &s, 2, wiredata, 6, NULL, 0, "up_short");
    test_pointer_marshal(fmtstr_up_ushort, &s, 2, wiredata, 6, NULL, 0, "up_ushort");

    i = 0x7fff;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (UINT_PTR)&i;
    *(unsigned short*)(wiredata + 4) = i;
    test_pointer_marshal(fmtstr_up_enum16, &i, 4, wiredata, 6, NULL, 0, "up_enum16");

    l = 0xcafebabe;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (UINT_PTR)&l;
    *(ULONG*)(wiredata + 4) = l;

    test_pointer_marshal(fmtstr_up_long, &l, 4, wiredata, 8, NULL, 0, "up_long");
    test_pointer_marshal(fmtstr_up_ulong, &l, 4, wiredata, 8, NULL, 0,  "up_ulong");
    test_pointer_marshal(fmtstr_up_enum32, &l, 4, wiredata, 8, NULL, 0,  "up_emun32");
    test_pointer_marshal(fmtstr_up_errorstatus, &l, 4, wiredata, 8, NULL, 0,  "up_errorstatus");

    ll = ((ULONGLONG)0xcafebabe) << 32 | 0xdeadbeef;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (UINT_PTR)&ll;
    *(unsigned int *)(wiredata + 4) = 0;
    *(ULONGLONG*)(wiredata + 8) = ll;
    test_pointer_marshal(fmtstr_up_longlong, &ll, 8, wiredata, 16, NULL, 0, "up_longlong");

    f = 3.1415f;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (UINT_PTR)&f;
    *(float*)(wiredata + 4) = f;
    test_pointer_marshal(fmtstr_up_float, &f, 4, wiredata, 8, NULL, 0, "up_float");

    d = 3.1415;
    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (UINT_PTR)&d;
    *(unsigned int *)(wiredata + 4) = 0;
    *(double*)(wiredata + 8) = d;
    test_pointer_marshal(fmtstr_up_double, &d, 8, wiredata, 16, NULL, 0,  "up_double");

}

static void test_nontrivial_pointer_types(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    DWORD size;
    void *ptr;
    char **p1;
    char *p2;
    char ch;
    unsigned char *mem, *mem_orig;

    static const unsigned char fmtstr_ref_unique_out[] =
    {
        0x12, 0x8,	/* FC_UP [simple_pointer] */
        0x2,		/* FC_CHAR */
        0x5c,		/* FC_PAD */
        0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
        NdrFcShort( 0xfffffffa ),	/* Offset= -6 (0) */
    };

    p1 = &p2;
    p2 = &ch;
    ch = 0x22;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_ref_unique_out;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrPointerBufferSize( &StubMsg,
                          (unsigned char *)p1,
                          &fmtstr_ref_unique_out[4] );

    /* Windows overestimates the buffer size */
    ok(StubMsg.BufferLength >= 5, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrPointerMarshall( &StubMsg, (unsigned char *)p1, &fmtstr_ref_unique_out[4] );
    ok(ptr == NULL, "ret %p\n", ptr);
    size = StubMsg.Buffer - StubMsg.BufferStart;
    ok(size == 5, "Buffer %p Start %p len %d\n", StubMsg.Buffer, StubMsg.BufferStart, size);
    ok(*(unsigned int *)StubMsg.BufferStart != 0, "pointer ID marshalled incorrectly\n");
    ok(*(unsigned char *)(StubMsg.BufferStart + 4) == 0x22, "char data marshalled incorrectly: 0x%x\n",
       *(unsigned char *)(StubMsg.BufferStart + 4));

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(void *));
    *(void **)mem = NULL;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 1);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 1, "free called %d\n", my_free_called);

    mem = my_alloc(sizeof(void *));
    *(void **)mem = NULL;
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 0, "free called %d\n", my_free_called);
    my_free(mem);

    mem = my_alloc(sizeof(void *));
    *(void **)mem = my_alloc(sizeof(char));
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 1, "free called %d\n", my_free_called);
    my_free(mem);

    /* Server */
    my_alloc_called = 0;
    my_free_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 0);
    ok(mem != StubMsg.BufferStart, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 0, "free called %d\n", my_free_called);
    my_free(mem);

    my_alloc_called = 0;
    my_free_called = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 1);
    ok(mem != StubMsg.BufferStart, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 0, "free called %d\n", my_free_called);
    my_free(mem);

    my_alloc_called = 0;
    mem = mem_orig;
    *(void **)mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    *(void **)mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, &fmtstr_ref_unique_out[4], 1);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    mem = my_alloc(sizeof(void *));
    *(void **)mem = NULL;
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 0, "free called %d\n", my_free_called);
    my_free(mem);

    mem = my_alloc(sizeof(void *));
    *(void **)mem = my_alloc(sizeof(char));
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, &fmtstr_ref_unique_out[4] );
    ok(my_free_called == 1, "free called %d\n", my_free_called);
    my_free(mem);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
}

static void test_simple_struct_marshal(const unsigned char *formattypes,
                                       void *memsrc, DWORD srcsize,
                                       const void *wiredata,
                                       ULONG wiredatalen,
                                       int(*cmp)(const void*,const void*,size_t),
                                       int num_additional_allocs,
                                       const char *msgpfx)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    DWORD size;
    void *ptr;
    unsigned char *mem, *mem_orig;

    my_alloc_called = my_free_called = 0;
    if(!cmp)
        cmp = memcmp;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = formattypes;

    NdrClientInitializeNew(&RpcMessage, &StubMsg, &StubDesc, 0);

    StubMsg.BufferLength = 0;
    NdrSimpleStructBufferSize( &StubMsg, memsrc, formattypes );
    ok(StubMsg.BufferLength >= wiredatalen, "%s: length %d\n", msgpfx, StubMsg.BufferLength);
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;
    ptr = NdrSimpleStructMarshall( &StubMsg,  memsrc, formattypes );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart);
    ok(!memcmp(StubMsg.BufferStart, wiredata, wiredatalen), "%s: incorrectly marshaled %08x %08x %08x\n", msgpfx, *(DWORD*)StubMsg.BufferStart,*((DWORD*)StubMsg.BufferStart+1),*((DWORD*)StubMsg.BufferStart+2));

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    size = NdrSimpleStructMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: size != MemorySize\n", msgpfx);
    ok(size == srcsize, "%s: mem size %u\n", msgpfx, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart);

    StubMsg.Buffer = StubMsg.BufferStart;
    size = NdrSimpleStructMemorySize( &StubMsg, formattypes );
    ok(size == StubMsg.MemorySize, "%s: size != MemorySize\n", msgpfx);
    ok(StubMsg.MemorySize == ((srcsize + 3) & ~3) + srcsize, "%s: mem size %u\n", msgpfx, size);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart);
    size = srcsize;
    /*** Unmarshalling first with must_alloc false ***/

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem_orig = mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, srcsize);
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == wiredatalen, "%s: Buffer %p Start %p\n", msgpfx, StubMsg.Buffer, StubMsg.BufferStart);
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called); 
    my_alloc_called = 0;
    my_free_called = 0;
    ok(StubMsg.MemorySize == 0, "%s: memorysize touched in unmarshal\n", msgpfx);
    NdrSimpleStructFree(&StubMsg, mem, formattypes );
    ok(my_free_called == num_additional_allocs, "free called %d\n", my_free_called);

    /* If we're a server we still use the supplied memory */
    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.IsClient = 0;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 0 );
    ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
    ok(mem == mem_orig, "%s: mem has changed %p %p\n", msgpfx, mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx); 
    ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    my_free_called = 0;
    ok(StubMsg.MemorySize == 0, "%s: memorysize touched in unmarshal\n", msgpfx);
    NdrSimpleStructFree(&StubMsg, mem, formattypes );
    ok(my_free_called == num_additional_allocs, "free called %d\n", my_free_called);

    /* ...unless we pass a NULL ptr, then the buffer is used. 
       Passing a NULL ptr while we're a client && !must_alloc
       crashes on Windows, so we won't do that. */

    if (0)  /* crashes on Win9x and NT4 */
    {
        mem = NULL;
        StubMsg.IsClient = 0;
        StubMsg.Buffer = StubMsg.BufferStart;
        ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, FALSE );
        ok(ptr == NULL, "%s: ret %p\n", msgpfx, ptr);
        ok(mem == StubMsg.BufferStart, "%s: mem not equal buffer\n", msgpfx);
        ok(!cmp(mem, memsrc, srcsize), "%s: incorrectly unmarshaled\n", msgpfx);
        ok(my_alloc_called == num_additional_allocs, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
        my_alloc_called = 0;
        ok(StubMsg.MemorySize == 0, "%s: memorysize touched in unmarshal\n", msgpfx);
        NdrSimpleStructFree(&StubMsg, mem, formattypes );
        ok(my_free_called == num_additional_allocs, "free called %d\n", my_free_called);
    }

    /*** now must_alloc is true ***/

    /* with must_alloc set we always allocate new memory whether or not we're
       a server and also when passing NULL */
    mem = mem_orig;
    StubMsg.IsClient = 1;
    StubMsg.Buffer = StubMsg.BufferStart;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem != mem_orig, "mem not changed %p %p\n", mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "incorrectly unmarshaled\n");
    ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    my_free_called = 0;
    ok(StubMsg.MemorySize == 0, "memorysize touched in unmarshal\n");
    NdrSimpleStructFree(&StubMsg, mem, formattypes );
    ok(my_free_called == num_additional_allocs, "free called %d\n", my_free_called);
    my_free(mem);

    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem != mem_orig, "mem not changed %p %p\n", mem, mem_orig);
    ok(!cmp(mem, memsrc, srcsize), "incorrectly unmarshaled\n");
    ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    my_free_called = 0;
    ok(StubMsg.MemorySize == 0, "memorysize touched in unmarshal\n");
    NdrSimpleStructFree(&StubMsg, mem, formattypes );
    ok(my_free_called == num_additional_allocs, "free called %d\n", my_free_called);
    my_free(mem);

    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.IsClient = 0;
    StubMsg.ReuseBuffer = 1;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem != mem_orig, "mem not changed %p %p\n", mem, mem_orig);
    ok(mem != StubMsg.BufferStart, "mem is buffer mem\n");
    ok(!cmp(mem, memsrc, srcsize), "incorrectly unmarshaled\n");
    ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    my_free_called = 0;
    ok(StubMsg.MemorySize == 0, "memorysize touched in unmarshal\n");
    NdrSimpleStructFree(&StubMsg, mem, formattypes );
    ok(my_free_called == num_additional_allocs, "free called %d\n", my_free_called);
    my_free(mem);

    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.IsClient = 0;
    StubMsg.ReuseBuffer = 1;
    ptr = NdrSimpleStructUnmarshall( &StubMsg, &mem, formattypes, 1 );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem != StubMsg.BufferStart, "mem is buffer mem\n");
    ok(!cmp(mem, memsrc, srcsize), "incorrectly unmarshaled\n"); 
    ok(my_alloc_called == num_additional_allocs + 1, "%s: my_alloc got called %d times\n", msgpfx, my_alloc_called);
    my_alloc_called = 0;
    my_free_called = 0;
    ok(StubMsg.MemorySize == 0, "memorysize touched in unmarshal\n");
    NdrSimpleStructFree(&StubMsg, mem, formattypes );
    ok(my_free_called == num_additional_allocs, "free called %d\n", my_free_called);
    my_free(mem);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);
}

typedef struct
{
    LONG l1;
    LONG *pl1;
    char *pc1;
} ps1_t;

static int ps1_cmp(const void *s1, const void *s2, size_t num)
{
    const ps1_t *p1, *p2;

    p1 = s1;
    p2 = s2;

    if(p1->l1 != p2->l1)
        return 1;

    if(p1->pl1 && p2->pl1)
    {
        if(*p1->pl1 != *p2->pl1)
            return 1;
    }
    else if(p1->pl1 || p2->pl1)
        return 1;

    if(p1->pc1 && p2->pc1)
    {
        if(*p1->pc1 != *p2->pc1)
            return 1;
    }
    else if(p1->pc1 || p2->pc1)
        return 1;

    return 0;
}

static void test_simple_struct(void)
{
    unsigned char wiredata[28];
    ULONG wiredatalen;
    LONG l;
    char c;
    ps1_t ps1;

    static const unsigned char fmtstr_simple_struct[] =
    {
        0x12, 0x0,      /* FC_UP */
        NdrFcShort( 0x2 ), /* Offset=2 */
        0x15, 0x3,      /* FC_STRUCT [align 4] */
        NdrFcShort( 0x18 ),      /* [size 24] */
        0x6,            /* FC_SHORT */
        0x2,            /* FC_CHAR */ 
        0x38,		/* FC_ALIGNM4 */
	0x8,		/* FC_LONG */
	0x8,		/* FC_LONG */
        0x39,		/* FC_ALIGNM8 */
        0xb,		/* FC_HYPER */ 
        0x5b,		/* FC_END */
    };
    struct {
        short s;
        char c;
        LONG l1, l2;
        LONGLONG ll;
    } s1;

    static const unsigned char fmtstr_pointer_struct[] =
    { 
        0x12, 0x0,      /* FC_UP */
        NdrFcShort( 0x2 ), /* Offset=2 */
#ifdef _WIN64
        0x1a,	/* FC_BOGUS_STRUCT */
        0x3,	/* 3 */
        NdrFcShort(0x18),	/* [size 24] */
        NdrFcShort(0x0),
        NdrFcShort(0x8),	/* Offset= 8 (266) */
        0x08,	/* FC_LONG */
        0x39,	/* FC_ALIGNM8 */
        0x36,	/* FC_POINTER */
        0x36,	/* FC_POINTER */
        0x5c,		/* FC_PAD */
        0x5b,		/* FC_END */
        0x12, 0x8,	/* FC_UP [simple_pointer] */
        0x08,	/* FC_LONG */
        0x5c,	/* FC_PAD */
        0x12, 0x8,	/* FC_UP [simple_pointer] */
        0x02,	/* FC_CHAR */
        0x5c,	/* FC_PAD */
#else
        0x16, 0x3,      /* FC_PSTRUCT [align 4] */
        NdrFcShort( 0xc ),      /* [size 12] */
        0x4b,		/* FC_PP */
        0x5c,		/* FC_PAD */
        0x46,		/* FC_NO_REPEAT */
        0x5c,		/* FC_PAD */
        NdrFcShort( 0x4 ),	/* 4 */
	NdrFcShort( 0x4 ),	/* 4 */
        0x13, 0x8,	/* FC_OP [simple_pointer] */
        0x8,		/* FC_LONG */
        0x5c,		/* FC_PAD */
        0x46,		/* FC_NO_REPEAT */
        0x5c,		/* FC_PAD */
	NdrFcShort( 0x8 ),	/* 8 */
	NdrFcShort( 0x8 ),	/* 8 */
	0x13, 0x8,	/* FC_OP [simple_pointer] */
        0x2,		/* FC_CHAR */
        0x5c,		/* FC_PAD */
        0x5b,		/* FC_END */
        0x8,		/* FC_LONG */
        0x8,		/* FC_LONG */
        0x8,		/* FC_LONG */
        0x5c,		/* FC_PAD */
        0x5b,		/* FC_END */
#endif
    };

    /* zero the entire structure, including the holes */
    memset(&s1, 0, sizeof(s1));

    /* FC_STRUCT */
    s1.s = 0x1234;
    s1.c = 0xa5;
    s1.l1 = 0xdeadbeef;
    s1.l2 = 0xcafebabe;
    s1.ll = ((ULONGLONG) 0xbadefeed << 32) | 0x2468ace0;

    wiredatalen = 24;
    memcpy(wiredata, &s1, wiredatalen); 
    test_simple_struct_marshal(fmtstr_simple_struct + 4, &s1, 24, wiredata, 24, NULL, 0, "struct");

    if (use_pointer_ids)
        *(unsigned int *)wiredata = 0x20000;
    else
        *(unsigned int *)wiredata = (UINT_PTR)&s1;
    memcpy(wiredata + 4, &s1, wiredatalen);
    test_pointer_marshal(fmtstr_simple_struct, &s1, 24, wiredata, 28, NULL, 0, "struct");

    if (sizeof(void *) == 8) return;  /* it cannot be represented as a simple struct on Win64 */

    /* zero the entire structure, including the hole */
    memset(&ps1, 0, sizeof(ps1));

    /* FC_PSTRUCT */
    ps1.l1 = 0xdeadbeef;
    l = 0xcafebabe;
    ps1.pl1 = &l;
    c = 'a';
    ps1.pc1 = &c;
    *(unsigned int *)(wiredata + 4) = 0xdeadbeef;
    if (use_pointer_ids)
    {
	*(unsigned int *)(wiredata + 8) = 0x20000;
	*(unsigned int *)(wiredata + 12) = 0x20004;
    }
    else
    {
	*(unsigned int *)(wiredata + 8) = (UINT_PTR)&l;
	*(unsigned int *)(wiredata + 12) = (UINT_PTR)&c;
    }
    memcpy(wiredata + 16, &l, 4);
    memcpy(wiredata + 20, &c, 1);

    test_simple_struct_marshal(fmtstr_pointer_struct + 4, &ps1, 17, wiredata + 4, 17, ps1_cmp, 2, "pointer_struct");
    if (use_pointer_ids)
    {
        *(unsigned int *)wiredata = 0x20000;
	*(unsigned int *)(wiredata + 8) = 0x20004;
	*(unsigned int *)(wiredata + 12) = 0x20008;
    }
    else
        *(unsigned int *)wiredata = (UINT_PTR)&ps1;
    test_pointer_marshal(fmtstr_pointer_struct, &ps1, 17, wiredata, 21, ps1_cmp, 2, "pointer_struct");
}

struct aligned
{
    int a;
    LONGLONG b;
};

static void test_struct_align(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *ptr;
    struct aligned *memsrc_orig, *memsrc, *mem;

    /* force bogus struct so that members are marshalled individually */
    static const unsigned char fmtstr[] =
    {
        0x1a,   /* FC_BOGUS_STRUCT */
        0x7,    /* alignment 8 */
        NdrFcShort(0x10),   /* memory size 16 */
        NdrFcShort(0x0),
        NdrFcShort(0x0),
        0x08,   /* FC_LONG */
        0x39,   /* FC_ALIGNM8 */
        0x0b,   /* FC_HYPER */
        0x5b,   /* FC_END */
    };

    memsrc_orig = heap_alloc_zero(sizeof(struct aligned) + 8);
    /* intentionally mis-align memsrc */
    memsrc = (struct aligned *)((((ULONG_PTR)memsrc_orig + 7) & ~7) + 4);

    memsrc->a = 0xdeadbeef;
    memsrc->b = ((ULONGLONG) 0xbadefeed << 32) | 0x2468ace0;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr;
    NdrClientInitializeNew(&RpcMessage, &StubMsg, &StubDesc, 0);

    StubMsg.BufferLength = 0;
    NdrComplexStructBufferSize(&StubMsg, (unsigned char *)memsrc, fmtstr);

    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = heap_alloc(StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrComplexStructMarshall(&StubMsg, (unsigned char *)memsrc, fmtstr);
    ok(ptr == NULL, "ret %p\n", ptr);

    /* Server */
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    ptr = NdrComplexStructUnmarshall(&StubMsg, (unsigned char **)&mem, fmtstr, 0);
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(!memcmp(mem, memsrc, sizeof(*memsrc)), "struct wasn't unmarshalled correctly\n");
    StubMsg.pfnFree(mem);

    heap_free(StubMsg.RpcMsg->Buffer);
    heap_free(memsrc_orig);
}

struct testiface
{
    IPersist IPersist_iface;
    LONG ref;
};

static struct testiface *impl_from_IPersist(IPersist *iface)
{
    return CONTAINING_RECORD(iface, struct testiface, IPersist_iface);
}

static HRESULT WINAPI test_persist_QueryInterface(IPersist *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IPersist))
    {
        *out = iface;
        IPersist_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_persist_AddRef(IPersist *iface)
{
    struct testiface *unk = impl_from_IPersist(iface);
    return ++unk->ref;
}

static ULONG WINAPI test_persist_Release(IPersist *iface)
{
    struct testiface *unk = impl_from_IPersist(iface);
    return --unk->ref;
}

static HRESULT WINAPI test_persist_GetClassId(IPersist *iface, GUID *clsid)
{
    *clsid = IID_IPersist;
    return S_OK;
}

static IPersistVtbl testiface_vtbl = {
    test_persist_QueryInterface,
    test_persist_AddRef,
    test_persist_Release,
    test_persist_GetClassId,
};

static void test_iface_ptr(void)
{
    struct testiface server_obj = {{&testiface_vtbl}, 1};
    struct testiface client_obj = {{&testiface_vtbl}, 1};

    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    RPC_MESSAGE RpcMessage;
    IPersist *proxy;
    HRESULT hr;
    GUID clsid;
    void *ptr;
    LONG ref;

    static const unsigned char fmtstr_ip[] =
    {
        FC_IP,
        FC_CONSTANT_IID,
        NdrFcLong(0x0000010c),
        NdrFcShort(0x0000),
        NdrFcShort(0x0000),
        0xc0,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x46,
    };

    CoInitialize(NULL);

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_ip;

    NdrClientInitializeNew(&RpcMessage, &StubMsg, &StubDesc, 0);
    StubMsg.BufferLength = 0;
    NdrInterfacePointerBufferSize(&StubMsg, (unsigned char *)&client_obj.IPersist_iface, fmtstr_ip);

    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    /* server -> client */

    StubMsg.IsClient = 0;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    IPersist_AddRef(&server_obj.IPersist_iface);
    ptr = NdrInterfacePointerMarshall(&StubMsg, (unsigned char *)&server_obj.IPersist_iface, fmtstr_ip);
    ok(!ptr, "ret %p\n", ptr);
    ok(server_obj.ref > 2, "got %d references\n", server_obj.ref);
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);

    NdrInterfacePointerFree(&StubMsg, (unsigned char *)&server_obj.IPersist_iface, fmtstr_ip);
    ok(server_obj.ref > 1, "got %d references\n", server_obj.ref);

    StubMsg.IsClient = 1;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    proxy = NULL;
    ptr = NdrInterfacePointerUnmarshall(&StubMsg, (unsigned char **)&proxy, fmtstr_ip, 0);
    ok(!ptr, "ret %p\n", ptr);
    ok(!!proxy, "mem not alloced\n");
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);
    ok(server_obj.ref > 1, "got %d references\n", server_obj.ref);

    hr = IPersist_GetClassID(proxy, &clsid);
    ok(hr == S_OK, "got hr %#x\n", hr);
    ok(IsEqualGUID(&clsid, &IID_IPersist), "wrong clsid %s\n", wine_dbgstr_guid(&clsid));

    ref = IPersist_Release(proxy);
    ok(ref == 1, "got %d references\n", ref);
    ok(server_obj.ref == 1, "got %d references\n", server_obj.ref);

    /* An existing interface pointer is released; this is necessary so that an
     * [in, out] pointer which changes does not leak references. */

    StubMsg.IsClient = 0;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    IPersist_AddRef(&server_obj.IPersist_iface);
    ptr = NdrInterfacePointerMarshall(&StubMsg, (unsigned char *)&server_obj.IPersist_iface, fmtstr_ip);
    ok(!ptr, "ret %p\n", ptr);
    ok(server_obj.ref > 2, "got %d references\n", server_obj.ref);
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);

    NdrInterfacePointerFree(&StubMsg, (unsigned char *)&server_obj.IPersist_iface, fmtstr_ip);
    ok(server_obj.ref > 1, "got %d references\n", server_obj.ref);

    StubMsg.IsClient = 1;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    proxy = &client_obj.IPersist_iface;
    IPersist_AddRef(proxy);
    ptr = NdrInterfacePointerUnmarshall(&StubMsg, (unsigned char **)&proxy, fmtstr_ip, 0);
    ok(!ptr, "ret %p\n", ptr);
    ok(!!proxy && proxy != &client_obj.IPersist_iface, "mem not alloced\n");
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);
    ok(server_obj.ref > 1, "got %d references\n", server_obj.ref);
    ok(client_obj.ref == 1, "got %d references\n", client_obj.ref);

    hr = IPersist_GetClassID(proxy, &clsid);
    ok(hr == S_OK, "got hr %#x\n", hr);
    ok(IsEqualGUID(&clsid, &IID_IPersist), "wrong clsid %s\n", wine_dbgstr_guid(&clsid));

    ref = IPersist_Release(proxy);
    ok(ref == 1, "got %d references\n", ref);
    ok(server_obj.ref == 1, "got %d references\n", server_obj.ref);

    /* client -> server */

    StubMsg.IsClient = 1;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    IPersist_AddRef(&client_obj.IPersist_iface);
    ptr = NdrInterfacePointerMarshall(&StubMsg, (unsigned char *)&client_obj.IPersist_iface, fmtstr_ip);
    ok(!ptr, "ret %p\n", ptr);
    ok(client_obj.ref > 2, "got %d references\n", client_obj.ref);
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);

    StubMsg.IsClient = 0;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    proxy = NULL;
    ptr = NdrInterfacePointerUnmarshall(&StubMsg, (unsigned char **)&proxy, fmtstr_ip, 0);
    ok(!ptr, "ret %p\n", ptr);
    ok(!!proxy, "mem not alloced\n");
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);
    ok(client_obj.ref > 2, "got %d references\n", client_obj.ref);

    hr = IPersist_GetClassID(proxy, &clsid);
    ok(hr == S_OK, "got hr %#x\n", hr);
    ok(IsEqualGUID(&clsid, &IID_IPersist), "wrong clsid %s\n", wine_dbgstr_guid(&clsid));

    ref = IPersist_Release(proxy);
    ok(client_obj.ref > 1, "got %d references\n", client_obj.ref);
    ok(ref == client_obj.ref, "expected %d references, got %d\n", client_obj.ref, ref);

    NdrInterfacePointerFree(&StubMsg, (unsigned char *)proxy, fmtstr_ip);
    ok(client_obj.ref == 1, "got %d references\n", client_obj.ref);

    /* same, but free the interface after calling NdrInterfacePointerFree */

    StubMsg.IsClient = 1;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    IPersist_AddRef(&client_obj.IPersist_iface);
    ptr = NdrInterfacePointerMarshall(&StubMsg, (unsigned char *)&client_obj.IPersist_iface, fmtstr_ip);
    ok(!ptr, "ret %p\n", ptr);
    ok(client_obj.ref > 2, "got %d references\n", client_obj.ref);
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);

    StubMsg.IsClient = 0;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    proxy = NULL;
    ptr = NdrInterfacePointerUnmarshall(&StubMsg, (unsigned char **)&proxy, fmtstr_ip, 0);
    ok(!ptr, "ret %p\n", ptr);
    ok(!!proxy, "mem not alloced\n");
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);
    ok(client_obj.ref > 2, "got %d references\n", client_obj.ref);

    NdrInterfacePointerFree(&StubMsg, (unsigned char *)proxy, fmtstr_ip);
    ok(client_obj.ref > 1, "got %d references\n", client_obj.ref);

    hr = IPersist_GetClassID(proxy, &clsid);
    ok(hr == S_OK, "got hr %#x\n", hr);
    ok(IsEqualGUID(&clsid, &IID_IPersist), "wrong clsid %s\n", wine_dbgstr_guid(&clsid));

    ref = IPersist_Release(proxy);
    ok(ref == 1, "got %d references\n", ref);
    ok(client_obj.ref == 1, "got %d references\n", client_obj.ref);

    /* An existing interface pointer is *not* released (in fact, it is ignored
     * and may be invalid). In practice it will always be NULL anyway. */

    StubMsg.IsClient = 1;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    IPersist_AddRef(&client_obj.IPersist_iface);
    ptr = NdrInterfacePointerMarshall(&StubMsg, (unsigned char *)&client_obj.IPersist_iface, fmtstr_ip);
    ok(!ptr, "ret %p\n", ptr);
    ok(client_obj.ref > 2, "got %d references\n", client_obj.ref);
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);

    StubMsg.IsClient = 0;
    my_alloc_called = my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    proxy = &server_obj.IPersist_iface;
    IPersist_AddRef(proxy);
    ptr = NdrInterfacePointerUnmarshall(&StubMsg, (unsigned char **)&proxy, fmtstr_ip, 0);
    ok(!ptr, "ret %p\n", ptr);
    ok(!!proxy && proxy != &server_obj.IPersist_iface, "mem not alloced\n");
    ok(!my_alloc_called, "alloc called %d\n", my_alloc_called);
    ok(!my_free_called, "free called %d\n", my_free_called);
    ok(client_obj.ref > 2, "got %d references\n", client_obj.ref);
    ok(server_obj.ref == 2, "got %d references\n", server_obj.ref);
    IPersist_Release(&server_obj.IPersist_iface);

    hr = IPersist_GetClassID(proxy, &clsid);
    ok(hr == S_OK, "got hr %#x\n", hr);
    ok(IsEqualGUID(&clsid, &IID_IPersist), "wrong clsid %s\n", wine_dbgstr_guid(&clsid));

    ref = IPersist_Release(proxy);
    ok(client_obj.ref > 1, "got %d references\n", client_obj.ref);
    ok(ref == client_obj.ref, "expected %d references, got %d\n", client_obj.ref, ref);

    NdrInterfacePointerFree(&StubMsg, (unsigned char *)proxy, fmtstr_ip);
    ok(client_obj.ref == 1, "got %d references\n", client_obj.ref);

    HeapFree(GetProcessHeap(), 0, StubMsg.BufferStart);

    CoUninitialize();
}

static void test_fullpointer_xlat(void)
{
    PFULL_PTR_XLAT_TABLES pXlatTables;
    ULONG RefId;
    int ret;
    void *Pointer;

    pXlatTables = NdrFullPointerXlatInit(2, XLAT_CLIENT);

    /* "marshaling" phase */

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 1, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x1, "RefId should be 0x1 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x1, "RefId should be 0x1 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebabe, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x2, "RefId should be 0x2 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, NULL, 0, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0, "RefId should be 0 instead of 0x%x\n", RefId);

    /* "unmarshaling" phase */

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x0, 0, &Pointer);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x2, 0, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == (void *)0xcafebabe, "Pointer should be 0xcafebabe instead of %p\n", Pointer);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x4, 0, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == NULL, "Pointer should be NULL instead of %p\n", Pointer);

    NdrFullPointerInsertRefId(pXlatTables, 0x4, (void *)0xdeadbabe);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x4, 1, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == (void *)0xdeadbabe, "Pointer should be (void *)0xdeadbabe instead of %p\n", Pointer);

    NdrFullPointerXlatFree(pXlatTables);

    pXlatTables = NdrFullPointerXlatInit(2, XLAT_SERVER);

    /* "unmarshaling" phase */

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x2, 1, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == NULL, "Pointer should be NULL instead of %p\n", Pointer);

    NdrFullPointerInsertRefId(pXlatTables, 0x2, (void *)0xcafebabe);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x2, 0, &Pointer);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(Pointer == (void *)0xcafebabe, "Pointer should be (void *)0xcafebabe instead of %p\n", Pointer);

    ret = NdrFullPointerQueryRefId(pXlatTables, 0x2, 1, &Pointer);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(Pointer == (void *)0xcafebabe, "Pointer should be (void *)0xcafebabe instead of %p\n", Pointer);

    /* "marshaling" phase */

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 1, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 1, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebabe, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x2, "RefId should be 0x2 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 0, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x4, "RefId should be 0x4 instead of 0x%x\n", RefId);

    /* "freeing" phase */

    ret = NdrFullPointerFree(pXlatTables, (void *)0xcafebeef);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 0x20, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xcafebeef, 1, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x3, "RefId should be 0x3 instead of 0x%x\n", RefId);

    ret = NdrFullPointerFree(pXlatTables, (void *)0xcafebabe);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);

    ret = NdrFullPointerFree(pXlatTables, (void *)0xdeadbeef);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 0x20, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x4, "RefId should be 0x4 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 1, &RefId);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);
    ok(RefId == 0x4, "RefId should be 0x4 instead of 0x%x\n", RefId);

    ret = NdrFullPointerQueryPointer(pXlatTables, (void *)0xdeadbeef, 1, &RefId);
    ok(ret == 1, "ret should be 1 instead of 0x%x\n", ret);
    ok(RefId == 0x4, "RefId should be 0x4 instead of 0x%x\n", RefId);

    ret = NdrFullPointerFree(pXlatTables, (void *)0xdeadbeef);
    ok(ret == 0, "ret should be 0 instead of 0x%x\n", ret);

    NdrFullPointerXlatFree(pXlatTables);
}

/* verify stub data that is identical between client and server */
static void test_common_stub_data( const char *prefix, const MIDL_STUB_MESSAGE *stubMsg )
{
    void *unset_ptr;

    memset(&unset_ptr, 0xcc, sizeof(unset_ptr));

#define TEST_ZERO(field, fmt) ok(stubMsg->field == 0, "%s: " #field " should have been set to zero instead of " fmt "\n", prefix, stubMsg->field)
#define TEST_POINTER_UNSET(field) ok(stubMsg->field == unset_ptr, "%s: " #field " should have been unset instead of %p\n", prefix, stubMsg->field)
#define TEST_ULONG_UNSET(field) ok(stubMsg->field == 0xcccccccc, "%s: " #field " should have been unset instead of 0x%x\n", prefix, stubMsg->field)
#define TEST_ULONG_PTR_UNSET(field) ok(stubMsg->field == (ULONG_PTR)unset_ptr, "%s: " #field " should have been unset instead of 0x%lx\n", prefix, stubMsg->field)

    TEST_POINTER_UNSET(BufferMark);
    TEST_ULONG_UNSET(MemorySize);
    TEST_POINTER_UNSET(Memory);
    TEST_ZERO(pAllocAllNodesContext, "%p");
    ok(stubMsg->pPointerQueueState == 0 ||
       broken(stubMsg->pPointerQueueState == unset_ptr), /* win2k */
       "%s: pPointerQueueState should have been unset instead of %p\n",
       prefix, stubMsg->pPointerQueueState);
    TEST_ZERO(IgnoreEmbeddedPointers, "%d");
    TEST_ZERO(PointerBufferMark, "%p");
    ok( stubMsg->uFlags == 0 ||
        broken(stubMsg->uFlags == 0xcc), /* win9x */
        "%s: uFlags should have been set to zero instead of 0x%x\n", prefix, stubMsg->uFlags );
    /* FIXME: UniquePtrCount */
    TEST_ULONG_PTR_UNSET(MaxCount);
    TEST_ULONG_UNSET(Offset);
    TEST_ULONG_UNSET(ActualCount);
    ok(stubMsg->pfnAllocate == my_alloc, "%s: pfnAllocate should have been %p instead of %p\n",
       prefix, my_alloc, stubMsg->pfnAllocate);
    ok(stubMsg->pfnFree == my_free, "%s: pfnFree should have been %p instead of %p\n",
       prefix, my_free, stubMsg->pfnFree);
    TEST_ZERO(StackTop, "%p");
    TEST_POINTER_UNSET(pPresentedType);
    TEST_POINTER_UNSET(pTransmitType);
    TEST_POINTER_UNSET(SavedHandle);
    ok(stubMsg->StubDesc == &Object_StubDesc, "%s: StubDesc should have been %p instead of %p\n",
       prefix, &Object_StubDesc, stubMsg->StubDesc);
    TEST_ZERO(FullPtrRefId, "%d");
    ok( stubMsg->PointerLength == 0 ||
        broken(stubMsg->PointerLength == 1), /* win9x, nt4 */
        "%s: pAsyncMsg should have been set to zero instead of %d\n", prefix, stubMsg->PointerLength );
    TEST_ZERO(fInDontFree, "%d");
    TEST_ZERO(fDontCallFreeInst, "%d");
    ok( stubMsg->fHasReturn == 0 || broken(stubMsg->fHasReturn), /* win9x, nt4 */
        "%s: fHasReturn should have been set to zero instead of %d\n", prefix, stubMsg->fHasReturn );
    TEST_ZERO(fHasExtensions, "%d");
    TEST_ZERO(fHasNewCorrDesc, "%d");
    ok(stubMsg->fIsIn == 0 || broken(stubMsg->fIsIn), /* win9x, nt4 */
       "%s: fIsIn should have been set to 0 instead of %d\n", prefix, stubMsg->fIsIn);
    TEST_ZERO(fIsOicf, "%d");
    ok(stubMsg->fBufferValid == 0,
       "%s: fBufferValid should have been set to 0 instead of %d\n", prefix, stubMsg->fBufferValid);
    TEST_ZERO(fNeedMCCP, "%d");
    ok(stubMsg->fUnused == 0 ||
       stubMsg->fUnused == -2, /* Vista */
       "%s: fUnused should have been set to 0 or -2 instead of %d\n", prefix, stubMsg->fUnused);
    ok(stubMsg->fUnused2 == 0xffffcccc, "%s: fUnused2 should have been 0xffffcccc instead of 0x%x\n",
       prefix, stubMsg->fUnused2);
    ok(stubMsg->dwDestContext == MSHCTX_DIFFERENTMACHINE,
       "%s: dwDestContext should have been MSHCTX_DIFFERENTMACHINE instead of %d\n",
       prefix, stubMsg->dwDestContext);
    TEST_ZERO(pvDestContext, "%p");
    TEST_POINTER_UNSET(SavedContextHandles);
    TEST_ULONG_UNSET(ParamNumber);
    TEST_ZERO(pRpcChannelBuffer, "%p");
    TEST_ZERO(pArrayInfo, "%p");
    TEST_POINTER_UNSET(SizePtrCountArray);
    TEST_POINTER_UNSET(SizePtrOffsetArray);
    TEST_POINTER_UNSET(SizePtrLengthArray);
    TEST_POINTER_UNSET(pArgQueue);
    TEST_ZERO(dwStubPhase, "%d");
    /* FIXME: where does this value come from? */
    trace("%s: LowStackMark is %p\n", prefix, stubMsg->LowStackMark);
    ok( stubMsg->pAsyncMsg == 0 || broken(stubMsg->pAsyncMsg == unset_ptr), /* win9x, nt4 */
        "%s: pAsyncMsg should have been set to zero instead of %p\n", prefix, stubMsg->pAsyncMsg );
    ok( stubMsg->pCorrInfo == 0 || broken(stubMsg->pCorrInfo == unset_ptr), /* win9x, nt4 */
        "%s: pCorrInfo should have been set to zero instead of %p\n", prefix, stubMsg->pCorrInfo );
    ok( stubMsg->pCorrMemory == 0 || broken(stubMsg->pCorrMemory == unset_ptr), /* win9x, nt4 */
        "%s: pCorrMemory should have been set to zero instead of %p\n", prefix, stubMsg->pCorrMemory );
    ok( stubMsg->pMemoryList == 0 || broken(stubMsg->pMemoryList == unset_ptr), /* win9x, nt4 */
        "%s: pMemoryList should have been set to zero instead of %p\n", prefix, stubMsg->pMemoryList );
    TEST_POINTER_UNSET(pCSInfo);
    TEST_POINTER_UNSET(ConformanceMark);
    TEST_POINTER_UNSET(VarianceMark);
    ok(stubMsg->Unused == (ULONG_PTR)unset_ptr, "%s: Unused should have be unset instead of 0x%lx\n",
       prefix, stubMsg->Unused);
    TEST_POINTER_UNSET(pContext);
    TEST_POINTER_UNSET(ContextHandleHash);
    TEST_POINTER_UNSET(pUserMarshalList);
    TEST_ULONG_PTR_UNSET(Reserved51_3);
    TEST_ULONG_PTR_UNSET(Reserved51_4);
    TEST_ULONG_PTR_UNSET(Reserved51_5);

#undef TEST_ULONG_PTR_UNSET
#undef TEST_ULONG_UNSET
#undef TEST_POINTER_UNSET
#undef TEST_ZERO
}

static void test_client_init(void)
{
    MIDL_STUB_MESSAGE stubMsg;
    RPC_MESSAGE rpcMsg;
    void *unset_ptr;

    memset(&rpcMsg, 0xcc, sizeof(rpcMsg));
    memset(&stubMsg, 0xcc, sizeof(stubMsg));
    memset(&unset_ptr, 0xcc, sizeof(unset_ptr));

    NdrClientInitializeNew(&rpcMsg, &stubMsg, &Object_StubDesc, 1);

    test_common_stub_data( "NdrClientInitializeNew", &stubMsg );

    ok(stubMsg.RpcMsg == &rpcMsg, "stubMsg.RpcMsg should have been %p instead of %p\n", &rpcMsg, stubMsg.RpcMsg);
    ok(rpcMsg.Handle == NULL, "rpcMsg.Handle should have been NULL instead of %p\n", rpcMsg.Handle);
    ok(rpcMsg.Buffer == unset_ptr, "rpcMsg.Buffer should have been unset instead of %p\n",
       rpcMsg.Buffer);
    ok(rpcMsg.BufferLength == 0xcccccccc, "rpcMsg.BufferLength should have been unset instead of %d\n", rpcMsg.BufferLength);
    ok(rpcMsg.ProcNum == 0x8001, "rpcMsg.ProcNum should have been 0x8001 instead of 0x%x\n", rpcMsg.ProcNum);
    ok(rpcMsg.TransferSyntax == unset_ptr, "rpcMsg.TransferSyntax should have been unset instead of %p\n", rpcMsg.TransferSyntax);
    ok(rpcMsg.RpcInterfaceInformation == Object_StubDesc.RpcInterfaceInformation,
        "rpcMsg.RpcInterfaceInformation should have been %p instead of %p\n",
        Object_StubDesc.RpcInterfaceInformation, rpcMsg.RpcInterfaceInformation);
    /* Note: ReservedForRuntime not tested */
    ok(rpcMsg.ManagerEpv == unset_ptr, "rpcMsg.ManagerEpv should have been unset instead of %p\n", rpcMsg.ManagerEpv);
    ok(rpcMsg.ImportContext == unset_ptr, "rpcMsg.ImportContext should have been unset instead of %p\n", rpcMsg.ImportContext);
    ok(rpcMsg.RpcFlags == 0, "rpcMsg.RpcFlags should have been 0 instead of 0x%x\n", rpcMsg.RpcFlags);

    ok(stubMsg.Buffer == unset_ptr, "stubMsg.Buffer should have been unset instead of %p\n",
       stubMsg.Buffer);
    ok(stubMsg.BufferStart == NULL, "stubMsg.BufferStart should have been NULL instead of %p\n",
       stubMsg.BufferStart);
    ok(stubMsg.BufferEnd == NULL, "stubMsg.BufferEnd should have been NULL instead of %p\n",
       stubMsg.BufferEnd);
    ok(stubMsg.BufferLength == 0, "stubMsg.BufferLength should have been 0 instead of %u\n",
       stubMsg.BufferLength);
    ok(stubMsg.IsClient == 1, "stubMsg.IsClient should have been 1 instead of %u\n", stubMsg.IsClient);
    ok(stubMsg.ReuseBuffer == 0, "stubMsg.ReuseBuffer should have been 0 instead of %d\n",
       stubMsg.ReuseBuffer);
    ok(stubMsg.CorrDespIncrement == 0, "stubMsg.CorrDespIncrement should have been 0 instead of %d\n",
       stubMsg.CorrDespIncrement);
    ok(stubMsg.FullPtrXlatTables == unset_ptr, "stubMsg.FullPtrXlatTables should have been unset instead of %p\n",
       stubMsg.FullPtrXlatTables);
}

static void test_server_init(void)
{
    MIDL_STUB_MESSAGE stubMsg;
    RPC_MESSAGE rpcMsg;
    unsigned char *ret;
    unsigned char buffer[256];

    memset(&rpcMsg, 0, sizeof(rpcMsg));
    rpcMsg.Buffer = buffer;
    rpcMsg.BufferLength = sizeof(buffer);
    rpcMsg.RpcFlags = RPC_BUFFER_COMPLETE;

    memset(&stubMsg, 0xcc, sizeof(stubMsg));

    ret = NdrServerInitializeNew(&rpcMsg, &stubMsg, &Object_StubDesc);
    ok(ret == NULL, "NdrServerInitializeNew should have returned NULL instead of %p\n", ret);

    test_common_stub_data( "NdrServerInitializeNew", &stubMsg );

    ok(stubMsg.RpcMsg == &rpcMsg, "stubMsg.RpcMsg should have been %p instead of %p\n", &rpcMsg, stubMsg.RpcMsg);
    ok(stubMsg.Buffer == buffer, "stubMsg.Buffer should have been %p instead of %p\n", buffer, stubMsg.Buffer);
    ok(stubMsg.BufferStart == buffer, "stubMsg.BufferStart should have been %p instead of %p\n", buffer, stubMsg.BufferStart);
    ok(stubMsg.BufferEnd == buffer + sizeof(buffer), "stubMsg.BufferEnd should have been %p instead of %p\n", buffer + sizeof(buffer), stubMsg.BufferEnd);
    todo_wine
    ok(stubMsg.BufferLength == 0, "stubMsg.BufferLength should have been 0 instead of %u\n", stubMsg.BufferLength);
    ok(stubMsg.IsClient == 0, "stubMsg.IsClient should have been 0 instead of %u\n", stubMsg.IsClient);
    ok(stubMsg.ReuseBuffer == 0 ||
       broken(stubMsg.ReuseBuffer == 1), /* win2k */
       "stubMsg.ReuseBuffer should have been set to zero instead of %d\n", stubMsg.ReuseBuffer);
    ok(stubMsg.CorrDespIncrement == 0 ||
       broken(stubMsg.CorrDespIncrement == 0xcc), /* <= Win 2003 */
       "CorrDespIncrement should have been set to zero instead of 0x%x\n", stubMsg.CorrDespIncrement);
    ok(stubMsg.FullPtrXlatTables == 0, "stubMsg.BufferLength should have been 0 instead of %p\n", stubMsg.FullPtrXlatTables);
}

static void test_ndr_allocate(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *p1, *p2;
    struct tag_mem_list_v2_t
    {
        DWORD magic;
        DWORD size;
        DWORD unknown;
        struct tag_mem_list_v2_t *next;
    } *mem_list_v2;
    const DWORD magic_MEML = 'M' << 24 | 'E' << 16 | 'M' << 8 | 'L';

    StubDesc = Object_StubDesc;
    NdrClientInitializeNew(&RpcMessage, &StubMsg, &StubDesc, 0);

    my_alloc_called = my_free_called = 0;
    p1 = NdrAllocate(&StubMsg, 10);
    p2 = NdrAllocate(&StubMsg, 24);
    ok(my_alloc_called == 2, "alloc called %d\n", my_alloc_called);
    ok(StubMsg.pMemoryList != NULL, "StubMsg.pMemoryList NULL\n");
    if(StubMsg.pMemoryList)
    {
        mem_list_v2 = StubMsg.pMemoryList;
        if (mem_list_v2->size == 24)
        {
            trace("v2 mem list format\n");
            ok((char *)mem_list_v2 == (char *)p2 + 24, "expected mem_list_v2 pointer %p, but got %p\n", (char *)p2 + 24, mem_list_v2);
            ok(mem_list_v2->magic == magic_MEML, "magic %08x\n", mem_list_v2->magic);
            ok(mem_list_v2->size == 24, "wrong size for p2 %d\n", mem_list_v2->size);
            ok(mem_list_v2->unknown == 0, "wrong unknown for p2 0x%x\n", mem_list_v2->unknown);
            ok(mem_list_v2->next != NULL, "next NULL\n");
            mem_list_v2 = mem_list_v2->next;
            if(mem_list_v2)
            {
                ok((char *)mem_list_v2 == (char *)p1 + 16, "expected mem_list_v2 pointer %p, but got %p\n", (char *)p1 + 16, mem_list_v2);
                ok(mem_list_v2->magic == magic_MEML, "magic %08x\n", mem_list_v2->magic);
                ok(mem_list_v2->size == 16, "wrong size for p1 %d\n", mem_list_v2->size);
                ok(mem_list_v2->unknown == 0, "wrong unknown for p1 0x%x\n", mem_list_v2->unknown);
                ok(mem_list_v2->next == NULL, "next %p\n", mem_list_v2->next);
            }
        }
        else win_skip("v1 mem list format\n");
    }
    /* NdrFree isn't exported so we can't test free'ing */
    my_free(p1);
    my_free(p2);
}

static void test_conformant_array(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *ptr;
    unsigned char *mem, *mem_orig;
    unsigned char memsrc[20];
    unsigned int i;

    static const unsigned char fmtstr_conf_array[] =
    {
        0x1b,              /* FC_CARRAY */
        0x0,               /* align */
        NdrFcShort( 0x1 ), /* elem size */
        0x40,              /* Corr desc:  const */
        0x0,
        NdrFcShort(0x10),  /* const = 0x10 */
        0x1,               /* FC_BYTE */
        0x5b               /* FC_END */
    };

    for (i = 0; i < sizeof(memsrc); i++)
        memsrc[i] = i * i;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_conf_array;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrConformantArrayBufferSize( &StubMsg,
                          memsrc,
                          fmtstr_conf_array );
    ok(StubMsg.BufferLength >= 20, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrConformantArrayMarshall( &StubMsg,  memsrc, fmtstr_conf_array );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(StubMsg.Buffer - StubMsg.BufferStart == 20, "Buffer %p Start %p len %d\n", StubMsg.Buffer, StubMsg.BufferStart, 20);
    ok(!memcmp(StubMsg.BufferStart + 4, memsrc, 16), "incorrectly marshaled\n");

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    /* passing mem == NULL with must_alloc == 0 crashes under Windows */
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 1);
    ok(mem != NULL, "mem not alloced\n");
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem_orig = mem;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 1);
    ok(mem != mem_orig, "mem not alloced\n");
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);

    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayFree( &StubMsg, mem, fmtstr_conf_array );
    ok(my_free_called == 0, "free called %d\n", my_free_called);
    StubMsg.pfnFree(mem);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 0);
    ok(mem == StubMsg.BufferStart + 4 || broken(!mem),  /* win9x, nt4 */
       "mem not pointing at buffer %p/%p\n", mem, StubMsg.BufferStart + 4);
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);
    my_alloc_called = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 1);
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    StubMsg.pfnFree(mem);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrConformantArrayUnmarshall( &StubMsg, &mem, fmtstr_conf_array, 1);
    ok(mem != StubMsg.BufferStart + 4, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    StubMsg.pfnFree(mem);
    StubMsg.pfnFree(mem_orig);

    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
}

static void test_conformant_string(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    DWORD size;
    void *ptr;
    unsigned char *mem, *mem_orig;
    char memsrc[] = "This is a test string";

    static const unsigned char fmtstr_conf_str[] =
    {
			0x11, 0x8,	/* FC_RP [simple_pointer] */
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
    };

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_conf_str;

    memset( &StubMsg, 0, sizeof(StubMsg) );  /* needed on win9x and nt4 */
    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrPointerBufferSize( &StubMsg,
                          (unsigned char *)memsrc,
                          fmtstr_conf_str );
    ok(StubMsg.BufferLength >= sizeof(memsrc) + 12, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrPointerMarshall( &StubMsg, (unsigned char *)memsrc, fmtstr_conf_str );
    ok(ptr == NULL, "ret %p\n", ptr);
    size = StubMsg.Buffer - StubMsg.BufferStart;
    ok(size == sizeof(memsrc) + 12, "Buffer %p Start %p len %d\n",
       StubMsg.Buffer, StubMsg.BufferStart, size);
    ok(!memcmp(StubMsg.BufferStart + 12, memsrc, sizeof(memsrc)), "incorrectly marshaled\n");

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(memsrc));
    /* Windows apparently checks string length on the output buffer to determine its size... */
    memset( mem, 'x', sizeof(memsrc) - 1 );
    mem[sizeof(memsrc) - 1] = 0;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 0);
    ok(mem == mem_orig, "mem not alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 1);
    ok(mem == mem_orig, "mem not alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    /* Prevent a memory leak when running with Wine.
       Remove once the todo_wine block above is fixed. */
    if (mem != mem_orig)
        HeapFree(GetProcessHeap(), 0, mem_orig);

    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, fmtstr_conf_str );
    ok(my_free_called == 1, "free called %d\n", my_free_called);

    mem = my_alloc(10);
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, fmtstr_conf_str );
    ok(my_free_called == 1, "free called %d\n", my_free_called);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 0);
    ok(mem == StubMsg.BufferStart + 12 || broken(!mem), /* win9x, nt4 */
       "mem not pointing at buffer %p/%p\n", mem, StubMsg.BufferStart + 12 );
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 1);
    ok(mem == StubMsg.BufferStart + 12 || broken(!mem), /* win9x, nt4 */
       "mem not pointing at buffer %p/%p\n", mem, StubMsg.BufferStart + 12 );
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(memsrc));
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 0);
    ok(mem == StubMsg.BufferStart + 12 || broken(!mem), /* win9x, nt4 */
       "mem not pointing at buffer %p/%p\n", mem, StubMsg.BufferStart + 12 );
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerUnmarshall( &StubMsg, &mem, fmtstr_conf_str, 1);
    ok(mem == StubMsg.BufferStart + 12 || broken(!mem), /* win9x, nt4 */
       "mem not pointing at buffer %p/%p\n", mem, StubMsg.BufferStart + 12 );
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    mem = my_alloc(10);
    my_free_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrPointerFree( &StubMsg, mem, fmtstr_conf_str );
    ok(my_free_called == 1, "free called %d\n", my_free_called);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
}

static void test_nonconformant_string(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    DWORD size;
    void *ptr;
    unsigned char *mem, *mem_orig;
    unsigned char memsrc[10] = "This is";
    unsigned char memsrc2[10] = "This is a";

    static const unsigned char fmtstr_nonconf_str[] =
    {
			0x26,		/* FC_CSTRING */
			0x5c,		/* FC_PAD */
			NdrFcShort( 0xa ),	/* 10 */
    };

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_nonconf_str;

    /* length < size */
    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;

    NdrNonConformantStringBufferSize( &StubMsg, memsrc, fmtstr_nonconf_str );
    ok(StubMsg.BufferLength >= strlen((char *)memsrc) + 1 + 8, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrNonConformantStringMarshall( &StubMsg, memsrc, fmtstr_nonconf_str );
    ok(ptr == NULL, "ret %p\n", ptr);
    size = StubMsg.Buffer - StubMsg.BufferStart;
    ok(size == strlen((char *)memsrc) + 1 + 8, "Buffer %p Start %p len %d\n",
       StubMsg.Buffer, StubMsg.BufferStart, size);
    ok(!memcmp(StubMsg.BufferStart + 8, memsrc, strlen((char *)memsrc) + 1), "incorrectly marshaled\n");

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(memsrc));
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 1);
    todo_wine
    ok(mem == mem_orig, "mem alloced\n");
    todo_wine
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem != mem_orig, "mem not alloced\n");
    ok(mem != StubMsg.BufferStart + 8, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    NdrOleFree(mem);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 1);
    todo_wine
    ok(mem == mem_orig, "mem alloced\n");
    todo_wine
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);

    /* length = size */
    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;

    NdrNonConformantStringBufferSize( &StubMsg, memsrc2, fmtstr_nonconf_str );
    ok(StubMsg.BufferLength >= strlen((char *)memsrc2) + 1 + 8, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrNonConformantStringMarshall( &StubMsg, memsrc2, fmtstr_nonconf_str );
    ok(ptr == NULL, "ret %p\n", ptr);
    size = StubMsg.Buffer - StubMsg.BufferStart;
    ok(size == strlen((char *)memsrc2) + 1 + 8, "Buffer %p Start %p len %d\n",
       StubMsg.Buffer, StubMsg.BufferStart, size);
    ok(!memcmp(StubMsg.BufferStart + 8, memsrc2, strlen((char *)memsrc2) + 1), "incorrectly marshaled\n");

    StubMsg.Buffer = StubMsg.BufferStart;
    StubMsg.MemorySize = 0;
    mem = NULL;

    /* Client */
    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    mem = mem_orig = HeapAlloc(GetProcessHeap(), 0, sizeof(memsrc));
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 1);
    todo_wine
    ok(mem == mem_orig, "mem alloced\n");
    todo_wine
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem != mem_orig, "mem not alloced\n");
    ok(mem != StubMsg.BufferStart + 8, "mem pointing at buffer\n");
    ok(my_alloc_called == 1, "alloc called %d\n", my_alloc_called);
    NdrOleFree(mem);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 0);
    ok(mem == mem_orig, "mem alloced\n");
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    my_alloc_called = 0;
    mem = mem_orig;
    StubMsg.Buffer = StubMsg.BufferStart;
    NdrNonConformantStringUnmarshall( &StubMsg, &mem, fmtstr_nonconf_str, 1);
    todo_wine
    ok(mem == mem_orig, "mem alloced\n");
    todo_wine
    ok(my_alloc_called == 0, "alloc called %d\n", my_alloc_called);

    HeapFree(GetProcessHeap(), 0, mem_orig);
    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
}

static void test_conf_complex_struct(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *ptr;
    unsigned int i;
    struct conf_complex
    {
      unsigned int size;
      unsigned int *array[1];
    };
    struct conf_complex *memsrc;
    struct conf_complex *mem;

    static const unsigned char fmtstr_complex_struct[] =
    {
/* 0 */
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 2 */	NdrFcShort( 0x4 ),	/* 4 */
/* 4 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 6 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 8 */
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 10 */
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 12 */	NdrFcShort( 0x4 ),	/* 4 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x1 ),	/* 1 */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 24 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 26 */
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 28 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 30 */
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 32 */	NdrFcShort( 0x4 ),	/* 4 */
/* 34 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (0) */
/* 36 */	NdrFcShort( 0x0 ),	/* Offset= 0 (36) */
/* 38 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
    };

    memsrc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                       FIELD_OFFSET(struct conf_complex, array[20]));
    memsrc->size = 20;

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_complex_struct;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;
    NdrComplexStructBufferSize( &StubMsg,
                                (unsigned char *)memsrc,
                                &fmtstr_complex_struct[30] );
    ok(StubMsg.BufferLength >= 28, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

    ptr = NdrComplexStructMarshall( &StubMsg, (unsigned char *)memsrc,
                                    &fmtstr_complex_struct[30] );
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(*(unsigned int *)StubMsg.BufferStart == 20, "Conformance should have been 20 instead of %d\n", *(unsigned int *)StubMsg.BufferStart);
    ok(*(unsigned int *)(StubMsg.BufferStart + 4) == 20, "conf_complex.size should have been 20 instead of %d\n", *(unsigned int *)(StubMsg.BufferStart + 4));
    for (i = 0; i < 20; i++)
      ok(*(unsigned int *)(StubMsg.BufferStart + 8 + i * 4) == 0, "pointer id for conf_complex.array[%d] should have been 0 instead of 0x%x\n", i, *(unsigned int *)(StubMsg.BufferStart + 8 + i * 4));

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
    ptr = NdrComplexStructUnmarshall( &StubMsg, (unsigned char **)&mem, &fmtstr_complex_struct[30], 0);
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem->size == 20, "mem->size wasn't unmarshalled correctly (%d)\n", mem->size);
    ok(mem->array[0] == NULL, "mem->array[0] wasn't unmarshalled correctly (%p)\n", mem->array[0]);
    StubMsg.pfnFree(mem);

    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);
    HeapFree(GetProcessHeap(), 0, memsrc);
}


static void test_conf_complex_array(void)
{
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc;
    void *ptr;
    unsigned int i, j;
    struct conf_complex
    {
        unsigned int dim1, dim2;
        DWORD **array;
    };
    struct conf_complex memsrc;
    struct conf_complex *mem;
    DWORD *buf, expected_length;

    static const unsigned char fmtstr_complex_array[] =
    {

/*  0 */        0x21,           /* FC_BOGUS_ARRAY */
                0x3,            /* 3 */
/*  2 */        NdrFcShort( 0x0 ),      /* 0 */
/*  4 */        0x19, 0x0,      /* Corr desc:  field pointer, FC_ULONG */
/*  6 */        NdrFcShort( 0x4 ),      /* 4 */
/*  8 */        NdrFcLong( 0xffffffff ),        /* -1 */
/* 12 */        0x8,            /* FC_LONG */
                0x5b,           /* FC_END */
/* 14 */
                0x21,           /* FC_BOGUS_ARRAY */
                0x3,            /* 3 */
/* 16 */        NdrFcShort( 0x0 ),      /* 0 */
/* 18 */        0x19,           /* Corr desc:  field pointer, FC_ULONG */
                0x0,            /*  */
/* 20 */        NdrFcShort( 0x0 ),      /* 0 */
/* 22 */        NdrFcLong( 0xffffffff ),        /* -1 */
/* 26 */        0x12, 0x0,      /* FC_UP */
/* 28 */        NdrFcShort( 0xffe4 ),   /* Offset= -28 (0) */
/* 30 */        0x5c,           /* FC_PAD */
                0x5b,           /* FC_END */

#ifdef _WIN64
/* 32 */        0x1a,           /* FC_BOGUS_STRUCT */
                0x3,            /* 3 */
/* 34 */        NdrFcShort( 0x10 ),     /* 16 */
/* 36 */        NdrFcShort( 0x0 ),      /* 0 */
/* 38 */        NdrFcShort( 0x6 ),      /* Offset= 6 (44) */
/* 40 */        0x8,            /* FC_LONG */
                0x8,            /* FC_LONG */
/* 42 */        0x36,           /* FC_POINTER */
                0x5b,           /* FC_END */
/* 44 */
                0x12, 0x0,      /* FC_UP */
/* 46 */        NdrFcShort( 0xffe0 ),   /* Offset= -32 (14) */
#else
/* 32 */
                0x16,           /* FC_PSTRUCT */
                0x3,            /* 3 */
/* 34 */        NdrFcShort( 0xc ),      /* 12 */
/* 36 */        0x4b,           /* FC_PP */
                0x5c,           /* FC_PAD */
/* 38 */        0x46,           /* FC_NO_REPEAT */
                0x5c,           /* FC_PAD */
/* 40 */        NdrFcShort( 0x8 ),      /* 8 */
/* 42 */        NdrFcShort( 0x8 ),      /* 8 */
/* 44 */        0x12, 0x0,      /* FC_UP */
/* 46 */        NdrFcShort( 0xffe0 ),   /* Offset= -32 (14) */
/* 48 */        0x5b,           /* FC_END */
                0x8,            /* FC_LONG */
/* 50 */        0x8,            /* FC_LONG */
                0x8,            /* FC_LONG */
/* 52 */        0x5c,           /* FC_PAD */
                0x5b,           /* FC_END */
#endif
    };

    memsrc.dim1 = 5;
    memsrc.dim2 = 3;

    memsrc.array = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, memsrc.dim1 * sizeof(DWORD*));

    for(i = 0; i < memsrc.dim1; i++)
    {
        memsrc.array[i] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, memsrc.dim2 * sizeof(DWORD));
        for(j = 0; j < memsrc.dim2; j++)
            memsrc.array[i][j] = i * memsrc.dim2 + j;
    }

    StubDesc = Object_StubDesc;
    StubDesc.pFormatTypes = fmtstr_complex_array;

    NdrClientInitializeNew(
                           &RpcMessage,
                           &StubMsg,
                           &StubDesc,
                           0);

    StubMsg.BufferLength = 0;

#ifdef _WIN64
    NdrComplexStructBufferSize( &StubMsg,
                                (unsigned char *)&memsrc,
                                &fmtstr_complex_array[32] );
#else
    NdrSimpleStructBufferSize( &StubMsg,
                                (unsigned char *)&memsrc,
                                &fmtstr_complex_array[32] );
#endif

    expected_length = (4 + memsrc.dim1 * (2 + memsrc.dim2)) * 4;
    ok(StubMsg.BufferLength >= expected_length, "length %d\n", StubMsg.BufferLength);

    /*NdrGetBuffer(&_StubMsg, _StubMsg.BufferLength, NULL);*/
    StubMsg.RpcMsg->Buffer = StubMsg.BufferStart = StubMsg.Buffer = HeapAlloc(GetProcessHeap(), 0, StubMsg.BufferLength);
    StubMsg.BufferEnd = StubMsg.BufferStart + StubMsg.BufferLength;

#ifdef _WIN64
    ptr = NdrComplexStructMarshall( &StubMsg, (unsigned char *)&memsrc,
                                    &fmtstr_complex_array[32] );
#else
    ptr = NdrSimpleStructMarshall( &StubMsg, (unsigned char *)&memsrc,
                                    &fmtstr_complex_array[32] );
#endif

    ok(ptr == NULL, "ret %p\n", ptr);
    ok((char*)StubMsg.Buffer == (char*)StubMsg.BufferStart + expected_length, "not at expected length\n");

    buf = (DWORD *)StubMsg.BufferStart;

    ok(*buf == memsrc.dim1, "dim1 should have been %d instead of %08x\n", memsrc.dim1, *buf);
    buf++;
    ok(*buf == memsrc.dim2, "dim2 should have been %d instead of %08x\n", memsrc.dim2, *buf);
    buf++;
    ok(*buf != 0, "pointer id should be non-zero\n");
    buf++;
    ok(*buf == memsrc.dim1, "Conformance should have been %d instead of %08x\n", memsrc.dim1, *buf);
    buf++;
    for(i = 0; i < memsrc.dim1; i++)
    {
        ok(*buf != 0, "pointer id[%d] should be non-zero\n", i);
        buf++;
    }
    for(i = 0; i < memsrc.dim1; i++)
    {
        ok(*buf == memsrc.dim2, "Conformance should have been %d instead of %08x\n", memsrc.dim2, *buf);
        buf++;
        for(j = 0; j < memsrc.dim2; j++)
        {
            ok(*buf == i * memsrc.dim2 + j, "got %08x\n", *buf);
            buf++;
        }
    }

    ok((void*)buf == StubMsg.Buffer, "not at end of buffer\n");

    /* Server */
    my_alloc_called = 0;
    StubMsg.IsClient = 0;
    mem = NULL;
    StubMsg.Buffer = StubMsg.BufferStart;
#ifdef _WIN64
    ptr = NdrComplexStructUnmarshall( &StubMsg, (unsigned char **)&mem, &fmtstr_complex_array[32], 0);
#else
    ptr = NdrSimpleStructUnmarshall( &StubMsg, (unsigned char **)&mem, &fmtstr_complex_array[32], 0);
#endif
    ok(ptr == NULL, "ret %p\n", ptr);
    ok(mem->dim1 == memsrc.dim1, "mem->dim1 wasn't unmarshalled correctly (%d)\n", mem->dim1);
    ok(mem->dim2 == memsrc.dim2, "mem->dim2 wasn't unmarshalled correctly (%d)\n", mem->dim2);
    ok(mem->array[1][0] == memsrc.dim2, "mem->array[1][0] wasn't unmarshalled correctly (%d)\n", mem->array[1][0]);

    StubMsg.Buffer = StubMsg.BufferStart;
#ifdef _WIN64
    NdrComplexStructFree( &StubMsg, (unsigned char*)mem, &fmtstr_complex_array[32]);
#else
    NdrSimpleStructFree( &StubMsg, (unsigned char*)mem, &fmtstr_complex_array[32]);
#endif

    HeapFree(GetProcessHeap(), 0, StubMsg.RpcMsg->Buffer);

    for(i = 0; i < memsrc.dim1; i++)
        HeapFree(GetProcessHeap(), 0, memsrc.array[i]);
    HeapFree(GetProcessHeap(), 0, memsrc.array);
}

static void test_ndr_buffer(void)
{
    static unsigned char ncalrpc[] = "ncalrpc";
    static unsigned char endpoint[] = "winetest:test_ndr_buffer";
    RPC_MESSAGE RpcMessage;
    MIDL_STUB_MESSAGE StubMsg;
    MIDL_STUB_DESC StubDesc = Object_StubDesc;
    unsigned char *ret;
    unsigned char *binding;
    RPC_BINDING_HANDLE Handle;
    RPC_STATUS status;
    ULONG prev_buffer_length;
    BOOL old_buffer_valid_location;

    StubDesc.RpcInterfaceInformation = (void *)&IFoo___RpcServerInterface;

    status = RpcServerUseProtseqEpA(ncalrpc, 20, endpoint, NULL);
    ok(RPC_S_OK == status, "RpcServerUseProtseqEp failed with status %u\n", status);
    status = RpcServerRegisterIf(IFoo_v0_0_s_ifspec, NULL, NULL);
    ok(RPC_S_OK == status, "RpcServerRegisterIf failed with status %u\n", status);
    status = RpcServerListen(1, 20, TRUE);
    ok(RPC_S_OK == status, "RpcServerListen failed with status %u\n", status);
    if (status != RPC_S_OK)
    {
        /* Failed to create a server, running client tests is useless */
        return;
    }

    status = RpcStringBindingComposeA(NULL, ncalrpc, NULL, endpoint, NULL, &binding);
    ok(status == RPC_S_OK, "RpcStringBindingCompose failed (%u)\n", status);

    status = RpcBindingFromStringBindingA(binding, &Handle);
    ok(status == RPC_S_OK, "RpcBindingFromStringBinding failed (%u)\n", status);
    RpcStringFreeA(&binding);

    NdrClientInitializeNew(&RpcMessage, &StubMsg, &StubDesc, 5);

    ret = NdrGetBuffer(&StubMsg, 10, Handle);
    ok(ret == StubMsg.Buffer, "NdrGetBuffer should have returned the same value as StubMsg.Buffer instead of %p\n", ret);
    ok(RpcMessage.Handle != NULL, "RpcMessage.Handle should not have been NULL\n");
    ok(RpcMessage.Buffer != NULL, "RpcMessage.Buffer should not have been NULL\n");
    ok(RpcMessage.BufferLength == 10 ||
       broken(RpcMessage.BufferLength == 12), /* win2k */
       "RpcMessage.BufferLength should have been 10 instead of %d\n", RpcMessage.BufferLength);
    ok(RpcMessage.RpcFlags == 0, "RpcMessage.RpcFlags should have been 0x0 instead of 0x%x\n", RpcMessage.RpcFlags);
    ok(StubMsg.Buffer != NULL, "Buffer should not have been NULL\n");
    ok(!StubMsg.BufferStart, "BufferStart should have been NULL instead of %p\n", StubMsg.BufferStart);
    ok(!StubMsg.BufferEnd, "BufferEnd should have been NULL instead of %p\n", StubMsg.BufferEnd);
    todo_wine
    ok(StubMsg.BufferLength == 0, "BufferLength should have left as 0 instead of being set to %d\n", StubMsg.BufferLength);
    old_buffer_valid_location = !StubMsg.fBufferValid;
    if (old_buffer_valid_location)
        ok(broken(StubMsg.CorrDespIncrement == TRUE), "fBufferValid should have been TRUE instead of 0x%x\n", StubMsg.CorrDespIncrement);
    else
        ok(StubMsg.fBufferValid, "fBufferValid should have been non-zero instead of 0x%x\n", StubMsg.fBufferValid);

    prev_buffer_length = RpcMessage.BufferLength;
    StubMsg.BufferLength = 1;
    NdrFreeBuffer(&StubMsg);
    ok(RpcMessage.Handle != NULL, "RpcMessage.Handle should not have been NULL\n");
    ok(RpcMessage.Buffer != NULL, "RpcMessage.Buffer should not have been NULL\n");
    ok(RpcMessage.BufferLength == prev_buffer_length, "RpcMessage.BufferLength should have been left as %d instead of %d\n", prev_buffer_length, RpcMessage.BufferLength);
    ok(StubMsg.Buffer != NULL, "Buffer should not have been NULL\n");
    ok(StubMsg.BufferLength == 1, "BufferLength should have left as 1 instead of being set to %d\n", StubMsg.BufferLength);
    if (old_buffer_valid_location)
        ok(broken(StubMsg.CorrDespIncrement == FALSE), "fBufferValid should have been FALSE instead of 0x%x\n", StubMsg.CorrDespIncrement);
    else
        ok(!StubMsg.fBufferValid, "fBufferValid should have been FALSE instead of %d\n", StubMsg.fBufferValid);

    /* attempt double-free */
    NdrFreeBuffer(&StubMsg);

    RpcBindingFree(&Handle);

    status = RpcServerUnregisterIf(NULL, NULL, FALSE);
    ok(status == RPC_S_OK, "RpcServerUnregisterIf failed (%u)\n", status);
}

static void test_NdrMapCommAndFaultStatus(void)
{
    RPC_STATUS rpc_status;
    MIDL_STUB_MESSAGE StubMsg;
    RPC_MESSAGE RpcMessage;

    NdrClientInitializeNew(&RpcMessage, &StubMsg, &Object_StubDesc, 5);

    for (rpc_status = 0; rpc_status < 10000; rpc_status++)
    {
        RPC_STATUS status;
        ULONG comm_status = 0;
        ULONG fault_status = 0;
        ULONG expected_comm_status = 0;
        ULONG expected_fault_status = 0;
        status = NdrMapCommAndFaultStatus(&StubMsg, &comm_status, &fault_status, rpc_status);
        ok(status == RPC_S_OK, "NdrMapCommAndFaultStatus failed with error %d\n", status);
        switch (rpc_status)
        {
        case ERROR_INVALID_HANDLE:
        case RPC_S_INVALID_BINDING:
        case RPC_S_UNKNOWN_IF:
        case RPC_S_SERVER_UNAVAILABLE:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_CALL_FAILED_DNE:
        case RPC_S_PROTOCOL_ERROR:
        case RPC_S_UNSUPPORTED_TRANS_SYN:
        case RPC_S_UNSUPPORTED_TYPE:
        case RPC_S_PROCNUM_OUT_OF_RANGE:
        case EPT_S_NOT_REGISTERED:
        case RPC_S_COMM_FAILURE:
            expected_comm_status = rpc_status;
            break;
        default:
            expected_fault_status = rpc_status;
        }
        ok(comm_status == expected_comm_status, "NdrMapCommAndFaultStatus should have mapped %d to comm status %d instead of %d\n",
            rpc_status, expected_comm_status, comm_status);
        ok(fault_status == expected_fault_status, "NdrMapCommAndFaultStatus should have mapped %d to fault status %d instead of %d\n",
            rpc_status, expected_fault_status, fault_status);
    }
}

static void test_NdrGetUserMarshalInfo(void)
{
    RPC_STATUS status;
    MIDL_STUB_MESSAGE stubmsg;
    USER_MARSHAL_CB umcb;
    NDR_USER_MARSHAL_INFO umi;
    unsigned char buffer[16];
    void *rpc_channel_buffer = (void *)(ULONG_PTR)0xcafebabe;
    RPC_MESSAGE rpc_msg;

    /* unmarshall */

    memset(&rpc_msg, 0xcc, sizeof(rpc_msg));
    rpc_msg.Buffer = buffer;
    rpc_msg.BufferLength = 16;

    memset(&stubmsg, 0xcc, sizeof(stubmsg));
    stubmsg.RpcMsg = &rpc_msg;
    stubmsg.dwDestContext = MSHCTX_INPROC;
    stubmsg.pvDestContext = NULL;
    stubmsg.Buffer = buffer + 15;
    stubmsg.BufferLength = 0;
    stubmsg.BufferEnd = NULL;
    stubmsg.pRpcChannelBuffer = rpc_channel_buffer;
    stubmsg.StubDesc = NULL;
    stubmsg.pfnAllocate = my_alloc;
    stubmsg.pfnFree = my_free;

    memset(&umcb, 0xcc, sizeof(umcb));
    umcb.Flags = MAKELONG(MSHCTX_INPROC, NDR_LOCAL_DATA_REPRESENTATION);
    umcb.pStubMsg = &stubmsg;
    umcb.Signature = USER_MARSHAL_CB_SIGNATURE;
    umcb.CBType = USER_MARSHAL_CB_UNMARSHALL;

    memset(&umi, 0xaa, sizeof(umi));

    status = NdrGetUserMarshalInfo(&umcb.Flags, 1, &umi);
    ok(status == RPC_S_OK, "NdrGetUserMarshalInfo failed with error %d\n", status);
    ok( umi.InformationLevel == 1,
       "umi.InformationLevel was %u instead of 1\n",
        umi.InformationLevel);
    ok( U1(umi).Level1.Buffer == buffer + 15,
       "umi.Level1.Buffer was %p instead of %p\n",
        U1(umi).Level1.Buffer, buffer);
    ok( U1(umi).Level1.BufferSize == 1,
       "umi.Level1.BufferSize was %u instead of 1\n",
        U1(umi).Level1.BufferSize);
    ok( U1(umi).Level1.pfnAllocate == my_alloc,
       "umi.Level1.pfnAllocate was %p instead of %p\n",
        U1(umi).Level1.pfnAllocate, my_alloc);
    ok( U1(umi).Level1.pfnFree == my_free,
       "umi.Level1.pfnFree was %p instead of %p\n",
        U1(umi).Level1.pfnFree, my_free);
    ok( U1(umi).Level1.pRpcChannelBuffer == rpc_channel_buffer,
       "umi.Level1.pRpcChannelBuffer was %p instead of %p\n",
        U1(umi).Level1.pRpcChannelBuffer, rpc_channel_buffer);

    /* buffer size */

    rpc_msg.Buffer = buffer;
    rpc_msg.BufferLength = 16;

    stubmsg.Buffer = buffer;
    stubmsg.BufferLength = 16;
    stubmsg.BufferEnd = NULL;

    umcb.CBType = USER_MARSHAL_CB_BUFFER_SIZE;

    memset(&umi, 0xaa, sizeof(umi));

    status = NdrGetUserMarshalInfo(&umcb.Flags, 1, &umi);
    ok(status == RPC_S_OK, "NdrGetUserMarshalInfo failed with error %d\n", status);
    ok( umi.InformationLevel == 1,
       "umi.InformationLevel was %u instead of 1\n",
        umi.InformationLevel);
    ok( U1(umi).Level1.Buffer == NULL,
       "umi.Level1.Buffer was %p instead of NULL\n",
        U1(umi).Level1.Buffer);
    ok( U1(umi).Level1.BufferSize == 0,
       "umi.Level1.BufferSize was %u instead of 0\n",
        U1(umi).Level1.BufferSize);
    ok( U1(umi).Level1.pfnAllocate == my_alloc,
       "umi.Level1.pfnAllocate was %p instead of %p\n",
        U1(umi).Level1.pfnAllocate, my_alloc);
    ok( U1(umi).Level1.pfnFree == my_free,
       "umi.Level1.pfnFree was %p instead of %p\n",
        U1(umi).Level1.pfnFree, my_free);
    ok( U1(umi).Level1.pRpcChannelBuffer == rpc_channel_buffer,
       "umi.Level1.pRpcChannelBuffer was %p instead of %p\n",
        U1(umi).Level1.pRpcChannelBuffer, rpc_channel_buffer);

    /* marshall */

    rpc_msg.Buffer = buffer;
    rpc_msg.BufferLength = 16;

    stubmsg.Buffer = buffer + 15;
    stubmsg.BufferLength = 0;
    stubmsg.BufferEnd = NULL;

    umcb.CBType = USER_MARSHAL_CB_MARSHALL;

    memset(&umi, 0xaa, sizeof(umi));

    status = NdrGetUserMarshalInfo(&umcb.Flags, 1, &umi);
    ok(status == RPC_S_OK, "NdrGetUserMarshalInfo failed with error %d\n", status);
    ok( umi.InformationLevel == 1,
       "umi.InformationLevel was %u instead of 1\n",
        umi.InformationLevel);
    ok( U1(umi).Level1.Buffer == buffer + 15,
       "umi.Level1.Buffer was %p instead of %p\n",
        U1(umi).Level1.Buffer, buffer);
    ok( U1(umi).Level1.BufferSize == 1,
       "umi.Level1.BufferSize was %u instead of 1\n",
        U1(umi).Level1.BufferSize);
    ok( U1(umi).Level1.pfnAllocate == my_alloc,
       "umi.Level1.pfnAllocate was %p instead of %p\n",
        U1(umi).Level1.pfnAllocate, my_alloc);
    ok( U1(umi).Level1.pfnFree == my_free,
       "umi.Level1.pfnFree was %p instead of %p\n",
        U1(umi).Level1.pfnFree, my_free);
    ok( U1(umi).Level1.pRpcChannelBuffer == rpc_channel_buffer,
       "umi.Level1.pRpcChannelBuffer was %p instead of %p\n",
        U1(umi).Level1.pRpcChannelBuffer, rpc_channel_buffer);

    /* free */

    rpc_msg.Buffer = buffer;
    rpc_msg.BufferLength = 16;

    stubmsg.Buffer = buffer;
    stubmsg.BufferLength = 16;
    stubmsg.BufferEnd = NULL;

    umcb.CBType = USER_MARSHAL_CB_FREE;

    memset(&umi, 0xaa, sizeof(umi));

    status = NdrGetUserMarshalInfo(&umcb.Flags, 1, &umi);
    ok(status == RPC_S_OK, "NdrGetUserMarshalInfo failed with error %d\n", status);
    ok( umi.InformationLevel == 1,
       "umi.InformationLevel was %u instead of 1\n",
        umi.InformationLevel);
    ok( U1(umi).Level1.Buffer == NULL,
       "umi.Level1.Buffer was %p instead of NULL\n",
        U1(umi).Level1.Buffer);
    ok( U1(umi).Level1.BufferSize == 0,
       "umi.Level1.BufferSize was %u instead of 0\n",
        U1(umi).Level1.BufferSize);
    ok( U1(umi).Level1.pfnAllocate == my_alloc,
       "umi.Level1.pfnAllocate was %p instead of %p\n",
        U1(umi).Level1.pfnAllocate, my_alloc);
    ok( U1(umi).Level1.pfnFree == my_free,
       "umi.Level1.pfnFree was %p instead of %p\n",
        U1(umi).Level1.pfnFree, my_free);
    ok( U1(umi).Level1.pRpcChannelBuffer == rpc_channel_buffer,
       "umi.Level1.pRpcChannelBuffer was %p instead of %p\n",
        U1(umi).Level1.pRpcChannelBuffer, rpc_channel_buffer);

    /* boundary test */

    rpc_msg.Buffer = buffer;
    rpc_msg.BufferLength = 15;

    stubmsg.Buffer = buffer + 15;
    stubmsg.BufferLength = 0;
    stubmsg.BufferEnd = NULL;

    umcb.CBType = USER_MARSHAL_CB_MARSHALL;

    status = NdrGetUserMarshalInfo(&umcb.Flags, 1, &umi);
    ok(status == RPC_S_OK, "NdrGetUserMarshalInfo failed with error %d\n", status);
    ok( U1(umi).Level1.BufferSize == 0,
       "umi.Level1.BufferSize was %u instead of 0\n",
        U1(umi).Level1.BufferSize);

    /* error conditions */

    rpc_msg.BufferLength = 14;
    status = NdrGetUserMarshalInfo(&umcb.Flags, 1, &umi);
    ok(status == ERROR_INVALID_USER_BUFFER,
        "NdrGetUserMarshalInfo should have failed with ERROR_INVALID_USER_BUFFER instead of %d\n", status);

    rpc_msg.BufferLength = 15;
    status = NdrGetUserMarshalInfo(&umcb.Flags, 9999, &umi);
    ok(status == RPC_S_INVALID_ARG,
        "NdrGetUserMarshalInfo should have failed with RPC_S_INVALID_ARG instead of %d\n", status);

    umcb.CBType = 9999;
    status = NdrGetUserMarshalInfo(&umcb.Flags, 1, &umi);
    ok(status == RPC_S_OK, "NdrGetUserMarshalInfo failed with error %d\n", status);

    umcb.CBType = USER_MARSHAL_CB_MARSHALL;
    umcb.Signature = 0;
    status = NdrGetUserMarshalInfo(&umcb.Flags, 1, &umi);
    ok(status == RPC_S_INVALID_ARG,
        "NdrGetUserMarshalInfo should have failed with RPC_S_INVALID_ARG instead of %d\n", status);
}

static void test_MesEncodeFixedBufferHandleCreate(void)
{
    ULONG encoded_size;
    RPC_STATUS status;
    handle_t handle;
    char *buffer;

    status = MesEncodeFixedBufferHandleCreate(NULL, 0, NULL, NULL);
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);

    status = MesEncodeFixedBufferHandleCreate(NULL, 0, NULL, &handle);
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);

    status = MesEncodeFixedBufferHandleCreate((char*)0xdeadbeef, 0, NULL, &handle);
    ok(status == RPC_X_INVALID_BUFFER, "got %d\n", status);

    buffer = (void*)((0xdeadbeef + 7) & ~7);
    status = MesEncodeFixedBufferHandleCreate(buffer, 0, NULL, &handle);
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);

    status = MesEncodeFixedBufferHandleCreate(buffer, 0, &encoded_size, &handle);
    todo_wine
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);
if (status == RPC_S_OK) {
    MesHandleFree(handle);
}
    status = MesEncodeFixedBufferHandleCreate(buffer, 32, NULL, &handle);
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);

    status = MesEncodeFixedBufferHandleCreate(buffer, 32, &encoded_size, &handle);
    ok(status == RPC_S_OK, "got %d\n", status);

    status = MesBufferHandleReset(NULL, MES_DYNAMIC_BUFFER_HANDLE, MES_ENCODE,
        &buffer, 32, &encoded_size);
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);

    /* convert to dynamic buffer handle */
    status = MesBufferHandleReset(handle, MES_DYNAMIC_BUFFER_HANDLE, MES_ENCODE,
        &buffer, 32, &encoded_size);
    ok(status == RPC_S_OK, "got %d\n", status);

    status = MesBufferHandleReset(handle, MES_DYNAMIC_BUFFER_HANDLE, MES_ENCODE,
        NULL, 32, &encoded_size);
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);

    status = MesBufferHandleReset(handle, MES_DYNAMIC_BUFFER_HANDLE, MES_ENCODE,
        &buffer, 32, NULL);
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);

    /* invalid handle type */
    status = MesBufferHandleReset(handle, MES_DYNAMIC_BUFFER_HANDLE+1, MES_ENCODE,
        &buffer, 32, &encoded_size);
    ok(status == RPC_S_INVALID_ARG, "got %d\n", status);

    status = MesHandleFree(handle);
    ok(status == RPC_S_OK, "got %d\n", status);
}

static void test_NdrCorrelationInitialize(void)
{
    MIDL_STUB_MESSAGE stub_msg;
    BYTE buf[256];

    memset( &stub_msg, 0, sizeof(stub_msg) );
    memset( buf, 0, sizeof(buf) );

    NdrCorrelationInitialize( &stub_msg, buf, sizeof(buf), 0 );
    ok( stub_msg.CorrDespIncrement == 2 ||
        broken(stub_msg.CorrDespIncrement == 0), /* <= Win 2003 */
        "got %d\n", stub_msg.CorrDespIncrement );

    memset( &stub_msg, 0, sizeof(stub_msg) );
    memset( buf, 0, sizeof(buf) );

    stub_msg.CorrDespIncrement = 1;
    NdrCorrelationInitialize( &stub_msg, buf, sizeof(buf), 0 );
    ok( stub_msg.CorrDespIncrement == 1, "got %d\n", stub_msg.CorrDespIncrement );
}

START_TEST( ndr_marshall )
{
    determine_pointer_marshalling_style();

    test_ndr_simple_type();
    test_simple_types();
    test_nontrivial_pointer_types();
    test_simple_struct();
    test_struct_align();
    test_iface_ptr();
    test_fullpointer_xlat();
    test_client_init();
    test_server_init();
    test_ndr_allocate();
    test_conformant_array();
    test_conformant_string();
    test_nonconformant_string();
    test_conf_complex_struct();
    test_conf_complex_array();
    test_ndr_buffer();
    test_NdrMapCommAndFaultStatus();
    test_NdrGetUserMarshalInfo();
    test_MesEncodeFixedBufferHandleCreate();
    test_NdrCorrelationInitialize();
}
