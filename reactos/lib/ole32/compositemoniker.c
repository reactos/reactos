/***************************************************************************************
 *	                      CompositeMonikers implementation
 *
 *               Copyright 1999  Noomen Hamza
 ***************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <compobj.h>
#include <storage32.h>

#include <debug.h>


#define  BLOCK_TAB_SIZE 5 /* represent the first size table and it's increment block size */

/* CompositeMoniker data structure */
typedef struct CompositeMonikerImpl{

    ICOM_VTABLE(IMoniker)*  lpvtbl1;  /* VTable relative to the IMoniker interface.*/

    /* The ROT (RunningObjectTable implementation) uses the IROTData interface to test whether 
     * two monikers are equal. That's whay IROTData interface is implemented by monikers.
     */
    ICOM_VTABLE(IROTData)*  lpvtbl2;  /* VTable relative to the IROTData interface.*/

    ULONG ref; /* reference counter for this object */

    IMoniker** tabMoniker; /* dynamaic table containing all components (monikers) of this composite moniker */

    ULONG    tabSize;      /* size of tabMoniker */

    ULONG    tabLastIndex;  /* first free index in tabMoniker */

} CompositeMonikerImpl;


/* EnumMoniker data structure */
typedef struct EnumMonikerImpl{

    ICOM_VFIELD(IEnumMoniker);  /* VTable relative to the IEnumMoniker interface.*/

    ULONG ref; /* reference counter for this object */

    IMoniker** tabMoniker; /* dynamic table containing the enumerated monikers */

    ULONG      tabSize; /* size of tabMoniker */

    ULONG      currentPos;  /* index pointer on the current moniker */

} EnumMonikerImpl;


/********************************************************************************/
/* CompositeMoniker prototype functions :                                       */

/* IUnknown prototype functions */
static HRESULT WINAPI CompositeMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI CompositeMonikerImpl_AddRef(IMoniker* iface);
static ULONG   WINAPI CompositeMonikerImpl_Release(IMoniker* iface);

/* IPersist prototype functions */
static HRESULT WINAPI CompositeMonikerImpl_GetClassID(IMoniker* iface, CLSID *pClassID);

/* IPersistStream prototype functions */
static HRESULT WINAPI CompositeMonikerImpl_IsDirty(IMoniker* iface);
static HRESULT WINAPI CompositeMonikerImpl_Load(IMoniker* iface, IStream* pStm);
static HRESULT WINAPI CompositeMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty);
static HRESULT WINAPI CompositeMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize);

/* IMoniker prototype functions */
static HRESULT WINAPI CompositeMonikerImpl_BindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI CompositeMonikerImpl_BindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI CompositeMonikerImpl_Reduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced);
static HRESULT WINAPI CompositeMonikerImpl_ComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite);
static HRESULT WINAPI CompositeMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker);
static HRESULT WINAPI CompositeMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker);
static HRESULT WINAPI CompositeMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash);
static HRESULT WINAPI CompositeMonikerImpl_IsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning);
static HRESULT WINAPI CompositeMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pCompositeTime);
static HRESULT WINAPI CompositeMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk);
static HRESULT WINAPI CompositeMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther, IMoniker** ppmkPrefix);
static HRESULT WINAPI CompositeMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath);
static HRESULT WINAPI CompositeMonikerImpl_GetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName);
static HRESULT WINAPI CompositeMonikerImpl_ParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut);
static HRESULT WINAPI CompositeMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys);

/********************************************************************************/
/* IROTData prototype functions                                                 */

/* IUnknown prototype functions */
static HRESULT WINAPI CompositeMonikerROTDataImpl_QueryInterface(IROTData* iface,REFIID riid,VOID** ppvObject);
static ULONG   WINAPI CompositeMonikerROTDataImpl_AddRef(IROTData* iface);
static ULONG   WINAPI CompositeMonikerROTDataImpl_Release(IROTData* iface);

/* IROTData prototype function */
static HRESULT WINAPI CompositeMonikerROTDataImpl_GetComparaisonData(IROTData* iface,BYTE* pbData,ULONG cbMax,ULONG* pcbData);

/* Local function used by CompositeMoniker implementation */
HRESULT WINAPI CompositeMonikerImpl_Construct(CompositeMonikerImpl* This,LPMONIKER pmkFirst, LPMONIKER pmkRest);
HRESULT WINAPI CompositeMonikerImpl_Destroy(CompositeMonikerImpl* iface);

/********************************************************************************/
/* IEnumMoniker prototype functions                                             */

/* IUnknown prototype functions */
static HRESULT WINAPI EnumMonikerImpl_QueryInterface(IEnumMoniker* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI EnumMonikerImpl_AddRef(IEnumMoniker* iface);
static ULONG   WINAPI EnumMonikerImpl_Release(IEnumMoniker* iface);

/* IEnumMoniker prototype functions */
static HRESULT WINAPI EnumMonikerImpl_Next(IEnumMoniker* iface,ULONG celt,IMoniker** rgelt,ULONG* pceltFetched);
static HRESULT WINAPI EnumMonikerImpl_Skip(IEnumMoniker* iface,ULONG celt);
static HRESULT WINAPI EnumMonikerImpl_Reset(IEnumMoniker* iface);
static HRESULT WINAPI EnumMonikerImpl_Clone(IEnumMoniker* iface,IEnumMoniker** ppenum);

HRESULT WINAPI EnumMonikerImpl_CreateEnumMoniker(IMoniker** tabMoniker,ULONG tabSize,ULONG currentPos,BOOL leftToRigth,IEnumMoniker ** ppmk);

/********************************************************************************/
/* Virtual function table for the CompositeMonikerImpl class which includes     */
/* IPersist, IPersistStream and IMoniker functions.                             */

static ICOM_VTABLE(IMoniker) VT_CompositeMonikerImpl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    CompositeMonikerImpl_QueryInterface,
    CompositeMonikerImpl_AddRef,
    CompositeMonikerImpl_Release,
    CompositeMonikerImpl_GetClassID,
    CompositeMonikerImpl_IsDirty,
    CompositeMonikerImpl_Load,
    CompositeMonikerImpl_Save,
    CompositeMonikerImpl_GetSizeMax,
    CompositeMonikerImpl_BindToObject,
    CompositeMonikerImpl_BindToStorage,
    CompositeMonikerImpl_Reduce,
    CompositeMonikerImpl_ComposeWith,
    CompositeMonikerImpl_Enum,
    CompositeMonikerImpl_IsEqual,
    CompositeMonikerImpl_Hash,
    CompositeMonikerImpl_IsRunning,
    CompositeMonikerImpl_GetTimeOfLastChange,
    CompositeMonikerImpl_Inverse,
    CompositeMonikerImpl_CommonPrefixWith,
    CompositeMonikerImpl_RelativePathTo,
    CompositeMonikerImpl_GetDisplayName,
    CompositeMonikerImpl_ParseDisplayName,
    CompositeMonikerImpl_IsSystemMoniker
};

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static ICOM_VTABLE(IROTData) VT_ROTDataImpl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    CompositeMonikerROTDataImpl_QueryInterface,
    CompositeMonikerROTDataImpl_AddRef,
    CompositeMonikerROTDataImpl_Release,
    CompositeMonikerROTDataImpl_GetComparaisonData
};

