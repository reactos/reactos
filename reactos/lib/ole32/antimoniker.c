/***************************************************************************************
 *	                      AntiMonikers implementation
 *
 *               Copyright 1999  Noomen Hamza
 ***************************************************************************************/

#include <string.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <compobj.h>
#include <storage32.h>

#include <debug.h>


/* AntiMoniker data structure */
typedef struct AntiMonikerImpl{

    ICOM_VTABLE(IMoniker)*  lpvtbl1;  /* VTable relative to the IMoniker interface.*/

    /* The ROT (RunningObjectTable implementation) uses the IROTData interface to test whether 
     * two monikers are equal. That's whay IROTData interface is implemented by monikers.
     */
    ICOM_VTABLE(IROTData)*  lpvtbl2;  /* VTable relative to the IROTData interface.*/

    ULONG ref; /* reference counter for this object */

} AntiMonikerImpl;

/********************************************************************************/
/* AntiMoniker prototype functions :                                            */

/* IUnknown prototype functions */
static HRESULT WINAPI AntiMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI AntiMonikerImpl_AddRef(IMoniker* iface);
static ULONG   WINAPI AntiMonikerImpl_Release(IMoniker* iface);

/* IPersist prototype functions */
static HRESULT WINAPI AntiMonikerImpl_GetClassID(IMoniker* iface, CLSID *pClassID);

/* IPersistStream prototype functions */
static HRESULT WINAPI AntiMonikerImpl_IsDirty(IMoniker* iface);
static HRESULT WINAPI AntiMonikerImpl_Load(IMoniker* iface, IStream* pStm);
static HRESULT WINAPI AntiMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty);
static HRESULT WINAPI AntiMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize);

/* IMoniker prototype functions */
static HRESULT WINAPI AntiMonikerImpl_BindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI AntiMonikerImpl_BindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI AntiMonikerImpl_Reduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced);
static HRESULT WINAPI AntiMonikerImpl_ComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite);
static HRESULT WINAPI AntiMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker);
static HRESULT WINAPI AntiMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker);
static HRESULT WINAPI AntiMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash);
static HRESULT WINAPI AntiMonikerImpl_IsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning);
static HRESULT WINAPI AntiMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pAntiTime);
static HRESULT WINAPI AntiMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk);
static HRESULT WINAPI AntiMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther, IMoniker** ppmkPrefix);
static HRESULT WINAPI AntiMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath);
static HRESULT WINAPI AntiMonikerImpl_GetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName);
static HRESULT WINAPI AntiMonikerImpl_ParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut);
static HRESULT WINAPI AntiMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys);

/********************************************************************************/
/* IROTData prototype functions                                                 */

/* IUnknown prototype functions */
static HRESULT WINAPI AntiMonikerROTDataImpl_QueryInterface(IROTData* iface,REFIID riid,VOID** ppvObject);
static ULONG   WINAPI AntiMonikerROTDataImpl_AddRef(IROTData* iface);
static ULONG   WINAPI AntiMonikerROTDataImpl_Release(IROTData* iface);

/* IROTData prototype function */
static HRESULT WINAPI AntiMonikerROTDataImpl_GetComparaisonData(IROTData* iface,BYTE* pbData,ULONG cbMax,ULONG* pcbData);

/* Local function used by AntiMoniker implementation */
HRESULT WINAPI AntiMonikerImpl_Construct(AntiMonikerImpl* iface);
HRESULT WINAPI AntiMonikerImpl_Destroy(AntiMonikerImpl* iface);

/********************************************************************************/
/* Virtual function table for the AntiMonikerImpl class which  include IPersist,*/
/* IPersistStream and IMoniker functions.                                       */
static ICOM_VTABLE(IMoniker) VT_AntiMonikerImpl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
static ICOM_VTABLE(IROTData) VT_ROTDataImpl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    AntiMonikerROTDataImpl_QueryInterface,
    AntiMonikerROTDataImpl_AddRef,
    AntiMonikerROTDataImpl_Release,
    AntiMonikerROTDataImpl_GetComparaisonData
};

/*******************************************************************************
 *        AntiMoniker_QueryInterface
 *******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    ICOM_THIS(AntiMonikerImpl,iface);
  
  Print(MAX_TRACE, ("(%p,%p,%p)\n",This,riid,ppvObject));

  /* Perform a sanity check on the parameters.*/
    if ( (This==0) || (ppvObject==0) )
	return E_INVALIDARG;
  
  /* Initialize the return parameter */
  *ppvObject = 0;

  /* Compare the riid with the interface IDs implemented by this object.*/
  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_IPersist, riid) ||
      IsEqualIID(&IID_IPersistStream, riid) ||
      IsEqualIID(&IID_IMoniker, riid)
     )
      *ppvObject = iface;
    else if (IsEqualIID(&IID_IROTData, riid))
        *ppvObject = (IROTData*)&(This->lpvtbl2);

  /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;
  
   /* Query Interface always increases the reference count by one when it is successful */
  AntiMonikerImpl_AddRef(iface);

  return S_OK;
}

