/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "dispex.h"
#include "activscp.h"

#include "wine/unicode.h"
#include "wine/list.h"

typedef struct _script_ctx_t script_ctx_t;
typedef struct _exec_ctx_t exec_ctx_t;

typedef struct {
    EXCEPINFO ei;
    VARIANT var;
} jsexcept_t;

typedef struct DispatchEx {
    const IDispatchExVtbl  *lpIDispatchExVtbl;

    LONG ref;

    script_ctx_t *ctx;
} DispatchEx;

#define _IDispatchEx_(x) ((IDispatchEx*) &(x)->lpIDispatchExVtbl)

HRESULT create_dispex(script_ctx_t*,DispatchEx**);

struct _script_ctx_t {
    LONG ref;

    SCRIPTSTATE state;
    exec_ctx_t *exec_ctx;
    LCID lcid;

    DispatchEx *script_disp;
};

void script_release(script_ctx_t*);

static inline void script_addref(script_ctx_t *ctx)
{
    ctx->ref++;
}

HRESULT WINAPI JScriptFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**);

typedef struct {
    void **blocks;
    DWORD block_cnt;
    DWORD last_block;
    DWORD offset;
    struct list custom_blocks;
} jsheap_t;

void jsheap_init(jsheap_t*);
void *jsheap_alloc(jsheap_t*,DWORD);
void jsheap_clear(jsheap_t*);
void jsheap_free(jsheap_t*);

extern LONG module_ref;

static inline void lock_module(void)
{
    InterlockedIncrement(&module_ref);
}

static inline void unlock_module(void)
{
    InterlockedDecrement(&module_ref);
}

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))