/********************************************************************************/
/* Virtual function table for the IROTData class                                */
static ICOM_VTABLE(IEnumMoniker) VT_EnumMonikerImpl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    EnumMonikerImpl_QueryInterface,
    EnumMonikerImpl_AddRef,
    EnumMonikerImpl_Release,
    EnumMonikerImpl_Next,
    EnumMonikerImpl_Skip,
    EnumMonikerImpl_Reset,
    EnumMonikerImpl_Clone
};

/*******************************************************************************
 *        CompositeMoniker_QueryInterface
 *******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    ICOM_THIS(CompositeMonikerImpl,iface);
  
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
    CompositeMonikerImpl_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_AddRef
 ******************************************************************************/
ULONG WINAPI CompositeMonikerImpl_AddRef(IMoniker* iface)
{
    ICOM_THIS(CompositeMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p)\n",This));

    return ++(This->ref);
}

/******************************************************************************
 *        CompositeMoniker_Release
 ******************************************************************************/
ULONG WINAPI CompositeMonikerImpl_Release(IMoniker* iface)
{
    ICOM_THIS(CompositeMonikerImpl,iface);
    ULONG i;
    
    Print(MAX_TRACE, ("(%p)\n",This));

    This->ref--;

    /* destroy the object if there's no more reference on it */
    if (This->ref==0){

        /* release all the components before destroying this object */
        for (i=0;i<This->tabLastIndex;i++)
            IMoniker_Release(This->tabMoniker[i]);
        
        CompositeMonikerImpl_Destroy(This);

        return 0;
    }
    return This->ref;;
}

/******************************************************************************
 *        CompositeMoniker_GetClassID
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    Print(MAX_TRACE, ("(%p,%p),stub!\n",iface,pClassID));

    if (pClassID==NULL)
        return E_POINTER;
            
    *pClassID = CLSID_CompositeMoniker;
        
    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_IsDirty
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    Print(MAX_TRACE, ("(%p)\n",iface));

    return S_FALSE;
}

/******************************************************************************
 *        CompositeMoniker_Load
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_Load(IMoniker* iface,IStream* pStm)
{
    HRESULT res;
    DWORD constant;
    CLSID clsid;
    WCHAR string[1]={0};
    
    ICOM_THIS(CompositeMonikerImpl,iface);    

    Print(MAX_TRACE, ("(%p,%p)\n",iface,pStm));

    /* this function call OleLoadFromStream function for each moniker within this object */

    /* read the a constant writen by CompositeMonikerImpl_Save (see CompositeMonikerImpl_Save for more details)*/
    res=IStream_Read(pStm,&constant,sizeof(DWORD),NULL);

    if (SUCCEEDED(res)&& constant!=3)
        return E_FAIL;

    while(1){
#if 0
        res=OleLoadFromStream(pStm,&IID_IMoniker,(void**)&This->tabMoniker[This->tabLastIndex]);
#endif
        res=ReadClassStm(pStm,&clsid);
        Print(MAX_TRACE, ("res=%ld",res));
        if (FAILED(res))
            break;

        if (IsEqualIID(&clsid,&CLSID_FileMoniker)){
            res=CreateFileMoniker(string,&This->tabMoniker[This->tabLastIndex]);
            if (FAILED(res))
                break;
            res=IMoniker_Load(This->tabMoniker[This->tabLastIndex],pStm);
            if (FAILED(res))
                break;
        }
        else if (IsEqualIID(&clsid,&CLSID_ItemMoniker)){
            CreateItemMoniker(string,string,&This->tabMoniker[This->tabLastIndex]);
            if (res!=S_OK)
                break;
            IMoniker_Load(This->tabMoniker[This->tabLastIndex],pStm);
            if (FAILED(res))
                break;
        }
        else if (IsEqualIID(&clsid,&CLSID_AntiMoniker)){
            CreateAntiMoniker(&This->tabMoniker[This->tabLastIndex]);
            if (FAILED(res))
                break;
            IMoniker_Load(This->tabMoniker[This->tabLastIndex],pStm);
            if (FAILED(res))
                break;
        }
        else if (IsEqualIID(&clsid,&CLSID_CompositeMoniker))
            return E_FAIL;

        else
        {
            Print(MIN_TRACE, ("()\n"));
            /* FIXME: To whoever wrote this code: It's either return or break. it cannot be both! */
            break;
            return E_NOTIMPL;
        }

        /* resize the table if needed */
        if (++This->tabLastIndex==This->tabSize){
                
            This->tabSize+=BLOCK_TAB_SIZE;
            This->tabMoniker=HeapReAlloc(GetProcessHeap(),0,This->tabMoniker,This->tabSize*sizeof(IMoniker));

            if (This->tabMoniker==NULL)
            return E_OUTOFMEMORY;
        }
    }

    return res;
}

/******************************************************************************
 *        CompositeMoniker_Save
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_Save(IMoniker* iface,IStream* pStm,BOOL fClearDirty)
{
    HRESULT res;
    IEnumMoniker *enumMk;
    IMoniker *pmk;
    DWORD constant=3;
    
    Print(MAX_TRACE, ("(%p,%p,%d)\n",iface,pStm,fClearDirty));

    /* this function call OleSaveToStream function for each moniker within this object */

    /* when I tested this function in windows system ! I usually found this constant in the begining of */
    /* the stream  I dont known why (there's no indication in specification) ! */
    res=IStream_Write(pStm,&constant,sizeof(constant),NULL);

    IMoniker_Enum(iface,TRUE,&enumMk);

    while(IEnumMoniker_Next(enumMk,1,&pmk,NULL)==S_OK){

        res=OleSaveToStream((IPersistStream*)pmk,pStm);

        IMoniker_Release(pmk);

        if (FAILED(res)){

            IEnumMoniker_Release(pmk);
            return res;
        }
    }

    IEnumMoniker_Release(enumMk);

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_GetSizeMax
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_GetSizeMax(IMoniker* iface,ULARGE_INTEGER* pcbSize)
{
    IEnumMoniker *enumMk;
    IMoniker *pmk;
    ULARGE_INTEGER ptmpSize;

    /* the sizeMax of this object is calculated by calling  GetSizeMax on each moniker within this object then */
    /* suming all returned sizemax */

    Print(MAX_TRACE, ("(%p,%p)\n",iface,pcbSize));

    if (pcbSize!=NULL)
        return E_POINTER;

    pcbSize->u.LowPart =0;
    pcbSize->u.HighPart=0;

    IMoniker_Enum(iface,TRUE,&enumMk);

    while(IEnumMoniker_Next(enumMk,1,&pmk,NULL)==TRUE){

        IMoniker_GetSizeMax(pmk,&ptmpSize);

        IMoniker_Release(pmk);

        pcbSize->u.LowPart +=ptmpSize.u.LowPart;
        pcbSize->u.HighPart+=ptmpSize.u.HighPart;
    }

    IEnumMoniker_Release(enumMk);

    return S_OK;
}

