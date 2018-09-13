//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       courhlp.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-17-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

//+---------------------------------------------------------------------------
//
//  Method:     CMapCStrToCVal::AddVal
//
//  Synopsis:
//
//  Arguments:  [LPCWSTR] --
//              [ULONG] --
//              [cNames] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapCStrToCVal::AddVal(LPCWSTR pwzKey, CObject *pCVal)
{
    NotfDebugOut((DEB_CMAPX, "%p _IN CMapCStrToCVal::AddVal\n", this));
    HRESULT hr = NOERROR;

    NotfAssert((pCVal && pwzKey));
    CLock lck(_mxs);

    if (pCVal && pwzKey)
    {
        CNodeObject *pNode;
        CKey ckey = pwzKey;

        if (_Map.Lookup(ckey, (CObject *&)pNode) )
        {
            pNode->Add(pCVal);
        }
        else
        {
           pNode = new CNodeObject(pCVal);
           if (pNode)
           {
               _Map.SetAt(ckey, pNode);
               _cElements++;
           }
           else
           {
               hr = E_OUTOFMEMORY;
           }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_CMAPX, "%p OUT CMapCStrToCVal::AddVal (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMapCStrToCVal::RemoveVal
//
//  Synopsis:
//
//  Arguments:  [pCVal] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapCStrToCVal::RemoveVal(LPCWSTR pwzKey, CObject *pCVal)
{
    NotfDebugOut((DEB_CMAPX, "%p _IN CMapCStrToCVal::RemoveVal\n", this));
    HRESULT hr = NOERROR;

    CLock lck(_mxs);

    if (pCVal && pwzKey)
    {
        CNodeObject *pNode;
        CKey ckey = pwzKey;

        if (_Map.Lookup(ckey, (CObject *&)pNode) )
        {
            if (pNode->Remove(pCVal) == FALSE)
            {
                // node is empty remove it and delete the node
                if (_Map.RemoveKey(ckey))
                {
                    delete pNode;
                    _cElements--;
                }
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_CMAPX, "%p OUT CMapCStrToCVal::RemoveVal (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMapCStrToCVal::FindFirst
//
//  Synopsis:
//
//  Arguments:  [pwzKey] --
//              [ppCVal] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapCStrToCVal::FindFirst(LPCWSTR pwzKey, CObject *&prCVal)
{
    NotfDebugOut((DEB_CMAPX, "%p _IN CMapCStrToCVal::FindFirst\n", this));
    HRESULT hr = E_FAIL;

    CLock lck(_mxs);
    NotfAssert(( pwzKey ));
    prCVal = 0;

    if (_cElements)
    {
        CNodeObject *pNode;
        CKey ckey = pwzKey;

        if (   (_Map.Lookup(ckey, (CObject *&)pNode) )
            && (pNode->FindFirst(prCVal)) )
        {
            hr = NOERROR;
        }
    }

    NotfDebugOut((DEB_CMAPX, "%p OUT CMapCStrToCVal::FindFirst (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMapCStrToCVal::FindNext
//
//  Synopsis:
//
//  Arguments:  [pwzKey] --
//              [prCVal] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapCStrToCVal::FindNext(LPCWSTR pwzKey, CObject *&prCVal)
{
    NotfDebugOut((DEB_CMAPX, "%p _IN CMapCStrToCVal::FindNext\n", this));
    HRESULT hr = E_FAIL;

    CLock lck(_mxs);
    NotfAssert(( pwzKey ));
    prCVal = 0;

    if (_cElements)
    {
        CNodeObject *pNode;
        CKey ckey = pwzKey;

        if (   (_Map.Lookup(ckey, (CObject *&)pNode) )
            && (pNode->FindNext(prCVal)) )
        {
            hr = NOERROR;
        }
    }

    NotfDebugOut((DEB_CMAPX, "%p OUT CMapCStrToCVal::FindNext (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMapCookieToCVal::AddVal
//
//  Synopsis:
//
//  Arguments:  [CPkgCookie] --
//              [ULONG] --
//              [cNames] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapCookieToCVal::AddVal(CPkgCookie *rKey, CObject *pCVal)
{
    NotfDebugOut((DEB_CMAPX, "%p _IN CMapCookieToCVal::AddVal\n", this));
    HRESULT hr = NOERROR;

    NotfAssert((pCVal && rKey));
    CLock lck(_mxs);

    if (pCVal && rKey)
    {
        //DumpIID(*rKey);
        CNodeObject *pNode;

        if (_Map.Lookup((REFCLSID)*rKey, (CObject *&)pNode) )
        {
            pNode->Add(pCVal);
        }
        else
        {
           pNode = new CNodeObject(pCVal);
           if (pNode)
           {
               _Map.SetAt((REFCLSID)*rKey, pNode);
               _cElements++;
           }
           else
           {
               hr = E_OUTOFMEMORY;
           }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_CMAPX, "%p OUT CMapCookieToCVal::AddVal (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMapCookieToCVal::RemoveVal
//
//  Synopsis:
//
//  Arguments:  [pCVal] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapCookieToCVal::RemoveVal(CPkgCookie *rKey, CObject *pCVal)
{
    NotfDebugOut((DEB_CMAPX, "%p _IN CMapCookieToCVal::RemoveVal\n", this));
    HRESULT hr = NOERROR;

    CLock lck(_mxs);

    if (pCVal && rKey)
    {
        //DumpIID(*rKey);
        CNodeObject *pNode;

        if (_Map.Lookup((REFCLSID)*rKey, (CObject *&)pNode) )
        {
            if (pNode->Remove(pCVal) == FALSE)
            {
                // node is empty remove it and delete the node
                if (_Map.RemoveKey((REFCLSID)*rKey))
                {
                    delete pNode;
                    _cElements--;
                }
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_CMAPX, "%p OUT CMapCookieToCVal::RemoveVal (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMapCookieToCVal::FindFirst
//
//  Synopsis:
//
//  Arguments:  [rKey] --
//              [ppCVal] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapCookieToCVal::FindFirst(CPkgCookie *rKey, CObject *&prCVal)
{
    NotfDebugOut((DEB_CMAPX, "%p _IN CMapCookieToCVal::FindFirst\n", this));
    HRESULT hr = E_FAIL;

    CLock lck(_mxs);
    NotfAssert(( rKey ));

    if (_cElements)
    {
        //DumpIID(*rKey);
        CNodeObject *pNode;

        if (   (_Map.Lookup((REFCLSID)*rKey, (CObject *&)pNode) )
            && (pNode->FindFirst(prCVal)) )
        {
            hr = NOERROR;
        }
    }

    NotfDebugOut((DEB_CMAPX, "%p OUT CMapCookieToCVal::FindFirst (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMapCookieToCVal::FindNext
//
//  Synopsis:
//
//  Arguments:  [rKey] --
//              [prCVal] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapCookieToCVal::FindNext(CPkgCookie *rKey, CObject *&prCVal)
{
    NotfDebugOut((DEB_CMAPX, "%p _IN CMapCookieToCVal::FindNext\n", this));
    HRESULT hr = E_FAIL;

    CLock lck(_mxs);
    NotfAssert(( rKey ));

    if (_cElements)
    {
        //DumpIID(*rKey);
        CNodeObject *pNode;

        if (   (_Map.Lookup((REFCLSID)*rKey, (CObject *&)pNode) )
            && (pNode->FindNext(prCVal)) )
        {
            hr = NOERROR;
        }
    }

    NotfDebugOut((DEB_CMAPX, "%p OUT CMapCookieToCVal::FindNext (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:     DeletRegSetting
//
//  Synopsis:
//
//  Arguments:  [pszRoot] --
//              [LPSTR] --
//
//  Returns:
//
//  History:    1-19-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT DeletRegSetting(LPCSTR pszRoot, LPCSTR pszKey)
{
    //NotfDebugOut((DEB_MGR, "%p _IN CNotificationMgr::DeletRegSetting (pszRoot:%s)\n", this,pszRoot));
    HRESULT hr = E_FAIL;
    HKEY hKey = NULL;
    DWORD   dwDisposition;

    NotfAssert((pszRoot));

    if (!pszKey)
    {
        RegDeleteKey(HKEY_CURRENT_USER, pszRoot);
    }
    else if (RegCreateKeyEx(HKEY_CURRENT_USER,pszRoot,0,NULL,0,HKEY_READ_WRITE_ACCESS,
                        NULL,&hKey,&dwDisposition) == ERROR_SUCCESS)
    {
        if (RegDeleteKey(hKey, pszKey) == ERROR_SUCCESS)
        {
            hr = NOERROR;
        }

        RegCloseKey(hKey);
    }

    //NotfDebugOut((DEB_MGR, "%p OUT CNotificationMgr::DeletRegSetting (hr:%lx)\n",this, hr));
    return hr;
}




