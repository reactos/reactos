//----------------------------------------------------------------------------
//
//  Maintained by: frankman
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       src\doc\datadoc\dflist.cxx
//
//  Contents:   Implementation of the data frame Listbox relevant methods.
//
//  Classes:    CDataFrame
//
//  Functions:  None.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"

#include "rootfrm.hxx"


// DeclareTag(tagDataFrame,"src\\ddoc\\datadoc\\datafrm.cxx","DataFrame");
// DeclareTag(tagDataFrameTemplate,"src\\ddoc\\datadoc\\datafrm.cxx","DataFrameTemplate");
// extern TAG tagPropose;
extern TAG tagDataFrame;
extern TAG tagDLUseSTD;


// BUGBUG: the later is from MFC, is this correct ?
#define HIMETRIC_INCH   2540    // HIMETRIC units per inch


#if DBG==1 && !defined(PRODUCT_97)
DEFINE_GLOBAL(long, g_lRowWidth, 0);
DEFINE_GLOBAL(long, g_lRowHeight, 0);
#endif

//+------------------------------------------------------------------------
//
//  Member:     CreateListbox
//
//  Arguments:
//
//
//
//  Synopsis:   creates a new dataframe, preset to work as a listbox
//
//-------------------------------------------------------------------------
HRESULT CDataFrame::CreateListbox(IUnknown     *pRowSet,
                                 OLE_COLOR      olecolorBack,
                                 OLE_COLOR      olecolorFore,
                                 IDispatch      *pFontObject,
                                 fmListBoxStyles listStyle,
                                 fmScrollBars   ScrollBarStyle,
                                 int            iColumnCount,
                                 int            iListRows,
                                 VARIANT_BOOL   fIntegralHeight,
                                 VARIANT_BOOL   fColumnHeads,
                                 int           *piColumnWidths,
                                 int            cb,
                                 DWORD          dwCookie)
{

    TraceTag((tagDataFrame, "CDataFrame::CreateListbox"));

    HRESULT     hr;
    long        lCalculatedWidth;
    int         c;
	int			iMinRows;

    BOOL        flag = _pDoc->_fDeferedPropertyUpdate;

    _pDoc->_fDeferedPropertyUpdate = TRUE;



    ((CRootDataFrame*)this)->DeleteInstances();

    Assert(_pDetail);
    Assert(_pDetail == _pDetail->getTemplate());
    Assert(listStyle > 0);

    // the detail template should be invisible from the beginning
    _fVisible = FALSE;

    for (c = _pDetail->_arySites.Size()-1;
         c >= 0; c--)

    {
        // _pDetail->DeleteSite(_pDetail->_arySites[c],DELSITE_NOREGENERATE | DELSITE_NOTABADJUST);
        _pDetail->_arySites[c]->Detach();
        _pDetail->_arySites[c]->Release();
    }
    _pDetail->_arySites.DeleteAll();
    ((CDetailFrame*)_pDetail)->EmptyRecycle();

    #if PRODUCT_97
    if (((CDetailFrame*)_pDetail)->TBag()->_pMatrix)
    {
        ((CDetailFrame*)_pDetail)->TBag()->_pMatrix->Init(_pDetail);
    }
    #endif

    if (_pHeader)
    {
        for (c = _pHeader->_arySites.Size()-1;
             c >= 0; c--)
        {
            // _pHeader->DeleteSite(_pHeader->_arySites[c],DELSITE_NOREGENERATE | DELSITE_NOTABADJUST);
            _pHeader->_arySites[c]->Detach();
            _pHeader->_arySites[c]->Release();
        }
        _pHeader->_arySites.DeleteAll();
        #if PRODUCT_97
        if (((CDetailFrame*)_pHeader)->TBag()->_pMatrix)
        {
            ((CDetailFrame*)_pHeader)->TBag()->_pMatrix->Init(_pHeader);
        }
        #endif

        ((CDetailFrame*)_pHeader)->EmptyRecycle();
        if (!fColumnHeads)
        {
            DeleteSite(_pHeader, DELSITE_NOTABADJUST);
            _pHeader = 0;
            TBag()->_fShowHeaders = FALSE;
        }
    }

    // arriving here, we should have a valid dataframe, already inserted in
    //  the forms context

    _pDoc->_fDeferedPropertyUpdate = TRUE;

    hr = THR(SetBackColor(olecolorBack));
    if (hr)
        goto Error;

    hr = THR(SetForeColor(olecolorFore));
    if (hr)
        goto Error;

    hr = THR(SetScrollBars(ScrollBarStyle));
    if (hr)
        goto Error;

    hr = THR(SetListBoxStyle(listStyle));
    if (hr)
        goto Error;


    // set up the size for the detail template:

    _pDetail->_rcl = _rcl;

    // give the detail template some size (any number will do just fine...
    _pDetail->_rcl.bottom = _rcl.top + 502;


    if (iListRows > 0)
    {
        hr = THR(SetMaxRows(iListRows));
        if (hr)
            goto Error;

		iMinRows = listStyle == fmListBoxStylesComboBox? 1 : iListRows;
        hr = THR(SetMinRows(iMinRows));
        if (hr)
            goto Error;
    }

    hr = THR(SetAutosizeStyle(fIntegralHeight ?
                        fmEnAutoSizeVertical :
                        fmEnAutoSizeNone));
    if (hr)
        goto Error;

    if (pRowSet)
    {
        // now we got to set the rowset pointer,
        hr = THR(SetupRowset(pRowSet));
        if (hr)
            goto Error;
        hr = THR(CreateDetailFromCursor(iColumnCount, piColumnWidths, cb));
        if (hr)
            goto Error;
    }


    hr = THR(SetColumnHeads(fColumnHeads));
    if (hr)
        goto Error;

    hr = THR(SetupVisuals(olecolorBack, olecolorFore, dwCookie));
    if (hr)
        goto Error;


//  BUGBUG (garybu) Huh?
//    CalculateColumnSize(&_pDoc->_RootSite._Transform, _rcl.Width(), &lCalculatedWidth, piColumnWidths, cb);
lCalculatedWidth=0;

    SetDeferedPropertyUpdate(flag);

Cleanup:
    RRETURN(hr);

Error:
    // BUGBUG: no recovery set up
    goto Cleanup;
}
//-End of Method ----------------------------------------------------------







