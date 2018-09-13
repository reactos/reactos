//+--------------------------------------------------------------------------
//
//  File:       stdform.cxx
//
//  Contents:   IDoc methods of CDoc
//
//  Classes:    (part of) CDoc
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifdef WIN16
#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#define _hxx_
#include "body.hdl"

#ifdef WIN16
UINT CTimeoutEventList::_uNextTimerID = 0xC000;

ExternTag(tagShDocMetaRefresh);
#endif

DeclareTag(tagTimerProblems, "Timer", "Timer problems");
#define ISVARIANTEMPTY(var) (V_VT(var) == VT_ERROR  || V_VT(var) == VT_EMPTY)

DeclareTag(tagReadystateAssert, "IgnoreRS", "ReadyState Assert");

MtDefine(CDocGetFile, Utilities, "CDoc::GetFile")
MtDefine(TIMEOUTEVENTINFO, CTimeoutEventList, "TIMEOUTEVENTINFO")
MtDefine(CDocPersistLoad_aryElements_pv, Locals, "Persistence stuff")


extern HRESULT CreateStreamOnFile(
        LPCTSTR lpstrFile,
        DWORD dwSTGM,
        LPSTREAM * ppstrm);

HRESULT GetFullyExpandedUrl(CBase *pBase, BSTR bstrUrl, BSTR *pbstrFullUrl, IServiceProvider *pSP = NULL);

//+---------------------------------------------------------------------------
//
//  Helper:     GetScriptSite
//
//----------------------------------------------------------------------------

HRESULT
GetScriptSiteCommandTarget (IServiceProvider * pSP, IOleCommandTarget ** ppCommandTarget)
{
    RRETURN(THR_NOTRACE(pSP->QueryService(SID_GetScriptSite,
                                          IID_IOleCommandTarget,
                                          (void**) ppCommandTarget)));
}

//+---------------------------------------------------------------------------
//
//  Member:     GetCallerCommandTarget
//
//  Synopsis:   walks up caller chain getting either first or last caller
//              and then gets it's command target
//
//----------------------------------------------------------------------------

HRESULT
GetCallerCommandTarget (
    CBase *              pBase,
    IServiceProvider *   pBaseSP,
    BOOL                 fFirstScriptSite,
    IOleCommandTarget ** ppCommandTarget)
{
    HRESULT                 hr = S_OK;
    IUnknown *              pUnkCaller = NULL;
    IServiceProvider    *   pCallerSP = NULL;
    IServiceProvider    *   pSP = NULL;
    IOleCommandTarget   *   pCmdTarget = NULL;
    BOOL                    fGoneUp = FALSE;


    Assert (ppCommandTarget);
    *ppCommandTarget = NULL;

    if (pBaseSP)
    {
        ReplaceInterface (&pSP, pBaseSP);
    }
    else
    {
        pBase->GetUnknownObjectAt(
            pBase->FindAAIndex (DISPID_INTERNAL_INVOKECONTEXT,CAttrValue::AA_Internal),
            &pUnkCaller);
        if (!pUnkCaller)
            goto Cleanup;

        hr = THR(pUnkCaller->QueryInterface(
                IID_IServiceProvider,
                (void**) &pSP));
        if (hr || !pSP)
            goto Cleanup;
    }

    Assert(pSP);

    // Crawl up the caller chain to find the first script engine in the Invoke chain.
    // Always hold onto the last valid command target you got
    for(;;)
    {
        hr = THR_NOTRACE(GetScriptSiteCommandTarget(pSP, &pCmdTarget));

        if ( !hr && pCmdTarget )
        {
            ReplaceInterface(ppCommandTarget, pCmdTarget ); // pCmdTarget now has 2 Addrefs
            ClearInterface (&pCmdTarget); // pCmdTarget now has 1 addref
        }
        if ( fFirstScriptSite && fGoneUp )
            break;

        // Skip up to the previous caller in the Invoke chain
        hr = THR_NOTRACE(pSP->QueryService(SID_GetCaller, IID_IServiceProvider, (void**)&pCallerSP));
        if (hr || !pCallerSP)
            break;

        fGoneUp = TRUE;

        ReplaceInterface(&pSP, pCallerSP);
        ClearInterface(&pCallerSP);
    }

Cleanup:
    ReleaseInterface(pUnkCaller);
    ReleaseInterface(pCallerSP);
    ReleaseInterface(pSP);

    hr = *ppCommandTarget ? S_OK : S_FALSE;
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Helper:     GetCallerURL
//
//  Synopsis:   Helper method,
//              gets the base url from the calling document.
//----------------------------------------------------------------------------


HRESULT
GetCallerURL(CStr &cstr, CBase *pBase, IServiceProvider * pSP)
{
    HRESULT             hr = S_OK;
    IOleCommandTarget * pCommandTarget;
    CVariant            Var;

    hr = THR(GetCallerCommandTarget(pBase, pSP, FALSE, &pCommandTarget));
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(pCommandTarget->Exec(
            &CGID_ScriptSite,
            CMDID_SCRIPTSITE_URL,
            0,
            NULL,
            &Var));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&Var) == VT_BSTR);
    hr = THR(cstr.Set(V_BSTR(&Var)));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pCommandTarget);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Helper:     GetCallerSecurityState
//
//  Synopsis:   Helper method,
//              gets the security state from the calling document.
//
//----------------------------------------------------------------------------


HRESULT
GetCallerSecurityState(SSL_SECURITY_STATE *pSecState, CBase *pBase, IServiceProvider * pSP)
{
    HRESULT             hr;
    IOleCommandTarget * pCommandTarget = NULL;
    CVariant            Var;

    *pSecState = SSL_SECURITY_UNSECURE;

    hr = THR(GetCallerCommandTarget(pBase, pSP, FALSE, &pCommandTarget));
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(pCommandTarget->Exec(
            &CGID_ScriptSite,
            CMDID_SCRIPTSITE_SECSTATE,
            0,
            NULL,
            &Var));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&Var) == VT_I4);
    *pSecState = (SSL_SECURITY_STATE)(V_I4(&Var));

Cleanup:
    ReleaseInterface(pCommandTarget);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetCallerHTMLDlgTrust
//
//----------------------------------------------------------------------------

BOOL
GetCallerHTMLDlgTrust(CBase *pBase)
{
    HRESULT             hr = S_OK;
    IOleCommandTarget * pCommandTarget = NULL;
    CVariant            var;
    BOOL                fTrusted = FALSE;

    hr = THR(GetCallerCommandTarget(pBase, NULL, TRUE, &pCommandTarget));
    if (hr)
    {
        goto Cleanup;
    }

    if (pCommandTarget)
    {
        hr = THR_NOTRACE(pCommandTarget->Exec(
                &CGID_ScriptSite,
                CMDID_SCRIPTSITE_HTMLDLGTRUST,
                0,
                NULL,
                &var));
        if (hr)
            goto Cleanup;

        Assert (VT_BOOL == V_VT(&var));
        fTrusted = V_BOOL(&var);
    }
    else
    {
        fTrusted = FALSE;
    }

Cleanup:
    ReleaseInterface(pCommandTarget);

    return fTrusted;
}

#ifdef WIN16

// The meta refresh callback is distinguished from real
// scripts by the fact that the 'language' and 'script'
// strings are identical, and begin with refresh:

HRESULT MetaRefreshCallback(CDoc * pDoc, BSTR lang, BSTR script)
{
    LPCSTR pszUrl;
    HRESULT hr = S_OK;

    if (!lang || !script
        || _tcscmp(lang, script)
        || _tcsnicmp(lang, 8, "refresh:", 8))
    {
        // they don't match, so we don't want to handle this.
        TraceTag((tagShDocMetaRefresh, "MetaRefreshCallback not useable--is real script.", pszUrl));
        return S_FALSE;
    }

    // everything should already be parsed, so we shouldn't have
    // to go through the complex parser again. Just find the first , or ;
    // (Note that this also skips over the http-equiv:).
    pszUrl = script;
    while (*pszUrl && *pszUrl != '=')
    {
        pszUrl++;
    }

    if (*pszUrl)
    {
        // found a URL
        pszUrl++;
        TraceTag((tagShDocMetaRefresh, "Want to jump to %s.", pszUrl));
        // BUGWIN16? Should this be made asynchronous?
        pDoc->FollowHyperlink(pszUrl);
    }
    else
    {
        // BUGWIN16: what flags should we use? This looks like what win32 uses 18jun97.
        DWORD dwBindf = BINDF_GETNEWESTVERSION|BINDF_PRAGMA_NO_CACHE;

        TraceTag((tagShDocMetaRefresh, "Want to refresh, time=%d", pszUrl, GetTickCount()));

        if (pDoc->_pPrimaryMarkup)
        {
            hr = GWPostMethodCall(pDoc,
                        ONCALL_METHOD(CDoc, ExecRefreshCallback, execrefreshcallback), dwBindf, FALSE, "CDoc::ExecRefreshCallback");
        }
    }


    return hr;
}
#endif // win16

//+------------------------------------------------------------------------
//
//  Function:   SetLastModDate
//
//  Synopsis:   Sets the mod date used by the OM and the History code
//
//-------------------------------------------------------------------------

FILETIME
CDoc::GetLastModDate()
{
    if (HtmCtx())
    {
        return(HtmCtx()->GetLastMod());
    }
    else
    {
        FILETIME ft = {0};
        return ft;
    }
}


//+------------------------------------------------------------------------
//
//  IDoc implementation
//
//-------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::GetcanPaste
//
//  Synopsis:   Returns whether the clipboard has readable data.
//
//--------------------------------------------------------------------------

HRESULT
CDoc::canPaste(VARIANT_BOOL * pfCanPaste)
{
    HRESULT         hr;
    IDataObject *   pDataObj = NULL;

    if (!pfCanPaste)
        RRETURN(SetErrorInfoInvalidArg());

    hr = THR(OleGetClipboard(&pDataObj));
    if (hr)
        goto Cleanup;

    hr = THR(FindLegalCF(pDataObj));

Cleanup:
    ReleaseInterface(pDataObj);
    *pfCanPaste = (hr == S_OK) ? VB_TRUE : VB_FALSE;
    return S_OK;
}




struct CTRLPOS
{
    IHTMLControlElement *  pCtrl;
    long        x;
    long        y;
};

