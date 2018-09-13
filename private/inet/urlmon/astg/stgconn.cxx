//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:	stgconn.cxx
//
//  Contents:	Connection points for Async Storage/Stream Wrappers
//
//  Classes:	
//
//  Functions:	
//
//  History:	19-Dec-95	SusiA	Created
//
//----------------------------------------------------------------------------

#include "astghead.cxx"
#pragma hdrstop

#include "stgwrap.hxx"

//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::CConnectionPoint, public
//
//  Synopsis:	Constructor
//
//  Arguments:
//
//  History:	28-Dec-95	SusiA	Created
//
//----------------------------------------------------------------------------

CConnectionPoint::CConnectionPoint()
{
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::CConnectionPoint:%p()\n", this));
    _cReferences = 1;
    _dwCookie = 0;
    _pSinkHead = NULL;
    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::CConnectionPoint\n"));
}


void CConnectionPoint::Init(IConnectionPointContainer *pCPC)
{
    _pCPC = pCPC;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::QueryInterface, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	01-Jan-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc = S_OK;
    astgDebugOut((DEB_TRACE,
                  "In  CConnectionPoint::QueryInterface:%p()\n",
                  this));

    *ppvObj = NULL;

    if ((IsEqualIID(iid, IID_IUnknown)) ||
	(IsEqualIID(iid, IID_IConnectionPoint)))
    {
        *ppvObj = (IConnectionPoint *)this;
        CConnectionPoint::AddRef();
    }
    else
    {
        return E_NOINTERFACE;
    }

    astgDebugOut((DEB_TRACE, "Out CConnectionPoint::QueryInterface\n"));
    return ResultFromScode(sc);
}



//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::AddRef, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	29-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CConnectionPoint::AddRef(void)
{
    ULONG ulRet;
    astgDebugOut((DEB_TRACE,
                  "In  CConnectionPoint::AddRef:%p()\n",
                  this));
    InterlockedIncrement(&_cReferences);
    ulRet = _cReferences;
    
    astgDebugOut((DEB_TRACE, "Out CConnectionPoint::AddRef\n"));
    return ulRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::Release, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CConnectionPoint::Release(void)
{
    LONG lRet;
    astgDebugOut((DEB_TRACE,
                  "In  CConnectionPoint::Release:%p()\n",
                  this));

    astgAssert(_cReferences > 0);
    lRet = InterlockedDecrement(&_cReferences);
    if (lRet == 0)
    {
        astgAssert((lRet > 0) && "Connection point released too many times.");
    }
    else if (lRet < 0)
        lRet = 0;
    
    astgDebugOut((DEB_TRACE, "Out CConnectionPoint::Release\n"));
    return (ULONG)lRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::GetConnectionInterface, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::GetConnectionInterface(IID *pIID)
{
    astgDebugOut((DEB_ITRACE,
                  "In  CConnectionPoint::GetConnectionInterface:%p()\n",
                  this));

    
    *pIID = IID_IProgressNotify;
          
    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::GetConnectionInterface\n"));
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::GetConnectionPointContainer, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::GetConnectionPointContainer(
    IConnectionPointContainer ** ppCPC)
{
    astgDebugOut((DEB_ITRACE,
                  "In  CConnectionPoint::GetConnectionPointContainer:%p()\n",
                  this));

    *ppCPC = _pCPC;
    _pCPC->AddRef();
    
    astgDebugOut((DEB_ITRACE,
                  "Out CConnectionPoint::GetConnectionPointContainer\n"));
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::Advise, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	29-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::Advise(IUnknown *pUnkSink,
                                      DWORD *pdwCookie)
{
    SCODE sc;
    CSinkList *pslTemp = NULL;
    CSinkList **ppslHead = NULL;
    void *pv = NULL;
    
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::Advise:%p()\n", this));
    
    IProgressNotify *ppn;
	
    // for the sweeper release, only one Advise sink per storage/Stream will be allowed 
    if (_pSinkHead != NULL)
        return  E_UNEXPECTED;

    //BUGBUG:  Multithread access
    astgMem(pslTemp = new CSinkList);

    //Note:  The QueryInterface will give us a reference to hold on to.
    astgChk(pUnkSink->QueryInterface(IID_IProgressNotify, &pv));
    pslTemp->SetProgressNotify((IProgressNotify *)pv);
    
    pslTemp->SetNext(_pSinkHead);

    *pdwCookie = ++_dwCookie;
    pslTemp->SetCookie(*pdwCookie);
    
    _pSinkHead = pslTemp;

    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::Advise\n"));
    return sc;
Err:
    delete pslTemp;
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::Unadvise, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::Unadvise(DWORD dwCookie)
{
    CSinkList *pslTemp;
    CSinkList *pslPrev;
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::Unadvise:%p()\n", this));

    pslTemp = _pSinkHead;
    pslPrev = NULL;
    
    while ((pslTemp != NULL) && (pslTemp->GetCookie() != dwCookie))
    {
        pslPrev = pslTemp;
        pslTemp = pslTemp->GetNext();
    }

    if (pslTemp != NULL)
    {
        //Found the sink.  Delete it from the list.
        if (pslPrev != NULL)
        {
            pslPrev->SetNext(pslTemp->GetNext());
        }
        else
        {
            _pSinkHead = pslTemp->GetNext();
        }
        pslTemp->GetProgressNotify()->Release();
        
        delete pslTemp;
    }
    else
        //Client passed in unknown cookie.
        return E_UNEXPECTED;
        
    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::Unadvise\n"));
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::EnumConnections, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::EnumConnections(
    IEnumConnections **ppEnum)
{
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::EnumConnections:%p()\n", this));
    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::EnumConnections\n"));
    return E_NOTIMPL;
}



 
