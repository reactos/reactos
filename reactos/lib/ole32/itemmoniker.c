/***************************************************************************************
 *	                      ItemMonikers implementation
 *
 *           Copyright 1999  Noomen Hamza
 ***************************************************************************************/

#include <string.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <compobj.h>
#include <storage32.h>

#include <debug.h>


/* ItemMoniker data structure */
typedef struct ItemMonikerImpl{

    ICOM_VTABLE(IMoniker)*  lpvtbl1;  /* VTable relative to the IMoniker interface.*/

    /* The ROT (RunningObjectTable implementation) uses the IROTData interface to test whether 
     * two monikers are equal. That's whay IROTData interface is implemented by monikers.
     */
    ICOM_VTABLE(IROTData)*  lpvtbl2;  /* VTable relative to the IROTData interface.*/

    ULONG ref; /* reference counter for this object */

    LPOLESTR itemName; /* item name identified by this ItemMoniker */

    LPOLESTR itemDelimiter; /* Delimiter string */
    
} ItemMonikerImpl;

/********************************************************************************/
/* ItemMoniker prototype functions :                                            */

/* IUnknown prototype functions */
static HRESULT WINAPI ItemMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI ItemMonikerImpl_AddRef(IMoniker* iface);
static ULONG   WINAPI ItemMonikerImpl_Release(IMoniker* iface);

/* IPersist prototype functions */
static HRESULT WINAPI ItemMonikerImpl_GetClassID(IMoniker* iface, CLSID *pClassID);

/* IPersistStream prototype functions */
static HRESULT WINAPI ItemMonikerImpl_IsDirty(IMoniker* iface);
static HRESULT WINAPI ItemMonikerImpl_Load(IMoniker* iface, IStream* pStm);
static HRESULT WINAPI ItemMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty);
static HRESULT WINAPI ItemMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize);

/* IMoniker prototype functions */
static HRESULT WINAPI ItemMonikerImpl_BindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI ItemMonikerImpl_BindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI ItemMonikerImpl_Reduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced);
static HRESULT WINAPI ItemMonikerImpl_ComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite);
static HRESULT WINAPI ItemMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker);
static HRESULT WINAPI ItemMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker);
static HRESULT WINAPI ItemMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash);
static HRESULT WINAPI ItemMonikerImpl_IsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning);
static HRESULT WINAPI ItemMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pItemTime);
static HRESULT WINAPI ItemMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk);
static HRESULT WINAPI ItemMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther, IMoniker** ppmkPrefix);
static HRESULT WINAPI ItemMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath);
static HRESULT WINAPI ItemMonikerImpl_GetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName);
static HRESULT WINAPI ItemMonikerImpl_ParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut);
static HRESULT WINAPI ItemMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys);

/* Local function used by ItemMoniker implementation */
HRESULT WINAPI ItemMonikerImpl_Construct(ItemMonikerImpl* iface, LPCOLESTR lpszDelim,LPCOLESTR lpszPathName);
HRESULT WINAPI ItemMonikerImpl_Destroy(ItemMonikerImpl* iface);

/********************************************************************************/
/* IROTData prototype functions                                                 */

/* IUnknown prototype functions */
static HRESULT WINAPI ItemMonikerROTDataImpl_QueryInterface(IROTData* iface,REFIID riid,VOID** ppvObject);
static ULONG   WINAPI ItemMonikerROTDataImpl_AddRef(IROTData* iface);
static ULONG   WINAPI ItemMonikerROTDataImpl_Release(IROTData* iface);

/* IROTData prototype function */
static HRESULT WINAPI ItemMonikerROTDataImpl_GetComparaisonData(IROTData* iface,BYTE* pbData,ULONG cbMax,ULONG* pcbData);

