//
//  CConnectionPoint
//
//  Common implementation for CConnectionPoint.
//

//
//  Since EnumConnections is called so much, we have a custom
//  enumerator for it which is faster than CStandardEnum and which
//  performs fewer memory allocations.
//

class CConnectionPointEnum : public IEnumConnections
{
public:
    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IEnumConnections methods
    //
    STDMETHOD(Next)(ULONG ccd, LPCONNECTDATA rgcd, ULONG *pcdFetched);
    STDMETHOD(Skip)(ULONG ccd) { return Next(ccd, NULL, NULL); }
    STDMETHOD(Reset)(void) { m_iPos = 0; return S_OK; }
    STDMETHOD(Clone)(IEnumConnections **ppecOut);

    friend HRESULT CConnectionPointEnum_Create(CConnectionPoint *pcp, int iPos, IEnumConnections **pecOut);

private:
    CConnectionPointEnum(CConnectionPoint *pcp, int iPos)
    : m_cRef(1), m_pcp(pcp), m_iPos(iPos) { m_pcp->AddRef(); }

    ~CConnectionPointEnum() { m_pcp->Release(); }

    int m_cRef;                         // refcount
    CConnectionPoint *m_pcp;            // my dad
    int m_iPos;                         // enumeration state
};


//
//  When we need to grow the sink array, we grow by this many.
//
#define GROWTH      8

//
//  OLE says that zero is never a valid cookie, so our cookies are
//  the array index biased by unity.
//
#define COOKIEFROMINDEX(i)      ((i) + 1)
#define INDEXFROMCOOKIE(dw)     ((dw) - 1)


CConnectionPoint::~CConnectionPoint ()
{
    // clean up some memory stuff
    UnadviseAll();
    if (m_rgSinks)
        CoTaskMemFree(m_rgSinks);
}


HRESULT CConnectionPoint::UnadviseAll(void)
{
    if (m_rgSinks)
    {
        int x;

        for (x = 0; x < m_cSinksAlloc; x++)
        {
            ATOMICRELEASE(m_rgSinks[x]);
        }
    }

    return S_OK;
}

//
//  For backwards-compatibility with IE4, our superclass is
//  CIE4ConnectionPoint.
//
STDMETHODIMP CConnectionPoint::QueryInterface(REFIID riid, void **ppvObjOut)
{
    if (IsEqualIID(riid, IID_IConnectionPoint) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObjOut = SAFECAST(this, IConnectionPoint *);
        AddRef();
        return S_OK;
    }

    *ppvObjOut = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP CConnectionPoint::GetConnectionInterface(IID *piid)
{
    *piid = *m_piid;

    return S_OK;
}

STDMETHODIMP CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
    return m_punk->QueryInterface(IID_IConnectionPointContainer, (void **)ppCPC);
}

