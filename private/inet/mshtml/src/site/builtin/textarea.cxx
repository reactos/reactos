//+---------------------------------------------------------------------
//
//   File:      textarea.cxx
//
//  Contents:   <TEXTAREA> <HTMLAREA>
//
//  Classes:    CTextarea, CRichtext
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

#ifndef X_TAREALYT_HXX_
#define X_TAREALYT_HXX_
#include "tarealyt.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXt_H_
#include "_text.h"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

EXTERN_C const GUID DIID_HTMLInputTextElementEvents;

#define _cxx_
#include "textarea.hdl"

MtDefine(CRichtext, Elements, "CRichtext")
MtDefine(CTextArea, Elements, "CTextArea")


IMPLEMENT_LAYOUT_FNS(CTextArea, CTextAreaLayout)
IMPLEMENT_LAYOUT_FNS(CRichtext, CRichtextLayout)

CElement::ACCELS CRichtext::s_AccelsTextareaDesign = CElement::ACCELS (NULL, IDR_ACCELS_INPUTTXT_DESIGN);
CElement::ACCELS CRichtext::s_AccelsTextareaRun    = CElement::ACCELS (NULL, IDR_ACCELS_INPUTTXT_RUN);

//+------------------------------------------------------------------------
//
//  Member:     CElement::s_classdesc
//
//  Synopsis:   class descriptor
//
//-------------------------------------------------------------------------


