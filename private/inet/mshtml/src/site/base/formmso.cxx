//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       src\core\formkrnl\formmso.cxx
//
//  Contents:   Implementation of IOleCommandTarget
//
//  Classes:    CDoc
//
//  Functions:
//
//  History:    04-May-95   RodC    Created
//
//----------------------------------------------------------------------------
#ifdef UNIX
#include <inetreg.h>
#endif

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>  // for the definition of ShellExecute
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_NTVERP_H_
#define X_NTVERP_H_
#include "ntverp.h"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_IMGANIM_HXX_
#define X_IMGANIM_HXX_
#include "imganim.hxx"   // for _pimganim
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_UPDSINK_HXX_
#define X_UPDSINK_HXX_
#include "updsink.hxx"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
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

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

// BUGBUG: raminh 2-26-98
// Once SelectionInOneFlowLayout() is implemented using MarkupServices
// within MshtmlEd, remember to take out #include "txtselr.h"

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_CTLRANGE_HXX_
#define X_CTLRANGE_HXX_
#include "ctlrange.hxx"
#endif

#ifndef X_CGLYPH_HXX_
#define X_CGLYPH_HXX_
#include "cglyph.hxx"
#endif

PerfDbgExtern(tagPerfWatch)

#ifdef UNIX
// A hack to compile:
EXTERN_C const GUID CGID_DocHostCommandHandler = {0xf38bc242,0xb950,0x11d1, {0x89,0x18,0x00,0xc0,0x4f,0xc2,0xc8,0x36}};
#endif // UNIX
EXTERN_C const GUID CGID_DocHostCommandHandler;

extern TCHAR g_achDLLCore[];
ExternTag(tagMsoCommandTarget);
extern void DumpFormatCaches();

#ifdef UNIX
extern int g_SelectedFontSize;
#endif

#ifndef NO_MULTILANG
extern IMultiLanguage2 *g_pMultiLanguage2; // JIT langpack
#endif

#ifndef NO_SCRIPT_DEBUGGER
extern interface IDebugApplication *g_pDebugApp;
#endif

ULONG ConvertSBCMDID(ULONG localIDM)
{
    struct SBIDMConvert {
        ULONG localIDM;
        ULONG SBCMDID;
    };
    static const SBIDMConvert SBIDMConvertTable[] =
    {
        { IDM_TOOLBARS,       SBCMDID_SHOWCONTROL },
        { IDM_STATUSBAR,      SBCMDID_SHOWCONTROL },
        { IDM_OPTIONS,        SBCMDID_OPTIONS },
        { IDM_ADDFAVORITES,   SBCMDID_ADDTOFAVORITES },
        { IDM_CREATESHORTCUT, SBCMDID_CREATESHORTCUT },
        { 0, 0 }
    };

    ULONG SBCmdID = IDM_UNKNOWN;
    int   i;

    for (i = 0; SBIDMConvertTable[i].localIDM; i ++)
    {
        if (SBIDMConvertTable[i].localIDM == localIDM)
        {
            SBCmdID = SBIDMConvertTable[i].SBCMDID;
            break;
        }
    }

    return SBCmdID;
}

//////////////
//  Globals // moved from rootlyt.cxx
//////////////

BSTR                g_bstrFindText = NULL;

HRESULT
GetFindText(BSTR *pbstr)
{
    LOCK_GLOBALS;

    RRETURN(FormsAllocString(g_bstrFindText, pbstr));
}


//+-------------------------------------------------------------------------
//
//  Method:     CDoc::RouteCTElement
//
//  Synopsis:   Route a command target call, either QueryStatus or Exec
//              to an element
//
//--------------------------------------------------------------------------

