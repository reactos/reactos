//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       notftobj.cxx
//
//  Contents:   implements the notification object and its enumerator
//
//  Classes:
//
//  Functions:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>
#ifndef unix
#include "..\utils\cvar.hxx"
#else
#include "../utils/cvar.hxx"
#endif /* !unix */

class CEnumPropertyMap : public IEnumPropertyMap
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP Next(ULONG celt,STATPROPMAP *rgelt,ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumPropertyMap **ppenum);

    CEnumPropertyMap(CNotfctnObj *pCNotfctnObj) :  _CRefs(), _cPos(0)
    {
        _pCNotfctnObj = pCNotfctnObj;
        _pos = 0;
        if (_pCNotfctnObj)
        {
            _pCNotfctnObj->AddRef();
        }
    }

    ~CEnumPropertyMap()
    {
        if (_pCNotfctnObj)
        {
            _pCNotfctnObj->Release();
        }
    }

private:
    CRefCount       _CRefs;         // the total refcount of this object
    ULONG           _cPos;          // the current position
    POSITION        _pos;           // the internal position for the map
    CNotfctnObj    *_pCNotfctnObj;     //
};


//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::~CNotfctnObj
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CNotfctnObj::~CNotfctnObj()
{
    if (_pCPkg)
    {
        RELEASE(_pCPkg);
    }

    BuildMap();
    ClearMap();

    if (_pPropStream)
    {
        _pPropStream->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::Create
//
//  Synopsis:
//
//  Arguments:  [pNotificationType] --
//              [ppPropMap] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CNotfctnObj::Create(PNOTIFICATIONTYPE pNotificationType, CNotfctnObj **ppPropMap)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotfctnObj::Create\n", NULL));

    HRESULT hr = NOERROR;
    CNotfctnObj *pPropMap = 0;

    if (!pNotificationType || !ppPropMap)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CNotfctnObj *pPropMap = new CNotfctnObj(pNotificationType);

        if (pPropMap)
        {
            *ppPropMap = pPropMap;
        }
        else
        {
            *ppPropMap = 0;
            hr = E_OUTOFMEMORY;
        }
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotfctnObj::Create (hr:%lx\n", pPropMap,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotfctnObj::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_INotification))
    {
        *ppvObj = this;
        AddRef();
    }
    else if (riid == IID_IPropertyMap)
    {
        *ppvObj = (IPropertyMap *)this;
        AddRef();
    }
    else if (riid == IID_IPersistStream)
    {
        *ppvObj = (IPersistStream *)this;
        AddRef();
    }
    //
    // this is a hack to find out if it is our
    // own object
    else if (riid == IID_INotificationRunning)
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotfctnObj::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotfctnObj::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNotfctnObj::AddRef(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotfctnObj::AddRef\n", this));

    LONG lRet = ++_CRefs;

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotfctnObj::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotfctnObj::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CNotfctnObj::Release(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotfctnObj::Release\n", this));
    --_CRefs;
    LONG lRet = _CRefs;

    if (lRet == 1)
    {
        // hack: need to break refcount cycle
        //
        if (_pCPkg)
        {
            // bugbug: need to switch this on
            //NotfAssert((_pCPkg->GetNotification() == this));

            if (_pCPkg->GetRefCount() == 1)
            {
                CPackage *pCPkg = _pCPkg;
                _pCPkg =  0;
                pCPkg->SetNotification(NULL);
                RELEASE(pCPkg);
                lRet = 0;
            }
        }
    }
    else if (lRet == 0)
    {
        if (_pCPkg)
        {
            // bugbug: need to switch this on
            //NotfAssert((_pCPkg->GetRefCount() == 1));
            //NotfAssert((   (_pCPkg->GetNotification() == this)
            //             || (_pCPkg->GetNotification() == 0) ));
        }
    
        delete this;
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotfctnObj::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetNotificationType
//
//  Synopsis:   return the notification type
//
//  Arguments:  [pNotificationType] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:      INTERNAL
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::GetNotificationType(
                PNOTIFICATIONTYPE pNotificationType
                )
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetNotificationType\n", this));
    NotfAssert((pNotificationType));

    HRESULT hr = NOERROR;

    if (!IS_BAD_NULL_WRITEPTR(pNotificationType, sizeof(NOTIFICATIONTYPE)))
    {
        *pNotificationType = _NotificationType;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetNotificationType (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetNotificationCookie
//
//  Synopsis:   returns the notification cookie
//
//  Arguments:  [pNotificationCookie] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:      INTERNAL
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::GetNotificationCookie(
                PNOTIFICATIONCOOKIE pNotificationCookie
                )
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetNotificationCookie\n", this));
    NotfAssert((pNotificationCookie));

    HRESULT hr = NOERROR;

    if (!IS_BAD_NULL_WRITEPTR(pNotificationCookie, sizeof(NOTIFICATIONCOOKIE)))
    {
        *pNotificationCookie = _NotificationCookie;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetNotificationCookie (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetNotificationInfo
//
//  Synopsis:   returns notification associated info
//
//  Arguments:  [pNotificationType] --
//              [pNotificationCookie] --
//              [pNotificationFlags] --
//              [pDeliverMode] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::GetNotificationInfo(
                                 PNOTIFICATIONTYPE      pNotificationType,
                                 PNOTIFICATIONCOOKIE    pNotificationCookie,
                                 NOTIFICATIONFLAGS     *pNotificationFlags,
                                 DELIVERMODE           *pDeliverMode,
                                 DWORD                  dwReserved
                                 )
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetNotificationInfo\n", this));
    HRESULT hr = NOERROR;

    if (   dwReserved
        || (   !pNotificationType
            && !pNotificationCookie
            && !pNotificationFlags
            && !pDeliverMode)
        || IS_BAD_WRITEPTR(pNotificationType, sizeof(NOTIFICATIONTYPE))
        || IS_BAD_WRITEPTR(pNotificationCookie, sizeof(NOTIFICATIONCOOKIE))
        || IS_BAD_WRITEPTR(pNotificationFlags, sizeof(NOTIFICATIONFLAGS))
        || IS_BAD_WRITEPTR(pDeliverMode, sizeof(DELIVERMODE))
       )
    {
        NotfAssert((FALSE && "Invalid parameter passed to CNotfctnObj::GetNotificationInfo"));
        hr = E_INVALIDARG;
    }
    else
    {
        if (pNotificationCookie)
        {
            *pNotificationCookie = _NotificationCookie;
        }
        if (pNotificationType)
        {
            *pNotificationType = _NotificationType;
        }
        if (pNotificationFlags)
        {
            *pNotificationFlags = (NOTIFICATIONFLAGS)0;
        }
        if (pDeliverMode)
        {
            *pDeliverMode = (_pCPkg) ? _pCPkg->GetDeliverMode() : 0;
        }
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetNotificationInfo (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::Clone
//
//  Synopsis:   clones a notification object with a new type and a new cookie
//              all entries of the map are cloned
//
//  Arguments:  [rNotificationType] --
//              [ppNotification] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::Clone(
                REFNOTIFICATIONTYPE rNotificationType,
                LPNOTIFICATION     *ppNotification,
                DWORD               dwReserved
                )
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::Clone\n", this));
    HRESULT hr = E_FAIL;
    CLock lck(_mxs);

    do
    {
        if (   !ppNotification
            || dwReserved)
        {
            NotfAssert((FALSE && "Invalid parameter passed to CNotfctnObj::Clone"));
            hr = E_INVALIDARG;
            break;
        }
        CNotfctnObj *pCNotObj = 0;

        hr = Create((PNOTIFICATIONTYPE) &rNotificationType, &pCNotObj);
        if (hr != NOERROR)
        {
            break;
        }

        POSITION pos = 0;
        STATPROPMAP propmap;
        VariantInit(&propmap.variantValue);
        HRESULT hrOut = GetFirst(pos, &propmap);

        // not elements in map - done!
        if (hrOut != NOERROR)
        {
            *ppNotification = pCNotObj;
            hr = NOERROR;
            break;
        }

        // loop over all elements
        //
        do
        {
            if (hrOut == NOERROR)
            {
                NotfAssert((propmap.pstrName));

                hr = pCNotObj->Write(
                                 propmap.pstrName
                                ,propmap.variantValue
                                ,propmap.dwFlags
                                );
                VariantClear(&propmap.variantValue);
                delete [] propmap.pstrName;
            }

            if (    (hrOut == NOERROR)
                 && (hr == NOERROR)
                 && pos)
            {
                VariantInit(&propmap.variantValue);
                hrOut = GetNext(pos, &propmap);
            }
            else
            {
                // stop if pos is null
                hrOut = E_FAIL;
            }

        } while ((hr == NOERROR) && (hrOut == NOERROR));

        if (hr == NOERROR)
        {
            *ppNotification = pCNotObj;
        }
        else
        {
            *ppNotification = 0;
            // should clean up
            pCNotObj->Release();
        }

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::Clone (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::Write
//
//  Synopsis:   writes an entry in the map - an old value gets overwriten
//              deletes the entry if the value is VT_EMPTY or VT_NULL
//
//  Arguments:  [pstrName] --
//              [variantValue] --
//              [now] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::Write(
                LPCWSTR         pstrName,         // name
                VARIANT         variantValue,     // the variant value
                DWORD           dwFlags           // null for now
                )
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::Write\n", this));
    HRESULT hr = S_OK;
    CLock lck(_mxs);


    if (   !pstrName
        || (wcslen(pstrName) == 0))
    {
        // need valid name
        NotfAssert((FALSE && "Invalid parameter passed to CNotfctnObj::WriteLite"));
        hr = E_INVALIDARG;
    }
    else if (variantValue.vt == VT_DISPATCH)
    {
        // we do not handle objects yet
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        hr = BuildMap();
        if (SUCCEEDED(hr))
        {
            hr = WriteLite(pstrName, variantValue, dwFlags);
        }
    }

    //
    // set the dirty flag
    //
    if (hr == NOERROR)
    {
        _fDirty =  TRUE;

        if (  _fPersisted
            && _pCPkg
            && (_pCPkg->IsPersisted(c_pszRegKey) == S_OK)
           )
        {
            CPackage *pPackage = NULL;
            if (SUCCEEDED(g_pCSchedListAgent->FindPackage(&_NotificationCookie, &pPackage, LM_LOCALCOPY)))
            {
                if (_pCPkg != pPackage && pPackage->GetNotification())
                {
                    pPackage->GetNotification()->Write(pstrName, variantValue, dwFlags);
                }
                RELEASE(pPackage);
            }
            // persist the package first
            hr = _pCPkg->SaveToPersist(c_pszRegKey,0, PF_NOTIFICATIONOBJECT_ONLY);

            GetGlobalNotfMgr()->SetLastUpdateDate(_pCPkg);
            if (_pCPkg->GetDeliverMode() & DM_DELIVER_DEFAULT_PROCESS)
            {
                PostSyncDefProcNotifications();
            }
        }
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::Write (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CNotfctnObj::WriteLite(LPCWSTR pstrName,         // name
                                     VARIANT variantValue,     // the variant value
                                     DWORD   dwFlags           // null for now
                                    )
{
    HRESULT hr = S_OK;
    CLock lck(_mxs);

    NotfAssert((_pPropStream == NULL));
    if (   (variantValue.vt == VT_EMPTY)
             || (variantValue.vt == VT_NULL)
            )
    {
        // delete the entry
        CKey ckey = pstrName;
        COleVariant *pXCVar;

        if (_Map.Lookup(ckey, (CObject *&)pXCVar) )
        {
            _Map.RemoveKey(ckey);
            _cElements--;
            delete pXCVar;
            hr = S_OK;
        }
        else
        {
            // entry was not in map
            hr = S_FALSE;
        }
    }
    else
    {
        COleVariant *pCVar = new COleVariant(variantValue);

        if (pCVar)
        {
            CKey ckey = pstrName;
            COleVariant *pXCVar;

            // check if exist - delete it and
            if (_Map.Lookup(ckey, (CObject *&)pXCVar) )
            {
                _Map.RemoveKey(ckey);
                _cElements--;
                pXCVar->Clear();
                delete pXCVar;
            }
            // write the new one
            {
                _Map.SetAt(ckey, (CObject *)pCVar);
                _cElements++;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::Read
//
//  Synopsis:   reads an entry from the map
//
//  Arguments:  [pstrName] --
//              [pVariantValue] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::Read(LPCWSTR  pstrName, VARIANT *pVariantValue)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::Read\n", this));
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    if (   !pstrName
        || !pVariantValue
        || (wcslen(pstrName) == 0))
    {
        // need valid name
        NotfAssert((FALSE && "Invalid parameter passed to CNotfctnObj::Read"));
        hr = E_INVALIDARG;
    }
    else
    {
        hr = BuildMap();

        if (SUCCEEDED(hr))
        {
            COleVariant *pCVar = 0;
            CKey ckey = pstrName;

            NotfAssert((_pPropStream == NULL));

            if (_Map.Lookup(ckey, (CObject *&)pCVar) )
            {
                // copy the variant - the caller is supposed to
                // call VariantClear
                VariantCopy(pVariantValue,pCVar);
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::Read (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetCount
//
//  Synopsis:   return # of elements in map
//
//  Arguments:  [pCount] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::GetCount(ULONG *pCount)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetCount\n", this));
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    NotfAssert((pCount));

    if (pCount)
    {
        if (!_pPropStream)
        {
            *pCount = _Map.GetCount();
        }
        else
        {
            LARGE_INTEGER li = {0, 0};
            
            hr = _pPropStream->Read(pCount, sizeof(ULONG), NULL);
            _pPropStream->Seek(li, STREAM_SEEK_SET, NULL);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetCount (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetEnumMAP
//
//  Synopsis:   returns a new enumerator
//
//  Arguments:  [ppEnumMap] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::GetEnumMAP(LPENUMPROPERTYMAP *ppEnumMap)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetEnumMAP\n", this));
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    if (!ppEnumMap)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppEnumMap = new CEnumPropertyMap(this);
        if (*ppEnumMap == NULL)
        {
             hr = E_OUTOFMEMORY;
        }
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetEnumMAP (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetFirst
//
//  Synopsis:   gets the first element of the map and the position
//
//  Arguments:  [pos] --
//              [pValue] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:      private methods
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::GetFirst(POSITION &pos, STATPROPMAP *pValue)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetFirst\n", this));
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    hr = BuildMap();
    if (SUCCEEDED(hr))
    {
        NotfAssert((_pPropStream == NULL));
        pos = _Map.GetStartPosition();

        if (pos)
        {
            COleVariant *pXCVar;
            LPCSTR pszStr = 0;
            CString cStr;
            _Map.GetNextAssoc(pos, cStr, (CObject *&) pXCVar);
            pszStr = cStr;

            if (pszStr)
            {
                pValue->dwFlags      = 0;
                pValue->pstrName     = DupA2W((LPSTR)pszStr);
                VariantCopy(&pValue->variantValue,pXCVar);
            }
            else
            {
                // nothing in list
                NotfAssert((!pos));
                // use an empty variant
                COleVariant CVar;
                pValue->dwFlags      = 0;
                pValue->pstrName     = 0;
                pValue->variantValue = CVar;
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    
    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetFirst (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetNext
//
//  Synopsis:   gets the next entry passed on position
//
//  Arguments:  [pos] --
//              [pValue] --
//
//  Returns:
//
//  History:    1-13-1997   JohannP (Johann Posch)   Created
//
//  Notes:      private methods
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::GetNext(POSITION &pos, STATPROPMAP *pValue)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetNext\n", this));
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    hr = BuildMap();
    if (SUCCEEDED(hr))
    {
        NotfAssert((_pPropStream == NULL));
        if (pos)
        {
            COleVariant *pXCVar;
            LPCSTR pszStr = 0;
            CString cStr;
            _Map.GetNextAssoc(pos, cStr, (CObject *&) pXCVar);
            pszStr = cStr;

            if (pszStr)
            {
                pValue->dwFlags      = 0;
                pValue->pstrName     = DupA2W((LPSTR)pszStr);
                VariantCopy(&pValue->variantValue,pXCVar);
            }
            else
            {
                NotfAssert((!pos));
                // use an empty variant
                COleVariant CVar;
                pValue->dwFlags      = 0;
                pValue->pstrName     = 0;
                pValue->variantValue = CVar;
            }
            if (!pos)
            {
                // was last element
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetNext (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumPropertyMap::QueryInterface
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
STDMETHODIMP CEnumPropertyMap::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumPropertyMap::QueryInterface\n", this));
    HRESULT hr = NOERROR;

    if (   (riid == IID_IUnknown)
        || (riid == IID_IEnumPropertyMap))
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumPropertyMap::QueryInterface\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CEnumPropertyMap::AddRef
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
STDMETHODIMP_(ULONG) CEnumPropertyMap::AddRef(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    NotfDebugOut((DEB_ENUM, "%p IN CEnumPropertyMap::AddRef(%ld)\n", this, _CRefs));

    LONG lRet = ++_CRefs;

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumPropertyMap::AddRef(%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CEnumPropertyMap::Release
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
STDMETHODIMP_(ULONG) CEnumPropertyMap::Release(void)
{
    GEN_VDATEPTRIN(this,ULONG,0L);
    NotfDebugOut((DEB_ENUM, "%p IN CEnumPropertyMap::Release(%ld)\n", this, _CRefs));

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumPropertyMap::Release(%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumPropertyMap::Next
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
STDMETHODIMP CEnumPropertyMap::Next(ULONG celt, STATPROPMAP * rgelt, ULONG * pceltFetched)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumPropertyMap::Next\n", this));
    HRESULT hr = NOERROR;
    ULONG cFound = 0;

    do
    {
        if (   !rgelt
            || (celt == 0)
            )
        {
            NotfAssert((FALSE && "Invalid parameter passed to CEnumPropertyMap::Next"));
            hr = E_INVALIDARG;
            break;
        }

        ULONG cEl = 0;

        _pCNotfctnObj->GetCount(&cEl);

        if (cEl <= _cPos)
        {
            // the end - no more elements to enumerate
            break;
        }
        // start out with fail
        HRESULT hrOut = E_FAIL;

        // check if at the begin
        if (_pos == 0)
        {
            NotfAssert((_cPos == 0));
            hrOut = _pCNotfctnObj->GetFirst(_pos, rgelt);
        }
        if (hrOut == NOERROR)
        {
            // set the position if the GetFirst succedded
            cFound++;
            _cPos++;
        }

        if (_pos)
        {
            NotfAssert((_cPos));
            for ( ; (cFound < celt) && (_pos != 0); cFound++, _cPos++)
            {
                hrOut = _pCNotfctnObj->GetNext(_pos, rgelt + cFound);
            }
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

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumPropertyMap::Next (hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumPropertyMap::Skip
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
STDMETHODIMP CEnumPropertyMap::Skip(ULONG celt)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumPropertyMap::Skip\n", this));
    HRESULT hr = E_NOTIMPL;

    // skip by looping over the elements which need to be skiped
    do
    {
        // fail for now!
        break;

        //STATPROPMAP statprop;

        ULONG cEl = 0;
        _pCNotfctnObj->GetCount(&cEl);

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

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumPropertyMap::Skip (hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumPropertyMap::Reset
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
STDMETHODIMP CEnumPropertyMap::Reset(void)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumPropertyMap::Reset\n", this));

    _pos = 0;
    _cPos = 0;

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumPropertyMap::Reset (hr:%lx)\n", this, S_OK));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEnumPropertyMap::Clone
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
STDMETHODIMP CEnumPropertyMap::Clone(IEnumPropertyMap ** ppenum)
{
    NotfDebugOut((DEB_ENUM, "%p IN CEnumPropertyMap::Clone\n", this));
    HRESULT hr = NOERROR;

    if (!IS_BAD_NULL_WRITEPTR(ppenum, sizeof(IEnumPropertyMap *)))
    {
        *ppenum = new CEnumPropertyMap(_pCNotfctnObj);
        if (*ppenum == NULL)
        {
             hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    NotfDebugOut((DEB_ENUM, "%p OUT CEnumPropertyMap::Clone (hr:%lx)\n", this, hr));
    return hr;
}

//
// IPersistStream methods
//
//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetClassID
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
STDMETHODIMP CNotfctnObj::GetClassID (CLSID *pClassID)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetClassID\n", this));
    NotfAssert((pClassID));
    HRESULT hr = NOERROR;

    if (!pClassID)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pClassID = CLSID_StdNotificationMgr;
    }

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetClassID (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::IsDirty
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::IsDirty()
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::IsDirty\n", this));
    HRESULT hr = NOERROR;

    hr =  (_fDirty) ? S_OK : S_FALSE;

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::IsDirty (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::Load
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
STDMETHODIMP CNotfctnObj::Load(IStream *pStm)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::Load\n", this));
    HRESULT hr = NOERROR;
    CLock lck(_mxs);

    do
    {
        if (!pStm)
        {
            hr = E_INVALIDARG;
            break;
        }

        // read the cookie
        hr = ReadFromStream(pStm, &_NotificationCookie, sizeof(NOTIFICATIONCOOKIE));
        BREAK_ONERROR(hr);

        // read the type
        hr = ReadFromStream(pStm, &_NotificationType, sizeof(NOTIFICATIONTYPE));
        BREAK_ONERROR(hr);

        PropStreamFromStream(pStm);

        //
        // remember this was an persisted object
        _fPersisted = TRUE;

        break;
    } while (TRUE);

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::Load (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::Save
//
//  Synopsis:
//
//  Arguments:  [pStm] --
//              [fClearDirty] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfctnObj::Save(IStream *pStm, BOOL fClearDirty)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::Save\n", this));
    HRESULT hr = E_FAIL;
    CLock lck(_mxs);

    NotfAssert((pStm));

    do
    {
        if (!pStm)
        {
            hr = E_INVALIDARG;
            break;
        }

        // save the notification cookie
        hr = WriteToStream(pStm, &_NotificationCookie, sizeof(NOTIFICATIONCOOKIE));
        BREAK_ONERROR(hr);

        // save the notification type
        hr = WriteToStream(pStm, &_NotificationType, sizeof(NOTIFICATIONTYPE));
        BREAK_ONERROR(hr);

//  Point of departure

        if (!_pPropStream)
        {
            ULONG cCount = _Map.GetCount();

            // save the # of elements in the map

            hr = WriteToStream(pStm, &cCount, sizeof(ULONG));
            BREAK_ONERROR(hr);

            POSITION pos = _Map.GetStartPosition();
            STATPROPMAP propmap;
            ULONG cb = 0;

            // loop over all items and save
            while (pos)
            {

                COleVariant *pXCVar = 0;
                LPCSTR pszStr = 0;
                CString cStr;
                _Map.GetNextAssoc(pos, cStr, (CObject *&) pXCVar);
                pszStr = cStr;

                if (!pXCVar)
                {
                    NotfAssert((!pos));
                    // done
                    break;
                }

                // should have  string
                NotfAssert((pszStr));

                SaveSTATPROPMAP prpSave;

                prpSave.cbSize = 0;
                prpSave.cbStrLen = strlen(pszStr);
                prpSave.dwFlags      = 0;
                prpSave.cbVarSizeExtra = 0;

                // save the property info
                hr = WriteToStream(pStm, &prpSave, sizeof(SaveSTATPROPMAP));
                BREAK_ONERROR(hr);

                // save the string
                hr = WriteToStream(pStm, (void *)pszStr, prpSave.cbStrLen);
                BREAK_ONERROR(hr);

                // now save the variant
                hr = pXCVar->Save(pStm);
                BREAK_ONERROR(hr);

                cb++;

            } // while (pos)

            NotfAssert((cCount == cb));
        }
        else
        {
            HGLOBAL hGlobal = NULL;
            STATSTG statStg;

            _pPropStream->Stat(&statStg, STATFLAG_NONAME);
            ULONG cb = statStg.cbSize.LowPart;
            
            hr = GetHGlobalFromStream(_pPropStream, &hGlobal);
            if (SUCCEEDED(hr))
            {
                void *pData = GlobalLock(hGlobal);
                if (pData)
                {
                    hr = WriteToStream(pStm, pData, cb);
                    GlobalUnlock(hGlobal);
                }
                else
                {
                    hr = E_UNEXPECTED;
                }
            }
        }

        if (fClearDirty)
        {
            _fDirty = FALSE;
        }

        //
        // remember that this object was once saved
        //
        _fPersisted = TRUE;

        break;
    } while (TRUE);
 
    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::Save (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotfctnObj::GetSizeMax
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
STDMETHODIMP CNotfctnObj::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    NotfDebugOut((DEB_NOTFOBJ, "%p _IN CNotfctnObj::GetSizeMax\n", this));
    HRESULT hr = E_NOTIMPL;

    NotfDebugOut((DEB_NOTFOBJ, "%p OUT CNotfctnObj::GetSizeMax (hr:%lx)\n",this, hr));
    return hr;
}


HRESULT CNotfctnObj::SyncTo(CNotfctnObj *pUpdatedNotfctnObj)
{
    HRESULT hr;
    CLock lck(_mxs);

    ClearMap();
    ReleasePropStream();
    pUpdatedNotfctnObj->BuildMap();

    CMapStringToOb& UpdatedMap = pUpdatedNotfctnObj->_Map;


    POSITION pos = UpdatedMap.GetStartPosition();
    while (pos)
    {
        COleVariant *pUpdatedVar;
        CString strUpdatedName;
        
        UpdatedMap.GetNextAssoc(pos, strUpdatedName, (CObject *&)pUpdatedVar);

        LPWSTR pwszName = WzA2WDynamic(strUpdatedName, NULL, 0, FALSE);
        if (pwszName)
        {
            WriteLite(pwszName, *pUpdatedVar, 0);
            delete pwszName;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }

    return hr;
}

void CNotfctnObj::ClearMap()
{
    POSITION  pos = _Map.GetStartPosition();

    while (pos)
    {
        COleVariant *pXCVar;
        LPCSTR pszStr = 0;
        CString cStr;
        _Map.GetNextAssoc(pos, cStr, (CObject *&) pXCVar);
        pszStr = cStr;

        if (pszStr)
        {
            _Map.RemoveKey(cStr);
            _cElements--;
            pXCVar->Clear();
            delete pXCVar;
        }
    }
    _cElements = 0;
}

STDMETHODIMP CNotfctnObj::BuildMap()
{
    HRESULT hr = S_OK;
    ULONG cCount = 0;
    ULONG i = 0;
    LPSTR pszStr = 0;
    CLock lck(_mxs);


    if (_pPropStream)
    {
        //  Prevent reentrancy problems
        IStream *pLocPropStream = _pPropStream;
        _pPropStream = NULL;

        ClearMap();
        
        // get the # of elements saved
        hr = ReadFromStream(pLocPropStream, &cCount, sizeof(ULONG));

        if (SUCCEEDED(hr))
        {

            // BUGUG: need to optimize the string allocation
            i = 0;
            while (i < cCount)
            {
                COleVariant CVar;
                SaveSTATPROPMAP prpSave = { 0 , 0 , 0, 0};

                NotfAssert((!pszStr));
                // read the property info
                hr = ReadFromStream(pLocPropStream, &prpSave, sizeof(SaveSTATPROPMAP));
                BREAK_ONERROR(hr);

                pszStr = new char[prpSave.cbStrLen +1];
                if (!pszStr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                // read the string
                hr = ReadFromStream(pLocPropStream, (void *)pszStr, prpSave.cbStrLen);
                BREAK_ONERROR(hr);

                *(pszStr+prpSave.cbStrLen) = 0;

                // now read the variant
                hr = CVar.Load(pLocPropStream);
                BREAK_ONERROR(hr);

                // add it to the map
                LPWSTR pwzStr = DupA2W((LPSTR)pszStr);
                if (!pwzStr)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                hr = WriteLite(
                               pwzStr             //propmap.pstrName
                               ,CVar               //propmap.variantValue
                               ,prpSave.dwFlags    //propmap.dwFlags
                              );

                CVar.Clear();

                delete [] pwzStr;

                BREAK_ONERROR(hr);

                if (pszStr)
                {
                    delete [] pszStr;
                }

                pszStr = NULL;

                i++;

            } // end for loop over all elements save
        }
        NotfAssert((i == cCount));

        if (pszStr)
        {
            delete [] pszStr;
        }

        pLocPropStream->Release();
    }

    return hr;
}

STDMETHODIMP CNotfctnObj::PropStreamFromStream(IStream *pStream)
{
    HRESULT hr = E_FAIL;
    STATSTG statStg;
    DWORD dwSize;
    LARGE_INTEGER liZero = {0, 0};
    ULARGE_INTEGER uliPos;
    CLock lck(_mxs);

    ReleasePropStream();

    //  Pretty, eh?
    pStream->Seek(liZero, STREAM_SEEK_CUR, &uliPos);
    pStream->Stat(&statStg, STATFLAG_NONAME);
    dwSize = (DWORD)(MAKEINT64(statStg.cbSize.LowPart, statStg.cbSize.HighPart) - 
                     MAKEINT64(uliPos.LowPart, uliPos.HighPart));

    if (dwSize)
    {
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwSize);

        if (hGlobal)
        {
            DWORD dwWritten;

            void *pData = GlobalLock(hGlobal);
            if (pData)
            {
                hr = pStream->Read(pData, dwSize, &dwWritten);
                GlobalUnlock(hGlobal);
            }
            else
            {
                hr = E_UNEXPECTED;
            }

            if (SUCCEEDED(hr))
            {
                hr = CreateStreamOnHGlobal(hGlobal, TRUE, &_pPropStream);
            }
            if (FAILED(hr))
            {
                _pPropStream = NULL;
                GlobalFree(hGlobal);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}


