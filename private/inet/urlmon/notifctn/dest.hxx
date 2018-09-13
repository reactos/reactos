//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       dest.hxx
//
//  Contents:   destination class
//
//  Classes:
//
//  Functions:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _DESTINATION_HXX_
#define _DESTINATION_HXX_

typedef struct _tagDESTINATIONDATA
{
    ULONG               cbSize;
    CLSID               NotificationDest;
    NOTFSINKMODE        NotfctnSinkMode;
    ULONG               cNotifications;
    NOTIFICATIONCOOKIE  RegisterCookie;
    DWORD               dwReserved;
    DWORD               dwThreadId;
    DWORD               dwProcessId;
    HWND                hWnd;
} DESTINATIONDATA;

//
// private notifcations sink mode
//
#define NM_ACCEPT_ALL  (NOTFSINKMODE) 0x10000000

class CDestination : public CObject, public CThreadId, public CLifePtr
{
public:
    operator CObject* ()
    {
        return (CObject *) this;
    }

    CDestination()
    {
    }

    CDestination(
                LPNOTIFICATIONSINK  pNotfctnSink,      // can be null - see mode
                LPDESTID            pDestID,
                NOTFSINKMODE        asMode,
                CPkgCookie         &rCookie,
                ULONG               cNotifications,
                PNOTIFICATIONTYPE   pNotificationIDs,
                DWORD               dwReserved
                )
                : CThreadId()
                , _RegisterCookie(rCookie)
    {
        _cbSize = sizeof(CDestination); // + (cNotifications * sizeof(NOTIFICATIONTYPE));
        _pNotfctnSink      = pNotfctnSink;
        if (_pNotfctnSink)
        {
            _pNotfctnSink->AddRef();
        }
        _asMode         = asMode;
        _dwReserved     = dwReserved;
        _cNotifications      = cNotifications;
        _pNotificationTypes  = pNotificationIDs;
        _dwProcessId = GetCurrentProcessId();
        _hWnd = GetThreadNotificationWnd();


        if (pDestID)
        {
            _NotificationDest = *pDestID;
        }
        else
        {
            _NotificationDest = CLSID_NULL;
        }
    }

    HRESULT InitDestination(
        LPNOTIFICATIONSINK  pNotfctnSink,      // can be null - see mode
        LPDESTID            pDestID,
        NOTFSINKMODE        asMode,
        CPkgCookie         &rCookie,
        ULONG               cNotifications,
        PNOTIFICATIONTYPE   pNotificationIDs,
        DWORD               dwReserved
        )
    {
        HRESULT hr = NOERROR;
        _cbSize = sizeof(CDestination); // + (cNotifications * sizeof(NOTIFICATIONTYPE));
        _RegisterCookie = rCookie;
        _pNotfctnSink      = pNotfctnSink;
        if (_pNotfctnSink)
        {
            _pNotfctnSink->AddRef();
        }
        _asMode         = asMode;
        _dwReserved     = dwReserved;
        _cNotifications      = cNotifications;
        _pNotificationTypes  = pNotificationIDs;

        if (pDestID)
        {
            _NotificationDest = *pDestID;
        }
        else
        {
            _NotificationDest = CLSID_NULL;
        }

        return hr;
    }

    ~CDestination()
    {
        if (_pNotfctnSink)
        {
            _pNotfctnSink->Release();
        }
        if (_pNotificationTypes)
        {
            delete _pNotificationTypes;
        }
    }

    BOOL IsDestMode(NOTFSINKMODE asIn)
    {
        return (BOOL) (_asMode & (asIn));
    }

    void SetDestMode(NOTFSINKMODE asIn)
    {
        _asMode |= (asIn);
    }

    LPDESTID  GetDestId()
    {
        return &_NotificationDest;
    }

    CPkgCookie GetRegisterCookie()
    {
        return _RegisterCookie;
    }

    void * _cdecl operator new(size_t size)
    {
        void * pBuffer;
        pBuffer = CoTaskMemAlloc(size);
        if (pBuffer)
        {
            memset(pBuffer,0, size);
        }
        return pBuffer;
    }

    void * _cdecl operator new(size_t size, ULONG cNotifications)
    {
        void * pBuffer;
        size += (cNotifications * sizeof(NOTIFICATIONTYPE));
        pBuffer = CoTaskMemAlloc(size);
        if (pBuffer)
        {
            memset(pBuffer,0, size);
        }
        return pBuffer;
    }

