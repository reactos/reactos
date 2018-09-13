/*----------------------------------------------------------------------------
/ Title;
/   dataobj.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   IDataObject implementation as used by My Documents folder to communicate with the
/   outside world.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#include "stddef.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Helper functions
/----------------------------------------------------------------------------*/

UINT g_clipboardFormats[MDCF_MAX] = { 0 };

HRESULT
AllocStorageMedium( FORMATETC* pFmt,
                    STGMEDIUM* pMedium,
                    DWORD cbStruct,
                    LPVOID* ppAlloc
                   );

HRESULT
CopyStorageMedium( STGMEDIUM* pMediumDst,
                   STGMEDIUM* pMediumSrc,
                   FORMATETC* pFmt
                  );


HRESULT
GetShellIDListArray( FORMATETC* pFmt,
                     STGMEDIUM* pMedium,
                     LPCITEMIDLIST pidlParent,
                     HDPA hdpaSpecial,
                     HDPA hdpaShell
                    );

HRESULT
GetFileDescriptors( FORMATETC* pFmt,
                    STGMEDIUM* pMedium,
                    LPTSTR pRootPath,
                    HDPA hdpaSpecial,
                    HDPA hdpaShell
                   );


/*-----------------------------------------------------------------------------
/ RegisterMyDocsClipboardFormats
/ --------------------------
/   Register the clipboard formats being used by this data object / view
/   implementation.
/
/ In:
/ Out:
/   BOOL -> it worked or not
/----------------------------------------------------------------------------*/
void RegisterMyDocsClipboardFormats(void)
{
    MDTraceEnter(TRACE_DATAOBJ, "RegisterMyDocsClipboardFormats");

    if ( !g_clipboardFormats[0] )
    {
        g_cfShellIDList          = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
        g_cfOffsets              = RegisterClipboardFormat(CFSTR_SHELLIDLISTOFFSET);
        g_cfFileDescriptorsA     = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
        g_cfFileDescriptorsW     = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
    }

    MDTrace(TEXT("g_cfShellIDList %08x"),      g_cfShellIDList);
    MDTrace(TEXT("g_cfOffsets %08x"),          g_cfOffsets);
    MDTrace(TEXT("g_cfFileDescriptorsA %08x"), g_cfFileDescriptorsA);
    MDTrace(TEXT("g_cfFileDescriptorsW %08x"), g_cfFileDescriptorsW);

    MDTraceLeave();
}


/*-----------------------------------------------------------------------------
/ CreateShellAIDL
/ ---------------
/   Given a hpda, create an array of shell idlists.
/
/ In:
/   HDPA -> list of ids to create array from
/   UINT * -> count of items in aidl (returned)
/
/ Out:
/   LPCITEMIDLIST * aidl -> NULL if error, otherwise the array
/----------------------------------------------------------------------------*/
LPCITEMIDLIST * CreateShellAIDL(HDPA hdpa, UINT * pcidl)
{
    LPCITEMIDLIST * aidl = NULL;
    UINT count, i;

    if (!pcidl)
        return NULL;

    count = DPA_GetPtrCount( hdpa );
    if (!count)
        return NULL;

    aidl = (LPCITEMIDLIST *)LocalAlloc( LPTR, sizeof(LPCITEMIDLIST) * count );
    if (!aidl)
        return NULL;

    for (i = 0; i < count; i++)
    {
        aidl[i] = (LPCITEMIDLIST)DPA_FastGetPtr( hdpa, i );
    }

    *pcidl = count;

    return aidl;

}

