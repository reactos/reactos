//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       sholder.cxx
//
//  Contents:   Implementation of CScriptHolder class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_SHOLDER_HXX_
#define X_SHOLDER_HXX_
#include "sholder.hxx"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

const CLSID CLSID_JScript  = {0xF414C260, 0x6AC0, 0x11CF, 0xB6, 0xD1, 0x00, 0xAA, 0x00, 0xBB, 0xBB, 0x58};

EXTERN_C const IID IID_IHTMLDialog;

#ifndef NO_SCRIPT_DEBUGGER
extern interface IDebugApplication *g_pDebugApp;
#endif // ndef NO_SCRIPT_DEBUGGER

MtDefine(CScriptHolder, ObjectModel, "CScriptHolder")
DeclareTag(tagScriptSite, "Script Holder", "Script Holder methods")
ExternTag(tagDisableLockAR);


CScriptHolder::CLock::CLock(CScriptHolder *pHolder, WORD wLockFlags)
{
    _pHolder = pHolder;
    _wLock = pHolder->_wLockFlags;
#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        _pHolder->AddRef();
    }
    
    pHolder->_wLockFlags |= wLockFlags;
}


CScriptHolder::CLock::~CLock()
{
    _pHolder->_wLockFlags = _wLock;
#if DBG==1
    if (!IsTagEnabled(tagDisableLockAR))
#endif
    {
        _pHolder->Release();
    }
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::CScriptHolder
//
//  Synopsis:   ctor
//
//---------------------------------------------------------------------------

CScriptHolder::CScriptHolder(CScriptCollection *pCollection) : _clsid(g_Zero.clsid)
{
    _ulRefs = 1;
    _pCollection = pCollection;
    _pScript = NULL;
    _pScriptParse = NULL;
    _pParseProcedure = NULL;
    _fCaseSensitive = FALSE;
    _wLockFlags = 0;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::~CScriptHolder
//
//  Synopsis:   dtor
//
//---------------------------------------------------------------------------

CScriptHolder::~CScriptHolder()
{
    IGNORE_HR(Close());
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::Close
//
//  Synopsis:   Close the script and clear interfaces to it.
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::Close()
{
    TraceTag((tagScriptSite, "Close"));

    HRESULT hr = S_OK;

    ClearInterface (&_pParseProcedure);
    ClearInterface(&_pScriptParse);
    ClearInterface(&_pScriptDebug);

    if (_pScript)
    {
        hr = THR(_pScript->Close());
    }

    ClearInterface(&_pScript);
    _pCollection = NULL;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::IllegalCall
//
//  Synopsis:   Return TRUE/FALSE if this method call is legal.
//
//---------------------------------------------------------------------------

BOOL
CScriptHolder::IllegalCall()
{
    if (!_pCollection || !_pCollection->Doc())
        return TRUE;
        
    if (_pCollection->Doc()->_dwTID != GetCurrentThreadId())
    {
        Assert(0 && "Script site called across apartment thread boundary (not an MSHTML bug)");
        return TRUE;
    }

    return FALSE;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::QueryInterface
//
//  Synopsis:   per IUnknown
//
//---------------------------------------------------------------------------

STDMETHODIMP
CScriptHolder::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (IllegalCall())
        RRETURN(E_NOINTERFACE);
        
    if (iid == IID_IActiveScriptSite ||
        iid == IID_IUnknown)
    {
        *ppv = (IActiveScriptSite *) this;
    }
    else if (iid == IID_IActiveScriptSiteWindow)
    {
        *ppv = (IActiveScriptSiteWindow *) this;
    }
    else if (iid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *)this;
    }
#ifndef NO_SCRIPT_DEBUGGER
    else if (iid == IID_IActiveScriptSiteDebug)
    {
        *ppv = (IActiveScriptSiteDebug *)this;
    }
#endif
    else if (iid == IID_IOleCommandTarget)
    {
        *ppv = (IOleCommandTarget *)this;
    }
    else if (iid == IID_IActiveScriptSiteInterruptPoll)
    {
        *ppv = (IActiveScriptSiteInterruptPoll *)this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::AddRef
//
//  Synopsis:   per IUnknown
//
//---------------------------------------------------------------------------

ULONG
CScriptHolder::AddRef()
{
    _ulRefs += 1;
    return _ulRefs;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::Release
//
//  Synopsis:   per IUnknown
//
//---------------------------------------------------------------------------

ULONG
CScriptHolder::Release()
{
    if (--_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }

    return _ulRefs;
}

// BUGBUG: ***TLL*** #define needs to go when new script engine and activscp.idl.
#define SCRIPTPROP_CONVERSIONLCID        0x00001002

void CScriptHolder::TurnOnFastSinking ()
{
    IActiveScriptProperty *pprop;

    Assert(_pScript);

    if (SUCCEEDED(_pScript->QueryInterface(IID_IActiveScriptProperty, (void **)&pprop)))
    {
        CVariant varValue(VT_BOOL);

        V_BOOL(&varValue) = VARIANT_TRUE;

        pprop->SetProperty(SCRIPTPROP_HACK_TRIDENTEVENTSINK, NULL, &varValue);

        pprop->Release();
    }
}


void CScriptHolder::SetConvertionLocaleToENU()
{
    IActiveScriptProperty *pprop;

    Assert(_pScript);

    if (SUCCEEDED(_pScript->QueryInterface(IID_IActiveScriptProperty, (void **)&pprop)))
    {
        CVariant varLocale(VT_I4);

        V_I4(&varLocale) = 0x409;       // English ENU

        // Turn on ENU for script locale.
        pprop->SetProperty(SCRIPTPROP_CONVERSIONLCID, NULL, &varLocale);

        pprop->Release();
    }
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::Init
//
//  Synopsis:   Second phase of construction.
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::Init(
        CBase *pBase,
        IActiveScript *pScript,
        IActiveScriptParse *pScriptParse,
        CLSID *pclsid)
{
    TraceTag((tagScriptSite, "Init"));

    HRESULT                         hr;
    long                            c;
    CNamedItem **                   ppNamedItem;
    SCRIPTSTATE                     ss = SCRIPTSTATE_STARTED;

    ReplaceInterface(&_pScript, pScript);
    ReplaceInterface(&_pScriptParse, pScriptParse);

    if (pclsid)
    {
        _clsid = *pclsid;
    }

    _pScriptDebug = 0;

    // BUGBUG (alexz) - we need some generic protocol allowing us to ask script engine
    // about it's case-sensitivity.
    if (IsEqualGUID(_clsid, CLSID_JScript))
    {
        _fCaseSensitive = TRUE;
    }

    // If script engine supports fast sinking (uses ITridentEventSink) then tell
    // the script engine we support it.
    TurnOnFastSinking();

    if (_pScriptParse)
    {
        hr = THR(_pScriptParse->InitNew());
        if (hr)
            goto Cleanup;
    }
    
    hr = THR(_pScript->SetScriptSite(this));
    if (hr)
        goto Cleanup;

    // Need to set conversion locale after call to SetScriptSite, as LCID is over-written
    // in a callback to GetLCID. (IE5 Bug# 83217)
    // 
    // Removing call to SetConvertionLocaleToENU()  (IE5 Bug 86509)
    //
    // SetConvertionLocaleToENU();

    // we check the state of the scriptengine first, there are objects out there
    // who connect during their download time (HTMLLAYOUT)
    hr = THR(_pScript->GetScriptState(&ss));
    if (hr)
    {
        if (E_NOTIMPL != hr)
            goto Cleanup;

        ss = SCRIPTSTATE_UNINITIALIZED;
        hr = S_OK;
    }

    if (ss == SCRIPTSTATE_INITIALIZED || ss == SCRIPTSTATE_UNINITIALIZED)
    {
        // This is a requirement for gets VBScript/JScript into a runnable state.

#ifdef NEVER
        //
        // Not used anymore because we won't treat solstice as a 
        // script engine due to security problems.
        //
        
        // BAD BETA 1 Bugfix for 24019
        // GaryBu and frankman decided to use the same for BETA2 due to simplicity reasons
        // we should change this to compat flags in case we have to do anything else
        // for the layoutcontrol
        if (!IsEqualCLSID(*pclsid, CLSID_IISForm))
#endif
        {
            // we ignore the error because some script engines might not support this operation
            // (e.g. java vm 2057.1+), yet we still want to finish creating script holder for them
            IGNORE_HR(_pScript->SetScriptState(SCRIPTSTATE_STARTED));
        }
    }

    hr = THR(_pCollection->Doc()->EnsureOmWindow());
    if (hr)
        goto Cleanup;

    //
    // Add the window as a global named item.
    //
    
    hr = THR(_pScript->AddNamedItem(
        DEFAULT_OM_SCOPE,
        SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE | SCRIPTITEM_GLOBALMEMBERS));
    if (S_OK == hr)
    {
        //
        // So the script engine supports operation AddNamedItem.
        //
        // Now add any existing forms also as named items.
        //

        for (c = _pCollection->_NamedItemsTable.Size(), 
                ppNamedItem = _pCollection->_NamedItemsTable;
             c > 0; 
             c--, ppNamedItem++)
        {
            hr = THR(_pScript->AddNamedItem(
                (*ppNamedItem)->_cstrName,
                SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        Assert (hr);
        if (E_NOTIMPL == hr)
        {
            // we ignore the error because some script engines might not support this operation
            // (e.g. java vm 2057.1+), yet we still want to finish creating script holder for them
            hr = S_OK;
        }
        else
            goto Cleanup;
    }


    //
    // cache _pParseProcedure
    //

    Assert (!_pParseProcedure);

    // if the QI fails that means that the script engine does not support the new IASPP code construction
    if (THR_NOTRACE(_pScript->QueryInterface(
        IID_IActiveScriptParseProcedure2, 
        (void**)&_pParseProcedure)))
    {
        // Try the old IASPP IID for old engines that haven't been re-compiled.
        THR_NOTRACE(_pScript->QueryInterface(
            IID_IActiveScriptParseProcedure, 
            (void**)&_pParseProcedure));
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::SetScriptState
//
//  Synopsis:   Set the script to some state using enum SCRIPTSTATE
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::SetScriptState(SCRIPTSTATE ss)
{
    TraceTag((tagScriptSite, "SetScriptState"));

    HRESULT hr = S_OK;
    
    if (!_pScript)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(_pScript->SetScriptState(ss));
    if (E_NOTIMPL == hr)
    {
        hr = S_OK;
    }

    // script engines which do not support IActiveScriptParseProcedure (as indicated by
    // condition (!_pParseProcedure)) might rely on actions (AddScriptlet calls) done in 
    // BuildObjectTypeInfo, so for them we ensure ObjectTypeInfo
    if (!_pParseProcedure && SCRIPTSTATE_CONNECTED == ss)
    {
        hr = THR(_pCollection->Doc()->EnsureObjectTypeInfo());
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::GetLCID
//
//  Synopsis:   per IActiveScriptSite
//
//  Arguments:  plcid       Returned locale info
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::GetLCID(LCID *plcid)
{
    TraceTag((tagScriptSite, "GetLCID"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    // Return the user locale ID, this make VBS locale sensitive and means
    // that VBS pages can not be written to be locale neutral (a page cannot
    // be written to run on different locales).  Only JScript can do this. 
    // However, VBS can be written for Intra-net style apps where functions
    // like string comparisons, UCase, LCase, etc. are used.  Also, some
    // pages could be written in both VBS and JS.  Where JS has the locale 
    // neutral things like floating point numbers, etc.
    *plcid = g_lcidUserDefault;

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::GetItemInfo
//
//  Synopsis:   per IActiveScriptSite
//
//  Arguments:  pstrName        Name to get info of
//              dwReturnMask    Mask for which item are needed
//              ppunkItemOut    IUnknown for item returned
//              pptinfoOut      ITypeInfo for item returned
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::GetItemInfo(
      LPCOLESTR   pchName,
      DWORD       dwReturnMask,
      IUnknown**  ppunkItemOut,
      ITypeInfo** pptinfoOut)
{
    TraceTag((tagScriptSite, "GetItemInfo"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    HRESULT         hr = S_OK;
    IUnknown *      pUnkItem = NULL;
    IDispatch *     pDispItem = NULL;

    //
    // validation
    //

    if (!_pCollection)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
    {
        if (!pptinfoOut)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        *pptinfoOut = NULL;
    }

    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
    {
        if (!ppunkItemOut)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        *ppunkItemOut = NULL;
    }


    //
    // find the item
    //

    if (StrCmpIC(DEFAULT_OM_SCOPE, pchName) == 0)
    {
        // this will get either private pDisp of _pOmWindow or it's aggregating object's pDisp
        hr = THR(_pCollection->Doc()->_pOmWindow->QueryInterface(IID_IDispatch, (void**)&pDispItem));
        if (hr)
            goto Cleanup;

        pUnkItem = pDispItem;
        pUnkItem->AddRef();
    }
    else
    {
        hr = THR(_pCollection->_NamedItemsTable.GetItem((LPTSTR)pchName, &pUnkItem));
        if (hr)
            goto Cleanup;

        if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
        {
            hr = THR(pUnkItem->QueryInterface(IID_IDispatch, (void**)&pDispItem));
            if (hr)
                goto Cleanup;
        }
    }

    //
    // get the info
    //

    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
    {
        *ppunkItemOut = pUnkItem;
        (*ppunkItemOut)->AddRef();
    }

    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
    {
        hr = THR(pDispItem->GetTypeInfo(
            0,
            LOCALE_SYSTEM_DEFAULT,
            pptinfoOut));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    ReleaseInterface(pUnkItem);
    ReleaseInterface(pDispItem);
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::GetDocVersionString
//
//  Synopsis:   per IActiveScriptSite
//
//  Arguments:  pbstrVersion    Returned version string
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::GetDocVersionString(BSTR *pbstrVersion)
{
    TraceTag((tagScriptSite, "GetDocVersionString"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    // BUGBUG need to return empty stirng?
    return E_NOTIMPL;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::RequestItems
//
//  Synopsis:   per IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::RequestItems()
{
    TraceTag((tagScriptSite, "RequestItems"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    if (!_pCollection)
        RRETURN(E_UNEXPECTED);

    // BUGBUG add named items
    return E_NOTIMPL;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::RequestTypeLibs
//
//  Synopsis:   per IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::RequestTypeLibs()
{
    TraceTag((tagScriptSite, "RequestTypeLibs"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    if (!_pCollection)
        RRETURN(E_UNEXPECTED);

    // BUGBUG add type library
    return E_NOTIMPL;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::OnScriptTerminate
//
//  Synopsis:   per IActiveScriptSite
//
//  Arguments:  pvarResult      Resultant variant
//              pexcepinfo      Exception info for errors
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::OnScriptTerminate(
    const VARIANT *pvarResult,
    const EXCEPINFO *pexcepinfo)
{
    TraceTag((tagScriptSite, "OnScriptTerminate"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    if (!_pCollection)
        RRETURN(E_UNEXPECTED);

    // BUGBUG what should we do?
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::OnStateChange
//
//  Synopsis:   per IActiveScriptSite
//
//  Arguments:  ssScriptState   New state of script
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::OnStateChange(SCRIPTSTATE ssScriptState)
{
    TraceTag((tagScriptSite, "OnStateChange"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    if (!_pCollection)
        RRETURN(E_UNEXPECTED);

    // BUGBUG what should we do?
    return S_OK;
}

BOOL OutLook98HackReqd(CDoc *pDoc, ULONG uLine, ULONG uCol)
{
    if (((36 == uLine) && (35 == uCol) ||
        (51 == uLine) && (28 == uCol)) && 
        !_tcsicmp(pDoc->_cstrUrl, _T("outday://")))
        return TRUE;

    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::OnScriptError
//
//  Synopsis:   per IActiveScriptSite
//
//  Arguments:  pScriptError    Error info interface
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::OnScriptError(IActiveScriptError *pScriptError)
{
    TraceTag((tagScriptSite, "OnScriptError"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    HRESULT          hr = S_OK;
    CDoc           * pDoc;
    ErrorRecord      errRecord;

    if (!_pCollection || (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_ALERTS)))
        RRETURN(E_UNEXPECTED);

    if (TestLock(SCRIPTLOCK_SCRIPTERROR))
        goto Cleanup;

    {
        CLock   Lock(this, SCRIPTLOCK_SCRIPTERROR);

        pDoc = _pCollection->Doc();
        Assert(pDoc);

        Assert(!!pDoc->_cstrUrl);

        hr = errRecord.Init(pScriptError, pDoc);
        if (hr)
            goto Cleanup;

        // HACK fix for IE5 bug# 58568
        if (OutLook98HackReqd(pDoc, errRecord._uLine, errRecord._uColumn))
            goto Cleanup;

#ifndef NO_SCRIPT_DEBUGGER
        if (errRecord._pScriptDebugDocument)
        {
            BOOL        fEnterDebugger;

            hr = THR(DoYouWishToDebug(pScriptError, &fEnterDebugger));
            if (hr)
                goto Cleanup;

            if (fEnterDebugger)
            {
                hr = THR(errRecord._pScriptDebugDocument->ViewSourceInDebugger(
                    errRecord._uLine - 1, errRecord._uColumn));
            }

            goto Cleanup; // done
        }
#endif // NO_SCRIPT_DEBUGGER

        if (pDoc->_pOmWindow && !TestLock(SCRIPTLOCK_FIREONERROR))
        {
            CLock   Lock(this, SCRIPTLOCK_FIREONERROR);

            hr = pDoc->ReportScriptError(errRecord);
        }
    }
    
Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::OnEnterScript
//
//  Synopsis:   per IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::OnEnterScript()
{
    HRESULT hr;

    TraceTag((tagScriptSite, "OnEnterScript"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    if (!_pCollection)
        RRETURN(E_UNEXPECTED);

#ifndef NO_SCRIPT_DEBUGGER
    if (_pCollection->_pCurrentDebugDocument)
    {
        IGNORE_HR(_pCollection->_pCurrentDebugDocument->UpdateDocumentSize());
    }
#endif

    hr = THR(_pCollection->Doc()->EnterScript());

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::OnLeaveScript
//
//  Synopsis:   per IActiveScriptSite
//
//  Arguments:  plcid       Returned locale info
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::OnLeaveScript()
{
    TraceTag((tagScriptSite, "OnLeaveScript"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    if (!_pCollection)
        RRETURN(E_UNEXPECTED);

    _pCollection->Doc()->LeaveScript();

    return(S_OK);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::DoYouWishToDebug
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::DoYouWishToDebug(IActiveScriptError * pScriptError, BOOL * pfEnterDebugger)
{
    int                 iRes = IDYES;
    CDoc               *pDoc;
    HRESULT             hr = S_OK;
    BOOL                fExcepInfo = TRUE;
    CExcepInfo          ExcepInfo;
    ULONG               ulLine = 0;
    LONG                ichError;
    DWORD               dwSrcContext;
    TCHAR              *pchDescription = NULL;
    TCHAR               achError[256];
    TCHAR               achDescription[512];

    *pfEnterDebugger = FALSE; 

    pDoc = _pCollection->Doc();
    Assert(pDoc);

    // Stack overflow
    if (pDoc->_fStackOverflow || pDoc->_fOutOfMemory)
    {
        // If we are in last script, stack has been unwound, so prevent 2nd dialog in OnLeaveScript
        if (pDoc->_cScriptNesting == 1)
        {
            pDoc->_fStackOverflow = FALSE;
            pDoc->_fOutOfMemory = FALSE;
        }

        goto Cleanup;
    }

    // Get the ExcepInfo
    if (!(pDoc->_dwLoadf & DLCTL_SILENT))
    {
        // BUGBUG (alexz) this should use ErrorRecord - otherwise this is largely duplicated code

        hr = THR(pScriptError->GetExceptionInfo(&ExcepInfo));
        if (!hr)
        {
            // If the script was aborted (through the script debugger),
            // don't put up any UI.
            if (ExcepInfo.scode == E_ABORT)
                goto Cleanup;

            pScriptError->GetSourcePosition(&dwSrcContext, &ulLine, &ichError);

            // HACK fix for IE5 bug# 58568
            if (OutLook98HackReqd(pDoc, ulLine, ichError))
                goto Cleanup;

            // vbscript passes empty strings and jscript passes NULL, so check for both
            if (ExcepInfo.bstrDescription && *ExcepInfo.bstrDescription)
            {
                pchDescription = ExcepInfo.bstrDescription;
            }
            else
            {
                GetErrorText(ExcepInfo.scode, achError, ARRAY_SIZE(achError));
                pchDescription = achError;
            }
        }
        else
        {
            fExcepInfo = FALSE;
        }

        if (!hr)
        {
            // Note that ulLine MUST have been set if HR == S_OK (0).
            hr = Format(0, achDescription, ARRAY_SIZE(achDescription), MAKEINTRESOURCE(IDS_FMTDEBUGCONTINUE),
                        GetResourceHInst(), MAKEINTRESOURCE(IDS_DEBUGCONTINUE), ulLine, pchDescription);
        }
        else
        {
            hr = Format(0, achDescription, ARRAY_SIZE(achDescription), _T("<0i>"),
                        GetResourceHInst(), MAKEINTRESOURCE(IDS_DEBUGCONTINUE));
        }

        // Display "Do you wish to debug?" dialog
        if (!hr)
        {
            CDoEnableModeless   dem(pDoc);
            iRes = ::MessageBox(
                dem._hwnd, achDescription, NULL, MB_YESNO | MB_ICONERROR);
        }
    }

    // If out of memory, iRes == 0, enter debugger
    if (iRes == IDYES || iRes == 0)
    { 
        hr = S_OK;
        *pfEnterDebugger = TRUE; 
        goto Cleanup;
    } 

    // Unwind the stack on stack overflow, if user decided to continue
    // Also unwind while calls are nested on the stack and if no excepinfo is available or if
    // the error is VBSERR_OutOfMemory. (It is possible to get out of memory before out of stack!
    if (!fExcepInfo || ExcepInfo.scode == VBSERR_OutOfStack)
    {
        if (pDoc->_cScriptNesting > 1)
            pDoc->_fStackOverflow = TRUE;
    }
    else if (ExcepInfo.scode == VBSERR_OutOfMemory)
    {
        if (pDoc->_cScriptNesting > 1)
            pDoc->_fOutOfMemory = TRUE;
    }

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::OnScriptErrorDebug
//
//  Synopsis:   per IActiveScriptSite
//
//  Arguments:  pErrorDebug:       Error info
//              pfEnterDebugger:   Whether to pass the error to the debugger
//                                 to do JIT debugging
//              pfCallOnScriptErrorWhenContinuing:
//                                 Whether to call IActiveScriptSite::OnScriptError()
//                                 when the user decides to continue by returning
//                                 the error
//                                       
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::OnScriptErrorDebug(IActiveScriptErrorDebug* pErrorDebug, BOOL *pfEnterDebugger,
                                  BOOL *pfCallOnScriptErrorWhenContinuing)
{
    TraceTag((tagScriptSite, "OnScriptErrorDebug"));

    HRESULT                 hr;
    IActiveScriptError *    pScriptError = NULL;

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);

    if (pfCallOnScriptErrorWhenContinuing)
    {
        *pfCallOnScriptErrorWhenContinuing = FALSE; 
    }

    hr = THR(pErrorDebug->QueryInterface(IID_IActiveScriptError, (void**)&pScriptError));
    if (hr)
        goto Cleanup;

    hr = THR(DoYouWishToDebug(pScriptError, pfEnterDebugger));

Cleanup:
    ReleaseInterface(pScriptError);

    RRETURN (hr);
}



//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::GetWindow
//
//  Synopsis:   per IActiveScriptWindow
//
//  Arguments:  phwnd       Resultant hwnd
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::GetWindow(HWND *phwnd)
{
    TraceTag((tagScriptSite, "GetWindow"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    if (!_pCollection)
        RRETURN(E_UNEXPECTED);

    *phwnd = _pCollection->Doc()->_state >= OS_INPLACE ?
            _pCollection->Doc()->_pInPlace->_hwnd :
            NULL;

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::EnableModeless
//
//  Synopsis:   per IActiveScriptWindow
//
//  Arguments:  fEnable         TRUE to enable, FALSE to disable
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::EnableModeless(BOOL fEnable)
{
    TraceTag((tagScriptSite, "EnableModeless"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    if (!_pCollection || (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_ALERTS)))
        RRETURN(E_UNEXPECTED);

    CDoEnableModeless   dem(_pCollection->Doc(), FALSE);

    if (fEnable)
    {
        dem.EnableModeless(TRUE);
    }
    else
    {
        dem.DisableModeless();

        // Return an explicit failure here if we couldn't do it.
        // This is needed to ensure that the count does not go
        // out of sync.
        if (!dem._fCallEnableModeless)
            return E_FAIL;
    }
    
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::QueryContinue
//
//  Synopsis:   per IActiveScriptSiteInterruptPoll
//
//---------------------------------------------------------------------------
HRESULT
CScriptHolder::QueryContinue(void)
{
    HRESULT hr;
    IActiveScriptStats *pScriptStats=NULL;
    ULONG ulLow,ulHigh = 0;

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    TraceTag((tagScriptSite, "QueryContinue"));

    if (!_pCollection||!_pScript)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // Find out how many statements have executed. This is an optional interface
    // so allow QI to fail
    hr = THR(_pScript->QueryInterface(IID_IActiveScriptStats,
        (void**)&pScriptStats ));
    if ( !hr )
    {
        hr = THR(pScriptStats->GetStat ( SCRIPTSTAT_STATEMENT_COUNT,
            &ulLow, &ulHigh ));
        if ( hr )
            goto Cleanup;

        hr = THR(pScriptStats->ResetStats());
        if ( hr )
            goto Cleanup;
    }
    else if ( hr != E_NOINTERFACE )
    {
        goto Cleanup;
    }

    hr = THR(_pCollection->Doc()->QueryContinueScript(ulHigh));

Cleanup:
    ReleaseInterface(pScriptStats);
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::QueryService
//
//  Synopsis:   per IServiceProvider
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::QueryService(REFGUID sid, REFIID iid, LPVOID *ppv)
{
    TraceTag((tagScriptSite, "QueryService"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    RRETURN(_pCollection->Doc()->QueryService(sid, iid, ppv));
}



#ifndef NO_SCRIPT_DEBUGGER

//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::GetDocumentContextFromPosition
//
//  Synopsis:   per IActiveScriptSiteDebug
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::GetDocumentContextFromPosition(
    DWORD                       dwCookie,
    ULONG                       uCharacterOffset,
    ULONG                       uNumChars,
    IDebugDocumentContext **    ppDebugDocumentContext)
{
    HRESULT                 hr;
    CScriptCookieTable *    pScriptCookieTable;

    hr = THR(_pCollection->Doc()->EnsureScriptCookieTable(&pScriptCookieTable));
    if (hr)
        goto Cleanup;

    hr = THR(pScriptCookieTable->GetScriptDebugDocumentContext(
        dwCookie, uCharacterOffset, uNumChars, ppDebugDocumentContext));

Cleanup:
    RRETURN (hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::GetRootApplicationNode
//
//  Note:       This method can be called on a non-apartment thread
//              because the debugger may need access to this method
//              when the ui thread is hung inside a breakpoint.
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::GetRootApplicationNode (IDebugApplicationNode ** ppDebugApplicationNode)
{
    TraceTag((tagScriptSite, "GetRootApplicationNode"));

    HRESULT                 hr = E_UNEXPECTED;
    CScriptDebugDocument *  pDebugDocument = NULL;
    CMarkupScriptContext *  pMarkupScriptContext = _pCollection->Doc()->PrimaryMarkup()->ScriptContext();
    IDebugDocumentHelper *  pDebugHelper = NULL;

    if (!ppDebugApplicationNode)
        RRETURN(E_POINTER);

    *ppDebugApplicationNode = NULL;

    pDebugDocument = pMarkupScriptContext ? pMarkupScriptContext->_pScriptDebugDocument : NULL;

    if (pDebugDocument)
    {
        pDebugDocument->GetDebugHelper(&pDebugHelper);
        if (!pDebugHelper)
        {
            goto Cleanup;
        }
    
        hr = THR(pDebugHelper->GetDebugApplicationNode(ppDebugApplicationNode));
        if (hr)
        {
            goto Cleanup;
        }
    }

Cleanup:
    if (pDebugDocument)
        pDebugDocument->ReleaseDebugHelper(pDebugHelper);

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::GetApplication
//
//  Synopsis:   per IActiveScriptSiteDebug
//
//  Note:       This method can be called on a non-apartment thread
//              because the debugger may need access to this method
//              when the ui thread is hung inside a breakpoint.
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::GetApplication( IDebugApplication **ppApp )
{
    TraceTag((tagScriptSite, "GetApplication"));
    Assert( ppApp );
    
    if( !ppApp )
        RRETURN(E_POINTER);
    if (!g_pDebugApp )
        RRETURN(E_UNEXPECTED);
    *ppApp = g_pDebugApp;
    (*ppApp)->AddRef( );

    RRETURN(S_OK);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::EnterBreakPoint
//
//  Synopsis:   per IActiveScriptSiteDebug
//
//  NOTE:       No longer used.  Replaced by IRemoteDebugApplicationEvents::OnEnterBreakPoint
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::EnterBreakPoint( void )
{
   TraceTag((tagScriptSite, "EnterBreakPoint"));
   Assert (!"CScriptHolder::EnterBreakPoint: obsolete member called");
   RRETURN(E_UNEXPECTED);
}


//---------------------------------------------------------------------------
//
//  Method:     CScriptHolder::LeaveBreakPoint
//
//  Synopsis:   per IActiveScriptSiteDebug
//
//  NOTE:       No longer used.  Replaced by IRemoteDebugApplicationEvents::OnLeaveBreakPoint
//
//---------------------------------------------------------------------------

HRESULT
CScriptHolder::LeaveBreakPoint( void )
{
    TraceTag((tagScriptSite, "LeaveBreakPoint"));
    Assert (!"CScriptHolder::EnterBreakPoint: obsolete member called");
    RRETURN(E_UNEXPECTED);
}

#endif // NO_SCRIPT_DEBUGGER

HRESULT 
CScriptHolder::QueryStatus (
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    TraceTag((tagScriptSite, "QueryStatus"));
    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    return S_OK;
}

HRESULT 
CScriptHolder::Exec (
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    TraceTag((tagScriptSite, "Exec"));

    if (IllegalCall())
        RRETURN(E_UNEXPECTED);
        
    HRESULT hr = S_OK;
    LPTSTR  pstrBaseURL = NULL;
    CDoc *  pDoc = _pCollection->Doc();

    if (!pguidCmdGroup || *pguidCmdGroup != CGID_ScriptSite)
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
        goto Cleanup;
    }

    switch (nCmdID)
    {
    case CMDID_SCRIPTSITE_URL:
    case CMDID_SCRIPTSITE_SID:

        if (!pvarargOut)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        
        V_VT(pvarargOut) = VT_BSTR;

        //
        // For security reasons, we use the actual URL of the calling doc
        // as opposed to the base URL. (We used the base URL in beta 2.)
        // Also this behavior is compat with Nav - dbau.
        //

        if (nCmdID == CMDID_SCRIPTSITE_URL)
        {
            hr = THR(pDoc->_cstrUrl.AllocBSTR(&V_BSTR(pvarargOut)));
            if (hr)
                goto Cleanup;
        }
        else    // nCmdID == CMDID_SCRIPTSITE_SID
        {
            BYTE    abSID[MAX_SIZE_SECURITY_ID];
            DWORD   cbSID = ARRAY_SIZE(abSID);

            memset(abSID, 0, cbSID);
            hr = THR(pDoc->GetSecurityID(abSID, &cbSID));
            if (hr)
                goto Cleanup;

            hr = FormsAllocStringLen(NULL, MAX_SIZE_SECURITY_ID, &V_BSTR(pvarargOut));
            if (hr)
                goto Cleanup;

            memcpy(V_BSTR(pvarargOut), abSID, MAX_SIZE_SECURITY_ID);
        }

        break;

    case CMDID_SCRIPTSITE_TRUSTEDDOC:
        if (!pvarargOut)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        V_VT(pvarargOut) = VT_BOOL;
        V_BOOL(pvarargOut) = pDoc->IsTrustedDoc() ? VARIANT_TRUE : VARIANT_FALSE;
        hr = S_OK;
        break;
        
    case CMDID_SCRIPTSITE_HTMLDLGTRUST:

        {
            if (!pvarargOut)
            {
                hr = E_POINTER;
                goto Cleanup;
            }
        
            V_VT(pvarargOut) = VT_EMPTY;

            if (pDoc->_fInHTMLDlg)
            {
                hr = THR(CTExec(
                    pDoc->_pClientSite,
                    &CGID_ScriptSite,
                    CMDID_SCRIPTSITE_HTMLDLGTRUST,
                    0,
                    NULL,
                    pvarargOut));

                if (hr)
                    goto Cleanup;
            }
            else
            {
                hr = OLECMDERR_E_NOTSUPPORTED;
            }

            break;
        }

    case CMDID_SCRIPTSITE_SECSTATE:
        {
            SSL_SECURITY_STATE sss;
            SSL_PROMPT_STATE   sps;
            
            if (!pvarargOut)
            {
                hr = E_POINTER;
                goto Cleanup;
            }

            V_VT(pvarargOut) = VT_I4;

            pDoc->GetRootSslState(&sss, &sps);

            V_I4(pvarargOut) = (long)sss;
            
            break;
        }

    default:
        hr = OLECMDERR_E_NOTSUPPORTED;
        break;

    }

Cleanup:
    MemFreeString(pstrBaseURL);
    RRETURN(hr);
}

HRESULT
CScriptHolder::GetDebug(IActiveScriptDebug **ppDebug)
{
    HRESULT hr = S_OK;

    if (!_pScriptDebug)
    {
        hr = THR(_pScript->QueryInterface(IID_IActiveScriptDebug, (LPVOID *)&_pScriptDebug));
    }

    if (hr == S_OK)
    {
        *ppDebug = _pScriptDebug;
        _pScriptDebug->AddRef();
    }

    RRETURN(hr);
}
