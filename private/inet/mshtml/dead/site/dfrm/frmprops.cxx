//-----------------------------------------------------------------------------
//  Copyright: (c) 1994-1995, Microsoft Corporation
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  File        frmprops.cxx
//
//  Contents    Implementation of the layout frame properties
//
//  Classes     CLayoutFrame
//
//  Maintained by  Lajosf
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"

extern TAG tagDataFrame;

// generated property procedures

//-----------------------------------------------------------------------------
//
//  range checking overload
//
//-----------------------------------------------------------------------------
HRESULT
CBaseFrame::SetInt(int *pInt, int iValue, int iRangeStart, int iRangeEnd, DISPID dispid)
{
    if (iValue >= iRangeStart && iValue <= iRangeEnd)
    {
        return SetInt(pInt, iValue, dispid);
    }

    return SetErrorInfoBadValue(dispid,
                                IDS_ES_ENTER_VALUE_IN_RANGE,
                                iRangeStart, iRangeEnd);
}
//-end-of-function-------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//  gets the long, checks the argument, returns errorinfo
//
//-----------------------------------------------------------------------------
HRESULT CBaseFrame::GetLong(long *pLong, long l)
{
    if (pLong)
    {
        *pLong = l;
        return S_OK;
    }
    return SetErrorInfoInvalidArg();
}
//-end-of-function-------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//  sets the LONG, checks the argument, returns errorinfo
//
//-----------------------------------------------------------------------------
HRESULT
CBaseFrame::SetLong(long *pLong, long l, DISPID dispid)
{
    if (l != *pLong)
    {
        *pLong = l;
        return OnDataChangeCloseError(dispid, TRUE);
    }
    return S_OK;

}
//-end-of-function-------------------------------------------------------------




//-----------------------------------------------------------------------------
//
//  gets the BOOL, checks the argument, returns errorinfo
//
//-----------------------------------------------------------------------------
HRESULT CBaseFrame::GetBool(VARIANT_BOOL *pf, BOOL flag)
{
    if (pf)
    {
        *pf = ENSURE_BOOL(flag);
        return S_OK;
    }
    return SetErrorInfoInvalidArg();
}
//-end-of-function-------------------------------------------------------------





// some macros....

#define RRETURNSETBOOL(pf, flag,dispid)             \
    {                                               \
        if (ENSURE_BOOL(flag) != ENSURE_BOOL(pf))   \
        {                                           \
            pf = ENSURE_BOOL(flag);                 \
            RRETURN(OnDataChangeCloseError(dispid, TRUE));    \
        }                                           \
        return S_OK;                                \
    }



#define RRETURNSETBOOLFROMINT(pf, value, max,dispid)    \
    {                                                   \
        if (value >= 0 && value <= max)                 \
        {                                               \
            pf = value;                                 \
            RRETURN(OnDataChangeCloseError(dispid, TRUE));        \
        }                                               \
        return SetErrorInfoBadValue(dispid,             \
                IDS_ES_ENTER_VALUE_IN_RANGE,            \
                0, max);                                \
    }






//-----------------------------------------------------------------------------
//
//  gets the Color, checks the argument, returns errorinfo
//
//-----------------------------------------------------------------------------
HRESULT CBaseFrame::GetColor(OLE_COLOR *pOlecolor, OLE_COLOR olecolor)
{
    if (pOlecolor)
    {
        *pOlecolor = olecolor;
        return S_OK;
    }
    return SetErrorInfoInvalidArg();
}
//-end-of-function-------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//  sets the Color, checks the argument, returns errorinfo
//
//-----------------------------------------------------------------------------
HRESULT CBaseFrame::SetColor(OLE_COLOR *pOlecolor, OLE_COLOR olecolor, DISPID dispid)
{
    if (IsOleColorValid(olecolor))
    {
        *pOlecolor = olecolor;
        return OnDataChangeCloseError(dispid, FALSE);
    }
    return SetErrorInfoInvalidArg();
}
//-end-of-function-------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//  gets the String, checks the argument, returns errorinfo
//
//-----------------------------------------------------------------------------
HRESULT CBaseFrame::GetBstr(BSTR *pbstr, CStr &cstr)
{
    if (pbstr)
    {
        return cstr.AllocBSTR(pbstr);
    }
    return SetErrorInfoInvalidArg();
}
//-end-of-function-------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//  sets the String, checks the argument, returns errorinfo
//
//-----------------------------------------------------------------------------
HRESULT CBaseFrame::SetBstr(LPTSTR lp, CStr &cstr, DISPID dispid)
{
    HRESULT hr = E_FAIL;
    if (lp)
    {
        hr = cstr.Set(lp);
        if (!hr)
        {
            return OnDataChangeCloseError(dispid, FALSE);
        }
        else
            return E_OUTOFMEMORY;
    }
    return SetErrorInfoInvalidArg();
}
//-end-of-function-------------------------------------------------------------













