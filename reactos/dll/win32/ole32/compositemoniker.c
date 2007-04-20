/*
 * CompositeMonikers implementation
 *
 * Copyright 1999  Noomen Hamza
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
#include "winuser.h"
#include "winerror.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "ole2.h"
#include "moniker.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define  BLOCK_TAB_SIZE 5 /* represent the first size table and it's increment block size */

/* CompositeMoniker data structure */
typedef struct CompositeMonikerImpl{

    const IMonikerVtbl*  lpvtbl1;  /* VTable relative to the IMoniker interface.*/

    /* The ROT (RunningObjectTable implementation) uses the IROTData
     * interface to test whether two monikers are equal. That's why IROTData
     * interface is implemented by monikers.
     */
    const IROTDataVtbl*  lpvtbl2;  /* VTable relative to the IROTData interface.*/

    const IMarshalVtbl*  lpvtblMarshal;  /* VTable relative to the IMarshal interface.*/

    LONG ref; /* reference counter for this object */

    IMoniker** tabMoniker; /* dynamaic table containing all components (monikers) of this composite moniker */

    ULONG    tabSize;      /* size of tabMoniker */

    ULONG    tabLastIndex;  /* first free index in tabMoniker */

} CompositeMonikerImpl;


/* EnumMoniker data structure */
typedef struct EnumMonikerImpl{

    const IEnumMonikerVtbl *lpVtbl;  /* VTable relative to the IEnumMoniker interface.*/

    LONG ref; /* reference counter for this object */

    IMoniker** tabMoniker; /* dynamic table containing the enumerated monikers */

    ULONG      tabSize; /* size of tabMoniker */

    ULONG      currentPos;  /* index pointer on the current moniker */

} EnumMonikerImpl;

static inline IMoniker *impl_from_IROTData( IROTData *iface )
{
    return (IMoniker *)((char*)iface - FIELD_OFFSET(CompositeMonikerImpl, lpvtbl2));
}

static inline IMoniker *impl_from_IMarshal( IMarshal *iface )
{
    return (IMoniker *)((char*)iface - FIELD_OFFSET(CompositeMonikerImpl, lpvtblMarshal));
}

static HRESULT EnumMonikerImpl_CreateEnumMoniker(IMoniker** tabMoniker,ULONG tabSize,ULONG currentPos,BOOL leftToRigth,IEnumMoniker ** ppmk);

/*******************************************************************************
 *        CompositeMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    CompositeMonikerImpl *This = (CompositeMonikerImpl *)iface;

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
        IsEqualIID(&IID_IMoniker, riid)
       )
        *ppvObject = iface;
    else if (IsEqualIID(&IID_IROTData, riid))
        *ppvObject = (IROTData*)&(This->lpvtbl2);
    else if (IsEqualIID(&IID_IMarshal, riid))
        *ppvObject = (IROTData*)&(This->lpvtblMarshal);

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI
CompositeMonikerImpl_AddRef(IMoniker* iface)
{
    CompositeMonikerImpl *This = (CompositeMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

static void CompositeMonikerImpl_ReleaseMonikersInTable(CompositeMonikerImpl *This)
{
    ULONG i;

    for (i = 0; i < This->tabLastIndex; i++)
        IMoniker_Release(This->tabMoniker[i]);

    This->tabLastIndex = 0;
}

/******************************************************************************
 *        CompositeMoniker_Release
 ******************************************************************************/
