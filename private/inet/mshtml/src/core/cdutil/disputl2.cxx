//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dsputil2.cxx
//
//  Contents:   More dispatch utilities.
//
//
//  History:
//              5-22-95     kfl     converted WCHAR to TCHAR
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

DeclareTag(tagLoadTypeInfo, "TypeInfo", "Trace typeinfo load")
MtDefine(LoadTypeLib, PerfPigs, "Loading type-library MSHTML.TLB")

//+----------------------------------------------------------------------------
//
//  Member:     GetTypeInfoFromCoClass
//
//  Synopsis:   Return either the default dispinterface or default source
//              interface ITypeInfo from a coclass
//
//  Arguments:  pTICoClass - ITypeInfo for containing coclass
//              fSource    - Return either source (TRUE) or default (FALSE) interface
//              ppTI       - Location at which to return the interface (may be NULL)
//              piid       - Location at which to return ther interface IID (may be NULL)
//
//  Returns:    S_OK, E_xxxx
//
//-----------------------------------------------------------------------------
HRESULT
GetTypeInfoFromCoClass(
    ITypeInfo *     pTICoClass,
    BOOL            fSource,
    ITypeInfo **    ppTI,
    IID *           piid)
{
    ITypeInfo * pTI = NULL;
    TYPEATTR *  pTACoClass = NULL;
    TYPEATTR *  pTA = NULL;
    IID         iid;
    HREFTYPE    href;
    int         i;
    int         flags;
    HRESULT     hr;

    Assert(pTICoClass);

    if (!ppTI)
        ppTI = &pTI;
    if (!piid)
        piid = &iid;

    *ppTI = NULL;
    *piid = IID_NULL;

    hr = THR(pTICoClass->GetTypeAttr(&pTACoClass));
    if (hr)
        goto Cleanup;
    Assert(pTACoClass->typekind == TKIND_COCLASS);

    for (i = 0; i < pTACoClass->cImplTypes; i++)
    {
        hr = THR(pTICoClass->GetImplTypeFlags(i, &flags));
        if (hr)
            goto Cleanup;

        if ((flags & IMPLTYPEFLAG_FDEFAULT) &&
            ((fSource && (flags & IMPLTYPEFLAG_FSOURCE)) ||
             (!fSource && !(flags & IMPLTYPEFLAG_FSOURCE))))
        {
            hr = THR(pTICoClass->GetRefTypeOfImplType(i, &href));
            if (hr)
                goto Cleanup;

            hr = THR(pTICoClass->GetRefTypeInfo(href, ppTI));
            if (hr)
                goto Cleanup;

            hr = THR((*ppTI)->GetTypeAttr(&pTA));
            if (hr)
                goto Cleanup;

            *piid = pTA->guid;
            goto Cleanup;
        }
    }

    hr = E_FAIL;

Cleanup:
    if (pTA)
    {
        Assert(*ppTI);
        (*ppTI)->ReleaseTypeAttr(pTA);
    }
    ReleaseInterface(pTI);
    if (pTACoClass)
    {
        Assert(pTICoClass);
        pTICoClass->ReleaseTypeAttr(pTACoClass);
    }
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//  
//  Function:   GetFormsTypeLibPath
//  
//  Synopsis:   Returns the path to a forms type library, either the
//              raw unadorned type library or the special merged
//              version, depending on dwFlags.
//  
//  Arguments:  [ach]       
//  
//  Returns:    HRESULT
//  
//-------------------------------------------------------------------------

void
GetFormsTypeLibPath(TCHAR * ach)
{
    _tcscpy(ach, g_achDLLCore);
}



//+---------------------------------------------------------------------------
//
//  Function:   GetFormsTypeLib
//
//  Synopsis:   Get cached Forms3 type library.
//
//              Note: Not only does it seem like a good idea to
//              to cache the type library, it gets around a suspected
//              bug in LoadTypeLib.  I (garybu) have observed LoadTypeLib
//              returning a bogus object when called repeatedly.
//
//  Arguments:  ppTL        The type library
//              fNoCache    Don't cache the library
//
//----------------------------------------------------------------------------

HRESULT
GetFormsTypeLib(ITypeLib **ppTL, BOOL fNoCache)
{
    THREADSTATE *   pts;
    HRESULT         hr;
    TCHAR           ach[MAX_PATH];
    TCHAR *         pchName;

    pts = GetThreadState();

    if (pts->pTLCache)
    {
        *ppTL = pts->pTLCache;
        (*ppTL)->AddRef();
        return S_OK;
    }

    TraceTag((tagPerf, "Loading Trident Type Library."));

    //BUGBUG (carled) again the crt library shutdown causes oleaut32 to leak memory
    // which is allocated on the very first call to GetFormsTypeLibPath. Remove this 
    // block once the crt libraries are no longer linked in.

    DbgMemoryTrackDisable(TRUE);

    GetFormsTypeLibPath(ach);
#ifndef UNIX // UNIX can use the dll name to find out thd tlb file.
    pchName = _tcsrchr(ach, '.');
    Assert(pchName);
    _tcscpy(pchName + 1, _T("tlb"));
#endif
    hr = THR(LoadTypeLib(ach, ppTL));

    DbgMemoryTrackDisable(FALSE);

    if (hr)
        goto Cleanup;

    MtAdd(Mt(LoadTypeLib), +1, 0);

    // Cache the library if requested and if our DllThreadPassivate will
    // execute (since it is through DllThreadPassivate the library is released)
    // (Some code, such as class initialization code, may load the type library
    //  before any objects have been created)
    if (!fNoCache && GetPrimaryObjectCount())
    {
        pts->pTLCache = *ppTL;
        (*ppTL)->AddRef();
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   DeinitTypeLibCache
//
//  Synopsis:   Release the forms type lib cache.
//
//----------------------------------------------------------------------------

void
DeinitTypeLibCache(THREADSTATE *pts)
{
    Assert(pts);
    ClearInterface(&pts->pTLCache);

    // There is a known bug in NT 4.0 versions prior to Service Pack 3
    // (build 1381) with freeing the typelib for OLEAUT.  We purposely leak
    // the typelib in this case.

    extern DWORD g_dwPlatformServicePack;

    if (    g_dwPlatformID != VER_PLATFORM_WIN32_NT
        ||  HIWORD(g_dwPlatformVersion) != 4
        ||  LOWORD(g_dwPlatformVersion) != 0
        ||  g_dwPlatformServicePack >= 3)
    {
        // release these here although created in a different path
        ClearInterface(&pts->pTypInfoStdOLECache);
        ClearInterface(&pts->pTypLibStdOLECache);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ValidateInvoke
//
//  Synopsis:   Validates arguments to a call of IDispatch::Invoke.  A call
//              to this function takes less space than the function itself.
//
//----------------------------------------------------------------------------

HRESULT
ValidateInvoke(
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr)
{
    if (pvarResult)
        VariantInit(pvarResult);

    if (pexcepinfo)
        InitEXCEPINFO(pexcepinfo);

    if (puArgErr)
        *puArgErr = 0;

    if (!pdispparams)
        RRETURN(E_INVALIDARG);

    return S_OK;
}
//+----------------------------------------------------------------------------
//
//  Function    :   ReadyStateInvoke
//
//  Synopsis    :   This helper function is called by the various invokes for 
//      those classes that support the ready state property. this centralizes the
//      logic and code for handling this case.
//
//  RETURNS :   S_OK,           readyState-get and no errors
//              E_INVALIDARG    readystate-get and errors
//              S_FALSE         not readystate-get
//
//-----------------------------------------------------------------------------

HRESULT
ReadyStateInvoke(DISPID dispid, 
                 WORD wFlags, 
                 long lReadyState, 
                 VARIANT * pvarResult)
{
    HRESULT hr = S_FALSE;

    if (dispid == DISPID_READYSTATE )
    {
        if (pvarResult && (wFlags & DISPATCH_PROPERTYGET))
        {
            V_VT(pvarResult) = VT_I4;
            V_I4(pvarResult) = lReadyState;
            hr =  S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Function:   DispatchGetTypeInfo
//
//  Synopsis:   GetTypeInfo helper.
//
//--------------------------------------------------------------------------

HRESULT
DispatchGetTypeInfo(REFIID riid, UINT itinfo, LCID lcid, ITypeInfo ** ppTI)
{
    HRESULT hr;

    Assert(ppTI);
    *ppTI = NULL;
    if (itinfo > 0)
        RRETURN(DISP_E_BADINDEX);

    hr = THR_NOTRACE(LoadF3TypeInfo(riid, ppTI));
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Function:   DispatchGetTypeInfoCount
//
//  Synopsis:   GetTypeInfoCount helper.
//
//--------------------------------------------------------------------------

HRESULT
DispatchGetTypeInfoCount(UINT * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   DispatchGetIDsOfNames
//
//  Synopsis:   GetIDsOfNames helper.
//
//--------------------------------------------------------------------------

HRESULT
DispatchGetIDsOfNames(
        REFIID riidInterface,
        REFIID riid,
        OLECHAR ** rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID * rgdispid)
{
    HRESULT     hr;
    ITypeInfo * pTI;

    if (!IsEqualIID(riid, IID_NULL))
        RRETURN(E_INVALIDARG);

    hr = DispatchGetTypeInfo(riidInterface, 0, lcid, &pTI);
    if (hr)
        RRETURN(hr);

    hr = pTI->GetIDsOfNames(rgszNames, cNames, rgdispid);
    pTI->Release();
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBaseEventSink::GetTypeInfoCount
//
//  Synopsis:   Returns E_NOTIMPL.
//
//  History:    2-16-94   adams   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBaseEventSink::GetTypeInfoCount(unsigned int *)
{
    RRETURN(E_NOTIMPL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBaseEventSink::GetTypeInfo
//
//  Synopsis:   Returns E_NOTIMPL
//
//  History:    2-16-94   adams   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBaseEventSink::GetTypeInfo(
    unsigned int,
    LCID,
    ITypeInfo FAR* FAR*)
{
    RRETURN(E_NOTIMPL);
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseEventSink::GetIDsOfNames
//
//  Synopsis:   Returns E_NOTIMPL.
//
//  History:    12-28-93   adams   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CBaseEventSink::GetIDsOfNames(
    REFIID,
    OLECHAR FAR* FAR*,
    unsigned int,
    LCID,
    DISPID FAR*)
{
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadF3TypeInfo
//
//  Synopsis:   Loads a typeinfo from the Forms3 type library.
//
//  Arguments:  [clsid] -- The clsid of the typeinfo to load.
//              [ppTI]  -- The resulting typeinfo.
//
//----------------------------------------------------------------------------

HRESULT
LoadF3TypeInfo(REFCLSID clsid, ITypeInfo ** ppTI)
{
    HRESULT     hr;
    ITypeLib *  pTL;

    hr = THR(GetFormsTypeLib(&pTL));
    if (hr)
        RRETURN(hr);

    hr = THR_NOTRACE(pTL->GetTypeInfoOfGuid(clsid,ppTI));
    pTL->Release();

#if DBG == 1
    if (IsTagEnabled(tagLoadTypeInfo))
    {
        if (!*ppTI)
        {
            TraceTag((tagLoadTypeInfo,
                      "Unable to load Forms^3 typeinfo %hr.", hr));
        }
        else
        {
            BSTR    bstr = NULL;
            (void) (*ppTI)->GetDocumentation(
                    MEMBERID_NIL, &bstr, NULL, NULL, NULL);

            TraceTag((tagLoadTypeInfo, "Loaded typeinfo %ls.", STRVAL(bstr)));
            FormsFreeString(bstr);
        }
    }
#endif

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// CCreateTypeInfoHelper::CCreateTypeInfoHelper
//
//---------------------------------------------------------------------------

CCreateTypeInfoHelper::CCreateTypeInfoHelper()
{
    memset (this, 0, sizeof(*this));
}

//---------------------------------------------------------------------------
//
// CCreateTypeInfoHelper::~CCreateTypeInfoHelper
//
//---------------------------------------------------------------------------

CCreateTypeInfoHelper::~CCreateTypeInfoHelper()
{
    ReleaseInterface(pTypLib);
    ReleaseInterface(pTypLibStdOLE);
    ReleaseInterface(pTypInfoStdOLE);
    ReleaseInterface(pTypInfoCoClass);
    ReleaseInterface(pTypInfoCreate);
    ReleaseInterface(pTIOut);
    ReleaseInterface(pTICoClassOut);
}

//---------------------------------------------------------------------------
//
// CCreateTypeInfoHelper::Start
//
//---------------------------------------------------------------------------

HRESULT
CCreateTypeInfoHelper::Start(REFIID riid)
{
    static GUID guidStdOle = {0x00020430,0x00,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46};

    HRESULT         hr;
    THREADSTATE *   pts;

#ifdef WIN16
    hr = THR(CreateTypeLib(SYS_WIN16, _T(""), &pTypLib));
#else
    hr = THR(CreateTypeLib2(SYS_WIN32, _T(""), &pTypLib));
#endif
    if (hr)
        goto Cleanup;

    //
    // Initialize the typlib with some of the usual defaults.
    //

    hr = THR(pTypLib->SetGuid(g_Zero.guid));
    if (hr)
        goto Cleanup;

    hr = THR(pTypLib->SetVersion(1, 0));
    if (hr)
        goto Cleanup;

    hr = THR(pTypLib->SetName(_T("Page")));
    if (hr)
        goto Cleanup;

    hr = THR(pTypLib->SetLcid(LOCALE_SYSTEM_DEFAULT));
    if (hr)
        goto Cleanup;

    pts = GetThreadState();

    if (pts->pTypLibStdOLECache)
    {
        pTypLibStdOLE = pts->pTypLibStdOLECache;
        pTypInfoStdOLE = pts->pTypInfoStdOLECache;
    }
    else
    {
        //
        // Get Information on the standard OLE IDispatch
        //

        hr = THR(LoadRegTypeLib(
            guidStdOle,
            STDOLE_MAJORVERNUM,
            STDOLE_MINORVERNUM,
            STDOLE_LCID,
            &pTypLibStdOLE));
        if (hr)
            goto Cleanup;

        hr = THR(pTypLibStdOLE->GetTypeInfoOfGuid(IID_IDispatch, &pTypInfoStdOLE));
        if (hr) 
            goto Cleanup;

        pts->pTypLibStdOLECache = pTypLibStdOLE;
        pts->pTypInfoStdOLECache = pTypInfoStdOLE;
    }
    pTypLibStdOLE->AddRef();
    pTypInfoStdOLE->AddRef();

    //
    // Now create the typeInfo for the objects.
    //

    hr = THR(pTypLib->CreateTypeInfo(_T("PageProps"), TKIND_DISPATCH, &pTypInfoCreate));
    if (hr)
        goto Cleanup;

    //
    // Again perform the standard initialization on the typeinfo.
    //

    hr = THR(pTypInfoCreate->SetGuid(riid));
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCreate->SetVersion(1, 0));
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCreate->AddRefTypeInfo(pTypInfoStdOLE, &hreftype));
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCreate->AddImplType(0, hreftype));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
// CCreateTypeInfoHelper::Finalize
//
//---------------------------------------------------------------------------

HRESULT
CCreateTypeInfoHelper::Finalize(LONG lImplTypeFlags)
{
    HRESULT hr;

    //
    // Finish off the type info.
    //

    hr = THR(pTypInfoCreate->LayOut());
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCreate->QueryInterface(
        IID_ITypeInfo,
        (void **)&pTIOut));
    if (hr)
        goto Cleanup;

    //
    // Now we have to create a coclass for the interface
    //

    hr = THR(pTypLib->CreateTypeInfo(_T("Page"), TKIND_COCLASS, &pTypInfoCoClass));
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCoClass->SetGuid(g_Zero.guid));
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCoClass->SetVersion(1, 0));
    if (hr)
        goto Cleanup;

    // Add the Page Property dispinterface to coclass
    hr = THR(pTypInfoCoClass->AddRefTypeInfo(pTIOut, &hreftype));
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCoClass->AddImplType(0, hreftype));
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCoClass->SetImplTypeFlags(0, lImplTypeFlags));
    if (hr)
        goto Cleanup;

    //
    // Finish off the CoClass
    hr = THR(pTypInfoCoClass->LayOut());
    if (hr)
        goto Cleanup;

    hr = THR(pTypInfoCoClass->QueryInterface(
        IID_ITypeInfo,
        (void **)&pTICoClassOut));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}
