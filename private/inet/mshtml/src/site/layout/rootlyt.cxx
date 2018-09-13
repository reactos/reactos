//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       rootlyt.cxx
//
//  Contents:   Implementation of CRootLayout
//
//  Classes:    CRootLayout
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifdef NEVER

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_ROOTLYT_HXX_
#define X_ROOTLYT_HXX_
#include "rootlyt.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx" // needed for EVENTPARAM
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPGRP_HXX_
#define X_DISPGRP_HXX_
#include "dispgrp.hxx"
#endif

#ifndef X_LSCACHE_HXX_
#define X_LSCACHE_HXX_
#include <lscache.hxx>
#endif

extern TAG tagNoOffScr;
extern TAG tagOscFullsize;
extern TAG tagOscTinysize;


MtDefine(CRootLayout, Layout, "CRootLayout")

const CLayout::LAYOUTDESC CRootLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

//////////////
//  Globals //
//////////////
/*
BSTR                g_bstrFindText = NULL;

HRESULT
GetFindText(BSTR *pbstr)
{
    LOCK_GLOBALS;

    RRETURN(FormsAllocString(g_bstrFindText, pbstr));
}

HRESULT
SetFindText(BSTR bstr)
{
    LOCK_GLOBALS;

    FormsFreeString(g_bstrFindText);
    g_bstrFindText = NULL;
    RRETURN(FormsAllocString(bstr, &g_bstrFindText));
}
*/
extern BSTR                g_bstrFindText;

HRESULT GetFindText(BSTR *pbstr);

HRESULT SetFindText(BSTR bstr);

//+---------------------------------------------------------------------------
//
//  Member:     CRootLayout::CRootLayout
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------

// NEWTREE: don't give the super constructor the context as it will
//          call AddRef before we are fully constructed (ug!)

CRootLayout::CRootLayout (CElement * pElementLayout)
    : super(pElementLayout)
{
    Assert(_fOpaque == FALSE);

    _fSizeThis = FALSE;
}


#ifdef  NEVER
//+---------------------------------------------------------------------------
//
//  Member:     CRootLayout::GetElementsInZOrder, public
//
//  Synopsis:   Returns the topsite in the element array
//
//----------------------------------------------------------------------------

HRESULT
CRootLayout::GetElementsInZOrder(CPtrAry<CElement *> *paryElements,
                               CElement            *pElementThis,
                               RECT                *prcDraw,
                               HRGN                 hrgn,
                               BOOL                 fIncludeNotVisible /* == FALSE */)
{
    Assert(pElementThis == ElementOwner());

    if (ClientSite() != ElementOwner() &&
        (fIncludeNotVisible || ClientSite()->IsVisible(FALSE)))
    {
        RRETURN(paryElements->Append(ClientSite()));
    }

    return S_OK;
}


#endif
//+---------------------------------------------------------------------------
//
//  Member:     CRootLayout::CalcSize
//
//  Synopsis:
//
//----------------------------------------------------------------------------

DWORD
CRootLayout::CalcSize(
    CCalcInfo * pci,
    SIZE *      psize,
    SIZE *      psizeDefault)
{
    CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSize           sizeOriginal;
    CDoc *          pDoc         = ElementOwner()->Doc();
    SIZE            size         = pci->_sizeDst;
    DWORD           grfReturn;

    Assert(pci);
    Assert(psize);

    GetSize(&sizeOriginal);

    // size the MarkupRoot to the new size.
    pci->SizeToParent(&size);          // fix for #13522, modify _sizeParent to the
                                     // new size.(srinib)

    // Don't do anything if the client site is detached
    if (    !MarkupRoot()->IsInMarkup()
        ||  (   ClientSite()
            &&  !ClientSite()->IsInMarkup()))
    {
        grfReturn = 0;
        goto Cleanup;
    }

    grfReturn = (pci->_grfLayout & LAYOUT_FORCE);

    if (    pci->_smMode == SIZEMODE_NATURAL
        ||  pci->_smMode == SIZEMODE_FULLSIZE
        ||  pci->_smMode == SIZEMODE_SET)
    {
        RECT    rc;

        rc.top  =
        rc.left = 0;
        rc.right  = psize->cx;
        rc.bottom = psize->cy;

        GetDisplay()->SetViewSize(rc);
        GetDisplay()->SetCaretWidth(0);
    }

    if (ClientSite() != ElementOwner())
    {
        if (pci->_hdc != NULL)
        {
            _fContentsAffectSize = FALSE;
            grfReturn = ClientSite()->GetLayout()->CalcSize(pci, psize);
        }
#if DBG==1
        else
            AssertSz (Doc()->IsPrintDoc(),
                      "NULL hdc in pci when calc'ing size of document.");
#endif
    }

    if (    pci->_smMode == SIZEMODE_NATURAL
        ||  pci->_smMode == SIZEMODE_SET
        ||  pci->_smMode == SIZEMODE_FULLSIZE)
    {
        grfReturn    |= LAYOUT_THIS  |
                        (psize->cx != sizeOriginal.cx
                                ? LAYOUT_HRESIZE
                                : 0) |
                        (psize->cy != sizeOriginal.cy
                                ? LAYOUT_VRESIZE
                                : 0);

        _fSizeThis = FALSE;
    }

Cleanup:
    return grfReturn;
}