int __cdecl
CompareCPTop(const void * pv1, const void * pv2)
{
    return (**(CTRLPOS **)pv1).y - (**(CTRLPOS **)pv2).y;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDoc::Getbody
//
//  Synopsis:   Get the body interface for this form
//
//--------------------------------------------------------------------------

HRESULT
CDoc::get_body(IHTMLElement ** ppDisp)
{
    return _pPrimaryMarkup->get_body(ppDisp);
}

HRESULT
CMarkup::get_body(IHTMLElement ** ppDisp)
{
    HRESULT hr = S_OK;;
    CElement * pElementClient;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDisp = NULL;

    pElementClient = GetElementClient();

    if (pElementClient)
    {
        Assert( pElementClient->Tag() != ETAG_ROOT );

        hr = pElementClient->QueryInterface( IID_IHTMLElement, (void **) ppDisp );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN1( SetErrorInfo( hr ), S_FALSE );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::paste
//
//  Synopsis:   Pastes controls from the clipboard into the form.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::paste( )
{
    // SitePaste closes the error info
    RRETURN(THR(_pElemCurrent->PasteClipboard()));
}


HRESULT
CDoc::cut()
{
    RRETURN_NOTRACE(Exec(
            (GUID *) &CGID_MSHTML,
            IDM_CUT,
            0,
            NULL,
            NULL));
}

HRESULT
CDoc::copy()
{
    RRETURN_NOTRACE(Exec(
            (GUID *) &CGID_MSHTML,
            IDM_COPY,
            0,
            NULL,
            NULL));
}

//---------------------------------------------------------------------------
//
//  Modes
//
//---------------------------------------------------------------------------


//----------------------------------------------------------------------------
//
//  Mode update functions.  They update the state of the mode and perform
//      any required side-effect.
//
//----------------------------------------------------------------------------


HRESULT
CDoc::UpdateDesignMode(BOOL fMode)
{
    BOOL    fOrgMode = _fDesignMode;
    HRESULT hr;

    // Cannot edit image files
    if (_fImageFile && fMode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(ExecStop());
    if (hr)
        goto Cleanup;

    hr = THR(EnsureDirtyStream());
    if (hr)
        goto Cleanup;

    // we should set this before we do anything with refreshing the page,
    // This way, the document flags will be set to disable the scripts, if 
    // we are in design mode.
    _fDesignMode = fMode;

    // BUGBUG: PSITEROOT null hack (johnbed)
    if( _pPrimaryMarkup )
    {
        hr = THR(ExecRefresh());
        if (FAILED(hr))
            goto Error;

        if (hr==S_FALSE)
        {
            // onbeforeunload was canceled, so restore
            // and get out of here.
            _fDesignMode = fOrgMode;
            hr = S_OK;
            goto Cleanup;
        }
    }

    if (_state == OS_UIACTIVE)
    {
#ifndef NO_OLEUI
        RemoveUI();
        hr = THR(InstallUI(FALSE));
        if (hr)
            goto Error;
#endif // NO_OLEUI

        // force to rebuild all collections
        //

// BUGBUG
// BUGBUG
// BUGBUG
// BUGBUG - This invalidates too much.  Need to clear the caches,
// not dirty the document.  Unless this operation does dirty the
// document implicitly.
// BUGBUG
// BUGBUG
// BUGBUG

        _pPrimaryMarkup->UpdateMarkupTreeVersion();
        
        Invalidate();
    }

Cleanup:
    RRETURN(hr);

Error:
    _fDesignMode = fOrgMode;
    THR_NOTRACE(ExecRefresh());
    goto Cleanup;
}




//+---------------------------------------------------------------
//
//  Member:     CDoc::GetdesignMode
//
//  Synopsis:   Get form's DesignMode
//
//---------------------------------------------------------------

HRESULT
CDoc::get_designMode(BSTR * pbstrMode)
{
    HRESULT hr;

    if ( pbstrMode == NULL )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    htmlDesignMode mode;
    mode = _fInheritDesignMode ? htmlDesignModeInherit :
                    (_fDesignMode ? htmlDesignModeOn : htmlDesignModeOff);
    hr = THR(STRINGFROMENUM ( htmlDesignMode, (long)mode, pbstrMode ));
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SetdesignMode
//
//  Synopsis:   Set form's DesignMode
//
//---------------------------------------------------------------

HRESULT
CDoc::put_designMode(BSTR bstrMode)
{
    HRESULT hr;
    htmlDesignMode mode;

    hr = THR(ENUMFROMSTRING ( htmlDesignMode, bstrMode, (long *)&mode ));
    if ( hr )
        goto Cleanup;

    hr = SetDesignMode (mode);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDoc::SetDesignMode ( htmlDesignMode mode )
{
    HRESULT hr = S_OK;

    if (mode == htmlDesignModeOn && _fFrameSet)
    {
        // Do nothing for frameset
        hr = MSOCMDERR_E_DISABLED;
        goto Cleanup;
    }

    if (IsInScript())
    {
        // Cannot set mode while script is executing.
        hr = MSOCMDERR_E_DISABLED;
        goto Cleanup;
    }

    if (mode != htmlDesignModeOn && mode != htmlDesignModeOff && mode != htmlDesignModeInherit)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (mode == htmlDesignModeInherit)
    {
        _fInheritDesignMode = TRUE;
        hr = THR(OnAmbientPropertyChange(DISPID_AMBIENT_USERMODE));
    }
    else
    {
        _fInheritDesignMode = FALSE;
        if (!!_fDesignMode != (mode == htmlDesignModeOn))
        {
            hr = THR(UpdateDesignMode(!_fDesignMode));
        }
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     open, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::open(BSTR mimeType, VARIANT varName, VARIANT varFeatures, VARIANT varReplace,
                    /* retval */ IDispatch **pDisp)
{
    LOADINFO        LoadInfo = { 0 };
    HRESULT         hr = S_OK;
    CStr            cstrCallerURL;
    SSL_SECURITY_STATE sssCaller;
    VARIANT       * pvarName, * pvarFeatures, * pvarReplace;
    CVariant        vRep;
    BOOL            fReplace = FALSE;
    BSTR            bstrFullUrl = NULL;

    if (pDisp)
        *pDisp = NULL;

    pvarName = (V_VT(&varName) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&varName) : &varName;
    pvarFeatures = (V_VT(&varFeatures) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&varFeatures) : &varFeatures;
    pvarReplace = (V_VT(&varReplace) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&varReplace) : &varReplace;

    // If parameter 3 is specified consider the call window.open
    if(!ISVARIANTEMPTY(pvarFeatures))
    {
        BSTR            bstrName, bstrFeatures;
        VARIANT_BOOL    vbReplace;

        // Check the parameter types
        if(V_VT(pvarName) != VT_BSTR ||
            (!ISVARIANTEMPTY(pvarFeatures) &&  V_VT(pvarFeatures) != VT_BSTR))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        bstrName = (ISVARIANTEMPTY(pvarName)) ? NULL : V_BSTR(pvarName);
        bstrFeatures = (ISVARIANTEMPTY(pvarFeatures)) ? NULL : V_BSTR(pvarFeatures);

        if(!ISVARIANTEMPTY(pvarReplace))
        {
            if(vRep.CoerceVariantArg(pvarReplace, VT_BOOL) != S_OK)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
            vbReplace = V_BOOL(&vRep);
        }
        else
        {
            vbReplace = VB_FALSE;
        }

        if((hr = THR(EnsureOmWindow())) == S_OK)
        {
            hr = THR(GetFullyExpandedUrl(this, mimeType, &bstrFullUrl));
            if (hr)
                goto Cleanup;

            // In this case mimiType contains the URL
            hr = THR(_pOmWindow->open(bstrFullUrl, bstrName, bstrFeatures,
                                            vbReplace, (IHTMLWindow2 **)pDisp));
        }

        goto Cleanup;
    }



    // If we're running script then do nothing.
    if (HtmCtx())
    {
        if (PrimaryMarkup()->IsInInline())
            goto Cleanup;

#if DBG==1
        // Any pending bindings then abort them.
        // Note that shdocvw forces READYSTATE_COMPLETE on subframes when we do this,
        // so don't assert when we notice that shdocvw's readystate is different from
        // the hosted doc (tagReadystateAssert is used in frmsite.cxx)
        BOOL    fOldReadyStateAssert = IsTagEnabled(tagReadystateAssert);

        EnableTag(tagReadystateAssert, FALSE);
#endif

        if (_pClientSite)
        {
            IGNORE_HR(CTExec(_pClientSite,
                             &CGID_ShellDocView,
                             SHDVID_DOCWRITEABORT,
                             0,
                             NULL,
                             NULL));
        }

#if DBG==1
        EnableTag(tagReadystateAssert, fOldReadyStateAssert);
#endif
    }

    // If second argument is "replace", set replace
    if (V_VT(pvarName) == VT_BSTR)
    {
        if (V_BSTR(pvarName) && !StrCmpI(V_BSTR(pvarName),_T("replace")))
        {
            fReplace = TRUE;
        }
    }

    if (mimeType)
    {
        MIMEINFO * pmi = GetMimeInfoFromMimeType(mimeType);

        // BUGBUG: (TerryLu) If we can't find the known mimetype then to match
        //  Navigator we need to be able to open a plugin from a list of
        //  plugins.  Navigator will as well allow going to netscape page
        //  on plugins.  This is a task which needs to be done.

        if (pmi && pmi->pfnImg)
        {
            // BUGBUG: (dinartem) We don't support opening image formats yet.
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        LoadInfo.pmi = pmi;
    }

    //
    // open implies close() if needed
    //

    if (HtmCtx() && HtmCtx()->IsOpened())
    {
        hr = THR(close());
        if (hr)
            goto Cleanup;
    }

    //
    // Before starting the unload sequence, fire onbeforeunload and allow abort
    //

    if (_pOmWindow && !_pOmWindow->Fire_onbeforeunload())
        goto Cleanup;

    //
    // discover the calling doc's URL and security state
    //

    hr = THR(GetCallerURL(cstrCallerURL, this, NULL));
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = THR(GetCallerSecurityState(&sssCaller, this, NULL));
    if (!SUCCEEDED(hr))
        goto Cleanup;

    Assert(cstrCallerURL.Length() && "There must always be a caller URL.");

    //
    // Right before clearing out the document, ask the shell to
    // create a history entry. (exception: replace history if
    // "replace" was specified or if opening over an about: page)
    //

    if (_pInPlace && _pInPlace->_pInPlaceSite)
    {
        CVariant var(VT_BSTR);
        hr = cstrCallerURL.AllocBSTR(&V_BSTR(&var));
        if (hr)
            goto Cleanup;

        if (fReplace || !_cstrUrl.Length() || GetUrlScheme(_cstrUrl) == URL_SCHEME_ABOUT)
        {
            IGNORE_HR(CTExec(_pInPlace->_pInPlaceSite,
                &CGID_Explorer,
                SBCMDID_REPLACELOCATION,
                0, &var, 0));
        }
        else
        {
            IGNORE_HR(CTExec(_pInPlace->_pInPlaceSite,
                &CGID_Explorer,
                SBCMDID_UPDATETRAVELLOG,
                0, &var, 0));
        }
    }

    LoadInfo.codepage = CP_UCS_2;
    LoadInfo.fKeepOpen = TRUE;
    LoadInfo.pchDisplayName = cstrCallerURL;
    LoadInfo.fUnsecureSource = (sssCaller <= SSL_SECURITY_MIXED);

    hr = THR(LoadFromInfo(&LoadInfo));
    if (hr)
        goto Cleanup;

    // Write a unicode signature in case we need to reload this document

    Assert(HtmCtx());

    HtmCtx()->WriteUnicodeSignature();

    hr = S_OK;

    if (pDisp)
    {
        hr = THR(_pPrimaryMarkup->QueryInterface(IID_IHTMLDocument2, (void**)pDisp));
    }

Cleanup:
    FormsFreeString(bstrFullUrl);
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     write
//
//  Synopsis:   Automation method,
//              inserts the sepcified HTML into the preparser
//
//----------------------------------------------------------------------------
HRESULT
CDoc::write(SAFEARRAY * psarray)
{
    HRESULT             hr = S_OK;
    CVariant            varstr;
    long                iArg, cArg;
    IUnknown           *pUnkCaller = NULL;
    IServiceProvider   *pSrvProvider = NULL;


    if (!PrimaryMarkup()->IsInInline() && (LoadStatus() == LOADSTATUS_DONE || !HtmCtx() || !HtmCtx()->IsOpened()))
    {
        hr = THR(open(NULL, varstr, varstr, varstr, NULL));
        if (hr)
            goto Cleanup;
    }

    Assert(HtmCtx());

    if (psarray == NULL ||  SafeArrayGetDim(psarray) != 1)
        goto Cleanup;

    cArg = psarray->rgsabound[0].cElements;

    // If we have a caller context (Established by IDispatchEx::InvokeEx,
    // use this to when converting the value in the safearray.
    GetUnknownObjectAt(FindAAIndex(DISPID_INTERNAL_INVOKECONTEXT,
                                   CAttrValue::AA_Internal),
                       &pUnkCaller);
    if (pUnkCaller)
    {
        IGNORE_HR(pUnkCaller->QueryInterface(IID_IServiceProvider,
                                             (void**)&pSrvProvider));

        CStr cstrCallerURL;
        SSL_SECURITY_STATE sssCaller;

        // Do mixed security check now: pick up URL
        hr = THR(GetCallerURL(cstrCallerURL, this, NULL));
        if (!SUCCEEDED(hr))
            goto Cleanup;

        hr = THR(GetCallerSecurityState(&sssCaller, this, NULL));
        if (!SUCCEEDED(hr))
            goto Cleanup;

        if (!ValidateSecureUrl(cstrCallerURL, FALSE, TRUE, sssCaller <= SSL_SECURITY_MIXED))
            goto Cleanup;
    }

    for (iArg = 0; iArg < cArg; ++iArg)
    {
        VariantInit(&varstr);

        hr = SafeArrayGetElement(psarray, &iArg, &varstr);

        if (hr == S_OK)
        {
            hr = VariantChangeTypeSpecial(&varstr, &varstr, VT_BSTR, pSrvProvider);
            if (hr == S_OK)
            {
                _iDocDotWriteVersion++;
                hr = THR(HtmCtx()->Write(varstr.bstrVal, TRUE));
            }

            VariantClear(&varstr);
        }

        if (hr)
            break;
    }

    //  bump up the count, this reduces the overall number of
    //      iterations that can happen before we prompt for denial of service
    //      (see CDoc::QueryContinueScript
    ScaleHeavyStatementCount();

Cleanup:
    ReleaseInterface(pSrvProvider);
    ReleaseInterface(pUnkCaller);

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     writeln
//
//  Synopsis:   Automation method,
//              inserts the sepcified HTML into the preparser
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::writeln(SAFEARRAY * psarray)
{
    HRESULT hr;

    hr = THR(write(psarray));

    if ((hr == S_OK) && HtmCtx())
    {
        hr = THR(HtmCtx()->Write(_T("\r\n"), TRUE));
    }

    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Member:     close, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CDoc::close(void)
{
    // Don't allow a document.close if a document.open didn't happen

    if (!HtmCtx() || !HtmCtx()->IsOpened() || !GetProgSinkC())
        goto Cleanup;

    HtmCtx()->Close();
    GetProgSinkC()->OnMethodCall((DWORD_PTR)GetProgSinkC());

    Assert(!HtmCtx()->IsOpened());

Cleanup:
    return(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Member:     clear, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CDoc::clear(void)
{
    // This routine is a no-op under Navigator and IE.  Use document.close
    // followed by document.open to clear all elements in the document.

    RRETURN(S_OK);
}


//+----------------------------------------------------------------------------
//
//  Member:     put_title, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::put_title(BSTR v)
{
    Assert(_pPrimaryMarkup);
    return _pPrimaryMarkup->put_title(v);
}

HRESULT STDMETHODCALLTYPE
CMarkup::put_title(BSTR v)
{
    HRESULT hr;

    hr = THR(EnsureTitle());
    if (hr)
        goto Cleanup;

    Assert(GetTitleElement());

    hr = THR(GetTitleElement()->SetTitle(v));

Cleanup:
    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//
//  Member:     get_title, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_title(BSTR *p)
{
    return _pPrimaryMarkup->get_title(p);
}

HRESULT STDMETHODCALLTYPE
CMarkup::get_title(BSTR *p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!GetTitleElement() || !GetTitleElement()->_cstrTitle)
    {
        *p = SysAllocString(_T(""));
        if (!*p)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = THR(GetTitleElement()->_cstrTitle.AllocBSTR( p ));
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}


//+---------------------------------------------------------------------------
//
//  member: GetBodyElement
//
//  synopsis : helper for the get_/put_ functions that need the body
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::GetBodyElement(IHTMLBodyElement ** ppBody)
{
    HRESULT hr = S_OK;
    CElement *pElement;

    if (!ppBody)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppBody = NULL;

    pElement = GetPrimaryElementClient();
    if (!pElement)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR_NOTRACE(pElement->QueryInterface(
        IID_IHTMLBodyElement, (void **) ppBody));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  member: GetBodyElement
//
//  synopsis : helper for the get_/put_ functions that need the CBodyElement
//             returns S_FALSE if body element is not found
//
//-----------------------------------------------------------------------------
HRESULT
CDoc::GetBodyElement(CBodyElement **ppBody)
{
    HRESULT hr = S_OK;

    if (ppBody == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppBody = NULL;

    if (!GetPrimaryElementClient())
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if (GetPrimaryElementClient()->Tag() == ETAG_BODY)
        *ppBody = DYNCAST( CBodyElement, GetPrimaryElementClient() );
    else
        hr = S_FALSE;

Cleanup:

    RRETURN1( hr, S_FALSE );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_bgColor, IOmDocument
//
//  Synopsis: defers to body get_bgcolor
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CDoc::get_bgColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAbgColor();
    }
    else
    {
        Val = pBody->GetFirstBranch()->GetCascadedbackgroundColor();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : _pOptionSettings->crBack()));

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_bgColor, IOmDocument
//
//  Synopsis: defers to body put_bgcolor
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::put_bgColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT            hr;

    IGNORE_HR(GetBodyElement(&pBody));

    // this only goes up
    PrimaryMarkup()->OnLoadStatus(LOADSTATUS_INTERACTIVE);

    if (!pBody)
    {
        // its NOT a body element. assume Frameset.
        hr = THR(s_propdescCDocbgColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
        if(hr)
            goto Cleanup;

        // Force a repaint and transition to a load-state where
        // we are allowed to redraw.
        GetView()->Invalidate((CRect *)NULL, TRUE);
    }
    else
    {
        // we have a body tag
        hr = THR(pBody->put_bgColor(p));
        ReleaseInterface(pBody);
        
        GetView()->EnsureView(LAYOUT_DEFEREVENTS | LAYOUT_SYNCHRONOUSPAINT);

        if (hr ==S_OK)
            Fire_PropertyChangeHelper(DISPID_CDoc_bgColor, 0);
        
    }

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}


//+----------------------------------------------------------------------------
//
//  Member:     get_fgColor, IOmDocument
//
//  Synopsis: defers to body get_text
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_fgColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAfgColor();
    }
    else
    {
        Val = pBody->GetFirstBranch()->GetCascadedcolor();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : _pOptionSettings->crText()));

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}


//+----------------------------------------------------------------------------
//
//  Member:     put_fgColor, IOmDocument
//
//  Synopsis: defers to body put_text
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::put_fgColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT        hr;

    IGNORE_HR(GetBodyElement(&pBody));
    if (!pBody)
    {
        hr = THR(s_propdescCDocfgColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }
    else
    {
        hr = THR(pBody->put_text(p));
        ReleaseInterface(pBody);
        if (hr == S_OK)
            Fire_PropertyChangeHelper(DISPID_CDoc_fgColor, 0);
    }

    RRETURN( SetErrorInfo(hr) );
}


//+----------------------------------------------------------------------------
//
//  Member:     get_linkColor, IOmDocument
//
//  Synopsis: defers to body get_link
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_linkColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAlinkColor();
    }
    else
    {
        Val = pBody->GetAAlink();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : _pOptionSettings->crAnchor()));

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}


//+----------------------------------------------------------------------------
//
//  Member:     put_linkColor, IOmDocument
//
//  Synopsis: defers to body put_link
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::put_linkColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT        hr;

    IGNORE_HR(GetBodyElement(&pBody));
    if (!pBody)
    {
        hr = THR(s_propdescCDoclinkColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }
    else
    {
        hr = THR(pBody->put_link(p));
        ReleaseInterface(pBody);
        if (hr==S_OK)
            Fire_PropertyChangeHelper(DISPID_CDoc_linkColor, 0);
    }

    RRETURN( SetErrorInfo(hr) );
}


//+----------------------------------------------------------------------------
//
//  Member:     get_alinkColor, IOmDocument
//
//  Synopsis: defers to body get_aLink
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_alinkColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAalinkColor();
    }
    else
    {
        Val = pBody->GetAAaLink();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : _pOptionSettings->crAnchor()));

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}


//+----------------------------------------------------------------------------
//
//  Member:     put_alinkColor, IOmDocument
//
//  Synopsis: defers to body put_aLink
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::put_alinkColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT        hr;

    IGNORE_HR(GetBodyElement(&pBody));
    if (!pBody)
    {
        hr = THR(s_propdescCDocalinkColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }
    else
    {
        hr = THR(pBody->put_aLink(p));
        ReleaseInterface(pBody);
        if (hr==S_OK)
            Fire_PropertyChangeHelper(DISPID_CDoc_alinkColor, 0);
    }

    RRETURN( SetErrorInfo(hr) );
}


//+----------------------------------------------------------------------------
//
//  Member:     get_vlinkColor, IOmDocument
//
//  Synopsis: defers to body get_vLink
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_vlinkColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAvlinkColor();
    }
    else
    {
        Val = pBody->GetAAvLink();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : _pOptionSettings->crAnchorVisited()));

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     put_vlinkColor, IOmDocument
//
//  Synopsis: defers to body put_vLink
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::put_vlinkColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT            hr;

    IGNORE_HR(GetBodyElement(&pBody));
    if (!pBody)
    {
        // not a body, assume frameset
        hr = THR(s_propdescCDocvlinkColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }
    else
    {
        hr = THR(pBody->put_vLink(p));
        ReleaseInterface(pBody);
        if (hr==S_OK)
            Fire_PropertyChangeHelper(DISPID_CDoc_vlinkColor, 0);
    }

    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     curWindow, IOmDocument
//
//  Synopsis: returns a pointer to the top window
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_parentWindow(IHTMLWindow2 **ppWindow)
{
    HRESULT     hr;

    if (!ppWindow)
        RRETURN (E_POINTER);

    *ppWindow = NULL;

    hr = THR(EnsureOmWindow());
    if (hr)
        goto Cleanup;

    hr = THR(_pOmWindow->QueryInterface(IID_IHTMLWindow2, (void**)ppWindow));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+----------------------------------------------------------------------------
//
//  Member:     activeElement, IOmDocument
//
//  Synopsis: returns a pointer to the active element (the element with the focus)
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_activeElement(IHTMLElement ** ppElement)
{
    HRESULT hr=S_OK;

    if (!ppElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppElement = NULL;

    if (_pElemCurrent && _pElemCurrent != PrimaryRoot())
    {
        CElement * pTarget = _pElemCurrent;

        //
        // Marka - check that currency and context are in sync. It IS valid for them to be out
        // of sync ONLY if we have a selection in an HTMLArea or similar control and have clicked
        // away on a button (ie lost focus in the control that has selection). 
        //
        // OR We're not in designmode 
        //
        // We leave this assert here to assure that currency and context are in sync during the places
        // in the Drt that get_activeElement is called (eg. during siteselect.js).
        //
        AssertSz(( !_pElemEditContext ||
                   ! _fDesignMode ||
                  _pElemEditContext == _pElemCurrent || 
                 ( _pElemEditContext->TestLock(CElement::ELEMENTLOCK_FOCUS ))||
                 ( _pElemEditContext->MarkupMaster()->TestLock(CElement::ELEMENTLOCK_FOCUS )) ||
                  (_pElemEditContext && _pElemEditContext->MarkupMaster() == _pElemCurrent)), "Currency and context do not match" );               
    

        // if an area has focus, we have to report that
        if (_pElemCurrent->Tag() == ETAG_IMG && _lSubCurrent>=0)                    
        {                                                   
            CAreaElement * pArea = NULL;                    
            CImgElement *pImg = DYNCAST(CImgElement, _pElemCurrent); 

            if (pImg->GetMap())
            {
                pImg->GetMap()->GetAreaContaining(_lSubCurrent, &pArea);
                pTarget = pArea;
            }
        }

        // all other cases fall through
        IGNORE_HR(pTarget->QueryInterface(IID_IHTMLElement,
                                          (void**)ppElement));
    }

    if (*ppElement == NULL && _fVID)
        hr = E_FAIL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Helper function:    RemoveSpecialUrl
//
//  Result: if pbstrUrlRes is not NULL, new BSTR will be allocated in *pbstrUrlRes;
//          otherwise pchUrlSrc will be NULL-terminated so to exclude special url.
//
//----------------------------------------------------------------------------

HRESULT
RemoveSpecialUrl(LPTSTR pchUrlSrc, BSTR * pbstrUrlRes)
{
    HRESULT     hr = S_OK;
    LPTSTR      pchSpecial = NULL;
        
    // search for the special character backwards from the end
    pchSpecial = _tcsrchr(pchUrlSrc, _T('\1'));
    if (pchSpecial)
    {
        if (pbstrUrlRes)
        {
            hr = THR(FormsAllocStringLen (pchUrlSrc, pchSpecial - pchUrlSrc, pbstrUrlRes));
            if (hr)
                goto Cleanup;
        }
        else
        {
            // search forward to null-terminate
            pchSpecial = _tcschr(pchUrlSrc, _T('\1'));
            if (pchSpecial)
                *pchSpecial = 0;
        }
    }
    else
    {
        if (pbstrUrlRes)
        {
            hr = THR(FormsAllocString (pchUrlSrc, pbstrUrlRes));
            if (hr)
                goto Cleanup;
        }
    }
    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     get_url, IOmDocument
//
//  Synopsis: returns the url of this document
//
//  NOTE 1: (carled)  the _cstrUrl member does not have the #hash, or ?Search infor
//  in its string. This is because LoadFromInfo gets the displayName from the Moniker
//  and removes this info.  _cstrURL is used in a hundred places throughout the codebase
//  and needs to not have this information.
//  NOTE 2: (carled) we can NOT delegate to location.href (ie4 behavior) becuase there is a 
//  bug in the timing between when shdocvw navigates the page and when this page is unloaded.
//  as a result it is possible to get the wrong url for this document.
//  NOTE 3 : (carled) so for OM compatability with NS and with IE4, I added another string
//  on the document (_cstrCOMPAT_OMUrl) its only purpose in life is to answer this question.
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_URL(BSTR * pbstrUrl)
{
    HRESULT     hr = S_OK;
    CStr        cstrRetString;
    TCHAR       achUrl[pdlUrlLen];
    DWORD       dwLength = ARRAY_SIZE(achUrl);

    if (!pbstrUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrUrl = NULL;

    hr = THR(RemoveSpecialUrl (_cstrUrl, pbstrUrl));
    if (hr)
        goto Cleanup;

    hr = THR(cstrRetString.SetBSTR(*pbstrUrl));
    if (hr)
        goto Cleanup;

    SysFreeString(*pbstrUrl);
    *pbstrUrl = NULL;

    if (_cstrCOMPAT_OMUrl.Length())
    {
        hr = THR(cstrRetString.Append(_cstrCOMPAT_OMUrl));
        if (hr)
            goto Cleanup;
    }

    if (!InternetCanonicalizeUrl(cstrRetString, 
                                 achUrl, 
                                 &dwLength, 
                                 ICU_DECODE | URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE))
        goto Cleanup;

    *pbstrUrl = SysAllocString(achUrl);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     put_URL, IOmDocument
//
//  Synopsis: set the url of this document by defering to put_ window.location.href
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::put_URL(BSTR b)
{
    IHTMLLocation * pLocation =NULL;
    HRESULT hr = S_OK;
    BSTR bstrNew = NULL;

    hr = THR(EnsureOmWindow());
    if (hr)
        goto Cleanup;

    hr =THR(_pOmWindow->get_location( &pLocation));
    if (hr)
        goto Cleanup;

    hr = THR(GetFullyExpandedUrl(this, b, &bstrNew));
    if (hr)
        goto Cleanup;

    hr =THR(pLocation->put_href(bstrNew));
    if (hr)
        goto Cleanup;

    Fire_PropertyChangeHelper(DISPID_CDoc_URL, 0);

Cleanup:
    FormsFreeString(bstrNew);
    ReleaseInterface(pLocation);
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Member:     get_location, IOmDocument
//
//  Synopsis : this defers to the window.location property
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_location(IHTMLLocation** ppLocation)
{
    HRESULT hr = S_OK;

    hr = THR(EnsureOmWindow());
    if (hr)
        goto Cleanup;

    hr =THR(_pOmWindow->get_location(ppLocation));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     get_lastModified, IOmDocument
//
//  Synopsis: returns the date of the most recent change to the document
//              this comes from the http header.
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_lastModified(BSTR * p)
{
    HRESULT  hr=S_OK;
    FILETIME ftLastMod;

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *p = NULL;

    ftLastMod = GetLastModDate();

    if (!ftLastMod.dwLowDateTime && !ftLastMod.dwHighDateTime)
    {
        // BUGBUG  - If the last modified date is requested early enough on a slow (modem) link
        // then sometimes the GetCacheInfo call fails in this case the current date and time is
        // returned.  This is not the optimal solution but it allows the user to use the page without
        // a scriping error and since we are guarenteed to be currently downloading this page for this
        // to fail the date should not appear to unusual.
        SYSTEMTIME  currentSysTime;

        GetSystemTime(&currentSysTime);
        if (!SystemTimeToFileTime(&currentSysTime, &ftLastMod))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    // We alway convert to fixed mm/dd/yyyy hh:mm:ss format TRUE means include the time
    hr = THR(ConvertDateTimeToString(ftLastMod, p, TRUE));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Member:     get_referrer, IOmDocument
//
//  Synopsis: returns the url of the document that had the link to this one
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::get_referrer(BSTR * p)
{
    HRESULT hr=S_OK;

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *p = NULL;

    if (_pDwnDoc && _pDwnDoc->GetDocReferer())
    {
        CStr    cstrRefer;

        cstrRefer.Set(_pDwnDoc->GetDocReferer());
        if (*cstrRefer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        {
            UINT    uProtRefer = GetUrlScheme(cstrRefer);
            UINT    uProtUrl   = GetUrlScheme(_cstrUrl);

            //only report the referred if: (referrer_scheme/target_Scheme)
            // http/http   http/https     https/https
            // so the if statement is :
            // (http_r && ( http_t || https_t)) || (https_r && https_t)
            if ((URL_SCHEME_HTTP == uProtRefer &&
                      (URL_SCHEME_HTTP == uProtUrl ||
                       URL_SCHEME_HTTPS == uProtUrl))
                 || (URL_SCHEME_HTTPS == uProtRefer &&
                     URL_SCHEME_HTTPS == uProtUrl))
            {
                cstrRefer.AllocBSTR(p);
                if (*p == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     get_domain, IOmDocument
//
//  Synopsis: returns the domain of the current document, initially the hostname
//      but once set, it is a sub-domain of the url hostname
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::get_domain(BSTR * p)
{
    HRESULT hr=S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!_cstrUrl.Length())
    {
        *p = 0;
        goto Cleanup;
    }

    if (_cstrSetDomain.Length())
    {
        hr = THR(_cstrSetDomain.AllocBSTR(p));
    }
    else
    {
        CStr    cstrComp;

        hr = THR_NOTRACE(GetMyDomain(&_cstrUrl, &cstrComp));
        if (hr==S_FALSE) hr = S_OK;
        if (hr)
            goto Cleanup;

        hr = THR(cstrComp.AllocBSTR(p));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     put_domain, IDocument
//
//  Synopsis: restricted to setting as a domain suffix of the hostname
//
//-----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::put_domain (BSTR p)
{
    HRESULT hr = S_OK;
    CStr    cstrComp;
    TCHAR * pTempSet;
    TCHAR * pTempUrl;
    long    lSetSize;
    long    lUrlSize;
    long    lOffset;

    if (!_cstrUrl.Length())
    {
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }

    hr = THR(GetMyDomain(&_cstrUrl, &cstrComp));
    if (hr)
        goto Cleanup;

    // set up variable for loop
    lSetSize = SysStringLen(p);
    lUrlSize = cstrComp.Length();

    if ((lUrlSize == lSetSize) && !FormsStringNICmp(cstrComp, lUrlSize, p, lSetSize))
        goto Cleanup;   // hr is S_OK

    hr       = E_INVALIDARG;

    if (lSetSize >lUrlSize)   // set is bigger than url
        goto Cleanup;

    lOffset = lUrlSize - lSetSize;
    pTempUrl = cstrComp + lOffset-1;

    //must be proper substring wrt the . in the url domain
    if (lOffset && *pTempUrl++ != _T('.'))
        goto Cleanup;

    if (!FormsStringNICmp(pTempUrl, lSetSize, p, lSetSize))
    {
        BYTE    abSID[MAX_SIZE_SECURITY_ID];
        DWORD   cbSID = ARRAY_SIZE(abSID);

        // match! now for one final check
        // there must be a '.' in the set string
        pTempSet = p+1;
        if (!_tcsstr(pTempSet, _T(".")))
            goto Cleanup;

        hr = THR(_cstrSetDomain.SetBSTR(p));
        if (hr)
            goto Cleanup;

        //
        // If we successfully set the domain, reset the sid of
        // the security proxy based on the new information.
        //


        hr = THR(EnsureOmWindow());
        if (hr)
            goto Cleanup;

        if (_pOmWindow)
        {
            COmWindow2 *    pWindow;

            hr = THR(GetSecurityID(abSID, &cbSID));
            if (hr)
                goto Cleanup;

            pWindow = _pOmWindow->Window();
            Assert(pWindow);

            hr = THR(_pOmWindow->Init(pWindow, abSID, cbSID));
            if (hr)
                goto Cleanup;
        }

        IGNORE_HR(OnPropertyChange(DISPID_CDoc_domain, 0));
    }

Cleanup:
    if (hr == S_FALSE)
        hr = CTL_E_METHODNOTAPPLICABLE;
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------
//
//  member:   GetMyDomain
//
//  Synopsis ;  The domain of the current doc :
//                 GetUrlComponenetHelper properly handles Http://
//              however, "file://..." either returns ""  for a local file
//              or returns the (?) share name for intranet files
//
//      since setting domains is irrelevant for files, this will
//      return the proper "doamin" for files, but an HR of S_FALSE.
//      the Get_ code ignores this, the put_code uses it to determine
//      wether to bother checking or not.
//
//-------------------------------------------------------------------

HRESULT
CDoc::GetMyDomain(CStr * pstrUrl, CStr * pcstrOut)
{
    HRESULT  hr = E_INVALIDARG;
    TCHAR    ach[pdlUrlLen];
    DWORD    dwSize;

    if (!pstrUrl || !pcstrOut)
        goto Cleanup;

    memset(ach, 0,sizeof(ach));

    // Clear the Output parameter
    pcstrOut->Set(_T("\0"));

    if (!pstrUrl->Length())
        goto Cleanup;

    hr = THR(CoInternetParseUrl( *pstrUrl,
                             PARSE_DOMAIN,
                             0,
                             ach,
                             ARRAY_SIZE(ach),
                             &dwSize,
                             0));

    if (hr)
        goto Cleanup;

    // set the return value
    pcstrOut->Set(ach);

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     CDoc:get_strReadyState
//
//  Synopsis:  first implementation, this is for the OM and uses the long _readyState
//      to determine the string returned.
//
//+------------------------------------------------------------------------------

HRESULT
CDoc::get_readyState(BSTR * p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

#ifdef VSTUDIO7
    // For readyState == quickdone, send the right string.
    if (PrimaryMarkup()->LoadStatus() == LOADSTATUS_QUICK_DONE)
        hr=THR(s_enumdeschtmlReadyState.StringFromEnum(htmlReadyStatequickdone, p));
    else
        hr=THR(s_enumdeschtmlReadyState.StringFromEnum(_readyState, p));
#else
    hr=THR(s_enumdeschtmlReadyState.StringFromEnum(_readyState, p));
#endif //VSTUDIO7

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     CDoc::get_Script
//
//  Synopsis:   returns OmWindow.  This routine returns the browser's
//   implementation of IOmWindow object, not our own _pOmWindow object.  This
//   is because the browser's object is the one with the longest lifetime and
//   which is handed to external entities.  See window.cxx COmWindow2::XXXX
//   for crazy delegation code.
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::get_Script(IDispatch **ppWindow)
{
    HRESULT     hr;

    if (!ppWindow)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppWindow = NULL;

#if 0
// RAID 24083: Don't give out the shdocvw window.  When we refresh the
//             document we don't tell shdocvw that we've pitched the old
//             _pOmWindow object.  It maintains a week reference on this
//             object and forwards onload events there.  Need a way to tell
//             shdocvw that it should hook up to the new window object after
//             refresh is started.

    hr = THR_NOTRACE(QueryService(
            SID_SHTMLWindow2,
            IID_IHTMLWindow2,
            (void**)ppWindow));

    // if successfull or error rather then E_NOINTERFACE,
    // which indicates that there is no shdocvw around,
    // then nothing more to do; otherwise get interface directly
    // from _pOmWindow; that interface will support only those methods
    // of IOmDocument which do not rely on shdocvw.

    if (E_NOINTERFACE != hr)
        goto Cleanup;

#endif

    hr = THR(EnsureOmWindow());
    if (hr)
        goto Cleanup;

    hr = THR(_pOmWindow->QueryInterface(IID_IDispatch, (void**)ppWindow));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDoc::createElement ( BSTR bstrTag, IHTMLElement * * ppnewElem )
{
    HRESULT hr = S_OK;
    
    hr = THR( PrimaryMarkup()->createElement( bstrTag, ppnewElem ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( SetErrorInfo( hr ) );
}



HRESULT
CDoc::releaseCapture()
{
    if (_pElementOMCapture)
    {
        CElement * pElement = _pElementOMCapture;

        _pElementOMCapture = NULL;
        _fOnLoseCapture = TRUE;
        pElement->Fire_onlosecapture();
        _fOnLoseCapture = FALSE;
        pElement->SubRelease();
        SetMouseCapture(NULL, NULL, TRUE);
    }

    RRETURN(SetErrorInfo(S_OK));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::FireTimeoutCode
//
//  Synopsis:   save the code associated with a TimerID
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CDoc::FireTimeOut( UINT uTimerID )
{
    TIMEOUTEVENTINFO * pTimeout = NULL;
    LONG                id;
    HRESULT             hr = S_OK;

    if (_fSuspendTimeout)
        goto Cleanup;

    // Check the list and if ther are timers with target time less then current
    //      execute them and remove from the list. Only events occured earlier then
    //      timer uTimerID will be retrieved
    // BUGBUG: check with Nav compat to see if Nav wraps to prevent clears during script exec.
    _cProcessingTimeout++;
    TraceTag((tagTimerProblems, "Got a timeout. Looking for a match to id %d",
                  uTimerID));


    while(_TimeoutEvents.GetFirstTimeoutEvent(uTimerID, &pTimeout) == S_OK)
    {
        // Now execute the given script
#ifdef WIN16
        // since timers have 16-bit timeouts, we may have to skip a
        // few before we execute the script & kill the timer.
        long lMSec = pTimeout->_dwTargetTime - GetTickCount();

        TraceTag((tagTimerProblems, "Got a timeout. targ=%ld, now=%ld, diff=%ld",
                  pTimeout->_dwTargetTime, GetTickCount(), lMSec));

        if (lMSec > 0L)
        {
            if (lMSec < 0xFFFFL)
            {
                //ideally we'd just call
                //ResetTimer (this, uTimerID, pTimeout->_dwTargetTime - GetTickCount());

                hr = FormsSetTimer(this,
                                   ONTICK_METHOD(CDoc, FireTimeOut, firetimeout),
                                   uTimerID,
                                   lMSec);

                TraceTag((tagTimerProblems, "Will not fire, attempted to reset timer to %ld, hr=%hr",
                          lMSec, hr));

                Assert(S_OK == hr);
                hr = S_OK;
            }
            else
            {
                TraceTag((tagTimerProblems, "Will not fire, did not attempt to reset timer."));
                hr = S_OK;
            }

            goto Cleanup;
        }
        else
        {
            TIMEOUTEVENTINFO * pTimeout2;
            TraceTag((tagTimerProblems, "Will fire."));
            _TimeoutEvents.GetTimeout(pTimeout->_uTimerID, &pTimeout2);
        }
#endif
        hr = ExecuteTimeoutScript(pTimeout);
        if ( 0 == pTimeout->_dwInterval || hr )
            // setTimeout (or something wrong with script): delete the timer
            delete pTimeout;
        else
        {
            // setInterval: put timeout back in queue with next time to fire
            _TimeoutEvents.AddPendingTimeout( pTimeout );
        }
    }
    _cProcessingTimeout--;

    // deal with any clearTimeouts (clearIntervals) that may have occurred as
    // a result of processing the scripts.
    while ( _TimeoutEvents.GetPendingClear(&id) )
    {
        if ( !_TimeoutEvents.ClearPendingTimeout((UINT)id) )
            ClearTimeout( id );
    }

    // we cleanup here because clearTimeout might have been called from setTimeout code
    // before an error occurred which we want to get rid of above (nav compat)
    if (hr)
        goto Cleanup;

    // Requeue pending timeouts (from setInterval)
    while ( _TimeoutEvents.GetPendingTimeout(&pTimeout) )
    {
        pTimeout->_dwTargetTime = (DWORD)GetTargetTime(pTimeout->_dwInterval);
        hr = THR(_TimeoutEvents.InsertIntoTimeoutList(pTimeout, NULL, FALSE));
        if (hr)
        {
            ClearTimeout( pTimeout->_uTimerID );
            goto Cleanup;
        }

        hr = THR(FormsSetTimer( this, ONTICK_METHOD(CDoc, FireTimeOut, firetimeout),
                                pTimeout->_uTimerID, pTimeout->_dwInterval ));
        if (hr)
        {
            ClearTimeout( pTimeout->_uTimerID );
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


// This function executes given timeout script and kills the associated timer

HRESULT
CDoc::ExecuteTimeoutScript(TIMEOUTEVENTINFO * pTimeout)
{
    HRESULT     hr = S_OK;

    Assert(pTimeout != NULL);

    Verify(FormsKillTimer(this, pTimeout->_uTimerID) == S_OK);

    if (pTimeout->_pCode)
    {
        DISPPARAMS  dp = g_Zero.dispparams;
        CVariant    varResult;
        EXCEPINFO   excepinfo;

        // Can't disconnect script engine while we're executing script.
        CDoc::CLock     Lock(this);

        hr = THR(pTimeout->_pCode->Invoke(DISPID_VALUE,
                                          IID_NULL,
                                          0,
                                          DISPATCH_METHOD,
                                          &dp,
                                          &varResult,
                                          &excepinfo,
                                          NULL));

    }
    else if (pTimeout->_code.Length() != 0)
    {
        CExcepInfo       ExcepInfo;
        CVariant         Var;

#ifdef WIN16
        hr = MetaRefreshCallback(this, pTimeout->_lang, pTimeout->_code);

        // if this returns S_FALSE or an error, then try the old code.
        if (hr && _pScriptCollection)
#else
        if (_pScriptCollection)
#endif
        if (_pScriptCollection)
        {
            hr = THR(_pScriptCollection->ParseScriptText(
                pTimeout->_lang,            // pchLanguage
                NULL,                       // pMarkup
                NULL,                       // pchType
                pTimeout->_code,            // pchCode
                NULL,                       // pchItemName
                _T("\""),                   // pchDelimiter
                0,                          // ulOffset
                0,                          // ulStartingLine
                NULL,                       // pSourceObject
                SCRIPTTEXT_ISVISIBLE,       // dwFlags
                &Var,                       // pvarResult
                &ExcepInfo));               // pExcepInfo
        }

    }

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDoc::AddTimeoutCode
//
//  Synopsis:   save the code associated with a TimerID
//
//----------------------------------------------------------------------------

HRESULT
CDoc::AddTimeoutCode(VARIANT *theCode, BSTR strLanguage, LONG lDelay, LONG lInterval,
                     UINT * uTimerID)
{
    HRESULT             hr;
    TIMEOUTEVENTINFO  * pTimeout;

    pTimeout = new(Mt(TIMEOUTEVENTINFO)) TIMEOUTEVENTINFO;
    if(pTimeout == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    if (V_VT(theCode) == VT_BSTR)
    {
        // Call the code associated with the timer.
        hr = THR_NOTRACE(_pScriptCollection->ConstructCode(
                            NULL,               // pchScope
                            V_BSTR(theCode),    // pchCode
                            NULL,               // pchFormalParams
                            strLanguage,        // pchLanguage
                            NULL,               // pMarkup
                            NULL,               // pchType
                            0,                  // ulOffset
                            0,                  // ulStartingLine
                            NULL,               // pSourceObject
                            0,                  // dwFlags
                            &(pTimeout->_pCode),// ppDispCode result
                            TRUE));             // fSingleLine

        // Script engine can't produce PCODE so we'll do it the old way compiling on
        // each timer event.
        if (hr)
        {
            Assert(pTimeout->_pCode == NULL);

            // Set various data
            hr = THR(pTimeout->_code.SetBSTR(V_BSTR(theCode)));
            if (hr)
                goto Error;

            hr = THR(pTimeout->_lang.SetBSTR(strLanguage));
            if (hr)
                goto Error;
        }
    }
    else if (V_VT(theCode) == VT_DISPATCH)
    {
        pTimeout->_pCode = V_DISPATCH(theCode);
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Save the time when the timeout happens
    pTimeout->_dwTargetTime = (DWORD)GetTargetTime(lDelay);

    // If lInterval=0, then called by setTimeout, otherwise called by setInterval
    pTimeout->_dwInterval = (DWORD)lInterval;

    // add the new element to the right position of the list
    // fills the timer id filed into the struct and returns
    // the value
    hr = THR(_TimeoutEvents.InsertIntoTimeoutList(pTimeout, uTimerID));
    if (hr)
        goto Error;

Cleanup:
    RRETURN(hr);

Error:
    delete pTimeout;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ClearTimeout
//
//  Synopsis:   Clears a previously setTimeout and setInterval
//
//----------------------------------------------------------------------------

HRESULT
CDoc::ClearTimeout(LONG lTimerID)
{
    HRESULT hr = S_OK;
    TIMEOUTEVENTINFO * pCurTimeout;

    if ( _cProcessingTimeout )
    {
        _TimeoutEvents.AddPendingClear( lTimerID );
    }
    else
    {
        // Get the timeout struct with given ID and remove it from the list
        hr = _TimeoutEvents.GetTimeout((DWORD)lTimerID, &pCurTimeout);
        if(hr == S_FALSE)
        {
            // Netscape just ignores the invalid arg silently - so do we.
            hr = S_OK;
            goto Cleanup;
        }

        if(pCurTimeout != NULL)
        {
            Verify(FormsKillTimer(this, pCurTimeout->_uTimerID) == S_OK);
            delete pCurTimeout;
        }
    }
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetTimeout
//
//  Synopsis:   Runs <Code> after <msec> milliseconds and returns a
//              timeout ID to be used by clearTimeout or clearInterval.
//              Also used for SetInterval.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SetTimeout(
    VARIANT *pCode,
    LONG lMSec,
    BOOL fInterval,     // periodic, repeating
    VARIANT *pvarLang,
    LONG * plTimerID)
{
    HRESULT   hr = E_INVALIDARG;
    UINT      uTimerID;
    CVariant  varLanguage;

    if (!plTimerID )
        goto Cleanup;

    *plTimerID = -1;

    hr = THR(varLanguage.CoerceVariantArg(pvarLang, VT_BSTR));
    if (hr == S_FALSE)
    {
        // language not supplied
        V_BSTR(&varLanguage) = NULL;
        hr = S_OK;
    }
    if (!OK(hr))
        goto Cleanup;

    // Perform indirection if it is appropriate:
    if( V_VT(pCode) == (VT_BYREF | VT_VARIANT))
        pCode = V_VARIANTREF(pCode);

     // Do we have code.
    if ((V_VT(pCode) == VT_DISPATCH && V_DISPATCH(pCode)) || (V_VT(pCode) == VT_BSTR))
    {
        // Accept empty strings, just don't do anything with an empty string.
        if ((V_VT(pCode) == VT_BSTR) && SysStringLen(V_BSTR(pCode)) == 0)
            goto Cleanup;

        // Register the Code.  If no language send NULL.
        hr = THR(AddTimeoutCode(pCode,
                                V_BSTR(&varLanguage),
                                lMSec,
                                (fInterval? lMSec : 0),    // Nav 4 treats setInterval w/ 0 as a setTimeout
                                &uTimerID));
        if (hr)
            goto Cleanup;

        // Register the Timeout,

#ifdef WIN16
        // Win16 timer timeouts are 16bit.

        if (lMSec > 0xfff5L)
        {
            lMSec = 0xfff5L;
        }

#endif

        hr = THR(FormsSetTimer(
                this,
                ONTICK_METHOD(CDoc, FireTimeOut, firetimeout),
                uTimerID,
                lMSec));
        if (hr)
            goto Error;

        // Return value
        *plTimerID = (LONG)uTimerID;
    }
    else
        hr = E_INVALIDARG;

Cleanup:
    RRETURN(hr);

Error:
    // clear out registered code
    ClearTimeout((LONG)uTimerID);
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CleanupScriptTimers
//
//----------------------------------------------------------------------------

void
CDoc::CleanupScriptTimers ( void )
{
    _TimeoutEvents.KillAllTimers(this);
}


HRESULT
CDoc::get_all( IHTMLElementCollection** ppDisp )
{
    return _pPrimaryMarkup->get_all(ppDisp);
}

HRESULT
CDoc::get_anchors( IHTMLElementCollection** ppDisp )
{
    return _pPrimaryMarkup->get_anchors(ppDisp);
}

HRESULT
CDoc::get_links( IHTMLElementCollection ** ppDisp )
{
    return _pPrimaryMarkup->get_links(ppDisp);
}

HRESULT
CDoc::get_forms( IHTMLElementCollection** ppDisp )
{
    return _pPrimaryMarkup->get_forms(ppDisp);
}

HRESULT
CDoc::get_applets( IHTMLElementCollection** ppDisp )
{
    return _pPrimaryMarkup->get_applets(ppDisp);
}

HRESULT
CDoc::get_images( IHTMLElementCollection** ppDisp )
{
    return _pPrimaryMarkup->get_images(ppDisp);
}

HRESULT
CDoc::get_scripts( IHTMLElementCollection** ppDisp )
{
    return _pPrimaryMarkup->get_scripts(ppDisp);
}

HRESULT
CDoc::get_frames( IHTMLFramesCollection2 ** ppDisp )
{
    HRESULT hr;

    hr = THR(EnsureOmWindow());
    if (hr)
        goto Cleanup;

    hr = THR(_pOmWindow->get_frames(ppDisp));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

HRESULT
CDoc::get_embeds( IHTMLElementCollection** ppDisp )
{
    return _pPrimaryMarkup->get_embeds(ppDisp);
}

HRESULT
CDoc::get_plugins( IHTMLElementCollection** ppDisp )
{
    // plugins is an alias for embeds
    RRETURN( get_embeds ( ppDisp ) );
}

HRESULT
CDoc::get_styleSheets( IHTMLStyleSheetsCollection** ppDisp )
{
    return _pPrimaryMarkup->get_styleSheets(ppDisp);
}

HRESULT
CMarkup::get_all( IHTMLElementCollection** ppDisp )
{
    RRETURN( SetErrorInfo( GetCollection(ELEMENT_COLLECTION, ppDisp) ));
}

HRESULT
CMarkup::get_anchors( IHTMLElementCollection** ppDisp )
{
    RRETURN( SetErrorInfo( GetCollection(ANCHORS_COLLECTION, ppDisp) ));
}

HRESULT
CMarkup::get_links( IHTMLElementCollection ** ppDisp )
{
    RRETURN( SetErrorInfo( GetCollection(LINKS_COLLECTION, ppDisp) ));
}

HRESULT
CMarkup::get_forms( IHTMLElementCollection** ppDisp )
{
    RRETURN( SetErrorInfo( GetCollection(FORMS_COLLECTION, ppDisp) ));
}

HRESULT
CMarkup::get_applets( IHTMLElementCollection** ppDisp )
{
    RRETURN( SetErrorInfo( GetCollection(APPLETS_COLLECTION, ppDisp) ));
}

HRESULT
CMarkup::get_images( IHTMLElementCollection** ppDisp )
{
    RRETURN( SetErrorInfo( GetCollection(IMAGES_COLLECTION, ppDisp) ));
}

HRESULT
CMarkup::get_scripts( IHTMLElementCollection** ppDisp )
{
    RRETURN( SetErrorInfo( GetCollection(SCRIPTS_COLLECTION, ppDisp) ));
}

HRESULT
CMarkup::get_embeds( IHTMLElementCollection** ppDisp )
{
    RRETURN( SetErrorInfo( GetCollection(EMBEDS_COLLECTION, ppDisp) ));
}

HRESULT
CMarkup::get_plugins( IHTMLElementCollection** ppDisp )
{
    // plugins is an alias for embeds
    RRETURN( get_embeds ( ppDisp ) );
}

//+------------------------------------------------------------------------
//
//  Member:     GetSelection
//
//  Synopsis:   for the Automation Object Model, this returns a pointer to
//                  the ISelectionObj interface. which fronts for the
//                  selection record exposed
//
//-------------------------------------------------------------------------

HRESULT
CDoc::get_selection( IHTMLSelectionObject ** ppDisp )
{
    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!_pCSelectionObject)
    {
        _pCSelectionObject = new CSelectionObject( this);
        if (!_pCSelectionObject)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCSelectionObject->QueryInterface(IID_IHTMLSelectionObject,
                                                (void**) ppDisp ));

        _pCSelectionObject->Release();

        if (hr )
            goto Cleanup;
    }
    else
    {
        hr = THR(_pCSelectionObject->QueryInterface(IID_IHTMLSelectionObject,
                                                (void**) ppDisp ));
        if (hr )
            goto Cleanup;
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//+----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------

static void
ChopToPath(TCHAR *szURL)
{
    TCHAR *szPathEnd;

    // Start scanning at terminating null the end of the string
    // Go back until we hit the last '/'  or the beginning of the string
    for ( szPathEnd = szURL + _tcslen(szURL);
        szPathEnd>szURL && *szPathEnd != _T('/');
        szPathEnd-- );

    // If we found the slash (and we're not looking at '//')
    // then terminate the string at the character following the slash
    // (If we didn't find the slash, then something is weird and we don't do anything)
    if (*szPathEnd == _T('/') && szPathEnd>szURL && szPathEnd[-1] != _T('/'))
    {
            // we are at the slash so set the character after the slash to NULL
        szPathEnd[1] = _T('\0');
    }
}

//+----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------

// Maximum length of a cookie string ( according to Netscape docs )
#define MAX_COOKIE_LENGTH 4096

STDMETHODIMP
CDoc::get_cookie(BSTR* retval)
{
    HRESULT hr = S_OK;

    if (!retval)
    {
        hr = E_POINTER;
    }
    else
    {
        TCHAR    achCookies[MAX_COOKIE_LENGTH + 1];
        DWORD   dwCookieSize = ARRAY_SIZE(achCookies);

        memset(achCookies, 0,sizeof(achCookies));

        *retval = NULL;

        if (InternetGetCookie(_cstrUrl, NULL, achCookies, &dwCookieSize))
        {
            if (dwCookieSize == 0) // We have no cookies
                achCookies[0] = _T('\0'); // So make the cookie string the empty str

            hr = FormsAllocString ( achCookies, retval );
        }
        // else return S_OK and an empty string 
    }

    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------

STDMETHODIMP CDoc::put_cookie(BSTR cookie)
{
    if (cookie)
    {
        InternetSetCookie(_cstrUrl, NULL, cookie);
        IGNORE_HR(OnPropertyChange(DISPID_CDoc_cookie, 0));
    }
    return S_OK;
}


//+----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------

STDMETHODIMP CDoc::get_expando(VARIANT_BOOL *pfExpando)
{
    HRESULT hr = S_OK;

    if (pfExpando)
    {
        *pfExpando = _fExpando ? VB_TRUE : VB_FALSE;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    RRETURN(SetErrorInfo(hr));
}


STDMETHODIMP CDoc::put_expando(VARIANT_BOOL fExpando)
{
    if (((_fExpando) ? VB_TRUE : VB_FALSE) != fExpando)
    {
        _fExpando = fExpando;
        Fire_PropertyChangeHelper(DISPID_CDoc_expando,0);
    }

    RRETURN(SetErrorInfo(S_OK));
}

//+-------------------------------------------------------------------------
//
// Members:     Get/SetCharset
//
// Synopsis:    Functions to get at the document's charset from the object
//              model.
//
//--------------------------------------------------------------------------

STDMETHODIMP CDoc::get_charset(BSTR* retval)
{
    TCHAR   achCharset[MAX_MIMECSET_NAME];
    HRESULT hr;

    hr = THR(GetMlangStringFromCodePage(GetCodePage(), achCharset,
                                        ARRAY_SIZE(achCharset)));
    if (hr)
        goto Cleanup;

    hr = FormsAllocString(achCharset, retval);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//--------------------------------------------------------------------------

STDMETHODIMP
CDoc::put_charset(BSTR mlangIdStr)
{
    HRESULT hr;
    CODEPAGE cp;

    hr = THR(GetCodePageFromMlangString(mlangIdStr, &cp));
    if (hr)
        goto Cleanup;

    Assert(cp != CP_UNDEFINED);

    hr = THR(SwitchCodePage(cp));
    if (hr)
        goto Cleanup;

    // Outlook98 calls put_charset and expects it not
    // to make us dirty.  They expect a switch from design
    // to browse mode to always reload from the original
    // source in this situation.  For them we make this routine
    // act just like IE4
    if( ! _fOutlook98 )
    {
        //
        // Make sure we have a META tag that's in sync with the document codepage.
        //
        hr = THR(UpdateCodePageMetaTag(cp));
        if (hr)
            goto Cleanup;
    
        IGNORE_HR(OnPropertyChange( DISPID_CDoc_charset, 0));
    }

    //
    // Clear our caches and force a replaint since codepages can have
    // distinct fonts.
    //
    EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
// Members:     Get/SetDefaultCharset
//
// Synopsis:    Functions to get at the thread's default charset from the
//              object model.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CDoc::get_defaultCharset(BSTR* retval)
{
    TCHAR   achCharset[MAX_MIMECSET_NAME];
    HRESULT hr;

    hr = THR(GetMlangStringFromCodePage(_pOptionSettings->codepageDefault,
                                        achCharset, ARRAY_SIZE(achCharset)));
    if (hr)
        goto Cleanup;

    hr = FormsAllocString(achCharset, retval);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//--------------------------------------------------------------------------

STDMETHODIMP
CDoc::put_defaultCharset(BSTR mlangIdStr)
{
#if NOT_YET

    // N.B. (johnv) This method will be exposed through the object model
    // but not through IDispatch in Beta2.  Commenting out until we can
    // do this, since this function may pose a security risk (a script
    // can make it impossible to view any subsequently browsed pages).

    HRESULT hr;
    CODEPAGE cp;

    hr = THR(GetCodePageFromMlangString(mlangIdStr, &cp));
    if (hr)
        goto Cleanup;

    if (cp != CP_UNDEFINED)
    {
        _pOptionSettings->codepageDefault = cp;
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));

#endif

    return S_OK;    // so enumerating through properties won't fail
}

//+---------------------------------------------------------------------------
//
//  Members:    Get/SetDir
//
//  Synopsis:   Functions to get at the document's direction from the object
//              model.
//
//----------------------------------------------------------------------------
HRESULT
CDoc::get_dir(BSTR * p)
{
    CHtmlElement * pHtmlElement;
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pHtmlElement = _pPrimaryMarkup->GetHtmlElement();
    if (pHtmlElement != NULL)
    {
        hr = THR(pHtmlElement->get_dir(p));
    }
    else
    {
        hr = THR(s_propdescCDocdir.b.GetEnumStringProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDoc::put_dir(BSTR v)
{
    CHtmlElement * pHtmlElement;
    HRESULT hr = S_OK;

    pHtmlElement = _pPrimaryMarkup->GetHtmlElement();

    if (pHtmlElement != NULL)
    {
        hr = THR(pHtmlElement->put_dir(v));
        if (hr == S_OK)
            Fire_PropertyChangeHelper(DISPID_CDoc_dir, 0);
    }
    else
    {
        hr = THR(s_propdescCDocdir.b.SetEnumStringProperty(v, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }

    // send the property change message to the body. These depend upon
    // being in edit mode or not.
    CBodyElement* pBody;

    hr = GetBodyElement(&pBody);
    if(!hr)
        pBody->OnPropertyChange(DISPID_A_DIR, ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS);

    RRETURN( SetErrorInfo(hr) );
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::GetDocDirection(pfRTL)
//
//  Synopsis:   Gets the document reading order. This is just a
//              reflection of the direction of the HTML element.
//
//  Returns:    S_OK if the direction was successfully set/retrieved.
//
//--------------------------------------------------------------------
HRESULT
CDoc::GetDocDirection(BOOL * pfRTL)
{
    long eHTMLDir = htmlDirNotSet;
    BSTR v = NULL;
    HRESULT hr;

    if (!pfRTL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pfRTL = FALSE;

    hr = THR(get_dir(&v));
    if (hr)
        goto Cleanup;

    hr = THR(s_enumdeschtmlDir.EnumFromString(v, &eHTMLDir));
    if (hr)
        goto Cleanup;

    *pfRTL = (eHTMLDir == htmlDirRightToLeft);

Cleanup:
    FormsFreeString(v);
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::SetDocDirection(eHTMLDir)
//
//  Synopsis:   Sets the document reading order. This is just a
//              reflection of the direction of the HTML element.
//
//  Returns:    S_OK if the direction was successfully set/retrieved.
//
//--------------------------------------------------------------------
HRESULT
CDoc::SetDocDirection(LONG eHTMLDir)
{
    BSTR bstrDir = NULL;
    HRESULT hr;

    hr = THR(s_enumdeschtmlDir.StringFromEnum(eHTMLDir, &bstrDir));
    if (hr)
        goto Cleanup;
    hr = THR(put_dir(bstrDir));

    _fRTLDocDirection = (eHTMLDir == htmlDirRightToLeft);

Cleanup:
    FormsFreeString(bstrDir);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Member: GetUrlCachedFileName gets the filename of the cached file
//         in Wininet's cache
//
//----------------------------------------------------------------------------
HRESULT
CDoc::GetFile(TCHAR **ppchFile)
{
    HRESULT hr = S_OK;

    Assert(!!_cstrUrl);
    Assert(ppchFile);

    *ppchFile = NULL;

    if (!_cstrUrl)
        goto Cleanup;

    if (GetUrlScheme(_cstrUrl) == URL_SCHEME_FILE)
    {
        TCHAR achPath[MAX_PATH];
        DWORD cchPath;

        hr = THR(CoInternetParseUrl(_cstrUrl, PARSE_PATH_FROM_URL, 0, achPath, ARRAY_SIZE(achPath), &cchPath, 0));
        if (hr)
            goto Cleanup;
        hr = THR(MemAllocString(Mt(CDocGetFile), achPath, ppchFile));
    }
    else
    {
        BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
        INTERNET_CACHE_ENTRY_INFO * pInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
        DWORD                       cInfo = sizeof(buf);

        if (RetrieveUrlCacheEntryFile(_cstrUrl, pInfo, &cInfo, 0))
        {
            DoUnlockUrlCacheEntryFile(_cstrUrl, 0);
            hr = THR(MemAllocString(Mt(CDocGetFile), pInfo->lpszLocalFileName, ppchFile));
        }
        else
            hr = E_FAIL;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Helper Function: GetFileTypeInfo, called by get_mimeType
//
//----------------------------------------------------------------------------

BSTR
GetFileTypeInfo(TCHAR * pchFileName)
{
#if !defined(WIN16) && !defined(WINCE)
    SHFILEINFO sfi;

    if (pchFileName &&
            pchFileName[0] &&
            SHGetFileInfo(
                pchFileName,
                FILE_ATTRIBUTE_NORMAL,
                &sfi,
                sizeof(sfi),
                SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
    {
        return SysAllocString(sfi.szTypeName);
    }
    else
#endif //!WIN16 && !WINCE
    {
        return NULL;
    }
}

//+---------------------------------------------------------------------------
//
// Member: get_mimeType
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::get_mimeType(BSTR * pMimeType)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    *pMimeType = NULL;

    hr = GetFile(&pchCachedFile);
    if (!hr)
    {
        *pMimeType = GetFileTypeInfo(pchCachedFile);
    }

    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_fileSize
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::get_fileSize(BSTR * pFileSize)
{
    HRESULT hr = S_OK;
    TCHAR   szBuf[64];
    TCHAR * pchCachedFile = NULL;

    if (pFileSize == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileSize = NULL;

    hr = GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);

            Format(0, szBuf, ARRAY_SIZE(szBuf), _T("<0d>"), (long)wfd.nFileSizeLow);
            *pFileSize = SysAllocString(szBuf);
        }
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
// Member: get_fileCreatedDate
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::get_fileCreatedDate(BSTR * pFileCreatedDate)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    if (pFileCreatedDate == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileCreatedDate = NULL;

    hr = GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
            // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(wfd.ftCreationTime, pFileCreatedDate, FALSE));
        }
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_fileModifiedDate
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::get_fileModifiedDate(BSTR * pFileModifiedDate)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    if (pFileModifiedDate == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileModifiedDate = NULL;

    hr = GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
            // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(wfd.ftLastWriteTime, pFileModifiedDate, FALSE));
        }
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}


// BUGBUG (lmollico): get_fileUpdatedDate won't work if src=file://htm
//+---------------------------------------------------------------------------
//
// Member: get_fileUpdatedDate
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::get_fileUpdatedDate(BSTR * pFileUpdatedDate)
{
    HRESULT   hr = S_OK;

    * pFileUpdatedDate = NULL;

    if (_cstrUrl)
    {
        BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
        INTERNET_CACHE_ENTRY_INFO * pInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
        DWORD                       cInfo = sizeof(buf);

        if (RetrieveUrlCacheEntryFile(_cstrUrl, pInfo, &cInfo, 0))
        {
            // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(pInfo->LastModifiedTime, pFileUpdatedDate, FALSE));
            DoUnlockUrlCacheEntryFile(_cstrUrl, 0);
        }
    }

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_security
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::get_security(BSTR * pSecurity)
{
    HRESULT hr = S_OK;
    TCHAR   szBuf[2048];
    BOOL    fSuccess = FALSE;

    if (_cstrUrl && (GetUrlScheme(_cstrUrl) == URL_SCHEME_HTTPS))
    {
        fSuccess = InternetGetCertByURL(_cstrUrl, szBuf, ARRAY_SIZE(szBuf));
    }

    if (!fSuccess)
    {
        LoadString(
                GetResourceHInst(),
                IDS_DEFAULT_DOC_SECURITY_PROP,
                szBuf,
                ARRAY_SIZE(szBuf));
    }

    * pSecurity = SysAllocString(szBuf);

    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
// Protocol Identifier and Protocol Friendly Name
// Adapted from wc_html.h and wc_html.c of classic MSHTML
//
//-----------------------------------------------------------------------------
typedef struct {
        TCHAR * szName;
        TCHAR * szRegKey;
} ProtocolRec;

static ProtocolRec ProtocolTable[] = {
        _T("file:"),     _T("file"),
        _T("mailto:"),   _T("mailto"),
        _T("gopher://"), _T("gopher"),
        _T("ftp://"),    _T("ftp"),
        _T("http://"),   _T("http"),
        _T("https://"),  _T("https"),
        _T("news:"),     _T("news"),
        NULL, NULL
};

TCHAR * ProtocolFriendlyName(TCHAR * szURL)
{
    TCHAR szBuf[MAX_PATH];
    int   i;

    if (!szURL)
        return NULL;

    LoadString(GetResourceHInst(), IDS_UNKNOWNPROTOCOL, szBuf,
        ARRAY_SIZE(szBuf));
    for (i = 0; ProtocolTable[i].szName; i ++)
    {
        if (_tcsnipre(ProtocolTable[i].szName, -1, szURL, -1))
            break;
    }
    if (ProtocolTable[i].szName)
    {
        DWORD dwLen = sizeof(szBuf);
        //DWORD dwValueType;
        HKEY  hkeyProtocol;

        LONG lResult = RegOpenKeyEx(
                HKEY_CLASSES_ROOT,
                ProtocolTable[i].szRegKey,
                0,
                KEY_QUERY_VALUE,
                &hkeyProtocol);
        if (lResult != ERROR_SUCCESS)
            goto Cleanup;

        lResult = RegQueryValue(
                hkeyProtocol,
                NULL,
                szBuf,
                (long *) &dwLen);
        RegCloseKey(hkeyProtocol);
    }

Cleanup:
    return SysAllocString(szBuf);
}

//+---------------------------------------------------------------------------
//
// Member: get_protocol
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDoc::get_protocol(BSTR * pProtocol)
{
    HRESULT   hr = S_OK;
    TCHAR   * pResult = NULL;

    pResult = ProtocolFriendlyName(_cstrUrl);

    if (pResult)
    {
        int z = (_tcsncmp(pResult, 4, _T("URL:"), -1) == 0) ? (4) : (0);
        * pProtocol = SysAllocString(pResult + z);
        SysFreeString(pResult);
    }
    else
    {
        *pProtocol = NULL;
    }
    RRETURN(SetErrorInfo(hr));

}

//+----------------------------------------------------------------------------
//
// Member: get_nameProp
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDoc::get_nameProp(BSTR * pName)
{
    RRETURN(SetErrorInfo(get_title(pName)));
}


//+----------------------------------------------------------------------------
//
// Member:      CDoc::GetInterfaceSafetyOptions
//
// Synopsis:    per IObjectSafety
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::GetInterfaceSafetyOptions(
    REFIID riid,
    DWORD *pdwSupportedOptions,
    DWORD *pdwEnabledOptions)
{
    *pdwSupportedOptions = *pdwEnabledOptions =
        INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
    return S_OK;
}


//+----------------------------------------------------------------------------
//
// Member:      CDoc::SetInterfaceSafetyOptions
//
// Synopsis:    per IObjectSafety
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::SetInterfaceSafetyOptions(
    REFIID riid,
    DWORD dwOptionSetMask,
    DWORD dwEnabledOptions)
{
    // This needs to hook into the IObjectSafety calls we make on objects.
    // (anandra)
    return S_OK;
}


//+----------------------------------------------------------------------------
//
// Member:      CDoc::SetUIHandler
//
// Synopsis:    per ICustomDoc
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::SetUIHandler(IDocHostUIHandler *pUIHandler)
{
    IOleCommandTarget * pHostUICommandHandler = NULL;

    ReplaceInterface(&_pHostUIHandler, pUIHandler);
    if (pUIHandler && _pHostUIHandler == pUIHandler)
        _fUIHandlerSet = TRUE;

    if (_pHostUIHandler)
    {
        // We don't care if this succeeds (for now), we init pHostUICommandHandler to NULL.
        //
        IGNORE_HR(_pHostUIHandler->QueryInterface(IID_IOleCommandTarget,
                                            (void**)&pHostUICommandHandler));
    }
    ReplaceInterface(&_pHostUICommandHandler, pHostUICommandHandler);
    ReleaseInterface(pHostUICommandHandler);
    
    return S_OK;
}


//+------------------------------------------------------------------------------
//
//      Member : FirePersistOnloads ()
//
//      Synopsis : temporary helper function to fire the history and shortcut onload
//          events.
//
//+------------------------------------------------------------------------------
void
CDoc::FirePersistOnloads()
{
    CNotification   nf;
    long            i;
    CStackPtrAry<CElement *, 64>  aryElements(Mt(CDocPersistLoad_aryElements_pv));
                
    if (_cstrHistoryUserData)
    {
        nf.XtagHistoryLoad(PrimaryRoot(), &aryElements);
        BroadcastNotify(&nf);

        for (i = 0; i < aryElements.Size(); i++)
        {
            aryElements[i]->TryPeerPersist(XTAG_HISTORY_LOAD, 0);
        }
    }
    else if (_pShortcutUserData)
    {
        FAVORITES_NOTIFY_INFO   sni;

        // load the favorites
        sni.pINPB = _pShortcutUserData;
        sni.bstrNameDomain = SysAllocString(_T("DOC"));
        if (sni.bstrNameDomain != NULL)
        {
            nf.FavoritesLoad(PrimaryRoot(), &aryElements);
            BroadcastNotify(&nf);

            for (i = 0; i < aryElements.Size(); i++)
            {
                aryElements[i]->TryPeerPersist(FAVORITES_LOAD, &sni);
            }

            SysFreeString(sni.bstrNameDomain);
        }
    }
}


HRESULT
CDoc::PersistFavoritesData(INamedPropertyBag * pINPB,
                          LPCWSTR strDomain)
{
    HRESULT      hr = S_OK;
    PROPVARIANT  varValue;

    Assert (pINPB);

    // now load the variant, and save each property we are
    // interested in
    V_VT(&varValue) = VT_BSTR;
    V_BSTR(&varValue) = _cstrUrl;

    // for the document.. ALWAYS save the baseurl. this
    //  is used later for the security checkes fo the subframes.
    hr = THR(pINPB->WritePropertyNPB(strDomain,
                                     _T("BASEURL"),
                                     &varValue));

    RRETURN(hr);
}

//+------------------------------------------------------------------------------
//
//  Member : GetPersistID
//
//  Synopsis: persistence support function. Called from the persistData object in
//      order to get security information from the correct domain
//
//--------------------------------------------------------------------------------
BSTR
CDoc::GetPersistID()
{
    CFrameSite*  pParentFrame = ParentFrameSite();

    if (pParentFrame )
        return pParentFrame->GetPersistID();
    else
        return SysAllocString(_T("DOC"));

}

//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::GetFirstTimeoutEvent
//
//  Synopsis:   Returns the first timeout event if given event is found in the
//                 list. Im most cases the returned event will be the one
//                 with given id, but if WM_TIMER messages came out of order
//                 it can be another one with smaller target time.
//              Removes the event from the list before returning.
//
//              Return value is S_FALSE if given event is not in the list
//--------------------------------------------------------------------------

HRESULT
CTimeoutEventList::GetFirstTimeoutEvent(UINT uTimerID, TIMEOUTEVENTINFO **ppTimeout)
{
    HRESULT           hr = S_OK;
    int  nNumEvents = _aryTimeouts.Size();
    int               i;

    Assert(ppTimeout != NULL);

    // Find the event first
    for(i = nNumEvents - 1; i >= 0  ; i--)
    {
        if(_aryTimeouts[i]->_uTimerID == uTimerID)
            break;
    }

    if(i < 0)
    {
        // The event is no longer active, or there is an error
        *ppTimeout = NULL;
        hr = S_FALSE;
        goto Cleanup;
    }

    // Elements are sorted and given event is in the list.
    // As long as given element is in the list we can return the
    //      last element without further checks
    *ppTimeout = _aryTimeouts[nNumEvents - 1];

#ifndef WIN16
    // Win16: Use GetTimeout(pTimeout->_uTimerID, dummy pTimeout) to delete this.

    // Remove it from the array
    _aryTimeouts.Delete(nNumEvents - 1);
#endif


Cleanup:
    RRETURN1(hr, S_FALSE);
}



//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::GetTimeout
//
//  Synopsis:   Gets timeout event with given timer id and removes it from the list
//
//              Return value is S_FALSE if given event is not in the list
//--------------------------------------------------------------------------

HRESULT
CTimeoutEventList::GetTimeout(UINT uTimerID, TIMEOUTEVENTINFO **ppTimeout)
{
    int                   i;
    HRESULT               hr;

    for(i = _aryTimeouts.Size() - 1; i >= 0  ; i--)
    {
        if(_aryTimeouts[i]->_uTimerID == uTimerID)
            break;
    }

    if(i >= 0)
    {
        *ppTimeout = _aryTimeouts[i];
        // Remove the pointer
        _aryTimeouts.Delete(i);
        hr = S_OK;
    }
    else
    {
        *ppTimeout = NULL;
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::InsertIntoTimeoutList
//
//  Synopsis:   Saves given timeout info pointer in the list
//
//              Returns the ID associated with timeout entry
//--------------------------------------------------------------------------

HRESULT
CTimeoutEventList::InsertIntoTimeoutList(TIMEOUTEVENTINFO *pTimeoutToInsert, UINT *puTimerID, BOOL fNewID)
{
    HRESULT hr = S_OK;
    int  i;
    int  nNumEvents = _aryTimeouts.Size();

    Assert(pTimeoutToInsert != NULL);

    // Fill the timer ID field with the next unused timer ID
    // We add this to make its appearance random
    if ( fNewID )
    {
#ifdef WIN16
        pTimeoutToInsert->_uTimerID = _uNextTimerID;
        _uNextTimerID = (_uNextTimerID < (UINT)0xFFFF) ? (_uNextTimerID+1) : 0xC000;
#else
        pTimeoutToInsert->_uTimerID = _uNextTimerID++ + (DWORD)(DWORD_PTR)this;
#endif
    }

    // Find the appropriate position. Current implementation keeps the elements
    // sorted by target time, with the one having min target time near the top
    for(i = 0; i < nNumEvents  ; i++)
    {
        if(pTimeoutToInsert->_dwTargetTime >= _aryTimeouts[i]->_dwTargetTime)
        {
            // Insert before current element
            hr = THR(_aryTimeouts.Insert(i, pTimeoutToInsert));
            if (hr)
                goto Cleanup;

            break;
        }
    }

    if(i == nNumEvents)
    {
        /// Append at the end
        hr = THR(_aryTimeouts.Append(pTimeoutToInsert));
        if (hr)
            goto Cleanup;
    }

    if (puTimerID)
        *puTimerID = pTimeoutToInsert->_uTimerID;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::KillAllTimers
//
//  Synopsis:   Stops all the timers in the list and removes events from the list
//
//--------------------------------------------------------------------------

void
CTimeoutEventList::KillAllTimers(void * pvObject)
{
    int i;

    for(i = _aryTimeouts.Size() - 1; i >= 0; i--)
    {
        Verify(FormsKillTimer(pvObject, _aryTimeouts[i]->_uTimerID  ) == S_OK);
        delete _aryTimeouts[i];
    }
    _aryTimeouts.DeleteAll();

    for(i = _aryPendingTimeouts.Size() - 1; i >= 0; i--)
    {
        delete _aryTimeouts[i];
    }
    _aryPendingTimeouts.DeleteAll();
    _aryPendingClears.DeleteAll();
}

//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::GetPendingTimeout
//
//  Synopsis:   Gets a pending Timeout, and removes it from the list
//
//--------------------------------------------------------------------------
BOOL
CTimeoutEventList::GetPendingTimeout( TIMEOUTEVENTINFO **ppTimeout )
{
    int i;
    Assert( ppTimeout );
    if ( (i=_aryPendingTimeouts.Size()-1) < 0 )
    {
        *ppTimeout = NULL;
        return FALSE;
    }

    *ppTimeout = _aryPendingTimeouts[i];
    _aryPendingTimeouts.Delete(i);
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::ClearPendingTimeout
//
//  Synopsis:   Removes a timer from the pending list and returns TRUE.
//              If timer with ID not found, returns FALSE
//
//--------------------------------------------------------------------------
BOOL
CTimeoutEventList::ClearPendingTimeout( UINT uTimerID )
{
    BOOL fFound = FALSE;

    for ( int i=_aryPendingTimeouts.Size() - 1; i >= 0; i-- )
    {
        if ( _aryPendingTimeouts[i]->_uTimerID == uTimerID )
        {
            delete _aryPendingTimeouts[i];
            _aryPendingTimeouts.Delete(i);
            fFound = TRUE;
            break;
        }
    }
    return fFound;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::GetPendingClear
//
//  Synopsis:   Returns TRUE and an ID of a timer that was cleared during
//              timer processing. If there are none left, it returns FALSE.
//
//--------------------------------------------------------------------------
BOOL
CTimeoutEventList::GetPendingClear( LONG *plTimerID )
{
    int i;
    if ( (i=_aryPendingClears.Size()-1) < 0 )
    {
        *plTimerID = 0;
        return FALSE;
    }

    *plTimerID = (LONG)_aryPendingClears[i];
    _aryPendingClears.Delete(i);
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     ReportScriptletError
//
//  Synopsis:   Displays the scriptlet error that occurred.  The pContext
//              is either CPeerSite or CPeerFactoryUrl.  Depending
//              on that context we'll either return compile time or runtime
//              errors.
//
//--------------------------------------------------------------------------

HRESULT
ErrorRecord::Init(IScriptletError *pSErr, LPTSTR pchDocURL)
{
    HRESULT     hr;

    Assert(pSErr);

    hr = THR(pSErr->GetExceptionInfo(&_ExcepInfo));
    if (hr)
        goto Cleanup;

    hr = THR(pSErr->GetSourcePosition(&_uLine, &_uColumn));
    if (hr)
        goto Cleanup;

    _pchURL = pchDocURL;

Cleanup:
    RRETURN(hr);
}

HRESULT
ErrorRecord::Init(IActiveScriptError *pASErr, CDoc * pDoc)
{
    HRESULT     hr;
    long        uColumn;
    LPTSTR      pchMarkupURL = NULL;

    Assert(pASErr);

    hr = THR(pASErr->GetExceptionInfo(&_ExcepInfo));
    if (hr)
        goto Cleanup;

    hr = THR(pASErr->GetSourcePosition(&_dwSrcContext, &_uLine, &uColumn));
    if (hr)
        goto Cleanup;

    if (NO_SOURCE_CONTEXT != _dwSrcContext && pDoc->_pScriptCookieTable)
    {
        HRESULT                 hr2;
        CMarkup *               pMarkup;
        CMarkupScriptContext *  pMarkupScriptContext;

        hr2 = THR(pDoc->_pScriptCookieTable->GetSourceObjects(_dwSrcContext, &pMarkup, NULL, &_pScriptDebugDocument));
        if (S_OK == hr2 && pMarkup)
        {
            pMarkupScriptContext = pMarkup->ScriptContext();
            pchMarkupURL = pMarkupScriptContext ? (LPTSTR)pMarkupScriptContext->_cstrUrl : NULL;
        }
    }

    _uColumn = uColumn;
    // IActiveScriptError assumes line #s/column #s are zero based.
    _uLine++;
    _uColumn++;

    _pchURL = pchMarkupURL ? pchMarkupURL : pDoc->_cstrUrl;

Cleanup:
    RRETURN(hr);
}

HRESULT
ErrorRecord::Init(HRESULT hrError, LPTSTR pchErrorMessage, LPTSTR pchDocURL)
{
    HRESULT     hr;

    _pchURL = pchDocURL;
    _ExcepInfo.scode = hrError;

    hr = THR(FormsAllocString(pchErrorMessage, &_ExcepInfo.bstrDescription));

    RRETURN (hr);
}

HRESULT
CDoc::ReportScriptError(ErrorRecord &errRecord)
{
    HRESULT     hr = S_OK;
    TCHAR      *pchDescription;
    TCHAR       achDescription[256];
    BSTR        bstrURL = NULL;
    BSTR        bstrDescr = NULL;
    BOOL        fContinueScript = FALSE;
    BOOL        fErrorHandled;

#ifdef UNIX // support scripting error dialog option.
    HKEY        key;
    TCHAR       ach[10];
    DWORD       dwType, dwLength = 10 * sizeof(TCHAR);
    BOOL        fShowDialog = TRUE;
#endif

    // No more messages to return or if the script was aborted (through the
    // script debugger) then don't put up any UI.
    if (_fEngineSuspended || errRecord._ExcepInfo.scode == E_ABORT)
        goto Cleanup;

    // These errors are reported on LeaveScript where we're guaranteed to have
    // enough memory and stack space for the error message.
    if (_fStackOverflow || _fOutOfMemory)
    {
        if (!_fEngineSuspended)
        {
            _badStateErrLine = errRecord._uLine;
            goto StopAllScripts;
        }
        else
            goto Cleanup;
    }

    //
    // Get a description of the error.
    //

    // vbscript passes empty strings and jscript passes NULL, so check for both

    if (errRecord._ExcepInfo.bstrDescription && *errRecord._ExcepInfo.bstrDescription)
    {
        pchDescription = errRecord._ExcepInfo.bstrDescription;
    }
    else
    {
        GetErrorText(errRecord._ExcepInfo.scode, achDescription, ARRAY_SIZE(achDescription));
        pchDescription = achDescription;
    }

    hr = THR(EnsureOmWindow());
    if (hr)
        goto Cleanup;

    if (_pOmWindow)
    {
        if (errRecord._pchURL)
        {
            hr = THR(FormsAllocString(errRecord._pchURL, &bstrURL));
            if (hr)
                goto Cleanup;
        }

        // Allocate a BSTR for the description string
        hr = THR(FormsAllocString(pchDescription, &bstrDescr));
        if (hr)
            goto Cleanup;

        fErrorHandled = _pOmWindow->Fire_onerror(bstrDescr,
                                                 bstrURL,
                                                 errRecord._uLine,
                                                 errRecord._uColumn,
                                                 errRecord._ExcepInfo.wCode,
                                                 FALSE);

        if (!fErrorHandled)
        {
    #ifdef UNIX // Popup error-dialog? No script error dialog = No script debugger ?
            if (ERROR_SUCCESS == RegOpenKey( HKEY_CURRENT_USER,
                                             _T("Software\\MICROSOFT\\Internet Explorer\\Main"),
                                             &key))
            {
                if (ERROR_SUCCESS == RegQueryValueEx( key, _T("Disable Scripting Error Dialog"),
                                                      0, &dwType, (LPBYTE)ach, &dwLength))
                {
                    fShowDialog = (_tcscmp(ach, _T("no")) == 0);
                }
                RegCloseKey(key);
            }

            if ( !fShowDialog )
            {
                fContinueScript = TRUE; //Keep script running.
                hr = S_OK;
                goto StopAllScripts;
            }
    #endif

            if (errRecord._ExcepInfo.scode == VBSERR_OutOfStack)
            {
                if (!_fStackOverflow)
                {
                    _badStateErrLine = errRecord._uLine;
                    _fStackOverflow = TRUE;
                }
            }
            else if (errRecord._ExcepInfo.scode == VBSERR_OutOfMemory)
            {
                if (!_fOutOfMemory)
                {
                    _badStateErrLine = errRecord._uLine;
                    _fOutOfMemory = TRUE;
                }
            }

            // Stack overflow or out of memory error?
            if (_fStackOverflow || _fOutOfMemory)
            {
                // If pending stack overflow/out of memory have we
                // unwound the stack enough before displaying the
                // message.  These CDoc::LeaveScript function will
                // actually display the message when we leave the
                // last script.
                if (!_fEngineSuspended)
                    goto StopAllScripts;
                else
                    goto Cleanup;
            }

            fContinueScript = _pOmWindow->Fire_onerror(bstrDescr,
                                                       bstrURL,
                                                       errRecord._uLine,
                                                       errRecord._uColumn,
                                                       errRecord._ExcepInfo.wCode,
                                                       TRUE);


StopAllScripts:
            //
            // If our container has asked us to refrain from using dialogs, we
            // should abort the script.  Otherwise we could end up in a loop
            // where we just sit and spin.
            //
            if ((_dwLoadf & DLCTL_SILENT) || !fContinueScript)
            {
                // Shutoff non-function object based script engines (VBSCRIPT)
                if (_pScriptCollection)
                    IGNORE_HR(_pScriptCollection->SetState(SCRIPTSTATE_DISCONNECTED));

                // Shutoff function object based script engines (JSCRIPT)
                _dwLoadf |= DLCTL_NO_SCRIPTS;

               _fEngineSuspended = TRUE;

                hr = S_OK;
            }
        }
    }

Cleanup:
    FormsFreeString(bstrDescr);
    FormsFreeString(bstrURL);

    RRETURN(hr);
}

STDMETHODIMP CDoc::createDocumentFragment(IHTMLDocument2 **ppNewDoc)
{
    return _pPrimaryMarkup->createDocumentFragment(ppNewDoc);
}

STDMETHODIMP CDoc::get_parentDocument(IHTMLDocument2 **ppParentDoc)
{
    // root doc!
    if (ppParentDoc)
        *ppParentDoc = NULL;

    return S_OK;
}

STDMETHODIMP CDoc::get_enableDownload(VARIANT_BOOL *pfDownlaod)
{
    return _pPrimaryMarkup->get_enableDownload(pfDownlaod);
}

STDMETHODIMP CDoc::put_enableDownload(VARIANT_BOOL fDownlaod)
{
    return _pPrimaryMarkup->put_enableDownload(fDownlaod);
}

STDMETHODIMP CDoc::get_baseUrl(BSTR * p)
{
    if ( p )
        *p = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

STDMETHODIMP CDoc::put_baseUrl(BSTR b)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

STDMETHODIMP CDoc::get_childNodes(IDispatch **ppChildCollection)
{
    return _pPrimaryMarkup->get_childNodes(ppChildCollection);
}

STDMETHODIMP CDoc::get_inheritStyleSheets(VARIANT_BOOL *pfInherit)
{
    return _pPrimaryMarkup->get_inheritStyleSheets(pfInherit);
}

STDMETHODIMP CDoc::put_inheritStyleSheets(VARIANT_BOOL fInherit)
{
    return _pPrimaryMarkup->put_inheritStyleSheets(fInherit);
}

HRESULT
CDoc::getElementsByName(BSTR v, IHTMLElementCollection** ppDisp)
{
    return _pPrimaryMarkup->getElementsByName(v, ppDisp);
}

HRESULT
CDoc::getElementsByTagName(BSTR v, IHTMLElementCollection** ppDisp)
{
    return _pPrimaryMarkup->getElementsByTagName(v, ppDisp);
}

HRESULT
CDoc::getElementById(BSTR v, IHTMLElement** ppel)
{
    return _pPrimaryMarkup->getElementById(v, ppel);
}
