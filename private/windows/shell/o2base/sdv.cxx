//+---------------------------------------------------------------------
//
//   File:       sdv.cxx
//
//------------------------------------------------------------------------

//[ srvrdv_overview
/*
                        SrvrDV Overview

The SrvrDV base class implements the persistent data and view aspects common
to most OLE Compound Document objects.  It implements the IDataObject,
IViewObject, and IPersist family of interfaces.

The set of formats supported in IDataObject::GetData and SetData methods is
very object dependent.  SrvrDV uses a table approach to allow each derived
class to specify precisely exactly the set of formats supported.  For each
of GetData and SetData there is a pair of tables.  One is a table of FORMATETC
structures that specify information about the format supported.  A parallel
table contains a pointer to a function that implements each format supported.
SrvrDV has a set of static methods that implement the standard OLE clipboard
formats.  The derived class can include these methods in its Get/Set tables.

*/
//]

#include "headers.hxx"
#pragma hdrstop

WCHAR szContents[] = L"contents";

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::SrvrDV, protected
//
//  Synopsis:   Constructor for SrvrCtrl object
//
//  Notes:      To create a properly initialized object you must
//              call the Init method immediately after construction.
//
//---------------------------------------------------------------

SrvrDV::SrvrDV(void)
{
    DOUT(L"SrvrDV: Constructing\r\n");

    _pmk = NULL;
    _lpstrDisplayName = NULL;
    _sizel.cx = 0; _sizel.cy = 0;
    _pDataAdviseHolder = NULL;

    _fFrozen = FALSE;
    _pViewAdviseHolder = NULL;

    _fDirty = FALSE;
    _fNoScribble = FALSE;
    _pStg = NULL;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Init, protected
//
//  Synopsis:   Fully initializes a SrvrCtrl object as part of a compound
//              document server aggregate
//
//  Arguments:  [pClass] -- The initialized class descriptor for the server
//              [pCtrl] -- The control subobject of the server we are a part of.
//
//  Returns:    NOERROR if successful
//
//  Notes:      The Init method of the control subobject creates the data/view and
//              inplace subobjects of the object and calls the respective Init methods
//              on each, including this one.
//              The class descriptor pointer is saved in the protected _pClass
//              member variable where it is accessible during the lifetime
//              of the object.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::Init(LPCLASSDESCRIPTOR pClass, LPSRVRCTRL pCtrl)
{
    _pClass = pClass;
    _pCtrl = pCtrl;

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Init, protected
//
//  Synopsis:   Fully initializes a SrvrCtrl object from an existing object
//              to be used as a data transfer object
//
//  Arguments:  [pClass] -- The initialized class descriptor for the server
//              [pDV] -- The data/view subobject of the object that is the source
//                       of this transfer data object
//
//  Returns:    NOERROR if successful
//
//  Notes:      This method is used in the process of the GetClipboardCopy method
//              for obtaining a transfer data object from an existing object.
//              The class descriptor pointer is saved in the protected _pClass
//              member variable where it is accessible during the lifetime
//              of the object.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::Init(LPCLASSDESCRIPTOR pClass, LPSRVRDV pDV)
{
    _pClass = pClass;           // stash the class descriptor pointer
    _pCtrl = NULL;              // we are a free-floating transfer data object

    // copy over member variables as appropriate
    _pmk = pDV->_pmk;
    if (_pmk != NULL)
       _pmk->AddRef();

    TaskAllocString(pDV->_lpstrDisplayName, &_lpstrDisplayName);
    _sizel = pDV->_sizel;

    // we create a temporary IStorage and take a snapshot of the storage.
    // The temporary IStorage will be automatically deleted when this
    // data object is released
    // BUGBUG: What happens when we run out of memory?
    // BUGBUG: Should re-try with disk-based docfile?
    //
    HRESULT hr;
    LPSTORAGE pStg;
    if (OK(hr = CreateStorageOnHGlobal(NULL, &pStg)))
    {
        // do a "Save Copy As" into our storage instance.
        if (OK(hr = OleSave((LPPERSISTSTORAGE)pDV, pStg, FALSE)))
        {
            if (OK(hr = pStg->Commit(STGC_DEFAULT)))
            {
                if (OK(hr = ((LPPERSISTSTORAGE)pDV)->SaveCompleted(NULL)))
                    (_pStg = pStg)->AddRef();
            }
        }
        pStg->Release();
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::~SrvrDV, protected
//
//  Synopsis:   Destructor for the SrvrDV object
//
//  Notes:      The destructor is called as a result of the servers
//              reference count going to 0.  It releases any held resources
//              including advise holders and storages.
//
//---------------------------------------------------------------

SrvrDV::~SrvrDV(void)
{
    if (_pmk != NULL)
        _pmk->Release();

    if (_lpstrDisplayName != NULL)
        TaskFreeString(_lpstrDisplayName);

    if (_pDataAdviseHolder != NULL)
        _pDataAdviseHolder->Release();

    if (_pViewAdviseHolder != NULL)
        _pViewAdviseHolder->Release();

    if (_pStg != NULL)
        _pStg->Release();

    DOUT(L"SrvrDV: Destructed\r\n");
}

#ifdef DOCGEN  // documentation for pure virtual function
//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetClipboardCopy, public
//
//  Synopsis:   Produces a data-transfer object representing a snapshot
//              of this data/view object
//
//  Arguments:  [ppDV] -- The place where the data-transfer object is returned
//
//  Returns:    Success if the data-transfer object was created.
//
//  Notes:      This method is called
//              as a result of an IOleObject::GetClipboardData call on the
//              control subobject.  The overridden method should create a new
//              instance of itself and call the appropriate Init method on
//              the new instance passing the `this' pointer.  This way the
//              new instance can initialize itself as a snapshot of this object.
//              All servers must override this pure virtual method.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::GetClipboardCopy(LPSRVRDV FAR* ppDV)
{}
#endif  // DOCGEN

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::SetExtent, public
//
//  Synopsis:   Informs the object that it has a new expected size
//
//  Notes:      IOleObject::SetExtent and GetExtent methods are passed
//              directly from the control subobject to the data/view subobject
//              via these methods.   See OLE documentation for the
//              arguments and return values.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::SetExtent(DWORD dwAspect, SIZEL& sizel)
{
    HRESULT hr = NOERROR;
    switch(dwAspect)
    {
    case DVASPECT_CONTENT:
    case DVASPECT_DOCPRINT:
        _sizel = sizel;
        break;
    case DVASPECT_THUMBNAIL:    //REVIEW: what size is a thumbnail?
    case DVASPECT_ICON:
        DOUT(L"SrvrDV::SetExtent E_FAIL\r\n");
        hr = E_FAIL; // icon aspect is fixed size
        break;
    default:
        DOUT(L"SrvrDV::SetExtent E_INVALIDARG\r\n");
        hr = E_INVALIDARG;
        break;
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetExtent, public
//
//  Synopsis:   Requests the current size for some draw aspect of the object
//
//  Notes:      IOleObject::SetExtent and GetExtent methods are passed
//              directly from the control subobject to the data/view subobject
//              via these methods.   See OLE documentation for the
//              arguments and return values.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::GetExtent(DWORD dwAspect, LPSIZEL lpsizel)
{
#if DBG
    OLECHAR achTemp[256];
    wsprintf(achTemp,L"SrvrDV::GetExtent (dwApsect = %ld, cx = %ld, cy = %ld)\r\n",
            dwAspect, _sizel.cx, _sizel.cy);
    DOUT(achTemp);
#endif

    HRESULT hr = NOERROR;
    switch(dwAspect) {
    default:
        DOUT(L"SrvrDV::GetExtent INVALIDARG\r\n");
    case DVASPECT_CONTENT:
    case DVASPECT_DOCPRINT:
        *lpsizel = _sizel;
        break;

    case DVASPECT_THUMBNAIL:
    case DVASPECT_ICON:
        //BUGBUG:  The iconic view is actually a metafile of the
        // icon with a text-string underneath.
        // This isn't the right calculation...
        lpsizel->cx = HimetricFromHPix(32);
        lpsizel->cy = HimetricFromVPix(32);
        break;
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::OnDataChange, public
//
//  Synopsis:   Sets the dirty flag and raises data and view changed
//              to all registered advises
//
//  Arguments:  [dwAdvf] -- from the ADVF Data Advise flags.
//                          Usually this is 0.  ADVF_DATAONSTOP is used
//                          when the object is closing down.
//  Notes:      This function should be called whenever the native
//              data of the object is modified.
//
//---------------------------------------------------------------

void
SrvrDV::OnDataChange(DWORD dwAdvf)
{
    _fDirty = TRUE;
    if (_pViewAdviseHolder != NULL)
        _pViewAdviseHolder->SendOnViewChange(DVASPECT_CONTENT);
    else
        DOUT(L"SrvrDV::OnDataChange _pViewAdviseHolder == NULL\r\n");
    if (_pDataAdviseHolder != NULL)
        _pDataAdviseHolder->SendOnDataChange((LPDATAOBJECT)this, 0, dwAdvf);
    else
        DOUT(L"SrvrDV::OnDataChange _pDataAdviseHolder == NULL\r\n");
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::SetMoniker, public
//
//  Synopsis:   Informs the object of its new, full moniker
//
//  Arguments:  [pmk] -- full moniker to the object
//
//  Notes:      The data/view subobject records the full moniker to the
//              object so it can properly dispense the standard OLE
//              Object Descriptor and Link Source clipboard formats.
//              This method is called whenever the IOleObject::SetMoniker
//              method is called on the control subobject.
//
//---------------------------------------------------------------

void
SrvrDV::SetMoniker(LPMONIKER pmk)
{
    if (_pmk != NULL)
    {
        _pmk->Release();

        if (_lpstrDisplayName != NULL)        // flush our cached display name
        {
            TaskFreeString(_lpstrDisplayName);
            _lpstrDisplayName = NULL;
        }
    }

    _pmk = pmk;

    if (_pmk != NULL)
    {
        _pmk->AddRef();
    }
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetMoniker, public
//
//  Synopsis:   Returns the full moniker to the object
//
//  Arguments:  [dwAssign] -- See IOleObject::GetMoniker
//              [ppmk] -- The place where the moniker is returned
//
//  Returns:    Success if the moniker is available.
//
//  Notes:      This returns the moniker that this data/view subobject has
//              previously recorded.  If no moniker is yet assigned
//              then the moniker is requested from the client site
//              via the IOleObject::GetMoniker method on the control subobject.
//              This method is used by the GetOBJECTDESCRIPTOR and GetLINKSOURCE
//              methods.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::GetMoniker(DWORD dwAssign, LPMONIKER FAR* ppmk)
{
    DOUT(L"SrvrDV::GetMoniker\r\n");
    *ppmk = NULL;   // set out params to NULL

    HRESULT hr = NOERROR;
    if (_pmk == NULL)
    {
        if (_pCtrl == NULL)
        {
            DOUT(L"SrvrDV::GetMoniker E_INVALIDARG\r\n");
            hr = MK_E_UNAVAILABLE;
        }
        else
        {
            hr = _pCtrl->GetMoniker(dwAssign, OLEWHICHMK_OBJFULL, &_pmk);
        }
    }

    if (OK(hr))
        (*ppmk = _pmk)->AddRef();

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetMonikerDisplayName, public
//
//  Synopsis:   Returns the display name from the object's moniker
//
//  Notes:      The display name of the object is used in for
//              dispensing the Object Descriptor clipboard format.
//              The caller must not free the string returned.
//
//---------------------------------------------------------------

LPWSTR
SrvrDV::GetMonikerDisplayName(DWORD dwAssign)
{
    //
    // NOTE: default dwAssign is OLEGETMONIKER_ONLYIFTHERE
    //
    // we maintain a moniker display name cache in the form of member
    // variable _lpstrDisplayName.
    //
    // If we don't have a display name cached then take our moniker and
    // squeeze a display name out of it
    //
    if (!_lpstrDisplayName)
    {
        LPMONIKER pmk;
        if (OK(GetMoniker(dwAssign, &pmk)))
        {
            ::GetMonikerDisplayName(pmk, &_lpstrDisplayName);
            pmk->Release();
        }
    }
    return _lpstrDisplayName;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetEMBEDDEDOBJECT, public
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the standard Embedded Object clipboard format
//
//  Arguments:  [pv] -- pointer to a SrvrDV object
//              [pformatetc] -- as in IDataObject::GetData, GetDataHere
//              [pmedium] -- as in IDataObject::GetData, GetDataHere
//              [fHere] -- TRUE for GetDataHere, FALSE for GetData
//
//  Returns:    Success if the clipboard format could be dispensed
//
//  Notes:      This and the other static GetXXX methods are for use
//              in the server's Get format tables.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::GetEMBEDDEDOBJECT( LPSRVRDV pDV,
                        LPFORMATETC pformatetc,
                        LPSTGMEDIUM pmedium,
                        BOOL fHere)
{
    LPPERSISTSTORAGE pPStg = (LPPERSISTSTORAGE)(LPSRVRDV)pDV;
    HRESULT hr = NOERROR;
    if (!fHere)
    {
        // fill in the pmedium structure
        pmedium->tymed = TYMED_ISTORAGE;
        hr = StgCreateDocfile(NULL,
                            STGM_DFRALL | STGM_CREATE | STGM_DELETEONRELEASE,
                            0L,
                            &pmedium->pstg);
        pmedium->pUnkForRelease = NULL;
    }

    if (OK(hr))
    {
        if (OK(hr = OleSave(pPStg, pmedium->pstg, FALSE)))
            hr = pPStg->SaveCompleted(NULL);

        // if we failed somehow and yet created a docfile, then we will
        // release the docfile to delete it
        //
        if (!OK(hr) && !fHere)
            pmedium->pstg->Release();
    }

    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetMETAFILEPICT, public
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the standard Metafilepict clipboard format
//
//  Arguments:  [pv] -- pointer to a SrvrDV object
//              [pformatetc] -- as in IDataObject::GetData, GetDataHere
//              [pmedium] -- as in IDataObject::GetData, GetDataHere
//              [fHere] -- TRUE for GetDataHere, FALSE for GetData
//
//  Returns:    Success if the clipboard format could be dispensed
//
//  Notes:      This member function uses IViewObject::Draw to construct
//              the metafile pict.
//              This and the other static GetXXX methods are for use
//              in the server's Get format tables.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::GetMETAFILEPICT( LPSRVRDV pDV,
                        LPFORMATETC pformatetc,
                        LPSTGMEDIUM pmedium,
                        BOOL fHere)
{
    DOUT(L"o2base/SrvrDV::GetMETAFILEPICT\r\n");

    LPVIEWOBJECT pView = (LPVIEWOBJECT)pDV;
    SIZEL sizel;
    pDV->GetExtent(pformatetc->dwAspect, &sizel);

    //RECT rc = { 0, 0, HPixFromHimetric(sizel.cx), VPixFromHimetric(sizel.cy) };
    RECT rc = { 0, 0, sizel.cx, sizel.cy };
    HRESULT hr = NOERROR;
    if (!fHere)
    {
        // fill in the pmedium structure
        pmedium->tymed = TYMED_MFPICT;
        pmedium->hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(METAFILEPICT));
        if (pmedium->hGlobal == NULL)
        {
            DOUT(L"o2base/SrvrDV::GetMETAFILEPICT failed\r\n");
            hr = E_OUTOFMEMORY;
        }
        pmedium->pUnkForRelease = NULL;
    }

    if (OK(hr))
    {
        HMETAFILE hmf;
        if (OK(hr = DrawMetafile(pView, rc, pformatetc->dwAspect, &hmf)))
        {
            LPMETAFILEPICT pPict = (LPMETAFILEPICT)GlobalLock(pmedium->hGlobal);
            if (pPict == NULL)
            {
                DOUT(L"SrvrDV::GetMETAFILEPICT E_INVALIDARG\r\n");

                DeleteMetaFile(hmf);
                hr = E_INVALIDARG;
            }
            else
            {
                // fill in the object descriptor
                pPict->mm   =  MM_ANISOTROPIC;
                pPict->hMF  =  hmf;
                pPict->xExt =  rc.right;
                pPict->yExt =  rc.bottom;

                GlobalUnlock(pmedium->hGlobal);
            }
        }

        // if we failed somehow and yet allocated memory,
        // then we will release it here...
        //
        if (!OK(hr) && !fHere)
            GlobalFree(pmedium->hGlobal);
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetOBJECTDESCRIPTOR, public
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the standard Object Descriptor clipboard format
//
//  Arguments:  [pv] -- pointer to a SrvrDV object
//              [pformatetc] -- as in IDataObject::GetData, GetDataHere
//              [pmedium] -- as in IDataObject::GetData, GetDataHere
//              [fHere] -- TRUE for GetDataHere, FALSE for GetData
//
//  Returns:    Success if the clipboard format could be dispensed
//
//  Notes:      This and the other static GetXXX methods are for use
//              in the server's Get format tables.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::GetOBJECTDESCRIPTOR( LPSRVRDV pDV,
                            LPFORMATETC pformatetc,
                            LPSTGMEDIUM pmedium,
                            BOOL fHere)
{
    HRESULT hr = NOERROR;
    LPCLASSDESCRIPTOR pClass = pDV->_pClass;

    //
    //REVIEW: what's the best display name for the OBJECTDESCRIPTOR?
    //

    OLECHAR achDisplay[256];
    lstrcpy(achDisplay, L"Microsoft ");
    lstrcat(achDisplay, pClass->_szUserClassType[USERCLASSTYPE_FULL]);
    LPOLESTR lpstrDisplay = achDisplay;
    DWORD dwDisplay = lpstrDisplay ?
                            (lstrlen(lpstrDisplay) + 1) * sizeof(OLECHAR) : 0;
    LPOLESTR lpstrUserTypeFull = pClass->_szUserClassType[USERCLASSTYPE_FULL];
    DWORD dwUserTypeFull = lpstrUserTypeFull ?
                            (lstrlen(lpstrUserTypeFull) + 1) * sizeof(OLECHAR) : 0;

    DWORD dwSize = sizeof(OBJECTDESCRIPTOR) + dwUserTypeFull + dwDisplay;

    if (!fHere)
    {
        // compute the amount of memory required

        // fill in the pmedium structure
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, dwSize);
        if (pmedium->hGlobal == NULL)
        {
            DOUT(L"o2base/SrvrDV::GetOBJECTDESCRIPTOR failed (pmedium)\r\n");
            hr = E_OUTOFMEMORY;
        }

        pmedium->pUnkForRelease = NULL;
    }

    if (OK(hr) && (GlobalSize(pmedium->hGlobal) >= dwSize))
    {
        LPOBJECTDESCRIPTOR pObjDesc =
                (LPOBJECTDESCRIPTOR)GlobalLock(pmedium->hGlobal);
        if (pObjDesc == NULL)
        {
            DOUT(L"o2base/SrvrDV::GetOBJECTDESCRIPTOR failed\r\n");
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //
            // fill in the object descriptor
            //
            //pObjDesc->cbSize = sizeof(OBJECTDESCRIPTOR);
            pObjDesc->cbSize = dwSize;
            pObjDesc->clsid = pClass->_clsid;
            pObjDesc->dwDrawAspect = DVASPECT_CONTENT;
            pObjDesc->sizel = pDV->_sizel;
            pObjDesc->pointl.y = pObjDesc->pointl.x = 0;
            pObjDesc->dwStatus = pClass->_dwMiscStatus;

            LPWSTR lpstrDest = (LPWSTR)(pObjDesc + 1);
            if(lpstrUserTypeFull)
            {
                lstrcpy(lpstrDest, lpstrUserTypeFull);
                pObjDesc->dwFullUserTypeName = sizeof(OBJECTDESCRIPTOR);
                lpstrDest += lstrlen(lpstrUserTypeFull) + 1;
            }
            else
                pObjDesc->dwFullUserTypeName = 0;

            if (lpstrDisplay)
            {
                pObjDesc->dwSrcOfCopy = pObjDesc->dwFullUserTypeName + dwUserTypeFull;
                lstrcpy(lpstrDest, lpstrDisplay);
            }
            else
                pObjDesc->dwSrcOfCopy = 0;

            GlobalUnlock(pmedium->hGlobal);
            hr = NOERROR;
        }

        // if we failed somehow and yet allocated memory,
        // then we will release it here...
        if (!OK(hr) && !fHere)
            GlobalFree(pmedium->hGlobal);
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetLINKSOURCE, public
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the standard Link Source clipboard format
//
//  Arguments:  [pv] -- pointer to a SrvrDV object
//              [pformatetc] -- as in IDataObject::GetData, GetDataHere
//              [pmedium] -- as in IDataObject::GetData, GetDataHere
//              [fHere] -- TRUE for GetDataHere, FALSE for GetData
//
//  Returns:    Success if the clipboard format could be dispensed
//
//  Notes:      This method uses the moniker cached by the data/view
//              object.
//              This and the other static GetXXX methods are for use
//              in the server's Get format tables.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::GetLINKSOURCE( LPSRVRDV pDV,
                    LPFORMATETC pformatetc,
                    LPSTGMEDIUM pmedium,
                    BOOL fHere)
{
    DOUT(L"SrvrDV::GetLINKSOURCE\r\n");
    LPMONIKER pmk;
    HRESULT hr;
    //if (OK(hr = pDV->GetMoniker(OLEGETMONIKER_ONLYIFTHERE, &pmk)))
    if (OK(hr = pDV->GetMoniker(OLEGETMONIKER_FORCEASSIGN, &pmk)))
    {
        if (!fHere)
        {
            pmedium->tymed = TYMED_ISTREAM;
            HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pmedium->pstm);
            pmedium->pUnkForRelease = NULL;
        }

        if (OK(hr))
        {
            CLSID clsid;
            if (OK(hr = pmk->GetClassID(&clsid)))
            {
                if (OK(hr = WriteClassStm(pmedium->pstm, clsid)))
                    hr = pmk->Save(pmedium->pstm, FALSE);
            }
        }

        pmk->Release();
    }

    return hr;
}


//+--------------------------------------------------------------
//
//  Member:     SrvrDV::GetData, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method searches the server's Get format table
//              for a compatible format.  If one is found it calls
//              the corresponding Get function.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::GetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    DOUT(L"SrvrDV::GetData\r\n");

    if (pformatetc == NULL || pmedium == NULL)
    {
        DOUT(L"SrvrDV::GetData E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    HRESULT hr;
    int i = FindCompatibleFormat(_pClass->_pGetFmtTable,
            _pClass->_cGetFmtTable,
            *pformatetc);
    if (i < 0)
    {
        DOUT(L"SrvrDV::GetData DV_E_FORMATETC\r\n");
        hr = DV_E_FORMATETC;
    }
    else
    {
        hr = (*_pGetFuncs[i]) (this, pformatetc, pmedium, FALSE);
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetDataHere, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method searches the server's Get format table
//              for a compatible format.  If one is found it calls
//              the corresponding Get function.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    DOUT(L"SrvrDV:GetDataHere\r\n");

    HRESULT hr;
    int i = FindCompatibleFormat(_pClass->_pGetFmtTable,
            _pClass->_cGetFmtTable,
            *pformatetc);
    if (i < 0)
    {
        DOUT(L"SrvrDV::GetDataHere DV_E_FORMATETC\r\n");
        hr = DV_E_FORMATETC;
    }
    else
    {
        hr = (*_pGetFuncs[i]) (this, pformatetc, pmedium, TRUE);
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::QueryGetData, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method searches the server's Get format table
//              for a compatible format.  The return value indicates
//              whether or not a compatible format was found.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::QueryGetData(LPFORMATETC pformatetc)
{
    return FindCompatibleFormat(_pClass->_pGetFmtTable,
            _pClass->_cGetFmtTable,
            *pformatetc) >=0 ? NOERROR : DV_E_FORMATETC;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetCanonicalFormatEtc, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method returns DATA_S_SAMEFORMATETC assuming
//              that each format the server dispenses is its own
//              canonical format.  If this is not the case then this
//              method should be overridden.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::GetCanonicalFormatEtc(LPFORMATETC pformatetc,
    LPFORMATETC pformatetcOut)
{
    return DATA_S_SAMEFORMATETC;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::SetData, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method searches the server's Set format table
//              for a compatible format.  If one is found it calls
//              the corresponding Set function.
//
//---------------------------------------------------------------
STDMETHODIMP
SrvrDV::SetData(LPFORMATETC pformatetc, STGMEDIUM FAR *pmedium, BOOL fRelease)
{
    DOUT(L"SrvrDV:SetData\r\n");

    HRESULT hr;
    int i = FindCompatibleFormat(_pClass->_pSetFmtTable,
            _pClass->_cSetFmtTable,
            *pformatetc);
    if (i < 0)
    {
        DOUT(L"SrvrDV::SetData DV_E_FORMATETC\r\n");
        hr = DV_E_FORMATETC;
    }
    else
    {
        hr = (*_pSetFuncs[i]) (this, pformatetc, pmedium);
    }

    if (fRelease)
    {
        ReleaseStgMedium(pmedium);
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::EnumFormatEtc, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method creates an enumerator over the Get or
//              Set format tables depending on the value of the
//              dwDirection argument.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc)
{
    if (ppenumFormatEtc == NULL)
    {
        DOUT(L"SrvrDV::EnumFormatEtc E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }
    *ppenumFormatEtc = NULL;            // set out params to NULL

    // create an enumerator over our static format table.
    HRESULT hr = E_INVALIDARG;
    switch (dwDirection)
    {
    case DATADIR_GET:
        hr = CreateFORMATETCEnum(_pClass->_pGetFmtTable,
                _pClass->_cGetFmtTable,
                ppenumFormatEtc);
        break;
    case DATADIR_SET:
        hr = CreateFORMATETCEnum(_pClass->_pSetFmtTable,
                _pClass->_cSetFmtTable,
                ppenumFormatEtc);
        break;
    default:
        DOUT(L"SrvrDV::EnumFormatEtc E_INVALIDARG (2)\r\n");
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::DAdvise, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method uses the standard OLE data advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::DAdvise(FORMATETC FAR* pFormatetc,
                DWORD advf,
                LPADVISESINK pAdvSink,
                DWORD FAR* pdwConnection)
{
    DOUT(L"SrvrDV::DAdvise\r\n");
    if (pdwConnection == NULL)
    {
        DOUT(L"SrvrDV::DAdvise E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *pdwConnection = NULL;              // set out params to NULL

    HRESULT hr = NOERROR;
    if (_pDataAdviseHolder == NULL)
        hr = CreateDataAdviseHolder(&_pDataAdviseHolder);

    if (OK(hr))
    {
        hr = _pDataAdviseHolder->Advise((LPDATAOBJECT)this,
                pFormatetc,
                advf,
                pAdvSink,
                pdwConnection);
    }
    else
    {
        DOUT(L"SrvrDV::DAdvise CreateDataAdviseHolder error\r\n");
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::DUnadvise, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method uses the standard OLE data advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::DUnadvise(DWORD dwConnection)
{
    if (_pDataAdviseHolder == NULL)
        return NOERROR;

    return _pDataAdviseHolder->Unadvise(dwConnection);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::EnumDAdvise, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method uses the standard OLE data advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::EnumDAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
    if (ppenumAdvise == NULL)
    {
        DOUT(L"SrvrDV::EnumDAdvise E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *ppenumAdvise = NULL;               // set out params to NULL

    HRESULT hr;
    if (_pDataAdviseHolder == NULL)
        hr = NOERROR;
    else
        hr = _pDataAdviseHolder->EnumAdvise(ppenumAdvise);

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::RenderContent, public
//
//  Synopsis:   Used to draw the content aspect of the object
//
//  Notes:      This method is used by the implementation of IViewObject::Draw
//              when the content aspect is requested.  The parameters are
//              identical to those of IViewObject::Draw.
//              All objects should override this method.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::RenderContent(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        HDC hdcDraw,
        LPCRECTL lprectl,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue) (DWORD),
        DWORD dwContinue)
{
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::RenderPrint, public
//
//  Synopsis:   Used to draw the print aspect of the object
//
//  Notes:      This method is used by the implementation of IViewObject::Draw
//              when the docprint aspect is requested.  The parameters are
//              identical to those of IViewObject::Draw.
//              By default this method calls RenderContent.  If the
//              server has special processing for the print case then
//              this method should be overridden.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::RenderPrint(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        HDC hdcDraw,
        LPCRECTL lprectl,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue) (DWORD),
        DWORD dwContinue)
{
    return RenderContent(dwDrawAspect,
            lindex,
            pvAspect,
            ptd,
            hicTargetDev,
            hdcDraw,
            lprectl,
            lprcWBounds,
            pfnContinue,
            dwContinue);
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::RenderThumbnail, public
//
//  Synopsis:   Used to draw the thumbnail aspect of the object
//
//  Notes:      This method is used by the implementation of IViewObject::Draw
//              when the thumbnail aspect is requested.  The parameters are
//              identical to those of IViewObject::Draw.
//              All objects should override this method.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::RenderThumbnail(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        HDC hdcDraw,
        LPCRECTL lprectl,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue) (DWORD),
        DWORD dwContinue)
{
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Draw, public
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method calls RenderContent/Print/Thumbnail for
//              those respective aspects.  It handles the icon aspect
//              automatically using the icon found in the class descriptor
//              indicated by the _pClass member.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::Draw(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        HDC hdcDraw,
        LPCRECTL lprectl,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue) (DWORD),
        DWORD dwContinue)
{
    HRESULT hr;
    switch(dwDrawAspect) {
    case DVASPECT_CONTENT:
        hr = RenderContent(dwDrawAspect,
                lindex,
                pvAspect,
                ptd,
                hicTargetDev,
                hdcDraw,
                lprectl,
                lprcWBounds,
                pfnContinue,
                dwContinue);
        break;

    case DVASPECT_DOCPRINT:
        hr = RenderPrint(dwDrawAspect,
                lindex,
                pvAspect,
                ptd,
                hicTargetDev,
                hdcDraw,
                lprectl,
                lprcWBounds,
                pfnContinue,
                dwContinue);
        break;

    case DVASPECT_THUMBNAIL:
        hr = RenderThumbnail(dwDrawAspect,
                lindex,
                pvAspect,
                ptd,
                hicTargetDev,
                hdcDraw,
                lprectl,
                lprcWBounds,
                pfnContinue,
                dwContinue);
        break;

    case DVASPECT_ICON:
        {
            //BUGBUG: This is not the right way to do iconic aspect rendering!
            RECT rc;
            RECTL rcTemp = *lprectl;
            RECTLtoRECT(rcTemp, &rc);
            DrawIcon(hdcDraw, rc.left, rc.top, _pClass->_hicon);
            hr = NOERROR;
        }
        break;

    default:
        DOUT(L"SrvrDV::Draw E_INVALIDARG\r\n");
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetColorSet, public
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method returns S_FALSE indicating the server
//              does not support this functionality.  Server's that
//              wish to support it should override this method.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::GetColorSet(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        LPLOGPALETTE FAR* ppColorSet)
{
    if (ppColorSet == NULL)
    {
        DOUT(L"SrvrDV::GetColorSet E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *ppColorSet = NULL; //set out params to NULL

    return S_FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Freeze, public
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method sets the frozen flag, _fFrozen.
//              The derived class must pay attention to this flag
//              and not allow any modifications that would change
//              the current rendering.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::Freeze(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DWORD FAR* pdwFreeze)
{
    if (pdwFreeze == NULL)
    {
        DOUT(L"SrvrDV::Freeze E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *pdwFreeze = 0; //set out params to NULL

    _fFrozen = TRUE;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Unfreeze, public
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method clears the frozen flag, _fFrozen.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::Unfreeze(DWORD dwFreeze)
{
    _fFrozen = FALSE;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::SetAdvise, public
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method implements an advise holder for the view
//              advise.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::SetAdvise(DWORD aspects, DWORD advf, LPADVISESINK pAdvSink)
{
    HRESULT hr = NOERROR;
    if (_pViewAdviseHolder == NULL)
        hr = CreateViewAdviseHolder(&_pViewAdviseHolder);

    if (OK(hr))
        hr = _pViewAdviseHolder->SetAdvise(aspects, advf, pAdvSink);

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetAdvise, public
//
//  Synopsis:   Method of IViewObject interface
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::GetAdvise(DWORD FAR* pAspects,
        DWORD FAR* pAdvf,
        LPADVISESINK FAR* ppAdvSink)
{
    if (ppAdvSink == NULL)
    {
        DOUT(L"SrvrDV::GetAdvise E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *ppAdvSink = NULL;                  // set out params to NULL

    HRESULT hr;
    if (_pViewAdviseHolder==NULL)
        hr = NOERROR;
    else
        hr = _pViewAdviseHolder->GetAdvise(pAspects, pAdvf, ppAdvSink);

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::LoadFromStream, protected
//
//  Synopsis:   Loads the object's persistent state from a stream
//
//  Arguments:  [pStrm] -- stream to load from
//
//  Returns:    Success iff persistent state was read
//
//  Notes:      This function is used in the implementation of
//              IPersistStream::Load and IPersistFile::Load when
//              the file is not a docfile.
//              All objects should override this method.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::LoadFromStream(LPSTREAM pStrm)
{
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::SaveToStream, protected
//
//  Synopsis:   Saves the object's persistent state to a stream
//
//  Arguments:  [pStrm] -- stream to save to
//
//  Returns:    Success iff persistent state was written
//
//  Notes:      This function is used in the implementation of
//              IPersistStream::Save and IPersistFile::Save when
//              the file is not a docfile.
//              All objects should override this method.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::SaveToStream(LPSTREAM pStrm)
{
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetStreamSizeMax, protected
//
//  Synopsis:   Returns the number of bytes required to serialize object
//
//  Notes:      This function is used in the implementation of
//              IPersistStream::GetSizeMax.
//              All objects should override this method.
//
//---------------------------------------------------------------
DWORD
SrvrDV::GetStreamSizeMax(void)
{
    return 0;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetClassID, public
//
//  Synopsis:   Method of IPersist interface
//
//  Notes:      This method uses the class id in the class descriptor.
//
//---------------------------------------------------------------
STDMETHODIMP
SrvrDV::GetClassID(LPCLSID lpClassID)
{
    if (lpClassID == NULL)
    {
        DOUT(L"SrvrDV::GetClassID E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *lpClassID = _pClass->_clsid;
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     SrvrDV::IsDirty, public
//
//  Synopsis:   Method of IPersistStream/Storage/File interface
//
//  Notes:      This method uses the dirty flag, _fDirty.
//              Objects should not set the _fDirty flag directly
//              but instead call the OnDataChange method to set the
//              flag.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::IsDirty(void)
{
    return (_fDirty ? NOERROR : S_FALSE);
}


//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Load, public
//
//  Synopsis:   Method of IPersistStream interface
//
//  Notes:      This function uses the LoadFromStream method and
//              transitions the object to the loaded state if the
//              load was successful.
//
//---------------------------------------------------------------
STDMETHODIMP
SrvrDV::Load(LPSTREAM pStrm)
{
    // object can be loaded only once!
    if (_pCtrl->State() != OS_PASSIVE)
    {
        DOUT(L"SrvrDV::Load E_FAIL\r\n");
        return E_FAIL;
    }

    HRESULT hr;
    if (OK(hr = LoadFromStream(pStrm)))
        hr = _pCtrl->TransitionTo(OS_LOADED);

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Save, public
//
//  Synopsis:   Method of IPersistStream interface
//
//  Notes:      This method uses the SaveToStream method and
//              clears the _fDirty flag as appropriate.
//              Containers that have nonserializeable embeddings can
//              override this method and return STG_E_CANTSAVE
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::Save(LPSTREAM pStrm, BOOL fClearDirty)
{
    HRESULT hr;
    if (OK(hr = SaveToStream(pStrm)))
    {
        if (fClearDirty)
        {
            _fDirty = FALSE;
        }
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetSizeMax
//
//  Synopsis:   Method of IPersistStream interface
//
//  Notes:
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::GetSizeMax(ULARGE_INTEGER FAR * pcbSize)
{
    if (pcbSize == NULL)
    {
        DOUT(L"SrvrDV::GetSizeMax E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    ULISet32(*pcbSize, GetStreamSizeMax());

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::LoadFromStorage, protected
//
//  Synopsis:   Loads the object's persistent state from a storage
//
//  Arguments:  [pSg] -- storage to load from
//
//  Returns:    Success iff persistent state was read
//
//  Notes:      This function is used in the implementation of
//              IPersistStorage::Load and IPersistFile::Load when
//              the file is a docfile.
//              This method opens a stream, "CONTENTS", and uses
//              method LoadFromStream to complete the load.
//              Servers that do more sophisticated loading will want
//              to override this method.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::LoadFromStorage(LPSTORAGE pStg)
{
    LPSTREAM pStrm;
    HRESULT hr;
    if (OK(hr = pStg->OpenStream(szContents, NULL, STGM_SRO, 0, &pStrm)))
    {
        hr = LoadFromStream(pStrm);
        pStrm->Release();
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member: SrvrDV::SaveToStorage
//
//  Synopsis:   Saves the object's persistent state to a storage
//
//  Arguments:  [pSg] -- storage to save to
//
//  Returns:    Success iff persistent state was written
//
//  Notes:      This function is used in the implementation of
//              IPersistStorage::Save and IPersistFile::Save when
//              the file is a docfile.
//              This method opens a stream, "CONTENTS", and uses
//              method SaveToStream to complete the save.
//              Servers that do more sophisticated saving will want
//              to override this method.
//
//---------------------------------------------------------------

HRESULT
SrvrDV::SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad)
{
    // write our native data stream
    HRESULT hr;
    LPSTREAM pStrm;
    hr = pStg->CreateStream(szContents, STGM_SALL|STGM_CREATE, 0L, 0L, &pStrm);
    if (OK(hr))
    {
        hr = SaveToStream(pStrm);
        pStrm->Release();
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::InitNew
//
//  Synopsis:   IPersistStorage Method
//
//  Notes:      This method transitions the object to loaded.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::InitNew(LPSTORAGE pStg)
{
    //
    //REVIEW: what happens if we attempt to load the same ctrl more than once?
    //
    if (pStg == NULL)
    {
        DOUT(L"SrvrDV::InitNew E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    HRESULT hr;
    if (OK(hr = _pCtrl->TransitionTo(OS_LOADED)))
        (_pStg = pStg)->AddRef();   // hold on to the storage

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Load
//
//  Synopsis:   IPersistStorage Method
//
//  Notes:      This method loads the object using LoadFromStorage and
//              then transitions the object to loaded.
//              A pointer to our storage is maintained in member variable
//              _pStg.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::Load(LPSTORAGE pStg)
{
    // object can be loaded only once!
    if (_pCtrl->State() != OS_PASSIVE)
    {
        DOUT(L"SrvrDV::Load E_FAIL\r\n");
        return E_FAIL;
    }

    if (pStg == NULL)
    {
        DOUT(L"SrvrDV::Load E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    // do the load and move to the loaded state
    HRESULT hr;
    if (OK(hr = LoadFromStorage(pStg)))
    {
        if (OK(hr = _pCtrl->TransitionTo(OS_LOADED)))
        {
            (_pStg = pStg)->AddRef();
        }
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Save
//
//  Synopsis:   Method of IPersistStorage interface
//
//  Notes:      This method uses SaveToStorage to write the persistent
//              state.  It also writes the full user type string to the
//              storage as is required.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::Save(LPSTORAGE pStg, BOOL fSameAsLoad)
{
    if (pStg == NULL)
    {
        DOUT(L"SrvrDV::Save E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    // write our native data stream
    HRESULT hr;
    if (OK(hr = SaveToStorage(pStg, fSameAsLoad)))
    {
        // Write the UserType string. We don't let this fail the operation.
        WriteFmtUserTypeStg(pStg,
                0,
                _pClass->_szUserClassType[USERCLASSTYPE_FULL]);

        _fNoScribble = TRUE;
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::SaveCompleted
//
//  Synopsis:   Method of IPersistStorage interface
//
//  Notes:      This method clears the dirty flag and updates our
//              storage pointer, _pStg, if required.
//              Servers that are also containers will want to override
//              this method to pass the call recursively to all loaded
//              embeddings.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::SaveCompleted(LPSTORAGE pStg)
{
    // if pStg is non-null then we are coming out of HANDS-OFF mode,
    // otherwise we are coming out of NO-SCRIBBLE mode.
    if (pStg != NULL)
    {
        // We should be in HANDS-OFF mode and hence able to Assert that
        // _pStg is NULL here by virtue of the HandsOffStorage call.
        // However, the official OLE sample container app "Outline"
        // fail to make the HandsOffStorage call.
        // In order to be robust we release our _pStg handle if it is
        // "illegally" NON-NULL
        //
        if (_pStg != NULL)
        {
            DOUT(L"SrvrDV: WARNING! SaveCompleted: ");
            DOUT(L"SrvrDV: Container failed to make required HandsOffStorage call.\n");
            _pStg->Release();
        }

        (_pStg = pStg)->AddRef();   // hold on to the new storage
    }

    _fDirty = FALSE;                // clear our dirty flag
    _fNoScribble = FALSE;           // we are out of NO-SCRIBBLE mode

    //REVIEW: should we advise in the case we are not fRemembering?
    if (_pCtrl != NULL)
        _pCtrl->OnSave();           // and notify any advises that we have saved

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::HandsOffStorage
//
//  Synopsis:   Method of IPersistStorage interface
//
//  Notes:      This method releases the storage we are holding on to.
//              Servers that are also containers will want to override
//              this method to pass the call recursively to all loaded
//              embeddings.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::HandsOffStorage(void)
{
    if (_pStg != NULL)
        _pStg->Release();
    _pStg = NULL;

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Load, public
//
//  Synopsis:   Method of IPersistFile interface
//
//  Notes:      This opens the file as a docfile and uses IPersistStorage::Load
//              to complete the operation.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::Load(LPCOLESTR lpstrFile, DWORD grfMode)
{
    // use the default storage modes if no flags were specified
    if (grfMode == 0)
        grfMode = STGM_DFRALL;

    // if they didn't specify a share mode then use deny-write
    if ( (grfMode & STGM_SHARE) == 0)
        grfMode |= STGM_SHARE_DENY_WRITE;

    // of course, we use transacted mode
    grfMode |= STGM_TRANSACTED;

    HRESULT hr;
    if (lpstrFile == NULL)
    {
        // lpstrFile NULL is a special-case indicating that we should
        // create a temporary docfile for the new file case
        //
        grfMode |= STGM_CREATE | STGM_DELETEONRELEASE;

        LPSTORAGE pStg;
        if (OK(hr = StgCreateDocfile(NULL, grfMode, 0L, &pStg)))
        {
            hr = InitNew(pStg);

            // IPersistStorage::InitNew will hold on to the pStg
            pStg->Release();
        }
        return hr;
    }

    LPSTORAGE pStg;
    if (OK(hr = StgOpenStorage(lpstrFile, NULL, grfMode, NULL, 0L, &pStg)))
    {
        hr = Load(pStg);

        // IPersistStorage::Load will hold on to the pStg
        pStg->Release();
    }
    //REVIEW:  Is the first SetMoniker happening correctly?

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::Save, public
//
//  Synopsis:   Method of IPersistFile interface
//
//  Notes:      If a file is specified then this creates a docfile and
//              uses IPersistStorage::Save to complete the operation.
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::Save(LPCOLESTR lpstrFile, BOOL fRemember)
{
    // if lpstrFile is NULL that means that we should save from where we
    // loaded.  Otherwise create a docfile with the specified name
    HRESULT hr = NOERROR;
    LPSTORAGE pStg;

    if (lpstrFile == NULL)
        (pStg = _pStg)->AddRef();
    else
        hr = StgCreateDocfile(lpstrFile, STGM_DFRALL|STGM_CREATE, 0L, &pStg);

    if (OK(hr))
    {
        hr = OleSave((LPPERSISTSTORAGE)this, pStg, pStg == _pStg);

        if (OK(hr))
        {
            // if we are to remember this storage then release our old
            // storage and hold on to the new.
            // Otherwise, wrap up a storage save by the usual SaveCompleted.
            if (lpstrFile != NULL && fRemember)
            {
                // release our previous storage or stream
                // and hold on to our new
                HandsOffStorage();
                ((LPPERSISTSTORAGE)this)->SaveCompleted(pStg);
            }
            else
            {
                // If we did a storage save and we are not switching to a new
                // storage then we complete the transaction with a SaveCompleted.
                ((LPPERSISTSTORAGE)this)->SaveCompleted(NULL);
            }
        }

        // Release the storage.  If we are supposed to hold on
        // to it then we have already add-ref'd it.
        pStg->Release();
    }

    // if we have renamed then
    if (lpstrFile != NULL)
    {
        // TBD: Send On_Renamed advise?
        //
        // inform our object of its new moniker
        //
        LPMONIKER pmk;
        if (OK(CreateFileMoniker((LPWSTR)lpstrFile, &pmk)))
        {
            _pCtrl->SetMoniker(OLEWHICHMK_OBJFULL, pmk);
            pmk->Release();
        }
    }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::SaveCompleted, public
//
//  Synopsis:   Method of IPersistFile interface
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::SaveCompleted(LPCOLESTR lpstrFile)
{
    //REVIEW: should we launch advise in the case we are not fRemembering?
    if (_pCtrl != NULL)
        _pCtrl->OnSave();   // and notify any advises that we have saved

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SrvrDV::GetCurFile, public
//
//  Synopsis:   Method of IPersistFile interface
//
//---------------------------------------------------------------

STDMETHODIMP
SrvrDV::GetCurFile(LPOLESTR FAR * ppstrFile)
{
    if (ppstrFile == NULL)
    {
        DOUT(L"SrvrDV::GetCurFile E_INVALIDARG\r\n");
        return E_INVALIDARG;
    }

    *ppstrFile = 0; //set out params to NULL

    HRESULT hr;

    // if we don't currently have a file then return the default filename
    if (_pStg == NULL)
    {
        // the default filename is *.ext where ext is our docfile extension
        WCHAR szDefault[6];
        wsprintf(szDefault, L"*%s", _pClass->_szDocfileExt);
        hr = TaskAllocString(szDefault, ppstrFile);
    }
    else
    {
        // the caller will free the task-allocated file name
        STATSTG statstg;
        if (OK(hr = _pStg->Stat(&statstg, STATFLAG_DEFAULT)))
            *ppstrFile = statstg.pwcsName;
    }

    return hr;
}