static ULONG WINAPI
CompositeMonikerImpl_Release(IMoniker* iface)
{
    CompositeMonikerImpl *This = (CompositeMonikerImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there's no more reference on it */
    if (ref == 0){

        /* release all the components before destroying this object */
        CompositeMonikerImpl_ReleaseMonikersInTable(This);

        HeapFree(GetProcessHeap(),0,This->tabMoniker);
        HeapFree(GetProcessHeap(),0,This);
    }
    return ref;
}

/******************************************************************************
 *        CompositeMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_CompositeMoniker;

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

/******************************************************************************
 *        CompositeMoniker_Load
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_Load(IMoniker* iface,IStream* pStm)
{
    HRESULT res;
    DWORD moniker_count;
    DWORD i;

    CompositeMonikerImpl *This = (CompositeMonikerImpl *)iface;

    TRACE("(%p,%p)\n",iface,pStm);

    /* this function call OleLoadFromStream function for each moniker within this object */

    res=IStream_Read(pStm,&moniker_count,sizeof(DWORD),NULL);
    if (res != S_OK)
    {
        ERR("couldn't reading moniker count from stream\n");
        return E_FAIL;
    }

    CompositeMonikerImpl_ReleaseMonikersInTable(This);

    for (i = 0; i < moniker_count; i++)
    {
        res=OleLoadFromStream(pStm,&IID_IMoniker,(void**)&This->tabMoniker[This->tabLastIndex]);
        if (FAILED(res))
        {
            ERR("couldn't load moniker from stream, res = 0x%08x\n", res);
            break;
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
static HRESULT WINAPI
CompositeMonikerImpl_Save(IMoniker* iface,IStream* pStm,BOOL fClearDirty)
{
    CompositeMonikerImpl *This = (CompositeMonikerImpl *)iface;
    HRESULT res;
    IEnumMoniker *enumMk;
    IMoniker *pmk;
    DWORD moniker_count = This->tabLastIndex;

    TRACE("(%p,%p,%d)\n",iface,pStm,fClearDirty);

    /* This function calls OleSaveToStream function for each moniker within
     * this object.
     * When I tested this function in windows, I usually found this constant
     * at the beginning of the stream. I don't known why (there's no
     * indication in the specification) !
     */
    res=IStream_Write(pStm,&moniker_count,sizeof(moniker_count),NULL);
    if (FAILED(res)) return res;

    IMoniker_Enum(iface,TRUE,&enumMk);

    while(IEnumMoniker_Next(enumMk,1,&pmk,NULL)==S_OK){

        res=OleSaveToStream((IPersistStream*)pmk,pStm);

        IMoniker_Release(pmk);

        if (FAILED(res)){

            IEnumMoniker_Release(enumMk);
            return res;
        }
    }

    IEnumMoniker_Release(enumMk);

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_GetSizeMax
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_GetSizeMax(IMoniker* iface,ULARGE_INTEGER* pcbSize)
{
    IEnumMoniker *enumMk;
    IMoniker *pmk;
    ULARGE_INTEGER ptmpSize;

    /* The sizeMax of this object is calculated by calling  GetSizeMax on
     * each moniker within this object then summing all returned values
     */

    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    pcbSize->QuadPart = sizeof(DWORD);

    IMoniker_Enum(iface,TRUE,&enumMk);

    while(IEnumMoniker_Next(enumMk,1,&pmk,NULL)==S_OK){

        IMoniker_GetSizeMax(pmk,&ptmpSize);

        IMoniker_Release(pmk);

        pcbSize->QuadPart = ptmpSize.QuadPart + sizeof(CLSID);
    }

    IEnumMoniker_Release(enumMk);

    return S_OK;
}

/******************************************************************************
 *                  CompositeMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_BindToObject(IMoniker* iface, IBindCtx* pbc,
               IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult)
{
    HRESULT   res;
    IRunningObjectTable *prot;
    IMoniker *tempMk,*antiMk,*mostRigthMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult);

    if (ppvResult==NULL)
        return E_POINTER;

    *ppvResult=0;
    /* If pmkToLeft is NULL, this method looks for the moniker in the ROT, and if found, queries the retrieved */
    /* object for the requested interface pointer. */
    if(pmkToLeft==NULL){

        res=IBindCtx_GetRunningObjectTable(pbc,&prot);

        if (SUCCEEDED(res)){

            /* if the requested class was loaded before ! we don't need to reload it */
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

        res=IMoniker_BindToObject(mostRigthMk,pbc,tempMk,riid,ppvResult);

        IMoniker_Release(tempMk);
        IMoniker_Release(mostRigthMk);
    }

    return res;
}

/******************************************************************************
 *        CompositeMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_BindToStorage(IMoniker* iface, IBindCtx* pbc,
               IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult)
{
    HRESULT   res;
    IMoniker *tempMk,*antiMk,*mostRigthMk,*leftMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,riid,ppvResult);

    *ppvResult=0;

    /* This method recursively calls BindToStorage on the rightmost component of the composite, */
    /* passing the rest of the composite as the pmkToLeft parameter for that call. */

    if (pmkToLeft)
    {
        res = IMoniker_ComposeWith(pmkToLeft, iface, FALSE, &leftMk);
        if (FAILED(res)) return res;
    }
    else
        leftMk = iface;

    IMoniker_Enum(iface, FALSE, &enumMoniker);
    IEnumMoniker_Next(enumMoniker, 1, &mostRigthMk, NULL);
    IEnumMoniker_Release(enumMoniker);

    res = CreateAntiMoniker(&antiMk);
    if (FAILED(res)) return res;
    res = IMoniker_ComposeWith(leftMk, antiMk, 0, &tempMk);
    if (FAILED(res)) return res;
    IMoniker_Release(antiMk);

    res = IMoniker_BindToStorage(mostRigthMk, pbc, tempMk, riid, ppvResult);

    IMoniker_Release(tempMk);

    IMoniker_Release(mostRigthMk);

    if (pmkToLeft)
        IMoniker_Release(leftMk);

    return res;
}

/******************************************************************************
 *        CompositeMoniker_Reduce
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
               IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    HRESULT   res;
    IMoniker *tempMk,*antiMk,*mostRigthMk,*leftReducedComposedMk,*mostRigthReducedMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%d,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

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

        return IMoniker_Reduce(mostRigthMk,pbc,dwReduceHowFar,&tempMk, ppmkReduced);
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
            IMoniker_Reduce(mostRigthMk,pbc,dwReduceHowFar,&tempMk,&leftReducedComposedMk)
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
static HRESULT WINAPI
CompositeMonikerImpl_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
               BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{
    TRACE("(%p,%p,%d,%p)\n",iface,pmkRight,fOnlyIfNotGeneric,ppmkComposite);

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
static HRESULT WINAPI
CompositeMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    CompositeMonikerImpl *This = (CompositeMonikerImpl *)iface;

    TRACE("(%p,%d,%p)\n",iface,fForward,ppenumMoniker);

    if (ppenumMoniker == NULL)
        return E_POINTER;

    return EnumMonikerImpl_CreateEnumMoniker(This->tabMoniker,This->tabLastIndex,0,fForward,ppenumMoniker);
}

/******************************************************************************
 *        CompositeMoniker_IsEqual
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    IEnumMoniker *enumMoniker1,*enumMoniker2;
    IMoniker *tempMk1,*tempMk2;
    HRESULT res1,res2,res;

    TRACE("(%p,%p)\n",iface,pmkOtherMoniker);

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
static HRESULT WINAPI
CompositeMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    IEnumMoniker *enumMoniker;
    IMoniker *tempMk;
    HRESULT res;
    DWORD tempHash;

    TRACE("(%p,%p)\n",iface,pdwHash);

    if (pdwHash==NULL)
        return E_POINTER;

    res = IMoniker_Enum(iface,TRUE,&enumMoniker);
    if(FAILED(res))
        return res;

    *pdwHash = 0;

    while(IEnumMoniker_Next(enumMoniker,1,&tempMk,NULL)==S_OK){
        res = IMoniker_Hash(tempMk, &tempHash);
        if(FAILED(res))
            break;
        *pdwHash = *pdwHash ^ tempHash;
        
        IMoniker_Release(tempMk);
    }

    IEnumMoniker_Release(enumMoniker);

    return res;
}

/******************************************************************************
 *        CompositeMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsRunning(IMoniker* iface, IBindCtx* pbc,
               IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning)
{
    IRunningObjectTable* rot;
    HRESULT res;
    IMoniker *tempMk,*antiMk,*mostRigthMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning);

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
static HRESULT WINAPI
CompositeMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc,
               IMoniker* pmkToLeft, FILETIME* pCompositeTime)
{
    HRESULT res;
    IMoniker *tempMk,*antiMk,*mostRigthMk,*leftMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pCompositeTime);

    if (pCompositeTime==NULL)
        return E_INVALIDARG;

    /* This method creates a composite of pmkToLeft (if non-NULL) and this moniker and uses the ROT to  */
    /* retrieve the time of last change. If the object is not in the ROT, the method recursively calls  */
    /* IMoniker::GetTimeOfLastChange on the rightmost component of the composite, passing the remainder */
    /* of the composite as the pmkToLeft parameter for that call.                                       */
    if (pmkToLeft)
    {
        IRunningObjectTable* rot;

        res = IMoniker_ComposeWith(pmkToLeft, iface, FALSE, &leftMk);

        res = IBindCtx_GetRunningObjectTable(pbc,&rot);
        if (FAILED(res))
        {
            IMoniker_Release(leftMk);
            return res;
        }

        if (IRunningObjectTable_GetTimeOfLastChange(rot,leftMk,pCompositeTime)==S_OK)
        {
            IMoniker_Release(leftMk);
            return res;
        }
    }
    else
        leftMk = iface;

    IMoniker_Enum(iface, FALSE, &enumMoniker);
    IEnumMoniker_Next(enumMoniker, 1, &mostRigthMk, NULL);
    IEnumMoniker_Release(enumMoniker);

    res = CreateAntiMoniker(&antiMk);
    res = IMoniker_ComposeWith(leftMk, antiMk, 0, &tempMk);
    IMoniker_Release(antiMk);

    res = IMoniker_GetTimeOfLastChange(mostRigthMk, pbc, tempMk, pCompositeTime);

    IMoniker_Release(tempMk);
    IMoniker_Release(mostRigthMk);

    if (pmkToLeft)
        IMoniker_Release(leftMk);

    return res;
}

/******************************************************************************
 *        CompositeMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    HRESULT res;
    IMoniker *tempMk,*antiMk,*mostRigthMk,*tempInvMk,*mostRigthInvMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p)\n",iface,ppmk);

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
static HRESULT WINAPI
CompositeMonikerImpl_CommonPrefixWith(IMoniker* iface, IMoniker* pmkOther,
               IMoniker** ppmkPrefix)
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
static VOID GetAfterCommonPrefix(IMoniker* pGenMk,IMoniker* commonMk,IMoniker** restMk)
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

        IMoniker_Release(tempMk);

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
static HRESULT WINAPI
CompositeMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmkOther,
               IMoniker** ppmkRelPath)
{
    HRESULT res;
    IMoniker *restOtherMk=0,*restThisMk=0,*invRestThisMk=0,*commonMk=0;

    TRACE("(%p,%p,%p)\n",iface,pmkOther,ppmkRelPath);

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
static HRESULT WINAPI
CompositeMonikerImpl_GetDisplayName(IMoniker* iface, IBindCtx* pbc,
               IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
    ULONG lengthStr=1;
    IEnumMoniker *enumMoniker;
    IMoniker* tempMk;
    LPOLESTR tempStr;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName);

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

        strcatW(*ppszDisplayName,tempStr);

        CoTaskMemFree(tempStr);
        IMoniker_Release(tempMk);
    }

    IEnumMoniker_Release(enumMoniker);

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_ParseDisplayName
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_ParseDisplayName(IMoniker* iface, IBindCtx* pbc,
               IMoniker* pmkToLeft, LPOLESTR pszDisplayName, ULONG* pchEaten,
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
static HRESULT WINAPI
CompositeMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_GENERICCOMPOSITE;

    return S_OK;
}

/*******************************************************************************
 *        CompositeMonikerIROTData_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
CompositeMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,
               VOID** ppvObject)
{

    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p,%p,%p)\n",iface,riid,ppvObject);

    return CompositeMonikerImpl_QueryInterface(This, riid, ppvObject);
}

/***********************************************************************
 *        CompositeMonikerIROTData_AddRef
 */
static ULONG WINAPI
CompositeMonikerROTDataImpl_AddRef(IROTData *iface)
{
    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_AddRef(This);
}

/***********************************************************************
 *        CompositeMonikerIROTData_Release
 */
static ULONG WINAPI CompositeMonikerROTDataImpl_Release(IROTData* iface)
{
    IMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_Release(This);
}

/******************************************************************************
 *        CompositeMonikerIROTData_GetComparisonData
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerROTDataImpl_GetComparisonData(IROTData* iface,
               BYTE* pbData, ULONG cbMax, ULONG* pcbData)
{
    IMoniker *This = impl_from_IROTData(iface);
    IEnumMoniker *pEnumMk;
    IMoniker *pmk;
    HRESULT hr;

    TRACE("(%p, %u, %p)\n", pbData, cbMax, pcbData);

    *pcbData = sizeof(CLSID);

    hr = IMoniker_Enum(This, TRUE, &pEnumMk);
    if (FAILED(hr)) return hr;

    while(IEnumMoniker_Next(pEnumMk, 1, &pmk, NULL) == S_OK)
    {
        IROTData *pROTData;
        hr = IMoniker_QueryInterface(pmk, &IID_IROTData, (void **)&pROTData);
        if (FAILED(hr))
            ERR("moniker doesn't support IROTData interface\n");

        if (SUCCEEDED(hr))
        {
            ULONG cbData;
            hr = IROTData_GetComparisonData(pROTData, NULL, 0, &cbData);
            IROTData_Release(pROTData);
            if (SUCCEEDED(hr) || (hr == E_OUTOFMEMORY))
            {
                *pcbData += cbData;
                hr = S_OK;
            }
            else
                ERR("IROTData_GetComparisonData failed with error 0x%08x\n", hr);
        }

        IMoniker_Release(pmk);

        if (FAILED(hr))
        {
            IEnumMoniker_Release(pEnumMk);
            return hr;
        }
    }
    if (cbMax < *pcbData)
        return E_OUTOFMEMORY;

    IEnumMoniker_Reset(pEnumMk);

    memcpy(pbData, &CLSID_CompositeMoniker, sizeof(CLSID));
    pbData += sizeof(CLSID);
    cbMax -= sizeof(CLSID);

    while (IEnumMoniker_Next(pEnumMk, 1, &pmk, NULL) == S_OK)
    {
        IROTData *pROTData;
        hr = IMoniker_QueryInterface(pmk, &IID_IROTData, (void **)&pROTData);
        if (FAILED(hr))
            ERR("moniker doesn't support IROTData interface\n");

        if (SUCCEEDED(hr))
        {
            ULONG cbData;
            hr = IROTData_GetComparisonData(pROTData, pbData, cbMax, &cbData);
            IROTData_Release(pROTData);
            if (SUCCEEDED(hr))
            {
                pbData += cbData;
                cbMax -= cbData;
            }
            else
                ERR("IROTData_GetComparisonData failed with error 0x%08x\n", hr);
        }

        IMoniker_Release(pmk);

        if (FAILED(hr))
        {
            IEnumMoniker_Release(pEnumMk);
            return hr;
        }
    }

    IEnumMoniker_Release(pEnumMk);

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_QueryInterface(IMarshal *iface, REFIID riid, LPVOID *ppv)
{
    IMoniker *This = impl_from_IMarshal(iface);

    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppv);

    return CompositeMonikerImpl_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI CompositeMonikerMarshalImpl_AddRef(IMarshal *iface)
{
    IMoniker *This = impl_from_IMarshal(iface);

    TRACE("(%p)\n",iface);

    return CompositeMonikerImpl_AddRef(This);
}

static ULONG WINAPI CompositeMonikerMarshalImpl_Release(IMarshal *iface)
{
    IMoniker *This = impl_from_IMarshal(iface);

    TRACE("(%p)\n",iface);

    return CompositeMonikerImpl_Release(This);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_GetUnmarshalClass(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, CLSID* pCid)
{
    IMoniker *This = impl_from_IMarshal(iface);

    TRACE("(%s, %p, %x, %p, %x, %p)\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pCid);

    return IMoniker_GetClassID(This, pCid);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_GetMarshalSizeMax(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, DWORD* pSize)
{
    IMoniker *This = impl_from_IMarshal(iface);
    IEnumMoniker *pEnumMk;
    IMoniker *pmk;
    HRESULT hr;
    ULARGE_INTEGER size;

    TRACE("(%s, %p, %x, %p, %x, %p)\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pSize);

    *pSize = 0x10; /* to match native */

    hr = IMoniker_Enum(This, TRUE, &pEnumMk);
    if (FAILED(hr)) return hr;

    hr = IMoniker_GetSizeMax(This, &size);

    while (IEnumMoniker_Next(pEnumMk, 1, &pmk, NULL) == S_OK)
    {
        ULONG size;

        hr = CoGetMarshalSizeMax(&size, &IID_IMoniker, (IUnknown *)pmk, dwDestContext, pvDestContext, mshlflags);
        if (SUCCEEDED(hr))
            *pSize += size;

        IMoniker_Release(pmk);

        if (FAILED(hr))
        {
            IEnumMoniker_Release(pEnumMk);
            return hr;
        }
    }

    IEnumMoniker_Release(pEnumMk);

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_MarshalInterface(LPMARSHAL iface, IStream *pStm, 
    REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags)
{
    IMoniker *This = impl_from_IMarshal(iface);
    IEnumMoniker *pEnumMk;
    IMoniker *pmk;
    HRESULT hr;
    ULONG i = 0;

    TRACE("(%p, %s, %p, %x, %p, %x)\n", pStm, debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags);

    hr = IMoniker_Enum(This, TRUE, &pEnumMk);
    if (FAILED(hr)) return hr;

    while (IEnumMoniker_Next(pEnumMk, 1, &pmk, NULL) == S_OK)
    {
        hr = CoMarshalInterface(pStm, &IID_IMoniker, (IUnknown *)pmk, dwDestContext, pvDestContext, mshlflags);

        IMoniker_Release(pmk);

        if (FAILED(hr))
        {
            IEnumMoniker_Release(pEnumMk);
            return hr;
        }
        i++;
    }

    if (i != 2)
        FIXME("moniker count of %d not supported\n", i);

    IEnumMoniker_Release(pEnumMk);

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_UnmarshalInterface(LPMARSHAL iface, IStream *pStm, REFIID riid, void **ppv)
{
    CompositeMonikerImpl *This = (CompositeMonikerImpl *)impl_from_IMarshal(iface);
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", pStm, debugstr_guid(riid), ppv);

    CompositeMonikerImpl_ReleaseMonikersInTable(This);

    /* resize the table if needed */
    if (This->tabLastIndex + 2 > This->tabSize)
    {
        This->tabSize += max(BLOCK_TAB_SIZE, 2);
        This->tabMoniker=HeapReAlloc(GetProcessHeap(),0,This->tabMoniker,This->tabSize*sizeof(IMoniker));

        if (This->tabMoniker==NULL)
            return E_OUTOFMEMORY;
    }

    hr = CoUnmarshalInterface(pStm, &IID_IMoniker, (void**)&This->tabMoniker[This->tabLastIndex]);
    if (FAILED(hr))
    {
        ERR("couldn't unmarshal moniker, hr = 0x%08x\n", hr);
        return hr;
    }
    This->tabLastIndex++;
    hr = CoUnmarshalInterface(pStm, &IID_IMoniker, (void**)&This->tabMoniker[This->tabLastIndex]);
    if (FAILED(hr))
    {
        ERR("couldn't unmarshal moniker, hr = 0x%08x\n", hr);
        return hr;
    }
    This->tabLastIndex++;

    return IMoniker_QueryInterface((IMoniker *)&This->lpvtbl1, riid, ppv);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_ReleaseMarshalData(LPMARSHAL iface, IStream *pStm)
{
    TRACE("(%p)\n", pStm);
    /* can't release a state-based marshal as nothing on server side to
     * release */
    return S_OK;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_DisconnectObject(LPMARSHAL iface, DWORD dwReserved)
{
    TRACE("(0x%x)\n", dwReserved);
    /* can't disconnect a state-based marshal as nothing on server side to
     * disconnect from */
    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_QueryInterface
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_QueryInterface(IEnumMoniker* iface,REFIID riid,void** ppvObject)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,riid,ppvObject);

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
    IEnumMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_AddRef
 ******************************************************************************/
static ULONG WINAPI
EnumMonikerImpl_AddRef(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);

}

/******************************************************************************
 *        EnumMonikerImpl_Release
 ******************************************************************************/
static ULONG WINAPI
EnumMonikerImpl_Release(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;
    ULONG i;
    ULONG ref;
    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there's no more reference on it */
    if (ref == 0) {

        for(i=0;i<This->tabSize;i++)
            IMoniker_Release(This->tabMoniker[i]);

        HeapFree(GetProcessHeap(),0,This->tabMoniker);
        HeapFree(GetProcessHeap(),0,This);
    }
    return ref;
}

/******************************************************************************
 *        EnumMonikerImpl_Next
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_Next(IEnumMoniker* iface,ULONG celt, IMoniker** rgelt,
               ULONG* pceltFethed)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;
    ULONG i;

    /* retrieve the requested number of moniker from the current position */
    for(i=0;((This->currentPos < This->tabSize) && (i < celt));i++)
    {
        rgelt[i]=This->tabMoniker[This->currentPos++];
        IMoniker_AddRef(rgelt[i]);
    }

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
static HRESULT WINAPI
EnumMonikerImpl_Skip(IEnumMoniker* iface,ULONG celt)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    if ((This->currentPos+celt) >= This->tabSize)
        return S_FALSE;

    This->currentPos+=celt;

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_Reset
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_Reset(IEnumMoniker* iface)
{

    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    This->currentPos=0;

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_Clone
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_Clone(IEnumMoniker* iface,IEnumMoniker** ppenum)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    return EnumMonikerImpl_CreateEnumMoniker(This->tabMoniker,This->tabSize,This->currentPos,TRUE,ppenum);
}

/********************************************************************************/
/* Virtual function table for the IROTData class                                */
static const IEnumMonikerVtbl VT_EnumMonikerImpl =
{
    EnumMonikerImpl_QueryInterface,
    EnumMonikerImpl_AddRef,
    EnumMonikerImpl_Release,
    EnumMonikerImpl_Next,
    EnumMonikerImpl_Skip,
    EnumMonikerImpl_Reset,
    EnumMonikerImpl_Clone
};

/******************************************************************************
 *        EnumMonikerImpl_CreateEnumMoniker
 ******************************************************************************/
static HRESULT
EnumMonikerImpl_CreateEnumMoniker(IMoniker** tabMoniker, ULONG tabSize,
               ULONG currentPos, BOOL leftToRigth, IEnumMoniker ** ppmk)
{
    EnumMonikerImpl* newEnumMoniker;
    int i;

    if (currentPos > tabSize)
        return E_INVALIDARG;

    newEnumMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(EnumMonikerImpl));

    if (newEnumMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    /* Initialize the virtual function table. */
    newEnumMoniker->lpVtbl       = &VT_EnumMonikerImpl;
    newEnumMoniker->ref          = 1;

    newEnumMoniker->tabSize=tabSize;
    newEnumMoniker->currentPos=currentPos;

    newEnumMoniker->tabMoniker=HeapAlloc(GetProcessHeap(),0,tabSize*sizeof(IMoniker));

    if (newEnumMoniker->tabMoniker==NULL) {
        HeapFree(GetProcessHeap(), 0, newEnumMoniker);
        return E_OUTOFMEMORY;
    }

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

/********************************************************************************/
/* Virtual function table for the CompositeMonikerImpl class which includes     */
/* IPersist, IPersistStream and IMoniker functions.                             */

static const IMonikerVtbl VT_CompositeMonikerImpl =
{
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
static const IROTDataVtbl VT_ROTDataImpl =
{
    CompositeMonikerROTDataImpl_QueryInterface,
    CompositeMonikerROTDataImpl_AddRef,
    CompositeMonikerROTDataImpl_Release,
    CompositeMonikerROTDataImpl_GetComparisonData
};

static const IMarshalVtbl VT_MarshalImpl =
{
    CompositeMonikerMarshalImpl_QueryInterface,
    CompositeMonikerMarshalImpl_AddRef,
    CompositeMonikerMarshalImpl_Release,
    CompositeMonikerMarshalImpl_GetUnmarshalClass,
    CompositeMonikerMarshalImpl_GetMarshalSizeMax,
    CompositeMonikerMarshalImpl_MarshalInterface,
    CompositeMonikerMarshalImpl_UnmarshalInterface,
    CompositeMonikerMarshalImpl_ReleaseMarshalData,
    CompositeMonikerMarshalImpl_DisconnectObject
};

/******************************************************************************
 *         Composite-Moniker_Construct (local function)
 *******************************************************************************/
static HRESULT
CompositeMonikerImpl_Construct(IMoniker** ppMoniker,
               LPMONIKER pmkFirst, LPMONIKER pmkRest)
{
    DWORD mkSys;
    IEnumMoniker *enumMoniker;
    IMoniker *tempMk;
    HRESULT res;
    CompositeMonikerImpl *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));

    if (!This)
        return E_OUTOFMEMORY;

    TRACE("(%p,%p,%p)\n",This,pmkFirst,pmkRest);

    /* Initialize the virtual function table. */
    This->lpvtbl1      = &VT_CompositeMonikerImpl;
    This->lpvtbl2      = &VT_ROTDataImpl;
    This->lpvtblMarshal= &VT_MarshalImpl;
    This->ref          = 1;

    This->tabSize=BLOCK_TAB_SIZE;
    This->tabLastIndex=0;

    This->tabMoniker=HeapAlloc(GetProcessHeap(),0,This->tabSize*sizeof(IMoniker));
    if (This->tabMoniker==NULL)
        return E_OUTOFMEMORY;

    if (!pmkFirst && !pmkRest)
    {
        *ppMoniker = (IMoniker *)This;
        return S_OK;
    }

    IMoniker_IsSystemMoniker(pmkFirst,&mkSys);

    /* put the first moniker contents in the beginning of the table */
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

        /* add a composite moniker to the moniker table (do the same thing
         * for each moniker within the composite moniker as a simple moniker
         * (see above for how to add a simple moniker case) )
         */
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

    /* only one moniker, then just return it */
    if (This->tabLastIndex == 1)
    {
        *ppMoniker = This->tabMoniker[0];
        IMoniker_AddRef(*ppMoniker);
        IMoniker_Release((IMoniker *)This);
    }
    else
        *ppMoniker = (IMoniker *)This;

    return S_OK;
}

/******************************************************************************
 *        CreateGenericComposite	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI
CreateGenericComposite(LPMONIKER pmkFirst, LPMONIKER pmkRest,
               LPMONIKER* ppmkComposite)
{
    IMoniker* moniker = 0;
    HRESULT        hr = S_OK;

    TRACE("(%p,%p,%p)\n",pmkFirst,pmkRest,ppmkComposite);

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

    hr = CompositeMonikerImpl_Construct(&moniker,pmkFirst,pmkRest);

    if (FAILED(hr))
        return hr;

    hr = IMoniker_QueryInterface(moniker,&IID_IMoniker,(void**)ppmkComposite);
    IMoniker_Release(moniker);

    return hr;
}

/******************************************************************************
 *        MonikerCommonPrefixWith	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI
MonikerCommonPrefixWith(IMoniker* pmkThis,IMoniker* pmkOther,IMoniker** ppmkCommon)
{
    FIXME("(),stub!\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI CompositeMonikerCF_QueryInterface(LPCLASSFACTORY iface,
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

static ULONG WINAPI CompositeMonikerCF_AddRef(LPCLASSFACTORY iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI CompositeMonikerCF_Release(LPCLASSFACTORY iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI CompositeMonikerCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pUnk, REFIID riid, LPVOID *ppv)
{
    IMoniker* pMoniker;
    HRESULT  hr;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CompositeMonikerImpl_Construct(&pMoniker, NULL, NULL);

    if (SUCCEEDED(hr))
    {
        hr = IMoniker_QueryInterface(pMoniker, riid, ppv);
        IMoniker_Release(pMoniker);
    }

    return hr;
}

static HRESULT WINAPI CompositeMonikerCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl CompositeMonikerCFVtbl =
{
    CompositeMonikerCF_QueryInterface,
    CompositeMonikerCF_AddRef,
    CompositeMonikerCF_Release,
    CompositeMonikerCF_CreateInstance,
    CompositeMonikerCF_LockServer
};
static const IClassFactoryVtbl *CompositeMonikerCF = &CompositeMonikerCFVtbl;

HRESULT CompositeMonikerCF_Create(REFIID riid, LPVOID *ppv)
{
    return IClassFactory_QueryInterface((IClassFactory *)&CompositeMonikerCF, riid, ppv);
}
