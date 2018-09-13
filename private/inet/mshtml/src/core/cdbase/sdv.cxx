//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//   File:       sdv.cxx
//
//------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifdef _MAC
#ifndef X_MACCONTROLS_H_
#define X_MACCONTROLS_H_
#include "maccontrols.h"
#endif
#endif

//  BUGBUG make this string shorter!

OLECHAR szContents[] = OLESTR("contents");

#if !defined(WIN16) && !defined(WINCE)
/*
 * CreateDCFromTargetDev()
 *
 * Purpose:
 *
 * Parameters:
 *
 * Return Value:
 *    SCODE  -  S_OK if successful
 */
HDC
CreateDCFromTargetDev(DVTARGETDEVICE FAR* ptd)
{
    HDC hdc=NULL;
    LPDEVNAMES lpDevNames;
    LPDEVMODE lpDevMode;
    LPTSTR lpszDriverName;
    LPTSTR lpszDeviceName;
    LPTSTR lpszPortName;

    if (ptd == NULL)
    {
        hdc = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
        goto errReturn;
    }

    lpDevNames = (LPDEVNAMES) ptd; // offset for size field

    if (ptd->tdExtDevmodeOffset == 0)
        lpDevMode = NULL;
    else
        lpDevMode = (LPDEVMODE) ((LPTSTR)ptd + ptd->tdExtDevmodeOffset);

    lpszDriverName = (LPTSTR) lpDevNames + ptd->tdDriverNameOffset;
    lpszDeviceName = (LPTSTR) lpDevNames + ptd->tdDeviceNameOffset;
    lpszPortName   = (LPTSTR) lpDevNames + ptd->tdPortNameOffset;

    hdc = CreateDC(lpszDriverName, lpszDeviceName, lpszPortName, lpDevMode);

errReturn:
    return hdc;
}
#endif // !WIN16 && !WINCE


