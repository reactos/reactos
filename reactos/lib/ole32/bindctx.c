/***************************************************************************************
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ***************************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* represent the first size table and it's increment block size */
#define  BLOCK_TAB_SIZE 10
#define  MAX_TAB_SIZE   0xFFFFFFFF

/* data structure of the BindCtx table elements */
typedef struct BindCtxObject{

    IUnknown*   pObj; /* point on a bound object */

    LPOLESTR  pkeyObj; /* key associated to this bound object */

    BYTE regType; /* registration type: 1 if RegisterObjectParam and 0 if RegisterObjectBound */

} BindCtxObject;

/* BindCtx data strucrture */
typedef struct BindCtxImpl{

    IBindCtxVtbl *lpVtbl; /* VTable relative to the IBindCtx interface.*/

    ULONG ref; /* reference counter for this object */

    BindCtxObject* bindCtxTable; /* this is a table in which all bounded objects are stored*/
    DWORD          bindCtxTableLastIndex;  /* first free index in the table */
    DWORD          bindCtxTableSize;   /* size table */

    BIND_OPTS2 bindOption2; /* a structure which contains the bind options*/

} BindCtxImpl;

/* IBindCtx prototype functions : */

/* IUnknown functions*/
static HRESULT WINAPI BindCtxImpl_QueryInterface(IBindCtx* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI BindCtxImpl_AddRef(IBindCtx* iface);
static ULONG   WINAPI BindCtxImpl_Release(IBindCtx* iface);
/* IBindCtx functions */
static HRESULT WINAPI BindCtxImpl_RegisterObjectBound(IBindCtx* iface,IUnknown* punk);
static HRESULT WINAPI BindCtxImpl_RevokeObjectBound(IBindCtx* iface, IUnknown* punk);
static HRESULT WINAPI BindCtxImpl_ReleaseBoundObjects(IBindCtx* iface);
static HRESULT WINAPI BindCtxImpl_SetBindOptions(IBindCtx* iface,BIND_OPTS *pbindopts);
static HRESULT WINAPI BindCtxImpl_GetBindOptions(IBindCtx* iface,BIND_OPTS *pbindopts);
static HRESULT WINAPI BindCtxImpl_GetRunningObjectTable(IBindCtx* iface,IRunningObjectTable** pprot);
static HRESULT WINAPI BindCtxImpl_RegisterObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown* punk);
static HRESULT WINAPI BindCtxImpl_GetObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown** punk);
static HRESULT WINAPI BindCtxImpl_EnumObjectParam(IBindCtx* iface,IEnumString** ppenum);
static HRESULT WINAPI BindCtxImpl_RevokeObjectParam(IBindCtx* iface,LPOLESTR pszkey);
/* Local functions*/
HRESULT WINAPI BindCtxImpl_Construct(BindCtxImpl* This);
HRESULT WINAPI BindCtxImpl_Destroy(BindCtxImpl* This);
HRESULT WINAPI BindCtxImpl_GetObjectIndex(BindCtxImpl* This,IUnknown* punk,LPOLESTR pszkey,DWORD *index);

/* Virtual function table for the BindCtx class. */
static IBindCtxVtbl VT_BindCtxImpl =
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

/*******************************************************************************
 *        BindCtx_QueryInterface
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_QueryInterface(IBindCtx* iface,REFIID riid,void** ppvObject)
{
  BindCtxImpl *This = (BindCtxImpl *)iface;

  TRACE("(%p,%p,%p)\n",This,riid,ppvObject);

  /* Perform a sanity check on the parameters.*/
  if ( (This==0) || (ppvObject==0) )
      return E_INVALIDARG;

  /* Initialize the return parameter.*/
  *ppvObject = 0;

  /* Compare the riid with the interface IDs implemented by this object.*/
  if (IsEqualIID(&IID_IUnknown, riid))
      *ppvObject = (IBindCtx*)This;
  else
      if (IsEqualIID(&IID_IBindCtx, riid))
          *ppvObject = (IBindCtx*)This;

  /* Check that we obtained an interface.*/
  if ((*ppvObject)==0)
      return E_NOINTERFACE;

   /* Query Interface always increases the reference count by one when it is successful */
  BindCtxImpl_AddRef(iface);

  return S_OK;
}

/******************************************************************************
 *       BindCtx_AddRef
 ******************************************************************************/