//+------------------------------------------------------------------------
//
//  Member:     SetColumnHeads
//
//  Synopsis:   turns on/off headers
//
//-------------------------------------------------------------------------
HRESULT CDataFrame::SetColumnHeads(VARIANT_BOOL fColumnHeads)
{
    HRESULT hr = S_OK;

    hr = SetShowHeaders(fColumnHeads);
    if (hr)
    {
        goto Cleanup;
    }
    if (fColumnHeads)
    {
        hr = CreateHeaderFromCursor();
    }

Cleanup:
    RRETURN(hr);
}
//-End of Method ----------------------------------------------------------




//+------------------------------------------------------------------------
//
//  Member:     SetupRowset
//
//  Synopsis:   takes the passed in rowset, and
//              maps it to the internally used OLECURSOR
//
//-------------------------------------------------------------------------
HRESULT CDataFrame::SetupRowset(IUnknown *pRowSet)
{

    TraceTag((tagDataFrame, "CDataFrame::SetupRowset"));

    HRESULT hr;
    CDataLayerAccessorSource *pdlas;
    CDataLayerCursor *pCursor;

    if (pRowSet)
    {
        hr = THR(LazyGetDLAccessorSource(&pdlas));
        if (hr)
            goto Cleanup;

        hr = THR(pdlas->SetRowsetSource(pRowSet));
        if (hr)
            goto Cleanup;

        hr = THR(pdlas->LazyGetDataLayerCursor(&pCursor)); // BUGBUG: Do we need this?
        if (hr)
            goto Cleanup;

        hr = THR(pdlas->SetAdviseDLASEvents(&TBag()->_dlSink));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = E_INVALIDARG;
    }

Cleanup:

    RRETURN(hr);
}
//-End of Method ----------------------------------------------------------




