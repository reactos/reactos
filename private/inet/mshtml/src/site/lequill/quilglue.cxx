//+----------------------------------------------------------------------------
//
// File:        quilglue.CXX
//
// Contents:    Implementation of CQuillGlue and related classes
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
// @doc INTERNAL
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_QDOCGLUE_HXX_
#define X_QDOCGLUE_HXX_
#include "qdocglue.hxx"
#endif

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "quilglue.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_QPROPS_HXX_
#define X_QPROPS_HXX_
#include "qprops.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_MARQUEE_H_
#define X_MARQUEE_H_
#include "marquee.hxx"
#endif

#ifndef X_TPOINTER_H_
#define X_TPOINTER_H_
#include "..\text\tpointer.hxx"
#endif

MtDefine(CQuillGlue, Quill, "CQuillGlue")


//+------------------------------------------------------------------------
//
//  Member:     CQuillGlue::QueryInterface, IUnknown
//
//  Synopsis:   QI.
//
//-------------------------------------------------------------------------
HRESULT
CQuillGlue::QueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if(riid == IID_IUnknown || riid == IID_ITextLayoutSite)
    {
        *ppv = this;
    }
    else if (riid == IID_ITextStory)
    {
        *ppv = (ITextStory *)this;
    }
    else if (riid == IID_ITextInPlacePaint)
    {
        *ppv = (ITextInPlacePaint *)this;
    }
    else if (riid == IID_ILineServicesHost)
    {
        *ppv = (ILineServicesHost *)this;
    }

    if(*ppv == NULL)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        ((LPUNKNOWN)* ppv)->AddRef();
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Helper functions.
//
//-------------------------------------------------------------------------

/*----------------------------------------------------------------------------
@mfunc CLayout * | GetOwnerLayoutAtCp | Quill support.
@contact sidda

@comm	Identifies the layout object responsible for formatting the specified
        Quill cp.
----------------------------------------------------------------------------*/
CLayout *
CQuillGlue::GetOwnerLayoutAtCp(
    long cpQuill)   // @parm Quill CP.
{
    long        cp = CpAbsFromCpRel(cpQuill);
    LONG        ich;
    CTreePos *  ptp = m_pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);

    // ASSUME that the query cp lies either in the body text, or at
    // the Begin Edge node position of a nested layout object, but NOT
    // within the text range of a nested layout object
    if (ptp->IsBeginElementScope())
    {
        CTreeNode *pNode = ptp->Branch();
        if (pNode->HasLayout())
            return pNode->GetLayout();
    }

    return m_pFlowLayout;
}

/*----------------------------------------------------------------------------
@mfunc long | CpAbsFromCpRel | Quill support.
@contact sidda

@comm	Converts from a Quill-specified CP to an absolute CP. This is
        necessary since each Quill story is zero-based, for example Quill
        starts the text in a table cell starts at cp 0.
----------------------------------------------------------------------------*/
long
CQuillGlue::CpAbsFromCpRel(
    long cpQuill)   // @parm Quill CP.
{
	long cpFirst = m_pFlowLayout->GetContentFirstCp();
    Assert(cpQuill + cpFirst >= 0);
    return cpQuill + cpFirst;
}

/*----------------------------------------------------------------------------
@mfunc long | CpRelFromCpAbs | Quill support.
@contact sidda

@comm	Converts from a Trident CP to a Quill CP. The resulting CP is valid
        for the Quill story attached to this layout.
----------------------------------------------------------------------------*/
long
CQuillGlue::CpRelFromCpAbs(
    long cpTrident) // @parm Trident CP.
{
	long cpFirst = m_pFlowLayout->GetContentFirstCp();
    Assert(cpTrident - cpFirst >= 0);
    return cpTrident - cpFirst;
}

/*----------------------------------------------------------------------------
@mfunc void | GetViewTopLeft | Quill support.
@contact sidda

@comm	Retrieves the top, left of the client rectangle for this layout.
----------------------------------------------------------------------------*/
void
CQuillGlue::GetViewTopLeft(
    long *pLeft,    // @parm Left edge in client coordinates returned here.
    long *pTop)     // @parm Top edge in client coordinates returned here.
{
    RECT        rcView;
    CCalcInfo   CI;

    CI.Init(m_pFlowLayout);
    m_pFlowLayout->GetClientRect((CRect *)&rcView, 0, &CI);

    *pLeft = rcView.left;
    *pTop = rcView.top;
}

/*----------------------------------------------------------------------------
@mfunc CElement * | GetFlowLayoutElement | Quill support.
@contact sidda

@comm	Return the associated HTML element.
----------------------------------------------------------------------------*/
CElement *
CQuillGlue::GetFlowLayoutElement() const
{
    return m_pFlowLayout->ElementOwner();
}

//+------------------------------------------------------------------------
//
//  Internal API functions (Trident talks to these).
//
//-------------------------------------------------------------------------

HRESULT
CQuillGlue::Init(CFlowLayout *pfl)
{
    HRESULT hr = S_OK;
    ITextLayoutGroup * plg;

    Assert(pfl);
    m_pFlowLayout = pfl;

    if (!m_pFlowLayout->FExternalLayout())
        return S_OK;

    plg = m_pFlowLayout->Doc()->GetLayoutGroup()->GetTextLayoutGroup();
	if (!plg)
		return S_FALSE;

    hr = plg->CreateTextLayoutElement((ITextStory *)this, &m_ptle);
    if (m_ptle)
        {
        // initialize view
        m_ptle->SetTextLayoutSite(this, 0);
        }

    return hr;
}

void
CQuillGlue::Detach()
{
    if (m_ptle)
        {
        m_ptle->SetTextLayoutSite(NULL, 0);
        m_ptle->Remove();
        }

    ClearInterface(&m_ptle);
}


void
CQuillGlue::Draw(CFormDrawInfo *pDI, RECT *prcView)
{
#ifndef DISPLAY_TREE
    CPeerHolder * pPeerHolder = m_pFlowLayout->ElementOwner()->GetRenderPeerHolder();

    if (pPeerHolder && pPeerHolder->TestRenderFlag(BEHAVIORRENDERINFO_BEFORECONTENT))
        IGNORE_HR(pPeerHolder->Draw(pDI, m_pFlowLayout, BEHAVIORRENDERINFO_BEFORECONTENT));

    if (!(pPeerHolder && pPeerHolder->TestRenderFlag(BEHAVIORRENDERINFO_DISABLECONTENT))
        && m_ptle)
        {
        // DRAW (clipped to clip rect)
        RECT rcDraw;
        if (IntersectRect(&rcDraw, prcView, pDI->ClipRect()))
            {
            // save pDI for nested layouts
            // $REVIEW alexmog: may not need this with display tree
            Assert(_pdi == NULL);
            _pdi = pDI;

            m_ptle->Update(pDI->_hdc, &rcDraw, 0, pDI);

            // reset local pdi, it won't be valid any more
            _pdi = NULL;
            }
        }

    if (pPeerHolder && pPeerHolder->TestRenderFlag(BEHAVIORRENDERINFO_AFTERCONTENT))
        IGNORE_HR(pPeerHolder->Draw(pDI, m_pFlowLayout, BEHAVIORRENDERINFO_AFTERCONTENT));
#endif
}

void
CQuillGlue::DrawRelativeElement(CElement * pElement, CFormDrawInfo * pDI)
{
#if 0 // $REVIEW sidda: does not work yet

    CSiteDrawList        sdl;
    int                  cSize;
    CElement        **   ppElement;

    // The given element better be relative.
    Assert(pElement->IsRelative());

    pElement->GetFirstBranch()->RenderParent()->
        GetLayout()->GetSiteDrawList(pDI,
                                      pDI->ClipRect(),
                                      NULL,
                                      FALSE,
                                      pElement,
                                      &sdl);

    // Draw any positioned elements contained inside us that have
    // a negative z-index.

    for (cSize = sdl.Size(), ppElement = sdl;
         cSize > 0;
         ++ppElement, cSize--)
    {
        if ((*ppElement)->GetFirstBranch()->GetCascadedzIndex() < 0)
        {
            (*ppElement)->DrawFilteredElement(pDI);
        }
        else
            break;
    }

    if (pElement->HasLayout())
        pElement->GetLayout()->Draw(pDI);

    // Now draw any positioned elements contained inside us that have
    // a positive z-index.

    for (cSize = sdl.Size(), ppElement = sdl;
         cSize > 0;
         ++ppElement, cSize--)
    {
        if ((*ppElement)->GetFirstBranch()->GetCascadedzIndex() >= 0)
        {
            (*ppElement)->DrawFilteredElement(pDI);
        }
    }
#endif // REVIEW
}

LONG
CQuillGlue::Render(
    CFormDrawInfo * pDI,
    const RECT &rcView,
    const RECT &rcRender,
    LONG iliStop)
{
    if (m_ptle)
    {
        RECT rc1 = rcView;
        RECT rc2 = rcRender;

        // WARNING: this code must be kept in sync with CQuillGlue::GetDisplayRect()
        rc1.left = 0;
        rc1.top = 0;

        Assert(!m_pFlowLayout->IsDisplayNone());

        // save pDI for drawing borders and backgrounds
        Assert(_pdi == NULL);
        _pdi = pDI;

        m_ptle->Update(pDI->_hdc, &rc1, &rc2, 0);

        // reset local pdi, it won't be valid any more
        _pdi = NULL;
    }
    return 0;
}

HRESULT
CQuillGlue::ExecCommand(long qcmd)
{
    return m_ptle ? m_ptle->ExecCommand(qcmd) : S_FALSE;
}

void
CQuillGlue::NukeLayout()
{
    if (m_ptle)
        m_ptle->NotifyLayoutChanged();
}

