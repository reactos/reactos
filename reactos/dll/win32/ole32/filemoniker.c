/*
 * FileMonikers implementation
 *
 * Copyright 1999  Noomen Hamza
 * Copyright 2007  Robert Shearman
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

/* filemoniker data structure */
typedef struct FileMonikerImpl{
    IMoniker IMoniker_iface;
    IROTData IROTData_iface;
    LONG ref;
    LPOLESTR filePathName; /* path string identified by this filemoniker */
    IUnknown *pMarshal; /* custom marshaler */
} FileMonikerImpl;

static inline FileMonikerImpl *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, FileMonikerImpl, IMoniker_iface);
}

static inline FileMonikerImpl *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, FileMonikerImpl, IROTData_iface);
}

/* Local function used by filemoniker implementation */
static HRESULT FileMonikerImpl_Construct(FileMonikerImpl* iface, LPCOLESTR lpszPathName);
static HRESULT FileMonikerImpl_Destroy(FileMonikerImpl* iface);

/*******************************************************************************
 *        FileMoniker_QueryInterface
 */
static HRESULT WINAPI
FileMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( ppvObject==0 )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid)      ||
        IsEqualIID(&IID_IPersist, riid)      ||
        IsEqualIID(&IID_IPersistStream,riid) ||
        IsEqualIID(&IID_IMoniker, riid)
       )
        *ppvObject = iface;

    else if (IsEqualIID(&IID_IROTData, riid))
        *ppvObject = &This->IROTData_iface;
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

    /* Query Interface always increases the reference count by one when it is successful */
    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        FileMoniker_AddRef
 */
static ULONG WINAPI
FileMonikerImpl_AddRef(IMoniker* iface)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p)\n",iface);

    return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 *        FileMoniker_Release
 */
static ULONG WINAPI
FileMonikerImpl_Release(IMoniker* iface)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);
    ULONG ref;

    TRACE("(%p)\n",iface);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there are no more references to it */
    if (ref == 0) FileMonikerImpl_Destroy(This);

    return ref;
}

/******************************************************************************
 *        FileMoniker_GetClassID
 */
static HRESULT WINAPI
FileMonikerImpl_GetClassID(IMoniker* iface, CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_FileMoniker;

    return S_OK;
}

/******************************************************************************
 *        FileMoniker_IsDirty
 *
 * Note that the OLE-provided implementations of the IPersistStream::IsDirty
 * method in the OLE-provided moniker interfaces always return S_FALSE because
 * their internal state never changes.
 */
static HRESULT WINAPI
FileMonikerImpl_IsDirty(IMoniker* iface)
{

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

/******************************************************************************
 *        FileMoniker_Load
 *
 * this function locates and reads from the stream the filePath string
 * written by FileMonikerImpl_Save
 */
static HRESULT WINAPI
FileMonikerImpl_Load(IMoniker* iface, IStream* pStm)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);
    HRESULT res;
    CHAR* filePathA = NULL;
    WCHAR* filePathW = NULL;
    ULONG bread;
    WORD  wbuffer;
    DWORD dwbuffer, bytesA, bytesW, len;
    int i;


    TRACE("(%p,%p)\n",iface,pStm);

    /* first WORD */
    res=IStream_Read(pStm,&wbuffer,sizeof(WORD),&bread);
    if (bread!=sizeof(WORD))
    {
        WARN("Couldn't read 0 word\n");
        goto fail;
    }

    /* read filePath string length (plus one) */
    res=IStream_Read(pStm,&bytesA,sizeof(DWORD),&bread);
    if (bread != sizeof(DWORD))
    {
        WARN("Couldn't read file string length\n");
        goto fail;
    }

    /* read filePath string */
    filePathA=HeapAlloc(GetProcessHeap(),0,bytesA);
    if (!filePathA)
    {
        res = E_OUTOFMEMORY;
        goto fail;
    }

    res=IStream_Read(pStm,filePathA,bytesA,&bread);
    if (bread != bytesA)
    {
        WARN("Couldn't read file path string\n");
        goto fail;
    }

    /* read the unknown value */
    IStream_Read(pStm,&wbuffer,sizeof(WORD),&bread);
    if (bread != sizeof(WORD))
    {
        WARN("Couldn't read unknown value\n");
        goto fail;
    }

    /* read the DEAD constant */
    IStream_Read(pStm,&wbuffer,sizeof(WORD),&bread);
    if (bread != sizeof(WORD))
    {
        WARN("Couldn't read DEAD constant\n");
        goto fail;
    }

    for(i=0;i<5;i++)
    {
        res=IStream_Read(pStm,&dwbuffer,sizeof(DWORD),&bread);
        if (bread!=sizeof(DWORD))
        {
            WARN("Couldn't read 0 padding\n");
            goto fail;
        }
    }

    res=IStream_Read(pStm,&dwbuffer,sizeof(DWORD),&bread);
    if (bread!=sizeof(DWORD))
        goto fail;

    if (!dwbuffer) /* No W-string */
    {        
        bytesA--;
        len=MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, filePathA, bytesA, NULL, 0);
        if (!len)
            goto fail;

        filePathW=HeapAlloc(GetProcessHeap(),0,(len+1)*sizeof(WCHAR));
        if (!filePathW)
        {
            res = E_OUTOFMEMORY;
            goto fail;
        }
        MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, filePathA, -1, filePathW, len+1);
        goto succeed;
    }

    if (dwbuffer < 6)
        goto fail;

    bytesW=dwbuffer - 6;

    res=IStream_Read(pStm,&dwbuffer,sizeof(DWORD),&bread);
    if (bread!=sizeof(DWORD) || dwbuffer!=bytesW)
        goto fail;

    res=IStream_Read(pStm,&wbuffer,sizeof(WORD),&bread);
    if (bread!=sizeof(WORD) || wbuffer!=0x3)
        goto fail;

    len=bytesW/sizeof(WCHAR);
    filePathW=HeapAlloc(GetProcessHeap(),0,(len+1)*sizeof(WCHAR));
    if(!filePathW)
    {
         res = E_OUTOFMEMORY;
         goto fail;
    }
    res=IStream_Read(pStm,filePathW,bytesW,&bread);
    if (bread!=bytesW)
         goto fail;

    filePathW[len]=0;

 succeed:
    HeapFree(GetProcessHeap(),0,filePathA);
    HeapFree(GetProcessHeap(),0,This->filePathName);
    This->filePathName=filePathW;

    return S_OK;

 fail:
    HeapFree(GetProcessHeap(), 0, filePathA);
    HeapFree(GetProcessHeap(), 0, filePathW);

    if (SUCCEEDED(res))
         res = E_FAIL;
    return res;
}