ULONG WINAPI BindCtxImpl_AddRef(IBindCtx* iface)
{
    BindCtxImpl *This = (BindCtxImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 *        BindCtx_Release
 ******************************************************************************/
ULONG WINAPI BindCtxImpl_Release(IBindCtx* iface)
{
    BindCtxImpl *This = (BindCtxImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    if (ref == 0){
        /* release all registered objects */
        BindCtxImpl_ReleaseBoundObjects((IBindCtx*)This);

        BindCtxImpl_Destroy(This);
    }
    return ref;
}


/******************************************************************************
 *         BindCtx_Construct (local function)
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_Construct(BindCtxImpl* This)
{
    TRACE("(%p)\n",This);

    /* Initialize the virtual function table.*/
    This->lpVtbl       = &VT_BindCtxImpl;
    This->ref          = 0;

    /* Initialize the BIND_OPTS2 structure */
    This->bindOption2.cbStruct  = sizeof(BIND_OPTS2);
    This->bindOption2.grfFlags = 0;
    This->bindOption2.grfMode = STGM_READWRITE;
    This->bindOption2.dwTickCountDeadline = 0;

    This->bindOption2.dwTrackFlags = 0;
    This->bindOption2.dwClassContext = CLSCTX_SERVER;
    This->bindOption2.locale = 1033;
    This->bindOption2.pServerInfo = 0;

    /* Initialize the bindctx table */
    This->bindCtxTableSize=BLOCK_TAB_SIZE;
    This->bindCtxTableLastIndex=0;
    This->bindCtxTable= HeapAlloc(GetProcessHeap(), 0,This->bindCtxTableSize*sizeof(BindCtxObject));

    if (This->bindCtxTable==NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

/******************************************************************************
 *        BindCtx_Destroy    (local function)
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_Destroy(BindCtxImpl* This)
{
    TRACE("(%p)\n",This);

    /* free the table space memory */
    HeapFree(GetProcessHeap(),0,This->bindCtxTable);

    /* free the bindctx structure */
    HeapFree(GetProcessHeap(),0,This);

    return S_OK;
}


/******************************************************************************
 *        BindCtx_RegisterObjectBound
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RegisterObjectBound(IBindCtx* iface,IUnknown* punk)
{
    BindCtxImpl *This = (BindCtxImpl *)iface;
    DWORD lastIndex=This->bindCtxTableLastIndex;

    TRACE("(%p,%p)\n",This,punk);

    if (punk==NULL)
        return E_POINTER;

    IUnknown_AddRef(punk);

    /* put the object in the first free element in the table */
    This->bindCtxTable[lastIndex].pObj = punk;
    This->bindCtxTable[lastIndex].pkeyObj = NULL;
    This->bindCtxTable[lastIndex].regType = 0;
    lastIndex= ++This->bindCtxTableLastIndex;

    if (lastIndex == This->bindCtxTableSize){ /* the table is full so it must be resized */

        if (This->bindCtxTableSize > (MAX_TAB_SIZE-BLOCK_TAB_SIZE)){
            FIXME("This->bindCtxTableSize: %ld is out of data limite \n",This->bindCtxTableSize);
            return E_FAIL;
        }

        This->bindCtxTableSize+=BLOCK_TAB_SIZE; /* new table size */

        This->bindCtxTable = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,This->bindCtxTable,
                                         This->bindCtxTableSize * sizeof(BindCtxObject));
        if (!This->bindCtxTable)
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

/******************************************************************************
 *        BindCtx_RevokeObjectBound
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RevokeObjectBound(IBindCtx* iface, IUnknown* punk)
{
    DWORD index,j;

    BindCtxImpl *This = (BindCtxImpl *)iface;

    TRACE("(%p,%p)\n",This,punk);

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
HRESULT WINAPI BindCtxImpl_ReleaseBoundObjects(IBindCtx* iface)
{
    DWORD i;

    BindCtxImpl *This = (BindCtxImpl *)iface;

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
HRESULT WINAPI BindCtxImpl_SetBindOptions(IBindCtx* iface,BIND_OPTS *pbindopts)
{
    BindCtxImpl *This = (BindCtxImpl *)iface;

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
HRESULT WINAPI BindCtxImpl_GetBindOptions(IBindCtx* iface,BIND_OPTS *pbindopts)
{
    BindCtxImpl *This = (BindCtxImpl *)iface;

    TRACE("(%p,%p)\n",This,pbindopts);

    if (pbindopts==NULL)
        return E_POINTER;

    if (pbindopts->cbStruct > sizeof(BIND_OPTS2))
    {
        WARN("invalid size\n");
        return E_INVALIDARG; /* FIXME : not verified */
    }
    memcpy(pbindopts, &This->bindOption2, pbindopts->cbStruct);
    return S_OK;
}

/******************************************************************************
 *        BindCtx_GetRunningObjectTable
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_GetRunningObjectTable(IBindCtx* iface,IRunningObjectTable** pprot)
{
    HRESULT res;

    BindCtxImpl *This = (BindCtxImpl *)iface;

    TRACE("(%p,%p)\n",This,pprot);

    if (pprot==NULL)
        return E_POINTER;

    res=GetRunningObjectTable(0, pprot);

    return res;
}

/******************************************************************************
 *        BindCtx_RegisterObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RegisterObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown* punk)
{
    DWORD index=0;
    BindCtxImpl *This = (BindCtxImpl *)iface;

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
    This->bindCtxTable[This->bindCtxTableLastIndex].pObj = punk;
    This->bindCtxTable[This->bindCtxTableLastIndex].regType = 1;

    if (pszkey==NULL)

        This->bindCtxTable[This->bindCtxTableLastIndex].pkeyObj=NULL;

    else{

        This->bindCtxTable[This->bindCtxTableLastIndex].pkeyObj=
            HeapAlloc(GetProcessHeap(),0,(sizeof(WCHAR)*(1+lstrlenW(pszkey))));

        if (This->bindCtxTable[This->bindCtxTableLastIndex].pkeyObj==NULL)
            return E_OUTOFMEMORY;
        strcpyW(This->bindCtxTable[This->bindCtxTableLastIndex].pkeyObj,pszkey);
    }

    This->bindCtxTableLastIndex++;

    if (This->bindCtxTableLastIndex == This->bindCtxTableSize){ /* table is full ! must be resized */

        This->bindCtxTableSize+=BLOCK_TAB_SIZE; /* new table size */

        if (This->bindCtxTableSize > (MAX_TAB_SIZE-BLOCK_TAB_SIZE)){
            FIXME("This->bindCtxTableSize: %ld is out of data limite \n",This->bindCtxTableSize);
            return E_FAIL;
        }
        This->bindCtxTable = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,This->bindCtxTable,
                                         This->bindCtxTableSize * sizeof(BindCtxObject));
        if (!This->bindCtxTable)
            return E_OUTOFMEMORY;
    }
    IUnknown_AddRef(punk);
    return S_OK;
}

