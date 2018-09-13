/*
 * @(#)eventhelp.cxx 1.0 07/06/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * Helper functions for XML connection points
 * 
 */

#include "core.hxx"
#pragma hdrstop
#include <dispex.h>
#include "eventhelp.hxx"

#define FIREEVENT_MAXARGS  10   // We can handle up to n params to a function
LONG g_lCookieCount = 0;

/////////////////////////////////////////////////////////////////
// CXMLConnectionPt - implements IConnectionPoint, fires events //
/////////////////////////////////////////////////////////////////
CXMLConnectionPt::CXMLConnectionPt(
    REFIID iid, 
    IUnknown *punkHost, 
    PCPNODE *ppCPList, 
    ULONG_PTR *plSpinLock,
    CPTYPE cptPreferredInterface) : _EventIID(iid)
{
    Assert(!ISNULLIID(iid) && "Can't do anything without an IID to act on !");
    
    Assert (ppCPList && "Pointer to list of connections required");
    _ppRootNode = ppCPList;
    
    Assert (punkHost && "Outer unknown required");
    _punkHost = punkHost;
    _punkHost->AddRef();
    
    Assert(plSpinLock && "NULL SpinLock supplied");
    _plSpinLock = plSpinLock;
    _cpt = cptPreferredInterface;

}
    

CXMLConnectionPt::~CXMLConnectionPt()
{
    if (_punkHost)
        _punkHost->Release();
}

    
///////////////////////////////////////////////////////////
// IConnectionPoint Interface
///////////////////////////////////////////////////////////
HRESULT 
STDMETHODCALLTYPE 
CXMLConnectionPt::GetConnectionInterface(IID *pIID)
{
    if (pIID)
    {
        pIID = (IID *)&_EventIID;
        return S_OK;
    }
    else
    {
        return E_POINTER;
    }
}