#ifdef  NEVER
HRESULT
CRootLayout::ForceRelayout ( )
{
    HRESULT hr = S_OK;
    CDoc * pDoc = Doc();
    CElement * pElemClient = pDoc->GetElementClient();
    DWORD elemchng = ELEMCHNG_CLEARCACHES;

    ElementOwner()->EnsureFormatCacheChange( & elemchng );

    pElemClient->ResizeElement(NFLAGS_FORCE);

    RRETURN( hr );
}

#endif
//+----------------------------------------------------------------------------
//
//  Member:     DoLayout
//
//  Synopsis:   Respond to a layout request
//
//  Arguments:  grfLayout - Set of LAYOUT_xxxx flags
//
//-----------------------------------------------------------------------------
void
CRootLayout::DoLayout(
    DWORD   grfLayout)
{
    CCalcInfo   CI(this);
    SIZE        size = CI._sizeDst;

    CI._grfLayout |= grfLayout;

    CalcSize(&CI, &size);

    Reset();
}


//+----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Receive a sent notification
//
//  Arguments:  pnf - Notification sent
//
//-----------------------------------------------------------------------------
void
CRootLayout::Notify(
    CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));
    Assert( pnf->Element() != ElementOwner()
        ||  pnf->IsType(NTYPE_ELEMENT_REMEASURE));

    //
    //  Ignore:
    //      a) Already consumed notifications
    //      b) Any non-text/layout change notifications
    //

    if (    pnf->IsHandled()
        ||  (   !pnf->IsTextChange()
            &&  !pnf->IsLayoutChange()))
        goto Cleanup;

    //
    //  Always "dirty" the layout associated with the element
    //

    if (    pnf->Element()
        &&  pnf->Element() != ElementOwner())
    {
        pnf->Element()->DirtyLayout();
    }

    //
    //  Ignore the notification if already "dirty"
    //

    if (    !IsDirty()
        &&  !TestLock(CElement::ELEMENTLOCK_SIZING)
        &&  GetContentMarkup() == Doc()->_pPrimaryMarkup       // the last condition: layout only main doc tree
        &&  Doc()->State() >= OS_INPLACE)
    {
        _fDirty = TRUE;
        PostLayoutRequest(pnf->LayoutFlags());
    }

Cleanup:
    //
    //  Handle the notification
    //

    pnf->SetHandler(ElementOwner());

    WHEN_DBG(_snLast = pnf->SerialNumber());
}



//+----------------------------------------------------------------------------
//
//  Member:     Reset
//
//  Synopsis:   Reset the notification channel
//
//-----------------------------------------------------------------------------
void
CRootLayout::Reset()
{
    _fDirty = FALSE;

    super::Reset();
}


//+-------------------------------------------------------------------------
//
//  Method:     CRootLayout::QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------
#ifdef NEVER
extern TAG tagMsoCommandTarget;