/********************************************************************************/
/* Virtual function table for the ItemMonikerImpl class which  include IPersist,*/
/* IPersistStream and IMoniker functions.                                       */
static ICOM_VTABLE(IMoniker) VT_ItemMonikerImpl =
    {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    ItemMonikerImpl_QueryInterface,
    ItemMonikerImpl_AddRef,
    ItemMonikerImpl_Release,
    ItemMonikerImpl_GetClassID,
    ItemMonikerImpl_IsDirty,
    ItemMonikerImpl_Load,
    ItemMonikerImpl_Save,
    ItemMonikerImpl_GetSizeMax,
    ItemMonikerImpl_BindToObject,
    ItemMonikerImpl_BindToStorage,
    ItemMonikerImpl_Reduce,
    ItemMonikerImpl_ComposeWith,
    ItemMonikerImpl_Enum,
    ItemMonikerImpl_IsEqual,
    ItemMonikerImpl_Hash,
    ItemMonikerImpl_IsRunning,
    ItemMonikerImpl_GetTimeOfLastChange,
    ItemMonikerImpl_Inverse,
    ItemMonikerImpl_CommonPrefixWith,
    ItemMonikerImpl_RelativePathTo,
    ItemMonikerImpl_GetDisplayName,
    ItemMonikerImpl_ParseDisplayName,
    ItemMonikerImpl_IsSystemMoniker
};

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static ICOM_VTABLE(IROTData) VT_ROTDataImpl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    ItemMonikerROTDataImpl_QueryInterface,
    ItemMonikerROTDataImpl_AddRef,
    ItemMonikerROTDataImpl_Release,
    ItemMonikerROTDataImpl_GetComparaisonData
};

/*******************************************************************************
 *        ItemMoniker_QueryInterface
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    ICOM_THIS(ItemMonikerImpl,iface);

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
  ItemMonikerImpl_AddRef(iface);

  return S_OK;
}

/******************************************************************************
 *        ItemMoniker_AddRef
 ******************************************************************************/
ULONG WINAPI ItemMonikerImpl_AddRef(IMoniker* iface)
{
    ICOM_THIS(ItemMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p)\n",This));

    return ++(This->ref);
}

/******************************************************************************
 *        ItemMoniker_Release
 ******************************************************************************/
ULONG WINAPI ItemMonikerImpl_Release(IMoniker* iface)
{
    ICOM_THIS(ItemMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p)\n",This));

    This->ref--;

    /* destroy the object if there's no more reference on it */
    if (This->ref==0){

        ItemMonikerImpl_Destroy(This);

        return 0;
    }
    return This->ref;;
}

/******************************************************************************
 *        ItemMoniker_GetClassID
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    Print(MAX_TRACE, ("(%p,%p),stub!\n",iface,pClassID));

    if (pClassID==NULL)
        return E_POINTER;
            
    *pClassID = CLSID_ItemMoniker;
        
    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_IsDirty
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    Print(MAX_TRACE, ("(%p)\n",iface));

    return S_FALSE;
}

/******************************************************************************
 *        ItemMoniker_Load
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Load(IMoniker* iface,IStream* pStm)
{

    ICOM_THIS(ItemMonikerImpl,iface);
    HRESULT res;
    DWORD delimiterLength,nameLength,lenW;
    CHAR *itemNameA,*itemDelimiterA;
    ULONG bread;

    /* for more details about data read by this function see coments of ItemMonikerImpl_Save function */

    /* read item delimiter string length + 1 */
    res=IStream_Read(pStm,&delimiterLength,sizeof(DWORD),&bread);
    if (bread != sizeof(DWORD))
        return E_FAIL;

    /* read item delimiter string */
    if (!(itemDelimiterA=HeapAlloc(GetProcessHeap(),0,delimiterLength)))
        return E_OUTOFMEMORY;
    res=IStream_Read(pStm,itemDelimiterA,delimiterLength,&bread);
    if (bread != delimiterLength)
    {
        HeapFree( GetProcessHeap(), 0, itemDelimiterA );
        return E_FAIL;
    }

    lenW = MultiByteToWideChar( CP_ACP, 0, itemDelimiterA, -1, NULL, 0 );
    This->itemDelimiter=HeapReAlloc(GetProcessHeap(),0,This->itemDelimiter,lenW*sizeof(WCHAR));
    if (!This->itemDelimiter)
    {
        HeapFree( GetProcessHeap(), 0, itemDelimiterA );
        return E_OUTOFMEMORY;
    }
    MultiByteToWideChar( CP_ACP, 0, itemDelimiterA, -1, This->itemDelimiter, lenW );
    HeapFree( GetProcessHeap(), 0, itemDelimiterA );

    /* read item name string length + 1*/
    res=IStream_Read(pStm,&nameLength,sizeof(DWORD),&bread);
    if (bread != sizeof(DWORD))
        return E_FAIL;

    /* read item name string */
    if (!(itemNameA=HeapAlloc(GetProcessHeap(),0,nameLength)))
        return E_OUTOFMEMORY;
    res=IStream_Read(pStm,itemNameA,nameLength,&bread);
    if (bread != nameLength)
    {
        HeapFree( GetProcessHeap(), 0, itemNameA );
        return E_FAIL;
    }

    lenW = MultiByteToWideChar( CP_ACP, 0, itemNameA, -1, NULL, 0 );
    This->itemName=HeapReAlloc(GetProcessHeap(),0,This->itemName,lenW*sizeof(WCHAR));
    if (!This->itemName)
    {
        HeapFree( GetProcessHeap(), 0, itemNameA );
        return E_OUTOFMEMORY;
    }
    MultiByteToWideChar( CP_ACP, 0, itemNameA, -1, This->itemName, lenW );
    HeapFree( GetProcessHeap(), 0, itemNameA );

    return res;
}