/*-----------------------------------------------------------------------------
/ CMyDocsDataObject
/----------------------------------------------------------------------------*/
CMyDocsDataObject::CMyDocsDataObject( IShellFolder * psf,
                                      LPCITEMIDLIST pidlRoot,
                                      LPCITEMIDLIST pidlReal,
                                      INT cidl,
                                      LPCITEMIDLIST* aidl
                                     )
{
    INT i;
    LPITEMIDLIST pidl;
    TCHAR szPath[ MAX_PATH ];

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::CMyDocsDataObject");

    //
    // Initialize stuff...
    //

    m_hdpaSpecial    = 0;
    m_hdpaShell      = 0;
    m_pdo            = NULL;
    m_pOffsetsMedium = NULL;
    m_pidlRoot       = ILClone( pidlRoot );
    m_pidlShellRoot  = ILClone( pidlReal );

    if (SHGetPathFromIDList( pidlReal, szPath ))
    {
        LocalAllocString( &m_rootPath, szPath );
    }

    //
    // Walk through list of pidls and move them to the appropriate
    // places (either special hpda or shell hpda).
    //

    for ( i = 0 ; i < cidl ; i++ )
    {
        //
        // Make a copy of the idlist
        //

        pidl = ILClone(aidl[i]);
        MDTrace(TEXT("Cloned IDLIST %08x to %08x"), aidl[i], pidl);

        //
        // If it's a special item, add it to the special HDPA
        //

        if (MDIsSpecialIDL( pidl ))
        {
            //
            // If the hdpa doesn't exist, create it
            //

            if (!m_hdpaSpecial)
            {
                m_hdpaSpecial = DPA_Create(4);
            }

            //
            // Add the item if we can, delete the copy we made if
            // we run into any problems...
            //

            if (m_hdpaSpecial)
            {
                if ( -1 == DPA_AppendPtr(m_hdpaSpecial, pidl ))
                {
                    MDTraceMsg("Failed to insert into the Special DPA");
                    DoILFree(pidl);
                }
            }
            else
            {
                MDTraceMsg("m_hdpaSpecial was NULL, cleaning up pidl");
                DoILFree( pidl );
            }
        }

        //
        // If it's a normal shell item, add it to the shell HDPA
        //

        else
        {
            //
            // If the hdpa doesn't exist, create it
            //

            if (!m_hdpaShell)
            {
                m_hdpaShell = DPA_Create(4);
            }

            //
            // Add the item if we can, delete the copy we made if
            // we run into any problems...
            //

            if (m_hdpaShell)
            {
                if ( -1 == DPA_AppendPtr(m_hdpaShell, pidl) )
                {
                    MDTraceMsg("Failed to insert into the Shell DPA");
                    DoILFree(pidl);
                }
            }
            else
            {
                MDTraceMsg("m_hdpaShell was NULL, cleaning up pidl");
                DoILFree( pidl );
            }

        }


    }

    //
    // Ensure that we have the clipboard formats registered we care
    // about are registered...
    //

    RegisterMyDocsClipboardFormats();

    //
    // If there are any shell items in our list, get a reference to the
    // shell's IDataObject for them...
    //

    if (m_hdpaShell)
    {

        UINT count;
        LPCITEMIDLIST * aidlShell = CreateShellAIDL( m_hdpaShell, &count );
        if (aidlShell)
        {
            psf->GetUIObjectOf( NULL, count, aidlShell,
                                IID_IDataObject, NULL, (LPVOID *)&m_pdo
                               );
            LocalFree( aidlShell );
        }
    }

    MDTraceLeave();
}

//
// Destructor and its callback, we discard that DPA by using the DPA_DestroyCallBack
// which gives us a chance to free the contents of each pointer as we go.
//
INT _DataObjectDestoryCB(LPVOID pVoid, LPVOID pData)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)pVoid;

    MDTraceEnter(TRACE_DATAOBJ, "_DataObjectDestroyCB");
    DoILFree(pidl);
    MDTraceLeaveValue(TRUE);
}

CMyDocsDataObject::~CMyDocsDataObject()
{
    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::~CMyDocsDataObject");

    if (m_hdpaSpecial)
    {
        DPA_DestroyCallback(m_hdpaSpecial, _DataObjectDestoryCB, NULL);
    }

    if (m_hdpaShell)
    {
        DPA_DestroyCallback(m_hdpaShell, _DataObjectDestoryCB, NULL);
    }
    DoILFree( m_pidlRoot );
    DoILFree( m_pidlShellRoot );
    LocalFreeString( &m_rootPath );

    if (m_pOffsetsMedium)
    {
        ReleaseStgMedium( m_pOffsetsMedium );
    }

    DoRelease( m_pdo );

    MDTraceLeave();
}

//
// IUnknown
//

#undef CLASS_NAME
#define CLASS_NAME CMyDocsDataObject
#include "unknown.inc"

