//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       oinet.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _OINET_HXX_
#define _OINET_HXX_

enum LOC
{
     LOC_NONE       = 1
    ,LOC_NAMESPACE
    ,LOC_INTERNAL
    ,LOC_EXTERNAL
};


//+---------------------------------------------------------------------------
//
//  Class:      CProtNode ()
//
//  Purpose:
//
//  Interface:  Add --
//              Remove
//              FindFirst --
//              FindNext --
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CNodeData;

class CNodeData
{
public:
    ATOM             _atom;
    IClassFactory   *_pCFProt;
    CLSID           _clsidProt;
    CNodeData       *_pNextNode;

    CNodeData(ATOM atom, IClassFactory *pCF, CLSID clsid) : _clsidProt(clsid)
    {
        _atom = atom;
        _pCFProt = pCF;
        if (_pCFProt)
        {
            _pCFProt->AddRef();
        }
    }

    ~CNodeData()
    {
        if (_pCFProt)
        {
            _pCFProt->Release();
        }
    }

private:
    CNodeData() : _clsidProt(CLSID_NULL)
    {
        TransAssert((FALSE));
    }

};


// Simple class to hold per protocol information. 
// Currently we only keep the name of the protocol here. 
class CProtocolData {
public:
    CProtocolData(): _pszProtocol(NULL), _pNext(NULL)
    {}
    ~CProtocolData( ) { delete [] _pszProtocol; }

    BOOL Init(LPCWSTR pszProt, CProtocolData *pNext)
    {
        int len = lstrlenW(pszProt) + 1;
        
        TransAssert(_pszProtocol == NULL && _pNext == NULL);
        
        _pszProtocol = new WCHAR[len];
        if (_pszProtocol)
        {
            StrCpyNW(_pszProtocol, pszProt, len);
            _pNext = pNext;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    // Access methods.
    LPCWSTR GetProtocol( ) const { return _pszProtocol; }
    CProtocolData * GetNext( ) const { return _pNext; }

public:
    LPWSTR  _pszProtocol;
    CProtocolData * _pNext;
};

        
//+---------------------------------------------------------------------------
//
//  Class:      CProtMgr ()
//
//  Purpose:
//
//  Interface:  QueryInterface --
//              AddRef --
//              Release --
//              Register --
//              UnRegister --
//              Add --
//              Remove --
//              FindFirst --
//              FindNext --
//              _cElements --
//              _CRefs --
//              _cElements --
//              _MapAtom --
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CProtMgr
{
public:
    STDMETHODIMP Register(IClassFactory *pCF, REFCLSID rclsid, LPCWSTR pszProtocol,
                          ULONG  cPatterns = 0, const LPCWSTR *ppwzPatterns = 0, DWORD dwReserved = 0);
    STDMETHODIMP Unregister(IClassFactory *pCF, LPCWSTR pszProtocol);

    // called by the protocol broker
    virtual STDMETHODIMP FindFirstCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid );
    virtual STDMETHODIMP FindNextCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid);
    virtual STDMETHODIMP LookupClsIDFromReg(LPCWSTR pwzUrl, CLSID *ppclsid, DWORD *pcClsIds = 0, DWORD *pdwFlags = 0, DWORD dwOpt = 0);
private:


public:
    CProtMgr() : _cElements(-1), _CRefs(1)
    {
        _pPosNode = 0;
        _pNextNode = 0;
    }

    ~CProtMgr()
    {
        TransAssert(((_cElements == -1 ) || (_cElements == 0)));
    }

protected:
    CRefCount       _CRefs;         // the total refcount of this object
    CRefCount       _cElements;     // elements in list
    CMutexSem       _mxs;           // single access to protocol manager
    CNodeData      *_pNextNode;     // head of list
    CNodeData      *_pPosNode;      // next position for looking when call on FindNext
};


class CProtMgrNameSpace : public CProtMgr
{
public:
    virtual STDMETHODIMP FindFirstCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid);
    virtual STDMETHODIMP FindNextCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid);
    virtual STDMETHODIMP LookupClsIDFromReg(LPCWSTR pwzUrl, CLSID *ppclsid, DWORD *pcClsIds = 0, DWORD *pdwFlags = 0, DWORD dwOpt = 0);

