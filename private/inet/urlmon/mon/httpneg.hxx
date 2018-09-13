//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       HTTPNEG.HXX
//
//  Contents:   Definitions for code to handle multiplexing multiple
//              concurrent IHttpNegotiate interfaces.
//
//  Classes:    CHttpNegHolder
//
//  Functions:
//
//  History:    01-30-96    JoeS (Joe Souza)        Created
//
//----------------------------------------------------------------------------
#ifndef _HTTPNEG_HXX_
#define _HTTPNEG_HXX_

class CHttpNegNode;
class CHttpNegNode
{
public:
    CHttpNegNode(IHttpNegotiate *pIHttpNeg)
    {
        _pIHttpNeg = pIHttpNeg;
        _pNext = NULL;
    }
    IHttpNegotiate *GetHttpNegotiate()
    {
        return _pIHttpNeg;
    }
    CHttpNegNode *GetNextNode()
    {
        return _pNext;
    }
    void SetNextNode(CHttpNegNode *pNext)
    {
        _pNext = pNext;
    }

private:
    IHttpNegotiate      *   _pIHttpNeg; // Pointer to caller's IHttpNegotiate.
    CHttpNegNode        *   _pNext;

};

class CHttpNegHolder : public IHttpNegotiate
{
public:
        CHttpNegHolder(void);

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);
        STDMETHOD_(ULONG,Release) (THIS);

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

        // *** Helper functions ***
        STDMETHOD(AddNode) (IHttpNegotiate *IHttpNeg);

private:
        STDMETHOD(RemoveAllNodes) (void);

    CRefCount    _CRefs;
    // BUGBUG: have node of 5 elements or so!
    CHttpNegNode *_pCHttpNegNode;
    LONG         _cElements;
};

#endif //_HTTPNEG_HXX_

