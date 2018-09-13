/*----------------------------------------------------------------------------
/ Title;
/   dataobj.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   IDataObject implementation as used by DS folder to communicate with the
/   outside world.
/----------------------------------------------------------------------------*/
#include "pch.h"
#include "stddef.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Helper functions
/----------------------------------------------------------------------------*/

CLIPFORMAT g_clipboardFormats[DSCF_MAX] = { 0 };

HRESULT CopyStorageMedium(FORMATETC* pFmt, STGMEDIUM* pMediumDst, STGMEDIUM* pMediumSrc);

HRESULT GetDsObjectNames(FORMATETC* pFmt, STGMEDIUM* pMedium, 
                            LPCITEMIDLIST pidlDsParent, HDPA hdpaIDL, 
                                DWORD dwProviderAND, DWORD dwProviderXOR);

HRESULT GetShellIDListArray(FORMATETC* pFmt, STGMEDIUM* pMedium, LPCITEMIDLIST pidlParent, HDPA hdpaIDL);
HRESULT GetShellFileDescriptors(FORMATETC* pFmt, STGMEDIUM* pMedium, LPCITEMIDLIST pidlParent, INT cbSkip, HDPA hdpaIDL);
HRESULT GetShellIDLOffsets(FORMATETC* pFmt, STGMEDIUM* pMedium, HDPA hdpaIDL, HWND hwndView);

typedef struct
{
    UINT cfFormat;
    STGMEDIUM medium;
} OTHERFMT, * LPOTHERFMT;


//
// our IDataObject implementation
//

class CDsDataObject : public IDataObject, CUnknown
{
    private:        
        HWND          _hwndView;
        LPITEMIDLIST  _pidlRoot;
        INT           _cbOffset;
        HDPA          _hidl;
        DWORD         _dwProviderAND;
        DWORD         _dwProviderXOR;

        HDSA          _hdsaOtherFmt;

    public:
        CDsDataObject(LPDSDATAOBJINIT pddoi);
        ~CDsDataObject();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IDataObject
	    STDMETHODIMP GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
	    STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
	    STDMETHODIMP QueryGetData(FORMATETC *pformatetc);
	    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
	    STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
	    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
	    STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
	    STDMETHODIMP DUnadvise(DWORD dwConnection);
	    STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise);
};

//
// format enumerator
//

class CDsEnumFormatETC : public IEnumFORMATETC, CUnknown
{
    private:
        INT _index;

    public:
        CDsEnumFormatETC();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IEnumIDList
        STDMETHODIMP Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched);
        STDMETHODIMP Skip(ULONG celt);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone(LPENUMFORMATETC* ppenum);
};


/*-----------------------------------------------------------------------------
/ RegisterDsClipboardFormats
/ --------------------------
/   Register the clipboard formats being used by this data object / view
/   implementation.
/
/ In:
/ Out:
/   BOOL -> it worked or not
/----------------------------------------------------------------------------*/
void RegisterDsClipboardFormats(void)
{
    TraceEnter(TRACE_DATAOBJ, "RegisterDsClipboardFormats");

    if ( !g_clipboardFormats[0] )
    {
        g_cfShellIDList = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
        g_cfShellDescriptors = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
        g_cfDsObjectNames = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
        g_cfShellIDLOffsets = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLISTOFFSET);
        g_cfDsDispSpecOptions = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSDISPLAYSPECOPTIONS);
    }
    
    Trace(TEXT("g_cfShellIDList %08x"), g_cfShellIDList);
    Trace(TEXT("g_cfShellDescriptors %08x"), g_cfShellDescriptors);
    Trace(TEXT("g_cfDsObjectNames %08x"), g_cfDsObjectNames);
    Trace(TEXT("g_cfShellIDLOffsets %08x"), g_cfShellIDLOffsets);
    Trace(TEXT("g_cfDsDispSpecOptions %08x"), g_cfDsDispSpecOptions);

    TraceLeave();
}


/*-----------------------------------------------------------------------------
/ CDsDataObject
/----------------------------------------------------------------------------*/

