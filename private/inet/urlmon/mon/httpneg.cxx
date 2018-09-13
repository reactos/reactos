//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       HTTPNEG.CXX
//
//  Contents:   Code to handle multiplexing multiple concurrent
//              IHttpNegotiate interfaces.
//
//  Classes:    CHttpNegHolder
//
//  Functions:
//
//  History:    01-30-96    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------

#include <urlint.h>
#include <urlmon.hxx>
#include "httpneg.hxx"

CHttpNegHolder::CHttpNegHolder(void) : _CRefs()
{
    _pCHttpNegNode = NULL;
    _cElements = 0;
}

STDMETHODIMP CHttpNegHolder::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    UrlMkDebugOut((DEB_BINDING, "%p _IN CHttpNegHolder::QueryInterface\n", this));

    *ppvObj = NULL;
    if (riid == IID_IHttpNegotiate)
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
        CHttpNegNode   *pNode;

        pNode = _pCHttpNegNode;

        while (pNode)
        {
            hr = pNode->GetHttpNegotiate()->QueryInterface(riid, ppvObj);

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

    UrlMkDebugOut((DEB_BINDING, "%p OUT CHttpNegHolder::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

STDMETHODIMP_(ULONG) CHttpNegHolder::AddRef(void)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CHttpNegHolder::AddRef\n", this));

    LONG lRet = ++_CRefs;

    UrlMkDebugOut((DEB_BINDING, "%p OUT CHttpNegHolder::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

STDMETHODIMP_(ULONG) CHttpNegHolder::Release(void)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CHttpNegHolder::Release\n", this));
    UrlMkAssert((_CRefs > 0));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        RemoveAllNodes();
        delete this;
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CHttpNegHolder::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

HRESULT CHttpNegHolder::BeginningTransaction(LPCWSTR szURL, LPCWSTR szHeaders,
            DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CHttpNegHolder::BeginningTransaction (szURL:%ws, szHeaders:%ws)\n", this, szURL, szHeaders));
    VDATETHIS(this);
    HRESULT         hr = NOERROR;
    CHttpNegNode    *pNode;
    LPWSTR          szTmp = NULL, szNew = NULL, szRunning = NULL;

    pNode = _pCHttpNegNode;

    while (pNode)
    {
        hr = pNode->GetHttpNegotiate()->BeginningTransaction(szURL, szHeaders, dwReserved, &szNew);
        UrlMkDebugOut((DEB_BINDING, "%p === CHttpNegHolder::BeginningTransaction (szNew:%ws )\n", this, szNew));

        if (hr == NOERROR && szNew != NULL && szRunning != NULL)
        {
            szTmp = szRunning;
            szRunning = new WCHAR [wcslen(szTmp) + 1 + wcslen(szNew) + 1];
            if (szRunning)
            {
                if (szTmp)
                {
                    wcscpy(szRunning, szTmp);
                    wcscat(szRunning, szNew);
                }
                else
                {
                    wcscpy(szRunning, szNew);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            delete szTmp;
            delete szNew;

            if (hr != NOERROR)
            {
                goto BegTransExit;
            }
        }
        else
        {
            szRunning = szNew;
        }

        pNode = pNode->GetNextNode();
    }

    *pszAdditionalHeaders = szRunning;

BegTransExit:

    UrlMkDebugOut((DEB_BINDING, "%p OUT CHttpNegHolder::BeginningTransaction (pszAdditionalHeaders:%ws )\n", this, *pszAdditionalHeaders));
    return hr;
}

HRESULT CHttpNegHolder::OnResponse(DWORD dwResponseCode,LPCWSTR szResponseHeaders,
                        LPCWSTR szRequestHeaders,LPWSTR *pszAdditionalRequestHeaders)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CHttpNegHolder::OnError\n", this));
    VDATETHIS(this);
    HRESULT         hr = NOERROR;
    CHttpNegNode    *pNode;
    LPWSTR          szTmp = NULL, szNew = NULL, szRunning = NULL;

    pNode = _pCHttpNegNode;

    while (pNode)
    {
        hr = pNode->GetHttpNegotiate()->OnResponse(dwResponseCode, szResponseHeaders, szRequestHeaders, &szNew);
        if (hr == NOERROR && szNew != NULL && szRunning != NULL)
        {
            szTmp = szRunning;
            szRunning = new WCHAR [wcslen(szTmp) + 1 + wcslen(szNew) + 1];
            if (szRunning)
            {
                if (szTmp)
                {
                    wcscpy(szRunning, szTmp);
                    wcscat(szRunning, szNew);
                }
                else
                {
                    wcscpy(szRunning, szNew);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            delete szTmp;
            delete szNew;

            if (hr != NOERROR)
            {
                goto OnErrorExit;
            }
        }
        else
        {
            szRunning = szNew;
        }

        pNode = pNode->GetNextNode();
    }

    *pszAdditionalRequestHeaders = szRunning;

    if (hr == E_NOTIMPL)
    {
        hr = NOERROR;
    }

OnErrorExit:

    UrlMkDebugOut((DEB_BINDING, "%p OUT CHttpNegHolder::OnError\n", this));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CHttpNegHolder::AddNode
//
//  Synopsis:
//
//  Arguments:  [pIHttpNeg] --
//
//  Returns:
//
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CHttpNegHolder::AddNode(IHttpNegotiate *pIHttpNeg)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CHttpNegHolder::AddNode \n", this));
    HRESULT hr = NOERROR;

    CHttpNegNode *pFirstNode = _pCHttpNegNode;
    CHttpNegNode *pNode;

    // Allocate memory for new pNode member.

    pNode = new CHttpNegNode(pIHttpNeg);

    if (!pNode)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        UrlMkDebugOut((DEB_BINDING, "%p IN  CHttpNegHolder::AddNode (New Node:%p, IHttpNeg:%p) \n",
                                        this, pNode,pNode->GetHttpNegotiate() ));

        // if a node is already
        if (pFirstNode)
        {
            pNode->SetNextNode(pFirstNode);
        }

        // the new node is the first node
        _pCHttpNegNode = pNode;
        _cElements++;
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CHttpNegHolder::AddNode (NewNode:%p, hr:%lx) \n", this, pNode, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CHttpNegHolder::RemoveAllNodes
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
HRESULT CHttpNegHolder::RemoveAllNodes(void)
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CHttpNegHolder::RemoveAllNodes\n", this));
    HRESULT hr = S_OK;

    CHttpNegNode *pNode = _pCHttpNegNode;
    CHttpNegNode *pNextNode = NULL;

    while (pNode)
    {
        pNextNode = pNode->GetNextNode();

        // release the object
        pNode->GetHttpNegotiate()->Release();
        UrlMkDebugOut((DEB_BINDING, "%p OUT CHttpNegHolder::RemoveAllNodes (Delete Node:%p, IHttpNeg:%p) \n",
                        this, pNode, pNode->GetHttpNegotiate()));
        delete pNode;
        _cElements--;

        pNode = pNextNode;
    }

    _pCHttpNegNode = NULL;
    UrlMkAssert((_cElements == 0));

    UrlMkDebugOut((DEB_BINDING, "%p OUT CHttpNegHolder::RemoveAllNodes (hr:%lx) \n", this, hr));
    return hr;
}