STDMETHODIMP CMyDocsDataObject::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IDataObject, (LPDATAOBJECT)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IDataObject methods
/----------------------------------------------------------------------------*/
STDMETHODIMP CMyDocsDataObject::GetData(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    HRESULT hr = E_FAIL;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::GetData");

    //
    // Do the simple case first -- only have shell items...
    //

    if (!m_hdpaSpecial && m_pdo)
    {
        MDTrace(TEXT("Calling shell directly"));
        hr = m_pdo->GetData( pFmt, pMedium );
        goto exit_gracefully;
    }

    //
    // Do some parameter validation
    //

    if ( !pFmt || !pMedium )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments to GetData");


    //
    // What we do next depends on the format...
    //

    if (pFmt->cfFormat == g_cfShellIDList)
    {
        hr = GetShellIDListArray(pFmt, pMedium, m_pidlShellRoot, m_hdpaSpecial, m_hdpaShell );
        FailGracefully(hr, "Failed when building CFSTR_SHELLIDLISTARRAY");
    }
    else if ( ( pFmt->cfFormat == g_cfFileDescriptorsA ) ||
              ( pFmt->cfFormat == g_cfFileDescriptorsW )
             )
    {
        hr = GetFileDescriptors( pFmt, pMedium, m_rootPath, m_hdpaSpecial, m_hdpaShell );
        FailGracefully(hr, "Failed when building CFSTR_FILEDESCRIPTOR");
    }
    else if (pFmt->cfFormat == g_cfOffsets)
    {
        if (m_pOffsetsMedium && (pFmt->tymed & TYMED_HGLOBAL))
        {
            hr = CopyStorageMedium( pMedium, m_pOffsetsMedium, pFmt );
        }
        else if (m_pdo)
        {
            hr = m_pdo->GetData( pFmt, pMedium );
        }
        else
        {
            ExitGracefully( hr, E_FAIL, "neither shell nor mydocs has g_cfOffsets" );
        }
    }
    else
    {
        //
        // It's not one of the formats we support, so see if the shell
        // supports them...
        //

        if (m_pdo)
        {
            MDTrace(TEXT("Calling shell directly..."));
            hr = m_pdo->GetData( pFmt, pMedium );
        }

#ifdef DEBUG
        if (FAILED(hr))
        {
            TCHAR szBuffer[MAX_PATH];
            LPTSTR pName = szBuffer;

            if ( !GetClipboardFormatName(pFmt->cfFormat, szBuffer, ARRAYSIZE(szBuffer)) )
            {
                pName = TEXT("<unknown>");
            }

            MDTrace(TEXT("Bad cfFormat given (%s) for special item (%08x)"), pName, pFmt->cfFormat);
        }
#endif

    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsDataObject::GetDataHere(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    HRESULT hr = E_NOTIMPL;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::GetDataHere");

    if (!m_hdpaSpecial && m_pdo)
    {
        hr = m_pdo->GetDataHere( pFmt, pMedium );
    }

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsDataObject::QueryGetData(FORMATETC* pFmt)
{
    HRESULT hr = E_FAIL;
#ifdef DEBUG
    TCHAR szBuffer[MAX_PATH];
    LPTSTR pName = szBuffer;

    if ( !GetClipboardFormatName(pFmt->cfFormat, szBuffer, ARRAYSIZE(szBuffer)) )
    {
        pName = TEXT("<unknown>");
    }
#endif


    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::QueryGetData");

#ifdef DEBUG
    MDTrace(TEXT("cfFormat requested: (%s) (%08x)"), pName, pFmt->cfFormat);
#endif

    //
    //  Check with the shell first...
    //
    if (m_pdo)
    {
        MDTrace(TEXT("Calling shell directly..."));
        hr = m_pdo->QueryGetData( pFmt );
    }

    if (FAILED(hr) || (hr == S_FALSE))
    {
        if (m_hdpaSpecial)
        {

            //
            // Check to see if this is a clipboard format we support for special items
            //

            if ( (pFmt->cfFormat == g_cfShellIDList) ||
                 (pFmt->cfFormat == g_cfFileDescriptorsA) ||
                 (pFmt->cfFormat == g_cfFileDescriptorsW)
                )
            {
                ExitGracefully(hr, S_FALSE, "Unsupported format passed to QueryGetData");
            }

            //
            // Format looks good, so now check that we can create a StgMedium for it
            //

            if ( !( pFmt->tymed & TYMED_HGLOBAL ) )
            {
                ExitGracefully(hr, E_INVALIDARG, "Non HGLOBAL StgMedium requested");
            }

            hr = S_OK;              // successs

        }
    }


exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsDataObject::GetCanonicalFormatEtc(FORMATETC* pFmtIn, FORMATETC *pFmtOut)
{
    HRESULT hr = DATA_S_SAMEFORMATETC;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::GetCanonicalFormatEtc");

    if (m_pdo)
    {
        hr = m_pdo->GetCanonicalFormatEtc( pFmtIn, pFmtOut );
    }

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsDataObject::SetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium, BOOL fRelease)
{
    HRESULT hr = E_NOTIMPL;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::SetData");
#ifdef DEBUG
    {
        TCHAR szBuffer[MAX_PATH];
        LPTSTR pName = szBuffer;

        if ( !GetClipboardFormatName(pFormatEtc->cfFormat, szBuffer, ARRAYSIZE(szBuffer)) )
        {
            pName = TEXT("<unknown>");
        }

        MDTrace(TEXT("Trying to set cfFormat (%s) (%08x)"), pName, pFormatEtc->cfFormat);
    }
#endif

    //
    // Try the shell first...
    //

    if (m_pdo)
    {
        hr = m_pdo->SetData( pFormatEtc, pMedium, fRelease );
    }

    //
    // If the shell didn't take it, we'll try...
    //

    if (FAILED(hr))
    {
        //
        // We only allow setting the ShellIDListsOffsets format
        //

        if (pFormatEtc->cfFormat == g_cfOffsets)
        {
            //
            // Do we have one lingering around?  If so, delete it...
            //

            if (m_pOffsetsMedium)
            {
                ReleaseStgMedium( m_pOffsetsMedium );
            }

            m_pOffsetsMedium = &m_OffsetsMedium;

            //
            // Do we have full ownership of the storage medium?
            //

            if (fRelease)
            {
                //
                // Yep, then just hold onto it...
                //

                *m_pOffsetsMedium = *pMedium;
                hr = S_OK;

            }
            else
            {
                //
                // Nope, we need to copy everything...
                //

                hr = CopyStorageMedium( m_pOffsetsMedium, pMedium, pFormatEtc );
            }
        }
    }

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc)
{
    HRESULT hr;
    CMyDocsEnumFormatETC* pEnumFormatEtc = NULL;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::EnumFormatEtc");

    // Check the direction parameter, if this is READ then we support it,
    // otherwise we don't.

    if ( dwDirection != DATADIR_GET )
        ExitGracefully(hr, E_NOTIMPL, "We only support DATADIR_GET");

    *ppEnumFormatEtc = (IEnumFORMATETC*)new CMyDocsEnumFormatETC(m_pdo);

    if ( !*ppEnumFormatEtc )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to enumerate the formats");

    (*ppEnumFormatEtc)->AddRef();         // object starts with refcount == 1

    hr = S_OK;

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

// None of the DAdvise methods are implemented currently

STDMETHODIMP CMyDocsDataObject::DAdvise(FORMATETC* pFormatEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
    HRESULT hr = OLE_E_ADVISENOTSUPPORTED;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::DAdvise");

    if (m_pdo)
    {
        hr = m_pdo->DAdvise( pFormatEtc, advf, pAdvSink, pdwConnection );
    }

    MDTraceLeaveResult(hr);
}

STDMETHODIMP CMyDocsDataObject::DUnadvise(DWORD dwConnection)
{
    HRESULT hr = OLE_E_ADVISENOTSUPPORTED;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::DUnadvise");

    if (m_pdo)
    {
        hr = m_pdo->DUnadvise( dwConnection );
    }

    MDTraceLeaveResult(hr);
}

STDMETHODIMP CMyDocsDataObject::EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
{
    HRESULT hr = OLE_E_ADVISENOTSUPPORTED;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsDataObject::EnumDAdvise");

    if (m_pdo)
    {
        hr = m_pdo->EnumDAdvise( ppenumAdvise );
    }

    MDTraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ CMyDocsEnumFormatETC
/----------------------------------------------------------------------------*/

CMyDocsEnumFormatETC::CMyDocsEnumFormatETC( IDataObject * pdo)
{
    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsEnumFormatETC::CMyDocsEnumFormatETC");

    m_index = 0;
    m_pefe  = NULL;

    if (pdo)
    {
        pdo->EnumFormatEtc( DATADIR_GET, &m_pefe );
    }

    MDTraceLeave();
}

CMyDocsEnumFormatETC::~CMyDocsEnumFormatETC()
{
    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsEnumFormatETC::~CMyDocsEnumFormatETC");

    DoRelease( m_pefe );

    MDTraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CMyDocsEnumFormatETC
#include "unknown.inc"

STDMETHODIMP CMyDocsEnumFormatETC::QueryInterface(REFIID riid, LPVOID* ppvObject)
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

STDMETHODIMP CMyDocsEnumFormatETC::Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched)
{
    HRESULT hr;
    ULONG fetched, celtSave = celt;
#ifdef DEBUG
    TCHAR szBuffer[MAX_PATH];
    LPTSTR pName = szBuffer;
#endif

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsEnumFormatETC::Next");

    if (m_pefe)
    {
        hr = m_pefe->Next(celt, rgelt, pceltFetched );
        MDTraceLeaveResult(hr);
    }

    if ( !celt || !rgelt )
        ExitGracefully(hr, E_INVALIDARG, "Bad count/return pointer passed");

    // Look through all the formats that we have started at our stored
    // index, if either the output buffer runs out, or we have no
    // more formats to enumerate then bail

    for ( fetched = 0 ; celt && (m_index < MDCF_MAX) ; celt--, m_index++, fetched++ )
    {
        rgelt[fetched].cfFormat = g_clipboardFormats[m_index];
        rgelt[fetched].ptd      = NULL;
        rgelt[fetched].dwAspect = DVASPECT_CONTENT;
        rgelt[fetched].lindex   = -1;
        rgelt[fetched].tymed    = TYMED_HGLOBAL;

#ifdef DEBUG
        if ( !GetClipboardFormatName(rgelt[fetched].cfFormat, szBuffer, ARRAYSIZE(szBuffer)) )
        {
            pName = TEXT("<unknown>");
        }

        MDTrace(TEXT("returning cfFormat (%s) (%08x), TYMED_ as 0x%x"), pName, rgelt[fetched].cfFormat, rgelt[fetched].tymed);
#endif

    }

    hr = ( fetched == celtSave) ? S_OK:S_FALSE;

exit_gracefully:

    if ( pceltFetched )
        *pceltFetched = fetched;

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsEnumFormatETC::Skip(ULONG celt)
{
    HRESULT hr = E_NOTIMPL;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsEnumFormatETC::Skip");

    if (m_pefe)
    {
        m_pefe->Skip( celt );
    }

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsEnumFormatETC::Reset()
{
    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsEnumFormatETC::Reset");

    if (m_pefe)
    {
        m_pefe->Reset();
    }

    m_index = 0;                // simple as that really

    MDTraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsEnumFormatETC::Clone(LPENUMFORMATETC* ppenum)
{
    HRESULT hr = E_NOTIMPL;

    MDTraceEnter(TRACE_DATAOBJ, "CMyDocsEnumFormatETC::Clone");

    if (m_pefe)
    {
        hr = m_pefe->Clone( ppenum );
    }

    MDTraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ Data collection functions
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ CopyStorageMedium
/ ------------------
/   Copies a storage medium (and the data in an HGLOBAL).  Only works
/   for TYMED_HGLOBAL mediums...
/
/ In:
/   pMediumDst -> where to copy to...
/   pFmt, pMediumSrc -> describe the source
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
CopyStorageMedium( STGMEDIUM* pMediumDst,
                   STGMEDIUM* pMediumSrc,
                   FORMATETC* pFmt
                  )
{
    HRESULT hr = E_FAIL;

    MDTraceEnter(TRACE_DATAOBJ, "CopyStorageMedium");

    if (pFmt->tymed & TYMED_HGLOBAL)
    {
        UINT cbStruct = GlobalSize( (HGLOBAL)pMediumSrc->hGlobal );
        HGLOBAL hGlobal;
        LPVOID lpSrc, lpDst;

        hr = AllocStorageMedium( pFmt, pMediumDst, cbStruct, (LPVOID *)&hGlobal );
        FailGracefully( hr, "Unable to allocated storage medium" );

        *pMediumDst = *pMediumSrc;
        pMediumDst->hGlobal = hGlobal;

        lpSrc = GlobalLock( pMediumSrc->hGlobal );
        lpDst = GlobalLock( pMediumDst->hGlobal );

        memcpy( lpDst, lpSrc, cbStruct );

        GlobalUnlock( pMediumSrc->hGlobal );
        GlobalUnlock( pMediumDst->hGlobal );

        hr = S_OK;
    }

exit_gracefully:

    MDTraceLeaveResult(hr);

}


/*-----------------------------------------------------------------------------
/ AllocStorageMedium
/ ------------------
/   Allocate a storage medium (validating the clipboard format as required).
/
/ In:
/   pFmt, pMedium -> describe the allocation
/   cbStruct = size of allocation
/   ppAlloc -> receives a pointer to the allocation / = NULL
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
AllocStorageMedium( FORMATETC* pFmt,
                    STGMEDIUM* pMedium,
                    DWORD cbStruct,
                    LPVOID* ppAlloc
                   )
{
    HRESULT hr;

    MDTraceEnter(TRACE_DATAOBJ, "AllocStorageMedium");

    MDTraceAssert(pFmt);
    MDTraceAssert(pMedium);

    // Validate parameters

    if ( ( cbStruct <= 0 ) || !( pFmt->tymed & TYMED_HGLOBAL ) )
        ExitGracefully(hr, E_INVALIDARG, "Zero size stored medium requested or non HGLOBAL");

    if ( (!( pFmt->dwAspect & DVASPECT_CONTENT)) )
        ExitGracefully(hr, DV_E_DVASPECT, "Bad aspect requested");

    // Allocate the medium via GlobalAlloc

    pMedium->tymed = TYMED_HGLOBAL;
    pMedium->hGlobal = GlobalAlloc(GPTR, cbStruct);
    pMedium->pUnkForRelease = NULL;

    if ( !pMedium->hGlobal )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate StgMedium");

    hr = S_OK;                  // success

exit_gracefully:

    if ( ppAlloc )
        *ppAlloc = SUCCEEDED(hr) ? (LPVOID)pMedium->hGlobal:NULL;

    MDTraceLeaveResult(hr);
}

void
AddIDListToIDA( LPIDA pIDArray,
                LPCITEMIDLIST pidl,
                DWORD * poffset,
                INT index
               )
{
    pIDArray->aoffset[index] = *poffset;
    memcpy(ByteOffset(pIDArray, *poffset), pidl, ILGetSize(pidl));
    *poffset += ILGetSize(pidl);
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
HRESULT
GetShellIDListArray( FORMATETC* pFmt,
                     STGMEDIUM* pMedium,
                     LPCITEMIDLIST pidlParent,
                     HDPA hdpaSpecial,
                     HDPA hdpaShell
                    )
{
    HRESULT hr;
    LPIDA pIDArray;
    DWORD cbStruct, offset;
    INT i, countSpecial = 0, countShell = 0, index;
    LPCITEMIDLIST pidl;
    IDREGITEM idlMyDocs;

    MDTraceEnter(TRACE_DATAOBJ, "GetShellIDListArray");

    //
    // How many IDLISTs do we have?
    //

    if (hdpaSpecial)
    {
        countSpecial = DPA_GetPtrCount(hdpaSpecial);
    }

    if (hdpaShell)
    {
        countShell = DPA_GetPtrCount(hdpaShell);
    }
    MDTrace(TEXT("Item count is %d special, %d shell"), countSpecial, countShell);

    //
    // Need to do some special stuff if it's a single special item
    //

    if (!countShell && (countSpecial == 1))
    {
        //
        // Make the parent the root folder idlist
        //

        idlMyDocs.cb = SIZEOF(IDREGITEM) - sizeof(WORD);
        idlMyDocs.bFlags = SHID_ROOT_REGITEM;
        idlMyDocs.bReserved = MYDOCS_SORT_INDEX;
        idlMyDocs.clsid = CLSID_MyDocumentsExt;
        idlMyDocs.next = 0;

        pidlParent = (LPCITEMIDLIST)&idlMyDocs;
    }


    //
    // compute size of structure needed...
    //

    cbStruct = SIZEOF(CIDA) + ILGetSize(pidlParent);
    offset = SIZEOF(CIDA);

    for ( i = 0 ; i < countSpecial ; i++ )
    {
        cbStruct += SIZEOF(UINT) + ILGetSize((LPCITEMIDLIST)DPA_FastGetPtr(hdpaSpecial, i));
        offset += SIZEOF(UINT);
    }

    for ( i = 0 ; i < countShell ; i++ )
    {
        cbStruct += SIZEOF(UINT) + ILGetSize((LPCITEMIDLIST)DPA_FastGetPtr(hdpaShell, i));
        offset += SIZEOF(UINT);
    }

    hr = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pIDArray);
    FailGracefully(hr, "Failed to allocate storage medium");

    //
    // Fill the structure with an array of IDLISTs, start with parent object in
    // offset 0
    //
#ifdef DEBUG
    {
        TCHAR szPath[ MAX_PATH ];

        if (MDGetPathFromIDL( (LPITEMIDLIST)pidlParent, szPath, NULL ))
        {
            MDTrace(TEXT("Parent Dir is: -%s-"), szPath);
        }
    }
#endif


    pIDArray->cidl = countSpecial + countShell;

//    MDTrace(TEXT("Adding parent  IDLIST %08x, at offset %d, index 0"), pidl, offset, index);
    AddIDListToIDA( pIDArray, pidlParent, &offset, 0 );

    //
    // Add special items
    //

    index = 1;
    for ( i = 0 ; i < countSpecial; i++ )
    {
        if (hdpaSpecial)
        {
            pidl = (LPITEMIDLIST)DPA_FastGetPtr( hdpaSpecial, i );
            MDTrace(TEXT("Adding special IDLIST %08x, at offset %d, index %d"), pidl, offset, index);
            AddIDListToIDA( pIDArray, pidl, &offset, index );
            index++;
        }
    }

    //
    // Add shell items
    //

    for ( i = 0 ; i < countShell; i++ )
    {

        if (hdpaShell)
        {
            pidl = (LPITEMIDLIST)DPA_FastGetPtr( hdpaShell, i );
            MDTrace(TEXT("Adding shell   IDLIST %08x, at offset %d, index %d"), pidl, offset, index);
            AddIDListToIDA( pIDArray, pidl, &offset, index );
            index++;
        }

    }


    hr = S_OK;          // success

exit_gracefully:

    if ( FAILED(hr) )
        ReleaseStgMedium(pMedium);

    MDTraceLeaveResult(hr);
}


void FillInFileDescriptor( LPFILEDESCRIPTOR pfgd, LPTSTR pPath, BOOL bUnicode )
{
#ifdef WINNT
    WIN32_FILE_ATTRIBUTE_DATA fad;

    // Get the file attributes (GetFileAttributesEx is
    // faster than FindFirstFile, so we'll use the former,
    // and copy the data into the structure of the latter).

    if( GetFileAttributesEx( pPath, GetFileExInfoStandard, &fad ))
    {
        pfgd->dwFileAttributes = fad.dwFileAttributes;
        pfgd->ftCreationTime   = fad.ftCreationTime;
        pfgd->ftLastAccessTime = fad.ftLastAccessTime;
        pfgd->ftLastWriteTime  = fad.ftLastWriteTime;
        pfgd->nFileSizeLow     = fad.nFileSizeLow;
        pfgd->dwFlags          = FD_ATTRIBUTES | FD_FILESIZE |
                                 FD_CREATETIME | FD_ACCESSTIME |
                                 FD_WRITESTIME;
    }
    else
    {
        if (bUnicode)
            memset( pfgd, 0, SIZEOF(FILEGROUPDESCRIPTORW) );
        else
            memset( pfgd, 0, SIZEOF(FILEGROUPDESCRIPTORA) );

        return;
    }
#else
    WIN32_FIND_DATA fd;
    HANDLE hFile;

    hFile = FindFirstFile(pPath, &fd);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        FindClose(hFile);

        pfgd->dwFileAttributes = fd.dwFileAttributes;
        pfgd->ftCreationTime   = fd.ftCreationTime;
        pfgd->ftLastAccessTime = fd.ftLastAccessTime;
        pfgd->ftLastWriteTime  = fd.ftLastWriteTime;
        pfgd->nFileSizeLow     = fd.nFileSizeLow;

        pfgd->dwFlags          = FD_ATTRIBUTES | FD_FILESIZE |
                                 FD_CREATETIME | FD_ACCESSTIME |
                                 FD_WRITESTIME;

    }
    else
    {
        if (bUnicode)
            memset( pfgd, 0, SIZEOF(FILEGROUPDESCRIPTORW) );
        else
            memset( pfgd, 0, SIZEOF(FILEGROUPDESCRIPTORA) );

        return;
    }
#endif

#ifdef UNICODE
    if (bUnicode)
    {
        lstrcpy( pfgd->cFileName, pPath);
    }
    else
    {
        WideCharToMultiByte( CP_ACP, 0,
                             pPath, -1,
                             (LPSTR)pfgd->cFileName,
                             ARRAYSIZE(pfgd->cFileName),
                             NULL,
                             NULL
                            );
    }
#else
    if (bUnicode)
    {
        MultiByteToWideChar( CP_ACP, 0,
                             pPath, -1,
                             (LPWSTR)pfgd->cFileName,
                             ARRAYSIZE(pfgd->cFileName)
                            );
    }
    else
    {
        lstrcpy( pfgd->cFileName, pPath);
    }
#endif

}


/*-----------------------------------------------------------------------------
/ GetFileDescriptors
/ -----------------------
/   Get the file descriptors for the given objects, this consists of a
/   array of leaf names and flags associated with them.
/
/ In:
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT
GetFileDescriptors( FORMATETC* pFmt,
                    STGMEDIUM* pMedium,
                    LPTSTR pRootPath,
                    HDPA hdpaSpecial,
                    HDPA hdpaShell
                   )
{
    HRESULT hr;
    DWORD cbStruct;
    LPFILEGROUPDESCRIPTOR pDescriptors = NULL;
    BOOL bUnicode = (pFmt->cfFormat == g_cfFileDescriptorsW);
    INT i, countSpecial = 0, countShell = 0, index;


    MDTraceEnter(TRACE_DATAOBJ, "GetFileDescriptors");

    //
    // Compute the structure size (to allocate the medium)
    //

    if (hdpaSpecial)
    {
        countSpecial = DPA_GetPtrCount(hdpaSpecial);
    }

    if (hdpaShell)
    {
        countShell = DPA_GetPtrCount(hdpaShell);
    }


    MDTrace(TEXT("Item count is %d special, %d shell"), countSpecial, countShell);

    if (bUnicode)
    {
        cbStruct  = SIZEOF(FILEGROUPDESCRIPTORW) +
                    (SIZEOF(FILEDESCRIPTORW) * (countSpecial + countShell));
    }
    else
    {
        cbStruct  = SIZEOF(FILEGROUPDESCRIPTORA) +
                    (SIZEOF(FILEDESCRIPTORA) * (countSpecial * countShell));
    }

    hr = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pDescriptors);
    FailGracefully(hr, "Failed to allocate storage medium");

    pDescriptors->cItems = countSpecial + countShell;

    index = 0;
    for ( i = 0; i < countSpecial; i++ )
    {
        TCHAR szPath [ MAX_PATH ];

        if (MDGetPathFromIDL( (LPITEMIDLIST)DPA_FastGetPtr(hdpaSpecial, i), szPath, pRootPath ))
        {
            FillInFileDescriptor( &(pDescriptors->fgd[index]), szPath, bUnicode );

            MDTrace( TEXT("Flags %08x, Attributes %08x, Name %s"),
                   pDescriptors->fgd[index].dwFlags,
                   pDescriptors->fgd[index].dwFileAttributes,
                   szPath
                  );

            index++;
        }

    }

    for ( i = 0; i < countShell; i++ )
    {
        TCHAR szPath [ MAX_PATH ];

        if (MDGetPathFromIDL( (LPITEMIDLIST)DPA_FastGetPtr(hdpaShell, i), szPath, pRootPath ))
        {
            FillInFileDescriptor( &(pDescriptors->fgd[index]), szPath, bUnicode );

            MDTrace( TEXT("Flags %08x, Attributes %08x, Name %s"),
                   pDescriptors->fgd[index].dwFlags,
                   pDescriptors->fgd[index].dwFileAttributes,
                   szPath
                  );
            index++;
        }

    }


    hr = S_OK;          // success


exit_gracefully:

    if ( FAILED(hr) )
        ReleaseStgMedium(pMedium);

    MDTraceLeaveResult(hr);
}


