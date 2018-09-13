//
// IConnectionPoint/IDispatch helper functions
//
#include "priv.h"
#include <shlobj.h>

//=============================================================================
//
//  IDispatch helper functions

//-----------------------------------------------------------------------------
//
//  SHPackDispParamsV
//
//  Takes a variable number of parameters for IDispatch, packages
//  them up.
//
//  pdispparams  - The DISPPARAMS structure that receives the result
//                 of the packaging.
//
//  rgvarg       - Array of length cArgs.
//                 It will be used to hold the parameters.
//
//  cArgs        - Number of pairs of generic arguments.
//
//  ap           - va_list of parameters to package.  We package up the
//                 first (2 * cArgs) of them.  See SHPackDispParams
//                 for details.

typedef struct FAKEBSTR {
    ULONG cb;
    WCHAR wsz[1];
} FAKEBSTR;

const FAKEBSTR c_bstrNULL = { 0, L"" };

LWSTDAPI SHPackDispParamsV(DISPPARAMS *pdispparams, VARIANTARG *rgvarg, UINT cArgs, va_list ap)
{
    HRESULT hr = S_OK;

    ZeroMemory(rgvarg, cArgs * SIZEOF(VARIANTARG));

    // fill out DISPPARAMS structure
    pdispparams->rgvarg = rgvarg;
    pdispparams->rgdispidNamedArgs = NULL;
    pdispparams->cArgs = cArgs;
    pdispparams->cNamedArgs = 0;

    // parameters are ordered in ap with the right-most parameter
    // at index zero and the left-most parameter at the highest index; essentially,
    // the parameters are pushed from right to left.  Put the first argument we
    // encounter at the highest index.

    // pVarArg points to the argument structure in the array we are currently
    // filling in.  Initialize this to point at highest argument (zero-based,
    // hence the -1).  For each passed-in argument we process, *decrement*
    // the pVarArg pointer to achieve the "push from right-to-left" effect.
    VARIANTARG * pVarArg = &rgvarg[cArgs - 1];

    int nCount = cArgs;
    while (nCount) 
    {
        VARENUM vaType = va_arg(ap,VARENUM);

        // We don't have to call VariantInit because we zerod out
        // the entire array before entering this loop

        V_VT(pVarArg) = vaType;

        // the next field is a union, so we can be smart about filling it in
        //
        if (vaType & VT_BYREF)
        {
            // All byrefs can be packed the same way
            V_BYREF(pVarArg) = va_arg(ap, LPVOID);
        }
        else
        {
            switch (vaType)
            {
            case VT_BSTR:
            {
                // parameter is a BSTR
                // MFC doesn't like it when you pass NULL for a VT_BSTR type
                V_BSTR(pVarArg) = va_arg(ap, BSTR);
                if (V_BSTR(pVarArg) == NULL)
                    V_BSTR(pVarArg) =(BSTR)c_bstrNULL.wsz;
#ifdef DEBUG
                // Check if this BSTR is a valid BSTR
                FAKEBSTR *bstr = CONTAINING_RECORD(V_BSTR(pVarArg), FAKEBSTR, wsz);
                ASSERT(bstr->cb == lstrlenW(bstr->wsz) * SIZEOF(WCHAR));
#endif
                break;
            }
    
            case VT_BOOL:
                V_BOOL(pVarArg) = va_arg(ap, VARIANT_BOOL);
                break;

            case VT_DISPATCH:
                V_DISPATCH(pVarArg) = va_arg(ap, LPDISPATCH);
                break;

            case VT_UNKNOWN:
                V_UNKNOWN(pVarArg) = va_arg(ap, LPUNKNOWN);
                break;

            default:
                AssertMsg(0, TEXT("Packing unknown variant type 0x%x as VT_I4"), vaType);
                // if we don't know what it is treat it as VT_I4.
                // Hopefully it's not a pointer or a VT_R8 or that sort of
                // thing, or we're screwed.
                V_VT(pVarArg) = VT_I4;

            case VT_I4:
                V_I4(pVarArg) = va_arg(ap, LONG);
                break;

            } 
        }

        nCount--;
        pVarArg--;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//
//  SHPackDispParams
//
//  Takes a variable number of generic parameters, packages
//  them up.
//
//  pdispparams  - The DISPPARAMS structure that receives the result
//                 of the packaging.
//
//  rgvarg       - Array of length cArgs.
//                 It will be used to hold the parameters.
//
//  cArgs        - Number of pairs of generic arguments (below).
//
//  ...          - A collection of (VARNUM, LPVOID) pairs of arguments.
//                 The first is the type of the argument, and the
//                 second is the corresponding value.
//
//                 As a special case, a null VT_BSTR can be passed
//                 as a NULL pointer and we will turn it into a
//                 genuine null BSTR.
//
//  The following VARENUMs are supported:
//
//      VT_BYREF        - Anything that is VT_BYREF is okay
//      VT_BSTR
//      VT_BOOL
//      VT_DISPATCH
//      VT_UNKNOWN
//      VT_I4
//
//  Any other type will be packaged randomly, so don't do that.
//
//  Example:
//
//      DISPPARAMS dispparams;
//      VARIANTARG args[4];                     // room for 4 parameters
//      SHPackDispParams(&dispparams, args, 4,  // and here they are
//                       VT_BSTR,   bstrURL,
//                       VT_I4,     dwFlags,
//                       VT_BSTR,   NULL,       // no post data
//                       VT_BSTR,   bstrHeaders);
//

LWSTDAPI SHPackDispParams(DISPPARAMS *pdispparams, VARIANTARG *rgvarg, UINT cArgs, ...)
{
    va_list ap;
    va_start(ap, cArgs);

    HRESULT hr = SHPackDispParamsV(pdispparams, rgvarg, cArgs, ap);

    va_end(ap);
    return hr;
}

//=============================================================================
//
//  IConnectionPoint helper functions


//-----------------------------------------------------------------------------
//
//  INVOKECALLBACK
//
//  Allows clients to customize the the invoke process.  The callback
//  receives the following parameters:
//
//  pdisp       - The IDispatch that is about to receive an invoke.
//
//  pinv        - SHINVOKEPARAMS structure that describes the invoke
//                that is about to occur.
//
//  The callback function is called before each sink is dispatched.
//  The callback can return any of the following values:
//
//  S_OK          Proceed with the invoke
//  S_FALSE       Skip this invoke but keep invoking others
//  E_FAIL        Stop invoking
//
//  A client can do lazy-evaluation of dispatch arguments by installing
//  a callback that sets up the dispatch arguments on the first callback.
//
//  A client can support a "Cancel" flag by returning E_FAIL once the
//  cancel has occurred.
//
//  A client can pre-validate an IDispatch for compatibility reasons
//  and either touch up the arguments and return S_OK, or decide that
//  the IDispatch should be skipped and return S_FALSE.
//
//  A client can append custom information to the end of the SHINVOKEPARAMS
//  structure to allow it to determine additional context.
//
//  A client can do post-invoke goo by doing work on the pre-invoke
//  of the subsequent callback (plus one final bout of work when the
//  entire enumeration completes).
//

//-----------------------------------------------------------------------------
//
//  GetConnectionPointSink
//
//  Obtaining a connection point sink is supposed to be easy.  You just
//  QI for the interface.  Unfortunately, too many components are buggy.
//
//  mmc.exe is stupid and faults if you QI for IDispatch
//  and punkCB is non-NULL.  And if you do pass in NULL,
//  it returns S_OK but fills punkCB with NULL anyway.
//  Somebody must've had a rough day.
//
//  Java responds only to its dispatch ID and not IID_IDispatch, even
//  though the dispatch ID is derived from IID_IDispatch.
//
//  The Explorer Band responds only to IID_IDispatch and not to
//  the dispatch ID.
//

HRESULT GetConnectionPointSink(IUnknown *pUnk, const IID *piidCB, IUnknown **ppunkCB)
{
    HRESULT hr = E_NOINTERFACE;
    *ppunkCB = NULL;                // Pre-zero it to work around MMC
    if (piidCB)                     // Optional interface (Java/ExplBand)
    {                   
        hr = pUnk->QueryInterface(*piidCB, (void **) ppunkCB);
        if (*ppunkCB == NULL)       // Clean up behind MMC
            hr = E_NOINTERFACE;
    }
    return hr;
}


//-----------------------------------------------------------------------------
//
//  EnumConnectionPointSinks
//
//  Enumerate the connection point sinks, calling the callback for each one
//  found.
//
//  The callback function is called once for each sink.  The IUnknown is
//  whatever interface we could get from the sink (either piidCB or piidCB2).
//

typedef HRESULT (CALLBACK *ENUMCONNECTIONPOINTSPROC)(
    /* [in, iid_is(*piidCB)] */ IUnknown *psink, LPARAM lParam);

HRESULT EnumConnectionPointSinks(
    IConnectionPoint *pcp,              // IConnectionPoint victim
    const IID *piidCB,                  // Interface for callback
    const IID *piidCB2,                 // Alternate interface for callback
    ENUMCONNECTIONPOINTSPROC EnumProc,  // Callback procedure
    LPARAM lParam)                      // Refdata for callback
{
    HRESULT hr;
    IEnumConnections * pec;

    if (pcp)
        EVAL(SUCCEEDED(hr = pcp->EnumConnections(&pec)));
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr))
    {
        CONNECTDATA cd;
        ULONG cFetched;

        while (S_OK == (hr = pec->Next(1, &cd, &cFetched)))
        {
            IUnknown *punkCB;

            ASSERT(1 == cFetched);

            hr = GetConnectionPointSink(cd.pUnk, piidCB, &punkCB);
            if (FAILED(hr))
                hr = GetConnectionPointSink(cd.pUnk, piidCB2, &punkCB);

            if (EVAL(SUCCEEDED(hr)))
            {
                hr = EnumProc(punkCB, lParam);
                punkCB->Release();
            }
            else
            {
                hr = S_OK;      // Pretend callback succeeded
            }
            cd.pUnk->Release();
            if (FAILED(hr)) break; // Callback asked to stop
        }
        pec->Release();
        hr = S_OK;
    }

    return hr;
}