/******************************************************************************
 *         Composite-Moniker_Construct (local function)
 *******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_Construct(CompositeMonikerImpl* This,LPMONIKER pmkFirst, LPMONIKER pmkRest)
{
    DWORD mkSys;
    IEnumMoniker *enumMoniker;
    IMoniker *tempMk;
    HRESULT res;
    
    Print(MAX_TRACE, ("(%p,%p,%p)\n",This,pmkFirst,pmkRest));

    /* Initialize the virtual fgunction table. */
    This->lpvtbl1      = &VT_CompositeMonikerImpl;
    This->lpvtbl2      = &VT_ROTDataImpl;
    This->ref          = 0;

    This->tabSize=BLOCK_TAB_SIZE;
    This->tabLastIndex=0;

    This->tabMoniker=HeapAlloc(GetProcessHeap(),0,This->tabSize*sizeof(IMoniker));
    if (This->tabMoniker==NULL)
        return E_OUTOFMEMORY;

    IMoniker_IsSystemMoniker(pmkFirst,&mkSys);

    /* put the first moniker contents in the begining of the table */
    if (mkSys!=MKSYS_GENERICCOMPOSITE){

        This->tabMoniker[(This->tabLastIndex)++]=pmkFirst;
        IMoniker_AddRef(pmkFirst);
    }
    else{

        IMoniker_Enum(pmkFirst,TRUE,&enumMoniker);
        
        while(IEnumMoniker_Next(enumMoniker,1,&This->tabMoniker[This->tabLastIndex],NULL)==S_OK){


            if (++This->tabLastIndex==This->tabSize){
                
                This->tabSize+=BLOCK_TAB_SIZE;
                This->tabMoniker=HeapReAlloc(GetProcessHeap(),0,This->tabMoniker,This->tabSize*sizeof(IMoniker));

                if (This->tabMoniker==NULL)
                    return E_OUTOFMEMORY;
            }
        }

        IEnumMoniker_Release(enumMoniker);
    }

    /* put the rest moniker contents after the first one and make simplification if needed */

    IMoniker_IsSystemMoniker(pmkRest,&mkSys);
    
    if (mkSys!=MKSYS_GENERICCOMPOSITE){

        /* add a simple moniker to the moniker table */

        res=IMoniker_ComposeWith(This->tabMoniker[This->tabLastIndex-1],pmkRest,TRUE,&tempMk);

        if (res==MK_E_NEEDGENERIC){ 

            /* there's no simplification in this case */
            This->tabMoniker[This->tabLastIndex]=pmkRest;

            This->tabLastIndex++;

            IMoniker_AddRef(pmkRest);
        }
        else if (tempMk==NULL){ 

            /* we have an antimoniker after a simple moniker so we can make a simplification in this case */
            IMoniker_Release(This->tabMoniker[This->tabLastIndex-1]);

            This->tabLastIndex--;
        }
        else if (SUCCEEDED(res)){

            /* the non-generic composition was successful so we can make a simplification in this case */
            IMoniker_Release(This->tabMoniker[This->tabLastIndex-1]);

            This->tabMoniker[This->tabLastIndex-1]=tempMk;
        } else
            return res;

        /* resize tabMoniker if needed */
        if (This->tabLastIndex==This->tabSize){
                
            This->tabSize+=BLOCK_TAB_SIZE;

            This->tabMoniker=HeapReAlloc(GetProcessHeap(),0,This->tabMoniker,This->tabSize*sizeof(IMoniker));

            if (This->tabMoniker==NULL)
            return E_OUTOFMEMORY;
        }
    }
    else{

        /* add a composite moniker to the moniker table (do the same thing for each moniker within the */
        /* composite moniker as a simple moniker (see above how to add a simple moniker case) ) */
        IMoniker_Enum(pmkRest,TRUE,&enumMoniker);

        while(IEnumMoniker_Next(enumMoniker,1,&This->tabMoniker[This->tabLastIndex],NULL)==S_OK){

            res=IMoniker_ComposeWith(This->tabMoniker[This->tabLastIndex-1],This->tabMoniker[This->tabLastIndex],TRUE,&tempMk);

            if (res==MK_E_NEEDGENERIC){

                This->tabLastIndex++;
            }
            else if (tempMk==NULL){

                IMoniker_Release(This->tabMoniker[This->tabLastIndex-1]);
                IMoniker_Release(This->tabMoniker[This->tabLastIndex]);
                This->tabLastIndex--;
            }
            else{

                IMoniker_Release(This->tabMoniker[This->tabLastIndex-1]);

                This->tabMoniker[This->tabLastIndex-1]=tempMk;
            }

            if (This->tabLastIndex==This->tabSize){
                
                This->tabSize+=BLOCK_TAB_SIZE;

                This->tabMoniker=HeapReAlloc(GetProcessHeap(),0,This->tabMoniker,This->tabSize*sizeof(IMoniker));

                if (This->tabMoniker==NULL)
                    return E_OUTOFMEMORY;
            }
        }

        IEnumMoniker_Release(enumMoniker);
    }

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_Destroy (local function)
 *******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_Destroy(CompositeMonikerImpl* This)
{
    Print(MAX_TRACE, ("(%p)\n",This));

    HeapFree(GetProcessHeap(),0,This->tabMoniker);

    HeapFree(GetProcessHeap(),0,This);

    return S_OK;
}