/******************************************************************************
 *        AntiMoniker_AddRef
 ******************************************************************************/
ULONG WINAPI AntiMonikerImpl_AddRef(IMoniker* iface)
{
    ICOM_THIS(AntiMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p)\n",This));

    return ++(This->ref);
}

/******************************************************************************
 *        AntiMoniker_Release
 ******************************************************************************/
ULONG WINAPI AntiMonikerImpl_Release(IMoniker* iface)
{
    ICOM_THIS(AntiMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p)\n",This));

    This->ref--;

    /* destroy the object if there's no more reference on it */
    if (This->ref==0){

        AntiMonikerImpl_Destroy(This);

        return 0;
    }
    return This->ref;;
}

/******************************************************************************
 *        AntiMoniker_GetClassID
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    Print(MAX_TRACE, ("(%p,%p),stub!\n",iface,pClassID));

    if (pClassID==NULL)
        return E_POINTER;
            
    *pClassID = CLSID_AntiMoniker;
        
    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsDirty
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    Print(MAX_TRACE, ("(%p)\n",iface));

    return S_FALSE;
}

/******************************************************************************
 *        AntiMoniker_Load
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_Load(IMoniker* iface,IStream* pStm)
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
HRESULT WINAPI AntiMonikerImpl_Save(IMoniker* iface,IStream* pStm,BOOL fClearDirty)
{
    DWORD constant=1;
    HRESULT res;
    
    /* data writen by this function is only a DWORD constant seted to 1 ! */
    res=IStream_Write(pStm,&constant,sizeof(constant),NULL);

    return res;
}

/******************************************************************************
 *        AntiMoniker_GetSizeMax
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_GetSizeMax(IMoniker* iface,
                                          ULARGE_INTEGER* pcbSize)/* Pointer to size of stream needed to save object */
{
    Print(MAX_TRACE, ("(%p,%p)\n",iface,pcbSize));

    if (pcbSize!=NULL)
        return E_POINTER;

    /* for more details see AntiMonikerImpl_Save coments */
    
    /* Normaly the sizemax must be the  size of DWORD ! but I tested this function it ususlly return 16 bytes */
    /* more than the number of bytes used by AntiMoniker::Save function */
    pcbSize->u.LowPart =  sizeof(DWORD)+16;

    pcbSize->u.HighPart=0;

    return S_OK;
}

/******************************************************************************
 *         AntiMoniker_Construct (local function)
 *******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_Construct(AntiMonikerImpl* This)
{

    Print(MAX_TRACE, ("(%p)\n",This));

    /* Initialize the virtual fgunction table. */
    This->lpvtbl1      = &VT_AntiMonikerImpl;
    This->lpvtbl2      = &VT_ROTDataImpl;
    This->ref          = 0;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_Destroy (local function)
 *******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_Destroy(AntiMonikerImpl* This)
{
    Print(MAX_TRACE, ("(%p)\n",This));

    return HeapFree(GetProcessHeap(),0,This);
}