HRESULT
CQuillGlue::HandleMessage(CMessage * pMessage, CElement * pElem, CTreeNode *pNodeContext)
{	
	POINT pt;
	long qclick = 0;
	HRESULT hr = S_FALSE;
	BOOL fCapture;
	CDoc * pDoc = m_pFlowLayout->Doc();
    HDC hdcRef = NULL;
    HWND hwndRef = NULL;

	Assert(pDoc);
	Assert(pDoc->_pInPlace->_hwnd);

    if (!m_pFlowLayout->FExternalLayout())
        {
        return S_FALSE;
        }

    Assert(m_ptle);

    if (m_pFlowLayout->Doc() && m_pFlowLayout->Doc()->InPlace())
        hwndRef = m_pFlowLayout->Doc()->InPlace()->_hwnd;

	if ((pMessage->message >= WM_MOUSEFIRST && pMessage->message <= WM_MOUSELAST))
	{
		pt.x = (WORD)pMessage->lParam;
		pt.y = ((WORD) (((DWORD) pMessage->lParam >> 16) & 0xFFFF));
		qclick = (pMessage->wParam & MK_SHIFT) ? qclickExtend :
					(pMessage->message == WM_LBUTTONDBLCLK ? qclickDouble : qclickSingle);

        hdcRef = GetDC(hwndRef);
	}

	switch (pMessage->message)
	{
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
			hr = m_ptle->MouseDown(hwndRef, hdcRef, pt, qclick);
			if (hr == S_OK)
				m_pFlowLayout->ElementOwner()->TakeCapture(TRUE);
			break;
		case WM_LBUTTONUP:
			hr = m_ptle->MouseUp(hwndRef, hdcRef, pt);
	
            fCapture =  pDoc->GetCaptureObject() == (void *) m_pFlowLayout->ElementOwner()
                            && ::GetCapture() == pDoc->_pInPlace->_hwnd;
			
			if (fCapture)
			{
				hr = THR(m_pFlowLayout->ElementOwner()->BecomeUIActive());
				if (hr)
					goto LExit;
			}

			m_pFlowLayout->ElementOwner()->TakeCapture(FALSE);
				
			break;
#if 1 /* SASHAT */
		case WM_MOUSEMOVE:

            fCapture =  pDoc->GetCaptureObject() == (void *) m_pFlowLayout->ElementOwner()
                            && ::GetCapture() == pDoc->_pInPlace->_hwnd;

			if (fCapture)
			{
				hr = m_ptle->MouseMove(hwndRef, hdcRef, pt);
			}
			break;
#endif // 0
        default:
            return m_ptle->HandleMessage(pMessage->message, pMessage->wParam, pMessage->lParam, NULL);
		break;
	}

LExit:
	if (hdcRef)
		ReleaseDC(hwndRef, hdcRef);

	return hr;
}

LONG
CQuillGlue::CpFromPoint(
    POINT pt,                     // Point to compute cp at (site coords)
    CLinePtr * const prp,         // Returns line pointer at cp (may be NULL)
    CTreePos ** pptp,             // pointer to return TreePos corresponding to the cp
    CLayout ** ppLayout,          // can be NULL
    BOOL fAllowEOL,               // Click at EOL returns cp after CRLF
    BOOL fIgnoreBeforeAfterSpace, // TRUE if ignoring pbefore/after space
    BOOL fExactFit,               // TRUE if cp should always round down (for element-hit-testing)
    BOOL * pfRightOfCp,
    BOOL * pfPsuedoHit,
    CCalcInfo * pci, 
    BOOL *pfEOL)
{
    // code stolen from CDisplay::CpFromPoint() cousin

    LONG    cpRet = -1;
    long	fEOL = 0;

    Assert(!m_pFlowLayout->IsDisplayNone());

    // find the CP that was hit
    if (S_OK == m_ptle->CpFromPoint(&pt, 0 /* dwFlags */, &cpRet, &fEOL))
    {
        cpRet = CpAbsFromCpRel(cpRet);

        if (pptp)
        {
            LONG ich;
            *pptp = m_pFlowLayout->GetContentMarkup()->TreePosAtCp(cpRet, &ich);
        }
    }
#if DBG==1
    else
    {
        int x = 5;  // place to put a breakpoint
    }
#endif

	if (pfEOL)
		*pfEOL = fEOL;

    return cpRet;
}

void
CQuillGlue::RegionFromElement(
    CElement * pElement,
    CDataAry<RECT> * paryRects, // @parm Array of rectangles returned here.
    LONG cpStart,
    LONG cpFinish,
    LONG cpClipStart,
    LONG cpClipFinish,
    BOOL fRelativeToView,       // @parm If TRUE, return info relative to view.
                                // If FALSE, return info relative to display, that is,
                                // client coordinates.
    BOOL fRestrictToVisible,    // @parm TODO: exclude stuff that's not visible on-screen.
    BOOL fBlockElement,
    RECT * prcBoundingRect)     // @parm If non-NULL, return bounding rect here.
{
    LONG    xOffset = 0;
    LONG    yOffset = 0;

    if (m_ptle)
        {
        if (fRelativeToView)
            GetViewTopLeft(&xOffset, &yOffset);

        cpStart = CpRelFromCpAbs(cpStart);
        cpFinish = CpRelFromCpAbs(cpFinish);
        cpClipStart = CpRelFromCpAbs(cpClipStart);
        cpClipFinish = CpRelFromCpAbs(cpClipFinish);
        m_ptle->RegionFromRange(cpStart, cpFinish, cpClipStart, cpClipFinish, paryRects,
                                fRestrictToVisible, fBlockElement, xOffset, yOffset, prcBoundingRect);
        }

    return;
}

LONG
CQuillGlue::GetMaxXScrollFromDisplay()
{
    return max(0L, (GetLongestLineWidth() - m_pFlowLayout->GetDisplay()->GetViewWidth()));
}

LONG
CQuillGlue::GetMaxYScrollFromDisplay()
{
    return max(0L, (GetNATURALBestHeight() + m_pFlowLayout->GetDisplay()->_yBottomMargin - m_pFlowLayout->GetDisplay()->GetViewHeight()));
}

#ifndef DISPLAY_TREE
HRESULT
CQuillGlue::PositionAndLayoutObjectsForScroll(CPositionInfo * ppi, LONG dx, LONG dy)
{
    // refresh PGR position after a scroll
    m_ptle->UpdateDisplayPosition();
    return S_OK;
}
#endif

HRESULT
CQuillGlue::GetLineInfoFromPoint(LONG xPos, LONG yPos, BOOL fSkipFrame, LONG xExposedWidth,
                        BOOL fIgnoreRelLines, LONG *pyLine, LONG *pcpFirst)
{
    // patterned after CDisplay::LineFromPos() cousin
    HRESULT hrRet;

    hrRet = m_ptle->GetLineInfoFromPoint(xPos, yPos, fSkipFrame, xExposedWidth, fIgnoreRelLines, pyLine, pcpFirst);
    if (pcpFirst)
        *pcpFirst = CpAbsFromCpRel(*pcpFirst);

    return hrRet;
}

HRESULT 
CQuillGlue::GetLineInfo(CTreePos *pTP, CFlowLayout *pFlowLayout, BOOL fAtEndOfLine, HTMLPtrDispInfoRec *pInfo)
{
    HRESULT		hr = S_OK;
    LONG		cp;

	Assert(pTP != NULL && pFlowLayout != NULL && pInfo != NULL);
    
	cp = pTP->GetCp();
	cp = CpRelFromCpAbs(cp);
	{
	    LONG lBaseline;
	    LONG lXPosition;
	    LONG lLineHeight;
	    LONG lTextHeight;
	    LONG lDescent;
		long cpMac;
		GetLength(&cpMac);
		BOOL fEnd = (cp >= cpMac);
		hr = m_ptle->GetLineInfo(cp, fEnd, &lBaseline, &lXPosition, 
									&lLineHeight, &lTextHeight, &lDescent);

		if (hr != S_OK)
			goto Cleanup;
			
		pInfo->lBaseline 	= lBaseline;
		pInfo->lXPosition	= lXPosition;
		pInfo->lTextHeight	= lTextHeight;
		pInfo->lLineHeight	= lLineHeight;
		pInfo->lDescent		= lDescent;
	}
Cleanup:
    RRETURN(hr);    
} 

LONG
CQuillGlue::GetMaxCpCalced()
{
    LONG cpMaxQuill;

    if (S_OK == m_ptle->GetMaxCpCalced(&cpMaxQuill))
    {
        return CpAbsFromCpRel(cpMaxQuill);
    }

    return m_pFlowLayout->GetContentFirstCp();
}

