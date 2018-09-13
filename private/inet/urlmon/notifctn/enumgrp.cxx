#include <notiftn.h>

//+---------------------------------------------------------------------------
//
//  Method:     CEnumSchedGroup::Create
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
HRESULT CEnumSchedGroup::Create(CSchedListAgent *pListAgent,ENUM_FLAGS dwMode, CEnumSchedGroup **ppCEnum)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumSchedGroup::Create\n", NULL));
    HRESULT hr = NOERROR;
    NotfAssert((ppCEnum && pListAgent));

    if (   !ppCEnum
        || !pListAgent)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CEnumSchedGroup *pEGrp = 0;
        *ppCEnum = pEGrp = new  CEnumSchedGroup(pListAgent, dwMode);
        if (!*ppCEnum)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pEGrp->Reset();
/*            hr = CScheduleGroup::UnPersistScheduleGroups( 0, 0, 0, 
                                    pEGrp->_prgCSchedGroups, 
                                    pEGrp->_cElements, 
                                    &pEGrp->_cElementsFilled);
*/
        }
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumSchedGroup::Create (hr:%lx)\n", NULL, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumSchedGroup::QueryInterface
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
STDMETHODIMP CEnumSchedGroup::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumSchedGroup::QueryInterface\n", this));
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

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumSchedGroup::QueryInterface\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CEnumSchedGroup::AddRef
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
STDMETHODIMP_(ULONG) CEnumSchedGroup::AddRef(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    NotfDebugOut((DEB_ENUM, "%p IN CEnumSchedGroup::AddRef(%ld)\n", this, _CRefs));

    LONG lRet = ++_CRefs;

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumSchedGroup::AddRef(%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CEnumSchedGroup::Release
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
STDMETHODIMP_(ULONG) CEnumSchedGroup::Release(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    NotfDebugOut((DEB_ENUM, "%p IN CEnumSchedGroup::Release(%ld)\n", this, _CRefs));

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumSchedGroup::Release(%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumSchedGroup::Next
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
STDMETHODIMP CEnumSchedGroup::Next(ULONG celt, LPSCHEDULEGROUP *rgelt, ULONG * pceltFetched)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumSchedGroup::Next\n", this));
    HRESULT hr = NOERROR;
    ULONG cFound = 0;
    CPackage    *pCPackage = 0;

    do
    {
        if (   !rgelt
            || (celt == 0)
            )
        {
            NotfAssert((FALSE && "Invalid parameter passed to CEnumSchedGroup::Next"));
            hr = E_INVALIDARG;
            break;
        }

        if (_cPos >= _cElementsFilled)
        {
            break;
        }
        
        for (cFound = 0; cFound < celt && _cPos < _cElementsFilled && cFound < _cElementsFilled; _cPos++)
        {
            if (_prgCSchedGroups[_cPos] != 0)
            {
                NOTIFICATIONCOOKIE GroupCookie = _prgCSchedGroups[_cPos]->GetGroupCookie();

                _prgCSchedGroups[_cPos]->Release();
                _prgCSchedGroups[_cPos] = 0;
                hr = CScheduleGroup::LoadPersistedGroup(c_pszRegKeyScheduleGroup, &GroupCookie, 0, &_prgCSchedGroups[_cPos]);
                if (hr == NOERROR)
                {
                    _prgCSchedGroups[_cPos]->AddRef();
                    *(rgelt + cFound) = _prgCSchedGroups[_cPos];
                    cFound++;
                }
            }
        }

#if 0
        ULONG cEl = 0;

        _pListAgent->GetPackageCount(&cEl);

        if (cEl <= _cPos)
        {
            // the end - no more elements to enumerate
            break;
        }

        HRESULT hrOut = NOERROR;

        // check if at the begin
        if (_posCookie == COOKIE_NULL)
        {
            NotfAssert((_cPos == 0));

            hrOut = _pListAgent->FindFirstGroupPackage(
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

            CScheduleGroup *pCSchedGroup = 0;
            hrOut = CScheduleGroup::CreateForEnum(
                                     pCPackage
                                    ,&pCSchedGroup
                                    );

            RELEASE(pCPackage);
            pCPackage = 0;

            if (hrOut == NOERROR)
            {
                NotfAssert((pCSchedGroup));
                *(rgelt + cFound) = (IScheduleGroup *) pCSchedGroup;
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

        if (_posCookie != COOKIE_NULL)
        {
            NotfAssert((_cPos));
            pCPackage = 0;

            for ( ; (cFound < celt) && (_posCookie != COOKIE_NULL); cFound++, _cPos++)
            {

                hrOut = _pListAgent->FindNextGroupPackage(
                                     &_posCookie             //PNOTIFICATIONCOOKIE packageCookie,
                                    ,&pCPackage              //CPackage          **ppCPackage,
                                    ,0 //,_dwMode            //DWORD               dwMode
                                    );

                if (hrOut != NOERROR)
                {
                    // break out of the loop
                    _posCookie = COOKIE_NULL;
                    break;
                }
                NotfAssert((pCPackage));

                CScheduleGroup *pCSchedGroup = 0;
                hrOut = CScheduleGroup::CreateForEnum(
                                         pCPackage
                                        ,&pCSchedGroup
                                        );
                RELEASE(pCPackage);
                pCPackage = 0;
                
                if (hrOut != NOERROR)
                {
                    _posCookie = COOKIE_NULL;
                    break;
                }
                NotfAssert((pCSchedGroup));
                *(rgelt + cFound) = pCSchedGroup;

            } // end for loop
        }

        NotfAssert((_cPos <= cEl));
#endif // 0

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
    NotfAssert((pCPackage == 0));

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumSchedGroup::Next (hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumSchedGroup::Skip
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
STDMETHODIMP CEnumSchedGroup::Skip(ULONG celt)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumSchedGroup::Skip\n", this));
    HRESULT hr = E_NOTIMPL;

    // skip by looping over the elements which need to be skiped
    do
    {
        // fail for now!
        break;

        ULONG cEl = 0;
        _pListAgent->GetPackageCount(&cEl);

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

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumSchedGroup::Skip (hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumSchedGroup::Reset
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
STDMETHODIMP CEnumSchedGroup::Reset(void)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumSchedGroup::Reset\n", this));

    FreeSchedGroups();

    HRESULT hr = CScheduleGroup::UnPersistScheduleGroups(0, 0, 0, 
                                                         _prgCSchedGroups, 
                                                         _cElements, 
                                                         &_cElementsFilled);

    _posCookie = COOKIE_NULL;
    _cPos = 0;

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumSchedGroup::Reset (hr:%lx)\n", this, S_OK));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumSchedGroup::Clone
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
STDMETHODIMP CEnumSchedGroup::Clone(IEnumScheduleGroup ** ppenum)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumSchedGroup::Clone\n", this));
    HRESULT hr = NOERROR;

    if (ppenum)
    {
        *ppenum = new CEnumSchedGroup(_pListAgent, _dwMode);
        if (*ppenum == NULL)
        {
             hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumSchedGroup::Clone (hr:%lx)\n", this, hr));
    return hr;
}