/******************************************************************************
 *                  CompositeMoniker_BindToObject
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_BindToObject(IMoniker* iface,
                                                 IBindCtx* pbc,
                                                 IMoniker* pmkToLeft,
                                                 REFIID riid,
                                                 VOID** ppvResult)
{
    HRESULT   res;
    IRunningObjectTable *prot;
    IMoniker *tempMk,*antiMk,*mostRigthMk;
    IEnumMoniker *enumMoniker;
    
    Print(MAX_TRACE, ("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult));

    if (ppvResult==NULL)
        return E_POINTER;
    
    *ppvResult=0;
    /* If pmkToLeft is NULL, this method looks for the moniker in the ROT, and if found, queries the retrieved */
    /* object for the requested interface pointer. */
    if(pmkToLeft==NULL){

        res=IBindCtx_GetRunningObjectTable(pbc,&prot);

        if (SUCCEEDED(res)){

            /* if the requested class was loaded befor ! we dont need to reload it */
            res = IRunningObjectTable_GetObject(prot,iface,(IUnknown**)ppvResult);

            if (res==S_OK)
                return res;
        }
    }
    else{
        /* If pmkToLeft is not NULL, the method recursively calls IMoniker::BindToObject on the rightmost */
        /* component of the composite, passing the rest of the composite as the pmkToLeft parameter for that call */

        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&mostRigthMk,NULL);
        IEnumMoniker_Release(enumMoniker);
        
        res=CreateAntiMoniker(&antiMk);
        res=IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
        IMoniker_Release(antiMk);
        
        res=CompositeMonikerImpl_BindToObject(mostRigthMk,pbc,tempMk,riid,ppvResult);

        IMoniker_Release(tempMk);
        IMoniker_Release(mostRigthMk);
    }

    return res;
}

/******************************************************************************
 *        CompositeMoniker_BindToStorage
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_BindToStorage(IMoniker* iface,
                                                  IBindCtx* pbc,
                                                  IMoniker* pmkToLeft,
                                                  REFIID riid,
                                                  VOID** ppvResult)
{
    HRESULT   res;
    IMoniker *tempMk,*antiMk,*mostRigthMk;
    IEnumMoniker *enumMoniker;

    Print(MAX_TRACE, ("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult));

    *ppvResult=0;

    /* This method recursively calls BindToStorage on the rightmost component of the composite, */
    /* passing the rest of the composite as the pmkToLeft parameter for that call. */

    if (pmkToLeft!=NULL){
        
        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&mostRigthMk,NULL);
        IEnumMoniker_Release(enumMoniker);
        
        res=CreateAntiMoniker(&antiMk);
        res=IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
        IMoniker_Release(antiMk);
        
        res=CompositeMonikerImpl_BindToStorage(mostRigthMk,pbc,tempMk,riid,ppvResult);

        IMoniker_Release(tempMk);

        IMoniker_Release(mostRigthMk);

        return res;
    }
    else
        return IMoniker_BindToStorage(iface,pbc,NULL,riid,ppvResult);
}

/******************************************************************************
 *        CompositeMoniker_Reduce
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_Reduce(IMoniker* iface,
                                           IBindCtx* pbc,
                                           DWORD dwReduceHowFar,
                                           IMoniker** ppmkToLeft,
                                           IMoniker** ppmkReduced)
{
    HRESULT   res;
    IMoniker *tempMk,*antiMk,*mostRigthMk,*leftReducedComposedMk,*mostRigthReducedMk;
    IEnumMoniker *enumMoniker;

    Print(MAX_TRACE, ("(%p,%p,%ld,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced));

    if (ppmkReduced==NULL)
        return E_POINTER;

    /* This method recursively calls Reduce for each of its component monikers. */

    if (ppmkToLeft==NULL){

        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&mostRigthMk,NULL);
        IEnumMoniker_Release(enumMoniker);
        
        res=CreateAntiMoniker(&antiMk);
        res=IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
        IMoniker_Release(antiMk);

        return CompositeMonikerImpl_Reduce(mostRigthMk,pbc,dwReduceHowFar,&tempMk, ppmkReduced);
    }
    else if (*ppmkToLeft==NULL)

        return IMoniker_Reduce(iface,pbc,dwReduceHowFar,NULL,ppmkReduced);

    else{

        /* separate the composite moniker in to left and right moniker */
        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&mostRigthMk,NULL);
        IEnumMoniker_Release(enumMoniker);
        
        res=CreateAntiMoniker(&antiMk);
        res=IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
        IMoniker_Release(antiMk);

        /* If any of the components  reduces itself, the method returns S_OK and passes back a composite */
        /* of the reduced components */
        if (IMoniker_Reduce(mostRigthMk,pbc,dwReduceHowFar,NULL,&mostRigthReducedMk) &&
            CompositeMonikerImpl_Reduce(mostRigthMk,pbc,dwReduceHowFar,&tempMk,&leftReducedComposedMk)
           )

            return CreateGenericComposite(leftReducedComposedMk,mostRigthReducedMk,ppmkReduced);

        else{
            /* If no reduction occurred, the method passes back the same moniker and returns MK_S_REDUCED_TO_SELF.*/

            IMoniker_AddRef(iface);

            *ppmkReduced=iface;

            return MK_S_REDUCED_TO_SELF;
        }
    }
}

/******************************************************************************
 *        CompositeMoniker_ComposeWith
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_ComposeWith(IMoniker* iface,
                                                IMoniker* pmkRight,
                                                BOOL fOnlyIfNotGeneric,
                                                IMoniker** ppmkComposite)
{
    Print(MAX_TRACE, ("(%p,%p,%d,%p)\n",iface,pmkRight,fOnlyIfNotGeneric,ppmkComposite));

    if ((ppmkComposite==NULL)||(pmkRight==NULL))
	return E_POINTER;

    *ppmkComposite=0;

    /* If fOnlyIfNotGeneric is TRUE, this method sets *pmkComposite to NULL and returns MK_E_NEEDGENERIC; */
    /* otherwise, the method returns the result of combining the two monikers by calling the */
    /* CreateGenericComposite function */
    
    if (fOnlyIfNotGeneric)
        return MK_E_NEEDGENERIC;
    
    return CreateGenericComposite(iface,pmkRight,ppmkComposite);
}

