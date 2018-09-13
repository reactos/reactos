//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       notftobj.hxx
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
#ifndef _NOTFTOBJ_HXX_
#define _NOTFTOBJ_HXX_

class CNotfctnObj : public INotification, public IPersistStream
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // get the notification flags
    STDMETHODIMP GetNotificationInfo(
                                     PNOTIFICATIONTYPE   pNotificationType,
                                     PNOTIFICATIONCOOKIE pNotificationCookie,
                                     NOTIFICATIONFLAGS  *pNotificationFlags,
                                     DELIVERMODE        *pDeliverMode,
                                     DWORD               dwReserved
                                     );
    //clones a notification with a new type and a new cookie
    STDMETHODIMP Clone(
                    REFNOTIFICATIONTYPE rNotificationType,
                    LPNOTIFICATION     *ppNotification,
                    DWORD               dwReserved
                    );


    STDMETHODIMP Write(
                    LPCWSTR         pstrName,         // name
                    VARIANT         variantValue,     // the variant value
                    DWORD           dwFlags           // null for now
                    );

    STDMETHODIMP Read(
                    LPCWSTR         pstrName,       // name
                    VARIANT        *pVariantValue   // the variant value
                    );

    STDMETHODIMP GetCount(
                    ULONG          *pCount
                    );

    STDMETHODIMP GetEnumMAP(
                    LPENUMPROPERTYMAP    *ppEnumMap
                    );


    // persiststream methods
    STDMETHODIMP GetClassID (CLSID *pClassID);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm,BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);
    // private methods
    STDMETHODIMP GetFirst(POSITION &pos, STATPROPMAP *pValue);
    STDMETHODIMP GetNext(POSITION &pos, STATPROPMAP *pValue);

    // internal methods
    // return the notification type
    STDMETHODIMP GetNotificationType(
                    PNOTIFICATIONTYPE pNotificationType
                    );

    // get the notification cookie
    STDMETHODIMP GetNotificationCookie(
                    PNOTIFICATIONCOOKIE pNotificationCookie
                    );
    void SetNotificationCookie(
                    NOTIFICATIONCOOKIE &rNotificationCookie
                    )
    {
        _NotificationCookie = rNotificationCookie;
    }

    //
    // this needs to be removed and done via IStream
    //
    HRESULT SetCPackage(CPackage *pCPkg)
    {
        if (_pCPkg != pCPkg)
        {
            if (_pCPkg)
            {
                RELEASE(_pCPkg);
            }
            _pCPkg = pCPkg;
            if (_pCPkg)
            {
                ADDREF(_pCPkg);
            }
        }

        return NOERROR;
    }
    CPackage *GetCPackage()
    {
        return _pCPkg;
    }
    
    ULONG GetRefCount()
    {
        return _CRefs;
    }

    HRESULT SyncTo(CNotfctnObj *pUpdatedNotfctnObj);

    void ClearMap();
    
    STDMETHODIMP BuildMap();
    STDMETHODIMP PropStreamFromStream(IStream *pStream);

    STDMETHODIMP WriteLite(LPCWSTR pstrName,         // name
                           VARIANT variantValue,     // the variant value
                           DWORD   dwFlags           // null for now
                          );
public:
    ~CNotfctnObj();

    static HRESULT Create(PNOTIFICATIONTYPE pNotificationType, CNotfctnObj **ppPropMap);

private:
    CNotfctnObj(PNOTIFICATIONTYPE pNotfctnType) : _CRefs()
    {
        _NotificationType = *pNotfctnType;
        CPkgCookie ccookieNot;
        _NotificationCookie = ccookieNot;
        _pCPkg = 0;
        _fDirty = FALSE;
        _fPersisted = FALSE;
        _pPropStream = NULL;
    }

    void ReleasePropStream()
    {
        CLock lck(_mxs);
        if (_pPropStream)
        {
            _pPropStream->Release();
            _pPropStream = NULL;
        }
    }

private:
    CRefCount           _CRefs;         // the total refcount of this object
    BOOL                _fDirty;        // dirty again
    BOOL                _fPersisted;    // was once persisted

    CRefCount           _cElements;     // # of elements in map
    CMutexSem           _mxs;           // single access
    CMapStringToOb      _Map;

    NOTIFICATIONTYPE    _NotificationType;
    NOTIFICATIONCOOKIE  _NotificationCookie;

    IStream            *_pPropStream;

    // hack to fix the persist problem
    // the pointer is not addref'd
    CPackage            *_pCPkg;        // the pointer back to the package
};

typedef enum
{
     EG_NORMAL       = 0
    ,EG_GROUPITEMS   = 1
    ,EG_GROUPS       = 2

} ENUMGROUP_MODE;

class CEnumNotification : public IEnumNotification
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP Next(ULONG celt,NOTIFICATIONITEM *rgelt,ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumNotification **ppenum);

    static HRESULT Create(CListAgent *pListAgent,
                          ENUM_FLAGS dwMode,
                          CEnumNotification **ppCEnum,
                          ENUMGROUP_MODE egMode = EG_NORMAL,
                          PNOTIFICATIONCOOKIE pGrpCookie = 0);

private:
    CEnumNotification(CListAgent *pListAgent,
                      ENUM_FLAGS dwMode,
                      ENUMGROUP_MODE egMode = EG_NORMAL,
                      PNOTIFICATIONCOOKIE pGrpCookie = 0)
    : _CRefs()
    , _cPos(0)
    , _posCookie(COOKIE_NULL)
    {
        NotfAssert((pListAgent));
        _pListAgent = pListAgent;
        if (_pListAgent)
        {
            _pListAgent->AddRef();
        }
        _EnumFlags = dwMode;
        _GroupMode = egMode;
        _grpCookie = COOKIE_NULL;

        if (pGrpCookie)
        {
            _grpCookie = *pGrpCookie;
            _GroupMode = EG_GROUPITEMS;
        }
    }

    void SetNewListAgent(CListAgent *pListAgent)
    {
        NotfAssert((pListAgent));
        if (_pListAgent)
        {
            _pListAgent->Release();
        }
        _pListAgent = pListAgent;
        if (_pListAgent)
        {
            _pListAgent->AddRef();
        }
    }

public:
    ~CEnumNotification()
    {
        if (_pListAgent)
        {
            _pListAgent->Release();
        }
    }

private:
    CRefCount           _CRefs;        // the total refcount of this object
    ULONG               _cPos;         // the current position
    CListAgent         *_pListAgent;   // the object we get the packages
    NOTIFICATIONCOOKIE  _posCookie;    // the internal position for the map
    ENUM_FLAGS          _EnumFlags;
    NOTIFICATIONCOOKIE  _grpCookie;    // group cookie to enum group only
    ENUMGROUP_MODE      _GroupMode;
};

#endif // _NOTFTOBJ_HXX_

