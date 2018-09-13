//+---------------------------------------------------------------------
//
//   File:      eobject.cxx
//
//  Contents:   Object element class, etc..
//
//  Classes:    CObjectElement, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_MIME64_HXX_
#define X_MIME64_HXX_
#include "mime64.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_PROPBAG_HXX_
#define X_PROPBAG_HXX_
#include "propbag.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"       // for CDataSourceProvider
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"       // for CDatabindTask
#endif

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include "dmembmgr.hxx"       // for CDataMemberMgr
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"     // for cpostdata
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

#define _cxx_
#include "object.hdl"

MtDefine(CObjectElement, Elements, "CObjectElement")
MtDefine(CObjectElement_aryParamBinding_pv, CObjectElement, "CObjectElement::_aryParamBinding::_pv")
MtDefine(CObjectElementStreamFromInlineData_pMimeType, Locals, "CObjectElement::StreamFromInlineData pMimeType")
MtDefine(CObjectElementGetIDForParamBinding_strTemp, Locals, "CObjectElement::GetIDForParamBinding strTemp")
MtDefine(CObjectElementSaveToDataStream_pchData, Locals, "CObjectElement::SaveToDataStream pchData")
MtDefine(CObjectElementCreateObject_pchUrl, CObjectElement, "CObjectElement::CreateObject pchUrl")
MtDefine(CObjectElementRetrieveClassidAndData_pchUrl, CObjectElement, "CObjectElement::RetrieveClassidAndData pchUrl")

#define CLSID_PREFIX                _T("clsid:")
#define CLSID_PREFIX_LEN            (ARRAY_SIZE(CLSID_PREFIX)-1)

#define DATA_PREFIX                 _T("data:")
#define DATA_PREFIX_LEN             (ARRAY_SIZE(DATA_PREFIX)-1)

#define DATA_COMPLETE_PREFIX        _T("data:application/x-oleobject;base64,")
#define DATA_COMPLETE_PREFIX_LEN    (ARRAY_SIZE(DATA_COMPLETE_PREFIX)-1)

#define DATA_ENCODING               _T("base64")
#define DATA_ENCODING_LEN           (ARRAY_SIZE(DATA_ENCODING)-1)

#define DATA_MIMETYPE               _T("application/x-oleobject")
#define DATA_MIMETYPE_LEN           (ARRAY_SIZE(DATA_MIMETYPE)-1)

#define CODEBASE_VERSION            _T("version=")

#define CODEBASE_KEY_LEN            4

EXTERN_C CLSID CLSID_AppletOCX;

#ifdef WIN16
const CLSID CLSID_CMSApplet16 = { 0x0D30EC48L, 0xDD33, 0x11D0, {0xA0, 0xFD, 0x00, 0x00, 0xF8, 0x75, 0x1B, 0x8D}};
// const CLSID CLSID_CMSApplet16 = { 0xfc144cc0, 0x2136, 0x11d0, {0x95, 0x38, 0x0, 0xaa, 0x0, 0x4b, 0x9c, 0x68}};
#endif

const CONNECTION_POINT_INFO CObjectElement::s_acpi[] =
{
    CPI_ENTRY(IID_IPropertyNotifySink, DISPID_A_PROPNOTIFYSINK)
    CPI_ENTRY(DIID_HTMLObjectElementEvents, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLObjectElementEvents2, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_IDispatch, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_ITridentEventSink, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_NULL, DISPID_A_EVENTSINK) // This will be filled up at
                                        //   with ocx's primary event iid.
    CPI_ENTRY_NULL
};

