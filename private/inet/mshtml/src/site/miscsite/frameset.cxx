//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       frameset.cxx
//
//  Contents:   Implementation of CFrameSetSite
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include "cguid.h"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_FRAMELYT_HXX_
#define X_FRAMELYT_HXX_
#include "framelyt.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include <htiframe.h>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#define _cxx_
#include <frameset.hdl>

MtDefine(CFrameSetSite, Elements, "CFrameSetSite")
MtDefine(CFrameSetSite_aryFrames_pv, CFrameSetSite, "CFrameSetSite::_aryFrames::_pv")
MtDefine(CFrameSetSite_aryFormVisit_pv, CFrameSetSite, "CFrameSetSite::_aryFormVisit::_pv")
MtDefine(CNoFramesElement, Elements, "CNoFramesElement")
MtDefine(CFramesetChildIterator, Layout, "CFramesetChildIterator")

enum XorYLoop { XDirection = 0, YDirection = 1, Terminate = 2 };

// implementation of CRootSite::CreateLayout()
IMPLEMENT_LAYOUT_FNS(CFrameSetSite, CFrameSetLayout)

#ifndef NO_PROPERTY_PAGE
const CLSID * CFrameSetSite::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE

CElement::ACCELS CFrameSetSite::s_AccelsFrameSetSiteDesign = CElement::ACCELS (&CElement::s_AccelsElementDesign, IDR_ACCELS_FRAMESET_DESIGN);
CElement::ACCELS CFrameSetSite::s_AccelsFrameSetSiteRun = CElement::ACCELS (&CElement::s_AccelsElementRun, IDR_ACCELS_FRAMESET_RUN);