/******************************************************************************
 *        FileMoniker_Save
 *
 * This function saves data of this object. In the beginning I thought
 * that I have just to write the filePath string on Stream. But, when I
 * tested this function with windows program samples, I noticed that it
 * was not the case. This implementation is based on XP SP2. Other versions
 * of Windows have minor variations.
 *
 * Data which must be written on stream is:
 * 1) WORD constant: zero (not validated by Windows)
 * 2) length of the path string ("\0" included)
 * 3) path string type A
 * 4) Unknown WORD value: Frequently 0xFFFF, but not always. If set very large,
 *     Windows returns E_OUTOFMEMORY
 * 5) WORD Constant: 0xDEAD (not validated by Windows)
 * 6) five DWORD constant: zero (not validated by Windows)
 * 7) If we're only writing the multibyte version,
 *     write a zero DWORD and finish.
 *
 * 8) DWORD: double-length of the path string type W ("\0" not
 *    included)
 * 9) WORD constant: 0x3
 * 10) filePath unicode string.
 *
 */
static HRESULT WINAPI
FileMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);
    HRESULT res;
    LPOLESTR filePathW=This->filePathName;
    CHAR*    filePathA;
    DWORD bytesA, bytesW, len;

    static const WORD FFFF = 0xFFFF; /* Constants */
    static const WORD DEAD = 0xDEAD;
    static const DWORD ZERO     = 0;
    static const WORD  THREE    = 0x3;

    int i;
    BOOL bUsedDefault, bWriteWide;

    TRACE("(%p,%p,%d)\n",iface,pStm,fClearDirty);

    if (pStm==NULL)
        return E_POINTER;

    /* write a 0 WORD */
    res=IStream_Write(pStm,&ZERO,sizeof(WORD),NULL);
    if (FAILED(res)) return res;

    /* write length of filePath string ( 0 included )*/
    bytesA = WideCharToMultiByte( CP_ACP, 0, filePathW, -1, NULL, 0, NULL, NULL );
    res=IStream_Write(pStm,&bytesA,sizeof(DWORD),NULL);
    if (FAILED(res)) return res;

    /* write A string (with '\0') */
    filePathA=HeapAlloc(GetProcessHeap(),0,bytesA);
    if (!filePathA)
        return E_OUTOFMEMORY;
    WideCharToMultiByte( CP_ACP, 0, filePathW, -1, filePathA, bytesA, NULL, &bUsedDefault);
    res=IStream_Write(pStm,filePathA,bytesA,NULL);
    HeapFree(GetProcessHeap(),0,filePathA);
    if (FAILED(res)) return res;

    /* write a WORD 0xFFFF */
    res=IStream_Write(pStm,&FFFF,sizeof(WORD),NULL);
    if (FAILED(res)) return res;

    /* write a WORD 0xDEAD */
    res=IStream_Write(pStm,&DEAD,sizeof(WORD),NULL);
    if (FAILED(res)) return res;

    /* write 5 zero DWORDs */
    for(i=0;i<5;i++)
    {
        res=IStream_Write(pStm,&ZERO,sizeof(DWORD),NULL);
        if (FAILED(res)) return res;
    }

    /* Write the wide version if:
     *    + couldn't convert to CP_ACP, 
     * or + it's a directory, 
     * or + there's a character > 0xFF 
     */
    len = lstrlenW(filePathW);
    bWriteWide = (bUsedDefault || (len > 0 && filePathW[len-1]=='\\' ));
    if (!bWriteWide)
    {
        WCHAR* pch;
        for(pch=filePathW;*pch;++pch) 
        {
            if (*pch > 0xFF)
            {
                bWriteWide = TRUE;
                break;
            }
        }
    }

    if (!bWriteWide)
        return IStream_Write(pStm,&ZERO,sizeof(DWORD),NULL);

    /* write bytes needed for the filepathW (without 0) + 6 */
    bytesW = len*sizeof(WCHAR) + 6;
    res=IStream_Write(pStm,&bytesW,sizeof(DWORD),NULL);
    if (FAILED(res)) return res;

    /* try again, without the extra 6 */
    bytesW -= 6;
    res=IStream_Write(pStm,&bytesW,sizeof(DWORD),NULL);
    if (FAILED(res)) return res;

    /* write a WORD 3 */
    res=IStream_Write(pStm,&THREE,sizeof(WORD),NULL);
    if (FAILED(res)) return res;

    /* write W string (no 0) */
    return IStream_Write(pStm,filePathW,bytesW,NULL);
}

/******************************************************************************
 *        FileMoniker_GetSizeMax
 */
