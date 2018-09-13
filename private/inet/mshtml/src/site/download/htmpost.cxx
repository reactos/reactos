//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmpost.cxx
//
//  Contents:   Implementation of CHtmPost class
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_ESCRIPT_HXX_
#define X_ESCRIPT_HXX_
#include "escript.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

#ifndef X_URLCOMP_HXX_
#define X_URLCOMP_HXX_
#include "urlcomp.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_VMEM_HXX_
#define X_VMEM_HXX_
#include "vmem.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include "prgsnk.h"
#endif


#define HTMPOST_BUFFER_SIZE     4096
#define HTMPOST_BUFFER_GROW     2048
#define POSTMAN_TIMESLICE       200

PerfDbgTag(tagHtmPost,      "Dwn", "Trace CHtmPost")
PerfDbgTag(tagOneToken,     "Dwn", "! HtmPost one token at a time");
PerfDbgTag(tagAllTokens,    "Dwn", "! HtmPost all available tokens at once");
PerfDbgTag(tagToken2,       "Dwn", "! HtmPost detailed token trace");
PerfDbgTag(tagParseSlowly,  "Dwn", "! HtmPost parse slowly")
PerfDbgTag(tagStrictPch,    "Dwn", "! HtmPost strict unterminated strings")
ExternTag(tagPalette);

PerfDbgExtern(tagPerfWatch)

MtDefine(CHtmPost, Dwn, "CHtmPost")
MtDefine(CHtmPost_aryphtSaved_pv, CHtmPost, "CHtmPost::_aryphtSaved::_pv")
MtDefine(CHtmPost_IsIndexHack_phtInput, Locals, "CHtmPost::IsIndexHack phtInput")
MtDefine(CHtmPost_ProcessIncludeToken_pchUrl, Locals, "CHtmPost::ProcessIncludeToken pchUrl")
MtDefine(CHtmPost_OnIncludeDwnChan_pbBuffer, Locals, "CHtmPost::OnIncludeDwnChan pbbuffer")
MtDefine(CHtmPost_OnIncludeDwnChan_pchSrcCode, Locals, "CHtmPost::OnIncludeDwnChan pchSrcCode")

DeclareTag(tagPeerCHtmPostRegisterXmlNamespace,  "Peer", "trace CHtmPost::RegisterXmlNamespace")

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

// Forward Declarations -------------------------------------------------------

void CALLBACK   PostManOnTimer(HWND hwnd, UINT umsg, UINT_PTR idevent, DWORD dwTime);
void            PostManSetTimer(THREADSTATE * pts);
void            PostManExecute(THREADSTATE * pts, DWORD dwTimeout, CHtmPost * pHtmPost);
HRESULT         PostManRunNested(THREADSTATE *pts, CHtmPost *pHtmPost);
void            PostManValidate();

//+------------------------------------------------------------------------
//
//  Member:     TraceToken
//
//-------------------------------------------------------------------------

#if DBG==1

#define PRETTY_CHAR(ch) ((ch) < 32 ? _T('~') : (ch) > 127 ? _T('@') : (ch))

void
TraceToken(CHtmPost *pHtmPost, CHtmTag *pht)
{
    if (pht)
    {
        if (pht->Is(ETAG_RAW_TEXT) || pht->Is(ETAG_RAW_COMMENT) || pht->Is(ETAG_RAW_TEXTFRAG))
        {
            TCHAR ach[128];
            char *pchTagName;
            UINT cch = min((UINT) ARRAY_SIZE(ach) - 1, (UINT)pht->GetCch());

            memcpy(ach, pht->GetPch(), cch * sizeof(TCHAR));
            ach[cch] = 0;
            for (TCHAR * pchT = ach; cch; --cch, ++pchT)
                *pchT = PRETTY_CHAR(*pchT);

            pchTagName = pht->Is(ETAG_RAW_TEXT) ? "TEXT" :
                         pht->Is(ETAG_RAW_COMMENT) ? "COMMENT" :
                         pht->Is(ETAG_RAW_SOURCE) ? "SOURCE" :
                         "TEXTFRAG";

            PerfDbgLog4(tagToken2, pHtmPost, "etag = %3ld %s cch=%ld \"%ls\"", pht->GetTag(),
                        pchTagName, pht->GetCch(), ach);
        }
        else if (pht->Is(ETAG_RAW_SOURCE))
        {
            PerfDbgLog2(tagToken2, pHtmPost, "etag = %3ld SOURCE cch=%ld",
                        pht->GetTag(), pht->GetSourceCch());
        }
        else if (pht->Is(ETAG_RAW_CODEPAGE))
        {
            PerfDbgLog3(tagToken2, pHtmPost, "etag = %3ld CODEPAGE cp=%ld fRestart=%ld",
                        pht->GetTag(), pht->GetCodepage(), pht->IsRestart());
        }
        else if (pht->Is(ETAG_RAW_DOCSIZE))
        {
            PerfDbgLog2(tagToken2, pHtmPost, "etag = %3ld DOCSIZE ulSize=%ld",
                        pht->GetTag(), pht->GetDocSize());
        }
        else if (pht->Is(ETAG_RAW_EOF))
        {
            PerfDbgLog1(tagToken2, pHtmPost, "etag = %3ld EOF", pht->GetTag());
        }
        else if (pht->Is(ETAG_SCRIPT) && !pht->IsEnd())
        {
            PerfDbgLog5(tagToken2, pHtmPost, "etag = %3ld <%lsSCRIPT%ls> ulLine=%ld ulOffset=%ld",
                        pht->GetTag(), pht->IsEnd() ? _T("/") : _T(""),
                        pht->IsEmpty() ? _T(" /") : _T(""), pht->GetLine(), pht->GetOffset());
        }
        else if (pht->GetTag() > ETAG_NULL && pht->GetTag() < ETAG_GENERIC)
        {
            PerfDbgLog4(tagToken2, pHtmPost, "etag = %3ld <%ls%ls%ls>",
                        pht->GetTag(), pht->IsEnd() ? _T("/") : _T(""),
                        NameFromEtag(pht->GetTag()), pht->IsEmpty() ? _T(" /") : _T(""));
        }
        else if (pht->Is(ETAG_RAW_BEGINSEL) || pht->Is(ETAG_RAW_ENDSEL))
        {
            PerfDbgLog2(tagToken2, pHtmPost, "etag = %3ld %ls",
                pht->GetTag(), pht->Is(ETAG_RAW_BEGINSEL) ? _T("BEGINSEL") : _T("ENDSEL"));
        }
        else
        {
            Assert(!pht->IsTiny());
            PerfDbgLog4(tagToken2, pHtmPost, "etag = %3ld <%ls%ls%ls>",
                        pht->GetTag(), pht->IsEnd() ? _T("/") : _T(""),
                        (!pht->IsTiny() && pht->GetPch()) ? pht->GetPch() : _T("(nil)"),
                        pht->IsEmpty() ? _T(" /") : _T(""));
        }

        CHtmTag::CAttr *pattr;
        int i = pht->GetAttrCount();

        if (i > 0)
        {
            for (pattr = pht->GetAttr(0); i; i--, pattr++)
            {
                PerfDbgLog2(tagToken2, pHtmPost, "       attr  %ls = %ls", pattr->_pchName, !pattr->_pchVal ? _T("NULL") : pattr->_pchVal);
            }
        }
    }
}

#else

#define TraceToken(post, pht)