//+------------------------------------------------------------------------
//
//  Member:     CreateDetailFromCursor
//
//  Synopsis:   walks the cursor and creates MDCs in the detailtemplate
//
//-------------------------------------------------------------------------
HRESULT CDataFrame::CreateDetailFromCursor(int iColumnCount, int *piColumnWidths, int cb)
{

    TraceTag((tagDataFrame, "CDataFrame::CreateDetailFromCursor"));
    Assert(_pDetail);
    Assert(_pDetail == _pDetail->getTemplate());
    Assert(_pDetail->_arySites.Size()==0);

    // All the above assumes that this method is called on an empty detail template

    CDataLayerCursor *pCursor;
    ULONG            ulColumnCount=0;
    CRectl           rcl(0,0,0,0);
    COleDataSite *   pControl = 0;
    int              i;
    int              lWidth;

    HRESULT hr;

    rcl = _pDetail->_rcl;

    hr = THR(LazyGetDataLayerCursor(&pCursor));
    if (hr)
        goto Cleanup;

    hr = THR(pCursor->GetColumnCount(ulColumnCount));
    if (hr)
        goto Cleanup;




    if (iColumnCount <= 0)
    {
        iColumnCount = (int) ulColumnCount;
    }
    else
    {
        iColumnCount = min(iColumnCount, (int) ulColumnCount);
    }



    //  if the liststyle is not plain, create the record selector site
    if ( TBag()->_eListStyle != fmListStylePlain )
    {

        // first check if we have to create anything (this will skip creating
        //  a recordselector if not needed.
        for (i=0; i<iColumnCount;i++)
        {
            lWidth = ((piColumnWidths) && (i<cb) && (*(piColumnWidths+i) >= 0))
                ? (*(piColumnWidths+i))
                : -1;

            if (lWidth != 0)            // Create control unless width == 0
            {
                goto Continue;
            }
        }
        // so we did not need to create any control, skip out
        goto Cleanup;

        Continue:

        CRectl rclSelector(rcl);

        rclSelector.right = rclSelector.left + HimetricFromHPix(RECORD_SELECTOR_SIZE +
                                                                2 * RECORD_SELECTOR_CLEARANCE);

        hr = CreateRecordSelector(&rclSelector);
        if ( hr )
            goto Cleanup;

        pControl = (COleDataSite *)_pDetail->_arySites[0];
        Assert(pControl->TestClassFlag(SITEDESC_OLEDATASITE));
        Assert(pControl->_fFakeControl);

        rcl.left += pControl->_rcl.Width();
    }

    for (i=0; i<iColumnCount;i++)
    {
        lWidth = ((piColumnWidths) && (i<cb) && (*(piColumnWidths+i) >= 0))
            ? (*(piColumnWidths+i))
            : -1;

        if (lWidth != 0)            // Create control unless width == 0
        {
            rcl.right = rcl.left + HIMETRIC_INCH;   // Any # will do.
            hr = THR(_pDetail->InsertNewSite(
                            CLSID_CMdcText,
                            NULL,
                            &rcl,
                            0,
                            (CSite **) &pControl));
            if (hr)
                goto Cleanup;

            hr = THR(pControl->CreateAccessorByColumnNumber(i+1));  // 1..n
            if (hr)
                goto Cleanup;

            rcl.left = rcl.right;
        }
    }





Cleanup:
    _pDetail->_rcl.right = rcl.right;
    RRETURN(hr);
}
//-End of Method ----------------------------------------------------------





//+------------------------------------------------------------------------
//
//  Member:     CreateHeaderFromCursor
//
//  Synopsis:   walks the cursor and the detail and creates matching MDCs
//              for every detail cell
//              the method is using the defered property update
//              to force regenerating
//
//-------------------------------------------------------------------------
HRESULT CDataFrame::CreateHeaderFromCursor(void)
{
    Assert(_fOwnTBag);  // should not be called on an instance
    Assert(_pDetail);
    Assert(_pHeader);

    COleDataSite *      pControl;
    COleDataSite        *pSite;
    CDetailFrame        *pDetail;
    HRESULT             hr = S_OK;
    CSite               **ppSite;
    CRectl              rcl;
    TCHAR               *ptchar=0;
    BOOL                flag = _pDoc->_fDeferedPropertyUpdate;
    int                 c;
    int                 iOldX;

    _pDoc->_fDeferedPropertyUpdate = TRUE;

    pDetail = (CDetailFrame*)(_pDetail->getTemplate());
    rcl = _pHeader->_rcl;
    iOldX = rcl.left;

    for (ppSite = pDetail->_arySites,  c = pDetail->_arySites.Size();
         c > 0;
         ppSite++, c--)
    {
        pSite = ((COleDataSite*)*ppSite);
        rcl.left = pSite->_rcl.left;
        rcl.right = pSite->_rcl.right;

        if (rcl.left >= iOldX)
        {
            hr = THR(_pHeader->InsertNewSite(
                        CLSID_CMdcText,
                        NULL,
                        &rcl,
                        0,
                        (CSite **) &pControl));
            if (hr)
                goto Error;

            pControl->TBag()->_pRelated = pSite;

            hr = pControl->BindIndirect();
            if (hr)
                goto Error;

            iOldX = rcl.right;
        }

    }
    hr = S_OK;

    SetDeferedPropertyUpdate(flag);

Error:
    RRETURN(hr);
}
//-End of Method ----------------------------------------------------------