/******************************************************************************
 *        ItemMoniker_Save
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Save(IMoniker* iface,
                                    IStream* pStm,/* pointer to the stream where the object is to be saved */
                                    BOOL fClearDirty)/* Specifies whether to clear the dirty flag */
{
    ICOM_THIS(ItemMonikerImpl,iface);
    HRESULT res;
    CHAR *itemNameA,*itemDelimiterA;

    /* data writen by this function are : 1) DWORD : size of item delimiter string ('\0' included ) */
    /*                                    2) String (type A): item delimiter string ('\0' included)          */
    /*                                    3) DWORD : size of item name string ('\0' included)       */
    /*                                    4) String (type A): item name string ('\0' included)               */

    DWORD nameLength = WideCharToMultiByte( CP_ACP, 0, This->itemName, -1, NULL, 0, NULL, NULL);
    DWORD delimiterLength = WideCharToMultiByte( CP_ACP, 0, This->itemDelimiter, -1, NULL, 0, NULL, NULL);
    itemNameA=HeapAlloc(GetProcessHeap(),0,nameLength);
    itemDelimiterA=HeapAlloc(GetProcessHeap(),0,delimiterLength);
    WideCharToMultiByte( CP_ACP, 0, This->itemName, -1, itemNameA, nameLength, NULL, NULL);
    WideCharToMultiByte( CP_ACP, 0, This->itemDelimiter, -1, itemDelimiterA, delimiterLength, NULL, NULL);

    res=IStream_Write(pStm,&delimiterLength,sizeof(DWORD),NULL);
    res=IStream_Write(pStm,itemDelimiterA,delimiterLength * sizeof(CHAR),NULL);
    res=IStream_Write(pStm,&nameLength,sizeof(DWORD),NULL);
    res=IStream_Write(pStm,itemNameA,nameLength * sizeof(CHAR),NULL);

    return res;
}

/******************************************************************************
 *        ItemMoniker_GetSizeMax
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetSizeMax(IMoniker* iface,
                                          ULARGE_INTEGER* pcbSize)/* Pointer to size of stream needed to save object */
{
    ICOM_THIS(ItemMonikerImpl,iface);
    DWORD delimiterLength=lstrlenW(This->itemDelimiter)+1;
    DWORD nameLength=lstrlenW(This->itemName)+1;

    Print(MAX_TRACE, ("(%p,%p)\n",iface,pcbSize));

    if (pcbSize!=NULL)
        return E_POINTER;

    /* for more details see ItemMonikerImpl_Save coments */
    
    pcbSize->u.LowPart =  sizeof(DWORD) + /* DWORD which contains delimiter length */
                        delimiterLength + /* item delimiter string */
                        sizeof(DWORD) + /* DWORD which contains item name length */
                        nameLength + /* item name string */
                        34; /* this constant was added ! because when I tested this function it usually */
                            /*  returns 34 bytes more than the number of bytes used by IMoniker::Save function */
    pcbSize->u.HighPart=0;

    return S_OK;
}

