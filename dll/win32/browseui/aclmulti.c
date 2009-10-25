/*
 *	Multisource AutoComplete list
 *
 *	Copyright 2007	Mikolaj Zalewski
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "shlguid.h"
#include "shlobj.h"

#include "wine/unicode.h"

#include "browseui.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

struct ACLMultiSublist {
    IUnknown *punk;
    IEnumString *pEnum;
    IACList *pACL;
};

typedef struct tagACLMulti {
    const IEnumStringVtbl *vtbl;
    const IACListVtbl *aclVtbl;
    const IObjMgrVtbl *objmgrVtbl;
    LONG refCount;
    INT nObjs;
    INT currObj;
    struct ACLMultiSublist *objs;
} ACLMulti;

static const IEnumStringVtbl ACLMultiVtbl;
static const IACListVtbl ACLMulti_ACListVtbl;
static const IObjMgrVtbl ACLMulti_ObjMgrVtbl;

static inline ACLMulti *impl_from_IACList(IACList *iface)
{
    return (ACLMulti *)((char *)iface - FIELD_OFFSET(ACLMulti, aclVtbl));
}

static inline ACLMulti *impl_from_IObjMgr(IObjMgr *iface)
{
    return (ACLMulti *)((char *)iface - FIELD_OFFSET(ACLMulti, objmgrVtbl));
}

static void release_obj(struct ACLMultiSublist *obj)
{
    IUnknown_Release(obj->punk);
    if (obj->pEnum)
        IEnumString_Release(obj->pEnum);
    if (obj->pACL)
        IACList_Release(obj->pACL);
}

HRESULT WINAPI ACLMulti_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ACLMulti *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = CoTaskMemAlloc(sizeof(ACLMulti));
    if (This == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(This, sizeof(*This));
    This->vtbl = &ACLMultiVtbl;
    This->aclVtbl = &ACLMulti_ACListVtbl;
    This->objmgrVtbl = &ACLMulti_ObjMgrVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    BROWSEUI_refCount++;
    return S_OK;
}

static void WINAPI ACLMulti_Destructor(ACLMulti *This)
{
    int i;
    TRACE("destroying %p\n", This);
    for (i = 0; i < This->nObjs; i++)
        release_obj(&This->objs[i]);
    CoTaskMemFree(This->objs);
    CoTaskMemFree(This);
    BROWSEUI_refCount--;
}

static HRESULT WINAPI ACLMulti_QueryInterface(IEnumString *iface, REFIID iid, LPVOID *ppvOut)
{
    ACLMulti *This = (ACLMulti *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IEnumString))
    {
        *ppvOut = This;
    }
    else if (IsEqualIID(iid, &IID_IACList))
    {
        *ppvOut = &This->aclVtbl;
    }
    else if (IsEqualIID(iid, &IID_IObjMgr))
    {
        *ppvOut = &This->objmgrVtbl;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ACLMulti_AddRef(IEnumString *iface)
{
    ACLMulti *This = (ACLMulti *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ACLMulti_Release(IEnumString *iface)
{
    ACLMulti *This = (ACLMulti *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ACLMulti_Destructor(This);
    return ret;
}

static HRESULT WINAPI ACLMulti_Append(IObjMgr *iface, IUnknown *obj)
{
    ACLMulti *This = impl_from_IObjMgr(iface);

    TRACE("(%p, %p)\n", This, obj);
    if (obj == NULL)
        return E_FAIL;

    This->objs = CoTaskMemRealloc(This->objs, sizeof(This->objs[0]) * (This->nObjs+1));
    This->objs[This->nObjs].punk = obj;
    IUnknown_AddRef(obj);
    if (FAILED(IUnknown_QueryInterface(obj, &IID_IEnumString, (LPVOID *)&This->objs[This->nObjs].pEnum)))
        This->objs[This->nObjs].pEnum = NULL;
    if (FAILED(IUnknown_QueryInterface(obj, &IID_IACList, (LPVOID *)&This->objs[This->nObjs].pACL)))
        This->objs[This->nObjs].pACL = NULL;
    This->nObjs++;
    return S_OK;
}

static HRESULT WINAPI ACLMulti_Remove(IObjMgr *iface, IUnknown *obj)
{
    ACLMulti *This = impl_from_IObjMgr(iface);
    int i;

    TRACE("(%p, %p)\n", This, obj);
    for (i = 0; i < This->nObjs; i++)
        if (This->objs[i].punk == obj)
        {
            release_obj(&This->objs[i]);
            memmove(&This->objs[i], &This->objs[i+1], (This->nObjs-i-1)*sizeof(struct ACLMultiSublist));
            This->nObjs--;
            This->objs = CoTaskMemRealloc(This->objs, sizeof(This->objs[0]) * This->nObjs);
            return S_OK;
        }

    return E_FAIL;
}

static HRESULT WINAPI ACLMulti_Next(IEnumString *iface, ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    ACLMulti *This = (ACLMulti *)iface;

    TRACE("(%p, %d, %p, %p)\n", iface, celt, rgelt, pceltFetched);
    while (This->currObj < This->nObjs)
    {
        if (This->objs[This->currObj].pEnum)
        {
            /* native browseui 6.0 also returns only one element */
            HRESULT ret = IEnumString_Next(This->objs[This->currObj].pEnum, 1, rgelt, pceltFetched);
            if (ret != S_FALSE)
                return ret;
        }
        This->currObj++;
    }

    if (pceltFetched)
        *pceltFetched = 0;
    *rgelt = NULL;
    return S_FALSE;
}

