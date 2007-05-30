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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "objbase.h"
#include "moniker.h"

#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* filemoniker data structure */
typedef struct FileMonikerImpl{

    const IMonikerVtbl*  lpvtbl1;  /* VTable relative to the IMoniker interface.*/

    /* The ROT (RunningObjectTable implementation) uses the IROTData interface to test whether
     * two monikers are equal. That's whay IROTData interface is implemented by monikers.
     */
    const IROTDataVtbl*  lpvtbl2;  /* VTable relative to the IROTData interface.*/

    LONG ref; /* reference counter for this object */

    LPOLESTR filePathName; /* path string identified by this filemoniker */

    IUnknown *pMarshal; /* custom marshaler */
} FileMonikerImpl;

static inline IMoniker *impl_from_IROTData( IROTData *iface )
{
    return (IMoniker *)((char*)iface - FIELD_OFFSET(FileMonikerImpl, lpvtbl2));
}

/* Local function used by filemoniker implementation */
static HRESULT WINAPI FileMonikerImpl_Construct(FileMonikerImpl* iface, LPCOLESTR lpszPathName);
static HRESULT WINAPI FileMonikerImpl_Destroy(FileMonikerImpl* iface);

/*******************************************************************************
 *        FileMoniker_QueryInterface
 */
static HRESULT WINAPI
FileMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    FileMonikerImpl *This = (FileMonikerImpl *)iface;

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( (This==0) || (ppvObject==0) )
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
        *ppvObject = (IROTData*)&(This->lpvtbl2);
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
    FileMonikerImpl *This = (FileMonikerImpl *)iface;

    TRACE("(%p)\n",iface);

    return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 *        FileMoniker_Release
 */
static ULONG WINAPI
FileMonikerImpl_Release(IMoniker* iface)
{
    FileMonikerImpl *This = (FileMonikerImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",iface);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there's no more reference on it */
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
    HRESULT res;
    CHAR* filePathA = NULL;
    WCHAR* filePathW = NULL;
    ULONG bread;
    WORD  wbuffer;
    DWORD dwbuffer, bytesA, bytesW, len;
    int i;

    FileMonikerImpl *This = (FileMonikerImpl *)iface;

    TRACE("(%p,%p)\n",iface,pStm);

    /* first WORD must be 0 */
    res=IStream_Read(pStm,&wbuffer,sizeof(WORD),&bread);
    if (bread!=sizeof(WORD) || wbuffer!=0)
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

    /* read the first constant */
    IStream_Read(pStm,&dwbuffer,sizeof(DWORD),&bread);
    if (bread != sizeof(DWORD) || dwbuffer != 0xDEADFFFF)
    {
        WARN("Couldn't read 0xDEADFFFF constant\n");
        goto fail;
    }

    for(i=0;i<5;i++)
    {
        res=IStream_Read(pStm,&dwbuffer,sizeof(DWORD),&bread);
        if (bread!=sizeof(DWORD) || dwbuffer!=0)
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
 * tested this function with windows programs samples, I noticed that it
 * was not the case. This implementation is based on XP SP2. Other versions
 * of Windows have minor variations.
 *
 * Data which must be written on stream is:
 * 1) WORD constant:zero
 * 2) length of the path string ("\0" included)
 * 3) path string type A
 * 4) DWORD constant : 0xDEADFFFF
 * 5) five DWORD constant: zero
 * 6) If we're only writing the multibyte version, 
 *     write a zero DWORD and finish.
 *
 * 7) DWORD: double-length of the the path string type W ("\0" not
 *    included)
 * 8) WORD constant: 0x3
 * 9) filePath unicode string.
 *
 */
