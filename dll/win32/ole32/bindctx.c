/*
 *	                      BindCtx implementation
 *
 *  Copyright 1999  Noomen Hamza
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
#include <string.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "objbase.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define  BINDCTX_FIRST_TABLE_SIZE 4

/* data structure of the BindCtx table elements */
typedef struct BindCtxObject{

    IUnknown*   pObj; /* point on a bound object */

    LPOLESTR  pkeyObj; /* key associated to this bound object */

    BYTE regType; /* registration type: 1 if RegisterObjectParam and 0 if RegisterObjectBound */

} BindCtxObject;

/* BindCtx data structure */
typedef struct BindCtxImpl{

    IBindCtx IBindCtx_iface;

    LONG ref; /* reference counter for this object */

    BindCtxObject* bindCtxTable; /* this is a table in which all bounded objects are stored*/
    DWORD          bindCtxTableLastIndex;  /* first free index in the table */
    DWORD          bindCtxTableSize;   /* size table */

    BIND_OPTS3 options;

} BindCtxImpl;

/* IBindCtx prototype functions : */
static HRESULT WINAPI BindCtxImpl_ReleaseBoundObjects(IBindCtx*);
static HRESULT BindCtxImpl_GetObjectIndex(BindCtxImpl*, IUnknown*, LPOLESTR, DWORD *);
static HRESULT BindCtxImpl_ExpandTable(BindCtxImpl *);

static inline BindCtxImpl *impl_from_IBindCtx(IBindCtx *iface)
{
return CONTAINING_RECORD(iface, BindCtxImpl, IBindCtx_iface);
}

/*******************************************************************************
 *        BindCtx_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_QueryInterface(IBindCtx* iface,REFIID riid,void** ppvObject)
{
    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p %s %p)\n",This, debugstr_guid(riid), ppvObject);

    /* Perform a sanity check on the parameters.*/
    if (!ppvObject)
        return E_POINTER;

    /* Initialize the return parameter.*/
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IBindCtx, riid))
    {
        *ppvObject = &This->IBindCtx_iface;
        IBindCtx_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

/******************************************************************************
 *       BindCtx_AddRef
 ******************************************************************************/
static ULONG WINAPI BindCtxImpl_AddRef(IBindCtx* iface)
{
    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI BindCtxImpl_Release(IBindCtx* iface)
{
    BindCtxImpl *context = impl_from_IBindCtx(iface);
    ULONG refcount = InterlockedDecrement(&context->ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        BindCtxImpl_ReleaseBoundObjects(&context->IBindCtx_iface);
        heap_free(context->bindCtxTable);
        heap_free(context);
    }

    return refcount;
}


/******************************************************************************
 *        BindCtx_RegisterObjectBound
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_RegisterObjectBound(IBindCtx* iface,IUnknown* punk)
{
    BindCtxImpl *This = impl_from_IBindCtx(iface);
    DWORD lastIndex=This->bindCtxTableLastIndex;

    TRACE("(%p,%p)\n",This,punk);

    if (punk==NULL)
        return S_OK;

    if (lastIndex == This->bindCtxTableSize)
    {
        HRESULT hr = BindCtxImpl_ExpandTable(This);
        if (FAILED(hr))
            return hr;
    }

    IUnknown_AddRef(punk);

    /* put the object in the first free element in the table */
    This->bindCtxTable[lastIndex].pObj = punk;
    This->bindCtxTable[lastIndex].pkeyObj = NULL;
    This->bindCtxTable[lastIndex].regType = 0;
    lastIndex= ++This->bindCtxTableLastIndex;

    return S_OK;
}

/******************************************************************************
 *        BindCtx_RevokeObjectBound
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_RevokeObjectBound(IBindCtx* iface, IUnknown* punk)
{
    DWORD index,j;

    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p,%p)\n",This,punk);

    if (!punk)
        return E_INVALIDARG;

    /* check if the object was registered or not */
    if (BindCtxImpl_GetObjectIndex(This,punk,NULL,&index)==S_FALSE)
        return MK_E_NOTBOUND;

    if(This->bindCtxTable[index].pObj)
        IUnknown_Release(This->bindCtxTable[index].pObj);
    HeapFree(GetProcessHeap(),0,This->bindCtxTable[index].pkeyObj);
    
    /* left-shift all elements in the right side of the current revoked object */
    for(j=index; j<This->bindCtxTableLastIndex-1; j++)
        This->bindCtxTable[j]= This->bindCtxTable[j+1];

    This->bindCtxTableLastIndex--;

    return S_OK;
}