//+------------------------------------------------------------------------
//
//  Member:     SetControlProperty
//
//  Synopsis:   walks the dataframe template tree
//              and stuffs the fontobject in
//
//-------------------------------------------------------------------------
HRESULT
CDataFrame::SetControlProperty(DISPID dispid, VARIANT *pVariant)
{
    HRESULT         hr = S_OK;
    BOOL            flag = _pDoc->_fDeferedPropertyUpdate;
    CSite           **ppSite;
    EXCEPINFO       except;
    int c;
    COleDataSite    *pSite;
    CBaseFrame      *pDetail = _pDetail->getTemplate();
    long            lHeightMax;



    memset(&except, 0, sizeof(EXCEPINFO));

    Assert(pDetail);



    hr = SetDeferedPropertyUpdate(TRUE);
    if (hr)
        goto Cleanup;

    while (pDetail)
    {
        lHeightMax = 0;
        for (ppSite = pDetail->_arySites,  c = pDetail->_arySites.Size();
             c > 0;
             ppSite++, c--)
        {
            pSite = (COleDataSite *)*ppSite;
            Assert(pSite->TestClassFlag(SITEDESC_OLEDATASITE));
            if (pSite->_fFakeControl)
                continue;

            Assert(pSite->GetMorphInterface());  // Verify that the control is an internal one.
            pSite->CacheDispatch();
            hr = SetDispProp(pSite->_pDisp, dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, pVariant, &except);
            if (hr)
                goto Error;

            if (dispid == DISPID_FONT)
            {
                hr = THR(ComputeNaturalExtent(pSite, &lHeightMax));
                if (hr)
                    goto Error;
            }
        }
        if (dispid == DISPID_FONT)
        {
            pDetail->_rcl.bottom = pDetail->_rcl.top + lHeightMax;
            pDetail->_fIsDirtyRectangle = TRUE;
        }

        if ( TBag()->_eListBoxStyle == fmListBoxStylesListBox )
        {
            Assert(pDetail->TestClassFlag(SITEDESC_DETAILFRAME));
            Assert(pDetail->_fOwnTBag);

            if ( dispid == DISPID_BACKCOLOR )
            {
                hr = THR(SetColor(&(_colorBack), pVariant->lVal, DISPID_BACKCOLOR));
                if (hr)
                    goto Error;
            }

            if ( dispid == DISPID_FORECOLOR )
            {
                hr = THR(SetColor(&(_colorFore), pVariant->lVal, DISPID_FORECOLOR));
                if (hr)
                    goto Error;
            }
        }

        if (pDetail == _pDetail->getTemplate())
        {
            pDetail = _pHeader ? _pHeader->getTemplate() : 0;
        }
        else
        {
            pDetail = 0;
        }

    }

Cleanup:
    hr = SetDeferedPropertyUpdate(flag);
    FreeEXCEPINFO(&except);
    RRETURN(CloseErrorInfo(hr));

Error:
    if (hr == DISP_E_EXCEPTION)
    {
        IGNORE_HR(SetErrorInfoFromEXCEPINFO(&except));
        hr = ResultFromScode(except.scode);
    }
    goto Cleanup;
}
//-End of Method ----------------------------------------------------------


