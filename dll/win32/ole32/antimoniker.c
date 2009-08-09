/*
 *	                      AntiMonikers implementation
 *
 *               Copyright 1999  Noomen Hamza
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "wine/debug.h"
#include "moniker.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* AntiMoniker data structure */
typedef struct AntiMonikerImpl{

    const IMonikerVtbl*  lpvtbl1;  /* VTable relative to the IMoniker interface.*/

    /* The ROT (RunningObjectTable implementation) uses the IROTData interface to test whether
     * two monikers are equal. That's whay IROTData interface is implemented by monikers.
     */
    const IROTDataVtbl*  lpvtbl2;  /* VTable relative to the IROTData interface.*/

    LONG ref; /* reference counter for this object */

    IUnknown *pMarshal; /* custom marshaler */
} AntiMonikerImpl;

static inline IMoniker *impl_from_IROTData( IROTData *iface )
{
    return (IMoniker *)((char*)iface - FIELD_OFFSET(AntiMonikerImpl, lpvtbl2));
}


/*******************************************************************************
 *        AntiMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    AntiMonikerImpl *This = (AntiMonikerImpl *)iface;

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
    else if (IsEqualIID(&IID_IROTData, riid))
        *ppvObject = &This->lpvtbl2;
    else if (IsEqualIID(&IID_IMarshal, riid))
    {
        HRESULT hr = S_OK;
        if (!This->pMarshal)
            hr = MonikerMarshal_Create(iface, &This->pMarshal);
        if (hr != S_OK)
            return hr;
        return IUnknown_QueryInterface(This->pMarshal, riid, ppvObject);
    }

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* always increase the reference count by one when it is successful */
    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI
AntiMonikerImpl_AddRef(IMoniker* iface)
{
    AntiMonikerImpl *This = (AntiMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 *        AntiMoniker_Release
 ******************************************************************************/
static ULONG WINAPI
AntiMonikerImpl_Release(IMoniker* iface)
{
    AntiMonikerImpl *This = (AntiMonikerImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there's no more reference on it */
    if (ref == 0)
    {
        if (This->pMarshal) IUnknown_Release(This->pMarshal);
        HeapFree(GetProcessHeap(),0,This);
    }

    return ref;
}

/******************************************************************************
 *        AntiMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_AntiMoniker;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

/******************************************************************************
 *        AntiMoniker_Load
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_Load(IMoniker* iface,IStream* pStm)
{
    DWORD constant=1,dwbuffer;
    HRESULT res;

    /* data read by this function is only a DWORD constant (must be 1) ! */
    res=IStream_Read(pStm,&dwbuffer,sizeof(DWORD),NULL);

    if (SUCCEEDED(res)&& dwbuffer!=constant)
        return E_FAIL;

    return res;
}

/******************************************************************************
 *        AntiMoniker_Save
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_Save(IMoniker* iface,IStream* pStm,BOOL fClearDirty)
{
    DWORD constant=1;
    HRESULT res;

    /* data written by this function is only a DWORD constant set to 1 ! */
    res=IStream_Write(pStm,&constant,sizeof(constant),NULL);

    return res;
}

/******************************************************************************
 *        AntiMoniker_GetSizeMax
 *
 * PARAMS
 * pcbSize [out] Pointer to size of stream needed to save object
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    /* for more details see AntiMonikerImpl_Save comments */

    /*
     * Normally the sizemax must be sizeof DWORD, but
     * I tested this function it usually return 16 bytes
     * more than the number of bytes used by AntiMoniker::Save function
     */
    pcbSize->u.LowPart =  sizeof(DWORD)+16;

    pcbSize->u.HighPart=0;

    return S_OK;
}

/******************************************************************************
 *                  AntiMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_BindToObject(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                             REFIID riid, VOID** ppvResult)
{
    TRACE("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult);
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_BindToStorage(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                              REFIID riid, VOID** ppvResult)
{
    TRACE("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult);
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_Reduce
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
                       IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    TRACE("(%p,%p,%d,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    if (ppmkReduced==NULL)
        return E_POINTER;

    AntiMonikerImpl_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}
/******************************************************************************
 *        AntiMoniker_ComposeWith
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
                            BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{

    TRACE("(%p,%p,%d,%p)\n",iface,pmkRight,fOnlyIfNotGeneric,ppmkComposite);

    if ((ppmkComposite==NULL)||(pmkRight==NULL))
	return E_POINTER;

    *ppmkComposite=0;

    if (fOnlyIfNotGeneric)
        return MK_E_NEEDGENERIC;
    else
        return CreateGenericComposite(iface,pmkRight,ppmkComposite);
}

/******************************************************************************
 *        AntiMoniker_Enum
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    TRACE("(%p,%d,%p)\n",iface,fForward,ppenumMoniker);

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsEqual
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    DWORD mkSys;

    TRACE("(%p,%p)\n",iface,pmkOtherMoniker);

    if (pmkOtherMoniker==NULL)
        return S_FALSE;

    IMoniker_IsSystemMoniker(pmkOtherMoniker,&mkSys);

    if (mkSys==MKSYS_ANTIMONIKER)
        return S_OK;
    else
        return S_FALSE;
}

/******************************************************************************
 *        AntiMoniker_Hash
 ******************************************************************************/
