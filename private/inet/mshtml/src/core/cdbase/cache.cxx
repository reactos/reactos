//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       cache.cxx
//
//  Contents:   CServer implmentation of IOleCache2
//
//  Classes:    CServer
//
//----------------------------------------------------------------------------

#include <headers.hxx>

MtDefine(CServerEnumCacheAry, Locals, "CServer::EnumCache CDataAry<STATDATA>")
MtDefine(CServerEnumCacheAry_pv, CServerEnumCacheAry, "CServer::EnumCache CDataAry<STATDATA>::_pv")
DECLARE_CDataAry(CServerEnumCacheAry, STATDATA, Mt(CServerEnumCacheAry), Mt(CServerEnumCacheAry_pv))

//+---------------------------------------------------------------------------
//
//  Member:     CServer::EnsureCache, public
//
//  Synopsis:   Initializes the cache object by creating the default data
//              cache and initializing its storage if necessary.
//
//  Arguments:  (none)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CServer::EnsureCache()
{
    HRESULT hr;

    //
    // Return immediatly if already initialized.
    //

    if (_pCache)
        return S_OK;

    hr = THR(CreateDataCache(NULL, CLSID_NULL, IID_IOleCache2,
            (void **)&_pCache));
    if (hr)
        goto Error;

    hr = THR(_pCache->QueryInterface(IID_IPersistStorage,
            (void **)&_pPStgCache));
    if (hr)
        goto Error;

    // BUGBUG Is it ok to ignore erros from pDefPstg methods called below?
    // If so, then use IGNORE_HR macro.

    if (_state >= OS_LOADED)
    {
        if (_pStg)
        {
            if (_fInitNewed)
            {
                _pPStgCache->InitNew(_pStg);
            }
            else
            {
                _pPStgCache->Load(_pStg);
                //
                // REVIEW - We fail if our container does a QI for IOleCache[2]
                // after calling Save() but before SaveCompleted().
                //
                if (_fNoScribble)
                {
                    TraceTag((tagError, "Attempt to QI for IOleCache(2) "
                                   "after a call to Save but before calling "
                                   "SaveCompleted!"));
                    TraceTag((tagError, "Fatal: Returning E_UNEXPECTED."));
                    hr = E_UNEXPECTED;
                    goto Error;
                }
            }
        }
        else if (_fHandsOff)
        {
            //
            // REVIEW - The Ctrl object is in Hands-Off state.  A QI for
            // IOleCache[2] is illegal at this time.
            //
            TraceTag((tagError, "Attempt to QI for IOleCache(2) "
                           "after a call to HandsOffStorage but before calling "
                           "SaveCompleted!"));
            TraceTag((tagError, "Fatal: Returning E_UNEXPECTED."));
            hr = E_UNEXPECTED;
            goto Error;
        }
        else
        {
            //  The control is using IPersistStreamInit which means we can't
            //  persist the cache data.  We have chosen to fail in this case,
            //  since the cache is not useful.
            //
            TraceTag((
                    tagError,
                    "Attempt to QI for IOleCache(2) on an "
                    "object which is being stored using IPersistStreamInit!"));
            TraceTag((tagError, "Fatal: Returning E_UNEXPECTED."));
            hr = E_UNEXPECTED;
            goto Error;
        }
    }

    hr = THR(_pCache->QueryInterface(IID_IViewObject2,
            (LPVOID*)&_pViewObjectCache));
    if (hr)
        goto Error;

Cleanup:
    RRETURN(hr);

Error:
    ClearInterface(&_pCache);
    ClearInterface(&_pPStgCache);
    ClearInterface(&_pViewObjectCache);
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::Cache, IOleCache
//
//  Synopsis:   Caches data
//
//  Arguments:  [petc]          -- Format to cache
//              [advf]          -- Cache flags
//              [pdwConnection] -- Place to put connection id.
//
//  Returns:    HRESULT
//
//  Notes:      As a DLL server, we normally do implicit caching of all our
//              data types.  The only exceptions are for the icon aspect and
//              if the ADVFCACHE_FORCEBUILTIN flag is specified.  Note that
//              the cache will not be persistent unless the container saves
//              us using a storage (as opposed to a stream).
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::Cache(FORMATETC *petc, DWORD advf, DWORD *pdwConnection)
{
    HRESULT hr;

    if (petc == NULL)
        RRETURN(E_INVALIDARG);

    if (petc->lindex != -1)
        RRETURN(DV_E_LINDEX);

    if (pdwConnection)
        *pdwConnection = 0;

    if ((petc->cfFormat == 0) ||
        (FindCompatibleFormat(ServerDesc()->_pGetFmtTable,
                              ServerDesc()->_cGetFmtTable,
                              *petc) != -1))
    {
        if ((advf & ADVFCACHE_FORCEBUILTIN) ||
            (petc->dwAspect == DVASPECT_ICON))
        {
            hr = THR(EnsureCache());
            if (hr)
                goto Cleanup;

             hr = THR(_pCache->Cache(petc, advf, pdwConnection));
        }
        else
        {
            // Implicit data type, so we return S_OK with a connection ID of
            // zero.
            hr = S_OK;
        }
    }
    else
    {
        //
        // Not a recognized format.
        //

        hr = DV_E_CLIPFORMAT;
    }

Cleanup:
    RRETURN2(hr, CACHE_S_FORMATETC_NOTSUPPORTED, CACHE_S_SAMECACHE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::Uncache, IOleCache
//
//  Synopsis:   Uncaches a previously cached data type
//
//  Arguments:  [dwConnection] -- Connection ID returned from call to ::Cache
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::Uncache(DWORD dwConnection)
{
    HRESULT hr;

    if (dwConnection == 0)     // They're uncaching an implicit data type.
        return S_OK;

    hr = THR(EnsureCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCache->Uncache(dwConnection));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::EnumCache, IOleCache
//
//  Synopsis:   Enumerates current cache nodes, including implicitely cached
//              data (which is pretty much everything).
//
//  Arguments:  [ppenumSTATDATA] -- Place to put enumerator
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::EnumCache(IEnumSTATDATA **ppenumSTATDATA)
{
    //
    // Due to the format of the GetFmtTable in the class descriptor, we
    // have to do some extra work here...
    //
    LPENUMSTATDATA          penum;
    LPFORMATETC             petc;
    STATDATA                statdata;
    int                     i;
    HRESULT                 hr;
    CServerEnumCacheAry *   pAryStat;

    hr = THR(EnsureCache());
    if (hr)
        goto Cleanup;

    pAryStat = new CServerEnumCacheAry;
    if (!pAryStat)
        RRETURN(E_OUTOFMEMORY);

    petc = ServerDesc()->_pGetFmtTable;

    statdata.advf   = 0;
    statdata.pAdvSink = NULL;
    statdata.dwConnection = 0;

    //
    // First, build our array of supported (implicitly cached) formats...
    //
    for (i = 0; i < ServerDesc()->_cGetFmtTable; i++)
    {
        statdata.formatetc = petc[i];

        pAryStat->AppendIndirect(&statdata);
    }

    //
    // Next, determine which formats are explictely cached and update the
    // info on them.
    //

    if (_pCache &&
            SUCCEEDED(_pCache->EnumCache(&penum)) && (penum != NULL))
    {
        while (penum->Next(1, &statdata, NULL) == S_OK)
        {
            for (i=0; i < pAryStat->Size(); i++)
            {
                if (statdata.formatetc.dwAspect == (*pAryStat)[i].formatetc.dwAspect)
                {
                    (*pAryStat)[i].advf = statdata.advf;
                    (*pAryStat)[i].dwConnection = statdata.dwConnection;
                }
            }

            CoTaskMemFree(statdata.formatetc.ptd);
            if (statdata.pAdvSink)
                statdata.pAdvSink->Release();
        }
        penum->Release();
    }

    //
    // The enumerator created during this call will delete pAryStat when
    // appropriate.
    //

    hr = THR(pAryStat->EnumElements(IID_IEnumSTATDATA,
            (LPVOID*)ppenumSTATDATA,
            FALSE, FALSE, TRUE));

    if (hr)
        goto Error;

Cleanup:
    RRETURN(hr);

Error:
    delete pAryStat;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::InitCache, public
//
//  Synopsis:   Initializes the cache.
//
//  Arguments:  [pDataObject] -- Data object to initialize from
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::InitCache(IDataObject *pDataObject)
{
    HRESULT hr;

    hr = THR(EnsureCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCache->InitCache(pDataObject));

Cleanup:
    RRETURN1(hr, CACHE_S_SOMECACHES_NOTUPDATED);
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::SetData, IOleObject
//
//  Synopsis:   Sets the data in the cache for a particlar data format.
//
//  Arguments:  [pformatetc] -- Format description
//              [pmedium]    -- Data
//              [fRelease]   -- If TRUE, pmedium will be released
//
//  Returns:    HRESULT
//
//  Notes:      We only allow the DVASPECT_ICON data to be set using this
//              method. All the other formats are implicitely cached.
//
//              Note that this function is renamed for SetData to SetDataCache
//              to avoid conflicts with IDataObject::SetData
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::SetDataCache(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    HRESULT hr;

    hr = THR(EnsureCache());
    if (hr)
        goto Cleanup;

    //
    // The only data we allow a caller to set is the icon
    //
    if (pformatetc->dwAspect == DVASPECT_ICON)
    {
        hr = THR(_pCache->SetData(pformatetc, pmedium, fRelease));
    }
    else
    {
        hr = THR(QueryGetData(pformatetc));

        if (SUCCEEDED(hr) && fRelease)
        {
            ReleaseStgMedium(pmedium);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::UpdateCache, IOleCache
//
//  Synopsis:   Updates the cache. Delegated to the default cache impl.
//
//  Arguments:  [pDataObject] -- Object to use for updating data
//              [grfUpdf]     -- Update flags
//              [pReserved]   -- Reserved
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::UpdateCache(LPDATAOBJECT pDataObject,
                       DWORD grfUpdf,
                       LPVOID pReserved)
{
    HRESULT hr;

    hr = THR(EnsureCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCache->UpdateCache(pDataObject, grfUpdf, pReserved));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::DiscardCache, public
//
//  Synopsis:   Discards the cache. Delegated to the default cache impl.
//
//  Arguments:  [dwDiscardOptions] -- Discard flags
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::DiscardCache(DWORD dwDiscardOptions)
{
    HRESULT hr;

    hr = THR(EnsureCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCache->DiscardCache(dwDiscardOptions));

Cleanup:
    RRETURN(hr);
}