HRESULT
CRootLayout::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext,
        BOOL fStopBobble)
{
    TraceTag((tagMsoCommandTarget, "CRootLayout::QueryStatus"));

    Assert(ElementOwner()->IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD *    pCmd = &rgCmds[0];
    UINT        idm;
    HRESULT     hr = S_OK;

    Assert(!pCmd->cmdf);

    switch (idm = ElementOwner()->IDMFromCmdID(pguidCmdGroup, pCmd->cmdID))
    {
    case IDM_REPLACE:
    case IDM_FONT:
    case IDM_GOTO:
    case IDM_HYPERLINK:
    case IDM_BOOKMARK:
    case IDM_IMAGE:
        if(Doc()->_fInHTMLDlg)
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
        break;

    case IDM_FIND:
        if (Doc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
        else
            pCmd->cmdf = Doc()->_fInHTMLDlg
                       ? MSOCMDSTATE_DISABLED
                       : MSOCMDSTATE_UP;
        break;

    case IDM_MENUEXT_COUNT:
        if(Doc()->_pOptionSettings)
            pCmd->cmdf = MSOCMDSTATE_UP;
        else
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
        break;
    }

    if(!ElementOwner()->IsEditable(TRUE) && idm >= IDM_MENUEXT_FIRST__ &&
        idm <= IDM_MENUEXT_LAST__ && Doc()->_pOptionSettings)
    {
        CONTEXTMENUEXT *    pCME;
        int                 nExts, nExtCur;

        // not supported unless the next test succeeds
        pCmd->cmdf = 0;

        nExts = Doc()->_pOptionSettings->aryContextMenuExts.Size();
        nExtCur = idm - IDM_MENUEXT_FIRST__;

        if(nExtCur < nExts)
        {
            // if we have it, it is enabled
            pCmd->cmdf = MSOCMDSTATE_UP;

            // the menu name is the text returned
            pCME = Doc()->_pOptionSettings->
                        aryContextMenuExts[idm - IDM_MENUEXT_FIRST__];
            pCmd->cmdf = MSOCMDSTATE_UP;

            Assert(pCME);

            if(pcmdtext && pcmdtext->cmdtextf == MSOCMDTEXTF_NAME)
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

    RRETURN_NOTRACE(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CRootLayout::Exec
//
//  Synopsis:   Called to execute a given command.  If the command is not
//              consumed, it may be routed to other objects on the routing
//              chain.
//
//--------------------------------------------------------------------------
HRESULT
CRootLayout::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut,
        BOOL fStopBobble)
{
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
    static DialogInfo   dlgInfo[] =
    {
        {IDM_FIND,          0,                     IDR_FINDDIALOG},
        {IDM_FIND,          0,                     IDR_BIDIFINDDIALOG},
        {IDM_REPLACE,       IDS_UNDOGENERICTEXT,   IDR_REPLACEDIALOG},
        {IDM_PARAGRAPH,     IDS_UNDOGENERICTEXT,   IDR_FORPARDIALOG},
        {IDM_FONT,          IDS_UNDOGENERICTEXT,   IDR_FORCHARDIALOG},
        {IDM_GOTO,          0,                     IDR_GOBOOKDIALOG},
        {IDM_IMAGE,         IDS_UNDONEWCTRL,       IDR_INSIMAGEDIALOG},
        {IDM_HYPERLINK,     IDS_UNDOGENERICTEXT,   IDR_EDLINKDIALOG},
        {IDM_BOOKMARK,      IDS_UNDOGENERICTEXT,   IDR_EDBOOKDIALOG},
    };
#endif // NO_HTML_DIALOG

    TraceTag((tagMsoCommandTarget, "CRootLayout::Exec"));

    Assert(ElementOwner()->IsCmdGroupSupported(pguidCmdGroup));

    UINT     idm;
    HRESULT  hr = MSOCMDERR_E_NOTSUPPORTED;

    switch (idm = ElementOwner()->IDMFromCmdID(pguidCmdGroup, nCmdID))
    {
        int             i;

    case IDM_ADDFAVORITES:
        {
            // Add the current document to the favorite folder ...
            //
            TCHAR * pszURL;
            TCHAR * pszTitle;
            CMarkup * pMarkup = GetContentMarkup();

            pszURL   = Doc()->_cstrUrl;
            pszTitle = (pMarkup->GetTitleElement() && pMarkup->GetTitleElement()->Length())
                     ? (pMarkup->GetTitleElement()->GetTitle())
                     : (NULL);
            hr = Doc()->AddToFavorites(pszURL, pszTitle);
            break;
        }
#ifndef NO_HTML_DIALOG
        // provide the options object to the dialog code
    case IDM_FIND:
    case IDM_REPLACE:
        // we should not invoke the dialogs out of the dialog...
        if (!Doc()->_fInHTMLDlg && nCmdexecopt != MSOCMDEXECOPT_DONTPROMPTUSER)
        {
            CVariant         cVarNull(VT_NULL);
            IDispatch      * pDispOptions=NULL;
            CParentUndoUnit* pCPUU = NULL;
            BSTR             bstrText = NULL;
            TCHAR            achOverrideFindUrl[pdlUrlLen];
            CDoc *           pDialogDoc = Doc();

            // The find dialog needs to search the active frame, if there is one
            if (idm == IDM_FIND)
            {
                CDoc *           pActiveFrameDoc = NULL;
                hr = THR(pDialogDoc->GetActiveFrame(achOverrideFindUrl,
                    ARRAY_SIZE(achOverrideFindUrl),
                    &pActiveFrameDoc, NULL));

                // The active frame doc can legally be null here, but
                // we'll crash if we continue.
                if (hr || pActiveFrameDoc == NULL)
                    break;

                if (pActiveFrameDoc != pDialogDoc)
                    pDialogDoc = pActiveFrameDoc;
            }
            COptionsHolder * pcoh = new COptionsHolder(pDialogDoc);

            if (pcoh == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
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
                pCPUU = ElementOwner()->OpenParentUnit(Doc(), dlgInfo[i].idsUndoText);
            }

            // get dispatch from stack variable
            hr = THR_NOTRACE(pcoh->QueryInterface(IID_IHTMLOptionsHolder,
                                     (void**)&pDispOptions));
            ReleaseInterface(pcoh);
            if (hr)
                break;

            // Save the execCommand argument so that the dialog can have acces
            // to them
            //
            pcoh->put_execArg(pvarargIn ? (VARIANT) * pvarargIn
                                        : (VARIANT)   cVarNull);

            hr = THR(GetFindText(&bstrText));
            if (hr)
                break;

            // Set the findText argument for the dialog
            THR_NOTRACE(pcoh->put_findText(bstrText));
            FormsFreeString(bstrText);
            bstrText = NULL;

            // Fix for bug# 9136. (a-pauln)
            // make an adjustment for the bidi find dialog
            // if we are on a machine that supports bidi
            if(idm == IDM_FIND && g_fBidiSupport)
            {
                i += 1;
            }

            // bring up the dialog
            hr = THR(ShowModalDialogHelper(
                    pDialogDoc,
                    dlgInfo[i].szidr,
                    pDispOptions,
                    pcoh));

            //get the findtext argument from the dialog
            THR_NOTRACE(pcoh->get_findText(&bstrText));
            IGNORE_HR(SetFindText(bstrText));
            FormsFreeString(bstrText);
            bstrText = NULL;

            // release dispatch, et al.
            ReleaseInterface(pDispOptions);

            if ( pCPUU )
            {
                IGNORE_HR(ElementOwner()->CloseParentUnit( pCPUU, hr ) );
            }
        }
        break;
#endif // NO_HTML_DIALOG

    case IDM_MENUEXT_COUNT:
        if(!pvarargOut)
        {
            hr = E_INVALIDARG;
        }
        else if(!Doc()->_pOptionSettings)
        {
            hr = OLECMDERR_E_DISABLED;
        }
        else
        {
            hr = S_OK;
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) =
                Doc()->_pOptionSettings->aryContextMenuExts.Size();
        }
    }

    // Handle context menu extensions - always eat the command here
    if( idm >= IDM_MENUEXT_FIRST__ && idm <= IDM_MENUEXT_LAST__)
    {
        hr = OnContextMenuExt(idm, pvarargIn);
    }

    if (hr == MSOCMDERR_E_NOTSUPPORTED)
    {
        hr = THR_NOTRACE(
            super::Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut,
                fStopBobble ) );
    }

    RRETURN_NOTRACE(hr);
}


//+-------------------------------------------------------------------
//
//  Method:     CRootLayout::OnContextMenuExt
//
//  Synopsis:   Handle launching the dialog when a ContextMenuExt
//              command is received
//
//--------------------------------------------------------------------
HRESULT
CRootLayout::OnContextMenuExt(UINT idm, VARIANTARG * pvarargIn)
{
    HRESULT          hr = E_FAIL;
    IDispatch      * pDispWindow=NULL;
    CParentUndoUnit* pCPUU = NULL;
    unsigned int     nExts;
    CONTEXTMENUEXT * pCME;
    EVENTPARAM     * pEvent = NULL;

    Assert(idm >= IDM_MENUEXT_FIRST__ && idm <= IDM_MENUEXT_LAST__);

    // find the html to run
    //
    nExts = Doc()->_pOptionSettings->aryContextMenuExts.Size();
    Assert((idm - IDM_MENUEXT_FIRST__) < nExts);
    pCME = Doc()->_pOptionSettings->
                aryContextMenuExts[idm - IDM_MENUEXT_FIRST__];
    Assert(pCME);

    // see if we need to push an event
    //
    if(!pvarargIn || V_VT(pvarargIn) != VT_I4
                  || V_I4(pvarargIn) != MENUEXT_COOKIE)
    {
        pEvent = new EVENTPARAM(Doc(), FALSE);
        if(!pEvent)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pEvent->pchType = _T("MenuExtUnknown");
    }

    // Undo stuff
    //
    pCPUU = ElementOwner()->OpenParentUnit(Doc(), IDS_CANTUNDO);

    // get dispatch for the main window
    //
    hr = THR(Doc()->EnsureOmWindow());
    if (hr)
        goto Cleanup;

    pDispWindow = (IHTMLWindow2*)(Doc()->_pOmWindow->Window());

    // bring up the dialog
    //
    hr = THR(ShowModalDialogHelper(
            Doc(),
            pCME->cstrActionUrl,
            pDispWindow,
            NULL,
            NULL,
            (pCME->dwFlags & MENUEXT_SHOWDIALOG)
                            ? 0 : (HTMLDLG_NOUI | HTMLDLG_AUTOEXIT)));

    if (pCPUU)
    {
        IGNORE_HR(ElementOwner()->CloseParentUnit(pCPUU, hr));
    }

Cleanup:
    delete pEvent;

    RRETURN(hr);
}

#endif
//+---------------------------------------------------------------------------
//
// Helper Function: IsValidAccessKey
//
//----------------------------------------------------------------------------
BOOL
IsValidAccessKey(CDoc * pDoc, CMessage * pmsg)
{
    BOOL fResult =  (pmsg->message == WM_SYSKEYDOWN)
                 || (pDoc->_fInHTMLDlg && pmsg->message == WM_CHAR);
    if (fResult)
    {
        fResult =  (pmsg->wParam != VK_MENU)
                && (pDoc->_aryAccessKeyItems.Size() > 0);
    }
    return fResult;
}

#ifdef NEVER
//+---------------------------------------------------------------------------
//
//  Member:     HandleMessage
//
//  Synopsis:   Handle messages bubbling when the passed site is non null
//
//  Arguments:  [pMessage]  -- message
//              [pChild]    -- pointer to child when bubbling allowed
//
//  Returns:    Returns S_OK if keystroke processed, S_FALSE if not.
//
//  Notes:      Delegate to CLayout::HandleMessage, not to super which is
//              CFlowLayout. EricVas should clean this up.
//----------------------------------------------------------------------------
HRESULT BUGCALL
CRootLayout::HandleMessage(CMessage * pMessage,
                         CElement * pElem,
                         CTreeNode *pNodeContext)
{
    HRESULT     hr    = S_FALSE;

    Assert(pNodeContext && pNodeContext->Element() == MarkupRoot());

    Assert(Doc()->State() >= OS_INPLACE);

#ifndef WIN16
    // CDoc::_pElemCurrent might be _pSiteRoot. In this case, we should pass
    // down WM_MOUSEWHEEL message to _pSiteClient
    // This is for Outlook Express (Raid 32150) and for roller under frame
    // (raid 38984)
    //
    if (pMessage->message == WM_MOUSEWHEEL)
    {
        if (Doc()->_pElemCurrent->Tag() == ETAG_ROOT && ClientSite()->Tag() != ETAG_ROOT)
        {
            IGNORE_HR(ClientSite()->HandleMessage(
                                pMessage,
                                ClientSite(),
                                ClientSite()->GetFirstBranch()));
        }
        hr = S_OK;
        goto Cleanup;
    }
#endif // ndef WIN16

    // Raid 57053
    // If we are in HTML dialog, we would like to check if pMessage matches
    // one control's accessKey (WM_CHAR message)
    // before we call super::HandleMessage(); otherwise this will be handled
    // by CSite::HandleMessage() by calling _pDoc->CServer::OnWindowMessage()
    //
    if (!Doc()->_fInHTMLDlg || pMessage->message != WM_CHAR)
    {
        hr = THR(CLayout::HandleMessage(pMessage, pElem, pNodeContext));
    }

    // Raid 44891
    // If CMessage::fNeedTranslate is TRUE, we should call TranslateMessage()
    // ourselves so that VK_RETURN WM_KEYDOWN can be translated to VK_RETURN
    // WM_CHAR, then be passed to us through message pump.
    //
    if (hr == S_FALSE && !ElementOwner()->IsEditable(TRUE)
                      && pMessage->fNeedTranslate
                      && pMessage->fPreDispatch
                      && pMessage->message == WM_KEYDOWN
                      && pMessage->wParam == VK_RETURN)
    {
        ::TranslateMessage(pMessage);
        hr = S_OK;
        goto Cleanup;
    }

    if ((hr != S_FALSE) ||
            (ElementOwner()->IsEditable(TRUE) && Doc()->_fInPre && IsTabKey(pMessage)))
    {
        goto Cleanup;
    }

    if (pElem)
    {
        if (IsFrameTabKey(pMessage) || IsTabKey(pMessage)
                || IsValidAccessKey(Doc(), pMessage))
        {
            hr = Doc()->HandleKeyNavigate(pMessage, FALSE);

            if (hr != S_FALSE)
                goto Cleanup;

            // Comment (jenlc). Say that the document has two frames, the
            // first frame has two controls with access keys ALT+A and ALT+B
            // respectively while the second frame has a control with access
            // key ALT+A. Suppose currently the focus is on the control with
            // access key ALT+B (the second control of the first frame) and
            // ALT+A is pressed, which control should get the focus? Currently
            // Trident let the control in the second frame get the focus.
            //
            if (IsTabKey(pMessage) || IsFrameTabKey(pMessage))
                IGNORE_HR(MarkupRoot()->BecomeCurrentAndActive());
        }

        // Raid 63207
        // If we call IOleControSite::TranslateAccelerator() here for WM_CHAR
        // in HTML Dialog, this would cause WM_CHAR message to be re-dispatched
        // back to us, which is an infinite loop.
        //
        if (hr == S_FALSE &&
                (!Doc()->_fInHTMLDlg || pMessage->message != WM_CHAR))
        {
            hr = Doc()->CallParentTA(pMessage);
        }

        if (IsFrameTabKey(pMessage) || IsTabKey(pMessage)
                || IsValidAccessKey(Doc(), pMessage))
        {
            if (hr == S_OK)
            {
                Doc()->_pElemUIActive = NULL;
            }
            else if (hr == S_FALSE)
            {
                hr = Doc()->HandleKeyNavigate(pMessage, TRUE);

                if (hr == S_FALSE && pMessage->message != WM_SYSKEYDOWN)
                {
                    ClientSite()->BecomeCurrentAndActive();
                    hr = S_OK;
                }
            }
        }
    }

    // Raid 57053
    // If we are in HTML dialog, and pMessage does not matche controls'
    // accessKey (WM_CHAR message with fPreDispatch not set), we call
    // super::HandleMessage() now.
    //
    if (hr == S_FALSE && Doc()->_fInHTMLDlg && pMessage->message == WM_CHAR)
    {
        hr = THR(CLayout::HandleMessage(pMessage, pElem, pNodeContext));
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

#endif

//+---------------------------------------------------------------------------
//
//  Member:     CRootLayout::GetYScroll/GetXScroll
//
//  Synopsis:   To set the brush origin correctly the paint code needs to know
//              the pixel scroll position of the document relative to the
//              window.
//
//----------------------------------------------------------------------------
long
CRootLayout::GetYScroll()
{
    Assert( ClientSite() );

    if(ClientSite()->GetLayout()->IsFlowLayout())
        return( DYNCAST(CFlowLayout, ClientSite()->GetLayout())->GetYScroll() );
    else
        return( 0 );
}

long
CRootLayout::GetXScroll()
{
    Assert( ClientSite() );

    if(ClientSite()->GetLayout()->IsFlowLayout())
        return( DYNCAST(CFlowLayout, ClientSite()->GetLayout())->GetXScroll() );
    else
        return( 0 );
}

long
CRootLayout::GetXScrollEx()
{
    Assert( ClientSite() );

    if(ClientSite()->GetLayout()->IsFlowLayout())
        return( DYNCAST(CFlowLayout, ClientSite()->GetLayout())->GetXScrollEx() );
    else
        return( 0 );
}


//+---------------------------------------------------------------------------
//
//  Member:     CRootLayout::GetBackgroundInfo
//
//  Synopsis:   Returns default background information
//
//----------------------------------------------------------------------------

BOOL
CRootLayout::GetBackgroundInfo(
    CFormDrawInfo *     pDI,
    BACKGROUNDINFO *    pbginfo,
    BOOL                fAll)
{
    Assert(pDI || !fAll);

    CColorValue cv = GetFirstBranch()->GetCascadedbackgroundColor();

    pbginfo->pImgCtx       = NULL;
    pbginfo->lImgCtxCookie = 0;

    pbginfo->crBack  = cv.IsDefined()
                            ? cv.GetColorRef()
                            : COLORREF_NONE;
    pbginfo->crTrans = COLORREF_NONE;

    return(TRUE);
}


#endif