static HRESULT WINAPI AntiMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    if (pdwHash==NULL)
        return E_POINTER;

    *pdwHash = 0x80000001;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_IsRunning(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                          IMoniker* pmkNewlyRunning)
{
    IRunningObjectTable* rot;
    HRESULT res;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning);

    if (pbc==NULL)
        return E_INVALIDARG;

    res=IBindCtx_GetRunningObjectTable(pbc,&rot);

    if (FAILED(res))
        return res;

    res = IRunningObjectTable_IsRunning(rot,iface);

    IRunningObjectTable_Release(rot);

    return res;
}

/******************************************************************************
 *        AntiMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI AntiMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pAntiTime)
{
    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pAntiTime);
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    TRACE("(%p,%p)\n",iface,ppmk);

    if (ppmk==NULL)
        return E_POINTER;

    *ppmk=0;

    return MK_E_NOINVERSE;
}

/******************************************************************************
 *        AntiMoniker_CommonPrefixWith
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    DWORD mkSys;

    IMoniker_IsSystemMoniker(pmkOther,&mkSys);

    if(mkSys==MKSYS_ANTIMONIKER){

        IMoniker_AddRef(iface);

        *ppmkPrefix=iface;

        IMoniker_AddRef(iface);

        return MK_S_US;
    }
    else
        return MonikerCommonPrefixWith(iface,pmkOther,ppmkPrefix);
}

/******************************************************************************
 *        AntiMoniker_RelativePathTo
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    TRACE("(%p,%p,%p)\n",iface,pmOther,ppmkRelPath);

    if (ppmkRelPath==NULL)
        return E_POINTER;

    IMoniker_AddRef(pmOther);

    *ppmkRelPath=pmOther;

    return MK_S_HIM;
}

/******************************************************************************
 *        AntiMoniker_GetDisplayName
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_GetDisplayName(IMoniker* iface, IBindCtx* pbc,
                               IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
    static const WCHAR back[]={'\\','.','.',0};

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName);

    if (ppszDisplayName==NULL)
        return E_POINTER;

    if (pmkToLeft!=NULL){
        FIXME("() pmkToLeft!=NULL not implemented\n");
        return E_NOTIMPL;
    }

    *ppszDisplayName=CoTaskMemAlloc(sizeof(back));

    if (*ppszDisplayName==NULL)
        return E_OUTOFMEMORY;

    lstrcpyW(*ppszDisplayName,back);

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_ParseDisplayName
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_ParseDisplayName(IMoniker* iface, IBindCtx* pbc,
                                 IMoniker* pmkToLeft, LPOLESTR pszDisplayName,
                                 ULONG* pchEaten, IMoniker** ppmkOut)
{
    TRACE("(%p,%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_ANTIMONIKER;

    return S_OK;
}

/*******************************************************************************
 *        AntiMonikerIROTData_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
AntiMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{
    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p,%p,%p)\n",iface,riid,ppvObject);

    return AntiMonikerImpl_QueryInterface(This, riid, ppvObject);
}

/***********************************************************************
 *        AntiMonikerIROTData_AddRef
 */
static ULONG WINAPI AntiMonikerROTDataImpl_AddRef(IROTData *iface)
{
    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return AntiMonikerImpl_AddRef(This);
}

/***********************************************************************
 *        AntiMonikerIROTData_Release
 */
static ULONG WINAPI AntiMonikerROTDataImpl_Release(IROTData* iface)
{
    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return AntiMonikerImpl_Release(This);
}

