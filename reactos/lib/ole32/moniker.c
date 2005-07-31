/*
 *	Monikers
 *
 *	Copyright 1998	Marcus Meissner
 *      Copyright 1999  Noomen Hamza
 *      Copyright 2005  Robert Shearman (for CodeWeavers)
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
 *
 * TODO:
 * - IRunningObjectTable should work interprocess, but currently doesn't.
 *   Native (on Win2k at least) uses an undocumented RPC interface, IROT, to
 *   communicate with RPCSS which contains the table of marshalled data.
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wtypes.h"
#include "ole2.h"

#include "wine/list.h"
#include "wine/debug.h"

#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* define the structure of the running object table elements */
struct rot_entry
{
    struct list        entry;
    MInterfacePointer* object; /* marshaled running object*/
    MInterfacePointer* moniker; /* marshaled moniker that identifies this object */
    MInterfacePointer* moniker_data; /* moniker comparison data that identifies this object */
    DWORD              cookie; /* cookie identifying this object */
    FILETIME           last_modified;
};

/* define the RunningObjectTableImpl structure */
typedef struct RunningObjectTableImpl
{
    const IRunningObjectTableVtbl *lpVtbl;
    ULONG ref;

    struct list rot; /* list of ROT entries */
    CRITICAL_SECTION lock;
} RunningObjectTableImpl;

static RunningObjectTableImpl* runningObjectTableInstance = NULL;



static inline HRESULT WINAPI
IrotRegister(DWORD *cookie)
{
    static DWORD last_cookie = 1;
    *cookie = InterlockedIncrement(&last_cookie);
    return S_OK;
}

/* define the EnumMonikerImpl structure */
typedef struct EnumMonikerImpl
{
    const IEnumMonikerVtbl *lpVtbl;
    ULONG      ref;

    MInterfacePointer **monikers;
    ULONG moniker_count;
    ULONG pos;
} EnumMonikerImpl;


/* IEnumMoniker Local functions*/
static HRESULT WINAPI EnumMonikerImpl_CreateEnumROTMoniker(MInterfacePointer **monikers,
    ULONG moniker_count, ULONG pos, IEnumMoniker **ppenumMoniker);

static HRESULT create_stream_on_mip_ro(const MInterfacePointer *mip, IStream **stream)
{
    HGLOBAL hglobal = GlobalAlloc(0, mip->ulCntData);
    void *pv = GlobalLock(hglobal);
    memcpy(pv, mip->abData, mip->ulCntData);
    GlobalUnlock(hglobal);
    return CreateStreamOnHGlobal(hglobal, TRUE, stream);
}

static inline void rot_entry_delete(struct rot_entry *rot_entry)
{
    /* FIXME: revoke entry from rpcss's copy of the ROT */
    if (rot_entry->object)
    {
        IStream *stream;
        HRESULT hr;
        hr = create_stream_on_mip_ro(rot_entry->object, &stream);
        if (hr == S_OK)
        {
            CoReleaseMarshalData(stream);
            IUnknown_Release(stream);
        }
    }
    if (rot_entry->moniker)
    {
        IStream *stream;
        HRESULT hr;
        hr = create_stream_on_mip_ro(rot_entry->moniker, &stream);
        if (hr == S_OK)
        {
            CoReleaseMarshalData(stream);
            IUnknown_Release(stream);
        }
    }
    HeapFree(GetProcessHeap(), 0, rot_entry->object);
    HeapFree(GetProcessHeap(), 0, rot_entry->moniker);
    HeapFree(GetProcessHeap(), 0, rot_entry->moniker_data);
    HeapFree(GetProcessHeap(), 0, rot_entry);
}