/******************************************************************************
 *                  AntiMoniker_BindToObject
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_BindToObject(IMoniker* iface,
                                            IBindCtx* pbc,
                                            IMoniker* pmkToLeft,
                                            REFIID riid,
                                            VOID** ppvResult)
{
    Print(MAX_TRACE, ("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult));
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_BindToStorage
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_BindToStorage(IMoniker* iface,
                                             IBindCtx* pbc,
                                             IMoniker* pmkToLeft,
                                             REFIID riid,
                                             VOID** ppvResult)
{
    Print(MAX_TRACE, ("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult));
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_Reduce
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_Reduce(IMoniker* iface,
                                      IBindCtx* pbc,
                                      DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft,
                                      IMoniker** ppmkReduced)
{
    Print(MAX_TRACE, ("(%p,%p,%ld,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced));

    if (ppmkReduced==NULL)
        return E_POINTER;

    AntiMonikerImpl_AddRef(iface);

    *ppmkReduced=iface;
    
    return MK_S_REDUCED_TO_SELF;
}
/******************************************************************************
 *        AntiMoniker_ComposeWith
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_ComposeWith(IMoniker* iface,
                                           IMoniker* pmkRight,
                                           BOOL fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite)
{

    Print(MAX_TRACE, ("(%p,%p,%d,%p)\n",iface,pmkRight,fOnlyIfNotGeneric,ppmkComposite));

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
HRESULT WINAPI AntiMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    Print(MAX_TRACE, ("(%p,%d,%p)\n",iface,fForward,ppenumMoniker));

    if (ppenumMoniker == NULL)
        return E_POINTER;
    
    *ppenumMoniker = NULL;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsEqual
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    DWORD mkSys;
    
    Print(MAX_TRACE, ("(%p,%p)\n",iface,pmkOtherMoniker));

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
HRESULT WINAPI AntiMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    if (pdwHash==NULL)
        return E_POINTER;

    *pdwHash=0;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsRunning
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_IsRunning(IMoniker* iface,
                                         IBindCtx* pbc,
                                         IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning)
{
    IRunningObjectTable* rot;
    HRESULT res;

    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning));

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
HRESULT WINAPI AntiMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pAntiTime)
{
    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pAntiTime));
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_Inverse
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    Print(MAX_TRACE, ("(%p,%p)\n",iface,ppmk));

    if (ppmk==NULL)
        return E_POINTER;

    *ppmk=0;

    return MK_E_NOINVERSE;
}

/******************************************************************************
 *        AntiMoniker_CommonPrefixWith
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    DWORD mkSys;
    
    IMoniker_IsSystemMoniker(pmkOther,&mkSys);

    if(mkSys==MKSYS_ITEMMONIKER){

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
HRESULT WINAPI AntiMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    Print(MAX_TRACE, ("(%p,%p,%p)\n",iface,pmOther,ppmkRelPath));

    if (ppmkRelPath==NULL)
        return E_POINTER;

    IMoniker_AddRef(pmOther);

    *ppmkRelPath=pmOther;

    return MK_S_HIM;
}

/******************************************************************************
 *        AntiMoniker_GetDisplayName
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_GetDisplayName(IMoniker* iface,
                                              IBindCtx* pbc,
                                              IMoniker* pmkToLeft,
                                              LPOLESTR *ppszDisplayName)
{
    WCHAR back[]={'\\','.','.',0};
    
    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName));

    if (ppszDisplayName==NULL)
        return E_POINTER;

    if (pmkToLeft!=NULL){
        Print(MIN_TRACE, ("() pmkToLeft!=NULL not implemented \n"));
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
HRESULT WINAPI AntiMonikerImpl_ParseDisplayName(IMoniker* iface,
                                                IBindCtx* pbc,
                                                IMoniker* pmkToLeft,
                                                LPOLESTR pszDisplayName,
                                                ULONG* pchEaten,
                                                IMoniker** ppmkOut)
{
    Print(MAX_TRACE, ("(%p,%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut));
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_IsSystemMoniker
 ******************************************************************************/
HRESULT WINAPI AntiMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    Print(MAX_TRACE, ("(%p,%p)\n",iface,pwdMksys));

    if (!pwdMksys)
        return E_POINTER;
    
    (*pwdMksys)=MKSYS_ANTIMONIKER;

    return S_OK;
}

/*******************************************************************************
 *        AntiMonikerIROTData_QueryInterface
 *******************************************************************************/
HRESULT WINAPI AntiMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{

    ICOM_THIS_From_IROTData(IMoniker, iface);

    Print(MAX_TRACE, ("(%p,%p,%p)\n",iface,riid,ppvObject));

    return AntiMonikerImpl_QueryInterface(This, riid, ppvObject);
}

/***********************************************************************
 *        AntiMonikerIROTData_AddRef
 */
ULONG   WINAPI AntiMonikerROTDataImpl_AddRef(IROTData *iface)
{
    ICOM_THIS_From_IROTData(IMoniker, iface);

    Print(MAX_TRACE, ("(%p)\n",iface));

    return AntiMonikerImpl_AddRef(This);
}

/***********************************************************************
 *        AntiMonikerIROTData_Release
 */
ULONG   WINAPI AntiMonikerROTDataImpl_Release(IROTData* iface)
{
    ICOM_THIS_From_IROTData(IMoniker, iface);
    
    Print(MAX_TRACE, ("(%p)\n",iface));

    return AntiMonikerImpl_Release(This);
}

/******************************************************************************
 *        AntiMonikerIROTData_GetComparaisonData
 ******************************************************************************/
HRESULT WINAPI AntiMonikerROTDataImpl_GetComparaisonData(IROTData* iface,
                                                         BYTE* pbData,
                                                         ULONG cbMax,
                                                         ULONG* pcbData)
{
    Print(MIN_TRACE, ("(),stub!\n"));
    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateAntiMoniker	[OLE.55]
 ******************************************************************************/
HRESULT WINAPI CreateAntiMoniker(LPMONIKER * ppmk)
{
    AntiMonikerImpl* newAntiMoniker = 0;
    HRESULT        hr = S_OK;
    IID riid=IID_IMoniker;
    
    Print(MAX_TRACE, ("(%p)\n",ppmk));

    newAntiMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(AntiMonikerImpl));

    if (newAntiMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    hr = AntiMonikerImpl_Construct(newAntiMoniker);

    if (FAILED(hr)){

        HeapFree(GetProcessHeap(),0,newAntiMoniker);
        return hr;
    }

    hr = AntiMonikerImpl_QueryInterface((IMoniker*)newAntiMoniker,&riid,(void**)ppmk);

    return hr;
}