static HRESULT WINAPI
FileMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    FileMonikerImpl *This = (FileMonikerImpl *)iface;

    HRESULT res;
    LPOLESTR filePathW=This->filePathName;
    CHAR*    filePathA;
    DWORD bytesA, bytesW, len;

    static const DWORD DEADFFFF = 0xDEADFFFF;  /* Constants */
    static const DWORD ZERO     = 0;
    static const WORD  THREE    = 0x3;

    int i;
    BOOL bUsedDefault, bWriteWide;

    TRACE("(%p,%p,%d)\n",iface,pStm,fClearDirty);

    if (pStm==NULL)
        return E_POINTER;

    /* write a 0 WORD */
    res=IStream_Write(pStm,&ZERO,sizeof(WORD),NULL);
    if (!SUCCEEDED(res)) return res;

    /* write length of filePath string ( 0 included )*/
    bytesA = WideCharToMultiByte( CP_ACP, 0, filePathW, -1, NULL, 0, NULL, NULL );
    res=IStream_Write(pStm,&bytesA,sizeof(DWORD),NULL);
    if (!SUCCEEDED(res)) return res;

    /* write A string (with '\0') */
    filePathA=HeapAlloc(GetProcessHeap(),0,bytesA);
    if (!filePathA)
        return E_OUTOFMEMORY;
    WideCharToMultiByte( CP_ACP, 0, filePathW, -1, filePathA, bytesA, NULL, &bUsedDefault);
    res=IStream_Write(pStm,filePathA,bytesA,NULL);
    HeapFree(GetProcessHeap(),0,filePathA);
    if (!SUCCEEDED(res)) return res;

    /* write a DWORD 0xDEADFFFF */
    res=IStream_Write(pStm,&DEADFFFF,sizeof(DWORD),NULL);
    if (!SUCCEEDED(res)) return res;

    /* write 5 zero DWORDs */
    for(i=0;i<5;i++)
    {
        res=IStream_Write(pStm,&ZERO,sizeof(DWORD),NULL);
        if (!SUCCEEDED(res)) return res;
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
    {
        res=IStream_Write(pStm,&ZERO,sizeof(DWORD),NULL);
        return res;
    }

    /* write bytes needed for the filepathW (without 0) + 6 */
    bytesW = len*sizeof(WCHAR) + 6;
    res=IStream_Write(pStm,&bytesW,sizeof(DWORD),NULL);
    if (!SUCCEEDED(res)) return res;

    /* try again, without the extra 6 */
    bytesW -= 6;
    res=IStream_Write(pStm,&bytesW,sizeof(DWORD),NULL);
    if (!SUCCEEDED(res)) return res;

    /* write a WORD 3 */
    res=IStream_Write(pStm,&THREE,sizeof(WORD),NULL);
    if (!SUCCEEDED(res)) return res;

    /* write W string (no 0) */
    res=IStream_Write(pStm,filePathW,bytesW,NULL);

    return res;
}

/******************************************************************************
 *        FileMoniker_GetSizeMax
 */
static HRESULT WINAPI
FileMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    FileMonikerImpl *This = (FileMonikerImpl *)iface;

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
HRESULT WINAPI FileMonikerImpl_Destroy(FileMonikerImpl* This)
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
    HRESULT   res=E_FAIL;
    CLSID     clsID;
    IUnknown* pObj=0;
    IRunningObjectTable *prot=0;
    IPersistFile  *ppf=0;
    IClassFactory *pcf=0;
    IClassActivator *pca=0;

    FileMonikerImpl *This = (FileMonikerImpl *)iface;

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

        IBindCtx_RegisterObjectBound(pbc,(IUnknown*)*ppvResult);

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

            /* verifie if the file contains a storage object */
            res=StgIsStorageFile(filePath);

            if(res==S_OK){

                res=StgOpenStorage(filePath,NULL,STGM_READWRITE|STGM_SHARE_DENY_WRITE,NULL,0,&pstg);

                if (SUCCEEDED(res)){

                    *ppvObject=pstg;

                    IStorage_AddRef(pstg);

                    return res;
                }
            }
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

    /* check if we have two filemonikers to compose or not */
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

	  if (newStr==NULL)
		return E_OUTOFMEMORY;

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

        for(i=0; strDec1[i]!=NULL;i++)
            CoTaskMemFree(strDec1[i]);
        for(i=0; strDec2[i]!=NULL;i++)
            CoTaskMemFree(strDec2[i]);
        CoTaskMemFree(strDec1);
        CoTaskMemFree(strDec2);

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
    FileMonikerImpl *This = (FileMonikerImpl *)iface;
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

    if (SUCCEEDED(IMoniker_GetDisplayName(pmkOtherMoniker,bind,NULL,&filePath))) {
	int result = lstrcmpiW(filePath, This->filePathName);
	CoTaskMemFree(filePath);
        if ( result == 0 ) return S_OK;
    }
    return S_FALSE;

}

/******************************************************************************
 *        FileMoniker_Hash
 */