static HRESULT WINAPI
FileMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    /* We could calculate exactly (see ...::Save()) but instead
     * we'll make a quick over-estimate, like Windows (NT4, XP) does.
     */
    pcbSize->u.LowPart  = 0x38 + 4 * lstrlenW(This->filePathName);
    pcbSize->u.HighPart = 0;

    return S_OK;
}

/******************************************************************************
 *        FileMoniker_Destroy (local function)
 *******************************************************************************/
HRESULT FileMonikerImpl_Destroy(FileMonikerImpl* This)
{
    TRACE("(%p)\n",This);

    if (This->pMarshal) IUnknown_Release(This->pMarshal);
    HeapFree(GetProcessHeap(),0,This->filePathName);
    HeapFree(GetProcessHeap(),0,This);

    return S_OK;
}

/******************************************************************************
 *                  FileMoniker_BindToObject
 */
static HRESULT WINAPI
FileMonikerImpl_BindToObject(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                             REFIID riid, VOID** ppvResult)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);
    HRESULT   res=E_FAIL;
    CLSID     clsID;
    IUnknown* pObj=0;
    IRunningObjectTable *prot=0;
    IPersistFile  *ppf=0;
    IClassFactory *pcf=0;
    IClassActivator *pca=0;

    *ppvResult=0;

    TRACE("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvResult);

    if(pmkToLeft==NULL){

        res=IBindCtx_GetRunningObjectTable(pbc,&prot);

        if (SUCCEEDED(res)){
            /* if the requested class was loaded before ! we don't need to reload it */
            res = IRunningObjectTable_GetObject(prot,iface,&pObj);

            if (res==S_FALSE){
                /* first activation of this class */
                res=GetClassFile(This->filePathName,&clsID);
                if (SUCCEEDED(res)){

                    res=CoCreateInstance(&clsID,NULL,CLSCTX_ALL,&IID_IPersistFile,(void**)&ppf);
                    if (SUCCEEDED(res)){

                        res=IPersistFile_Load(ppf,This->filePathName,STGM_READ);
                        if (SUCCEEDED(res)){

                            pObj=(IUnknown*)ppf;
                            IUnknown_AddRef(pObj);
                        }
                    }
                }
            }
        }
    }
    else{
        res=IMoniker_BindToObject(pmkToLeft,pbc,NULL,&IID_IClassFactory,(void**)&pcf);

        if (res==E_NOINTERFACE){

            res=IMoniker_BindToObject(pmkToLeft,pbc,NULL,&IID_IClassActivator,(void**)&pca);

            if (res==E_NOINTERFACE)
                return MK_E_INTERMEDIATEINTERFACENOTSUPPORTED;
        }
        if (pcf!=NULL){

            IClassFactory_CreateInstance(pcf,NULL,&IID_IPersistFile,(void**)&ppf);

            res=IPersistFile_Load(ppf,This->filePathName,STGM_READ);

            if (SUCCEEDED(res)){

                pObj=(IUnknown*)ppf;
                IUnknown_AddRef(pObj);
            }
        }
        if (pca!=NULL){

            FIXME("()\n");

            /*res=GetClassFile(This->filePathName,&clsID);

            if (SUCCEEDED(res)){

                res=IClassActivator_GetClassObject(pca,&clsID,CLSCTX_ALL,0,&IID_IPersistFile,(void**)&ppf);

                if (SUCCEEDED(res)){

                    pObj=(IUnknown*)ppf;
                    IUnknown_AddRef(pObj);
                }
            }*/
        }
    }

    if (pObj!=NULL){
        /* get the requested interface from the loaded class */
        res= IUnknown_QueryInterface(pObj,riid,ppvResult);

        IBindCtx_RegisterObjectBound(pbc,*ppvResult);

        IUnknown_Release(pObj);
    }

    if (prot!=NULL)
        IRunningObjectTable_Release(prot);

    if (ppf!=NULL)
        IPersistFile_Release(ppf);

    if (pca!=NULL)
        IClassActivator_Release(pca);

    if (pcf!=NULL)
        IClassFactory_Release(pcf);

    return res;
}

/******************************************************************************
 *        FileMoniker_BindToStorage
 */
