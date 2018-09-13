#include <notiftn.h>

//+---------------------------------------------------------------------------
//
//  Method:     CEnumNotification::Create
//
//  Synopsis:
//
//  Arguments:  [ENUM_FLAGS] --
//              [dwMode] --
//              [ppCEnum] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CEnumNotification::Create(CListAgent *pListAgent,
                                  ENUM_FLAGS dwMode, 
                                  CEnumNotification **ppCEnum, 
                                  ENUMGROUP_MODE egMode,
                                  PNOTIFICATIONCOOKIE pGrpCookie
                                  )
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumNotification::Create\n", NULL));
    HRESULT hr = NOERROR;
    NotfAssert((ppCEnum && pListAgent));

    if (   !ppCEnum
        || !pListAgent)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //CSortPkgList *pCPkgList = pListAgent->GetPkgListAgent(dwMode);
                
        *ppCEnum = new  CEnumNotification(pListAgent, dwMode, egMode, pGrpCookie);
        if (!*ppCEnum)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumNotification::Create (hr:%lx)\n", NULL, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumNotification::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumNotification::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumNotification::QueryInterface\n", this));
    HRESULT hr = NOERROR;

    if (   (riid == IID_IUnknown)
        || (riid == IID_IEnumNotification))
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumNotification::QueryInterface\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CEnumNotification::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumNotification::AddRef(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    NotfDebugOut((DEB_ENUM, "%p IN CEnumNotification::AddRef(%ld)\n", this, _CRefs));

    LONG lRet = ++_CRefs;

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumNotification::AddRef(%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CEnumNotification::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumNotification::Release(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    NotfDebugOut((DEB_ENUM, "%p IN CEnumNotification::Release(%ld)\n", this, _CRefs));

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumNotification::Release(%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumNotification::Next
//
//  Synopsis:
//
//  Arguments:  [celt] --
//              [rgelt] --
//              [pceltFetched] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumNotification::Next(ULONG celt, NOTIFICATIONITEM * rgelt, ULONG * pceltFetched)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumNotification::Next\n", this));
    HRESULT hr = NOERROR;
    ULONG cFound = 0;
    CPackage    *pCPackage = 0;

    do
    {
        if (   !rgelt
            || (celt == 0)
            )
        {
            NotfAssert((FALSE && "Invalid parameter passed to CEnumNotification::Next"));
            hr = E_INVALIDARG;
            break;
        }

        ULONG cEl = 0;
        BOOL fFound = FALSE;

        //cEl = _pListAgent->GetCount();
        _pListAgent->GetPackageCount(&cEl);

        if (cEl <= _cPos)
        {
            // the end - no more elements to enumerate
            break;
        }
        //
        // validate cookie and position
        //
        NotfAssert(( (   ((_posCookie == COOKIE_NULL) && (_cPos == 0))
                       || ((_posCookie != COOKIE_NULL) && (_cPos != 0)) ) ));
        
        
        if (   (_posCookie == COOKIE_NULL)
            && (_cPos != 0))
        {
            // the end - no more elements to enumerate
            break;
        }


        HRESULT hrOut = NOERROR;

        // check if at the begin
        if (_posCookie == COOKIE_NULL)
        {
            NotfAssert((_cPos == 0));

            hrOut = _pListAgent->FindFirstPackage(
                                   &_posCookie         //PNOTIFICATIONCOOKIE packageCookie,
                                  ,&pCPackage          //CPackage          **ppCPackage,
                                  ,0                   //DWORD               dwMode
                                  );
            if (hrOut != NOERROR)
            {
                // break out of the loop
                _posCookie = COOKIE_NULL;
                break;
            }

            NotfAssert((pCPackage));

            if (_EnumFlags & EF_NOTIFICATION_INPROGRESS)
            {
                // check if the package is running
                if (   (pCPackage->GetNotificationState() & PF_RUNNING)
                    && ( !pCPackage->IsReport() ) )
                {
                    fFound = TRUE;
                }
            }
            else if (_EnumFlags & EF_NOTIFICATION_THROTTLED)
            {
                if (   !(pCPackage->GetNotificationState() & PF_RUNNING)
                    && !pCPackage->IsReport())
                {
                    fFound = TRUE;
                }
            }
            else if (_EnumFlags & EF_NOTIFICATION_SUSPENDED)
            {
                if (   (pCPackage->GetNotificationState() & (PF_SUSPENDED))
                    && (pCPackage->GetNotificationState() & (PF_RUNNING))
                    && !pCPackage->IsReport())
                {
                    fFound = TRUE;
                }
            }
            else if (_GroupMode == EG_GROUPITEMS) 
            {
                
                if (pCPackage->GetGroupCookie())
                {
                    fFound = (_grpCookie == *pCPackage->GetGroupCookie()) ? TRUE : FALSE;
                }
                else
                {
                    fFound = FALSE;
                }
            }
            else if (pCPackage->GetNotificationState() & PF_WAITING)
            {
                fFound = TRUE;
            }
            
            NotfAssert((pCPackage));

            
            if (fFound)
            {
                (rgelt + cFound)->cbSize = sizeof(NOTIFICATIONITEM);
                hrOut = pCPackage->GetNotificationItem(rgelt + cFound, _EnumFlags);
            
                if (hrOut == NOERROR)
                {
                    cFound++;
                    _cPos++;
                }
                else
                {
                    // stop again
                    _posCookie = COOKIE_NULL;
                    break;
                }
            }
            RELEASE(pCPackage);
            pCPackage = 0;
        }

        if (_posCookie != COOKIE_NULL)
        {
            //NotfAssert((_cPos));
            pCPackage = 0;

            for ( ; (cFound < celt) && (_posCookie != COOKIE_NULL);)
            {
                fFound = FALSE;
                if (pCPackage)
                {
                    RELEASE(pCPackage);
                    pCPackage = 0;
                }
 

                hrOut = _pListAgent->FindNextPackage(
                                     &_posCookie             //PNOTIFICATIONCOOKIE packageCookie,
                                    ,&pCPackage              //CPackage          **ppCPackage,
                                    ,0                      //DWORD               dwMode
                                    );

                if (hrOut != NOERROR)
                {
                    // break out of the loop
                    _posCookie = COOKIE_NULL;
                    break;
                }
                NotfAssert((pCPackage));

                if (_EnumFlags & EF_NOTIFICATION_INPROGRESS)
                {
                    // check if the package is running
                    if (   (pCPackage->GetNotificationState() & PF_RUNNING)
                        && ( !pCPackage->IsReport() ))
                    {
                        fFound = TRUE;
                    }
                }
                else if (_EnumFlags & EF_NOTIFICATION_THROTTLED)
                {
                    if (   !(pCPackage->GetNotificationState() & PF_RUNNING)
                        && !pCPackage->IsReport())
                    {
                        fFound = TRUE;
                    }
                }
                else if (_EnumFlags & EF_NOTIFICATION_SUSPENDED)
                {
                    if (   (pCPackage->GetNotificationState() & (PF_SUSPENDED))
                        && (pCPackage->GetNotificationState() & (PF_RUNNING))
                        && !pCPackage->IsReport())
                    {
                        fFound = TRUE;
                    }
                }
                else if (_GroupMode == EG_GROUPITEMS) 
                {
                    
                    if (pCPackage->GetGroupCookie())
                    {
                        fFound = (_grpCookie == *pCPackage->GetGroupCookie()) ? TRUE : FALSE;
                    }
                    else
                    {
                        fFound = FALSE;
                    }
                }
                else if (pCPackage->GetNotificationState() & PF_WAITING)
                {
                    fFound = TRUE;
                }

                if (fFound)
                {
                    (rgelt + cFound)->cbSize = sizeof(NOTIFICATIONITEM);
                    hrOut = pCPackage->GetNotificationItem(rgelt + cFound, _EnumFlags);

                    if (hrOut != NOERROR)
                    {
                        _posCookie = COOKIE_NULL;
                        break;
                    }
                    cFound++;
                    _cPos++;
                }
                
                RELEASE(pCPackage);
                pCPackage = 0;
  
            } // end for loop
        }

        NotfAssert((_cPos <= cEl));

        break;
    } while (TRUE);

    if (hr == NOERROR)
    {
        if (pceltFetched)
        {
            *pceltFetched = cFound;
        }

        hr = ((cFound == celt) ? NOERROR : S_FALSE);
    }
    if (pCPackage)
    {
        RELEASE(pCPackage);
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumNotification::Next (hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumNotification::Skip
//
//  Synopsis:
//
//  Arguments:  [celt] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumNotification::Skip(ULONG celt)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumNotification::Skip\n", this));
    HRESULT hr = E_NOTIMPL;

    // skip by looping over the elements which need to be skiped
    do
    {
        // fail for now!
        break;

        //STATPROPMAP statprop;

        ULONG cEl = 0;
        _pListAgent->GetPackageCount(&cEl);
        //cEl = _pListAgent->GetCount();

        _cPos += celt;

        if (_cPos <= cEl)
        {
            hr = NOERROR;
        }
        else
        {
            // last position
            _cPos = cEl;
            hr = S_FALSE;
        }

        break;
    } while (TRUE);

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumNotification::Skip (hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumNotification::Reset
//
//  Synopsis:   resets the enumerator pointer to the begin
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumNotification::Reset(void)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumNotification::Reset\n", this));

    _posCookie = COOKIE_NULL;
    _cPos = 0;

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumNotification::Reset (hr:%lx)\n", this, S_OK));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumNotification::Clone
//
//  Synopsis:
//
//  Arguments:  [ppenum] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CEnumNotification::Clone(IEnumNotification ** ppenum)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumNotification::Clone\n", this));
    HRESULT hr = NOERROR;

    if (ppenum)
    {
        *ppenum = new CEnumNotification(_pListAgent, _EnumFlags);
        if (*ppenum == NULL)
        {
             hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumNotification::Clone (hr:%lx)\n", this, hr));
    return hr;
}