/******************************************************************************
 *         ItemMoniker_Construct (local function)
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Construct(ItemMonikerImpl* This, LPCOLESTR lpszDelim,LPCOLESTR lpszItem)
{

    int sizeStr1=lstrlenW(lpszItem), sizeStr2;
    static const OLECHAR emptystr[1];
    LPCOLESTR	delim;

    Print(MAX_TRACE, ("(%p,%p)\n",This,lpszItem));

    /* Initialize the virtual fgunction table. */
    This->lpvtbl1      = &VT_ItemMonikerImpl;
    This->lpvtbl2      = &VT_ROTDataImpl;
    This->ref          = 0;

    This->itemName=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(sizeStr1+1));
    if (!This->itemName)
	return E_OUTOFMEMORY;
    lstrcpyW(This->itemName,lpszItem);

    if (!lpszDelim)
	Print(MIN_TRACE, ("lpszDelim is NULL. Using empty string which is possibly wrong.\n"));

    delim = lpszDelim ? lpszDelim : emptystr;

    sizeStr2=lstrlenW(delim);
    This->itemDelimiter=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(sizeStr2+1));
    if (!This->itemDelimiter) {
	HeapFree(GetProcessHeap(),0,This->itemName);
	return E_OUTOFMEMORY;
    }
    lstrcpyW(This->itemDelimiter,delim);
    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_Destroy (local function)
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Destroy(ItemMonikerImpl* This)
{
    Print(MAX_TRACE, ("(%p)\n",This));

    if (This->itemName)
        HeapFree(GetProcessHeap(),0,This->itemName);

    if (This->itemDelimiter)
        HeapFree(GetProcessHeap(),0,This->itemDelimiter);

    HeapFree(GetProcessHeap(),0,This);

    return S_OK;
}

