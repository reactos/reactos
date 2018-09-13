#ifndef __CNCTNPT_H__
#define __CNCTNPT_H__

//
//  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//
//  This class is shared between DLLs, and DLLs that use it have already
//  shipped as part of IE4 (specifically, shell32).  This means that
//  any changes you make must be done EXTREMELY CAREFULLY and TESTED
//  FOR INTEROPERABILITY WITH IE4!  For one thing, you have to make sure
//  that none of your changes alter the vtbl used by IE4's shell32.
//
//  If you change CIE4ConnectionPoint, you must build SHDOC401 and
//  test it on IE4!
//
//  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//

//
//  First, the class as it was defined in IE4.  All virtual functions
//  must be be listed in exactly the same order as they were in IE4.
//  Fortunately, no cross-component users mucked with the member
//  variables.
//
//  Change any of these at your own risk.
//
class CIE4ConnectionPoint : public IConnectionPoint {

public:
    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) PURE;
    virtual STDMETHODIMP_(ULONG) AddRef(void) PURE;
    virtual STDMETHODIMP_(ULONG) Release(void) PURE;

    // IConnectionPoint methods
    //
    virtual STDMETHODIMP GetConnectionInterface(IID FAR* pIID) PURE;
    virtual STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer FAR* FAR* ppCPC) PURE;
    virtual STDMETHODIMP Advise(LPUNKNOWN pUnkSink, DWORD FAR* pdwCookie) PURE;
    virtual STDMETHODIMP Unadvise(DWORD dwCookie) PURE;
    virtual STDMETHODIMP EnumConnections(LPENUMCONNECTIONS FAR* ppEnum) PURE;

    // This is how you actually fire the events
    // Those called by shell32 are virtual
    // (Renamed to DoInvokeIE4)
    virtual HRESULT DoInvokeIE4(LPBOOL pf, LPVOID *ppv, DISPID dispid, DISPPARAMS *pdispparams) PURE;

    // This helper function does work that callers of DoInvoke often need done
    virtual HRESULT DoInvokePIDLIE4(DISPID dispid, LPCITEMIDLIST pidl, BOOL fCanCancel) PURE;

};

//
// CConnectionPoint is an implementation of a conection point.
// To get the rest of the implementation, you also have to include
// lib\cnctnpt.cpp in your project.
//
// Embed an instance of CConnectionPoint in an object that needs to
// implement a connectionpoint and call SetOwner to initialize it.
//
// Fire events to anyone connected to this connectionpoint via DoInvoke
// or DoOnChanged.  External clients should use the shlwapi functions
// like IConnectionPoint_Invoke or IConnectionPoint_OnChanged.
//

class CConnectionPoint : public CIE4ConnectionPoint {
    friend class CConnectionPointEnum;

public:
    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void)
        { return m_punk->AddRef(); }
    virtual STDMETHODIMP_(ULONG) Release(void)
        { return m_punk->Release(); }

    // IConnectionPoint methods
    //
    virtual STDMETHODIMP GetConnectionInterface(IID * pIID);
    virtual STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer ** ppCPC);
    virtual STDMETHODIMP Advise(LPUNKNOWN pUnkSink, DWORD * pdwCookie);
    virtual STDMETHODIMP Unadvise(DWORD dwCookie);
    virtual STDMETHODIMP EnumConnections(LPENUMCONNECTIONS * ppEnum);

    // CIE4ConnectionPoint methods - called by IE4's shell32
    virtual HRESULT DoInvokeIE4(LPBOOL pf, LPVOID *ppv, DISPID dispid, DISPPARAMS *pdispparams);

    // DoInvokePidlIE4 is strange in that shell32 linked to it but never
    // actually called it.  This makes the implementation particularly simple.
    virtual HRESULT DoInvokePIDLIE4(DISPID dispid, LPCITEMIDLIST pidl, BOOL fCanCancel)
    { return E_NOTIMPL; }

public:
    // Additional helper methods

    // Performs a basic DISPID Invoke on the object
    inline HRESULT InvokeDispid(DISPID dispid) {
        return IConnectionPoint_SimpleInvoke(this, dispid, NULL);
    }

    // Performs an OnChanged on the object
    inline HRESULT OnChanged(DISPID dispid) {
        return IConnectionPoint_OnChanged(this, dispid);
    }

    // A couple functions to setup and destroy this subclass object
    ~CConnectionPoint(); // not virtual: nobody inherits from this class

    //
    //  The containing object must call SetOwner to initialize the
    //  connection point.
    //
    //  punk - The IUnknown of the object this ConnectionPoint is
    //         embedded in; it will be treated as the connection
    //         point container.
    //
    //  piid - The IID that the sinks are expected to support.
    //         If you call DoInvoke, then it must be derived from
    //         IID_IDispatch.  If you call DoOnChanged, then it must
    //         be exactly &IID_IPropertyNotifySink.
    //
    void SetOwner(IUnknown* punk, const IID* piid)
        {
            // Validate the special requirement on the piid parameter.
            if (*piid == IID_IPropertyNotifySink)
            {
                ASSERT(piid == &IID_IPropertyNotifySink);
            }

            // don't AddRef -- we're a member variable of the object punk points to
            m_punk = punk;
            m_piid = piid;
        }

    // The underline version is inline
    BOOL _HasSinks() { return (BOOL)m_cSinks; }

    // We are empty if there are no sinks
    BOOL IsEmpty() { return !_HasSinks(); }

    HRESULT UnadviseAll(void);

    // A lot of people need to convert a CConnectionPoint into an
    // IConnectionPoint.  We used to be multiply inherited, hence the
    // need for this member, but that's gone now.
    IConnectionPoint *CastToIConnectionPoint()
        { return SAFECAST(this, IConnectionPoint*); }

private:
    IUnknown **m_rgSinks;
    int m_cSinks;
    int m_cSinksAlloc;

    IUnknown *m_punk;   // IUnknown of object containing us
    const IID *m_piid;  // IID of this connection point
};

#endif