/******************************************************************************
 *        BindCtx_ReleaseBoundObjects
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_ReleaseBoundObjects(IBindCtx* iface)
{
    DWORD i;

    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p)\n",This);

    for(i=0;i<This->bindCtxTableLastIndex;i++)
    {
        if(This->bindCtxTable[i].pObj)
            IUnknown_Release(This->bindCtxTable[i].pObj);
        HeapFree(GetProcessHeap(),0,This->bindCtxTable[i].pkeyObj);
    }
    
    This->bindCtxTableLastIndex = 0;

    return S_OK;
}

/******************************************************************************
 *        BindCtx_SetBindOptions
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_SetBindOptions(IBindCtx* iface,BIND_OPTS *pbindopts)
{
    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p,%p)\n",This, pbindopts);

    if (pbindopts==NULL)
        return E_POINTER;

    if (pbindopts->cbStruct > sizeof(This->options))
    {
        WARN("invalid size %lu.\n", pbindopts->cbStruct);
        return E_INVALIDARG;
    }
    memcpy(&This->options, pbindopts, pbindopts->cbStruct);
    return S_OK;
}

/******************************************************************************
 *        BindCtx_GetBindOptions
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_GetBindOptions(IBindCtx* iface,BIND_OPTS *pbindopts)
{
    BindCtxImpl *This = impl_from_IBindCtx(iface);
    DWORD size;

    TRACE("(%p,%p)\n",This,pbindopts);

    if (pbindopts==NULL)
        return E_POINTER;

    size = min(pbindopts->cbStruct, sizeof(This->options));
    memcpy(pbindopts, &This->options, size);
    pbindopts->cbStruct = size;

    return S_OK;
}

/******************************************************************************
 *        BindCtx_GetRunningObjectTable
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_GetRunningObjectTable(IBindCtx* iface,IRunningObjectTable** pprot)
{
    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p,%p)\n",This,pprot);

    if (pprot==NULL)
        return E_POINTER;

    return GetRunningObjectTable(0, pprot);
}

/******************************************************************************
 *        BindCtx_RegisterObjectParam
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_RegisterObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown* punk)
{
    DWORD index=0;
    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_w(pszkey),punk);

    if (punk==NULL)
        return E_INVALIDARG;

    if (pszkey!=NULL && BindCtxImpl_GetObjectIndex(This,NULL,pszkey,&index)==S_OK)
    {
	TRACE("Overwriting existing key\n");
	if(This->bindCtxTable[index].pObj!=NULL)
	    IUnknown_Release(This->bindCtxTable[index].pObj);
	This->bindCtxTable[index].pObj=punk;
	IUnknown_AddRef(punk);
	return S_OK;
    }

    if (This->bindCtxTableLastIndex == This->bindCtxTableSize)
    {
        HRESULT hr = BindCtxImpl_ExpandTable(This);
        if (FAILED(hr))
            return hr;
    }

    This->bindCtxTable[This->bindCtxTableLastIndex].pObj = punk;
    This->bindCtxTable[This->bindCtxTableLastIndex].regType = 1;

    if (pszkey==NULL)

        This->bindCtxTable[This->bindCtxTableLastIndex].pkeyObj=NULL;

    else
    {

        This->bindCtxTable[This->bindCtxTableLastIndex].pkeyObj=
            HeapAlloc(GetProcessHeap(),0,(sizeof(WCHAR)*(1+lstrlenW(pszkey))));

        if (This->bindCtxTable[This->bindCtxTableLastIndex].pkeyObj==NULL)
            return E_OUTOFMEMORY;
        lstrcpyW(This->bindCtxTable[This->bindCtxTableLastIndex].pkeyObj,pszkey);
    }

    This->bindCtxTableLastIndex++;

    IUnknown_AddRef(punk);
    return S_OK;
}

/******************************************************************************
 *        BindCtx_GetObjectParam
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_GetObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown** punk)
{
    DWORD index;
    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_w(pszkey),punk);

    if (punk==NULL)
        return E_POINTER;

    *punk=0;

    if (BindCtxImpl_GetObjectIndex(This,NULL,pszkey,&index)==S_FALSE)
        return E_FAIL;

    IUnknown_AddRef(This->bindCtxTable[index].pObj);

    *punk = This->bindCtxTable[index].pObj;

    return S_OK;
}

/******************************************************************************
 *        BindCtx_RevokeObjectParam
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_RevokeObjectParam(IBindCtx* iface,LPOLESTR ppenum)
{
    DWORD index,j;

    BindCtxImpl *This = impl_from_IBindCtx(iface);

    TRACE("(%p,%s)\n",This,debugstr_w(ppenum));

    if (BindCtxImpl_GetObjectIndex(This,NULL,ppenum,&index)==S_FALSE)
        return E_FAIL;

    /* release the object if it's found */
    if(This->bindCtxTable[index].pObj)
        IUnknown_Release(This->bindCtxTable[index].pObj);
    HeapFree(GetProcessHeap(),0,This->bindCtxTable[index].pkeyObj);
    
    /* remove the object from the table with a left-shifting of all objects in the right side */
    for(j=index; j<This->bindCtxTableLastIndex-1; j++)
        This->bindCtxTable[j]= This->bindCtxTable[j+1];

    This->bindCtxTableLastIndex--;

    return S_OK;
}