//-----------------------------------------------------------------------------
//
//  InvokeCallback
//
//  Send out the callback (if applicable) and then do the invoke if the
//  callback said that was a good idea.
//
//  Parameters:
//
//      pcp          -  IConnectionPoint whose sinks are to be Invoke()d.
//                      If this parameter is NULL, the function does nothing.
//      pinv         -  Structure containing parameters to INVOKE.

HRESULT CALLBACK EnumInvokeCallback(IUnknown *psink, LPARAM lParam)
{
    IDispatch *pdisp = (IDispatch *)psink;
    LPSHINVOKEPARAMS pinv = (LPSHINVOKEPARAMS)lParam;
    HRESULT hr;

    if (pinv->Callback)
    {
        // Now see if the callback wants to do pre-vet the pdisp.
        // It can return S_FALSE to skip this callback or E_FAIL to
        // stop the invoke altogether
        hr = pinv->Callback(pdisp, pinv);
        if (hr != S_OK) return hr;
    }

    pdisp->Invoke(pinv->dispidMember, *pinv->piid, pinv->lcid,
                  pinv->wFlags, pinv->pdispparams, pinv->pvarResult,
                  pinv->pexcepinfo, pinv->puArgErr);

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  IConnectionPoint_InvokeIndirect
//
//  Given a connection point, call the IDispatch::Invoke for each
//  connected sink.
//
//  The return value merely indicates whether the command was dispatched.
//  If any particular sink fails the IDispatch::Invoke, we will still
//  return S_OK, since the command was indeed dispatched.
//
//  Parameters:
//
//      pcp          -  IConnectionPoint whose sinks are to be Invoke()d.
//                      If this parameter is NULL, the function does nothing.
//      pinv         -  Structure containing parameters to INVOKE.
//                      The pdispparams field can be NULL; we will turn it
//                      into a real DISPPARAMS for you.
//
//  The SHINVOKEPARAMS.flags field can contain the following flags.
//
//      IPFL_USECALLBACK    - The callback field contains a callback function
//                            Otherwise, it will be set to NULL.
//      IPFL_USEDEFAULT     - Many fields in the SHINVOKEPARAMS will be set to
//                            default values to save the caller effort:
//
//                  riid            =   IID_NULL
//                  lcid            =   0
//                  wFlags          =   DISPATCH_METHOD
//                  pvarResult      =   NULL
//                  pexcepinfo      =   NULL
//                  puArgErr        =   NULL
//

LWSTDAPI IConnectionPoint_InvokeIndirect(
    IConnectionPoint *pcp,
    SHINVOKEPARAMS *pinv)
{
    HRESULT hr;
    DISPPARAMS dp = { 0 };
    IID iidCP;

    if (pinv->pdispparams == NULL)
        pinv->pdispparams = &dp;

    if (!(pinv->flags & IPFL_USECALLBACK))
    {
        pinv->Callback = NULL;
    }

    if (pinv->flags & IPFL_USEDEFAULTS)
    {
        pinv->piid            =  &IID_NULL;
        pinv->lcid            =   0;
        pinv->wFlags          =   DISPATCH_METHOD;
        pinv->pvarResult      =   NULL;
        pinv->pexcepinfo      =   NULL;
        pinv->puArgErr        =   NULL;
    }

    // Try both the interface they actually connected on,
    // as well as IDispatch.  Apparently Java responds only to
    // the connecting interface, and ExplBand responds only to
    // IDispatch, so we have to try both.  (Sigh.  Too many buggy
    // components in the system.)

    hr = EnumConnectionPointSinks(pcp,
                                  (pcp->GetConnectionInterface(&iidCP) == S_OK) ? &iidCP : NULL,
                                  &IID_IDispatch,
                                  EnumInvokeCallback,
                                  (LPARAM)pinv);

    // Put the original NULL back so the caller can re-use the SHINVOKEPARAMS.
    if (pinv->pdispparams == &dp)
        pinv->pdispparams = NULL;

    return hr;
}

//-----------------------------------------------------------------------------
//
//  IConnectionPoint_InvokeWithCancel
//
//  Wrapper around IConnectionPoint_InvokeIndirect with special Cancel
//  semantics.
//
//  Parameters:
//
//      pcp          -  IConnectionPoint whose sinks are to be Invoke()d.
//                      If this parameter is NULL, the function does nothing.
//      dispid       -  The DISPID to invoke
//      pdispparams  -  The DISPPARAMS for the invoke
//      pfCancel     -  Optional BOOL to cancel the invoke
//      ppvCancel    -  Optional LPVOID to cancel the invoke
//
//  If either *pfCancel or *ppvCancel is nonzero/non-NULL, we stop the invoke
//  process.  This allows a sink to "handle" the event and prevent other
//  sinks from receiving it.  The ppvCancel parameter is for dispid's which
//  are queries that are asking for somebody to create an object and return it.
//
//  It is the caller's responsibility to check the values of *pfCancel
//  and/or *ppvCancel to determine if the operation was cancelled.
//

typedef struct INVOKEWITHCANCEL {
    SHINVOKEPARAMS inv;
    LPBOOL pfCancel;
    void **ppvCancel;
} INVOKEWITHCANCEL;

HRESULT CALLBACK InvokeWithCancelProc(IDispatch *psink, SHINVOKEPARAMS *pinv)
{
    INVOKEWITHCANCEL *piwc = CONTAINING_RECORD(pinv, INVOKEWITHCANCEL, inv);

    if ((piwc->pfCancel && *piwc->pfCancel) ||
        (piwc->ppvCancel && *piwc->ppvCancel))
        return E_FAIL;

    return S_OK;
}

LWSTDAPI IConnectionPoint_InvokeWithCancel(
    IConnectionPoint *pcp,
    DISPID dispidMember,
    DISPPARAMS * pdispparams,
    LPBOOL pfCancel,
    void **ppvCancel)
{
    INVOKEWITHCANCEL iwc;

    iwc.inv.flags = IPFL_USECALLBACK | IPFL_USEDEFAULTS;
    iwc.inv.dispidMember = dispidMember;
    iwc.inv.pdispparams = pdispparams;
    iwc.inv.Callback = InvokeWithCancelProc;
    iwc.pfCancel = pfCancel;
    iwc.ppvCancel = ppvCancel;

    return IConnectionPoint_InvokeIndirect(pcp, &iwc.inv);
}

//-----------------------------------------------------------------------------
//
//  IConnectionPoint_SimpleInvoke
//
//  Wrapper around IConnectionPoint_InvokeIndirect with IPFL_USEDEFAULTS.
//

LWSTDAPI IConnectionPoint_SimpleInvoke(IConnectionPoint *pcp, DISPID dispidMember, DISPPARAMS *pdispparams)
{
    SHINVOKEPARAMS inv;

    inv.flags = IPFL_USEDEFAULTS;
    inv.dispidMember = dispidMember;
    inv.pdispparams = pdispparams;

    return IConnectionPoint_InvokeIndirect(pcp, &inv);
}

//
//  Takes a variable number of parameters for IDispatch, packages
//  them up, and invokes them.
//
//  The parameters to the IDispatch::Invoke will be
//
//      dispidMember    -   dispidMember
//      riid            -   IID_NULL
//      lcid            -   0
//      wFlags          -   DISPATCH_METHOD
//      pdispparams     -   <parameters to this function>
//      pvarResult      -   NULL
//      pexcepinfo      -   NULL
//      puArgErr        -   NULL
//
//  The parameters to this function are
//
//  pcp          - IConnectionPoint whose sinks should be Invoke()d.
//                 If this parameter is NULL, the function does nothing.
//  dispidMember - The DISPID to invoke.
//  rgvarg       - Array of length cArgs.
//                 It will be used to hold the parameters.
//  cArgs        - Number of pairs of generic arguments (below).
//
//  ap           - va_list of parameters to package.  We package up the
//                 first (2 * cArgs) of them.  See SHPackDispParams
//                 for details.
//

LWSTDAPI IConnectionPoint_InvokeParamV(IConnectionPoint *pcp, DISPID dispidMember, 
                                       VARIANTARG *rgvarg, UINT cArgs, va_list ap)
{
    HRESULT hr;

    if (pcp)
    {
        DISPPARAMS dp;
        hr = SHPackDispParamsV(&dp, rgvarg, cArgs, ap);
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = IConnectionPoint_SimpleInvoke(pcp, dispidMember, &dp);
        }
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}

//
//  Worker function for many classes of IConnectionPoint_Invoke
//  clients.  Just pass the parameters and we'll pack them up for you.
//
//  The parameters to the IDispatch::Invoke will be
//
//      dispidMember    -   dispidMember
//      riid            -   IID_NULL
//      lcid            -   0
//      wFlags          -   DISPATCH_METHOD
//      pdispparams     -   <parameters to this function>
//      pvarResult      -   NULL
//      pexcepinfo      -   NULL
//      puArgErr        -   NULL
//
//  The parameters to this function are
//
//  pcp          - IConnectionPoint whose sinks should be Invoke()d.
//                 If this parameter is NULL, the function does nothing.
//
//  dispidMember - The DISPID to invoke.
//
//  rgvarg       - Array of length cArgs.
//                 It will be used to hold the parameters.
//
//  cArgs        - Number of pairs of generic arguments (below).
//
//  ...          - A collection of (VARENUM, blah) pairs of arguments.
//                 See SHPackDispParams for details.
//
//  Example:
//
//      VARIANTARG args[1];
//      IConnectionPoint_InvokeParam(pcp, DISPID_PROPERTYCHANGE,
//                                   args, 1,
//                                   VT_BSTR, bstrProperty);

LWSTDAPIV IConnectionPoint_InvokeParam(IConnectionPoint *pcp, DISPID dispidMember, 
                                       VARIANTARG *rgvarg, UINT cArgs, ...)
{
    HRESULT hr;
    va_list ap;

    va_start(ap, cArgs);

    hr = IConnectionPoint_InvokeParamV(pcp, dispidMember, rgvarg, cArgs, ap);

    va_end(ap);

    return hr;
}

//
//  Given a connection point that represents IPropertyNotifySink,
//  call the IPropertyNotifySink::OnChanged for each connected sink.
//
//  Parameters:
//
//      pcp          -  IConnectionPoint whose sinks are to be notified.
//                      If this parameter is NULL, the function does nothing.
//      dispid       -  To pass to IPropertyNotifySink::OnChanged.

HRESULT CALLBACK OnChangedCallback(IUnknown *psink, LPARAM lParam)
{
    IPropertyNotifySink *pns = (IPropertyNotifySink *)psink;
    DISPID dispid = (DISPID)lParam;

    pns->OnChanged(dispid);

    return S_OK;
}

LWSTDAPI IConnectionPoint_OnChanged(IConnectionPoint *pcp, DISPID dispid)
{
#ifdef DEBUG
    // Make sure it really is an IPropertyNotifySink connection point.
    if (pcp)
    {
        IID iid;
        HRESULT hr = pcp->GetConnectionInterface(&iid);
        ASSERT(SUCCEEDED(hr) && iid == IID_IPropertyNotifySink);
    }
#endif
    return EnumConnectionPointSinks(pcp, &IID_IPropertyNotifySink, NULL,
                                    OnChangedCallback, (LPARAM)dispid);
}

//=============================================================================
//
//  IConnectionPointContainer helper functions

//
//  QI's for IConnectionPointContainer and then does the FindConnectionPoint.
//
//  Parameters:
//
//      punk         -  The object who might be an IConnectionPointContainer.
//                      This parameter may be NULL, in which case the
//                      operation fails.
//      riidCP       -  The connection point interface to locate.
//      pcpOut       -  Receives the IConnectionPoint, if any.

LWSTDAPI IUnknown_FindConnectionPoint(IUnknown *punk, REFIID riidCP, 
                                      IConnectionPoint **pcpOut)
{
    HRESULT hr;

    *pcpOut = NULL;

    if (punk)
    {
        IConnectionPointContainer *pcpc;
        hr = punk->QueryInterface(IID_IConnectionPointContainer, (void **)&pcpc);
        if (SUCCEEDED(hr))
        {
            hr = pcpc->FindConnectionPoint(riidCP, pcpOut);
            pcpc->Release();
        }
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}

//
//  Given an IUnknown, query for its connection point container,
//  find the corresponding connection point, package up the
//  invoke parameters, and call the IDispatch::Invoke for each
//  connected sink.
//
//  See IConnectionPoint_InvokeParam for additional semantics.
//
//  Parameters:
//
//      punk         -  Object that might be an IConnectionPointContainer
//      riidCP       -  ConnectionPoint interface to request
//      pinv         -  Arguments for the Invoke.
//

LWSTDAPI IUnknown_CPContainerInvokeIndirect(IUnknown *punk, REFIID riidCP,
                SHINVOKEPARAMS *pinv)
{
    IConnectionPoint *pcp;
    HRESULT hr = IUnknown_FindConnectionPoint(punk, riidCP, &pcp);
    if (SUCCEEDED(hr))
    {
        hr = IConnectionPoint_InvokeIndirect(pcp, pinv);
        pcp->Release();
    }
    return hr;
}

//
//  This is the ultimate in one-stop shopping.
//
//  Given an IUnknown, query for its connection point container,
//  find the corresponding connection point, package up the
//  invoke parameters, and call the IDispatch::Invoke for each
//  connected sink.
//
//  See IConnectionPoint_InvokeParam for additional semantics.
//
//  Parameters:
//
//      punk         -  Object that might be an IConnectionPointContainer
//      riidCP       -  ConnectionPoint interface to request
//      dispidMember -  The DISPID to invoke.
//      rgvarg       -  Array of length cArgs.
//                      It will be used to hold the parameters.
//      cArgs        -  Number of pairs of generic arguments (below).
//      ...          -  A collection of (VARNUM, LPVOID) pairs of arguments.
//                      See SHPackDispParams for details.
//
//  Example:
//
//      IUnknown_CPContainerInvokeParam(punk, DIID_DShellFolderViewEvents,
//                                      DISPID_SELECTIONCHANGED, NULL, 0);

LWSTDAPIV IUnknown_CPContainerInvokeParam(
    IUnknown *punk, REFIID riidCP,
    DISPID dispidMember, VARIANTARG *rgvarg, UINT cArgs, ...)
{
    IConnectionPoint *pcp;
    HRESULT hr = IUnknown_FindConnectionPoint(punk, riidCP, &pcp);

    if (SUCCEEDED(hr))
    {
        va_list ap;
        va_start(ap, cArgs);
        hr = IConnectionPoint_InvokeParamV(pcp, dispidMember, rgvarg, cArgs, ap);
        va_end(ap);
        pcp->Release();
    }

    return hr;
}

//
//  Given an IUnknown, query for its connection point container,
//  find the corresponding connection point, and call the
//  IPropertyNotifySink::OnChanged for each connected sink.
//
//  Parameters:
//
//      punk         -  Object that might be an IConnectionPointContainer
//      dispid       -  To pass to IPropertyNotifySink::OnChanged.

LWSTDAPI IUnknown_CPContainerOnChanged(IUnknown *punk, DISPID dispid)
{
    IConnectionPoint *pcp;
    HRESULT hr = IUnknown_FindConnectionPoint(punk, IID_IPropertyNotifySink, &pcp);
    if (SUCCEEDED(hr))
    {
        hr = IConnectionPoint_OnChanged(pcp, dispid);
        pcp->Release();
    }
    return hr;
}