static HRESULT WINAPI
FileMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    FileMonikerImpl *This = (FileMonikerImpl *)iface;

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
    FileMonikerImpl *This = (FileMonikerImpl *)iface;
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

    LPOLESTR pathThis,pathOther,*stringTable1,*stringTable2,commonPath;
    IBindCtx *pbind;
    DWORD mkSys;
    ULONG nb1,nb2,i,sameIdx;
    BOOL machimeNameCase=FALSE;

    if (ppmkPrefix==NULL)
        return E_POINTER;

    if (pmkOther==NULL)
        return E_INVALIDARG;

    *ppmkPrefix=0;

    /* check if we have the same type of moniker */
    IMoniker_IsSystemMoniker(pmkOther,&mkSys);

    if(mkSys==MKSYS_FILEMONIKER){
        HRESULT ret;

        CreateBindCtx(0,&pbind);

        /* create a string based on common part of the two paths */

        IMoniker_GetDisplayName(iface,pbind,NULL,&pathThis);
        IMoniker_GetDisplayName(pmkOther,pbind,NULL,&pathOther);

        nb1=FileMonikerImpl_DecomposePath(pathThis,&stringTable1);
        nb2=FileMonikerImpl_DecomposePath(pathOther,&stringTable2);

        if (nb1==0 || nb2==0)
            return MK_E_NOPREFIX;

        commonPath=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(min(lstrlenW(pathThis),lstrlenW(pathOther))+1));

        *commonPath=0;

        for(sameIdx=0; ( (stringTable1[sameIdx]!=NULL) &&
                         (stringTable2[sameIdx]!=NULL) &&
                         (lstrcmpiW(stringTable1[sameIdx],stringTable2[sameIdx])==0)); sameIdx++);

        if (sameIdx > 1 && *stringTable1[0]=='\\' && *stringTable2[1]=='\\'){

            machimeNameCase=TRUE;

            for(i=2;i<sameIdx;i++)

                if( (*stringTable1[i]=='\\') && (i+1 < sameIdx) && (*stringTable1[i+1]=='\\') ){
                    machimeNameCase=FALSE;
                    break;
            }
        }

        if (machimeNameCase && *stringTable1[sameIdx-1]=='\\')
            sameIdx--;

        if (machimeNameCase && (sameIdx<=3) && (nb1 > 3 || nb2 > 3) )
            ret = MK_E_NOPREFIX;
        else
        {
            for(i=0;i<sameIdx;i++)
                strcatW(commonPath,stringTable1[i]);
    
            for(i=0;i<nb1;i++)
                CoTaskMemFree(stringTable1[i]);
    
            CoTaskMemFree(stringTable1);
    
            for(i=0;i<nb2;i++)
                CoTaskMemFree(stringTable2[i]);
    
            CoTaskMemFree(stringTable2);
    
            ret = CreateFileMoniker(commonPath,ppmkPrefix);
        }
        HeapFree(GetProcessHeap(),0,commonPath);
        return ret;
    }
    else
        return MonikerCommonPrefixWith(iface,pmkOther,ppmkPrefix);
}

/******************************************************************************
 *        DecomposePath (local function)
 */
int FileMonikerImpl_DecomposePath(LPCOLESTR str, LPOLESTR** stringTable)
{
    static const WCHAR bSlash[] = {'\\',0};
    WCHAR word[MAX_PATH];
    int i=0,j,tabIndex=0;
    LPOLESTR *strgtable ;

    int len=lstrlenW(str);

    TRACE("%s, %p\n", debugstr_w(str), *stringTable);

    strgtable =CoTaskMemAlloc(len*sizeof(LPOLESTR));

    if (strgtable==NULL)
	return E_OUTOFMEMORY;

    while(str[i]!=0){

        if(str[i]==bSlash[0]){

            strgtable[tabIndex]=CoTaskMemAlloc(2*sizeof(WCHAR));

            if (strgtable[tabIndex]==NULL)
	    	return E_OUTOFMEMORY;

            strcpyW(strgtable[tabIndex++],bSlash);

            i++;

        }
        else {

            for(j=0; str[i]!=0 && str[i]!=bSlash[0] ; i++,j++)
                word[j]=str[i];

            word[j]=0;

            strgtable[tabIndex]=CoTaskMemAlloc(sizeof(WCHAR)*(j+1));

            if (strgtable[tabIndex]==NULL)
                return E_OUTOFMEMORY;

            strcpyW(strgtable[tabIndex++],word);
        }
    }
    strgtable[tabIndex]=NULL;

    *stringTable=strgtable;

    return tabIndex;
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
    len2=FileMonikerImpl_DecomposePath(str2,&tabStr2);

    if (FAILED(len1) || FAILED(len2))
	return E_OUTOFMEMORY;

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

    for(j=0; tabStr1[j]!=NULL;j++)
        CoTaskMemFree(tabStr1[j]);
    for(j=0; tabStr2[j]!=NULL;j++)
        CoTaskMemFree(tabStr2[j]);
    CoTaskMemFree(tabStr1);
    CoTaskMemFree(tabStr2);
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
    FileMonikerImpl *This = (FileMonikerImpl *)iface;

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

    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    return FileMonikerImpl_QueryInterface(This, riid, ppvObject);
}