const CElement::CLASSDESC CTextArea::s_classdesc =
{
    {
        &CLSID_HTMLTextAreaElement,     // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_TEXTSITE |
        ELEMENTDESC_DONTINHERITSTYLE |
        ELEMENTDESC_SHOWTWS |
        ELEMENTDESC_CANSCROLL |
        ELEMENTDESC_VPADDING |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLTextAreaElement,      // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLTextAreaElement, // _pfnTearOff
    &s_AccelsTextareaDesign,            // _pAccelsDesign
    &s_AccelsTextareaRun                // _pAccelsRun
};

//+------------------------------------------------------------------------
//
//  Member:     CTextArea::CreateElement()
//
//  Synopsis:   called by the parser to create an instance
//
//-------------------------------------------------------------------------

HRESULT
CTextArea::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CTextArea(pht->GetTag(), pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Member:     CTextArea::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------
HRESULT
CTextArea::Save(CStreamWriteBuff * pStmWrBuff, BOOL fEnd)
{
    HRESULT hr  = S_OK;

    Assert(!_fInSave);
    _fInSave = TRUE;

    if (!fEnd)
    {
        pStmWrBuff->BeginPre();
    }

    hr = super::Save(pStmWrBuff, fEnd);

    if(hr)
        goto Cleanup;

    if (fEnd)
    {
        pStmWrBuff->EndPre();
    }

    Assert(_fInSave); // this will catch recursion
    _fInSave = FALSE;

Cleanup:
    RRETURN(hr);
}
void
CTextArea::GetPlainTextWithBreaks(TCHAR * pchBuff)
{
    CTxtPtr     tp(GetMarkup(), Layout()->GetContentFirstCp());
    long        i;
    CDisplay *   pdp = Layout()->GetDisplay();
    long        c = pdp->Count();
    CLine *     pLine;

    Assert(Layout()->GetMultiLine());
    Assert(pchBuff);

    for(i = 0; i < c; i++)
    {
        pLine = pdp->Elem(i);

        if (pLine->_cch)
        {
            tp.GetRawText(pLine->_cch, pchBuff);
            tp.AdvanceCp(pLine->_cch);
            pchBuff += pLine->_cch;

            // If the line ends in a '\r', we need to append a '\r'.
            // Otherwise, it must be having a soft break (unless it is
            // the last line), so we need to append a '\r\n'
            if (pLine->_fHasBreak)
            {
                Assert(*(pchBuff - 1) == _T('\r'));
                *pchBuff++ = _T('\n');
            }
            else if ( i < c - 1)
            {
                *pchBuff++ = _T('\r');
                *pchBuff++ = _T('\n');
            }
        }
    }
    // Null-terminate
    *pchBuff = 0;
}


long
CTextArea::GetPlainTextLengthWithBreaks()
{
    long        len     = 1; // for trailing '\0'
    long        i;
    CDisplay *   pdp = Layout()->GetDisplay();
    long        c = pdp->Count();
    CLine *     pLine;

    Assert(HasLayout());
    Assert(Layout()->GetMultiLine());
    for(i = 0; i < c; i++)
    {
        // Every line except the last must be non-empty.
        Assert(i == c - 1 || pdp->Elem(i)->_cch > 0);

        pLine = pdp->Elem(i);
        Assert(pLine);
        len += pLine->_cch;

        // If the line ends in a '\r', we need to append a '\r'.
        // Otherwise, it must be having a soft break (unless it is
        // the last line), so we need to append a '\r\n'
        if (pLine->_fHasBreak)
        {
            len++;
        }
        else if (i < c - 1)
        {
            len += 2;
        }
    }

    return len;
}

//+------------------------------------------------------------------------
//
//  Member:     CTextArea::GetSubmitValue()
//
//  Synopsis:   returns the inner HTML
//
//-------------------------------------------------------------------------

HRESULT
CTextArea::GetSubmitValue(CStr *pstr)
{
    HRESULT hr          = S_OK;

    // Make sure the site is not detached. Otherwise, we
    // cannot get to the runs
    if (!IsInMarkup())
        goto Cleanup;

    if (GetAAwrap() == htmlWrapHard)
    {
        long len = GetPlainTextLengthWithBreaks();
        if (len > 0)
        {
            IGNORE_HR(pstr->SetLengthNoAlloc(0));
            hr = THR(pstr->ReAlloc(len));
            if (hr)
                goto Cleanup;
            GetPlainTextWithBreaks(*pstr);
        }
    }
    else
    {
        hr = THR(GetValueHelper(pstr));
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CTextArea::ApplyDefaultFormat()
//
//  Synopsis:
//
//-------------------------------------------------------------------------

HRESULT
CTextArea::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    CDoc *              pDoc     = Doc();
    CODEPAGESETTINGS *  pCS      = pDoc->_pCodepageSettings;
    COLORREF            crWindow = GetSysColorQuick(COLOR_WINDOW);
    HRESULT             hr       = S_OK;

    if (   !pCFI->_pff->_ccvBackColor.IsDefined()
        ||  pCFI->_pff->_ccvBackColor.GetColorRef() != crWindow)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._ccvBackColor = crWindow;
    }

    pCFI->PrepareCharFormat();
    pCFI->PrepareParaFormat();

    pCFI->_cf()._ccvTextColor.SetSysColor(COLOR_WINDOWTEXT);

    // our intrinsics shouldn't inherit the cursor property. they have a 'default'
    pCFI->_cf()._bCursorIdx = styleCursorAuto;

    pCFI->_cf()._fBold = FALSE;
    pCFI->_cf()._wWeight = FW_NORMAL; //FW_NORMAL = 400
    pCFI->_cf()._yHeight = 200;       // 10 * 20, 10 points

    // Thai does not have a fixed pitch font. Leave it as proportional
    if (pDoc->GetCodePage() != CP_THAI)
    {
        pCFI->_cf()._bPitchAndFamily = FIXED_PITCH;
        // pCF->_fBumpSizeDown = TRUE;         // not necessary?
        pCFI->_cf()._latmFaceName = pCS->latmFixedFontFace;
    }
    pCFI->_cf()._bCharSet = pCS->bCharSet;
    pCFI->_cf()._fNarrow = IsNarrowCharSet(pCFI->_cf()._bCharSet);
    pCFI->_pf()._cuvTextIndent.SetPoints(0);
    pCFI->_fPre = TRUE;

    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    pCFI->PrepareCharFormat();

    // font height in CharFormat is already nonscaling size in twips
    pCFI->_cf().SetHeightInNonscalingTwips( pCFI->_cf()._yHeight );

    pCFI->UnprepareForDebug();

Cleanup:
    RRETURN(hr);
}

// Richtext member functions

const CElement::CLASSDESC CRichtext::s_classdesc =
{
    {
        &CLSID_HTMLRichtextElement,      // _pclsid
        0,                               // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                  // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                          // _pcpi
        ELEMENTDESC_TEXTSITE          |
        ELEMENTDESC_SHOWTWS           |
        ELEMENTDESC_ANCHOROUT         |
        ELEMENTDESC_NOBKGRDRECALC     |
        ELEMENTDESC_CANSCROLL         |
        ELEMENTDESC_HASDEFDESCENT     |
        ELEMENTDESC_NOTIFYENDPARSE,      // _dwFlags
        &IID_IHTMLTextAreaElement,       // _piidDispinterface
        &s_apHdlDescs,                   // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLTextAreaElement, // _pfnTearOff
    &s_AccelsTextareaDesign,    // _pAccelsDesign
    &s_AccelsTextareaRun        // _pAccelsRun
};

//+------------------------------------------------------------------------
//
//  Member:     CRichtext::CreateElement()
//
//  Synopsis:   called by the parser to create instance
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CRichtext(pht->GetTag(), pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}



//+------------------------------------------------------------------------
//
//  Member:     CRichtext::Init2()
//
//  Synopsis:   called by the parser to initialize instance
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::Init2(CInit2Context * pContext)
{
    HRESULT hr = S_OK;

    hr = THR(super::Init2(pContext));
    if (!OK(hr))
        goto Cleanup;

#ifdef  NEVER
    if (Tag() == ETAG_HTMLAREA)
    {
        SetAAtype(htmlInputRichtext);
    }
    else
#endif
    {
        Assert(Tag() == ETAG_TEXTAREA);
        SetAAtype(htmlInputTextarea);
    }

    if (!GetAAreadOnly())
    {
        _fEditAtBrowse = TRUE;
    }

    _iHistoryIndex = (unsigned short)Doc()->_dwHistoryIndex++;

Cleanup:
    RRETURN1(hr, S_INCOMPLETE);
}



//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//              Also, fires a property change notification to the site.
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr          = S_OK;

    hr = THR(super::OnPropertyChange(dispid, dwFlags));
    if (hr)
        goto Cleanup;

    switch (dispid)
    {
    case DISPID_CRichtext_wrap:
        Layout()->SetWrap();
        break;

    case DISPID_CRichtext_readOnly:
        _fEditAtBrowse = !GetAAreadOnly();
        // update the editability in the edit context
        if (HasCurrency())
        {
            Doc()->EnsureEditContext(this);
        }
        break;
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CRichtext::QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CRichtext::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD   * pCmd = & rgCmds[0];
    ULONG      cmdID;

    Assert(!pCmd->cmdf);

    cmdID = CBase::IDMFromCmdID( pguidCmdGroup, pCmd->cmdID );
    switch (cmdID)
    {
    case IDM_INSERTOBJECT:
        // Don't allow objects to be inserted in TEXTAREA
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        return S_OK;

    default:
        RRETURN_NOTRACE(super::QueryStatus(
            pguidCmdGroup,
            1,
            pCmd,
            pcmdtext));
    }
}


HRESULT
CRichtext::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    int      idm = CBase::IDMFromCmdID(pguidCmdGroup, nCmdID);
    HRESULT  hr  = MSOCMDERR_E_NOTSUPPORTED;

    switch (idm)
    {
    case IDM_SELECTALL:
        select();
        hr = S_OK;
        break;
    }

    if (hr == MSOCMDERR_E_NOTSUPPORTED)
    {
        hr = super::Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut);
    }

    RRETURN_NOTRACE(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CRichtext::DelayLoadHistoryValue()
//
//  Synopsis:   DelayedLoadHistory will call this
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::DelayLoadHistoryValue()
{
    HRESULT hr      = S_OK;
    CStr    cstrVal;
    DWORD   dwTemp;
    DWORD   dwScrollPos = 0;
    CDoc   *pDoc = Doc();
    DWORD   dwHistoryIndex = 0x80000000 | (DWORD)_iHistoryIndex & 0x0FFFF;

    // Load the history stream
    IGNORE_HR(pDoc->GetLoadHistoryStream(dwHistoryIndex, 
                                HistoryCode(), 
                                &_pStreamHistory));

    if (_pStreamHistory && !Doc()->_fUserInteracted)
    {
        CDataStream ds(_pStreamHistory);
        DWORD   dwEncoding;

        // Load encoding changing history
        hr = THR(ds.LoadDword(&dwEncoding));
        if (hr)
            goto Cleanup;

        if (!dwEncoding)
        {
            // load value
            hr = THR(ds.LoadCStr(&cstrVal));
            if (hr)
                goto Cleanup;

            hr = THR(SetValueHelperInternal(&cstrVal));
            if (hr)
                goto Cleanup;

            // load _fTextChanged
            hr = THR(ds.LoadDword(&dwTemp));
            if (hr)
                goto Cleanup;

            _fTextChanged = dwTemp ? TRUE : FALSE;
        }

        // load scroll pos
        hr = THR(ds.LoadDword(&dwScrollPos));
        if (hr)
            goto Cleanup;

        {
            CLayout *pLayout = Layout();
            CDispNode   *pDispNode = pLayout->GetElementDispNode();
            if (pDispNode && pDispNode->IsScroller())
            {
                pLayout->ScrollToY(dwScrollPos);
            }
        }

        //
        // we might insert new elements, we need to ensure the tree cache
        // is up to date
        //
    }
Cleanup:
    ClearInterface(&_pStreamHistory);
    RRETURN(hr);
}

/*
//+------------------------------------------------------------------------
//
//  Member:     CRichtext::LoadHistoryValue()
//
//  Synopsis:   Load history
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::LoadHistoryValue()
{
    HRESULT hr = S_OK;
    CStr    cstrVal;
    DWORD   dwTemp;

    if (_pStreamHistory)
    {
        CDataStream ds(_pStreamHistory);
    }
Cleanup:
    RRETURN(hr);
}

*/

//+------------------------------------------------------------------------
//
//  Member:     CRichtext::SaveHistoryValue()
//
//  Synopsis:   save history:
//                              - value
//                              - _fTextChanged
//                              - scroll position
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::SaveHistoryValue(CHistorySaveCtx *phsc)
{
    CDataStream ds;
    HRESULT     hr      = S_OK;
    IStream *   pStream = NULL;
    CStr        cstrVal;
    DWORD       dwHistoryIndex;

    Assert(phsc);
    if (!phsc)
        goto Cleanup;

    dwHistoryIndex = 0x80000000 | (DWORD)_iHistoryIndex & 0x0FFFF;
    hr = THR(phsc->BeginSaveStream(dwHistoryIndex, HistoryCode(), &pStream));
    if (hr)
        goto Cleanup;

    ds.Init(pStream);

    hr = THR(ds.SaveDword(_fChangingEncoding ? 1 : 0));
    if (hr)
        goto Cleanup;
    if (!_fChangingEncoding)
    {
        // save value
        hr = THR(GetValueHelper(&cstrVal));
        if (hr)
            goto Cleanup;
        hr = THR(ds.SaveCStr(&cstrVal));
        if (hr)
            goto Cleanup;

        // save _fTextChanged
        hr = THR(ds.SaveDword(_fTextChanged ? 1 : 0));
        if (hr)
            goto Cleanup;
    }

    // save scroll pos
    hr = THR(ds.SaveDword(Layout()->GetYScroll()));
    if (hr)
        goto Cleanup;

    hr = THR(phsc->EndSaveStream());
    if (hr)
        goto Cleanup;
Cleanup:
    ReleaseInterface(pStream);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CRichtext::Notify()
//
//  Synopsis:   handle notifications
//
//-------------------------------------------------------------------------

void
CRichtext::Notify(CNotification *pNF)
{
    IMarkupPointer* pStart = NULL;
    IHTMLElement* pIElement = NULL;
    HRESULT hr = S_OK;
    BOOL fRefTaken = FALSE;
    
    if (pNF->Type() != NTYPE_DELAY_LOAD_HISTORY)
    {
        super::Notify(pNF);
    }
    switch (pNF->Type())
    {

    case NTYPE_ELEMENT_GOTMNEMONIC:
    {
        if (! Doc()->_fDesignMode )
        {
            CDoc* pDoc = Doc();
            fRefTaken = TRUE;
            hr = THR( pDoc->CreateMarkupPointer( & pStart ));
            if ( hr )
                goto Cleanup;
            
            hr = THR( this->QueryInterface( IID_IHTMLElement, (void**) & pIElement ));
            if ( hr )
                goto Cleanup;

            hr = THR( pStart->MoveAdjacentToElement( pIElement, ELEM_ADJ_BeforeEnd ));
            if ( hr )
                goto Cleanup;

            hr = THR( pDoc->Select( pStart, pStart, SELECTION_TYPE_Caret));                
            if ( hr )
                goto Cleanup;
        }                 
    }            
    break;

    case NTYPE_ELEMENT_LOSTMNEMONIC:
    {
        if (! Doc()->_fDesignMode )
        {
            Doc()->NotifySelection( SELECT_NOTIFY_DESTROY_ALL_SELECTION, NULL );
        }                 
    }        
    break;

    case NTYPE_SAVE_HISTORY_1:
        pNF->SetSecondChanceRequested();
        break;
 
    case NTYPE_SAVE_HISTORY_2:
        {
            CHistorySaveCtx *   pCtx = NULL;

            pNF->Data((void **)&pCtx);
            IGNORE_HR(SaveHistoryValue(pCtx));
        }
        break;

    case NTYPE_DELAY_LOAD_HISTORY:
        IGNORE_HR(DelayLoadHistoryValue());
        super::Notify(pNF);
        break;

    case NTYPE_END_PARSE:
        GetValueHelper(&_cstrDefaultValue);
        _fLastValueSet = FALSE;
        break;

    // BUGBUG: this might have problems with undo (jbeda)
    case NTYPE_ELEMENT_EXITTREE_1:
        ClearInterface(&_pStreamHistory);
        break;

    case NTYPE_SET_CODEPAGE:
        _fChangingEncoding = TRUE;
        break;
    }

Cleanup:
    if ( fRefTaken )
    {
        ReleaseInterface( pStart );
        ReleaseInterface( pIElement );
    }
    
}


//+------------------------------------------------------------------------
//
//  Member:     CRichtext::GetValueHelper()
//
//  Synopsis:   returns the inner HTML
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::GetValueHelper(CStr *pstr)
{
    HRESULT hr = S_OK;

    if (Tag()==ETAG_TEXTAREA)
    {
        hr = THR(GetPlainTextInScope(pstr));
    }
    else
    {
        BSTR    bStrValue;

#ifdef  NEVER
        Assert(Tag()==ETAG_HTMLAREA);
#endif
        hr = THR(GetText(&bStrValue, WBF_NO_WRAP|WBF_NO_TAG_FOR_CONTEXT));;
        if (hr)
            goto Cleanup;

        Assert(pstr);
        hr = pstr->SetBSTR(bStrValue);
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CRichtext::GetSubmitValue()
//
//  Synopsis:   returns the inner HTML
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::GetSubmitValue(CStr *pstr)
{
    RRETURN(GetValueHelper(pstr));
}

//+------------------------------------------------------------------------
//
//  Member:     CRichtext::GetWordWrap()
//
//  Synopsis:   Callback to tell if the word wrap should be done
//
//-------------------------------------------------------------------------

BOOL
CRichtext::GetWordWrap() const
{
    return  htmlWrapOff == GetAAwrap() ? FALSE : TRUE;
}

HRESULT
CRichtext::SetValueHelperInternal(CStr *pstr, BOOL fOM /* = TRUE */)
{
    HRESULT hr = S_OK;
    int c= pstr->Length();

#ifdef  NEVER
    Assert(Tag()==ETAG_HTMLAREA || Tag()==ETAG_TEXTAREA);
#else
    Assert(Tag()==ETAG_TEXTAREA);
#endif
    Assert(pstr);

#ifdef  NEVER
    hr = THR( Inject( Inside, Tag() == ETAG_HTMLAREA, *pstr, c ) );
#else
    hr = THR( Inject( Inside, FALSE, *pstr, c ) );
#endif

#ifndef NO_DATABINDING
    if (SUCCEEDED(hr))
    {
        hr = SaveDataIfChanged(ID_DBIND_DEFAULT);
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }
    }
#endif

    // Set this to prevent OnPropertyChange(_Value_) from firing twice
    // when value is set through OM. This flag is cleared in OnTextChange().
    _fFiredValuePropChange = fOM;

    // bugbug, make sure this is covered when turns the HTMLAREA on
    _cstrLastValue.Set(*pstr, c);
    _fLastValueSet = TRUE;
    _fTextChanged = FALSE;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CRichtext::SetValueHelper()
//
//  Synopsis:   set the inner HTML
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::SetValueHelper(CStr *pstr)
{
    return SetValueHelperInternal(pstr);
}

//+----------------------------------------------------------------------------
//
//  Method:     GetSubmitInfo
//
//  Synopsis:   returns the submit info string if there is a value
//              (name && value pair)
//
//  Returns:    S_OK if successful
//              E_NOTIMPL if not applicable for current element
//
//-----------------------------------------------------------------------------
HRESULT
CRichtext::GetSubmitInfo(CPostData * pSubmitData)
{
    LPCTSTR pstrName = GetAAsubmitname();

    //  no name --> no submit!
    if ( ! pstrName )
        return S_FALSE;

    CStr    cstrValue;
    HRESULT hr = GetSubmitValue(&cstrValue);
    if (hr)
        goto Cleanup;

    hr = THR(pSubmitData->AppendNameValuePair(pstrName, cstrValue, Doc()));

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRichtext::createTextRange
//
//----------------------------------------------------------------------------
HRESULT
CRichtext::createTextRange( IHTMLTxtRange * * ppDisp )
{
    HRESULT hr = S_OK;

    hr = THR( EnsureInMarkup() );
    
    if (hr)
        goto Cleanup;

    hr = THR( GetMarkup()->createTextRange( ppDisp, this ) );
    
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}

//+------------------------------------------------------------------------
//
//  Member:     CRichtext::ApplyDefaultFormat()
//
//  Synopsis:
//
//-------------------------------------------------------------------------

HRESULT
CRichtext::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    CUnitValue  uv;
    DWORD       dwRawValue;
    int         i;
    HRESULT     hr = S_OK;

    pCFI->PrepareFancyFormat();

    // No vertical spacing between para's
    pCFI->_ff()._cuvSpaceBefore.SetPoints(0);
    pCFI->_ff()._cuvSpaceAfter.SetPoints(0);

    // Border info
    pCFI->_ff()._ccvBorderColorLight.SetSysColor(COLOR_3DLIGHT);
    pCFI->_ff()._ccvBorderColorDark.SetSysColor(COLOR_3DDKSHADOW);
    pCFI->_ff()._ccvBorderColorHilight.SetSysColor(COLOR_BTNHIGHLIGHT);
    pCFI->_ff()._ccvBorderColorShadow.SetSysColor(COLOR_BTNSHADOW);

    uv.SetValue( 2, CUnitValue::UNIT_PIXELS );
    dwRawValue = uv.GetRawValue();

    for ( i = BORDER_TOP; i <= BORDER_LEFT; i++ )
    {
        pCFI->_ff()._cuvBorderWidths[i].SetRawValue( dwRawValue );
        pCFI->_ff()._bBorderStyles[i] = fmBorderStyleSunken;
    }
    // End Border info

    //
    // Add default padding and scrolling
    //

    Assert(TEXT_INSET_DEFAULT_LEFT == TEXT_INSET_DEFAULT_RIGHT);
    Assert(TEXT_INSET_DEFAULT_LEFT == TEXT_INSET_DEFAULT_TOP);
    Assert(TEXT_INSET_DEFAULT_LEFT == TEXT_INSET_DEFAULT_BOTTOM);

    uv.SetValue(TEXT_INSET_DEFAULT_LEFT, CUnitValue::UNIT_PIXELS);

    pCFI->_ff()._cuvPaddingLeft   =
    pCFI->_ff()._cuvPaddingRight  =
    pCFI->_ff()._cuvPaddingTop    =
    pCFI->_ff()._cuvPaddingBottom = uv;

    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    if (    pCFI->_pff->_bOverflowX == (BYTE)styleOverflowNotSet
        ||  pCFI->_pff->_bOverflowY == (BYTE)styleOverflowNotSet)
    {
        pCFI->PrepareFancyFormat();

        if (pCFI->_pff->_bOverflowX == (BYTE)styleOverflowNotSet)
        {
            pCFI->_ff()._bOverflowX = GetAAwrap() != htmlWrapOff
                                            ? (BYTE)styleOverflowHidden
                                            : (BYTE)styleOverflowScroll;
        }

        if (pCFI->_pff->_bOverflowY == (BYTE)styleOverflowNotSet)
        {
            pCFI->_ff()._bOverflowY = (BYTE)styleOverflowScroll;
        }

        pCFI->UnprepareForDebug();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT BUGCALL
CRichtext::select(void)
{
    HRESULT             hr          = S_OK;
    CMarkup *           pMarkup     = GetMarkup();
    CDoc *              pDoc        = Doc();
    CMarkupPointer      ptrStart(pDoc);
    CMarkupPointer      ptrEnd(pDoc); 
    IMarkupPointer *    pIStart; 
    IMarkupPointer *    pIEnd; 

    if (!pMarkup)
        goto Cleanup;

#if 0
    hr = pDoc->SetEditContext(this, TRUE, FALSE);
#else
    // BUGBUG (MohanB) We need to make this current because that's the only
    // way selection works right now. GetCurrentSelectionRenderingServices()
    // looks for the current element. MarkA should fix this.
    hr = BecomeCurrent(0);
#endif
    if (hr)
        goto Cleanup;

    hr = ptrStart.MoveToCp(GetFirstCp(), pMarkup);
    if (hr)
        goto Cleanup;
    hr = ptrEnd.MoveToCp(GetLastCp(), pMarkup);
    if (hr)
        goto Cleanup;
    Verify(S_OK == ptrStart.QueryInterface(IID_IMarkupPointer, (void**)&pIStart));
    Verify(S_OK == ptrEnd.QueryInterface(IID_IMarkupPointer, (void**)&pIEnd));
    hr = pDoc->Select(pIStart, pIEnd, SELECTION_TYPE_Selection);
    pIStart->Release();
    pIEnd->Release();
Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CRichtext::RequestYieldCurrency
//
//  Synopsis:   Check if OK to Relinquish currency
//
//  Arguments:  BOOl fForce -- if TRUE, force change and ignore user cancelling the
//                             onChange event
//
//  Returns:    S_OK: ok to yield currency
//
//--------------------------------------------------------------------------

HRESULT
CRichtext::RequestYieldCurrency(BOOL fForce)
{
    CStr    cstr;
    HRESULT hr = S_OK;

    if ((hr = GetValueHelper(&cstr)) == S_OK)
    {
        BOOL fFire =  FormsStringCmp(cstr,
                            _fLastValueSet ?
                                _cstrLastValue : _cstrDefaultValue)
                        != 0;

        if (!fFire)
            goto Cleanup;

        if (!Fire_onchange())   //JS event
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = super::RequestYieldCurrency(fForce);
        if (hr == S_OK)
        {
            _cstrLastValue.Set(cstr);
            _fLastValueSet = TRUE;
        }
    }

Cleanup:
    if (fForce && FAILED(hr))
    {
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CRichText::BecomeUIActive
//
//  Synopsis:   Check imeMode to set state of IME.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become ui active.
//
//--------------------------------------------------------------------------

HRESULT
CRichtext::BecomeUIActive()
{
    HRESULT hr = S_OK;

    hr = THR(super::BecomeUIActive());
    if (hr)
        goto Cleanup;

    hr = THR(SetImeState());
    if (hr)
        goto Cleanup;

Cleanup:    
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CRichtext::YieldCurrency
//
//  Synopsis:   Relinquish currency
//
//  Arguments:  pSiteNew    New site that wants currency
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CRichtext::YieldCurrency(CElement *pElemNew)
{
    HRESULT hr;

    _fDoReset = FALSE;

    Assert(HasLayout());

    hr = THR(super::YieldCurrency(pElemNew));

    RRETURN(hr);
}


HRESULT BUGCALL
CRichtext::HandleMessage(CMessage * pMessage)
{
    HRESULT         hr = S_FALSE;
    CFormElement *  pForm;
    BOOL    fEditable = IsEditable(TRUE);

    if ( !CanHandleMessage() ||
         (!fEditable && !IsEnabled()) )
    {
        goto Cleanup;
    }

    if (!fEditable && _fDoReset)
    {
        if (pMessage->message == WM_KEYDOWN && pMessage->wParam == VK_ESCAPE)
        {
            pForm = GetParentForm();
            if (pForm)
            {
                _fDoReset = FALSE;
                hr = THR(pForm->DoReset(TRUE));
                if (hr != S_FALSE)
                    goto Cleanup;
            }
        }
        else if (pMessage->message >= WM_KEYFIRST &&
                pMessage->message <= WM_KEYLAST &&
                pMessage->wParam != VK_ESCAPE)
        {
            _fDoReset = FALSE;
        }
    }

    switch (pMessage->message)
    {
        case WM_RBUTTONDOWN:
            // Ignore right-click (single click) if the input text box has focus.
            // This prevents the selected contents in an input text control from
            // being loosing selection.

            //
            // marka - we only do this for browse mode. Why ? We want going UI Active
            // to be under mshtmled's rules (ie first click site select, second drills in)
            //
            if ( ! Doc()->_fDesignMode )
            {
                hr = THR(BecomeCurrent(pMessage->lSubDivision));
            }                
            goto Cleanup;
            
        // We handle all WM_CONTEXTMENUs
        case WM_CONTEXTMENU:
            hr = THR(OnContextMenu(
                    (short) LOWORD(pMessage->lParam),
                    (short) HIWORD(pMessage->lParam),
                    CONTEXT_MENU_CONTROL));
            goto Cleanup;
    }

    if (!fEditable &&
        pMessage->message == WM_KEYDOWN &&
        pMessage->wParam == VK_ESCAPE)
    {
        _fDoReset = TRUE;
        SetValueHelperInternal(_fLastValueSet ? &_cstrLastValue : &_cstrDefaultValue, FALSE);
        hr = S_FALSE;
        goto Cleanup;
    }

    // Let supper take care of event firing
    // Since we let TxtEdit handle messages we do JS events after
    // it comes back
    hr = super::HandleMessage(pMessage);

Cleanup:

    RRETURN1(hr, S_FALSE);
}


HRESULT
CRichtext::put_status(VARIANT status)
{
    switch(status.vt)
    {
    case VT_NULL:
        _vStatus.vt = VT_NULL;
        break;
    case VT_BOOL:
        _vStatus.vt = VT_BOOL;
        V_BOOL(&_vStatus) = V_BOOL(&status);
        break;
    default:
        _vStatus.vt = VT_BOOL;
        V_BOOL(&_vStatus) = VB_TRUE;
    }

    Verify(S_OK==OnPropertyChange(DISPID_CRichtext_status, 0));

    RRETURN(S_OK);
}

HRESULT
CRichtext::get_status(VARIANT * pStatus)
{
    if (_vStatus.vt==VT_NULL)
    {
        pStatus->vt = VT_NULL;
    }
    else
    {
        pStatus->vt = VT_BOOL;
        V_BOOL(pStatus) = V_BOOL(&_vStatus);
    }
    RRETURN(S_OK);
}


HRESULT
CRichtext::DoReset(void)
{
    RRETURN(SetValueHelperInternal(&_cstrDefaultValue));
}

#ifndef NO_DATABINDING
class CDBindMethodsTextarea : public CDBindMethodsText
{
    typedef CDBindMethodsText super;
public:
    CDBindMethodsTextarea() : super(0)   {}
    ~CDBindMethodsTextarea()    {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;
};

static const CDBindMethodsTextarea DBindMethodsTextarea;

const CDBindMethods *
CRichtext::GetDBindMethods()
{
    return &DBindMethodsTextarea;
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: transfer bound data to textarea element.  We need to override
//            the CElement implementation to remember the value being
//            transferred as the "last value".  This keeps the onchange event
//            from firing simply because the value is databound.
//
//  Arguments:
//            [id]      - ID of binding point.
//            [fHTML]   - is HTML-formatted data called for?
//            [pvData]  - pointer to data to transfer, in the expected data
//                        type.  For text-based transfers, must be BSTR.
//
//-----------------------------------------------------------------------------

HRESULT
CDBindMethodsTextarea::BoundValueToElement ( CElement *pElem,
                                                 LONG id,
                                                 BOOL fHTML,
                                                 LPVOID pvData ) const
{
    HRESULT hr = super::BoundValueToElement(pElem, id, fHTML, pvData);    // do the transfer
    if (!hr)
    {
        // BUGBUG: We were relying on OnTextChange to fire PropertyChange
        // event for value, but this is not reliable if the page is loading
        // and our layout is not listening to notification yet.
        // 

        Assert(pElem->Tag() == ETAG_TEXTAREA);
        CRichtextLayout *pLayout = DYNCAST(CRichtext, pElem)->Layout();

        if (!pLayout || !pLayout->IsListening())
        {
            Verify(!pElem->OnPropertyChange(DISPID_A_VALUE, 0));
        }
        // remember the value
        CRichtext *pElemRT = DYNCAST(CRichtext, pElem);
        pElemRT->_cstrLastValue.Set(*(LPCTSTR*)pvData);
        pElemRT->_fLastValueSet = TRUE;
    }
    return hr;
}
#endif // ndef NO_DATABINDING