/******************************************************************************
 *                  ItemMoniker_BindToObject
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_BindToObject(IMoniker* iface,
                                            IBindCtx* pbc,
                                            IMoniker* pmkToLeft,
                                            REFIID riid,
                                            VOID** ppvResult)
{
    ICOM_THIS(ItemMonikerImpl,iface);
    
    HRESULT   res;
    IID    refid=IID_IOleItemContainer;
    IOleItemContainer *poic=0;

    Print(MAX_TRACE, ("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult));

    if(ppvResult ==NULL)
        return E_POINTER;

    if(pmkToLeft==NULL)
        return E_INVALIDARG;

    *ppvResult=0;

    res=IMoniker_BindToObject(pmkToLeft,pbc,NULL,&refid,(void**)&poic);

    if (SUCCEEDED(res)){

        res=IOleItemContainer_GetObject(poic,This->itemName,BINDSPEED_MODERATE,pbc,riid,ppvResult);

        IOleItemContainer_Release(poic);
    }

    return res;
}

/******************************************************************************
 *        ItemMoniker_BindToStorage
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_BindToStorage(IMoniker* iface,
                                             IBindCtx* pbc,
                                             IMoniker* pmkToLeft,
                                             REFIID riid,
                                             VOID** ppvResult)
{
    ICOM_THIS(ItemMonikerImpl,iface);

    HRESULT   res;
    IOleItemContainer *poic=0;

    Print(MAX_TRACE, ("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult));

    *ppvResult=0;

    if(pmkToLeft==NULL)
        return E_INVALIDARG;
        
    res=IMoniker_BindToObject(pmkToLeft,pbc,NULL,&IID_IOleItemContainer,(void**)&poic);

    if (SUCCEEDED(res)){

        res=IOleItemContainer_GetObjectStorage(poic,This->itemName,pbc,riid,ppvResult);

        IOleItemContainer_Release(poic);
    }

    return res;
}

/******************************************************************************
 *        ItemMoniker_Reduce
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Reduce(IMoniker* iface,
                                      IBindCtx* pbc,
                                      DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft,
                                      IMoniker** ppmkReduced)
{
    Print(MAX_TRACE, ("(%p,%p,%ld,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced));

    if (ppmkReduced==NULL)
        return E_POINTER;

    ItemMonikerImpl_AddRef(iface);

    *ppmkReduced=iface;
    
    return MK_S_REDUCED_TO_SELF;
}
/******************************************************************************
 *        ItemMoniker_ComposeWith
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_ComposeWith(IMoniker* iface,
                                           IMoniker* pmkRight,
                                           BOOL fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite)
{
    HRESULT res=S_OK;
    DWORD mkSys,mkSys2;
    IEnumMoniker* penumMk=0;
    IMoniker *pmostLeftMk=0;
    IMoniker* tempMkComposite=0;

    Print(MAX_TRACE, ("(%p,%p,%d,%p)\n",iface,pmkRight,fOnlyIfNotGeneric,ppmkComposite));

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
 *        ItemMoniker_Enum
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    Print(MAX_TRACE, ("(%p,%d,%p)\n",iface,fForward,ppenumMoniker));

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_IsEqual
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{

    CLSID clsid;
    LPOLESTR dispName1,dispName2;
    IBindCtx* bind;
    HRESULT res;

    Print(MAX_TRACE, ("(%p,%p)\n",iface,pmkOtherMoniker));

    if (pmkOtherMoniker==NULL)
        return S_FALSE;
    
    /* This method returns S_OK if both monikers are item monikers and their display names are */
    /* identical (using a case-insensitive comparison); otherwise, the method returns S_FALSE. */

    IMoniker_GetClassID(pmkOtherMoniker,&clsid);

    if (!IsEqualCLSID(&clsid,&CLSID_ItemMoniker))
        return S_FALSE;

    res=CreateBindCtx(0,&bind);
    if (FAILED(res))
        return res;

    IMoniker_GetDisplayName(iface,bind,NULL,&dispName1);
    IMoniker_GetDisplayName(pmkOtherMoniker,bind,NULL,&dispName2);
    
    if (lstrcmpW(dispName1,dispName2)!=0)
        return S_FALSE;
    
    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_Hash
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    ICOM_THIS(ItemMonikerImpl,iface);

    int  h = 0,i,skip,len;
    int  off = 0;
    LPOLESTR val;

    if (pdwHash==NULL)
        return E_POINTER;
    
    val =  This->itemName;
    len = lstrlenW(val);

    if (len < 16) {
        for (i = len ; i > 0; i--) {
            h = (h * 37) + val[off++];
}
    } else {
        /* only sample some characters */
 	skip = len / 8;
 	for (i = len ; i > 0; i -= skip, off += skip) {
            h = (h * 39) + val[off];
 	}
    }

    *pdwHash=h;

    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_IsRunning
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsRunning(IMoniker* iface,
                                         IBindCtx* pbc,
                                         IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning)
{
    IRunningObjectTable* rot;
    HRESULT res;
    IOleItemContainer *poic=0;
    ICOM_THIS(ItemMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning));

    /* If pmkToLeft is NULL, this method returns TRUE if pmkNewlyRunning is non-NULL and is equal to this */
    /* moniker. Otherwise, the method checks the ROT to see whether this moniker is running.              */
    if (pmkToLeft==NULL)
        if ((pmkNewlyRunning!=NULL)&&(IMoniker_IsEqual(pmkNewlyRunning,iface)==S_OK))
            return S_OK;
        else {
            if (pbc==NULL)
                return E_POINTER;

            res=IBindCtx_GetRunningObjectTable(pbc,&rot);

            if (FAILED(res))
                return res;

            res = IRunningObjectTable_IsRunning(rot,iface);

            IRunningObjectTable_Release(rot);
        }
    else{

        /* If pmkToLeft is non-NULL, the method calls IMoniker::BindToObject on the pmkToLeft parameter,         */
        /* requesting an IOleItemContainer interface pointer. The method then calls IOleItemContainer::IsRunning,*/
        /* passing the string contained within this moniker. */
        
        res=IMoniker_BindToObject(pmkToLeft,pbc,NULL,&IID_IOleItemContainer,(void**)&poic);

        if (SUCCEEDED(res)){

            res=IOleItemContainer_IsRunning(poic,This->itemName);

            IOleItemContainer_Release(poic);
        }
    }

    return res;
}