/******************************************************************************
 *        CompositeMoniker_Enum
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    ICOM_THIS(CompositeMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p,%d,%p)\n",iface,fForward,ppenumMoniker));

    if (ppenumMoniker == NULL)
        return E_POINTER;
    
    return EnumMonikerImpl_CreateEnumMoniker(This->tabMoniker,This->tabLastIndex,0,fForward,ppenumMoniker);
}

/******************************************************************************
 *        CompositeMoniker_IsEqual
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    IEnumMoniker *enumMoniker1,*enumMoniker2;
    IMoniker *tempMk1,*tempMk2;
    HRESULT res1,res2,res;
    
    Print(MAX_TRACE, ("(%p,%p)\n",iface,pmkOtherMoniker));

    if (pmkOtherMoniker==NULL)
        return S_FALSE;

    /* This method returns S_OK if the components of both monikers are equal when compared in the */
    /* left-to-right order.*/
    IMoniker_Enum(pmkOtherMoniker,TRUE,&enumMoniker1);

    if (enumMoniker1==NULL)
        return S_FALSE;
    
    IMoniker_Enum(iface,TRUE,&enumMoniker2);

    while(1){

        res1=IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);
        res2=IEnumMoniker_Next(enumMoniker2,1,&tempMk2,NULL);
        
        if((res1==S_OK)&&(res2==S_OK)){

            if(IMoniker_IsEqual(tempMk1,tempMk2)==S_FALSE){
                res= S_FALSE;
                break;
            }
            else
                continue;
        }
        else if ( (res1==S_FALSE) && (res2==S_FALSE) ){
                res = S_OK;
                break;
        }
        else{
            res = S_FALSE;
            break;
        }

        if (res1==S_OK)
            IMoniker_Release(tempMk1);

        if (res2==S_OK)
            IMoniker_Release(tempMk2);
    }

    IEnumMoniker_Release(enumMoniker1);
    IEnumMoniker_Release(enumMoniker2);

    return res;
}
/******************************************************************************
 *        CompositeMoniker_Hash
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    Print(MIN_TRACE, ("(),stub!\n"));

    return E_NOTIMPL;
}

/******************************************************************************
 *        CompositeMoniker_IsRunning
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_IsRunning(IMoniker* iface,
                                              IBindCtx* pbc,
                                              IMoniker* pmkToLeft,
                                              IMoniker* pmkNewlyRunning)
{
    IRunningObjectTable* rot;
    HRESULT res;
    IMoniker *tempMk,*antiMk,*mostRigthMk;
    IEnumMoniker *enumMoniker;

    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning));

    /* If pmkToLeft is non-NULL, this method composes pmkToLeft with this moniker and calls IsRunning on the result.*/
    if (pmkToLeft!=NULL){

        CreateGenericComposite(pmkToLeft,iface,&tempMk);

        res = IMoniker_IsRunning(tempMk,pbc,NULL,pmkNewlyRunning);

        IMoniker_Release(tempMk);

        return res;
    }
    else
        /* If pmkToLeft is NULL, this method returns S_OK if pmkNewlyRunning is non-NULL and is equal */
        /* to this moniker */
        
        if (pmkNewlyRunning!=NULL)

            if (IMoniker_IsEqual(iface,pmkNewlyRunning)==S_OK)
                return S_OK;

            else
                return S_FALSE;

        else{

            if (pbc==NULL)
                return E_POINTER;

            /* If pmkToLeft and pmkNewlyRunning are both NULL, this method checks the ROT to see whether */
            /* the moniker is running. If so, the method returns S_OK; otherwise, it recursively calls   */
            /* IMoniker::IsRunning on the rightmost component of the composite, passing the remainder of */
            /* the composite as the pmkToLeft parameter for that call.                                   */
            
             res=IBindCtx_GetRunningObjectTable(pbc,&rot);

            if (FAILED(res))
                return res;

            res = IRunningObjectTable_IsRunning(rot,iface);
            IRunningObjectTable_Release(rot);

            if(res==S_OK)
                return S_OK;

            else{

                IMoniker_Enum(iface,FALSE,&enumMoniker);
                IEnumMoniker_Next(enumMoniker,1,&mostRigthMk,NULL);
                IEnumMoniker_Release(enumMoniker);

                res=CreateAntiMoniker(&antiMk);
                res=IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
                IMoniker_Release(antiMk);

                res=IMoniker_IsRunning(mostRigthMk,pbc,tempMk,pmkNewlyRunning);

                IMoniker_Release(tempMk);
                IMoniker_Release(mostRigthMk);

                return res;
            }
        }
}

/******************************************************************************
 *        CompositeMoniker_GetTimeOfLastChange
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
                                                        IBindCtx* pbc,
                                                        IMoniker* pmkToLeft,
                                                        FILETIME* pCompositeTime)
{
    IRunningObjectTable* rot;
    HRESULT res;
    IMoniker *tempMk,*antiMk,*mostRigthMk;
    IEnumMoniker *enumMoniker;
    
    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pCompositeTime));

    if (pCompositeTime==NULL)
        return E_INVALIDARG;

    /* This method creates a composite of pmkToLeft (if non-NULL) and this moniker and uses the ROT to  */
    /* retrieve the time of last change. If the object is not in the ROT, the method recursively calls  */
    /* IMoniker::GetTimeOfLastChange on the rightmost component of the composite, passing the remainder */
    /* of the composite as the pmkToLeft parameter for that call.                                       */
    if (pmkToLeft!=NULL){

        res=CreateGenericComposite(pmkToLeft,iface,&tempMk);

        res=IBindCtx_GetRunningObjectTable(pbc,&rot);

        if (FAILED(res))
            return res;

        if (IRunningObjectTable_GetTimeOfLastChange(rot,tempMk,pCompositeTime)==S_OK)
            return res;
        else

            IMoniker_Enum(iface,FALSE,&enumMoniker);
            IEnumMoniker_Next(enumMoniker,1,&mostRigthMk,NULL);
            IEnumMoniker_Release(enumMoniker);

            res=CreateAntiMoniker(&antiMk);
            res=IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
            IMoniker_Release(antiMk);

            res=CompositeMonikerImpl_GetTimeOfLastChange(mostRigthMk,pbc,tempMk,pCompositeTime);

            IMoniker_Release(tempMk);
            IMoniker_Release(mostRigthMk);

            return res;
    }
    else
        return IMoniker_GetTimeOfLastChange(iface,pbc,NULL,pCompositeTime);
}

/******************************************************************************
 *        CompositeMoniker_Inverse
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    HRESULT res;
    IMoniker *tempMk,*antiMk,*mostRigthMk,*tempInvMk,*mostRigthInvMk;
    IEnumMoniker *enumMoniker;

    Print(MAX_TRACE, ("(%p,%p)\n",iface,ppmk));

    if (ppmk==NULL)
        return E_POINTER;

    /* This method returns a composite moniker that consists of the inverses of each of the components */
    /* of the original composite, stored in reverse order */

    res=CreateAntiMoniker(&antiMk);
    res=IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
    IMoniker_Release(antiMk);

    if (tempMk==NULL)

        return IMoniker_Inverse(iface,ppmk);

    else{

        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&mostRigthMk,NULL);
        IEnumMoniker_Release(enumMoniker);

        IMoniker_Inverse(mostRigthMk,&mostRigthInvMk);
        CompositeMonikerImpl_Inverse(tempMk,&tempInvMk);

        res=CreateGenericComposite(mostRigthInvMk,tempInvMk,ppmk);

        IMoniker_Release(tempMk);
        IMoniker_Release(mostRigthMk);
        IMoniker_Release(tempInvMk);
        IMoniker_Release(mostRigthInvMk);

        return res;
    }
}