HRESULT 
CQuillGlue::MoveMarkupPointer(
				CMarkupPointer *pMarkupPointer, 
				LAYOUT_MOVE_UNIT eUnit, 
				LONG lXCurReally, 
				BOOL fEOL)
{
	CTreeNode	*pNode;
	CMarkup		*pMarkup;
	POINT	pt;
	LONG	cpCur, cpNew;
	HRESULT	hr = S_OK;

	//	we don't care if pointer should be adjusted relatively view (not text)
	Assert(eUnit != LAYOUT_MOVE_UNIT_TopOfWindow && eUnit != LAYOUT_MOVE_UNIT_BottomOfWindow);

    // get cp for current position
    cpCur = pMarkupPointer->TreePos()->GetCp();
	cpCur = CpRelFromCpAbs(cpCur);

	pt.x = lXCurReally;
	pt.y = 0;
	
	hr = m_ptle->CpMoveFromCp(cpCur, fEOL, pt, (long)eUnit, &cpNew, &fEOL);
	if (hr != S_OK)
	{
		Assert(("ITextLayoutElement::CpMoveFromCp failed.", 0));
		goto Cleanup;
	}

    // get markup
    pNode = pMarkupPointer->CurrentScope();
    if (!pNode)
    {
		Assert(("IMarkupPointer::CurrentScope failed.", 0));
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

	pMarkup = pNode->GetMarkup();
	if (!pMarkup)
	{
		Assert(("CTreeNode::GetMarkup failed", 0));
        hr = E_UNEXPECTED;
        goto Cleanup;
	}

	cpNew = CpAbsFromCpRel(cpNew);

    if (cpNew < 1 || cpNew > pMarkup->Cch())
        goto Cleanup;

    hr = pMarkupPointer->MoveToCp(cpNew, pMarkup, !fEOL);

Cleanup:
	return hr;
}

VOID CQuillGlue::ShowSelected(CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected)
{
	LONG	cpStart, cpEnd;

	Assert(ptpStart != NULL && ptpEnd != NULL);
	cpStart	= ptpStart->GetCp();
	cpEnd	= ptpEnd->GetCp();

	{
		//	$REVIEW olego 
		//	Actually call to CpRelFromCpAbs() should be here but because of 
		//	Trident's selecting specific behaviour I need to do extra checking...
		long cpFirst = m_pFlowLayout->GetContentFirstCp();

		cpStart -= cpFirst;
		if (cpStart < 0)
			cpStart = 0;

		cpEnd -= cpFirst;
		if (cpEnd < 0)
			cpEnd = 0;
	}

	m_ptle->NotifySelectionChanged(cpStart, cpEnd, fSelected);
}

#ifndef DISPLAY_TREE
DWORD
CQuillGlue::SetPosition(CPositionInfo * ppi, RECT * prcPosition)
{
    // save ppi for nested calls
    Assert(_ppi == NULL);
    _ppi = ppi;

#ifdef DBG
    ELEMENT_TAG etag = m_pFlowLayout->GetDisplay()->GetFlowLayoutElement()->Tag();
#endif

    // set position on layout
#ifndef DISPLAY_TREE
    if (!m_pFlowLayout->_fParked && m_pFlowLayout->_fPositioned && m_ptle)
#else
    if (m_ptle)
#endif
        {
        m_ptle->UpdateDisplayPosition();
        }

    // reset local ppi, int won't be valid any more
    _ppi = NULL;
    return 0;
}
#endif

BOOL
CQuillGlue::RecalcView(CCalcInfo * pci, BOOL fFullRecalc)
{
    CSaveCalcInfo       sci(pci);   // save & restore the pci a la CDisplay::RecalcLines()

    // allow ourselves request layout again after this is done.
    // REVIEW alexmog: why isn't there a standard mechanizm to handle this?
    // REVIEW alexmog: layout request doesn't always result in this call (e.g. hidden layout)
    //                 so this flag can get stuck. See if it becomes a problem.
    _fLayoutRequested = FALSE;

    Assert(!m_pFlowLayout->IsDisplayNone());

    // save pci for nested calls
    Assert(_pci == NULL);
    _pci = pci;

    if (pci->_smMode == SIZEMODE_SET)
    {
        pci->_smMode = SIZEMODE_NATURAL;
    }

    // code imitated from CDisplay::RecalcView() cousin

    switch (pci->_smMode)
        {
        case SIZEMODE_FULLSIZE:
        case SIZEMODE_NATURAL:
            {
                long yBestHeightOld = m_yBestHeight;
				int  fCalcExactWidth = !m_pFlowLayout->GetDisplay()->GetWordWrap();

                m_ptle->GetLayoutHeight(0 /*nUnit*/, fFullRecalc, FALSE /*fLazy*/, 
					&m_xWidthLongestLine, &m_yBestHeight, &m_yBaseline, fCalcExactWidth, &m_fNoContent);

                // Story layout size in CDisplay 
                //      (because in many places, CDisplay is asked for layout size.
                //      In the future, all such places should redirect size requests 
                //      to external layout as needed)
                // Originally added for marquee:
				//	    CMarqueeLayout::CalcSize ignores results of CFlowLayout::CalcSize 
				//	    and calls CDisplay::GetMaxHeight to retreive ySize of marquee 
                //      
				{
					CDisplay	*pDp = m_pFlowLayout->GetDisplay();
				
 					pDp->_yHeightMax = pDp->_yHeight = m_yBestHeight;
					pDp->_xWidth = m_xWidthLongestLine;
				}

                // Resize display node and stuff
                //      code pasted from CDisplay::RecalcLinesWithMeasurer()
                if (m_yBestHeight != yBestHeightOld)
                {
                    m_pFlowLayout->SizeContentDispNode(CSize(m_xWidthLongestLine, m_yBestHeight));

                    // If our contents affects our size, ask our parent to initiate a re-size
                    if (    m_pFlowLayout->GetAutoSize()
                        ||  m_pFlowLayout->_fContentsAffectSize)
                    {
                        m_pFlowLayout->ElementOwner()->ResizeElement();
                    }
                }

#if 0 
                // $REVIEW alexmog: CDisplay has this for relative lines. Do we care?
                if (m_pFlowLayout->_fContainsRelative)
                    UpdateRelDispNodeCache(NULL);
#endif

                m_pFlowLayout->GetDisplay()->AdjustDispNodes(_pDispNodePrev, NULL);

                m_pFlowLayout->NotifyMeasuredRange(GetFlowLayoutElement()->GetFirstCp(), GetFlowLayoutElement()->GetLastCp());

                break;
            }

        case SIZEMODE_MMWIDTH:
        case SIZEMODE_MINWIDTH:
            {
                m_ptle->GetMinMaxWidth(0 /*nUnit*/, &m_xMinWidth, &m_xMaxWidth, &m_yBestHeight, &m_yBaseline, &m_fNoContent);
                m_xWidthLongestLine = m_xMaxWidth;

                // cache min/max only if there are no sites inside !
                if (!m_pFlowLayout->ContainsChildLayout())
                {
                    m_fMinMaxValid = TRUE;
                }

                // REVIEW sidda: temporary hack for buttons till we support background layout
                // Currently CInputSlaveLayout::CalcSizeHelper uses SIZEMODE_MMWIDTH
                if (m_pFlowLayout->ElementOwner()->Tag() == ETAG_INPUT &&
                    (DYNCAST(CInput, m_pFlowLayout->ElementOwner())->GetType() == htmlInputButton ||
                    DYNCAST(CInput, m_pFlowLayout->ElementOwner())->GetType() == htmlInputReset ||
                    DYNCAST(CInput, m_pFlowLayout->ElementOwner())->GetType() == htmlInputSubmit))
                    {
                        m_fMinMaxValid = FALSE;
                        m_ptle->ShiftLines(m_xMaxWidth, m_yBestHeight, 0, 0, 0, 0);
                    }

                break;
            }

#if DBG==1
        default:
            AssertSz(0, "CQuillGlue::RecalcView Unknown SIZEMODE_xxxx");
            break;
#endif
        }

    // reset local pci, it won't be valid any more
    _pci = NULL;

    // code duplicated from CDisplay::RecalcView() cousin

    // Note if any contained sites have a percentage width or height
    // (This check is usually perform as a by-product of positioning the sites.
    //  However, when RecalcView is used, such as during min/max calculations,
    //  that code is not called; thus it is duplicated here.)
    CFlowLayout * pFlowLayout = m_pFlowLayout;
    CLayout     * pLayout;
    DWORD         dw;

    pFlowLayout->_fChildWidthPercent  =
    pFlowLayout->_fChildHeightPercent = FALSE;

    for (pLayout = pFlowLayout->GetFirstLayout(&dw);
         pLayout;
         pLayout = pFlowLayout->GetNextLayout(&dw))
    {
        pFlowLayout->_fChildWidthPercent  = (pFlowLayout->_fChildWidthPercent  ||
                                                    pLayout->PercentWidth());
        pFlowLayout->_fChildHeightPercent = (pFlowLayout->_fChildHeightPercent ||
                                                    pLayout->PercentHeight());
    }
    pFlowLayout->ClearLayoutIterator(dw);

    return TRUE;
}

void
CQuillGlue::RecalcLineShift(CCalcInfo * pci, DWORD grfLayout)
{
    Assert (m_fMinMaxValid && !m_pFlowLayout->ContainsChildLayout());
    Assert(!m_pFlowLayout->IsDisplayNone());

    if (m_ptle)
    {
        long    dxpBody = m_pFlowLayout->GetDisplay()->GetMaxPixelWidth();
        long xLeftPadding, xRightPadding, yTopPadding, yBottomPadding;

        GetPadding(0,tpuTargetPixels, &xLeftPadding, &xRightPadding, &yTopPadding, &yBottomPadding);
        m_ptle->ShiftLines(dxpBody, m_yBestHeight, xLeftPadding, xRightPadding, yTopPadding, yBottomPadding);
    }
}

BOOL
CQuillGlue::HasLines()
{
    if (m_ptle)
        return (m_ptle->HaveLines() == S_OK ? TRUE : FALSE);

    return FALSE;
}

LONG
CQuillGlue::GetLongestLineWidth()
{
    Assert(m_xWidthLongestLine >= m_xMinWidth);
    return m_xWidthLongestLine;
}

HRESULT
CQuillGlue::GetCharacterMetrics(
    long cp,
    long Type,
    RECT *prc,
    long *px,
    long *py)
{
    HRESULT hr = S_FALSE;

    // REVIEW sidda: add support for absolutely-positioned objects
    if (px) *px = 0;
    if (py) *py = 0;

    cp = CpRelFromCpAbs(cp);
    if (m_ptle)
    {
        hr = m_ptle->GetCpMetrics(cp, Type, prc, px, py);
    }
    return hr;
}

// @doc QTAPI

//+------------------------------------------------------------------------
//
//  ITextStory implementation.
//
//-------------------------------------------------------------------------

/*----------------------------------------------------------------------------
@interface ITextStory |

    Exposes text and properties in the HTML backing store.

@meth   HRESULT | GetLength | Retrieve character length of text.
@meth   HRESULT | GetRunLimits | Retrieve CP range for a run of text.
@meth   HRESULT | GetPapLimits | Retrieve CP range for a paragraph.
@meth   HRESULT | FetchText | Retrieve plain text.
@meth   HRESULT | FetchTextProperties | Retrieve properties for a run of text.
@meth   HRESULT | IsNestedLayoutAtCp | Determine if a nested layout exists.
@meth   HRESULT | GetNestedObjectAttributes | Fetch object alignment attributes.
@meth   HRESULT | GetNestedObjectMargins | Fetch object margins.

@comm   This method exposes the text that is to be displayed by a layout
        element.

@supby	<o TextLayoutSite>
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
@method HRESULT | ITextStory | GetLength |

    Retrieve character length of text.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetLength(
    long *pcpMac)   // @parm Character length is returned in *<p pcpMac>.
{
	Assert(m_pFlowLayout && m_pFlowLayout->IsFlowLayout());
    Assert(pcpMac);
    *pcpMac = m_pFlowLayout->GetContentTextLength();
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextStory | GetPapLimits |

    Retrieve CP range for a run of text.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetPapLimits(
    long cpRel,        // @parm CP within a run. REVIEW alexmog(sidda): can cp > pcpFirst?
    long *pcpFirst, // @parm First CP of the paragraph returned in *<p pcpFirst>.
    long *pcpLim)   // @parm Last CP of the paragraph returned in *<p pcpLim>.
{
	Assert(m_pFlowLayout && m_pFlowLayout->IsFlowLayout());
	Assert(pcpFirst || pcpLim);
    LONG ich;

    long cp = CpAbsFromCpRel(cpRel);

    CTreePos *ptp = m_pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);

    *pcpFirst = CpRelFromCpAbs(ptp->GetCp());
    *pcpLim = *pcpFirst + ptp->GetCch();

#ifdef REVIEW   // sidda: fun tree
    // $REVIEW azmyh: cpFirst is used by LS to flag the first line in a paragraph,
    // to properly apply paragraph properties,text run cpFirst should not be used as Pap cpFirst,
    // to get the coreect pap cpFirst we should use CLinesrv
    // This is a temporary workaround by going back one charcter and locking for a possible paragraph boundary.
    if (cp > m_pFlowLayout->GetFirstCp())
        {
	    CRchTxtPtr rtp(m_pFlowLayout->GetPed(), cp - 1 );
	    rtp.RetreatToNonEmpty();
        wchar_t chTmp;
	    int cch = rtp._rpTX.GetText(1, &chTmp);
        if (chTmp != WCH_BLOCKBREAK && chTmp != WCH_TXTSITEBREAK && chTmp != WCH_TXTSITEEND)
            *pcpFirst = CpRelFromCpAbs(rtp.GetCp());
        }
#endif  // REVIEW

	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextStory | GetRunLimits |

    Retrieve CP range for a run of text.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetRunLimits(
    long cpRel,        // @parm CP within a run. REVIEW alexmog(sidda): can cp > pcpFirst?
    long *pcpFirst, // @parm First CP of the run returned in *<p pcpFirst>.
    long *pcpLim)   // @parm Last CP of the run returned in *<p pcpLim>.
{
	Assert(m_pFlowLayout && m_pFlowLayout->IsFlowLayout());
	Assert(pcpFirst || pcpLim);

    LONG ich;

    long cp = CpAbsFromCpRel(cpRel);

    CTreePos *ptp = m_pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);

    *pcpFirst = cpRel;
    *pcpLim = *pcpFirst + ptp->GetCch() - ich;

	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextStory | FetchText |

    Retrieve plain text.

@rvalue	S_OK | Success.
@rvalue E_OUTOFMEMORY | Ran out of memory.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::FetchText(
    long cpRel,         // @parm CP from which to fetch text.
    wchar_t *pchBuffer, // @parm Caller-allocated buffer where text is to be returned.
    int cchBuffer,      // @parm Length in unicode characters of buffer.
    int *pcchFetch)     // @parm *<p pcchFetch> is set to the number of characters fetched.
{
	Assert(m_pFlowLayout && m_pFlowLayout->IsFlowLayout());

    if (pcchFetch)
        *pcchFetch = 0;

    long cpMac;
    GetLength(&cpMac);

    if (!cpMac)
        return S_OK;

	if (!pchBuffer || cchBuffer < 1) 
		return E_INVALIDARG;

    long cp = CpAbsFromCpRel(cpRel);

    // REVIEW alexmog: to get the run with common properties, I need CRchTxtPtr, right?
	CTxtPtr tp(m_pFlowLayout->GetContentMarkup(), cp);

	int cch = tp.GetText(cchBuffer, pchBuffer);

	if (pcchFetch)
		*pcchFetch = cch;
	
	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextStory | FetchTextProperties |

    Retrieve properties for a run of text.

@rvalue	S_OK | Success.
@rvalue E_OUTOFMEMORY | Ran out of memory.

@comm   <p propertyGroup> can be one of the following values:

@flag   propertyGroupChar | Fetch Quill's CHP properties.
@flag   propertyGroupPara | Fetch Quill's PAP properties.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::FetchTextProperties(
    long cpRel,                 // @parm CP for which properties are to be fetched.
    long propertyGroup,         // @parm which properties to be fetched.
                                // See below for a list of values.
    ITextPropertyList **pptpl)  // @parm Property list returned in *<p pptpl>.
{
    Assert(m_pFlowLayout && m_pFlowLayout->IsFlowLayout());

    if (pptpl)
        *pptpl = NULL;

    long cpMac;
    GetLength(&cpMac);
    if (!cpMac)
        return S_OK;

    long cp = CpAbsFromCpRel(cpRel);

    LONG ich;
    CTreePos *ptp = m_pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);
    CTreeNode *pNode = ptp->GetBranch();

    // create propety list 
    // (this doesn't mean a new one would be created, it should 
    //  usually be taken from a pool of unused property list objects)
	HRESULT hr = TLS(_pQLM)->CreateTextPropertyList(pptpl);
    if (FAILED(hr))
        return hr;

    CParentInfo PI;
    PI.Init(m_pFlowLayout);

    if (propertyGroup == propertyGroupChar)
        {
        // Use local access pointer (for conversion callbacks)
        // REVIEW alexmog: we wouldn't need this if prop access was supported on format classes
        CCharFormatPropertyAccess propaccessCF;
        propaccessCF.put_This(pNode->GetCharFormat());
        propaccessCF.put_ParentInfo(&PI);
        propaccessCF.put_FF(pNode->GetFancyFormat());

        // generate the list for CF
        // REVIEW alexmog: this IS expensive - use GetDefaultPropertyList
        hr = (*pptpl)->GetFullList(&dictCF, &propaccessCF);
        }
    else
        {
        Assert(propertyGroup == propertyGroupPara);

        // Use local access pointer (for conversion callbacks)
        CParaFormatPropertyAccess propaccessPF;
        propaccessPF.put_This(pNode->GetParaFormat());
        propaccessPF.put_ParentInfo(&PI);
        propaccessPF.put_FF(pNode->GetFancyFormat());
        propaccessPF.put_CF(pNode->GetCharFormat());

        // generate the list for PF
        hr = (*pptpl)->GetFullList(&dictPF, &propaccessPF);
        }

    return hr;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextStory | IsNestedLayoutAtCp |

    Determine if the specified CP contains a nested layout object.

@rvalue	S_OK | Nested layout object exists at specified cp.
@rvalue S_FALSE | No nested layout object exists at specified cp.

@xref   <om ITextLayoutSite.CalcNestedSizeAtCp>, <om ITextLayoutSite.DrawNestedLayoutAtCp>,
        <om ITextLayoutSite.PositionNestedLayout>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::IsNestedLayoutAtCp(
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via <om ILineServicesHost.GetTreePointer> or 0.
    long cp)            // @parm CP of nested layout.
{
    long cpMac;
    GetLength(&cpMac);
    if (!cpMac)
        return S_FALSE;

    if (dwCookieTP)
    {
        CTreePos **pptp = (CTreePos **)dwCookieTP;
        long cpAbs = CpAbsFromCpRel(cp);
        long ich;

        SyncTreePointerToCp(pptp, cpAbs, &ich);
        if (IsNestedLayoutAtTreePos(dwCookieTP) == S_OK)
            return S_OK;
    }
    else
    {
        CLayout *pLayout;
        pLayout = GetOwnerLayoutAtCp(cp);
        if (pLayout != NULL && pLayout != m_pFlowLayout)
            return S_OK;
    }

    return S_FALSE;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextStory | GetNestedObjectMargins |

    Fetch nested object margins

@rvalue	S_OK | Nested layout object exists at specified cp.
@rvalue S_FALSE | No nested layout object exists at specified cp.

----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetNestedObjectMargins(
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via <om ILineServicesHost.GetTreePointer> or 0.
    long cp,            // @parm CP of nested layout.
    RECT *prcMargins)   // @parm Margins around object returned here.
{
    CLayout *pLayout;

    if (dwCookieTP)
        {
        Assert((*(CTreePos **)dwCookieTP)->GetCp() == CpAbsFromCpRel(cp));
        pLayout = GetNestedLayoutFromTreePointer((CTreePos **)dwCookieTP);
        }
    else
        pLayout = GetOwnerLayoutAtCp(cp);

    if (pLayout == NULL)
        goto LExit;

    Assert(pLayout != m_pFlowLayout);
    if(pLayout == m_pFlowLayout)
        goto LExit;

    long cpMac;
    GetLength(&cpMac);
    if (cpMac)
        {
        CCalcInfo   CI;
        CI.Init(pLayout);
        // get the margin info for the site
        pLayout->GetMarginInfo(&CI, &prcMargins->left, &prcMargins->top, &prcMargins->right, &prcMargins->bottom);
        }
    else
        {
        prcMargins->left = 0;
        prcMargins->right = 0;
        prcMargins->top = 0;
        prcMargins->bottom = 0;
        }

    return S_OK;

LExit:
    return S_FALSE;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextStory | GetNestedObjectAttributes |

    Fetch object alignment attributes.

@rvalue	S_OK | Nested layout object exists at specified cp.
@rvalue S_FALSE | No nested layout object exists at specified cp.

@comm   *<p palign> is set to one of the following values:

@flag   htmlControlAlignNotSet = 0 | No alignment.
@flag   htmlControlAlignLeft = 1 | Horizontally left aligned.
@flag   htmlControlAlignCenter = 2 | Horizontally center aligned.
@flag   htmlControlAlignRight = 3 | Horizontally right aligned.
@flag   htmlControlAlignTextTop = 4 | Vertically aligned to text top.
@flag   htmlControlAlignAbsMiddle = 5 | Vertically aligned to absolute middle.
@flag   htmlControlAlignBaseline = 6 | Vertically aligned to text baseline.
@flag   htmlControlAlignAbsBottom = 7 | Vertically aligned to absolute bottom.
@flag   htmlControlAlignBottom = 8 | Vertically aligned to bottom.
@flag   htmlControlAlignMiddle = 9 | Vertically aligned to middle.
@flag   htmlControlAlignTop = 10 | Vertically aligned to top.

@comm   *<p pstylePos> will be set to one of the following values:

@flag   stylePositionNotSet = 0 | No positioning style.
@flag   stylePositionstatic = 1 | Default positioning style.
@flag   stylePositionrelative = 2 | Relative positioning.
@flag   stylePositionabsolute = 3 | Absolute positioning.

@comm   Note that <p palign> and <p pstylePos> are mutually exclusive.
        That is, if *<p palign> is set to something other than htmlControlAlignNotSet,
        then *<p pstylePos> will be set only to stylePositionstatic.

@comm   <p pdwFlags> will be set with the following flags:

@flag   OBJFLAGS_OWNLINE | Line should be terminated after object. For
                            example, tables behave this way.
@flag   OBJFLAGS_HIDDEN | Object should not be displayed.
@flag   OBJFLAGS_ABSAUTOTOP | Object is absolutely-positioned with top=auto
@flag   OBJFLAGS_ABSAUTOLEFT | Object is absolutely-positioned with left=auto
@flag   OBJFLAGS_PERCENTWIDTH | Object has percent width.
@flag   OBJFLAGS_PERCENTHEIGHT | Object has percent height.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetNestedObjectAttributes(
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via <om ILineServicesHost.GetTreePointer> or 0.
    long cp,            // @parm CP of nested layout.
    long *pcwchRun,     // @parm If non-NULL, *<p pcwchRun> will be set to the
                        // count of characters in the nested layout object.
    long *palign,       // @parm Object alignment returned here. See below for a list
                        // of values.
    long *pstylePos,    // @parm Object positioning style returned here. See below
                        // for a list of values.
    DWORD *pdwFlags)    // @parm If non-NULL, set some flags. See below for a list.
{
    CElement    *pElementLayout;
    CLayout *pLayout;

    if (dwCookieTP)
    {
        Assert((*(CTreePos **)dwCookieTP)->GetCp() == CpAbsFromCpRel(cp));
        pLayout = GetNestedLayoutFromTreePointer((CTreePos **)dwCookieTP);
    }
    else
        pLayout = GetOwnerLayoutAtCp(cp);

    if (pcwchRun) *pcwchRun = 0;
    if (palign) *palign = 0;
    if (pstylePos) *pstylePos = 0;
    if (pdwFlags) *pdwFlags = 0;

    long cpMac;
    GetLength(&cpMac);
    if (!cpMac)
        return S_OK;

    if (pLayout == NULL)
        goto LExit;

    Assert(pLayout != m_pFlowLayout);

    if(pLayout == m_pFlowLayout)
        goto LExit;

    pElementLayout = pLayout->ElementOwner();
    if (pElementLayout)
        {
        htmlControlAlign atSite;  // The alignment of the site
        if (palign)
            {
            atSite = pElementLayout->GetSiteAlign();
            *palign = atSite;
            }

        if (pstylePos)
            {
            const CFancyFormat * pFF = pElementLayout->GetFirstBranch()->GetFancyFormat();
            stylePosition        bPositionType = (stylePosition)pFF->_bPositionType;
            *pstylePos = bPositionType;

#if DBG == 1
            if (palign)
                {
                Assert(*pstylePos == stylePositionNotSet ||
                       *pstylePos == stylePositionstatic ||
                       *pstylePos == stylePositionrelative ||
                       *palign == htmlControlAlignNotSet);
                }
#endif
            }

        if (pcwchRun)
            *pcwchRun = pElementLayout->GetElementCch() + 2;

        if (pdwFlags)
            {
            BOOL fOwnLine;

            // sidda: stolen from CEmbeddedILSObj::Fmt()
            fOwnLine =    pElementLayout->HasFlag(TAGDESC_OWNLINE)
                       || (   !pElementLayout->_fSite
                           && pElementLayout->IsBlockTag()
                          );

            if (fOwnLine)
                *pdwFlags |= OBJFLAGS_OWNLINE;

            if(pLayout->IsDisplayNone())
                *pdwFlags |= OBJFLAGS_HIDDEN;

            if (pElementLayout->IsAbsolute())
                {
                CTreeNode   *pNodeContext = pLayout->GetFirstBranch();
                CUnitValue  uvTop, uvLeft;
                BOOL        fTopAuto, fLeftAuto;

                uvTop  = pNodeContext->GetCascadedtop();
                uvLeft = pNodeContext->GetCascadedleft();

                fTopAuto  = (uvTop.IsNull() || uvTop.GetUnitType() == CUnitValue::UNIT_ENUM);
                fLeftAuto = (uvLeft.IsNull() || uvLeft.GetUnitType() == CUnitValue::UNIT_ENUM);

                if (fTopAuto)
                    *pdwFlags |= OBJFLAGS_ABSAUTOTOP;

                if (fLeftAuto)
                    *pdwFlags |= OBJFLAGS_ABSAUTOLEFT;
                }

            if (pLayout->PercentWidth())
                *pdwFlags |= OBJFLAGS_PERCENTWIDTH;

            if (pLayout->PercentHeight())
                *pdwFlags |= OBJFLAGS_PERCENTHEIGHT;
            }
        }

LExit:
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  ITextLayoutSite implementation
//
//-------------------------------------------------------------------------

/*----------------------------------------------------------------------------
@interface ITextLayoutSite |

    Provides layout and display support to an HTML layout element.

@meth   HRESULT | GetLayoutSize | .
@meth   HRESULT | GetMargins | .
@meth   HRESULT | GetColumns | .
@meth   HRESULT | GetConstraintPoints | .
@meth   HRESULT | GetDisplayRect | .
@meth   HRESULT | GetScale | .
@meth   HRESULT | GetBkgndMode | .
@meth   HRESULT | GetRotation | .
@meth   HRESULT | GetTextFlow | .
@meth   HRESULT | GetVerticalAlign | .
@meth   HRESULT | GetAllowSplitWordsAcrossConstraints | .
@meth   HRESULT | GetHyphenation | .
@meth   HRESULT | GetScrollPosition | .
@meth   HRESULT | GetVisible | .

@meth   void | OnSelectionChanged | .
@meth   HRESULT | SetCrs | .
@meth   HRESULT | GetVisibleRect | .
@meth   HRESULT | EnsureRectVisible | .
@meth   HRESULT | ScrollForMouseDrag | .

@meth   BOOL | HasFocus | .
@meth   HRESULT | RequestResize | .
@meth   HRESULT | GetHdcRef | .
@meth   HRESULT | FreeHdcRef | .
@meth   BOOL | UpdateWindow | .
@meth   HRESULT | CursorOut | .

@meth   HRESULT | EventNotify | .

@meth   HRESULT | CalcNestedSizeAtCp | .
@meth   HRESULT | GetAbsoluteRenderPosition | .
@meth   HRESULT | DrawNestedLayoutAtCp | .
@meth   HRESULT | PositionNestedLayout | .

@meth   HRESULT | AppendRectHelper | Add a rectangle to a region collection.
@meth   HRESULT | InitDisplayTreeForPositioning | Prepare for a positioning pass.
@meth   HRESULT | PositionLayoutDispNode | Position a nested layout object.
@meth   HRESULT | DrawBackgroundAndBorder | Draw background and border for a text element.

@supby	<o TextLayoutSite>
----------------------------------------------------------------------------*/

// Rounding MulDiv
inline int MulDivR(int w, int wMul, int wDiv)
{
	// system call rounds
	AssertSz(wDiv, "divide by zero");
	return MulDiv(w, wMul, wDiv);
}

#define DzlFromDxp(dxpInch, dxp) (dxpInch ? MulDivR(dxp, dzlInch, dxpInch) : 1)
#define DzlFromDyp(dypInch, dyp) (dypInch ? MulDivR(dyp, dzlInch, dypInch) : 1)

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetLayoutSize |

    Retrieve the dimensions available for this layout element to flow into.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
#define dzlInch 914440

STDMETHODIMP
CQuillGlue::GetLayoutSize(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    int nUnit,          // @parm Unit of layout requested. For now, assume display pixels.
    long *pdxpWidth,    // @parm Width returned in *<p pdxpWidth>.
    long *pdypHeight)   // @parm Height returned in *<p pdypHeight>.
{
	if (!m_pFlowLayout->GetDisplay()->GetWordWrap())
    {
		*pdxpWidth = 0x7FFFFFFF;//	In a case of marquee the xWidth should greater than the width of 
		                        //  the longest line could be.
    }
	else
	{
	    LONG xWidth = m_pFlowLayout->GetDisplay()->GetViewWidth();

#ifdef REVIEW // alexmog: move this to Quill side, we can do more intelligent caching there
		// stretch layout width for longest word
		//$REVIEW alexmog: this is supporting what is apparently a Netscape compatibility feature - 
		//                 flow layout can't be narrower than the longest word in it, even if
		//                 its width is fixed. We may want to make this optional
		if (m_fMinMaxValid && xWidth < m_xMinWidth)
			xWidth = m_xMinWidth;
#endif // REVIEW

		*pdxpWidth = DzlFromDxp(g_sizePixelsPerInch.cx, xWidth);
	}
    
#ifdef REVIEW   // sidda: support infinite height for main body
    *pdypHeight = DzlFromDyp(g_sizePixelsPerInch.cy, m_pFlowLayout->GetDisplay()->GetViewHeight());
#endif  // REVIEW
    *pdypHeight = DzlFromDyp(g_sizePixelsPerInch.cy, 0xffff);
        
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetPadding |

    Retrieve the padding dimensions this layout element should respect.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/

STDMETHODIMP
CQuillGlue::GetPadding(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    int nUnit,
    long *pdxpLeft,
    long *pdxpRight,
    long *pdypTop,
    long *pdypBottom)
{
	CElement *pElement = GetFlowLayoutElement();
	CDisplay *pDisplay = m_pFlowLayout->GetDisplay();
    ELEMENT_TAG etag = pElement->Tag();

    CCalcInfo   CI;
    CI.Init(m_pFlowLayout);

    long        lPadding[PADDING_MAX];
    m_pFlowLayout->GetDisplay()->GetPadding(&CI, lPadding);

	long dxlLeft = lPadding[PADDING_LEFT];
	long dxlRight = lPadding[PADDING_RIGHT];
	long dylTop = lPadding[PADDING_TOP];
	long dylBottom = lPadding[PADDING_BOTTOM];

    if(etag == ETAG_MARQUEE &&
        !pElement->IsEditable() &&
        !pDisplay->Printing())
    {
        CMarquee *pMarquee = DYNCAST(CMarquee, pElement);
		dxlLeft += pMarquee->_lXMargin;
		dxlRight += pMarquee->_lXMargin;
//		dylTop += pMarquee->_lXMargin;
//		dylBottom += pMarquee->_lXMargin;
    }

    if (nUnit != tpuTargetPixels)
	{
        *pdxpLeft = DzlFromDxp(g_sizePixelsPerInch.cx, dxlLeft);
        *pdxpRight = DzlFromDxp(g_sizePixelsPerInch.cx, dxlRight);
        *pdypTop = DzlFromDyp(g_sizePixelsPerInch.cy, dylTop);
        *pdypBottom = DzlFromDyp(g_sizePixelsPerInch.cy, dylBottom);
    }
	else
	{
        *pdxpLeft = dxlLeft;
        *pdxpRight = dxlRight;
        *pdypTop = dylTop;
        *pdypBottom = dylBottom;
	}

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetColumns |

    Retrieve the number of columns.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetColumns(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    int *pcColumns)
{
    *pcColumns = 1;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetConstraintPoints |

    Retrieve constraints the layout element should avoid.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetConstraintPoints(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    ITextConstraintPoints **ppPoints,
    DWORD *pdwConsCookie)
{
    *pdwConsCookie = 0;
    *ppPoints = NULL;
    return E_NOTIMPL;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetDisplayRect |

    Retrieve the current display rectangle.

@rvalue	S_OK | Success.

@comm   NOTE: this is NOT in client coordinates!
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetDisplayRect(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    RECT *prect)        // @parm Rectangle returned in client coordinates.
{
#ifdef REVIEW   // following code gives client coordinates
    m_pFlowLayout->GetRect(prect, COORDSYS_GLOBAL);
#endif

    // WARNING: this code must be kept in sync with CQuillGlue::Render()

    // get logical view rectangle, including scroll offsets
    m_pFlowLayout->GetClientRect(prect);

    // nuke scroll offsets
    prect->left = 0;
    prect->top = 0;

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetScale |

    Retrieve the number of pixels per inch in the display.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetScale(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long *pdxpInch,
    long *pdypInch)
{
    *pdxpInch = g_sizePixelsPerInch.cx;
    *pdypInch = g_sizePixelsPerInch.cy;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetBkgndMode |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetBkgndMode(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long *pBkMode,
    COLORREF *pcrBack)
{
    *pBkMode = qbkmodeTransparent;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetRotation |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetRotation(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    float *pdegs)
{
	IHTMLElement * pelem = NULL;
	IHTMLElement2 * pelem2 = NULL;
	IHTMLCurrentStyle * pCurStyle = NULL;
	VARIANT varMsoRot = { 0 };
	HRESULT hrAttr = S_OK;

	*pdegs = 0;	// clear it

	GetCurrentElement(&pelem);
	if (!pelem) 
		goto NotFound;

	pelem->QueryInterface(IID_IHTMLElement2,(LPVOID*)&pelem2);
	if (!pelem2) 
		goto NotFound;

	pelem2->get_currentStyle(&pCurStyle);
	if (!pCurStyle) 
		goto NotFound;

	hrAttr = pCurStyle->getAttribute(L"mso-rotate", FALSE, &varMsoRot);
	if (hrAttr == S_OK && varMsoRot.vt == VT_BSTR) // then there was a style setting.
	{
		// change string to int
		if (VariantChangeType(&varMsoRot, &varMsoRot, 0, VT_I4) == S_OK)
		{
			int nAng = varMsoRot.lVal;
			if (nAng<0) 
				nAng += 360*((-nAng)/360 + 1);// accept negative angles but return a corrected angle
			*pdegs = nAng;

			AssertSz((*pdegs >= 0), "Invalid angle - must be non-negative"); // *pdegs has to be non-negative. 
																		 // It can be greater than 360 because Quill will take care of it.
		}
	}

NotFound:

	// release carefully!!
	if (pelem) pelem->Release();
	if (pelem2) pelem2->Release();
	if (pCurStyle) pCurStyle->Release();

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetTextFlow |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetTextFlow(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long *pqtf)
{
    *pqtf = qtfHorizontal;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetVerticalAlign |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetVerticalAlign(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long *pVA)
{
    *pVA = qvaTop;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetAllowSplitWordsAcrossConstraints |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetAllowSplitWordsAcrossConstraints(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    BOOL *pfAllow)
{
    *pfAllow = FALSE;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetHyphenation |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetHyphenation(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long *pfHyphenate,
    float *pptsHyphenationZone)
{
    *pfHyphenate = FALSE;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetScrollPosition |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetScrollPosition(
    long *pdxpScroll,
    long *pdypScroll)
{
    *pdxpScroll = m_pFlowLayout->GetXScroll();
    *pdypScroll = m_pFlowLayout->GetYScroll();
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetVisible |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetVisible(
    long *pfVisible)
{
#ifndef DISPLAY_TREE
    *pfVisible = (m_pFlowLayout->_fParked ? FALSE : TRUE);
#else
    // $REVIEW alexmog: when should it return false?
    *pfVisible = TRUE;
#endif
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | OnSelectionChanged |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP_(void)
CQuillGlue::OnSelectionChanged(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long qselchg)
{
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | SetCrs |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::SetCrs(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long dgc,
    float degs)
{

#ifdef SASHAT
    switch (dgc)
        {
        case 0: //qcrsNone:
        break;
        case 1: // qcrsDefaultPointer:
            //m_pFlowLayout->ElementOwner()->SetCursorStyle(IDC_ARROW);
            SetCursorIDC(IDC_ARROW);
        break;

        case 2: // qcrsWaitCursor:
        break;
        case 3: // qcrsInsCursor:
            //m_pFlowLayout->ElementOwner()->SetCursorStyle(IDC_IBEAM);
            SetCursorIDC(IDC_IBEAM);
        break;
        /*
        case qcrsDragAvail:
        case qcrsDrag:
        case qcrsDragCopy:
        case qcrsDragAndScroll:
        case qcrsDragCopyAndScroll:
        case qcrsDragInvalid:
        case qcrsDragInvalidAndScroll:
        case qcrsLeftMargin:
        case qcrsOverField:
        case qcrsOverObject:
        case qcrsMoveObject:
        case qcrsSizeObjectNorth:
        case qcrsSizeObjectNorthEast:
        case qcrsSizeObjectEast:
        case qcrsSizeObjectSouthEast:
        case qcrsSizeObjectSouth:
        case qcrsSizeObjectSouthWest:
        case qcrsSizeObjectWest:
        case qcrsSizeObjectNorthWest:
        case qcrsCropObjectNorth:
        case qcrsCropObjectNorthEast:
        case qcrsCropObjectEast:
        case qcrsCropObjectSouthEast:
        case qcrsCropObjectSouth:
        case qcrsCropObjectSouthWest:
        case qcrsCropObjectWest:
        case qcrsCropObjectNorthWest:
        case qcrsCropObject:
        case qcrsMaxQuill:
        */
        default:
        break;
    } // switch

    return S_OK;

#else
    // nobody call it
    return E_NOTIMPL;
    
#endif // SASHAT

    
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetVisibleRect |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetVisibleRect(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    RECT *prect)
{
    prect->left =
    prect->top =
    prect->right =
    prect->bottom = 0;

#ifdef DBG
    ELEMENT_TAG etag = m_pFlowLayout->GetDisplay()->GetFlowLayoutElement()->Tag();
#endif

    // obtain the display rectangle of the body layout
    if (m_pFlowLayout->Doc())
    {
        prect->right = m_pFlowLayout->Doc()->_dci._sizeDst.cx;
        prect->bottom = m_pFlowLayout->Doc()->_dci._sizeDst.cy;
    }

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | EnsureRectVisible |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::EnsureRectVisible(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    RECT *prcBound,
    RECT *prcFrame,
    long scrHorz,
    long scrVert)
{
    RECT    rcView;
    LONG    xWidthView;
    LONG    yHeightView;
    LONG    xScroll = 0;
    LONG    yScroll = 0;

#ifndef DISPLAY_TREE
    m_pFlowLayout->GetVisibleClientRect(&rcView);
#else
    m_pFlowLayout->GetClientRect(&rcView);
#endif

    xWidthView = rcView.right - rcView.left;
    yHeightView = rcView.bottom - rcView.top;

    if (prcBound->right - prcBound->left <= xWidthView)
    {
        // entire width of requested rectangle fits - make sure its visible
        if (prcBound->left < rcView.left)
        {
            // scroll left to make left of requested rectangle visible
            xScroll = prcBound->left - rcView.left;
        }
        else if (prcBound->right > rcView.right)
        {
            // scroll right to make right of requested rectangle visible
            xScroll = prcBound->right - rcView.right;
        }
    }
    else
    {
        // rectangle is larger - make sure left is visible
        xScroll = prcBound->left - rcView.left;
    }

    if (prcBound->bottom - prcBound->top <= yHeightView)
    {
        // entire height of requested rectangle fits - make sure its visible
        if (prcBound->top < rcView.top)
        {
            // scroll up to make top of requested rectangle visible
            yScroll = prcBound->top - rcView.top;
        }
        else if (prcBound->bottom > rcView.bottom)
        {
            // scroll down to make bottom of requested rectangle visible
            yScroll = prcBound->bottom - rcView.bottom;
        }
    }
    else
    {
        // rectangle is larger - make sure top is visible
        yScroll = prcBound->top - rcView.top;
    }

#ifndef DISPLAY_TREE
    if (xScroll)
    {
        xScroll += m_pFlowLayout->GetDisplay()->GetXScrollEx();
        m_pFlowLayout->GetDisplay()->ScrollView(NULL, xScroll, -1, FALSE, TRUE);
    }
    if (yScroll)
    {
        yScroll += m_pFlowLayout->GetDisplay()->GetYScroll();
        m_pFlowLayout->GetDisplay()->ScrollView(NULL, -1, yScroll, FALSE, TRUE);
    }
#else    
    m_pFlowLayout->ScrollBy(CSize(xScroll, yScroll));
#endif

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | ScrollForMouseDrag |

    Retrieve the number of columns.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::ScrollForMouseDrag(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    POINT pt)
{
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | HasFocus |

    Retrieve the number of columns.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL)
CQuillGlue::HasFocus(
    DWORD dwSiteCookie) // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
{
    return TRUE;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | RequestResize |

    Request size change. Unused

@rvalue	S_OK    | OK
@rvalue	S_FALSE | Not OK
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::RequestResize(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    float ptsNewWidth,
    float ptsNewHeight)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------------------------
@method HRESULT | CQuillGlue | RequestLayout |

    Request layout. 
    NOTE: not a member of ITextLayoutSite. 
    Add it there if you need (and you'll have to define layout flags if you do that).

@rvalue	S_OK    | Layout request enqueued.
@rvalue	S_FALSE | Layout request have been enqueued already.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::RequestLayout(DWORD grfLayout)
{
    if (!_fLayoutRequested)
    {
        // $REVIEW (alexmog, PERF): This is kind of simplified version of that can be
        //      seen in CFlowLayout. We will want to revisit this for performance
        if (!m_pFlowLayout->_fSizeThis)
        {
            m_pFlowLayout->PostLayoutRequest(grfLayout);

            // don't request layout again until this one happens
            _fLayoutRequested = TRUE;
            
            return S_OK;
        }
    }
    else
    {
        Assert(TRUE);   // set breakpoint here if you need to know...
    }
    
    return S_FALSE;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetHdcRef |

    Retrieve the number of columns.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetHdcRef(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    HDC *phdcRef)
{
    if (m_pFlowLayout->Doc())
    {
        if (m_pFlowLayout->Doc()->InPlace())
	        m_hwndRef = m_pFlowLayout->Doc()->InPlace()->_hwnd;

        *phdcRef = GetDC(m_hwndRef);
        return S_OK;
    }
    return E_FAIL;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | FreeHdcRef |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::FreeHdcRef(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    HDC hdcRef)
{
	if (hdcRef)
		ReleaseDC(m_hwndRef, hdcRef);

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | UpdateWindow |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL)
CQuillGlue::UpdateWindow(
    DWORD dwSiteCookie) // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
{
    return FALSE;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | CursorOut |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::CursorOut(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    POINT pt,
    long qcmdPhysical,
    long qcmdLogical)
{
    return S_FALSE;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | EventNotify |

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::EventNotify(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long qevt,
    long evtLong1,
    long evtLong2)
{
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | CalcNestedSizeAtCp |

    Compute the size of a nested layout object.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::CalcNestedSizeAtCp(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via <om ILineServicesHost.GetTreePointer> or 0.
    long cp,            // @parm CP of nested layout.
    long fUnderLayout,  // @parm Non-zero to signify we are under layout.
    long xWidthMax,     // @parm Maximum column width available to object.
    SIZE *psize)        // @parm Size returned here.
{
    int xMinWidth;
    CLayout *pLayout;

    if (dwCookieTP)
        {
        Assert((*(CTreePos **)dwCookieTP)->GetCp() == CpAbsFromCpRel(cp));
        pLayout = GetNestedLayoutFromTreePointer((CTreePos **)dwCookieTP);
        }
    else
        pLayout = GetOwnerLayoutAtCp(cp);

    psize->cx = 0;
    psize->cy = 0;

    if (pLayout == NULL)
        goto LExit;

    Assert(pLayout != m_pFlowLayout);
    if(pLayout == m_pFlowLayout)
        goto LExit;

    Assert(!pLayout->IsDisplayNone());

    // block to avoid compiler errors due to goto
    {    
        CCalcInfo   *pci = _pci;
        CCalcInfo   CI;
        if (pci == NULL)
        {
            CI.Init(pLayout);
            pci = &CI;
        }

        // if the object is still parked when we're asking for its metrics
        // that indicates SetPosition has not been called on it.
#ifndef DISPLAY_TREE
        if (!fUnderLayout && pLayout->_fParked)
#else
        if (!fUnderLayout)
#endif
        {
            LONG xLeftMargin, xRightMargin, yTopMargin, yBottomMargin;
            
            // get the margin info for the site
            pLayout->GetMarginInfo(pci, &xLeftMargin, &yTopMargin, &xRightMargin, &yBottomMargin);

            // adjust the size to include margins
#ifndef DISPLAY_TREE
            psize->cx = pLayout->_sizeProposed.cx;
            psize->cy = pLayout->_sizeProposed.cy;
#else
            pLayout->GetSize(psize);
#endif
            psize->cx += xLeftMargin + xRightMargin;
            psize->cy += yTopMargin + yBottomMargin;
        }
        else
        {
            // set _smMode to MMWIDTH and then restore it. GetSiteWidth doesn't imply any mode.
            SIZEMODE smMode = pci->_smMode;
#if 0 //$REVIEW alexmog
            pci->_smMode = SIZEMODE_MMWIDTH;
#endif

            // see CEmbeddedILSObj::Fmt
            m_pFlowLayout->GetSiteWidth(pLayout, pci, fUnderLayout, xWidthMax, &xMinWidth, &psize->cx, &psize->cy);

            pci->_smMode = smMode;
        }
    }        

LExit:
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | GetAbsoluteRenderPosition |

    Determine the offset for an absolutely-positioned nested layout object.

@rvalue	S_OK | Success.

@comm   This API is unused.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetAbsoluteRenderPosition(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via <om ILineServicesHost.GetTreePointer> or 0.
    long cp,            // @parm CP of absolutely-positioned nested layout.
    long *pdxpLeft,     // @parm X pixel offset returned here, if applicable.
    long *pdypTop)      // @parm Y pixel offset returned here, if applicable.
{
    CPoint pt;
    CLayout *pLayout;

    *pdxpLeft = 0;
    *pdypTop = 0;

    if (dwCookieTP)
        {
        Assert((*(CTreePos **)dwCookieTP)->GetCp() == CpAbsFromCpRel(cp));
        pLayout = GetNestedLayoutFromTreePointer((CTreePos **)dwCookieTP);
        }
    else
        pLayout = GetOwnerLayoutAtCp(cp);

    if (pLayout == NULL)
        goto LExit;

    Assert(pLayout != m_pFlowLayout);
    if(pLayout == m_pFlowLayout)
        goto LExit;

    Assert(!pLayout->IsDisplayNone());

    pLayout->GetPosition(&pt);
    *pdxpLeft = pt.x;
    *pdypTop = pt.y;

LExit:
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | DrawNestedLayoutAtCp |

    Retrieve the number of columns.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::DrawNestedLayoutAtCp(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via <om ILineServicesHost.GetTreePointer> or 0.
    long cp)            // @parm CP of nested layout.
{
#ifndef DISPLAY_TREE // $REVIEW alexmog: this doesn't need to happen at all with display tree

    // _pdi expected to be set for nested calls
    Assert(_pdi);

    // see CLSRenderer::RenderSite
    // this code is taken from there.

    RECT            rc;
    stylePosition   bPositionType;     // static, relative, or absolute
    CLayout *pLayout;

    if (dwCookieTP)
        {
        Assert((*(CTreePos **)dwCookieTP)->GetCp() == CpAbsFromCpRel(cp));
        pLayout = GetNestedLayoutFromTreePointer((CTreePos **)dwCookieTP);
        }
    else
        pLayout = GetOwnerLayoutAtCp(cp);

    if (_pdi == NULL || pLayout == NULL)
        goto LExit;

    Assert(pLayout != m_pFlowLayout);

    if(pLayout == m_pFlowLayout)
        goto LExit;

    bPositionType = pLayout->GetFirstBranch()->GetCascadedposition();

    rc = pLayout->_rc;

    if (pLayout->GetFirstBranch()->IsPositionStatic(bPositionType) &&
        !pLayout->_fParked)
    {
        RECT rcTemp;

        if (IntersectRect(&rcTemp, &rc, _pdi->ClipRect()))
        {
            pLayout->DrawFilteredElement(_pdi);
        }
    }

LExit:
#endif
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | PositionNestedLayout |

    Unpark and set the position of the nested layout.

@rvalue	S_OK | Success.

@todo   Support parking.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::PositionNestedLayout(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via <om ILineServicesHost.GetTreePointer> or 0.
    long cp,            // @parm CP of nested layout.
    RECT *prc)          // @parm New position in display coordinates of the object.
{
#ifndef DISPLAY_TREE
    CCalcInfo       CI;
    CPositionInfo   PI;
    CPositionInfo   *ppi = _ppi;
    CElement        *pElementLayout;
    RECT            *prcClip;
    RECT            rc;
    RECT            rcSite = *prc;
    BOOL            fPositioned;
    BOOL            fVisible = TRUE;
    BOOL            fDoParking = TRUE;  // $REVIEW sidda: nuke this with display tree
    CLayout         *pLayout;

#ifdef REVIEW // alexmog: try this optimization later
    // do nothing if this is under _fCalcOnly. 
    // $REVIEW alexmog: I am not sure if this should qualify for being called a hack. 
    //                  If we pass this to GetLayoutWidth or GetLayoutHight,
    //                  its only use will be to prevent this call. So, 
    //                  why not just check it here?
    if (m_pFlowLayout->_fCalcOnly)
        goto LExit;
#endif // REVIEW

    if (dwCookieTP)
    {
        Assert((*(CTreePos **)dwCookieTP)->GetCp() == CpAbsFromCpRel(cp));
        pLayout = GetNestedLayoutFromTreePointer((CTreePos **)dwCookieTP);
    }
    else
        pLayout = GetOwnerLayoutAtCp(cp);

    if (pLayout == NULL)
        goto LExit;

    Assert(pLayout != m_pFlowLayout);

    if(pLayout == m_pFlowLayout)
        goto LExit;

    if (ppi == NULL)
        {
        if (_pci)
            PI.Init(_pci);
        else
            {
            CI.Init(m_pFlowLayout);
            PI.Init(&CI);
            }
        PI.ClipToSite(m_pFlowLayout);
        ppi = &PI;
        }

    // see CDisplay::PositionSite(). This code is duplicated from there.

    pElementLayout = pLayout->ElementOwner();
    fPositioned = !pElementLayout->IsPositionStatic();

    //
    // Determine the clipping RECT
    // (Positioned sites use the clipping RECT of their "region" parent,
    //  all others use that passed down in the CPositionInfo)
    //

    prcClip = ppi->ClipRect(fPositioned);

    //
    // Finally, position the site if it is within the clipping RECT
    //

#ifdef DISPLAY_TREE
    if (CDebugPaint::UseDisplayTree() ||
        (fVisible && IntersectRect(&rc, &rcSite, prcClip)))
#else
    if (fVisible && IntersectRect(&rc, &rcSite, prcClip))
#endif
    {
        ppi->_grfLayout &= ~(LAYOUT_PARKING | LAYOUT_UNPARKING);

#ifndef DISPLAY_TREE
        if (pLayout->_fParked)
        {
            ppi->_grfLayout |= LAYOUT_UNPARKING;
        }

        pLayout->_fParked       = FALSE;
        pLayout->ElementOwner()->_fPositionThis = TRUE;
#endif

#if 0
     // REVIEW sidda: Quill does not update _sizeProposed enough!
     // This causes Trident to assert so we work around that here.
     if (pLayout->_sizeProposed.cx != rcSite.right - rcSite.left)
         {
         pLayout->_sizeProposed.cx = rcSite.right - rcSite.left;
         }
     if (pLayout->_sizeProposed.cy != rcSite.bottom - rcSite.top)
         {
         pLayout->_sizeProposed.cy = rcSite.bottom - rcSite.top;
         }
#endif

        // NOTE: _grfLayout must have LAYOUT_NOINVAL to avoid 
        //       invalidation of nested objects. Parent must
        //       know when to update them.
        {
            DWORD grfLayout = ppi->_grfLayout;
            ppi->_grfLayout |= LAYOUT_NOINVAL;
#ifndef DISPLAY_TREE
            pLayout->SetPosition(ppi, &rcSite);
#else
            pLayout->SetPosition(CPoint(rcSite.left, rcSite.top));
#endif
            ppi->_grfLayout  = grfLayout;
        }
    }

    //
    // If the site does not intersect the clipping RECT, park it if requested
    //

    else if (fDoParking)
    {
#ifndef DISPLAY_TREE
        if (    !pLayout->_fParked
            ||  pLayout->ElementOwner()->_fPositionThis
            ||  pLayout->_fPositionBelow
            ||  pLayout->_fLayoutPositioned
            ||  rcSite.right  - rcSite.left != pLayout->_rc.right  - pLayout->_rc.left
            ||  rcSite.bottom - rcSite.top  != pLayout->_rc.bottom - pLayout->_rc.top)
        {
            rcSite.top    = -100 - pLayout->_sizeProposed.cy;
            rcSite.bottom = -100;

            ppi->_grfLayout &= ~(LAYOUT_PARKING | LAYOUT_UNPARKING);
            if (!pLayout->_fParked)
            {
                ppi->_grfLayout |= LAYOUT_PARKING;
            }

            pLayout->_fParked       = TRUE;
            pLayout->ElementOwner()->_fPositionThis = TRUE;
            pLayout->SetPosition(ppi, &rcSite);
        }
#endif
    }

LExit:
#endif
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | AppendRectHelper |

    Add a rectangle to a region collection.

@rvalue	S_OK | Success.
@rvalue S_FALSE | Rectangle not appended due to CP range.
@rvalue E_OUTOFMEMORY | Ran out of memory.

@comm   This callback method enables Trident to manage the memory
        of a region collection.

@xref   <om ITextLayoutElement.RegionFromRange>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::AppendRectHelper(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    void *pvaryRects,   // @parm Previously passed to <om ITextLayoutElement.RegionFromRange>
    RECT *prcBound,     // @parm Previously passed to <om ITextLayoutElement.RegionFromRange>
    RECT *prcLine,      // @parm Rectangle to be added to the collection.
    LONG xOffset,       // @parm Previously passed to <om ITextLayoutElement.RegionFromRange>
    LONG yOffset,       // @parm Previously passed to <om ITextLayoutElement.RegionFromRange>
    LONG cp,            // @parm First CP of the rectangle.
    LONG cpClipStart,   // @parm Previously passed to <om ITextLayoutElement.RegionFromRange>
    LONG cpClipFinish)  // @parm Previously passed to <om ITextLayoutElement.RegionFromRange>
{
    // code duplicated from AppendRectToElemRegion()

    Assert(pvaryRects);
    Assert(prcLine);

    CDataAry <RECT> * paryRects = (CDataAry <RECT> *)pvaryRects;
    HRESULT hr = S_FALSE;

    if(xOffset || yOffset)
        OffsetRect(prcLine, xOffset, yOffset);

    cp = CpAbsFromCpRel(cp);
    cpClipStart = CpAbsFromCpRel(cpClipStart);
    cpClipFinish = CpAbsFromCpRel(cpClipFinish);

    if(cp >= cpClipStart && cp <= cpClipFinish)
    {
        hr = paryRects->AppendIndirect(prcLine);
    }
    if(prcBound)
    {
        UnionRect(prcBound, prcBound, prcLine);
    }

    return hr;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | InitDisplayTreeForPositioning |

    Prepare for a positioning pass.

@rvalue	S_OK | Success.

@comm   Prepares for subsequent calls to <om .PositionLayoutDispNode>.

@xref   <om .PositionLayoutDispNode>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::InitDisplayTreeForPositioning(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long cpStart)   // @parm Starting CP of this layout engine.
{
    long cp = CpAbsFromCpRel(cpStart);

    _pDispNodePrev = m_pFlowLayout->GetDisplay()->GetPreviousDispNode(cp);
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | PositionLayoutDispNode |

    Position a nested layout object.

@rvalue	S_OK | Success.

@comm   Inserts the nested layout object's display tree node immediately after
        the last one inserted and also provides it with it's parent-relative
        top-left-corner.

        It is the caller's responsibility to ensure that nested layout object
        nodes are processed in the correct z-order.

@xref   <om .InitDisplayTreeForPositioning>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::PositionLayoutDispNode(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    long cpObject,  // @parm CP of nested layout object.
    long dx,        // @parm Parent-relative X offset of nested object's top-left corner.
    long dy)        // @parm Parent-relative Y offset of nested object's top-left corner.
{
    // $REVIEW sidda: terrible splaying of tree can happen here
    // better to use a tree pointer which is manually advanced
    CLayout *pLayout = GetOwnerLayoutAtCp(cpObject);
    CElement *pElementLayout = pLayout->ElementOwner();

    Assert(!pLayout->IsDisplayNone());

    if (!pElementLayout->IsPositionStatic())
    {
		// This moves absolute and relative objects to higher z-order. 
		// They figure out their position themselves.
        {
            CPoint ptOffset(dx, dy);
            pElementLayout->ZChangeElement(0, &ptOffset);
        }
    }
    else
    {
        CParentInfo PI;
        PI.Init(m_pFlowLayout);

        _pDispNodePrev = m_pFlowLayout->GetDisplay()->AddLayoutDispNode(
                                            &PI,
                                            pLayout,
                                            dx,
                                            dy,
                                            _pDispNodePrev);
    }
    
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ITextLayoutSite | DrawBackgroundAndBorder |

    Draw the background and border for a text element.

@rvalue	S_OK | Success.
----------------------------------------------------------------------------*/
STDMETHODIMP 
CQuillGlue::DrawBackgroundAndBorder(
    DWORD dwSiteCookie, // @parm Site cookie specified in <om ITextLayoutElement.SetTextLayoutSite>
    RECT *prcClip,      // @parm Clipping rectangle. This is the text page.
    long cpStart,       // @parm Starting CP of element from which to draw.
    long cpLim)         // @parm Lim CP upto which to draw.
{
    LONG cpJunk;
    RECT rcJunk = *prcClip;
    long cpStartAbs = CpAbsFromCpRel(cpStart);
    long cpLimAbs = CpAbsFromCpRel(cpLim);

    Assert(_pdi);
    m_pFlowLayout->GetDisplay()->DrawBackgroundAndBorder(
                                    cpStartAbs,
                                    _pdi,
                                    -1,         // ili
                                    -1,         // lCount
                                    &cpJunk,    // &piliDraw
                                    -1,         // yLi
                                    rcJunk,     // rcView
                                    rcJunk,     // rcClip
                                    cpLimAbs
                                    );
    return S_OK;
}

STDMETHODIMP 
CQuillGlue::GetHtmlDoc(IHTMLDocument2** ppDoc)
{
    if (!ppDoc)
       return E_INVALIDARG;

    *ppDoc = NULL;
    
    Assert(m_pFlowLayout);

    return m_pFlowLayout->Doc()->QueryInterface(IID_IHTMLDocument2, (LPVOID*)ppDoc);
}

STDMETHODIMP
CQuillGlue::GetCurrentElement(IHTMLElement** ppElement)
{
    if (!ppElement)
       return E_INVALIDARG;

    *ppElement = NULL;
    
    Assert(m_pFlowLayout);

    return m_pFlowLayout->ElementOwner()->QueryInterface(IID_IHTMLElement, (LPVOID*)ppElement);
}

//+------------------------------------------------------------------------
//
//  ITextInPlacePaint implementation
//
//-------------------------------------------------------------------------

/*----------------------------------------------------------------------------
@interface ITextInPlacePaint |

	Provides view-driven access to the client's drawing surface. Supports
	efficient compositing based on close cooperation with the client.

@comm   See <i IQuillInPlacePaint> for a full description.

@supby	<o TextLayoutSite>
----------------------------------------------------------------------------*/

STDMETHODIMP
CQuillGlue::BeginIncrementalPaint(DWORD dwSiteCookie, long tc, RECT *prcMax,
                                  DWORD dwFlags, HDC *phdcRef, POINT *rgptRotate)
{
	*phdcRef = NULL;

	if (m_pFlowLayout->Doc() && m_pFlowLayout->Doc()->InPlace())
		{
		m_hwndPaint = m_pFlowLayout->Doc()->InPlace()->_hwnd;
		m_hdcPaint = *phdcRef = GetDC(m_hwndPaint);
		return S_OK;
		}

	return E_FAIL;
}

STDMETHODIMP
CQuillGlue::EndIncrementalPaint(DWORD dwSiteCookie, long tc)
{
	if (m_hdcPaint)
		{
		ReleaseDC(m_hwndPaint, m_hdcPaint);
		}

	m_hwndPaint = NULL;
	m_hdcPaint = NULL;
	return S_OK;
}

STDMETHODIMP
CQuillGlue::BeginRectPaint(DWORD dwSiteCookie, long tc, RECT *prc, DWORD dwFlags,
                           long SurfaceKind, HDC *phdc, POINT *rgptRotate)
{
	*phdc = m_hdcPaint;

    if (dwFlags == qpaintEditRequestScreenAccess)
        {
        // TODO: clip to rectangle
        m_fScreenAccess = TRUE;
        }
    else
        {
	// Give Quill a drawing surface, but don't let any drawing happen now -- we rely on window invalidation to redraw the rectangle
		::IntersectClipRect(m_hdcPaint, 0, 0, 0, 0);
        m_rectPaint = *prc;
        }

	return S_OK;
}

STDMETHODIMP
CQuillGlue::EndRectPaint(DWORD dwSiteCookie, long tc, HDC *phdc)
{
	if (!m_fScreenAccess)
        {
		::InvalidateRect(m_hwndPaint, &m_rectPaint, TRUE);
        }
    else
        {
        // TODO: refresh foreground layers
        m_fScreenAccess = FALSE;
        }

	return S_OK;
}

BOOL
CQuillGlue::IsRectObscured(DWORD dwSiteCookie, long tc, RECT *prc, POINT *rgptRotate)
{
    return TRUE;
}

//
// InvalidateRect
//
// Directly invalidate rectangle to cause a Windows paint message for it.
//
STDMETHODIMP
CQuillGlue::InvalidateRect(DWORD dwSiteCookie, long tc, RECT *prc, DWORD dwFlags, POINT *rgptRotate)
{
    if (m_pFlowLayout)
    {
        m_pFlowLayout->Invalidate(prc, 0L); 
    }
	return S_OK;
}