STDMETHODIMP CConnectionPoint::Advise(IUnknown *pUnk,DWORD *pdwCookie)
{
    HRESULT    hr;
    IUnknown **rgUnkNew;
    IUnknown  *punkTgt;
    int        i = 0;

    if (!pdwCookie)
        return E_POINTER;

    *pdwCookie = 0;

    // first, make sure everybody's got what they thinks they got

    hr = pUnk->QueryInterface(*m_piid, (LPVOID *)&punkTgt);
    if (SUCCEEDED(hr))
    {
#ifdef DEBUG
        //
        //  If we are not an IPropertyNotifySink, then we had better
        //  be derived from IDispatch.  Try to confirm.
        //
        if (m_piid != &IID_IPropertyNotifySink)
        {
            IDispatch *pdisp;
            if (SUCCEEDED(pUnk->QueryInterface(IID_IDispatch, (LPVOID *)&pdisp)))
            {
                pdisp->Release();
            }
            else
            {
                AssertMsg(0, TEXT("CConnectionPoint: IID %08x not derived from IDispatch"), m_piid->Data1);
            }
        }
#endif
    }
    else
    {
        if (m_piid != &IID_IPropertyNotifySink)
        {
            // This is against spec, but raymondc is guessing that this is done
            // for compatibility with VB or some other scripting language that
            // talks IDispatch but not necessarily the IDispatch-derived
            // thingie that we officially speak.  Since we really source
            // merely IDispatch::Invoke, we can satisfactorily accept any
            // IDispatch as a sink.
            hr = pUnk->QueryInterface(IID_IDispatch, (LPVOID*)&punkTgt);
        }
    }

    if (SUCCEEDED(hr))
    {

        // we no longer optimize the case where there is only one sink
        // because it's rarely the case any more.

        //
        //  If the table is full, then grow it.
        //
        if (m_cSinks >= m_cSinksAlloc)
        {
            //  CoTaskMemRealloc is so smart.  If you realloc from NULL, it
            //  means Alloc.  What this means for us?  No special cases!

            rgUnkNew = (IUnknown **)CoTaskMemRealloc(m_rgSinks, (m_cSinksAlloc + GROWTH) * sizeof(IUnknown *));
            if (!rgUnkNew)
            {
                punkTgt->Release();
                // GetLastError();
                return E_OUTOFMEMORY;
            }
            m_rgSinks = rgUnkNew;

            //
            //  OLE does not guarantee that the new memory is zero-initialized.
            //
            ZeroMemory(&m_rgSinks[m_cSinksAlloc], GROWTH * sizeof(IUnknown *));

            m_cSinksAlloc += GROWTH;
        }

        //
        //  Look for an empty slot.  There has to be one since we grew the
        //  table if we were full.
        //
        for (i = 0; m_rgSinks[i]; i++) {
            ASSERT(i < m_cSinksAlloc);
        }

        ASSERT(m_rgSinks[i] == NULL);   // Should've found a free slot
        m_rgSinks[i] = punkTgt;

        *pdwCookie = COOKIEFROMINDEX(i);
        m_cSinks++;

        // notify our owner that someone is connecting to us --
        // they may want to hook something up at the last minute
        //
        IConnectionPointCB* pcb;
        if (SUCCEEDED(m_punk->QueryInterface(IID_IConnectionPointCB, (LPVOID*)&pcb)))
        {
            pcb->OnAdvise(*m_piid, m_cSinks, *pdwCookie);
            pcb->Release();
        }
    }
    else
    {
        hr = CONNECT_E_CANNOTCONNECT;
    }

    return hr;
}

