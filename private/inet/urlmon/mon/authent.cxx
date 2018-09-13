//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       AUTHENT.CXX
//
//  Contents:   Code to handle multiplexing multiple concurrent
//              IAuthenticate interfaces.
//
//  Classes:    CBasicAuthHolder
//
//  Functions:
//
//  History:    02-05-96    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

#include <urlint.h>
#include <urlmon.hxx>
#include "authent.hxx"

CBasicAuthHolder::CBasicAuthHolder(void) : _CRefs()
{
    _pCBasicAuthNode = NULL;
    _cElements = 0;
}

STDMETHODIMP CBasicAuthHolder::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    UrlMkDebugOut((DEB_BINDING, "%p _IN CBasicAuthHolder::QueryInterface\n", this));

    *ppvObj = NULL;
    if (riid == IID_IAuthenticate)
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
        CBasicAuthNode  *pNode;

        pNode = _pCBasicAuthNode;

        while (pNode)
        {
            hr = pNode->GetBasicAuthentication()->QueryInterface(riid, ppvObj);

            if (hr == NOERROR)
            {
                pNode = NULL;
            }
            else
            {
                pNode = pNode->GetNextNode();
            }
        }
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CBasicAuthHolder::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

STDMETHODIMP_(ULONG) CBasicAuthHolder::AddRef(void)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CBasicAuthHolder::AddRef\n", this));

    LONG lRet = ++_CRefs;

    UrlMkDebugOut((DEB_BINDING, "%p OUT CBasicAuthHolder::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

STDMETHODIMP_(ULONG) CBasicAuthHolder::Release(void)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CBasicAuthHolder::Release\n", this));
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        RemoveAllNodes();
        delete this;
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CBasicAuthHolder::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

HRESULT CBasicAuthHolder::Authenticate(HWND* phwnd, LPWSTR *pszUsername,
            LPWSTR *pszPassword)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CBasicAuthHolder::Authenticate\n", this));
    VDATETHIS(this);
    HRESULT         hr = NOERROR;
    CBasicAuthNode  *pNode;

    pNode = _pCBasicAuthNode;

    while (pNode)
    {
        hr = pNode->GetBasicAuthentication()->Authenticate(phwnd, pszUsername, pszPassword);
        if (hr == S_OK)
        {
            break;
        }

        pNode = pNode->GetNextNode();
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CBasicAuthHolder::Authenticate\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBasicAuthHolder::AddNode
//
//  Synopsis:
//
//  Arguments:  [pIBasicAuth] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBasicAuthHolder::AddNode(IAuthenticate *pIBasicAuth)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CBasicAuthHolder::AddNode \n", this));
    HRESULT hr = NOERROR;

    CBasicAuthNode *pFirstNode = _pCBasicAuthNode;
    CBasicAuthNode *pNode;

    // Allocate memory for new pNode member.

    pNode = new CBasicAuthNode(pIBasicAuth);

    if (!pNode)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        UrlMkDebugOut((DEB_BINDING, "%p IN  CBasicAuthHolder::AddNode (New Node:%p, IBasicAuth:%p) \n",
                                        this, pNode,pNode->GetBasicAuthentication() ));

        // if a node is already
        if (pFirstNode)
        {
            pNode->SetNextNode(pFirstNode);
        }

        // the new node is the first node
        _pCBasicAuthNode = pNode;
        _cElements++;
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CBasicAuthHolder::AddNode (NewNode:%p, hr:%lx) \n", this, pNode, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBasicAuthHolder::RemoveAllNodes
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CBasicAuthHolder::RemoveAllNodes(void)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CBasicAuthHolder::RemoveAllNodes\n", this));
    HRESULT hr = S_OK;

    CBasicAuthNode *pNode = _pCBasicAuthNode;
    CBasicAuthNode *pNextNode = NULL;

    while (pNode)
    {
        pNextNode = pNode->GetNextNode();

        // release the object
        pNode->GetBasicAuthentication()->Release();
        UrlMkDebugOut((DEB_BINDING, "%p OUT CBasicAuthHolder::RemoveAllNodes (Delete Node:%p, IBasicAuth:%p) \n",
                        this, pNode, pNode->GetBasicAuthentication()));
        delete pNode;
        _cElements--;

        pNode = pNextNode;
    }

    _pCBasicAuthNode = NULL;
    UrlMkAssert((_cElements == 0));

    UrlMkDebugOut((DEB_BINDING, "%p OUT CBasicAuthHolder::RemoveAllNodes (hr:%lx) \n", this, hr));
    return hr;
}