/* moniker_data must be freed with HeapFree when no longer in use */
static HRESULT get_moniker_comparison_data(IMoniker *pMoniker, MInterfacePointer **moniker_data)
{
    HRESULT hr;
    IROTData *pROTData = NULL;
    ULONG size = 0;
    hr = IMoniker_QueryInterface(pMoniker, &IID_IROTData, (void *)&pROTData);
    if (hr != S_OK)
    {
        ERR("Failed to query moniker for IROTData interface, hr = 0x%08lx\n", hr);
        return hr;
    }
    IROTData_GetComparisonData(pROTData, NULL, 0, &size);
    *moniker_data = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MInterfacePointer, abData[size]));
    (*moniker_data)->ulCntData = size;
    hr = IROTData_GetComparisonData(pROTData, (*moniker_data)->abData, size, &size);
    if (hr != S_OK)
    {
        ERR("Failed to copy comparison data into buffer, hr = 0x%08lx\n", hr);
        HeapFree(GetProcessHeap(), 0, *moniker_data);
        return hr;
    }
    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_QueryInterface
 */
static HRESULT WINAPI
RunningObjectTableImpl_QueryInterface(IRunningObjectTable* iface,
                                      REFIID riid,void** ppvObject)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,riid,ppvObject);

    /* validate arguments */

    if (ppvObject==0)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IRunningObjectTable, riid))
        *ppvObject = (IRunningObjectTable*)This;

    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    IRunningObjectTable_AddRef(iface);

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_AddRef
 */
static ULONG WINAPI
RunningObjectTableImpl_AddRef(IRunningObjectTable* iface)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/***********************************************************************
 *        RunningObjectTable_Initialize
 */
static HRESULT WINAPI
RunningObjectTableImpl_Destroy(void)
{
    struct list *cursor, *cursor2;

    TRACE("()\n");

    if (runningObjectTableInstance==NULL)
        return E_INVALIDARG;

    /* free the ROT table memory */
    LIST_FOR_EACH_SAFE(cursor, cursor2, &runningObjectTableInstance->rot)
    {
        struct rot_entry *rot_entry = LIST_ENTRY(cursor, struct rot_entry, entry);
        list_remove(&rot_entry->entry);
        rot_entry_delete(rot_entry);
    }

    /* free the ROT structure memory */
    HeapFree(GetProcessHeap(),0,runningObjectTableInstance);
    runningObjectTableInstance = NULL;

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_Release
 */
static ULONG WINAPI
RunningObjectTableImpl_Release(IRunningObjectTable* iface)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* uninitialize ROT structure if there's no more references to it */
    if (ref == 0)
    {
        struct list *cursor, *cursor2;
        LIST_FOR_EACH_SAFE(cursor, cursor2, &This->rot)
        {
            struct rot_entry *rot_entry = LIST_ENTRY(cursor, struct rot_entry, entry);
            list_remove(&rot_entry->entry);
            rot_entry_delete(rot_entry);
        }
        /*  RunningObjectTable data structure will be not destroyed here ! the destruction will be done only
         *  when RunningObjectTableImpl_UnInitialize function is called
         */
    }

    return ref;
}

/***********************************************************************
 *        RunningObjectTable_Register
 *
 * PARAMS
 * grfFlags       [in] Registration options 
 * punkObject     [in] the object being registered
 * pmkObjectName  [in] the moniker of the object being registered
 * pdwRegister    [in] the value identifying the registration
 */