CDsDataObject::CDsDataObject(LPDSDATAOBJINIT pddoi) :
    _hwndView(pddoi->hwnd),
    _cbOffset(pddoi->cbOffset),
    _dwProviderAND(pddoi->dwProviderAND),
    _dwProviderXOR(pddoi->dwProviderXOR)
{
    INT i;
    LPITEMIDLIST pidl;
    USES_CONVERSION;

    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::CDsDataObject");

    _pidlRoot = ILClone(pddoi->pidlRoot);
    TraceAssert(_pidlRoot);

    _hidl = DPA_Create(16);
    TraceAssert(_hidl);

    Trace(TEXT("Adding %d ITEMIDLISTS to IDataObject"), pddoi->cidl);
    
    for ( i = 0 ; _hidl && (i < pddoi->cidl) ; i++ )
    {
        LPITEMIDLIST pidl = ILClone(pddoi->aidl[i]);
        if ( pidl && (-1 == DPA_AppendPtr(_hidl, pidl)) )
        {
            ILFree(pidl);
        }
    }

    RegisterDsClipboardFormats();            // ensure our private formats are registered

    TraceLeave();
}

//
// Destructor and its callback, we discard that DPA by using the DPA_DestroyCallBack
// which gives us a chance to free the contents of each pointer as we go.
// 

INT _DataObjectILDestoryCB(LPVOID pVoid, LPVOID pData)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)pVoid;

    TraceEnter(TRACE_DATAOBJ, "_DataObjectILDestroyCB");
    DoILFree(pidl);
    TraceLeaveValue(TRUE);
}

INT _DataObjectFmtDestroyCB(LPVOID pVoid, LPVOID pData)
{
    LPOTHERFMT pOtherFmt = (LPOTHERFMT)pVoid;

    TraceEnter(TRACE_DATAOBJ, "_DataObjectFmtDestroyCB");
    ReleaseStgMedium(&pOtherFmt->medium);
    TraceLeaveValue(TRUE);
}

