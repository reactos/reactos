/*
 * Pointer Moniker Implementation
 *
 * Copyright 1999 Noomen Hamza
 * Copyright 2008 Robert Shearman (for CodeWeavers)
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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "objbase.h"
#include "oleidl.h"
#include "wine/debug.h"
#include "moniker.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* PointerMoniker data structure */
typedef struct PointerMonikerImpl{

    const IMonikerVtbl*  lpvtbl;  /* VTable relative to the IMoniker interface.*/

    LONG ref; /* reference counter for this object */

    IUnknown *pObject; /* custom marshaler */
} PointerMonikerImpl;

static HRESULT WINAPI
PointerMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    PointerMonikerImpl *This = (PointerMonikerImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,riid,ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( (This==0) || (ppvObject==0) )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid))
        *ppvObject = iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* always increase the reference count by one when it is successful */
    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        PointerMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI
PointerMonikerImpl_AddRef(IMoniker* iface)
{
    PointerMonikerImpl *This = (PointerMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 *        PointerMoniker_Release
 ******************************************************************************/
static ULONG WINAPI
PointerMonikerImpl_Release(IMoniker* iface)
{
    PointerMonikerImpl *This = (PointerMonikerImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there's no more reference on it */
    if (ref == 0)
    {
        if (This->pObject) IUnknown_Release(This->pObject);
        HeapFree(GetProcessHeap(),0,This);
    }

    return ref;
}

/******************************************************************************
 *        PointerMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_PointerMoniker;

    return S_OK;
}

/******************************************************************************
 *        PointerMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

/******************************************************************************
 *        PointerMoniker_Load
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Load(IMoniker* iface,IStream* pStm)
{
    TRACE("(%p)\n", pStm);

    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_Save
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    TRACE("(%p, %d)\n", pStm, fClearDirty);

    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_GetSizeMax
 *
 * PARAMS
 * pcbSize [out] Pointer to size of stream needed to save object
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    pcbSize->u.LowPart = 0;
    pcbSize->u.HighPart = 0;

    return E_NOTIMPL;
}

/******************************************************************************
 *                  PointerMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_BindToObject(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                             REFIID riid, VOID** ppvResult)
{
    PointerMonikerImpl *This = (PointerMonikerImpl *)iface;

    TRACE("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult);

    if (!This->pObject)
        return E_UNEXPECTED;

    return IUnknown_QueryInterface(This->pObject, riid, ppvResult);
}

/******************************************************************************
 *        PointerMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_BindToStorage(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                              REFIID riid, VOID** ppvResult)
{
    PointerMonikerImpl *This = (PointerMonikerImpl *)iface;

    TRACE("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult);

    if (!This->pObject)
        return E_UNEXPECTED;

    return IUnknown_QueryInterface(This->pObject, riid, ppvResult);
}

/******************************************************************************
 *        PointerMoniker_Reduce
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
                       IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    TRACE("(%p,%p,%d,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    if (ppmkReduced==NULL)
        return E_POINTER;

    PointerMonikerImpl_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}
/******************************************************************************
 *        PointerMoniker_ComposeWith
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
                            BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{

    HRESULT res=S_OK;
    DWORD mkSys,mkSys2;
    IEnumMoniker* penumMk=0;
    IMoniker *pmostLeftMk=0;
    IMoniker* tempMkComposite=0;

    TRACE("(%p,%d,%p)\n", pmkRight, fOnlyIfNotGeneric, ppmkComposite);

    if ((ppmkComposite==NULL)||(pmkRight==NULL))
	return E_POINTER;

    *ppmkComposite=0;

    IMoniker_IsSystemMoniker(pmkRight,&mkSys);

    /* If pmkRight is an anti-moniker, the returned moniker is NULL */
    if(mkSys==MKSYS_ANTIMONIKER)
        return res;

    else
        /* if pmkRight is a composite whose leftmost component is an anti-moniker,           */
        /* the returned moniker is the composite after the leftmost anti-moniker is removed. */

         if(mkSys==MKSYS_GENERICCOMPOSITE){

            res=IMoniker_Enum(pmkRight,TRUE,&penumMk);

            if (FAILED(res))
                return res;

            res=IEnumMoniker_Next(penumMk,1,&pmostLeftMk,NULL);

            IMoniker_IsSystemMoniker(pmostLeftMk,&mkSys2);

            if(mkSys2==MKSYS_ANTIMONIKER){

                IMoniker_Release(pmostLeftMk);

                tempMkComposite=iface;
                IMoniker_AddRef(iface);

                while(IEnumMoniker_Next(penumMk,1,&pmostLeftMk,NULL)==S_OK){

                    res=CreateGenericComposite(tempMkComposite,pmostLeftMk,ppmkComposite);

                    IMoniker_Release(tempMkComposite);
                    IMoniker_Release(pmostLeftMk);

                    tempMkComposite=*ppmkComposite;
                    IMoniker_AddRef(tempMkComposite);
                }
                return res;
            }
            else
                return CreateGenericComposite(iface,pmkRight,ppmkComposite);
         }
         /* If pmkRight is not an anti-moniker, the method combines the two monikers into a generic
          composite if fOnlyIfNotGeneric is FALSE; if fOnlyIfNotGeneric is TRUE, the method returns
          a NULL moniker and a return value of MK_E_NEEDGENERIC */
          else
            if (!fOnlyIfNotGeneric)
                return CreateGenericComposite(iface,pmkRight,ppmkComposite);

            else
                return MK_E_NEEDGENERIC;
}

/******************************************************************************
 *        PointerMoniker_Enum
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    TRACE("(%p,%d,%p)\n",iface,fForward,ppenumMoniker);

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

/******************************************************************************
 *        PointerMoniker_IsEqual
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    PointerMonikerImpl *This = (PointerMonikerImpl *)iface;
    DWORD mkSys;

    TRACE("(%p,%p)\n",iface,pmkOtherMoniker);

    if (pmkOtherMoniker==NULL)
        return S_FALSE;

    IMoniker_IsSystemMoniker(pmkOtherMoniker,&mkSys);

    if (mkSys==MKSYS_POINTERMONIKER)
    {
        PointerMonikerImpl *pOtherMoniker = (PointerMonikerImpl *)pmkOtherMoniker;
        return This->pObject == pOtherMoniker->pObject ? S_OK : S_FALSE;
    }
    else
        return S_FALSE;
}

/******************************************************************************
 *        PointerMoniker_Hash
 ******************************************************************************/
static HRESULT WINAPI PointerMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    PointerMonikerImpl *This = (PointerMonikerImpl *)iface;

    if (pdwHash==NULL)
        return E_POINTER;

    *pdwHash = (DWORD)This->pObject;

    return S_OK;
}

/******************************************************************************
 *        PointerMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_IsRunning(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                          IMoniker* pmkNewlyRunning)
{
    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning);

    return S_OK;
}

/******************************************************************************
 *        PointerMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI PointerMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pAntiTime)
{
    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pAntiTime);
    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    TRACE("(%p,%p)\n",iface,ppmk);

    return CreateAntiMoniker(ppmk);
}

/******************************************************************************
 *        PointerMoniker_CommonPrefixWith
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    TRACE("(%p, %p)\n", pmkOther, ppmkPrefix);

    *ppmkPrefix = NULL;

    if (PointerMonikerImpl_IsEqual(iface, pmkOther))
    {
        IMoniker_AddRef(iface);

        *ppmkPrefix=iface;

        return MK_S_US;
    }
    else
        return MK_E_NOPREFIX;
}

/******************************************************************************
 *        PointerMoniker_RelativePathTo
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    TRACE("(%p,%p,%p)\n",iface,pmOther,ppmkRelPath);

    if (ppmkRelPath==NULL)
        return E_POINTER;

    *ppmkRelPath = NULL;

    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_GetDisplayName
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_GetDisplayName(IMoniker* iface, IBindCtx* pbc,
                               IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName);

    if (ppszDisplayName==NULL)
        return E_POINTER;

    *ppszDisplayName = NULL;
    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_ParseDisplayName
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_ParseDisplayName(IMoniker* iface, IBindCtx* pbc,
                                 IMoniker* pmkToLeft, LPOLESTR pszDisplayName,
                                 ULONG* pchEaten, IMoniker** ppmkOut)
{
    PointerMonikerImpl *This = (PointerMonikerImpl *)iface;
    HRESULT hr;
    IParseDisplayName *pPDN;

    TRACE("(%p,%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);

    if (pmkToLeft)
        return MK_E_SYNTAX;

    if (!This->pObject)
        return E_UNEXPECTED;

    hr = IUnknown_QueryInterface(This->pObject, &IID_IParseDisplayName, (void **)&pPDN);
    if (FAILED(hr))
        return hr;

    hr = IParseDisplayName_ParseDisplayName(pPDN, pbc, pszDisplayName, pchEaten, ppmkOut);
    IParseDisplayName_Release(pPDN);

    return hr;
}

/******************************************************************************
 *        PointerMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    *pwdMksys = MKSYS_POINTERMONIKER;

    return S_OK;
}

/********************************************************************************/
/* Virtual function table for the PointerMonikerImpl class which  include IPersist,*/
/* IPersistStream and IMoniker functions.                                       */
static const IMonikerVtbl VT_PointerMonikerImpl =
{
    PointerMonikerImpl_QueryInterface,
    PointerMonikerImpl_AddRef,
    PointerMonikerImpl_Release,
    PointerMonikerImpl_GetClassID,
    PointerMonikerImpl_IsDirty,
    PointerMonikerImpl_Load,
    PointerMonikerImpl_Save,
    PointerMonikerImpl_GetSizeMax,
    PointerMonikerImpl_BindToObject,
    PointerMonikerImpl_BindToStorage,
    PointerMonikerImpl_Reduce,
    PointerMonikerImpl_ComposeWith,
    PointerMonikerImpl_Enum,
    PointerMonikerImpl_IsEqual,
    PointerMonikerImpl_Hash,
    PointerMonikerImpl_IsRunning,
    PointerMonikerImpl_GetTimeOfLastChange,
    PointerMonikerImpl_Inverse,
    PointerMonikerImpl_CommonPrefixWith,
    PointerMonikerImpl_RelativePathTo,
    PointerMonikerImpl_GetDisplayName,
    PointerMonikerImpl_ParseDisplayName,
    PointerMonikerImpl_IsSystemMoniker
};

/******************************************************************************
 *         PointerMoniker_Construct (local function)
 *******************************************************************************/
static void PointerMonikerImpl_Construct(PointerMonikerImpl* This, IUnknown *punk)
{
    TRACE("(%p)\n",This);

    /* Initialize the virtual function table. */
    This->lpvtbl       = &VT_PointerMonikerImpl;
    This->ref          = 1;
    if (punk)
        IUnknown_AddRef(punk);
    This->pObject      = punk;
}

/***********************************************************************
 *           CreatePointerMoniker (OLE32.@)
 *
 * Creates a moniker which represents a pointer.
 *
 * PARAMS
 *  punk [I] Pointer to the object to represent.
 *  ppmk [O] Address that receives the pointer to the created moniker.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: Any HRESULT code.
 */
HRESULT WINAPI CreatePointerMoniker(LPUNKNOWN punk, LPMONIKER *ppmk)
{
    PointerMonikerImpl *This;

    TRACE("(%p, %p)\n", punk, ppmk);

    if (!ppmk)
        return E_INVALIDARG;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
    {
        *ppmk = NULL;
        return E_OUTOFMEMORY;
    }

    PointerMonikerImpl_Construct(This, punk);
    *ppmk = (IMoniker *)&This->lpvtbl;
    return S_OK;
}

static HRESULT WINAPI PointerMonikerCF_QueryInterface(LPCLASSFACTORY iface,
                                                  REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI PointerMonikerCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI PointerMonikerCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI PointerMonikerCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    IMoniker *pMoniker;
    HRESULT  hr;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreatePointerMoniker(NULL, &pMoniker);
    if (FAILED(hr))
        return hr;

	hr = IMoniker_QueryInterface(pMoniker, riid, ppv);

    if (FAILED(hr))
        IMoniker_Release(pMoniker);

    return hr;
}

static HRESULT WINAPI PointerMonikerCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl PointerMonikerCFVtbl =
{
    PointerMonikerCF_QueryInterface,
    PointerMonikerCF_AddRef,
    PointerMonikerCF_Release,
    PointerMonikerCF_CreateInstance,
    PointerMonikerCF_LockServer
};
static const IClassFactoryVtbl *PointerMonikerCF = &PointerMonikerCFVtbl;

HRESULT PointerMonikerCF_Create(REFIID riid, LPVOID *ppv)
{
    return IClassFactory_QueryInterface((IClassFactory *)&PointerMonikerCF, riid, ppv);
}