static HRESULT WINAPI
RunningObjectTableImpl_Register(IRunningObjectTable* iface, DWORD grfFlags,
               IUnknown *punkObject, IMoniker *pmkObjectName, DWORD *pdwRegister)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    struct rot_entry *rot_entry;
    HRESULT hr = S_OK;
    IStream *pStream = NULL;
    DWORD mshlflags;

    TRACE("(%p,%ld,%p,%p,%p)\n",This,grfFlags,punkObject,pmkObjectName,pdwRegister);

    /*
     * there's only two types of register : strong and or weak registration
     * (only one must be passed on parameter)
     */
    if ( ( (grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) || !(grfFlags & ROTFLAGS_ALLOWANYCLIENT)) &&
         (!(grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) ||  (grfFlags & ROTFLAGS_ALLOWANYCLIENT)) &&
         (grfFlags) )
    {
        ERR("Invalid combination of ROTFLAGS: %lx\n", grfFlags);
        return E_INVALIDARG;
    }

    if (punkObject==NULL || pmkObjectName==NULL || pdwRegister==NULL)
        return E_INVALIDARG;

    rot_entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*rot_entry));
    if (!rot_entry)
        return E_OUTOFMEMORY;

    CoFileTimeNow(&rot_entry->last_modified);

    /* marshal object */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        return hr;
    }
    mshlflags = (grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) ? MSHLFLAGS_TABLESTRONG : MSHLFLAGS_TABLEWEAK;
    hr = CoMarshalInterface(pStream, &IID_IUnknown, punkObject, MSHCTX_LOCAL | MSHCTX_NOSHAREDMEM, NULL, mshlflags);
    /* FIXME: a cleaner way would be to create an IStream class that writes
     * directly to an MInterfacePointer */
    if (hr == S_OK)
    {
        HGLOBAL hglobal;
        hr = GetHGlobalFromStream(pStream, &hglobal);
        if (hr == S_OK)
        {
            SIZE_T size = GlobalSize(hglobal);
            const void *pv = GlobalLock(hglobal);
            rot_entry->object = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MInterfacePointer, abData[size]));
            rot_entry->object->ulCntData = size;
            memcpy(&rot_entry->object->abData, pv, size);
            GlobalUnlock(hglobal);
        }
    }
    IStream_Release(pStream);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        return hr;
    }

    hr = get_moniker_comparison_data(pmkObjectName, &rot_entry->moniker_data);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        return hr;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        return hr;
    }
    /* marshal moniker */
    hr = CoMarshalInterface(pStream, &IID_IMoniker, (IUnknown *)pmkObjectName, MSHCTX_LOCAL | MSHCTX_NOSHAREDMEM, NULL, MSHLFLAGS_TABLESTRONG);
    /* FIXME: a cleaner way would be to create an IStream class that writes
     * directly to an MInterfacePointer */
    if (hr == S_OK)
    {
        HGLOBAL hglobal;
        hr = GetHGlobalFromStream(pStream, &hglobal);
        if (hr == S_OK)
        {
            SIZE_T size = GlobalSize(hglobal);
            const void *pv = GlobalLock(hglobal);
            rot_entry->moniker = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MInterfacePointer, abData[size]));
            rot_entry->moniker->ulCntData = size;
            memcpy(&rot_entry->moniker->abData, pv, size);
            GlobalUnlock(hglobal);
        }
    }
    IStream_Release(pStream);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        return hr;
    }

    /* FIXME: not the right signature of IrotRegister function */
    hr = IrotRegister(&rot_entry->cookie);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        return hr;
    }

    /* gives a registration identifier to the registered object*/
    *pdwRegister = rot_entry->cookie;

    EnterCriticalSection(&This->lock);
    /* FIXME: see if object was registered before and return MK_S_MONIKERALREADYREGISTERED */
    list_add_tail(&This->rot, &rot_entry->entry);
    LeaveCriticalSection(&This->lock);

    return hr;
}

/***********************************************************************
 *        RunningObjectTable_Revoke
 *
 * PARAMS
 *  dwRegister [in] Value identifying registration to be revoked
 */
static HRESULT WINAPI
RunningObjectTableImpl_Revoke( IRunningObjectTable* iface, DWORD dwRegister) 
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    struct rot_entry *rot_entry;

    TRACE("(%p,%ld)\n",This,dwRegister);

    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, struct rot_entry, entry)
    {
        if (rot_entry->cookie == dwRegister)
        {
            list_remove(&rot_entry->entry);
            LeaveCriticalSection(&This->lock);

            rot_entry_delete(rot_entry);
            return S_OK;
        }
    }
    LeaveCriticalSection(&This->lock);

    return E_INVALIDARG;
}

/***********************************************************************
 *        RunningObjectTable_IsRunning
 *
 * PARAMS
 *  pmkObjectName [in]  moniker of the object whose status is desired 
 */
static HRESULT WINAPI
RunningObjectTableImpl_IsRunning( IRunningObjectTable* iface, IMoniker *pmkObjectName)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    MInterfacePointer *moniker_data;
    HRESULT hr;
    struct rot_entry *rot_entry;

    TRACE("(%p,%p)\n",This,pmkObjectName);

    hr = get_moniker_comparison_data(pmkObjectName, &moniker_data);
    if (hr != S_OK)
        return hr;

    hr = S_FALSE;
    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, struct rot_entry, entry)
    {
        if ((rot_entry->moniker_data->ulCntData == moniker_data->ulCntData) &&
            !memcmp(moniker_data, rot_entry->moniker_data, moniker_data->ulCntData))
        {
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&This->lock);

    /* FIXME: call IrotIsRunning */

    HeapFree(GetProcessHeap(), 0, moniker_data);

    return hr;
}

