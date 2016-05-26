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

#include "precomp.h"

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

    BIND_OPTS2 bindOption2; /* a structure which contains the bind options*/

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

/******************************************************************************
 *        BindCtx_Destroy    (local function)
 *******************************************************************************/
static HRESULT BindCtxImpl_Destroy(BindCtxImpl* This)
{
    TRACE("(%p)\n",This);

    /* free the table space memory */
    HeapFree(GetProcessHeap(),0,This->bindCtxTable);

    /* free the bindctx structure */
    HeapFree(GetProcessHeap(),0,This);

    return S_OK;
}

/******************************************************************************
 *        BindCtx_Release
 ******************************************************************************/
static ULONG WINAPI BindCtxImpl_Release(IBindCtx* iface)
{
    BindCtxImpl *This = impl_from_IBindCtx(iface);
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        /* release all registered objects */
        BindCtxImpl_ReleaseBoundObjects(&This->IBindCtx_iface);

        BindCtxImpl_Destroy(This);
    }
    return ref;
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

    TRACE("(%p,%p)\n",This,pbindopts);

    if (pbindopts==NULL)
        return E_POINTER;

    if (pbindopts->cbStruct > sizeof(BIND_OPTS2))
    {
        WARN("invalid size\n");
        return E_INVALIDARG; /* FIXME : not verified */
    }
    memcpy(&This->bindOption2, pbindopts, pbindopts->cbStruct);
    return S_OK;
}

/******************************************************************************
 *        BindCtx_GetBindOptions
 ******************************************************************************/
static HRESULT WINAPI
BindCtxImpl_GetBindOptions(IBindCtx* iface,BIND_OPTS *pbindopts)
{
    BindCtxImpl *This = impl_from_IBindCtx(iface);
    ULONG cbStruct;

    TRACE("(%p,%p)\n",This,pbindopts);

    if (pbindopts==NULL)
        return E_POINTER;

    cbStruct = pbindopts->cbStruct;
    if (cbStruct > sizeof(BIND_OPTS2))
        cbStruct = sizeof(BIND_OPTS2);

    memcpy(pbindopts, &This->bindOption2, cbStruct);
    pbindopts->cbStruct = cbStruct;

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
                       (lstrcmpW(This->bindCtxTable[i].pkeyObj,pszkey)==0)
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


/* Virtual function table for the BindCtx class. */
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
 *         BindCtx_Construct (local function)
 *******************************************************************************/
static HRESULT BindCtxImpl_Construct(BindCtxImpl* This)
{
    TRACE("(%p)\n",This);

    /* Initialize the virtual function table.*/
    This->IBindCtx_iface.lpVtbl = &VT_BindCtxImpl;
    This->ref          = 0;

    /* Initialize the BIND_OPTS2 structure */
    This->bindOption2.cbStruct  = sizeof(BIND_OPTS2);
    This->bindOption2.grfFlags = 0;
    This->bindOption2.grfMode = STGM_READWRITE;
    This->bindOption2.dwTickCountDeadline = 0;

    This->bindOption2.dwTrackFlags = 0;
    This->bindOption2.dwClassContext = CLSCTX_SERVER;
    This->bindOption2.locale = GetThreadLocale();
    This->bindOption2.pServerInfo = 0;

    /* Initialize the bindctx table */
    This->bindCtxTableSize=0;
    This->bindCtxTableLastIndex=0;
    This->bindCtxTable = NULL;

    return S_OK;
}

/******************************************************************************
 *        CreateBindCtx (OLE32.@)
 *
 * Creates a bind context. A bind context encompasses information and options
 * used when binding to a moniker.
 *
 * PARAMS
 *  reserved [I] Reserved. Set to 0.
 *  ppbc     [O] Address that receives the bind context object.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: Any HRESULT code.
 */
HRESULT WINAPI CreateBindCtx(DWORD reserved, LPBC * ppbc)
{
    BindCtxImpl* newBindCtx;
    HRESULT hr;

    TRACE("(%d,%p)\n",reserved,ppbc);

    if (!ppbc) return E_INVALIDARG;

    *ppbc = NULL;

    if (reserved != 0)
    {
        ERR("reserved should be 0, not 0x%x\n", reserved);
        return E_INVALIDARG;
    }

    newBindCtx = HeapAlloc(GetProcessHeap(), 0, sizeof(BindCtxImpl));
    if (newBindCtx == 0)
        return E_OUTOFMEMORY;

    hr = BindCtxImpl_Construct(newBindCtx);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(),0,newBindCtx);
        return hr;
    }

    return BindCtxImpl_QueryInterface(&newBindCtx->IBindCtx_iface,&IID_IBindCtx,(void**)ppbc);
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

    TRACE("(%p, %x, %s, %p)\n", pmk, grfOpt, debugstr_guid(riid), ppvResult);

    res = CreateBindCtx(grfOpt, &pbc);
    if (SUCCEEDED(res))
    {
        res = IMoniker_BindToObject(pmk, pbc, NULL, riid, ppvResult);
        IBindCtx_Release(pbc);
    }
    return res;
}