#endif

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::Init
//
//  Synopsis:   Hooks up the post parser to the doc (for output),
//              post channel  (for input), and load ctx (for notification)
//
//-------------------------------------------------------------------------
HRESULT
CHtmPost::Init(CHtmLoad * pHtmLoad, CHtmTagStm * pHtmTagStm,
    CDoc * pDoc, CMarkup * pMarkup, HTMPASTEINFO * phpi, BOOL fSync)
{
    HRESULT hr;

    _dwFlags    = POSTF_ONETIME | (phpi ? POSTF_PASTING : 0);
    _pDoc       = pDoc;
    _pMarkup    = pMarkup;
    _pHtmLoad   = pHtmLoad;
    _pHtmTagStm = pHtmTagStm;
    _pMarkup    = pMarkup;
    _pNodeRoot  = _pMarkup->Root()->GetFirstBranch();
    _phpi       = phpi;

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    _dwIncludeDownloadCookie = NULL;
#endif

    _pDoc->SubAddRef();
    _pMarkup->SubAddRef();
    _pHtmLoad->SubAddRef();
    _pHtmTagStm->AddRef();

    _pHtmParse  = new CHtmParse;

    if (!_pHtmParse)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_pHtmParse->Init(_pDoc, _pMarkup, _pNodeRoot));
    if (hr)
        goto Cleanup;

    if (_dwFlags & POSTF_DIE)
    {
        hr = E_ABORT;
        goto Cleanup;
    }

    // hook up async notification if not parsing synchronously

    if (!fSync)
    {
        _pHtmTagStm->SetCallback(OnDwnChanCallback, this);
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::OnDwnChanCallback
//
//  Synopsis:   Callback when post stream has more data
//
//-------------------------------------------------------------------------
void CALLBACK
CHtmPost::OnDwnChanCallback(void * pvObj, void * pvArg)
{
    PerfDbgLog(tagPerfWatch, pvObj, "+CHtmPost::OnDwnChanCallback");
    PerfDbgLog(tagHtmPost, pvObj, "+CHtmPost::OnDwnChanCallback");

#ifdef SWITCHES_ENABLED
    if (    !IsSwitchSerialize()
        ||  ((CHtmPost *)pvArg)->_pHtmTagStm->IsEofWritten()
        ||  ((CHtmPost *)pvArg)->_pHtmLoad->_pHtmPre->IsSuspended())
        PostManResume((CHtmPost *)pvArg, TRUE);
#else
    PostManResume((CHtmPost *)pvArg, TRUE);
#endif

    PerfDbgLog(tagHtmPost, pvObj, "-CHtmPost::OnDwnChanCallback");
    PerfDbgLog(tagPerfWatch, pvObj, "-CHtmPost::OnDwnChanCallback");
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::RunNested
//
//  Synopsis:   Runs the parser for script, in a nested context.
//
//              Allowed to be called recursively (unlike ::Exec)
//
//-------------------------------------------------------------------------
HRESULT
CHtmPost::RunNested()
{
    PerfDbgLog(tagPerfWatch, this, "+CHtmPost::RunNested");

    DWORD   dwFlagsSav;
    HRESULT hr;

    // save nested state
    dwFlagsSav = (_dwFlags & (POSTF_RESUME_PREPARSER | POSTF_NESTED));

    // set initial nested state
    Assert(!(_dwFlags & POSTF_NEED_EXECUTE)); // does not need saving

    _dwFlags &= ~POSTF_RESUME_PREPARSER;
    _dwFlags |= POSTF_NESTED;

    // stop this post from running on message loop; and run to exhaustion
    hr = THR(Exec(INFINITE));

    // restore nested state
    _dwFlags = (_dwFlags & ~(POSTF_RESUME_PREPARSER | POSTF_NESTED)) | dwFlagsSav;

    PerfDbgLog(tagPerfWatch, this, "-CHtmPost::RunNested");

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     TagIsHiddenInput
//
//  Synopsis:   TRUE if tag is <INPUT TYPE=HIDDEN>
//
//-------------------------------------------------------------------------
BOOL
TagIsHiddenInput(CHtmTag * pht)
{
    TCHAR *pchType;
    long iType;

    if (pht->GetTag() != ETAG_INPUT)
        return FALSE;

    if (!pht->ValFromName(_T("type"), &pchType) ||
            FAILED(s_enumdeschtmlInput.EnumFromString(pchType, &iType, FALSE)))
    {
        iType = htmlInputText;
    }

    return (iType == htmlInputHidden);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::Exec
//
//  Synopsis:   Runs the parser
//
//-------------------------------------------------------------------------
HRESULT
CHtmPost::Exec(DWORD dwTimeout)
{
    PerfDbgLog(tagPerfWatch, this, "+CHtmPost::Exec");

    HRESULT hr = S_OK;

#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagAllTokens))
        dwTimeout = INFINITE;
#endif

#ifdef SWITCHES_ENABLED
    if (IsSwitchSerialize())
        dwTimeout = INFINITE;
#endif

    if (_dwFlags & (POSTF_SLEEP | POSTF_DIE | POSTF_ONETIME))
    {
        if (_dwFlags & (POSTF_SLEEP | POSTF_DIE))
        {
            // This can happen because CHtmPostTask can get awakened by a call
            // to OnChan which happened some time ago.  It will unlock and run
            // itself once and we'll end up getting called.  But we really don't
            // want to do anything, so we just return.  It will then call our
            // IsPending method and put the task back to sleep.
            goto Cleanup;
        }

        PerfDbgLog1(tagToken2, this, "Beginning CHtmPost processing of %ls ----------------------------------------",
                    _pDoc->_cstrUrl);

        hr = THR(_pHtmLoad->OnPostStart());
        if (hr)
            goto Cleanup;

        _dwFlags &= ~POSTF_ONETIME;
    }

    do
    {
#ifdef VSTUDIO7
        if (!(_dwFlags & (POSTF_NEED_EXECUTE | POSTF_RESUME_PREPARSER | POSTF_NEED_BASETAGS)))
#else
        if (!(_dwFlags & (POSTF_NEED_EXECUTE | POSTF_RESUME_PREPARSER)))
#endif //VSTUDIO7
        {
            // prepare parsers for a run of tokens

            hr = THR(Broadcast(CHtmParse::Prepare));
            if (hr)
                goto Cleanup;

            // read tokens until out of time or out of tokens (try at least once)

            if (!IsAtEof())
            {
                hr = THR(ProcessTokens(dwTimeout));
                if (hr)
                    goto Cleanup;
            }

            if (!IsAtEof() && !(_dwFlags & POSTF_RESTART))
            {
                // commit parsers

                hr = THR(Broadcast(CHtmParse::Commit));
                if (hr)
                    goto Cleanup;
            }
            else
            {
                // flush through EOF text token to create <BODY> etc
                CNotification   nf;

                CHtmTag ht;

                ht.Reset();
                ht.SetTag(ETAG_RAW_EOF);
                ht.SetPch(NULL);
                ht.SetCch(0);

                TraceToken(this, &ht);

                hr = THR(_pHtmParse->ParseToken(&ht));
                if (hr)
                    goto Cleanup;

                hr = THR(Broadcast(CHtmParse::Finish));
                if (hr)
                    goto Cleanup;

                // BUGBUG (EricVas) Note to DBau - work this into the parser scheme

                nf.EndParse(_pMarkup->Root());
                _pMarkup->Root()->Notify(&nf);

                // Grab the left and right selection pos'es
                if (_phpi)
                {
                    _pHtmParse->GetPointers(&(_phpi->ptpSelBegin), &(_phpi->ptpSelEnd));
                }

                if (!(_dwFlags & POSTF_RESTART))
                {
                    // Wait for inplace activation after processing EOF.  Pretend we need execution to
                    // keep IsDone from going TRUE while we wait.

                    _dwFlags |= POSTF_WAIT | POSTF_NEED_EXECUTE;
                }
            }
        }

        //
        // Block the parser if we need to (our execute will happen
        // after we unblock)
        //

        if (_dwFlags & POSTF_WAIT)
        {
            if (!(_dwFlags & POSTF_DONTWAITFORINPLACE))
            {
                _pDoc->_fNeedInPlaceActivation = TRUE;
            }

            if (!_pMarkup->AllowInlineExecution())
            {
                // This will put the htmpost to sleep until the script
                // can be executed.

                _pMarkup->WaitInlineExecution();
                if (_dwFlags & POSTF_SLEEP)
                    goto Cleanup;

                Assert(_pMarkup->AllowInlineExecution());
            }

            _dwFlags &= ~(POSTF_WAIT | POSTF_DONTWAITFORINPLACE);
        }

        if (_dwFlags & POSTF_NEED_EXECUTE)
        {
            //
            // The parser is now in a clean state, we can execute
            //

            _dwFlags &= ~POSTF_NEED_EXECUTE;

            // execute
            hr = THR(Broadcast(&CHtmParse::Execute));
            if (hr || (_dwFlags & (POSTF_SLEEP | POSTF_DIE)))
                goto Cleanup;
        }

#ifdef VSTUDIO7
        if (_dwFlags & POSTF_NEED_BASETAGS)
        {
            _dwFlags &= ~POSTF_NEED_BASETAGS;
            Assert(_dwFlags & POSTF_RESUME_PREPARSER);

            if (_pMarkup->IsInInline())
            {
                _pDoc->_fNeedBaseTags = TRUE;
                goto Cleanup;
            }
            else
            {
                hr = _pDoc->GetBaseTagsFromFactory();
            }
            if (hr)
                goto Cleanup;
        }
#endif //VSTUDIO7

        // If the preparser needs to be resumed, we must have exhausted all tokens
        Assert(!(_dwFlags & POSTF_RESUME_PREPARSER) || IsPending());

#if DBG==1 || defined(PERFTAGS)
        if (dwTimeout != INFINITE && IsPerfDbgEnabled(tagOneToken))
            break;
#endif

        if (GetTickCount() > dwTimeout)
            break;

        if (_dwFlags & POSTF_RESTART)
            break;

    } while (!IsPending() && !IsAtEof());


    // Check to see if we found an author palette (or if it's too late)
    if (!_pDoc->_fGotAuthorPalette)
    {
        TraceTag((tagPalette, "Checking for author palette in pDwnDoc %08x %d", _pDoc, _pDoc->_pDwnDoc->GotAuthorPalette()));
        if (_pDoc->_pDwnDoc->GotAuthorPalette())
        {
            _pDoc->_fGotAuthorPalette = TRUE;
            _pDoc->InvalidateColors();
        }
    }

    // resume the preparser after recursive invokation of scripts

    if (_dwFlags & POSTF_RESUME_PREPARSER)
    {
        _dwFlags &= ~POSTF_RESUME_PREPARSER;
        _pHtmLoad->OnPostFinishScript();
    }

    // If the parse needs to be restarted do it now.

    if (_dwFlags & POSTF_RESTART)
    {
        hr = THR(_pHtmLoad->OnPostRestart(_cpRestart));
        goto Cleanup;
    }

Cleanup:
    PerfDbgLog(tagPerfWatch, this, "-CHtmPost::Exec");
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::Broadcast
//
//  Synopsis:   Prepares each context for possible parsing
//
//-------------------------------------------------------------------------
HRESULT CHtmPost::Broadcast(HRESULT (BUGCALL CHtmParse::*pfnAction)())
{
    HRESULT hr;

    hr = THR(CALL_METHOD(_pHtmParse, pfnAction, ()));
    if (hr)
    {
        _dwFlags |= POSTF_DIE;
        goto Cleanup;
    }

    if (_dwFlags & POSTF_DIE)
    {
        hr = E_ABORT;
        goto Cleanup;
    }

    if (_pHtmParse->NeedExecute())
    {
        _dwFlags |= POSTF_NEED_EXECUTE;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::IsPending
//
//  Synopsis:   TRUE if blocked, but expecting to become unblocked
//
//-------------------------------------------------------------------------
BOOL CHtmPost::IsPending()
{
    BOOL fDataPend = _pHtmTagStm->IsPending();
    BOOL fPostPend = (_dwFlags & POSTF_SLEEP) || fDataPend;

    PerfDbgLog3(tagHtmPost, this, "CHtmPost::IsPending "
        "(fSleep: %s, fDataPend: %s) --> %s",
        (_dwFlags & POSTF_SLEEP) ? "T" : "F", fDataPend ? "T" : "F",
        fPostPend ? "TRUE" : "FALSE");

    return(fPostPend);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::IsAtEof
//
//  Synopsis:   TRUE if there will never be any more data to process.
//
//              Always false when inside RunNested (EOF must only be
//              processed at the top level).
//
//-------------------------------------------------------------------------
BOOL CHtmPost::IsAtEof()
{
    BOOL fDataEof = _pHtmTagStm->IsEof();
    BOOL fPostEof = !(_dwFlags & POSTF_NESTED) && ((_dwFlags & POSTF_STOP) || fDataEof);

    PerfDbgLog3(tagHtmPost, this, "CHtmPost::IsAtEof "
        "(fDataEof: %s, fStop: %s) --> %s",
        fDataEof ? "T" : "F", (_dwFlags & POSTF_STOP) ? "T" : "F",
        fPostEof ? "TRUE" : "FALSE");

    return(fPostEof);
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::IsDone
//
//  Synopsis:   TRUE if done.
//
//-------------------------------------------------------------------------
BOOL CHtmPost::IsDone()
{
    BOOL fAtEof = IsAtEof();
    BOOL fPostDone = fAtEof && !(_dwFlags & (POSTF_NEED_EXECUTE | POSTF_RESUME_PREPARSER));

    PerfDbgLog4(tagHtmPost, this, "CHtmPost::IsDone "
        "(IsAtEof: %s, fNeedExecute: %s, fResumePreparser: %s) --> %s",
        fAtEof ? "T" : "F", (_dwFlags & POSTF_NEED_EXECUTE) ? "T" : "F",
        (_dwFlags & POSTF_RESUME_PREPARSER) ? "T" : "F",
        fPostDone ? "TRUE" : "FALSE");

    return(fPostDone);
}

//+------------------------------------------------------------------------
//
//  Function:   IsDiscardableFramesetTag
//
//  Synopsis:   Determines if pht is a <FRAMESET> tag that NS 3.0
//              discards.
//
//-------------------------------------------------------------------------
BOOL IsDiscardableFramesetTag(CHtmTag *pht)
{
    TCHAR *pchR;
    TCHAR *pchC;
    return ((pht->IsBegin(ETAG_FRAMESET) &&
        (!pht->ValFromName(_T("ROWS"), &pchR) || !pchR || !_tcschr(pchR, _T(','))) &&
        (!pht->ValFromName(_T("COLS"), &pchC) || !pchC || ! _tcschr(pchC, _T(','))))
        ||
        pht->Is(ETAG_RAW_EOF));
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::ProcessTokens
//
//  Synopsis:   Rearranges tokens and sends them on to the head or body
//              parsers.
//
//              Contains the logic for sorting out the top-level structure
//              of a document: <!DOCTYPE>, <HTML>, <HEAD>, <BODY>, <FRAMESET>
//
//-------------------------------------------------------------------------
HRESULT CHtmPost::ProcessTokens(DWORD dwTimeout)
{
    CHtmTag *   pht = NULL;
    CHtmTag *   phtSlowProcess;
    CHtmTag     htSlowProcess;
    DWORD       dwFlags;
    HRESULT     hr = S_OK;

#if DBG==1 || defined(PERFTAGS)
    TCHAR *     pchVMem = NULL;
#endif

NextToken:

    pht = _pHtmTagStm->ReadTag(pht);
    if (pht == NULL)
        goto Cleanup;

#if DBG==1 || defined(PERFTAGS)
    if (    IsPerfDbgEnabled(tagStrictPch)
        &&  (pht->Is(ETAG_RAW_TEXT) || pht->Is(ETAG_RAW_COMMENT)))
    {
        if (pht->GetCch() == 0)
            pht->SetPch(NULL);
        else
        {
            VMemFree(pchVMem);
            pchVMem = (TCHAR *)VMemAlloc(pht->GetCch() * sizeof(TCHAR), VMEM_BACKSIDESTRICT);

            if (pchVMem)
            {
                memcpy(pchVMem, pht->GetPch(), pht->GetCch() * sizeof(TCHAR));
                pht->SetPch(pchVMem);
            }
        }
    }
#endif

    dwFlags = TagDescFromEtag(pht->GetTag())->_dwTagDescFlags;

    if (dwFlags & TAGDESC_SLOWPROCESS)
    {
        goto SlowProcess;
    }

    TraceToken(this, pht);

    hr = THR(_pHtmParse->ParseToken(pht));
    if (hr)
        goto Cleanup;

ContinueProcess:

    if (_pHtmParse->NeedExecute())
    {
        _dwFlags |= POSTF_NEED_EXECUTE;
    }

    if (dwFlags & (TAGDESC_WAITATSTART|TAGDESC_WAITATEND))
    {
        if (pht->IsEnd())
        {
            if (dwFlags & TAGDESC_WAITATEND)
            {
                _dwFlags |= POSTF_WAIT;
            }

            if (pht->Is(ETAG_SCRIPT) && !pht->IsDefer())
            {
                _dwFlags |= POSTF_RESUME_PREPARSER;
            }
        }
        else if (dwFlags & TAGDESC_WAITATSTART)
        {
            _dwFlags |= POSTF_WAIT;
        }

        if ((_dwFlags & POSTF_WAIT) && (dwFlags & TAGDESC_DONTWAITFORINPLACE))
        {
            _dwFlags |= POSTF_DONTWAITFORINPLACE;
        }
    }

    if (_dwFlags & (POSTF_NEED_EXECUTE | POSTF_RESUME_PREPARSER | POSTF_STOP | POSTF_WAIT))
        goto Cleanup;

    if (GetTickCount() > dwTimeout)
        goto Cleanup;

#if DBG==1 || defined(PERFTAGS)
    if (dwTimeout != INFINITE && IsPerfDbgEnabled(tagOneToken))
        goto Cleanup;
#endif

    goto NextToken;

SlowProcess:
    phtSlowProcess = pht;

    // Process a codepage token, which can come anywhere
    if (pht->Is(ETAG_RAW_CODEPAGE))
    {
        TraceToken(this, pht);

        if (MlangValidateCodePage(_pDoc, pht->GetCodepage(), NULL, FALSE) != S_OK)
        {
            // put original codepage back but we can no longer abort restarting here
            // if the original codepage was CP_AUTO, we have to use something guaranteed
            // to be valid on the system otherwise we'll get invoked again because of
            // detection.
            if (_pDoc->GetCodePage() == CP_AUTO)
                pht->SetCodepage(g_cpDefault);
            else
                pht->SetCodepage(_pDoc->GetCodePage());
            pht->SetRestart();
        }

        if (pht->IsRestart())
        {
            _dwFlags |= POSTF_RESTART;
            _cpRestart = pht->GetCodepage();
        }
        else if (_pMarkup == _pDoc->_pPrimaryMarkup)
        {   // Switch codepage only for primary markup

            hr = THR(_pDoc->SwitchCodePage(pht->GetCodepage()));
            if (hr)
                goto Cleanup;

            if (    !IsAutodetectCodePage(_pDoc->GetCodePage())
                ||  WindowsCodePageFromCodePage(pht->GetCodepage()) != CP_JPN_SJ)
            {
                // Make sure the ParseCtx is in a sane state
                // before EnsureFormatCacheChange
                hr = THR(Broadcast(CHtmParse::Commit));
                if (hr)
                    goto Cleanup;

                // Do not force a relayout if we switched from Japanese
                // autodetect to a Japanese family codepage.
                _pDoc->EnsureFormatCacheChange(
                    ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASURECONTENTS);

                hr = THR(Broadcast(CHtmParse::Prepare));
                if (hr)
                    goto Cleanup;
            }
        }
        goto Cleanup;
    }

    // Process a docsize token, which can come anywhere
    if (pht->Is(ETAG_RAW_DOCSIZE))
    {
        if (!(_dwFlags & POSTF_PASTING))
        {
            CMarkupScriptContext *  pScriptContext = _pMarkup->ScriptContext();

            TraceToken(this, pht);

            if (pScriptContext && pScriptContext->_pScriptDebugDocument)
            {
                IGNORE_HR(pScriptContext->_pScriptDebugDocument->SetDocumentSize(pht->GetDocSize()));
            }
        }
        goto Cleanup;
    }

#ifdef VSTUDIO7
    if (pht->Is(ETAG_RAW_FACTORY))
    {
        hr = ProcessFactoryPI(pht);
        goto ContinueProcess;
    }

    if (pht->Is(ETAG_RAW_TAGSOURCE))
    {
        // All the processing happened in the pre-parser.
        goto ContinueProcess;
    }
#endif //VSTUDIO7

    // if we still think this unknown tag, check if it is actually a generic tag or XML PI
    if (pht->Is(ETAG_UNKNOWN))
    {
        if (IsXmlPI(pht))
        {
            RegisterXmlPI (pht);

            goto Cleanup; // do not create any elements for XML_DECL
        }

        pht->SetTag(IsGenericElement(pht));
    }

    if (pht->Is(ETAG_HTML))
    {
        RegisterHtmlTagNamespaces(pht);
    }

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    if (pht->Is(ETAG_RAW_INCLUDE))
    {

        // so that people don't get the expectation that they can paste one of these
        // constructs into the middle of HTML and get it to expand;
        // instead, we should abort the paste operation

        if(_dwFlags & POSTF_PASTING)
        {
            hr = E_ABORT;
            goto Cleanup;
        }

        ProcessIncludeToken(pht);

        _dwFlags |= POSTF_RESUME_PREPARSER;

        goto ContinueProcess;
    }
#endif

    // BUGBUG (48236): some attributes on the body and frameset get delegated to the doc.  During
    // paste operations, we don't want these to clobber what the main doc already has.
    if (    _dwFlags & POSTF_PASTING
        &&  (   pht->IsBegin(ETAG_BODY)
            ||  pht->IsBegin(ETAG_FRAMESET)))
    {
        // Initialize htSlowProcess to empty tag
        phtSlowProcess = &htSlowProcess;
        htSlowProcess.Reset();
        htSlowProcess.SetTag( pht->GetTag() );
    }

    hr = THR(ParseToken(phtSlowProcess));
    if (hr)
        goto Cleanup;

    if (dwFlags & TAGDESC_ENTER_TREE_IMMEDIATELY)
    {
        hr = THR(Broadcast(CHtmParse::Commit));
        if (hr)
            goto Cleanup;

        hr = THR(Broadcast(CHtmParse::Prepare));
        if (hr)
            goto Cleanup;
    }

    goto ContinueProcess;

Cleanup:
#if DBG==1 || defined(PERFTAGS)
    VMemFree(pchVMem);
#endif
    RRETURN(hr);
}

#ifdef CLIENT_SIDE_INCLUDE_FEATURE

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::ProcessIncludeToken
//
//  Synopsis:    Starts download of referenced url in include token
//
//-------------------------------------------------------------------------
HRESULT
CHtmPost::ProcessIncludeToken(CHtmTag *pht)
{
    Assert(pht->Is(ETAG_RAW_INCLUDE));

    HRESULT    hr;
    CBitsCtx *      pBitsCtx = NULL;
    TCHAR *pchUrl = NULL;
    TCHAR *pchData;
    ULONG ulCount;
    ULONG ulUrlSize;

    ULONG pos1, pos4;
    TCHAR *pchnonwhitespace = NULL, *pchbeg = NULL, *pchend = NULL, *pchInfo = NULL;
    TCHAR *pchInclude = _T("include");
    int   cchInclude = _tcslen(pchInclude);
    TCHAR chQuote = _T('\0');

    CElement *pCurrentElement = NULL;

    pchData = pht->GetPch();
    ulCount = pht->GetCch();

    //
    // skip passed the "include" string then begin parsing
    //

    for (pos4 = 0; pos4 < ulCount - cchInclude; pos4++)
    {
        if (_tcsnicmp(pchData + pos4, cchInclude, pchInclude, cchInclude) == 0)
        {
            pchInfo = pchData + pos4 + cchInclude;
            ulCount = ulCount - pos4 - cchInclude;
            break;
        }
    }

    // if "include" string not found, something tragic has occured so exit

    if (!pchInfo)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Back up within the ']>' to delimit parsing
    while (ulCount && pchInfo[ulCount - 1] != _T(']'))
        ulCount -= 1;

    // if "]" string not found, something tragic has occured so exit

    if (ulCount <= 1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // chop off ']'

    ulCount -= 1;


    // get the fist nonwhitespace character

    for (pos1 = 0; pos1 < ulCount; pos1++)
    {
        TCHAR *pchcurrent = pchInfo + pos1;

        if (!ISSPACE(*pchcurrent))
        {
            pchnonwhitespace = pchcurrent;
            break;
        }
    }

    // if no non-whitespace chars encountered we must have run off
    // the end, so leave

    if (!pchnonwhitespace)
        goto Cleanup;

    // try to match quotes etc

    if (ISQUOTE(*pchnonwhitespace))
    {
        chQuote = *pchnonwhitespace;
        pchbeg = pchnonwhitespace + 1;
        for (pchend = pchbeg; (unsigned)(pchend - pchInfo) < ulCount; pchend += 1)
        {
            if (*pchend == chQuote)
                break;
        }
    }
    else
    {
        pchbeg = pchnonwhitespace;
        for (pchend = pchbeg; (unsigned)(pchend - pchInfo) < ulCount; pchend += 1)
        {
            if (ISSPACE(*pchend))
                break;
        }
    }

    // create a new string and copy characters from the established
    // url beginning and end

    ulUrlSize = pchend - pchbeg;

    if (!ulUrlSize)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Limit number of includes

    #define MAX_INCLUDES 1024

    if (_ulIncludes >= MAX_INCLUDES)
        goto Cleanup;

    _ulIncludes += 1;

    hr = THR(MemAllocStringBuffer(Mt(CHtmPost_ProcessIncludeToken_pchUrl), ulUrlSize, pchbeg, &pchUrl));
    if (hr)
        goto Cleanup;

    ProcessValueEntities(pchUrl, &ulUrlSize);

    pCurrentElement = _pHtmParse->GetCurrentElement();

    // Security check: access allowed?
    {
        TCHAR   cBuf[pdlUrlLen];
        hr = THR(_pDoc->ExpandUrl(pchUrl, ARRAY_SIZE(cBuf), cBuf, pCurrentElement));
        if (hr)
            goto Cleanup;

        // Access not allowed: do nothing
        if (!_pDoc->AccessAllowed(cBuf))
            goto Cleanup;
    }

    hr = THR(_pDoc->NewDwnCtx(DWNCTX_BITS, pchUrl, pCurrentElement, (CDwnCtx **) &pBitsCtx, FALSE, PROGSINK_CLASS_CONTROL));
    if (hr)
        goto Cleanup;

    if (_pBitsCtxInclude)
        _pBitsCtxInclude->Release();

    pBitsCtx->AddRef();
    _pBitsCtxInclude = pBitsCtx;

    _pMarkup->EnterScriptDownload(&_dwIncludeDownloadCookie);

    if (pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
    {
        OnIncludeDwnChan(pBitsCtx);
    }
    else
    {
        pBitsCtx->SetProgSink(_pDoc->GetProgSink());
        pBitsCtx->SetCallback(OnIncludeDwnChanCallback, this);
        pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
    }

Cleanup:

    if (pBitsCtx)
        pBitsCtx->Release();

    if (pchUrl)
        delete pchUrl;

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::OnIncludeDwnChanCallback
//
//  Synopsis:   Callback when post stream for !INCLUDE token has more data
//
//-------------------------------------------------------------------------
void
CHtmPost::OnIncludeDwnChan(CBitsCtx * pBitsCtx)
{
    PerfDbgLog(tagPerfWatch, pBitsCtx, "+CHtmPost::OnIncludeDwnChanCallback");
    PerfDbgLog(tagHtmPost, pBitsCtx, "+CHtmPost::OnIncludeDwnChanCallback");

    HRESULT         hr;
    ULONG           ulState;
    char *          pbBuffer = NULL;
    IStream *       pStream = NULL;
    STATSTG         statstg;
    ULONG           cbLen;
    ULONG           cchLen;
    BOOL            fEndCR;
    ULONG           cbRead;
    TCHAR *         pchEnd;
    TCHAR *         pchUrl;
    TCHAR *         pchSrcCode = NULL;

    if (pBitsCtx != _pBitsCtxInclude)
        return;

    ulState = pBitsCtx->GetState();

    hr = THR(_pMarkup->EnterInline());
    if (hr)
        goto Cleanup;

    if (ulState & DWNLOAD_COMPLETE)
    {
        pchUrl = (LPTSTR) pBitsCtx->GetUrl();

        // If unsecure download, may need to remove lock icon on Doc
        _pDoc->OnSubDownloadSecFlags(pchUrl, pBitsCtx->GetSecFlags());

        // if load completed OK, load file and convert to unicode

        hr = THR(pBitsCtx->GetStream(&pStream));
        if (hr)
            goto Cleanup;

        hr = THR(pStream->Stat(&statstg, STATFLAG_NONAME));
        if (hr)
            goto Cleanup;

        cbLen = statstg.cbSize.LowPart;
        if (statstg.cbSize.HighPart || cbLen == 0xFFFFFFFF)
            goto Cleanup;

        pbBuffer = (char *)MemAlloc(Mt(CHtmPost_OnIncludeDwnChan_pbBuffer), cbLen);
        if (!pbBuffer)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pStream->Read(pbBuffer, cbLen, &cbRead));

        if (hr == S_FALSE)
            hr = S_OK;
        else if (hr || cbRead != cbLen)
            goto Cleanup;

        cchLen = MultiByteToWideChar(CP_ACP, 0, pbBuffer, cbLen, NULL, 0);

        pchSrcCode = (TCHAR *)MemAlloc(Mt(CHtmPost_OnIncludeDwnChan_pchSrcCode), (cchLen+1) * sizeof(TCHAR));
        if (!pchSrcCode)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        Verify(cchLen == (unsigned)MultiByteToWideChar(CP_ACP, 0, pbBuffer, cbLen, pchSrcCode, cchLen));

        fEndCR = FALSE;
        pchEnd = pchSrcCode + cchLen;
        cchLen -= NormalizerChar(pchSrcCode, &pchEnd);
        pchSrcCode[cchLen] = _T('\0');

        hr = THR(_pDoc->HtmCtx()->Write(pchSrcCode, FALSE));
        if (hr)
            goto Cleanup;
    }

    hr = THR(_pMarkup->LeaveInline());
    if (hr)
        goto Cleanup;

Cleanup:

    // even if download failed, tell the cdoc that it's done
    if (ulState & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
    {
        if (_dwIncludeDownloadCookie)
        {
            _pMarkup->LeaveScriptDownload(&_dwIncludeDownloadCookie);
            _dwIncludeDownloadCookie = NULL;
        }

        pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
        _pBitsCtxInclude->Release();
        _pBitsCtxInclude = NULL;
    }

    MemFree(pbBuffer);
    MemFree(pchSrcCode);

    ReleaseInterface(pStream);

    PerfDbgLog(tagHtmPost, pBitsCtx, "-CHtmPost::OnIncludeDwnChanCallback");
    PerfDbgLog(tagPerfWatch, pBitsCtx, "-CHtmPost::OnIncludeDwnChanCallback");

}

#endif // CLIENT_SIDE_INCLUDE_FEATURE



#ifdef VSTUDIO7
//+------------------------------------------------------------------------
//
//  Member:     CHtmPost::ProcessFactoryPI
//
//  Synopsis:   Process FACTORY
//
//-------------------------------------------------------------------------
HRESULT
CHtmPost::ProcessFactoryPI(CHtmTag *pht)
{
    LPTSTR                   pchCodebase = NULL;

    Assert(_pDoc && _pDoc->HtmCtx());

    if (!pht->ValFromName(_T("codebase"), &pchCodebase))
        goto Cleanup;

    _pDoc->EnsureIdentityFactory(pchCodebase);
    _dwFlags |= (POSTF_NEED_BASETAGS | POSTF_RESUME_PREPARSER);

Cleanup:
    RRETURN(S_OK);
}
#endif //VSTUDIO7

//-------------------------------------------------------------------------
//
//  Member:     CHtmPost::IsIndexHack
//
//  Synopsis:   For Netscape as well as pre-HTML 4.0 specs, we change
//              the <ISINDEX> tag into a mini-form by creating and
//              pushing the appropriate tags.
//
//-------------------------------------------------------------------------
HRESULT
CHtmPost::IsIndexHack(CHtmTag *pht)
{
    CHtmTag ht;
    CHtmTag * phtInput = NULL;
    CHtmTag::CAttr *pAttr;
    TCHAR ach[100];
    UINT c;
    HRESULT hr;

    ht.Reset();
    ht.SetTag(ETAG_FORM);

    pAttr = pht->AttrFromName(_T("action"));
    if (pAttr)
    {
        ht.SetAttrCount(1);
        *ht.GetAttr(0) = *pAttr;
    }

    hr = THR(ParseToken(&ht));
    if (hr)
        goto Cleanup;

    ht.Reset();
    ht.SetTag(ETAG_HR);

    hr = THR(ParseToken(&ht));
    if (hr)
        goto Cleanup;

    // add text "prompt"
    ht.Reset();
    ht.SetTag(ETAG_RAW_TEXT);

    pAttr = pht->AttrFromName(_T("prompt"));
    if (pAttr)
    {
        ht.SetPch(pAttr->_pchVal);
        ht.SetCch(pAttr->_cchVal);
    }
    else // attribute not found so add a default prompt from IE3
    {
        ht.SetPch(ach);
        ht.SetCch(LoadString(GetResourceHInst(), IDS_DEFAULT_ISINDEX_PROMPT, ach, ARRAY_SIZE(ach) - 1));
        // if it fails we won't display anything.
    }

    if (ht.GetCch())
    {
        hr = THR(ParseToken(&ht));
        if (hr)
            goto Cleanup;
    }

    phtInput = (CHtmTag *)MemAlloc(Mt(CHtmPost_IsIndexHack_phtInput), CHtmTag::ComputeSize(FALSE, pht->GetAttrCount() + 1));
    if (phtInput == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    phtInput->Reset();
    phtInput->SetTag(ETAG_INPUT);
    phtInput->SetAttrCount(1);
    pAttr = phtInput->GetAttr(0);
    pAttr->_pchName = _T("name");
    pAttr->_cchName = 4;
    pAttr->_pchVal = _T("isindex");
    pAttr->_cchVal = 7;
    pAttr->_ulLine = 0;
    pAttr->_ulOffset = 0;

    if (pht->GetAttrCount())
    {
        for (pAttr = pht->GetAttr(0), c = pht->GetAttrCount(); c; pAttr++, c--)
        {
            // Everything that is not an "action" or a "prompt" gets applied
            // to the input tag.
            if ( _tcsicmp(pAttr->_pchName, _T("action")) &&
                 _tcsicmp(pAttr->_pchName, _T("prompt")) )
            {
                phtInput->SetAttrCount(phtInput->GetAttrCount() + 1);
                *phtInput->GetAttr(phtInput->GetAttrCount() - 1) = *pAttr;
            }
        }
    }

    hr = THR(ParseToken(phtInput));
    if (hr)
        goto Cleanup;

    ht.Reset();
    ht.SetTag(ETAG_HR);

    hr = THR(ParseToken(&ht));
    if (hr)
        goto Cleanup;

    ht.Reset();
    ht.SetTag(ETAG_FORM);
    ht.SetEnd();

    hr = THR(ParseToken(&ht));
    if (hr)
        goto Cleanup;

Cleanup:
    MemFree(phtInput);
    RRETURN(hr);
}

// CHtmPost -------------------------------------------------------------------

void
CHtmPost::Run(DWORD dwTimeout)
{
    PerfDbgLog(tagPerfWatch, this, "+CHtmPost::Run");
    PerfDbgLog1(tagHtmPost, this, "+CHtmPost::Run %ls", GetUrl());

    SwitchesBegTimer(SWITCHES_TIMER_PARSER);

    HRESULT hr;

    for (;;)
    {
        hr = THR(Exec(dwTimeout));
        if (hr)
            goto Error;

        if (IsDone())
        {
            _pHtmLoad->OnPostDone(S_OK);
            break;
        }

        if (IsPending())
        {
            // Give the tokenizer a chance to run if it wants to.  We may end up
            // not having to suspend after all.

            Sleep(0);

            if (IsPending())
            {
                PerfDbgLog(tagHtmPost, this, "CHtmPost::Run (blocking)");
                PostManSuspend(this);
                break;
            }
        }

        if (GetTickCount() > dwTimeout)
            break;

#if DBG==1 || defined(PERFTAGS)
        if (IsPerfDbgEnabled(tagOneToken))
            break;
#endif
    }

Cleanup:

    SwitchesEndTimer(SWITCHES_TIMER_PARSER);

    PerfDbgLog(tagHtmPost, this, "-CHtmPost::Run");
    PerfDbgLog(tagPerfWatch, this, "-CHtmPost::Run");
    return;

Error:
    _pHtmLoad->OnPostDone(hr);
    goto Cleanup;
}

void CHtmPost::Passivate()
{
    PerfDbgLog1(tagHtmPost, this, "+CHtmPost::Passivate %ls", GetUrl());

    PostManDequeue(this);

    if (_pHtmLoad)
    {
        _pHtmLoad->SubRelease();
        _pHtmLoad = NULL;
    }

    if (_pHtmTagStm)
    {
        _pHtmTagStm->Disconnect();
        _pHtmTagStm->Release();
        _pHtmTagStm = NULL;
    }

    if (_pDoc)
    {
        _pDoc->SubRelease();
        _pDoc = NULL;
    }

    if (_pMarkup)
    {
        _pMarkup->SubRelease();
        _pMarkup = NULL;
    }

    if (_pHtmParse)
    {
        delete _pHtmParse;
        _pHtmParse = NULL;
    }

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
    if (_pBitsCtxInclude)
    {
        _pBitsCtxInclude->Release();
        _pBitsCtxInclude = NULL;
    }
#endif

    if (_pchError)
    {
        delete _pchError;
        _pchError = NULL;
    }

    PerfDbgLog(tagHtmPost, this, "-CHtmPost::Passivate");
}

void CHtmPost::Die()
{
    Verify(!(Broadcast(&CHtmParse::Die)));
    _dwFlags |= POSTF_DIE;
}

void CHtmPost::DoStop()
{
    _dwFlags |= POSTF_STOP;
    PostManResume(this, FALSE);
}

HRESULT CHtmPost::ParseToken(CHtmTag * pht)
{
    BOOL fAllowed;
    HRESULT hr;

    TraceToken(this, pht);

    switch (pht->GetTag())
    {
    case ETAG_NOEMBED:
        hr = THR(_pDoc->ProcessURLAction(URLACTION_SCRIPT_RUN, &fAllowed, 0, NULL, NULL, NULL, 0, TRUE));
        if (hr)
            goto Cleanup;

        if (!fAllowed)
            pht->SetTag(ETAG_NOEMBED_OFF);
        break;

    case ETAG_NOSCRIPT:
        hr = THR(_pDoc->ProcessURLAction(URLACTION_SCRIPT_RUN, &fAllowed, 0, NULL, NULL, NULL, 0, TRUE));
        if (hr)
            goto Cleanup;

        if (!fAllowed)
            pht->SetTag(ETAG_NOSCRIPT_OFF);
        break;

    // Next two cases (ETAG_FORM & ETAG_ISINDEX) are the hack to get ISINDEX
    // to work correctly.
    case ETAG_FORM:
        if (pht->IsEnd())
            _dwFlags &= ~POSTF_IN_FORM;
        else
            _dwFlags |=  POSTF_IN_FORM;
        break;

    case ETAG_ISINDEX:
        // ISINDEX cannot live inside a form and should be ignored.
        if (!pht->IsEnd() && !(_dwFlags & POSTF_IN_FORM))
        {
            hr = THR(IsIndexHack(pht));
            goto Cleanup;
        }
        break;

    case ETAG_FRAMESET:

        // Drop begin <FRAMESET> tags with no ROWS, COLS

        if (!pht->IsEnd())
        {
            // But don't drop the first one seen (partial IE3 compat)

            if (!(_dwFlags & POSTF_SEENFRAMESET))
            {
                _dwFlags |= POSTF_SEENFRAMESET;
            }
            else
            {
                if (IsDiscardableFramesetTag(pht))
                {
                    hr = S_OK;
                    goto Cleanup;
                }
            }
        }
    }

    hr = THR(_pHtmParse->ParseToken(pht));
    if (hr)
        goto Cleanup;

    if (_pHtmParse->NeedExecute())
    {
        _dwFlags |= POSTF_NEED_EXECUTE;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:   CHtmPost::RegisterXmlPI
//
//-------------------------------------------------------------------------

void
CHtmPost::RegisterXmlPI(CHtmTag * pht)
{
    LPTSTR pchNamespace;
    LPTSTR pchUrn;

    if (!pht->IsEnd())
    {
        pht->ValFromName(_T("PREFIX"), &pchNamespace);
        if (pchNamespace && pchNamespace[0])
        {
            pht->ValFromName(_T("NS"), &pchUrn);

            _pMarkup->RegisterXmlNamespace(pchNamespace, pchUrn, XMLNAMESPACETYPE_TAG);
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:   CHtmPost::RegisterHtmlTagNamespaces
//
//-------------------------------------------------------------------------

void
CHtmPost::RegisterHtmlTagNamespaces(CHtmTag * pht)
{
    HRESULT             hr;
    int                 idx;
    LPTSTR              pchNamespace;
    CHtmTag::CAttr *    pAttr;

    for (idx = 0;; idx++)
    {
        pchNamespace = pht->GetXmlNamespace(&idx);
        if (!pchNamespace)
            break;

        pAttr = pht->GetAttr(idx);

        hr = THR(_pMarkup->RegisterXmlNamespace(pchNamespace, pAttr->_pchVal, XMLNAMESPACETYPE_ATTR));
        if (hr)
            goto Cleanup;

        pAttr->_pchName = NULL;
        pAttr->_cchName = 0;
    }

Cleanup:
    return;
}

//+------------------------------------------------------------------------
//
//  Member:   CHtmPost::IsXmlPI
//
//-------------------------------------------------------------------------

BOOL
CHtmPost::IsXmlPI(CHtmTag * pht)
{
    LPTSTR pchName = pht->GetPch();

    if (_T('?') == pchName[0]) // skip possible '?' mark
        pchName++;

    if (0 == StrCmpIC(XML_PI, pchName))
    {
        return TRUE;
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:   CHtmPost::IsGenericElement
//
//-------------------------------------------------------------------------

ELEMENT_TAG
CHtmPost::IsGenericElement(CHtmTag * pht)
{
    return _pMarkup->IsGenericElement(/* pchName = */ pht->GetPch(), /* fAllSprinklesGeneric = */ FALSE);
}

// PostMan --------------------------------------------------------------------

void CALLBACK
PostManOnTimer(HWND hwnd, UINT umsg, UINT_PTR idevent, DWORD dwTime)
{
    PerfDbgLog(tagPerfWatch, NULL, "+PostManOnTimer");
    PerfDbgLog(tagHtmPost, 0, "+PostManOnTimer");

    THREADSTATE *   pts         = GetThreadState();
    DWORD           dwTick      = GetTickCount();
    DWORD           dwTimeout   = dwTick + POSTMAN_TIMESLICE;
    CHtmPost *      pHtmPost;

    #if DBG==1
    PostManValidate();
    #endif

    while (pts->post.cRunable)
    {
        pHtmPost = NULL;

        for (;;)
        {
            if (pts->post.pHtmPostNext == NULL)
                pts->post.pHtmPostNext = pts->post.pHtmPostHead;

            pHtmPost = pts->post.pHtmPostNext;
            pts->post.pHtmPostNext = pHtmPost->_pHtmPostNext;

            if (!(pHtmPost->_dwFlags & (POSTF_BLOCKED | POSTF_RUNNING)))
                break;
        }

        Assert(pHtmPost);

        PostManExecute(pts, dwTimeout, pHtmPost);

        if (GetTickCount() - dwTick > POSTMAN_TIMESLICE)
            break;

#if DBG==1 || defined(PERFTAGS)
        if (IsPerfDbgEnabled(tagParseSlowly))
            break;
#endif
    }

    #if DBG==1
    PostManValidate();
    #endif

    PerfDbgLog(tagHtmPost, 0, "-PostManOnTimer");
    PerfDbgLog(tagPerfWatch, NULL, "-PostManOnTimer");
}

void
PostManSetTimer(THREADSTATE * pts)
{
    if (!!pts->post.fTimer != (!!pts->post.cRunable && !pts->post.cLock))
    {
        if (pts->post.fTimer)
        {
            PerfDbgLog(tagHtmPost, 0, "PostManSetTimer KillTimer");
            PerfDbgLog(tagPerfWatch, 0, "PostManSetTimer KillTimer");
            KillTimer(pts->gwnd.hwndGlobalWindow, TIMER_POSTMAN);
            pts->post.fTimer = FALSE;
        }
        else
        {
            PerfDbgLog(tagHtmPost, 0, "PostManSetTimer SetTimer");
            PerfDbgLog(tagPerfWatch, 0, "PostManSetTimer SetTimer");
            SetTimer(pts->gwnd.hwndGlobalWindow, TIMER_POSTMAN,
                #if DBG==1 || defined(PERFTAGS)
                    IsPerfDbgEnabled(tagParseSlowly) ? 10 : 0,
                #else
                    0,
                #endif
                PostManOnTimer);
            pts->post.fTimer = TRUE;
        }

        #if DBG==1
        PostManValidate();
        #endif
    }
}

void
PostManExecute(THREADSTATE * pts, DWORD dwTimeout, CHtmPost * pHtmPost)
{
    PerfDbgLog(tagPerfWatch, pHtmPost, "+PostManExecute");
    PerfDbgLog(tagHtmPost, pHtmPost, "+PostManExecute");

    Assert(  pHtmPost->_dwFlags & POSTF_ENQUEUED);
    Assert(!(pHtmPost->_dwFlags & POSTF_BLOCKED));
    Assert(!(pHtmPost->_dwFlags & POSTF_RUNNING));

    pHtmPost->AddRef();
    pHtmPost->_dwFlags |= POSTF_RUNNING;
    pts->post.cRunable -= 1;
    pts->post.cRunning += 1;

    PostManSetTimer(pts);

    #if DBG==1
    PostManValidate();
    #endif

    pHtmPost->Run(dwTimeout);

    if (pHtmPost->_dwFlags & POSTF_RUNNING)
    {
        Assert(pHtmPost->_dwFlags & POSTF_ENQUEUED);

        pHtmPost->_dwFlags &= ~POSTF_RUNNING;
        pts->post.cRunable += !(pHtmPost->_dwFlags & POSTF_BLOCKED);
        pts->post.cRunning -= 1;

        PostManSetTimer(pts);
    }

    pHtmPost->Release();

    #if DBG==1
    PostManValidate();
    #endif

    PerfDbgLog(tagHtmPost, pHtmPost, "-PostManExecute");
    PerfDbgLog(tagPerfWatch, pHtmPost, "-PostManExecute");
}

void
PostManEnqueue(CHtmPost * pHtmPost)
{
    if (!(pHtmPost->_dwFlags & POSTF_ENQUEUED))
    {
        PerfDbgLog(tagHtmPost, pHtmPost, "+PostManEnqueue");

        THREADSTATE *   pts         = GetThreadState();
        CHtmPost **     ppHtmPost   = &pts->post.pHtmPostHead;
        CHtmPost *      pHtmPostT;

        for (; (pHtmPostT = *ppHtmPost) != NULL;
                ppHtmPost = &pHtmPostT->_pHtmPostNext)
            ;

        *ppHtmPost              = pHtmPost;
        pHtmPost->_pHtmPostNext = NULL;
        pHtmPost->_dwFlags     |= POSTF_BLOCKED | POSTF_ENQUEUED;
        pHtmPost->_dwFlags     &= ~POSTF_RUNNING;
        pHtmPost->SubAddRef();

        #if DBG==1
        PostManValidate();
        #endif

        PerfDbgLog(tagHtmPost, pHtmPost, "-PostManEnqueue");
    }
}

void
PostManDequeue(CHtmPost * pHtmPost)
{
    if (pHtmPost->_dwFlags & POSTF_ENQUEUED)
    {
        PerfDbgLog(tagHtmPost, pHtmPost, "+PostManDequeue");

        THREADSTATE *   pts         = GetThreadState();
        CHtmPost **     ppHtmPost   = &pts->post.pHtmPostHead;
        CHtmPost *      pHtmPostT;

        for (; (pHtmPostT = *ppHtmPost) != NULL; ppHtmPost = &pHtmPostT->_pHtmPostNext)
        {
            if (pHtmPostT == pHtmPost)
            {
                if (!(pHtmPost->_dwFlags & (POSTF_BLOCKED | POSTF_RUNNING)))
                {
                    pts->post.cRunable -= 1;
                }

                if (pHtmPost->_dwFlags & POSTF_RUNNING)
                {
                    pts->post.cRunning -= 1;
                }

                if (pts->post.pHtmPostNext == pHtmPost)
                {
                    pts->post.pHtmPostNext = pHtmPost->_pHtmPostNext;
                }

                *ppHtmPost = pHtmPost->_pHtmPostNext;

                pHtmPost->_dwFlags &= ~(POSTF_ENQUEUED | POSTF_BLOCKED | POSTF_RUNNING);
                pHtmPost->SubRelease();

                PostManSetTimer(pts);

                #if DBG==1
                PostManValidate();
                #endif

                break;
            }
        }

        PerfDbgLog(tagHtmPost, pHtmPost, "-PostManDequeue");
    }
}

void
PostManLock()
{
    THREADSTATE *   pts         = GetThreadState();
    pts->post.cLock += 1;
    PostManSetTimer(pts);
}

void
PostManUnlock()
{
    THREADSTATE *   pts         = GetThreadState();
    Assert(pts->post.cLock);
    pts->post.cLock -= 1;
    PostManSetTimer(pts);
}

void
PostManSuspend(CHtmPost * pHtmPost)
{
    if (     (pHtmPost->_dwFlags & POSTF_ENQUEUED)
       &&   !(pHtmPost->_dwFlags & POSTF_BLOCKED))
    {
        PerfDbgLog(tagPerfWatch, pHtmPost, "+PostManSuspend");
        PerfDbgLog(tagHtmPost, pHtmPost, "+PostManSuspend");

        THREADSTATE * pts = GetThreadState();

        pHtmPost->_dwFlags |= POSTF_BLOCKED;

        if (!(pHtmPost->_dwFlags & POSTF_RUNNING))
        {
            pts->post.cRunable -= 1;

            PostManSetTimer(pts);
        }

        #if DBG==1
        PostManValidate();
        #endif

        PerfDbgLog(tagHtmPost, pHtmPost, "-PostManSuspend");
        PerfDbgLog(tagPerfWatch, pHtmPost, "-PostManSuspend");
    }
}

void
PostManResume(CHtmPost * pHtmPost, BOOL fExecute)
{
    if (    (pHtmPost->_dwFlags & POSTF_ENQUEUED)
        &&  (pHtmPost->_dwFlags & POSTF_BLOCKED))
    {
        PerfDbgLog(tagPerfWatch, pHtmPost, "+PostManResume");
        PerfDbgLog(tagHtmPost, pHtmPost, "+PostManResume");

        THREADSTATE * pts = GetThreadState();

        pHtmPost->_dwFlags &= ~POSTF_BLOCKED;

        if (!(pHtmPost->_dwFlags & POSTF_RUNNING))
        {
            pts->post.cRunable += 1;
        }

        if (fExecute && pts->post.cRunning == 0)
            PostManExecute(pts, GetTickCount() + POSTMAN_TIMESLICE, pHtmPost);
        else
            PostManSetTimer(pts);

        #if DBG==1
        PostManValidate();
        #endif

        PerfDbgLog(tagHtmPost, pHtmPost, "-PostManResume");
        PerfDbgLog(tagPerfWatch, pHtmPost, "-PostManResume");
    }
}

void
DeinitPostMan(THREADSTATE * pts)
{
    PerfDbgLog(tagHtmPost, 0, "DeinitPostMan");

    Assert(pts->post.pHtmPostHead == NULL &&
        "Active post tasks remain at thread shutdown");

    pts->post.cRunable = 0;
    PostManSetTimer(pts);
}

#if DBG==1
void
PostManValidate()
{
    THREADSTATE *   pts        = GetThreadState();
    DWORD           cRunning   = 0;
    DWORD           cRunable   = 0;
    BOOL            fFoundNext = !pts->post.pHtmPostNext;
    CHtmPost *      pHtmPost   = pts->post.pHtmPostHead;

    for (; pHtmPost; pHtmPost = pHtmPost->_pHtmPostNext)
    {
        if (pHtmPost == pts->post.pHtmPostNext)
        {
            fFoundNext = TRUE;
        }

        Assert(pHtmPost->_dwFlags & POSTF_ENQUEUED);

        if (pHtmPost->_dwFlags & POSTF_RUNNING)
        {
            cRunning += 1;
        }
        else if (!(pHtmPost->_dwFlags & POSTF_BLOCKED))
        {
            cRunable += 1;
        }
    }

    Assert(fFoundNext);
    Assert(cRunning == pts->post.cRunning);
    Assert(cRunable == pts->post.cRunable);
}
#endif

#if DBG != 1
#pragma optimize("", on)
#endif