//+------------------------------------------------------------------------
//
//  Member:     SetupFonts
//
//  Synopsis:   walks the dataframe template tree
//              and stuffs the fontobject in
//
//-------------------------------------------------------------------------
HRESULT
CDataFrame::SetupVisuals(OLE_COLOR olecolorBack, OLE_COLOR olecolorFore, DWORD dwCookie)
{
    TraceTag((tagDataFrame, "CDataFrame::SetupFonts"));
    Assert(_pDetail);
    Assert(_pDetail == _pDetail->getTemplate());

    CSite **        ppSite;
    COleDataSite    *pControl;
    HRESULT         hr = S_OK;
    int             c;
    CBaseFrame      *pDetail = _pDetail->getTemplate();
    long            lHeightMax;
    CRectl          rcl;
    VARIANT         var;

    VariantInit(&var);

    Assert(pDetail);


    while (pDetail)
    {
        ((CDetailFrame*)pDetail)->SetBackColor(olecolorBack);
        ((CDetailFrame*)pDetail)->SetForeColor(olecolorFore);
        ((CDetailFrame*)pDetail)->SuppressControlBorders(TRUE);

        lHeightMax = 0;

        for (ppSite = pDetail->_arySites,  c = pDetail->_arySites.Size();
             c > 0;
             ppSite++, c--)
        {
            pControl = (COleDataSite*)*ppSite;

            pControl->_fNoUIActivate = TRUE;

            if ( pControl->_fFakeControl )
                continue;

            var.lVal = dwCookie;
            hr = THR(pControl->GetMorphInterface()->DebugHook(102254, &var));
            if (hr)
                goto Cleanup;

            hr = THR(ComputeNaturalExtent(pControl, &lHeightMax));
            if (hr)
                goto Cleanup;
        }

        // now set the template size to be the max

        pDetail->_rcl.bottom = pDetail->_rcl.top + lHeightMax;

        if (pDetail == _pDetail->getTemplate())
        {
            pDetail = _pHeader ? _pHeader->getTemplate() : 0;
        }
        else
        {
            pDetail = 0;
        }
    }

    if (_pHeader)
    {
        // move the detailtemplate up/down...
        rcl = _pDetail->getTemplate()->_rcl;
        rcl.top = _pHeader->_rcl.bottom;
        rcl.bottom = rcl.top + _pDetail->getTemplate()->_rcl.Height();
        _pDetail->getTemplate()->Move(&rcl,  SITEMOVE_NOFIREEVENT);
    }



Cleanup:
    RRETURN(hr);
}
//-End of Method ----------------------------------------------------------