static HRESULT WINAPI
FileMonikerImpl_BindToStorage(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                              REFIID riid, VOID** ppvObject)
{
    LPOLESTR filePath=0;
    IStorage *pstg=0;
    HRESULT res;

    TRACE("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvObject);

    if (pmkToLeft==NULL){

        if (IsEqualIID(&IID_IStorage, riid)){

            /* get the file name */
            IMoniker_GetDisplayName(iface,pbc,pmkToLeft,&filePath);

            res=StgOpenStorage(filePath,NULL,STGM_READWRITE|STGM_SHARE_DENY_WRITE,NULL,0,&pstg);

            if (SUCCEEDED(res))
                *ppvObject=pstg;

            CoTaskMemFree(filePath);
        }
        else
            if ( (IsEqualIID(&IID_IStream, riid)) || (IsEqualIID(&IID_ILockBytes, riid)) )
                return E_FAIL;
            else
                return E_NOINTERFACE;
    }
    else {

        FIXME("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvObject);

        return E_NOTIMPL;
    }
    return res;
}

/******************************************************************************
 *        FileMoniker_Reduce
 ******************************************************************************/
static HRESULT WINAPI
FileMonikerImpl_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
                       IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    TRACE("(%p,%p,%d,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    if (ppmkReduced==NULL)
        return E_POINTER;

    IMoniker_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}

static void free_stringtable(LPOLESTR *stringTable)
{
    int i;

    for (i=0; stringTable[i]!=NULL; i++)
        CoTaskMemFree(stringTable[i]);
    CoTaskMemFree(stringTable);
}

/******************************************************************************
 *        FileMoniker_ComposeWith
 */
static HRESULT WINAPI
FileMonikerImpl_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
                            BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{
    HRESULT res;
    LPOLESTR str1=0,str2=0,*strDec1=0,*strDec2=0,newStr=0;
    static const WCHAR twoPoint[]={'.','.',0};
    static const WCHAR bkSlash[]={'\\',0};
    IBindCtx *bind=0;
    int i=0,j=0,lastIdx1=0,lastIdx2=0;
    DWORD mkSys;

    TRACE("(%p,%p,%d,%p)\n",iface,pmkRight,fOnlyIfNotGeneric,ppmkComposite);

    if (ppmkComposite==NULL)
        return E_POINTER;

    if (pmkRight==NULL)
	return E_INVALIDARG;

    *ppmkComposite=0;

    IMoniker_IsSystemMoniker(pmkRight,&mkSys);

    /* check if we have two FileMonikers to compose or not */
    if(mkSys==MKSYS_FILEMONIKER){

        CreateBindCtx(0,&bind);

        IMoniker_GetDisplayName(iface,bind,NULL,&str1);
        IMoniker_GetDisplayName(pmkRight,bind,NULL,&str2);

        /* decompose pathnames of the two monikers : (to prepare the path merge operation ) */
        lastIdx1=FileMonikerImpl_DecomposePath(str1,&strDec1)-1;
        lastIdx2=FileMonikerImpl_DecomposePath(str2,&strDec2)-1;

        if ((lastIdx1==-1 && lastIdx2>-1)||(lastIdx1==1 && lstrcmpW(strDec1[0],twoPoint)==0))
            return MK_E_SYNTAX;

        if(lstrcmpW(strDec1[lastIdx1],bkSlash)==0)
            lastIdx1--;

        /* for etch "..\" in the left of str2 remove the right element from str1 */
        for(i=0; ( (lastIdx1>=0) && (strDec2[i]!=NULL) && (lstrcmpW(strDec2[i],twoPoint)==0) ) ;i+=2){

            lastIdx1-=2;
        }

        /* the length of the composed path string  is raised by the sum of the two paths lengths  */
        newStr=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(lstrlenW(str1)+lstrlenW(str2)+1));

        if (newStr)
        {
            /* new path is the concatenation of the rest of str1 and str2 */
            for(*newStr=0,j=0;j<=lastIdx1;j++)
                strcatW(newStr,strDec1[j]);

            if ((strDec2[i]==NULL && lastIdx1>-1 && lastIdx2>-1) || lstrcmpW(strDec2[i],bkSlash)!=0)
                strcatW(newStr,bkSlash);

            for(j=i;j<=lastIdx2;j++)
                strcatW(newStr,strDec2[j]);

            /* create a new moniker with the new string */
            res=CreateFileMoniker(newStr,ppmkComposite);

            /* free all strings space memory used by this function */
            HeapFree(GetProcessHeap(),0,newStr);
        }
        else res = E_OUTOFMEMORY;

        free_stringtable(strDec1);
        free_stringtable(strDec2);

        CoTaskMemFree(str1);
        CoTaskMemFree(str2);

        return res;
    }
    else if(mkSys==MKSYS_ANTIMONIKER){

        *ppmkComposite=NULL;
        return S_OK;
    }
    else if (fOnlyIfNotGeneric){

        *ppmkComposite=NULL;
        return MK_E_NEEDGENERIC;
    }
    else

        return CreateGenericComposite(iface,pmkRight,ppmkComposite);
}

/******************************************************************************
 *        FileMoniker_Enum
 */
static HRESULT WINAPI
FileMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    TRACE("(%p,%d,%p)\n",iface,fForward,ppenumMoniker);

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

/******************************************************************************
 *        FileMoniker_IsEqual
 */
static HRESULT WINAPI
FileMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);
    CLSID clsid;
    LPOLESTR filePath;
    IBindCtx* bind;
    HRESULT res;

    TRACE("(%p,%p)\n",iface,pmkOtherMoniker);

    if (pmkOtherMoniker==NULL)
        return S_FALSE;

    IMoniker_GetClassID(pmkOtherMoniker,&clsid);

    if (!IsEqualCLSID(&clsid,&CLSID_FileMoniker))
        return S_FALSE;

    res = CreateBindCtx(0,&bind);
    if (FAILED(res)) return res;

    res = S_FALSE;
    if (SUCCEEDED(IMoniker_GetDisplayName(pmkOtherMoniker,bind,NULL,&filePath))) {
	if (!lstrcmpiW(filePath, This->filePathName))
            res = S_OK;
	CoTaskMemFree(filePath);
    }

    IBindCtx_Release(bind);
    return res;
}

/******************************************************************************
 *        FileMoniker_Hash
 */
static HRESULT WINAPI
FileMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);
    int  h = 0,i,skip,len;
    int  off = 0;
    LPOLESTR val;

    if (pdwHash==NULL)
        return E_POINTER;

    val =  This->filePathName;
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
 *        FileMoniker_IsRunning
 */
static HRESULT WINAPI
FileMonikerImpl_IsRunning(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                          IMoniker* pmkNewlyRunning)
{
    IRunningObjectTable* rot;
    HRESULT res;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning);

    if ( (pmkNewlyRunning!=NULL) && (IMoniker_IsEqual(pmkNewlyRunning,iface)==S_OK) )
        return S_OK;

    if (pbc==NULL)
        return E_POINTER;

    res=IBindCtx_GetRunningObjectTable(pbc,&rot);

    if (FAILED(res))
        return res;

    res = IRunningObjectTable_IsRunning(rot,iface);

    IRunningObjectTable_Release(rot);

    return res;
}