/***********************************************************************
 *        RunningObjectTable_GetObject
 *
 * PARAMS
 * pmkObjectName [in] Pointer to the moniker on the object 
 * ppunkObject   [out] variable that receives the IUnknown interface pointer
 */
static HRESULT WINAPI
RunningObjectTableImpl_GetObject( IRunningObjectTable* iface,
                     IMoniker *pmkObjectName, IUnknown **ppunkObject)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    MInterfacePointer *moniker_data;
    HRESULT hr;
    struct rot_entry *rot_entry;

    TRACE("(%p,%p,%p)\n",This,pmkObjectName,ppunkObject);

    if (ppunkObject == NULL)
        return E_POINTER;

    *ppunkObject = NULL;

    hr = get_moniker_comparison_data(pmkObjectName, &moniker_data);
    if (hr != S_OK)
        return hr;

    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, struct rot_entry, entry)
    {
        if ((rot_entry->moniker_data->ulCntData == moniker_data->ulCntData) &&
            !memcmp(moniker_data, rot_entry->moniker_data, moniker_data->ulCntData))
        {
            IStream *pStream;
            hr = create_stream_on_mip_ro(rot_entry->object, &pStream);
            if (hr == S_OK)
            {
                hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)ppunkObject);
                IStream_Release(pStream);
            }

            LeaveCriticalSection(&This->lock);
            HeapFree(GetProcessHeap(), 0, moniker_data);

            return hr;
        }
    }
    LeaveCriticalSection(&This->lock);

    /* FIXME: call IrotGetObject */
    WARN("Moniker unavailable - app may require interprocess running object table\n");
    hr = MK_E_UNAVAILABLE;

    HeapFree(GetProcessHeap(), 0, moniker_data);

    return hr;
}

/***********************************************************************
 *        RunningObjectTable_NoteChangeTime
 *
 * PARAMS
 *  dwRegister [in] Value identifying registration being updated
 *  pfiletime  [in] Pointer to structure containing object's last change time
 */
static HRESULT WINAPI
RunningObjectTableImpl_NoteChangeTime(IRunningObjectTable* iface,
                                      DWORD dwRegister, FILETIME *pfiletime)
{
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    struct rot_entry *rot_entry;

    TRACE("(%p,%ld,%p)\n",This,dwRegister,pfiletime);

    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, struct rot_entry, entry)
    {
        if (rot_entry->cookie == dwRegister)
        {
            rot_entry->last_modified = *pfiletime;
            LeaveCriticalSection(&This->lock);
            return S_OK;
        }
    }
    LeaveCriticalSection(&This->lock);

    /* FIXME: call IrotNoteChangeTime */

    return E_INVALIDARG;
}

/***********************************************************************
 *        RunningObjectTable_GetTimeOfLastChange
 *
 * PARAMS
 *  pmkObjectName  [in]  moniker of the object whose status is desired 
 *  pfiletime      [out] structure that receives object's last change time
 */
static HRESULT WINAPI
RunningObjectTableImpl_GetTimeOfLastChange(IRunningObjectTable* iface,
                            IMoniker *pmkObjectName, FILETIME *pfiletime)
{
    HRESULT hr = MK_E_UNAVAILABLE;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    MInterfacePointer *moniker_data;
    struct rot_entry *rot_entry;

    TRACE("(%p,%p,%p)\n",This,pmkObjectName,pfiletime);

    if (pmkObjectName==NULL || pfiletime==NULL)
        return E_INVALIDARG;

    hr = get_moniker_comparison_data(pmkObjectName, &moniker_data);
    if (hr != S_OK)
        return hr;

    hr = MK_E_UNAVAILABLE;

    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, struct rot_entry, entry)
    {
        if ((rot_entry->moniker_data->ulCntData == moniker_data->ulCntData) &&
            !memcmp(moniker_data, rot_entry->moniker_data, moniker_data->ulCntData))
        {
            *pfiletime = rot_entry->last_modified;
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&This->lock);

    /* FIXME: if (hr != S_OK) call IrotGetTimeOfLastChange */

    HeapFree(GetProcessHeap(), 0, moniker_data);
    return hr;
}

