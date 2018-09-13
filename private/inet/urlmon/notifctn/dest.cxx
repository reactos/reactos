//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       dest.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>


//+---------------------------------------------------------------------------
//
//  Method:     CDestination::GetClassID
//
//  Synopsis:
//
//  Arguments:  [pClassID] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CDestination::GetClassID (CLSID *pClassID)
{
    NotfDebugOut((DEB_DEST, "%p _IN CDestination::\n", this));
    HRESULT hr = NOERROR;

    *pClassID = CLSID_StdNotificationMgr;

    NotfDebugOut((DEB_DEST, "%p OUT CDestination:: (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDestination::IsDirty
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CDestination::IsDirty(void)
{
    NotfDebugOut((DEB_DEST, "%p _IN CDestination::IsDirty\n", this));
    HRESULT hr = S_FALSE;

    NotfDebugOut((DEB_DEST, "%p OUT CDestination::IsDirty (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDestination::Load
//
//  Synopsis:
//
//  Arguments:  [pStm] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CDestination::Load(IStream *pStm)
{
    NotfDebugOut((DEB_DEST, "%p _IN CDestination::Load\n", this));
    HRESULT hr = NOERROR;
    NotfAssert((pStm));

    PNOTIFICATIONTYPE pNotfTypes  = 0;
    DESTINATIONDATA       destdata;
    DWORD cbSaved;

    do
    {
        destdata.cbSize = sizeof(DESTINATIONDATA);

        hr =  pStm->Read(&destdata, sizeof(DESTINATIONDATA), &cbSaved);
        BREAK_ONERROR(hr);
        NotfAssert(( sizeof(DESTINATIONDATA) == cbSaved));

        NotfAssert(( destdata.cNotifications ));
        pNotfTypes = new NOTIFICATIONTYPE [destdata.cNotifications];

        if (!pNotfTypes)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        // read the notification types
        hr =  pStm->Read(pNotfTypes, destdata.cNotifications * sizeof(NOTIFICATIONTYPE)  , &cbSaved);
        BREAK_ONERROR(hr);
        NotfAssert(( (destdata.cNotifications * sizeof(NOTIFICATIONTYPE) ) == cbSaved));

        if ((destdata.cNotifications * sizeof(NOTIFICATIONTYPE) ) != cbSaved)
        {
            // stop - the size did not match; old or invalid entry
            hr = E_FAIL;
            break;
        }

        CPkgCookie ccookie = destdata.RegisterCookie;

        hr = InitDestination(
                             0
                             ,&destdata.NotificationDest
                             ,destdata.NotfctnSinkMode
                             ,ccookie
                             ,destdata.cNotifications
                             ,pNotfTypes
                             ,destdata.dwReserved
                             );
        BREAK_ONERROR(hr);

        BREAK_ONERROR(hr);
        hr = SetDestinationData(&destdata);

        pNotfTypes = 0;

        break;
    } while (TRUE);

    if (pNotfTypes)
    {
        delete pNotfTypes;
    }

    NotfDebugOut((DEB_DEST, "%p OUT CDestination::Load (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDestination::Save
//
//  Synopsis:
//
//  Arguments:  [BOOL] --
//              [fClearDirty] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CDestination::Save(IStream *pStm,BOOL fClearDirty)
{
    NotfDebugOut((DEB_DEST, "%p _IN CDestination::Save\n", this));
    NotfAssert((pStm));
    HRESULT hr = NOERROR;

    DESTINATIONDATA       destdata;
    DWORD cbSaved;

    do
    {
        destdata.cbSize = sizeof(DESTINATIONDATA);
        // get and write the notification item
        hr = GetDestinationData(&destdata);
        BREAK_ONERROR(hr);
        hr =  pStm->Write(&destdata, sizeof(DESTINATIONDATA), &cbSaved);
        BREAK_ONERROR(hr);
        NotfAssert(( sizeof(DESTINATIONDATA) == cbSaved));

        // get and write the notification types
        NotfAssert(( GetNotfTypes() ));
        hr =  pStm->Write(GetNotfTypes(), destdata.cNotifications * sizeof(NOTIFICATIONTYPE)  , &cbSaved);
        NotfAssert(( (destdata.cNotifications * sizeof(NOTIFICATIONTYPE) ) == cbSaved));

        break;
    } while (TRUE);

    NotfDebugOut((DEB_DEST, "%p OUT CDestination::Save (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDestination::GetSizeMax
//
//  Synopsis:
//
//  Arguments:  [pcbSize] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CDestination::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    NotfDebugOut((DEB_DEST, "%p _IN CDestination::GetSizeMax\n", this));
    HRESULT hr = NOERROR;

    pcbSize->LowPart += sizeof(DESTINATIONDATA) + (GetNotfTypeCount() * sizeof(NOTIFICATIONTYPE));
    pcbSize->HighPart = 0;

    NotfDebugOut((DEB_DEST, "%p OUT CDestination::GetSizeMax (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDestination::RemovePersist
//
//  Synopsis:   remove a persistance package form the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDestination::RemovePersist(LPCSTR pszWhere, DWORD dwMode)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CDestination::RemovePersist\n", NULL));
    HRESULT hr = NOERROR;
    NotfAssert((pszWhere));
    // remove the peristed package from the stream

    LPSTR       pszRegKey = 0;
    LPSTR       pszSubKey = 0;

    do
    {
        if (!pszWhere)
        {
            hr = E_INVALIDARG;
            break;
        }

        //pszSubKey = StringAFromCLSID( &(GetNotificationCookie()) );
        pszSubKey = StringAFromCLSID( _RegisterCookie );

        if (!pszSubKey)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        {
            long    lRes;
            DWORD   dwDisposition, dwType, dwSize;
            HKEY    hKey;
            char szKeyToDelete[1024];

            strcpy(szKeyToDelete, pszWhere);

            lRes = RegCreateKeyEx(HKEY_CURRENT_USER,szKeyToDelete,0,NULL,0,HKEY_READ_WRITE_ACCESS,
                            NULL,&hKey,&dwDisposition);

            if(lRes == ERROR_SUCCESS)
            {
                strcpy(szKeyToDelete, pszSubKey);
                lRes = RegDeleteValue(hKey, szKeyToDelete);
            }

            if (lRes != ERROR_SUCCESS)
            {
                hr = E_FAIL;
            }
            if (hKey)
            {
                RegCloseKey(hKey);
            }
        }

        if (pszRegKey)
        {
            delete pszRegKey;
        }
        if (pszSubKey)
        {
            delete pszSubKey;
        }

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_PACKAGE, "%p OUT CDestination::RemovePersist (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDestination::SaveToPersist
//
//  Synopsis:   saves a peristance package to the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDestination::SaveToPersist(LPCSTR pszWhere, DWORD dwMode)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CDestination::SaveToPersist\n", NULL));
    HRESULT hr = E_INVALIDARG;
    NotfAssert((pszWhere));
    // save the package

    CRegStream *pRegStm = 0;
    LPSTR       pszRegKey = 0;
    LPSTR       pszSubKey = 0;

    do
    {
        // BUBUG: need to save the clsid of the current process
        //CLSID clsid = CLSID_StdNotificationMgr;
        //pszRegKey = StringAFromCLSID( &clsid );
        pszSubKey = StringAFromCLSID(  _RegisterCookie );

        if (pszSubKey)
        {
            pRegStm = new CRegStream(HKEY_CURRENT_USER, pszWhere,pszSubKey, TRUE);
        }

        if (pszRegKey)
        {
            delete pszRegKey;
        }
        if (pszSubKey)
        {
            delete pszSubKey;
        }

        if (!pRegStm)
        {
           hr = E_OUTOFMEMORY;
           break;
        }

        IStream *pStm = 0;
        hr = pRegStm->GetStream(&pStm);
        if (hr != NOERROR)
        {

            delete pRegStm;
            break;
        }

        // save the item and the notification
        hr = Save(pStm, TRUE);
        BREAK_ONERROR(hr);

        if (pStm)
        {
            pStm->Release();
        }

        if (pRegStm)
        {
            pRegStm->SetDirty();
            delete pRegStm;
        }

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_PACKAGE, "%p OUT CDestination::SaveToPersist (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDestination::LoadFromPersist
//
//  Synopsis:   loads a persistence package form the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CDestination::LoadFromPersist(LPCSTR pszWhere, LPSTR pszSubKey, DWORD dwMode, CDestination **ppCDestination)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CDestination::LoadFromPersist\n", NULL));
    HRESULT hr = E_INVALIDARG;
    NotfAssert((pszWhere));
    // save the package

    CRegStream *pRegStm = 0;
    CDestination *pCDest = 0;

    do
    {
        if (   !pszWhere
            || !pszSubKey
            || !ppCDestination )

        {
            break;
        }

        pCDest = new CDestination();

        if (!pCDest)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        pRegStm = new CRegStream(HKEY_CURRENT_USER, pszWhere,pszSubKey, FALSE);

        if (!pRegStm)
        {
           hr = E_OUTOFMEMORY;
           break;
        }

        IStream *pStm = 0;
        hr = pRegStm->GetStream(&pStm);
        if (hr != NOERROR)
        {
            delete pRegStm;
            break;
        }

        // save the item and the notification
        hr = pCDest->Load(pStm);
        BREAK_ONERROR(hr);

        if (pStm)
        {
            pStm->Release();
        }

        if (pRegStm)
        {
            delete pRegStm;
        }

        *ppCDestination = pCDest;

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_PACKAGE, "%p OUT CDestination::LoadFromPersist (hr:%lx)\n",pCDest, hr));
    return hr;
}