/******************************************************************************
 *        BindCtx_EnumObjectParam
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_EnumObjectParam(IBindCtx* iface,IEnumString** pszkey)
{
    TRACE("(%p,%p)\n",iface,pszkey);

    *pszkey = NULL;

    /* not implemented in native either */
    return E_NOTIMPL;
}

/********************************************************************************
 *        GetObjectIndex (local function)
 ********************************************************************************/
static HRESULT BindCtxImpl_GetObjectIndex(BindCtxImpl* This,
                                          IUnknown* punk,
                                          LPOLESTR pszkey,
                                          DWORD *index)
{
    DWORD i;
    BOOL found = FALSE;

    TRACE("(%p,%p,%p,%p)\n",This,punk,pszkey,index);

    if (punk==NULL)
        /* search object identified by a register key */
        for(i=0; ( (i<This->bindCtxTableLastIndex) && (!found));i++)
        {
            if(This->bindCtxTable[i].regType==1){

                if ( ( (This->bindCtxTable[i].pkeyObj==NULL) && (pszkey==NULL) ) ||
                     ( (This->bindCtxTable[i].pkeyObj!=NULL) &&
                       (pszkey!=NULL) &&
                       (wcscmp(This->bindCtxTable[i].pkeyObj,pszkey)==0)
                     )
                   )

                    found = TRUE;
            }
        }
    else
        /* search object identified by a moniker*/
        for(i=0; ( (i<This->bindCtxTableLastIndex) && (!found));i++)
            if(This->bindCtxTable[i].pObj==punk)
                found = TRUE;

    if (index != NULL)
        *index=i-1;

    if (found)
        return S_OK;
    TRACE("key not found\n");
    return S_FALSE;
}

static HRESULT BindCtxImpl_ExpandTable(BindCtxImpl *This)
{
    if (!This->bindCtxTableSize)
    {
        This->bindCtxTableSize = BINDCTX_FIRST_TABLE_SIZE;
        This->bindCtxTable = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                                       This->bindCtxTableSize * sizeof(BindCtxObject));
    }
    else
    {
        This->bindCtxTableSize *= 2;

        This->bindCtxTable = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,This->bindCtxTable,
                                         This->bindCtxTableSize * sizeof(BindCtxObject));
    }

    if (!This->bindCtxTable)
        return E_OUTOFMEMORY;

    return S_OK;
}

static const IBindCtxVtbl VT_BindCtxImpl =
{
    BindCtxImpl_QueryInterface,
    BindCtxImpl_AddRef,
    BindCtxImpl_Release,
    BindCtxImpl_RegisterObjectBound,
    BindCtxImpl_RevokeObjectBound,
    BindCtxImpl_ReleaseBoundObjects,
    BindCtxImpl_SetBindOptions,
    BindCtxImpl_GetBindOptions,
    BindCtxImpl_GetRunningObjectTable,
    BindCtxImpl_RegisterObjectParam,
    BindCtxImpl_GetObjectParam,
    BindCtxImpl_EnumObjectParam,
    BindCtxImpl_RevokeObjectParam
};

/******************************************************************************
 *        CreateBindCtx (OLE32.@)
 */
HRESULT WINAPI CreateBindCtx(DWORD reserved, IBindCtx **bind_context)
{
    BindCtxImpl *object;

    TRACE("%#lx, %p.\n", reserved, bind_context);

    if (!bind_context) return E_INVALIDARG;

    *bind_context = NULL;

    if (reserved)
    {
        WARN("reserved should be 0, not %#lx.\n", reserved);
        return E_INVALIDARG;
    }

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IBindCtx_iface.lpVtbl = &VT_BindCtxImpl;
    object->ref = 1;
    object->options.cbStruct = sizeof(object->options);
    object->options.grfMode = STGM_READWRITE;
    object->options.dwClassContext = CLSCTX_SERVER;
    object->options.locale = GetThreadLocale();

    *bind_context = &object->IBindCtx_iface;

    return S_OK;
}

/******************************************************************************
 *              BindMoniker        [OLE32.@]
 *
 * Binds to a moniker.
 *
 * PARAMS
 *  pmk      [I] Moniker to bind to.
 *  grfOpt   [I] Reserved option flags. Set to 0.
 *  riid     [I] ID of the interface to bind to.
 *  pvResult [O] Address that receives the interface of the object that was bound to.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: Any HRESULT code.
 */
HRESULT WINAPI BindMoniker(LPMONIKER pmk, DWORD grfOpt, REFIID riid, LPVOID * ppvResult)
{
    HRESULT res;
    IBindCtx * pbc;

    TRACE("%p, %lx, %s, %p.\n", pmk, grfOpt, debugstr_guid(riid), ppvResult);

    res = CreateBindCtx(grfOpt, &pbc);
    if (SUCCEEDED(res))
    {
        res = IMoniker_BindToObject(pmk, pbc, NULL, riid, ppvResult);
        IBindCtx_Release(pbc);
    }
    return res;
}