/***********************************************************************
 *        RunningObjectTable_EnumRunning
 *
 * PARAMS
 *  ppenumMoniker  [out]  receives the IEnumMoniker interface pointer 
 */
static HRESULT WINAPI
RunningObjectTableImpl_EnumRunning(IRunningObjectTable* iface,
                                   IEnumMoniker **ppenumMoniker)
{
    HRESULT hr;
    RunningObjectTableImpl *This = (RunningObjectTableImpl *)iface;
    MInterfacePointer **monikers;
    ULONG moniker_count = 0;
    const struct rot_entry *rot_entry;
    ULONG i = 0;

    EnterCriticalSection(&This->lock);

    LIST_FOR_EACH_ENTRY( rot_entry, &This->rot, const struct rot_entry, entry )
        moniker_count++;

    monikers = HeapAlloc(GetProcessHeap(), 0, moniker_count * sizeof(*monikers));

    LIST_FOR_EACH_ENTRY( rot_entry, &This->rot, const struct rot_entry, entry )
    {
        SIZE_T size = FIELD_OFFSET(MInterfacePointer, abData[rot_entry->moniker->ulCntData]);
        monikers[i] = HeapAlloc(GetProcessHeap(), 0, size);
        memcpy(monikers[i], rot_entry->moniker, size);
        i++;
    }

    LeaveCriticalSection(&This->lock);
    
    /* FIXME: call IrotEnumRunning and append data */

    hr = EnumMonikerImpl_CreateEnumROTMoniker(monikers, moniker_count, 0, ppenumMoniker);

    return hr;
}

/******************************************************************************
 *		GetRunningObjectTable (OLE2.30)
 */
HRESULT WINAPI
GetRunningObjectTable16(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    FIXME("(%ld,%p),stub!\n",reserved,pprot);
    return E_NOTIMPL;
}

/***********************************************************************
 *           GetRunningObjectTable (OLE32.@)
 */
HRESULT WINAPI
GetRunningObjectTable(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    IID riid=IID_IRunningObjectTable;
    HRESULT res;

    TRACE("()\n");

    if (reserved!=0)
        return E_UNEXPECTED;

    if(runningObjectTableInstance==NULL)
        return CO_E_NOTINITIALIZED;

    res = IRunningObjectTable_QueryInterface((IRunningObjectTable*)runningObjectTableInstance,&riid,(void**)pprot);

    return res;
}

/******************************************************************************
 *              OleRun        [OLE32.@]
 */
HRESULT WINAPI OleRun(LPUNKNOWN pUnknown)
{
    IRunnableObject *runable;
    IRunnableObject *This = (IRunnableObject *)pUnknown;
    LRESULT ret;

    ret = IRunnableObject_QueryInterface(This,&IID_IRunnableObject,(LPVOID*)&runable);
    if (ret)
        return 0; /* Appears to return no error. */
    ret = IRunnableObject_Run(runable,NULL);
    IRunnableObject_Release(runable);
    return ret;
}

/******************************************************************************
 *              MkParseDisplayName        [OLE32.@]
 */
HRESULT WINAPI MkParseDisplayName(LPBC pbc, LPCOLESTR szUserName,
				LPDWORD pchEaten, LPMONIKER *ppmk)
{
    FIXME("(%p, %s, %p, %p): stub.\n", pbc, debugstr_w(szUserName), pchEaten, *ppmk);

    if (!(IsValidInterface((LPUNKNOWN) pbc)))
        return E_INVALIDARG;

    return MK_E_SYNTAX;
}

/******************************************************************************
 *              CreateClassMoniker        [OLE32.@]
 */
HRESULT WINAPI CreateClassMoniker(REFCLSID rclsid, IMoniker ** ppmk)
{
    FIXME("%s\n", debugstr_guid( rclsid ));
    if( ppmk )
        *ppmk = NULL;
    return E_NOTIMPL;
}

