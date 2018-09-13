//+------------------------------------------------------------------------
//
//  File:       wscript.cxx
//
//  Contents:   CDoc deferred-script execution support.
//
//              For handling two main cases:
//
//              (1) Deferring inline script execution while not yet
//                  in-place active.
//
//              (2) Deferring inline script execution while waiting
//                  for script to download, or while other inline
//                  script is already running higher on the stack.
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ESCRIPT_HXX_
#define X_ESCRIPT_HXX_
#include "escript.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#define SCRIPTSDEFERDELAY 113

//+-------------------------------------------------------------------
//
//  Member:     CDoc::IsInScript
//
//  Synopsis:   TRUE if we are in a script
//
//--------------------------------------------------------------------

BOOL 
CDoc::IsInScript()
{
    // If we are in a script - then IsInScript is true.  NOTE: we aggregate
    // our scriptnesting with those of our children
    return (_cScriptNesting > 0) ? TRUE : FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::EnterScript
//
//  Synopsis:   Note that we are inside a script
//
//--------------------------------------------------------------------

HRESULT 
CDoc::EnterScript()
{
    IOleCommandTarget * pCT;
    HRESULT hr = S_OK;

    _cScriptNesting++;
    //  Fire off Exec (this will go up the frame hierarchy) to indicate that
    //  SHDOCVW should not retry deactivating any docobjects whose deactivation was
    //  deferred due to a child being in script code.
        //  Quickly forward this up to command target of our clientsite
        //  this is how we tell SHDOCVW to try to activate any view whose
        //  activation was deferred while some frame was in a script.  as
        //  a script that is executing blocks activation of any parent frame
        //  this has to (potentially) be forwarded all the way up to browser
        //  window

    if (_cScriptNesting > 15)
    {
        hr = VBSERR_OutOfStack;
        _fStackOverflow = TRUE;
        goto Cleanup;
    }

    if (_cScriptNesting == 1)
    {
        if (_pClientSite)
        {
            hr = THR_NOTRACE(_pClientSite->QueryInterface(
                        IID_IOleCommandTarget,
                        (void **) &pCT));
            if (!hr)
            {
                THR_NOTRACE(pCT->Exec(
                        &CGID_ShellDocView,
                        SHDVID_NODEACTIVATENOW,
                        OLECMDEXECOPT_DONTPROMPTUSER,
                        NULL,
                        NULL));
                pCT->Release();
            }

            hr = S_OK;
        }

        // Initialize error condition flags/counters.
        _fStackOverflow = FALSE;
        _fOutOfMemory = FALSE;
        _fEngineSuspended = FALSE;
        _badStateErrLine = 0;

        // Start tracking total number of script statements executed
        _dwTotalStatementCount = 0;

        EnsureOptionSettings();

        // Initialize the max statement count from default
        _dwMaxStatements = _pOptionSettings->dwMaxStatements;
        
        _fUserStopRunawayScript = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::LeaveScript
//
//  Synopsis:   Note that we are leaving a script
//
//--------------------------------------------------------------------

HRESULT 
CDoc::LeaveScript()
{
    IOleCommandTarget * pCT;
    HRESULT hr = S_OK;

    Assert(_cScriptNesting > 0);

    if (_cScriptNesting == 0)
        return E_FAIL;

    _cScriptNesting--;

    if (_cScriptNesting == 0)
    {
        if (_pClientSite)
        {
            //  Fire off Exec (this will go up the frame hierarchy) to indicate that
            //  SHDOCVW should retry deactivating any docobjects whose deactivation was
            //  deferred due to a child being in script code.
            hr = THR_NOTRACE(_pClientSite->QueryInterface(
                        IID_IOleCommandTarget,
                        (void **) &pCT));
            if (!hr)
            {
                THR_NOTRACE(pCT->Exec(
                        &CGID_ShellDocView,
                        SHDVID_DEACTIVATEMENOW,
                        OLECMDEXECOPT_DONTPROMPTUSER,
                        NULL,
                        NULL));
                pCT->Release();
            }
            hr = S_OK;
        }

        // Clear accumulated count of statements
        _dwTotalStatementCount = 0;
        _dwMaxStatements = 0;

        // Reset the user's inputs so that other scripts may execute
        _fUserStopRunawayScript = FALSE;

        if (_fStackOverflow || _fOutOfMemory)
        {
            // Bring up a simple dialog box for stack overflow or
            // out of memory.  We don't have any room to start up
            // the parser and create an HTML dialog as OnScriptError
            // would do.
            TCHAR   achBuffer[256];

            Format(0,
                   achBuffer,
                   ARRAY_SIZE(achBuffer),
                   _T("<0s> at line: <1d>"),
                   _fStackOverflow ? _T("Stack overflow") : _T("Out of memory"),
                   _badStateErrLine);
            _pOmWindow->alert(achBuffer);
        }
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::QueryContinueScript
//
//  Synopsis:   See if the current script fragment has been executing
//              for too long.
//
//--------------------------------------------------------------------

HRESULT CDoc::QueryContinueScript( ULONG ulStatementCount )
{

    // If the user has already made a decision about aborting or
    // continuing the script, repeat that answer until all presently
    // running scripts are dead.
    if (_fUserStopRunawayScript)
        return E_ABORT;

    // it is possible that this call get's reentered while we are 
    // currently displaying the ContinueScript dialog
    // prevent popping up another dialog and continue to run the 
    // scripts until the user decided on the first one (frankman&GaryBu)
    if (_fQueryContinueDialogIsUp)
        return S_OK;
    
    // Accumulate the statement  since we were last called
    _dwTotalStatementCount += ulStatementCount;

    // rgardner, for now it seems sensible not to factor in time. Many OCXs
    // for instance will take ages to do their job, but only take one
    // statement to do so.

    if ( _dwTotalStatementCount > _dwMaxStatements )
    {
        int iUserResponse;
        TCHAR achMsg[256];

        _fQueryContinueDialogIsUp = TRUE;
        
        IGNORE_HR(Format(0, achMsg, ARRAY_SIZE(achMsg), MAKEINTRESOURCE(IDS_RUNAWAYSCRIPT)));
        ShowMessageEx( &iUserResponse, MB_ICONEXCLAMATION | MB_YESNO, NULL, 0, achMsg );

        _fQueryContinueDialogIsUp = FALSE;
            

        if (iUserResponse == IDYES)
        {
            _fUserStopRunawayScript = TRUE;
            return E_ABORT;
        }
        else
        {
            _dwTotalStatementCount = 0;
            // User has chosen to continue execution, increase the interval to the next
            // warning
            if ( _dwMaxStatements < ((DWORD)-1)>>3 )
                _dwMaxStatements <<= 3;
            else
                _dwMaxStatements = (DWORD)-1;
        }
    }
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::IsInInline
//
//  Synopsis:   TRUE if we are in an inline script
//
//--------------------------------------------------------------------

BOOL CMarkup::IsInInline()
{
    return HasScriptContext() ? (ScriptContext()->_cInlineNesting > 0) : FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::EnterInline
//
//  Synopsis:   Note that we are inside an inline script
//
//--------------------------------------------------------------------

HRESULT 
CMarkup::EnterInline()
{
    HRESULT                 hr;
    CMarkupScriptContext *  pScriptContext;

    hr = THR(EnsureScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    // inline script contributes twice to _cScriptNesting
    hr = THR(_pDoc->EnterScript());
    if (hr)
        goto Cleanup;

    pScriptContext->_cInlineNesting++;
    
Cleanup:
    RRETURN(hr); 
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::LeaveInline
//
//  Synopsis:   Note that we are leaving an inline script
//
//--------------------------------------------------------------------

HRESULT 
CMarkup::LeaveInline()
{
    HRESULT                 hr;
    CMarkupScriptContext *  pScriptContext = ScriptContext();
    
    Assert (pScriptContext);

    if (pScriptContext->_cInlineNesting == 1)
    {
        // Before leaving top-level execution, commit queued
        // scripts. (may block parser)
        hr = THR(CommitQueuedScripts());
        if (hr)
            goto Cleanup;
    }
        
    // inline script contributes twice to _cScriptNesting
    hr = THR(_pDoc->LeaveScript());
    if (hr)
        goto Cleanup;

    pScriptContext->_cInlineNesting--;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::AllowInlineExecution
//
//  Synopsis:   Return whether inline scripts are allowed to be
//              parsed.
//
//--------------------------------------------------------------------

BOOL
CMarkup::AllowInlineExecution()
{
    // script parsing contexts are always alowed to Execute if we are
    // already running inside an inline script (note: the script parsing
    // context knows to queue nested <SCRIPT SRC=*> tags for actual
    // execution after the outermost script is finished).
    if (IsInInline())
        return TRUE;

    // Otherwise, we're at the top level
    return AllowImmediateExecution();
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::AllowImmediateExecution
//
//  Synopsis:   Return whether scripts are allowed to be committed.
//
//--------------------------------------------------------------------
BOOL
CMarkup::AllowImmediateExecution()
{
    // No Execute is allowed while we are not inplace
    if (_pDoc->_fNeedInPlaceActivation &&
        !_pDoc->IsPrintDoc() &&
        (_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE) &&
        _pDoc->State() < OS_INPLACE)
        return FALSE;
        
    // No Execute is allowed while we are downloading script
    if (HasScriptContext() && ScriptContext()->_cScriptDownloading)
        return FALSE;

    // Normally inline script can be run
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::WaitInlineExecution
//
//  Synopsis:   Sleeps the CHtmLoadCtx and sets things up to
//              wait until script execution is allowed again
//
//--------------------------------------------------------------------

HRESULT
CMarkup::WaitInlineExecution()
{
    HRESULT                 hr;
    BOOL                    fRequestInPlaceActivation = FALSE;
    CMarkupScriptContext *  pScriptContext;

    hr = THR(EnsureScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    Assert(!pScriptContext->_fWaitScript);

    // If we're not inplace active, request inplace activation
    if (_pDoc->_fNeedInPlaceActivation &&
        (_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE) &&
        _pDoc->State() < OS_INPLACE)
    {
        fRequestInPlaceActivation = TRUE;
        pScriptContext->_fWaitScript = TRUE;
    }
    
    // If we're waiting on script to download
    if (pScriptContext->_cScriptDownloading)
    {
        pScriptContext->_fWaitScript = TRUE;
    }
    
    Assert(pScriptContext->_fWaitScript);

    // Sleep the HTML loader while we wait
    if (pScriptContext->_fWaitScript && HtmCtx())
    {
        HtmCtx()->Sleep(TRUE);
    }

    if (fRequestInPlaceActivation)
    {
        hr = THR(_pDoc->RegisterMarkupForInPlace(this));
        if (hr)
            goto Cleanup;

        // No more pics allowed.
        if (_pDoc->_pctPics)
            THR(_pDoc->_pctPics->Exec(&CGID_ShellDocView, SHDVID_NOMOREPICSLABELS, 0, NULL, NULL));

        // Request in-place activation by going interactive
        _pDoc->SetInteractive(0);
    }

Cleanup:
    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::WakeUpExecution
//
//--------------------------------------------------------------------

HRESULT
CMarkup::WakeUpExecution()
{
    HRESULT                 hr = S_OK;
    CMarkupScriptContext *  pScriptContext = ScriptContext();

    // If we were blocking execution of inline script, unblock
    if (pScriptContext && pScriptContext->_fWaitScript && AllowInlineExecution())
    {
        pScriptContext->_fWaitScript = FALSE;
        if (HtmCtx())
            HtmCtx()->Sleep(FALSE);
    }
    
    // If we are outside of inline script when we discover
    // we are in-place, ensure execution of any queued
    // scripts which may have been waiting
    // (may reblock parser)
    if (!pScriptContext || pScriptContext->_cInlineNesting == 0)
    {
        hr = THR(CommitQueuedScriptsInline());
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::RegisterMarkupForInPlace
//
//--------------------------------------------------------------------

HRESULT
CDoc::RegisterMarkupForInPlace(CMarkup * pMarkup)
{
    HRESULT     hr;

    // assert that the markup is not registered already
    Assert (-1 == _aryMarkupNotifyInPlace.Find(pMarkup));

    hr = _aryMarkupNotifyInPlace.Append(pMarkup);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::UnregisterMarkupForInPlace
//
//--------------------------------------------------------------------

HRESULT
CDoc::UnregisterMarkupForInPlace(CMarkup * pMarkup)
{
    HRESULT     hr = S_OK;

    _aryMarkupNotifyInPlace.DeleteByValue(pMarkup);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::NotifyMarkupsInPlace
//
//--------------------------------------------------------------------

HRESULT
CDoc::NotifyMarkupsInPlace()
{
    HRESULT     hr = S_OK;
    int         c;
    CMarkup *   pMarkup;

    while (0 != (c = _aryMarkupNotifyInPlace.Size()))
    {
        pMarkup = _aryMarkupNotifyInPlace[c - 1];

        _aryMarkupNotifyInPlace.Delete(c - 1);

        hr = THR(pMarkup->WakeUpExecution());
        if (hr)
            break;
    }
    Assert (0 == _aryMarkupNotifyInPlace.Size());

    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::IsScriptDownload
//
//--------------------------------------------------------------------

BOOL
CMarkup::IsScriptDownload()
{
    return HasScriptContext() ? (ScriptContext()->_cScriptDownloading > 0) : FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::EnterScriptDownload
//
//  Synopsis:   Note that a script is being downloaded
//
//--------------------------------------------------------------------

void
CMarkup::EnterScriptDownload(DWORD * pdwCookie)
{
    HRESULT                 hr;
    CMarkupScriptContext *  pScriptContext;

    hr = THR(EnsureScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    if (*pdwCookie != pScriptContext->_dwScriptDownloadingCookie)
    {
        *pdwCookie = pScriptContext->_dwScriptDownloadingCookie;
        pScriptContext->_cScriptDownloading++;
    }

Cleanup:
    return;
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::LeaveScriptDownload
//
//  Synopsis:   Unblock script if we were waiting for script
//              download to run script.
//
//--------------------------------------------------------------------

HRESULT
CMarkup::LeaveScriptDownload(DWORD * pdwCookie)
{
    HRESULT                 hr = S_OK;
    CMarkupScriptContext *  pScriptContext = ScriptContext();
    
    Assert (pScriptContext);

    if (*pdwCookie == pScriptContext->_dwScriptDownloadingCookie)
    {
        *pdwCookie = 0;
        pScriptContext->_cScriptDownloading--;

        hr = THR(WakeUpExecution());
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::EnqueueScriptToCommit
//
//  Synopsis:   Remember an inline script (with SRC=*) which needs
//              to be committed before unblocking the parser.
//
//              This happens when a script with SRC=* is written
//              by an inline script.
//
//--------------------------------------------------------------------

HRESULT
CMarkup::EnqueueScriptToCommit(CScriptElement *pelScript)
{
    HRESULT                 hr;
    CMarkupScriptContext *  pScriptContext;
    
    hr = THR(EnsureScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    hr = THR(pScriptContext->_aryScriptEnqueued.Append(pelScript));
    if (hr)
        goto Cleanup;

    pelScript->AddRef();

Cleanup:
    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::CommitQueuedScripts
//
//  Synopsis:   Execute saved up scripts
//
//              Must be called from within EnterInline/LeaveInline
//
//--------------------------------------------------------------------

HRESULT
CMarkup::CommitQueuedScripts()
{
    HRESULT                 hr = S_OK;
    CScriptElement *        pelScript = NULL;
    CMarkupScriptContext *  pScriptContext = ScriptContext();

    Assert(pScriptContext->_cInlineNesting == 1);

continue_label:

    while (pScriptContext->_aryScriptEnqueued.Size() && AllowImmediateExecution())
    {
        pelScript = pScriptContext->_aryScriptEnqueued[0];
        
        pScriptContext->_aryScriptEnqueued.Delete(0);

        hr = THR(pelScript->CommitCode());
        if (hr)
            goto Cleanup;

        pelScript->Release();
        pelScript = NULL;
    }

    if (pScriptContext->_aryScriptEnqueued.Size() && !pScriptContext->_fWaitScript)
    {
        Assert(!AllowImmediateExecution());
        hr = THR(WaitInlineExecution());
        if (hr)
            goto Cleanup;
        if (AllowImmediateExecution())
            goto continue_label;
    }
        
#ifdef VSTUDIO7
    // Once all the scripts are commited, check to see if we did
    // not handling finish a <?FACTORY> tag. If so, call GetBaseTagsFromFactory()
    // on that guy.
    if (_pDoc->_fNeedBaseTags && !pScriptContext->_aryScriptEnqueued.Size())
    {
        Assert(!_pDoc->_cstrIdentityFactoryUrl.IsNull());
        hr = _pDoc->GetBaseTagsFromFactory(_pDoc->_cstrIdentityFactoryUrl);
    }
#endif //VSTUDIO7

Cleanup:
    if (pelScript)
        pelScript->Release();
    
    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::CommitQueuedScriptsInline
//
//  Synopsis:   Execute saved up scripts
//
//              Does the EnterInline/LeaveInline so that inline
//              scripts can be executed from the outside
//              CHtmScriptParseCtx::Execute.
//
//--------------------------------------------------------------------

HRESULT
CMarkup::CommitQueuedScriptsInline()
{
    HRESULT                 hr = S_OK;
    HRESULT                 hr2;
    CMarkupScriptContext *  pScriptContext = ScriptContext();
    
    // optimization: nothing to do if there are no queued scripts
    if (!pScriptContext || !pScriptContext->_aryScriptEnqueued.Size())
        goto Cleanup;

    hr = THR(EnterInline());
    if (hr)
        goto Cleanup;

    hr2 = THR(CommitQueuedScripts());

    hr = THR(LeaveInline());
    if (hr2)
        hr = hr2;
        
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DeferScript
//
//----------------------------------------------------------------------------

HRESULT
CDoc::DeferScript(CScriptElement * pScript)
{
    HRESULT     hr;
    BOOL        fAllow;

    hr = THR(ProcessURLAction(URLACTION_SCRIPT_RUN, &fAllow));
    if (hr || !fAllow)
        goto Cleanup;

    pScript->AddRef();
    pScript->_fDeferredExecution = TRUE;

    hr = THR(_aryElementDeferredScripts.Append(pScript));
    if (hr)
        goto Cleanup;

    if (!_fDeferredScripts)
    {
        _fDeferredScripts = TRUE;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CommitDeferredScripts
//
//----------------------------------------------------------------------------

HRESULT
CDoc::CommitDeferredScripts(BOOL fEarly)
{
    HRESULT             hr = S_OK;
    int                 i;
    BOOL                fLeftSome = FALSE;

    if (!_fDeferredScripts)
        return S_OK;

    // Loop is structured inefficiently on purpose: we need to withstand
    // _aryElementDefereedScripts growing/being deleted in place
    
    for (i = 0; i < _aryElementDeferredScripts.Size(); i++)
    {
        CScriptElement *pScript = _aryElementDeferredScripts[i];
        
        if (pScript)
        {
            if (fEarly && pScript->_fSrc)
            {
                fLeftSome = TRUE;
            }
            else
            {
                _aryElementDeferredScripts[i] = NULL;
                
                Assert(pScript->_fDeferredExecution);
                pScript->_fDeferredExecution = FALSE;
                IGNORE_HR(pScript->CommitCode());
                pScript->Release();
            }
        }
    }

    Assert(!fLeftSome || fEarly);

    if (!fLeftSome)
    {
        _fDeferredScripts = FALSE;
        _aryElementDeferredScripts.DeleteAll();
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CommitScripts
//
//  Synopsis:   hooks up scripts 
//
//----------------------------------------------------------------------------

HRESULT
CDoc::CommitScripts(CBase *pelTarget, BOOL fHookup)
{
    HRESULT             hr = S_OK;
    CScriptElement *    pScript;
    int                 cScripts;
    int                 iScript;
    BOOL                fAllow;
    CCollectionCache *  pCollectionCache;

    if (!pelTarget)
    {
        hr = THR(CommitDeferredScripts(FALSE));
        if (hr)
            goto Cleanup;
    }

    if (!_fHasScriptForEvent)
        goto Cleanup;
        
    hr = THR(PrimaryMarkup()->EnsureCollectionCache(CMarkup::SCRIPTS_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = PrimaryMarkup()->CollectionCache();

    cScripts = pCollectionCache->SizeAry(CMarkup::SCRIPTS_COLLECTION);
    if (!cScripts)
        goto Cleanup;
        
    hr = THR(ProcessURLAction(URLACTION_SCRIPT_RUN, &fAllow));
    if (hr || !fAllow)
        goto Cleanup;

    // iterate through all scripts in doc's script collection
    for (iScript = 0; iScript < cScripts; iScript++)
    {
        CElement *pElemTemp;

        hr = THR(pCollectionCache->GetIntoAry (CMarkup::SCRIPTS_COLLECTION, iScript, &pElemTemp));
        if (hr)
            goto Cleanup;

        pScript = DYNCAST(CScriptElement, pElemTemp);

        Assert (ETAG_SCRIPT == pElemTemp->Tag());

        if (!pScript->_fScriptCommitted || pelTarget)
            IGNORE_HR(pScript->CommitFunctionPointersCode(pelTarget, fHookup));
    }

Cleanup:
    RRETURN (hr);
}