HRESULT 
STDMETHODCALLTYPE 
CXMLConnectionPt::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
    HRESULT hr;
    if (ppCPC)
    {
        Assert(_punkHost && "No outer unknown - can't get IConnectionPointContainer");

        if (_punkHost)
        {
            hr = SUCCEEDED(_punkHost->QueryInterface(IID_IConnectionPointContainer, (LPVOID *)ppCPC)) ? S_OK : E_FAIL;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_POINTER;
    }
    return hr;    
}

    
HRESULT 
STDMETHODCALLTYPE 
CXMLConnectionPt::Advise( 
    IUnknown *pUnkSink,
    DWORD *pdwCookie)
{
    HRESULT hr;
    
    if (pUnkSink && pdwCookie)
    {
        
        PCPNODE pNode = new_ne CPNODE;
        if (pNode)
        {
            BusyLock bl(_plSpinLock);

            switch (_cpt)
            {
            case CP_Dispatch:
                IDispatch *pDisp;
                if (SUCCEEDED(hr = pUnkSink->QueryInterface(IID_IDispatch, (LPVOID *)&pDisp)))
                {
                    pNode->pdispConnector = pDisp;
                    pNode->cpt = CP_Dispatch;
                    pDisp->Release();
                }
                
                Assert (SUCCEEDED(hr) && "Failed to get the expected sink");
                break;

            case CP_PropertyNotifySink:
                IPropertyNotifySink *pns;
                if (SUCCEEDED(hr = pUnkSink->QueryInterface(IID_IPropertyNotifySink, (LPVOID *)&pns)))
                {
                    pNode->ppnsConnector = pns;
                    pNode->cpt = CP_PropertyNotifySink;
                    pns->Release();
                }

                Assert (SUCCEEDED(hr) && "Failed to get the expected sink");
                break;

            default:
                hr = E_NOINTERFACE; // Force the IUnknown case below
                break;

            }

            if (FAILED(hr)) // Default case, or failed QI above
            {
                pNode->cpt = CP_Unknown;
                pNode->punkConnector = pUnkSink;
            }
            
            pNode->pNext = *_ppRootNode;
#ifdef _DEBUG
            pNode->dwThreadId = GetCurrentThreadId();
#endif

            *pdwCookie = (DWORD)::InterlockedIncrement(&g_lCookieCount); // The cookie
#ifndef _WIN64
#else // _WIN64
        if ((*_ppRootNode)->pNext)
        {
            // If we're not the first node, the cookie is the cookie of the current 
            // initial node in the list plus 1
            *pdwCookie = pNode->dwCookie = (*_ppRootNode)->pNext->dwCookie + 1;
        }
        else
        {
            *pdwCookie = pNode->dwCookie = 0;
        }
#endif
            
            // Just insert the node at the beginning of the list
            *_ppRootNode = pNode;
            
        }
        else
        {
            // Couldn't create the new node
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        // Passed in one or more bad pointers
        hr = E_POINTER;
    }
    
Cleanup:
    
    return hr;
}
    

#ifdef _WIN64
#define COOKIE_IS_A_MATCH(ppN, Cookie)   ((*ppN)->dwCookie == Cookie)
#else // !_WIN64
#define COOKIE_IS_A_MATCH(ppN, Cookie)   ((DWORD_PTR)*ppN == (DWORD_PTR)Cookie)
#endif

HRESULT 
STDMETHODCALLTYPE 
CXMLConnectionPt::Unadvise(DWORD dwCookie)
{
    HRESULT hr = S_OK;
    
    BusyLock bl(_plSpinLock);
    PCPNODE *ppNode = _ppRootNode;

    // Find the node
    while (ppNode && *ppNode && !COOKIE_IS_A_MATCH(ppNode, dwCookie))
        ppNode = &((*ppNode)->pNext);
    
    // Found ?
    if (*ppNode)
    {
        PCPNODE pTemp = *ppNode;
        *ppNode = (*ppNode)->pNext;
        delete pTemp;
    }
    else
    {
        hr = CONNECT_E_NOCONNECTION;
    }

    return hr;
}
    
    
    
HRESULT 
STDMETHODCALLTYPE 
CXMLConnectionPt::EnumConnections(IEnumConnections **ppEnum)
{
    HRESULT hr = S_OK;

    if (ppEnum)
    {
        CXMLEnumConnections *pEnumConnections;
        // Create a new enumerator
        pEnumConnections = new_ne CXMLEnumConnections(_EventIID, *_ppRootNode, _punkHost, _plSpinLock);
        
        if (NULL == pEnumConnections)
            return E_OUTOFMEMORY;
        
        *ppEnum = (IEnumConnections *)pEnumConnections;
        
    }
    else
    {
        hr = E_POINTER;
    }

Cleanup:
    return hr;
}



CXMLConnectionPtContainer::CXMLConnectionPtContainer(
    REFIID iid, 
    IUnknown *pUnk, 
    PCPNODE *ppNodeList, 
    ULONG_PTR *plSpinLock) : _EventIID(iid)
{
    Assert (!ISNULLIID(iid) && "NULL IID for events is not valid");

    Assert (pUnk && "Must provide outer IUnknown!");
    Assert (plSpinLock && "NULL SpinLock supplied");
    
    _plSpinLock = plSpinLock;
    _punkOuter = pUnk;
    
    if (pUnk)
        pUnk->AddRef();
    
    _ppNodeList = ppNodeList;
    
}
             
CXMLConnectionPtContainer::~CXMLConnectionPtContainer()
{
    if (_punkOuter)
        _punkOuter->Release();
}
    
    
CXMLConnectionPt* 
CXMLConnectionPtContainer::CreateConnectionPoint(REFIID riid, CPTYPE cpt)
{
    CXMLConnectionPt *pXCPT;
    
    pXCPT = new_ne CXMLConnectionPt(
        riid,
        _punkOuter, 
        _ppNodeList,
        _plSpinLock,
        cpt);
    
    Assert(pXCPT && "Unable to allocate Connection Point");
    
    return pXCPT;
    
}

////////////////////////////////////////////////
// IUnknown methods
////////////////////////////////////////////////
HRESULT 
STDMETHODCALLTYPE 
CXMLConnectionPtContainer::QueryInterface(REFIID riid, void **ppvObject)
{
    if (riid == IID_IUnknown)
    {
        *ppvObject = static_cast<IUnknown*>(this);
    }
    else if (riid == IID_IConnectionPointContainer)
    {
        *ppvObject = static_cast<IConnectionPointContainer *>(this);
    }
    else if (_punkOuter)
    {
        return _punkOuter->QueryInterface(riid, ppvObject);
    }
    
    AddRef();
    return S_OK;
}


////////////////////////////////////////////////
// IConnectionPointContainer methods
////////////////////////////////////////////////

// For enumeration, we simply create an tear-off enumerator object which 
// knows only about the supplied event interface and IDispatch

HRESULT 
STDMETHODCALLTYPE 
CXMLConnectionPtContainer::EnumConnectionPoints( 
    IEnumConnectionPoints  **ppEnum)
{
    HRESULT hr = S_OK;
    
    if (NULL == ppEnum)
        return E_POINTER;
    
    CXMLEnumConnectionPt *pCPE = new_ne CXMLEnumConnectionPt(_EventIID, this);
    
    *ppEnum = (IEnumConnectionPoints *)pCPE;
    
    if (NULL == pCPE)
        hr = E_OUTOFMEMORY;
Cleaup:
    return hr;
}
    
// FindConnectionPoint only knows about the supplied interface 
// REVIEW: Do we have to make this able to find any connection point
// in the whole system ?  Not a problem currently since we're only
// doing events off the document, but it might be necessary later (SimonB, 07-06-1998)


HRESULT 
STDMETHODCALLTYPE 
CXMLConnectionPtContainer::FindConnectionPoint( 
    REFIID riid,
    IConnectionPoint **ppCP)
{
    HRESULT hr = S_OK;
    CPTYPE cpt = CP_Unknown;
    
    if (NULL != ppCP)
    {
        *ppCP = NULL;
    }
    else
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Put IID_IPropertyNotifyChange first so that, if this sink is already 
    // meant to handle that interface, we pass the hint through correctly, 
    // rather than assuming CP_Dispatch
    if (IsEqualGUID(riid, IID_IPropertyNotifySink))
    {
        cpt = CP_PropertyNotifySink;
        
    }
    else if (IsEqualGUID(riid, _EventIID) || IsEqualGUID(riid, IID_IDispatch))
    {
        cpt = CP_Dispatch;
    }
    else
    {
        hr = CONNECT_E_NOCONNECTION;
        goto Cleanup;
    }

    // Make sure we create a connection point with the right interface
    *ppCP = (IConnectionPoint *) CreateConnectionPoint(riid, cpt);
    if (*ppCP == NULL)
        hr = E_OUTOFMEMORY;

Cleanup:    

    return hr;
}


CXMLEnumConnectionPt::CXMLEnumConnectionPt(
    REFIID iid, 
    CXMLConnectionPtContainer *pCPC)
{
    Assert (!ISNULLIID(iid) && "NULL IID for events is not valid");

    _iIndex = 0;
    
    Assert (pCPC && "Must pass in a pointer to a CXMLCPC");
    _pCPC = NULL;
    
    if (pCPC)
    {
        _pCPC = pCPC;
        _pCPC->AddRef();
    }
    // Initialize the outgoing interface table
    _Interfaces[0].cpt = CP_PropertyNotifySink;
    _Interfaces[0].iidOutgoingInterface = IID_IPropertyNotifySink;
    _Interfaces[1].cpt = CP_Dispatch;
    _Interfaces[1].iidOutgoingInterface = iid;

}

CXMLEnumConnectionPt::~CXMLEnumConnectionPt()
{
    
    if (_pCPC)
        _pCPC->Release();
}


///////////////////////////////////////////////////
// IEnumConnectionPoints methods
///////////////////////////////////////////////////
    
HRESULT 
STDMETHODCALLTYPE 
CXMLEnumConnectionPt::Next( 
    ULONG cConnections,
    LPCONNECTIONPOINT *ppCP,
    ULONG *pcFetched)
{
    HRESULT hr = S_OK;
    int i;

    if (ppCP)
    {
        *ppCP = NULL;
    }
    else
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if (cConnections < 1)
    {
        hr = S_FALSE;
        goto Cleanup;
    }
    
    for (i = 0; i < (int)cConnections && (UINT)(i + _iIndex) < 2; i++)
    {
        ppCP[i] = (LPCONNECTIONPOINT) _pCPC->CreateConnectionPoint(_Interfaces[i + _iIndex].iidOutgoingInterface, _Interfaces[i + _iIndex].cpt);
        if (NULL == ppCP[i])
        {
            // Release everything we've already created
            i--;
            while (i > 0)
                (ppCP[i--])->Release();

            hr = E_OUTOFMEMORY;
            break;
        }
    }

    if (pcFetched && SUCCEEDED(hr))
        *pcFetched = i - 1;
        
    // Did we give back as many connections as requested ?  Need to do a signed compare, hence the cast.
    if (i < (int)cConnections)
        hr = S_FALSE;
   
Cleanup:
    return hr;
}
    

HRESULT 
STDMETHODCALLTYPE 
CXMLEnumConnectionPt::Skip(ULONG cConnections)
{
    _iIndex += cConnections;

    if (_iIndex > 2) 
        return S_FALSE;
    else
        return S_OK;
}
    
HRESULT 
STDMETHODCALLTYPE 
CXMLEnumConnectionPt::Reset()
{
    _iIndex = 0;
    return S_OK;
}
    

HRESULT 
STDMETHODCALLTYPE 
CXMLEnumConnectionPt::Clone(IEnumConnectionPoints **ppEnum)
{
    Assert (FALSE && "Clone not implemented !");
    return E_NOTIMPL;
}



CXMLEnumConnections::CXMLEnumConnections(
    REFIID iid, 
    PCPNODE pRootNode, 
    IUnknown *pUnk, 
    ULONG_PTR *plSpinLock) : _EventIID(iid)
{
    Assert (!ISNULLIID(iid) && "NULL IID for events is not valid");

    Assert(pUnk && "An outer object is required !");
    
    Assert(pRootNode && "A pointer to the connection list is required");
    _pRootNode = NULL;

    // Make sure this gets set up before CopyConnectionList is called
    Assert(plSpinLock && "NULL SpinLock supplied");
    _plSpinLock = plSpinLock;

    pUnk->AddRef();
    CopyConnectionList(pRootNode);
    _punkObj = pUnk;

    _pCurrentConnection = _pRootNode;
}

CXMLEnumConnections::~CXMLEnumConnections()
{
    _punkObj->Release();
    ReleaseCPNODEList(_pRootNode);
}


// Make a copy of the list of connections so that we can safely iterate 
// through it at any time.  Validity of connections is obviously not
// guaranteed
HRESULT 
CXMLEnumConnections::CopyConnectionList(PCPNODE pRootNode)
{
    HRESULT hr = S_OK;
    PCPNODE pSrcNode = pRootNode;
    PCPNODE pTempNode;
    PCPNODE *ppLast = &_pRootNode;
    
    BusyLock bl(_plSpinLock);
    
    while (pSrcNode)
    {

        pTempNode = new_ne CPNODE;
        if (pTempNode)
        {
            // Chain the node onto the list
            *ppLast = pTempNode;

#ifdef _DEBUG
            pTempNode->dwThreadId = pSrcNode->dwThreadId;
#endif

            pTempNode->cpt = pSrcNode->cpt;
            switch (pSrcNode->cpt)
            {
            case CP_Invalid:
                break;
            case CP_Unknown:
                pTempNode->punkConnector = pSrcNode->punkConnector;
                break;
            case CP_Dispatch:
                pTempNode->pdispConnector = pSrcNode->pdispConnector;
                break;
            case CP_PropertyNotifySink:
                pTempNode->ppnsConnector = pSrcNode->ppnsConnector;
                break;
            }
            
            // Yes, setting it to be NULL, only to over-write on the 
            // next iteration but it's safe and not perf critical
            pTempNode->pNext = NULL; 

            ppLast = &pTempNode->pNext;
            pSrcNode = pSrcNode->pNext;
        }
        else
        {
            Assert (FALSE && "Could not copy connection node list");
            hr = E_OUTOFMEMORY;
            break;
        }
    }

    if (FAILED(hr))
        ReleaseCPNODEList(_pRootNode);

    return hr;
}


HRESULT 
STDMETHODCALLTYPE 
CXMLEnumConnections::Next( 
    ULONG cConnections,
    LPCONNECTDATA rgcd,
    ULONG *pcFetched)
{

    // I have to use returns here because otherwise, in case of error, the 
    // mutex would have to be grabbed (before the two if blocks below, and 
    // then immediately released

    if (cConnections < 1)
    {
        return E_INVALIDARG;
        goto Cleanup;
    }

    if (cConnections > 1 && !pcFetched)
    {
        return E_POINTER;
    }
    
    HRESULT hr = S_OK;
    ULONG i = 0;

    while (i < cConnections && _pCurrentConnection)
    {
        rgcd[i].dwCookie = (DWORD)::InterlockedIncrement(&g_lCookieCount);
        switch (_pCurrentConnection->cpt)
        {
        case CP_Invalid:
            AssertSz(FALSE, "Invalid connection appears in list");
            break;
        case CP_Unknown:
            rgcd[i].pUnk = (IUnknown *)(_pCurrentConnection->punkConnector);
            break;
        case CP_Dispatch:
            rgcd[i].pUnk = (IUnknown *)(_pCurrentConnection->pdispConnector);
            break;
        case CP_PropertyNotifySink:
            rgcd[i].pUnk = (IUnknown *)(_pCurrentConnection->ppnsConnector);
            break;
        }
        rgcd[i].pUnk->AddRef();
        
        i++;
        _pCurrentConnection = _pCurrentConnection->pNext;
    }
    
    if (pcFetched)
        *pcFetched = i;
    
    if (i != cConnections)
        hr = S_FALSE;

Cleanup:

    return hr;
}
    
    
HRESULT 
STDMETHODCALLTYPE 
CXMLEnumConnections::Skip(ULONG cConnections)
{
    int i = cConnections;

    while (_pCurrentConnection && i > 0)
    {
        _pCurrentConnection = _pCurrentConnection->pNext;
        i--;
    }
    
    return (0 == i) ? S_OK : S_FALSE;
}
    
    
HRESULT 
STDMETHODCALLTYPE 
CXMLEnumConnections::Reset()
{
    _pCurrentConnection = _pRootNode;
    return S_OK;
}


HRESULT 
STDMETHODCALLTYPE 
CXMLEnumConnections::Clone(IEnumConnections **ppEnum)
{
    Assert(FALSE && "Call to unimplemted method");
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////
// Helper functions

UINT
CrackVarArgList(
    VARIANTARG **ppva,  // points one past last variant in the array and the array is 
                        // filled backwards by decrementing *ppva then filling variant.
    VARIANTARG *pvaStart,
    va_list args)
/*
    Takes variable arguments of the form

        VARTYPE, value,
        NULL

    and puts them into an array of VARIANTARGs.  Walks backwards through 
    the list.  For example:

        VT_R8, 1.3,
        VT_I4, 1,
        VT_BSTR, bstrSomestring,
        NULL

    will produce an array of VARIANTARGS in reverse order (which is what is 
    needed for IDispatch)
*/
{
    VARTYPE vt;
    UINT cArgs = 0;
    HRESULT hr = S_OK;

    // First get all the arguments and build the VARIANTARGS
    while(TRUE)
    {
        // Get the first arg - this gives us the type
        if ((vt = va_arg(args, VARTYPE)) == 0)
            break;
        
        (*ppva)--;
        VARIANTARG *pva = *ppva;

        if (pva == pvaStart)
        {
            Assert(FALSE && "Too many arguments - increase FIREEVENT_MAXARGS");
            return E_FAIL;
        }

        pva->vt = vt;
        
        switch (pva->vt)
        {
        case VT_I2:
            pva->iVal = va_arg(args, short);
            break;
            
        case VT_I4:
            pva->lVal = va_arg(args, long);
            break;
            
        case VT_INT:
            pva->vt = VT_I4;
            pva->lVal = va_arg(args, int);
            break;
            /*
            case VT_R4:
            // Note that when an argument of type float is passed in a variable
            // argument list, the compiler actually converts the float to a
            // double and pushes the double onto the stack.
            
              V_R4(pva) = float( va_arg(args, double) );
              break;
              
                case VT_R8:
                V_R8(pva) = va_arg(args, double);
                break;
            */
        case VT_BOOL:
            V_BOOL(pva) = va_arg(args, VARIANT_BOOL);
            break;
            
        case VT_BSTR:
            if ( (pva->bstrVal = va_arg(args, LPOLESTR)) &&
                ((pva->bstrVal = SysAllocString(pva->bstrVal)) == NULL) )
                return E_OUTOFMEMORY;
            break;
            
        case VT_DISPATCH:
            V_DISPATCH(pva) = va_arg(args, LPDISPATCH);
            if (V_DISPATCH(pva) != NULL)
                V_DISPATCH(pva)->AddRef();
            break;
            
        case VT_UNKNOWN:
            V_UNKNOWN(pva) = va_arg(args, LPUNKNOWN);
            if (V_UNKNOWN(pva) != NULL)
                V_UNKNOWN(pva)->AddRef();
            break;
            
        case VT_VARIANT:
            VariantInit(pva);
            if (FAILED(hr = VariantCopy(pva, &va_arg(args, VARIANT))))
                return hr;
            break;
            
        default:
            Assert(FALSE && "Unhandled type - please add, or pass it through as a variant");
            // Just ignore a bad param
            (*ppva)++;
            cArgs--;
        }
        
        cArgs++;
    }

    return cArgs;
}

HRESULT 
FireEventThroughCP(
    DISPID dispid,
    PCPNODE pConnections,
    LONG_PTR *plSpinLock,
    ...)
{
    // Quick exit
    if (NULL == pConnections)
        return S_OK;
    
    HRESULT hr = S_OK;
    VARIANTARG rgArgs[FIREEVENT_MAXARGS]; 
    VARIANTARG *pva = &(rgArgs[FIREEVENT_MAXARGS]);
    VARTYPE vt;
    DISPPARAMS dp;
    BOOL fDispArgsInitialized = FALSE;
    UINT cArgs = 0;
    va_list args;
    
    // BusyLock bl(plSpinLock);

    while (pConnections && (SUCCEEDED(hr)))
    {
        if (CP_Dispatch == pConnections->cpt)
        {
            // We only want to set up these args once, and only when needed
            if (!fDispArgsInitialized)
            {
                va_start(args, plSpinLock);
                cArgs = CrackVarArgList(&pva, rgArgs, args);
                // Set up the DISPARAMS structure
                dp.rgvarg = pva;
                dp.cArgs = cArgs;
                dp.rgdispidNamedArgs = NULL;
                dp.cNamedArgs = 0;

                fDispArgsInitialized = TRUE;
            }

            hr = pConnections->pdispConnector->Invoke(
                dispid,
                IID_NULL,
                LOCALE_SYSTEM_DEFAULT,
                DISPATCH_METHOD,
                &dp,
                NULL,  // No result expected
                NULL,  // No exception info
                NULL); // Don't care if an arg has a problem
        }
        else if (pConnections->cpt == CP_PropertyNotifySink)
        {
            pConnections->ppnsConnector->OnChanged(dispid);
        }
        
        pConnections = pConnections->pNext;
    }

    // If we set up the arguments, we need to free them up
    if (fDispArgsInitialized)
    {
        while (cArgs > 0)
        {
            VariantClear(pva++);
            cArgs--;
        }
        
    }

    return hr;
    
}

// Handles firing through IDispatch and through IPropertyNotifySink
HRESULT 
FireEventWithNoArgsThroughCP(
    DISPID dispid,
    PCPNODE pConnections,
    ULONG_PTR *plSpinLock)
{
    // Fast exit
    if (NULL == pConnections)
        return S_OK;

    VARTYPE vt;
    DISPPARAMS dp;
    HRESULT hr = S_OK;

    dp.rgvarg = NULL;
    dp.cArgs = 0;
    dp.rgdispidNamedArgs = NULL;
    dp.cNamedArgs = 0;
    
    // BusyLock bl(plSpinLock);

    while (pConnections && (SUCCEEDED(hr)))
    {
        if (pConnections->cpt == CP_Dispatch)
        {
            hr = pConnections->pdispConnector->Invoke(
                dispid,
                IID_NULL,
                LOCALE_SYSTEM_DEFAULT,
                DISPATCH_METHOD,
                &dp,
                NULL,  // No result expected
                NULL,  // No exception info
                NULL); // Don't care if an arg has a problem
        }
        else if (pConnections->cpt == CP_PropertyNotifySink)
        {
            pConnections->ppnsConnector->OnChanged(dispid);
        }
        
        pConnections = pConnections->pNext;
    }

    return hr;
}

/*
    This function below is used to call functions in script the Trident way.  Basically, when
    a function name is passed in to a function, you are given an IDispatch pointer.  You invoke 
    against it with dispid == DISPID_THIS, and voila !
*/

HRESULT 
FireEventThroughInvoke0(
    VARIANT *pvarRes,
    IDispatch *pDisp,
    IDispatch *pSelf,
    ...)
{
    DISPPARAMS dp;
    VARIANTARG rgva[FIREEVENT_MAXARGS];
    VARIANTARG *pva = &(rgva[FIREEVENT_MAXARGS]);
    DISPID dispidSelf = DISPID_THIS;
    UINT cArgs = 0;
    va_list args;
    HRESULT hr;

    Assert (pDisp && "NULL pDisp is not allowed to this function");
    if (!pDisp)
        return E_POINTER;

    if (pSelf)
    {
        // Set up the first arg, which is "self"
        pva--;
        VariantInit(pva);
        pva->vt = VT_DISPATCH;
        pva->pdispVal = pSelf;
        pSelf->AddRef();
    }
    va_start(args, pSelf);
    cArgs = CrackVarArgList(&pva, rgva, args);

    dp.rgvarg = (cArgs > 0 || pSelf) ? pva : NULL;
    dp.cArgs = cArgs + (pSelf ? 1 : 0);
    dp.rgdispidNamedArgs = pSelf ? &dispidSelf : NULL;
    dp.cNamedArgs = pSelf ? 1 : 0;

    if (pvarRes)
    {
        VariantInit(pvarRes);
    }

    hr = pDisp->Invoke(
        0, // Special Trident trick
        IID_NULL,
        LOCALE_SYSTEM_DEFAULT,
        DISPATCH_METHOD,
        &dp,
        pvarRes,
        NULL,
        NULL);

    while (pva < &(rgva[FIREEVENT_MAXARGS]))
    {
        VariantClear(pva++);
    }

    return hr;
}

// 
// Deletes a linked list of connections, releasing appropriately   
void ReleaseCPNODEList(PCPNODE pRootNode)
{
    PCPNODE pTemp;

    while (pRootNode)
    {
        pTemp = pRootNode;
        pRootNode = pRootNode->pNext;

        delete pTemp;
    }

    return;
}