/* Virtual function table for the IRunningObjectTable class. */
static IRunningObjectTableVtbl VT_RunningObjectTableImpl =
{
    RunningObjectTableImpl_QueryInterface,
    RunningObjectTableImpl_AddRef,
    RunningObjectTableImpl_Release,
    RunningObjectTableImpl_Register,
    RunningObjectTableImpl_Revoke,
    RunningObjectTableImpl_IsRunning,
    RunningObjectTableImpl_GetObject,
    RunningObjectTableImpl_NoteChangeTime,
    RunningObjectTableImpl_GetTimeOfLastChange,
    RunningObjectTableImpl_EnumRunning
};

/***********************************************************************
 *        RunningObjectTable_Initialize
 */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void)
{
    TRACE("\n");

    /* create the unique instance of the RunningObjectTableImpl structure */
    runningObjectTableInstance = HeapAlloc(GetProcessHeap(), 0, sizeof(RunningObjectTableImpl));

    if (!runningObjectTableInstance)
        return E_OUTOFMEMORY;

    /* initialize the virtual table function */
    runningObjectTableInstance->lpVtbl = &VT_RunningObjectTableImpl;

    /* the initial reference is set to "1" ! because if set to "0" it will be not practis when */
    /* the ROT referred many times not in the same time (all the objects in the ROT will  */
    /* be removed every time the ROT is removed ) */
    runningObjectTableInstance->ref = 1;

    list_init(&runningObjectTableInstance->rot);
    InitializeCriticalSection(&runningObjectTableInstance->lock);

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_UnInitialize
 */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize()
{
    TRACE("\n");

    if (runningObjectTableInstance==NULL)
        return E_POINTER;

    RunningObjectTableImpl_Release((IRunningObjectTable*)runningObjectTableInstance);

    RunningObjectTableImpl_Destroy();

    return S_OK;
}

/***********************************************************************
 *        EnumMoniker_QueryInterface
 */
static HRESULT WINAPI EnumMonikerImpl_QueryInterface(IEnumMoniker* iface,REFIID riid,void** ppvObject)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,riid,ppvObject);

    /* validate arguments */
    if (ppvObject == NULL)
        return E_INVALIDARG;

    *ppvObject = NULL;

    if (IsEqualIID(&IID_IUnknown, riid))
        *ppvObject = (IEnumMoniker*)This;
    else
        if (IsEqualIID(&IID_IEnumMoniker, riid))
            *ppvObject = (IEnumMoniker*)This;

    if ((*ppvObject)==NULL)
        return E_NOINTERFACE;

    IEnumMoniker_AddRef(iface);

    return S_OK;
}

/***********************************************************************
 *        EnumMoniker_AddRef
 */
static ULONG   WINAPI EnumMonikerImpl_AddRef(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/***********************************************************************
 *        EnumMoniker_release
 */
static ULONG   WINAPI EnumMonikerImpl_Release(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* unitialize rot structure if there's no more reference to it*/
    if (ref == 0)
    {
        ULONG i;

        TRACE("(%p) Deleting\n",This);

        for (i = 0; i < This->moniker_count; i++)
            HeapFree(GetProcessHeap(), 0, This->monikers[i]);
        HeapFree(GetProcessHeap(), 0, This->monikers);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}
/***********************************************************************
 *        EnmumMoniker_Next
 */
static HRESULT   WINAPI EnumMonikerImpl_Next(IEnumMoniker* iface, ULONG celt, IMoniker** rgelt, ULONG * pceltFetched)
{
    ULONG i;
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;
    HRESULT hr = S_OK;

    TRACE("(%p) TabCurrentPos %ld Tablastindx %ld\n", This, This->pos, This->moniker_count);

    /* retrieve the requested number of moniker from the current position */
    for(i = 0; (This->pos < This->moniker_count) && (i < celt); i++)
    {
        IStream *stream;
        hr = create_stream_on_mip_ro(This->monikers[This->pos++], &stream);
        if (hr != S_OK) break;
        hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&rgelt[i]);
        IStream_Release(stream);
        if (hr != S_OK) break;
    }

    if (pceltFetched != NULL)
        *pceltFetched= i;

    if (hr != S_OK)
        return hr;

    if (i == celt)
        return S_OK;
    else
        return S_FALSE;

}

/***********************************************************************
 *        EnmumMoniker_Skip
 */
static HRESULT   WINAPI EnumMonikerImpl_Skip(IEnumMoniker* iface, ULONG celt)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    TRACE("(%p)\n",This);

    if  (This->pos + celt >= This->moniker_count)
        return S_FALSE;

    This->pos += celt;

    return S_OK;
}