/******************************************************************************
 *        CompositeMoniker_CommonPrefixWith
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{
    DWORD mkSys;
    HRESULT res1,res2;
    IMoniker *tempMk1,*tempMk2,*mostLeftMk1,*mostLeftMk2;
    IEnumMoniker *enumMoniker1,*enumMoniker2;
    ULONG i,nbCommonMk=0;
    
    /* If the other moniker is a composite, this method compares the components of each composite from left  */
    /* to right. The returned common prefix moniker might also be a composite moniker, depending on how many */
    /* of the leftmost components were common to both monikers.                                              */

    if (ppmkPrefix==NULL)
        return E_POINTER;
    
    *ppmkPrefix=0;

    if (pmkOther==NULL)
        return MK_E_NOPREFIX;
    
    IMoniker_IsSystemMoniker(pmkOther,&mkSys);

    if((mkSys==MKSYS_GENERICCOMPOSITE)){

        IMoniker_Enum(iface,TRUE,&enumMoniker1);
        IMoniker_Enum(pmkOther,TRUE,&enumMoniker2);

        while(1){

            res1=IEnumMoniker_Next(enumMoniker1,1,&mostLeftMk1,NULL);
            res2=IEnumMoniker_Next(enumMoniker2,1,&mostLeftMk2,NULL);

            if ((res1==S_FALSE) && (res2==S_FALSE)){

                /* If the monikers are equal, the method returns MK_S_US and sets ppmkPrefix to this moniker.*/
                *ppmkPrefix=iface;
                IMoniker_AddRef(iface);
                return  MK_S_US;
            }
            else if ((res1==S_OK) && (res2==S_OK)){

                if (IMoniker_IsEqual(mostLeftMk1,mostLeftMk2)==S_OK)

                    nbCommonMk++;

                else
                    break;

            }
            else if (res1==S_OK){

                /* If the other moniker is a prefix of this moniker, the method returns MK_S_HIM and sets */
                /* ppmkPrefix to the other moniker.                                                       */
                *ppmkPrefix=pmkOther;
                return MK_S_HIM;
            }
            else{
                /* If this moniker is a prefix of the other, this method returns MK_S_ME and sets ppmkPrefix */
                /* to this moniker.                                                                          */
                *ppmkPrefix=iface;
                return MK_S_ME;
            }
        }

        IEnumMoniker_Release(enumMoniker1);
        IEnumMoniker_Release(enumMoniker2);

        /* If there is no common prefix, this method returns MK_E_NOPREFIX and sets ppmkPrefix to NULL. */
        if (nbCommonMk==0)
            return MK_E_NOPREFIX;

        IEnumMoniker_Reset(enumMoniker1);

        IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);

        /* if we have more than one commun moniker the result will be a composite moniker */
        if (nbCommonMk>1){

            /* initialize the common prefix moniker with the composite of two first moniker (from the left)*/
            IEnumMoniker_Next(enumMoniker1,1,&tempMk2,NULL);
            CreateGenericComposite(tempMk1,tempMk2,ppmkPrefix);
            IMoniker_Release(tempMk1);
            IMoniker_Release(tempMk2);
            
            /* compose all common monikers in a composite moniker */
            for(i=0;i<nbCommonMk;i++){

                IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);

                CreateGenericComposite(*ppmkPrefix,tempMk1,&tempMk2);

                IMoniker_Release(*ppmkPrefix);

                IMoniker_Release(tempMk1);
                
                *ppmkPrefix=tempMk2;
            }
            return S_OK;
        }
        else{
            /* if we have only one commun moniker the result will be a simple moniker which is the most-left one*/
            *ppmkPrefix=tempMk1;

            return S_OK;
        }
    }
    else{
        /* If the other moniker is not a composite, the method simply compares it to the leftmost component
         of this moniker.*/

        IMoniker_Enum(iface,TRUE,&enumMoniker1);

        IEnumMoniker_Next(enumMoniker1,1,&mostLeftMk1,NULL);

        if (IMoniker_IsEqual(pmkOther,mostLeftMk1)==S_OK){

            *ppmkPrefix=pmkOther;

            return MK_S_HIM;
        }
        else
            return MK_E_NOPREFIX;
    }
}
/***************************************************************************************************
 *        GetAfterCommonPrefix (local function)
 *  This function returns a moniker that consist of the remainder when the common prefix is removed
 ***************************************************************************************************/