public:
    CProtMgrNameSpace() :  _pProtList(NULL)
    {
        _cElements = 0;
    }

    ~CProtMgrNameSpace()
    {
        CProtocolData *pDelProtData = _pProtList;
        while (pDelProtData != NULL)
        {
            CProtocolData *pNext = pDelProtData->GetNext();
            delete pDelProtData;
            pDelProtData = pNext;
        }
    }

private:
    CProtocolData * _pProtList;

    // Should we look up the registry for this prot. This function assumes caller will lookup the
    // registry if it returns TRUE. 
    STDMETHODIMP ShouldLookupRegistry(LPCWSTR pwszProt); 
};

class CProtMgrMimeHandler : public CProtMgr
{
public:
    virtual STDMETHODIMP LookupClsIDFromReg(LPCWSTR pwzUrl, CLSID *ppclsid, DWORD *pcClsIds = 0, DWORD *pdwFlags = 0, DWORD dwOpt = 0);

public:
    CProtMgrMimeHandler()
    {
    }

    ~CProtMgrMimeHandler()
    {
    }
private:
};

//+---------------------------------------------------------------------------
//
//  Class:      COInetSession ()
//
//  Purpose:
//
//  Interface:  QueryInterface --
//              AddRef --
//              Release --
//              RegisterNameSpace --
//              UnregisterNameSpace --
//              RegisterMimeFilter --
//              UnregisterMimeFilter --
//              CreateBinding --
//              SetSessionOption --
//              GetSessionOption --
//              CreateFirstProtocol --
//              CreateNextProtocol --
//              CreateHandler --
//              FindOInetProtocolClsID --
//              Create --
//              _dwWhere --
//              _dwProtMax --
//              FindFirstCF --
//              FindNextCF --
//              FindInternalCF --
//              dwId --
//              pCF --
//              ProtMap --
//              _CRefs --
//              _CProtMgr --
//              _CProtMgrNameSpace --
//              _CProtMgrMimeFilter --
//              _dwWhere --
//              _dwProtMax --
//              INTERNAL_PROTOCOL_MAX --
//
//  History:    11-20-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class COInetSession : public IOInetSession, public IOInetProtocolInfo 
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP RegisterNameSpace(
        IClassFactory *pCF,
        REFCLSID rclsid,
        LPCWSTR pwzProtocol,
        ULONG          cPatterns,
        const LPCWSTR *ppwzPatterns,
        DWORD          dwReserved
        );

    STDMETHODIMP UnregisterNameSpace(
        IClassFactory *pCF,
        LPCWSTR pszProtocol
        );

    STDMETHODIMP RegisterMimeFilter(
        IClassFactory *pCF,
        REFCLSID rclsid,
        LPCWSTR pwzType
        );

    STDMETHODIMP UnregisterMimeFilter(
        IClassFactory *pCF,
        LPCWSTR pwzType
        );

    STDMETHODIMP CreateBinding(
        LPBC pBC,
        LPCWSTR szUrl,
        IUnknown *pUnkOuter,
        IUnknown **ppUnk,
        IOInetProtocol **ppOInetProt,
        DWORD dwOption
        );

    STDMETHODIMP SetSessionOption(
        DWORD dwOption,
        LPVOID pBuffer,
        DWORD dwBufferLength,
        DWORD dwReserved
        );

    STDMETHODIMP GetSessionOption(
        DWORD dwOption,
        LPVOID pBuffer,
        DWORD *pdwBufferLength,
        DWORD dwReserved
        );

    // *** IOInetProtocolInfo

    STDMETHODIMP ParseUrl(
        LPCWSTR     pwzUrl,
        PARSEACTION ParseAction,
        DWORD       dwFlags,
        LPWSTR      pwzResult,
        DWORD       cchResult,
        DWORD      *pcchResult,
        DWORD       dwReserved
        );

    STDMETHODIMP CombineUrl(
        LPCWSTR     pwzBaseUrl,
        LPCWSTR     pwzRelativeUrl,
        DWORD       dwFlags,
        LPWSTR      pwzResult,
        DWORD       cchResult,
        DWORD      *pcchResult,
        DWORD       dwReserved
        );

    STDMETHODIMP CompareUrl(
        LPCWSTR pwzUrl1,
        LPCWSTR pwzUrl2,
        DWORD dwFlags
        );

    STDMETHODIMP QueryInfo(
        LPCWSTR         pwzUrl,
        QUERYOPTION     OueryOption,
        DWORD           dwQueryFlags,
        LPVOID          pBuffer,
        DWORD           cbBuffer,
        DWORD          *pcbBuf,
        DWORD           dwReserved
        );


    // COInetSession methods used by the protocol broker to get protocols
    STDMETHODIMP CreateFirstProtocol(LPCWSTR pwzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk,  IOInetProtocol **ppProt, CLSID *pclsid, DWORD dwOpt = 0);
    STDMETHODIMP CreateNextProtocol(LPCWSTR pwzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk,  IOInetProtocol **ppProt, CLSID *pclsid);
    STDMETHODIMP CreateHandler(LPCWSTR pwzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk,  IOInetProtocol **ppProt, CLSID *pclsid);

    STDMETHODIMP FindOInetProtocolClsID(LPCWSTR pwzUrl, CLSID *pclsid);

    static HRESULT Create(DWORD dwMode, COInetSession **ppCOInetSession);

    COInetSession() : _CProtMgrNameSpace(),  _CProtMgr(), _CProtMgrMimeFilter()
    {
        _dwWhere = LOC_NAMESPACE;
        _dwProtMax = INTERNAL_PROTOCOL_MAX;

        for(DWORD i = 0; i < _dwProtMax; i++)
        {
            _ProtMap[i].pCF = 0;
        }
    }
    ~COInetSession()
    {
        for(DWORD i = 0; i < _dwProtMax; i++)
        {
            if (_ProtMap[i].pCF != 0)
            {
                _ProtMap[i].pCF->Release();
            }
        }
    }