/******************************************************************************
 *        FileMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI
FileMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc,
                                    IMoniker* pmkToLeft, FILETIME* pFileTime)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);
    IRunningObjectTable* rot;
    HRESULT res;
    WIN32_FILE_ATTRIBUTE_DATA info;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pFileTime);

    if (pFileTime==NULL)
        return E_POINTER;

    if (pmkToLeft!=NULL)
        return E_INVALIDARG;

    res=IBindCtx_GetRunningObjectTable(pbc,&rot);

    if (FAILED(res))
        return res;

    res= IRunningObjectTable_GetTimeOfLastChange(rot,iface,pFileTime);

    if (FAILED(res)){ /* the moniker is not registered */

        if (!GetFileAttributesExW(This->filePathName,GetFileExInfoStandard,&info))
            return MK_E_NOOBJECT;

        *pFileTime=info.ftLastWriteTime;
    }

    return S_OK;
}

/******************************************************************************
 *        FileMoniker_Inverse
 */
static HRESULT WINAPI
FileMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    TRACE("(%p,%p)\n",iface,ppmk);

    return CreateAntiMoniker(ppmk);
}

/******************************************************************************
 *        FileMoniker_CommonPrefixWith
 */
static HRESULT WINAPI
FileMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,IMoniker** ppmkPrefix)
{

    LPOLESTR pathThis = NULL, pathOther = NULL, *stringTable1 = NULL;
    LPOLESTR *stringTable2 = NULL, commonPath = NULL;
    IBindCtx *bindctx;
    DWORD mkSys;
    ULONG nb1,nb2,i,sameIdx;
    BOOL machineNameCase = FALSE;
    HRESULT ret;

    if (ppmkPrefix==NULL)
        return E_POINTER;

    if (pmkOther==NULL)
        return E_INVALIDARG;

    *ppmkPrefix=0;

    /* check if we have the same type of moniker */
    IMoniker_IsSystemMoniker(pmkOther,&mkSys);
    if (mkSys != MKSYS_FILEMONIKER)
        return MonikerCommonPrefixWith(iface, pmkOther, ppmkPrefix);

    ret = CreateBindCtx(0, &bindctx);
    if (FAILED(ret))
        return ret;

    /* create a string based on common part of the two paths */
    ret = IMoniker_GetDisplayName(iface, bindctx, NULL, &pathThis);
    if (FAILED(ret))
        goto failed;

    ret = IMoniker_GetDisplayName(pmkOther, bindctx, NULL, &pathOther);
    if (FAILED(ret))
        goto failed;

    nb1 = FileMonikerImpl_DecomposePath(pathThis, &stringTable1);
    if (FAILED(nb1)) {
        ret = nb1;
        goto failed;
    }

    nb2 = FileMonikerImpl_DecomposePath(pathOther, &stringTable2);
    if (FAILED(nb2)) {
        ret = nb2;
        goto failed;
    }

    if (nb1 == 0 || nb2 == 0) {
        ret = MK_E_NOPREFIX;
        goto failed;
    }

    commonPath = CoTaskMemAlloc(sizeof(WCHAR)*(min(lstrlenW(pathThis),lstrlenW(pathOther))+1));
    if (!commonPath) {
        ret = E_OUTOFMEMORY;
        goto failed;
    }

    *commonPath = 0;
    for(sameIdx=0; ( (stringTable1[sameIdx]!=NULL) &&
                     (stringTable2[sameIdx]!=NULL) &&
                     (lstrcmpiW(stringTable1[sameIdx],stringTable2[sameIdx])==0)); sameIdx++);

    if (sameIdx > 1 && *stringTable1[0]=='\\' && *stringTable2[1]=='\\'){
        machineNameCase = TRUE;

    for(i=2;i<sameIdx;i++)
        if( (*stringTable1[i]=='\\') && (i+1 < sameIdx) && (*stringTable1[i+1]=='\\') ){
            machineNameCase = FALSE;
            break;
        }
    }

    if (machineNameCase && *stringTable1[sameIdx-1]=='\\')
        sameIdx--;

    if (machineNameCase && (sameIdx<=3) && (nb1 > 3 || nb2 > 3) )
        ret = MK_E_NOPREFIX;
    else
    {
        for (i = 0; i < sameIdx; i++)
            strcatW(commonPath,stringTable1[i]);
        ret = CreateFileMoniker(commonPath, ppmkPrefix);
    }

failed:
    IBindCtx_Release(bindctx);
    CoTaskMemFree(pathThis);
    CoTaskMemFree(pathOther);
    CoTaskMemFree(commonPath);
    if (stringTable1) free_stringtable(stringTable1);
    if (stringTable2) free_stringtable(stringTable2);

    return ret;
}

/******************************************************************************
 *        DecomposePath (local function)
 */