VOID WINAPI GetAfterCommonPrefix(IMoniker* pGenMk,IMoniker* commonMk,IMoniker** restMk)
{
    IMoniker *tempMk,*tempMk1,*tempMk2;
    IEnumMoniker *enumMoniker1,*enumMoniker2,*enumMoniker3;
    ULONG nbRestMk=0;
    DWORD mkSys;
    HRESULT res1,res2;
    
    *restMk=0;

    /* to create an enumerator for pGenMk with current position pointed on the first element after common  */
    /* prefix: enum the two monikers (left-right) then compare these enumerations (left-right) and stop  */
    /* on the first difference. */
    IMoniker_Enum(pGenMk,TRUE,&enumMoniker1);

    IMoniker_IsSystemMoniker(commonMk,&mkSys);

    if (mkSys==MKSYS_GENERICCOMPOSITE){

        IMoniker_Enum(commonMk,TRUE,&enumMoniker2);
        while(1){

            res1=IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);
            res2=IEnumMoniker_Next(enumMoniker2,1,&tempMk2,NULL);

            if ((res1==S_FALSE)||(res2==S_FALSE)){

                if (res1==S_OK)

                    nbRestMk++;

                IMoniker_Release(tempMk1);
                IMoniker_Release(tempMk1);

                break;
            }
            IMoniker_Release(tempMk1);
            IMoniker_Release(tempMk1);
        }
    }
    else{
        IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);
        IMoniker_Release(tempMk1);
    }

    /* count the number of elements in the enumerator after the common prefix */
    IEnumMoniker_Clone(enumMoniker1,&enumMoniker3);

    for(;IEnumMoniker_Next(enumMoniker3,1,&tempMk,NULL)==S_OK;nbRestMk++)

        IMoniker_Release(tempMk);;

    if (nbRestMk==0)
        return;

    /* create a generic composite moniker with monikers located after the common prefix */
    IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);

    if (nbRestMk==1){

        *restMk= tempMk1;
        return;
    }
    else {

        IEnumMoniker_Next(enumMoniker1,1,&tempMk2,NULL);

        CreateGenericComposite(tempMk1,tempMk2,restMk);

        IMoniker_Release(tempMk1);

        IMoniker_Release(tempMk2);

        while(IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL)==S_OK){

            CreateGenericComposite(*restMk,tempMk1,&tempMk2);

            IMoniker_Release(tempMk1);

            IMoniker_Release(*restMk);

            *restMk=tempMk2;
        }
    }
}
/******************************************************************************
 *        CompositeMoniker_RelativePathTo
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmkOther, IMoniker** ppmkRelPath)
{
    HRESULT res;
    IMoniker *restOtherMk=0,*restThisMk=0,*invRestThisMk=0,*commonMk=0;

    Print(MAX_TRACE, ("(%p,%p,%p)\n",iface,pmkOther,ppmkRelPath));

    if (ppmkRelPath==NULL)
        return E_POINTER;

    *ppmkRelPath=0;

    /* This method finds the common prefix of the two monikers and creates two monikers that consist     */
    /* of the remainder when the common prefix is removed. Then it creates the inverse for the remainder */
    /* of this moniker and composes the remainder of the other moniker on the right of it.               */

    /* finds the common prefix of the two monikers */
    res=IMoniker_CommonPrefixWith(iface,pmkOther,&commonMk);    

    /* if there's no common prefix or the two moniker are equal the relative is the other moniker */
    if ((res== MK_E_NOPREFIX)||(res==MK_S_US)){

        *ppmkRelPath=pmkOther;
        IMoniker_AddRef(pmkOther);
        return MK_S_HIM;
    }

    GetAfterCommonPrefix(iface,commonMk,&restThisMk);
    GetAfterCommonPrefix(pmkOther,commonMk,&restOtherMk);

    /* if other is a prefix of this moniker the relative path is the inverse of the remainder path of this */
    /* moniker when the common prefix is removed                                                           */
    if (res==MK_S_HIM){

        IMoniker_Inverse(restThisMk,ppmkRelPath);
        IMoniker_Release(restThisMk);
    }
    /* if this moniker is a prefix of other moniker the relative path is the remainder path of other moniker */
    /* when the common prefix is removed                                                                     */
    else if (res==MK_S_ME){

        *ppmkRelPath=restOtherMk;
        IMoniker_AddRef(restOtherMk);
    }
    /* the relative path is the inverse for the remainder of this moniker and the remainder of the other  */
    /* moniker on the right of it.                                                                        */
    else if (res==S_OK){

        IMoniker_Inverse(restThisMk,&invRestThisMk);
        IMoniker_Release(restThisMk);
        CreateGenericComposite(invRestThisMk,restOtherMk,ppmkRelPath);
        IMoniker_Release(invRestThisMk);
        IMoniker_Release(restOtherMk);
    }
    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_GetDisplayName
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_GetDisplayName(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   LPOLESTR *ppszDisplayName)
{
    ULONG lengthStr=1;
    IEnumMoniker *enumMoniker;
    IMoniker* tempMk;
    LPOLESTR tempStr;

    Print(MAX_TRACE, ("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName));

    if (ppszDisplayName==NULL)
        return E_POINTER;
    
    *ppszDisplayName=CoTaskMemAlloc(sizeof(WCHAR));

    if (*ppszDisplayName==NULL)
        return E_OUTOFMEMORY;

    /* This method returns the concatenation of the display names returned by each component moniker of */
    /* the composite */

    **ppszDisplayName=0;

    IMoniker_Enum(iface,TRUE,&enumMoniker);
    
    while(IEnumMoniker_Next(enumMoniker,1,&tempMk,NULL)==S_OK){

        IMoniker_GetDisplayName(tempMk,pbc,NULL,&tempStr);

        lengthStr+=lstrlenW(tempStr);

        *ppszDisplayName=CoTaskMemRealloc(*ppszDisplayName,lengthStr * sizeof(WCHAR));

        if (*ppszDisplayName==NULL)
            return E_OUTOFMEMORY;

        lstrcatW(*ppszDisplayName,tempStr);

        CoTaskMemFree(tempStr);
        IMoniker_Release(tempMk);
    }

    IEnumMoniker_Release(enumMoniker);

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_ParseDisplayName
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_ParseDisplayName(IMoniker* iface,
                                                     IBindCtx* pbc,
                                                     IMoniker* pmkToLeft,
                                                     LPOLESTR pszDisplayName,
                                                     ULONG* pchEaten,
                                                     IMoniker** ppmkOut)
{
    IEnumMoniker *enumMoniker;
    IMoniker *tempMk,*mostRigthMk,*antiMk;
    /* This method recursively calls IMoniker::ParseDisplayName on the rightmost component of the composite,*/
    /* passing everything else as the pmkToLeft parameter for that call. */

    /* get the most right moniker */
    IMoniker_Enum(iface,FALSE,&enumMoniker);
    IEnumMoniker_Next(enumMoniker,1,&mostRigthMk,NULL);
    IEnumMoniker_Release(enumMoniker);

    /* get the left moniker */
    CreateAntiMoniker(&antiMk);
    IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
    IMoniker_Release(antiMk);

    return IMoniker_ParseDisplayName(mostRigthMk,pbc,tempMk,pszDisplayName,pchEaten,ppmkOut);
}

/******************************************************************************
 *        CompositeMoniker_IsSystemMoniker
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    Print(MAX_TRACE, ("(%p,%p)\n",iface,pwdMksys));

    if (!pwdMksys)
        return E_POINTER;
    
    (*pwdMksys)=MKSYS_GENERICCOMPOSITE;

    return S_OK;
}

/*******************************************************************************
 *        CompositeMonikerIROTData_QueryInterface
 *******************************************************************************/
HRESULT WINAPI CompositeMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{

    ICOM_THIS_From_IROTData(IMoniker, iface);

    Print(MAX_TRACE, ("(%p,%p,%p)\n",iface,riid,ppvObject));

    return CompositeMonikerImpl_QueryInterface(This, riid, ppvObject);
}

/***********************************************************************
 *        CompositeMonikerIROTData_AddRef
 */
ULONG   WINAPI CompositeMonikerROTDataImpl_AddRef(IROTData *iface)
{
    ICOM_THIS_From_IROTData(IMoniker, iface);

    Print(MAX_TRACE, ("(%p)\n",iface));

    return CompositeMonikerImpl_AddRef(This);
}

/***********************************************************************
 *        CompositeMonikerIROTData_Release
 */
ULONG   WINAPI CompositeMonikerROTDataImpl_Release(IROTData* iface)
{
    ICOM_THIS_From_IROTData(IMoniker, iface);
    
    Print(MAX_TRACE, ("(%p)\n",iface));

    return CompositeMonikerImpl_Release(This);
}

/******************************************************************************
 *        CompositeMonikerIROTData_GetComparaisonData
 ******************************************************************************/
HRESULT WINAPI CompositeMonikerROTDataImpl_GetComparaisonData(IROTData* iface,
                                                              BYTE* pbData,
                                                              ULONG cbMax,
                                                              ULONG* pcbData)
{
    Print(MIN_TRACE, ("(),stub!\n"));
    return E_NOTIMPL;
}

/******************************************************************************
 *        EnumMonikerImpl_QueryInterface
 ******************************************************************************/
HRESULT WINAPI EnumMonikerImpl_QueryInterface(IEnumMoniker* iface,REFIID riid,void** ppvObject)
{
    ICOM_THIS(EnumMonikerImpl,iface);
  
    Print(MAX_TRACE, ("(%p,%p,%p)\n",This,riid,ppvObject));

    /* Perform a sanity check on the parameters.*/
    if ( (This==0) || (ppvObject==0) )
	return E_INVALIDARG;
  
    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IEnumMoniker, riid))
        *ppvObject = iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    EnumMonikerImpl_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_AddRef
 ******************************************************************************/
ULONG   WINAPI EnumMonikerImpl_AddRef(IEnumMoniker* iface)
{
    ICOM_THIS(EnumMonikerImpl,iface);

    Print(MAX_TRACE, ("(%p)\n",This));

    return ++(This->ref);

}

/******************************************************************************
 *        EnumMonikerImpl_Release
 ******************************************************************************/
ULONG   WINAPI EnumMonikerImpl_Release(IEnumMoniker* iface)
{
    ICOM_THIS(EnumMonikerImpl,iface);
    ULONG i
        ;
    Print(MAX_TRACE, ("(%p)\n",This));

    This->ref--;

    /* destroy the object if there's no more reference on it */
    if (This->ref==0){
        
        for(i=0;i<This->tabSize;i++)
            IMoniker_Release(This->tabMoniker[i]);

        HeapFree(GetProcessHeap(),0,This->tabMoniker);
        HeapFree(GetProcessHeap(),0,This);

        return 0;
    }
    return This->ref;;
}

/******************************************************************************
 *        EnumMonikerImpl_Next
 ******************************************************************************/
HRESULT WINAPI EnumMonikerImpl_Next(IEnumMoniker* iface,ULONG celt, IMoniker** rgelt, ULONG* pceltFethed){

    ICOM_THIS(EnumMonikerImpl,iface);
    ULONG i;

    /* retrieve the requested number of moniker from the current position */
    for(i=0;((This->currentPos < This->tabSize) && (i < celt));i++)

        rgelt[i]=This->tabMoniker[This->currentPos++];

    if (pceltFethed!=NULL)
        *pceltFethed= i;
    
    if (i==celt)
        return S_OK;
    else
        return S_FALSE;
}

/******************************************************************************
 *        EnumMonikerImpl_Skip
 ******************************************************************************/
HRESULT WINAPI EnumMonikerImpl_Skip(IEnumMoniker* iface,ULONG celt){

    ICOM_THIS(EnumMonikerImpl,iface);

    if ((This->currentPos+celt) >= This->tabSize)
        return S_FALSE;

    This->currentPos+=celt;
    
    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_Reset
 ******************************************************************************/
HRESULT WINAPI EnumMonikerImpl_Reset(IEnumMoniker* iface){

    ICOM_THIS(EnumMonikerImpl,iface);

    This->currentPos=0;

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_Clone
 ******************************************************************************/
HRESULT WINAPI EnumMonikerImpl_Clone(IEnumMoniker* iface,IEnumMoniker** ppenum){

    ICOM_THIS(EnumMonikerImpl,iface);

    return EnumMonikerImpl_CreateEnumMoniker(This->tabMoniker,This->tabSize,This->currentPos,TRUE,ppenum);
}

/******************************************************************************
 *        EnumMonikerImpl_CreateEnumMoniker
 ******************************************************************************/
HRESULT WINAPI EnumMonikerImpl_CreateEnumMoniker(IMoniker** tabMoniker,
                                                 ULONG tabSize,
                                                 ULONG currentPos,
                                                 BOOL leftToRigth,
                                                 IEnumMoniker ** ppmk)
{
    EnumMonikerImpl* newEnumMoniker;
    int i;


    newEnumMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(EnumMonikerImpl));

    if (newEnumMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    if (currentPos > tabSize)
        return E_INVALIDARG;
    
    /* Initialize the virtual function table. */
    ICOM_VTBL(newEnumMoniker)    = &VT_EnumMonikerImpl;
    newEnumMoniker->ref          = 0;

    newEnumMoniker->tabSize=tabSize;
    newEnumMoniker->currentPos=currentPos;

    newEnumMoniker->tabMoniker=HeapAlloc(GetProcessHeap(),0,tabSize*sizeof(IMoniker));

    if (newEnumMoniker->tabMoniker==NULL)
        return E_OUTOFMEMORY;

    if (leftToRigth)
        for (i=0;i<tabSize;i++){

            newEnumMoniker->tabMoniker[i]=tabMoniker[i];
            IMoniker_AddRef(tabMoniker[i]);
        }
    else
        for (i=tabSize-1;i>=0;i--){

            newEnumMoniker->tabMoniker[tabSize-i-1]=tabMoniker[i];
            IMoniker_AddRef(tabMoniker[i]);
        }

    *ppmk=(IEnumMoniker*)newEnumMoniker;

    return S_OK;
}

/******************************************************************************
 *        CreateGenericComposite	[OLE.55]
 ******************************************************************************/
HRESULT WINAPI CreateGenericComposite(LPMONIKER pmkFirst, LPMONIKER pmkRest, LPMONIKER* ppmkComposite)
{
    CompositeMonikerImpl* newCompositeMoniker = 0;
    HRESULT        hr = S_OK;

    Print(MAX_TRACE, ("(%p,%p,%p)\n",pmkFirst,pmkRest,ppmkComposite));

    if (ppmkComposite==NULL)
        return E_POINTER;
    
    *ppmkComposite=0;

    if (pmkFirst==NULL && pmkRest!=NULL){

        *ppmkComposite=pmkRest;
        return S_OK;
    }
    else if (pmkFirst!=NULL && pmkRest==NULL){
        *ppmkComposite=pmkFirst;
        return S_OK;
    }
    else  if (pmkFirst==NULL && pmkRest==NULL)
        return S_OK;

    newCompositeMoniker = HeapAlloc(GetProcessHeap(), 0,sizeof(CompositeMonikerImpl));

    if (newCompositeMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    hr = CompositeMonikerImpl_Construct(newCompositeMoniker,pmkFirst,pmkRest);

    if (FAILED(hr)){

        HeapFree(GetProcessHeap(),0,newCompositeMoniker);
        return hr;
    }
    if (newCompositeMoniker->tabLastIndex==1)

        hr = IMoniker_QueryInterface(newCompositeMoniker->tabMoniker[0],&IID_IMoniker,(void**)ppmkComposite);
    else

        hr = CompositeMonikerImpl_QueryInterface((IMoniker*)newCompositeMoniker,&IID_IMoniker,(void**)ppmkComposite);

    return hr;
}

/******************************************************************************
 *        MonikerCommonPrefixWith	[OLE.55]
 ******************************************************************************/
HRESULT WINAPI MonikerCommonPrefixWith(IMoniker* pmkThis,IMoniker* pmkOther,IMoniker** ppmkCommon)
{
    Print(MIN_TRACE, ("(),stub!\n"));
    return E_NOTIMPL;
}