/***********************************************************************
 *        EnmumMoniker_Reset
 */
static HRESULT   WINAPI EnumMonikerImpl_Reset(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;

    This->pos = 0;	/* set back to start of list */

    TRACE("(%p)\n",This);

    return S_OK;
}

/***********************************************************************
 *        EnmumMoniker_Clone
 */
static HRESULT   WINAPI EnumMonikerImpl_Clone(IEnumMoniker* iface, IEnumMoniker ** ppenum)
{
    EnumMonikerImpl *This = (EnumMonikerImpl *)iface;
    MInterfacePointer **monikers = HeapAlloc(GetProcessHeap(), 0, sizeof(*monikers)*This->moniker_count);
    ULONG i;

    TRACE("(%p)\n",This);

    for (i = 0; i < This->moniker_count; i++)
    {
        SIZE_T size = FIELD_OFFSET(MInterfacePointer, abData[This->monikers[i]->ulCntData]);
        monikers[i] = HeapAlloc(GetProcessHeap(), 0, size);
        memcpy(monikers[i], This->monikers[i], size);
    }

    /* copy the enum structure */ 
    return EnumMonikerImpl_CreateEnumROTMoniker(monikers, This->moniker_count,
        This->pos, ppenum);
}

/* Virtual function table for the IEnumMoniker class. */
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

/***********************************************************************
 *        EnumMonikerImpl_CreateEnumROTMoniker
 *        Used by EnumRunning to create the structure and EnumClone
 *	  to copy the structure
 */
static HRESULT WINAPI EnumMonikerImpl_CreateEnumROTMoniker(MInterfacePointer **monikers,
                                                 ULONG moniker_count,
                                                 ULONG current_pos,
                                                 IEnumMoniker **ppenumMoniker)
{
    EnumMonikerImpl* This = NULL;

    if (!ppenumMoniker)
        return E_INVALIDARG;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(EnumMonikerImpl));
    if (!This) return E_OUTOFMEMORY;

    TRACE("(%p)\n", This);

    /* initialize the virtual table function */
    This->lpVtbl = &VT_EnumMonikerImpl;

    /* the initial reference is set to "1" */
    This->ref = 1;			/* set the ref count to one         */
    This->pos = 0;		/* Set the list start posn to start */
    This->moniker_count = moniker_count; /* Need the same size table as ROT */
    This->monikers = monikers;

    *ppenumMoniker =  (IEnumMoniker*)This;

    return S_OK;
}


/* Shared implementation of moniker marshaler based on saving and loading of
 * monikers */

#define ICOM_THIS_From_IMoniker(class, name) class* This = (class*)(((char*)name)-FIELD_OFFSET(class, lpVtblMarshal))

typedef struct MonikerMarshal
{
    const IUnknownVtbl *lpVtbl;
    const IMarshalVtbl *lpVtblMarshal;
    
    ULONG ref;
    IMoniker *moniker;
} MonikerMarshal;