/******************************************************************************
 *        AntiMonikerIROTData_GetComparisonData
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerROTDataImpl_GetComparisonData(IROTData* iface, BYTE* pbData,
                                          ULONG cbMax, ULONG* pcbData)
{
    DWORD constant = 1;

    TRACE("(%p, %u, %p)\n", pbData, cbMax, pcbData);

    *pcbData = sizeof(CLSID) + sizeof(DWORD);
    if (cbMax < *pcbData)
        return E_OUTOFMEMORY;

    memcpy(pbData, &CLSID_AntiMoniker, sizeof(CLSID));
    memcpy(pbData+sizeof(CLSID), &constant, sizeof(DWORD));

    return S_OK;
}

/********************************************************************************/
/* Virtual function table for the AntiMonikerImpl class which  include IPersist,*/
/* IPersistStream and IMoniker functions.                                       */
static const IMonikerVtbl VT_AntiMonikerImpl =
{
    AntiMonikerImpl_QueryInterface,
    AntiMonikerImpl_AddRef,
    AntiMonikerImpl_Release,
    AntiMonikerImpl_GetClassID,
    AntiMonikerImpl_IsDirty,
    AntiMonikerImpl_Load,
    AntiMonikerImpl_Save,
    AntiMonikerImpl_GetSizeMax,
    AntiMonikerImpl_BindToObject,
    AntiMonikerImpl_BindToStorage,
    AntiMonikerImpl_Reduce,
    AntiMonikerImpl_ComposeWith,
    AntiMonikerImpl_Enum,
    AntiMonikerImpl_IsEqual,
    AntiMonikerImpl_Hash,
    AntiMonikerImpl_IsRunning,
    AntiMonikerImpl_GetTimeOfLastChange,
    AntiMonikerImpl_Inverse,
    AntiMonikerImpl_CommonPrefixWith,
    AntiMonikerImpl_RelativePathTo,
    AntiMonikerImpl_GetDisplayName,
    AntiMonikerImpl_ParseDisplayName,
    AntiMonikerImpl_IsSystemMoniker
};

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static const IROTDataVtbl VT_ROTDataImpl =
{
    AntiMonikerROTDataImpl_QueryInterface,
    AntiMonikerROTDataImpl_AddRef,
    AntiMonikerROTDataImpl_Release,
    AntiMonikerROTDataImpl_GetComparisonData
};

/******************************************************************************
 *         AntiMoniker_Construct (local function)
 *******************************************************************************/
static HRESULT AntiMonikerImpl_Construct(AntiMonikerImpl* This)
{

    TRACE("(%p)\n",This);

    /* Initialize the virtual function table. */
    This->lpvtbl1      = &VT_AntiMonikerImpl;
    This->lpvtbl2      = &VT_ROTDataImpl;
    This->ref          = 0;
    This->pMarshal     = NULL;

    return S_OK;
}

/******************************************************************************
 *        CreateAntiMoniker	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI CreateAntiMoniker(LPMONIKER * ppmk)
{
    AntiMonikerImpl* newAntiMoniker = 0;
    HRESULT        hr = S_OK;
    IID riid=IID_IMoniker;

    TRACE("(%p)\n",ppmk);

    newAntiMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(AntiMonikerImpl));

    if (newAntiMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    hr = AntiMonikerImpl_Construct(newAntiMoniker);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(),0,newAntiMoniker);
        return hr;
    }

    hr = AntiMonikerImpl_QueryInterface((IMoniker*)newAntiMoniker,&riid,(void**)ppmk);

    return hr;
}

static HRESULT WINAPI AntiMonikerCF_QueryInterface(LPCLASSFACTORY iface,
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

static ULONG WINAPI AntiMonikerCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI AntiMonikerCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI AntiMonikerCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    IMoniker *pMoniker;
    HRESULT  hr;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreateAntiMoniker(&pMoniker);
    if (FAILED(hr))
        return hr;

  	hr = IMoniker_QueryInterface(pMoniker, riid, ppv);

    if (FAILED(hr))
        IMoniker_Release(pMoniker);

    return hr;
}

static HRESULT WINAPI AntiMonikerCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl AntiMonikerCFVtbl =
{
    AntiMonikerCF_QueryInterface,
    AntiMonikerCF_AddRef,
    AntiMonikerCF_Release,
    AntiMonikerCF_CreateInstance,
    AntiMonikerCF_LockServer
};
static const IClassFactoryVtbl *AntiMonikerCF = &AntiMonikerCFVtbl;

HRESULT AntiMonikerCF_Create(REFIID riid, LPVOID *ppv)
{
    return IClassFactory_QueryInterface((IClassFactory *)&AntiMonikerCF, riid, ppv);
}