//+---------------------------------------------------------------
//
//  Member:     CServer::SendOnDataChange
//
//  Synopsis:   Send data change notification to advise sinks.
//
//---------------------------------------------------------------
void
CServer::SendOnDataChange(DWORD_PTR dwAdvf)
{
    HRESULT         hr;
    IDataObject *   pDO;

    if (_pDataAdviseHolder)
    {
        hr = PrivateQueryInterface(IID_IDataObject, (void **)&pDO);
        if (OK(hr))
        {
            _pDataAdviseHolder->SendOnDataChange(pDO, 0, (DWORD)dwAdvf);
            pDO->Release();
        }
    }

    _fDataChangePosted = FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     CServer::OnDataChange, public
//
//  Synopsis:   Raises data and view changed to all registered 
//              advises
//
//  Notes:      This function should be called whenever the native
//              data of the object is modified.
//
//---------------------------------------------------------------

void
CServer::OnDataChange(BOOL fInvalidateView )
{
    if(fInvalidateView)
        OnViewChange(DVASPECT_CONTENT);

    if (_pDataAdviseHolder && !_fDataChangePosted)
    {
        HRESULT hr = GWPostMethodCall(this, ONCALL_METHOD(CServer, SendOnDataChange, sendondatachange), 0, FALSE, "CServer::SendOnDataChange");
        if (!hr)
            _fDataChangePosted = TRUE;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CServer::OnPropertyChange
//
//  Synopsis:   Fires property change event, and then OnDataChange
//
//  Arguments:  [dispidProperty] -- PROPID of property that changed
//              [dwFlags]        -- Flags to inhibit behavior
//
//  Notes:      The [dwFlags] parameter has the following values that can be
//              OR'd together:
//
//              SERVERCHNG_NOPROPCHANGE -- Inhibits the OnChanged notification
//                 through the PropNotifySink.
//              SERVERCHNG_NOVIEWCHANGE -- Inhibits the OnViewChange notification.
//              SERVERCHNG_NODATACHANGE -- Inhibits the OnDataChange notification.
//
//----------------------------------------------------------------------------

HRESULT
CServer::OnPropertyChange(
        DISPID dispidProperty,
        DWORD  dwFlags)
{
    if (TestLock(SERVERLOCK_PROPNOTIFY) || _state < OS_LOADED)
        return S_OK;

    _lDirtyVersion = MAXLONG;

    if (!(dwFlags & SERVERCHNG_NOPROPCHANGE))
    {
        IGNORE_HR(FireOnChanged(dispidProperty));
    }

    if (!(dwFlags & SERVERCHNG_NODATACHANGE))
    {
        SendOnDataChange(0);
    }

    if (!(dwFlags & SERVERCHNG_NOVIEWCHANGE))
    {
        OnViewChange(DVASPECT_CONTENT);
    }

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CServer::GetMonikerDisplayName, public
//
//  Synopsis:   Returns the display name from the object's moniker
//
//  Notes:      The display name of the object is used in for dispensing
//              the Object Descriptor clipboard format.  The caller must
//              free the string returned using TaskFreeString.
//
//---------------------------------------------------------------

LPTSTR
CServer::GetMonikerDisplayName(DWORD dwAssign)
{
    //  Default dwAssign is OLEGETMONIKER_ONLYIFTHERE

    LPMONIKER pmk;
    LPTSTR    lpstrDisplayName = NULL;

    if (OK(GetMoniker(dwAssign, OLEWHICHMK_OBJFULL, &pmk)))
    {
        ::GetMonikerDisplayName(pmk, &lpstrDisplayName);
        pmk->Release();
    }

    return lpstrDisplayName;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetEMBEDDEDOBJECT, static
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the standard Embedded Object clipboard format
//
//  Arguments:  [pServer] -- pointer to a CServer object
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
CServer::GetEMBEDDEDOBJECT(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
    HRESULT     hr = E_FAIL;

#if !defined(WIN16) && !defined(WINCE)
    if (!fHere)
    {
        // fill in the pmedium structure
        pmedium->tymed = TYMED_ISTORAGE;
        pmedium->pUnkForRelease = NULL;

        hr = THR(StgCreateDocfile(
                NULL,
                STGM_DFRALL | STGM_CREATE | STGM_DELETEONRELEASE,
                0,
                &pmedium->pstg));
        if (hr)
            goto Error;
    }

    hr = THR(WriteClassStg(pmedium->pstg, *pServer->BaseDesc()->_pclsid));
    if (hr)
        goto Error;

    hr = THR(pServer->Save(pmedium->pstg, FALSE));
    if (hr)
        goto Error;

    hr = THR(pServer->SaveCompleted((IStorage *) NULL));
    if (!hr)
    {
        IGNORE_HR(pmedium->pstg->Commit(STGC_DEFAULT));
    }


Error:
    //  If we failed somehow and yet created a docfile, then we will
    //      release the docfile to delete it

    if (hr && !fHere)
        ClearInterface(&pmedium->pstg);
#endif // !WIN16 && !WINCE

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetMETAFILEPICT, static
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the standard Metafilepict clipboard format
//
//  Arguments:  [pServer] -- pointer to a CServer object
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
CServer::GetMETAFILEPICT(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
#if defined(WIN16) || defined(WINCE)
    RRETURN(E_FAIL);
#else
    HRESULT         hr      = S_OK;
    RECTL           rcl;
    HMETAFILE       hmf     = NULL;
    HDC             hdc;
    LPMETAFILEPICT  pPict;
#ifdef _MAC
    UINT        dwFlags = GMEM_SHARE | GMEM_MOVEABLE;
    HANDLE      hdl;
    HANDLE  *   phdl = &hdl;
#else
    UINT        dwFlags = GMEM_SHARE;
    HANDLE  *   phdl = &pmedium->hGlobal;
#endif



    if (!fHere)
    {
        //  Fill in the pmedium structure

        pmedium->tymed = TYMED_MFPICT;
        pmedium->pUnkForRelease = NULL;
        *phdl = GlobalAlloc(dwFlags, sizeof(METAFILEPICT));

        if (*phdl == NULL)
            goto MemoryError;
#ifdef _MAC
        if(!UnwrapHandle(*phdl,(Handle*)&pmedium->hGlobal))
        {
            goto MemoryError;
        }
#  if DBG == 1
        else
        {
            // the wlm HANDLE is no more...
            hdl = NULL;
        }
#  endif
#endif
    }

    rcl.left = rcl.top = 0;
    hr = THR(pServer->GetExtent(
            pformatetc->dwAspect,
            pformatetc->lindex,
            pformatetc->ptd,
            (SIZEL *)&rcl.right));
    if (hr)
        goto Error;

    hdc = CreateMetaFileA(NULL);
    if (!hdc)
        goto MemoryError;

    rcl.right = HPixFromHimetric(rcl.right);
    rcl.bottom = VPixFromHimetric(rcl.bottom);

    SetMapMode(hdc, MM_ANISOTROPIC);
    SetWindowOrgEx(hdc, 0, 0, NULL);
    SetWindowExtEx(hdc, rcl.right, rcl.bottom, NULL);

    hr = THR(pServer->Draw(pformatetc->dwAspect,
            pformatetc->lindex,
            NULL,
            pformatetc->ptd,
            NULL,
            hdc,
            &rcl,
            &rcl,
            NULL,
            0));

    hmf = CloseMetaFile(hdc);

    if (hmf == NULL)
        goto MemoryError;

    if (hr)
        goto Error;

#ifdef _MAC
    if(!WrapHandle((Handle)pmedium->hGlobal,phdl,FALSE,dwFlags))
    {
        goto MemoryError;
    }
#endif
    pPict = (LPMETAFILEPICT) GlobalLock(*phdl);
    if (pPict == NULL)
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // fill in the object descriptor

    pPict->mm   =  MM_ANISOTROPIC;
    pPict->hMF  =  hmf;
    //
    // The following two values MUST BE in HIMETRIC, as long as the mapping
    // mode is MM_ANISOTROPIC.
    //
    pPict->xExt = HimetricFromHPix(rcl.right);
    pPict->yExt = HimetricFromVPix(rcl.bottom);

    GlobalUnlock(*phdl);

Cleanup:
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    if (!fHere && *phdl)
        GlobalFree(*phdl);

    if (hmf)
        DeleteMetaFile(hmf);

    goto Cleanup;
#endif //WIN16 || WINCE
}


//+---------------------------------------------------------------
//
//  Member:     CServer::GetENHMETAFILE, static
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the enhanced metafile clipboard format
//
//  Arguments:  [pServer] -- pointer to a CServer object
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
CServer::GetENHMETAFILE(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
#if defined(WIN16) || defined(WINCE)
    RRETURN(E_FAIL);
#else
    HRESULT         hr      = S_OK;
    RECTL           rcl;
    HDC             hdc;
    HDC             hdcRef = CreateDCFromTargetDev(pformatetc->ptd);
    if (!hdcRef)
        goto MemoryError;

    pmedium->tymed = TYMED_ENHMF;
    pmedium->pUnkForRelease = NULL;
    pmedium->hEnhMetaFile = NULL;

    rcl.left = rcl.top = 0;
    hr = THR(pServer->GetExtent(
            pformatetc->dwAspect,
            pformatetc->lindex,
            pformatetc->ptd,
            (SIZEL *)&rcl.right));
    if (hr)
        goto Error;

    hdc = CreateEnhMetaFileA(
            hdcRef,
            NULL,
            (RECT *)&rcl,
            NULL);
    if (!hdc)
        goto MemoryError;

    rcl.right = MulDivQuick(rcl.right,
            GetDeviceCaps(hdcRef, HORZRES),
            100 * GetDeviceCaps(hdcRef, HORZSIZE));
    rcl.bottom = MulDivQuick(rcl.bottom,
            GetDeviceCaps(hdcRef, VERTRES),
            100 * GetDeviceCaps(hdcRef, VERTSIZE));

    hr = THR(pServer->Draw(pformatetc->dwAspect,
            pformatetc->lindex,
            NULL,
            pformatetc->ptd,
            NULL,
            hdc,
            &rcl,
            &rcl,
            NULL,
            0));

    pmedium->hEnhMetaFile = CloseEnhMetaFile(hdc);

    if (pmedium->hEnhMetaFile == NULL)
        goto MemoryError;

    if (hr)
        goto Error;

Cleanup:
    if (hdcRef)
        DeleteDC(hdcRef);
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    if (pmedium->hEnhMetaFile)
        DeleteEnhMetaFile(pmedium->hEnhMetaFile);

    goto Cleanup;
#endif // WIN16 || WINCE
}


//+---------------------------------------------------------------
//
//  Member:     CServer::GetOBJECTDESCRIPTOR, public
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the standard Object Descriptor clipboard format
//
//  Arguments:  [pServer] -- pointer to a CServer object
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
CServer::GetOBJECTDESCRIPTOR(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
    HRESULT             hr                  = S_OK;
    LPTSTR              lpstrDisplay        = NULL;
    size_t              cch;
    size_t              cbSize;
    size_t              cbUserTypeFull;
    size_t              cbDisplay;
    LPOBJECTDESCRIPTOR  pObjDesc;
    TCHAR               achUserTypeFull[MAX_USERTYPE_LEN + 1];
#ifdef _MAC
    UINT        dwFlags = GMEM_SHARE | GMEM_MOVEABLE;
    HANDLE      hdl;
    HANDLE  *   phdl = &hdl;
#else
    UINT        dwFlags = GMEM_SHARE;
    HANDLE  *   phdl = &pmedium->hGlobal;
#endif



    //  REVIEW This is not the best display name for the OBJECTDESCRIPTOR
    //    It would be more useful to see the Class name...

    lpstrDisplay = pServer->GetMonikerDisplayName(OLEGETMONIKER_ONLYIFTHERE);



    Verify(LoadString(
            GetResourceHInst(),
            IDS_USERTYPEFULL(pServer->BaseDesc()->_idrBase),
            achUserTypeFull,
            ARRAY_SIZE(achUserTypeFull)));

    // Compute the size of the descriptor.

    Assert(_tcsclen(achUserTypeFull));

    cbUserTypeFull = (_tcsclen(achUserTypeFull) + 1) * sizeof(TCHAR);

    cbDisplay = 0;
    if (lpstrDisplay != NULL)
    {
        cch = _tcsclen(lpstrDisplay);
        if (cch)
            cbDisplay = (cch + 1) * sizeof(TCHAR);
    }

    cbSize = sizeof(OBJECTDESCRIPTOR) + cbUserTypeFull + cbDisplay;

    if (!fHere)
    {
        // fill in the pmedium structure

        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->pUnkForRelease = NULL;
        *phdl = GlobalAlloc(dwFlags, cbSize);
        if (*phdl == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
#ifdef _MAC
        if(!UnwrapHandle(*phdl,(Handle*)&pmedium->hGlobal))
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
    }
    if(!WrapHandle( (Handle)pmedium->hGlobal, phdl,FALSE,dwFlags))
    {
        hr = E_HANDLE;
        goto Error;
#endif
    }

    pObjDesc = (LPOBJECTDESCRIPTOR) GlobalLock(*phdl);
    if (!pObjDesc)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    // fill in the object descriptor

    pObjDesc->cbSize = cbSize;
    pObjDesc->clsid = *pServer->BaseDesc()->_pclsid;
    pObjDesc->dwDrawAspect = DVASPECT_CONTENT;
    pObjDesc->dwStatus = pServer->ServerDesc()->_dwMiscStatus;

#ifdef _MAC
    pObjDesc->sizel.cx = HPixFromHimetric(pServer->_sizel.cx);
    pObjDesc->sizel.cy = VPixFromHimetric(pServer->_sizel.cy);
#else
    pObjDesc->sizel = pServer->_sizel;
#endif
    pObjDesc->pointl.y = pObjDesc->pointl.x = 0;

    pObjDesc->dwFullUserTypeName = sizeof(OBJECTDESCRIPTOR);
    memcpy(pObjDesc + 1, achUserTypeFull, cbUserTypeFull);

    if (lpstrDisplay == NULL)
    {
        pObjDesc->dwSrcOfCopy = 0;
    }
    else
    {
        pObjDesc->dwSrcOfCopy = sizeof(OBJECTDESCRIPTOR) + cbUserTypeFull;
        memcpy(
                ((BYTE *) pObjDesc) + pObjDesc->dwSrcOfCopy,
                lpstrDisplay,
                cbDisplay);
    }

    GlobalUnlock(*phdl);

Cleanup:
    TaskFreeString(lpstrDisplay);
    RRETURN(hr);

Error:
    if (!fHere && *phdl)
    {
        GlobalFree(*phdl);
        pmedium->hGlobal = NULL;
    }
    goto Cleanup;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetLINKSOURCE, public
//
//  Synopsis:   Implementation of IDataObject::GetData and GetDataHere
//              for the standard Link Source clipboard format
//
//  Arguments:  [pServer] -- pointer to a CServer object
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
CServer::GetLINKSOURCE(
        CServer * pServer,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
    HRESULT     hr;
    LPMONIKER   pmk = NULL;
    CLSID       clsid;

    if (!fHere)
    {
        // fill in the pmedium structure

        pmedium->tymed = TYMED_ISTREAM;
        pmedium->pUnkForRelease = NULL;
        hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pmedium->pstm));
        if (hr)
            goto Error;
    }

    hr = THR(pServer->GetMoniker(
            OLEGETMONIKER_ONLYIFTHERE,
            OLEWHICHMK_OBJFULL,
            &pmk));
    if (hr)
        goto Error;

    hr = THR(pmk->GetClassID(&clsid));
    if (hr)
        goto Error;

    hr = THR(WriteClassStm(pmedium->pstm, clsid));
    if (hr)
        goto Error;

    hr = THR(pmk->Save(pmedium->pstm, FALSE));
    if (hr)
        goto Error;

Cleanup:
    ReleaseInterface(pmk);

    RRETURN(hr);

Error:
    if (!fHere && pmedium->pstm)
        ClearInterface(&pmedium->pstm);

    goto Cleanup;
}


//+--------------------------------------------------------------
//
//  Member:     CServer::GetData, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method searches the server's Get format table
//              for a compatible format.  If one is found it calls
//              the corresponding Get function.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    HRESULT         hr;
    int             i;

    if (pformatetc == NULL || pmedium == NULL)
        RRETURN(E_INVALIDARG);

    i = FindCompatibleFormat(
            ServerDesc()->_pGetFmtTable,
            ServerDesc()->_cGetFmtTable,
            *pformatetc);

    if (i < 0)
    {
        hr = DV_E_FORMATETC;
    }
    else
    {
        LPDATAOBJECT    pDO;
        //
        // For the icon aspect, check the cache first.
        //
        if (pformatetc->dwAspect == DVASPECT_ICON && _pCache)
        {
            hr = THR(_pCache->QueryInterface(
                    IID_IDataObject,
                    (LPVOID *) &pDO));
            if (!hr)
            {
                hr = THR(pDO->GetData(pformatetc, pmedium));
                pDO->Release();

                if (!hr)
                    RRETURN(hr);
            }
        }
        hr = THR((*ServerDesc()->_pGetFuncs[i]) (this, pformatetc, pmedium, FALSE));
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetDataHere, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method searches the server's Get format table
//              for a compatible format.  If one is found it calls
//              the corresponding Get function.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    HRESULT         hr;
    int             i;

    i = FindCompatibleFormat(
            ServerDesc()->_pGetFmtTable,
            ServerDesc()->_cGetFmtTable,
            *pformatetc);

    if (i < 0)
    {
        hr = DV_E_FORMATETC;
    }
    else
    {
        LPDATAOBJECT    pDO;
        //
        // For the icon aspect, check the cache first.
        //
        if ((pformatetc->dwAspect == DVASPECT_ICON) && _pCache)
        {
            hr = THR(_pCache->QueryInterface(IID_IDataObject, (LPVOID *) &pDO));
            if (!hr)
            {
                hr = THR(pDO->GetDataHere(pformatetc, pmedium));
                pDO->Release();

                if (!hr)
                    RRETURN(hr);
            }
        }
        hr = THR((*ServerDesc()->_pGetFuncs[i]) (this, pformatetc, pmedium, TRUE));
    }

    RRETURN1(hr, S_FALSE);
}




//+---------------------------------------------------------------
//
//  Member:     CServer::QueryGetData, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method searches the server's Get format table
//              for a compatible format.  The return value indicates
//              whether or not a compatible format was found.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::QueryGetData(LPFORMATETC pformatetc)
{
    int     i;

    i = FindCompatibleFormat(
            ServerDesc()->_pGetFmtTable,
            ServerDesc()->_cGetFmtTable,
            *pformatetc);

    RRETURN((i >= 0) ? S_OK : DV_E_FORMATETC);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetCanonicalFormatEtc, public
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
CServer::GetCanonicalFormatEtc(
        LPFORMATETC pformatetc,
        LPFORMATETC pformatetcOut)
{
    return DATA_S_SAMEFORMATETC;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::SetData, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method searches the server's Set format table
//              for a compatible format.  If one is found it calls
//              the corresponding Set function.
//
//---------------------------------------------------------------
STDMETHODIMP
CServer::SetData(
        LPFORMATETC pformatetc,
        STGMEDIUM FAR *pmedium,
        BOOL fRelease)
{
    HRESULT hr;
    int     i;

    if (!pformatetc || !pmedium)
        RRETURN(E_INVALIDARG);

    i = FindCompatibleFormat(
            ServerDesc()->_pSetFmtTable,
            ServerDesc()->_cSetFmtTable,
            *pformatetc);

    if (i < 0)
    {
        hr = DV_E_FORMATETC;
    }
    else
    {
        hr = THR((*ServerDesc()->_pSetFuncs[i])(this, pformatetc, pmedium));
    }

    if (fRelease)
        ReleaseStgMedium(pmedium);

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::EnumFormatEtc, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method creates an enumerator over the Get or
//              Set format tables depending on the value of the
//              dwDirection argument.
//
//              BUGBUG -- This method is incorrect because it ends
//              up enumerating formatetc's with more than one
//              DVASPECT flag set in the dwAspect member at a time.
//              This is illegal.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::EnumFormatEtc(
        DWORD dwDirection,
        LPENUMFORMATETC FAR* ppenumFormatEtc)
{
    HRESULT     hr;

    if (!ppenumFormatEtc)
        RRETURN(E_INVALIDARG);

    *ppenumFormatEtc = NULL;            // set out params to NULL

    //  Create an enumerator over our static format table.

    switch (dwDirection)
    {
    case DATADIR_GET:
        hr = THR(CreateFORMATETCEnum(
                ServerDesc()->_pGetFmtTable,
                ServerDesc()->_cGetFmtTable,
                ppenumFormatEtc));
        break;

    case DATADIR_SET:
        hr = THR(CreateFORMATETCEnum(
                ServerDesc()->_pSetFmtTable,
                ServerDesc()->_cSetFmtTable,
                ppenumFormatEtc));
        break;

    default:
        hr = E_INVALIDARG;
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::DAdvise, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method uses the standard OLE data advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::DAdvise(
        FORMATETC FAR* pFormatetc,
        DWORD advf,
        LPADVISESINK pAdvSink,
        DWORD FAR* pdwConnection)
{
    HRESULT         hr;
    IDataObject *   pDO;
    int             i;

    if (pdwConnection == NULL)
        RRETURN(E_INVALIDARG);

    *pdwConnection = NULL;              // set out params to NULL

    //
    //  Make sure we support the requested format if
    //  no request for data is pending.
    //

    if ((advf & ADVF_NODATA) == 0)
    {
        i = FindCompatibleFormat(
                ServerDesc()->_pGetFmtTable,
                ServerDesc()->_cGetFmtTable,
                *pFormatetc);

        if (i < 0)
            RRETURN(DATA_E_FORMATETC);
    }

    if (_pDataAdviseHolder == NULL)
    {
        hr = THR(CreateDataAdviseHolder(&_pDataAdviseHolder));
        if (hr)
            goto Cleanup;
    }

    hr = PrivateQueryInterface(IID_IDataObject, (void **)&pDO);
    if (hr)
        goto Cleanup;

    hr = THR(_pDataAdviseHolder->Advise(
            pDO,
            pFormatetc,
            advf,
            pAdvSink,
            pdwConnection));

    pDO->Release();

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::DUnadvise, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method uses the standard OLE data advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::DUnadvise(DWORD dwConnection)
{
    HRESULT     hr;

    if (!_pDataAdviseHolder)
        RRETURN(OLE_E_NOCONNECTION);

    hr = THR(_pDataAdviseHolder->Unadvise(dwConnection));

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::EnumDAdvise, public
//
//  Synopsis:   Method of IDataObject interface
//
//  Notes:      This method uses the standard OLE data advise holder.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::EnumDAdvise (LPENUMSTATDATA FAR* ppenumAdvise)
{
    HRESULT     hr;

    if (!ppenumAdvise)
        RRETURN(E_INVALIDARG);

    *ppenumAdvise = NULL;               // set out params to NULL

    if (_pDataAdviseHolder == NULL)
    {
        hr = S_OK;
    }
    else
    {
        hr = THR(_pDataAdviseHolder->EnumAdvise(ppenumAdvise));
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::Draw, IViewObject
//
//  Synopsis:   Render object to the DC.
//              Derived classes should override this method to implemnt
//              rendering for DVASPECT_CONTENT.
//
//  Arguments:  Per IViewObjectDraw.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Draw(
        DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hdcTargetDev,
        HDC hdcDraw,
        LPCRECTL prclDraw,
        LPCRECTL prcWBounds,
        BOOL (CALLBACK * pfnContinue) (ULONG_PTR),
        ULONG_PTR dwContinue)
{
    HRESULT     hr;
    CDrawInfo   DI;
    RECT        rc;
    POINT       ptOrg = { 0 };
    SIZE        sizeExt;
    BOOL        fDeleteTargetDev = FALSE;
    
    Assert((prclDraw || _state >= OS_INPLACE) &&
        "violation of prcl==NULL contract");

    Assert(hdcDraw);

    if (dwDrawAspect ==  DVASPECT_ICON)
    {
        hr = E_FAIL;

        //
        //  See if the cache has our icon first before we draw it.
        //
        if (_pViewObjectCache)
        {
            hr = THR(_pViewObjectCache->Draw(
                    dwDrawAspect,
                    lindex,
                    pvAspect,
                    ptd,
                    hdcTargetDev,
                    hdcDraw,
                    prclDraw,
                    prcWBounds,
                    pfnContinue,
                    dwContinue));
        }

// WINCEREVIEW - no support for metafiles or different mapping modes in CE
#ifndef WINCE
        if (hr)
        {
            //
            // Get the 'standard' icon
            //
            HGLOBAL hMetaPict = OleGetIconOfClass(*BaseDesc()->_pclsid,
                                                  NULL, TRUE);

            LPMETAFILEPICT pMF = (LPMETAFILEPICT)GlobalLock(hMetaPict);

            POINT ptOrg;
            SIZE  sizeExt;

            SetMapMode(hdcDraw, pMF->mm);
            SetViewportOrgEx(hdcDraw, prclDraw->left, prclDraw->top, &ptOrg);
            SetViewportExtEx(hdcDraw,
                             prclDraw->right - prclDraw->left,
                             prclDraw->bottom - prclDraw->top,
                             &sizeExt);

            PlayMetaFile(hdcDraw, pMF->hMF);

            SetViewportOrgEx(hdcDraw, ptOrg.x, ptOrg.y, (POINT *)NULL);
            SetViewportExtEx(hdcDraw, sizeExt.cx, sizeExt.cy, (SIZE *)NULL);

            GlobalUnlock(hMetaPict);

            DeleteMetaFile(pMF->hMF);
        }
#endif // WINCE

        RRETURN(hr);
    }

    if (prclDraw &&
        ( (_sizel.cx == 0 && prclDraw->right - prclDraw->left != 0) ||
          (_sizel.cy == 0 && prclDraw->bottom - prclDraw->top != 0) ) )
    {
        Assert(0 && "Host error: Infinite scale factor.");
        RRETURN(E_FAIL);
    }

    // Copy rectangle because we scribble on it.

    rc = *(prclDraw ? (RECT*)prclDraw : (RECT*)&_pInPlace->_rcPos);

    // Save the DC always

    if (SaveDC(hdcDraw) == 0)
        RRETURN(GetLastWin32Error());

// WINCEREVIEW - no support for metafiles or different mapping modes in CE
//only mapping mode we know in MM_TEXT
#ifndef WINCE
    // Retrieve current window origin
    GetWindowOrgEx(hdcDraw, & ptOrg);
    
    // Ensure that there is a one to one mapping between logial units and
    // device units.

    // If we are drawing into a meta file, then make sure that x and y
    // are going in the right directions.

    if (GetDeviceCaps(hdcDraw, TECHNOLOGY) == DT_METAFILE)
    {
        BOOL    fSwapXExt = rc.left > rc.right;
        BOOL    fSwapYExt = rc.top > rc.bottom;

        if (fSwapXExt || fSwapYExt)
        {
            GetWindowExtEx(hdcDraw, &sizeExt);

            if (fSwapXExt)
            {
                ptOrg.x    = -ptOrg.x;
                sizeExt.cx = -sizeExt.cx;
                rc.left    = -rc.left;
                rc.right   = -rc.right;
            }

            if (fSwapYExt)
            {
                ptOrg.y    = -ptOrg.y;
                sizeExt.cy = -sizeExt.cy;
                rc.top     = -rc.top;
                rc.bottom  = -rc.bottom;
            }

            SetWindowOrgEx(hdcDraw, ptOrg.x, ptOrg.y, (POINT *)NULL);
            SetWindowExtEx(hdcDraw, sizeExt.cx, sizeExt.cy, (SIZE *)NULL);
        }
    }
    else
    {
        // Convert the rc to the device space, and set of the mapping so that
        // there is a one to one mapping to device.

        // BUGBUG: It may be better performing to place the dc in MM_TEXT
#ifdef WIN16
        RECTword temp = {rc.left, rc.top, rc.right, rc.bottom };
        LPtoDP(hdcDraw, (POINTword *) & temp, 2);
        CopyRect( &rc, &temp );
#else
        LPtoDP(hdcDraw, LPPOINT( & rc ), 2);
#endif

        GetWindowExtEx(hdcDraw, &sizeExt);
        SetViewportExtEx(hdcDraw, sizeExt.cx, sizeExt.cy, (SIZE *)NULL);
    }
#endif // !WINCE

#ifdef _MAC
	EnsureMacScrollbars(hdcDraw);
#endif

    // According to Doc. We should ignore hdcTargetDev if ptd is NULL.
    // since TextServices requires hdcTargetDev to be NULL if ptd is NULL
    // we better null it here
    //
    // NOTE(SujalP): Our code cannot survive with hdcTargetDev=NULL. It has
    // to reflect some physical device. Hence we make it reflect the ptd
    // passed in. If the ptd too is NULL then we make hdcTargetDev reflect
    // the screen (this is done inside CreateDCFromTargetDev).
    if (   NULL == ptd
        || NULL == hdcTargetDev
       )
    {
        hdcTargetDev = CreateDCFromTargetDev(ptd);
        if (!hdcTargetDev)
            RRETURN(GetLastWin32Error());
        fDeleteTargetDev = TRUE;
    }

    // Set the viewport origin to our starting point, and normalize the
    //  draw rectangle.
    ptOrg.x   += rc.left;
    ptOrg.y   += rc.top;
    rc.left    = rc.top = 0;
    rc.right  -= ptOrg.x;
    rc.bottom -= ptOrg.y;
    SetViewportOrgEx(hdcDraw, ptOrg.x, ptOrg.y, (POINT *)NULL);

    memset(&DI, 0, sizeof(DI));
    DI._dwDrawAspect = dwDrawAspect;
    DI._lindex = lindex;
    DI._pvAspect = pvAspect;
    DI._ptd = ptd;
    DI._hic = hdcTargetDev;
    DI._hdc = hdcDraw;
    DI._prcWBounds = prcWBounds;
    DI._dwContinue = dwContinue;
    DI._pfnContinue = pfnContinue;
    DI._fInplacePaint = prclDraw == NULL;

#ifdef  IE5_ZOOM

    SIZE    sizeT;

    sizeT.cx = GetDeviceCaps(DI._hic, LOGPIXELSX);
    sizeT.cy = GetDeviceCaps(DI._hic, LOGPIXELSY);

    ((CTransform *)&DI)->Init(&rc, _sizel, &sizeT);

#else   // !IE5_ZOOM

    ((CTransform *)&DI)->Init(&rc, _sizel);


    DI._sizeInch.cx = GetDeviceCaps(DI._hic, LOGPIXELSX);
    DI._sizeInch.cy = GetDeviceCaps(DI._hic, LOGPIXELSY);

#endif  // IE5_ZOOM

    GetPalette(DI._hdc, &DI._fHtPalette);

    // Delegate to the other Draw method, which is overridden by derived
    // classes.

    // Inhibit OnViewChange Calls from Invalidate, as this could potentially
    // cause Draw to be called back
    CLock ViewLock(this, SERVERLOCK_VIEWCHANGE);

    hr = Draw(&DI, &rc);

    RestoreDC(hdcDraw, -1);

    if (fDeleteTargetDev)
    {
        DeleteDC(hdcTargetDev);
    }
    
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::DrawHighContrastBackground
//
//  Note:       Draws background for controls
//
//----------------------------------------------------------------------------

void
CServer::DrawHighContrastBackground(HDC hdc, const RECT * prc, COLORREF crBack)
{
    COLORREF crOld;

    Assert(g_fHighContrastMode);

    crOld = ::SetBkColor(hdc, crBack);
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, prc, 0, 0, 0);
    ::SetBkColor(hdc, crOld);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::GetColorSet, public
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method returns S_FALSE indicating the server
//              does not support this functionality.  Server's that
//              wish to support it should override this method.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetColorSet(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        LPLOGPALETTE FAR* ppColorSet)
{
    if (!ppColorSet)
        RRETURN(E_INVALIDARG);

    *ppColorSet = (LPLOGPALETTE)CoTaskMemAlloc(sizeof(LOGPAL256));
    if (*ppColorSet == NULL)
        return E_OUTOFMEMORY;

    memcpy(*ppColorSet, &g_lpHalftone, sizeof(LOGPAL256));

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::GetPalette, public
//
//  Synopsis:   Returns the document palette
//
//  Notes:      Returns the document palette.  This implementation is
//              really lame and doesn't cache the result from the ambient.
//              it is really expected to be overridden by the derived
//              class.
//
//---------------------------------------------------------------

HPALETTE
CServer::GetPalette(HDC hdc, BOOL *pfHtPal)
{
    CVariant var;
    HPALETTE hpal = GetAmbientPalette();

    if (hpal == NULL)
        hpal = GetDefaultPalette();

    if (hdc && hpal)
    {
        SelectPalette(hdc, hpal, TRUE);
        RealizePalette(hdc);
    }

    if (pfHtPal)
        *pfHtPal = FALSE;
        
    return hpal;
}

//+---------------------------------------------------------------
//
//  Member:     CServer::Freeze, public
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method sets flag _fViewFrozen.
//
//              The derived class should pay attention to this flag
//              and not allow any modifications that would change
//              the current rendering.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Freeze(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DWORD FAR* pdwFreeze)
{
    if (pdwFreeze == NULL)
        RRETURN(E_INVALIDARG);

    *pdwFreeze = 0; //set out params to NULL

    _fViewFrozen = TRUE;
    return NOERROR;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::Unfreeze, public
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method clears the flag _fViewFrozen.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Unfreeze(DWORD dwFreeze)
{
    _fViewFrozen = FALSE;
    return NOERROR;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::SetAdvise, IViewObject
//
//  Synopsis:   Method of IViewObject interface
//
//  Notes:      This method implements an advise holder for the view
//              advise.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::SetAdvise(DWORD dwAspects, DWORD dwAdvf, LPADVISESINK pAdvSink)
{
    ClearInterface(&_pAdvSink);

    if (!pAdvSink)
        return S_OK;

    if ((dwAspects != DVASPECT_CONTENT) || (dwAdvf != 0))
    {
        _dwAspects = dwAspects;
        _dwAdvf = dwAdvf;
    }

    if (OK(pAdvSink->QueryInterface(IID_IAdviseSinkEx, (void **)&_pAdvSink)))
    {
        _fUseAdviseSinkEx = TRUE;
    }
    else
    {
        _fUseAdviseSinkEx = FALSE;
        _pAdvSink = pAdvSink;
        _pAdvSink->AddRef();
    }

    if (dwAdvf & ADVF_PRIMEFIRST)
        OnViewChange(dwAspects);

    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetAdvise, public
//
//  Synopsis:   Method of IViewObject interface
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetAdvise(
        DWORD FAR* pdwAspects,
        DWORD FAR* pdwAdvf,
        LPADVISESINK FAR* ppAdvSink)
{
    if (pdwAspects)
        *pdwAspects = _dwAspects;

    if (pdwAdvf)
        *pdwAdvf = _dwAdvf;

    if (ppAdvSink)
    {
        *ppAdvSink = _pAdvSink;

        // COMMFIXLAJOSF : 7694
        if (_pAdvSink)
            _pAdvSink->AddRef();
    }

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetExtent, public
//
//  Synopsis:   Method of IViewObject2
//
//  Arguments:  [dwDrawAspect] -- View aspect of interest
//              [lindex]       -- Always -1
//              [ptd]          -- Target device being used
//              [lpsizel]      -- Place to put object's size
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::GetExtent(
        DWORD dwDrawAspect,
        LONG lindex,
        DVTARGETDEVICE* ptd,
        LPSIZEL lpsizel)
{
    //
    // Until the cache is in place we'll just forward to IOleObject's
    // implementation of this method.
    //

    RRETURN(GetExtent(dwDrawAspect, lpsizel));
}

void
CServer::SendOnViewChange(DWORD_PTR dwAspects)
{
    if (_pAdvSink && (dwAspects & _dwAspects))
    {
        _pAdvSink->OnViewChange((DWORD)dwAspects, -1);
        if (_dwAdvf & ADVF_ONLYONCE)
            SetAdvise(NULL, 0, 0);
    }

    _fViewChangePosted = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::OnViewChange
//
//  Synopsis:   Sends an view change notification to registered sinks.
//
//  Arguments:  [dwAspect] -- Aspect of view that changed.
//
//  Returns:    HRESULT.
//
//  History:    4-06-94   adams   Created
//
//----------------------------------------------------------------------------

void
CServer::OnViewChange(DWORD dwAspects)
{
    if (dwAspects == DVASPECT_CONTENT)
    {
        if (_pAdvSink && !_fViewChangePosted && !TestLock(SERVERLOCK_VIEWCHANGE))
        {
            HRESULT hr = GWPostMethodCall(this, ONCALL_METHOD(CServer, SendOnViewChange, sendonviewchange), dwAspects, FALSE, "CServer::SendOnViewChange");
            if (!hr)
                _fViewChangePosted = TRUE;
        }
    }
    else
    {
        SendOnViewChange(dwAspects);
    }
}


//+---------------------------------------------------------------
//
//  Member:     CServer::LoadFromStream, protected
//
//  Synopsis:   Loads the object's persistent state from a stream
//
//  Arguments:  [pStrm] -- stream to load from
//
//  Returns:    Success iff persistent state was read
//
//  Notes:      This function is used in the implementation of
//              IPersistStreamInit::Load and IPersistFile::Load when
//              the file is not a docfile.
//              All objects should override this method.
//
//---------------------------------------------------------------

HRESULT
CServer::LoadFromStream(LPSTREAM pStrm)
{
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::SaveToStream, protected
//
//  Synopsis:   Saves the object's persistent state to a stream
//
//  Arguments:  [pStrm] -- stream to save to
//
//  Returns:    Success iff persistent state was written
//
//  Notes:      This function is used in the implementation of
//              IPersistStreamInit::Save and IPersistFile::Save when
//              the file is not a docfile.
//              All objects should override this method.
//
//---------------------------------------------------------------

HRESULT
CServer::SaveToStream(LPSTREAM pStrm)
{
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetStreamSizeMax, protected
//
//  Synopsis:   Returns the number of bytes required to serialize object
//
//  Notes:      This function is used in the implementation of
//              IPersistStreamInit::GetSizeMax.
//              All objects should override this method.
//
//---------------------------------------------------------------
DWORD
CServer::GetStreamSizeMax(void)
{
    return 0;
}


//+---------------------------------------------------------------
//
//  Member:     CServer::IsDirty, public
//
//  Synopsis:   Method of IPersistStreamInit/Storage/File interface
//
//  Notes:      This method uses the dirty flag, _fDirty.
//              Objects should not set the _fDirty flag directly
//              but instead call the OnDataChange method to set the
//              flag.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::IsDirty(void)
{
    return (_lDirtyVersion ? NOERROR : S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CServer::Load, public
//
//  Synopsis:   Method of IPersistStreamInit interface
//
//  Notes:      This function uses the LoadFromStream method and
//              transitions the object to the loaded state if the
//              load was successful.
//
//---------------------------------------------------------------
STDMETHODIMP
CServer::Load(LPSTREAM pStrm)
{
    RRETURN(THR(LoadFromStream(pStrm)));
}

//+---------------------------------------------------------------
//
//  Member:     CServer::Save, public
//
//  Synopsis:   Method of IPersistStreamInit interface
//
//  Notes:      This method uses the SaveToStream method and
//              clears the _fDirty flag as appropriate.
//              Containers that have nonserializeable embeddings can
//              override this method and return STG_E_CANTSAVE
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Save(LPSTREAM pStrm, BOOL fClearDirty)
{
    HRESULT hr;

    if (pStrm == NULL)
    {
        hr = S_FALSE;
        // (hackhack) clear dirty bit if asked
    }
    else if (_state == OS_PASSIVE)
    {
        RRETURN(E_UNEXPECTED);
    }
    else
    {
        hr = THR(SaveToStream(pStrm));
        if (hr)
            goto Cleanup;
    }
    
    if (fClearDirty)
        _lDirtyVersion = 0;

Cleanup:
    
    RRETURN1( hr, S_FALSE );
}



//+---------------------------------------------------------------
//
//  Member:     CServer::GetSizeMax
//
//  Synopsis:   Method of IPersistStreamInit interface
//
//  Notes:
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::GetSizeMax(ULARGE_INTEGER FAR * pcbSize)
{
    if (pcbSize == NULL)
        RRETURN(E_INVALIDARG);

    ULISet32(*pcbSize, GetStreamSizeMax());
    return NOERROR;
}



//+------------------------------------------------------------------------
//
//  Member:     CServer::InitNew
//
//  Synopsis:   Method of IPersistStreamInit interface
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CServer::InitNew( )
{
    HRESULT     hr = S_OK;

    // object can be loaded only once!

    if (_state != OS_PASSIVE)
        RRETURN(CO_E_ALREADYINITIALIZED);
    Assert(!_pCache);
    _fInitNewed = TRUE;
    _lDirtyVersion = MAXLONG;     // set dirty to true, as per OLE spec (frankman)

    _state = OS_LOADED;

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CServer::LoadFromStorage, protected
//
//  Synopsis:   Loads the object's persistent state from a storage
//
//  Arguments:  [pStg] -- storage to load from
//
//  Returns:    Success iff persistent state was read
//
//  Notes:      This function is used in the implementation of
//              IPersistStorage::Load and IPersistFile::Load when
//              the file is a docfile.
//
//              This method opens a stream, "CONTENTS", and uses
//              method LoadFromStream to complete the load.
//              Servers that do more sophisticated loading will want
//              to override this method.
//
//---------------------------------------------------------------

HRESULT
CServer::LoadFromStorage(LPSTORAGE pStg)
{
    HRESULT     hr;
    LPSTREAM    pStrm   = NULL;

    hr = THR(pStg->OpenStream(szContents, NULL, STGM_SRO, 0, &pStrm));
    if (hr)
        goto Cleanup;

    hr = THR(LoadFromStream(pStrm));

Cleanup:
    ReleaseInterface(pStrm);

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member: CServer::SaveToStorage
//
//  Synopsis:   Saves the object's persistent state to a storage
//
//  Arguments:  [pStg] -- storage to save to
//
//  Returns:    Success iff persistent state was written
//
//  Notes:      This function is used in the implementation of
//              IPersistStorage::Save and IPersistFile::Save when
//              the file is a docfile.
//
//              This method opens a stream, "CONTENTS", and uses
//              method SaveToStream to complete the save.
//              Servers that do more sophisticated saving will want
//              to override this method.
//
//---------------------------------------------------------------

HRESULT
CServer::SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad)
{
    HRESULT     hr;
    LPSTREAM    pStrm   = NULL;

    hr = THR(pStg->CreateStream(
            szContents,
            STGM_SALL|STGM_CREATE,
            0L,
            0L,
            &pStrm));
    if (hr)
        goto Cleanup;

    hr = THR(SaveToStream(pStrm));

Cleanup:
    ReleaseInterface(pStrm);

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::InitNew
//
//  Synopsis:   IPersistStorage Method
//
//  Notes:      This method transitions the object to loaded.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::InitNew(LPSTORAGE pStg)
{
    HRESULT     hr  = S_OK;

    if (pStg == NULL)
        RRETURN(E_INVALIDARG);

    // object can be loaded only once!

    if (_state != OS_PASSIVE)
        RRETURN(CO_E_ALREADYINITIALIZED);

    //  CONSIDER what's the cleanup behavior here?

    if (_pPStgCache)
    {
        hr = THR(_pPStgCache->InitNew(pStg));
        if (hr)
            goto Cleanup;
    }

    _pStg = pStg;
    pStg->AddRef();

    _fInitNewed = TRUE;
    _lDirtyVersion = MAXLONG;     // set dirty to true, as per OLE spec (frankman)

    _state = OS_LOADED;

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::Load
//
//  Synopsis:   IPersistStorage Method
//
//  Notes:      This method loads the object using LoadFromStorage and
//              then transitions the object to loaded.
//
//              A pointer to our storage is maintained in member variable
//              _pStg.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Load(LPSTORAGE  pStg)
{
    HRESULT         hr;
    IOleCache2 *    pCache2 = NULL;
    if (pStg == NULL)
        RRETURN(E_INVALIDARG);

    //  Object can be loaded only once!

    if (_state != OS_PASSIVE)
        RRETURN(CO_E_ALREADYINITIALIZED);

    //  Do the load and move to the loaded state

    hr = THR(LoadFromStorage(pStg));
    if (hr)
        goto Cleanup;

    _pStg = pStg;
    pStg->AddRef();

    _state = OS_LOADED;

       // Don't nned to save it until the doc changes
    _lDirtyVersion = 0;

#if 0
    //
    // We need to see if the cache has any saved data that should be
    // loaded.  The following QI forces the cache object to be created,
    // and then we attempt a load.  If any of this fails, then that's OK.
    // It just means we don't have a cache or there was no cache data to
    // load.
    //

    //  BUGBUG this QI may not force the cache to be loaded if
    //    this control is aggregated

    hr = THR(_pUnkOuter->QueryInterface(IID_IOleCache2,
                                        (LPVOID *) &pCache2));
    if (hr)
        goto Cleanup;

    //  CONSIDER what's our error recovery strategy here?

    Assert(_pCache && _pCache->_pDefPStg);

    hr = THR(_pCache->_pDefPStg->Load(pStg));
#endif

Cleanup:
    ReleaseInterface(pCache2);
    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::Save
//
//  Synopsis:   Method of IPersistStorage interface
//
//  Notes:      This method uses SaveToStorage to write the persistent
//              state.  It also writes the full user type string to the
//              storage as is required.
//
//---------------------------------------------------------------

STDMETHODIMP
CServer::Save(LPSTORAGE  pStg, BOOL fSameAsLoad)
{
    HRESULT     hr;
    CLIPFORMAT  clipfmt;
    TCHAR       achUserTypeFull[MAX_USERTYPE_LEN + 1];

    if (pStg == NULL)
        RRETURN(E_INVALIDARG);

    if (_state == OS_PASSIVE)
        RRETURN(E_UNEXPECTED);

    if (_fHandsOff || _fNoScribble)
    {
        TraceTag((tagError, "IPersistStorage::Save called in hands-off or "
                  "no-scribble mode!"));
        RRETURN(E_UNEXPECTED);
    }

    hr = THR(SaveToStorage(pStg, fSameAsLoad));
    if (hr)
        goto Cleanup;

    // Write the UserType string. We don't let this fail the operation.

    clipfmt = (ServerDesc()->_cGetFmtTable > 0) ?
                    ServerDesc()->_pGetFmtTable[0].cfFormat : 0;

    Verify(LoadString(
            GetResourceHInst(),
            IDS_USERTYPEFULL(BaseDesc()->_idrBase),
            achUserTypeFull,
            ARRAY_SIZE(achUserTypeFull)));

    WriteFmtUserTypeStg(pStg, clipfmt, achUserTypeFull);

    _fNoScribble = TRUE;
    _fSameAsLoad = fSameAsLoad;
    _lDirtyVersion = 0;

    //  CONSIDER what's our error recovery strategy here?

    if (_pPStgCache)
        hr = THR(_pPStgCache->Save(pStg, fSameAsLoad));

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::SaveCompleted
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
CServer::SaveCompleted(LPSTORAGE  pStg)
{
    HRESULT hr  = S_OK;

    if (_fHandsOff == TRUE && NULL == pStg)
    {
        //
        // We are in hands off mode and we get didn't get the expected storage.
        //
        TraceTagEx((tagCServer, TAG_NONEWLINE, "CServer::SaveCompleted:"));
        TraceTagEx((tagCServer, TAG_NONAME|TAG_NONEWLINE, "We are in hands off mode and "));
        TraceTagEx((tagCServer, TAG_NONAME, "we get didn't get a storage"));
        RRETURN(E_UNEXPECTED);
    }

    if (_fHandsOff == FALSE && _fNoScribble == FALSE)
    {
        //
        // SaveCompleted is being called in a "normal" state i.e.  without
        // HandsOffStorage() or Save() having been called.
        //
        RRETURN(E_UNEXPECTED);
    }

    if (pStg != NULL)
    {
        if (_pStg != NULL)
        {
            HandsOffStorage();
        }

        Assert(_pStg == NULL);

        _pStg = pStg;
        _pStg->AddRef();
    }

    if (pStg != NULL || _fSameAsLoad)
    {
        if (_fNoScribble)
        {
            _lDirtyVersion = 0;     // clear our dirty flag

            if (_pOleAdviseHolder)
                _pOleAdviseHolder->SendOnSave();
        }
    }

    if (_pPStgCache)
        hr = THR(_pPStgCache->SaveCompleted(pStg));

    _fSameAsLoad = FALSE;
    _fNoScribble = FALSE;       // we are out of NO-SCRIBBLE mode
    _fHandsOff   = FALSE;       // we are out of HANDS-OFF mode

    RRETURN(hr);
}



//+---------------------------------------------------------------
//
//  Member:     CServer::HandsOffStorage
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
CServer::HandsOffStorage(void)
{
    HRESULT     hr  = S_OK;

    ClearInterface(&_pStg);

    if (_pPStgCache)
        hr = THR(_pPStgCache->HandsOffStorage());

    _fHandsOff = TRUE;

    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//
//  Member:     LoadFromBag
//
//  Synopsis:   Load object state from a property bag
//
//-----------------------------------------------------------------------------

HRESULT
CServer::LoadFromBag(
    LPPROPERTYBAG   pBag,
    LPERRORLOG      pErrLog)
{
    RRETURN(S_OK);
}



//+----------------------------------------------------------------------------
//
//  Member:     SaveToBag
//
//  Synopsis:   Save object state to a property bag
//
//-----------------------------------------------------------------------------

HRESULT
CServer::SaveToBag(
    LPPROPERTYBAG   pBag,
    BOOL            fSaveAllProperties)
{
    RRETURN(S_OK);
}



//+----------------------------------------------------------------------------
//
//  Member:     CServer::Load
//
//  Synopsis:   Method of IPersistPropertyBag interface
//
//  Notes:      This method uses the LoadFromBag method
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CServer::Load(
    LPPROPERTYBAG   pBag,
    LPERRORLOG      pErrLog)
{
    HRESULT hr;

    if (pBag == NULL)
        RRETURN(E_INVALIDARG);

    // object can be loaded only once!
    if (_state != OS_PASSIVE)
        RRETURN(E_UNEXPECTED);

    hr = THR(LoadFromBag(pBag, pErrLog));
    if (hr)
        goto Cleanup;

    _state = OS_LOADED;

Cleanup:
    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//
//  Member:     CServer::Save
//
//  Synopsis:   Method of IPersistPropertyBag interface
//
//  Notes:      This method uses the SaveToBag method and
//              clears the _fDirty flag as appropriate.
//              Containers that have nonserializeable embeddings can
//              override this method and return STG_E_CANTSAVE
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CServer::Save(
    LPPROPERTYBAG   pBag,
    BOOL            fClearDirty,
    BOOL            fSaveAllProperties)
{
    HRESULT hr;

    if (pBag == NULL)
        RRETURN(E_INVALIDARG);

    if (_state == OS_PASSIVE)
        RRETURN(E_UNEXPECTED);

    hr = THR(SaveToBag(pBag, fSaveAllProperties));
    if (hr)
        goto Cleanup;

    if (fClearDirty)
        _lDirtyVersion = 0;

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetRect
//
//  Synopsis:   Method of the IViewObjectEx interface.
//
//----------------------------------------------------------------------------


STDMETHODIMP
CServer::GetRect(DWORD dwAspect, LPRECTL prcl)
{
    HRESULT hr              = OLE_E_BLANK;
    DWORD   dwViewStatus    = 0L;

    TraceTag((tagCServer, "CServer::GetRect"));

    switch (dwAspect)
    {
    case DVASPECT_CONTENT:
        hr = S_OK;
        break;

    case DVASPECT_TRANSPARENT:
        IGNORE_HR(GetViewStatus(&dwViewStatus));
        if (dwViewStatus & VIEWSTATUS_DVASPECTTRANSPARENT)
            hr = S_OK;
#if DBG == 1
        else
            Assert(0 && "CServer::GetRect -- DVASPECTTRANSPARENT not supported");
#endif
        break;

    case DVASPECT_OPAQUE:
        IGNORE_HR(GetViewStatus(&dwViewStatus));
        if (dwViewStatus & VIEWSTATUS_DVASPECTOPAQUE)
            hr = S_OK;
#if DBG == 1
        else
            Assert(0 && "CServer::GetRect -- DVASPECTOPAQUE not supported");
#endif
        break;
    }

    if (hr == S_OK)
    {
        SetRectl(prcl, 0, 0, _sizel.cx, _sizel.cy);
    }
#if DBG == 1
    else
        Assert(0 && "Unhandled value");
#endif

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetViewStatus
//
//  Synopsis:   Method of the IViewObjectEx interface.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::GetViewStatus(DWORD *pdwStatus)
{
    TraceTag((tagCServer, "CServer::GetViewStatus"));
    *pdwStatus = ServerDesc()->_dwViewStatus;
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::QueryHitPoint
//
//  Synopsis:   Method of the IViewObjectEx interface.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::QueryHitPoint(
        DWORD dwAspect,
        LPCRECT prcBounds,
        POINT ptLoc,
        LONG lCloseHint,
        DWORD *pHitResult)
{
    HRESULT hr;

    TraceTag((tagCServer, "CServer::QueryHitPoint"));

    switch (dwAspect)
    {
    case DVASPECT_CONTENT:
    {
        *pHitResult = (PtInRect(prcBounds, ptLoc)) ? 
                        HITRESULT_HIT : 
                        HITRESULT_OUTSIDE;
        hr = S_OK;
        break;
    }

    default:
        *pHitResult = 0;
        hr = E_NOTIMPL;
        break;
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CServer::QueryHitRect
//
//  Synopsis:   Method of the IViewObjectEx interface.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::QueryHitRect(DWORD dwAspect, LPCRECT pRectBounds,
                      LPCRECT prcLoc, LONG lCloseHint, DWORD *pHitResult)
{
    HRESULT hr;

    TraceTag((tagCServer, "CServer::QueryHitRect"));

    switch (dwAspect)
    {
    case DVASPECT_CONTENT:
    {
        *pHitResult = HITRESULT_HIT;
        hr = S_OK;
        break;
    }

    default:
        *pHitResult = 0;
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetNaturalExtent
//
//  Synopsis:   Method of the IViewObjectEx interface.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::GetNaturalExtent(
    DWORD dwAspect, LONG lindex, DVTARGETDEVICE * ptd,
    HDC hicTargetDev, DVEXTENTINFO * pExtentInfo, LPSIZEL psizel )
{
    TraceTag((tagCServer, "CServer::GetNaturalExtent"));
    // BUGBUG: Should this routine deal with aspects other than content?
    return E_NOTIMPL;
}


#ifdef _MAC
//+---------------------------------------------------------------------------
//
//  Member:     CServer::EnsureMacScrollbars
//
//  Synopsis:   Method of the IViewObjectEx interface.
//
//----------------------------------------------------------------------------

void CServer::EnsureMacScrollbars(HDC hdc)
{
	if ( !_hVertScroll )
		_hVertScroll = CreateMacScrollbar(hdc);
	if ( !_hHorzScroll )
		_hHorzScroll = CreateMacScrollbar(hdc);	
}
#endif