    const CDestination& operator = (const CDestination *pCDest)
    {
        memcpy(this,pCDest,pCDest->_cbSize);
        return *this;
    }
    const CDestination& operator = (const CDestination& rCDest)
    {
        memcpy(this,&rCDest,rCDest._cbSize);
        return *this;
    }

    ULONG GetNotfTypeCount()
    {
        return _cNotifications;
    }
    HRESULT LookupNotificationType(NOTIFICATIONTYPE &rNotfType)
    {
        HRESULT hr = E_FAIL;

        NotfAssert((_pNotificationTypes && _cNotifications));

        // need to check for all notificaitons

        for (ULONG i = 0; i < _cNotifications; i++)
        {
            if (*(_pNotificationTypes+i) == rNotfType)
            {
                // found match
                hr = S_OK;
                i = _cNotifications;
            }
        }
        return hr;
    }

    HRESULT GetAcceptor(INotificationSink *& prAcceptor)
    {
        HRESULT hr = S_OK;
        if (_pNotfctnSink)
        {
            _pNotfctnSink->AddRef();
            prAcceptor = _pNotfctnSink;
        }
        else
        {
            // cocreateinstance the object if necessary
            hr  = E_FAIL;
        }

        return hr;
    }

    BOOL GotNotificationSink()
    {
        return (BOOL) _pNotfctnSink;
    }
    

    HRESULT SetNotificationSink(INotificationSink *& pNotfctnSink)
    {
        HRESULT hr = NOERROR;

        NotfAssert((!_pNotfctnSink));
        _pNotfctnSink      = pNotfctnSink;
        if (_pNotfctnSink)
        {
            _pNotfctnSink->AddRef();
        }

        return hr;
    }

    PNOTIFICATIONTYPE  GetNotfTypes()
    {
        return _pNotificationTypes;
    }

    HRESULT GetDestinationData(DESTINATIONDATA *pDestData)
    {
        NotfAssert((pDestData));

        pDestData->cbSize           = sizeof(DESTINATIONDATA);
        pDestData->NotificationDest = _NotificationDest;
        pDestData->NotfctnSinkMode  = _asMode;
        pDestData->cNotifications   = _cNotifications;
        pDestData->RegisterCookie   = _RegisterCookie;
        pDestData->dwReserved       = _dwReserved;
        pDestData->dwThreadId       = GetThreadId();
        pDestData->dwProcessId      = _dwProcessId;
        pDestData->hWnd             = _hWnd;
        return NOERROR;
    }

    HRESULT SetDestinationData(DESTINATIONDATA *pDestData)
    {
        NotfAssert((pDestData));

        _NotificationDest = pDestData->NotificationDest ;
        _asMode           = pDestData->NotfctnSinkMode  ;
        _cNotifications   = pDestData->cNotifications   ;
        _RegisterCookie   = pDestData->RegisterCookie   ;
        _dwReserved       = pDestData->dwReserved       ;
        _hWnd             = pDestData->hWnd             ;
        _dwProcessId      = pDestData->dwProcessId      ;
        SetThreadId(pDestData->dwThreadId);

        return NOERROR;
    }
    HWND GetPort()
    {
        NotfAssert((_hWnd));
        return _hWnd;
    }


    // persiststream methods
    STDMETHODIMP GetClassID (CLSID *pClassID);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm,BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

        // persist helper
    HRESULT SaveToPersist(LPCSTR pszWhere, DWORD dwMode = 0);
    HRESULT RemovePersist(LPCSTR pszWhere, DWORD dwMode = 0);
    static HRESULT LoadFromPersist(LPCSTR pszWhere,
                                   LPSTR pszSubKey,
                                   DWORD dwMode,
                                   CDestination **ppCDest);

private:
    LPNOTIFICATIONSINK  _pNotfctnSink;      // can be null - see mode
    DESTID              _NotificationDest;
    NOTFSINKMODE        _asMode;
    ULONG               _cbSize;
    CPkgCookie          _RegisterCookie;
    DWORD               _dwReserved;
    DWORD               _dwProcessId;
    HWND                _hWnd;

    // last two components
    ULONG               _cNotifications;
    PNOTIFICATIONTYPE   _pNotificationTypes;
    NOTIFICATIONTYPE    _NotificationID[1];
    // not data beyond here!!
};
#endif // _DESTINATION_HXX_

