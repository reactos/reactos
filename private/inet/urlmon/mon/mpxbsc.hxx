//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       MPXBSC.HXX
//
//  Contents:   Definitions for code to handle multiplexing multiple
//              concurrent IBindStatusCallback interfaces.
//
//  Classes:    CBindStatusCallback
//
//  Functions:
//
//  History:    01-04-96    JoeS (Joe Souza)        Created
//              01-15-96    JohannP (Johann Posch)  Modified to new IBSC
//
//----------------------------------------------------------------------------
#ifndef _MPXBSC_HXX_
#define _MPXBSC_HXX_

#include <sem.hxx>

// Definitions for Local Node Flags
#define NODE_FLAG_REMOVEOK      0x00000001

PerfDbgExtern(tagCBSCHolder)

class CBSCNode;
class CBSCNode
{
public:
    CBSCNode(IBindStatusCallback *pIBSC, DWORD grfFlags)
    {
        _grfFlags = grfFlags;

        _pIBSC = pIBSC;
        _pIServProv = NULL;
        _pIHttpNeg = NULL;
        _pIAuth = NULL;

        _pNext = NULL;
        _dwLocalFlags = 0;
    }
    ~CBSCNode()
    {
        PerfDbgLog4(tagCBSCHolder, this, "+CBSCNode::~CBSCNode (IBSC:%lx, IServProv:%lx, IHttpNeg:%lx, IAuth:%lx)",
            _pIBSC, _pIServProv, _pIHttpNeg, _pIAuth);

        if (_pIServProv)
        {
            _pIServProv->Release();
        }
        if (_pIHttpNeg)
        {
            _pIHttpNeg->Release();
        }
        if (_pIAuth)
        {
            _pIAuth->Release();
        }
        if (_pIBSC)
        {
            _pIBSC->Release();
        }

        PerfDbgLog(tagCBSCHolder, this, "-CBSCNode::~CBSCNode");
    }

    IServiceProvider *GetServiceProvider()
    {
        return _pIServProv;
    }
    IHttpNegotiate *GetHttpNegotiate()
    {
        return _pIHttpNeg;
    }
    IAuthenticate *GetAuthenticate()
    {
        return _pIAuth;
    }
    IBindStatusCallback *GetBSCB()
    {
        return _pIBSC;
    }
    CBSCNode *GetNextNode()
    {
        return _pNext;
    }
    void SetServiceProvider(IServiceProvider *pIServProv)
    {
        _pIServProv = pIServProv;
    }
    void SetHttpNegotiate(IHttpNegotiate *pIHttpNeg)
    {
        _pIHttpNeg = pIHttpNeg;
    }
    void SetAuthenticate(IAuthenticate *pIBasicAuth)
    {
        _pIAuth = pIBasicAuth;
    }
    void SetNextNode(CBSCNode *pNext)
    {
        _pNext = pNext;
    }
    DWORD GetFlags()
    {
        return _grfFlags;
    }
    void SetLocalFlags(DWORD bitmask)
    {
        _dwLocalFlags |= bitmask;
    }
    BOOL CheckLocalFlags(DWORD bitmask)
    {
        return(_dwLocalFlags & bitmask);
    }

private:
    DWORD                   _grfFlags;
    IBindStatusCallback *   _pIBSC;         // Pointer to caller's IBindStatusCallback.
    IServiceProvider    *   _pIServProv;    // Pointer to caller's IServiceProvider.
    IHttpNegotiate      *   _pIHttpNeg;     // Pointer to caller's IHttpNegotiate.
    IAuthenticate       *   _pIAuth;        // Pointer to caller's IAuthenticate.
    CBSCNode            *   _pNext;
    DWORD                   _dwLocalFlags;
};

class CBSCHolder : public IBindStatusCallback, public IServiceProvider,
                   public IHttpNegotiate, public IAuthenticate
{
public:
        CBSCHolder(void);
        ~CBSCHolder(void);

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);
        STDMETHOD_(ULONG,Release) (THIS);

        // *** IBindStatusCallback methods ***
        STDMETHOD(OnStartBinding) (DWORD grfBINDINFOF, IBinding * pib);
        STDMETHOD(OnLowResource) (DWORD reserved);
        STDMETHOD(OnProgress) (ULONG ulProgress, ULONG ulProgressMax,
                    ULONG ulStatusCode, LPCWSTR szStatusText);
        STDMETHOD(OnStopBinding) (HRESULT hresult, LPCWSTR szError);

        STDMETHOD(GetBindInfo)(
            DWORD *grfBINDINFOF,
            BINDINFO *pbindinfo);

        STDMETHOD(OnDataAvailable)(
            DWORD grfBSC,
            DWORD dwSize,
            FORMATETC *pformatetc,
            STGMEDIUM *pstgmed
            );

        STDMETHOD(OnObjectAvailable)(
            REFIID riid,
            IUnknown *punk);

        STDMETHOD(GetPriority)(LONG * pnPriority);

        // *** IServiceProvider ***
        STDMETHOD (QueryService)(REFGUID rsid, REFIID iid, void ** ppvObj);

        // *** IHttpNegotiate methods ***
        STDMETHOD(BeginningTransaction) (
            LPCWSTR szURL,
            LPCWSTR szHeaders,
            DWORD dwReserved,
            LPWSTR *pszAdditionalHeaders);

        STDMETHOD(OnResponse) (
            DWORD dwResponseCode,
            LPCWSTR szResponseHeaders,
            LPCWSTR szRequestHeaders,
            LPWSTR *pszAdditionalRequestHeaders);

        // *** IAuthenticate methods ***
        STDMETHOD(Authenticate) (
            HWND* phwnd,
            LPWSTR *pszUsername,
            LPWSTR *pszPassword);

        // *** Helper functions ***
        STDMETHOD(AddNode) (IBindStatusCallback *IBSC, DWORD grfFlags);
        STDMETHOD(RemoveNode) (IBindStatusCallback *IBSC);
        STDMETHOD(ObtainService)(REFGUID rsid, REFIID iid, void ** ppvObj);
        STDMETHOD(SetMainNode) (IBindStatusCallback *pIBSC, IBindStatusCallback **ppIBSCPrev);

private:
    CMutexSem    _mxs;          // single access to add and release nodes
    CRefCount    _CRefs;
    CBSCNode    *_pCBSCNode;
    LONG         _cElements;
    BOOL         _fBindStarted;
    BOOL         _fBindStopped;
    BOOL         _fHttpNegotiate;
    BOOL         _fAuthenticate;

    // BUGBUG: have node of 5 elements or so!
    // CBSCNode     _pCBSCNodeN[5];
};

HRESULT GetBSCHolder(LPBC pBC, CBSCHolder **ppCBSHolder);

#endif //_MPXBSC_HXX_