//+------------------------------------------------------------------------
//
//  Member:     Calculate columnsize
//
//  Synopsis:   gets called when the extend is know to calculate
//              the not specified widths of controls
//
//-------------------------------------------------------------------------
HRESULT
CDataFrame::CalculateColumnSize(CTransform * pTransform,
                                long lRowWidth,
                                OUT long *plCalculatedWidth,
                                int *piColumnWidths,
                                int cb)
{
    int             i;
    int             c;
    int             iLeftOvers=0;
    int             iHoleCount=0;
    int             iCompleteWidth=0;
    int             iLastOne;
    CRectl          rcl;
    CSite           **ppSite;
    COleDataSite    *pSite;
    CBaseFrame      *pDetail = _pDetail->getTemplate();
    long            lRight;

    // first figure how many holes we have in the array

    Assert(!cb || piColumnWidths);
    Assert(plCalculatedWidth);

    *plCalculatedWidth = -1;        // it's not calculated yet

    for (i=0; i<cb;i++)
    {
        if ((*(piColumnWidths+i) < 0))  //  < 0 indicates autosize
        {
            iLeftOvers++;
        }
        else
        {
            // assign iLastOne, so that it will be initialized
            // with the right value in the case of just one column
            iLastOne = (*(piColumnWidths+i));
            iCompleteWidth+=iLastOne;
        }
    }

    // now calculate how many controls we have that were unspecified
    iLeftOvers = pDetail->_arySites.Size() - (cb-iLeftOvers);

    //  if the liststyle is not plain, substract the recordselector
    if ( TBag()->_eListStyle != fmListStylePlain )
    {
        iLeftOvers--;
        // we also need to substract the width of the button from
        //  the rowwidth, before we do the rest of the calculations...
        for (ppSite = pDetail->_arySites,  c = pDetail->_arySites.Size();
        c > 0;
        ppSite++, c--)
        {
            pSite = ((COleDataSite*)*ppSite);
            if ( pSite->_fFakeControl )
            {
                // found the recordselector, get the width and bail out
                iCompleteWidth += pSite->_rcl.Width();
                break;
            }
        }
    }

    rcl = pDetail->_rcl;

    if (iLeftOvers > 0)
    {
        //
        iLeftOvers = (lRowWidth - (long)iCompleteWidth) / iLeftOvers;
        iLeftOvers = max(HIMETRIC_INCH, iLeftOvers);
        iLastOne = iLeftOvers;
    }
    else
    {
        // so everyone has a fixed size, we probably need to pad the righmost one
        if ((long)iCompleteWidth < lRowWidth && piColumnWidths)
        {
            iLastOne = (int)lRowWidth-iCompleteWidth + (*(piColumnWidths+cb-1));
        }
    }


    for (i=0, ppSite = pDetail->_arySites,  c = pDetail->_arySites.Size();
        c > 0;
        ppSite++, c--, i++)
    {
        pSite = ((COleDataSite*)*ppSite);
        if ( pSite->_fFakeControl )
        {
            i--;
            rcl.left = pSite->_rcl.right;
            continue;
        }

        iCompleteWidth = ((piColumnWidths) && (i<cb) && (*(piColumnWidths+i) >= 0))
            ? (*(piColumnWidths+i))
            : (long) iLeftOvers;

        if (iCompleteWidth == 0)
        {
            // we never created that control, therefore skip it
            i--;
            continue;
        }

        if (c==1)
        {
            // last control...
            iCompleteWidth = iLastOne;
        }

        rcl.right = rcl.left +iCompleteWidth;
        pSite->_rcl = rcl;
        rcl.left = rcl.right;
    }

    // Change the template size.
    if (pDetail->_arySites.Size())
    {
        // so we have controls, ergo the template is going
        // to be as wide as all controls together
        lRight = rcl.right;
    }
    else
    {
        // else the detail is going to be the same size as
        //  the rowwidth that was passed in
        lRight = pDetail->_rcl.left + lRowWidth;
    }

    if (pDetail->_rcl.right != lRight)
    {
        pDetail->_rcl.right = lRight;
        pDetail->_fIsDirtyRectangle = TRUE;
        if (_pHeader)
        {
            _pHeader->_rcl.right = lRight;
            _pHeader->_fIsDirtyRectangle = TRUE;
        }
    }


    if (lRowWidth < pDetail->_rcl.Width())
    {
        *plCalculatedWidth = lRowWidth;
    }
#if DBG==1 && !defined(PRODUCT_97)
    g_lRowWidth.Exchange(pDetail->_rcl.Width());
    g_lRowHeight.Exchange(pDetail->_rcl.Height());
#endif

    // Notification that template size have changed.
    if (pTransform)
        AlignTemplatesToPixels(pTransform);
    TBag()->_propertyChanges.SetCreateToFit(TRUE);

    if (*plCalculatedWidth == -1)
    {
        *plCalculatedWidth = pDetail->_rcl.Width();
    }

    return S_OK;

}
//-End of Method ----------------------------------------------------------