STDMETHODIMP CConnectionPoint::Unadvise(DWORD dwCookie)
{
    if (!dwCookie)
        return S_OK;

    int x = INDEXFROMCOOKIE(dwCookie);

    // Validate the cookie.
    if (x >= m_cSinksAlloc || m_rgSinks[x] == NULL)
        return CONNECT_E_NOCONNECTION;

    // notify our owner that someone is disconnecting from us --
    // they may want to clean up from the OnAdvise call
    // Perform the callback while the sink is still alive, in case
    // the callback wants to do some last-minute communication.
    //
    IConnectionPointCB* pcb;
    if (SUCCEEDED(m_punk->QueryInterface(IID_IConnectionPointCB, (LPVOID*)&pcb)))
    {
        pcb->OnUnadvise(*m_piid, m_cSinks - 1, dwCookie);
        pcb->Release();
    }

    // Free up the slot.  We cannot relocate any elements because that
    // would screw up the outstanding cookies.
    ATOMICRELEASE(m_rgSinks[x]);
    m_cSinks--;

    // Don't free the memory on the loss of the last sink; a new one
    // will probably show up soon.

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CConnectionPoint::EnumConnections
//=--------------------------------------------------------------------------=
// enumerates all current connections
//
// Paramters:
//    IEnumConnections ** - [out] new enumerator object
//
// Output:
//    HRESULT
//
// NOtes:
STDMETHODIMP CConnectionPoint::EnumConnections(IEnumConnections **ppEnumOut)
{
#if 1
    return CConnectionPointEnum_Create(this, 0, ppEnumOut);
#else
    CONNECTDATA *rgConnectData = NULL;
    int i, cSinks;

    // CopyAndAddRefObject assumes that the IUnknown comes first
    // So does CStandardEnum
    COMPILETIME_ASSERT(FIELD_OFFSET(CONNECTDATA, pUnk) == 0);

    cSinks = 0;
    if (_HasSinks())
    {
        // allocate some memory big enough to hold all of the sinks.
        //
        // Must use GlobalAlloc because CStandardEnum uses GlobalFree.
        //
        rgConnectData = (CONNECTDATA *)GlobalAlloc(GMEM_FIXED, m_cSinks * sizeof(CONNECTDATA));
        if (!rgConnectData)
            return E_OUTOFMEMORY;

        // fill in the array
        //
        for (i = 0; i < m_cSinksAlloc; i++)
        {
            if (m_rgSinks[i])
            {
                rgConnectData[cSinks].pUnk = m_rgSinks[i];
                rgConnectData[cSinks].dwCookie = i + 1;
                cSinks++;

                // In case m_rgSinks gets out of sync with m_cSinks,
                // just stop when the array gets full.
                if (cSinks >= m_cSinks)
                {
                    break;
                }
            }
        }
        // Make sure we found all the items we should've found
        ASSERT(cSinks == m_cSinks);
    }

    // create a statndard  enumerator object.
    //
    *ppEnumOut = (IEnumConnections *)(IEnumGeneric *)new CStandardEnum(IID_IEnumConnections,
                       TRUE, cSinks, sizeof(CONNECTDATA), rgConnectData, CopyAndAddRefObject);
    if (!*ppEnumOut)
    {
        CoTaskMemFree(rgConnectData);
        return E_OUTOFMEMORY;
    }

    return S_OK;
#endif
}

//
// CConnectionPoint::DoInvokeIE4
//
// Calls all sinks' IDispatch::Invoke() with Cancel semantics.
HRESULT CConnectionPoint::DoInvokeIE4(LPBOOL pf, LPVOID *ppv, DISPID dispid, DISPPARAMS *pdispparams)
{
    return IConnectionPoint_InvokeWithCancel(this->CastToIConnectionPoint(),
                                    dispid, pdispparams, pf, ppv);
}

//
//  CConnectionPointEnum
//

HRESULT CConnectionPointEnum_Create(CConnectionPoint *pcp, int iPos, IEnumConnections **ppecOut)
{
    *ppecOut = new CConnectionPointEnum(pcp, iPos);
    return *ppecOut ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CConnectionPointEnum::QueryInterface(REFIID riid, void **ppvObjOut)
{
    if (IsEqualIID(riid, IID_IEnumConnections) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObjOut = (IUnknown *)this;
        AddRef();
        return S_OK;
    }

    *ppvObjOut = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CConnectionPointEnum::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CConnectionPointEnum::Release()
{
    ULONG cRef = --m_cRef;
    if (cRef == 0)
        delete this;

    return cRef;
}

//
//  Next also doubles as Skip.  If you pass a NULL output buffer, then
//  nothing gets copied (i.e., you're a Skip).
//
STDMETHODIMP CConnectionPointEnum::Next(ULONG ccd, LPCONNECTDATA rgcd, ULONG *pcdFetched)
{
    ULONG ccdFetched = 0;

    while (ccdFetched < ccd)
    {
        //
        //  Look for the next sink or the end of the array
        //
        while (m_iPos < m_pcp->m_cSinksAlloc && m_pcp->m_rgSinks[m_iPos] == NULL)
        {
            m_iPos++;
        }

        if (m_iPos >= m_pcp->m_cSinksAlloc)
            break;

        if (rgcd)
        {
            //
            //  Copy it to the output buffer
            //
            rgcd->pUnk = m_pcp->m_rgSinks[m_iPos];
            rgcd->dwCookie = COOKIEFROMINDEX(m_iPos);
            rgcd->pUnk->AddRef();
            rgcd++;
        }
        m_iPos++;
        ccdFetched++;
    }

    if (pcdFetched)
        *pcdFetched = ccdFetched;

    return (ccdFetched < ccd) ? S_FALSE : S_OK;
}

//
//  Our clone enumerates the same CConnectionPoint from the same position.
//
STDMETHODIMP CConnectionPointEnum::Clone(IEnumConnections **ppecOut)
{
    return CConnectionPointEnum_Create(m_pcp, m_iPos, ppecOut);
}