const CElement::CLASSDESC CFrameSetSite::s_classdesc =
{
    {
        &CLSID_HTMLFrameSetSite,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _apiidConect
        ELEMENTDESC_BODY |                  // _dwFlags
        ELEMENTDESC_NOTIFYENDPARSE,
        &IID_IHTMLFrameSetElement,          // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLFrameSetElement,
    &s_AccelsFrameSetSiteDesign,            // _pAccelsDesign
    &s_AccelsFrameSetSiteRun                // _pAccelsRun
};

const CElement::CLASSDESC CNoFramesElement::s_classdesc = { 0 };

//+------------------------------------------------------------------------
//
//  Member:     CFrameSetSite::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CFrameSetSite::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_TEAROFF(this, IHTMLFrameSetElement2, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrameSetSite::CreateElement, public
//
//  Synopsis:   Creates a CFrameSetSite
//
//----------------------------------------------------------------------------

HRESULT
CFrameSetSite::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert (ppElement);

    if (pht->Is(ETAG_FRAMESET))
    {
        *ppElement = new CFrameSetSite(pDoc);
    }
    else // ETAG_NOFRAMES
    {
        Assert(pht->Is(ETAG_NOFRAMES));

        *ppElement = new CNoFramesElement(pDoc);
    }

    RRETURN ((*ppElement) ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrameSetSite::CFrameSetSite, public
//
//  Synopsis:   CFrameSetSite ctor
//
//----------------------------------------------------------------------------

CFrameSetSite::CFrameSetSite ( CDoc * pDoc )
  : super(ETAG_FRAMESET, pDoc)
{
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
    m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    _pNFE = NULL;
};


//+---------------------------------------------------------------------------
//
//  Member:     CFrameSetSite::Init2, public
//
//  Synopsis:   Parse the ROWS and/or COLS attributes
//
//  Arguments:  (none)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CFrameSetSite::Init2(CInit2Context * pContext)
{
    HRESULT   hr;
    CDoc *    pDoc = Doc();
    
    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

    pDoc->_fFrameSet = TRUE;
    pDoc->_fFrameBorderCacheValid = FALSE;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CFrameSetSite::Notify
//
//--------------------------------------------------------------------------

void
CFrameSetSite::Notify(CNotification *pNF)
{
    super::Notify(pNF);

    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
    {
        LPCTSTR    pStrFrameBorder = GetAAframeBorder();
        CElement * pParent         = GetFirstBranch()->Parent()->Element();

        //
        // Alert the view that the top client element may have changed
        //

        Doc()->OpenView();

        DoNetscapeMappings();

        if (pStrFrameBorder)
        {
            _fFrameBorder =  pStrFrameBorder[0] == _T('y')
                          || pStrFrameBorder[0] == _T('Y')
                          || pStrFrameBorder[0] == _T('1');
        }
        else if (pParent->Tag() == ETAG_FRAMESET)
        {
            _fFrameBorder = DYNCAST(CFrameSetSite, pParent)->_fFrameBorder;
        }
        else // top-level <frameset>, default is TRUE.
        {
            _fFrameBorder = TRUE;
        }
    }
        
        Layout()->SetRowsCols();

        //
        // Notify the view of a new possible top element
        //

        SendNotification(NTYPE_VIEW_ATTACHELEMENT);

    case NTYPE_END_PARSE:
        ResizeElement();
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        //
        // Notify the view that the top element may have left
        //

        SendNotification(NTYPE_VIEW_DETACHELEMENT);
        break;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CFrameSetSite::ExpectedFrames
//
//  Synopsis:   Returns the number of frames expected (used during parsing)
//
//--------------------------------------------------------------------------

int
CFrameSetSite::ExpectedFrames()
{
    int iRows = Layout()->_aryRows.Size();
    int iCols = Layout()->_aryCols.Size();

    if (!iRows)
        iRows = 1;

    if (!iCols)
        iCols = 1;

    return (iRows * iCols);
}


//+-------------------------------------------------------------------------
//
// Member: CFrameSetSite::FrameSpacingAttribute
//
//  Synopsis:   walk up the tree looking for a framespacing attribute
//
//--------------------------------------------------------------------------
CUnitValue
CFrameSetSite::FrameSpacingAttribute()
{
    CUnitValue uv = GetAAframeSpacing();
    CLayout *  pLayoutParent = GetCurParentLayout();

    if (uv.IsNull() && pLayoutParent && pLayoutParent->Tag() == ETAG_FRAMESET)
    {
        uv = DYNCAST(CFrameSetSite, pLayoutParent->ElementOwner())->FrameSpacingAttribute();
    }
    return uv;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameSetSite::GetFrameSpacing()
//
//  Synopsis:   compute the amount of space between frames
//
//--------------------------------------------------------------------------

int
CFrameSetSite::GetFrameSpacing()
{
    CUnitValue uv = FrameSpacingAttribute();
    return uv.IsNull() ? 2 : uv.GetPixelValue();
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameSetSite::DoNetscapeMappings()
//
//  Synopsis:   map netscapes frame attributes to ours
//
//--------------------------------------------------------------------------

void
CFrameSetSite::DoNetscapeMappings()
{
    CUnitValue uv = FrameSpacingAttribute();
    if (uv.IsNull())
    {
        uv = BorderAttribute();
        if (!uv.IsNull())
        {
            // OK, so we have a border attribute and no
            // framespacing attribute.  Now we need to
            // map the values appropriately.

            int iPixelValueFrameSpacing = uv.GetPixelValue();
            VARIANT iFrameSpacing;
            iFrameSpacing.vt = VT_I2;
            iFrameSpacing.iVal = (SHORT)iPixelValueFrameSpacing;

            switch(iPixelValueFrameSpacing)
            {
            case 0:
                iFrameSpacing.iVal = 0;
                put_StringHelper(_T("0"), (PROPERTYDESC *)&s_propdescCFrameSetSiteframeBorder);
                break;
            case 1:
            case 2:
                iFrameSpacing.iVal = _fFrameBorder ? 2 : 4;
                put_StringHelper(_T("0"), (PROPERTYDESC *)&s_propdescCFrameSetSiteframeBorder);
                break;
            case 3:
            case 4:
                iFrameSpacing.iVal = _fFrameBorder ? 1 : 4;
                put_StringHelper(_fFrameBorder ? _T("1") : _T("0"), (PROPERTYDESC *)&s_propdescCFrameSetSiteframeBorder);
                break;
            default:
                iFrameSpacing.iVal = (_fFrameBorder ? iPixelValueFrameSpacing-4
                                                   : iPixelValueFrameSpacing);
                break;
            }
            put_VariantHelper(iFrameSpacing, (PROPERTYDESC *)&s_propdescCFrameSetSiteframeSpacing);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CFrameSetSite::IsOverflowFrame
//
//  Synopsis : first we need to verify that our frameset is not an overflow of
//    a frameset above us.
//----------------------------------------------------------------------------

#ifdef _M_IA64
//$ WIN64: Why is there unreachable code in the retail build of this next function for IA64?
#pragma warning(disable:4702) /* unreachable code */
#endif

BOOL
CFrameSetSite::IsOverflowFrame(CElement *pel)
{

    if (CElement::IsOverflowFrame())
        return TRUE;
    else
    {
        int                 cexp = 0;
        int                 i = 0;
        CTreeNode *         pNode = NULL;
        static ELEMENT_TAG  atagStop = ETAG_FRAMESET;
        static ELEMENT_TAG  atagChild[2] = { ETAG_FRAMESET, ETAG_FRAME };
        CChildIterator  iter(
            this,
            NULL,
            CHILDITERATOR_USETAGS,
            &atagStop, 1,
            atagChild, ARRAY_SIZE(atagChild));

        cexp = ExpectedFrames();
    
        for (i = 0, pNode = iter.NextChild(); 
             (pNode && i < cexp); 
             pNode = iter.NextChild(), i++)
        {
            // seen request frame as expected
            if (pNode->Element() == pel)
                return FALSE;
        }

        // seen all expected frames already
        if (i >= cexp)
            return TRUE;

        // no more frames
        if (!pNode)
        {
            Assert(0);
        }
    }

    return FALSE;
}

#ifdef _M_IA64
#pragma warning(default:4702) /* unreachable code */
#endif

//+------------------------------------------------------------------
//
//  Member:     CFrameSetSite::GetFrameFlat
//
//  Synopsis:   gets frame # nIndex, where indexing goes flat through
//              nested framesets.
//              Returns:
//
//              nIndex          index of frame to get
//              pcFrames        number of frames passed by (including all frames
//                              in nested framesets)
//              ppFrame         frame with index nIndex if found
//
//              return value    TRUE            if frame with index nIndex found
//                              FALSE           otherwise
//
//-------------------------------------------------------------------

BOOL
CFrameSetSite::GetFrameFlat (LONG nIndex, LONG * pcFrames, CFrameElement ** ppFrame)
{
    CLayout         * pLayout;
    CFrameSetLayout * pLayoutThis = Layout();
    LONG              cFrames, cFramesSub;
    DWORD_PTR         dw;
    BOOL              fFound = FALSE;

    if (ppFrame)
        *ppFrame = NULL;

    cFrames = 0;

    for(pLayout = pLayoutThis->GetFirstLayout(&dw, FALSE);
        pLayout;
        pLayout = pLayoutThis->GetNextLayout(&dw, FALSE))
    {
        if (ETAG_FRAME == pLayout->Tag())
        {
            cFrames++;
            if (cFrames == nIndex+1)
            {
                fFound = TRUE;
                if (ppFrame)
                    *ppFrame = DYNCAST(CFrameElement, pLayout->ElementOwner());
                break;
            }
        }
        else if (ETAG_FRAMESET == pLayout->Tag())
        {
            fFound = DYNCAST(CFrameSetSite, pLayout->ElementOwner())->GetFrameFlat(nIndex - cFrames, &cFramesSub, ppFrame);
            cFrames += cFramesSub;
            if (fFound)
                break;
        }
    }

    pLayoutThis->ClearLayoutIterator(dw, FALSE);
    if (pcFrames)
        *pcFrames = cFrames;

    return fFound;
}

//+--------------------------------------------------------------
//
//  Member:     CFrameSetSite::GetFramesCount
//
//  Synopsis:   returns number of frames in this frameset,
//              including all nested frames
//
//---------------------------------------------------------------

HRESULT
CFrameSetSite::GetFramesCount (LONG * pcFrames)
{
    if (!pcFrames)
        RRETURN (E_POINTER);

    GetFrameFlat(-1, pcFrames, NULL);

    return S_OK;
}


//+--------------------------------------------------------------
//
//  Member:     CFrameSetSite::GetOmWindow
//
//  Synopsis:   returns IDispatch of om window of doc
//              in frame # nFrame
//
//---------------------------------------------------------------

HRESULT
CFrameSetSite::GetOmWindow (LONG nFrame, IHTMLWindow2 ** ppOmWindow)
{
    HRESULT             hr = S_OK;
    BOOL                fFound;
    CFrameElement *     pFrame;

    if (!ppOmWindow)
        RRETURN (E_POINTER);

    *ppOmWindow = NULL;

    fFound = GetFrameFlat(nFrame, NULL, &pFrame);
    if (!fFound)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert (pFrame);

    hr = THR(pFrame->GetOmWindow(ppOmWindow));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CFrameSetSite::BorderColorAttribute
//
//  Synopsis:   walk up the tree looking for a bordercolor attribute
//              if one isn't found, return NULL
//
//-------------------------------------------------------------------------
CColorValue
CFrameSetSite::BorderColorAttribute()
{
    CColorValue ccvBorderColor = GetAAborderColor();

    if(!ccvBorderColor.IsNull())
        return ccvBorderColor;
    else
    {
        CLayout *pLayoutParent = GetCurParentLayout();
        if(pLayoutParent && pLayoutParent->Tag() == ETAG_FRAMESET)
            return DYNCAST(CFrameSetSite, pLayoutParent->ElementOwner())->BorderColorAttribute();
    }

    return ccvBorderColor;
}


//+------------------------------------------------------------------------
//
//  Function:   CFrameSetSite::FrameBorderAttribute
//
//  Synopsis:   walk up the tree looking for a frameborder attribute
//              if one isn't found, return NULL
//
//-------------------------------------------------------------------------
void
CFrameSetSite::FrameBorderAttribute(BOOL fFrameBorder, BOOL fDefined)
{
    LPCTSTR pStrFrameBorder     = GetAAframeBorder();
    BOOL    fFrameBorderDefined = (pStrFrameBorder) ? (TRUE) : (fDefined);

    _fFrameBorder = (pStrFrameBorder)
                  ? (   pStrFrameBorder[0] == _T('y')
                     || pStrFrameBorder[0] == _T('Y')
                     || pStrFrameBorder[0] == _T('1'))
                  : (fFrameBorder);

    CFrameSetLayout * pLayoutThis = Layout();
    DWORD_PTR  dw;
    CLayout  * pLayout = pLayoutThis->GetFirstLayout(&dw, FALSE);
    BOOL       fFirst  = TRUE;
    BOOL       fFrameBorderThis = _fFrameBorder;
    BOOL       fFrameBorderChild;

    while (pLayout)
    {
        ELEMENT_TAG eTag = (ELEMENT_TAG) pLayout->ElementOwner()->_etag;

        switch (eTag)
        {
        case ETAG_FRAME:
            {
                CFrameSite * pFrame = DYNCAST(CFrameSite, pLayout->ElementOwner());
                pStrFrameBorder = pFrame->GetAAframeBorder();
                pFrame->_fFrameBorder = (pStrFrameBorder)
                                      ? (   pStrFrameBorder[0] == _T('y')
                                         || pStrFrameBorder[0] == _T('Y')
                                         || pStrFrameBorder[0] == _T('1'))
                                      : (_fFrameBorder);
                fFrameBorderChild = pFrame->_fFrameBorder;
            }
            break;

        case ETAG_FRAMESET:
            {
                CFrameSetSite * pFrameSet = DYNCAST(CFrameSetSite,
                                                    pLayout->ElementOwner());
                pFrameSet->FrameBorderAttribute(_fFrameBorder,
                                                fFrameBorderDefined);
                fFrameBorderChild = pFrameSet->_fFrameBorder;
            }
            break;

        default:
            fFrameBorderChild = fFrameBorderThis;
            break;
        }

        if (fFirst)
        {
            fFrameBorderThis = fFrameBorderChild;
            fFirst           = FALSE;
        }
        else if (fFrameBorderThis != fFrameBorderChild)
        {
            fFrameBorderThis = TRUE;
        }

        pLayout = pLayoutThis->GetNextLayout(&dw, FALSE);
    }

    pLayoutThis->ClearLayoutIterator(dw, FALSE);

    if (!fFrameBorderDefined)
    {
        _fFrameBorder = fFrameBorderThis;
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CFrameSetSite::BorderAttribute
//
//  Synopsis:   walk up the tree looking for a border attribute
//              if one isn't found, return NULL
//
//-------------------------------------------------------------------------

CUnitValue
CFrameSetSite::BorderAttribute()
{
    CUnitValue uv = GetAAborder();
    if (!uv.IsNull())
        return uv;
    else
    {
        CLayout *pLayoutParent = GetCurParentLayout();
        if(pLayoutParent && pLayoutParent->Tag() == ETAG_FRAMESET)
            return DYNCAST(CFrameSetSite, pLayoutParent->ElementOwner())->BorderAttribute();
    }
    return uv;
}

//+------------------------------------------------------------------------
//
// Member:      CFrameSetSite::ApplyDefaultFormat, CSite
//
//-------------------------------------------------------------------------
HRESULT
CFrameSetSite::ApplyDefaultFormat(CFormatInfo * pCFI)
{
    CDoc *  pDoc = Doc();

    if ((pDoc == pDoc->GetRootDoc()) &&
            (pDoc->GetPrimaryElementClient() == this) &&
            ((pDoc->_dwFrameOptions & FRAMEOPTIONS_NO3DBORDER) == 0) &&
            ((pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_NO3DBORDER) == 0))
    {
        // set 3D border color attributes for top-level frameset
        //
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._ccvBorderColorLight.SetValue(
                GetSysColorQuick(COLOR_3DLIGHT), FALSE);
        pCFI->_ff()._ccvBorderColorShadow.SetValue(
                GetSysColorQuick(COLOR_BTNSHADOW), FALSE);
        pCFI->_ff()._ccvBorderColorHilight.SetValue(
                GetSysColorQuick(COLOR_BTNHIGHLIGHT), FALSE);
        pCFI->_ff()._ccvBorderColorDark.SetValue(
                GetSysColorQuick(COLOR_3DDKSHADOW), FALSE);
        pCFI->UnprepareForDebug();
    }
    return super::ApplyDefaultFormat(pCFI);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSetSite::GetBorderInfo
//
//  Synopsis:   Generate the border information for a framesite
//
//-------------------------------------------------------------------------

DWORD
CFrameSetSite::GetBorderInfo(
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    BOOL            fAll)
{
    CDoc *  pDoc = Doc();
    if ((pDoc == pDoc->GetRootDoc()) &&
            (pDoc->GetPrimaryElementClient() == this) &&
            ((pDoc->_dwFrameOptions & FRAMEOPTIONS_NO3DBORDER) == 0) &&
            ((pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_NO3DBORDER) == 0))
    {
        // set border style and border space for top-level frameset
        //
        pborderinfo->abStyles[BORDER_TOP]
                = pborderinfo->abStyles[BORDER_RIGHT]
                = pborderinfo->abStyles[BORDER_BOTTOM]
                = pborderinfo->abStyles[BORDER_LEFT]
                = fmBorderStyleSunken;
        pborderinfo->aiWidths[BORDER_TOP]
                = pborderinfo->aiWidths[BORDER_RIGHT]
                = pborderinfo->aiWidths[BORDER_BOTTOM]
                = pborderinfo->aiWidths[BORDER_LEFT]
                = 2;
        pborderinfo->wEdges = BF_RECT;
    }

    return super::GetBorderInfo(pdci, pborderinfo, fAll);
}


int CFrameSetSite::iPixelFrameHighlightWidth = 0;
int CFrameSetSite::iPixelFrameHighlightBuffer = 0;

//+---------------------------------------------------------------------------
//
//  Member:     CNoFramesElement::Save, public
//
//  Synopsis:   Saves all our embedded sites.
//
//  Arguments:  [pStreamWrBuff] -- Stream to write to
//              [fEnd]          -- TRUE if we're saving the end tag
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CNoFramesElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr;

    // No frameset for plaintext mode
    if (pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
        return S_OK;

    hr = super::Save(pStreamWrBuff, fEnd);
    if (hr)
        goto Cleanup;

    if (!fEnd)
    {
        DWORD dwOldFlags;

        hr = THR(pStreamWrBuff->NewLine());
        if (hr)
            goto Cleanup;
        hr = THR(pStreamWrBuff->NewLine());
        if (hr)
            goto Cleanup;

        dwOldFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);
        pStreamWrBuff->BeginPre();

        hr = THR(pStreamWrBuff->Write(_cstrNoFrames));
        if (hr)
            goto Cleanup;

        pStreamWrBuff->EndPre();
        pStreamWrBuff->RestoreFlags(dwOldFlags);

        hr = THR(pStreamWrBuff->NewLine());
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// Member: OnPropertyChange
//
//-----------------------------------------------------------------------------
HRESULT
CFrameSetSite::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr;

    hr = THR(super::OnPropertyChange(dispid, dwFlags));
    if (hr)
        goto Cleanup;

    if (dispid == DISPID_CFrameSetSite_rows || dispid == DISPID_CFrameSetSite_cols)
    {
        Layout()->_aryCols.DeleteAll();
        Layout()->_aryRows.DeleteAll();

        Layout()->SetRowsCols();
        Layout()->CancelManualResize(FALSE);
        Layout()->CancelManualResize(TRUE);
        ResizeElement();
    }

    if (dispid == DISPID_CFrameSetSite_frameBorder)
    {
        Doc()->_fFrameBorderCacheValid = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// Member: WaitForRecalc
//
//-----------------------------------------------------------------------------

void
CFrameSetSite::WaitForRecalc()
{
    CFrameSetLayout * pLayoutThis = Layout();
    CLayout *         pLayout;
    CFrameElement *   pFrameElement;
    CDoc *            pDoc;
    DWORD_PTR         dw;

    for (pLayout = pLayoutThis->GetFirstLayout(&dw, FALSE);
         pLayout;
         pLayout = pLayoutThis->GetNextLayout(&dw, FALSE))
    {
        if (ETAG_FRAME == pLayout->Tag())
        {
            pFrameElement = DYNCAST(CFrameElement, pLayout->ElementOwner());

            if (pFrameElement->GetCDoc(&pDoc) == S_OK)
            {
                pDoc->WaitForRecalc();
            }
        }
        else if (ETAG_FRAMESET == pLayout->Tag())
        {
            DYNCAST(CFrameSetSite, pLayout->ElementOwner())->WaitForRecalc();
        }
    }

    pLayoutThis->ClearLayoutIterator(dw, FALSE);
}