//+------------------------------------------------------------------------
//
//  Member:     HRESULT AlignTemplatesToPixels(CTransform * pTransform);
//
//  Synopsis:   walks over the templates and resizes them to be
//              pixel aligned
//
//  Arguments:  none
//
//  returns:    S_OK
//
//-------------------------------------------------------------------------
HRESULT
CDataFrame::AlignTemplatesToPixels(CTransform * pTransform)
{
    CRectl          rcl;
    CRectl          rclTemp;
    COleDataSite   *pSite;
    CSite           **ppSite;
    COleDataSite    *pRelated;
    int             c;
    CBaseFrame      *pDetail = _pDetail->getTemplate();
    int             cx;
    SIZEL           sizel;
    CTransform      Transform;
    CRect           rc;
    CTBag          *pTBag = TBag();

    if (_pDoc->_state < OS_INPLACE ||
        !pTransform ||
        !(pTransform->_sizeSrc.cx &&
          pTransform->_sizeSrc.cy &&
          pTransform->_sizeDst.cx &&
          pTransform->_sizeDst.cy) )
    {
        rc.left = rc.top = 0;
        rc.right = g_sizePixelsPerInch.cx;
        rc.bottom = g_sizePixelsPerInch.cy;
        sizel.cx = HIMETRIC_INCH;
        sizel.cy = HIMETRIC_INCH;
        Transform.Init(&rc, sizel);
        pTransform = &Transform;
    }

    rcl = pDetail->_rcl;
    pTransform->WindowFromDocument(&rc, &rcl);
    pTransform->DocumentFromWindow(&rcl, &rc);
    // BUGBUG ugly hack we need until relative coordinate system (and maybe even after)
    // this sets up the transform so that a row height is an integral number of
    // himetric per pixel (istvanc)
    pTransform->_sizeDst.cy = rc.bottom - rc.top;
    pTransform->_sizeSrc.cy = rcl.bottom - rcl.top;

    //  If we are a listbox with ListStyle: Option, make sure that
    //  the radiobutton/checkbox images fit in the row without cbeing clipped.

    if ( pTBag->_eListStyle == fmListStyleOption &&
         rc.Height() < RECORD_SELECTOR_SIZE + 2 * RECORD_SELECTOR_CLEARANCE )
    {
        rc.bottom = rc.top + RECORD_SELECTOR_SIZE + 2 * RECORD_SELECTOR_CLEARANCE;
        pTransform->DocumentFromWindow(&rcl, &rc);
    }

    if (rcl != pDetail->_rcl)
    {
        pDetail->_rcl = rcl;
        pDetail->_fIsDirtyRectangle = TRUE;
    }

    for (ppSite = pDetail->_arySites,  c = pDetail->_arySites.Size();
        c > 0;
        ppSite++, c--)
    {
        pSite = ((COleDataSite*)*ppSite);

        cx = pSite->_rcl.Width();

        pTransform->WindowFromDocument(&sizel, cx, 0);
        cx = sizel.cx;
        pTransform->DocumentFromWindow(&sizel, cx, 0);
        rcl.right = rcl.left + sizel.cx;
        pSite->_rcl = rcl;
        rcl.left = rcl.right;

    }

    if (pDetail->_rcl.right != rcl.right)
    {
        pDetail->_rcl.right = rcl.right;
        pDetail->_fIsDirtyRectangle = TRUE;
    }

    if (_pHeader)
    {

        pTransform->WindowFromDocument((RECT*)&(_pHeader->_rcl), (RECTL*)&(_pHeader->_rcl));
        pTransform->DocumentFromWindow((RECTL*)&(_pHeader->_rcl), (RECT*)&(_pHeader->_rcl));
        if (_pHeader->_rcl.left != _pDetail->_rcl.left ||
            _pHeader->_rcl.right != _pDetail->_rcl.right)
        {
            _pHeader->_fIsDirtyRectangle = TRUE;
            _pHeader->_rcl.left = _pDetail->_rcl.left;
            _pHeader->_rcl.right = _pDetail->_rcl.right;
        }

        for (ppSite = _pHeader->_arySites,  c = _pHeader->_arySites.Size();
             c > 0;
             ppSite++, c--)
        {
            pSite = ((COleDataSite*)*ppSite);
            pRelated = pSite->TBag()->_pRelated;
            Assert(pRelated);
            pSite->_rcl.left = pRelated->_rcl.left;
            pSite->_rcl.right = pRelated->_rcl.right;
            pSite->_rcl.bottom = _pHeader->_rcl.bottom;
        }
    }

    return S_OK;
}
//-End of Method ----------------------------------------------------------



//+------------------------------------------------------------------------
//
//  Member:     CreateRecordSelector
//
//  Synopsis:   inserts the record selector site into the detail template
//
//  Arguments:  prcl:   points to the rcl the site should occupy
//
//-------------------------------------------------------------------------

HRESULT
CDataFrame::CreateRecordSelector(CRectl * prcl)
{
    HRESULT hr = S_OK;
    COleDataSite * pRS;
    CBaseFrame * pfrTemplate;

    pRS = NULL;
    pfrTemplate = _pDetail->getTemplate();

    TraceTag((tagDataFrame,"CDataFrame::SetRecordSelectors"));

    //  Show RecordSelector site

    //  BUGBUG: Fire events and stuff

    //  Create a new OleDataSiteRS
    pRS = (COleDataSite *)pfrTemplate->CreateSite(ETAG_OLEDATASITE);
    if ( ! pRS )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //  Set the FakeControl OlesiteFlag
    pRS->_fFakeControl = TRUE;
    pRS->_fRecordSelector = TRUE;

    //  insert it into the detail template
    hr = pRS->InsertedAt(pfrTemplate,CLSID_CFakeControl,NULL,prcl,0 /*non-centered*/);
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}
//-End of Method ----------------------------------------------------------








//+-------------------------------------------------------------------------
//
//  Method:     ComputeExtent
//
//  Synopsis:   compute the extent of the data frame based on the proposed size
//              part of the Dataframe expert interface
//
//--------------------------------------------------------------------------