private:
    // private methods
    STDMETHODIMP FindFirstCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid, DWORD dwOpt = 0);
    STDMETHODIMP FindNextCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid);
    STDMETHODIMP FindInternalCF(LPCWSTR pszProt, IClassFactory **ppUnk, CLSID *pclsid);
    STDMETHODIMP CreateProtocolInfo(LPCWSTR pwzUrl, IOInetProtocolInfo **ppProtInfo);
    STDMETHODIMP CreateSecurityMgr(LPCWSTR pwzUrl, IInternetSecurityManager **ppSecMgr);

    typedef struct _tagProtMap
    {
        DWORD           dwId;
        IClassFactory  *pCF;

    } ProtMap;
    VOID    UpdateTransLevelHandlerCount(BOOL bAttach);

private:
    CRefCount           _CRefs;          // the total refcount of this object

    CProtMgr            _CProtMgr;
    CProtMgrNameSpace   _CProtMgrNameSpace;
    CProtMgrMimeHandler _CProtMgrMimeFilter;
    DWORD               _dwWhere;
    DWORD               _dwProtMax;
    ProtMap             _ProtMap[INTERNAL_PROTOCOL_MAX];
};
HRESULT GetCOInetSession(DWORD dwMode, COInetSession **ppOInetSession, DWORD dwReserved);
HRESULT OInetGetSession(DWORD dwMode, IOInetSession **ppOInetSession, DWORD dwReserved);


DWORD IsKnownProtocol(LPCWSTR wzProtocol);
BOOL IsOInetProtocol(LPBC pBndCtx, LPCWSTR wzProtocol);

DWORD IsKnownOInetProtocolClass(CLSID *pclsid);
CLSID *GetKnownOInetProtocolClsID(DWORD dwProtoId);
HRESULT CreateKnownProtocolInstance(DWORD dwProtId, REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk);

HRESULT GetProtMgr(CProtMgr **ppCProtMgr);
HRESULT GetTransactionObjects(LPBC pBndCtx, LPCWSTR wzUrl, IUnknown *pUnkOuter, IUnknown **ppUnk, IOInetProtocol **ppProt, DWORD dwOption, CTransData **pCTransData);


#endif // _OINET_HXX_


                            