static HRESULT WINAPI ACLMulti_Reset(IEnumString *iface)
{
    ACLMulti *This = (ACLMulti *)iface;
    int i;

    This->currObj = 0;
    for (i = 0; i < This->nObjs; i++)
    {
        if (This->objs[i].pEnum)
            IEnumString_Reset(This->objs[i].pEnum);
    }
    return S_OK;
}

static HRESULT WINAPI ACLMulti_Skip(IEnumString *iface, ULONG celt)
{
    /* native browseui 6.0 returns this: */
    return E_NOTIMPL;
}

static HRESULT WINAPI ACLMulti_Clone(IEnumString *iface, IEnumString **ppOut)
{
    *ppOut = NULL;
    /* native browseui 6.0 returns this: */
    return E_OUTOFMEMORY;
}

static HRESULT WINAPI ACLMulti_Expand(IACList *iface, LPCWSTR wstr)
{
    ACLMulti *This = impl_from_IACList(iface);
    HRESULT res = S_OK;
    int i;

    for (i = 0; i < This->nObjs; i++)
    {
        if (!This->objs[i].pACL)
            continue;
        res = IACList_Expand(This->objs[i].pACL, wstr);
        if (res == S_OK)
            break;
    }
    return res;
}

static const IEnumStringVtbl ACLMultiVtbl =
{
    ACLMulti_QueryInterface,
    ACLMulti_AddRef,
    ACLMulti_Release,

    ACLMulti_Next,
    ACLMulti_Skip,
    ACLMulti_Reset,
    ACLMulti_Clone
};

static HRESULT WINAPI ACLMulti_IObjMgr_QueryInterface(IObjMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    ACLMulti *This = impl_from_IObjMgr(iface);
    return ACLMulti_QueryInterface((IEnumString *)This, iid, ppvOut);
}

static ULONG WINAPI ACLMulti_IObjMgr_AddRef(IObjMgr *iface)
{
    ACLMulti *This = impl_from_IObjMgr(iface);
    return ACLMulti_AddRef((IEnumString *)This);
}

static ULONG WINAPI ACLMulti_IObjMgr_Release(IObjMgr *iface)
{
    ACLMulti *This = impl_from_IObjMgr(iface);
    return ACLMulti_Release((IEnumString *)This);
}

static const IObjMgrVtbl ACLMulti_ObjMgrVtbl =
{
    ACLMulti_IObjMgr_QueryInterface,
    ACLMulti_IObjMgr_AddRef,
    ACLMulti_IObjMgr_Release,

    ACLMulti_Append,
    ACLMulti_Remove
};

static HRESULT WINAPI ACLMulti_IACList_QueryInterface(IACList *iface, REFIID iid, LPVOID *ppvOut)
{
    ACLMulti *This = impl_from_IACList(iface);
    return ACLMulti_QueryInterface((IEnumString *)This, iid, ppvOut);
}

static ULONG WINAPI ACLMulti_IACList_AddRef(IACList *iface)
{
    ACLMulti *This = impl_from_IACList(iface);
    return ACLMulti_AddRef((IEnumString *)This);
}

static ULONG WINAPI ACLMulti_IACList_Release(IACList *iface)
{
    ACLMulti *This = impl_from_IACList(iface);
    return ACLMulti_Release((IEnumString *)This);
}

static const IACListVtbl ACLMulti_ACListVtbl =
{
    ACLMulti_IACList_QueryInterface,
    ACLMulti_IACList_AddRef,
    ACLMulti_IACList_Release,

    ACLMulti_Expand
};