/***********************************************************************
 *        FileMonikerIROTData_AddRef
 */
static ULONG WINAPI
FileMonikerROTDataImpl_AddRef(IROTData *iface)
{
    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",This);

    return IMoniker_AddRef(This);
}

/***********************************************************************
 *        FileMonikerIROTData_Release
 */
static ULONG WINAPI
FileMonikerROTDataImpl_Release(IROTData* iface)
{
    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",This);

    return FileMonikerImpl_Release(This);
}

/******************************************************************************
 *        FileMonikerIROTData_GetComparaisonData
 */
static HRESULT WINAPI
FileMonikerROTDataImpl_GetComparisonData(IROTData* iface, BYTE* pbData,
                                          ULONG cbMax, ULONG* pcbData)
{
    IMoniker *This = impl_from_IROTData(iface);
    FileMonikerImpl *This1 = (FileMonikerImpl *)This;
    int len = (strlenW(This1->filePathName)+1);
    int i;
    LPWSTR pszFileName;

    TRACE("(%p, %u, %p)\n", pbData, cbMax, pcbData);

    *pcbData = sizeof(CLSID) + len * sizeof(WCHAR);
    if (cbMax < *pcbData)
        return E_OUTOFMEMORY;

    memcpy(pbData, &CLSID_FileMoniker, sizeof(CLSID));
    pszFileName = (LPWSTR)(pbData+sizeof(CLSID));
    for (i = 0; i < len; i++)
        pszFileName[i] = toupperW(This1->filePathName[i]);

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
static HRESULT WINAPI
FileMonikerImpl_Construct(FileMonikerImpl* This, LPCOLESTR lpszPathName)
{
    int nb=0,i;
    int sizeStr=lstrlenW(lpszPathName);
    LPOLESTR *tabStr=0;
    static const WCHAR twoPoint[]={'.','.',0};
    static const WCHAR bkSlash[]={'\\',0};
    BYTE addBkSlash;

    TRACE("(%p,%s)\n",This,debugstr_w(lpszPathName));

    /* Initialize the virtual fgunction table. */
    This->lpvtbl1      = &VT_FileMonikerImpl;
    This->lpvtbl2      = &VT_ROTDataImpl;
    This->ref          = 0;
    This->pMarshal     = NULL;

    This->filePathName=HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*(sizeStr+1));

    if (This->filePathName==NULL)
        return E_OUTOFMEMORY;

    strcpyW(This->filePathName,lpszPathName);

    nb=FileMonikerImpl_DecomposePath(This->filePathName,&tabStr);

    if (nb > 0 ){

        addBkSlash=1;
        if (lstrcmpW(tabStr[0],twoPoint)!=0)
            addBkSlash=0;
        else
            for(i=0;i<nb;i++){

                if ( (lstrcmpW(tabStr[i],twoPoint)!=0) && (lstrcmpW(tabStr[i],bkSlash)!=0) ){
                    addBkSlash=0;
                    break;
                }
                else

                    if (lstrcmpW(tabStr[i],bkSlash)==0 && i<nb-1 && lstrcmpW(tabStr[i+1],bkSlash)==0){
                        *tabStr[i]=0;
                        sizeStr--;
                        addBkSlash=0;
                        break;
                    }
            }

        if (lstrcmpW(tabStr[nb-1],bkSlash)==0)
            addBkSlash=0;

        This->filePathName=HeapReAlloc(GetProcessHeap(),0,This->filePathName,(sizeStr+1)*sizeof(WCHAR));

        *This->filePathName=0;

        for(i=0;tabStr[i]!=NULL;i++)
            strcatW(This->filePathName,tabStr[i]);

        if (addBkSlash)
            strcatW(This->filePathName,bkSlash);
    }

    for(i=0; tabStr[i]!=NULL;i++)
        CoTaskMemFree(tabStr[i]);
    CoTaskMemFree(tabStr);

    return S_OK;
}

/******************************************************************************
 *        CreateFileMoniker (OLE32.@)
 ******************************************************************************/
HRESULT WINAPI CreateFileMoniker(LPCOLESTR lpszPathName, LPMONIKER * ppmk)
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
	hr = FileMonikerImpl_QueryInterface((IMoniker*)newFileMoniker,&IID_IMoniker,(void**)ppmk);
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
                                          LPDWORD pchEaten, LPMONIKER *ppmk)
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
        IUnknown_AddRef(iface);
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
	hr = FileMonikerImpl_QueryInterface((IMoniker*)newFileMoniker, riid, ppv);
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