/******************************************************************************
 *        BindCtx_GetObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_GetObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown** punk)
{
    DWORD index;
    BindCtxImpl *This = (BindCtxImpl *)iface;

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
HRESULT WINAPI BindCtxImpl_RevokeObjectParam(IBindCtx* iface,LPOLESTR ppenum)
{
    DWORD index,j;

    BindCtxImpl *This = (BindCtxImpl *)iface;

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
HRESULT WINAPI BindCtxImpl_EnumObjectParam(IBindCtx* iface,IEnumString** pszkey)
{
    FIXME("(%p,%p),stub!\n",iface,pszkey);
    return E_NOTIMPL;
}

/********************************************************************************
 *        GetObjectIndex (local function)
 ********************************************************************************/
HRESULT WINAPI BindCtxImpl_GetObjectIndex(BindCtxImpl* This,
                                          IUnknown* punk,
                                          LPOLESTR pszkey,
                                          DWORD *index)
{

    DWORD i;
    BYTE found=0;

    TRACE("(%p,%p,%p,%p)\n",This,punk,pszkey,index);

    if (punk==NULL)
        /* search object identified by a register key */
        for(i=0; ( (i<This->bindCtxTableLastIndex) && (!found));i++){

            if(This->bindCtxTable[i].regType==1){

                if ( ( (This->bindCtxTable[i].pkeyObj==NULL) && (pszkey==NULL) ) ||
                     ( (This->bindCtxTable[i].pkeyObj!=NULL) &&
                       (pszkey!=NULL) &&
                       (lstrcmpW(This->bindCtxTable[i].pkeyObj,pszkey)==0)
                     )
                   )

                    found=1;
            }
        }
    else
        /* search object identified by a moniker*/
        for(i=0; ( (i<This->bindCtxTableLastIndex) && (!found));i++)
            if(This->bindCtxTable[i].pObj==punk)
                found=1;

    if (index != NULL)
        *index=i-1;

    if (found)
        return S_OK;
    TRACE("key not found\n");
    return S_FALSE;
}

/******************************************************************************
 *        CreateBindCtx16
 ******************************************************************************/
HRESULT WINAPI CreateBindCtx16(DWORD reserved, LPBC * ppbc)
{
    FIXME("(%ld,%p),stub!\n",reserved,ppbc);
    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateBindCtx (OLE32.@)
 ******************************************************************************/
HRESULT WINAPI CreateBindCtx(DWORD reserved, LPBC * ppbc)
{
    BindCtxImpl* newBindCtx = 0;
    HRESULT hr;
    IID riid=IID_IBindCtx;

    TRACE("(%ld,%p)\n",reserved,ppbc);

    newBindCtx = HeapAlloc(GetProcessHeap(), 0, sizeof(BindCtxImpl));

    if (newBindCtx == 0)
        return E_OUTOFMEMORY;

    hr = BindCtxImpl_Construct(newBindCtx);

    if (FAILED(hr)){

        HeapFree(GetProcessHeap(),0,newBindCtx);
        return hr;
    }

    hr = BindCtxImpl_QueryInterface((IBindCtx*)newBindCtx,&riid,(void**)ppbc);

    return hr;
}

HRESULT WINAPI BindMoniker(LPMONIKER pmk, DWORD grfOpt, REFIID riid, LPVOID * ppvResult)
{
    HRESULT res;
    IBindCtx * pbc;

    TRACE("(%p, %lx, %s, %p)\n", pmk, grfOpt, debugstr_guid(riid), ppvResult);

    res = CreateBindCtx(grfOpt, &pbc);
    if (SUCCEEDED(res))
        res = IMoniker_BindToObject(pmk, pbc, NULL, riid, ppvResult);
    return res;
}
