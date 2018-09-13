//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       AUTHENT.HXX
//
//  Contents:   Definitions for code to handle multiplexing multiple
//              concurrent IAuthenticate interfaces.
//
//  Classes:    CBasicAuthHolder
//
//  Functions:
//
//  History:    02-05-96    JoeS (Joe Souza)        Created
//
//----------------------------------------------------------------------------
#ifndef _AUTHENT_HXX_
#define _AUTHENT_HXX_

class CBasicAuthNode;
class CBasicAuthNode
{
public:
    CBasicAuthNode(IAuthenticate *pIBasicAuth)
    {
        _pIBasicAuth = pIBasicAuth;
        _pNext = NULL;
    }
    IAuthenticate *GetBasicAuthentication()
    {
        return _pIBasicAuth;
    }
    CBasicAuthNode *GetNextNode()
    {
        return _pNext;
    }
    void SetNextNode(CBasicAuthNode *pNext)
    {
        _pNext = pNext;
    }

private:
    IAuthenticate    *   _pIBasicAuth; // Pointer to caller's IAuthenticate.
    CBasicAuthNode          *   _pNext;

};

class CBasicAuthHolder : public IAuthenticate
{
public:
        CBasicAuthHolder(void);

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);
        STDMETHOD_(ULONG,Release) (THIS);

        // *** IAuthenticate methods ***
        STDMETHOD(Authenticate) (
            HWND* phwnd,
            LPWSTR *pszUsername,
            LPWSTR *pszPassword);

        // *** Helper functions ***
        STDMETHOD(AddNode) (IAuthenticate *IBasicAuth);

private:
        STDMETHOD(RemoveAllNodes) (void);

    CRefCount    _CRefs;
    // BUGBUG: have node of 5 elements or so!
    CBasicAuthNode *_pCBasicAuthNode;
    LONG         _cElements;
};

#endif //_AUTHENT_HXX_