HRESULT
CDoc::RouteCTElement(CElement *pElement, CTArg *parg)
{
    HRESULT     hr = OLECMDERR_E_NOTSUPPORTED;
    CTreeNode * pNodeParent;
    AAINDEX     aaindex;
    IUnknown *  pUnk = NULL;
    CDoc *      pDocSec = NULL;
    
    if (TestLock(FORMLOCK_QSEXECCMD))
    {
        pDocSec = this;
    }
    else if (_pDocParent && _pDocParent->TestLock(FORMLOCK_QSEXECCMD))
    {
        pDocSec = _pDocParent;
    }

    if (pDocSec)
    {
        aaindex = pDocSec->FindAAIndex(
            DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
        if (aaindex != AA_IDX_UNKNOWN)
        {
            hr = THR(pDocSec->GetUnknownObjectAt(aaindex, &pUnk));
            if (hr)
                goto Cleanup;
        }
    }
    
    while (pElement)
    {
        Assert(pElement->Tag() != ETAG_ROOT || 
               pElement == _pPrimaryMarkup->Root());

        if (pElement == _pPrimaryMarkup->Root())
            break;

        if (pUnk)
        {
            pElement->AddUnknownObject(
                DISPID_INTERNAL_INVOKECONTEXT, pUnk, CAttrValue::AA_Internal);
        }
        
        if (parg->fQueryStatus)
        {
            Assert(parg->pqsArg->cCmds == 1);
            
            hr = THR_NOTRACE(pElement->QueryStatus(
                    parg->pguidCmdGroup,
                    parg->pqsArg->cCmds,
                    parg->pqsArg->rgCmds,
                    parg->pqsArg->pcmdtext));
            if (parg->pqsArg->rgCmds[0].cmdf)
                break;  // Element handled it.
        }
        else
        {
            hr = THR_NOTRACE(pElement->Exec(
                    parg->pguidCmdGroup,
                    parg->pexecArg->nCmdID,
                    parg->pexecArg->nCmdexecopt,
                    parg->pexecArg->pvarargIn,
                    parg->pexecArg->pvarargOut));
            if (hr != OLECMDERR_E_NOTSUPPORTED)
                break;
        }

        if (pUnk)
        {
            pElement->FindAAIndexAndDelete(
                DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
        }

        if (pElement == GetPrimaryElementClient())
            break;

        if (!pElement->IsInMarkup())
            break;
            
        pNodeParent = pElement->GetFirstBranch()->Parent();
        pElement = pNodeParent ? pNodeParent->Element() : NULL;

        Assert(pElement->Tag() != ETAG_ROOT);
    }

Cleanup:
    if (pUnk && pElement)
    {
        pElement->FindAAIndexAndDelete(
            DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
    }
    ReleaseInterface(pUnk);
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDoc::QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CDoc::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    TraceTag((tagMsoCommandTarget, "CDoc::QueryStatus"));

    // Check to see if the command is in our command set.
    if (!IsCmdGroupSupported(pguidCmdGroup))
        RRETURN(OLECMDERR_E_UNKNOWNGROUP);

    MSOCMD *    pCmd;
    INT         c;
    UINT        idm;
    HRESULT     hr = S_OK;
    MSOCMD      msocmd;
    CTArg       ctarg;
    CTQueryStatusArg    qsarg;
    
    // Loop through each command in the ary, setting the status of each.
    for (pCmd = rgCmds, c = cCmds; --c >= 0; pCmd++)
    {
        // By default command status is NOT SUPPORTED.
        pCmd->cmdf = 0;

        idm = IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);
        if (pcmdtext && pcmdtext->cmdtextf == MSOCMDTEXTF_STATUS)
        {
            pcmdtext[c].cwActual = LoadString(
                    GetResourceHInst(),
                    IDS_MENUHELP(idm),
                    pcmdtext[c].rgwz,
                    pcmdtext[c].cwBuf);
        }

        if (    !_fDesignMode
            &&  idm >= IDM_MENUEXT_FIRST__
            &&  idm <= IDM_MENUEXT_LAST__
            &&  _pOptionSettings)
        {
            CONTEXTMENUEXT *    pCME;
            int                 nExts, nExtCur;

            // not supported unless the next test succeeds
            pCmd->cmdf = 0;

            nExts = _pOptionSettings->aryContextMenuExts.Size();
            nExtCur = idm - IDM_MENUEXT_FIRST__;

            if(nExtCur < nExts)
            {
                // if we have it, it is enabled
                pCmd->cmdf = MSOCMDSTATE_UP;

                // the menu name is the text returned
                pCME = _pOptionSettings->
                            aryContextMenuExts[idm - IDM_MENUEXT_FIRST__];
                pCmd->cmdf = MSOCMDSTATE_UP;

                Assert(pCME);

                if (pcmdtext && pcmdtext->cmdtextf == MSOCMDTEXTF_NAME)
                {
                    hr = Format(
                            0,
                            pcmdtext->rgwz,
                            pcmdtext->cwBuf,
                            pCME->cstrMenuValue);
                    if (!hr)
                        pcmdtext->cwActual = _tcslen(pcmdtext->rgwz);

                    // ignore the hr
                    hr = S_OK;
                }
            }
        }

        switch (idm)
        {
        case IDM_REPLACE:
        case IDM_FONT:
        case IDM_GOTO:
        case IDM_HYPERLINK:
        case IDM_BOOKMARK:
        case IDM_IMAGE:
            if(_fInHTMLDlg)
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;

        case IDM_FIND:
            if (_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            else
                pCmd->cmdf = _fInHTMLDlg ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;
            break;

        case IDM_PROPERTIES:
            pCmd->cmdf = MSOCMDSTATE_UP;
            hr = S_OK;
            break;
            
        case IDM_MENUEXT_COUNT:
            pCmd->cmdf = _pOptionSettings ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
            break;

        case IDM_REDO:
        case IDM_UNDO:
            QueryStatusUndoRedo((IDM_UNDO == idm), pCmd, pcmdtext);
            break;

        case IDM_SAVE:
            if (!_fDesignMode)
            {
                // Disable Save Command if in BROWSE mode.
                //
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
                break;
            }
            // fall through to QueryStatus the DocFrame if in EDIT mode.

        case IDM_NEW:
        case IDM_OPEN:
            //  Bubble it out to the DocFrame

            msocmd.cmdf  = 0;
            msocmd.cmdID = (idm == IDM_NEW) ? (OLECMDID_NEW) :
                   ((idm == IDM_OPEN) ? (OLECMDID_OPEN) : (OLECMDID_SAVE));
            hr = THR(CTQueryStatus(_pInPlace->_pInPlaceSite, NULL, 1, &msocmd, NULL));
            if (!hr)
                pCmd->cmdf = msocmd.cmdf;

            break;

        case IDM_SAVEAS:
            pCmd->cmdf = IsFullWindowEmbed() ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;
            break;

        case IDM_ISTRUSTEDDLG:
            if(_fInTrustedHTMLDlg)
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            else
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;

#if !defined(WIN16) && !defined(WINCE)
#if !defined(NO_SCRIPT_DEBUGGER)
        case IDM_TOOLBARS:
        case IDM_STATUSBAR:
            ULONG        SBCmdId;
            VARIANTARG   varIn, varOut;

            pCmd->cmdf = MSOCMDSTATE_DISABLED;

            SBCmdId = ConvertSBCMDID(idm);
            varIn.vt   = VT_I4;
            varIn.lVal = MAKELONG(
                    (idm == IDM_TOOLBARS) ? (FCW_INTERNETBAR) : (FCW_STATUS),
                    SBSC_QUERY);

            hr = THR(CTExec(
                    _pInPlace->_pInPlaceSite,
                    &CGID_Explorer,
                    SBCmdId,
                    0,
                    &varIn,
                    &varOut));
            if (!hr && varOut.vt == VT_I4)
            {
                switch (varOut.lVal)
                {
                case SBSC_HIDE:
                    pCmd->cmdf = MSOCMDSTATE_UP;
                    break;

                case SBSC_SHOW:
                    pCmd->cmdf = MSOCMDSTATE_DOWN;
                    break;
                }
            }

            break;

#endif // NO_SCRIPT_DEBUGGER

        case IDM_OPTIONS:
        case IDM_ADDFAVORITES:
        case IDM_CREATESHORTCUT:
            msocmd.cmdf  = 0;
            msocmd.cmdID = ConvertSBCMDID(idm);
            hr = THR(CTQueryStatus(
                    _pInPlace->_pInPlaceSite,
                    &CGID_Explorer,
                    1,
                    &msocmd,
                    NULL));
            if (!hr)
            {
                pCmd->cmdf = (msocmd.cmdf & MSOCMDF_ENABLED) ?
                        (MSOCMDSTATE_UP) : (MSOCMDSTATE_DISABLED);
            }
            break;
#endif // !WIN16 && !WINCE

        case IDM_PAGESETUP:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_PRINT:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_PRINTQUERYJOBSPENDING:
            pCmd->cmdf = (PrintJobsPending() ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED);
            break;

        case IDM_HELP_CONTENT:
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;

        case IDM_HELP_README:
        case IDM_HELP_ABOUT:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_BROWSEMODE:
            if (_fDesignMode)
                pCmd->cmdf = MSOCMDSTATE_UP;
            else
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            break;

        case IDM_EDITMODE:
            if (_fImageFile) // Cannot edit image files
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            else if (_fFrameSet)
                pCmd->cmdf = MSOCMDSTATE_DISABLED;  // Disable if it is frameset
            else if (_fDesignMode)
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            else
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_VIEWSOURCE:
            if (_fImageFile || _fFullWindowEmbed) // No source for non-HTML files
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            else
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

#ifndef NO_SCRIPT_DEBUGGER
        case IDM_SCRIPTDEBUGGER:
            if (PrimaryMarkup()->HasScriptContext() &&
                PrimaryMarkup()->ScriptContext()->_pScriptDebugDocument)
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_BREAKATNEXT:
        case IDM_LAUNCHDEBUGGER:
            pCmd->cmdf = (PrimaryMarkup()->HasScriptContext() &&
                          PrimaryMarkup()->ScriptContext()->_pScriptDebugDocument) ?
                            MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
            break;
#endif // ndef NO_SCRIPT_DEBUGGER

        case IDM_STOP:
            pCmd->cmdf = _fDesignMode ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;
            break;

        case IDM_STOPDOWNLOAD:
            pCmd->cmdf = _fSpin ? MSOCMDF_ENABLED : 0;
            break;

        case IDM_REFRESH_TOP:
        case IDM_REFRESH_TOP_FULL:
            GetRootDoc()->QueryRefresh(&pCmd->cmdf);
            break;

        case IDM_REFRESH:
        case IDM_REFRESH_THIS:
        case IDM_REFRESH_THIS_FULL:
            QueryRefresh(&pCmd->cmdf);
            break;

        case IDM_CONTEXTMENU:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_GOBACKWARD:
        case IDM_GOFORWARD:
            {
                // default this to disabled since we're not
                // hosted in shdocvw when we're on the desktop
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
                LPOLECLIENTSITE lpClientSite;
                hr = GetRootDoc()->GetClientSite(&lpClientSite);
                if (!OK(hr) || !lpClientSite)
                    goto Cleanup;

                IOleCommandTarget *pCommandTarget;
                hr = lpClientSite->QueryInterface(IID_IOleCommandTarget,
                                                  (void**)&pCommandTarget);
                if (!OK(hr) || !pCommandTarget)
                    goto Cleanup;

                MSOCMD rgCmds1[1];
                rgCmds1[0].cmdID = (idm == IDM_GOBACKWARD)
                    ? SHDVID_CANGOBACK
                    : SHDVID_CANGOFORWARD;
                rgCmds1[0].cmdf  = 0;
                hr = pCommandTarget->QueryStatus(&CGID_ShellDocView,
                                                 1,
                                                 rgCmds1,
                                                 NULL);
                if (OK(hr))
                    pCmd->cmdf = rgCmds1[0].cmdf ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;

                ReleaseInterface(pCommandTarget);
           Cleanup:
                ReleaseInterface(lpClientSite);
           }
           break;

        case IDM_BASELINEFONT1:
        case IDM_BASELINEFONT2:
        case IDM_BASELINEFONT3:
        case IDM_BASELINEFONT4:
        case IDM_BASELINEFONT5:
            //
            // depend on that IDM_BASELINEFONT1, IDM_BASELINEFONT2,
            // IDM_BASELINEFONT3, IDM_BASELINEFONT4, IDM_BASELINEFONT5 to be
            // consecutive integers.
            //
            {
                if (GetBaselineFont() ==
                    (short)(idm - IDM_BASELINEFONT1 + BASELINEFONTMIN))
                {
                    pCmd->cmdf = MSOCMDSTATE_DOWN;
                }
                else
                {
                    pCmd->cmdf = MSOCMDSTATE_UP;
                }
            }
            break;

        case IDM_SHDV_MIMECSETMENUOPEN:
        case IDM_SHDV_FONTMENUOPEN:
        case IDM_SHDV_GETMIMECSETMENU:
        case IDM_SHDV_GETFONTMENU:
        case IDM_LANGUAGE:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_DIRLTR:
        case IDM_DIRRTL:
            {
                BOOL fDocRTL;

                hr = THR(GetDocDirection(&fDocRTL));
                if (hr == S_OK && ((!fDocRTL) ^ (idm == IDM_DIRRTL)))
                {
                    pCmd->cmdf = MSOCMDSTATE_DOWN;
                }
                else
                {
                    pCmd->cmdf = MSOCMDSTATE_UP;
                }
            }
            break;

        case IDM_SHDV_DEACTIVATEMENOW:
        case IDM_SHDV_NODEACTIVATENOW:
            //  This is Exec only in the upward direction.
            //  we shouldn't get here.
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;

        case IDM_SHDV_CANDEACTIVATENOW:
            //  We return ENABLED unless we or one of our OCs [eg a frame]
            //  are in a script or otherwise not able to be deactivated.  if this is
            //  disabled, SHDOCVW will defer the activation until signaled by
            //  a SHDVID_DEACTIVATEMENOW on script exit [at which time, it
            //  will redo the SHDVID_CANDEACTIVATENOW querystatus]
            if (IsInScript())
                pCmd->cmdf = MSOCMDSTATE_DISABLED;
            else
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

        case IDM_SHDV_PAGEFROMPOSTDATA:
            if (_pDwnPost)
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            else
                pCmd->cmdf = MSOCMDSTATE_UP;
            break;

#ifdef IDM_SHDV_ONCOLORSCHANGE
                        // Let the shell know we support the new palette notification
                case IDM_SHDV_ONCOLORSCHANGE:
                        pCmd->cmdf = MSOCMDF_SUPPORTED;
                        break;
#endif
        case IDM_HTMLEDITMODE:
            pCmd->cmdf = _fInHTMLEditMode ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP;
            break;

        case IDM_SHOWALLTAGS:
            pCmd->cmdf = _fShowAlignedSiteTags &&
                         _fShowMiscTags &&
                         _fShowScriptTags &&
                         _fShowStyleTags &&
                         _fShowCommentTags &&
                         _fShowAreaTags &&
                         _fShowUnknownTags &&
                         _fShowMiscTags ?
                         MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_SHOWALIGNEDSITETAGS:
            pCmd->cmdf = _fShowAlignedSiteTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_SHOWSCRIPTTAGS:
            pCmd->cmdf = _fShowScriptTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_SHOWSTYLETAGS:
            pCmd->cmdf = _fShowStyleTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_SHOWCOMMENTTAGS:
            pCmd->cmdf = _fShowCommentTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_SHOWAREATAGS:
            pCmd->cmdf = _fShowAreaTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_SHOWUNKNOWNTAGS:
            pCmd->cmdf = _fShowUnknownTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_SHOWMISCTAGS:
            pCmd->cmdf = _fShowMiscTags ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_SHOWZEROBORDERATDESIGNTIME:
            pCmd->cmdf = GetView()->IsFlagSet(CView::VF_ZEROBORDER) ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_NOACTIVATENORMALOLECONTROLS:
            pCmd->cmdf = _fNoActivateNormalOleControls ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_NOACTIVATEDESIGNTIMECONTROLS:
            pCmd->cmdf = _fNoActivateDesignTimeControls ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

        case IDM_NOACTIVATEJAVAAPPLETS:
            pCmd->cmdf = _fNoActivateJavaApplets ? MSOCMDSTATE_UP : MSOCMDSTATE_DOWN;
            break;

#if DBG==1
        case IDM_DEBUG_TRACETAGS:
        case IDM_DEBUG_DUMPOTRACK:
        case IDM_DEBUG_RESFAIL:
        case IDM_DEBUG_BREAK:
        case IDM_DEBUG_VIEW:
        case IDM_DEBUG_DUMPTREE:
        case IDM_DEBUG_DUMPFORMATCACHES:
        case IDM_DEBUG_DUMPLINES:
        case IDM_DEBUG_MEMMON:
        case IDM_DEBUG_METERS:
        case IDM_DEBUG_DUMPDISPLAYTREE:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;
#endif // DBG == 1

        }

//#ifndef NO_IME
        // Enables the languages in the browse context menu
        if( !_fDesignMode && idm >= IDM_MIMECSET__FIRST__ &&
                             idm <= IDM_MIMECSET__LAST__)
        {
            CODEPAGE cp = GetCodePageFromMenuID(idm);

            if (cp == GetCodePage() || cp == CP_UNDEFINED)
            {
                pCmd->cmdf = MSOCMDSTATE_DOWN;
            }
            else
            {
                pCmd->cmdf = MSOCMDSTATE_UP;
            }
        }
//#endif // !NO_IME

        //
        // If still not handled then try menu object.
        //

        ctarg.pguidCmdGroup = pguidCmdGroup;
        ctarg.fQueryStatus = TRUE;
        ctarg.pqsArg = &qsarg;
        qsarg.cCmds = 1;
        qsarg.rgCmds = pCmd;
        qsarg.pcmdtext = pcmdtext;
        
        if (!pCmd->cmdf && _pMenuObject)
        {
            hr = THR_NOTRACE(RouteCTElement(_pMenuObject, &ctarg));
        }

        //
        // Next try the current element;
        //
        
        if (!pCmd->cmdf && _pElemCurrent)
        {
            hr = THR_NOTRACE(RouteCTElement(_pElemCurrent, &ctarg));
        }

        //
        // Finally try edit router
        //
        
        if (!pCmd->cmdf)
        {
            hr = THR_NOTRACE( _EditRouter.QueryStatusEditCommand(
                    pguidCmdGroup,
                    1,
                    pCmd,
                    pcmdtext,
                    (IUnknown *)(IPrivateUnknown *)this,
                    this ));
            if (hr == S_OK
                &&  (pCmd->cmdf & MSOCMDF_ENABLED)
                &&  _pElemEditContext
                &&  _pElemEditContext != _pElemCurrent)
            {
                DWORD cmdfSave = pCmd->cmdf;

                // Check if the edit context wants to disallow the command (fix for 46807)
                pCmd->cmdf = 0;
                hr = THR_NOTRACE(_pElemEditContext->QueryStatus(
                    pguidCmdGroup,
                    1,
                    pCmd,
                    pcmdtext));
                if (pCmd->cmdf != MSOCMDSTATE_DISABLED)
                {
                    pCmd->cmdf = cmdfSave;
                }
            }
        }

        // Prevent any command but the first from setting this.
        pcmdtext = NULL;
    }

    SRETURN(hr);
}

#if !defined(UNIX)

extern HRESULT DisplaySource(LPCTSTR tszSourceName);

HRESULT CDoc::InvokeEditor( LPCTSTR tszSourceName )
{
    return DisplaySource(tszSourceName);
}

#else // !UNIX

HRESULT CDoc::InvokeEditor( LPCTSTR lptszPath )
{
    HRESULT         hr = S_OK;

    TCHAR           tszCommand[pdlUrlLen];
    TCHAR           tszExpandedCommand[pdlUrlLen];
    UINT            nCommandSize;
    int             i;
    HKEY    hkey;
    DWORD   dw;
    TCHAR *pchPos;
    BOOL bMailed;
    STARTUPINFO stInfo;

    hr = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_VSOURCECLIENTS,
                        0, NULL, 0, KEY_READ, NULL, &hkey, &dw);
    if (hr != ERROR_SUCCESS)
        goto Cleanup;

    dw = pdlUrlLen;
    hr = RegQueryValueEx(hkey, REGSTR_PATH_CURRENT, NULL, NULL, (LPBYTE)tszCommand, &dw);
    if (hr != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        goto Cleanup;
    }

    dw = ExpandEnvironmentStrings(tszCommand, tszExpandedCommand, pdlUrlLen);
    if (!dw)
    {
        _tcscpy(tszExpandedCommand, tszCommand);
    }
    _tcscat(tszCommand, tszExpandedCommand);

    for (i = _tcslen(tszCommand); i > 0; i--)
    {
        if (tszCommand[i] == '/')
        {
            tszCommand[i] = '\0';
            break;
        }
    }

    _tcscat(tszCommand, TEXT(" "));
    _tcscat(tszCommand, lptszPath);

    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.cb = sizeof(stInfo);
    stInfo.wShowWindow= SW_SHOWNORMAL;
    bMailed = CreateProcess(tszExpandedCommand,
                            tszCommand,
                            NULL, NULL, TRUE,
                            CREATE_NEW_CONSOLE,
                            NULL, NULL, &stInfo, NULL);

Cleanup:

    return hr;
}

#endif


//+-------------------------------------------------------------------------
//
//  Method:     CDoc::Exec
//
//  Synopsis:   Called to execute a given command.  If the command is not
//              consumed, it may be routed to other objects on the routing
//              chain.
//
//--------------------------------------------------------------------------

HRESULT
CDoc::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    TraceTag((tagMsoCommandTarget, "CDoc::Exec"));

    if (!IsCmdGroupSupported(pguidCmdGroup))
        RRETURN(OLECMDERR_E_UNKNOWNGROUP);


#ifndef NO_HTML_DIALOG
    struct DialogInfo
    {
        UINT    idm;
        UINT    idsUndoText;
        TCHAR * szidr;
    };

    // BUGBUG (cthrash) We should define and use better undo text.  Furthermore,
    // we should pick an appropriate one depending (for image, link, etc.)
    // on whether we're creating anew or editting an existing object.
    //
    // Fix for bug# 9136. (a-pauln)
    // Watch order of this array. Find dialogs need to be at the bottom,
    // and in the order listed (IDR_FINDDIALOG, IDR_BIDIFINDDIALOG).
    //
    // Find resources have been relocated to shdocvw (peterlee)
    static DialogInfo   dlgInfo[] =
    {
        {IDM_FIND,          0,                     NULL}, //IDR_FINDDIALOG,
        {IDM_FIND,          0,                     NULL}, //IDR_BIDIFINDDIALOG,
        {IDM_REPLACE,       IDS_UNDOGENERICTEXT,   IDR_REPLACEDIALOG},
        {IDM_PARAGRAPH,     IDS_UNDOGENERICTEXT,   IDR_FORPARDIALOG},
        {IDM_FONT,          IDS_UNDOGENERICTEXT,   IDR_FORCHARDIALOG},
        {IDM_GOTO,          0,                     IDR_GOBOOKDIALOG},
        {IDM_IMAGE,         IDS_UNDONEWCTRL,       IDR_INSIMAGEDIALOG},
        {IDM_HYPERLINK,     IDS_UNDOGENERICTEXT,   IDR_EDLINKDIALOG},
        {IDM_BOOKMARK,      IDS_UNDOGENERICTEXT,   IDR_EDBOOKDIALOG},
    };
#endif // NO_HTML_DIALOG

    CDoc::CLock         Lock(this);
#ifndef NO_EDIT
    CParentUndoUnit *   pCPUU = NULL;
#endif // NO_EDIT
    UINT                idm;
    HRESULT             hr = OLECMDERR_E_NOTSUPPORTED;
    DWORD               nCommandID;
    CTArg               ctarg;
    CTExecArg           execarg;
    
    //  artakka showhelp is not implemented (v2?)
    if(nCmdexecopt == MSOCMDEXECOPT_SHOWHELP)
    {
        return E_NOTIMPL;
    }

    // HELLO!!!!!!!!!!!!
    //
    // Commands can either go to the local CDoc (aka 'this') or
    // to the "root cdoc" as computed by GetRootDoc().
    // Commands like print, open, etc. go to the root CDoc.
    // Some of the others must stay "local" because they
    // aren't coming directly from the keyboard but rather
    // from some other code which tunneled down to us.
    // (mwagner)
    //

    CDoc *              pDocTarget = GetRootDoc();

    idm = IDMFromCmdID(pguidCmdGroup, nCmdID);

    // Handle context menu extensions - always eat the command here
    if( idm >= IDM_MENUEXT_FIRST__ && idm <= IDM_MENUEXT_LAST__)
    {
        hr = OnContextMenuExt(idm, pvarargIn);
        goto Cleanup;
    }

    switch (idm)
    {
        int             result;

#if DBG==1
    case IDM_DEBUG_MEMMON:
        DbgExOpenMemoryMonitor();
        hr = S_OK;
        break;

    case IDM_DEBUG_METERS:
        DbgExMtOpenMonitor();
        hr = S_OK;
        break;

    case IDM_DEBUG_TRACETAGS:
        DbgExDoTracePointsDialog(FALSE);
        hr = S_OK;
        break;

    case IDM_DEBUG_RESFAIL:
        DbgExShowSimFailDlg();
        hr = S_OK;
        break;

    case IDM_DEBUG_DUMPOTRACK:
        DbgExTraceMemoryLeaks();
        hr = S_OK;
        break;

    case IDM_DEBUG_BREAK:
        DebugBreak();
        hr = S_OK;
        break;

    case IDM_DEBUG_VIEW:
        DbgExOpenViewObjectMonitor(_pInPlace->_hwnd, (IUnknown *)(IViewObject *) this, TRUE);
        hr = S_OK;
        break;

    case IDM_DEBUG_DUMPTREE:
        {
            if(_pElemCurrent->GetMarkup())
                _pElemCurrent->GetMarkup()->DumpTree();
            break;
        }
    case IDM_DEBUG_DUMPLINES:
        {
            CFlowLayout * pFlowLayout = _pElemCurrent->GetFirstBranch()->GetFlowLayout();
            if(pFlowLayout)
                pFlowLayout->DumpLines();
            break;
        }
    case IDM_DEBUG_DUMPDISPLAYTREE:
        GetView()->DumpDisplayTree();
        break;

    case IDM_DEBUG_DUMPFORMATCACHES:
        DumpFormatCaches();
        break;
#endif

    case IDM_ADDFAVORITES:
        if (_pMenuObject)
        {
            hr = THR_NOTRACE(_pMenuObject->Exec(
                pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
        }

        if (_pElemCurrent && hr == OLECMDERR_E_NOTSUPPORTED)
        {
            hr = THR_NOTRACE(_pElemCurrent->Exec(
                pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
        }

        if (hr == OLECMDERR_E_NOTSUPPORTED)
        {
            // Add the current document to the favorite folder ...
            //
            TCHAR * pszURL;
            TCHAR * pszTitle;
            CMarkup * pMarkup = PrimaryMarkup();

            pszURL   = _cstrUrl;
            pszTitle = (pMarkup->GetTitleElement() && pMarkup->GetTitleElement()->Length())
                     ? (pMarkup->GetTitleElement()->GetTitle())
                     : (NULL);
            hr = AddToFavorites(pszURL, pszTitle);
        }
        break;
        
#ifndef NO_HTML_DIALOG
    // provide the options object to the dialog code
    case IDM_FIND:
    case IDM_REPLACE:
        // we should not invoke the dialogs out of the dialog...
        if (!_fInHTMLDlg && nCmdexecopt != MSOCMDEXECOPT_DONTPROMPTUSER)
        {
            CVariant            cVarNull(VT_NULL);
            IDispatch      *    pDispOptions = NULL;
            CParentUndoUnit*    pCPUU = NULL;
            BSTR                bstrText = NULL;
            TCHAR               achOverrideFindUrl[pdlUrlLen];
            COptionsHolder *    pcoh = NULL;
            CDoc *              pDoc = this;
            int                 i;        

            // The find dialog needs to search the active frame, if there is one
            if (idm == IDM_FIND)
            {
                CDoc *  pActiveFrameDoc = NULL;

                hr = THR(pDoc->GetActiveFrame(achOverrideFindUrl,
                    ARRAY_SIZE(achOverrideFindUrl),
                    &pActiveFrameDoc, NULL));
                if (FAILED(hr))
                    goto Cleanup_FindReplace;

                Assert((hr == S_FALSE && pActiveFrameDoc == 0) || (hr == S_OK && pActiveFrameDoc != 0));

                // If the active frame doc is null, just use what we've got.
                if (pActiveFrameDoc && pActiveFrameDoc != pDoc)
                {
                    pDoc = pActiveFrameDoc;
                }
            }
            pcoh = new COptionsHolder(pDoc);

            if (pcoh == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup_FindReplace;
            }

            // find RID string
            for (i = 0; i < ARRAY_SIZE(dlgInfo); ++i)
            {
                if (idm == dlgInfo[i].idm)
                    break;
            }
            Assert(i < ARRAY_SIZE(dlgInfo));

            if (dlgInfo[i].idsUndoText)
            {
                pCPUU = OpenParentUnit(this, dlgInfo[i].idsUndoText);
            }

            // get dispatch from stack variable
            hr = THR_NOTRACE(pcoh->QueryInterface(IID_IHTMLOptionsHolder,
                                     (void**)&pDispOptions));
            if (hr)
                goto Cleanup_FindReplace;

            // Save the execCommand argument so that the dialog can have acces
            // to them
            //
#ifdef _MAC     // casting so bad I left in the #ifdef
            pcoh->put_execArg(pvarargIn ? (VARIANT) * pvarargIn
                                        : *((VARIANT *) ((void *)&cVarNull)));
#else
            pcoh->put_execArg(pvarargIn ? (VARIANT) * pvarargIn
                                        : (VARIANT)   cVarNull);
#endif

            hr = THR(GetFindText(&bstrText));
            if (hr)
                goto Cleanup_FindReplace;

            // Set the findText argument for the dialog
            THR_NOTRACE(pcoh->put_findText(bstrText));
            FormsFreeString(bstrText);
            bstrText = NULL;

            if (idm == IDM_REPLACE)
            {
                hr = THR(ShowModalDialogHelper(
                        pDoc,
                        dlgInfo[i].szidr,
                        pDispOptions,
                        pcoh));
                goto UIHandled;
            }

            // Fix for bug# 9136. (a-pauln)
            // make an adjustment for the bidi find dialog
            // if we are on a machine that supports bidi
            BOOL fbidi;
            fbidi = (idm == IDM_FIND && g_fBidiSupport);

            // Let host show find dialog
            VARIANT varIn;
            VARIANT varOut;

            V_VT(&varIn) = VT_DISPATCH;
            V_DISPATCH(&varIn) = pDispOptions;

            // The HTMLView object in Outlook 98 returns S_OK for all exec
            // calls, even those for which it should return OLECMD_E_NOTSUPPORTED.
            if (pDoc->_pHostUICommandHandler && !pDoc->_fOutlook98)
            {
                hr = pDoc->_pHostUICommandHandler->Exec(
                    &CGID_DocHostCommandHandler,
                    OLECMDID_SHOWFIND,
                    fbidi,
                    &varIn,
                    &varOut);

                if (!hr)
                    goto UIHandled;
            }

            // Let backup show find dialog
            pDoc->EnsureBackupUIHandler();
            if (pDoc->_pBackupHostUIHandler)
            {
                IOleCommandTarget * pBackupHostUICommandHandler;
                hr = pDoc->_pBackupHostUIHandler->QueryInterface(IID_IOleCommandTarget,
                    (void **) &pBackupHostUICommandHandler);
                if (hr)
                    goto Cleanup_FindReplace;

                hr = THR(pBackupHostUICommandHandler->Exec(
                    &CGID_DocHostCommandHandler,
                    OLECMDID_SHOWFIND,
                    fbidi,
                    &varIn,
                    &varOut));
                ReleaseInterface(pBackupHostUICommandHandler);
            }

UIHandled:
Cleanup_FindReplace:
            // release dispatch, et al.
            ReleaseInterface(pcoh);
            ReleaseInterface(pDispOptions);

            if ( pCPUU )
            {
                IGNORE_HR(CloseParentUnit( pCPUU, hr ) );
            }
        }

        break;


    case IDM_PROPERTIES:
        if (_pMenuObject && _pMenuObject->HasPages())
        {
            THR(ShowPropertyDialog(1, &_pMenuObject));
        }
        else if (!_pMenuObject && _pElemCurrent && _pElemCurrent->HasPages())
        {
            THR(ShowPropertyDialog(1, &_pElemCurrent));
        }
        else
        {
            THR(ShowPropertyDialog(0, NULL));
        }
        hr = S_OK;
        break;
#endif // NO_HTML_DIALOG

    case IDM_MENUEXT_COUNT:
        if(!pvarargOut)
        {
            hr = E_INVALIDARG;
        }
        else if(!_pOptionSettings)
        {
            hr = OLECMDERR_E_DISABLED;
        }
        else
        {
            hr = S_OK;
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) =
                _pOptionSettings->aryContextMenuExts.Size();
        }
        break;

    case IDM_UNDO:
        hr = THR(EditUndo());
        break;

    case IDM_REDO:
        hr = THR(EditRedo());
        break;

    case IDM_SHDV_CANDOCOLORSCHANGE:
        {
            hr = S_OK;
            break;
        }

    case IDM_SHDV_CANSUPPORTPICS:
        if (!pvarargIn || (pvarargIn->vt != VT_UNKNOWN))
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            SetPicsCommandTarget((IOleCommandTarget *)pvarargIn->punkVal);
            hr = S_OK;
        }
        break;

    case IDM_SHDV_ISDRAGSOURCE:
        if (!pvarargOut)
        {
            Assert(pvarargOut);
            hr = E_INVALIDARG;
        }
        else
        {
            pvarargOut->vt = VT_I4;
            V_I4(pvarargOut) = _fIsDragDropSrc;
            hr = S_OK;
        }
        break;

#if !defined(WIN16) && !defined(WINCE) && !defined(NO_SCRIPT_DEBUGGER)
    case IDM_TOOLBARS:
    case IDM_STATUSBAR:
    case IDM_OPTIONS:
    case IDM_CREATESHORTCUT:
        DWORD        CmdOptions;
        VARIANTARG * pVarIn;
        VARIANTARG   var;
        CDoc       * pDoc;


        nCommandID = ConvertSBCMDID(idm);
        CmdOptions = 0;
        pDoc       = pDocTarget;
        if (idm == IDM_OPTIONS)
        {
            V_VT(&var) = VT_I4;
            V_I4(&var) = SBO_NOBROWSERPAGES;
            pVarIn   = &var;
        }
        else if (idm == IDM_CREATESHORTCUT)
        {
            pDoc       = this;
            pVarIn     = NULL;
            CmdOptions = MSOCMDEXECOPT_PROMPTUSER;
        }
        else // IDM_TOOLBARS and IDM_STATUSBAR
        {
            V_VT(&var) = VT_I4;
            V_I4(&var) = MAKELONG(
                    (idm == IDM_TOOLBARS) ? (FCW_INTERNETBAR) : (FCW_STATUS),
                    SBSC_TOGGLE);
            pVarIn   = &var;
        }

        hr = THR(CTExec(
                pDoc->_pInPlace->_pInPlaceSite,
                &CGID_Explorer,
                nCommandID,
                CmdOptions,
                pVarIn,
                0));
        break;
#endif // !WIN16 && !WINCE

    case IDM_NEW:
    case IDM_OPEN:
    case IDM_SAVE:
        //  Bubble it out to the DocFrame

        switch(idm)
        {
        case IDM_NEW:
            nCommandID = OLECMDID_NEW;
            break;
        case IDM_OPEN:
            nCommandID = OLECMDID_OPEN;
            break;
        default:
            nCommandID = OLECMDID_SAVE;
            break;
        }
        hr = THR(CTExec(
            (IUnknown *)(pDocTarget->_pInPlace ?
                (IUnknown *)pDocTarget->_pInPlace->_pInPlaceSite : (IUnknown *)pDocTarget->_pClientSite),
            NULL, nCommandID, 0, 0, 0));
        break;

    case IDM_SAVEAS:
        {
            // if _pElemCurrent is IFrame or Frame, Send saveas command it,
            if (_pElemCurrent->Tag() == ETAG_IFRAME ||
                _pElemCurrent->Tag() == ETAG_FRAME)
            {
                hr = THR_NOTRACE(_pElemCurrent->Exec(
                        pguidCmdGroup,
                        nCmdID,
                        nCmdexecopt,
                        pvarargIn,
                        pvarargOut));
            }

            // If frame does not handle the command or _pElemCurrent is not a frame
            // Save current document
            if (hr == OLECMDERR_E_NOTSUPPORTED)
            {
                // Pass it up to the host
                // If we don't have a _pHostUICommandHandler, then hr will remain OLECMDERR_E_NOTSUPPORTED
                if (    _pHostUICommandHandler
                && !(nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
                )
                {
                    hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
                }

                // Only do it ourselves if the host doesn't understand the CGID or the CMDid
                //
                if (FAILED(hr))
                {
                    TCHAR * pchPathName = NULL;
                    BOOL fShowUI = TRUE;

                    if (pvarargIn && V_VT(pvarargIn) == VT_BSTR)
                    {
                        pchPathName = V_BSTR(pvarargIn);
                    }

                    if (nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
                    {
                        MSOCMD msocmd;

                        msocmd.cmdf  = 0;
                        msocmd.cmdID = OLECMDID_ALLOWUILESSSAVEAS;
                        if (!THR(CTQueryStatus(_pInPlace->_pInPlaceSite, NULL, 1, &msocmd, NULL)))
                            fShowUI = !(msocmd.cmdf == MSOCMDSTATE_UP);
                    }

                    if (!fShowUI && !pchPathName)
                        hr = E_INVALIDARG;
                    else
                        hr = pDocTarget->PromptSave(TRUE, fShowUI, pchPathName);
                }
            }

            if ( hr == S_FALSE )
            {
                hr = OLECMDERR_E_CANCELED;
            }
        }
        break;

    case IDM_SAVEASTHICKET:
        {
            if (!pvarargIn)
            {
                hr = E_INVALIDARG;
            }
            else
            {
                CVariant cvarDocument;

                hr = THR(cvarDocument.CoerceVariantArg(pvarargIn, VT_UNKNOWN));
                if (SUCCEEDED(hr))
                {
                    hr = THR(SaveSnapshotHelper( V_UNKNOWN(&cvarDocument), true ));
                }
            }
        }
        break;

#ifndef NO_PRINT
    case IDM_PAGESETUP:

        // If we have a HostUICommandHandler, and the caller did NOT request no-UI, pass it up to the host
        // If we don't have a _pHostUICommandHandler, then hr will remain OLECMDERR_E_NOTSUPPORTED
        if (_pHostUICommandHandler
            && !(nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
            && !_fOutlook98
            )
        {
            hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
        }

        // Only do it ourselves if the host doesn't understand the CGID or the CMDid
        //
        if (FAILED(hr))
        {
            hr = pDocTarget->PageSetup();
            if ( hr == S_FALSE )
            {
                hr = OLECMDERR_E_CANCELED;
            }
        }
        break;

    case IDM_EXECPRINT :  // comes from script ExecCommand
    case IDM_PRINT:       // comes from IOleCommandTarget
        {
            DWORD dwUILess = 0;
            BOOL  fOutlook98 = _fOutlook98;

            dwUILess = (pvarargIn) ?
                            (((V_VT(pvarargIn) == VT_I2) ? pvarargIn->iVal:0) |
                                    ((nCmdexecopt&MSOCMDEXECOPT_DONTPROMPTUSER) ?
                                            PRINT_DONTBOTHERUSER : 0))
                           : (nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)? PRINT_DONTBOTHERUSER : 0;

            // if no-UI is requested from execCommand, then we better be a trusted Dialog, or an HTA.
            // HTA's also use this trusted bit
            // window.print doesn't make this request
            if (dwUILess && nCmdID==IDM_EXECPRINT && !_fInTrustedHTMLDlg )
            {
                nCmdexecopt ^= OLECMDEXECOPT_DONTPROMPTUSER;
                dwUILess = 0;
                if (pvarargIn && (V_VT(pvarargIn)== VT_I2) )
                {
                    pvarargIn->iVal ^= PRINT_DONTBOTHERUSER;
                }
            }

            // BUGBUG: 68038 - _fOutlook98 is not set when we are printing the Outlook98 Today page.
            // So we use the "outday://" url to identify that we are in Outlook.  Even if somebody
            // else invents an "outday" protocol, they would still not run into this since no address
            // is specified after "outday://".
            if (!_fOutlook98 && _cstrUrl.Length() && !_tcscmp(_cstrUrl, _T("outday://")))
                _fOutlook98 = TRUE;

            // If we have a HostUICommandHandler, and the caller did NOT request no-UI, pass it up to the host
            // If we don't have a _pHostUICommandHandler, then hr will remain OLECMDERR_E_NOTSUPPORTED
            if (_pHostUICommandHandler
                && !(nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
                && !_fOutlook98
                )
            {
                hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler,
                                                      nCmdID,
                                                      nCmdexecopt,
                                                      pvarargIn,
                                                      pvarargOut));
            }

            // Only do it ourselves if the host doesn't understand the CGID or the CMDid
            if (FAILED(hr))
            {
                hr = pDocTarget->DoPrint(0, 0, dwUILess,
                                        (pvarargIn && V_ISARRAY(pvarargIn) && V_ISBYREF(pvarargIn)) ?
                                            V_ARRAY(pvarargIn) :
                                            0);
            }

            _fOutlook98 = fOutlook98;

            if ( hr == S_FALSE )
                hr = OLECMDERR_E_CANCELED;
        }

        break;
#endif // NO_PRINT

    case IDM_HELP_CONTENT:
        break;

#ifndef WIN16
    case IDM_HELP_ABOUT:

        ShowMessage(
                &result,
                MB_APPLMODAL | MB_OK,
                0,
                IDS_HELPABOUT_STRING,
                VER_PRODUCTVERSION,
#if DBG==1
                _T("\r\n"), g_achDLLCore
#else
                _T(""), _T("")
#endif
                );

        hr = S_OK;
        break;
#endif // !WIN16

#ifndef NO_EDIT
    case IDM_BROWSEMODE:
        hr = THR_NOTRACE(SetDesignMode(htmlDesignModeOff));
        break;

    case IDM_EDITMODE:
        hr = THR_NOTRACE(pDocTarget->SetDesignMode(htmlDesignModeOn));
        break;
#endif // NO_EDIT

#ifndef NO_SCRIPT_DEBUGGER
    case IDM_BREAKATNEXT:
        hr = THR(g_pDebugApp->CauseBreak());
        break;

    case IDM_LAUNCHDEBUGGER:
        if (_pScriptCollection)
        {
            hr = THR(_pScriptCollection->ViewSourceInDebugger());
        }
        else
        {
            hr = E_UNEXPECTED;
        }
        break;
#endif // NO_SCRIPT_DEBUGGER

#ifndef WINCE

    case IDM_VIEWSOURCE:
        // this is all because the @#$&*% shell team refuses to fix the fact that IDM_VIEWSOURCE 
        // is not overridable by the aggregator via IOleCommandTarget. So the XML Mime viewer has to
        // go inside out and send VIEWPRETRANSFORMSOURCE.
        if (IsAggregatedByXMLMime()) {
            IOleCommandTarget *pIOCT = NULL;
            HRESULT hr = THR(PunkOuter()->QueryInterface(IID_IOleCommandTarget, (void **)&pIOCT));
            if (hr)
                break;
            if (!pIOCT) {
                hr = E_POINTER;
                break;
            }
            hr = THR(pIOCT->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
            ReleaseInterface(pIOCT);
            break;
        }
        // else fall thru
       
    case IDM_VIEWPRETRANSFORMSOURCE:
        // Do nothing for non-HTML files
        if (_fImageFile)
        {
            hr = S_OK;
            break;
        }

#ifndef UNIX
        // If there's a frameset in the body, launch the
        // analyzer dialog.
        if (_fFramesetInBody)
        {
            COptionsHolder *    pcoh            = NULL;
            IDispatch      *    pDispOptions    = NULL;
            TCHAR               achAnalyzeDlg[] = _T("analyze.dlg");

            pcoh = new COptionsHolder(this);
            if (!pcoh)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup_ViewSource;
            }

            // get dispatch from stack variable
            hr = THR_NOTRACE(pcoh->QueryInterface(IID_IHTMLOptionsHolder,
                                 (void**)&pDispOptions));
            if (hr)
                goto Cleanup_ViewSource;

            hr = THR(ShowModalDialogHelper(
                this,
                achAnalyzeDlg,
                pDispOptions, pcoh));
            if (hr)
                goto Cleanup_ViewSource;

Cleanup_ViewSource:
            // release dispatch, et al.
            ReleaseInterface(pcoh);
            ReleaseInterface(pDispOptions);

        }
#endif

        {
            TCHAR   tszPath[MAX_PATH];

            hr = THR(GetViewSourceFileName(tszPath));
            if (hr)
                break;

            InvokeEditor( tszPath );
        }

        break;
#endif // WINCE

#ifndef WIN16
    case IDM_HELP_README:
        HKEY  hkey;
        LONG  lr, lLength;

        lr = RegOpenKey(
                HKEY_CLASSES_ROOT,
                TEXT("CLSID\\{25336920-03F9-11CF-8FD0-00AA00686F13}"),
                &hkey);

        if (lr == ERROR_SUCCESS)
        {
            TCHAR   szPathW[MAX_PATH];

            lLength = sizeof(szPathW);
            lr = RegQueryValue(
                    hkey,
                    TEXT("InprocServer32"),
                    szPathW,
                    &lLength);
            RegCloseKey(hkey);
            if (lr == ERROR_SUCCESS)
            {
                // Right now szPath contains the full path of fm30pad.exe
                // need to replace fm30pad.exe with m3readme.htm
                //
                TCHAR *pch;

                pch = _tcsrchr(szPathW, _T('\\'));
                if (pch)
                    *pch = 0;
                // BUGBUG hardcoded filename string?  -Tomsn
                _tcscat(szPathW, _T("\\readme.htm"));

                // test whether m3readme.htm exists.
                //
                HANDLE hFileReadme;

                hFileReadme = CreateFile(
                        szPathW,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
                if (hFileReadme != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hFileReadme);
                    hr = THR(FollowHyperlink(szPathW));
                    if (!hr)
                        break;
                }
            }
        }
        break;
#endif // ndef WIN16

    case IDM_STOP:
        hr = THR(ExecStop());
        break;

    case IDM_ENABLE_INTERACTION:
        if (!pvarargIn || (pvarargIn->vt != VT_I4))
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            BOOL fEnableInteraction = pvarargIn->lVal;

            if (!!_fEnableInteraction != !!fEnableInteraction)
            {
                CNotification   nf;

                _fEnableInteraction = fEnableInteraction;
                if ( _pUpdateIntSink )
                    // don't bother drawing accumulated inval rgn if minimized
                    _pUpdateIntSink->_pTimer->Freeze( !fEnableInteraction );

                if (_fBroadcastInteraction)
                {
                    BOOL dirtyBefore = !!_lDirtyVersion;
                    nf.EnableInteraction1(PrimaryRoot());
                    BroadcastNotify(&nf);
                    //
                    // BUGBUG ( marka ) - reset erroneous dirtying the document.
                    //
                    if ( ( ! dirtyBefore ) && ( _lDirtyVersion ) )
                        _lDirtyVersion = 0;
                }

                if (TLS(pImgAnim) && !_pDocParent)
                    TLS(pImgAnim)->SetAnimState(
                        (DWORD_PTR) this,
                        fEnableInteraction ? ANIMSTATE_PLAY : ANIMSTATE_PAUSE);
            }
            hr = S_OK;
        }
        break;

    case IDM_ONPERSISTSHORTCUT:
        {
            INamedPropertyBag  *    pINPB = NULL;
            FAVORITES_NOTIFY_INFO   sni;
            CNotification           nf;

            // first put my information into the defualt structure
            // if this is the first call (top level document) then we want to
            // set the base url. for normal pages we are nearly done.  For
            // frameset pages, we need to compare domains for security purposes
            // and establish subdomains if necessary
            if (!pvarargIn ||
                (pvarargIn->vt != VT_UNKNOWN) ||
                !V_UNKNOWN(pvarargIn) )
            {
                hr = E_INVALIDARG;
                break;
            }

            hr = THR_NOTRACE(V_UNKNOWN(pvarargIn)->QueryInterface(IID_INamedPropertyBag,
                                                                  (void **)&pINPB));
            if (hr)
                break;

            hr = THR(PersistFavoritesData(pINPB, (LPCWSTR)_T("DEFAULT")));
            if (hr)
            {
                ReleaseInterface((IUnknown*) pINPB);
                break;
            }

            // initialize the info strucuture
            sni.pINPB = pINPB;
            sni.bstrNameDomain = SysAllocString(_T("DOC"));
            if (sni.bstrNameDomain == NULL)
            {
                ReleaseInterface((IUnknown*) pINPB);
                hr = E_OUTOFMEMORY;
                break;
            }

            // then propogate the event to my children
            nf.FavoritesSave(PrimaryRoot(), &sni);
            BroadcastNotify(&nf);

            ClearInterface(&sni.pINPB);
            SysFreeString(sni.bstrNameDomain);
        }
        break;

    case IDM_REFRESH:
    case IDM_REFRESH_TOP:
    case IDM_REFRESH_TOP_FULL:
    case IDM_REFRESH_THIS:
    case IDM_REFRESH_THIS_FULL:
    {
        LONG lOleCmdidf;

        //
        // Give the container a chance to handle the refresh.
        //

        if (_pHostUICommandHandler)
        {
            hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler, idm, nCmdexecopt, pvarargIn, pvarargOut));
        }

        if (FAILED(hr))
        {
            if (idm != IDM_REFRESH_TOP && idm != IDM_REFRESH_TOP_FULL)
            {
                pDocTarget = this;
            }

            if (idm == IDM_REFRESH)
            {
                if (pvarargIn && pvarargIn->vt == VT_I4)
                    lOleCmdidf = pvarargIn->lVal;
                else
                    lOleCmdidf = OLECMDIDF_REFRESH_NORMAL;
            }
            else if (idm == IDM_REFRESH_TOP_FULL || idm == IDM_REFRESH_THIS_FULL)
            {
                lOleCmdidf = OLECMDIDF_REFRESH_COMPLETELY|OLECMDIDF_REFRESH_PROMPTIFOFFLINE;
            }
            else
            {
                lOleCmdidf = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_PROMPTIFOFFLINE;
            }

            if (pDocTarget->_pPrimaryMarkup)
            {
                hr = GWPostMethodCall(pDocTarget,
                                      ONCALL_METHOD(CDoc, ExecRefreshCallback, execrefreshcallback),
                                      lOleCmdidf, FALSE, "CDoc::ExecRefreshCallback");
            }
            else
            {
                hr = S_OK;
            }
        }

        break;
    }

    case IDM_CONTEXTMENU:
        {
            CMessage Message(
                    InPlace()->_hwnd,
                    WM_CONTEXTMENU,
                    (WPARAM) InPlace()->_hwnd,
                    MAKELPARAM(0xFFFF, 0xFFFF));
            Message.SetNodeHit(_pElemCurrent->GetFirstBranch());
            hr = THR(PumpMessage(&Message, _pElemCurrent->GetFirstBranch()));
        }
        break;

    case IDM_GOBACKWARD:
    case IDM_GOFORWARD:
        hr = THR(pDocTarget->FollowHistory(idm==IDM_GOFORWARD));
        break;

    case IDM_SHDV_SETPENDINGURL:
        if (!pvarargIn || (pvarargIn->vt != VT_BSTR) || (pvarargIn->bstrVal == NULL))
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            hr = SetUrl(pvarargIn->bstrVal);
        }
        break;

#ifdef IE5_ZOOM

    case IDM_ZOOMPERCENT:
        if (pvarargIn && (VT_I4 == V_VT(pvarargIn)))
        {
            int iZoomPercent = V_I4(pvarargIn);

            V_I4(pvarargIn) = MAKELONG(iZoomPercent, 100);

            hr = Exec((GUID *)&CGID_MSHTML,
                    IDM_ZOOMRATIO,
                    MSOCMDEXECOPT_DONTPROMPTUSER,
                    pvarargIn,
                    pvarargOut);
        }
        break;

    case IDM_ZOOMRATIO:

        if (pvarargIn && (VT_I4 == V_VT(pvarargIn)))
        {
            if (_fFrameSet)
            {
                CNotification           nf;
                COnCommandExecParams    param;

                param.pguidCmdGroup = pguidCmdGroup;
                param.nCmdID        = nCmdID;
                param.nCmdexecopt   = nCmdexecopt;
                param.pvarargIn     = pvarargIn;
                param.pvarargOut    = pvarargOut;

                nf.Command(PrimaryRoot(), (void *)&param);
                BroadcastNotify(&nf);

                //  REVIEW (olego 09/15/98)
                //  When zoom will be WYSIWYG we won't need to Relayout
                hr = ForceRelayout();
            }
            else
            {
                int     iZoomRatio;
                long    Numer;
                long    Denom;
                unsigned short u;
                short   s;

                // REVIEW sidda: can we pass multiple parameters to Exec()?

                iZoomRatio = V_I4(pvarargIn);

                // obtain numerator/denominator, taking sign extension into account

                u = LOWORD(iZoomRatio);
                s = u;
                Numer = s;

                u = HIWORD(iZoomRatio);
                s = u;
                Denom = s;

                hr = zoom(Numer, Denom);
            }
        }
        break;

    case IDM_GETZOOMNUMERATOR:
        {
            long Numer;

            get_zoomNumerator(&Numer);

            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = Numer;

            hr = S_OK;
        }
        break;

    case IDM_GETZOOMDENOMINATOR:
        {
            long Denom;

            get_zoomDenominator(&Denom);

            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = Denom;

            hr = S_OK;
        }
        break;

#endif  // IE5_ZOOM

    // JuliaC -- This is hack for InfoViewer's "Font Size" toolbar button
    // For details, please see bug 45627
    case IDM_INFOVIEW_ZOOM:

        if (pvarargIn && (VT_I4 == V_VT(pvarargIn)))
        {
            int iZoom;

            iZoom = V_I4(pvarargIn);

            if (iZoom < (long) BASELINEFONTMIN || iZoom > (long) BASELINEFONTMAX)
            {
                hr = E_INVALIDARG;
                break;
            }

            hr = Exec((GUID *)&CGID_MSHTML,
                    iZoom + IDM_BASELINEFONT1,
                    MSOCMDEXECOPT_DONTPROMPTUSER,
                    NULL,
                    NULL);
            if (hr)
                break;
        }

        if (pvarargOut)
        {
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = (long) _sBaselineFont;
        }

        hr = S_OK;
        break;

    case IDM_INFOVIEW_GETZOOMRANGE:

        V_VT(pvarargOut) = VT_I4;
        V_I4(pvarargOut) = MAKELONG((SHORT)BASELINEFONTMIN, (SHORT)BASELINEFONTMAX);

        hr = S_OK;
        break;
    // End of hack for InfoViewer's "Font Size" toolbar button

    case IDM_BASELINEFONT1:
    case IDM_BASELINEFONT2:
    case IDM_BASELINEFONT3:
    case IDM_BASELINEFONT4:
    case IDM_BASELINEFONT5:
        //
        // depend on that IDM_BASELINEFONT1, IDM_BASELINEFONT2,
        // IDM_BASELINEFONT3, IDM_BASELINEFONT4, IDM_BASELINEFONT5 to be
        // consecutive integers.
        //
        if (_sBaselineFont != (short)(idm - IDM_BASELINEFONT1 + BASELINEFONTMIN))
        {
            // {keyroot}\International\Scripts\{script-id}\IEFontSize

            static TCHAR * s_szScripts = TEXT("\\Scripts");
            const TCHAR * szSubKey = _pOptionSettings->fUseCodePageBasedFontLinking
                                     ? L""
                                     : s_szScripts;

            DWORD dwFontSize = (idm - IDM_BASELINEFONT1 + BASELINEFONTMIN);
            TCHAR *pchPath, *pch;
            int cch0 = _tcslen(_pOptionSettings->achKeyPath);
            int cch1 = _tcslen(s_szPathInternational);
            int cch2 = _tcslen(szSubKey);
            
            pchPath = pch = new TCHAR[cch0 + cch1 + cch2 + 1 + 10 + 1];

            if (pchPath)
            {
                ULONG ulArg = _pOptionSettings->fUseCodePageBasedFontLinking
                              ? ULONG(_pCodepageSettings->uiFamilyCodePage)
                              : ULONG(RegistryAppropriateSidFromSid(DefaultSidForCodePage(_pCodepageSettings->uiFamilyCodePage)));
                
                StrCpy( pch, _pOptionSettings->achKeyPath );
                pch += cch0;
                StrCpy( pch, s_szPathInternational );
                pch += cch1;
                StrCpy( pch, szSubKey );
                pch += cch2;
                *pch++ = _T('\\');
                _ultot(ulArg, pch, 10);

                IGNORE_HR( SHSetValue(HKEY_CURRENT_USER, pchPath, TEXT("IEFontSize"),
                                      REG_BINARY, (void *)&dwFontSize, sizeof(dwFontSize)) );

                delete [] pchPath;
            }           
        }

        _sBaselineFont = _pCodepageSettings->sBaselineFontDefault =
                    (short)(idm - IDM_BASELINEFONT1 + BASELINEFONTMIN);

#ifdef UNIX
        g_SelectedFontSize = _sBaselineFont; // save the selected font size for new CDoc.
#endif


        EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);
        ForceRelayout();

        {   // update font history version number
            THREADSTATE * pts = GetThreadState();
            pts->_iFontHistoryVersion++;        
        }

        // Send this command to our children
        {
            COnCommandExecParams cmdExecParams;
            cmdExecParams.pguidCmdGroup = pguidCmdGroup;
            cmdExecParams.nCmdID        = nCmdID;
            CNotification   nf;

            nf.Command(PrimaryRoot(), &cmdExecParams);
            BroadcastNotify(&nf);
        }

        //BUGWIN16: No CGID_ExplorerBarDoc !!
#ifndef WIN16
        //
        // tell shell to apply this exec to applicable explorer bars
        //
        IGNORE_HR(CTExec(
                pDocTarget->_pInPlace ?
                    (IUnknown *)pDocTarget->_pInPlace->_pInPlaceSite : (IUnknown *)pDocTarget->_pClientSite,
                &CGID_ExplorerBarDoc, nCmdID, 0, 0, 0));
#endif

        hr             = S_OK;
        break;

    // Complex Text for setting default document reading order
    case IDM_DIRLTR:
        hr = SetDocDirection(htmlDirLeftToRight);
        break;

    case IDM_DIRRTL:
        hr = SetDocDirection(htmlDirRightToLeft);
        break;

    case IDM_SHDV_MIMECSETMENUOPEN:
        if (pvarargIn)
        {
            int nIdm;
            CODEPAGE cp = GetCodePage();
            BOOL fDocRTL;
            Assert(pvarargIn->vt == VT_I4);

            hr = THR(GetDocDirection(&fDocRTL));
            if (hr == S_OK)
            {
                hr = THR(ShowMimeCSetMenu(_pOptionSettings, &nIdm, cp,
                                           pvarargIn->lVal,
                                           fDocRTL, IsCpAutoDetect()));

                if (hr == S_OK)
                {
                    if (nIdm >= IDM_MIMECSET__FIRST__ && nIdm <= IDM_MIMECSET__LAST__)
                    {
                        idm = nIdm;     // handled below
                    }
                    else if (nIdm == IDM_DIRLTR || nIdm == IDM_DIRRTL)
                    {
                        Exec((GUID *)&CGID_MSHTML, nIdm, 0, NULL, NULL);
                    }

                }
            }
        }
        break;

    case IDM_SHDV_FONTMENUOPEN:
        if (pvarargIn)
        {
            int nIdm;
            Assert(pvarargIn->vt == VT_I4);

            hr = THR(ShowFontSizeMenu(&nIdm, _sBaselineFont,
                                       pvarargIn->lVal));

            if (hr == S_OK)
            {
                if ( (nIdm >= IDM_BASELINEFONT1 && nIdm <= IDM_BASELINEFONT5) )
                {
                    Exec((GUID *)&CGID_MSHTML, nIdm, 0, NULL, NULL);
                }
            }
        }
        break;

    case IDM_SHDV_GETMIMECSETMENU:
        if (pvarargOut)
        {
            BOOL fDocRTL;

            V_VT(pvarargOut) = VT_I4;
            hr = THR(GetDocDirection(&fDocRTL));
            if (hr == S_OK)
            {
                V_I4(pvarargOut) = HandleToLong(GetEncodingMenu(_pOptionSettings, GetCodePage(), fDocRTL, IsCpAutoDetect()));

                hr = V_I4(pvarargOut)? S_OK: S_FALSE;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_SHDV_GETFONTMENU:
        if (pvarargOut)
        {
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = HandleToLong(GetFontSizeMenu(_sBaselineFont));

            hr = V_I4(pvarargOut)? S_OK: S_FALSE;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_SHDV_GETDOCDIRMENU:
        if (pvarargOut)
        {
            BOOL fDocRTL;

            hr = THR(GetDocDirection(&fDocRTL));

            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = HandleToLong(GetOrAppendDocDirMenu(GetCodePage(), fDocRTL));
            hr = V_I4(pvarargOut)? S_OK: OLECMDERR_E_DISABLED;
        }
        else
        {
            hr = E_INVALIDARG;
        }
         break;

    case IDM_SHDV_DOCCHARSET:
    case IDM_SHDV_DOCFAMILYCHARSET:
        // Return the family or actual charset for the doc
        if (pvarargOut)
        {
            UINT uiCodePage = idm == IDM_SHDV_DOCFAMILYCHARSET ?
                              WindowsCodePageFromCodePage(GetCodePage()) :
                              GetCodePage();

            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = uiCodePage;

            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
        break;

    case IDM_SHDV_GETFRAMEZONE:
        if (!pvarargOut)
        {
            hr = E_POINTER;
        }
        else
        {
            //
            // First get our own zone.  Then do broadcast to get zones
            // from child frames.
            //

            hr = THR(GetFrameZone(pvarargOut));
            if (OK(hr) && _fHasOleSite)
            {
                CElement * pPrimaryClient = GetPrimaryElementClient();
                if(pPrimaryClient)
                {
                    CNotification   nf;

                    nf.GetFrameZone(pPrimaryClient, (void *)pvarargOut);
                    BroadcastNotify(&nf);
                }
            }

            if (hr)
            {
                V_VT(pvarargOut) = VT_EMPTY;
            }
        }
        break;
        // Support for Context Menu Extensions
    case IDM_SHDV_ADDMENUEXTENSIONS:
        {
            if (   !pvarargIn  || (pvarargIn->vt  != VT_I4)
                || !pvarargOut || (pvarargOut->vt != VT_I4)
                )
            {
                Assert(pvarargIn);
                hr = E_INVALIDARG;
            }
            else
            {
                HMENU hmenu = (HMENU)LongToHandle(V_I4(pvarargIn));
                int   id    = V_I4(pvarargOut);
                hr = THR(InsertMenuExt(hmenu, id));
            }
        }
        break;
    case IDM_RUNURLSCRIPT:
        // This enables us to run scripts inside urls on the
        // current document.  The Variant In parameter is an URL
        if (pvarargIn->vt == VT_BSTR)
        {
            // get dispatch for the main window
            //
            hr = THR(EnsureOmWindow());
            if (SUCCEEDED (hr))
            {
                IDispatch      * pDispWindow=NULL;
                pDispWindow = (IHTMLWindow2*)(_pOmWindow->Window());

                // bring up the dialog
                //
                hr = THR(ShowModalDialogHelper(
                        this,
                        pvarargIn->bstrVal,
                        pDispWindow,
                        NULL,
                        NULL,
                        HTMLDLG_NOUI | HTMLDLG_AUTOEXIT));
            }
        }
        break;
    case IDM_HTMLEDITMODE:
        if (!pvarargIn || (pvarargIn->vt != VT_BOOL))
        {
            Assert(pvarargIn);
            hr = E_INVALIDARG;
        }
        else
        {
            GUID guidCmdGroup = CGID_MSHTML;
            _fInHTMLEditMode = !!V_BOOL(pvarargIn);

            IGNORE_HR(Exec(&guidCmdGroup, IDM_COMPOSESETTINGS, 0, pvarargIn, NULL));
            hr = S_OK;
        }
        break;

    case IDM_REGISTRYREFRESH:
        IGNORE_HR(OnSettingsChange());
        break;

    case IDM_DEFAULTBLOCK:
        if (pvarargIn)
        {
            hr = THR(SetupDefaultBlockTag(pvarargIn));
            if (S_OK == hr)
            {
                CNotification   nf;

                nf.EditModeChange(GetPrimaryElementTop());
                BroadcastNotify(&nf);
            }
        }
        if (pvarargOut)
        {
            V_VT(pvarargOut) = VT_BSTR;
            if (GetDefaultBlockTag() == ETAG_DIV)
                V_BSTR(pvarargOut) = SysAllocString(_T("DIV"));
            else
                V_BSTR(pvarargOut) = SysAllocString(_T("P"));
            hr = S_OK;
        }

        break;

    case OLECMDID_ONUNLOAD:
        {
            BOOL fRetval = _pOmWindow ? _pOmWindow->Fire_onbeforeunload() : TRUE;
            hr = S_OK;
            if (pvarargOut)
            {
               V_VT  (pvarargOut) = VT_BOOL;
               V_BOOL(pvarargOut) = VARIANT_BOOL_FROM_BOOL(fRetval);
            }
        }
        break;

    case OLECMDID_DONTDOWNLOADCSS:
        {
            CDoc *pDoc = GetRootDoc();

            if (pDoc->DesignMode())
                pDoc->_fDontDownloadCSS = TRUE;
            hr = S_OK;
        }
        break;

    case IDM_GETBYTESDOWNLOADED:
        if (!pvarargOut)
        {
            hr = E_POINTER;
        }
        else
        {
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = _pDwnDoc ? _pDwnDoc->GetBytesRead() : 0;
        }
        break;

    case IDM_PERSISTSTREAMSYNC:
        _fPersistStreamSync = TRUE;
        hr = S_OK;
        break;

    case IDM_SHOWZEROBORDERATDESIGNTIME:
    case IDM_NOFIXUPURLSONPASTE:
        {
            CElement *pElement = GetPrimaryElementClient();
            BOOL fSet;

            if (!pvarargIn || pvarargIn->vt != VT_BOOL)
            {
                //
                // If they give us junk, just toggle the flag.
                //
                fSet = ! _fShowZeroBorderAtDesignTime;
            }
            else
            {
                fSet = ENSURE_BOOL(pvarargIn->bVal);
            }

            if (idm == IDM_SHOWZEROBORDERATDESIGNTIME)
            {
                _fShowZeroBorderAtDesignTime = fSet;
                if (fSet)
                {
                    _view.SetFlag(CView::VF_ZEROBORDER);
                }
                else
                {
                    _view.ClearFlag(CView::VF_ZEROBORDER);
                }

                CNotification nf;
                nf.ZeroGrayChange(GetPrimaryElementTop());
                BroadcastNotify( & nf );
                
                Invalidate();
                hr = S_OK;
            }
            if( idm == IDM_NOFIXUPURLSONPASTE )
            {
                hr = S_OK;
                _fNoFixupURLsOnPaste = fSet;
            }
                _pOptionSettings->dwMiscFlags = _dwMiscFlags();

            //
            // BUGBUG marka - is this supposed to be doing an invalidate ?
            //
            if (pElement)
                pElement->ResizeElement(NFLAGS_FORCE);

            // Send this command to our children
            {
                COnCommandExecParams cmdExecParams;
                cmdExecParams.pguidCmdGroup = pguidCmdGroup;
                cmdExecParams.nCmdID        = nCmdID;
                CNotification   nf;

                nf.Command(PrimaryRoot(), &cmdExecParams);
                BroadcastNotify(&nf);
            }
            break;
        }

    case IDM_SHOWALLTAGS:
    case IDM_SHOWALIGNEDSITETAGS:
    case IDM_SHOWSCRIPTTAGS:
    case IDM_SHOWSTYLETAGS:
    case IDM_SHOWCOMMENTTAGS:
    case IDM_SHOWAREATAGS:
    case IDM_SHOWUNKNOWNTAGS:
    case IDM_SHOWMISCTAGS:
    case IDM_SHOWWBRTAGS:
       {
            //
            //  TODO: cleanup these flags [ashrafm]
            //
            CVariant var;
            BOOL fSet = FALSE;

            if (pvarargIn)
            {
                if(pvarargIn->vt != VT_BOOL)
                    break;                
                fSet = ENSURE_BOOL(V_BOOL(pvarargIn));
            }
            else
            {
                switch(idm)
                {
                    case IDM_SHOWALIGNEDSITETAGS:
                        fSet = !_fShowAlignedSiteTags; break;
                    case IDM_SHOWSCRIPTTAGS:
                        fSet = !_fShowScriptTags; break;
                    case IDM_SHOWSTYLETAGS:
                        fSet = !_fShowStyleTags; break;
                    case IDM_SHOWCOMMENTTAGS:
                        fSet = !_fShowCommentTags; break;
                    case IDM_SHOWAREATAGS:
                        fSet = !_fShowAreaTags; break;
                    case IDM_SHOWMISCTAGS:
                        fSet = !_fShowMiscTags; break;
                    case IDM_SHOWUNKNOWNTAGS:
                        fSet = !_fShowUnknownTags; break;
                    case IDM_SHOWWBRTAGS:
                        fSet = !_fShowWbrTags; break;
                    case IDM_SHOWALLTAGS:
                        fSet = !(_fShowWbrTags && _fShowUnknownTags && _fShowMiscTags &&
                                    _fShowAreaTags && _fShowCommentTags && _fShowStyleTags &&
                                    _fShowScriptTags && _fShowAlignedSiteTags);
                        break;
                    default:Assert(0);
                }
                V_VT(&var) = VT_BOOL;
                V_BOOL(&var) = VARIANT_BOOL_FROM_BOOL(fSet);

                pvarargIn = &var;
            }

            //
            // HACKHACK: EnsureGlyphTableExistsAndExecute should be able to delete from the table
            //
            if (!fSet && (idm == IDM_SHOWALLTAGS || idm == IDM_SHOWMISCTAGS))
            {
                // Empty the glyph table
                idm = IDM_EMPTYGLYPHTABLE;
            }

            hr = EnsureGlyphTableExistsAndExecute(
                    pguidCmdGroup, idm, nCmdexecopt,pvarargIn, pvarargOut);
            if (hr)
                break;

            if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWALIGNEDSITETAGS)
                _fShowAlignedSiteTags = fSet;
            if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWSCRIPTTAGS)
                _fShowScriptTags = fSet;
            if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWSTYLETAGS)
                _fShowStyleTags =  fSet;
            if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWCOMMENTTAGS)
                _fShowCommentTags = fSet;
            if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWAREATAGS)
                _fShowAreaTags = fSet;
            if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWMISCTAGS)
                _fShowMiscTags = fSet;
            if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWUNKNOWNTAGS)
                _fShowUnknownTags = fSet;
            if(idm == IDM_SHOWALLTAGS || idm == IDM_SHOWWBRTAGS)
                _fShowWbrTags = fSet;
            break;
        }

    case IDM_ADDTOGLYPHTABLE:
    case IDM_REMOVEFROMGLYPHTABLE:
    case IDM_REPLACEGLYPHCONTENTS:
        {
            if (!pvarargIn->bstrVal)
                break;

            hr = EnsureGlyphTableExistsAndExecute(
                    pguidCmdGroup, idm, nCmdexecopt,pvarargIn, pvarargOut);
            break;
        }
    case IDM_EMPTYGLYPHTABLE:
        {
            hr = EnsureGlyphTableExistsAndExecute(
                    pguidCmdGroup, idm, nCmdexecopt,pvarargIn, pvarargOut);
            break;
        }
    case IDM_NOACTIVATENORMALOLECONTROLS:
    case IDM_NOACTIVATEDESIGNTIMECONTROLS:
    case IDM_NOACTIVATEJAVAAPPLETS:
        {
            if (!pvarargIn || pvarargIn->vt != VT_BOOL)
                break;

            BOOL fSet = ENSURE_BOOL(pvarargIn->bVal);

            if (idm == IDM_NOACTIVATENORMALOLECONTROLS)
                _fNoActivateNormalOleControls = fSet;
            else if (idm == IDM_NOACTIVATEDESIGNTIMECONTROLS)
                _fNoActivateDesignTimeControls = fSet;
            else
                _fNoActivateJavaApplets = fSet;

            _pOptionSettings->dwMiscFlags = _dwMiscFlags();

            hr = S_OK;
            break;
        }

    case IDM_SETDIRTY:
        if (!pvarargIn || pvarargIn->vt != VT_BOOL)
        {
            hr = E_INVALIDARG;
            break;
        }

        hr = THR(SetDirtyFlag(ENSURE_BOOL(pvarargIn->bVal)));
        break;

    case IDM_PRESERVEUNDOALWAYS:
        if (pvarargIn && pvarargIn->vt == VT_BOOL)
        {
            TLS( fAllowParentLessPropChanges ) = pvarargIn->boolVal;

            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_WAITFORRECALC:
    {
        // This private command is issued by MSHTMPAD to force background recalc to
        // finish so that accurate timings can be measured.

        WaitForRecalc();
        hr = S_OK;
        break;
    }

    case IDM_GETSWITCHTIMERS:
    {
#ifdef SWITCHTIMERS_ENABLED
        // This private command is issued by MSHTMPAD to collect detailed timing information.
        void AnsiToWideTrivial(const CHAR * pchA, WCHAR * pchW, LONG cch);
        char ach[256];
        SwitchesGetTimers(ach);
        AnsiToWideTrivial(ach, pvarargOut->bstrVal, lstrlenA(ach));
#endif
        hr = S_OK;
        break;
    }

    case IDM_DWNH_SETDOWNLOAD:
        if (pvarargIn && pvarargIn->vt == VT_UNKNOWN)
        {
            hr = THR(SetDownloadNotify(pvarargIn->punkVal));
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;

    case IDM_SAVEPICTURE:
        if (!_pMenuObject)
        {
            //
            // only do work here if there is no menu object, and there an
            // image on this document.  This is here to handle saveAs on a
            // file://c:/temp/foo.gif type document url.  If for some reason
            // this IDM comes through on a non-image document, we will try to
            // save the first image in the images collection instead.  If there
            // isn't one, then we fail,
            //
            CElement          * pImg = NULL;
            CCollectionCache  * pCollectionCache = NULL;

            hr = THR(PrimaryMarkup()->EnsureCollectionCache(CMarkup::IMAGES_COLLECTION));
            if (!hr)
            {
                pCollectionCache = PrimaryMarkup()->CollectionCache();

                hr = THR(pCollectionCache->GetIntoAry(CMarkup::IMAGES_COLLECTION, 0, &pImg));
                if (!hr)
                {
                    Assert(pImg);

                    hr = THR_NOTRACE(pImg->Exec(pguidCmdGroup,
                                        nCmdID,
                                        nCmdexecopt,
                                        pvarargIn,
                                        pvarargOut));
                }
            }

            if (hr)
                hr = OLECMDERR_E_NOTSUPPORTED;
        }
        break;
    case IDM_SAVEPRETRANSFORMSOURCE:
        if (!pvarargIn || (pvarargIn->vt != VT_BSTR) || !V_BSTR(pvarargIn) )     
            hr = E_INVALIDARG;
        else
        {            
            hr = SavePretransformedSource(V_BSTR(pvarargIn));
        }
        break;
    }

    if(FAILED(hr) && hr != OLECMDERR_E_NOTSUPPORTED)
        goto Cleanup;

#ifndef NO_MULTILANG
    if( idm >= IDM_MIMECSET__FIRST__ && idm <= IDM_MIMECSET__LAST__)
    {
        CODEPAGE cp = GetCodePageFromMenuID(idm);
        THREADSTATE * pts = GetThreadState();

        // assigning IDM_MIMECSET__LAST__ to CpAutoDetect mode
        if ( cp == CP_UNDEFINED && idm == IDM_MIMECSET__LAST__ )
        {
            SetCpAutoDetect(!IsCpAutoDetect());
            _fCodepageOverridden = FALSE;

            if (IsCpAutoDetect() && g_pMultiLanguage2)
            {
                // we need the same refreshing effect as the regular cp
                cp = CP_AUTO;
            }
        }

        // NB (cthrash) ValidateCodePage allows us to JIT download a language pack.
        // If we don't have IMultiLanguage2, JIT downloadable codepages will not
        // appear in the language menu (ie only codepages currently available on
        // the system will be provided as options) and thus ValidateCodePage
        // is not required.

        if (   CP_UNDEFINED != cp
#ifndef UNIX
            && S_OK == MlangValidateCodePage(this, cp, _pInPlace->_hwnd, TRUE)
#endif
            )
        {
            CNotification   nf;

            nf.Initialize(
                NTYPE_SET_CODEPAGE,
                PrimaryRoot(),
                PrimaryRoot()->GetFirstBranch(),
                (void *)(UINT_PTR)cp,
                0);

            // if AutoDetect mode is on, we don't make a change
            // to the default codepage for the document
            if (!IsCpAutoDetect() && !HaveCodePageMetaTag())
            {
                // [review]
                // here we save the current setting to the registry
                // we should find better timing to do it
                SaveDefaultCodepage(cp);
            }
            _fCodepageOverridden = TRUE;

            BroadcastNotify(&nf);
            SwitchCodePage(cp);

            // Bubble down code page to nested documents
            BubbleDownCodePage(cp);

            IGNORE_HR(ExecRefresh());
        }
        pts->_iFontHistoryVersion++;
    }
#endif // !NO_MULTLANG

    if (hr == OLECMDERR_E_NOTSUPPORTED)
    {
        ctarg.pguidCmdGroup = pguidCmdGroup;
        ctarg.fQueryStatus = FALSE;
        ctarg.pexecArg = &execarg;
        execarg.nCmdID = nCmdID;
        execarg.nCmdexecopt = nCmdexecopt;
        execarg.pvarargIn = pvarargIn;
        execarg.pvarargOut = pvarargOut;
        
        if (_pMenuObject)
        {
            hr = THR_NOTRACE(RouteCTElement(_pMenuObject, &ctarg));
        }
        
        if (hr == OLECMDERR_E_NOTSUPPORTED && _pElemCurrent)
        {
            hr = THR_NOTRACE(RouteCTElement(_pElemCurrent, &ctarg));
        }
    }
    
    if (hr == OLECMDERR_E_NOTSUPPORTED)
    {
        hr = THR_NOTRACE( _EditRouter.ExecEditCommand(pguidCmdGroup,
                nCmdID, nCmdexecopt,
                pvarargIn, pvarargOut,
                (IUnknown *)(IPrivateUnknown *)this,
                this ) );
    }

#ifndef NO_EDIT
    CloseParentUnit(pCPUU, S_OK);
#endif // NO_EDIT
    Assert(TestLock(SERVERLOCK_STABILIZED));

    if (!hr && (!pvarargOut || pvarargIn))
        DeferUpdateUI();

    //  These are the legitimate error codes from IOleCommandtarget::Exec
    //  Everything else is forced to S_OK
    if ( hr != OLECMDERR_E_NOTSUPPORTED &&
         hr != OLECMDERR_E_DISABLED     &&
         hr != OLECMDERR_E_UNKNOWNGROUP &&
         hr != OLECMDERR_E_CANCELED     &&
         hr != OLECMDERR_E_NOHELP           )
    {
        if(!ShowLastErrorInfo(hr))
            hr = S_OK;
    }
Cleanup:
    SRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CDoc::OnContextMenuExt
//
//  Synopsis:   Handle launching the dialog when a ContextMenuExt
//              command is received
//
//--------------------------------------------------------------------
HRESULT
CDoc::OnContextMenuExt(UINT idm, VARIANTARG * pvarargIn)
{
    HRESULT          hr = E_FAIL;
    IDispatch      * pDispWindow=NULL;
    CParentUndoUnit* pCPUU = NULL;
    unsigned int     nExts;
    CONTEXTMENUEXT * pCME;

    Assert(idm >= IDM_MENUEXT_FIRST__ && idm <= IDM_MENUEXT_LAST__);

    // find the html to run
    //
    nExts = _pOptionSettings->aryContextMenuExts.Size();
    Assert((idm - IDM_MENUEXT_FIRST__) < nExts);
    pCME = _pOptionSettings->
                aryContextMenuExts[idm - IDM_MENUEXT_FIRST__];
    Assert(pCME);

    // Undo stuff
    //
    pCPUU = OpenParentUnit(this, IDS_CANTUNDO);

    // get dispatch for the main window
    //
    hr = THR(EnsureOmWindow());
    if (hr)
        goto Cleanup;

    pDispWindow = (IHTMLWindow2*)(_pOmWindow->Window());

    // bring up the dialog
    //
    hr = THR(ShowModalDialogHelper(
            this,
            pCME->cstrActionUrl,
            pDispWindow,
            NULL,
            NULL,
            (pCME->dwFlags & MENUEXT_SHOWDIALOG)
                            ? 0 : (HTMLDLG_NOUI | HTMLDLG_AUTOEXIT)));

    if (pCPUU)
    {
        IGNORE_HR(CloseParentUnit(pCPUU, hr));
    }

Cleanup:
    RRETURN(hr);
}


void
CDoc::WaitForRecalc()
{
    PerfDbgLog(tagPerfWatch, this, "+CDoc::WaitForRecalc");

    CElement *  pElement = GetPrimaryElementClient();

    if (_view.HasLayoutTask())
    {
        PerfDbgLog(tagPerfWatch, this, "CDoc::WaitForRecalc (EnsureView)");
        _view.EnsureView(LAYOUT_DEFERPAINT);
    }

    if (pElement)
    {
        PerfDbgLog(tagPerfWatch, this, "CDoc::WaitForRecalc (Body/Frame WaitForRecalc)");
        if (pElement->Tag() == ETAG_BODY)
            ((CBodyElement *)pElement)->WaitForRecalc();
        else if (pElement->Tag() == ETAG_FRAMESET)
            ((CFrameSetSite *)pElement)->WaitForRecalc();
    }

    if (_view.HasLayoutTask())
    {
        PerfDbgLog(tagPerfWatch, this, "CDoc::WaitForRecalc (EnsureView)");
        _view.EnsureView(LAYOUT_DEFERPAINT);
    }

    PerfDbgLog(tagPerfWatch, this, "CDoc::WaitForRecalc (UpdateForm)");
    UpdateForm();

    PerfDbgLog(tagPerfWatch, this, "-CDoc::WaitForRecalc");
}

//+---------------------------------------------------------------------------
//
// Member: AddToFavorites
//
//----------------------------------------------------------------------------
HRESULT
CDoc::AddToFavorites(TCHAR * pszURL, TCHAR * pszTitle)
{
#if defined(WIN16) || defined(WINCE)
    return S_FALSE;
#else
    VARIANTARG varURL;
    VARIANTARG varTitle;
    IUnknown * pUnk;
    HRESULT    hr;

    varURL.vt        = VT_BSTR;
    varURL.bstrVal   = pszURL;
    varTitle.vt      = VT_BSTR;
    varTitle.bstrVal = pszTitle;

    if (_pInPlace && _pInPlace->_pInPlaceSite)
    {
        pUnk = _pInPlace->_pInPlaceSite;
    }
    else
    {
        pUnk = _pClientSite;
    }

    hr = THR(CTExec(
            pUnk,
            &CGID_Explorer,
            SBCMDID_ADDTOFAVORITES,
            MSOCMDEXECOPT_PROMPTUSER,
            &varURL,
            &varTitle));

    RRETURN(hr);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandSupported
//
//  Synopsis:
//
//  Returns: returns true if given command (like bold) is supported
//----------------------------------------------------------------------------

HRESULT
CDoc::queryCommandSupported(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(super::queryCommandSupported(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandEnabled
//
//  Synopsis:
//
//  Returns: returns true if given command is currently enabled. For toolbar
//          buttons not being enabled means being grayed.
//----------------------------------------------------------------------------

HRESULT
CDoc::queryCommandEnabled(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(super::queryCommandEnabled(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandState
//
//  Synopsis:
//
//  Returns: returns true if given command is on. For toolbar buttons this
//          means being down. Note that a command button can be disabled
//          and also be down.
//----------------------------------------------------------------------------

HRESULT
CDoc::queryCommandState(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(super::queryCommandState(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandIndeterm
//
//  Synopsis:
//
//  Returns: returns true if given command is in indetermined state.
//          If this value is TRUE the value returnd by queryCommandState
//          should be ignored.
//----------------------------------------------------------------------------

HRESULT
CDoc::queryCommandIndeterm(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(super::queryCommandIndeterm(bstrCmdId, pfRet));
}



//+---------------------------------------------------------------------------
//
//  Member:     queryCommandText
//
//  Synopsis:
//
//  Returns: Returns the text that describes the command (eg bold)
//----------------------------------------------------------------------------

HRESULT
CDoc::queryCommandText(BSTR bstrCmdId, BSTR *pcmdText)
{
    RRETURN(super::queryCommandText(bstrCmdId, pcmdText));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandValue
//
//  Synopsis:
//
//  Returns: Returns the  command value like font name or size.
//----------------------------------------------------------------------------

HRESULT
CDoc::queryCommandValue(BSTR bstrCmdId, VARIANT *pvarRet)
{
    RRETURN(super::queryCommandValue(bstrCmdId, pvarRet));
}



//+---------------------------------------------------------------------------
//
//  Member:     execCommand
//
//  Synopsis:   Executes given command
//
//  Returns:
//----------------------------------------------------------------------------

HRESULT
CDoc::execCommand(BSTR bstrCmdId, VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    HRESULT hr;
    CDoc::CLock Lock(this, FORMLOCK_QSEXECCMD);
    BOOL fAllow;

    hr = THR(AllowClipboardAccess(bstrCmdId, &fAllow));
    if (hr || !fAllow)
        goto Cleanup;           // Fail silently

    hr = THR( super::execCommand( bstrCmdId, showUI, value ) );

    if (pfRet)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}


//+---------------------------------------------------------------------------
//
//  Member:     execCommandShowHelp
//
//  Synopsis:
//
//  Returns:
//----------------------------------------------------------------------------

HRESULT
CDoc::execCommandShowHelp(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    HRESULT hr;

    hr = THR(super::execCommandShowHelp(bstrCmdId));

    if(pfRet != NULL)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::elementFromPoint, public
//
//  Synopsis:   Returns the element under the given point.
//
//  Arguments:  [x]         -- x position
//              [y]         -- y position
//              [ppElement] -- Place to return element.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::elementFromPoint(long x, long y, IHTMLElement **ppElement)
{
    HRESULT    hr = S_OK;
    POINT      pt = { x, y };
    CTreeNode *pNode;
    HTC        htc;
    CMessage   msg;

    msg.pt = pt;

    if (!ppElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *ppElement = NULL;

    htc = HitTestPoint(&msg, &pNode, HT_VIRTUALHITTEST);

    if (htc != HTC_NO)
    {
        Assert( pNode->Tag() != ETAG_ROOT );

        if (pNode->Tag() == ETAG_TXTSLAVE)
        {
            Assert(pNode->Element()->MarkupMaster());
            pNode = pNode->Element()->MarkupMaster()->GetFirstBranch();
            Assert(pNode);
        }

        if (pNode->Tag() == ETAG_IMG)
        {
            // did we hit an area?
            pNode->Element()->SubDivisionFromPt(msg.ptContent, &msg.lSubDivision);
            if (msg.lSubDivision >=0)
            {
                // hit was on an area in an img, return the area.
                CAreaElement * pArea = NULL;
                CImgElement  * pImg = DYNCAST(CImgElement, pNode->Element());

                Assert(pImg->GetMap());

                // if we can get a node for the area, then return it
                // otherwise default back to returning the element.
                pImg->GetMap()->GetAreaContaining(msg.lSubDivision, &pArea);
                if (pArea)
                {
                    pNode = pArea->GetFirstBranch();
                    Assert(pNode);
                }
            }
        }

        hr = THR( pNode->GetElementInterface( IID_IHTMLElement, (void **) ppElement ) );
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
// Member: AddPages (IShellPropSheetExt interface)
//
// Add Internet Property Sheets
//
//-----------------------------------------------------------------------------
typedef struct tagIEPROPPAGEINFO
{
    UINT  cbSize;
    DWORD dwFlags;
    LPSTR pszCurrentURL;
    DWORD dwRestrictMask;
    DWORD dwRestrictFlags;
} IEPROPPAGEINFO, *LPIEPROPPAGEINFO;

DYNLIB  s_dynlibINETCPL = { NULL, NULL, "inetcpl.cpl" };

DYNPROC s_dynprocAddInternetPropertySheets =
        { NULL, & s_dynlibINETCPL, "AddInternetPropertySheetsEx" };

#ifndef WIN16
HRESULT
CDoc::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    TCHAR  achFile[MAX_PATH];
    ULONG  cchFile = ARRAY_SIZE(achFile);
    LPTSTR pszURLW;
    char   szURL[1024];
    LPSTR  pszURL = NULL;

    HRESULT hr;

    if (_cstrUrl && GetUrlScheme(_cstrUrl) == URL_SCHEME_FILE)
    {
        hr = THR(PathCreateFromUrl(_cstrUrl, achFile, &cchFile, 0));
        if (hr)
            goto Cleanup;

        pszURLW = achFile;
    }
    else
    {
        pszURLW = _cstrUrl;
    }

    if (pszURLW)
    {
        pszURL = szURL;
        WideCharToMultiByte(CP_ACP, 0, pszURLW, -1, pszURL, sizeof(szURL), NULL, NULL);
    }

    hr = THR(LoadProcedure(& s_dynprocAddInternetPropertySheets));
    if (hr)
        goto Cleanup;

    IEPROPPAGEINFO iepi;

    iepi.cbSize = sizeof(iepi);
    iepi.dwFlags = (DWORD)-1;
    iepi.pszCurrentURL = pszURL;
    iepi.dwRestrictMask = 0;    // turn off all mask bits

    hr = THR((*(HRESULT (WINAPI *)
                    (LPFNADDPROPSHEETPAGE,
                     LPARAM,
                     PUINT,
                     LPFNPSPCALLBACK,
                     LPIEPROPPAGEINFO))
             s_dynprocAddInternetPropertySheets.pfn)
                     (lpfnAddPage, lParam, NULL, NULL, &iepi));

Cleanup:
    if (hr)
        hr = E_FAIL;
    RRETURN (hr);
}
#endif // !WIN16



HRESULT
CDoc::toString(BSTR* String)
{
    RRETURN(super::toString(String));
};


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetupDefaultBlockTag(VARIANTARG pvarargIn)
//
//  Synopsis:   This function parses the string coming in and sets up the
//              default composition font.
//
//  Params:     [vargIn]: A BSTR, either "P" or "DIV"
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDoc::SetupDefaultBlockTag(VARIANTARG *pvarargIn)
{
    HRESULT hr = E_INVALIDARG;
    BSTR pstr;

    //
    // If its not a BSTR, do nothing.
    //
    if (V_VT(pvarargIn) != VT_BSTR)
        goto Cleanup;

    // Get the string
    pstr = V_BSTR(pvarargIn);

    if (!StrCmpC (pstr, _T("DIV")))
    {
        SetDefaultBlockTag(ETAG_DIV);
    }
    else if (!StrCmpC (pstr, _T("P")))
    {
        SetDefaultBlockTag(ETAG_P);
    }
    else
    {
        SetDefaultBlockTag(ETAG_P);
        AssertSz(0, "Unexpected type");
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    RRETURN(hr);
}


HRESULT
CDoc::EnsureGlyphTableExistsAndExecute (
        GUID * pguidCmdGroup,
        UINT idm,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT hr = S_OK;

    if (idm == IDM_EMPTYGLYPHTABLE)
    {
        delete _pGlyphTable;
        _pGlyphTable = NULL;
        hr = THR(ForceRelayout());
        RRETURN(hr);
    }

    if (!_pGlyphTable)
    {
        _pGlyphTable = new CGlyph (this);
        if (!_pGlyphTable)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = _pGlyphTable->Init();
        if (hr)
            goto Cleanup;
    }
    hr = _pGlyphTable->Exec(
        pguidCmdGroup, idm, nCmdexecopt,pvarargIn, pvarargOut);

Cleanup:
    RRETURN (hr);
}