int FileMonikerImpl_DecomposePath(LPCOLESTR str, LPOLESTR** stringTable)
{
    static const WCHAR bSlash[] = {'\\',0};
    LPOLESTR word;
    int i=0,j,tabIndex=0, ret=0;
    LPOLESTR *strgtable ;

    int len=lstrlenW(str);

    TRACE("%s, %p\n", debugstr_w(str), *stringTable);

    strgtable = CoTaskMemAlloc((len + 1)*sizeof(*strgtable));

    if (strgtable==NULL)
	return E_OUTOFMEMORY;

    word = CoTaskMemAlloc((len + 1)*sizeof(WCHAR));

    if (word==NULL)
    {
        ret = E_OUTOFMEMORY;
        goto lend;
    }

    while(str[i]!=0){

        if(str[i]==bSlash[0]){

            strgtable[tabIndex]=CoTaskMemAlloc(2*sizeof(WCHAR));

            if (strgtable[tabIndex]==NULL)
            {
                ret = E_OUTOFMEMORY;
                goto lend;
            }

            strcpyW(strgtable[tabIndex++],bSlash);

            i++;

        }
        else {

            for(j=0; str[i]!=0 && str[i]!=bSlash[0] ; i++,j++)
                word[j]=str[i];

            word[j]=0;

            strgtable[tabIndex]=CoTaskMemAlloc(sizeof(WCHAR)*(j+1));

            if (strgtable[tabIndex]==NULL)
            {
                ret = E_OUTOFMEMORY;
                goto lend;
            }

            strcpyW(strgtable[tabIndex++],word);
        }
    }
    strgtable[tabIndex]=NULL;

    *stringTable=strgtable;

    ret = tabIndex;

lend:
    if (ret < 0)
    {
        for (i = 0; i < tabIndex; i++)
            CoTaskMemFree(strgtable[i]);

        CoTaskMemFree(strgtable);
    }

    CoTaskMemFree(word);

    return ret;
}

/******************************************************************************
 *        FileMoniker_RelativePathTo
 */
static HRESULT WINAPI
FileMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    IBindCtx *bind;
    HRESULT res;
    LPOLESTR str1=0,str2=0,*tabStr1=0,*tabStr2=0,relPath=0;
    DWORD len1=0,len2=0,sameIdx=0,j=0;
    static const WCHAR back[] ={'.','.','\\',0};

    TRACE("(%p,%p,%p)\n",iface,pmOther,ppmkRelPath);

    if (ppmkRelPath==NULL)
        return E_POINTER;

    if (pmOther==NULL)
        return E_INVALIDARG;

    res=CreateBindCtx(0,&bind);
    if (FAILED(res))
	return res;

    res=IMoniker_GetDisplayName(iface,bind,NULL,&str1);
    if (FAILED(res))
	return res;
    res=IMoniker_GetDisplayName(pmOther,bind,NULL,&str2);
    if (FAILED(res))
	return res;

    len1=FileMonikerImpl_DecomposePath(str1,&tabStr1);
    if (FAILED(len1))
        return E_OUTOFMEMORY;
    len2=FileMonikerImpl_DecomposePath(str2,&tabStr2);

    if (FAILED(len2))
    {
        free_stringtable(tabStr1);
        return E_OUTOFMEMORY;
    }

    /* count the number of similar items from the begin of the two paths */
    for(sameIdx=0; ( (tabStr1[sameIdx]!=NULL) &&
		   (tabStr2[sameIdx]!=NULL) &&
               (lstrcmpiW(tabStr1[sameIdx],tabStr2[sameIdx])==0)); sameIdx++);

    /* begin the construction of relativePath */
    /* if the two paths have a consecutive similar item from the begin ! the relativePath will be composed */
    /* by "..\\" in the begin */
    relPath=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(1+lstrlenW(str1)+lstrlenW(str2)));

    *relPath=0;

    if (len2>0 && !(len1==1 && len2==1 && sameIdx==0))
        for(j=sameIdx;(tabStr1[j] != NULL); j++)
            if (*tabStr1[j]!='\\')
                strcatW(relPath,back);

    /* add items of the second path (similar items with the first path are not included) to the relativePath */
    for(j=sameIdx;tabStr2[j]!=NULL;j++)
        strcatW(relPath,tabStr2[j]);

    res=CreateFileMoniker(relPath,ppmkRelPath);

    free_stringtable(tabStr1);
    free_stringtable(tabStr2);
    CoTaskMemFree(str1);
    CoTaskMemFree(str2);
    HeapFree(GetProcessHeap(),0,relPath);

    if (len1==0 || len2==0 || (len1==1 && len2==1 && sameIdx==0))
        return MK_S_HIM;

    return res;
}

/******************************************************************************
 *        FileMoniker_GetDisplayName
 */
static HRESULT WINAPI
FileMonikerImpl_GetDisplayName(IMoniker* iface, IBindCtx* pbc,
                               IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
    FileMonikerImpl *This = impl_from_IMoniker(iface);
    int len=lstrlenW(This->filePathName);

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName);

    if (ppszDisplayName==NULL)
        return E_POINTER;

    if (pmkToLeft!=NULL)
        return E_INVALIDARG;

    *ppszDisplayName=CoTaskMemAlloc(sizeof(WCHAR)*(len+1));
    if (*ppszDisplayName==NULL)
        return E_OUTOFMEMORY;

    strcpyW(*ppszDisplayName,This->filePathName);

    TRACE("-- %s\n", debugstr_w(*ppszDisplayName));
    
    return S_OK;
}

/******************************************************************************
 *        FileMoniker_ParseDisplayName
 */
static HRESULT WINAPI
FileMonikerImpl_ParseDisplayName(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                     LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
    FIXME("(%p,%p,%p,%p,%p,%p),stub!\n",iface,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);
    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_IsSystemMoniker
 */
static HRESULT WINAPI
FileMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_FILEMONIKER;

    return S_OK;
}

/*******************************************************************************
 *        FileMonikerIROTData_QueryInterface
 */
static HRESULT WINAPI
FileMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{

    FileMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    return FileMonikerImpl_QueryInterface(&This->IMoniker_iface, riid, ppvObject);
}

/***********************************************************************
 *        FileMonikerIROTData_AddRef
 */
static ULONG WINAPI
FileMonikerROTDataImpl_AddRef(IROTData *iface)
{
    FileMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",This);

    return IMoniker_AddRef(&This->IMoniker_iface);
}

/***********************************************************************
 *        FileMonikerIROTData_Release
 */