HRESULT
CDataFrame::ComputeExtent(
                          IN  int   *piColumnWidths,
                          IN  int   cb,
                          IN  void  *pTransform,   // BUGBUG - Type is CTransform. Couldn't get
                          IN  SIZEL *pszlProposed, // CTransform to compile in IDL file.
                          OUT SIZEL *pszlComputed)
{
    HRESULT         hr = S_OK;
    CSizel          szlProposed;
    long            lCalculatedWidth;
    CBaseFrame *    pDetail = _pDetail->getTemplate();
    CRectl          rcl;

    if (!pszlProposed || !pszlComputed)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    szlProposed = *pszlProposed;

    hr = THR(CalculateColumnSize((CTransform *) pTransform,
                                 szlProposed.cx,
                                 &lCalculatedWidth,
                                 piColumnWidths,
                                 cb));
    if (hr)
        goto Cleanup;

    szlProposed.cx = lCalculatedWidth;

    // 1. Set proposed position
    rcl.left = _rcl.left;
    rcl.top = _rcl.top;
    rcl.SetSize(szlProposed);

    if (TBag()->_eListBoxStyle == fmListBoxStylesComboBox)
    {
        // combobox

        // Clip the crows to the szlProposed. SzlProposed.cy is
        // the screen height when the style is combobox.  SzlProposed
        // is not the screen size when the style is a listbox.

        // Per the spec, a combobox's height is computed from max rows.
        // However, if the computed height is greater than the szlProposed
        // then the max rows is reduced to fit within the szlProposed.

        // We're explicitly not firing a property change notification here.
        CBaseFrame  *   pDetailTemplate = _pDetail->getTemplate();
        CSizel          szlTemplate (pDetailTemplate->_rcl.Size());
        CRectl      *   prclHeader = _pHeader? &_pHeader->_rcl: NULL;
        CTBag       *   pTBag = TBag();
        long            lMaxRows = pTBag->_lMaxRows;
        long            lMinRows = pTBag->_lMinRows;

        CalcSizeFromProperties(&rcl, prclHeader, &szlTemplate, lMaxRows,lMinRows);
        if (rcl.Dimension (1) > szlProposed.cy)
        {
            rcl.SetSize(szlProposed);
            CalcPropertiesFromSize(&rcl, prclHeader, &szlTemplate, &lMaxRows, &lMinRows);
            CalcSizeFromProperties(&rcl, prclHeader, &szlTemplate, lMaxRows,lMinRows);
        }
    }
    else
    {
        // listbox

        // Reset all proposed flags.
        _pDoc->_RootSite.DoResetProposed();
        pDetail->SetProposed(pDetail, &pDetail->_rcl);
        SetProposed(this, &rcl);
#if PRODUCT_97
        _fProposedOutsideScrollbars = _fOutsideScrollbars;
#endif

        // 2. Calculate desired proposed position
        hr = CalcProposedPositions();
        GetProposed(this, &rcl);
    }

    pszlComputed->cx = rcl.Width();
    pszlComputed->cy = (rcl.Height() ? rcl.Height() : pszlProposed->cy);

Cleanup:
    RRETURN (hr);
}
//-End of Method ----------------------------------------------------------




//+-------------------------------------------------------------------------
//
//  Method:     ComputeExtent
//
//  Synopsis:   compute the extent of the data frame based on the proposed size
//              part of the Dataframe expert interface
//
//--------------------------------------------------------------------------
HRESULT CDataFrame::ComputeNaturalExtent(COleDataSite *pods, long * plHeightMax)
{
    Assert(pods->_fUseViewObjectEx);
    Assert(pods->_pVO);
    HRESULT hr;
    SIZEL           sizel;
    CRectl          rcl;

    Assert(pods->GetMorphInterface());  // Verify that the control is an internal one.

    // now get the natural height of the control....
    sizel = pods->_rcl.Size();
    hr = ((IViewObjectEx*)(pods->_pVO))->GetNaturalExtent(DVASPECT_CONTENT, -1, 0, 0, 0, &sizel);
                Assert(!hr && "GetNaturalExtent failed");
    if (hr)
        goto Cleanup;

    rcl = pods->_rcl;
    rcl.bottom = pods->_rcl.top + sizel.cy;

    pods->Move(&rcl, SITEMOVE_NOFIREEVENT);

    *plHeightMax = max(*plHeightMax, sizel.cy);

Cleanup:
    RRETURN(hr);
}
//-End of Method ----------------------------------------------------------