const CElement::CLASSDESC CObjectElement::s_classdesc =
{
    {
        &CLSID_HTMLObjectElement,       // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NEVERSCROLL    |
        ELEMENTDESC_OLESITE        |    // _dwFlags
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK,
        &IID_IHTMLObjectElement,        // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLObjectElement, // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};

static void
GetVersionFromString(const TCHAR *pchBuf, DWORD *pdwMS, DWORD *pdwLS);

//+---------------------------------------------------------------------------
//
//  Function:   GetVersionFromString
//
//  Synopsis:   Converts version in text format (a,b,c,d) into two dwords
//              (a,b), (c,d).  The printed version number is of format a.b.d
//              (but, we don't care)
//
//  Notes:      Lifted from URLMON.DLL sources.
//
//----------------------------------------------------------------------------

void
GetVersionFromString(const TCHAR *pchBuf, DWORD *pdwMS, DWORD *pdwLS)
{
    const TCHAR *   pch = pchBuf;
    TCHAR           ch;
    USHORT          n = 0;
    USHORT          a = 0;
    USHORT          b = 0;
    USHORT          c = 0;
    USHORT          d = 0;
    enum HAVE { HAVE_NONE, HAVE_A, HAVE_B, HAVE_C, HAVE_D } have = HAVE_NONE;

    *pdwMS = 0;
    *pdwLS = 0;

    if (!pch)            // default to zero if none provided
        return;

    // BUGBUG:  philco 5/6/96
    // Need to add code to handle trailing/leading/embedded spaces in the
    // version string.
    if (!StrCmpC(pch, _T("-1,-1,-1,-1")))
    {
        *pdwMS = 0xFFFFFFFF;
        *pdwLS = 0xFFFFFFFF;
        return;
    }

    for (ch = *pch++;;ch = *pch++)
    {
        if (ch == ',' || ch == '\0')
        {
            switch (have)
            {
            case HAVE_NONE:
                a = n;
                have = HAVE_A;
                break;

            case HAVE_A:
                b = n;
                have = HAVE_B;
                break;

            case HAVE_B:
                c = n;
                have = HAVE_C;
                break;

            case HAVE_C:
                d = n;
                have = HAVE_D;
                break;

            case HAVE_D:
                goto Error;
            }

            if (ch == '\0')
            {
                // all done convert a,b,c,d into two dwords of version

                *pdwMS = ((a << 16)|b);
                *pdwLS = ((c << 16)|d);
                return;
            }

            n = 0; // reset
        }
        else if (ch < '0' || ch > '9')
            goto Error;
        else
        {
            n = n*10 + (ch - '0');
        }
    } /* end forever */

Error:
    *pdwMS = *pdwLS = 0;
}


//+---------------------------------------------------------------------------
//
//  element creator used by parser
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);

    *ppElementResult = new CObjectElement(pht->GetTag(), pDoc);

    RRETURN ( (*ppElementResult) ? S_OK : E_OUTOFMEMORY);
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement constructor
//
//----------------------------------------------------------------------------

CObjectElement::CObjectElement (ELEMENT_TAG etag, CDoc *pDoc)
  : super(etag, pDoc), _aryParamBinding(Mt(CObjectElement_aryParamBinding_pv))

{
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::Passivate, CBase
//
//----------------------------------------------------------------------------

void
CObjectElement::Passivate()
{
    long i;

#ifndef NO_DATABINDING
    //
    // There may be deferred calls pending to DeferredSaveData which
    // we need to clear
    //

    GWKillMethodCall(this, ONCALL_METHOD(CObjectElement, DeferredSaveData, deferredsavedata), 0);
#endif

    for ( i = _aryParamBinding.Size() ; i ; )
    {
        PARAMBINDING *pParamBinding = &_aryParamBinding[--i];

        MemFree(pParamBinding->_strParamName);
        MemFree(pParamBinding->_strDataSrc);
        MemFree(pParamBinding->_strDataFld);
        MemFree(pParamBinding->_strDataFormatAs);
    }
    _aryParamBinding.DeleteAll();

    if (_pParamBag)
    {
        // This will only happen if the </OBJECT> end-tag is missing.
        TraceTag((tagError, "Missing </OBJECT> tag in HTML source!"));
        ReleaseParamBag();
    }

    super::Passivate();
}


//+----------------------------------------------------------------------------
//
//  Member:     CObjectElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CObjectElement::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_HTML_TEAROFF(this, IHTMLElement2, NULL)
        QI_HTML_TEAROFF(this, IHTMLObjectElement, NULL)
        QI_INHERITS2(this, IUnknown, IHTMLObjectElement)
        QI_HTML_TEAROFF(this, IHTMLObjectElement2, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     InvokeExReady
//
//  Synopsis  :this is only here to handle readyState queries, everything
//      else is passed on to the super
//
//+------------------------------------------------------------------------

#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif

STDMETHODIMP
CObjectElement::ContextThunk_InvokeExReady(DISPID dispid,
                              LCID lcid,
                              WORD wFlags,
                              DISPPARAMS *pdispparams,
                              VARIANT *pvarResult,
                              EXCEPINFO *pexcepinfo,
                              IServiceProvider *pSrvProvider)
{
    IUnknown * pUnkContext;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    HRESULT  hr = S_OK;

    hr = THR(ValidateInvoke(pdispparams, pvarResult, pexcepinfo, NULL));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(ReadyStateInvoke(dispid, wFlags, _lReadyState, pvarResult));
    if (hr == S_FALSE)
    {
        hr = THR_NOTRACE(super::ContextInvokeEx(dispid,
                                         lcid,
                                         wFlags,
                                         pdispparams,
                                         pvarResult,
                                         pexcepinfo,
                                         pSrvProvider,
                                         pUnkContext ? pUnkContext : (IUnknown*)this));
    }

Cleanup:
    RRETURN(hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::StreamFromInlineData
//
//  Synopsis:   Retrieve DATA as a mime64 encoded string.
//
//  Notes:      Parses a data attribute of the form:
//
//              DATA="data:application/x-oleobject;base64,8dhfh8d4mvc983f"
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::StreamFromInlineData(TCHAR *pchData, IStream ** ppStm)
{
    if (!pchData || !ppStm)
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    LPTSTR pData = pchData + DATA_PREFIX_LEN; // skip past data: URL specifier

    LPTSTR pComma = _tcsrchr(pData, _T(','));
    if (pComma == NULL)
        return E_INVALIDARG;    // Bad data attribute

    LPTSTR pSemiColon = _tcsrchr(pData, _T(';'));
    if (pSemiColon > pComma)
        pSemiColon = NULL;    // handles case where ';' appears in the encoded data.

    // Indicates the end of the mimetype designation.
    LPTSTR pMimeTerminator = pSemiColon ? pSemiColon : pComma;

    LPTSTR pMimeType = NULL;
    int mimelength = pMimeTerminator - pData + 1;  // leave room for the null terminator

    if (mimelength > 0)
    {
        pMimeType = (TCHAR *)MemAlloc(Mt(CObjectElementStreamFromInlineData_pMimeType),
            mimelength * sizeof(TCHAR));
        if (pMimeType)
        {
            // Since _tcsncpy doesn't terminate the string,
            // fill the memory with zeros.
            ::ZeroMemory(pMimeType, mimelength * sizeof(TCHAR));
            _tcsncpy(pMimeType, pData, (mimelength - 1));
        }
    }
    else
    {
        // Default to this mime type.
        MemAllocString(Mt(CObjectElementStreamFromInlineData_pMimeType),
            _T("text/plain;charset=US-ASCII"), &pMimeType);
    }

    if (pSemiColon)
    {
        pSemiColon = CharNext(pSemiColon);
        if ((_tcsnicmp( pSemiColon, DATA_ENCODING_LEN, DATA_ENCODING, DATA_ENCODING_LEN) == 0) && (_tcsncmp(pMimeType, DATA_MIMETYPE_LEN, DATA_MIMETYPE, DATA_MIMETYPE_LEN) == 0))
        {
            pComma = CharNext(pComma);
            hr = Mime64Decode(pComma, ppStm);
            goto Cleanup;
        }
    }

Cleanup:
    MemFree(pMimeType);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::SaveToDataStream
//
//  Synopsis:   Set the DATA attribute as a mime64 encoded string.
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::SaveToDataStream()
{
    HRESULT                 hr = S_OK;
    IPersistStreamInit *    pPSI = NULL;
    IStream *               pStm = NULL;
    STATSTG                 statstg;
    HGLOBAL                 hGlobal;
    LPVOID                  pv;
    TCHAR *                 pchData = NULL;

    hr = THR_NOTRACE(QueryControlInterface(IID_IPersistStreamInit, (void **)&pPSI));
    if (hr)
    {
        if (E_NOINTERFACE == hr) // eat up certain errors
            hr = S_OK;
        goto Cleanup;
    }

    hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pStm));
    if(hr)
        goto Cleanup;

    hr = THR(OleSaveToStream((IPersistStream*)pPSI, pStm));
    if (hr)
    {
        if (E_NOTIMPL == hr)
            hr = S_OK;  // bug 49053
        goto Cleanup;
    }

    hr = THR(pStm->Stat(&statstg, STATFLAG_NONAME));
    if (hr)
        goto Cleanup;

    if (statstg.cbSize.HighPart)
    {
        hr = E_FAIL; // way too long stream to save it
        goto Cleanup;
    }

    hr = THR(GetHGlobalFromStream(pStm, &hGlobal));
    if (hr)
        goto Cleanup;

    pv = GlobalLock(hGlobal);

    // we allocate this size under assumption that
    // mime64 encoding increases size if input not more than 2 times
    pchData = new(Mt(CObjectElementSaveToDataStream_pchData)) TCHAR [DATA_COMPLETE_PREFIX_LEN + 2 * statstg.cbSize.LowPart + 1];
    if (!pchData)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _tcscpy (pchData, DATA_COMPLETE_PREFIX);

    hr = THR(Mime64Encode((BYTE *)pv, statstg.cbSize.LowPart, pchData + DATA_COMPLETE_PREFIX_LEN));
    if (hr)
        goto Cleanup;

    GlobalUnlock(hGlobal);

    hr = THR(SetAAdata(pchData));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pPSI);
    ReleaseInterface(pStm);
    delete [] pchData;
    RRETURN(hr);
}

// (alexz) (anandra) (oliverse) (hollyba)
// because of a bug in JavaVM (as of Sep 3, 97), applets can crash when printed on
// FE Win95 systems. For stability reasons, instead of making the fix in JavaVM
// (which is not safe) we disable printing them on FE Win95 systems. In case if
// Java VM team makes a private or other drop of java VM with the problem fixed,
// registry key Software\\Microsoft\\Internet Explorer\\PrintAppletsOnFEWin95 can
// be put into the registry which will allow printing of applets on FE Win95.
// All other systems should not be affected by this fix.
//

BOOL PrintAppletsOnFEWin95()
{
    static TCHAR szTableRootKey[] =
        _T("Software\\Microsoft\\Internet Explorer\\PrintAppletsOnFEWin95");

    LONG    lRet;
    HKEY    hkeyRoot = NULL;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTableRootKey, 0, KEY_READ, &hkeyRoot);

    if (hkeyRoot)
        RegCloseKey (hkeyRoot);

    return (ERROR_SUCCESS == lRet) ? TRUE : FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CObjectElement::CreateObject()
//
//  Synopsis:   Helper to instantiate control
//
//-------------------------------------------------------------------------

HRESULT
CObjectElement::CreateObject()
{
    HRESULT             hr = S_OK;
    OLECREATEINFO       info;
    TCHAR *             pchCodeBase = NULL;
    TCHAR *             pchVersion = NULL;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR *             pchBaseUrl = cBuf;

    GWKillMethodCall((COleSite *)this, ONCALL_METHOD(COleSite, DeferredCreateObject, deferredcreateobject), 0);

    // if already created, do nothing
    if (_pUnkCtrl)
        goto Cleanup;

    //
    // This is where we create the object based on the html attributes
    // First try and get a clsid if possible.
    //

    RetrieveClassidAndData(
        &info.clsid,
        &info.pStream,
        &info.pchDataUrl,
        &info.pchClassid);

    //
    // Now initialize the mime type
    //

    if (info.clsid == CLSID_NULL && GetAAtype())
    {
        MemAllocString(Mt(OleCreateInfo),
            (TCHAR *)GetAAtype(), &info.pchMimeType);
    }

    //
    // Initialize the major and minor versions to a default of 0
    //

    info.dwMajorVer = info.dwMinorVer = (DWORD)0;

    //
    // Go about determining the source of the object/applet
    //

    info.pchSourceUrl = NULL;
    pchCodeBase = (TCHAR*) GetAAcodeBase();
    if (pchCodeBase)
    {
        // CODEBASE= values can take several forms:
        // 1. Full URL path:  CODEBASE=http://www.this.com/ctrls/foo.cab#version=...
        // 2. Relative URL:   CODEBASE=ctrls/foo.cab#version=...
        // 3. No URL:         CODEBASE=#version=...

        if (pchCodeBase[0] != _T('#') )
        {
            TCHAR   cBuf[pdlUrlLen];

            // There is a part of a URL to expand & combine:
            hr = THR(Doc()->ExpandUrl(pchCodeBase, ARRAY_SIZE(cBuf), cBuf, this));
            if (hr)
                goto Cleanup;

            MemAllocString(Mt(CObjectElementCreateObject_pchUrl), cBuf, &info.pchSourceUrl);
            if (info.pchSourceUrl == NULL)
                goto Cleanup;

            pchCodeBase = info.pchSourceUrl;

            //
            // Search the codebase for '#'.  This is the delimiter for the
            // version.
            //

// (ferhane/alanshi) 
// For Fusion usage, URLMON now needs the complete URL as the codebase string, 
// including the version information. So, we stopped terminating the codebase
// string at the '#' sign. The version string is still copied and used separately.

            pchVersion = _tcschr(info.pchSourceUrl, _T('#'));

            if (pchVersion)
                pchVersion++;
        }
        else
        {   // There was only a version number:
            pchVersion = pchCodeBase+1;
        }

        if (pchVersion)
        {
            //
            // Continue searching for the "version="
            //

            if (_tcsnipre(
                    CODEBASE_VERSION, ARRAY_SIZE(CODEBASE_VERSION) - 1,
                    pchVersion, -1))
            {
                // Advance past the "version="
                pchVersion += ARRAY_SIZE(CODEBASE_VERSION) - 1;
                GetVersionFromString(pchVersion, &info.dwMajorVer, &info.dwMinorVer);
            }
        }
      }

    //
    // Finally in the case of the applet oc, specifically save
    // all attributes into the param bag so the applet oc can retrieve
    // everything.
    //
#ifndef WIN16
    if (info.clsid == CLSID_AppletOCX)
#else
        if (info.clsid == CLSID_CMSApplet16)
#endif
    {
        if (Doc()->IsPrintDoc() && g_fFarEastWin9X)
        {
            if (!PrintAppletsOnFEWin95())
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }

        if (Doc()->IsPrintDoc() && _pParamBag)     // (we already know we're an applet)
        {
            VARIANT    var;
            IStream  * pStream;

            var.vt = VT_I4;

            if (OK(_pParamBag->Read(L"_Marshaled_pUnk", &var, NULL, 0)))
            {
                if (var.vt == VT_I4)
                {
                    pStream = (IStream *) var.punkVal;

                    // Might need exception handler around the following
                    //
                    CoGetInterfaceAndReleaseStream(pStream, IID_IUnknown, (void**)&_pUnkPrintDelegate);
                }
            }
        }

        hr = THR(EnsureParamBag());
        if (hr)
            goto Cleanup;

        Assert(_pParamBag);

        //
        // Save attributes into param bag.
        //

        hr = THR(super::SaveAttributes(_pParamBag));
        if (hr)
            goto Cleanup;

#ifdef WIN16

                if (Tag() == ETAG_APPLET)
                {
                        hr = SaveWin16AppletProps(_pParamBag);
                        if (hr)
                                goto Cleanup;
                }
#endif

        //
        // Set the codebase attribute to point to the full base url
        // The VM ocx needs this.
        //

        if (!pchCodeBase)
        {
            hr = THR(Doc()->ExpandUrl(_T("."), ARRAY_SIZE(cBuf), pchBaseUrl, this));
            if (hr)
                goto Cleanup;

            hr = THR(_pParamBag->AddProp(_T("codeBase"), pchBaseUrl));
            if (hr)
                goto Cleanup;
        }
        else
        {
            // update the entry with the new string, 
            hr = THR(_pParamBag->FindAndSetProp(_T("codeBase"), pchCodeBase));
            if (hr)
                goto Cleanup;
        }
    }

    if (TLS(_pDataObjectForPaste))
    {
        info.pDataObject = TLS(_pDataObjectForPaste);
        (info.pDataObject)->AddRef();

        // Just to make the point that we have consumed this data obj.
        TLS(_pDataObjectForPaste) = NULL;
    }

    if (_pParamBag)
    {
        info.pPropBag = _pParamBag;
        _pParamBag->AddRef();
    }

    {
        CDoc * pDocRoot = Doc()->GetRootDoc();
        //
        // (t-chrisr) Silly hack so that webcheck can find out all the
        // information it needs to properly suck down java applets.
        //
        if (_pParamBag &&
            (OlesiteTag() == OSTAG_APPLET) &&
            pDocRoot &&
            (pDocRoot->_dwLoadf & DLCTL_NO_JAVA) &&
            pDocRoot->_pClientSite)
        {
            VARIANTARG  varargIn;
            VARIANTARG  varargOut;
            VariantInit(&varargIn);
            V_VT(&varargIn) = VT_UNKNOWN;
            varargIn.punkVal = (IPropertyBag2*)_pParamBag;

            //
            // if this fails, it should do no harm to us
            //
            IGNORE_HR(CTExec(pDocRoot->_pClientSite,
                            &CGID_JavaParambagCompatHack,
                            0,
                            MSOCMDEXECOPT_DONTPROMPTUSER,
                            &varargIn,
                            &varargOut));
        }
    }

    hr = THR(super::CreateObject(&info));


Cleanup:
    // If the control has been created, release the param bag.
    if (_pUnkCtrl)
        ReleaseParamBag();
        
#ifdef WIN16
    return hr;
#else
    RRETURN(hr);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::RetrieveClassidAndData
//
//  Synopsis:   Helper for retrieving a CLSID from html attributes
//
//----------------------------------------------------------------------------

void
CObjectElement::RetrieveClassidAndData(
    CLSID *pclsid,
    IStream **ppStream,
    TCHAR **ppchData,
    TCHAR **ppchClassid)
{
    HRESULT     hr;
    IBindCtx *  pBC = NULL;
    TCHAR *     pchData = (TCHAR *)GetAAdata();
    TCHAR *     pchClsid = (TCHAR *)GetAAclassid();

    //
    //  Rules for retrieval:
    //      -   If we have a data:, get the clsid from it.
    //      -   If we had a real clsid present as an attribute, use it.
    //      -   If the code attribute is present, default to the applet ocx.
    //      -   Try to infer the clsid from the codeType attribute.
    //      -   Failing that try to infer the clsid from the type attribute.
    //

    if (pchData)
    {
        if (!_tcsnipre(DATA_PREFIX, DATA_PREFIX_LEN, pchData, -1))
        {
            TCHAR   cBuf[pdlUrlLen];
            MemFreeString(*ppchData);
            hr = THR(Doc()->ExpandUrl(pchData, ARRAY_SIZE(cBuf), cBuf, this));
            if (hr)
                goto Error;
            MemAllocString(Mt(CObjectElementRetrieveClassidAndData_pchUrl), cBuf, ppchData);
            if (ppchData == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
        }
        else
        {
            LARGE_INTEGER   dlibMove = {0,0};

            // inline mime64-encoded data.  Parse and decode.
            hr = THR(StreamFromInlineData(pchData, ppStream));
            if (hr)
                goto Error;

            // seek to the begining of the stream
            hr = THR((*ppStream)->Seek(dlibMove, STREAM_SEEK_SET, NULL));
            if (hr)
                goto Error;

            //
            // read the clsid from the first 16 bytes
            //

#ifdef BIG_ENDIAN
            // BUGBUG fix big endian
            Assert( 0 && "Fix big endian read of clsid" );
#endif

            hr = THR((*ppStream)->Read(pclsid, 16, NULL));
            if (hr)
                goto Error;

            goto Cleanup;
        }
    }

    if (pchClsid && *pchClsid)
    {
        if (!OK(CLSIDFromHtmlString(pchClsid, pclsid)))
        {
            TCHAR   achBuf[pdlUrlLen];

            MemFreeString(*ppchClassid);
            hr = THR(Doc()->ExpandUrl(pchClsid, ARRAY_SIZE(achBuf), achBuf, this));
            if (hr)
                goto Error;
            MemAllocString(Mt(CObjectElementRetrieveClassidAndData_pchUrl), achBuf, ppchClassid);
            if (ppchClassid == NULL)
                goto Error;
        }
        goto Cleanup;
    }

    if ((Tag() == ETAG_APPLET) || GetAAcode())
    {
#ifndef WIN16
        *pclsid = CLSID_AppletOCX;
#else
                *pclsid = CLSID_CMSApplet16;
#endif
        goto Cleanup;
    }

    hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pBC, 0));
    if (hr)
        goto Error;

    if (GetAAcodeType())
    {
        hr = THR_NOTRACE(GetClassFileOrMime(
                pBC,
                NULL,
                NULL,
                0,
                GetAAcodeType(),
                0,
                pclsid));
    }
    else if (GetAAtype())
    {
        hr = THR_NOTRACE(GetClassFileOrMime(
                pBC,
                NULL,
                NULL,
                0,
                GetAAtype(),
                0,
                pclsid));
    }

    pBC->Release();

    if (hr)
        goto Error;

Cleanup:
    return;

Error:
    ClearInterface(ppStream);
    MemFreeString(*ppchData);
    *ppchData = NULL;
    *pclsid = GUID_NULL;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::Save
//
//  Synopsis:   called twice: for opening <OBJECT> and for closing </OBJECT>.
//              Adds to parent's implementation saving parambag if necessary
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr = S_OK;
    TCHAR * pchData;
    BOOL    fSaveParams = FALSE;
    BOOL    fHadParamBag = !!(_pParamBag);

    if (!fEnd && !pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        //
        // Saving open tag.
        //

        //
        // Many cases to consider here.
        // Case 1:  DATA attribute exists and it's not a data:
        //          Just save the attribute as is, and no saving params
        // Case 2:  DATA attribute exists and it is a data:
        //          Ask control to persist itself into stream and
        //          mime encode it.
        // Case 3:  Try IPersistPropertyBag::Save
        // Case 4:  Try IPersistStream::Save
        //

        pchData = (TCHAR *)GetAAdata();
        if (pchData)
        {
            if (_tcsnipre(DATA_PREFIX, DATA_PREFIX_LEN, pchData, -1))
            {
                // Case 2

                hr = THR(SaveToDataStream());
                if (hr)
                    goto Cleanup;
            }
        }
        else
        {
            // Case 3

            if (_pParamBag)
            {
                fSaveParams = TRUE;
            }
            
            if (OK(ExchangeParamBag(FROMCONTROL)))
            {
                fSaveParams = TRUE;
            }
            else
            {
                // Case 4:

                hr = THR(SaveToDataStream());
                if (hr)
                    goto Cleanup;
            }
        }
    }

    hr = THR(super::Save(pStreamWrBuff, fEnd));
    if (hr)
        goto Cleanup;

    if (!fEnd && !pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        // If we're saving for printing
        // AND it's an applet 
        if ( Doc()->_fSaveTempfileForPrinting
            && OlesiteTag() == OSTAG_APPLET)
        {
            // before we can marshal this pointer, we need to verify
            // that the document this oject is on is not embedding in
            // a frameset document.  If so, we don't want to do this.
            //
            // BUGBUG do we also need to check for !ParentIFrameSite()
            if (!Doc()->ParentFrameSite())
            {
                VARIANT      var;
                IStream    * pStream = NULL;

                if (_pParamBag && _pUnkCtrl)
                {
                    if (OK(CoMarshalInterThreadInterfaceInStream(IID_IUnknown, _pUnkCtrl, &pStream)))
                    {
                        var.vt = VT_I4;
                        var.punkVal = pStream;
                        _pParamBag->Write(L"_Marshaled_pUnk", &var);
                        VariantClear(&var);
                    }
                }
            }
        }

        // if open tag was just written
        hr = THR(SaveParamBindings(pStreamWrBuff));
        if (hr)
            goto Cleanup;

        if (fSaveParams && _pParamBag)
        {
            CPropertyBag *pParamBagFiltered = _pParamBag;

            if (!pStreamWrBuff->TestFlag(WBF_NO_DATABIND_ATTRS))
            {
                hr = THR(RemoveBoundParams(_pParamBag, fHadParamBag,
                                            &pParamBagFiltered));
            }
            
            hr = THR(pParamBagFiltered->Save(pStreamWrBuff));
            if (pParamBagFiltered != _pParamBag)
                pParamBagFiltered->Release();
            if (hr)
                goto Cleanup;
        }

        // save altHtml after PARAMs
        hr = THR(SaveAltHtml(pStreamWrBuff));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    // we release the param bag, if we created it now. 
    // We should not release it if we already had it when we were called.
    if ( !fHadParamBag )
        ReleaseParamBag();

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::RemoveBoundParams
//
//  Synopsis:   Helper routine, called by CObjectElement::Save.  This walks
//              through the param bag removing any params that also appear
//              in the param binding list, so that we don't persist them
//              twice.
//
//              To avoid allocations, we return the original bag
//              whenever possible - either because there were no changes
//              to make or because the caller said it was OK to change
//              the original list in place (by passing fPreserve = FALSE).
//              It's the caller's responsibility to release the new bag
//              if we did have to allocate a new one.
//
//  Arguments:  pParamBag           param bag (input)
//              fPreserve           true if we shouldn't change input bag
//              ppParamBagReturn    filtered param bag (output)
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::RemoveBoundParams(CPropertyBag *pParamBag,
                                    BOOL fPreserve,
                                    CPropertyBag **ppParamBagReturn)
{
    Assert(pParamBag && ppParamBagReturn);
    HRESULT hr = S_OK;
    int k;
    PROPNAMEVALUE *pProp;
    CPropertyBag *pParamBagNew;
    BOOL fNeedCopy = FALSE;

    // assume we can reuse the original bag
    *ppParamBagReturn = pParamBag;

    // if there are no param bindings, there's nothing to do
    if (_aryParamBinding.Size() == 0)
        goto Cleanup;

    // march through the input bag backwards, looking for duplicates
    for (k=pParamBag->_aryProps.Size()-1, pProp=&(pParamBag->_aryProps[k]);
         !fNeedCopy && k >= 0;
         --k, --pProp)
    {
        int j;
        PARAMBINDING *pParamBinding;

        for (j=_aryParamBinding.Size(), pParamBinding=&_aryParamBinding[0];
             j > 0;
             --j, ++pParamBinding)
        {
            if (!_tcsicmp(pProp->_cstrName, pParamBinding->_strParamName))
            {
                // there's a change to make.  Make it in place, if that's
                // allowed;  otherwise go do a full copy/filter.
                if (fPreserve)
                {
                    fNeedCopy = TRUE;
                }
                else
                {
                    pProp->Free();
                    pParamBag->_aryProps.Delete(k);
                }
                break;      // we've seen the dupe, no sense looking at the rest
            }
        }
    }

    // if we didn't need to allocate a new bag, we're done
    if (!fNeedCopy)
        goto Cleanup;

    // allocate a new param bag
    pParamBagNew = new CPropertyBag;
    if (pParamBagNew == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    // copy the properties that don't appear as param bindings
    for (k=pParamBag->_aryProps.Size(), pProp=&(pParamBag->_aryProps[0]);
         k > 0;
         --k, ++pProp)
    {
        int j;
        PARAMBINDING *pParamBinding;
        BOOL fCopy = TRUE;

        for (j=_aryParamBinding.Size(), pParamBinding=&_aryParamBinding[0];
             j > 0;
             --j, ++pParamBinding)
        {
            if (!_tcsicmp(pProp->_cstrName, pParamBinding->_strParamName))
            {
                fCopy = FALSE;
                break;      // we've seen the dupe, no sense looking at the rest
            }
        }

        // if the property wasn't a param binding, copy it to the new bag
        if (fCopy)
        {
            HRESULT hr1;
            PROPNAMEVALUE *pPropNew = pParamBagNew->_aryProps.Append();

            if (pPropNew == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            
            hr = pPropNew->_cstrName.Set(pProp->_cstrName);
            VariantInit(&pPropNew->_varValue);
            hr1 = VariantCopy(&pPropNew->_varValue, &pProp->_varValue);
            if (hr || hr1)
            {
                hr = hr ? hr : hr1;
                goto Cleanup;
            }
        }
    }

    // return the new param bag
    *ppParamBagReturn = pParamBagNew;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::SaveParamBindings
//
//  Synopsis:   Helper routine, called by CObjectElement::Save, which persists
//              any bindings specified by attributes of PARAM tags.
//
//  Arguments:  pStreamWrBuff:  stream to write HTML to
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::SaveParamBindings(CStreamWriteBuff * pStreamWrBuff)
{
    HRESULT         hr = S_OK;
    int             cParamBindings = _aryParamBinding.Size();
    PARAMBINDING   *pParamBinding;
    DWORD           dwOldBuffFlags;

    if (pStreamWrBuff->TestFlag(WBF_NO_DATABIND_ATTRS))
        goto Cleanup;

    if (cParamBindings == 0)
        goto Cleanup;

    dwOldBuffFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);
    pStreamWrBuff->BeginIndent();

    for (pParamBinding = &_aryParamBinding[0]; cParamBindings--; pParamBinding++)
    {

        hr = pStreamWrBuff->NewLine();
        if (hr)
            break;

        hr = pStreamWrBuff->Write(
                pStreamWrBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC) && pStreamWrBuff->TestFlag(WBF_SAVE_FOR_XML)
                    ? _T("<HTML:PARAM NAME=") : _T("<PARAM NAME=")
                );
        if (hr)
            break;

        hr = pStreamWrBuff->WriteQuotedText( pParamBinding->_strParamName, TRUE );
        if( hr )
            break;

        if (pParamBinding->_strDataSrc)
        {
            hr = pStreamWrBuff->Write(_T(" DATASRC="));
            if( hr )
                break;

            hr = pStreamWrBuff->WriteQuotedText( pParamBinding->_strDataSrc, TRUE );
            if( hr )
                break;
        }

        if (pParamBinding->_strDataFld)
        {
            hr = pStreamWrBuff->Write(_T(" DATAFLD="));
            if( hr )
                break;

            hr = pStreamWrBuff->WriteQuotedText( pParamBinding->_strDataFld, TRUE );
            if( hr )
                break;
        }

        if (pParamBinding->_strDataFormatAs)
        {
            hr = pStreamWrBuff->Write(_T(" DATAFORMATAS="));
            if( hr )
                break;

            hr = pStreamWrBuff->WriteQuotedText( pParamBinding->_strDataFormatAs, TRUE );
            if( hr )
                break;
        }

        hr = pStreamWrBuff->Write(_T(">"));
        if( hr )
            break;
    }

    pStreamWrBuff->EndIndent();
    pStreamWrBuff->RestoreFlags(dwOldBuffFlags);

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::SaveAltHtml
//
//  Synopsis:   Helper routine, called by CObjectElement::Save, which saves
//              any "alternate" html originally inside the OBJECT tag
//
//  Arguments:  pStreamWrBuff:  stream to write HTML to
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::SaveAltHtml (CStreamWriteBuff *pStreamWrBuff)
{
    HRESULT         hr = S_OK;
    TCHAR         * pchAltHtml = (LPTSTR)GetAAaltHtml();
    DWORD           dwOldBuffFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);

    if (pchAltHtml)
    {
        hr = pStreamWrBuff->NewLine();
        if (hr)
            goto Cleanup;

        hr = pStreamWrBuff->Write(pchAltHtml);
    }

Cleanup:
    pStreamWrBuff->RestoreFlags(dwOldBuffFlags);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::Getobject
//
//  Synopsis:   implementation of pdl-abstract attribute
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::get_object(IDispatch **ppDisp)
{
    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDisp = NULL;

    if (!IsInMarkup())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    CacheDispatch();
    if (!_pDisp)
        goto Cleanup;

    if (IsSafeToScript() && AccessAllowed(_pDisp))
    {
        *ppDisp = _pDisp;
        _pDisp->AddRef();
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::namedRecordset
//
//  Synopsis:   returns an ADO Recordset for the named data member.  Tunnels
//              into the hierarchy using the path, if given.
//
//  Arguments:  bstrDataMember  name of data member (NULL for default)
//              pvarHierarchy   BSTR path through hierarchy (optional)
//              pRecordSet      where to return the recordset.
//
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::namedRecordset(BSTR bstrDatamember,
                               VARIANT *pvarHierarchy,
                               IDispatch **pRecordSet)
{
    HRESULT hr;
    CDataMemberMgr *pdmm;

#ifndef NO_DATABINDING
    EnsureDataMemberManager();
    pdmm = GetDataMemberManager();
    if (pdmm)
    {
        hr = pdmm->namedRecordset(bstrDatamember, pvarHierarchy, pRecordSet);
        if (hr == S_FALSE)
            hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }
    
#else
    *pRecordSet = NULL;
    hr = S_OK;
#endif NO_DATABINDING

    RRETURN (SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::getRecordSet
//
//  Synopsis:   returns an ADO Recordset pointer if this site is a data
//              source control
//
//  Arguments:  IDispatch **    pointer to a pointer to a record set.
//
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::get_recordset(IDispatch **pRecordSet)
{
    HRESULT hr = S_OK;
    *pRecordSet = NULL;                 // Make sure to null on failure.
    CDataMemberMgr *pdmm;
    
#ifndef NO_DATABINDING
    EnsureDataMemberManager();
    pdmm = GetDataMemberManager();
    if (pdmm)
    {
        // normal case, return ADO recordset based on my provider
        hr = pdmm->namedRecordset(NULL, NULL, pRecordSet);
    }
    else
    {
        hr = S_FALSE;
    }
    
    // backward compatability with IE3 and ADC1.1.  If my control isn't a provider
    // (in the Trident sense), it may expose its own recordset property. Delegate.
    if (hr == S_FALSE)
    {
        DISPID dispid;
        VARIANT varRecordset;
        static OLECHAR * oszRecordset = _T("Recordset");

        hr = S_OK;
        if (!_pUnkCtrl)                 // Make sure site really has an object
            goto Cleanup;

        CacheDispatch();

        if ( !_pDisp )
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        // get dispid of control's recordset property
        hr = _pDisp->GetIDsOfNames(IID_NULL, &oszRecordset, 1,
                                    g_lcidUserDefault, &dispid);
        if (hr)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        // get recordset property
        VariantInit(&varRecordset);
        hr = GetDispProp(_pDisp, dispid, g_lcidUserDefault, &varRecordset);
        if (hr)
            goto Cleanup;

        // return it to caller
        if (varRecordset.vt == VT_DISPATCH || varRecordset.vt == VT_UNKNOWN)
            *pRecordSet = varRecordset.pdispVal;
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
            VariantClear(&varRecordset);
            goto Cleanup;
        }
    }

Cleanup:
#endif // ndef NO_DATABINDING
    RRETURN (SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::setRecordSet
//
//  Synopsis:   returns an ADO Recordset pointer if this site is a data
//              source control
//
//  Arguments:  IDispatch **    pointer to a pointer to a record set.
//
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::put_recordset(IDispatch *pRecordSet)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND; // Trident does not support put_recordset

#ifndef NO_DATABINDING
    // backward compatability with IE3 and ADC1.1.  If my control isn't a provider
    // (in the Trident sense), it may expose its own recordset property. Delegate.
    if (!IsDataProvider())
    {
        DISPID dispid;
        VARIANT varRecordset;
        static OLECHAR * oszRecordset = _T("Recordset");

        CacheDispatch();

        if ( !_pDisp )
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        // get dispid of control's recordset property
        hr = _pDisp->GetIDsOfNames(IID_NULL, &oszRecordset, 1,
                                    g_lcidUserDefault, &dispid);
        if (hr)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        // set recordset property
        VariantInit(&varRecordset);
        varRecordset.vt = VT_DISPATCH;
        varRecordset.pdispVal = pRecordSet;
        hr = SetDispProp(_pDisp, dispid, g_lcidUserDefault, &varRecordset);
    }

Cleanup:
#endif // ndef NO_DATABINDING
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectElement::get_BaseHref
//
//  Synopsis:   Returns the base href for this object tag.
//
//----------------------------------------------------------------------------

HRESULT
CObjectElement::get_BaseHref(BSTR *pbstr)
{
        RRETURN( SetErrorInfo(GetBaseHref( pbstr )) );
}

#ifndef NO_DATABINDING
class CDBindMethodsObject : public CDBindMethods
{
    typedef CDBindMethods super;

public:
    CDBindMethodsObject()   {}
    ~CDBindMethodsObject()  {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;

    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                                         BOOL fHTML, LPVOID pvData) const;


protected:
    virtual DBIND_KIND DBindKindImpl(CElement *pElem,
                                     LONG id,
                                     DBINFO *pdbi) const;

    virtual BOOL    FDataSrcValidImpl(CElement *pElem) const { return TRUE; }
    virtual BOOL    FDataFldValidImpl(CElement *pElem) const { return TRUE; }
    virtual BOOL    IsReadyImpl(CElement *pElem) const;

    virtual HRESULT GetNextDBSpecCustom(CElement *pElem,
                                        LONG *pid,
                                        DBSPEC *pdbs) const;

};

static const CDBindMethodsObject DBindMethodsObject;

const CDBindMethods *
CObjectElement::GetDBindMethods()
{

    return &DBindMethodsObject;
}


//+----------------------------------------------------------------------------
//
//  Function: DBindKindImpl, CDBindMethods
//
//  Synopsis: Indicate whether or not <element, id> can be databound, and
//            optionally return additional info about binding -- an ID used
//            for transfer, and the desired data type to be used for transfer.
//
//  Arguments:
//            [id]    - binding being asked about; 0 mean defaulbind attribute,
//                      other refers to a param binding
//            [pdbi]  - pointer to struct to get data type and ID;
//                      may be NULL
//
//  Returns:  Binding status: one of
//              DBIND_NONE
//              DBIND_SINGLEVALUE
//              DBIND_ICURSOR
//              DBIND_IROWSET
//              DBIND_DLCURSOR
//
//-----------------------------------------------------------------------------

DBIND_KIND
CDBindMethodsObject::DBindKindImpl(CElement *pElem,
                                   LONG id,
                                   DBINFO *pdbi) const
{
    DBIND_KIND dbk = DBIND_NONE;

    CObjectElement *pObj = DYNCAST(CObjectElement, pElem);
    BOOL fReady = (pObj->_lReadyState >= READYSTATE_LOADED);
    CLASSINFO *pci;     // don't call GetClassInfo unless we have to

    // We use a private dbi, distinct from *pdbi.  Note that pdbi may be NULL;
    // if non-NULL, it has already been initialized.  BUGBUG: there is probably
    // a good way to remove the need for fully initializing our own local DBINFO.
    DBINFO dbi;

    dbi._vt = VT_EMPTY;
    // If an Object/Applet can be bound, it will be bound READ/WRITE.
    dbi._dwTransfer = 0;    // no HTML, R/W; different than default *pdbi

    Assert(pObj->Tag() == ETAG_OBJECT
            || pObj->Tag() == ETAG_APPLET
            || pObj->Tag() == ETAG_EMBED );

    // avoid asking for CLASSINFO unless we really have a need --

    if (id != ID_DBIND_DEFAULT)
    {
        CObjectElement::PARAMBINDING *pParamBinding;

        Assert(id - (ID_DBIND_DEFAULT + 1) >= 0);
        Assert(id  - (ID_DBIND_DEFAULT + 1) < pObj->_aryParamBinding.Size());

        pParamBinding = &pObj->_aryParamBinding[id - (ID_DBIND_DEFAULT + 1)];
        pObj->EnsureParamType(pParamBinding);
        if (pParamBinding->_vt == VT_EMPTY)
            goto Cleanup;

        if (!FormsIsEmptyString(pParamBinding->_strDataFld))
        {
            dbk = DBIND_SINGLEVALUE;
            dbi._vt = CVarType(pParamBinding->_vt, pParamBinding->_vt != VT_VARIANT);
        }
        else if (pParamBinding->_vt == VT_UNKNOWN || pParamBinding->_vt == VT_DISPATCH)
        {
            dbk = DBIND_IDATASOURCE;
        }

        goto Cleanup;
    }

    pci = pObj->GetClassInfo();

    //Assert(pObj->GetAAdataFld() || pObj->GetAAdataSrc());

    if (!pObj->GetAAdataFld())  // set-binding requested
    {
    // BUGBUG: check that it's not a read-only property
        if (pci->dispIDBind == pci->dispidIDataSource &&
            pci->dispidIDataSource != DISPID_UNKNOWN)
        {
            dbk = DBIND_IDATASOURCE;
        }
        else if (pci->dispidCursor != DISPID_UNKNOWN)
        {
            dbk = DBIND_ICURSOR;
        }
        goto Cleanup;
    }

    if (pci->dispIDBind != DISPID_UNKNOWN)
    {
        VARTYPE vtBind = pci->vtBindType & VT_TYPEMASK;
        dbk = DBIND_SINGLEVALUE;
        dbi._vt = CVarType(vtBind, vtBind != VT_VARIANT && !pObj->IsVTableValid());
    }

Cleanup:
    // Don't bind to objects that aren't safe for scripting
    if (dbk != DBIND_NONE && fReady && !pObj->IsSafeToScript())
    {
        dbk = DBIND_NONE;
    }

    if (pdbi && dbk != DBIND_NONE)
    {
        *pdbi = dbi;
    }
    return dbk;
}


//+----------------------------------------------------------------------------
//
//  Function: IsReadyImpl, CDBindMethods
//
//  Synopsis: Indicate whether the element is ready to be bound.
//
//  Arguments:  pElem       pointer to object element being queried
//
//-----------------------------------------------------------------------------

BOOL
CDBindMethodsObject::IsReadyImpl(CElement *pElem) const
{
    CObjectElement *pObj = DYNCAST(CObjectElement, pElem);

    return (pObj->_lReadyState >= READYSTATE_LOADED);
}

//+----------------------------------------------------------------------------
//
//  Function: GetNextDBSpecCustom, CDBindMethods
//
//  Synopsis: Inspect whatever attributes and styles necessary to determine
//            what bindings are specified in addition to the one on the
//            element's tag itself.
//  Arguments:
//            [pElem] - element being asked bout
//            [pid]   - pointer to ID before the first to be considered
//            [pdbs]  - pointer to struct to get spec; can't be NULL
//
//  Returns:  S_OK:     We have a spec, *pid and *pdbs filled in
//            S_FALSE:  No more specs
//
//  Notes:    ID_DBIND_DEFAULT refers to the elements itself.  Successive
//            values refer to param bindings.
//
//-----------------------------------------------------------------------------

HRESULT
CDBindMethodsObject::GetNextDBSpecCustom(CElement *pElem,
                                         LONG *pid,
                                         DBSPEC *pdbs) const
{
    HRESULT hr = S_FALSE;
    CObjectElement *pObj = DYNCAST(CObjectElement, pElem);
    LONG size = pObj->_aryParamBinding.Size();
    LONG id = *pid;

    CObjectElement::PARAMBINDING *pParamBinding;
    LONG iParamBinding;

    id ++;  // the next id we will try
    iParamBinding = id - (ID_DBIND_DEFAULT + 1);

    if (iParamBinding < 0 || iParamBinding >= size)
    {
        goto Cleanup;
    }

    pParamBinding = &pObj->_aryParamBinding[iParamBinding];
    pdbs->_pStrDataSrc = pParamBinding->_strDataSrc;
    pdbs->_pStrDataFld = pParamBinding->_strDataFld;
    pdbs->_pStrDataFormatAs = pParamBinding->_strDataFormatAs;

    *pid = id;
    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Function: GetIDForParamBinding, CObjectElement
//
//  Synopsis: For a given PARAMBINDING specification, make sure that we have
//            a dispid to be used to either put or get a property value.
//            Helper for BoundValueToElement and BoundValueFromElement.
//
//  Arguments:
//            [pParamBinding]  - PARAMBINDING structure with property name,
//                               and may or may not have filled in DISPIDs
//                               for getting and putting property values.
//            [fPut]           - Does caller want to a put a value (as opposed
//                               to fetching one?)
//
//  Returns:  S_OK             - Desired DISPID has been set filled in with
//                               somethint other than DISPID_UNKNOWN in the
//                               PARAMBINDING.
//            E_*              - some erorr
//
//-----------------------------------------------------------------------------

HRESULT
CObjectElement::GetIDForParamBinding(PARAMBINDING *pParamBinding, BOOL fPut)
{
    HRESULT hr = S_OK;
    DISPID *pdispid = fPut ? &pParamBinding->_dispidPut
                           : &pParamBinding->_dispidGet;
    DISPID dispid;
    LPTSTR strName;

    if (*pdispid != DISPID_UNKNOWN)
    {
        goto Cleanup;
    }

    strName = pParamBinding->_strParamName;

    CacheDispatch();
    if (!_pDisp)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(_pDisp->GetIDsOfNames(IID_NULL,
                                   &strName,
                                   1,
                                   g_lcidUserDefault,
                                   &dispid ) );
    if (!hr)
    {
        pParamBinding->_dispidPut = pParamBinding->_dispidGet = dispid;
    }
    else
    {
        static const TCHAR PUT_PREFIX[] = _T("set");
        static const TCHAR GET_PREFIX[] = _T("get");
        LPCTSTR strPrefix = fPut ? PUT_PREFIX : GET_PREFIX;
        LPTSTR strTemp =  new(Mt(CObjectElementGetIDForParamBinding_strTemp)) TCHAR[_tcslen(strPrefix) + _tcslen(strName) + 1];

        if (strTemp == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _tcscpy(strTemp, strPrefix);
        _tcscat(strTemp, strName);

        hr = THR(_pDisp->GetIDsOfNames(IID_NULL,
                                       &strTemp,
                                       1,
                                       g_lcidUserDefault,
                                       &dispid ) );
        delete [] strTemp;
        if (!hr)
        {
            *pdispid = dispid;
            pParamBinding->_dwInvokeFlags = DISPATCH_METHOD;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound OBJECT.  Only called if DBindKind
//            previous returned TRUE.
//
//  Arguments:
//            [id]      - ID of binding point.  id 0 is the object's
//                        defaultbind property.  Other IDs are PARAM bindings.
//            [pvData]  - pointer to data to transfer, datatype specified
//                        by object's typeinfo
//
//-----------------------------------------------------------------------------

HRESULT
CDBindMethodsObject::BoundValueToElement(CElement *pElem,
                                         LONG id,
                                         BOOL,
                                         LPVOID pvData) const
{
    HRESULT                     hr = S_OK;
    DISPID                      dispid;
    DWORD                       dwInvokeFlags = DISPATCH_PROPERTYPUT;
    CObjectElement             *pObj = DYNCAST(CObjectElement, pElem);
    EXCEPINFO                   except;

    // if the object isn't ready yet, don't do anything and return S_OK.
    // This happens, for example, for applets in a repeated table.
    if (pObj->_lReadyState < READYSTATE_COMPLETE)
        goto Cleanup;

    pObj->CacheDispatch();
    if (!pObj->_pDisp)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (id == ID_DBIND_DEFAULT)
    {
        CLASSINFO      *pci = pObj->GetClassInfo();

        if (pObj->IsVTableValid())
        {
            hr = pObj->VTableDispatch(pObj->_pDisp,
                            pci->vtBindType,
                            COleSite::VTBL_PROPSET,
                            pvData,
                            pci->uPutBindIndex);
            goto Cleanup;
        }

        dispid = pci->dispIDBind;
        dwInvokeFlags = pci->dwFlagsBind;
    }
    else
    {
        CObjectElement::PARAMBINDING *pParamBinding;

        pParamBinding = &pObj->_aryParamBinding[id - (ID_DBIND_DEFAULT + 1)];
        hr = pObj->GetIDForParamBinding(pParamBinding, TRUE);
        if (hr)
            goto Cleanup;
        dispid = pParamBinding->_dispidPut;
        dwInvokeFlags = pParamBinding->_dwInvokeFlags;
    }

    InitEXCEPINFO(&except);
    hr = THR(SetDispProp(pObj->_pDisp,
                         dispid,
                         g_lcidUserDefault,
                         (VARIANT *) pvData,
                         &except,
                         dwInvokeFlags));
    FreeEXCEPINFO(&except);

Cleanup:
    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueFromElement, CDBindMethods
//
//  Synopsis: Transfer data from bound OBJECT.  Only called if DBindKind
//            previous returned TRUE.
//
//  Arguments:
//            [id]      - ID of binding point.  id 0 is the object's
//                        defaultbind property.  Other IDs are PARAM bindings.
//            [pvData]  - pointer to buffer for transfer, datatype specified
//                        by object's typeinfo
//
//-----------------------------------------------------------------------------

HRESULT
CDBindMethodsObject::BoundValueFromElement(CElement *pElem,
                                           LONG id,
                                           BOOL,
                                           LPVOID pvData) const
{
    HRESULT                     hr;
    DISPID                      dispid;
    DWORD                       dwInvokeFlags = DISPATCH_PROPERTYGET;
    CObjectElement             *pObj = DYNCAST(CObjectElement, pElem);
    EXCEPINFO                   except;

    pObj->CacheDispatch();
    if (!pObj->_pDisp)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (id == ID_DBIND_DEFAULT)
    {
        CLASSINFO      *pci = pObj->GetClassInfo();

        if (pObj->IsVTableValid())
        {
            hr = pObj->VTableDispatch(pObj->_pDisp,
                            pci->vtBindType,
                            COleSite::VTBL_PROPGET,
                            pvData,
                            pci->uGetBindIndex);
            goto Cleanup;
        }

        dispid = pci->dispIDBind;
        dwInvokeFlags = pci->dwFlagsBind;
    }
    else
    {
        CObjectElement::PARAMBINDING *pParamBinding;

        pParamBinding = &pObj->_aryParamBinding[id - (ID_DBIND_DEFAULT + 1)];
        hr = pObj->GetIDForParamBinding(pParamBinding, TRUE);
        if (hr)
            goto Cleanup;
        dispid = pParamBinding->_dispidGet;
        dwInvokeFlags = pParamBinding->_dwInvokeFlags;
    }

    InitEXCEPINFO(&except);
    hr = THR(GetDispProp(pObj->_pDisp,
                         dispid,
                         g_lcidUserDefault,
                         (VARIANT *) pvData,
                         &except,
                         dwInvokeFlags));
    FreeEXCEPINFO(&except);
    if (hr)
        goto Cleanup;

Cleanup:
    return(hr);
}


//+----------------------------------------------------------------------------
//
//  Function: EnsureParamType (private helper)
//
//  Synopsis: Determine the preferred type of a param binding
//
//  Arguments:
//            pParamBinding     binding whose type we want
//
//-----------------------------------------------------------------------------

void
CObjectElement::EnsureParamType(PARAMBINDING *pParamBinding)
{
    HRESULT     hr;
    DISPID      dispid;
    EXCEPINFO   except;
    VARIANT     var;
    CLASSINFO   *pci;

    if (pParamBinding->_vt != VT_EMPTY)     // we've already found the type
        goto Cleanup;

    if (_lReadyState < READYSTATE_LOADED)   // we're not ready to answer
        goto Cleanup;

    CacheDispatch();                        // need _pDisp to do anything
    if (!_pDisp)
        goto Cleanup;

    // get the dispid for the property
    hr = GetIDForParamBinding(pParamBinding, FALSE);
    if (hr)
        goto Cleanup;
    dispid = pParamBinding->_dispidGet;

    // Read the property from the control
    VariantInit(&var);
    InitEXCEPINFO(&except);
    hr = THR(GetDispProp(_pDisp,
                         dispid,
                         g_lcidUserDefault,
                         &var,
                         &except,
                         pParamBinding->_dwInvokeFlags));
    FreeEXCEPINFO(&except);
    if (hr)
        goto Cleanup;

    // remember its type
    switch (var.vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        pParamBinding->_vt = VT_VARIANT;
        break;
    case VT_UNKNOWN:
    case VT_DISPATCH:
        // in this case, we need to get the typeinfo to see if the DataSource
        // property is put or putref
        pParamBinding->_vt = var.vt;
        pci = GetClassInfo();
        if (dispid == pci->dispidIDataSource)
        {
            pParamBinding->_dwInvokeFlags = pci->dwFlagsDataSource;
        }
        else if (dispid == pci->dispidRowset)
        {
            pParamBinding->_dwInvokeFlags = pci->dwFlagsRowset;
        }
        break;
    default:
        pParamBinding->_vt = var.vt;
        break;
    }
    VariantClear(&var);

Cleanup:
    return;
}

//+-------------------------------------------------------------------------
//
//  Method:     CObjectElement::DeferredSaveData
//
//  Synopsis:   If the Site was considered dirty, then save the data in a bound
//              element to what is it bound to (often, a database).
//              It is no longer dirty after flush.  Works on the default
//              binding only, not parameter bindings.
//
//  Arguments:  DWORD trickily encodes the bound id.  Because GWKillMethodCall
//              treats dw == 0 as a request to kill all calls no matter what
//              dw, we have to make sure that dw == 0 represents ID_DBIND_ALL.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

void
CObjectElement::DeferredSaveData(DWORD_PTR dw)
{
    // note use of super:: -- we don't need to call GWKillMethodCall, which
    //  CObjectElement::SaveDataIfChanged would do.

    // ID_DBIND_ALL adjusment is so that dw == 0 maps to id == ID_DBIND_ALL
    IGNORE_HR(super::SaveDataIfChanged((DWORD)dw + ID_DBIND_ALL, /* fLoud */ FALSE));
}


//+-------------------------------------------------------------------------
//
//  Method:     CObjectElement::SaveDataIfChanged, CSite
//
//  Synopsis:   Determine whether or not is appropate to save the value
//              in  a control to a datasource, and do so.  Fire any appropriate
//              events.
//
//  Returns:    S_OK: no work to do, or transfer successful
//
//--------------------------------------------------------------------------
HRESULT
CObjectElement::SaveDataIfChanged(LONG id, BOOL fLoud, BOOL fForceIsCurrent)
{
    // We kill any pending defered calls to ourselves, just in case we got here
    // other than from a defered call.
    // ID_DBIND_ALL adjustment is so that dw == 0 maps to id == ID_DBIND_ALL
    GWKillMethodCall(this, ONCALL_METHOD(CObjectElement, DeferredSaveData, deferredsavedata), (DWORD_PTR)(id - ID_DBIND_ALL));

    RRETURN1(super::SaveDataIfChanged(id, fLoud, fForceIsCurrent), S_FALSE);
}
#endif // ndef NO_DATABINDING

//+-------------------------------------------------------------------------
//
//  Method:     CObjectElement::OnControlRequestEdit, COleSite
//
//  Synopsis:   Give Site (or its derived class) a chance to act on a
//              control's OnRequestEdit notification,  and cancel any forwarding
//              which might normally take place through our XObject.
//
//  Arguments:  dispid of property who's value has changed
//
//  Returns:    HRESULT.  Any non-zero HRESULT, including S_FALSE, will
//              cancel OnRequestEdit forwarding.
//
//--------------------------------------------------------------------------

HRESULT
CObjectElement::OnControlRequestEdit(DISPID dispid)
{
    HRESULT             hr;
    DBMEMBERS          *pdbm;
    CLASSINFO          *pci;
    PARAMBINDING       *pParamBinding;
    int                 cParamBindings;

    hr = super::OnControlRequestEdit(dispid);
    if (hr)
    {
        goto Cleanup;
    }

    if (dispid == DISPID_UNKNOWN)
    {
        goto Cleanup;
    }

    if (_state < OS_LOADED)
    {
        // don't know if this check is necessary here, but it was necessary in
        // OnControlChanged.
        goto Cleanup;
    }

#ifndef NO_DATABINDING
    pdbm = GetDBMembers();
    if (!pdbm)
    {
        goto Cleanup;
    }

    cParamBindings = _aryParamBinding.Size();
    pParamBinding = &_aryParamBinding[cParamBindings];

    while (pParamBinding--, cParamBindings--)
    {
        if (pParamBinding->_dispidGet == dispid)
        {
            if (FAILED(pdbm->CheckSrcWritable(this,
                                 cParamBindings + (ID_DBIND_DEFAULT+1) ) ) )
            {
                hr = S_FALSE;
                goto Cleanup;
            }
        }
    }

    // we've considered the PARAM bindings; now consider the default
    //  binding.

    // Avoid accessing the ClassInfo unless we know that ID_DBIND_DEFAULT
    //  is bound.
    if (!pdbm->FBoundID(this, ID_DBIND_DEFAULT))
    {
        goto Cleanup;
    }

    // Now that we're sure we need it, get the ClassInfo
    pci = GetClassInfo();
    if (dispid != pci->dispIDBind)
    {
        goto Cleanup;
    }

    if (FAILED(pdbm->CheckSrcWritable(this, ID_DBIND_DEFAULT)))
    {
        hr = S_FALSE;
    }
#endif // ndef NO_DATABINDING

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CObjectElement::OnControlChanged, COleSite
//
//  Synopsis:   Give Site (or its derived class) a chance to act on a
//              control's OnChanged notification,  and cancel any forwarding
//              which might normally take place through our XObject.
//
//  Arguments:  dispid of property who's value has changed
//
//  Returns:    HRESULT.  Any non-zero HRESULT, including S_FALSE, will
//              cancel OnChanged forwarding.
//
//  BUGBUG:     We don't have code here to restore the dispid back to
//              the bound value, if the bound source isn't writable.
//              Probably should have a DeferedRestoreData.
//
//--------------------------------------------------------------------------

HRESULT
CObjectElement::OnControlChanged(DISPID dispid)
{
    HRESULT             hr          = S_OK;
    DBMEMBERS           *pdbm;
    CLASSINFO           *pci;
    CDoc *              pDoc = Doc();
    CDataMemberMgr *    pdmm = GetDataMemberManager();

    // handle changes in readystate
    Verify(!super::OnControlChanged(dispid));

    if (_state < OS_LOADED)
    {
        // (alexz) (anandra)
        // this is done specifically for Marquee control of IE 3.0,
        // which has DISPID_ScrollStyleX == DISPID_VALUE == 0 (which is a bug).
        // When loading from param bag, it fires OnPropertyChange (DISPID_ScrollStyleX),
        // which leads our code here as we think that was an OnValuePropertyChange; then it
        // causes save, which crashes that control because it's code happened to be
        // not robust for the case when somebody operates with it's properties while it is in
        // process of loading.
        goto Cleanup;
    }

#ifndef NO_DATABINDING
    // if the databinding interface changed, let the provider know
    if (pdmm)
    {
        if (dispid == pdmm->GetDatabindingDispid() || dispid == DISPID_UNKNOWN)
        {
            IGNORE_HR(pdmm->ChangeDataBindingInterface(NULL, TRUE));
        }
    }

    pdbm = GetDBMembers();
    if (!pdbm)
    {
        goto Cleanup;
    }

    if (this != pDoc->_pElemCurrent)
    {
        int           cParamBindings = _aryParamBinding.Size();
        PARAMBINDING *pParamBinding  = &_aryParamBinding[cParamBindings];

        while (pParamBinding--, cParamBindings--)
        {
            if ((dispid == DISPID_UNKNOWN || pParamBinding->_dispidGet == dispid)
                && pdbm->CompareWithSrc(this, cParamBindings + (ID_DBIND_DEFAULT+1)) )
            {
                // note two adjustments here
                //  ID_DBIND_DEFAULT+1 adjust array offset to binding id.
                //  -ID_DBIND_ALL adjusts binding id to required DWORD value
                //      (see DeferSaveData for details)
                GWPostMethodCall(this,
                                 ONCALL_METHOD(CObjectElement, DeferredSaveData, deferredsavedata),
                                 (DWORD_PTR) cParamBindings + (ID_DBIND_DEFAULT+1) - ID_DBIND_ALL,
                                 FALSE, "CObjectElement::DeferredSaveData");
            }
        }
    }

    // we only have to consider ClassInfo information if we have a binding
    //  on the tag itself
    if (!pdbm->FBoundID(this, ID_DBIND_DEFAULT))
    {
        goto Cleanup;
    }

    // Now that we're sure we need it, get the ClassInfo
    pci = GetClassInfo();

    if (dispid != DISPID_UNKNOWN && dispid != pci->dispIDBind)
    {
        goto Cleanup;
    }

    // for current control, we only transfer value now if IMMEDIATEBIND.
    if (this == pDoc->_pElemCurrent && !pci->FImmediateBind())
    {
        goto Cleanup;
    }


    // fetch that value
    if (!pdbm->CompareWithSrc(this, ID_DBIND_DEFAULT))
    {
        goto Cleanup;
    }

    // See DeferredSaveData for explanation of  "- ID_DBIND_ALL"
    GWPostMethodCall(this,
                     ONCALL_METHOD(CObjectElement, DeferredSaveData, deferredsavedata),
                     (DWORD_PTR) (ID_DBIND_DEFAULT - ID_DBIND_ALL), FALSE, "CObjectElement::DeferredSaveData");

#endif // ndef NO_DATABINDING
Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     OnFailToCreate
//
//  Synopsis:   Called when an OBJECT fails to instantiate correctly
//
//-------------------------------------------------------------------------

HRESULT
CObjectElement::OnFailToCreate()
{
    CDoc *  pDoc = Doc();

    Assert((pDoc->_aryDefunctObjects.Find(this) == -1) && "OnFailToCreate called more than once!");
    pDoc->_aryDefunctObjects.Append(this);

    RRETURN (THR(super::OnFailToCreate()));
}


//+------------------------------------------------------------------------
//
//  Member:     GetSumbitInfo
//
//  Synopsis:   Retrieve info for form sumbission and give it to the
//              CPostData.
//
//-------------------------------------------------------------------------

HRESULT
CObjectElement::GetSubmitInfo(CPostData *pSubmitData)
{
    HRESULT            hr = S_FALSE;
    const CCharFormat *pCF;
    CVariant           vt;
    LPCTSTR            lpstrName;


    // Make sure we've actually got a control.
    if(!_pDisp)
        goto Cleanup;

    // If it doesn't have a name, then it doesn't go in the string!
    lpstrName = GetAAname();
    if(!lpstrName)
        goto Cleanup;

    // Get the CharFormat for the LCID.
    pCF = GetFirstBranch()->GetCharFormat();
    if(!pCF)
        goto Cleanup;

    // Finally, get the default value.
    hr = THR(GetDispProp(_pDisp, DISPID_VALUE, pCF->_lcid, &vt));
    if(hr)
        goto Cleanup;

    hr = THR(vt.CoerceVariantArg(VT_BSTR));
    if(hr)
        goto Cleanup;

    hr = THR(pSubmitData->AppendNameValuePair(lpstrName, V_BSTR(&vt), Doc()));

Cleanup:
    // If we got any kind of error, return S_FALSE
    // so that we're just skipped in the submit info.
    if(hr)
    {
        hr = S_FALSE;
    }
    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
CObjectElement::get_readyState(VARIANT *pVarResult)
{
    HRESULT     hr;

    if (!pVarResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = get_readyState(&V_I4(pVarResult));
    if (!hr)
        V_VT(pVarResult) = VT_I4;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP
CObjectElement::get_readyState(long *pReadyState)
{
    HRESULT     hr = S_OK;

    if (!pReadyState)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pReadyState = _lReadyState;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP CObjectElement::put_classid(BSTR v)
{
    HRESULT hr = CTL_E_SETNOTSUPPORTEDATRUNTIME;
    CVoid *pSubObj;
    if (_pUnkCtrl || GetAAclassid() || GetAAcodeType() || GetAAtype())
        goto Cleanup;

    RECALC_PUT_HELPER(s_propdescCObjectElementclassid.b.dispid)
    pSubObj = CVOID_CAST(GetAttrArray());
    hr = THR(s_propdescCObjectElementclassid.b.SetStringProperty(v, this, pSubObj));
    if (hr)
        goto Cleanup;

    hr = THR(CreateObject());

Cleanup:
    return SetErrorInfo(hr);
}

STDMETHODIMP CObjectElement::get_classid(BSTR *p)
{
    RECALC_GET_HELPER(s_propdescCObjectElementclassid.b.dispid)
    CVoid *pSubObj = CVOID_CAST(GetAttrArray());
    return SetErrorInfo(s_propdescCObjectElementclassid.b.GetStringProperty(p, this, pSubObj));
}

STDMETHODIMP CObjectElement::put_codeType(BSTR v)
{
    HRESULT hr = CTL_E_SETNOTSUPPORTEDATRUNTIME;
    CVoid *pSubObj;
    if (_pUnkCtrl || GetAAcodeType() || GetAAclassid() || GetAAtype())
        goto Cleanup;

    RECALC_PUT_HELPER(s_propdescCObjectElementcodeType.b.dispid)
    pSubObj = CVOID_CAST(GetAttrArray());
    hr = THR(s_propdescCObjectElementcodeType.b.SetStringProperty(v, this, pSubObj));
    if (hr)
        goto Cleanup;

    hr = THR(CreateObject());

Cleanup:
    return SetErrorInfo(hr);
}

STDMETHODIMP CObjectElement::get_codeType(BSTR *p)
{
    RECALC_GET_HELPER(s_propdescCObjectElementcodeType.b.dispid)
    CVoid *pSubObj = CVOID_CAST(GetAttrArray());
    return SetErrorInfo(s_propdescCObjectElementcodeType.b.GetStringProperty(p, this, pSubObj));
}

STDMETHODIMP CObjectElement::put_type(BSTR v)
{
    HRESULT hr = CTL_E_SETNOTSUPPORTEDATRUNTIME;
    CVoid *pSubObj;
    if (_pUnkCtrl || GetAAtype() || GetAAclassid() || GetAAcodeType())
        goto Cleanup;

    RECALC_PUT_HELPER(s_propdescCObjectElementtype.b.dispid)
    pSubObj = CVOID_CAST(GetAttrArray());
    hr = THR(s_propdescCObjectElementtype.b.SetStringProperty(v, this, pSubObj));
    if (hr)
        goto Cleanup;

    hr = THR(CreateObject());

Cleanup:
    return SetErrorInfo(hr);
}

STDMETHODIMP CObjectElement::get_type(BSTR *p)
{
    RECALC_GET_HELPER(s_propdescCObjectElementtype.b.dispid)
    CVoid *pSubObj = CVOID_CAST(GetAttrArray());
    return SetErrorInfo(s_propdescCObjectElementtype.b.GetStringProperty(p, this, pSubObj));
}
