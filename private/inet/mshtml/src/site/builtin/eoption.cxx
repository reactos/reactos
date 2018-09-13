//+---------------------------------------------------------------------
//
//   File:      eoption.cxx
//
//  Contents:   Option element class, etc..
//
//  Classes:    COptionElement, etc..
//
//------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_TEXTXFRM_HXX_
#define X_TEXTXFRM_HXX_
#include "textxfrm.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_ESELECT_HXX_
#define X_ESELECT_HXX_
#include "eselect.hxx"
#endif

#ifndef X_SELLYT_HXX_
#define X_SELLYT_HXX_
#include "sellyt.hxx"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#define _cxx_
#include "option.hdl"

MtDefine(COptionElement, Elements, "COptionElement")
MtDefine(COptionElementFactory, Elements, "COptionElementFactory")

#if DBG == 1
static unsigned s_OptionSize = sizeof(COptionElement);
#endif

extern class CFontCache & fc();

const CElement::CLASSDESC COptionElement::s_classdesc =
{
    {
        &CLSID_HTMLOptionElement,           // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        0,                                  // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOANCESTORCLICK|
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLOptionElement,            // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLOptionElement,     // apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


HRESULT COptionElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(pht->Is(ETAG_OPTION));
    Assert(ppElementResult);
    *ppElementResult = new COptionElement(pht->GetTag(), pDoc);
    return *ppElementResult ? S_OK : E_OUTOFMEMORY;
}



#if DBG == 1
void
COptionElement::Passivate(void)
{
    CSelectElement * pSelect = GetParentSelect();

    Assert(! pSelect || pSelect->_poptLongestText != this);

    super::Passivate();
}
#endif // DBG


//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::Notify
//
//  Synopsis:   Placeholder for the notification stuff
//
//  Note:       The OPTION might need it for managing the SELECT's cache
//
//----------------------------------------------------------------------------

void
COptionElement::Notify(CNotification *pNF)
{
    DWORD           dw = pNF->DataAsDWORD();

    super::Notify(pNF);

    switch(pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        {
            CSelectElement * pSelect = GetParentSelect();

            if ( pSelect )
            {
                pSelect->_fOptionsDirty = TRUE;
                pSelect->Layout()->InternalNotify();
            }
        }

        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        _fInCollection = FALSE;
        if (!(dw & EXITTREE_DESTROY))
        {
            CSelectElement * pSelect = GetParentSelect();

            if (pSelect)
            {
                pSelect->_fOptionsDirty = TRUE;
                pSelect->Layout()->InternalNotify();
            }
        }
        break;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     COptionElement::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------

HRESULT
COptionElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{

    // BUGBUG - (68121 krisma) If we are printing, and the parent select of 
    // the option is a combobox, and there is no option currently selected,
    // we need to save an empty select so that we print without any options 
    // showing. (By default, a combo box shows the first option.)

    HRESULT hr = S_OK;

    if (pStreamWrBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC))
    {
        CSelectElement * pSelect = GetParentSelect();
        if (pSelect && !pSelect->_fListbox && pSelect->GetCurSel() == -1)
        {
            goto Cleanup;
        }
    }
    hr = super::Save(pStreamWrBuff, fEnd);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     COptionElement::Init2
//
//  Synopsis:   Initialization phase after attributes were set.
//
//  Note:       Save the SELECTED flag into DefaultSelected
//
//-------------------------------------------------------------------------

HRESULT
COptionElement::Init2(CInit2Context * pContext)
{
    HRESULT hr;

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    _fDefaultSelected = _fSELECTED;

Cleanup:
    RRETURN1(hr, S_INCOMPLETE);
}


//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::Getindex
//
//  Synopsis:   Gets the index of the option object
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::get_index(long * plIndex)
{
    if (!plIndex)
        RRETURN(SetErrorInfoInvalidArg());

    if ( _fInCollection )
    {
        *plIndex = GetParentSelect()->_aryOptions.Find(this);
    }
    else
    {
        Assert(0 && "We shouldn't get here: getIndex while not in Options collection");
    }

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::put_index
//
//  Synopsis:   Pretends to set the index of the option object
//              This is a silent no-op.
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::put_index(long lIndex)
{
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::get_form
//
//  Synopsis:   Returns the form above the option, if there is one.
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::get_form(IHTMLFormElement **ppDispForm)
{
    HRESULT          hr = S_OK;
    CFormElement   * pForm;
    CSelectElement * pSelect;

    if (!ppDispForm)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDispForm = NULL;

    pSelect = GetParentSelect();

    if (pSelect)
        pForm = pSelect->GetParentForm();
    else
        goto Cleanup; // return S_OK/NULL

    if (pForm)
    {
        hr = THR_NOTRACE(pForm->QueryInterface(IID_IHTMLFormElement,
                                              (void**)ppDispForm));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN( SetErrorInfoPGet( hr, DISPID_CSite_form));
}


//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::get_text
//
//  Synopsis:   Gets the index of the option object
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::get_text(BSTR * pbstrText)
{
    HRESULT hr = S_OK;

    if (!pbstrText)
        RRETURN(SetErrorInfoInvalidArg());

    *pbstrText = NULL;


#if (DBG == 1 && defined(WIN16))
    CSelectElement *pSelect;

    //  This code sanity-check the text in the listbox
    //  against the text in the OPTION element to make sure they
    //  are still in sync.

    if ( _fInCollection &&
         NULL != (pSelect = GetParentSelect()) &&
         pSelect->_hwnd
        )
    {
        CStr cstrText;
        long cchText;
        long lIndex;

        lIndex = pSelect->_aryOptions.Find(this);

        Assert(lIndex > -1 );

        cchText = pSelect->SendSelectMessage(CSelectElement::Select_GetTextLen, lIndex, 0);
        if ( cchText == LB_ERR )
            goto Win32Error;

        hr = cstrText.ReAlloc(cchText);
        if ( hr )
            goto Error;

        cchText = pSelect->SendSelectMessage(CSelectElement::Select_GetText,
                                             lIndex,
                                             (LPARAM)(LPTSTR)cstrText);
        if ( cchText == LB_ERR )
            goto Win32Error;

        cstrText.SetLengthNoAlloc(cchText);

        Assert(0 == StrCmpC(_cstrText, cstrText));
    }
#endif

    if ( ! _cstrText.IsNull() )
    {
        hr = _cstrText.AllocBSTR(pbstrText);
        if ( hr )
            goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));


#if (DBG == 1 && defined(WIN16))

    //  Error handling block for the sanity check above

Error:
    SysFreeString(*pbstrText);
    *pbstrText = NULL;
    goto Cleanup;

Win32Error:
    hr = GetLastWin32Error();
    goto Error;

#endif

}







//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::put_text
//
//  Synopsis:   Set the display text pf the OPTION element
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::put_text(BSTR bstrText)
{
    HRESULT hr;

    // Note that VID 6.0 constructed a bad BSTR, without a proper length,
    // but zero terminated. So, we're going to rely on zero termination
    // instead of the length.
    LPTSTR pchText=(LPTSTR)bstrText;

    CSelectElement * pSelect = NULL;
    BOOL fOldEnableLayoutRequests = TRUE;
    BOOL fOldFlagValid = FALSE;

    long lIndex;
    long lSelectedIndex = 0;
    LRESULT lr;

    hr = _cstrText.Set(pchText);
    if ( hr )
        goto Cleanup;

    // invalidate font linking test
    _fCheckedFontLinking = FALSE;

    if( !IsInMarkup() )
        goto Cleanup;

    pSelect = GetParentSelect();
    if ( pSelect )
    {
        fOldFlagValid = TRUE;
        fOldEnableLayoutRequests = pSelect->_fEnableLayoutRequests;
        pSelect->_fEnableLayoutRequests = FALSE;
    }

    {
        CMarkupPointer p1( Doc() ), p2( Doc() );

        hr = THR( p1.MoveAdjacentToElement( this, ELEM_ADJ_AfterBegin ) );

        if (hr)
            goto Cleanup;
                  
        hr = THR( p2.MoveAdjacentToElement( this, ELEM_ADJ_BeforeEnd ) );

        if (hr)
            goto Cleanup;

        hr = THR( Doc()->Remove( & p1, & p2 ) );

        if (hr)
            goto Cleanup;

        hr = THR( Doc()->InsertText( & p1, pchText, -1 ) );

        if (hr)
            goto Cleanup;
    }

    //  Old code brought back for synchronous update
    if ( _fInCollection && (NULL != pSelect) && pSelect->_hwnd )
    {
        lIndex = pSelect->_aryOptions.Find(this);

        Assert(lIndex > -1 );

        if ( ! pSelect->IsMultiSelect() )
        {
            lSelectedIndex = pSelect->SendSelectMessage(CSelectElement::Select_GetCurSel, 0, 0);
        }

        lr = pSelect->SendSelectMessage(CSelectElement::Select_DeleteString, lIndex, 0);
        if ( lr == LB_ERR )
            goto Win32Error;

        lr = pSelect->SendSelectMessage(CSelectElement::Select_InsertString, lIndex, (LPARAM)(LPTSTR)pchText);
        if ( lr == LB_ERR )
            goto Win32Error;

        if ( ! pSelect->IsMultiSelect() && lSelectedIndex == lIndex )
        {
            pSelect->SetCurSel(lSelectedIndex);
        }

        pSelect->DeferUpdateWidth();
    }

Cleanup:
    if ( fOldFlagValid && pSelect)
    {
        pSelect->_fEnableLayoutRequests = fOldEnableLayoutRequests;
    }
    RRETURN(SetErrorInfo(hr));

Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::put_defaultSelected
//
//  Synopsis:   Gets the index of the option object
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::put_defaultSelected(VARIANT_BOOL f)
{
    _fDefaultSelected = (f == VB_TRUE);

    return S_OK;
}






//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::get_defaultSelected
//
//  Synopsis:   Gets the index of the option object
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::get_defaultSelected(VARIANT_BOOL * pf)
{
    if ( ! pf )
        RRETURN (SetErrorInfo(E_POINTER));

    *pf = _fDefaultSelected ? VB_TRUE : VB_FALSE;

    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::GetSelectedHelper
//
//  Synopsis:   Gets the selected state of the option object
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::GetSelectedHelper(long * pf)
{
    HRESULT hr = S_OK;

    if (!pf)
        RRETURN(SetErrorInfoInvalidArg());

#if DBG == 1
    CSelectElement *pSelect;
    long lIndex;

    if ( _fSELECTED && ( NULL != (pSelect = GetParentSelect()) ) 
        && !( pSelect->_fMultiple ) && ( pSelect->_aryOptions.Size() > 0 ))
    {
        lIndex = pSelect->_aryOptions.Find(this);
        Assert (lIndex == pSelect->_iCurSel);
    }

#endif // DBG == 1

    *pf = _fSELECTED;

    RRETURN(hr);
}





//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::SetSelectedHelper
//
//  Synopsis:   Gets the selected state of the option object
//
//----------------------------------------------------------------------------

HRESULT
COptionElement::SetSelectedHelper(long f)
{
    HRESULT hr = S_OK;
    CSelectElement *pSelect;
    long lIndex;
    LRESULT lr;
    int iOldSel;

    _fSELECTED = f;

    if ( _fInCollection && NULL != (pSelect = GetParentSelect()) )
    {
        iOldSel = pSelect->_iCurSel;
        int iCurSel;
        lIndex = pSelect->_aryOptions.Find(this);

        Assert(lIndex > -1 );
        if ( pSelect->_fMultiple )
        {
            lr = pSelect->SetSel(lIndex, !!f);
            if ( lr == LB_ERR )
                goto Win32Error;
        }
        else
        {
            //  Force collection update
            lr = pSelect->SetCurSel(lIndex, SETCURSEL_UPDATECOLL);
            if ( lr == LB_ERR )
                goto Win32Error;
        }

        iCurSel = pSelect->GetCurSel();
        if (iCurSel != iOldSel)
        {
            hr = THR(pSelect->OnPropertyChange(DISPID_CSelectElement_selectedIndex, 0));
            if (hr)
                goto Cleanup;
            if (pSelect->HasValueChanged(iOldSel, iCurSel))
            {
                hr = THR(pSelect->OnPropertyChange(DISPID_CSelectElement_value, 0));
                if (hr)
                    goto Cleanup;
            }
        }
    }


Cleanup:
    RRETURN(hr);

Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;
}








//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::GetDisplayColors
//
//  Synopsis:   Computes the fore and background colors of the option element
//              based on selection state, styles and Windows color preferences.
//
//----------------------------------------------------------------------------

void
COptionElement::GetDisplayColors(COLORREF * pcrFore, COLORREF * pcrBack, BOOL fListbox)
{
    CColorValue ccv;
    CSelectElement * pSelect;

    Assert(pcrFore);
    Assert(pcrBack);


    //  Set up the textcolor
    if ( (NULL != (pSelect = GetParentSelect())) &&
          pSelect->GetAAdisabled() )
    {
        *pcrFore = GetSysColorQuick(COLOR_GRAYTEXT);
    }
    else if ( fListbox && _fSELECTED )
    {
        *pcrFore = GetSysColorQuick(COLOR_HIGHLIGHTTEXT);
    }
    else if ( (ccv = GetFirstBranch()->GetCascadedcolor()).IsDefined() )
    {
        *pcrFore = ccv.GetColorRef();
    }
    else
    {
        *pcrFore = GetSysColorQuick(COLOR_WINDOWTEXT);
    }

    //Set up the backcolor
    if ( fListbox && _fSELECTED )
    {
        *pcrBack = GetSysColorQuick(COLOR_HIGHLIGHT);
    }
    else if ( (ccv = GetFirstBranch()->GetCascadedbackgroundColor()).IsDefined() )
    {
        *pcrBack = ccv.GetColorRef();
    }
    else
    {
        *pcrBack = GetSysColorQuick(COLOR_WINDOW);
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     COptionElement::GetDisplayText
//
//  Synopsis:   Returns the display text according to TEXT_TRANSFORM.
//
//----------------------------------------------------------------------------

CStr *
COptionElement::GetDisplayText(CStr * pcstrBuf)
{
    BYTE bTextTransform = GetFirstBranch()->GetCascadedtextTransform();

    Assert(pcstrBuf);

    if ( ( bTextTransform == styleTextTransformNotSet ) ||
         ( bTextTransform == styleTextTransformNone ) )
    {
        return &_cstrText;
    }
    else
    {
        TransformText( *pcstrBuf,
                       _cstrText,
                       _cstrText.Length(),
                       bTextTransform );

        return pcstrBuf;
    }
}




//+---------------------------------------------------------------
//
//  Member   : COptionElement::OnPropertyChange
//
//  Synopsis : Do any work that Options need when a property is
//             changed
//
//+---------------------------------------------------------------
HRESULT
COptionElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    CSelectElement *pSelect;

    if ( NULL != (pSelect = GetParentSelect()) )
    {
            // some changes invalidate collections
        if (dwFlags & ELEMCHNG_UPDATECOLLECTION)
        {
            pSelect->InvalidateCollection();

            // Clear this flag: exclusive or
            dwFlags ^= ELEMCHNG_UPDATECOLLECTION;
        }

        //  BUGBUG:(laszlog): Need to add code to invalidate/resize
        //                    the containing SELECT if the contents
        //                    or contents style change.

        if ( dispid == DISPID_COptionElement_text ||
             dispid == DISPID_UNKNOWN )
        {
            pSelect->DeferUpdateWidth();
        }

        hr = THR(super::OnPropertyChange(dispid, dwFlags));
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member   : COptionElement::MeasureLine
//
//  Synopsis : Measure the length of a single line
//
//+---------------------------------------------------------------
long
COptionElement::MeasureLine(CCalcInfo * pci)
{
    CStr cstrBuffer;
    CStr * pcstrDisplayText;
    long lWidth;
    long cch;
    long lCharWidth = -1;
    CSelectElement * pSelect;
    int i;
    TCHAR * pch;
    const CCharFormat *pcf;
    const CParaFormat *pPF;
    BOOL fRTL;
    UINT taOld = 0;
    CCcs * pccs = 0;
    CCalcInfo   CI;

    pSelect = GetParentSelect();

    if ( ! pSelect )
        return -1;

    if ( ! pci )
    {
        CI.Init(pSelect->GetCurLayout());
        pci = &CI;
    }


    pcstrDisplayText = GetDisplayText(&cstrBuffer);

    cch = pcstrDisplayText->Length();

    //  We don't support letterspacing in the listbox for v1.0
    //lWidth = (cch - 1) * GetCascadedletterSpacing().GetPixelValue(pci, CUnitValue::DIRECTION_CX, 0);
    lWidth = 0;

    pcf = pSelect->GetFirstBranch()->GetCharFormat();
    pPF = pSelect->GetFirstBranch()->GetParaFormat();
    
    fRTL = pPF->HasRTL(TRUE);
    // ComplexText
    if(fRTL)
    {
        taOld = GetTextAlign(pci->_hdc);
        SetTextAlign(pci->_hdc, TA_RTLREADING | TA_RIGHT);
    }

    if ( ! pcf )
        return -1;  //  <<<< inline return

    pccs = fc().GetCcs(pci->_hdc, pci, pcf);
    if ( ! pccs )
        return -1;

    if (CheckFontLinking(pci->_hdc, pccs) && pcf != NULL)
    {
        // bugbug: (benwest) this code won't be necessary after after bug 28568 is resolved...
        // the listbox charformat should be non-scaling but currently isn't.
        CCharFormat cf = *pcf;
        cf.SetHeightInNonscalingTwips(cf._yHeight);
        cf._bCrcFont = cf.ComputeFontCrc();
        // end

        lCharWidth = FontLinkTextOut(pci->_hdc, 0, 0, 0, NULL, *pcstrDisplayText, cch, pci, &cf, FLTO_TEXTEXTONLY);
        if (lCharWidth > 0)
        {
            lWidth += lCharWidth; // width of entire string
        }
    }

    if (lCharWidth < 0)
    {
        for ( i = cch, pch = *pcstrDisplayText;
              i > 0;
              i--, pch++ )
        {
            if ( ! pccs->Include(*pch, lCharWidth) )
            {
                Assert(0 && "Char not in font!");
            }
            lWidth += lCharWidth;
        }
    }

    SetTextAlign(pci->_hdc, taOld);

    pccs->Release();

    return lWidth;
}

//+---------------------------------------------------------------
//
//  Member   : COptionElement::CheckFontLinking
//
//  Synopsis : Returns TRUE iff this option needs font linking.
//
//+---------------------------------------------------------------
BOOL COptionElement::CheckFontLinking(HDC hdc, CCcs *pccs)
{
    CStr cstrTransformed;
    CStr *pcstrDisplayText;
    LPCTSTR pString;

    if (_fCheckedFontLinking) // assuming only one thread will run this per element
    {
        return _fNeedsFontLinking;
    }

    _fCheckedFontLinking = TRUE;
    _fNeedsFontLinking = FALSE; // init for failure

    pcstrDisplayText = GetDisplayText(&cstrTransformed);
    pString = *pcstrDisplayText;

    if (pString == NULL)
    {
        return FALSE;
    }

    return (_fNeedsFontLinking = NeedsFontLinking(hdc, pccs, pString, _tcslen(pString), Doc()));
}


HRESULT
COptionElement::CacheText(void)
{
    HRESULT hr;

    CTreePos * ptpStart;
    CTreePos * ptpEnd;

    GetTreeExtent(&ptpStart, &ptpEnd);

    if( !ptpStart || !ptpEnd)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    {
        long    cp = ptpStart->GetCp() + 1;
        long    cch = ptpEnd->GetCp() - cp;

        CTxtPtr tp( GetMarkup(), cp );

        cch = tp.GetPlainTextLength( cch );

        _cstrText.Free();

        hr = _cstrText.ReAlloc(cch);
        if ( hr )
            goto Cleanup;

        Verify( cch == tp.GetPlainText( cch, _cstrText ) );
        _cstrText.SetLengthNoAlloc(cch);
    }


Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Class:      COptionElementFactory
//
//----------------------------------------------------------------------------

const COptionElementFactory::CLASSDESC COptionElementFactory::s_classdesc =
{
    {
        &CLSID_HTMLOptionElementFactory,     // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLOptionElementFactory,      // _piidDispinterface
        &s_apHdlDescs,                       // _apHdlDesc
    },
    (void *)s_apfnIHTMLOptionElementFactory,         // _apfnTearOff
};

// Get this into CVariant next full build I do




//+---------------------------------------------------------------------------
//
//  Member:     COptionElementFactory::create
//
//  Synopsis:   Manufactures a new COptionElement/
//
//  Note:       Supports the "new Option(...)" JavaScript syntax
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
COptionElementFactory::create(VARIANT varText,
    VARIANT varvalue,
    VARIANT varDefaultSelected,
    VARIANT varSelected,
    IHTMLOptionElement**ppnewElem )
{
    HRESULT hr;
    COptionElement *pOptionElem;
    CElement *pNewElem = NULL;
    CVariant varBSTRText;
    CVariant varBSTRvalue;
    CVariant varBOOLDefaultSelected;
    CVariant varBOOLSelected;

    if ( !ppnewElem )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppnewElem = NULL;

    // actualy ( [ BSTR text, [ BSTR value, [ BOOL defaultselected, [ BOOL selected] ] ] )
    // Create an Option element 
    hr = THR(_pDoc->CreateElement(ETAG_OPTION, &pNewElem ));
    if ( hr )
        goto Cleanup;

    pOptionElem = DYNCAST(COptionElement, pNewElem);

    hr = THR(pOptionElem->QueryInterface ( IID_IHTMLOptionElement, (void **)ppnewElem ));

    // Now set up the properties specified ( if present )

    // Text
    hr = THR(varBSTRText.CoerceVariantArg(&varText, VT_BSTR) );
    if ( hr == S_OK )
    {
        hr = THR(pOptionElem->_cstrText.Set ( V_BSTR(&varBSTRText) ));
    }
    if ( !OK(hr) )
        goto Cleanup;

    // Value
    hr = THR(varBSTRvalue.CoerceVariantArg(&varvalue,VT_BSTR) );
    if ( hr == S_OK )
    {
        hr = THR(pOptionElem->SetAAvalue ( V_BSTR(&varBSTRvalue) ));
    }
    if ( !OK(hr) )
        goto Cleanup;

    // defaultSelected
    hr = THR(varBOOLDefaultSelected.CoerceVariantArg(&varDefaultSelected, VT_BOOL));
    if ( hr == S_OK )
    {
        pOptionElem->_fDefaultSelected = V_BOOL(&varBOOLDefaultSelected);
    }
    if ( !OK(hr) )
        goto Cleanup;

    // selected
    hr = THR(varBOOLSelected.CoerceVariantArg(&varSelected, VT_BOOL) );
    if ( hr == S_OK )
    {
        pOptionElem -> _fSELECTED = V_BOOL(&varBOOLSelected) == VB_TRUE ? TRUE : FALSE;
    }
    if ( !OK(hr) )
        goto Cleanup;


Cleanup:
    if (OK(hr))
    {
        hr = S_OK; // not to propagate possible S_FALSE
    }
    else
    {
        ReleaseInterface(*(IUnknown**)ppnewElem);
    }

    CElement::ClearPtr(&pNewElem);

    RRETURN(hr);
}