static ULONG WINAPI
FileMonikerROTDataImpl_Release(IROTData* iface)
{
    FileMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",This);

    return FileMonikerImpl_Release(&This->IMoniker_iface);
}

/******************************************************************************
 *        FileMonikerIROTData_GetComparisonData
 */
static HRESULT WINAPI
FileMonikerROTDataImpl_GetComparisonData(IROTData* iface, BYTE* pbData,
                                          ULONG cbMax, ULONG* pcbData)
{
    FileMonikerImpl *This = impl_from_IROTData(iface);
    int len = strlenW(This->filePathName)+1;
    int i;
    LPWSTR pszFileName;

    TRACE("(%p, %u, %p)\n", pbData, cbMax, pcbData);

    *pcbData = sizeof(CLSID) + len * sizeof(WCHAR);
    if (cbMax < *pcbData)
        return E_OUTOFMEMORY;

    memcpy(pbData, &CLSID_FileMoniker, sizeof(CLSID));
    pszFileName = (LPWSTR)(pbData+sizeof(CLSID));
    for (i = 0; i < len; i++)
        pszFileName[i] = toupperW(This->filePathName[i]);

    return S_OK;
}

/*
 * Virtual function table for the FileMonikerImpl class which include IPersist,
 * IPersistStream and IMoniker functions.
 */
static const IMonikerVtbl VT_FileMonikerImpl =
{
    FileMonikerImpl_QueryInterface,
    FileMonikerImpl_AddRef,
    FileMonikerImpl_Release,
    FileMonikerImpl_GetClassID,
    FileMonikerImpl_IsDirty,
    FileMonikerImpl_Load,
    FileMonikerImpl_Save,
    FileMonikerImpl_GetSizeMax,
    FileMonikerImpl_BindToObject,
    FileMonikerImpl_BindToStorage,
    FileMonikerImpl_Reduce,
    FileMonikerImpl_ComposeWith,
    FileMonikerImpl_Enum,
    FileMonikerImpl_IsEqual,
    FileMonikerImpl_Hash,
    FileMonikerImpl_IsRunning,
    FileMonikerImpl_GetTimeOfLastChange,
    FileMonikerImpl_Inverse,
    FileMonikerImpl_CommonPrefixWith,
    FileMonikerImpl_RelativePathTo,
    FileMonikerImpl_GetDisplayName,
    FileMonikerImpl_ParseDisplayName,
    FileMonikerImpl_IsSystemMoniker
};

/* Virtual function table for the IROTData class. */
static const IROTDataVtbl VT_ROTDataImpl =
{
    FileMonikerROTDataImpl_QueryInterface,
    FileMonikerROTDataImpl_AddRef,
    FileMonikerROTDataImpl_Release,
    FileMonikerROTDataImpl_GetComparisonData
};

/******************************************************************************
 *         FileMoniker_Construct (local function)
 */
static HRESULT FileMonikerImpl_Construct(FileMonikerImpl* This, LPCOLESTR lpszPathName)
{
    int nb=0,i;
    int sizeStr=lstrlenW(lpszPathName);
    LPOLESTR *tabStr=0;
    static const WCHAR twoPoint[]={'.','.',0};
    static const WCHAR bkSlash[]={'\\',0};
    BOOL addBkSlash;

    TRACE("(%p,%s)\n",This,debugstr_w(lpszPathName));

    /* Initialize the virtual function table. */
    This->IMoniker_iface.lpVtbl = &VT_FileMonikerImpl;
    This->IROTData_iface.lpVtbl = &VT_ROTDataImpl;
    This->ref          = 0;
    This->pMarshal     = NULL;

    This->filePathName=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(sizeStr+1));

    if (This->filePathName==NULL)
        return E_OUTOFMEMORY;

    strcpyW(This->filePathName,lpszPathName);

    nb=FileMonikerImpl_DecomposePath(This->filePathName,&tabStr);

    if (nb > 0 ){

        addBkSlash = TRUE;
        if (lstrcmpW(tabStr[0],twoPoint)!=0)
            addBkSlash = FALSE;
        else
            for(i=0;i<nb;i++){

                if ( (lstrcmpW(tabStr[i],twoPoint)!=0) && (lstrcmpW(tabStr[i],bkSlash)!=0) ){
                    addBkSlash = FALSE;
                    break;
                }
                else

                    if (lstrcmpW(tabStr[i],bkSlash)==0 && i<nb-1 && lstrcmpW(tabStr[i+1],bkSlash)==0){
                        *tabStr[i]=0;
                        sizeStr--;
                        addBkSlash = FALSE;
                        break;
                    }
            }

        if (lstrcmpW(tabStr[nb-1],bkSlash)==0)
            addBkSlash = FALSE;

        This->filePathName=HeapReAlloc(GetProcessHeap(),0,This->filePathName,(sizeStr+1)*sizeof(WCHAR));

        *This->filePathName=0;

        for(i=0;tabStr[i]!=NULL;i++)
            strcatW(This->filePathName,tabStr[i]);

        if (addBkSlash)
            strcatW(This->filePathName,bkSlash);
    }

    free_stringtable(tabStr);

    return S_OK;
}

/******************************************************************************
 *        CreateFileMoniker (OLE32.@)
 ******************************************************************************/