CDsDataObject::~CDsDataObject()
{
    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::~CDsDataObject");

    DoILFree(_pidlRoot);
    
    DPA_DestroyCallback(_hidl, _DataObjectILDestoryCB, NULL);

    if ( _hdsaOtherFmt )
        DSA_DestroyCallback(_hdsaOtherFmt, _DataObjectFmtDestroyCB, NULL);

    TraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CDsDataObject
#include "unknown.inc"

STDMETHODIMP CDsDataObject::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IDataObject, (LPDATAOBJECT)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


//
// instance creation goop
//

STDAPI CDsDataObject_CreateInstance(LPDSDATAOBJINIT pddoi, REFIID riid, void **ppv)
{
    CDsDataObject* pdo = new CDsDataObject(pddoi);
    if ( !pdo )
        return E_OUTOFMEMORY;

    HRESULT hres = pdo->QueryInterface(riid, ppv);
    pdo->Release();

    return hres;
}


/*-----------------------------------------------------------------------------
/ IDataObject methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsDataObject::GetData(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    HRESULT hr;
    INT i;

    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::GetData");

    if ( !pFmt || !pMedium )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments to GetData");

    // which format?

    if ( pFmt->cfFormat == g_cfDsObjectNames )
    {
        hr = GetDsObjectNames(pFmt, pMedium, _ILSkip(_pidlRoot, _cbOffset), _hidl, _dwProviderAND, _dwProviderXOR);
        FailGracefully(hr, "Failed when build CF_DSOBJECTNAMES");
    }
    else if ( pFmt->cfFormat == g_cfShellIDList )
    {
        hr = GetShellIDListArray(pFmt, pMedium, _pidlRoot, _hidl);
        FailGracefully(hr, "Failed when build CF_SHELLIDLISTARRAY");
    }
    else if ( pFmt->cfFormat == g_cfShellDescriptors )
    {
        hr = GetShellFileDescriptors(pFmt, pMedium, _pidlRoot, _cbOffset, _hidl);
        FailGracefully(hr, "Failed when build CF_SHELLFILEDESCRIPTORS");
    }
    else 
    {
        // haven't found a match for the data yet in the static list,
        // so lets check to see if we can find it in the DSA, if we can then
        // we can just clone it.

        for ( i = 0 ; _hdsaOtherFmt && (i < DSA_GetItemCount(_hdsaOtherFmt)); i++ )
        {
            LPOTHERFMT pOtherFmt = (LPOTHERFMT)DSA_GetItemPtr(_hdsaOtherFmt, i);
            TraceAssert(pOtherFmt);

            if ( pOtherFmt->cfFormat == pFmt->cfFormat )
            {
                hr = CopyStorageMedium(pFmt, pMedium, &pOtherFmt->medium);
                FailGracefully(hr, "Failed to copy the storage medium");

                goto exit_gracefully;
            }
        }

        if ( pFmt->cfFormat != g_cfShellIDLOffsets )
            ExitGracefully(hr, DV_E_FORMATETC, "Bad format passed to GetData");

        hr = GetShellIDLOffsets(pFmt, pMedium, _hidl, _hwndView);
        FailGracefully(hr, "Failed to get the IDLIST position array");
    }

    hr = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsDataObject::GetDataHere(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::GetDataHere");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsDataObject::QueryGetData(FORMATETC* pFmt)
{
    HRESULT hr;
    INT i;
    BOOL fSupported = FALSE;

    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::QueryGetData");

    // check the valid clipboard formats either the static list, or the
    // DSA which contains the ones we have been set with.

    for ( i = 0 ; !fSupported && (i < ARRAYSIZE(g_clipboardFormats)) ; i++ )
    {
        if ( g_clipboardFormats[i] == pFmt->cfFormat )
        {  
            TraceMsg("Format supported (static list)");    
            fSupported = TRUE;
        }
    }

    for ( i = 0 ; !fSupported && _hdsaOtherFmt && (i < DSA_GetItemCount(_hdsaOtherFmt)) ; i++ )
    {
        LPOTHERFMT pOtherFmt = (LPOTHERFMT)DSA_GetItemPtr(_hdsaOtherFmt, i);
        TraceAssert(pOtherFmt);

        if ( pOtherFmt->cfFormat == pFmt->cfFormat )
        {
            TraceMsg("Format is supported (set via ::SetData");
            fSupported = TRUE;
        }
    }

    if ( !fSupported )
        ExitGracefully(hr, DV_E_FORMATETC, "Bad format passed to QueryGetData");

    // format looks good, lets check the other parameters

    if ( !( pFmt->tymed & TYMED_HGLOBAL ) )
        ExitGracefully(hr, E_INVALIDARG, "Non HGLOBAL StgMedium requested");

    if ( ( pFmt->ptd ) || !( pFmt->dwAspect & DVASPECT_CONTENT) || !( pFmt->lindex == -1 ) )
        ExitGracefully(hr, E_INVALIDARG, "Bad format requested");

    hr = S_OK;              // successs

exit_gracefully:

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsDataObject::GetCanonicalFormatEtc(FORMATETC* pFmtIn, FORMATETC *pFmtOut)
{
    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::GetCanonicalFormatEtc");
    
    // The easiest way to implement this is to tell the world that the 
    // formats would be identical, therefore leaving nothing to be done.

    TraceLeaveResult(DATA_S_SAMEFORMATETC);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsDataObject::SetData(FORMATETC* pFmt, STGMEDIUM* pMedium, BOOL fRelease)
{
    HRESULT hr;
    INT i;
    OTHERFMT otherfmt = { 0 };
    USES_CONVERSION;

    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::SetData");

    // All the user to store data with our DataObject, however we are
    // only interested in allowing them to this with particular clipboard formats

    if ( fRelease && !( pFmt->tymed & TYMED_HGLOBAL ) )
        ExitGracefully(hr, E_INVALIDARG, "fRelease == TRUE, but not a HGLOBAL allocation");

    if ( !_hdsaOtherFmt )
    {
        _hdsaOtherFmt = DSA_Create(SIZEOF(OTHERFMT), 4);
        TraceAssert(_hdsaOtherFmt);

        if ( !_hdsaOtherFmt )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate the DSA for items");
    }

    // if there is another copy of this data in the IDataObject then lets discard it.

    for ( i = 0 ; i < DSA_GetItemCount(_hdsaOtherFmt) ; i++ )
    {
        LPOTHERFMT pOtherFmt = (LPOTHERFMT)DSA_GetItemPtr(_hdsaOtherFmt, i);
        TraceAssert(pOtherFmt);

        if ( pOtherFmt->cfFormat == pFmt->cfFormat )
        {
            Trace(TEXT("Discarding previous entry for this format at index %d"), i); 
            ReleaseStgMedium(&pOtherFmt->medium);
            DSA_DeleteItem(_hdsaOtherFmt, i);
            break;            
        }
    }

    // now put a copy of the data passed to ::SetData into the DSA.
   
    otherfmt.cfFormat = pFmt->cfFormat;

    hr = CopyStorageMedium(pFmt, &otherfmt.medium, pMedium);
    FailGracefully(hr, "Failed to copy the STORAGEMEIDUM");
        
    if ( -1 == DSA_AppendItem(_hdsaOtherFmt, &otherfmt) )
    {
        ReleaseStgMedium(&otherfmt.medium);
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to add the data to the DSA");
    }

    hr = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc)
{
    HRESULT hr;
    CDsEnumFormatETC* pEnumFormatEtc = NULL;

    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::EnumFormatEtc");

    // Check the direction parameter, if this is READ then we support it,
    // otherwise we don't.

    if ( dwDirection != DATADIR_GET )
        ExitGracefully(hr, E_NOTIMPL, "We only support DATADIR_GET");

    *ppEnumFormatEtc = (IEnumFORMATETC*)new CDsEnumFormatETC();

    if ( !*ppEnumFormatEtc )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to enumerate the formats");

    hr = S_OK;

exit_gracefully:    

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

// None of the DAdvise methods are implemented currently

STDMETHODIMP CDsDataObject::DAdvise(FORMATETC* pFormatEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::DAdvise");
    TraceLeaveResult(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP CDsDataObject::DUnadvise(DWORD dwConnection)
{
    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::DUnadvise");
    TraceLeaveResult(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP CDsDataObject::EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
{
    TraceEnter(TRACE_DATAOBJ, "CDsDataObject::EnumDAdvise");
    TraceLeaveResult(OLE_E_ADVISENOTSUPPORTED);
}


/*-----------------------------------------------------------------------------
/ CDsEnumFormatETC
/----------------------------------------------------------------------------*/

CDsEnumFormatETC::CDsEnumFormatETC()
{
    TraceEnter(TRACE_DATAOBJ, "CDsEnumFormatETC::CDsEnumFormatETC");

    _index = 0;

    TraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CDsEnumFormatETC
#include "unknown.inc"

STDMETHODIMP CDsEnumFormatETC::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IEnumFORMATETC, (LPENUMFORMATETC)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IEnumFORMATETC methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsEnumFormatETC::Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched)
{
    HRESULT hr;
    ULONG fetched = 0;
    ULONG celtOld = celt;

    TraceEnter(TRACE_DATAOBJ, "CDsEnumFormatETC::Next");

    if ( !celt || !rgelt )
        ExitGracefully(hr, E_INVALIDARG, "Bad count/return pointer passed");

    // Look through all the formats that we have started at our stored
    // index, if either the output buffer runs out, or we have no
    // more formats to enumerate then bail

    for ( fetched = 0 ; celt && (_index < DSCF_MAX) ; celt--, _index++, fetched++ )
    {
        rgelt[fetched].cfFormat = g_clipboardFormats[_index];
        rgelt[fetched].ptd = NULL;
        rgelt[fetched].dwAspect = DVASPECT_CONTENT;
        rgelt[fetched].lindex = -1;
        rgelt[fetched].tymed = TYMED_HGLOBAL;
    }

    hr = ( fetched != celtOld ) ? S_FALSE:S_OK;

exit_gracefully:

    if ( pceltFetched )
        *pceltFetched = fetched;

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsEnumFormatETC::Skip(ULONG celt)
{
    TraceEnter(TRACE_DATAOBJ, "CDsEnumFormatETC::Skip");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsEnumFormatETC::Reset()
{
    TraceEnter(TRACE_DATAOBJ, "CDsEnumFormatETC::Reset");
    
    _index = 0;                // simple as that really

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsEnumFormatETC::Clone(LPENUMFORMATETC* ppenum)
{
    TraceEnter(TRACE_DATAOBJ, "CDsEnumFormatETC::Clone");
    TraceLeaveResult(E_NOTIMPL);
}


/*-----------------------------------------------------------------------------
/ Data collection functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ GetDsObjectNames
/ ----------------
/   Build a CF_DSOBJECTNAMES structure from the DPA's containing the object name
/   and class (and some help from DsFolder).
/
/ In:
/   pFmt, pMedium -> describe the allocation
/   clsidNamesapce = namespace the IDLISTs represent
/   hdpaIDL = DPA contianing the objects
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetDsObjectNames(FORMATETC* pFmt, STGMEDIUM* pMedium, 
                            LPCITEMIDLIST pidlDsParent, HDPA hdpaIDL, 
                                DWORD dwProviderAND, DWORD dwProviderXOR)
{
    HRESULT hr;
    DWORD cbStruct, offset;
    LPDSOBJECTNAMES pDsObjectNames;
    IDLISTDATA data;
    INT i, count;
    LPITEMIDLIST pidl;
    LPWSTR pPath = NULL;

    TraceEnter(TRACE_DATAOBJ, "GetDsObjectNames");

    // Compute the structure size (to allocate the medium)

    count = DPA_GetPtrCount(hdpaIDL);    
    Trace(TEXT("Item count is %d"), count);

    cbStruct = SIZEOF(DSOBJECTNAMES);
    offset = SIZEOF(DSOBJECTNAMES);

    for ( i = 0; i < count; i++ )
    {
        LPCITEMIDLIST pidlItem = (LPCITEMIDLIST)DPA_FastGetPtr(hdpaIDL, i);
        TraceAssert(pidlItem);

        cbStruct += SIZEOF(DSOBJECT);
        offset += SIZEOF(DSOBJECT);

        // Unpack the leaf element of the IDLIST, and construct an absolute one
        // that we can then convert to an ADS path.  Having done this we can
        // then compute the amount of storage required.

        hr = UnpackIdList(ILFindLastID(pidlItem), DSIDL_HASCLASS, &data);
        FailGracefully(hr, "Failed to unpack the IDLIST");

        pidl = ILCombine(pidlDsParent, pidlItem);

        if ( !pidl )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to combine pidlDsParent + pidl of object");

        hr = PathFromIdList(pidl, &pPath, NULL);             // get the ADsPath

        DoILFree(pidl);
        FailGracefully(hr, "Failed to convert IDLIST to ADsPath");

        cbStruct += StringByteSizeW(pPath);
        cbStruct += StringByteSizeW(data.pObjectClass);

        LocalFreeStringW(&pPath);
    }

    // cbStruct = size of the storage medium we need, so allocate it and fill it!

    hr = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pDsObjectNames);
    FailGracefully(hr, "Failed to allocate storage medium");

    pDsObjectNames->clsidNamespace = CLSID_MicrosoftDS;
    pDsObjectNames->cItems = count;                     
    
    for ( i = 0; i < count; i++ )
    {
        LPCITEMIDLIST pidlItem = (LPCITEMIDLIST)DPA_FastGetPtr(hdpaIDL, i);
        TraceAssert(pidlItem);

        // Unpack the leaf IDLIST that we have and then construct an absolute DS IDLIST
        // from the components we have.  Have done that we can then build the 
        // clipboard structure (of that path, class and flags).

        hr = UnpackIdList(ILFindLastID(pidlItem), DSIDL_HASCLASS, &data);
        FailGracefully(hr, "Failed to unpack the IDLIST");

        pidl = ILCombine(pidlDsParent, pidlItem);

        if ( !pidl )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to combine pidlDsParent + pidl of object");

        hr = PathFromIdList(pidl, &pPath, NULL);                     // get the ADsPath

        DoILFree(pidl);
        FailGracefully(hr, "Failed to convert IDLIST to ADsPath");

        // Now add the string elements to the structure we are making, updating the
        // offsets to represent the strings added.

        if ( data.dwFlags & DSIDL_ISCONTAINER )
            pDsObjectNames->aObjects[i].dwFlags |= DSOBJECT_ISCONTAINER;
        
        pDsObjectNames->aObjects[i].dwProviderFlags = (data.dwProviderFlags & dwProviderAND) ^ dwProviderXOR;

        pDsObjectNames->aObjects[i].offsetName = offset;
        StringByteCopyW(pDsObjectNames, offset, pPath);
        offset += StringByteSizeW(pPath);

        pDsObjectNames->aObjects[i].offsetClass = offset;
        StringByteCopyW(pDsObjectNames, offset, data.pObjectClass);
        offset += StringByteSizeW(data.pObjectClass);       

        LocalFreeStringW(&pPath);
    }
   
    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        ReleaseStgMedium(pMedium);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetShellIDListArray
/ -------------------
/   Return an IDLIST array packed as a clipboard format to the caller.
/
/ In:
/   pFmt, pMedium -> describe the allocation
/   clsidNamesapce = namespace the IDLISTs represent
/   hdpaIDL = DPA contianing the objects
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetShellIDListArray(FORMATETC* pFmt, STGMEDIUM* pMedium, LPCITEMIDLIST pidlParent, HDPA hdpaIDL)
{
    HRESULT hr;
    IDLISTDATA data;
    LPIDA pIDArray;
    DWORD cbStruct, offset;
    INT i, count;
    LPCITEMIDLIST pidl;

    TraceEnter(TRACE_DATAOBJ, "GetShellIDListArray");

    // Compute the structure size (to allocate the medium)

    count = DPA_GetPtrCount(hdpaIDL);    
    Trace(TEXT("Item count is %d"), count);

    cbStruct = SIZEOF(CIDA) + ILGetSize(pidlParent);
    offset = SIZEOF(CIDA);

    for ( i = 0 ; i < count ; i++ )
    {
        cbStruct += SIZEOF(UINT) + ILGetSize((LPCITEMIDLIST)DPA_FastGetPtr(hdpaIDL, i));
        offset += SIZEOF(UINT);
    }

    hr = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pIDArray);
    FailGracefully(hr, "Failed to allocate storage medium");

    // Fill the structure with an array of IDLISTs, we use a trick where by the
    // first offset (0) is the root IDLIST of the folder we are dealing with. 
    
    pIDArray->cidl = count;
    pidl = pidlParent;                      // start with the parent object in offset 0

    for ( i = 0 ; i <= count ; i++ )
    {
        Trace(TEXT("Adding IDLIST %08x, at offset %d, index %d"), pidl, offset, i);

        pIDArray->aoffset[i] = offset;
        CopyMemory(ByteOffset(pIDArray, offset), pidl, ILGetSize(pidl));
        offset += ILGetSize(pidl);

        pidl = (LPCITEMIDLIST)DPA_GetPtr(hdpaIDL, i);
    }

    hr = S_OK;          // success

exit_gracefully:

    if ( FAILED(hr) )
        ReleaseStgMedium(pMedium);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetShellFileDescriptors
/ -----------------------
/   Get the file descriptors for the given objects, this consists of a 
/   array of leaf names and flags associated with them.
/
/ In:
/   pFmt, pMedium -> describe the allocation
/   clsidNamesapce = namespace the IDLISTs represent
/   hdpaIDL = DPA contianing the objects
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetShellFileDescriptors(FORMATETC* pFmt, STGMEDIUM* pMedium, LPCITEMIDLIST pidlParent, INT cbSkip, HDPA hdpaIDL)
{
    HRESULT hr;
    DWORD cbStruct, offset;
    LPFILEGROUPDESCRIPTOR pDescriptors = NULL;
    IDLISTDATA data;
    LPCITEMIDLIST pidl;
    INT i, count;
    IADsPathname* pDsPathname = NULL;
    LPWSTR pName = NULL;
#ifndef UNICODE
    WCHAR szBuffer[MAX_PATH];
#endif
    USES_CONVERSION;

    TraceEnter(TRACE_DATAOBJ, "GetShellFileDescriptors");

    // Compute the structure size (to allocate the medium)

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&pDsPathname);
    FailGracefully(hr, "Failed to get the IADsPathname interface");

    count = DPA_GetPtrCount(hdpaIDL);    
    Trace(TEXT("Item count is %d"), count);

    cbStruct  = SIZEOF(FILEGROUPDESCRIPTOR) + (SIZEOF(FILEDESCRIPTOR) * count);

    hr = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pDescriptors);
    FailGracefully(hr, "Failed to allocate storage medium");

    pDescriptors->cItems = count;

    for ( i = 0; i < count; i++ )
    {
        LPCITEMIDLIST pidl = (LPCITEMIDLIST)DPA_FastGetPtr(hdpaIDL, i);
        TraceAssert(pidl);

        hr = UnpackIdList(pidl, DSIDL_HASCLASS, &data);
        FailGracefully(hr, "Failed to unpack the IDLIST");

        pDescriptors->fgd[i].dwFlags = FD_ATTRIBUTES;
        pDescriptors->fgd[i].dwFileAttributes = AttributesFromIdList(&data);

#if UNICODE
        hr = NameFromIdList(pidl, _ILSkip(pidlParent, cbSkip), 
                            pDescriptors->fgd[i].cFileName, ARRAYSIZE(pDescriptors->fgd[i].cFileName), 
                                pDsPathname);
#else
        hr = NameFromIdList(pidl, _ILSkip(pidlParent, cbSkip),  szBuffer, ARRAYSIZE(szBuffer), pDsPathname);
        StrCpyN(pDescriptors->fgd[i].cFileName, W2T(szBuffer), ARRAYSIZE(pDescriptors->fgd[i].cFileName));
#endif

        Trace(TEXT("Flags %08x, Attributes %08x, Name %s"), 
                                        pDescriptors->fgd[i].dwFlags, 
                                        pDescriptors->fgd[i].dwFileAttributes, 
                                        pDescriptors->fgd[i].cFileName);
    }

    hr = S_OK;          // success

exit_gracefully:

    if ( FAILED(hr) )
        ReleaseStgMedium(pMedium);

    DoRelease(pDsPathname);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetShellIDLOffsets
/ ------------------
/   Get an array of IDLISTs and their view positions to allow drag/drop
/   and positioning of the contents of the view.
/
/ In:
/   pFmt, pMedium -> describe the allocation
/   hdpaIDL = DPA contianing the objects
/   hwndView = view to query for the item positions
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetShellIDLOffsets(FORMATETC* pFmt, STGMEDIUM* pMedium, HDPA hdpaIDL, HWND hwndView)
{
    HRESULT hr;
    DWORD cbStruct;
    LPCITEMIDLIST pidl;
    LPPOINT pPoints;
    INT i, count;

    TraceEnter(TRACE_DATAOBJ, "GetShellIDLOffsets");

    // Compute the structure size (to allocate the medium)

    count = DPA_GetPtrCount(hdpaIDL);    
    Trace(TEXT("Item count is %d"), count);

    cbStruct  = SIZEOF(POINT)*count;
    
    hr = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pPoints);
    FailGracefully(hr, "Failed to allocate storage medium");

    // walk all the items returning the x/y offset as a point (from the view)

    for ( i = 0 ; i < count ;  i++ )
    {
        pidl = ILFindLastID((LPCITEMIDLIST)DPA_FastGetPtr(hdpaIDL, i));
        TraceAssert(pidl);

        SendMessage(hwndView, SVM_GETITEMPOSITION, (WPARAM)pidl, (LPARAM)&pPoints[i] );   
    }

 exit_gracefully:

    TraceLeaveResult(hr);
 }