/******************************************************************************
 *        ItemMoniker_GetTimeOfLastChange
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pItemTime)
{
    IRunningObjectTable* rot;
    HRESULT res;
    IMoniker *compositeMk;

    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pItemTime));

    if (pItemTime==NULL)
        return E_INVALIDARG;

    /* If pmkToLeft is NULL, this method returns MK_E_NOTBINDABLE */
    if (pmkToLeft==NULL)

        return MK_E_NOTBINDABLE;
    else {

        /* Otherwise, the method creates a composite of pmkToLeft and this moniker and uses the ROT to access  */
        /* the time of last change. If the object is not in the ROT, the method calls                          */
        /* IMoniker::GetTimeOfLastChange on the pmkToLeft parameter.                                            */

        res=CreateGenericComposite(pmkToLeft,iface,&compositeMk);

        res=IBindCtx_GetRunningObjectTable(pbc,&rot);

        if (IRunningObjectTable_GetTimeOfLastChange(rot,compositeMk,pItemTime)!=S_OK)

            res=IMoniker_GetTimeOfLastChange(pmkToLeft,pbc,NULL,pItemTime);

        IMoniker_Release(compositeMk);
}

    return res;
}

/******************************************************************************
 *        ItemMoniker_Inverse
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    Print(MAX_TRACE, ("(%p,%p)\n",iface,ppmk));

    if (ppmk==NULL)
        return E_POINTER;

    return CreateAntiMoniker(ppmk);
}

/******************************************************************************
 *        ItemMoniker_CommonPrefixWith
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    DWORD mkSys;
    IMoniker_IsSystemMoniker(pmkOther,&mkSys);
    /* If the other moniker is an item moniker that is equal to this moniker, this method sets *ppmkPrefix */
    /* to this moniker and returns MK_S_US */

    if((mkSys==MKSYS_ITEMMONIKER) && (IMoniker_IsEqual(iface,pmkOther)==S_OK) ){

        *ppmkPrefix=iface;
        
        IMoniker_AddRef(iface);
        
        return MK_S_US;
    }
    else
        /* otherwise, the method calls the MonikerCommonPrefixWith function. This function correctly handles */
        /* the case where the other moniker is a generic composite. */
        return MonikerCommonPrefixWith(iface,pmkOther,ppmkPrefix);
}

/******************************************************************************
 *        ItemMoniker_RelativePathTo
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    Print(MAX_TRACE, ("(%p,%p,%p)\n",iface,pmOther,ppmkRelPath));

    if (ppmkRelPath==NULL)
        return E_POINTER;

    *ppmkRelPath=0;
    
    return MK_E_NOTBINDABLE;
}

/******************************************************************************
 *        ItemMoniker_GetDisplayName
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetDisplayName(IMoniker* iface,
                                              IBindCtx* pbc,
                                              IMoniker* pmkToLeft,
                                              LPOLESTR *ppszDisplayName)
{
    ICOM_THIS(ItemMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName));

    if (ppszDisplayName==NULL)
        return E_POINTER;

    if (pmkToLeft!=NULL){
        return E_INVALIDARG;
    }

    *ppszDisplayName=CoTaskMemAlloc(sizeof(WCHAR)*(lstrlenW(This->itemDelimiter)+lstrlenW(This->itemName)+1));

    if (*ppszDisplayName==NULL)
        return E_OUTOFMEMORY;

    lstrcpyW(*ppszDisplayName,This->itemDelimiter);
    lstrcatW(*ppszDisplayName,This->itemName);
    
    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_ParseDisplayName
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_ParseDisplayName(IMoniker* iface,
                                                IBindCtx* pbc,
                                                IMoniker* pmkToLeft,
                                                LPOLESTR pszDisplayName,
                                                ULONG* pchEaten,
                                                IMoniker** ppmkOut)
{
    IOleItemContainer* poic=0;
    IParseDisplayName* ppdn=0;
    LPOLESTR displayName;
    HRESULT res;
    ICOM_THIS(ItemMonikerImpl,iface);

    /* If pmkToLeft is NULL, this method returns MK_E_SYNTAX */
    if (pmkToLeft==NULL)

        return MK_E_SYNTAX;

    else{
        /* Otherwise, the method calls IMoniker::BindToObject on the pmkToLeft parameter, requesting an */
        /* IParseDisplayName interface pointer to the object identified by the moniker, and passes the display */        /* name to IParseDisplayName::ParseDisplayName */
        res=IMoniker_BindToObject(pmkToLeft,pbc,NULL,&IID_IOleItemContainer,(void**)&poic);

        if (SUCCEEDED(res)){

            res=IOleItemContainer_GetObject(poic,This->itemName,BINDSPEED_MODERATE,pbc,&IID_IParseDisplayName,(void**)&ppdn);

            res=IMoniker_GetDisplayName(iface,pbc,NULL,&displayName);

            res=IParseDisplayName_ParseDisplayName(ppdn,pbc,displayName,pchEaten,ppmkOut);

            IOleItemContainer_Release(poic);
            IParseDisplayName_Release(ppdn);
        }
}
    return res;
}