static HRESULT WINAPI MonikerMarshalInner_QueryInterface(IUnknown *iface, REFIID riid, LPVOID *ppv)
{
    MonikerMarshal *This = (MonikerMarshal *)iface;
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);
    *ppv = NULL;
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IMarshal, riid))
    {
        *ppv = &This->lpVtblMarshal;
        IUnknown_AddRef((IUnknown *)&This->lpVtblMarshal);
        return S_OK;
    }
    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MonikerMarshalInner_AddRef(IUnknown *iface)
{
    MonikerMarshal *This = (MonikerMarshal *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MonikerMarshalInner_Release(IUnknown *iface)
{
    MonikerMarshal *This = (MonikerMarshal *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref) HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static const IUnknownVtbl VT_MonikerMarshalInner =
{
    MonikerMarshalInner_QueryInterface,
    MonikerMarshalInner_AddRef,
    MonikerMarshalInner_Release
};

static HRESULT WINAPI MonikerMarshal_QueryInterface(IMarshal *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_From_IMoniker(MonikerMarshal, iface);
    return IMoniker_QueryInterface(This->moniker, riid, ppv);
}

static ULONG WINAPI MonikerMarshal_AddRef(IMarshal *iface)
{
    ICOM_THIS_From_IMoniker(MonikerMarshal, iface);
    return IMoniker_AddRef(This->moniker);
}

static ULONG WINAPI MonikerMarshal_Release(IMarshal *iface)
{
    ICOM_THIS_From_IMoniker(MonikerMarshal, iface);
    return IMoniker_Release(This->moniker);
}

static HRESULT WINAPI MonikerMarshal_GetUnmarshalClass(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, CLSID* pCid)
{
    ICOM_THIS_From_IMoniker(MonikerMarshal, iface);

    TRACE("(%s, %p, %lx, %p, %lx, %p)\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pCid);

    return IMoniker_GetClassID(This->moniker, pCid);
}

static HRESULT WINAPI MonikerMarshal_GetMarshalSizeMax(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, DWORD* pSize)
{
    ICOM_THIS_From_IMoniker(MonikerMarshal, iface);
    HRESULT hr;
    ULARGE_INTEGER size;

    TRACE("(%s, %p, %lx, %p, %lx, %p)\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pSize);

    hr = IMoniker_GetSizeMax(This->moniker, &size);
    if (hr == S_OK)
        *pSize = (DWORD)size.QuadPart;
    return hr;
}

static HRESULT WINAPI MonikerMarshal_MarshalInterface(LPMARSHAL iface, IStream *pStm, 
    REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags)
{
    ICOM_THIS_From_IMoniker(MonikerMarshal, iface);

    TRACE("(%p, %s, %p, %lx, %p, %lx)\n", pStm, debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags);

    return IMoniker_Save(This->moniker, pStm, FALSE);
}

static HRESULT WINAPI MonikerMarshal_UnmarshalInterface(LPMARSHAL iface, IStream *pStm, REFIID riid, void **ppv)
{
    ICOM_THIS_From_IMoniker(MonikerMarshal, iface);
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", pStm, debugstr_guid(riid), ppv);

    hr = IMoniker_Load(This->moniker, pStm);
    if (hr == S_OK)
        hr = IMoniker_QueryInterface(This->moniker, riid, ppv);
    return hr;
}

static HRESULT WINAPI MonikerMarshal_ReleaseMarshalData(LPMARSHAL iface, IStream *pStm)
{
    TRACE("()\n");
    /* can't release a state-based marshal as nothing on server side to
     * release */
    return S_OK;
}

static HRESULT WINAPI MonikerMarshal_DisconnectObject(LPMARSHAL iface, DWORD dwReserved)
{
    TRACE("()\n");
    /* can't disconnect a state-based marshal as nothing on server side to
     * disconnect from */
    return S_OK;
}

static const IMarshalVtbl VT_MonikerMarshal =
{
    MonikerMarshal_QueryInterface,
    MonikerMarshal_AddRef,
    MonikerMarshal_Release,
    MonikerMarshal_GetUnmarshalClass,
    MonikerMarshal_GetMarshalSizeMax,
    MonikerMarshal_MarshalInterface,
    MonikerMarshal_UnmarshalInterface,
    MonikerMarshal_ReleaseMarshalData,
    MonikerMarshal_DisconnectObject
};

HRESULT MonikerMarshal_Create(IMoniker *inner, IUnknown **outer)
{
    MonikerMarshal *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &VT_MonikerMarshalInner;
    This->lpVtblMarshal = &VT_MonikerMarshal;
    This->ref = 1;
    This->moniker = inner;

    *outer = (IUnknown *)&This->lpVtbl;
    return S_OK;
}