HRESULT WINAPI CreateFileMoniker(LPCOLESTR lpszPathName, IMoniker **ppmk)
{
    FileMonikerImpl* newFileMoniker;
    HRESULT  hr;

    TRACE("(%s,%p)\n",debugstr_w(lpszPathName),ppmk);

    if (!ppmk)
        return E_POINTER;

    if(!lpszPathName)
        return MK_E_SYNTAX;

    *ppmk=NULL;

    newFileMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(FileMonikerImpl));

    if (!newFileMoniker)
        return E_OUTOFMEMORY;

    hr = FileMonikerImpl_Construct(newFileMoniker,lpszPathName);

    if (SUCCEEDED(hr))
        hr = IMoniker_QueryInterface(&newFileMoniker->IMoniker_iface,&IID_IMoniker,(void**)ppmk);
    else
        HeapFree(GetProcessHeap(),0,newFileMoniker);

    return hr;
}

/* find a character from a set in reverse without the string having to be null-terminated */
static inline WCHAR *memrpbrkW(const WCHAR *ptr, size_t n, const WCHAR *accept)
{
    const WCHAR *end, *ret = NULL;
    for (end = ptr + n; ptr < end; ptr++) if (strchrW(accept, *ptr)) ret = ptr;
    return (WCHAR *)ret;
}

HRESULT FileMoniker_CreateFromDisplayName(LPBC pbc, LPCOLESTR szDisplayName,
                                          LPDWORD pchEaten, IMoniker **ppmk)
{
    LPCWSTR end;
    static const WCHAR wszSeparators[] = {':','\\','/','!',0};

    for (end = szDisplayName + strlenW(szDisplayName);
         end && (end != szDisplayName);
         end = memrpbrkW(szDisplayName, end - szDisplayName, wszSeparators))
    {
        HRESULT hr;
        IRunningObjectTable *rot;
        IMoniker *file_moniker;
        LPWSTR file_display_name;
        LPWSTR full_path_name;
        DWORD full_path_name_len;
        int len = end - szDisplayName;

        file_display_name = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
        if (!file_display_name) return E_OUTOFMEMORY;
        memcpy(file_display_name, szDisplayName, len * sizeof(WCHAR));
        file_display_name[len] = '\0';

        hr = CreateFileMoniker(file_display_name, &file_moniker);
        if (FAILED(hr))
        {
            HeapFree(GetProcessHeap(), 0, file_display_name);
            return hr;
        }

        hr = IBindCtx_GetRunningObjectTable(pbc, &rot);
        if (FAILED(hr))
        {
            HeapFree(GetProcessHeap(), 0, file_display_name);
            IMoniker_Release(file_moniker);
            return hr;
        }

        hr = IRunningObjectTable_IsRunning(rot, file_moniker);
        IRunningObjectTable_Release(rot);
        if (FAILED(hr))
        {
            HeapFree(GetProcessHeap(), 0, file_display_name);
            IMoniker_Release(file_moniker);
            return hr;
        }
        if (hr == S_OK)
        {
            TRACE("found running file moniker for %s\n", debugstr_w(file_display_name));
            *pchEaten = len;
            *ppmk = file_moniker;
            HeapFree(GetProcessHeap(), 0, file_display_name);
            return S_OK;
        }

        full_path_name_len = GetFullPathNameW(file_display_name, 0, NULL, NULL);
        if (!full_path_name_len)
        {
            HeapFree(GetProcessHeap(), 0, file_display_name);
            IMoniker_Release(file_moniker);
            return MK_E_SYNTAX;
        }
        full_path_name = HeapAlloc(GetProcessHeap(), 0, full_path_name_len * sizeof(WCHAR));
        if (!full_path_name)
        {
            HeapFree(GetProcessHeap(), 0, file_display_name);
            IMoniker_Release(file_moniker);
            return E_OUTOFMEMORY;
        }
        GetFullPathNameW(file_display_name, full_path_name_len, full_path_name, NULL);

        if (GetFileAttributesW(full_path_name) == INVALID_FILE_ATTRIBUTES)
            TRACE("couldn't open file %s\n", debugstr_w(full_path_name));
        else
        {
            TRACE("got file moniker for %s\n", debugstr_w(szDisplayName));
            *pchEaten = len;
            *ppmk = file_moniker;
            HeapFree(GetProcessHeap(), 0, file_display_name);
            HeapFree(GetProcessHeap(), 0, full_path_name);
            return S_OK;
        }
        HeapFree(GetProcessHeap(), 0, file_display_name);
        HeapFree(GetProcessHeap(), 0, full_path_name);
        IMoniker_Release(file_moniker);
    }

    return MK_E_CANTOPENFILE;
}


static HRESULT WINAPI FileMonikerCF_QueryInterface(LPCLASSFACTORY iface,
                                                  REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI FileMonikerCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI FileMonikerCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI FileMonikerCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    FileMonikerImpl* newFileMoniker;
    HRESULT  hr;
    static const WCHAR wszEmpty[] = { 0 };

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    newFileMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(FileMonikerImpl));
    if (!newFileMoniker)
        return E_OUTOFMEMORY;

    hr = FileMonikerImpl_Construct(newFileMoniker, wszEmpty);

    if (SUCCEEDED(hr))
        hr = IMoniker_QueryInterface(&newFileMoniker->IMoniker_iface, riid, ppv);
    if (FAILED(hr))
        HeapFree(GetProcessHeap(),0,newFileMoniker);

    return hr;
}

static HRESULT WINAPI FileMonikerCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl FileMonikerCFVtbl =
{
    FileMonikerCF_QueryInterface,
    FileMonikerCF_AddRef,
    FileMonikerCF_Release,
    FileMonikerCF_CreateInstance,
    FileMonikerCF_LockServer
};
static const IClassFactoryVtbl *FileMonikerCF = &FileMonikerCFVtbl;

HRESULT FileMonikerCF_Create(REFIID riid, LPVOID *ppv)
{
    return IClassFactory_QueryInterface((IClassFactory *)&FileMonikerCF, riid, ppv);
}