/******************************************************************************
 *        ItemMoniker_IsSystemMoniker
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    Print(MAX_TRACE, ("(%p,%p)\n",iface,pwdMksys));

    if (!pwdMksys)
        return E_POINTER;
    
    (*pwdMksys)=MKSYS_ITEMMONIKER;

    return S_OK;
}

/*******************************************************************************
 *        ItemMonikerIROTData_QueryInterface
 *******************************************************************************/
HRESULT WINAPI ItemMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{

    ICOM_THIS_From_IROTData(IMoniker, iface);

    Print(MAX_TRACE, ("(%p,%p,%p)\n",iface,riid,ppvObject));

    return ItemMonikerImpl_QueryInterface(This, riid, ppvObject);
}

/***********************************************************************
 *        ItemMonikerIROTData_AddRef
 */
ULONG   WINAPI ItemMonikerROTDataImpl_AddRef(IROTData *iface)
{
    ICOM_THIS_From_IROTData(IMoniker, iface);

    Print(MAX_TRACE, ("(%p)\n",iface));

    return ItemMonikerImpl_AddRef(This);
}

/***********************************************************************
 *        ItemMonikerIROTData_Release
 */
ULONG   WINAPI ItemMonikerROTDataImpl_Release(IROTData* iface)
{
    ICOM_THIS_From_IROTData(IMoniker, iface);
    
    Print(MAX_TRACE, ("(%p)\n",iface));

    return ItemMonikerImpl_Release(This);
}

/******************************************************************************
 *        ItemMonikerIROTData_GetComparaisonData
 ******************************************************************************/
HRESULT WINAPI ItemMonikerROTDataImpl_GetComparaisonData(IROTData* iface,
                                                         BYTE* pbData,
                                                         ULONG cbMax,
                                                         ULONG* pcbData)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateItemMoniker16	[OLE2.28]
 ******************************************************************************/
HRESULT WINAPI CreateItemMoniker16(LPCOLESTR16 lpszDelim,LPCOLESTR  lpszItem,LPMONIKER* ppmk)
{

    Print(MIN_TRACE, ("(%s,%p),stub!\n",lpszDelim,ppmk));
    *ppmk = NULL;
    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateItemMoniker	[OLE.55]
 ******************************************************************************/
HRESULT WINAPI CreateItemMoniker(LPCOLESTR lpszDelim,LPCOLESTR  lpszItem, LPMONIKER * ppmk)
{
    ItemMonikerImpl* newItemMoniker = 0;
    HRESULT        hr = S_OK;
    IID riid=IID_IMoniker;

    Print(MAX_TRACE, ("(%p,%p,%p)\n",lpszDelim,lpszItem,ppmk));

    newItemMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(ItemMonikerImpl));

    if (newItemMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    hr = ItemMonikerImpl_Construct(newItemMoniker,lpszDelim,lpszItem);

    if (FAILED(hr)){

        HeapFree(GetProcessHeap(),0,newItemMoniker);
    return hr;
    }

    return ItemMonikerImpl_QueryInterface((IMoniker*)newItemMoniker,&riid,(void**)ppmk);
}