//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetMouseIcon
//
//  Synopsis    Gets the _hMouseIcon property
//
//  Arguments   ppDisp   pointer where the _hMouseIcon address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetMouseIcon(IDispatch **ppDisp)
{
    TraceTag((tagDataFrame,"CDataFrame::GetMouseIcon"));
    RRETURN(GetIcon(ppDisp, (IDispatch *)TBag()->_pMousePicture));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetMouseIcon
//
//  Synopsis    Sets the _hMouseIcon property
//
//  Arguments   pDisp   new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetMouseIcon(IDispatch * pDisp)
{
    TraceTag((tagDataFrame,"CDataFrame::SetMouseIcon"));
    RRETURN(SetIcon((IDispatch**)&(TBag()->_pMousePicture),  pDisp, DISPID_MouseIcon));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetMousePointer
//
//  Synopsis    Gets the _iMousePointer property
//
//  Arguments   piMousePointer       pointer where the _iMousePointer address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetMousePointer(fmMousePointer * pMousePointer)
{
    TraceTag((tagDataFrame,"CDataFrame::GetMousePointer"));
    RRETURN(GetInt((int*)pMousePointer, TBag()->_iMousePointer));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetMousePointer
//
//  Synopsis    Sets the _iMousePointer property
//
//  Arguments   iMousePointer       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetMousePointer(fmMousePointer MousePointer)
{
    TraceTag((tagDataFrame,"CDataFrame::SetMousePointer"));
    RRETURN(SetInt(&(TBag()->_iMousePointer), MousePointer, DISPID_MousePointer));
}





//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetAutosize
//
//  Synopsis    Gets the _enAutosize property
//
//  Arguments   pfRepeat         pointer where the _enAutosize address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAutosizeStyle(fmEnAutoSize *enAutosize)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAutosize"));
    if  (enAutosize  == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    *enAutosize = (fmEnAutoSize)TBag()->_enAutosize;
    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetAutosize
//
//  Synopsis    Sets the _enAutosize property
//
//  Arguments   enAutosize     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAutosizeStyle(fmEnAutoSize  enAutosize)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAutosize"));

    CTBag *pTBag = TBag();
    if (pTBag->_enAutosize != (unsigned) enAutosize)
    {
        if (enAutosize & fmEnAutoSizeHorizontal)
        {
            if (!pTBag->_lMinCols && !pTBag->_lMaxCols)
            {
                pTBag->_lMinCols = 1;    // always show at least 1 column
                pTBag->_lMaxCols = -1;   // expand to fit all rows
            }
        }
        if (enAutosize & fmEnAutoSizeVertical)
        {
            if (!pTBag->_lMinRows && !pTBag->_lMaxRows)
            {
                pTBag->_lMinRows = 1;    // always show at least 1 row
                pTBag->_lMaxRows = -1;   // exapnd to fit all rows
            }
        }
        pTBag->_enAutosize = enAutosize;
        RRETURN(OnDataChangeCloseError(DISPID_DATADOC_Autosize, TRUE));
    }
    return S_OK;
}







//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetScrollbars
//
//  Synopsis    Gets the _iScrollbars property
//
//  Arguments   piScrollbars         pointer where the _iScrollbars address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetScrollBars(fmScrollBars * pScrollbars)
{
    TraceTag((tagDataFrame,"CDataFrame::GetScrollbars"));
    if (pScrollbars  == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    *pScrollbars = TBag()->_iScrollbars;
    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetScrollbars
//
//  Synopsis    Sets the _iScrollbars property
//
//  Arguments   iScrollbars     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetScrollBars(fmScrollBars iScrollbars)
{
    TraceTag((tagDataFrame,"CDataFrame::SetScrollbars"));
    TBag()->_iScrollbars = iScrollbars;
    RRETURN(OnDataChangeCloseError(DISPID_ScrollBars, FALSE));
}


//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetShow3D
//
//  Synopsis    Gets the _iShow3D property
//
//  Arguments   piShow3D         pointer where the _iShow3D address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetShow3D(int * piShow3D)
{
    TraceTag((tagDataFrame,"CDataFrame::GetShow3D"));
    RRETURN(GetInt(piShow3D, TBag()->_iShow3D));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetShow3D
//
//  Synopsis    Sets the _iShow3D property
//
//  Arguments   iShow3D     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetShow3D(int  iShow3D)
{
    TraceTag((tagDataFrame,"CDataFrame::SetShow3D"));
    RRETURN(SetInt(&(TBag()->_iShow3D), iShow3D, DISPID_Show3D));
}



//+---------------------------------------------------------------
//
//  Member:     CDataFrame::SetRowSource
//
//  Synopsis:   Sets RowSource property
//
//---------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetRowSource(LPTSTR bstrRowSource)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRowSource"));

    CDataLayerAccessorSource *pdlas;
    HRESULT hr=S_OK;

    if (bstrRowSource)
    {
        hr = THR(LazyGetDLAccessorSource(&pdlas));
        if (hr || pdlas->GetRowSource() &&
                  !_tcsicmp(pdlas->GetRowSource(), bstrRowSource) )
        {
            goto Cleanup;
        }
    }
    else if (HasDLAccessorSource())
    {
        IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
        Assert(pdlas);
    }
    else
    {
        goto Cleanup;
    }

    hr = THR(pdlas->SetRowSource(bstrRowSource));
    if (hr)
    {
        goto Cleanup;
    }
    hr = OnDataChange(DISPID_RowSource, TRUE);

Cleanup:
    RRETURN(CloseErrorInfo(hr));
}

//+---------------------------------------------------------------
//
//  Member:     CDataFrame::GetRowSource
//
//  Synopsis:   Access RowSource property
//
//---------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetRowSource(BSTR * pbstrRowSource)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRowSource"));
    HRESULT hr;

    if (pbstrRowSource == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (HasDLAccessorSource())
        {
            CDataLayerAccessorSource *pdlas;
            IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
            Assert(pdlas);
            hr = THR(pdlas->GetRowSource().AllocBSTR(pbstrRowSource));
        }
        else
        {
            *pbstrRowSource = NULL;
            hr = S_OK;
        }
    }
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetEnabled
//
//  Synopsis:   Gets Enabled property
//
//  Arguments:  pfEnabled       ptr to location to return _fEnabled
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetEnabled(VARIANT_BOOL * pfEnabled)
{
    TraceTag((tagDataFrame,"CDataFrame::GetEnabled"));
    if (pfEnabled == NULL)
        RRETURN(E_INVALIDARG);

    *pfEnabled = _fEnabled;

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetEnabled
//
//  Synopsis:   Sets fEnabled property
//
//  Arguments:  Enabled     set Enabled property to this
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetEnabled(VARIANT_BOOL fEnabled)
{
    TraceTag((tagDataFrame,"CDataFrame::SetEnabled"));
    RRETURNSETBOOL(_fEnabled, fEnabled, DISPID_ENABLED);
}


//+---------------------------------------------------------------------------
// S N A K I N G  i m p l e m e n t a t i o n
//----------------------------------------------------------------------------



//!!!!!! Please, Eliminate one of the GetSnaking...





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetDeferedPropertyUpdate
//
//  Synopsis:   Gets DeferedPropertyUpdate property
//
//  Arguments:  pfDeferedPropertyUpdate       ptr to location to return _fDeferedPropertyUpdate
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDeferedPropertyUpdate(VARIANT_BOOL * pfDeferedPropertyUpdate)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDeferedPropertyUpdate"));
    RRETURN(GetBool(pfDeferedPropertyUpdate, _pDoc->_fDeferedPropertyUpdate));
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetDeferedPropertyUpdate
//
//  Synopsis:   Sets fDeferedPropertyUpdate property
//
//  Arguments:  DeferedPropertyUpdate     set DeferedPropertyUpdate property to this
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetDeferedPropertyUpdate(VARIANT_BOOL fDeferedPropertyUpdate)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDeferedPropertyUpdate"));
    RRETURNSETBOOL(_pDoc->_fDeferedPropertyUpdate, fDeferedPropertyUpdate, DISPID_DeferedPropertyUpdate);
}






//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetShowHeaders
//
//  Synopsis:   Gets ShowHeaders
//
//  Arguments:  pfShowHeaders       ptr to location to return _fShowHeaders
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetShowHeaders(VARIANT_BOOL * pfShowHeaders)
{
    TraceTag((tagDataFrame,"CDataFrame::GetShowHeaders"));
    RRETURN(GetBool(pfShowHeaders, TBag()->_fShowHeaders));
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetShowHeaders
//
//  Synopsis:   Sets _fShowHeaders property
//
//  Arguments:  fShowHeaders
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetShowHeaders(VARIANT_BOOL fShowHeaders)
{
    HRESULT hr = S_OK;

    TraceTag((tagDataFrame,"CDataFrame::SetShowHeaders"));
    if (ENSURE_BOOL(TBag()->_fShowHeaders) != ENSURE_BOOL(fShowHeaders))
    {
        TBag()->_fShowHeaders = ENSURE_BOOL(fShowHeaders);
        if (TBag()->_fShowHeaders && !getTemplate()->_pHeader)
        {
            hr= getTemplate()->InitSubFrameTemplate(&getTemplate()->_pHeader, SITEDESC_HEADERFRAME);
        }
        if (!hr)
        {
            hr = OnDataChange (DISPID_ShowHeaders);
        }

    }
    RRETURN(CloseErrorInfo(hr));
}










//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetListBoxStyle
//
//  Synopsis:   Sets the ListBoxStyle property
//
//  Arguments:  eListBoxStyle: the enum'd value of none/listbox/combobox
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetListBoxStyle(fmListBoxStyles eListBoxStyle)
{
    if ( (eListBoxStyle < fmListBoxStylesNone) || (eListBoxStyle > fmListBoxStylesComboBox) )
        RRETURN(SetErrorInfoInvalidArg());

    if ( eListBoxStyle != (fmListBoxStyles)(TBag()->_eListBoxStyle) )
        OnListBoxStyleChange(eListBoxStyle);

    TBag()->_eListBoxStyle = eListBoxStyle;

    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetListBoxStyle
//
//  Synopsis:   Gets the ListBoxStyle property
//
//  Arguments:  peListBoxStyle: pointer to the enum'd value of none/listbox/combobox
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetListBoxStyle(fmListBoxStyles * peListBoxStyle)
{
    if ( ! peListBoxStyle )
        RRETURN(SetErrorInfoInvalidArg());

    *peListBoxStyle = (fmListBoxStyles)TBag()->_eListBoxStyle;

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetMultiSelect
//
//  Synopsis:   Sets the MultiSelect property
//
//  Arguments:  eMultiSelect: the enum'd value of none/listbox/combobox
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetMultiSelect(fmMultiSelect eMultiSelect)
{
    HRESULT hr = S_OK;
    CTBag * pTBag = TBag();

    if ( (eMultiSelect < fmMultiSelectSingle) || (eMultiSelect > fmMultiSelectExtended) )
        RRETURN(SetErrorInfoInvalidArg());

    if ( eMultiSelect != (fmMultiSelect)pTBag->_eMultiSelect )
    {
        if ( eMultiSelect == fmMultiSelectSingle &&
             _pDetail &&
             _pDetail->TestClassFlag(SITEDESC_REPEATER) &&
             ((CRepeaterFrame*)_pDetail)->_pCurrent )
        {
            RootFrame(this)->ClearSelection(TRUE);
        }

        pTBag->_eMultiSelect = eMultiSelect;

    }

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetMultiSelect
//
//  Synopsis:   Gets the MultiSelect property
//
//  Arguments:  peMultiSelect: pointer to the enum'd value of none/listbox/combobox
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetMultiSelect(fmMultiSelect * peMultiSelect)
{
    if ( ! peMultiSelect )
        RRETURN(SetErrorInfoInvalidArg());

    *peMultiSelect = (fmMultiSelect)TBag()->_eMultiSelect;

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetListStyle
//
//  Synopsis:   Sets the ListStyle property
//
//  Arguments:  iListStyle: the enum'd value of none/listbox/combobox
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetListStyle(fmListStyle eListStyle)
{
    #if PRODUCT_97
    if ( (eListStyle < fmListStylePlain) || (eListStyle > fmListStyleMark) )
    #else
    if ( (eListStyle < fmListStylePlain) || (eListStyle > fmListStyleOption) )
    #endif
        RRETURN(SetErrorInfoInvalidArg());

    TBag()->_eListStyle = eListStyle;

    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetListStyle
//
//  Synopsis:   Gets the ListStyle property
//
//  Arguments:  piListStyle: pointer to the enum'd value of none/listbox/combobox
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetListStyle(fmListStyle * peListStyle)
{
    if ( ! peListStyle )
        RRETURN(SetErrorInfoInvalidArg());

    *peListStyle = (fmListStyle)TBag()->_eListStyle;

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     GetListIndex
//
//  Synopsis:   Implements the ListIndex property for the listbox
//              the method makes the pListIndex 0 based at the end
//              because of the outer object model
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::GetListIndex (long * plListIndex)
{
    HRESULT hr;
    CTBag       *   pTBag = TBag();
    CDetailFrame  *   plfr;

    if ( ! plListIndex )
        RRETURN(SetErrorInfoInvalidArg());

    *plListIndex = 0;

    if ( ! pTBag->_eListBoxStyle )
    {
        *plListIndex = -1;
        return E_NOTIMPL;
    }

    if ( pTBag->_eMultiSelect == fmMultiSelectSingle )
    {
        //  return the index of the selected row or -1
        if ( _parySelectedQualifiers )
        {
#if NEVER
            Assert( (_parySelectedQualifiers->Size() == 1) &&
                    ((*_parySelectedQualifiers)[0]->pqEnd == NULL) &&
                    (((*_parySelectedQualifiers)[0]->qStart.type < QUALI_HEADER) ||
                     ((*_parySelectedQualifiers)[0]->qStart.type == QUALI_DETAIL)) );
#endif
            hr = RecordNumberFromSelector((*_parySelectedQualifiers)[0],(ULONG *)plListIndex);
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        //  return the index of the focus row
        if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
        {
            plfr = (CDetailFrame *)(((CRepeaterFrame *)_pDetail)->_pCurrent);
        }
        else
        {
            plfr = (CDetailFrame *)_pDetail;
        }

        if ( plfr )
        {
            hr = GetRecordNumber (plfr->getHRow(), (ULONG *)plListIndex);
        }
        else
        {
            hr = S_OK;
        }
    }

    if (!hr)
    {
        *plListIndex -= 1;
    }

    RRETURN(CloseErrorInfo(hr));
}




//+---------------------------------------------------------------------------
//
//  Member:     SetListIndex
//
//  Synopsis:   Implements the ListIndex property for the listbox
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::SetListIndex (long lListIndex)
{
    HRESULT             hr = S_OK;
    SelectUnit        * psu = NULL;
    CDetailFrame      * plfr = NULL;
    CDataLayerCursor  * pCursor;
    HROW                hrowNewCurrent;
    CTBag             * pTBag = TBag();

    if (lListIndex < 0)
    {
        //  deselect everything in single-select
        //  meaningless in multiselect
        if ( (pTBag->_eMultiSelect == fmMultiSelectSingle) )
        {
            RootFrame(this)->ClearSelection(FALSE);
            SetSelectionStates();
            //  Review: should we scroll the old current into view?
            //  It hasn't changed but lost selection.
        }
        else
        {
            SetCurrent(NULL);
        }
        goto Cleanup;
    }

    // add one to the listindex to
    // overcome differences between the object model and
    //  the cursor
    lListIndex += 1;

    psu = new SelectUnit;
    if ( !psu )
        goto MemoryError;

    hr = SelectorFromRecordNumber (lListIndex, psu);
    if (hr)
        goto Error;

    if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {

        // Create a layout for "iListIndex" row, make it current and bring that
        // layout into view.
        hr = ((CRepeaterFrame *)_pDetail)->CreateNewCurrentLayout (
                psu->qStart.bookmark, lListIndex, &plfr);
        // note, the S_FALSE is valid (it means we need to call create to fit).

        if (!SUCCEEDED(hr))
            goto Error;

        if (!plfr->IsInSiteList())
        {
            if (hr == S_OK)
            {
                plfr->ScrollIntoView ();
            }
            else
            {
                CRectl rcl;
                GetDetailSectionRectl (&rcl);
                _pDetail->CreateToFit (&rcl, SITEMOVE_POPULATEDIR);
            }
        }

        // Update the selection
        if (pTBag->_eMultiSelect == fmMultiSelectSingle)
        {
            RootFrame(this)->ClearSelection(FALSE);

            // Set the selection tree
            psu->aryfControls.SetReservedBit(SRB_LAYOUTFRAME_SELECTED);
            hr = SelectSubtree (psu, SS_ADDTOSELECTION);
            if ( hr )
                goto Error;

            // Update the instance selection state
            // (recursive, goes through all the children)
            SetSelectionStates();
        }
        else
        {
            delete psu;
            psu = NULL;
        }
    } else
    {
        // Move the detail section to the new row.
        hr = LazyGetDataLayerCursor(&pCursor);
        if (hr)
            goto Error;

        hr = pCursor->GetRowAt(psu->qStart.bookmark, &hrowNewCurrent);
        if (hr)
            goto Error;

        _pDetail->MoveToRow (hrowNewCurrent, 0);
    }
Cleanup:
    RRETURN(CloseErrorInfo(hr));

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    delete psu;
    goto Cleanup;

}




//+---------------------------------------------------------------------------
//
//  Member:     GetListItemSelected
//
//  Synopsis:   Implements the Selected property for the listbox
//
//  Argument:   lItemIndex:     The index for the simulated selected
//                              flag array
//
//  Note:       There's no corresponding data member in CDataFrame
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::GetListItemSelected (long lItemIndex, VARIANT_BOOL * pfSelected)
{
    HRESULT hr = S_OK;
    SelectUnit su;

    if ( ! pfSelected )
        RRETURN(SetErrorInfoInvalidArg());

    if ( ! _parySelectedQualifiers )
    {
        *pfSelected = VB_FALSE;
        goto Cleanup;
    }

    // object model adjustment
    lItemIndex += 1;

    su.pqEnd = NULL;    //  so that Clear() doesn't blow up
    su.Clear();
    hr = SelectorFromRecordNumber(lItemIndex, &su);
    if ( hr )
        goto Cleanup;

    *pfSelected = (NULL != _parySelectedQualifiers->Find(&su)) ? VB_TRUE : VB_FALSE;

Cleanup:
    RRETURN(CloseErrorInfo(hr));
}




//+---------------------------------------------------------------------------
//
//  Member:     SetListItemSelected
//
//  Synopsis:   Implements the Selected property for the listbox
//
//  Argument:   lItemIndex:     The index for the simulated selected
//                              flag array
//
//  Note:       There's no corresponding data member in CDataFrame
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::SetListItemSelected (long lItemIndex, VARIANT_BOOL fSelected)
{
    HRESULT hr = S_OK;
    SelectUnit * psu = NULL;

    if ( (! _parySelectedQualifiers) && (! fSelected) )
        goto Cleanup;

    psu = new SelectUnit;
    if ( !psu )
        goto MemoryError;

    // object model adjustment
    lItemIndex += 1;

    hr = SelectorFromRecordNumber(lItemIndex, psu);
    if ( hr )
        goto Error;

    psu->aryfControls.SetReservedBit(SRB_LAYOUTFRAME_SELECTED);

    if ( fSelected )
    {
        if ( TBag()->_eMultiSelect == fmMultiSelectSingle )
        {
            RootFrame(this)->ClearSelection(FALSE);
        }
        hr = SelectSubtree(psu, SS_ADDTOSELECTION);
        if (FAILED(hr))
            goto Error;
        else if ( hr == S_FALSE )
        {
            delete psu;
            psu = 0;
            hr = S_OK;
        }
    }
    else
    {
        //  Call the dataframe, it can split the selection
        hr = DeselectSubtree(psu, SS_REMOVEFROMSELECTION);
        delete psu;
        psu = NULL;
        if (FAILED(hr))
            goto Error;
        else
            hr = S_OK;
    }

    SetSelectionStates();

    hr = OnDataChangeCloseError(DISPID_DATADOC_ListItemSelected, TRUE);

Cleanup:
    RRETURN(CloseErrorInfo(hr));

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    delete psu;
    goto Cleanup;

}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetMinRows
//
//  Synopsis:   Get Min Rows
//
//  Arguments:  plMinRows
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetMinRows (long * plMinRows)
{
    RRETURN(GetLong(plMinRows, TBag()->_lMinRows));
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetMinRows
//
//  Synopsis:
//
//  Arguments:  lMinRows
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetMinRows (long lMinRows)
{
    CTBag * pTBag = TBag();

    if (lMinRows != pTBag->_lMinRows)
    {
        if (lMinRows < 0)
        {
            RRETURN(SetErrorInfoBadValue(DISPID_MinRows,
                                     IDS_ES_ENTER_VALUE_GT_ZERO));
        }

        if (pTBag->_lMaxRows >= 0 && lMinRows > pTBag->_lMaxRows)
        {
            pTBag->_lMaxRows = lMinRows;
        }

        pTBag->_lMinRows = lMinRows;
        TBag()->_fMinMaxPropertyChanged = TRUE;
        RRETURN(OnDataChangeCloseError(DISPID_MinRows, TRUE));
    }
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetMaxRows
//
//  Synopsis:
//
//  Arguments:  plMaxRows
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetMaxRows (long * plMaxRows)
{
    RRETURN(GetLong(plMaxRows, TBag()->_lMaxRows));
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetMaxRows
//
//  Synopsis:
//
//  Arguments:  lMaxRows
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetMaxRows (long lMaxRows)
{
    CTBag * pTBag = TBag();

    if (lMaxRows != pTBag->_lMaxRows)
    {
        if (lMaxRows < FITPARENT)
        {
            RRETURN(SetErrorInfoBadValue(DISPID_MaxRows,
                                     IDS_ES_ENTER_VALUE_IN_RANGE,
                                     FITPARENT, 100000));
        }

        if (lMaxRows >= 0)
        {
            if (lMaxRows < pTBag->_lMinRows)
            {
                pTBag->_lMinRows = lMaxRows;
            }
        }
        pTBag->_lMaxRows = lMaxRows;
        TBag()->_fMinMaxPropertyChanged = TRUE;
        RRETURN(OnDataChangeCloseError(DISPID_MaxRows, TRUE));
    }

    return S_OK;
}





#if PRODUCT_97


// #if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetRunTestcode
//
//  Synopsis:   Excecutes testing code, only for debug
//
//  Arguments:  fRunTestcode - what testprogram to run
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetRunTestcode(int iRunTestcode)
{
    HRESULT hr = S_OK;

    TraceTag((tagDataFrame,"CDataFrame::SetRunTestcode"));

    switch (iRunTestcode)
    {
        case 0:
            hr = getTemplate()->SetColumnHeads(TRUE);
            break;
    }
    RRETURN(CloseErrorInfo(hr));
}

// #endif


//+------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetDetail
//
//  Synopsis:   Returns the Detail for this data frame
//
//  Arguments:  [ppDetail]  Detail returned in *ppDetail
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDetail(IDataFrame ** ppDetail)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDetail"));

    if (ppDetail == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    RRETURN(getTemplate()->_pDetail->QueryInterface(IID_IDataFrame, (void**)ppDetail));
}



STDMETHODIMP
CDataFrame::GetDatabase(BSTR* pbstrDatabase)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDatabase"));
    HRESULT hr;

    if (pbstrDatabase == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (HasDLAccessorSource())
        {
            CDataLayerAccessorSource *pdlas;
            IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
            Assert(pdlas);
            hr = THR(pdlas->GetDatabase().AllocBSTR(pbstrDatabase));
        }
        else
        {
            *pbstrDatabase = NULL;
            hr = S_OK;
        }
    }
    RRETURN(hr);
}


STDMETHODIMP
CDataFrame::SetDatabase(LPTSTR bstrDatabase)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDatabase"));

    CDataLayerAccessorSource *pdlas;
    HRESULT hr=S_OK;

    if (bstrDatabase)
    {
        hr = THR(LazyGetDLAccessorSource(&pdlas));
        if (hr || pdlas->GetDatabase() &&
                  !_tcsicmp(pdlas->GetDatabase(), bstrDatabase) )
        {
            goto Cleanup;
        }
    }
    else if (HasDLAccessorSource())
    {
        IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
        Assert(pdlas);
    }
    else
    {
        goto Cleanup;
    }

    hr = THR(pdlas->SetDatabase(bstrDatabase));
    if (hr)
    {
        goto Cleanup;
    }
    hr = OnDataChange(DISPID_Database, TRUE);  // BUGBUG: Is this right?

Cleanup:
    RRETURN(CloseErrorInfo(hr));
}



//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetActiveInstance
//
//  Synopsis    Gets the active instance property
//
//  Arguments   ppdispActiveInstance    pointer where the active instance
//                                      address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetActiveInstance(IDispatch** ppdispActiveInstance)
{
    TraceTag((tagDataFrame,"CDataFrame::GetActiveInstance"));
    if (ppdispActiveInstance == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    return E_NOTIMPL;
}


//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetAllowAdditions
//
//  Synopsis    Gets the _fAllowAdditions property
//
//  Arguments   pfAllowAdditions    pointer where to put the property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAllowAdditions(VARIANT_BOOL* pfAllowAdditions)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAllowAdditions"));
    RRETURN(GetBool(pfAllowAdditions, TBag()->_fAllowAdditions));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetAllowAdditions
//
//  Synopsis    Sets the _fAllowAdditions property
//
//  Arguments   fAllowAdditions     new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAllowAdditions(VARIANT_BOOL  fAllowAdditions)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAllowAdditions"));
    RRETURNSETBOOL(TBag()->_fAllowAdditions, fAllowAdditions, DISPID_AllowAdditions);
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetAllowDeletions
//
//  Synopsis    Gets the _fAllowDeletions property
//
//  Arguments   pfAllowDeletions        pointer where to put the property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAllowDeletions(VARIANT_BOOL* pfAllowDeletions)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAllowDeletions"));
    RRETURN(GetBool(pfAllowDeletions, TBag()->_fAllowDeletions));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetAllowDeletions
//
//  Synopsis    Sets the _fAllowDeletions property
//
//  Arguments   fAllowDeletions     new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAllowDeletions(VARIANT_BOOL  fAllowDeletions)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAllowDeletions"));
    RRETURNSETBOOL(TBag()->_fAllowDeletions, fAllowDeletions, DISPID_AllowDeletions);
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetAllowEditing
//
//  Synopsis    Gets the _fAllowEditing property
//
//  Arguments   pfAllowEditing      pointer where to put the property to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAllowEditing(VARIANT_BOOL* pfAllowEditing)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAllowEditing"));
    RRETURN(GetBool(pfAllowEditing, TBag()->_fAllowEditing));
}


//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetAllowEditing
//
//  Synopsis    Sets the _fAllowEditing  property
//
//  Arguments   fAllowEditing       new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAllowEditing(VARIANT_BOOL  fAllowEditing)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAllowEditing"));
    RRETURNSETBOOL(TBag()->_fAllowEditing, fAllowEditing, DISPID_AllowEditing);
}


//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetAllowFilters
//
//  Synopsis    Gets the _fAllowFilters property
//
//  Arguments   pfAllowFilters      pointer where to put the property to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAllowFilters(VARIANT_BOOL* pfAllowFilters)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAllowFilters"));
    RRETURN(GetBool(pfAllowFilters, TBag()->_fAllowFilters));
}


//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetAllowFilters
//
//  Synopsis    Sets the _fAllowFilters property
//
//  Arguments   fAllowFilters       new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAllowFilters(VARIANT_BOOL  fAllowFilters)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAllowFilters"));
    RRETURNSETBOOL(TBag()->_fAllowFilters, fAllowFilters, DISPID_AllowFilters);
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetAllowUpdating
//
//  Synopsis    Gets the _iAllowUpdating property
//
//  Arguments   piAllowUpdating    pointer where the property goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAllowUpdating(VARIANT_BOOL* pfAllowUpdating)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAllowUpdating"));
    RRETURN(GetBool(pfAllowUpdating, TBag()->_iAllowUpdating));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetAllowUpdating
//
//  Synopsis    Sets the _iAllowUpdating property
//
//  Arguments   iAllowUpdating      new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAllowUpdating(VARIANT_BOOL fAllowUpdating)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAllowUpdating"));
    RRETURNSETBOOL(TBag()->_iAllowUpdating, fAllowUpdating, DISPID_AllowUpdating);
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetAlternateBackColor
//
//  Synopsis    Gets the _colorAlternateBackColor property
//
//  Arguments   pcolorAlternateBackColor    pointer where the property goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAlternateBackColor(OLE_COLOR* pcolorAlternateBackColor)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAlternateBackColor"));
    RRETURN(GetColor(pcolorAlternateBackColor, TBag()->_colorAlternateBackColor));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetAlternateBackColor
//
//  Synopsis    Sets the _colorAlternateBackColor property
//
//  Arguments   colorAlternateBackColor     new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAlternateBackColor(OLE_COLOR  colorAlternateBackColor)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAlternateBackColor"));
    RRETURN(SetColor(&(TBag()->_colorAlternateBackColor), colorAlternateBackColor, DISPID_AlternateBackColor));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetAlternateInterval
//
//  Synopsis    Gets the _iAlternateInterval property
//
//  Arguments   piAlternateInterval    pointer where the _iAlternateInterval
//                                     address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAlternateInterval(int * piAlternateInterval)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAlternateInterval"));
    RRETURN(GetInt(piAlternateInterval, TBag()->_iAlternateInterval));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetAlternateInterval
//
//  Synopsis    Sets the _iAlternateInterval property
//
//  Arguments   iAlternateInterval    new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAlternateInterval(int  iAlternateInterval)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAlternateInterval"));
    RRETURN(SetInt(&(TBag()->_iAlternateInterval), iAlternateInterval, DISPID_AlternateInterval));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetAutoEject
//
//  Synopsis    Gets the _fAutoEject property
//
//  Arguments   pfAutoEject    pointer where the _fAutoEject address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetAutoEject(VARIANT_BOOL* pfAutoEject)
{
    TraceTag((tagDataFrame,"CDataFrame::GetAutoEject"));
    RRETURN(GetBool(pfAutoEject, TBag()->_fAutoEject));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetAutoEject
//
//  Synopsis    Sets the _fAutoEject property
//
//  Arguments   fAutoEject      new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetAutoEject(VARIANT_BOOL  fAutoEject)
{
    TraceTag((tagDataFrame,"CDataFrame::SetAutoEject"));
    RRETURNSETBOOL(TBag()->_fAutoEject, fAutoEject, DISPID_AutoEject);
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetColumnSpacing
//
//  Synopsis    Gets the _iColumnSpacing property
//
//  Arguments   piColumnSpacing    pointer where the _iColumnSpacing address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetColumnSpacing(int * piColumnSpacing)
{
    TraceTag((tagDataFrame,"CDataFrame::GetColumnSpacing"));
    CTBag *pTBag = TBag();
    RRETURN(GetInt(piColumnSpacing, pTBag->_uPadding[!pTBag->_fDirection]));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetColumnSpacing
//
//  Synopsis    Sets the _iColumSpacing property
//
//  Arguments   iColumnSpacing      new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetColumnSpacing(int  iColumnSpacing)
{
    TraceTag((tagDataFrame,"CDataFrame::SetColumnSpacing"));
    CTBag *pTBag = TBag();
    RRETURN(SetInt((int*)&(pTBag->_uPadding[!pTBag->_fDirection]), iColumnSpacing, DISPID_ColumnSpacing));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetRowSpacing
//
//  Synopsis    Gets the _iRowSpacing property
//
//  Arguments   piRowSpacing    pointer where the _iRowSpacing address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRowSpacing(int * piRowSpacing)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRowSpacing"));
    CTBag *pTBag = TBag();
    RRETURN(GetInt(piRowSpacing, pTBag->_uPadding[pTBag->_fDirection]));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetRowSpacing
//
//  Synopsis    Sets the _iColumSpacing property
//
//  Arguments   iRowSpacing      new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRowSpacing(int  iRowSpacing)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRowSpacing"));
    CTBag *pTBag = TBag();
    RRETURN(SetInt((int*)&(pTBag->_uPadding[pTBag->_fDirection]), iRowSpacing, DISPID_RowSpacing));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetCommitSync
//
//  Synopsis    Gets the _iCommitSync property
//
//  Arguments   piCommitSync    pointer where the _iCommitSync address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetCommitSync(int * piCommitSync)
{
    TraceTag((tagDataFrame,"CDataFrame::GetCommitSync"));
    RRETURN(GetInt(piCommitSync, TBag()->_iCommitSync));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetCommitSync
//
//  Synopsis    Sets the _iCommitSync property
//
//  Arguments   iCommitSync     new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetCommitSync(int  iCommitSync)
{
    TraceTag((tagDataFrame,"CDataFrame::SetCommitSync"));
    RRETURN(SetInt(&(TBag()->_iCommitSync), iCommitSync, DISPID_CommitSync));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetCommitWhat
//
//  Synopsis    Gets the _iCommitWhat property
//
//  Arguments   piCommitWhat    pointer where the _iCommitWhat address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetCommitWhat(int * piCommitWhat)
{
    TraceTag((tagDataFrame,"CDataFrame::GetCommitWhat"));
    RRETURN(GetInt(piCommitWhat, TBag()->_iCommitWhat));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetCommitWhat
//
//  Synopsis    Sets the _iCommitWhat property
//
//  Arguments   iCommitWhat    new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetCommitWhat(int  iCommitWhat)
{
    TraceTag((tagDataFrame,"CDataFrame::SetCommitWhat"));
    RRETURN(SetInt(&(TBag()->_iCommitWhat), iCommitWhat, DISPID_CommitWhat));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetCommitWhen
//
//  Synopsis    Gets the _iCommitWhen property
//
//  Arguments   piCommitWhen    pointer where the _iCommitWhen address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetCommitWhen(int * piCommitWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::GetCommitWhen"));
    RRETURN(GetInt(piCommitWhen, TBag()->_iCommitWhen));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetCommitWhen
//
//  Synopsis    Sets the _iCommitWhen property
//
//  Arguments   iCommitWhen     new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetCommitWhen(int  iCommitWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::SetCommitWhen"));
    RRETURN(SetInt(&(TBag()->_iCommitWhen), iCommitWhen, DISPID_CommitWhen));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetContinousForm
//
//  Synopsis    Gets the _fContinousForm property
//
//  Arguments   pfContinousForm    pointer where the _fContinousForm address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetContinousForm(VARIANT_BOOL* pfContinousForm)
{
    TraceTag((tagDataFrame,"CDataFrame::GetContinousForm"));
    RRETURN(GetBool(pfContinousForm, TBag()->_fContinousForm));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetcontinousForm
//
//  Synopsis    Gets the _fContinousForm property
//
//  Arguments   fContinousForm    new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetContinousForm(VARIANT_BOOL  fContinousForm)
{
    TraceTag((tagDataFrame,"CDataFrame::SetContinousForm"));
    RRETURNSETBOOL(TBag()->_fContinousForm, fContinousForm, DISPID_ContinousForm);
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetDataOnly
//
//  Synopsis    Gets the _fDataOnly property
//
//  Arguments   pfDataOnly    pointer where the _fDataOnly address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDataOnly(VARIANT_BOOL* pfDataOnly)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDataOnly"));
    RRETURN(GetBool(pfDataOnly, TBag()->_fDataOnly));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetDataOnly
//
//  Synopsis    Sets the _fDataOnly property
//
//  Arguments   fDataOnly    new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDataOnly(VARIANT_BOOL  fDataOnly)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDataOnly"));
    RRETURNSETBOOL(TBag()->_fDataOnly, fDataOnly, DISPID_DataOnly);
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetDataGrouping
//
//  Synopsis    Gets the _iDateGrouping property
//
//  Arguments   piDateGrouping    pointer where the _iDateGrouping address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDateGrouping(int * piDateGrouping)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDateGrouping"));
    RRETURN(GetInt(piDateGrouping, TBag()->_iDateGrouping));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetDateGrouping
//
//  Synopsis    Sets the _iDateGrouping property
//
//  Arguments   iDateGrouping       new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDateGrouping(int  iDateGrouping)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDateGrouping"));
    RRETURN(SetInt(&(TBag()->_iDateGrouping), iDateGrouping, DISPID_DateGrouping));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetDefaultEditing
//
//  Synopsis    Gets the _iDefaultEditing property
//
//  Arguments   piDefaultEditing    pointer where the _iDefaultEditing address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDefaultEditing(int * piDefaultEditing)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDefaultEditing"));
    RRETURN(GetInt(piDefaultEditing, TBag()->_iDefaultEditing));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetDefaultEditing
//
//  Synopsis    Sets the _iDefaultEditing property
//
//  Arguments   iDefaultEditing     new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDefaultEditing(int  iDefaultEditing)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDefaultEditing"));
    RRETURN(SetInt(&(TBag()->_iDefaultEditing), iDefaultEditing, DISPID_DefaultEditing));
}

//+------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetHeader
//
//  Synopsis:   Returns the Header for this data frame
//
//  Arguments:  [ppHeader]  Header returned in *ppHeader
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetHeader(IDataFrame ** ppHeader)
{
    TraceTag((tagDataFrame,"CDataFrame::GetHeader"));
    HRESULT hr = E_FAIL;

    if (ppHeader == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    if (getTemplate()->_pHeader)
    {
        hr = THR(getTemplate()->_pHeader->QueryInterface(IID_IDataFrame, (void**)ppHeader));
    }
    RRETURN(hr);
}




//+------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetFooter
//
//  Synopsis:   Returns the Footer for this data frame
//
//  Arguments:  [ppFooter]  Footer returned in *ppFooter
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetFooter(IDataFrame ** ppFooter)
{
    TraceTag((tagDataFrame,"CDataFrame::GetFooter"));
    HRESULT hr = E_FAIL;

    if (ppFooter == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    if (getTemplate()->_pFooter)
    {
        hr = THR(getTemplate()->_pFooter->QueryInterface(IID_IDataFrame, (void**)ppFooter));
    }
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetDirty
//
//  Synopsis    Gets the _fDirty property
//
//  Arguments   pfDirty      pointer where the _fDirty address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDirty(VARIANT_BOOL* pfDirty)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDirty"));
    RRETURN(GetBool(pfDirty, TBag()->_fDirty));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetDirty
//
//  Synopsis    Sets the _fDirty property
//
//  Arguments   fDirty      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDirty(VARIANT_BOOL  fDirty)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDirty"));
    RRETURNSETBOOL(TBag()->_fDirty, fDirty, DISPID_Dirty);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetDirtyDataColor
//
//  Synopsis    Gets the _colorDirtyDataColor property
//
//  Arguments   pcolorDirtyDataColor         pointer where the _colorDirtyDataColor
//                                           address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDirtyDataColor(OLE_COLOR* pcolorDirtyDataColor)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDirtyDataColor"));
    RRETURN(GetColor(pcolorDirtyDataColor, TBag()->_colorDirtyDataColor));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetDirtyDataColor
//
//  Synopsis    Sets the _colorDirtyDataColor property
//
//  Arguments   colorDirtyDataColor     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDirtyDataColor(OLE_COLOR  colorDirtyDataColor)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDirtyDataColor"));
    RRETURN(SetColor(&(TBag()->_colorDirtyDataColor), colorDirtyDataColor, DISPID_DirtyDataColor));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetDirtyPencil
//
//  Synopsis    Gets the _fDirtyPencil property
//
//  Arguments   pfDirtyPencil        pointer where the _fDirtyPencil address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDirtyPencil(VARIANT_BOOL* pfDirtyPencil)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDirtyPencil"));
    RRETURN(GetBool(pfDirtyPencil, TBag()->_fDirtyPencil));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetDirtyPencil
//
//  Synopsis    Sets the _fDirtyPencil property
//
//  Arguments   fDirtyPencil        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDirtyPencil(VARIANT_BOOL  fDirtyPencil)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDirtyPencil"));
    RRETURNSETBOOL(TBag()->_fDirtyPencil,fDirtyPencil, DISPID_DirtyPencil);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetDisplayWhen
//
//  Synopsis    Gets the _iDisplayWhen property
//
//  Arguments   piDisplayWhen        pointer where the _iDisplayWhen address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDisplayWhen(int * piDisplayWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDisplayWhen"));
    RRETURN(GetInt(piDisplayWhen, TBag()->_iDisplayWhen));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetDisplayWhen
//
//  Synopsis    Sets the _iDisplayWhen property
//
//  Arguments   iDisplayWhen        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDisplayWhen(int  iDisplayWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDisplayWhen"));
    RRETURN(SetInt(&(TBag()->_iDisplayWhen), iDisplayWhen, DISPID_DisplayWhen));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetFooterTTexxt
//
//  Synopsis    Gets the TBag()->_cstrFooter property
//
//  Arguments   pbstrFooter      pointer where the TBag()->_cstrFooter address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetFooterText(BSTR* pbstrFooter)
{
    TraceTag((tagDataFrame,"CDataFrame::GetFooterText"));
    RRETURN(GetBstr(pbstrFooter, TBag()->_cstrFooter));

}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetFooterText
//
//  Synopsis    Sets the TBag()->_cstrFooter property
//
//  Arguments   bstrFooter      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetFooterText(LPTSTR  bstrFooter)
{
    TraceTag((tagDataFrame,"CDataFrame::SetFooterText"));
    RRETURN(SetBstr(bstrFooter,TBag()->_cstrFooter,DISPID_FooterText));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetForceNewPage
//
//  Synopsis    Gets the _iForceNewPage property
//
//  Arguments   piForceNewPage       pointer where the _iForceNewPage address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetForceNewPage(int * piForceNewPage)
{
    TraceTag((tagDataFrame,"CDataFrame::GetForceNewPage"));
    RRETURN(GetInt(piForceNewPage, TBag()->_iForceNewPage));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetForceNewPage
//
//  Synopsis    Sets the _iForceNewPage property
//
//  Arguments   iForceNewPage       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetForceNewPage(int  iForceNewPage)
{
    TraceTag((tagDataFrame,"CDataFrame::SetForceNewPage"));
    RRETURN(SetInt(&(TBag()->_iForceNewPage), iForceNewPage, DISPID_ForceNewPage));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetFormatCount
//
//  Synopsis    Gets the _lFormatCount property
//
//  Arguments   plFormatCount        pointer where the _lFormatCount address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetFormatCount(long* plFormatCount)
{
    TraceTag((tagDataFrame,"CDataFrame::GetFormatCount"));
    RRETURN(GetLong(plFormatCount, TBag()->_lFormatCount));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetFormatCount
//
//  Synopsis    Sets the _lFormatCount property
//
//  Arguments   lFormatCount        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetFormatCount(long  lFormatCount)
{
    TraceTag((tagDataFrame,"CDataFrame::SetFormatCount"));
    RRETURN(SetLong(&(TBag()->_lFormatCount), lFormatCount, DISPID_FormatCount));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetFullIntervals
//
//  Synopsis    Gets the _fFullIntervals property
//
//  Arguments   pfFullIntervals      pointer where the _fFullIntervals address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetFullIntervals(VARIANT_BOOL* pfFullIntervals)
{
    TraceTag((tagDataFrame,"CDataFrame::GetFullIntervals"));
    RRETURN(GetBool(pfFullIntervals, TBag()->_fFullIntervals));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetFullIntervals
//
//  Synopsis    Sets the _fFullIntervals property
//
//  Arguments   fFullIntervals      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetFullIntervals(VARIANT_BOOL  fFullIntervals)
{
    TraceTag((tagDataFrame,"CDataFrame::SetFullIntervals"));
    RRETURNSETBOOL(TBag()->_fFullIntervals, fFullIntervals, DISPID_FullIntervals);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetGroupInterval
//
//  Synopsis    Gets the _iGroupInterval property
//
//  Arguments   piGroupInterval      pointer where the _iGroupInterval address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetGroupInterval(int * piGroupInterval)
{
    TraceTag((tagDataFrame,"CDataFrame::GetGroupInterval"));
    RRETURN(GetInt(piGroupInterval, TBag()->_iGroupInterval));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetGroupInterval
//
//  Synopsis    Sets the _iGroupInterval property
//
//  Arguments   iGroupInterval      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetGroupInterval(int  iGroupInterval)
{
    TraceTag((tagDataFrame,"CDataFrame::SetGroupInterval"));
    RRETURN(SetInt(&(TBag()->_iGroupInterval), iGroupInterval, DISPID_GroupInterval));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetGroupOn
//
//  Synopsis    Gets the _iGroupOn property
//
//  Arguments   piGroupOn        pointer where the _iGroupOn address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetGroupOn(int * piGroupOn)
{
    TraceTag((tagDataFrame,"CDataFrame::GetGroupOn"));
    RRETURN(GetInt(piGroupOn, TBag()->_iGroupOn));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetGroupOn
//
//  Synopsis    Sets the _iGroupOn property
//
//  Arguments   iGroupOn        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetGroupOn(int  iGroupOn)
{
    TraceTag((tagDataFrame,"CDataFrame::SetGroupOn"));
    RRETURN(SetInt(&(TBag()->_iGroupOn), iGroupOn, DISPID_GroupOn));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetHeaderText
//
//  Synopsis    Gets the TBag()->_cstrHeader property
//
//  Arguments   pbstrHeader      pointer where the TBag()->_cstrHeader address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetHeaderText(BSTR* pbstrHeader)
{
    TraceTag((tagDataFrame,"CDataFrame::GetHeaderText"));
    RRETURN(GetBstr(pbstrHeader, TBag()->_cstrHeader));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetHeaderText
//
//  Synopsis    Sets the TBag()->_cstrHeader property
//
//  Arguments   bstrHeader      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetHeaderText(LPTSTR  bstrHeader)
{
    TraceTag((tagDataFrame,"CDataFrame::SetHeaderText"));
    RRETURN(SetBstr(bstrHeader,TBag()->_cstrHeader,DISPID_HeaderText));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetHideDuplicates
//
//  Synopsis    Gets the _fHideDuplicates property
//
//  Arguments   pfHideDuplicates         pointer where the _fHideDuplicates
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetHideDuplicates(VARIANT_BOOL* pfHideDuplicates)
{
    TraceTag((tagDataFrame,"CDataFrame::GetHideDuplicates"));
    RRETURN(GetBool(pfHideDuplicates, TBag()->_fHideDuplicates));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetHideDuplicates
//
//  Synopsis    Sets the _fHideDuplicates property
//
//  Arguments   fHideDuplicates     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetHideDuplicates(VARIANT_BOOL  fHideDuplicates)
{
    TraceTag((tagDataFrame,"CDataFrame::SetHideDuplicates"));
    RRETURNSETBOOL(TBag()->_fHideDuplicates, fHideDuplicates, DISPID_HideDuplicates);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetKeepTogether
//
//  Synopsis    Gets the _iKeepTogether property
//
//  Arguments   piKeepTogether       pointer where the _iKeepTogether address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetKeepTogether(int * piKeepTogether)
{
    TraceTag((tagDataFrame,"CDataFrame::GetKeepTogether"));
    RRETURN(GetInt(piKeepTogether, TBag()->_iKeepTogether));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetKeepTogether
//
//  Synopsis    Sets the _iKeepTogether property
//
//  Arguments   iKeepTogether       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetKeepTogether(int  iKeepTogether)
{
    TraceTag((tagDataFrame,"CDataFrame::SetKeepTogether"));
    RRETURN(SetInt(&(TBag()->_iKeepTogether), iKeepTogether, DISPID_KeepTogether));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetLayoutForPrint
//
//  Synopsis    Gets the _iLayoutForPrint property
//
//  Arguments   piLayoutForPrint         pointer where the _iLayoutForPrint
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetLayoutForPrint(int * piLayoutForPrint)
{
    TraceTag((tagDataFrame,"CDataFrame::GetLayoutForPrint"));
    RRETURN(GetInt(piLayoutForPrint, TBag()->_iLayoutForPrint));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetLayoutForPrint
//
//  Synopsis    Sets the _iLayoutForPrint property
//
//  Arguments   iLayoutForPrint     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetLayoutForPrint(int  iLayoutForPrint)
{
    TraceTag((tagDataFrame,"CDataFrame::SetLayoutForPrint"));
    RRETURN(SetInt(&(TBag()->_iLayoutForPrint), iLayoutForPrint, DISPID_LayoutForPrint));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetLockRecord
//
//  Synopsis    Gets the _fLockRecord property
//
//  Arguments   pfLockRecord         pointer where the _fLockRecord address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetLockRecord(VARIANT_BOOL* pfLockRecord)
{
    TraceTag((tagDataFrame,"CDataFrame::GetLockRecord"));
    RRETURN(GetBool(pfLockRecord, _fLockRecord));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetLockRecord
//
//  Synopsis    Sets the _fLockRecord property
//
//  Arguments   fLockRecord     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetLockRecord(VARIANT_BOOL  fLockRecord)
{
    TraceTag((tagDataFrame,"CDataFrame::SetLockRecord"));
    RRETURNSETBOOL(_fLockRecord, fLockRecord, DISPID_LockRecord);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetMoveLayout
//
//  Synopsis    Gets the _fMoveLayout property
//
//  Arguments   pfMoveLayout         pointer where the _fMoveLayout address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetMoveLayout(VARIANT_BOOL* pfMoveLayout)
{
    TraceTag((tagDataFrame,"CDataFrame::GetMoveLayout"));
    RRETURN(GetBool(pfMoveLayout, TBag()->_fMoveLayout));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetMoveLayout
//
//  Synopsis    Sets the _fMoveLayout property
//
//  Arguments   fMoveLayout     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetMoveLayout(VARIANT_BOOL  fMoveLayout)
{
    TraceTag((tagDataFrame,"CDataFrame::SetMoveLayout"));
    RRETURNSETBOOL(TBag()->_fMoveLayout, fMoveLayout, DISPID_MoveLayout);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetNavigationButtons
//
//  Synopsis    Gets the _fNavigationButtons property
//
//  Arguments   pfNavigationButtons      pointer where the _fNavigationButtons
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetNavigationButtons(VARIANT_BOOL* pfNavigationButtons)
{
    TraceTag((tagDataFrame,"CDataFrame::GetNavigationButtons"));
    RRETURN(GetBool(pfNavigationButtons, TBag()->_fNavigationButtons));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetNavigationButtons
//
//  Synopsis    Sets the _fNavigationButtons property
//
//  Arguments   fNavigationButtons      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetNavigationButtons(VARIANT_BOOL  fNavigationButtons)
{
    TraceTag((tagDataFrame,"CDataFrame::SetNavigationButtons"));
    RRETURNSETBOOL(TBag()->_fNavigationButtons, fNavigationButtons, DISPID_NavigationButtons);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetNewRowOrCol
//
//  Synopsis    Gets the _iNewRowOrCol property
//
//  Arguments   piNewRowOrCol        pointer where the _iNewRowOrCol address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetNewRowOrCol(int * piNewRowOrCol)
{
    TraceTag((tagDataFrame,"CDataFrame::GetNewRowOrCol"));
    RRETURN(GetInt(piNewRowOrCol, TBag()->_iNewRowOrCol));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetNewRowOrCol
//
//  Synopsis    Sets the _iNewRowOrCol property
//
//  Arguments   iNewRowOrCol        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetNewRowOrCol(int  iNewRowOrCol)
{
    TraceTag((tagDataFrame,"CDataFrame::SetNewRowOrCol"));
    RRETURN(SetInt(&(TBag()->_iNewRowOrCol), iNewRowOrCol, DISPID_NewRowOrCol));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetNewStar
//
//  Synopsis    Gets the _fNewStar property
//
//  Arguments   pfNewStar        pointer where the _fNewStar address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetNewStar(VARIANT_BOOL* pfNewStar)
{
    TraceTag((tagDataFrame,"CDataFrame::GetNewStar"));
    RRETURN(GetBool(pfNewStar, TBag()->_fNewStar));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetNewStar
//
//  Synopsis    Sets the _fNewStar property
//
//  Arguments   fNewStar        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetNewStar(VARIANT_BOOL  fNewStar)
{
    TraceTag((tagDataFrame,"CDataFrame::SetNewStar"));
    RRETURNSETBOOL(TBag()->_fNewStar, fNewStar, DISPID_NewStar);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetNextRecord
//
//  Synopsis    Gets the _fNextRecord property
//
//  Arguments   pfNextRecord         pointer where the _fNextRecord address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetNextRecord(VARIANT_BOOL* pfNextRecord)
{
    TraceTag((tagDataFrame,"CDataFrame::GetNextRecord"));
    RRETURN(GetBool(pfNextRecord, TBag()->_fNextRecord));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetNextRecord
//
//  Synopsis    Sets the _fNextRecord property
//
//  Arguments   fNextRecord     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetNextRecord(VARIANT_BOOL  fNextRecord)
{
    TraceTag((tagDataFrame,"CDataFrame::SetNextRecord"));
    RRETURNSETBOOL(TBag()->_fNextRecord, fNextRecord, DISPID_NextRecord);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetNormalDataColor
//
//  Synopsis    Gets the _colorNormalDataColor property
//
//  Arguments   pcolorNormalDataColor        pointer where the
//                                           TBag()->_colorNormalDataColor address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetNormalDataColor(OLE_COLOR* pcolorNormalDataColor)
{
    TraceTag((tagDataFrame,"CDataFrame::GetNormalDataColor"));
    RRETURN(GetColor(pcolorNormalDataColor, TBag()->_colorNormalDataColor));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetNormalDataColor
//
//  Synopsis    Sets the _colorNormalDataColor property
//
//  Arguments   colorNormalDataColor        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetNormalDataColor(OLE_COLOR  colorNormalDataColor)
{
    TraceTag((tagDataFrame,"CDataFrame::SetNormalDataColor"));
    RRETURN(SetColor(&(TBag()->_colorNormalDataColor), colorNormalDataColor, DISPID_NormalDataColor));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOneFooterPerPage
//
//  Synopsis    Gets the _fOneFooterPerPage property
//
//  Arguments   pfOneFooterPerPage       pointer where the
//                                       TBag()->_fOneFooterPerPage address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOneFooterPerPage(VARIANT_BOOL* pfOneFooterPerPage)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOneFooterPerPage"));
    RRETURN(GetBool(pfOneFooterPerPage, TBag()->_fOneFooterPerPage));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOneFooterPerPage
//
//  Synopsis    Sets the _fOneFooterPerPage property
//
//  Arguments   fOneFooterPerPage       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOneFooterPerPage(VARIANT_BOOL  fOneFooterPerPage)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOneFooterPerPage"));
    RRETURNSETBOOL(TBag()->_fOneFooterPerPage, fOneFooterPerPage, DISPID_OneFooterPerPage);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOneHeaderPerPage
//
//  Synopsis    Gets the _fOneHeaderPerPage property
//
//  Arguments   pfOneHeaderPerPage       pointer where the
//                                       TBag()->_fOneHeaderPerPage address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOneHeaderPerPage(VARIANT_BOOL* pfOneHeaderPerPage)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOneHeaderPerPage"));
    RRETURN(GetBool(pfOneHeaderPerPage, TBag()->_fOneHeaderPerPage));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOneHeaderPerPage
//
//  Synopsis    Sets the _fOneHeaderPerPage property
//
//  Arguments   fOneHeaderPerPage       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOneHeaderPerPage(VARIANT_BOOL  fOneHeaderPerPage)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOneHeaderPerPage"));
    RRETURNSETBOOL(TBag()->_fOneHeaderPerPage, fOneHeaderPerPage, DISPID_OneHeaderPerPage);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOpenArgs
//
//  Synopsis    Gets the TBag()->_cstrOpenArgs property
//
//  Arguments   pbstrOpenArgs        pointer where the TBag()->_cstrOpenArgs address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOpenArgs(BSTR* pbstrOpenArgs)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOpenArgs"));
    RRETURN(GetBstr(pbstrOpenArgs, TBag()->_cstrOpenArgs));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOpenArgs
//
//  Synopsis    Sets the TBag()->_cstrOpenArgs property
//
//  Arguments   bstrOpenArgs        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOpenArgs(LPTSTR  bstrOpenArgs)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOpenArgs"));
    RRETURN(SetBstr(bstrOpenArgs,TBag()->_cstrOpenArgs,DISPID_OpenArgs));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOutline
//
//  Synopsis    Gets the _iOutline property
//
//  Arguments   piOutline        pointer where the _iOutline address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOutline(int * piOutline)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOutline"));
    RRETURN(GetInt(piOutline, TBag()->_iOutline));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOutline
//
//  Synopsis    Sets the _iOutline property
//
//  Arguments   iOutline        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOutline(int  iOutline)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOutline"));
    RRETURN(SetInt(&(TBag()->_iOutline), iOutline, DISPID_Outline));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOutlineCollapseIcon
//
//  Synopsis    Gets the _hOutlineCollapseIcon property
//
//  Arguments   phOutlineCollapseIcon        pointer where the
//                                           TBag()->_hOutlineCollapseIcon address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOutlineCollapseIcon(IDispatch ** pOutlineCollapseIcon)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOutlineCollapseIcon"));
    RRETURN(GetIcon(pOutlineCollapseIcon, (IDispatch *)TBag()->_pOutlineCollapsePicture));
}



//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOutlineCollapseIcon
//
//  Synopsis    Sets the _hOutlineCollapseIcon property
//
//  Arguments   hOutlineCollapseIcon        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOutlineCollapseIcon(IDispatch*  OutlineCollapseIcon)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOutlineCollapseIcon"));
    RRETURN(SetIcon((IDispatch**)&(TBag()->_pOutlineCollapsePicture), OutlineCollapseIcon, DISPID_OutlineCollapseIcon));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOutlineExpandIcon
//
//  Synopsis    Gets the _hOutlineExpandIcon property
//
//  Arguments   phOutlineExpandIcon      pointer where the
//                                       TBag()->_hOutlineExpandIcon address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOutlineExpandIcon(IDispatch** pOutlineExpandIcon)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOutlineExpandIcon"));
    RRETURN(GetIcon(pOutlineExpandIcon, (IDispatch *)TBag()->_pOutlineExpandPicture));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOutlineExpandIcon
//
//  Synopsis    Sets the _hOutlineExpandIcon property
//
//  Arguments   hOutlineExpandIcon      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOutlineExpandIcon(IDispatch*  OutlineExpandIcon)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOutlineExpandIcon"));
    RRETURN(SetIcon((IDispatch**)&(TBag()->_pOutlineExpandPicture), OutlineExpandIcon, DISPID_OutlineExpandIcon));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOutlineLeafIcon
//
//  Synopsis    Gets the _hOutlineLeafIcon property
//
//  Arguments   phOutlineLeafIcon        pointer where the
//                                       TBag()->_hOutlineLeafIcon address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOutlineLeafIcon(IDispatch** pOutlineLeafIcon)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOutlineLeafIcon"));
    RRETURN(GetIcon(pOutlineLeafIcon, (IDispatch *)TBag()->_pOutlineLeafPicture));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOutlineLeafIcon
//
//  Synopsis    Sets the _hOutlineLeafIcon property
//
//  Arguments   hOutlineLeafIcon        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOutlineLeafIcon(IDispatch*  OutlineLeafIcon)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOutlineLeafIcon"));
    RRETURN(SetIcon((IDispatch**)&(TBag()->_pOutlineLeafPicture), OutlineLeafIcon, DISPID_OutlineLeafIcon));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOutlinePrint
//
//  Synopsis    Gets the _iOutlinePrint property
//
//  Arguments   piOutlinePrint       pointer where the _iOutlinePrint address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOutlinePrint(int * piOutlinePrint)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOutlinePrint"));
    RRETURN(GetInt(piOutlinePrint, TBag()->_iOutlinePrint));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOutlinePrint
//
//  Synopsis    Sets the _iOutlinePrint property
//
//  Arguments   iOutlinePrint       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOutlinePrint(int  iOutlinePrint)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOutlinePrint"));
    RRETURN(SetInt(&(TBag()->_iOutlinePrint), iOutlinePrint, DISPID_OutlinePrint));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetOutlineShowWhen
//
//  Synopsis    Gets the _iOutlineShowWhen property
//
//  Arguments   piOutlineShowWhen        pointer where the _iOutlineShowWhen
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetOutlineShowWhen(int * piOutlineShowWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::GetOutlineShowWhen"));
    RRETURN(GetInt(piOutlineShowWhen, TBag()->_iOutlineShowWhen));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetOutlineShowWhen
//
//  Synopsis    Sets the _iOutlineShowWhen property
//
//  Arguments   iOutlineShowWhen        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetOutlineShowWhen(int  iOutlineShowWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::SetOutlineShowWhen"));
    RRETURN(SetInt(&(TBag()->_iOutlineShowWhen), iOutlineShowWhen, DISPID_OutlineShowWhen));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetParentPosition
//
//  Synopsis    Gets the _iParentPosition property
//
//  Arguments   piParentPosition         pointer where the _iParentPosition address
//                                       goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetParentPosition(int * piParentPosition)
{
    TraceTag((tagDataFrame,"CDataFrame::GetParentPosition"));
    RRETURN(GetInt(piParentPosition, TBag()->_iParentPosition));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetParentPosition
//
//  Synopsis    Sets the _iParentPosition property
//
//  Arguments   iParentPosition     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetParentPosition(int  iParentPosition)
{
    TraceTag((tagDataFrame,"CDataFrame::SetParentPosition"));
    RRETURN(SetInt(&(TBag()->_iParentPosition), iParentPosition, DISPID_ParentPosition));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetPrintCount
//
//  Synopsis    Gets the _lPrintCount property
//
//  Arguments   plPrintCount         pointer where the _lPrintCount address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetPrintCount(long* plPrintCount)
{
    TraceTag((tagDataFrame,"CDataFrame::GetPrintCount"));
    RRETURN(GetLong(plPrintCount, TBag()->_lPrintCount));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetPrintCount
//
//  Synopsis    Sets the _lPrintCount property
//
//  Arguments   lPrintCount     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetPrintCount(long  lPrintCount)
{
    TraceTag((tagDataFrame,"CDataFrame::SetPrintCount"));
    RRETURN(SetLong(&(TBag()->_lPrintCount), lPrintCount, DISPID_PrintCount));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetPrintSection
//
//  Synopsis    Gets the _fPrintSection property
//
//  Arguments   pfPrintSection       pointer where the _fPrintSection address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetPrintSection(VARIANT_BOOL* pfPrintSection)
{
    TraceTag((tagDataFrame,"CDataFrame::GetPrintSection"));
    RRETURN(GetBool(pfPrintSection, TBag()->_fPrintSection));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetPrintSection
//
//  Synopsis    Sets the _fPrintSection property
//
//  Arguments   fPrintSection       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetPrintSection(VARIANT_BOOL  fPrintSection)
{
    TraceTag((tagDataFrame,"CDataFrame::SetPrintSection"));
    RRETURNSETBOOL(TBag()->_fPrintSection, fPrintSection, DISPID_PrintSection);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetQBFAutoExecute
//
//  Synopsis    Gets the _fQBFAutoExecute property
//
//  Arguments   pfQBFAutoExecute         pointer where the _fQBFAutoExecute
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetQBFAutoExecute(VARIANT_BOOL* pfQBFAutoExecute)
{
    TraceTag((tagDataFrame,"CDataFrame::GetQBFAutoExecute"));
    RRETURN(GetBool(pfQBFAutoExecute, TBag()->_fQBFAutoExecute));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetQBFAutoExecute
//
//  Synopsis    Sets the _fQBFAutoExecute property
//
//  Arguments   fQBFAutoExecute     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetQBFAutoExecute(VARIANT_BOOL  fQBFAutoExecute)
{
    TraceTag((tagDataFrame,"CDataFrame::SetQBFAutoExecute"));
    RRETURNSETBOOL(TBag()->_fQBFAutoExecute, fQBFAutoExecute, DISPID_QBFAutoExecute);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetQBFAvailable
//
//  Synopsis    Gets the _fQBFAvailable property
//
//  Arguments   pfQBFAvailable       pointer where the _fQBFAvailable address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetQBFAvailable(VARIANT_BOOL* pfQBFAvailable)
{
    TraceTag((tagDataFrame,"CDataFrame::GetQBFAvailable"));
    RRETURN(GetBool(pfQBFAvailable, TBag()->_fQBFAvailable));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetQBFAvailable
//
//  Synopsis    Sets the _fQBFAvailable property
//
//  Arguments   fQBFAvailable       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetQBFAvailable(VARIANT_BOOL  fQBFAvailable)
{
    TraceTag((tagDataFrame,"CDataFrame::SetQBFAvailable"));
    RRETURNSETBOOL(TBag()->_fQBFAvailable, fQBFAvailable, DISPID_QBFAvailable);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetQBFMode
//
//  Synopsis    Gets the _fQBFMode property
//
//  Arguments   pfQBFMode        pointer where the _fQBFMode address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetQBFMode(VARIANT_BOOL* pfQBFMode)
{
    TraceTag((tagDataFrame,"CDataFrame::GetQBFMode"));
    RRETURN(GetBool(pfQBFMode, TBag()->_fQBFMode));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetQBFMode
//
//  Synopsis    Sets the _fQBFMode property
//
//  Arguments   fQBFMode        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetQBFMode(VARIANT_BOOL  fQBFMode)
{
    TraceTag((tagDataFrame,"CDataFrame::SetQBFMode"));
    RRETURNSETBOOL(TBag()->_fQBFMode, fQBFMode, DISPID_QBFMode);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetQBFShowData
//
//  Synopsis    Gets the _fQBFShowData property
//
//  Arguments   pfQBFShowData        pointer where the _fQBFShowData address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetQBFShowData(VARIANT_BOOL* pfQBFShowData)
{
    TraceTag((tagDataFrame,"CDataFrame::GetQBFShowData"));
    RRETURN(GetBool(pfQBFShowData, TBag()->_fQBFShowData));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetQBFShowData
//
//  Synopsis    Sets the _fQBFShowData property
//
//  Arguments   fQBFShowData        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetQBFShowData(VARIANT_BOOL  fQBFShowData)
{
    TraceTag((tagDataFrame,"CDataFrame::SetQBFShowData"));
    RRETURNSETBOOL(TBag()->_fQBFShowData, fQBFShowData, DISPID_QBFShowData);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetReadOnlyDataColor
//
//  Synopsis    Gets the _colorReadOnlyDataColor property
//
//  Arguments   pcolorReadOnlyDataColor      pointer where the _colorReadOnlyDataColor
//                                           address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetReadOnlyDataColor(OLE_COLOR* pcolorReadOnlyDataColor)
{
    TraceTag((tagDataFrame,"CDataFrame::GetReadOnlyDataColor"));
    RRETURN(GetColor(pcolorReadOnlyDataColor, TBag()->_colorReadOnlyDataColor));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetReadOnlyDataColor
//
//  Synopsis    Sets the _colorReadOnlyDataColor property
//
//  Arguments   colorReadOnlyDataColor      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetReadOnlyDataColor(OLE_COLOR  colorReadOnlyDataColor)
{
    TraceTag((tagDataFrame,"CDataFrame::SetReadOnlyDataColor"));
    RRETURN(SetColor(&(TBag()->_colorReadOnlyDataColor), colorReadOnlyDataColor, DISPID_ReadOnlyDataColor));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordCount
//
//  Synopsis    Gets the _lRecordCount property
//
//  Arguments   plRecordCount        pointer where the _lRecordCount address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordCount(long* plRecordCount)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordCount"));
    RRETURN(GetLong(plRecordCount, TBag()->_lRecordCount));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetRecordCount
//
//  Synopsis    Sets the _lRecordCount property
//
//  Arguments   lRecordCount        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRecordCount(long  lRecordCount)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRecordCount"));
    RRETURN(SetLong(&(TBag()->_lRecordCount), lRecordCount, DISPID_RecordCount));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordLocks
//
//  Synopsis    Gets the _iRecordLocks property
//
//  Arguments   piRecordLocks        pointer where the _iRecordLocks address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordLocks(int * piRecordLocks)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordLocks"));
    RRETURN(GetInt(piRecordLocks, TBag()->_iRecordLocks));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetRecordLocks
//
//  Synopsis    Sets the _iRecordLocks property
//
//  Arguments   iRecordLocks        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRecordLocks(int  iRecordLocks)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRecordLocks"));
    RRETURN(SetInt(&(TBag()->_iRecordLocks), iRecordLocks, DISPID_RecordLocks));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordPosition
//
//  Synopsis    Gets the _iRecordPosition property
//
//  Arguments   piRecordPosition         pointer where the _iRecordPosition
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordPosition(int * piRecordPosition)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordPosition"));
    RRETURN(GetInt(piRecordPosition, TBag()->_iRecordPosition));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetRecordPosition
//
//  Synopsis    Sets the _iRecordPosition property
//
//  Arguments   iRecordPosition     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRecordPosition(int  iRecordPosition)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRecordPosition"));
    RRETURN(SetInt(&(TBag()->_iRecordPosition), iRecordPosition, DISPID_RecordPosition));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordSelectors
//
//  Synopsis    Gets the _fRecordSelectors property
//
//  Arguments   pfRecordSelectors        pointer where the _iRecordSelectors
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordSelectors(VARIANT_BOOL * pfRecordSelectors)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordSelectors"));
    RRETURN(GetBool(pfRecordSelectors, TBag()->_fRecordSelectors));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetRecordSelectors
//
//  Synopsis    Sets the _fRecordSelectors property
//
//  Arguments   fRecordSelectors        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRecordSelectors(VARIANT_BOOL  fRecordSelectors)
{
    HRESULT hr = S_OK;
    CTBag * pTBag = TBag();
    BOOL f;
    COleDataSite * pRS;
    CBaseFrame * pfrTemplate;

    if ( ENSURE_BOOL(fRecordSelectors) == ENSURE_BOOL(pTBag->_fRecordSelectors) )
        goto Cleanup;


    f = _pDoc->_fDeferedPropertyUpdate;
    pRS = NULL;
    pfrTemplate = _pDetail->getTemplate();


    TraceTag((tagDataFrame,"CDataFrame::SetRecordSelectors"));

    //  Show/hide the internal record selector site
    if ( fRecordSelectors && ! pTBag->_fRecordSelectors )
    {
        CRectl rcl(pfrTemplate->_rcl);
        //  Show RecordSelector site

        //  BUGBUG: Deferring is temporarily disabled, as it suppresses the
        //  expected matrix resize in response to pSite->SetWidth().

        //SetDeferedPropertyUpdate(TRUE);

        RetireRecordSelector();
        //  BUGBUG: Fire events and stuff

        //  Create a new OleDataSiteRS
        pRS = (COleDataSite *)pfrTemplate->CreateSite(ETAG_OLEDATASITE);
        if ( ! pRS )
        {
            hr = E_FAIL;
            goto Error;
        }

        //  Set the FakeControl OlesiteFlag
        pRS->SetFlag((OLEDATASITE_FLAG)(OLEDATASITE_FLAG_FAKECONTROL | OLEDATASITE_FLAG_RECORDSELECTOR));

        //  position it to the left edge
        //  Set the site's height to detail template height (done in rcl constructor)
        //  Set the width to zero
        rcl.right = rcl.left + 1;

        //  insert it into the detail template
        hr = pRS->InsertedAt(pfrTemplate,CLSID_CFakeControl,NULL,&rcl,0 /*non-centered*/);
        if ( hr )
            goto Error;

        //  set its width to some width. This should trigger the matrix resize logic
        //      and push the control templated to the right.


        pRS->SetWidth(HimetricFromHPix(17));


        //  BUGBUG: Deferring is temporarily disabled, as it suppresses the
        //  expected matrix resize in response to pSite->SetWidth().

        //SetDeferedPropertyUpdate(f);

    }
    else if ( ! fRecordSelectors && pTBag->_fRecordSelectors )
    {
        //  Hide RecordSelector site
        CSite ** ppSite;
        int c;

        //SetDeferedPropertyUpdate(TRUE);

        for ( ppSite = pfrTemplate->_arySites, c = pfrTemplate->_arySites.Size(); c > 0; c--, ppSite++ )
        {
            if ( (*ppSite)->TestClassFlag(SITEDESC_OLEDATASITE) &&
                 ((COleDataSite*)(*ppSite))->TestFlag(OLEDATASITE_FLAG_FAKECONTROL) )
            {
                pRS = (COleDataSite*)(*ppSite);

                pRS->SetWidth(1);
                pRS->AddRef();  //  Keep the template alive
                hr = THR(pfrTemplate->DeleteSite(pRS,0));
                pRS->Release();
            }
        }

        //SetDeferedPropertyUpdate(f);
    }

    pTBag->_fRecordSelectors = fRecordSelectors;

Cleanup:
    RRETURN(hr);

Error:
    goto Cleanup;
}


//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordset
//
//  Synopsis    Gets the _dispRecordset property
//
//  Arguments   pdispRecordset       pointer where the _dispRecordset address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordset(IDispatch** ppdispRecordset)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordset"));
    if (ppdispRecordset  == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordsetClone
//
//  Synopsis    Gets the _dispRecordsetClone property
//
//  Arguments   pdispRecordsetClone      pointer where the _dispRecordsetClone
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordsetClone(IDispatch** ppdispRecordsetClone)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordsetClone"));
    if (ppdispRecordsetClone  == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    return E_NOTIMPL;
}



//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordSourceSample
//
//  Synopsis    Gets the _fRecordSourceSample property
//
//  Arguments   pfRecordSourceSample         pointer where the
//                                          TBag()->_fRecordSourceSample address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordSourceSample(VARIANT_BOOL* pfRecordSourceSample)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordSourceSample"));
    RRETURN(GetBool(pfRecordSourceSample, TBag()->_fRecordSourceSample));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetRecordSourceSample
//
//  Synopsis    Sets the _fRecordSourceSample property
//
//  Arguments   fRecordSourceSample     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRecordSourceSample(VARIANT_BOOL  fRecordSourceSample)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRecordSourceSample"));
    RRETURNSETBOOL(TBag()->_fRecordSourceSample, fRecordSourceSample, DISPID_RecordSourceSample);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordSourceSync
//
//  Synopsis    Gets the _iRecordSourceSync property
//
//  Arguments   piRecordSourceSync       pointer where the
//                                      TBag()->_iRecordSourceSync address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordSourceSync(int * piRecordSourceSync)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordSourceSync"));
    RRETURN(GetInt(piRecordSourceSync, TBag()->_iRecordSourceSync));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetRecordSourceSync
//
//  Synopsis    Sets the _iRecordSourceSync property
//
//  Arguments   iRecordSourceSync       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRecordSourceSync(int  iRecordSourceSync)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRecordSourceSync"));
    RRETURN(SetInt(&(TBag()->_iRecordSourceSync), iRecordSourceSync, DISPID_RecordSourceSync));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRecordSourceType
//
//  Synopsis    Gets the _iRecordSourceType property
//
//  Arguments   piRecordSourceType       pointer where the
//                                       TBag()->_iRecordSourceType address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordSourceType(int * piRecordSourceType)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRecordSourceType"));
    RRETURN(GetInt(piRecordSourceType, TBag()->_iRecordSourceType));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetRecordSourceType
//
//  Synopsis    Sets the _iRecordSourceType property
//
//  Arguments   iRecordSourceType       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRecordSourceType(int  iRecordSourceType)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRecordSourceType"));
    RRETURN(SetInt(&(TBag()->_iRecordSourceType), iRecordSourceType, DISPID_RecordSourceType));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetDirection
//
//  Synopsis    Gets the _fDirection property
//
//  Arguments   pfRepeat         pointer where the _fRepeated address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDirection(fmRepeatDirection* pfDirection)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDirection"));
    if  (pfDirection  == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    *pfDirection = (fmRepeatDirection)TBag()->_fDirection;
    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetDirection
//
//  Synopsis    Sets the _fDirection property
//
//  Arguments   fRepeat     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDirection(fmRepeatDirection  fDirection)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDirection"));

    CTBag *pTBag = TBag();
    if (ENSURE_BOOL(pTBag->_fDirection) != fDirection)
    {
        unsigned int u;

        pTBag->_fDirection = fDirection;
        // swap itemsacross and padding accordingly
        u = pTBag->_uItems [0];
        pTBag->_uItems [0] = pTBag->_uItems [1];
        pTBag->_uItems [1] = u;
        u = pTBag->_uPadding [0];
        pTBag->_uPadding [1] = pTBag->_uPadding [0];
        pTBag->_uPadding [1] = u;
        RRETURN(OnDataChangeCloseError(DISPID_Repeat, TRUE));
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetRequeryWhen
//
//  Synopsis    Gets the _iRequeryWhen property
//
//  Arguments   piRequeryWhen        pointer where the _iRequeryWhen address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRequeryWhen(int * piRequeryWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::GetRequeryWhen"));
    RRETURN(GetInt(piRequeryWhen, TBag()->_iRequeryWhen));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetRequeryWhen
//
//  Synopsis    Sets the _iRequeryWhen property
//
//  Arguments   iRequeryWhen        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRequeryWhen(int  iRequeryWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::SetRequeryWhen"));
    RRETURN(SetInt(&(TBag()->_iRequeryWhen), iRequeryWhen, DISPID_RequeryWhen));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetResetPages
//
//  Synopsis    Gets the _fResetPages property
//
//  Arguments   pfResetPages         pointer where the _fResetPages address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetResetPages(VARIANT_BOOL* pfResetPages)
{
    TraceTag((tagDataFrame,"CDataFrame::GetResetPages"));
    RRETURN(GetBool(pfResetPages, TBag()->_fResetPages));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetResetPages
//
//  Synopsis    Sets the _fResetPages property
//
//  Arguments   fResetPages     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetResetPages(VARIANT_BOOL  fResetPages)
{
    TraceTag((tagDataFrame,"CDataFrame::SetResetPages"));
    RRETURNSETBOOL(TBag()->_fResetPages, fResetPages, DISPID_ResetPages);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetScrollHeight
//
//  Synopsis    Gets the _lScrollHeight property
//
//  Arguments   plScrollHeight       pointer where the _lScrollHeight address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetScrollHeight(long* plScrollHeight)
{
    TraceTag((tagDataFrame,"CDataFrame::GetScrollHeight"));
    RRETURN(GetLong(plScrollHeight, _pDetail->_rcl.Height()));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetScrollLeft
//
//  Synopsis    Gets the ScrollLeft property
//
//  Note:       _lScrollLeft value in the TBag is used only for setting the new
//              Scroll left position.
//
//  Arguments   plScrollLeft         pointer where the _lScrollLeft address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetScrollLeft(long* plScrollLeft)
{
    TraceTag((tagDataFrame,"CDataFrame::GetScrollLeft"));
    RRETURN(GetLong(plScrollLeft, GetScrollPos(0)));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetScrollLeft
//
//  Synopsis    Sets the _lScrollLeft property
//
//  Arguments   lScrollLeft     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetScrollLeft(long  lScrollLeft)
{
    TraceTag((tagDataFrame,"CDataFrame::SetScrollLeft"));
    RRETURN(SetLong(&(TBag()->_lScrollLeft), lScrollLeft, DISPID_ScrollLeft));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetScrollTop
//
//  Synopsis    Gets the _lScrollTop property
//
//  Arguments   plScrollTop      pointer where the _lScrollTop address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetScrollTop(long* plScrollTop)
{
    TraceTag((tagDataFrame,"CDataFrame::GetScrollTop"));
    RRETURN(GetLong(plScrollTop, GetScrollPos(1)));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetScrollTop
//
//  Synopsis    Sets the _lScrollTop property
//
//  Arguments   lScrollTop      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetScrollTop(long  lScrollTop)
{
    TraceTag((tagDataFrame,"CDataFrame::SetScrollTop"));
    RRETURN(SetLong(&(TBag()->_lScrollTop), lScrollTop, DISPID_ScrollTop));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetScrollWidth
//
//  Synopsis    Gets the _lScrollWidth property
//
//  Arguments   plScrollWidth        pointer where the _lScrollWidth address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetScrollWidth(long* plScrollWidth)
{
    TraceTag((tagDataFrame,"CDataFrame::GetScrollWidth"));
    RRETURN(GetLong(plScrollWidth, _pDetail->_rcl.Width()));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetSelectedControlBackCol
//
//  Synopsis    Gets the _colorSelectedControlBackCol property
//
//  Arguments   pcolorSelectedControlBackCol         pointer where the
//                                                   TBag()->_colorSelectedControlBackCol
//                                                   address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetSelectedControlBackCol(OLE_COLOR* pcolorSelectedControlBackCol)
{
    TraceTag((tagDataFrame,"CDataFrame::GetSelectedControlBackCol"));
    RRETURN(GetColor(pcolorSelectedControlBackCol, TBag()->_colorSelectedControlBackCol));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetSelectedControlBackCol
//
//  Synopsis    Sets the _colorSelectedControlBackCol property
//
//  Arguments   colorSelectedControlBackCol     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetSelectedControlBackCol(OLE_COLOR  colorSelectedControlBackCol)
{
    TraceTag((tagDataFrame,"CDataFrame::SetSelectedControlBackCol"));
    RRETURN(SetColor(&(TBag()->_colorSelectedControlBackCol), colorSelectedControlBackCol,
                    DISPID_SelectedControlBackCol));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetShowGridLines
//
//  Synopsis    Gets the _iShowGridLines property
//
//  Arguments   piShowGridLines      pointer where the _iShowGridLines address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetShowGridLines(int * piShowGridLines)
{
    TraceTag((tagDataFrame,"CDataFrame::GetShowGridLines"));
    RRETURN(GetInt(piShowGridLines, TBag()->_iShowGridLines));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetShowGridLines
//
//  Synopsis    Sets the _iShowGridLines property
//
//  Arguments   iShowGridLines      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetShowGridLines(int  iShowGridLines)
{
    TraceTag((tagDataFrame,"CDataFrame::SetShowGridLines"));
    RRETURN(SetInt(&(TBag()->_iShowGridLines), iShowGridLines, DISPID_ShowGridLines));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetShowNewRowAtBottom
//
//  Synopsis    Gets the _fShowNewRowAtBottom property
//
//  Arguments   pfShowNewRowAtBottom         pointer where the
//                                           TBag()->_fShowNewRowAtBottom address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetShowNewRowAtBottom(VARIANT_BOOL* pfShowNewRowAtBottom)
{
    TraceTag((tagDataFrame,"CDataFrame::GetShowNewRowAtBottom"));
    RRETURN(GetBool(pfShowNewRowAtBottom, TBag()->_fShowNewRowAtBottom));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetShowNewRowAtBottom
//
//  Synopsis    Sets the _fShowNewRowAtBottom property
//
//  Arguments   fShowNewRowAtBottom     new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetShowNewRowAtBottom(VARIANT_BOOL  fShowNewRowAtBottom)
{
    TraceTag((tagDataFrame,"CDataFrame::SetShowNewRowAtBottom"));
    RRETURNSETBOOL(TBag()->_fShowNewRowAtBottom, fShowNewRowAtBottom, DISPID_ShowNewRowAtBottom);
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetShowWhen
//
//  Synopsis    Gets the _iShowWhen property
//
//  Arguments   piShowWhen       pointer where the _iShowWhen address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetShowWhen(int * piShowWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::GetShowWhen"));
    RRETURN(GetInt(piShowWhen, TBag()->_iShowWhen));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetShowWhen
//
//  Synopsis    Sets the _iShowWhen property
//
//  Arguments   iShowWhen       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetShowWhen(int  iShowWhen)
{
    TraceTag((tagDataFrame,"CDataFrame::SetShowWhen"));
    RRETURN(SetInt(&(TBag()->_iShowWhen), iShowWhen, DISPID_ShowWhen));
}
//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetSpecialEffect
//
//  Synopsis    Gets the _iSpecialEffect property
//
//  Arguments   piSpecialEffect      pointer where the _iSpecialEffect address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetSpecialEffect(int * piSpecialEffect)
{
    TraceTag((tagDataFrame,"CDataFrame::GetSpecialEffect"));
    RRETURN(GetInt(piSpecialEffect, TBag()->_iSpecialEffect));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetSpecialEffect
//
//  Synopsis    Sets the _iSpecialEffect property
//
//  Arguments   iSpecialEffect      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetSpecialEffect(int  iSpecialEffect)
{
    TraceTag((tagDataFrame,"CDataFrame::SetSpecialEffect"));
    RRETURN(SetInt(&(TBag()->_iSpecialEffect), iSpecialEffect, DISPID_SpecialEffect));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetViewMode
//
//  Synopsis    Gets the _iViewMode property
//
//  Arguments   piViewMode       pointer where the _iViewMode address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetViewMode(int * piViewMode)
{
    TraceTag((tagDataFrame,"CDataFrame::GetViewMode"));
    RRETURN(GetInt(piViewMode, TBag()->_iViewMode));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetViewMode
//
//  Synopsis    Sets the _iViewMode property
//
//  Arguments   iViewMode       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetViewMode(int  iViewMode)
{
    TraceTag((tagDataFrame,"CDataFrame::SetViewMode"));
    RRETURN(SetInt(&(TBag()->_iViewMode), iViewMode, DISPID_ViewMode));
}




//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetDataDirty
//
//  Synopsis    Gets the _fDataDirty property
//
//  Arguments   pfDataDirty      pointer where the _fDataDirty address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetDataDirty(VARIANT_BOOL* pfDataDirty)
{
    TraceTag((tagDataFrame,"CDataFrame::GetDataDirty"));
    RRETURN(GetBool(pfDataDirty, _fDataDirty));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetDataDirty
//
//  Synopsis    Sets the _fDataDirty property
//
//  Arguments   fDataDirty      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetDataDirty(VARIANT_BOOL  fDataDirty)
{
    TraceTag((tagDataFrame,"CDataFrame::SetDataDirty"));
    RRETURNSETBOOL(_fDataDirty, fDataDirty, DISPID_DataDirty);
}


//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetLayoutCurrent
//
//  Synopsis    Gets the _fLayoutCurrent property
//
//  Arguments   pfLayoutCurrent      pointer where the _fLayoutCurrent address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetLayoutCurrent(VARIANT_BOOL* pfLayoutCurrent)
{
    TraceTag((tagDataFrame,"CDataFrame::GetLayoutCurrent"));
    if (pfLayoutCurrent == NULL)
        RRETURN(SetErrorInfoInvalidArg());

    return E_FAIL;
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetLayoutCurrent
//
//  Synopsis    Sets the _fLayoutCurrent property
//
//  Arguments   fLayoutCurrent      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetLayoutCurrent(VARIANT_BOOL  fLayoutCurrent)
{
    TraceTag((tagDataFrame,"CDataFrame::SetLayoutCurrent"));

    return E_FAIL;
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetNewData
//
//  Synopsis    Gets the _fNewData property
//
//  Arguments   pfNewData        pointer where the _fNewData address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetNewData(VARIANT_BOOL* pfNewData)
{
    TraceTag((tagDataFrame,"CDataFrame::GetNewData"));
    RRETURN(GetBool(pfNewData, _fNewData));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetNewData
//
//  Synopsis    Sets the _fNewData property
//
//  Arguments   fNewData        new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetNewData(VARIANT_BOOL  fNewData)
{
    TraceTag((tagDataFrame,"CDataFrame::SetNewData"));
    RRETURNSETBOOL(_fNewData, fNewData, DISPID_NewData);
}


//-----------------------------------------------------------------------------
//  Member      CDataFrame::GetRecordSelectorControl
//
//  Synopsis    Gets the _pctrlRecordSelector from template
//
//  Arguments   ppdispRecordSelector  pointer where the RecordSelector
//                                      address goes to
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetRecordSelector(IControl** ppRecordSelector)
{
    CTBag * pTBag = TBag();

    TraceTag((tagDataFrame,"CDataFrame::GetRecordSelector"));
    RRETURN(GetLong((long*) ppRecordSelector, (pTBag->_pctrlRecordSelector)?
                                              (long) pTBag->_pctrlRecordSelector->ControlI()
                                              : 0));
}

//-----------------------------------------------------------------------------
//  Member      CDataFrame::SetRecordSelector
//
//  Synopsis    Sets the _pctrlRecordSelector
//
//  Arguments   pdispRecordSelector  new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetRecordSelector(IControl* pRecordSelector)
{
    HRESULT hr = S_OK;

    TraceTag((tagDataFrame,"CDataFrame::SetRecordSelector"));

    if ( ! _fOwnTBag )
    {
        CDataFrame *   pTemplate;

        pTemplate = getTemplate();
        if (pTemplate)
        {
            hr =  THR((pTemplate->SetRecordSelector(pRecordSelector)));
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        //  We are the template

        CTBag * pTBag = TBag();
        BOOL fDeferedPropertyUpdate = _pDoc->_fDeferedPropertyUpdate;
        COleDataSite * pdsRecordSelector = (COleDataSite*)pRecordSelector;

        _pDoc->_fDeferedPropertyUpdate = TRUE;

        //  Retire the old record selector. This should allow for record selector
        //  removal too.

        RetireRecordSelector();

        pTBag->_pctrlRecordSelector = (COleDataSite*)pRecordSelector;
        pdsRecordSelector->AddRef();

        Assert(pdsRecordSelector->_pParent &&
               (pdsRecordSelector->_pParent == _pDetail));
        Assert(-1 != _pDetail->_arySites.Find(pdsRecordSelector));

        //  Mark the site with the recordselector flag
        pdsRecordSelector->SetFlag(OLEDATASITE_FLAG_RECORDSELECTOR);

        _pDoc->_fDeferedPropertyUpdate = fDeferedPropertyUpdate;
    }

    //  BUGBUG: We need to fire some kind of an event here

    RRETURN(hr);
}




//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetLinkMasterFields
//
//  Synopsis    Sets the TBag()->...LinkMasterFields property
//
//  Arguments   BSTR *    pbstrLinkMasterFields
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetLinkMasterFields(LPTSTR bstrLinkMasterFields)
{
    TraceTag((tagDataFrame,"CDataFrame::SetLinkMasterFields"));

    CDataLayerAccessorSource *pdlas;
    HRESULT hr=S_OK;

    if (bstrLinkMasterFields)
    {
        hr = THR(LazyGetDLAccessorSource(&pdlas));
        if (hr || pdlas->GetLinkMasterFields() &&
                  !_tcsicmp(pdlas->GetLinkMasterFields(), bstrLinkMasterFields) )
        {
            goto Cleanup;
        }
    }
    else if (HasDLAccessorSource())
    {
        IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
        Assert(pdlas);
    }
    else
    {
        goto Cleanup;
    }

    hr = THR(pdlas->SetLinkMasterFields(bstrLinkMasterFields));
    if (hr)
    {
        goto Cleanup;
    }
    hr = OnDataChange(DISPID_LinkMasterFields, FALSE);

Cleanup:
    RRETURN(CloseErrorInfo(hr));
}
//+---End of Method-------------------------------------------------------------



//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetLinkMasterFields
//
//  Synopsis    Gets the TBag()->...LinkMasterFields property
//
//  Arguments   BSTR *    pbstrLinkMasterFields
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetLinkMasterFields(BSTR * pbstrLinkMasterFields)
{
    TraceTag((tagDataFrame,"CDataFrame::GetLinkMasterFields"));

    HRESULT hr;

    if (pbstrLinkMasterFields == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (HasDLAccessorSource())
        {
            CDataLayerAccessorSource *pdlas;
            IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
            Assert(pdlas);
            hr = THR(pdlas->GetLinkMasterFields().AllocBSTR(pbstrLinkMasterFields));
        }
        else
        {
            *pbstrLinkMasterFields = NULL;
            hr = S_OK;
        }
    }
    RRETURN(hr);
}


//+---End of Method-------------------------------------------------------------






//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetLinkChildFields
//
//  Synopsis    Sets the TBag()->...LinkChildFields property
//
//  Arguments   BSTR *    pbstrLinkChildFields
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetLinkChildFields(LPTSTR bstrLinkChildFields)
{
    TraceTag((tagDataFrame,"CDataFrame::SetLinkChildFields"));

    CDataLayerAccessorSource *pdlas;
    HRESULT hr=S_OK;

    if (bstrLinkChildFields)
    {
        hr = THR(LazyGetDLAccessorSource(&pdlas));
        if (hr || pdlas->GetLinkChildFields() &&
                  !_tcsicmp(pdlas->GetLinkChildFields(), bstrLinkChildFields) )
        {
            goto Cleanup;
        }
    }
    else if (HasDLAccessorSource())
    {
        IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
        Assert(pdlas);
    }
    else
    {
        goto Cleanup;
    }

    hr = THR(pdlas->SetLinkChildFields(bstrLinkChildFields));
    if (hr)
    {
        goto Cleanup;
    }
    hr = OnDataChange(DISPID_LinkChildFields, FALSE);

Cleanup:
    RRETURN(CloseErrorInfo(hr));
}
//+---End of Method-------------------------------------------------------------



//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetLinkChildFields
//
//  Synopsis    Gets the TBag()->...LinkChildFields property
//
//  Arguments   BSTR *    pbstrLinkChildFields
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetLinkChildFields(BSTR* pbstrLinkChildFields)
{
    TraceTag((tagDataFrame,"CDataFrame::GetLinkChildFields"));

    HRESULT hr;

    if (pbstrLinkChildFields == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (HasDLAccessorSource())
        {
            CDataLayerAccessorSource *pdlas;
            IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
            Assert(pdlas);
            hr = THR(pdlas->GetLinkChildFields().AllocBSTR(pbstrLinkChildFields));
        }
        else
        {
            *pbstrLinkChildFields = NULL;
            hr = S_OK;
        }
    }
    RRETURN(hr);
}
//+---End of Method-------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::GetSnakingDirection
//
//  Synopsis    Gets the _iSnakingDirection property
//
//  Arguments   piSnakingDirection       pointer where the _iSnakingDirection
//                                       address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetSnakingDirection(int * piSnakingDirection)
{
    TraceTag((tagDataFrame,"CDataFrame::GetSnakingDirection"));
    RRETURN(GetInt(piSnakingDirection, TBag()->_iSnakingDirection));
}

//-----------------------------------------------------------------------------
//
//  Member      CDataFrame::SetSnakingDirection
//
//  Synopsis    Sets the _iSnakingDirection property
//
//  Arguments   iSnakingDirection       new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::SetSnakingDirection(int  iSnakingDirection)
{
    TraceTag((tagDataFrame,"CDataFrame::SetSnakingDirection"));
    RRETURN(SetInt(&(TBag()->_iSnakingDirection), iSnakingDirection, DISPID_SnakingDirection));
}



//+---------------------------------------------------------------------------
//
//  Member:     GetItemsAcross
//
//  Synopsis:   Get Items Across.
//
//  Arguments:  puItemsAcross           get number of columns
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::GetItemsAcross (int * puItemsAcross)
{
    TraceTag((tagDataFrame, "CDataFrame::GetItemsAcross"));
    RRETURN(GetInt(puItemsAcross, TBag()->_uItems[ACROSS]));
}












//+---------------------------------------------------------------------------
//
//  Member:     SetItemsAcross
//
//  Synopsis:   Set Items Across.
//
//  Arguments:  uItemsAcross            number of columns
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::SetItemsAcross (int iItemsAcross)
{
    TraceTag((tagDataFrame, "CDataFrame::SetItemsAcross"));
    RRETURN(SetInt((int*)&(TBag()->_uItems[ACROSS]), iItemsAcross, 0, 256, DISPID_ItemsAcross));
}








//+---------------------------------------------------------------------------
//
//  Member:     SetRepeat
//
//  Synopsis:   Set the layout Repeat Vertical/Horizontal
//
//  Arguments:  vh          Repeat flag.
//
//  Returns:    Returns always S_OK.
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::SetRepeat (VARIANT_BOOL fNewRepeat)
{
    TraceTag((tagDataFrame, "CDataFrame::SetRepeat"));

    RRETURNSETBOOL(TBag()->_fRepeated, fNewRepeat, DISPID_Repeat);
}





//+---------------------------------------------------------------------------
//
//  Member:     GetRepeat
//
//  Synopsis:   Get the layout Repeat Vertical/Horizontal.
//
//  Arguments:  pfRepeat             pointer to the result
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::GetRepeat (VARIANT_BOOL *pfRepeat)
{
    TraceTag((tagDataFrame, "CDataFrame::GetRepeat"));
    RRETURN(GetBool(pfRepeat, TBag()->_fRepeated));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetShowFooters
//
//  Synopsis:   Gets ShowFooters
//
//  Arguments:  pfShowFooters       ptr to location to return _fShowFooters
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDataFrame::GetShowFooters(VARIANT_BOOL * pfShowFooters)
{
    TraceTag((tagDataFrame,"CDataFrame::GetShowFooters"));
    RRETURN(GetBool(pfShowFooters,TBag()->_fShowFooters));
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetShowFooters
//
//  Synopsis:   Sets _fShowFooters property
//
//  Arguments:  fShowFooters
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetShowFooters(VARIANT_BOOL fShowFooters)
{
    HRESULT hr = S_OK;

    TraceTag((tagDataFrame,"CDataFrame::SetShowFooters"));
    if (ENSURE_BOOL(TBag()->_fShowFooters) != ENSURE_BOOL(fShowFooters))
    {
        TBag()->_fShowFooters = ENSURE_BOOL(fShowFooters);

        if (TBag()->_fShowFooters && !getTemplate()->_pFooter)
        {
            hr= getTemplate()->InitSubFrameTemplate(&getTemplate()->_pFooter, SITEDESC_HEADERFRAME, TRUE);
        }
        if (!hr)
        {
            hr = OnDataChange (DISPID_ShowFooters);
        }
    }
    RRETURN(CloseErrorInfo(hr));
}







//+---------------------------------------------------------------------------
//
//  Keyboard navigation property accessors
//
//----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetArrowEnterFieldBehavior
//
//  Synopsis:   Gets the ArrowEnterFieldBehavior property
//
//  Arguments:  piArrowEnterFieldBehavior   location to return the value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::GetArrowEnterFieldBehavior(int * piArrowEnterFieldBehavior)
{
    RRETURN(GetInt(piArrowEnterFieldBehavior, TBag()->_uArrowEnterFieldBehavior));
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetArrowEnterFieldBehavior
//
//  Synopsis:   Sets the ArrowEnterFieldBehavior property
//
//  Arguments:  iArrowEnterFieldBehavior:   the new value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::SetArrowEnterFieldBehavior(int iArrowEnterFieldBehavior)
{
    RRETURNSETBOOLFROMINT(TBag()->_uArrowEnterFieldBehavior, iArrowEnterFieldBehavior, 3, DISPID_ArrowEnterFieldBehavior);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetArrowKeyBehavior
//
//  Synopsis:   Gets the ArrowKeyBehavior property
//
//  Arguments:  piArrowKeyBehavior   location to return the value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::GetArrowKeyBehavior(int * piArrowKeyBehavior)
{
    RRETURN(GetInt(piArrowKeyBehavior, TBag()->_fArrowKeyBehavior));
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetArrowKeyBehavior
//
//  Synopsis:   Sets the ArrowKeyBehavior property
//
//  Arguments:  iArrowKeyBehavior:   the new value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::SetArrowKeyBehavior(int iArrowKeyBehavior)
{
    RRETURNSETBOOLFROMINT(TBag()->_fArrowKeyBehavior, iArrowKeyBehavior,  1, DISPID_ArrowKeyBehavior);

}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetArrowKeys
//
//  Synopsis:   Gets the ArrowKeys property
//
//  Arguments:  piArrowKeys   location to return the value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::GetArrowKeys(int * piArrowKeys)
{
    CTBag * pTBag = TBag();
    RRETURN(GetInt(piArrowKeys, (pTBag->_fForceArrowsToTabs << 1) | pTBag->_fArrowStaysInFrame));
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetArrowKeys
//
//  Synopsis:   Sets the ArrowKeys property
//
//  Arguments:  iArrowKeys:   the new value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::SetArrowKeys(int iArrowKeys)
{
    if ( (iArrowKeys >= 0) && (iArrowKeys <= 2) )
    {
        iArrowKeys &= 0x3;
        TBag()->_fArrowStaysInFrame = iArrowKeys & 0x1;
        TBag()->_fForceArrowsToTabs = (iArrowKeys & 0x2) >> 1;
        RRETURN(OnDataChangeCloseError(DISPID_ArrowKeys, FALSE));
    }
    else
        RRETURN(SetErrorInfoBadValue(DISPID_ArrowKeys,
                                     IDS_ES_ENTER_VALUE_IN_RANGE,
                                     0, 2));
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetDynamicTabOrder
//
//  Synopsis:   Gets the DynamicTabOrder property
//
//  Arguments:  pfDynamicTabOrder   location to return the value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::GetDynamicTabOrder(VARIANT_BOOL * pfDynamicTabOrder)
{
    RRETURN(GetBool(pfDynamicTabOrder, TBag()->_fDynamicTabOrder));
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetDynamicTabOrder
//
//  Synopsis:   Sets the DynamicTabOrder property
//
//  Arguments:  fDynamicTabOrder:   the new value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::SetDynamicTabOrder(VARIANT_BOOL fDynamicTabOrder)
{
    RRETURNSETBOOL(TBag()->_fDynamicTabOrder, fDynamicTabOrder, DISPID_DynamicTabOrder);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetMoveAfterEnter
//
//  Synopsis:   Gets the MoveAfterEnter property
//
//  Arguments:  piMoveAfterEnter   location to return the value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::GetMoveAfterEnter(int * piMoveAfterEnter)
{
    RRETURN(GetInt(piMoveAfterEnter, TBag()->_uMoveAfterEnter));
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetMoveAfterEnter
//
//  Synopsis:   Sets the MoveAfterEnter property
//
//  Arguments:  iMoveAfterEnter:   the new value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::SetMoveAfterEnter(int iMoveAfterEnter)
{
    RRETURNSETBOOLFROMINT(TBag()->_uMoveAfterEnter, iMoveAfterEnter, 2, DISPID_MoveAfterEnter);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetTabEnterFieldBehavior
//
//  Synopsis:   Gets the TabEnterFieldBehavior property
//
//  Arguments:  piTabEnterFieldBehavior   location to return the value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::GetTabEnterFieldBehavior(int * piTabEnterFieldBehavior)
{
    RRETURN(GetInt(piTabEnterFieldBehavior, TBag()->_uTabEnterFieldBehavior));
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetTabEnterFieldBehavior
//
//  Synopsis:   Sets the TabEnterFieldBehavior property
//
//  Arguments:  iTabEnterFieldBehavior:   the new value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::SetTabEnterFieldBehavior(int iTabEnterFieldBehavior)
{
    RRETURNSETBOOLFROMINT(TBag()->_uTabEnterFieldBehavior, iTabEnterFieldBehavior, 3, DISPID_TabEnterFieldBehavior);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetTabOut
//
//  Synopsis:   Gets the TabOut property
//
//  Arguments:  piTabOut   location to return the value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::GetTabOut(int * piTabOut)
{
    RRETURN(GetInt(piTabOut, TBag()->_uTabOut));
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetTabOut
//
//  Synopsis:   Sets the TabOut property
//
//  Arguments:  iTabOut:   the new value
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::SetTabOut(int iTabOut)
{
    RRETURNSETBOOLFROMINT(TBag()->_uTabOut, iTabOut, 3, DISPID_TabOut);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetMinCols
//
//  Synopsis:
//
//  Arguments:  plMinCols
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetMinCols (long * plMinCols)
{
    RRETURN(GetLong(plMinCols, TBag()->_lMinCols));
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetMinCols
//
//  Synopsis:
//
//  Arguments:  lMinCols
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetMinCols (long lMinCols)
{
    CTBag * pTBag = TBag();
    long    lMaxCols;

    if (lMinCols != pTBag->_lMinCols)
    {
        if (lMinCols < 0)
        {
            RRETURN(SetErrorInfoBadValue(DISPID_MinCols,
                                     IDS_ES_ENTER_VALUE_GT_ZERO));
        }

        lMaxCols = pTBag->_lMaxCols;
        if (lMaxCols >= 0 && lMinCols > lMaxCols)
        {
                lMinCols = lMaxCols;
        }
        pTBag->_lMinCols = lMinCols;
        RRETURN(OnDataChangeCloseError(DISPID_MinCols, TRUE));
    }
    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetMaxCols
//
//  Synopsis:
//
//  Arguments:  plMaxCols
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetMaxCols (long * plMaxCols)
{
    RRETURN(GetLong(plMaxCols, TBag()->_lMaxCols));
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetMaxCols
//
//  Synopsis:
//
//  Arguments:  lMaxCols
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetMaxCols (long lMaxCols)
{
    CTBag * pTBag = TBag();
    long    lMinCols;

    if (lMaxCols != pTBag->_lMaxCols)
    {
        if (lMaxCols < FITPARENT)
        {
            RRETURN(SetErrorInfoBadValue(DISPID_MaxCols,
                                     IDS_ES_ENTER_VALUE_IN_RANGE,
                                     FITPARENT, 100000));
        }

        if (lMaxCols >= 0)
        {
            lMinCols = pTBag->_lMinCols;
            if (lMaxCols < lMinCols)
            {
                lMaxCols = lMinCols;
            }

        }
        pTBag->_lMaxCols = lMaxCols;
        RRETURN(OnDataChangeCloseError(DISPID_MaxCols, TRUE));
    }

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetNewRecordShow
//
//  Synopsis:
//
//  Arguments:  pfNewRecordShow
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::GetNewRecordShow (VARIANT_BOOL *pfNewRecordShow)
{
    RRETURN(GetBool(pfNewRecordShow, TBag()->_fNewRecordShow));
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetNewRecordShow
//
//  Synopsis:
//
//  Arguments:  fNewRecordShow
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::SetNewRecordShow (VARIANT_BOOL fNewRecordShow)
{
    RRETURNSETBOOL(TBag()->_fNewRecordShow, fNewRecordShow, DISPID_NewRecordShow);
}




#endif




//+---------------------------------------------------------------------------
//
//  Member:     GetTopIndex
//
//  Synopsis:   Implements the TopIndex property for the listbox
//              Returns the index (record number of the top row).
//              returnvalue modified by -1 due to object model
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::GetTopIndex (long * plTopIndex)
{
    HRESULT             hr = S_OK;
    CTBag           *   pTBag = TBag();
    CDetailFrame    *   plfr=0;
    CRepeaterFrame  *   pRepeater;

    if ( ! plTopIndex )
        RRETURN(SetErrorInfoInvalidArg());

    if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {
        pRepeater = (CRepeaterFrame*)_pDetail;
        if (pRepeater->_arySites.Size())
        {
            plfr = (CDetailFrame *)(pRepeater->getLayoutFrame(pRepeater->getTopFrameIdx()));
        }
    }
    else
    {
        plfr = (CDetailFrame *)_pDetail;
    }

    if (plfr)
    {
        hr = GetRecordNumber (plfr->getHRow(), (ULONG *)plTopIndex);
        if (!hr)
        {
            *plTopIndex -= 1;
        }
    }
    else
    {
        *plTopIndex = -1L;
    }

    RRETURN(CloseErrorInfo(hr));
}




//+---------------------------------------------------------------------------
//
//  Member:     SetTopIndex
//
//  Synopsis:   Implements the TopIndex property for the listbox
//              Change event is fired from OnScroll
//              value modified by +1 due to object model
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::SetTopIndex (long lTopIndex)
{
    HRESULT hr;
    long    lTopPosition;

    // objectmodel adjustment
    lTopIndex += 1;

    if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {
        if (_pDetail->_arySites.Size())
        {
            CalcRowPosition (lTopIndex, &lTopPosition, 0);
            hr = OnScroll (((CRepeaterFrame*)_pDetail)->getDirection(), SB_THUMBPOSITION, lTopPosition);
        }
        else
        {
            hr = E_FAIL;
        }

    }
    else
    {
        hr = SetListIndex (lTopIndex);
    }
    RRETURN(CloseErrorInfo(hr));
}




//+---------------------------------------------------------------------------
//
//  Member:     GetVisibleCount
//
//  Synopsis:   Implements the VisibleCount property for the listbox
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::GetVisibleCount (long * plVisibleCount)
{
    HRESULT hr = S_OK;

    if ( ! plVisibleCount )
        RRETURN(SetErrorInfoInvalidArg());

    if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {
        // BUGBUG: What about snaking? ask Eric R.
        *plVisibleCount = ((CRepeaterFrame*)_pDetail)->_uRepeatInPage;
    }
    else
    {
        *plVisibleCount = 1;
    }

    RRETURN(hr);
}




//
//
//  end of file
//
//----------------------------------------------------------------------------


