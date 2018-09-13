#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CARET_HXX_
#define X_CARET_HXX_
#include "caret.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif


#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef _X_ADORNER_HXX_
#define _X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_DIMM_H_
#define X_DIMM_H_
#include "dimm.h"
#endif

#ifndef X_BREAKER_HXX_
#define X_BREAKER_HXX_
#include "breaker.hxx"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_TXTSLAVE_HXX_
#define X_TXTSLAVE_HXX_
#include "txtslave.hxx"
#endif

#ifdef UNIX
#include "quxcopy.hxx"
#endif

static const LPTSTR astrCursor[] =
{
    IDC_SIZEALL,                    // HTC_TOPBORDER         = 21,
    IDC_SIZEALL,                    // HTC_LEFTBORDER        = 22,
    IDC_SIZEALL,                    // HTC_BOTTOMBORDER      = 23,
    IDC_SIZEALL,                    // HTC_RIGHTBORDER       = 24,
    IDC_SIZENWSE,                   // HTC_TOPLEFTHANDLE     = 25,
    IDC_SIZEWE,                     // HTC_LEFTHANDLE        = 26,
    IDC_SIZENS,                     // HTC_TOPHANDLE         = 27,
    IDC_SIZENESW,                   // HTC_BOTTOMLEFTHANDLE  = 28,
    IDC_SIZENESW,                   // HTC_TOPRIGHTHANDLE    = 29,
    IDC_SIZENS,                     // HTC_BOTTOMHANDLE      = 30,
    IDC_SIZEWE,                     // HTC_RIGHTHANDLE       = 31,
    IDC_SIZENWSE,                   // HTC_BOTTOMRIGHTHANDLE = 32
};


MtDefine(CDocRegionFromMarkupPointers_aryRects_pv, Locals, "CDoc::RegionFromMarkupPointers aryRects::_pv")


DeclareTag(tagSelectionTimer, "Selection", "Selection Timer Actions in CDoc")
DeclareTag(tagViewServicesErrors, "ViewServices", "Show Viewservices errors")
DeclareTag( tagViewServicesCpHit, "ViewServices", "Show Cp hit from CpFromPoint")
DeclareTag( tagViewServicesShowEtag, "ViewServices", "Show _etag in MoveMarkupPointer")

DeclareTag( tagViewServicesShowScrollRect, "ViewServices", "Show Scroll Rect")
DeclareTag(tagEditDisableEditFocus, "Edit", "Disable On Edit Focus")

const long scrollSize = 5;

////////////////////////////////////////////////////////////////
//    IHTMLViewServices methods


HRESULT
CDoc::MoveMarkupPointerToPoint( 
    POINT               pt, 
    IMarkupPointer *    pPointer, 
    BOOL *              pfNotAtBOL, 
    BOOL *              pfAtLogicalBOL,
    BOOL *              pfRightOfCp, 
    BOOL                fScrollIntoView )
{
    RRETURN( THR( MoveMarkupPointerToPointEx( pt, pPointer, TRUE, pfNotAtBOL, pfAtLogicalBOL, pfRightOfCp, fScrollIntoView ))); // Default to global coordinates
}

HRESULT 
CDoc::MoveMarkupPointerToPointEx(
    POINT               pt,
    IMarkupPointer *    pPointer,
    BOOL                fGlobalCoordinates,
    BOOL *              pfNotAtBOL,
    BOOL *              pfAtLogicalBOL,
    BOOL *              pfRightOfCp,
    BOOL                fScrollIntoView )
{
    HRESULT hr = E_FAIL;
    CMarkupPointer * pPointerInternal = NULL;
    POINT ptContent;
    
    CTreeNode * pTreeNode = GetNodeFromPoint( pt, fGlobalCoordinates, &ptContent );
    
    if( pTreeNode == NULL )
        goto Cleanup;
        
    if ( ( ptContent.x == 0 ) && ( ptContent.y == 0 ) )
    {
        //
        // BUGBUG Sometimes - HitTestPoint returns ptContent of 0 We take over ourselves.
        //
        CFlowLayout * pLayout = NULL;
        
        pLayout = GetFlowLayoutForSelection( pTreeNode );
        if( pLayout == NULL )
            goto Cleanup;

        CPoint myPt( pt );
        pLayout->TransformPoint( &myPt, COORDSYS_GLOBAL, COORDSYS_CONTENT, NULL );

        ptContent.x = myPt.x;
        ptContent.y = myPt.y;
    }
    
    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) &pPointerInternal ));
    if( FAILED( hr ))
        goto Cleanup;

    hr = THR( MovePointerToPointInternal( ptContent, pTreeNode, pPointerInternal, pfNotAtBOL, pfAtLogicalBOL, pfRightOfCp, fScrollIntoView, GetFlowLayoutForSelection( pTreeNode ) ));

Cleanup:
    RRETURN( hr );
}


HRESULT
CDoc::MoveMarkupPointerToMessage( 
    IMarkupPointer *    pPointer ,
    SelectionMessage *  pMessage,
    BOOL *              pfNotAtBOL,
    BOOL *              pfAtLogicalBOL,
    BOOL *              pfRightOfCp,
    BOOL *              pfValidTree,
    BOOL                fScrollIntoView,    
    IHTMLElement*       pIContainerElement, 
    BOOL*               pfSameLayout,
    BOOL                fHitTestEndOfLine )
{
    HRESULT             hr = S_OK;
    BOOL                fValidTree = FALSE;
    CTreeNode *         pTreeNode = NULL;
    CMarkupPointer *    pPointerInternal = NULL;
    CMarkup *           pPointerMarkup = NULL;
    CMarkup *           pCookieMarkup = NULL;
    CElement*           pContainerElement = NULL;    
    CLayout*            pContainingLayout = NULL;
    CRect globalContextRect;
    
    AssertSz( pIContainerElement, "No container element given - is edit context set ? ");
    if ( pIContainerElement )
    {
        hr = THR( pIContainerElement->QueryInterface( CLSID_CElement, (void**) & pContainerElement ));
        if ( FAILED( hr ))
            goto Cleanup;

        pContainingLayout = DYNCAST( CLayout, GetFlowLayoutForSelection(pContainerElement->GetFirstBranch())) ; 
    }
    
    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if( FAILED( hr ))
        goto Cleanup;
        
    pTreeNode = static_cast<CTreeNode*>( (void *)(pMessage->elementCookie) );

    //
    // Set the Cookie Markup 
    // before We "fix" the TreeNode to the edit context if the point is outside
    //
    if( pTreeNode )
    {
        Assert( ! pTreeNode->IsDead() );
        if ( pTreeNode->IsDead())
        {
            hr = E_FAIL;
            goto Cleanup;
        }            
        pCookieMarkup = pTreeNode->GetMarkup(); 
    }
    
    if ( pContainingLayout )
    {
        pContainingLayout->GetRect( & globalContextRect, COORDSYS_GLOBAL );
        if( !::PtInRect( & globalContextRect, pMessage->pt ))
        {
            //
            // If the point is Outside the Edit Context but inside the global window
            // we will do the hit test on the Edit Context
            //
            pTreeNode = pContainingLayout->GetFirstBranch();
            pMessage->ptContent = pMessage->pt;
            pContainingLayout->TransformPoint( (CPoint*) &pMessage->ptContent, COORDSYS_GLOBAL, COORDSYS_CONTENT );
        }
    }
    
    //
    // Do some hit test work - to cater for WM_TIMER messages .
    //
    if( pTreeNode == NULL || pTreeNode->Element()->Tag() == ETAG_ROOT  )
    {
        CFlowLayout * pLayout = NULL;
        BOOL fEmptySpace = FALSE;            
        pTreeNode = GetNodeFromPoint( pMessage->pt , TRUE, NULL, NULL, & fEmptySpace);
        if ( ! pTreeNode )
            pTreeNode = PrimaryMarkup()->GetElementClient()->GetFirstBranch();

        Assert( pTreeNode );
        // Stuff the resolved tree node back into the message...
        pMessage->elementCookie = (DWORD_PTR)pTreeNode;
        pMessage->fEmptySpace = fEmptySpace;
        
        CLayout *pLayoutOriginal = pTreeNode->GetUpdatedLayout();
        
        pLayout = GetFlowLayoutForSelection( pTreeNode );
        if( pLayout == NULL )
            goto Cleanup;

        //
        // Set the ptContent to be based on what layout we're over
        // so wehn MoveMarkupPointerInternal compensates below - we're in sync.
        //
        CPoint myPt( pMessage->pt );
        if ( pLayoutOriginal && pLayoutOriginal != pLayout )
            pLayoutOriginal->TransformPoint( & myPt, COORDSYS_GLOBAL, COORDSYS_CONTENT, NULL );
        else
            pLayout->TransformPoint( &myPt, COORDSYS_GLOBAL, COORDSYS_CONTENT, NULL );

        TraceTag(( tagViewServicesCpHit, "We had a null TreeNode. Point Transformed from:%ld,%ld to %ld,%ld\n", 
                pMessage->pt.x, pMessage->pt.y,myPt.x, myPt.y ));
                
        // Stuff the resolved point back into the message...
        pMessage->ptContent.x = myPt.x;
        pMessage->ptContent.y = myPt.y;
            
    }
    else if ( ( pMessage->ptContent.x == 0 ) && ( pMessage->ptContent.y == 0 ) )
    {
        CFlowLayout * pLayout = NULL;
        
        pLayout = GetFlowLayoutForSelection( pTreeNode );
        if( pLayout == NULL )
            goto Cleanup;

        CPoint myPt( pMessage->pt );
        pLayout->TransformPoint( &myPt, COORDSYS_GLOBAL, COORDSYS_CONTENT, NULL );

        TraceTag(( tagViewServicesCpHit, "We had a null ptContent. Point Transformed from:%ld,%ld to %ld,%ld\n", 
                pMessage->pt.x, pMessage->pt.y,myPt.x, myPt.y ));
                
        // Stuff the resolved point back into the message...
        pMessage->ptContent.x = myPt.x;
        pMessage->ptContent.y = myPt.y;
    }
    
    //
    // Figure out if we are in the same tree as the passed in pointer. Fail if the 
    // passed in pointer is unpositioned. Note that we have to do this BEFORE we
    // move the tree pointer.
    //
    if ( ! pCookieMarkup )
        pCookieMarkup = pTreeNode->GetMarkup();

    if (pPointerInternal->IsPositioned())
    {
        pPointerMarkup = pPointerInternal->Markup();
        fValidTree = ( pCookieMarkup == pPointerMarkup );
    }
    else
    {
        fValidTree = FALSE;
    }
    
    hr = THR( MovePointerToPointInternal( 
                                            pMessage->ptContent, 
                                            pTreeNode, 
                                            pPointerInternal, 
                                            pfNotAtBOL, 
                                            pfAtLogicalBOL, 
                                            pfRightOfCp, 
                                            fScrollIntoView, 
                                            pContainingLayout, 
                                            pfSameLayout,
                                            fHitTestEndOfLine ));
    if( FAILED( hr ))
        goto Cleanup;
        
    
Cleanup:
    if ( pfValidTree )
        *pfValidTree = fValidTree;

    RRETURN(hr);
}

    
HRESULT
CDoc::MovePointerToPointInternal(
    POINT               tContentPoint,
    CTreeNode *         pNode,
    CMarkupPointer *    pPointer,
    BOOL *              pfNotAtBOL,
    BOOL *              pfAtLogicalBOL,
    BOOL *              pfRightOfCp,
    BOOL                fScrollIntoView,
    CLayout*            pContainingLayout ,
    BOOL*               pfValidLayout,
    BOOL                fHitTestEndOfLine /* = TRUE*/)
{
    HRESULT         hr = S_OK;
    LONG            cp = 0;
    BOOL            fNotAtBOL;
    BOOL            fAtLogicalBOL;
    BOOL            fRightOfCp;
    CFlowLayout *   pFlowLayout = NULL;
    CMarkup *       pMarkup = NULL;
    CTreePos *      ptp = NULL;
    BOOL            fPtNotAtBOL = FALSE;
    CPoint          ptHit(tContentPoint);
    CPoint          ptGlobal;

#if DBG == 1
    CElement* pDbgElement = pNode->Element();
    ELEMENT_TAG eDbgTag = pDbgElement->_etag;    
    TraceTag((tagViewServicesShowEtag, "MovePointerToPointInternal. _etag:%d", eDbgTag));
#endif
    
    Assert(pNode);
    pFlowLayout = GetFlowLayoutForSelection( pNode );
    if(!pFlowLayout)
        goto Error;

    {
        LONG cpMin = pFlowLayout->GetContentFirstCp();
        LONG cpMax = pFlowLayout->GetContentLastCp();
        CDisplay * pdp = pFlowLayout->GetDisplay();
        CLinePtr rp( pdp );
        CLayout *pLayoutOriginal = pNode->GetUpdatedLayout();
        LONG cchPreChars = 0;

        //
        // BUGBUG : (johnbed) this is a completely arbitrary hack, but I have to
        // improvise since I don't know what line I'm on
        //
        fPtNotAtBOL = ptHit.x > 15;   

        if (pLayoutOriginal && pFlowLayout != pLayoutOriginal)
        {
            //
            // BUGBUG - right now - hit testing over the TR or Table is broken
            // we simply fail this situation. Selection then effectively ignores this point
            //
            if ( pLayoutOriginal->ElementOwner()->_etag == ETAG_TABLE ||
                   pLayoutOriginal->ElementOwner()->_etag == ETAG_TR )
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            pLayoutOriginal->TransformPoint(&ptHit, COORDSYS_CONTENT, COORDSYS_GLOBAL);
            pFlowLayout->TransformPoint(&ptHit, COORDSYS_GLOBAL, COORDSYS_CONTENT);
        }

        pdp->WaitForRecalc(-1, ptHit.y );

        ptGlobal = ptHit;
        pFlowLayout->TransformPoint(&ptGlobal, COORDSYS_CONTENT, COORDSYS_GLOBAL);
        cp = pdp->CpFromPointReally(    ptGlobal,       // Point
                                        &rp,            // Line Pointer
                                        &ptp,           // Tree Pos
                                        fHitTestEndOfLine ? CDisplay::CFP_ALLOWEOL : 0 ,
                                        &fRightOfCp,
                                        &cchPreChars);

        TraceTag(( tagViewServicesCpHit, "ViewServices: cpFromPoint:%d\n", cp));

        if( cp < cpMin )
        {
            cp = cpMin;
            rp.RpSetCp( cp , fPtNotAtBOL );
        }
        else if ( cp > cpMax )
        {
            cp = cpMax;
            rp.RpSetCp( cp , fPtNotAtBOL );
        }
        
        fNotAtBOL = rp.RpGetIch() != 0;     // Caret OK at BOL if click
        fAtLogicalBOL = rp.RpGetIch() <= cchPreChars;
    }
    
    //
    // If we are positioned on a line that contains no text, 
    // then we should be at the beginning of that line
    //

    // TODO - Implement this in today's world

    //
    // Prepare results
    //
    
    if( pfNotAtBOL )
        *pfNotAtBOL = fNotAtBOL;

    if ( pfAtLogicalBOL )
        *pfAtLogicalBOL = fAtLogicalBOL;
    
    if( pfRightOfCp )
        *pfRightOfCp = fRightOfCp;

    pMarkup = pFlowLayout->GetContentMarkup();
    if( ! pMarkup )
        goto Error;

    hr = pPointer->MoveToCp( cp, pMarkup );
    if( hr )
        goto Error;
        
#if DBG == 1 // Debug Only

    //
    // Test that the pointer is in a valid position
    //
    
        {
        CTreeNode * pTst = pPointer->CurrentScope(MPTR_SHOWSLAVE);
        
        if( pTst )
        {
            if(  pTst->Element()->Tag() == ETAG_ROOT )
                TraceTag( ( tagViewServicesErrors, " MovePointerToPointInternal --- Root element "));
        }
        else
        {
            TraceTag( ( tagViewServicesErrors, " MovePointerToPointInternal --- current scope is null "));
        }
    }
#endif // DBG == 1 

    //
    // Scroll this point into view
    //
    
    if( fScrollIntoView && pFlowLayout && OK( hr ) )
    {

        //
        // BUGBUG - take the FlowLayout off of the ElemEditContext.
        //


        if ( _pElemEditContext && _pElemEditContext->GetFirstBranch() )
        {
            CFlowLayout* pScrollLayout = NULL;        
            if ( _pElemEditContext->_etag == ETAG_TXTSLAVE )
            {
                pScrollLayout = _pElemEditContext->MarkupMaster()->GetFlowLayout();
            }
            else
                pScrollLayout = _pElemEditContext->GetFlowLayout();
                
            Assert( pScrollLayout );
            if ( pScrollLayout )
            {   
                if ( pScrollLayout != pFlowLayout )
                {
                    //
                    // BUGBUG - yet another transform.
                    //
                    pFlowLayout->TransformPoint(&ptHit, COORDSYS_CONTENT, COORDSYS_GLOBAL);
                    pScrollLayout->TransformPoint(&ptHit, COORDSYS_GLOBAL, COORDSYS_CONTENT);
                }
                     
                {
                    //
                    // BUGBUG: This scrolls the point into view - rather than the pointer we found
                    // unfotunately - scroll range into view is very jerky.
                    //
                    CRect r( ptHit.x - scrollSize, ptHit.y - scrollSize, ptHit.x + scrollSize, ptHit.y + scrollSize );            
                    pScrollLayout->ScrollRectIntoView( r, SP_MINIMAL, SP_MINIMAL, TRUE );
                }
            }
        }
    }

    goto Cleanup;
    

Error:
    TraceTag( ( tagViewServicesErrors, " MovePointerToPointInternal --- Failed "));
    return( E_UNEXPECTED );
    
Cleanup:
    if ( pfValidLayout )
        *pfValidLayout = ( pFlowLayout == pContainingLayout );
        
    RRETURN(hr);
}

//
// GetCharFormatInfo
//
// Pass in a MarkupPointer, a family mask, and a pointer to a
// data struct, we populate the struct and mark the mask in the
// struct to denote which fields are sucessfully set.
// We return S_FALSE if there is an error getting the format info.
// Note that it is possible that the command will partially 
// complete sucessfully. In such a case, check the eFamily field
// in the struct to see how far we got.

HRESULT 
CDoc::GetCharFormatInfo( IMarkupPointer *pPointer, WORD wFamily, HTMLCharFormatData *pInfo )
{
    HRESULT hr = S_OK;
    CMarkupPointer  * pPointerInternal;
    CTreeNode * pNode;
    const CCharFormat * pCFormat;
    
    pInfo->dwSize = sizeof( pInfo );
    pInfo->wFamilyFlags = CHAR_FORMAT_None;

    // Cast the passed in IMarkupPointer to a CMarkupPointer so I can get at internal goodness
    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if (hr)
        goto Cleanup;

    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);

    if (!pNode)
        goto Cleanup;

    pCFormat = pNode->GetCharFormat();

    if( ! pCFormat )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // from pFormat, just copy the appropriate data into the struct...
    if( (wFamily & CHAR_FORMAT_FontStyle) == CHAR_FORMAT_FontStyle )
    {
        pInfo->fBold = pCFormat->_fBold;
        pInfo->fItalic = pCFormat->_fItalic;
        pInfo->fUnderline = pCFormat->_fUnderline;
        pInfo->fOverline = pCFormat->_fOverline;
        pInfo->fStrike = pCFormat->_fStrikeOut;
        pInfo->fSubScript = pCFormat->_fSubscript;
        pInfo->fSuperScript = pCFormat->_fSuperscript;
        pInfo->wFamilyFlags = pInfo->wFamilyFlags | CHAR_FORMAT_FontStyle;
    }
    
    if( (wFamily & CHAR_FORMAT_FontInfo) == CHAR_FORMAT_FontInfo )
    {
        pInfo->fExplicitFace = pCFormat->_fExplicitFace;
        pInfo->wWeight = pCFormat->_wWeight;
        pInfo->wFontSize = pCFormat->_yHeight;
        pInfo->wFamilyFlags = pInfo->wFamilyFlags | CHAR_FORMAT_FontInfo;
    }
    
    if( (wFamily & CHAR_FORMAT_FontName) == CHAR_FORMAT_FontName )
    {
        _tcscpy( pInfo->szFont, pCFormat->GetFaceName() );
        pInfo->wFamilyFlags = pInfo->wFamilyFlags | CHAR_FORMAT_FontName;
    }
    
    if( (wFamily & CHAR_FORMAT_ColorInfo) == CHAR_FORMAT_ColorInfo )
    {
        const CFancyFormat * pFFormat;
        pFFormat = pNode->GetFancyFormat();

        pInfo->fHasBgColor = pCFormat->_fHasBgColor;
        pInfo->dwTextColor = pCFormat->_ccvTextColor.GetIntoRGB();

        // Now we need to dig into the fancy format to get the bg color.
        // If we can't get it, we don't want to completely error out. 
        // By returning S_FALSE, but still setting the ColorInfo flag, 
        // we indicate that the rest of the color info is valid.
        
        pInfo->wFamilyFlags = pInfo->wFamilyFlags | CHAR_FORMAT_ColorInfo;

        if( ! pFFormat )
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        pInfo->dwBackColor = pFFormat->_ccvBackColor.GetIntoRGB();
    }

    if( (wFamily & CHAR_FORMAT_ParaFormat) == CHAR_FORMAT_ParaFormat )
    {
        const CParaFormat * pPFormat;
        pPFormat = pNode->GetParaFormat();
        if( pPFormat == NULL )
            goto Cleanup;

        if( pPFormat->_fPre )
        {
            // Trident lies if we are in an input or a textarea. This messes up css pre-ness
            
            CMarkup * pMarkup = pPointerInternal->Markup();
            CTreeNode * pPreNode = pMarkup->SearchBranchForTag( pNode, ETAG_PRE );
            pInfo->fPre = ! ( pPreNode == NULL );
        }
        
        pInfo->fRTL = pPFormat->_fRTL;

        pInfo->wFamilyFlags = pInfo->wFamilyFlags | CHAR_FORMAT_ParaFormat;
    }
    
    
Cleanup:
    RRETURN( hr );
}


#define FONT_INDEX_SHIFT 3  // Font sizes on the toolbar are from -2 to 4, internally they are 1 to 7

HRESULT 
CDoc::ConvertVariantFromHTMLToTwips(VARIANT *pvarargIn)
{
    HRESULT hr = S_FALSE;
    long offset = 0;

    Assert(pvarargIn);
    if (((CVariant *)pvarargIn)->IsEmpty())
        goto Cleanup;

    if (V_VT(pvarargIn) == VT_BSTR)
    {
        if (   *V_BSTR(pvarargIn) == _T('+')
            || *V_BSTR(pvarargIn) == _T('-')
           )
        {
            offset = FONT_INDEX_SHIFT;
        }
    }
    hr = THR(VariantChangeTypeSpecial(pvarargIn, pvarargIn, VT_I4));
    if (hr)
        goto Cleanup;
    V_I4(pvarargIn) += offset;

    if (V_I4(pvarargIn) < 1 || V_I4(pvarargIn) > 7)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    V_I4(pvarargIn) = ConvertHtmlSizeToTwips(V_I4(pvarargIn));

Cleanup:
    RRETURN1(hr, S_FALSE);

}


HRESULT
CDoc::ConvertVariantFromTwipsToHTML(VARIANT *pvarargIn)
{
    Assert(pvarargIn);
    Assert(V_VT(pvarargIn) == VT_I4 || V_VT(pvarargIn) == VT_NULL);

    if (V_VT(pvarargIn) == VT_I4)
    {
        long htmlSize = ConvertTwipsToHtmlSize(V_I4(pvarargIn));

        if (ConvertHtmlSizeToTwips(htmlSize) == V_I4(pvarargIn))
            V_I4(pvarargIn) = htmlSize;
        else
            V_VT(pvarargIn) = VT_NULL;
    }

    return S_OK;
}

//*********************************************************************************
//
// BUGBUG - this routine returns values in the Local coords of the layout the pointer is in !
// We should either make this more explicit via a name change - or better allow specification
// of the cood-sys you are using.
//
//*********************************************************************************


HRESULT
CDoc::GetLineInfo(IMarkupPointer *pPointer, BOOL fAtEndOfLine, HTMLPtrDispInfoRec *pInfo)
{
    HRESULT             hr = S_OK;
    CMarkupPointer *    pPointerInternal;
    CFlowLayout *       pFlowLayout;
    POINT               pt;
    CTreeNode *         pNode = NULL;
    CCharFormat const * pCharFormat = NULL;
    LONG                cp;

    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if (hr)
        goto Cleanup;

    Assert( pPointerInternal->IsPositioned() );
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);   
    if(pNode == NULL)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pCharFormat = pNode->GetCharFormat();
    pFlowLayout = GetFlowLayoutForSelection(pNode);

    if(!pFlowLayout)
    {
        hr = OLE_E_BLANK;
        goto Cleanup;
    }
    

    //
    // fill in the structure with metrics from current position
    //

    {
        CLinePtr   rp(pFlowLayout->GetDisplay());
        cp = pPointerInternal->GetCp();
        CCalcInfo CI;
        CI.Init(pFlowLayout);
        BOOL fComplexLine;
        BOOL fRTLFlow;
        
        //
        // Query Position Info
        //
        
        if (-1 == pFlowLayout->GetDisplay()->PointFromTp( cp, NULL, fAtEndOfLine, FALSE, pt, &rp, TA_BASELINE, &CI, 
                                                          &fComplexLine, &fRTLFlow ))
        {
            hr = OLE_E_BLANK;
            goto Cleanup;
        }

        pInfo->lXPosition = pt.x;
        pInfo->lBaseline = pt.y;
        pInfo->fRTLLine = rp->_fRTL;
        pInfo->fRTLFlow = fRTLFlow;
        pInfo->fAligned = ENSURE_BOOL( rp->_fHasAligned );
        pInfo->fHasNestedRunOwner = ENSURE_BOOL( rp->_fHasNestedRunOwner);
        
        Assert( pFlowLayout->ElementOwner()->Tag() != ETAG_ROOT );
        
        if( pFlowLayout->ElementOwner()->Tag() != ETAG_ROOT )
        {
            pInfo->lLineHeight = rp->_yHeight;
            pInfo->lDescent = rp->_yDescent;
            pInfo->lTextHeight = rp->_yHeight - rp->_yDescent;
        }
        else
        {
            pInfo->lLineHeight = 0;
            pInfo->lDescent = 0;
            pInfo->lTextHeight = 1;
        }

        //
        // try to compute true text height
        //
        {
            CCcs     *pccs;
            CBaseCcs *pBaseCcs;
            
            pccs = fc().GetCcs( CI._hdc, &CI, pCharFormat );
            if(!pccs)
                goto Cleanup;

            pBaseCcs = pccs->GetBaseCcs();
            pInfo->lTextHeight = pBaseCcs->_yHeight + pBaseCcs->_yOffset;
            pInfo->lTextDescent = pBaseCcs->_yDescent;
            pccs->Release();
        }  
    }

Cleanup:
    RRETURN(hr);    
}

HRESULT
CDoc::IsPointerBetweenLines(
    IMarkupPointer* pPointer,
    BOOL* pfBetweenLines )
{
    HRESULT             hr = S_OK;
    CMarkupPointer *    pPointerInternal;
    CFlowLayout *       pFlowLayout;
    CTreeNode *         pNode = NULL;
    LONG                cp;
    BOOL                fBetweenLines = TRUE;
    
    if( pPointer == NULL || pfBetweenLines == NULL )
    {
        AssertSz( 0, "pPointer and pfBetweenLines must be valid!" );
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
    
    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if (hr)
        goto Cleanup;

    Assert( pPointerInternal->IsPositioned() );
    
    cp = pPointerInternal->GetCp();
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);

    if (!pNode)
        goto Cleanup;
    
    pFlowLayout = GetFlowLayoutForSelection(pNode);

    if( ! pFlowLayout )
        goto Cleanup;

    {
        LONG cpMin = pFlowLayout->GetContentFirstCp();
        LONG cpMax = pFlowLayout->GetContentLastCp();
        
        if( cp < cpMin )
            cp = cpMin;
        if( cp > cpMax )
            cp = cpMax;

        CLinePtr   rp1(pFlowLayout->GetDisplay());
        CLinePtr   rp2(pFlowLayout->GetDisplay());
        rp1.RpSetCp( cp , TRUE );
        rp2.RpSetCp( cp , FALSE );
        fBetweenLines = (rp1.GetIRun() != rp2.GetIRun() );
    }    

Cleanup:
    if( pfBetweenLines )
        *pfBetweenLines = fBetweenLines;
        
    RRETURN( hr );
}

HRESULT
CDoc::GetElementsInZOrder(POINT pt, IHTMLElement **rgElements, DWORD *pCount)
{
    return E_NOTIMPL; // REVIEW (tomlaw):  NetDocs can wait for this
}


HRESULT
CDoc::GetTopElement(POINT pt, IHTMLElement **ppElement)
{
    return elementFromPoint(pt.x,pt.y,ppElement); // REVIEW (tomlaw):  be more precise about z-ordering?
}

HRESULT
LineStart(CFlowLayout *pFlowLayout, LONG *pcp, BOOL *pfNotAtBOL, BOOL *pfAtLogicalBOL, BOOL fAdjust)
{
    // ensure calculated view
    pFlowLayout->GetDisplay()->WaitForRecalc( *pcp, -1 );    
    CLinePtr rp(pFlowLayout->GetDisplay());
    LONG cpNew;
    LONG cchSkip;
    LONG cp = *pcp;
    CLine * pli;
    HRESULT hr = S_OK;
    
    
    rp.RpSetCp( cp , *pfNotAtBOL );
    cp = cp - rp.GetIch();
    pli = rp.CurLine();
    
    // Check that we are sane
    if( cp < 1 || cp >= pFlowLayout->GetDisplay()->GetMarkup()->Cch() )
    {
        hr = E_FAIL;
        goto Cleanup;            
    }

    if( fAdjust && pli )
    {
        pFlowLayout->GetDisplay()->WaitForRecalc( max(min(cp + pli->_cch, pFlowLayout->GetContentLastCp() - 1l), 
                                                      pFlowLayout->GetContentFirstCp()), -1 );    
        rp.GetPdp()->FormattingNodeForLine(cp, NULL, pli->_cch, &cchSkip, NULL, NULL);
        cpNew = cp + cchSkip;
    }
    else
    {
        cpNew = cp;
    }
    
    *pfNotAtBOL = cpNew != cp;
    *pfAtLogicalBOL = TRUE;
    *pcp = cpNew;

Cleanup:
    RRETURN( hr );
}

HRESULT
LineEnd(CFlowLayout *pFlowLayout, LONG *pcp, BOOL *pfNotAtBOL, BOOL *pfAtLogicalBOL, BOOL fAdjust)
{
    // ensure calculated view up to current position
    pFlowLayout->GetDisplay()->WaitForRecalc( *pcp, -1 );    
    CLinePtr rp(pFlowLayout->GetDisplay());
    CLine * pli;
    LONG cpLineBegin;
    LONG cp = *pcp;
    HRESULT hr = S_OK;
    
    rp.RpSetCp( cp , *pfNotAtBOL );
    cpLineBegin = cp - rp.GetIch();
    pli = rp.CurLine();

    if( pli != NULL )
    {
        pFlowLayout->GetDisplay()->WaitForRecalc( max(min(cpLineBegin + pli->_cch, pFlowLayout->GetContentLastCp() - 1l), 
                                                      pFlowLayout->GetContentFirstCp()), -1 );    
        if( fAdjust )
            cp = cpLineBegin + rp.GetAdjustedLineLength();
        else
            cp = cpLineBegin + pli->_cch;
    }
    else
    {
        cp = cpLineBegin;
    }
    
    // Check that we are sane
    if( cp < 1 || cp >= pFlowLayout->GetDisplay()->GetMarkup()->Cch() )
    {
        hr = E_FAIL;
        goto Cleanup;       
    }
        
    *pfNotAtBOL = cp != cpLineBegin ;
    *pfAtLogicalBOL = cp == cpLineBegin;
    *pcp = cp;

Cleanup:
    RRETURN( hr );
}

HRESULT
CDoc::MoveMarkupPointer(
    IMarkupPointer *    pPointer, 
    LAYOUT_MOVE_UNIT    eUnit, 
    LONG                lXCurReally, 
    BOOL *              pfNotAtBOL,
    BOOL *              pfAtLogicalBOL)
{
    HRESULT hr = S_OK;
    CFlowLayout *pFlowLayout;
    CElement *pElemInternal = NULL;
    CTreeNode   *pNode;
    CMarkup * pMarkup = NULL;
    CMarkupPointer *pPointerInternal = NULL;
    LONG cp;
    BOOL fAdjusted = TRUE;
    
    // InterfacePointers

    
    hr = pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal);
    if (hr)
        goto Cleanup;

    // get element for current position so we can get it's flow layout    
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);
    if (!pNode)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }
   
    pElemInternal = pNode->Element();
    Assert(pElemInternal);

    pFlowLayout = GetFlowLayoutForSelection(pNode);
    if(!pFlowLayout)
    {
        hr = OLE_E_BLANK;
        goto Cleanup;
    }

    // get cp for current position
    cp = pPointerInternal->GetCp();

    //
    // use layout to get new position
    //

    pMarkup = pNode->GetMarkup();
    // Get the line where we are positioned.

    switch (eUnit)
    {
        case LAYOUT_MOVE_UNIT_OuterLineStart:
            fAdjusted = FALSE;
            // fall through
        case LAYOUT_MOVE_UNIT_CurrentLineStart:
        {
            hr = THR( LineStart(pFlowLayout, &cp, pfNotAtBOL, pfAtLogicalBOL, fAdjusted) );
            if (hr)
                goto Cleanup;
                
            // move pointer to new position
            hr = pPointerInternal->MoveToCp(cp, pMarkup);
            break;
        }
        
       case LAYOUT_MOVE_UNIT_OuterLineEnd:
            fAdjusted = FALSE;
            // fall through
       case LAYOUT_MOVE_UNIT_CurrentLineEnd:
        {
            hr = THR( LineEnd(pFlowLayout, &cp, pfNotAtBOL, pfAtLogicalBOL, fAdjusted) );
            if (hr)
                goto Cleanup;
                
            // move pointer to new position
            hr = pPointerInternal->MoveToCp(cp, pMarkup);
            break;
        }
        
        case LAYOUT_MOVE_UNIT_PreviousLine:
        case LAYOUT_MOVE_UNIT_PreviousLineEnd:
        {
            CLinePtr rp(pFlowLayout->GetDisplay());        

            // Move one line up. This may cause the txt site to be different.

            if(! pFlowLayout->GetMultiLine() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            
            rp.RpSetCp( cp , *pfNotAtBOL );
            
            if( lXCurReally <= 0 )
                lXCurReally = 12;
            
            pFlowLayout = pFlowLayout->GetDisplay()->MoveLineUpOrDown(NAVIGATE_UP, rp, lXCurReally, &cp, pfNotAtBOL, pfAtLogicalBOL);
            // Check that we are sane
            if( cp < 1 || cp >= pMarkup->Cch() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            if( !pFlowLayout)
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            {
                CLinePtr rpNew(pFlowLayout->GetDisplay());
                rpNew.RpSetCp(cp, *pfNotAtBOL);
                if (rpNew.RpGetIch() == 0)
                    hr = THR(LineStart(pFlowLayout, &cp, pfNotAtBOL, pfAtLogicalBOL, TRUE ));
                else if (rpNew.RpGetIch() == rpNew->_cch)
                    hr = THR(LineEnd(pFlowLayout, &cp, pfNotAtBOL, pfAtLogicalBOL, TRUE ));
                if (hr)
                    goto Cleanup;
            }

            if( eUnit == LAYOUT_MOVE_UNIT_PreviousLineEnd )
            {
                hr = THR( LineEnd(pFlowLayout, &cp, pfNotAtBOL, pfAtLogicalBOL, TRUE) );
                if( hr )
                    goto Cleanup;
            }
            
            // move pointer to new position
            hr = pPointerInternal->MoveToCp(cp, pMarkup);
            break;
        }
        
        case LAYOUT_MOVE_UNIT_NextLine:
        case LAYOUT_MOVE_UNIT_NextLineStart:
        {
            CLinePtr rp(pFlowLayout->GetDisplay());
            // Move down line up. This may cause the txt site to be different.

            if(! pFlowLayout->GetMultiLine() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            
            rp.RpSetCp( cp , *pfNotAtBOL );
            
            if( lXCurReally <= 0 )
                lXCurReally = 12;
            
            pFlowLayout = pFlowLayout->GetDisplay()->MoveLineUpOrDown(NAVIGATE_DOWN, rp, lXCurReally, &cp, pfNotAtBOL, pfAtLogicalBOL);
            // Check that we are sane
            if( cp < 1 || cp >= pMarkup->Cch() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            if( !pFlowLayout)
            {
                hr = E_FAIL;
                goto Cleanup;
            }                

            {
                CLinePtr rpNew(pFlowLayout->GetDisplay());
                rpNew.RpSetCp(cp, *pfNotAtBOL);
                if (rpNew.RpGetIch() == 0)
                    hr = THR(LineStart(pFlowLayout, &cp, pfNotAtBOL, pfAtLogicalBOL, TRUE));
                else if (rpNew.RpGetIch() == rpNew->_cch)
                    hr = THR(LineEnd(pFlowLayout, &cp, pfNotAtBOL, pfAtLogicalBOL, TRUE));
                if (hr)
                    goto Cleanup;
            }
            
            if( eUnit == LAYOUT_MOVE_UNIT_NextLineStart )
            {
                hr = THR( LineStart(pFlowLayout, &cp, pfNotAtBOL, pfAtLogicalBOL, TRUE) );
                if( hr )
                    goto Cleanup;
            }
            
            // move pointer to new position
            hr = pPointerInternal->MoveToCp(cp, pMarkup);

            break;
        }
        
        
        case LAYOUT_MOVE_UNIT_TopOfWindow:
        {
            //
            // BUGBUG: We need to be a bit more precise about where the
            // top of the first line is
            //
            
            // Go to top of window, not container...
            CPoint pt;
            pt.x = 12;
            pt.y = 12;
                        
            hr = THR( MoveMarkupPointerToPointEx( pt, pPointer, TRUE, pfNotAtBOL, pfAtLogicalBOL, NULL, FALSE ));

            goto Cleanup;
        }
        
        case LAYOUT_MOVE_UNIT_BottomOfWindow:
        {
            //
            // BUGBUG: More precision about the actual end of line needed
            //
            
            // Little harder, first have to calc the window size

            CSize szScreen;
            CPoint pt;
            
            //
            // Get the rect of the document's window
            //

            if ( !_view.IsActive() )
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            
            _view.GetViewSize( &szScreen );
            pt = szScreen.AsPoint();

            Assert( pt.x > 0 && pt.y > 0 );
            pt.x -= 12;
            pt.y -= 12;
            hr = THR( MoveMarkupPointerToPointEx( pt, pPointer, TRUE, pfNotAtBOL, pfAtLogicalBOL, NULL, FALSE ));

            goto Cleanup;                
        }
        
        default:
            return E_NOTIMPL;

    }

Cleanup:
    return hr;
}


HRESULT
CDoc::RegionFromMarkupPointers( IMarkupPointer *pPointerStart, 
                                IMarkupPointer *pPointerEnd, 
                                HRGN *phrgn)
{
    HRESULT                 hr;
    CMarkupPointer *        pStart  = NULL;
    CMarkupPointer *        pEnd    = NULL;
    RECT                    rcBounding = g_Zero.rc;
    CStackDataAry<RECT, 4>  aryRects(Mt(CDocRegionFromMarkupPointers_aryRects_pv));

    // check parameters
    if ( !pPointerStart || !pPointerEnd || !phrgn )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    // clean the out parameter
    *phrgn = NULL;

    // Get the CMarkupPointer values for the IMarkupPointer 
    // parameters we received
    hr = pPointerStart->QueryInterface( CLSID_CMarkupPointer, (void **)&pStart );
    if ( hr ) 
        goto Cleanup;

    hr = pPointerEnd->QueryInterface( CLSID_CMarkupPointer, (void **)&pEnd );
    if ( hr ) 
        goto Cleanup;

    // We better have these pointers.
    Assert( pStart );
    Assert( pEnd );

    // Get rectangles
    hr = RegionFromMarkupPointers( pStart, pEnd, &aryRects, &rcBounding );
    if ( hr )
        goto Cleanup;


//BUGBUG:   [FerhanE]
//          The code below has to change in order to return a region that contains
//          multiple rectangles. To do that, a region must be created for each 
//          member of the rect. array and combined with the complete region.
//
//          Current code only returns the region for the bounding rectangle.

    // Create and return BOUNDING region
    *phrgn = CreateRectRgn( rcBounding.left ,rcBounding.top,
                            rcBounding.right, rcBounding.bottom );

Cleanup:
    RRETURN( hr );
}


HRESULT
CDoc::RegionFromMarkupPointers( CMarkupPointer  *   pStart, 
                                CMarkupPointer  *   pEnd,
                                CDataAry<RECT>  *   paryRects, 
                                RECT            *   pBoundingRect = NULL )
{
    HRESULT         hr = S_OK;
    CTreeNode *     pTreeNode = NULL;
    CFlowLayout *   pFlowLayout = NULL;
    long            cpStart = 0;        // Starting cp.
    long            cpEnd = 0;          // Ending cp

    CElement *      pElem = NULL;

    if ( !pStart || !pEnd || !paryRects)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Calculate the starting and ending cps
    cpStart = pStart->GetCp();
    cpEnd = pEnd->GetCp();

    //Get the flow layout that the markup pointer is placed in.
    pTreeNode = pStart->CurrentScope(MPTR_SHOWSLAVE);
    if ( !pTreeNode )
        goto Error;

    pFlowLayout = GetFlowLayoutForSelection(pTreeNode);
    if ( !pFlowLayout )
        goto Error;

    // get the element we are in.
    pElem = pTreeNode->Element();
    
    // Get the rectangles.
    pFlowLayout->GetDisplay()->RegionFromElement(   pElem, 
                                                    paryRects,  
                                                    NULL, NULL, 
                                                    RFE_SELECTION|RFE_SCREENCOORD, 
                                                    cpStart, cpEnd, 
                                                    pBoundingRect );

Cleanup:
    RRETURN( hr );

Error:
    RRETURN( E_FAIL );
}


HRESULT 
CDoc::GetCurrentSelectionRenderingServices( 
    ISelectionRenderingServices ** ppSelRenSvc )
{
    HRESULT     hr      = S_OK;
    Assert( _pElemCurrent || _pElemEditContext );
    CMarkup *   pMarkup = GetCurrentMarkup();

    if ( pMarkup )
    {
        hr = THR( pMarkup->QueryInterface( IID_ISelectionRenderingServices, ( void**) ppSelRenSvc ));
    }
    else
        hr = E_FAIL;

    RRETURN ( hr ) ;
}



HRESULT 
CDoc::GetCurrentSelectionSegmentList( 
    ISegmentList ** ppSegment)
{
    HRESULT hr = S_OK;
    Assert( _pElemCurrent || _pElemEditContext );
    CMarkup *   pMarkup = GetCurrentMarkup();
                
    if ( pMarkup )
    {
        hr = THR( pMarkup->QueryInterface( IID_ISegmentList, ( void**) ppSegment ));
    }
    else
        hr = E_FAIL;
        
    RRETURN ( hr ) ;
}


CMarkup * 
CDoc::GetCurrentMarkup()
{
    CMarkup * pMarkup = NULL;
    
    if (_pElemEditContext)
    {
        pMarkup = _pElemEditContext->GetMarkup();
    }
    else if (_pElemCurrent)
    {
        if (_pElemCurrent->HasSlaveMarkupPtr())
        {
            pMarkup = _pElemCurrent->GetSlaveMarkupPtr();
        }
        else
        {
            pMarkup = _pElemCurrent->GetMarkup();
        }
    }

    return pMarkup;
}        

//+====================================================================================
//
// Method: GetElementBlockDirection
//
// Synopsis: Get the paragraph format of the current element
//           Used by mshtmled to know how to update the DIR attribute
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::GetElementBlockDirection ( IHTMLElement * pIHTMLElement, BSTR * pbstrDir )
{
    HRESULT hr;
    CElement * pElement = NULL;
    CTreeNode * pNode = NULL;

    if (!pIHTMLElement || !pbstrDir || !IsOwnerOf( pIHTMLElement ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIHTMLElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pNode = pElement->GetFirstBranch();

    if(!pNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(STRINGFROMENUM(styleDir, pNode->GetCascadedBlockDirection(), pbstrDir));
    
Cleanup:

    RRETURN( hr );
}

//+====================================================================================
//
// Method: SetElementBlockDirection
//
// Parameters: pIHTMLElement - the element which is current. 
//             eHTMLDir - htmlDirLeftToRight or htmlDirRightToLeft - direction to set
// Synopsis: Set the paragraph format of the current element if is has changed
//           Used by mshtmled to know how to update the DIR attribute
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::SetElementBlockDirection( IHTMLElement * pIHTMLElement, long eHTMLDir )
{
    CElement * pElement = NULL;
    CTreeNode * pNode = NULL;
    BSTR bstrCurrent = NULL;
    BSTR bstrNew = NULL;
    HRESULT hr;

    if (!pIHTMLElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIHTMLElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // If we are a text slave we need to change the direction on the master.
    if(pElement->Tag() != ETAG_TXTSLAVE)
    {
        pNode = pElement->GetFirstBranch();
    }
    else
    {
        CElement* pElementMaster = ((CTxtSlave*)pElement)->MarkupMaster();
        pNode = pElementMaster->GetFirstBranch();
    }

    if(!pNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }


    hr = THR(STRINGFROMENUM(styleDir, pNode->GetCascadedBlockDirection(), &bstrCurrent));

    hr = THR(s_enumdeschtmlDir.StringFromEnum(eHTMLDir, &bstrNew));
    if (hr)
        goto Cleanup;

    // if we don't have the same direction set it now
    if (StrCmpIW(bstrCurrent, bstrNew) != 0)
        hr = THR(pNode->Element()->put_dir(bstrNew));

Cleanup:
    FormsFreeString(bstrCurrent);
    FormsFreeString(bstrNew);
    RRETURN(hr);
}

//+====================================================================================
//
// Method: IsBidiEnabled
//
// Synopsis: Is this system has a bidi (right to left) keyboard enabled
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::IsBidiEnabled( BOOL *pfEnabled )
{
    HRESULT hr = S_OK;
    *pfEnabled = g_fBidiSupport;
        
    RRETURN( hr );
}

//+====================================================================================
//
// Method: ShouldObjectHaveBorder
//
// Synopsis: Check to see if the _fNoUIActivateInDesign flag is set.
//           Used by mshtmled to decide whether to create a UI Active border around this 
//           element ( presumed to be an Object tag  - as all the other rules are fixed ).
//
//------------------------------------------------------------------------------------

HRESULT 
CDoc::ShouldObjectHaveBorder  ( 
                IHTMLElement* pIElement, 
                BOOL* pfDrawBorder ) 
{
    HRESULT     hr;
    CElement  * pElement = NULL;
    CLayout* pLayout = NULL;
    
    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR_NOTRACE( pIElement->QueryInterface( CLSID_CElement,
            (void **) & pElement ) );
    if (hr)
        goto Cleanup;

    pLayout = pElement->GetUpdatedLayout();

    *pfDrawBorder = ! pLayout->_fNoUIActivateInDesign ;

Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsCaretVisible
//
// Synopsis: Checks for caret visibility - if there's no caret - return false.
//
//------------------------------------------------------------------------------------

BOOL
CDoc::IsCaretVisible( BOOL * pfPositioned )
{
    BOOL fVisible = FALSE;
    BOOL fPositioned = FALSE;
    
    if ( _pCaret )
    {
        _pCaret->IsVisible( & fVisible );
        fPositioned = _pCaret->IsPositioned();
    }
    
    if ( pfPositioned )
        *pfPositioned = fPositioned;
        
    return fVisible;        
}

HRESULT 
CDoc::GetCaret(
    IHTMLCaret ** ppCaret )
{
    HRESULT hr = S_OK;

    // bugbug (johnbed) : when CView comes into being, the caret will be
    // stored there and will require a view pointer as well.
    
    // lazily construct the caret...
    
    if( _pCaret == NULL )
    {
        _pCaret = new CCaret( this );
        
        if( _pCaret == NULL )
            goto Error;

        _pCaret->AddRef();      // Doc holds a ref to caret, released in passivate
        _pCaret->Init();        // Init the object
        _pCaret->Hide();        // Default to hidden - host or edit can show after move.
    }
    
    if (ppCaret)
    {
        hr = _pCaret->QueryInterface( IID_IHTMLCaret, (void **) ppCaret );
    }

    RRETURN( hr );

Error:
    return E_OUTOFMEMORY;
}


HRESULT 
CDoc::IsLayoutElement ( IHTMLElement * pIElement,
                            BOOL  * fResult ) 
{
    HRESULT     hr;
    CElement  * pElement = NULL;

    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR_NOTRACE( pIElement->QueryInterface( CLSID_CElement,
            (void **) & pElement ) );
    if (hr)
        goto Cleanup;

    if (pElement->Tag() == ETAG_TXTSLAVE)
    {
        pElement = pElement->MarkupMaster();
        if (!pElement)
            goto Cleanup;
    }
    *fResult = pElement->NeedsLayout();

Cleanup:
    RRETURN( hr );
}


HRESULT 
CDoc::IsBlockElement ( IHTMLElement * pIElement,
                           BOOL  * fResult ) 
{
    HRESULT     hr;
    CElement  * pElement = NULL;

    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR_NOTRACE( pIElement->QueryInterface( CLSID_CElement,
            (void **) & pElement ) );
    if (hr)
        goto Cleanup;

    *fResult = pElement->IsBlockElement();

Cleanup:
    RRETURN( hr );
}

HRESULT 
CDoc::IsEditableElement ( IHTMLElement * pIElement,
                          BOOL  * fResult ) 
{
    HRESULT     hr;
    CElement  * pElement = NULL;

    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = pIElement->QueryInterface( CLSID_CElement,
                                    (void **) & pElement );
    if (hr)
        goto Cleanup;

    *fResult = pElement->IsEditable();

Cleanup:
    RRETURN( hr );
}

HRESULT 
CDoc::IsContainerElement( IHTMLElement * pIElement,
                          BOOL         * pfContainer,
                          BOOL         * pfHTML) 
{
    HRESULT     hr;
    CElement  * pElement = NULL;

    if (!pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR_NOTRACE( pIElement->QueryInterface( CLSID_CElement,
            (void **) & pElement ) );
    if (hr)
        goto Cleanup;

    if (pfContainer)
        *pfContainer = pElement->HasFlag(TAGDESC_CONTAINER);
    
    if (pfHTML)
    {
        CTreeNode *pNode = pElement->GetFirstBranch();
    
        *pfHTML = pNode ? pNode->SupportsHtml() : FALSE;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CDoc::GetFlowElement ( IMarkupPointer * pIPointer,
                       IHTMLElement  ** ppIElement )
{
    HRESULT           hr;
    BOOL              fPositioned = FALSE;
    CFlowLayout     * pFlowLayout;
    CTreeNode       * pTreeNode = NULL;
    CMarkupPointer  * pMarkupPointer = NULL;
    CElement        * pElement = NULL;

    if (! pIPointer || !ppIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppIElement = NULL;

    hr = THR( pIPointer->IsPositioned( & fPositioned ) );
    if (hr)
        goto Cleanup;
    if (! fPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR_NOTRACE( pIPointer->QueryInterface(CLSID_CMarkupPointer, (void **) &pMarkupPointer) );    
    if (hr)
        goto Cleanup;

    pTreeNode = (pMarkupPointer->IsPositioned() ) ? pMarkupPointer->CurrentScope(MPTR_SHOWSLAVE) : NULL;
    if (! pTreeNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pFlowLayout = GetFlowLayoutForSelection(pTreeNode);
    
    if (!pFlowLayout)
    {
        hr = S_OK ;
        goto Cleanup;
    }
    
    pElement = pFlowLayout->ElementContent();

    Assert(pElement);

    if (! pElement)
        goto Cleanup;

    hr = THR_NOTRACE( pElement->QueryInterface( IID_IHTMLElement, (void **) ppIElement ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN1( hr, S_FALSE );
}

//+------------------------------------------------------------------------------------
//
// Funcion:   MapToCoorinateEnum()
//
// Synopsis:  Helper function which maps a given COORD_SYSTEM to its corresponding
//            CLayout::COORDINATE_SYSTEM
//           
//-------------------------------------------------------------------------------------

COORDINATE_SYSTEM
MapToCoordinateEnum ( COORD_SYSTEM eCoordSystem )
{   
    COORDINATE_SYSTEM eCoordinate;

    switch( eCoordSystem )
    {
    case COORD_SYSTEM_GLOBAL:   
        eCoordinate = COORDSYS_GLOBAL;
        break;

    case COORD_SYSTEM_PARENT: 
        eCoordinate = COORDSYS_PARENT;
        break;

    case COORD_SYSTEM_CONTAINER:
        eCoordinate = COORDSYS_CONTAINER;
        break;

    case COORD_SYSTEM_CONTENT:
        eCoordinate = COORDSYS_CONTENT;
        break;

    default:
        AssertSz( FALSE, "Invalid COORD_SYSTEM tag" );
        eCoordinate = COORDSYS_CONTENT;
    }


    return eCoordinate;
}

//+------------------------------------------------------------------------------------
//
// Method:    GetClientRect
//
// Synopsis:  Get the Client Rect of a given IHTMLElement.
//           
//-------------------------------------------------------------------------------------

HRESULT
CDoc::GetClientRect ( IHTMLElement* pIElement,
                       COORD_SYSTEM eSource,
                       RECT* pRect  )                        
{
    HRESULT         hr;
    CElement    *   pElement;
    CTreeNode   *   pNode;
    CFlowLayout *       pFlowLayout;
    
    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void** ) & pElement ));
    if ( hr )
        goto Cleanup;

    pNode = pElement->GetFirstBranch();
    if ( ! pNode )
    {
        AssertSz(0, "No longer in Tree");
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pFlowLayout = GetFlowLayoutForSelection( pNode );
    if ( ! pFlowLayout )
    {
        AssertSz(0, "Has no layout ");
        hr = E_FAIL;
        goto Cleanup;
    }
    pFlowLayout->GetClientRect( pRect , MapToCoordinateEnum( eSource ) );

    
Cleanup:
    RRETURN( hr );

}


//+------------------------------------------------------------------------------------
//
// Method:    GetContentRect
//
// Synopsis:  Get the ContentRect of a given IHTMLElement.
//           
//-------------------------------------------------------------------------------------

HRESULT
CDoc::GetContentRect ( IHTMLElement* pIElement,
                       COORD_SYSTEM eSource,
                       RECT* pRect  )                        
{
    HRESULT         hr;
    CElement    *   pElement;
    CTreeNode   *   pNode;
    CFlowLayout *       pFlowLayout;
    
    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void** ) & pElement ));
    if ( hr )
        goto Cleanup;

    pNode = pElement->GetFirstBranch();
    if ( ! pNode )
    {
        AssertSz(0, "No longer in Tree");
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pFlowLayout = GetFlowLayoutForSelection( pNode );
    if ( ! pFlowLayout )
    {
        AssertSz(0, "Has no layout ");
        hr = E_FAIL;
        goto Cleanup;
    }
    pFlowLayout->GetContentRect( (CRect*) pRect,  MapToCoordinateEnum( eSource ) );
    
Cleanup:
    RRETURN( hr );

}

//+------------------------------------------------------------------------------------
//
// Method:    TransformPoint
//
// Synopsis:  Exposes CLayout::TransformPoint() to MshtmlEd view ViewServices
//           
//-------------------------------------------------------------------------------------

HRESULT
CDoc::TransformPoint ( POINT        * pPoint,
                       COORD_SYSTEM eSource,
                       COORD_SYSTEM eDestination,
                       IHTMLElement * pIElement )                        
{
    HRESULT         hr;
    CElement    *   pElement;
    CTreeNode   *   pNode;
    CLayout *       pLayout;
    CPoint          cpoint( pPoint->x, pPoint->y );
    CFlowLayout*    pFlowLayout = NULL;
    const CFancyFormat * pFF;
    CTreeNode * pNodeParent = NULL;
    BOOL fRTL = FALSE;
    
    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void** ) & pElement ));
    if ( hr )
        goto Cleanup;

    pNode = pElement->GetFirstBranch();
    if ( ! pNode )
    {
        AssertSz(0, "No longer in Tree");
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pLayout = GetLayoutForSelection( pNode );
    if ( ! pLayout )
    {
        AssertSz(0, "Has no layout ");
        hr = E_FAIL;
        goto Cleanup;
    }
    
    pLayout->TransformPoint( &cpoint, 
                                 MapToCoordinateEnum( eSource ), 
                                 MapToCoordinateEnum( eDestination ),
                                 NULL );
    pPoint->x = cpoint.x;
    pPoint->y = cpoint.y;

    // (paulnel) Is our parent right to left? If so, we need to convert the offset left
    // amount, that will be a negative value from the parent's top right, to a 
    // positive offset from the parent's top left. To do this we will
    // use the formula "left += parent.width" or "left = parent.left - left"
    if (pNode->GetUpdatedNearestLayout())
    {
        pNodeParent = (pNode->Element()->NeedsLayout())
                            ? pNode->ZParentBranch()
                            : pNode->GetUpdatedNearestLayoutNode();

        fRTL = (pNodeParent != NULL && pNodeParent->GetCharFormat()->_fRTL);
    }
    pFlowLayout = GetFlowLayoutForSelection( pNode );
    if(fRTL)
    {
        pFF = pNode->GetFancyFormat();

        // We don't want to assume that only two types of positioning are used
        switch(pFF->_bPositionType)
        {
        case stylePositionrelative:
            pPoint->x += pFlowLayout->GetWidth();
            break;

        case stylePositionabsolute:
            {
                // get the parent's node. If it is RTL then find the
                // distance from point to top/left
                CPoint ptParentTopLeft;
                pLayout->GetParentTopLeft(&ptParentTopLeft);
                pPoint->x = pPoint->x - ptParentTopLeft.x;
            }
            break;
        }
    }

Cleanup:
    RRETURN( hr );

}

//+====================================================================================
//
// Method:GetElementFromCookie
//
// Synopsis: Return the IHTMLElement for an "element cookie"
//
//  The "element cookie" is the CTreeNode which was passed to the edit dll, but not exposed
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::GetElementFromCookie( void* elementCookie, IHTMLElement** ppElement )
{
    HRESULT hr = S_OK;

    CTreeNode* pTreeNode ;
    CElement* pElement;
    
    if ( elementCookie )
    {
        pTreeNode = static_cast<CTreeNode*>( elementCookie );   
        pElement = pTreeNode->Element();

        if ( pElement->_etag == ETAG_TXTSLAVE )
        {
            pElement= pElement->MarkupMaster();
        } 

        hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void**) ppElement ) );
    }
    else
        hr = E_INVALIDARG;
        
    RRETURN ( hr );
} 


HRESULT 
CDoc::GetViewHWND(
    HWND * pHWND )
{
    RRETURN( GetWindow( pHWND ) );
}

HRESULT 
CDoc::GetActiveIMM(
    IActiveIMMApp** ppActiveIMM)
{
#ifndef NO_IME
    Assert(ppActiveIMM);

    extern IActiveIMMApp * GetActiveIMM();

    *ppActiveIMM = GetActiveIMM();
    if (*ppActiveIMM)
        (*ppActiveIMM)->AddRef();

    return S_OK;
#else
    return E_FAIL;
#endif
}

HRESULT
CDoc::IsRtfConverterEnabled(BOOL *pfEnabled)
{
#ifndef NO_RTF
    *pfEnabled = RtfConverterEnabled();
#else
    *pfEnabled = NULL;
#endif

    return S_OK;
}

HRESULT
CDoc::DoTheDarnPasteHTML (
    IMarkupPointer * pIPointerStart,
    IMarkupPointer * pIPointerFinish,
    HGLOBAL          hGlobalHtml )
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pPointerStart;
    CMarkupPointer * pPointerFinish;

    Assert( pIPointerStart && hGlobalHtml );

    hr = THR(
        pIPointerStart->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerStart ) );

    if (hr)
        goto Cleanup;

    if (!pIPointerFinish)
        pIPointerFinish = pIPointerStart;
    
    hr = THR(
        pIPointerFinish->QueryInterface(
            CLSID_CMarkupPointer, (void **) & pPointerFinish ) );

    if (hr)
        goto Cleanup;
    
    EnsureLogicalOrder ( pPointerStart, pPointerFinish );

    HRESULT HandleUIPasteHTML ( CMarkupPointer *, CMarkupPointer *, HGLOBAL, BOOL );

    hr = THR( HandleUIPasteHTML( pPointerStart, pPointerFinish, hGlobalHtml, FALSE ) );

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::ConvertRTFToHTML(LPOLESTR pszRtf, HGLOBAL* phglobalHTML)
{
    HRESULT hr = S_OK;

    CRtfToHtmlConverter* pcnv = new CRtfToHtmlConverter(NULL);
    if (!pcnv)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pcnv->StringRtftoStringHtml((LPSTR) pszRtf, phglobalHTML));

    delete pcnv;

Cleanup:
    return hr;
}


//+====================================================================================
//
// Method:InflateBlockElement
//
// Synopsis: Sets break on empty flag
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::InflateBlockElement( IHTMLElement * pElem )
{
    HRESULT   hr;
    CElement* pElement;

    hr = pElem->QueryInterface(CLSID_CElement, (LPVOID *)&pElement);
    if (   SUCCEEDED(hr)
        && !pElement->_fBreakOnEmpty
       )
    {
        CNotification nf;
        CMarkup      *pMarkupNotify;

        pElement->_fBreakOnEmpty  = TRUE;

        pMarkupNotify = pElement->GetMarkup();
        if (!pMarkupNotify)
            goto Cleanup;

        nf.ElementResize(pElement, 0);
        pMarkupNotify->Notify(&nf);
    }

Cleanup:
    RRETURN ( hr );
} 

//+====================================================================================
//
// Method:IsInflatedBlockElement
//
// Synopsis: Returns true if the break on empty flag is set
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::IsInflatedBlockElement( IHTMLElement * pElem, BOOL * pfInflated )
{
    HRESULT   hr = E_FAIL;
    CElement* pElement;
    BOOL fInflated = FALSE;

    Assert( pElem && pfInflated );

    if( pElem == NULL || pfInflated == NULL )
        goto Cleanup;

    hr = pElem->QueryInterface(CLSID_CElement, (LPVOID *)&pElement);

    if( hr || pElement == NULL )
        goto Cleanup;
        
    fInflated = pElement->_fBreakOnEmpty;
    hr = S_OK;
    
Cleanup:

    if( pfInflated )
        *pfInflated = fInflated;
        
    RRETURN ( hr );
} 


//+====================================================================================
//
// Method:IsMultiLineFlowElement
//
// Synopsis: Sets break on empty flag
//
//------------------------------------------------------------------------------------

HRESULT 
CDoc::IsMultiLineFlowElement( IHTMLElement * pIElement,
                              BOOL         * pfMultiLine) 
{
    HRESULT       hr;
    CElement      *pElement;
    CFlowLayout   *pLayout;

    *pfMultiLine = FALSE;

    hr = pIElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement);
    if (SUCCEEDED(hr))
    {
        pLayout = GetFlowLayoutForSelection(pElement->GetFirstBranch());
        if (pLayout)
            *pfMultiLine = pLayout->GetMultiLine();
    }
    
    RRETURN(hr);
}


//+====================================================================================
//
// Method:GetElementAttributeCount
//
// Synopsis: gets the attribute count
//
//------------------------------------------------------------------------------------

HRESULT 
CDoc::GetElementAttributeCount( IHTMLElement * pIElement,
                                UINT         * pCount) 
{
    
    HRESULT     hr;
    CElement    *pElement;
    AAINDEX     aaix = AA_IDX_UNKNOWN;
    CAttrArray  *pAttrArray;
    
    *pCount = 0;

    hr = pIElement->QueryInterface(CLSID_CElement, (LPVOID *)&pElement);
    if (SUCCEEDED(hr))
    {
        //
        // Count basic attributes
        //
        
        while ( (aaix = pElement->FindAAType(CAttrValue::AA_Attribute, aaix) ) 
                    != AA_IDX_UNKNOWN)
        {
            if (pElement->GetDispIDAt(aaix) != DISPID_CElement_uniqueName)
                *pCount += 1;
        }

        //
        // Count inline style attributes
        //

        pAttrArray = pElement->GetInLineStyleAttrArray();
        if (pAttrArray != NULL && pAttrArray->HasAnyAttribute(FALSE))
        {
    		*pCount += 1;
        }
        
    }
    
    RRETURN(hr);
}

HRESULT
CDoc::MergeAttributes(IHTMLElement *pIHTMLElementMergeTarget, IHTMLElement *pIHTMLElementMergeSrc, BOOL fCopyId)
{
    HRESULT hr = E_POINTER;
    CElement *pelMergeTarget;

    if (!pIHTMLElementMergeTarget || !pIHTMLElementMergeSrc)
        goto Cleanup;
    
    hr = THR(pIHTMLElementMergeTarget->QueryInterface(CLSID_CElement, (void **)&pelMergeTarget));
    if (hr)
        goto Cleanup;

    hr = THR(pelMergeTarget->MergeAttributesInternal(pIHTMLElementMergeSrc, FALSE, fCopyId));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+====================================================================================
//
// Method: FireOnBeforeEditFocus
//
// Synopsis: fire the OnBefore UI Activate Event.
//
//------------------------------------------------------------------------------------


HRESULT 
CDoc::FireOnBeforeEditFocus(IHTMLElement *pINextActiveElem, BOOL *pfRet)
{
    HRESULT hr = E_POINTER;
    CElement *pNextActiveElem;
    VARIANT_BOOL fRet = VB_TRUE;

    if ( ! _fDesignMode )
    {
        //
        // We fire this at edit time only. Tell mshtmled.dll its ok to do what it wants
        // to do (ie position caret) and go ahead.
        //
        fRet = VB_TRUE ;
        goto Cleanup;
    }
    
    if (!pINextActiveElem)
        goto Cleanup;
    
    hr = THR(pINextActiveElem->QueryInterface(CLSID_CElement, (void **)&pNextActiveElem));
    if (hr)
        goto Cleanup;
    

    fRet = (VARIANT_BOOL)pNextActiveElem->BubbleCancelableEvent(NULL, 
                                                    0, 
                                                    DISPID_EVMETH_ONBEFOREEDITFOCUS, 
                                                    DISPID_EVPROP_ONBEFOREEDITFOCUS, 
                                                    _T("onbeforeeditfocus"), 
                                                    (BYTE *)VTS_NONE);


#if DBG == 1
    if ( IsTagEnabled(tagEditDisableEditFocus))
    {
        fRet = VB_FALSE ; // FOR TESTING
    }
#endif

Cleanup:

    if (pfRet)
        *pfRet = fRet;

    RRETURN(hr);
}

//+====================================================================================
//
// Method: FireOnSelectStartFromMessage
//
// Synopsis: Allows external Selection Managers to Fire_onDragStart into Elements.
//
//------------------------------------------------------------------------------------


HRESULT 
CDoc::FireOnSelectStart(
        IHTMLElement* pIElement)
{
    CElement * pElement = NULL;
    CTreeNode* pNode = NULL;

    IGNORE_HR(pIElement->QueryInterface(CLSID_CElement, (void **) &pElement));

        
    if( !pElement)
        return( E_FAIL );
    else
    {
        pNode = pElement->GetFirstBranch();
        if ( pNode )
            return( THR( pNode->Element()->Fire_onselectstart(pNode)));
        else
            return E_FAIL ;
    }            
}


HRESULT
CDoc::FireCancelableEvent(
    IHTMLElement * pIElement,
    LONG           dispidMethod,
    LONG           dispidProp,
    BSTR           bstrEventType,
    BOOL *         pfResult)
{
    HRESULT hr;
    CElement * pElement;
    BOOL fResult;

    hr = THR(pIElement->QueryInterface(CLSID_CElement, (void **) &pElement));
    if (hr)
        goto Cleanup;

    fResult = pElement->BubbleCancelableEvent(NULL, 0, dispidMethod, dispidProp, bstrEventType, (BYTE *) VTS_NONE);

    if (pfResult)
        *pfResult = fResult;

Cleanup:
    RRETURN(hr);
}

//+====================================================================================
//
// Method: AddElementAdorner
//
// Synopsis: Create a CElementAdorner, and add it to our array of adorners.
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::AddElementAdorner( 
    IHTMLElement* pIElement,
    IElementAdorner * pIElementAdorner,
    DWORD_PTR* ppeCookie)
{
    HRESULT hr = S_OK;
    CElementAdorner * pAdorner = NULL;
    CElement* pElement = NULL ;

    if (    !pIElement
        ||  !pIElementAdorner
        ||  !ppeCookie)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );
    if ( hr )
        goto Cleanup;

    if ( pElement->_etag == ETAG_TXTSLAVE )
    {
        pElement = pElement->MarkupMaster();
    }
     
    if ( !_view.IsActive() )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pAdorner = DYNCAST( CElementAdorner, _view.CreateAdorner( pElement ) );

    if ( !pAdorner )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pAdorner->SetSite( pIElementAdorner );
    pAdorner->AddRef();

    *ppeCookie = (DWORD_PTR) pAdorner ;

Cleanup:

    RRETURN ( hr );    
}


//+====================================================================================
//
// Method: RemoveElementAdorner
//
// Synopsis: Remove an ElementAdorner from our array - using the cookie we're given.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::RemoveElementAdorner( 
    DWORD_PTR peCookie)
{
    HRESULT hr;

    if ( !peCookie )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CElementAdorner* pAdorner = (CElementAdorner*) (peCookie);
        CView* pView = pAdorner->GetView();
        if ( ( pView ) && ( _ulRefs != ULREF_IN_DESTRUCTOR ) && ( pView->Doc() != NULL ) )
        {
            Verify(pView->OpenView());
            pView->RemoveAdorner( pAdorner );
        }
        pAdorner->Release();
        hr = S_OK;
    }

    RRETURN ( hr );

}


//+====================================================================================
//
// Method: GetElementAdornerBounds
//
// Synopsis: Get the bounds of this adorner - in Global coordinates.
//
//  HACK - This should be part of an interface - so the 'AdornerClient' does so directly.
//
//
//------------------------------------------------------------------------------------

HRESULT 
CDoc::GetElementAdornerBounds( 
    DWORD_PTR peAdornerCookie,
    RECT* pRectBounds )
{
    CElementAdorner* pAdorner;
    HRESULT hr;

    if (    !peAdornerCookie
        ||  !pRectBounds )
    {
        hr = E_INVALIDARG;
    }

    else
    {
        pAdorner = (CElementAdorner*) peAdornerCookie;
        Assert( pAdorner );
        Assert( pAdorner->GetView()->IsValidAdorner( pAdorner ) );

        pAdorner->GetBounds( pRectBounds );

        hr = S_OK;
    }

    RRETURN ( hr );
}


HRESULT
CDoc::IsNoScopeElement( IHTMLElement* pIElement, BOOL* pfNoScope )
{
    HRESULT hr = S_OK;
    CElement* pElement = NULL;
    
    if ( pIElement )
    {
        hr = THR( pIElement->QueryInterface( CLSID_CElement, (void** ) & pElement ));
        if ( hr )
            goto Cleanup;

        if ( pfNoScope )
            *pfNoScope = pElement->IsNoScope();
    }
    else
    {
        hr = E_INVALIDARG;
    }
Cleanup:

    RRETURN ( hr );
}

HRESULT
CDoc::GetOuterContainer( 
    IHTMLElement* pIElement, 
    IHTMLElement** ppIOuterElement, 
    BOOL fIgnoreOutermostContainer, 
    BOOL * pfHitContainer    )
{
    HRESULT hr = S_OK;
    CElement* pElement = NULL ;
    CTreeNode* pCurNode  ;
    CElement* pOuterElement  = NULL;
    CElement* pCurElement = NULL;
    
    //
    // If we aren't given an element - then we'll return the body as outer element
    //
    if ( pIElement )
    {
        hr = THR( pIElement->QueryInterface( CLSID_CElement, (void** ) & pElement ));
        if ( hr )
            goto Cleanup;

        pCurNode = pElement->GetFirstBranch();

        while ( pCurNode )
        {
            pCurElement = pCurNode->Element();
            if ( pCurElement->HasFlag(TAGDESC_CONTAINER) )
            {
                pOuterElement = pCurNode->Element();
                break;
            }
            pCurNode = pCurNode->Parent();
            pCurNode = pCurNode->Parent();
        }
    
        if ( pOuterElement && ( pOuterElement->_etag == ETAG_BODY ) && fIgnoreOutermostContainer ) 
            pOuterElement = NULL ;
    }
    else
    {
        pOuterElement = PrimaryMarkup()->GetElementClient();
    }
    
    if ( ( ppIOuterElement ) && ( pOuterElement ) )
    {
        hr = THR( pOuterElement->QueryInterface( IID_IHTMLElement, (void**) ppIOuterElement ));
    }

    if ( pfHitContainer )
    {
        *pfHitContainer = ( pOuterElement != NULL ) ;
    }
    
Cleanup:
    RRETURN ( hr );
}


HRESULT 
CDoc::ScrollElement(
    IHTMLElement*       pIElement,
    LONG                lPercentToScroll,
    POINT*              pScrollDelta )
{
    // TODO (johnbed) now we only scroll vertically, in fact, we ignore eDirection
    // all together. Fix this some day.

    HRESULT hr = S_OK;
    CElement * pElement = NULL;
    CTreeNode * pNode = NULL;
    CFlowLayout * pFlowLayout = NULL;
    CDispNode * pDispNode = NULL;
    CDispScroller * pScroller = NULL;
    SIZE szBefore;
    SIZE szAfter;
    
    pScrollDelta->x = 0;
    pScrollDelta->y = 0;
    
    hr = pIElement->QueryInterface( CLSID_CElement, (void **)&pElement );
    if (hr)
        goto Cleanup;

    pNode = pElement->GetFirstBranch();
    pFlowLayout = GetFlowLayoutForSelection( pNode );

    pDispNode = pFlowLayout->GetElementDispNode();

    if( ! pDispNode->IsScroller() )
        goto Cleanup; // nothing to do

    pScroller = (CDispScroller *) pDispNode;
    // For some reason, compiler barfs here, but this is the
    // "safe" thing to do.
    // pScroller = dynamic_cast<CDispScroller *>( pDispNode );
    if( pScroller == NULL )
        goto Error;
        
    pScroller->GetScrollOffset( &szBefore );
    pFlowLayout->ScrollByPercent( CSize( 0, lPercentToScroll ));
    pScroller->GetScrollOffset( &szAfter );
    pScrollDelta->x = szAfter.cx - szBefore.cx;
    pScrollDelta->y = szAfter.cy - szBefore.cy;
    goto Cleanup;

Error:
    hr = E_FAIL;
Cleanup:
    RRETURN( hr );
}

HRESULT
CDoc::GetScrollingElement(
    IMarkupPointer*     pPosition,
    IHTMLElement*       pIBoundary,
    IHTMLElement**      ppElement )
{
    HRESULT hr = E_FAIL;
    Assert( ppElement != NULL );
    
    CMarkupPointer * pPtr = NULL;
    CElement * pBoundary = NULL;
    CElement * pTest = NULL;
    CTreeNode * pNode = NULL;
    CLayout * pLayout = NULL;
    CDispNode * pDispNode = NULL;
    BOOL fDone = FALSE;

    *ppElement = NULL;
    
    hr = pPosition->QueryInterface( CLSID_CMarkupPointer, (void **)&pPtr );
    if (hr)
        goto Cleanup;

    hr = pIBoundary->QueryInterface( CLSID_CElement, (void **)&pBoundary );
    if (hr)
        goto Cleanup;

    pNode = pPtr->CurrentScope(MPTR_SHOWSLAVE);

    while( pNode && ! fDone )
    {
        pTest = pNode->Element();
        if( pTest == NULL )
            goto Cleanup;
            
        pLayout = pTest->GetUpdatedLayout();
        pDispNode = (pLayout == NULL ? NULL : pLayout->GetElementDispNode());

        if( pDispNode && pDispNode->IsScroller() )
        {
            hr = THR( pNode->Element()->QueryInterface( IID_IHTMLElement, (void **) ppElement ));
            if( hr )
            {
                *ppElement = NULL;
                goto Cleanup;
            }

            fDone = TRUE;
        }

        if( pTest == pBoundary ) // This may not be the right way to do this comparison
            fDone = TRUE;

        pNode = pNode->Parent();
    }

Cleanup:
    return( hr );
}

// declared in formkrnl.hxx

CTreeNode *
CDoc::GetNodeFromPoint(
    const POINT &   pt,
    BOOL            fGlobalCoordinates,
    POINT *         pptContent /* = NULL */,
    LONG *          plCpMaybe /* = NULL */ ,
    BOOL*           pfEmptySpace)
{   
    CTreeNode *         pTreeNode = NULL;
    
    POINT               ptContent;
    CDispNode *         pDispNode = NULL;
    COORDINATE_SYSTEM   coordSys;
    HITTESTRESULTS      HTRslts;
    HTC                 htcResult = HTC_NO;
    DWORD               dwHTFlags = HT_VIRTUALHITTEST |       // Turn on virtual hit testing
                                    HT_NOGRABTEST |           // Ignore grab handles if they exist
                                    HT_IGNORESCROLL;          // Ignore testing scroll bars                                  

    ptContent = g_Zero.pt;

    if( fGlobalCoordinates )
        coordSys = COORDSYS_GLOBAL;    
    else
        coordSys = COORDSYS_CONTENT;

    //
    //  Do a hit test to figure out what node would be hit by this point.
    //  I know this seems like a lot of work to just get the TreeNode,
    //  but we have to do this. Also, we can't trust the cp returned in
    //  HTRslts. Some day, perhaps we can.
    //

    if( !_view.IsActive() ) 
        goto Cleanup;

    htcResult = _view.HitTestPoint( pt, 
                                    coordSys,
                                    NULL,
                                    dwHTFlags, 
                                    &HTRslts,
                                    &pTreeNode, 
                                    ptContent, 
                                    &pDispNode );
Cleanup:
    if( pptContent != NULL )
    {
        pptContent->x = ptContent.x;
        pptContent->y = ptContent.y;
    }

    if( plCpMaybe != NULL )
    {
        *plCpMaybe = HTRslts._cpHit;
    }
    
    if ( pfEmptySpace )
    {
        *pfEmptySpace = HTRslts._fWantArrow;
    }
        return pTreeNode;
}


CFlowLayout *
CDoc::GetFlowLayoutForSelection( CTreeNode * pNode )
{
    if ( ! pNode )
        return NULL;
        
    CMarkup *       pMarkup;
    CFlowLayout *   pFlowLayout = pNode->GetFlowLayout();

    // Handle slave markups
    if (!pFlowLayout)
    {
        pMarkup = pNode->GetMarkup();
        if (pMarkup->Master())
        {
            pFlowLayout = pMarkup->Master()->HasFlowLayout();
        }
    }
    return pFlowLayout;
}

//+====================================================================================
//
// Method: GetLayoutForSelection
//
// Synopsis: Get the Layout for a TreeNode - compensating for Master/Slaveness
//
//------------------------------------------------------------------------------------


CLayout *
CDoc::GetLayoutForSelection( CTreeNode * pNode )
{
    CMarkup *       pMarkup;
    CLayout *   pLayout = pNode->GetUpdatedNearestLayout();

    // Handle slave markups
    if (!pLayout)
    {
        pMarkup = pNode->GetMarkup();
        if (pMarkup->Master())
        {
            pLayout = pMarkup->Master()->GetUpdatedLayout();
        }
    }
    return pLayout;
}

//+====================================================================================
//
// Method: StartHTMLEditorDblClickTimer
//
// Synopsis: Sets the MouseMove Timer interval for WM_TIMER
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::SetHTMLEditorMouseMoveTimer()
{
    HRESULT hr = S_OK;

    SetMouseMoveTimer( g_iDragDelay );
    
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: StartHTMLEditorDblClickTimer
//
// Synopsis: Starts the DoubleClick Timer for the selection tracker. Used by ProcessFollowUpAction
//          Sets up OnEditDblClkTimer
//           to recieve ticks from WM_TIMER.
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::StartHTMLEditorDblClickTimer()
{
    HRESULT hr = S_OK;

    Assert( ! _fInEditTimer );

    hr = THR(FormsSetTimer(
        this,
        ONTICK_METHOD(CDoc, OnEditDblClkTimer, oneditdblclktimer),
        SELTRACK_DBLCLK_TIMERID,
        GetDoubleClickTime()));

    
    WHEN_DBG( _fInEditTimer= TRUE ); 

    TraceTag((tagSelectionTimer, "StartHTMLEditor DblClickTImer"));
    RRETURN ( hr );
}

//+====================================================================================
//
// Method:StopHTMLEditorDblClickTimer
//
// Synopsis: Stops the DoubleClickTimer for the Selection Tracker
//
//------------------------------------------------------------------------------------



HRESULT
CDoc::StopHTMLEditorDblClickTimer()
{
    HRESULT hr = S_OK;
    
    hr = THR( FormsKillTimer( this, SELTRACK_DBLCLK_TIMERID  ) );
    TraceTag((tagSelectionTimer, "StopHTMLEditor DblClickTImer"));

    WHEN_DBG( _fInEditTimer = FALSE );
    RRETURN1 ( hr, S_FALSE ); // False if there was no timer.
}


HRESULT
CDoc::HTMLEditorTakeCapture( )
{
    HRESULT hr = S_OK;

    Assert( State() >= OS_INPLACE);
    Assert( ! _fInEditCapture );
    
    SetMouseCapture( MOUSECAPTURE_METHOD( CDoc, HandleEditMessageCapture, handleeditmessagecapture),
                     (void *) this,
                     FALSE);

    _fInEditCapture = TRUE ;
    _fVirtualHitTest = TRUE;
    RRETURN ( hr );
}



HRESULT
CDoc::HTMLEditorReleaseCapture( )
{
    HRESULT hr = S_OK;

    Assert( _fInEditCapture );

    //
    // Set the bits indicating we no longer have capture.
    // So that HandleEditCapture - will NOT pass the WM_CAPTURECHANGED back to the editor.
    //

    _fInEditCapture = FALSE ; 
    _fVirtualHitTest = FALSE; 
    SetMouseCapture( NULL, NULL, TRUE );

    RRETURN ( hr );
}

//+====================================================================================
//
// Method:EnsureEditContext
//
// Synopsis: External method to Ensure Edit Context is valid. 
//
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::EnsureEditContext( IMarkupPointer* pIPointer )
{
    HRESULT hr = S_OK;
    IHTMLElement* pIHTMLElement = NULL;
    CElement* pCurElement = NULL;


    hr = THR( CurrentScopeOrSlave( pIPointer, & pIHTMLElement ));
    if ( hr )
        goto Cleanup;

    if ( pIHTMLElement )
    {
        hr = THR( pIHTMLElement->QueryInterface( CLSID_CElement, (void**) & pCurElement ));
        if ( hr )
            goto Cleanup;

        hr = THR( EnsureEditContext( pCurElement, TRUE ));
    }
    else
        hr = E_FAIL;
Cleanup:
    ReleaseInterface( pIHTMLElement );
    
    RRETURN( hr );
}

//+====================================================================================
//
// Method: EnsureEditContext
//
// Synopsis: Ensure we have a valid Edit Context, by finding what the edit context should
//           be and setting currency on it.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::EnsureEditContext( CElement* pElement, BOOL fDrillIn )
{
    HRESULT hr = S_OK;
    CElement* pEditContextElement = NULL ;

    GetEditContext( pElement,
                    & pEditContextElement,
                    NULL,
                    NULL,
                    fDrillIn );

    Assert( pEditContextElement );

    //
    // BUGBUG - _pElemEditContext should always be in sync with _pElemCurrent
    // however there are some cases where _pElemCurrent gets set to the root and testing 
    // for pEditContextEleemnt == _pElemEditContext is not sufficient
    //
    if ( pEditContextElement != _pElemEditContext || 
        ( pEditContextElement != _pElemCurrent &&
          pEditContextElement->MarkupMaster() != _pElemCurrent ))
    {
        if ( _pInPlace )
        {    
            hr = THR( pEditContextElement->BecomeCurrent(0, NULL, NULL));
        }
        else
        {
            _pElemEditContext = pEditContextElement;
        }
    }
    
    if ( ! pEditContextElement->IsEditable() || ! _pInPlace)
    {
        //
        // The Edit Context is not Editable, and calling becomeCurrent is NOT sufficient to set
        // the EditContext. This is valid if we were called in browse mode.
        // We call SetEditContext directly
        //
        hr =  THR( SetEditContext(( pEditContextElement->HasSlaveMarkupPtr() ? 
                                         pEditContextElement->GetSlaveMarkupPtr()->FirstElement() : 
                                         pEditContextElement ), 
                                         TRUE, 
                                         FALSE, 
                                         fDrillIn ));
        
    }
    
    //
    // There are cases where become current may fail. A common one is 
    // from forced currency changes (via script) - during a focus change
    // change in currency.
    // 
    // During focus changes - we set the ELEMENTLOCK_FOCUS flag. Any forced 
    // currency changes via calls to become current - will fail - until
    // the end of the focus change has occurred.
    //
    // A common example of this is the Find Dialog's use of the select method
    // during a focus change. This is all fine - but will cause the assert below to fire
    // - so I've added the ElementLockFocus check
    //
    Assert( _pElemEditContext );
    AssertSz( ( !_pInPlace || 
                ( _pElemEditContext == _pElemCurrent ) || 
                ( pEditContextElement->TestLock(CElement::ELEMENTLOCK_FOCUS ))||
                ( pEditContextElement->MarkupMaster()->TestLock(CElement::ELEMENTLOCK_FOCUS )) || 
                ( _pElemEditContext && _pElemEditContext->MarkupMaster() == _pElemCurrent) ), 
                "Currency and context do not match" );               
    
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: AdjustEditContext
//
// Synopsis: Adjust the Markup Pointers to completely enclose the allowed editing region.
//           and Return the Element "I really should be editing" 
//
//          During editing - we never bypass this enclosed region    
//
// Rules for the Element "I really should be editing" 
//
// If the Element has a slave markup ptr - the elemnet I really want is a slave
//
// If we are in Browse Mode - move the pointers inside the outermost _fContainer (BODY, HTMLAREA, BUTTON)
// 
// Else if we are in edit mode - move the pointers until they enclose the outermost editable element.
// 
// BUGBUG - this should be in edit dll as it is some dancing of pointers.
// But we want to hide "inner tree ness" hence it must be here.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::AdjustEditContext( 
                            CElement* pStartElement, 
                            CElement** ppEditThisElement, 
                            IMarkupPointer* pIStart,
                            IMarkupPointer* pIEnd , 
                            BOOL * pfEditThisEditable,
                            BOOL * pfEditParentEditable,
                            BOOL * pfNoScopeElement ,
                            BOOL fSetCurrencyIfInvalid /* = FALSE*/,
                            BOOL fSetElemEditContext /* = TRUE */,
                            BOOL fDrillingIn /* = FALSE*/)
{
    HRESULT hr = S_OK;

    CElement* pEditableElement = NULL;

    //
    // Get the Editable Element 
    //
    hr = THR ( GetEditContext( pStartElement,
                                &pEditableElement ,
                                pIStart,
                                pIEnd,
                                fDrillingIn,
                                pfEditThisEditable,
                                pfEditParentEditable,
                                pfNoScopeElement ));
    if ( hr )
        goto Cleanup;
        
    // Clear the old selection, if the edit context is changing
    if ( (_pElemEditContext != pEditableElement) && fSetElemEditContext )
    {
        hr =  THR(NotifySelection(SELECT_NOTIFY_DESTROY_SELECTION, NULL));
        if (hr)
            goto Cleanup;
    }

    if ( fSetElemEditContext )
    {
        _pElemEditContext = pEditableElement;
        Assert( _pElemEditContext->_etag != ETAG_TABLE && !IsTablePart( _pElemEditContext) );
    }    

    if ( fSetCurrencyIfInvalid && ( _pElemEditContext != _pElemCurrent ))
    {
        hr = THR( _pElemEditContext->BecomeCurrent(0) );
    }            

    if ( ppEditThisElement )
    {
        *ppEditThisElement = pEditableElement;
    }
Cleanup:

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: GetEditContext
//
// Synopsis: External Routine for getting the Edit Context.
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::GetEditContext( 
                            IHTMLElement* pIStartElement, 
                            IHTMLElement** ppIEditThisElement, 
                            IMarkupPointer* pIStart, 
                            IMarkupPointer* pIEnd, 
                            BOOL fDrillingIn,  
                            BOOL * pfEditThisEditable ,
                            BOOL * pfEditParentEditable ,
                            BOOL * pfNoScopeElement)
{
    HRESULT hr = S_OK;
    CElement* pElementInternal = NULL;
    CElement* pOuterElement = NULL;
    
    hr = THR( pIStartElement->QueryInterface( CLSID_CElement , ( void**) & pElementInternal ));
    if ( hr )
        goto Cleanup;

    hr = THR( GetEditContext(
                                pElementInternal,
                                & pOuterElement,
                                pIStart,
                                pIEnd,
                                fDrillingIn,
                                pfEditThisEditable,
                                pfEditParentEditable,
                                pfNoScopeElement ));
    if ( hr )
        goto Cleanup;

    if ( ( !hr ) && ( ppIEditThisElement ) )
    {
        Assert( pOuterElement );
        hr = THR( pOuterElement->QueryInterface( IID_IHTMLElement, (void**) ppIEditThisElement ));
    }

Cleanup:

    RRETURN( hr );   
}

//+====================================================================================
//
// Method: GetEditContext
//
// Synopsis: Get the element that is our "edit context".
//           Return the Markup Pointers that completely enclose the allowed editing region.
//           and Return the Element "I really should be editing" 
//
//          During editing - we never bypass this enclosed region    
//
// Rules for the Element "I really should be editing" 
//
// If the Element has a slave markup ptr - the elemnet I really want is a slave
//
// If we are in Browse Mode - move the pointers inside the outermost _fContainer (BODY, HTMLAREA, BUTTON)
// 
// Else if we are in edit mode - move the pointers until they enclose the outermost editable element.
// 
// BUGBUG - this should be in edit dll as it is some dancing of pointers.
// But we want to hide "inner tree ness" hence it must be here.
//
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
//
// Meaning of flags on Get Edit Context
//
// fDrillingIn - we are "drilling in". In PumpMessage - we arn't drilling in, in setEditContextRange we are drilling out
// If true - we will stop on Slaves/Tables.
// fReturnTables - normally we never want to make the Edit Context a table. 
//  However in PumpMessage we do some extra work - to make the parent current.
//------------------------------------------------------------------------------------

HRESULT
CDoc::GetEditContext( 
                            CElement* pStartElement, 
                            CElement** ppEditThisElement, 
                            IMarkupPointer* pIStart, /* = NULL */
                            IMarkupPointer* pIEnd, /*= NULL */ 
                            BOOL fDrillingIn, /*= FALSE */ 
                            BOOL * pfEditThisEditable, /* = NULL*/
                            BOOL * pfEditParentEditable, /* = NULL */
                            BOOL * pfNoScopeElement /*=NULL */)
{
    HRESULT hr = S_OK;
    CElement* pEditableElement = NULL;
    CElement* pCurElement = NULL;
    IHTMLElement* pICurElement = NULL;
    CTreeNode* pCurNode = FALSE;
    BOOL fNoScope = FALSE;
    BOOL fEditable = FALSE;
    BOOL fHasReadOnly = FALSE;
    CElement* pLastReadOnly = NULL;
    IHTMLInputElement * pIInputElement = NULL;
    IHTMLTextAreaElement* pITextAreaElement = NULL;    
    BOOL fRefTaken = FALSE;
    
    if ( pStartElement->HasSlaveMarkupPtr() && fDrillingIn )
    {
        pEditableElement = pStartElement->GetSlaveMarkupPtr()->FirstElement();
    }
    if ( ! pEditableElement )
    {
        if ( _fDesignMode )
        {            
            if ( pStartElement->IsNoScope() && fDrillingIn )
            {
                //
                // Stop on a No-Scope (eg. CheckBox - or input ). If we're drilling in .
                //
                pEditableElement = pStartElement;
                fNoScope = TRUE;
            }            
            else
            {
                pCurNode = pStartElement->GetFirstBranch();
                //
                // If we're not drilling in - and starting at a site selectable element
                // we can then start at the ParentLayout Node - as this will be our edit context
                //
                if ( !fDrillingIn )
                {
                    if ( pStartElement->_etag == ETAG_TXTSLAVE )
                    {
                        pCurNode = pStartElement->MarkupMaster()->GetFirstBranch();                        
                        AssertSz( pCurNode, "Master not in Tree");
                    }

                    if ( pCurNode ) // should always be true - unless master wasn't in tree.
                    {
                        pStartElement = pCurNode->GetUpdatedNearestLayoutElement();
                        pCurNode = pStartElement->GetFirstBranch();
                    }
                    
                    if ( IsElementSiteSelectable( pStartElement ) )
                    {
                        pCurNode = pCurNode->GetUpdatedParentLayoutNode();                   
                    }
                }       
                
                while ( pCurNode )
                {
                    pCurElement = pCurNode->Element();
                    
                    //
                    // marka - we no longer want to have the edit context a table. However
                    // it is still valid to call GetEditContext in PumpMessage and have to special case
                    // tables. 
                    //
                    // If the pCurElement is any derivative of a table - we will find the outermost table cell.
                    // Pumpmessage will then make the parent of this table current.
                    //
                    if ( IsTablePart( pCurElement ))
                    {
                        pCurElement = GetOutermostTableElement( pCurElement );                    
                        Assert( pCurElement->_etag == ETAG_TABLE );

                        pEditableElement = pCurElement->GetUpdatedParentLayoutElement();
                            
                        break;
                    }  
                    else if ( ( pCurElement->HasFlowLayout() ) ||
                         ( pCurElement->_etag == ETAG_SELECT ) || 
                         ( pCurElement->_etag == ETAG_TXTSLAVE && fDrillingIn) )
                    {
                        pEditableElement = pCurElement;
                        break;
                    }    
                    if (( pCurElement->_etag == ETAG_TXTSLAVE )  && !fDrillingIn )
                    {
                        pCurNode = pCurElement->MarkupMaster()->GetFirstBranch();
                    }
                    pCurNode = pCurNode->Parent();          
                }
            }
            if ( ! pEditableElement )
                pEditableElement = PrimaryMarkup()->GetElementClient();        
        }
        else
        {
            //
            // Move until you hit the outermost editable element
            //
            BOOL fFoundEditableElement = pStartElement->_fEditAtBrowse && pStartElement->IsEnabled() ;

            pCurNode = pStartElement->GetFirstBranch();
            while ( pCurNode )
            {
                pCurElement = pCurNode->Element();
                fEditable = pCurElement->_fEditAtBrowse && pCurElement->IsEnabled();
                if ( ( fEditable  ) || 
                     ( pCurElement->_etag == ETAG_TXTSLAVE ) )
                {
                    pEditableElement = pCurElement;          
                    fFoundEditableElement = TRUE;
                    if ( pCurElement->_etag == ETAG_TXTSLAVE )
                        break;
                }
                if ( !fHasReadOnly && !fFoundEditableElement &&
                     pCurElement->_etag == ETAG_INPUT || 
                     pCurElement->_etag == ETAG_TEXTAREA )
                {
                    VARIANT_BOOL fRet = VB_FALSE;
                    fRefTaken = TRUE;
                    
                    if ( pCurElement->_etag == ETAG_INPUT )
                    {
                        hr = THR( pCurElement->QueryInterface( IID_IHTMLInputElement, (void**) & pIInputElement));

                        if ( hr )
                            goto Cleanup;

                        hr = THR( pIInputElement->get_readOnly( & fRet ));

                        if ( hr )
                            goto Cleanup;
    
                    }
                    else
                    {
                        hr = THR( pCurElement->QueryInterface( IID_IHTMLTextAreaElement, (void**) & pITextAreaElement));

                        if ( hr )
                            goto Cleanup;

                        hr = THR( pITextAreaElement->get_readOnly( & fRet ));
                        
                        if ( hr )
                            goto Cleanup;
                    }
                    if ( fRet == VB_TRUE )
                    {
                        fHasReadOnly = TRUE;
                        pLastReadOnly = pCurElement ;
                    }
                }
                if ( fFoundEditableElement && ( ! fEditable ) )
                {
                    break;
                }
                pCurNode = pCurNode->Parent();
            }
            
            if ( ! fFoundEditableElement )
            {
                //
                // There is no Edtiable element. Look for a ReadOnly Element.
                //
                if ( fHasReadOnly )
                {
                    pEditableElement = pLastReadOnly;
                }               
                else      
                {                     
                    //
                    // We're in browse mode, there is nothing editable.
                    // assume this is a selection in the body. Make the context the body.
                    //
                    pEditableElement = PrimaryMarkup()->GetElementClient();
                }                    
            }    
        }
    }

    AssertSz( pEditableElement->_etag != ETAG_TABLE, "Found Editable element to be table");


    //
    // Set the Pointers
    //
    if ( pEditableElement && pIStart && pIEnd )
    {
        hr = THR( pEditableElement->QueryInterface( IID_IHTMLElement, (void**) & pICurElement ));
        if ( hr )
            goto Cleanup;

        if ( ! fNoScope )
        {
            hr = THR( pIStart->MoveAdjacentToElement( pICurElement,  ELEM_ADJ_AfterBegin ));
            if ( hr )
                goto Cleanup;

            hr = THR( pIEnd->MoveAdjacentToElement( pICurElement,  ELEM_ADJ_BeforeEnd ));
            if ( hr )
                goto Cleanup;
        }
        else
        {
            hr = THR( pIStart->MoveAdjacentToElement( pICurElement,  ELEM_ADJ_BeforeBegin ));
            if ( hr )
                goto Cleanup;

            hr = THR( pIEnd->MoveAdjacentToElement( pICurElement,  ELEM_ADJ_AfterEnd ));
            if ( hr )
                goto Cleanup;       
        }
    }

    if ( ppEditThisElement )
    {
        *ppEditThisElement = pEditableElement;
    }

    if ( pfEditThisEditable )
    {
        if ( ! _fDesignMode && ! pEditableElement->IsEnabled() )
            *pfEditThisEditable = FALSE;
        else      
            *pfEditThisEditable =  pEditableElement->IsEditable(FALSE);
    }

    if ( pfEditParentEditable )
    {
        //
        // BUGBUG - more master/slave bizzaritude ! Getting the parent on the slave
        // goes to the root.
        //
        if ( pEditableElement->_etag != ETAG_TXTSLAVE )       
        {
            pCurNode = pEditableElement->GetFirstBranch();
            if ( pCurNode )
                pCurNode = pCurNode->Parent();
            AssertSz( pCurNode, "pCurNode is Null");
            if ( !pCurNode )
            {
                hr = E_FAIL; // Parent is not in tree - fail & Bail
                goto Cleanup;
            }               
        }            
        else
        {
            CElement* pParentElement = pEditableElement->MarkupMaster();
            if ( pParentElement )
            {
                pCurNode = pParentElement->GetFirstBranch();
                if ( pCurNode )
                    pCurNode = pCurNode->Parent();
            }
            AssertSz( pCurNode, "pCurNode is Null");
            if ( !pCurNode )
            {
                hr = E_FAIL;  // Parent is not in tree - fail & Bail
                goto Cleanup;
            }                
        }

        if ( pCurNode )
            * pfEditParentEditable = pCurNode->Element()->IsEditable(FALSE);
    }

    if ( pfNoScopeElement )
    {
        *pfNoScopeElement = fNoScope;        
    }
Cleanup:
    if ( fRefTaken )
    {
        ReleaseInterface( pIInputElement );
        ReleaseInterface( pITextAreaElement );
    }        
    ReleaseInterface( pICurElement );
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: GetEleemntOutsideTable
//
// Synopsis: Given an elemnet that is a "TablePart"
//              get the outermost table element
//
//------------------------------------------------------------------------------------


CElement*
CDoc::GetOutermostTableElement( CElement* pElement)
{
    BOOL fDone = FALSE;
    CElement* pCurElement = pElement;
    CTreeNode * pCurNode = pElement->GetFirstBranch();
    CElement* pNextElement = NULL;
    
    Assert( IsTablePart( pCurElement ));
    Assert( pCurElement->GetFirstBranch() );
    
    while ( ! fDone )
    {
        //
        // Step out to find the Table you're in ( and there had better be one)
        //
        while ( pCurElement->_etag != ETAG_TABLE )
        {                                          
            pCurNode = pCurNode->Parent();
            pCurElement = pCurNode->Element();  
        }

        pNextElement = pCurElement->GetUpdatedParentLayoutElement();
        if ( !IsTablePart( pNextElement ))
            fDone = TRUE;  
        else
        {
            pCurElement = pNextElement;
            pCurNode = pCurElement->GetFirstBranch();
            Assert( pCurNode );
        }
    }

    return pCurElement;
}

BOOL 
CDoc::IsTablePart( CElement* pElement)
{
    if ( !pElement )
        return FALSE;
    else 
        return ( ( pElement->_etag == ETAG_TD ) ||
           ( pElement->_etag == ETAG_TR ) ||
           ( pElement->_etag == ETAG_TBODY ) || 
           ( pElement->_etag == ETAG_TFOOT ) || 
           ( pElement->_etag == ETAG_TH ) ||
           ( pElement->_etag == ETAG_THEAD ) ||
           ( pElement->_etag == ETAG_CAPTION ) ||
           ( pElement->_etag == ETAG_TC ) ||
           ( pElement->_etag == ETAG_COL ) || 
           ( pElement->_etag == ETAG_COLGROUP ) );
}
//+====================================================================================
//
// Method: ScrollPointersIntoView
//
// Synopsis: Given two pointers ( that denote a selection). Do the "right thing" to scroll
//           them into view.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::ScrollPointersIntoView(
    IMarkupPointer *    pStart,
    IMarkupPointer *    pEnd)
{
    HRESULT hr = S_OK;
    CTreeNode* pNode;
    CFlowLayout* pFlowLayout ;
    CMarkupPointer* pPointerInternal = NULL;
    CMarkupPointer* pPointerInternal2 = NULL;
    int cpStart, cpEnd, cpTemp;

    hr = THR( pStart->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if ( hr )
        goto Cleanup;

        
    hr = THR( pEnd->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal2 ));
    if ( hr )
        goto Cleanup;
        
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);
    pFlowLayout = GetFlowLayoutForSelection( pNode );

    cpStart = pPointerInternal->GetCp();
    cpEnd = pPointerInternal2->GetCp();
    if ( cpStart > cpEnd )
    {
        cpTemp = cpStart ;
        cpStart = cpEnd;
        cpEnd = cpTemp;
    }


    pFlowLayout->ScrollRangeIntoView( cpStart, cpEnd , SP_MINIMAL , SP_MINIMAL);
    
Cleanup:
    RRETURN ( hr );
}

HRESULT
CDoc::ScrollPointerIntoView(
    IMarkupPointer *    pPointer,
    BOOL                fNotAtBOL,
    POINTER_SCROLLPIN   eScrollAmount )
{
    HRESULT hr = S_OK;
    CFlowLayout * pFlowLayout = NULL;
    SCROLLPIN ePin = SP_MINIMAL;
    HTMLPtrDispInfoRec LineInfo;
    CMarkupPointer* pPointerInternal = NULL ;
    CTreeNode* pNode;
    CFlowLayout* pNodeFlow = NULL;
    CSize viewSize;
    
    GetLineInfo( pPointer, fNotAtBOL, &LineInfo );
    

    LONG x, y, delta, height, clipX, clipY;
    x = LineInfo.lXPosition;
    y = LineInfo.lBaseline;
    delta = LineInfo.lLineHeight ;

    GetView()->GetViewSize( & viewSize);
    clipX = viewSize.cx / 4;
    clipY = viewSize.cy / 4;
    
    height = min( (int) LineInfo.lLineHeight , (int) clipY);
    CRect rc( x - min( delta , clipX  ) , y - height, x + min( delta, clipX )  , y + LineInfo.lDescent + 2  );

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointerInternal ));
    if ( hr )
        goto Cleanup;
        
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE);
    if ( pNode )
        pNodeFlow = GetFlowLayoutForSelection( pNode );
    else
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    switch( eScrollAmount )
    {
        case POINTER_SCROLLPIN_TopLeft:
            ePin = SP_TOPLEFT;
            break;
        case POINTER_SCROLLPIN_BottomRight:
            ePin = SP_BOTTOMRIGHT;
            break;        
        default:
            ePin = SP_MINIMAL;
            break;
    }

    //
    // We always scroll on the _pElemEditContext
    //
    if ( _pElemEditContext )
        pFlowLayout = GetFlowLayoutForSelection( _pElemEditContext->GetFirstBranch() );

    if ( pFlowLayout && pNodeFlow )
    {            
        if ( pNodeFlow != pFlowLayout ) // Handle GetLineInfo being in Local Coords.
        {
            pNodeFlow->TransformRect( (RECT*) & rc, COORDSYS_CONTENT, COORDSYS_GLOBAL );
            pFlowLayout->TransformRect( (RECT*) & rc, COORDSYS_GLOBAL, COORDSYS_CONTENT );    
        }       

        TraceTag(( tagViewServicesShowScrollRect, "ScrollRect: left:%ld top:%ld right:%ld bottom:%ld",
                                rc.left, rc.top, rc.right, rc.bottom ));
            
        hr = pFlowLayout->ScrollRectIntoView( rc, ePin , ePin, TRUE ) ? S_OK : S_FALSE;

        
    }        
    else
        hr = E_FAIL;
        
Cleanup:        
    return hr;
}

//+====================================================================================
//
// Method: ScrollPointIntoView
//
// Synopsis: Scroll any arbitrary point into view on a given elemnet.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::ScrollPointIntoView( IHTMLElement* pIElement, POINT * ptGlobal )
{
    HRESULT hr = S_OK;
    CElement* pElement;
    CPoint ptHit;
    ptHit.x = ptGlobal->x;
    ptHit.y = ptGlobal->y;
    
    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;
        
    
    if ( pElement && pElement->GetFirstBranch() )
    {
        CFlowLayout* pScrollLayout = NULL;        

        pScrollLayout = GetFlowLayoutForSelection( pElement->GetFirstBranch());
            
        Assert( pScrollLayout );
        if ( pScrollLayout )
        {   
            pScrollLayout->TransformPoint(&ptHit, COORDSYS_GLOBAL, COORDSYS_CONTENT);
            {
                CRect r( ptHit.x - scrollSize, ptHit.y - scrollSize, ptHit.x + scrollSize, ptHit.y + scrollSize );            
                pScrollLayout->ScrollRectIntoView( r, SP_MINIMAL, SP_MINIMAL, TRUE );
            }
        }
        else
            hr = E_FAIL;
    }
    else
        hr = E_FAIL;
Cleanup:
    RRETURN( hr );
}

HRESULT
CDoc::ArePointersInSameMarkup( IMarkupPointer * pP1, IMarkupPointer * pP2, BOOL * pfSameMarkup )
{
    HRESULT hr = S_OK;
    CMarkupPointer * p1 = NULL;
    CMarkupPointer * p2 = NULL;
    CMarkup * pM1 = NULL;
    CMarkup * pM2 = NULL;
    
    hr = THR( pP1->QueryInterface(CLSID_CMarkupPointer, (void **)&p1 ));
    if (hr) 
        goto Cleanup;
    hr = THR( pP2->QueryInterface(CLSID_CMarkupPointer, (void **)&p2 ));
    if (hr) 
        goto Cleanup;

    pM1 = p1->Markup();
    pM2 = p2->Markup();

    // NOTE: two unpositioned markups are NOT in the same tree
    *pfSameMarkup = ( pM1 != NULL && ((DWORD_PTR) pM1 == (DWORD_PTR) pM2 ));
    
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: DragEleemnt
//
// Synopsis: Allow a Drag/Drop.
//
//------------------------------------------------------------------------------------

HRESULT 
CDoc::DragElement(IHTMLElement* pIElement, DWORD dwKeyState)
{
    HRESULT hr = S_OK;

    CElement* pElement = NULL;

    hr = THR( pIElement->QueryInterface( CLSID_CElement, ( void** ) & pElement ));
    if ( hr )
        goto Cleanup;
        
    hr =  pElement->DragElement(
                GetFlowLayoutForSelection( _pElemEditContext->GetFirstBranch() ),
                dwKeyState, 
                NULL,
                -1)  ? S_OK : S_FALSE;
        
Cleanup:
    RRETURN1 ( hr, S_FALSE );
    
}


//+====================================================================================
//
// Method: BecomeCurrent
//
// Synopsis: Force a currency change on the element.
//
//------------------------------------------------------------------------------------

HRESULT 
CDoc::BecomeCurrent(IHTMLElement* pIElement, SelectionMessage* pSelMessage)
{
    HRESULT hr = S_OK;

    CElement* pElement = NULL;
    CMessage msg;

    hr = THR( pIElement->QueryInterface( CLSID_CElement, ( void** ) & pElement ));
    if ( hr )
        goto Cleanup;
    Assert( pElement );

    SelectionMessageToCMessage(pSelMessage, &msg);

    //
    // Blindly become current. This handles things like UI-Activation of Site Selected
    // objects.
    //
    hr = THR( pElement->BecomeCurrent( 0, NULL, &msg, TRUE) );
    
Cleanup:
    RRETURN1 ( hr, S_FALSE );
    
}


//+====================================================================================
//
// Method: GetElementDragBounds
//
// Synopsis: Get the bounds (in global coords) 
//           of the layout for this element ( used for dragging )
//  
//
//------------------------------------------------------------------------------------

HRESULT 
CDoc::GetElementDragBounds( IHTMLElement* pIElement, 
                            RECT* pElementDragBounds )
{
    HRESULT   hr = S_OK;
    CElement* pElement = NULL;
    CLayout * pLayout = NULL;
    CRect     rc;

    hr = THR( pIElement->QueryInterface( CLSID_CElement, ( void** ) & pElement ));
    if ( hr )
        goto Cleanup;

    Assert( pElement );

    pLayout = pElement->GetUpdatedNearestLayout();
    if ( !pLayout )
    {
        *pElementDragBounds = g_Zero.rc;
        goto Cleanup;
    }

    pLayout->GetRect( &rc, COORDSYS_GLOBAL );

    pElementDragBounds->left = rc.left;
    pElementDragBounds->right = rc.right;
    pElementDragBounds->top = rc.top;
    pElementDragBounds->bottom = rc.bottom;
    
Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: UpdateAdorner
//
// Synopsis: Call PositionChanged/Size Changed on the adorner.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::UpdateAdorner( 
    DWORD_PTR peCookie)
{
    HRESULT hr;

    if ( !peCookie )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CElementAdorner* pAdorner = (CElementAdorner*) peCookie;

        pAdorner->UpdateAdorner();
        
        hr = S_OK;
    }

    RRETURN ( hr );

}

//+====================================================================================
//
// Method: InvalidateAdorner
//
// Synopsis: Explicitly invalidate an adorner - by calling inval on its dispnode.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::InvalidateAdorner( 
    DWORD_PTR peCookie)
{
    HRESULT hr;

    if ( !peCookie )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CElementAdorner* pAdorner = (CElementAdorner*) peCookie;

        pAdorner->Invalidate();

        if (_pInPlace->_hwnd)
        {
            RedrawWindow( _pInPlace->_hwnd, NULL, NULL, RDW_INTERNALPAINT );
        }
        
        hr = S_OK;
    }

    RRETURN ( hr );

}

//+====================================================================================
//
// Method: ScrollIntoView
//
// Synopsis:Scroll teh adorner into view
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::ScrollIntoView( 
    DWORD_PTR peCookie)
{
    HRESULT hr;

    if ( !peCookie )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CElementAdorner* pAdorner = (CElementAdorner*) peCookie;

        if ( pAdorner->ScrollIntoView() )
            hr = S_OK;
        else
            hr = S_FALSE;
    }

    RRETURN1 ( hr, S_FALSE );

}

//+====================================================================================
//
// Method: IsElementLocked
//
// Synopsis: Is this element Locked
//
// BUGBUG - this could be implemented outside of trident - were it not for Master/Slave
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::IsElementLocked( 
        IHTMLElement* pIElement, 
        BOOL* pfLocked )
{
    HRESULT hr = S_OK;
    IHTMLStyle * pCurStyle = NULL;
    CVariant varLocked;
    BSTR    bstrLocked ; 
    BOOL fIsLocked = FALSE;
    CElement* pElement = NULL;

    hr = THR( pIElement->QueryInterface(CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;

    if ( pElement->_etag == ETAG_TXTSLAVE )
    {
        pElement = pElement->MarkupMaster();

        hr = THR( pElement->get_style( & pCurStyle ));
        if ( hr )
            goto Cleanup;
    }    
    else
    {
        hr = THR( pIElement->get_style( & pCurStyle ));
        if ( hr )
            goto Cleanup;
    }
    Assert( pIElement );
    
    hr = THR( pCurStyle->getAttribute( _T("Design_Time_Lock"), 0, & varLocked ));
    if ( hr )
        goto Cleanup;
        
    if (!varLocked.IsEmpty() )
    {   
        bstrLocked = V_BSTR(&varLocked);
        fIsLocked = ( StrCmpIW(_T("true"), bstrLocked ) == 0);
    }
    
Cleanup:
    VariantClear( & varLocked );
    ReleaseInterface( pCurStyle );
    
    if ( pfLocked )
        *pfLocked = fIsLocked;

    RRETURN( hr );        
}

//+====================================================================================
//
// Method: MakeParentCurrent
//
// Synopsis: Used to make the Parent Element of the Message current.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::MakeParentCurrent(IHTMLElement* pIElement)
{
    HRESULT hr = S_OK;
    
    CTreeNode * pNodeParent = NULL;
    CElement* pElement = NULL;
    CElement* pParentElement;
    CTreeNode* pNode = NULL;
    
    hr = THR( pIElement->QueryInterface(CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;

    // We are trying to get the parent layout, so jump to the master's tree
    if (pElement->Tag() == ETAG_TXTSLAVE)
    {
        pElement = pElement->MarkupMaster();
    }

    pNode = pElement->GetFirstBranch();

    Assert( pNode );

    if ( pNode )
    {
        pNodeParent = pNode->GetUpdatedParentLayoutNode();
        
        if (pNodeParent && pNodeParent->GetFlowLayout())
        {
            pParentElement = pNodeParent->Element();
            
            //
            // marka - right now - OLE Sites aren't becoming current
            //

            if ( _pElemCurrent->_etag == ETAG_OBJECT )
               _pElemCurrent->YieldUI( pParentElement );   
               
            hr = THR( EnsureEditContext( pParentElement ));
        }
    }
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: GetOuterMostEditableElement
//
// Synopsis: Given an element - get the outermost elemnet that 's editable.
//           In _fDesignMode - thsi is the Body.
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::GetOuterMostEditableElement( IHTMLElement* pIElement,
                                   IHTMLElement** ppIElement )
{
    HRESULT hr = S_OK;
    CElement* pElement = NULL;
    CTreeNode* pCurNode;
    CElement* pCurElement;
    
    if ( _fDesignMode || !pIElement)
    {
        pElement = PrimaryMarkup()->GetElementClient();
    }
    else
    {
        hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement));
        if ( hr )
            goto Cleanup;
            
        if ( pElement->_etag == ETAG_TXTSLAVE )
            pElement = pElement->MarkupMaster();
            
        if ( pElement->_fEditAtBrowse || 
             pElement->_etag == ETAG_BODY )
        {       
            pCurNode = pElement->GetFirstBranch();
            pElement = pCurNode->Element();
            pCurElement = pElement;
            
            while( pCurNode &&                 
                   pCurElement->_fEditAtBrowse &&
                   pElement->_etag != ETAG_BODY )
            {
                pElement = pCurElement;                         
                pCurNode = pCurNode->Parent();
                pCurElement = pCurNode->Element();   
            }
        }
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }            
    }
    
    if ( pElement )
    {
        hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void**) ppIElement ));
    }
    
Cleanup:    
    RRETURN( hr );
    
}

HRESULT 
CDoc::IsSite( 
    IHTMLElement *  pIElement, 
    BOOL*           pfSite, 
    BOOL*           pfText, 
    BOOL*           pfMultiLine, 
    BOOL*           pfScrollable )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;
    CTreeNode * pNode = NULL;
    
    BOOL fSite = FALSE;
    BOOL fText = FALSE;
    BOOL fMultiLine = FALSE;
    BOOL fScrollable = FALSE;
    
    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;

    if (pElement->Tag() == ETAG_TXTSLAVE)
    {
        pElement = pElement->MarkupMaster();
        if (!pElement)
        {
            goto Cleanup;
        }

    }

    pNode = pElement->GetFirstBranch();
    if( pNode == NULL )
        goto Cleanup;
        
    fSite = pNode->NeedsLayout();

    if( pfText )
    {
        CFlowLayout * pLayout = NULL;
        pLayout = pNode->HasFlowLayout();

        if( pLayout != NULL )
        {
            fText = TRUE;
            fSite = TRUE;
        }

        if( fText && pfMultiLine )
        {
            fMultiLine = pLayout->GetMultiLine();
        }

        // BUGBUG (johnbed) we may at some point want to break this apart from the flow layout check
        if( fText && pfScrollable )
        {
            CDispNode * pDispNode = pLayout->GetElementDispNode();
            fScrollable = pDispNode && pDispNode->IsScroller();
        }
    }
    
Cleanup:
    if( pfSite )
        *pfSite = fSite;

    if( pfText )
        *pfText = fText;

    if( pfMultiLine )
        *pfMultiLine = fMultiLine;

    if( pfScrollable )
        *pfScrollable = fScrollable;

    RRETURN( hr );
}


//+====================================================================================
//
// Method: QueryBreaks
//
// Synopsis: Returnline break information associated with a given pointer
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::QueryBreaks ( IMarkupPointer * pPointer, DWORD * pdwBreaks, BOOL fWantPendingBreak )
{
    HRESULT             hr;
    CMarkupPointer *    pmpPointer = NULL;
    CLineBreakCompat    breaker;   

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpPointer ) );
    if (hr)
        return hr;

    breaker.SetWantPendingBreak ( fWantPendingBreak );

    return breaker.QueryBreaks( pmpPointer, pdwBreaks );
}


//+====================================================================================
//
// Method: MergeBlock
//
// Synopsis: 
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::MergeDeletion ( IMarkupPointer * pPointer )
{
    extern HRESULT MergeBlock ( CMarkupPointer * pmpPointer );
    HRESULT             hr;
    CMarkupPointer *    pmpPointer = NULL;

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pmpPointer ) );
    if (hr)
        return hr;

    return MergeBlock( pmpPointer );
}


//+====================================================================================
//
// Method: IsEqualElement
//
// Synopsis: Verify that elements are the same for Master/SlaveNess.
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::IsEqualElement( IHTMLElement* pIElement1, IHTMLElement* pIElement2 )
{
    HRESULT hrEqual = S_FALSE;
    IObjectIdentity * pIIdent = NULL;
    HRESULT hr = S_OK;

    hr = THR( pIElement1->QueryInterface( IID_IObjectIdentity, (void**) & pIIdent ));
    if ( hr )
        goto Cleanup;
        
    hrEqual = pIIdent->IsEqualObject( pIElement2 );

    if ( hrEqual == S_FALSE )
    {
        CElement* pElement1 = NULL;
        CElement* pElement2 = NULL;

        hr = THR( pIElement1->QueryInterface( CLSID_CElement, (void**) & pElement1 ));
        if ( hr )
            goto Cleanup;

        hr = THR( pIElement2->QueryInterface( CLSID_CElement, (void**) & pElement2 ));
        if ( hr )
            goto Cleanup;

        if ( pElement1->_etag == ETAG_TXTSLAVE && pElement2 == pElement1->MarkupMaster() )
            hrEqual = S_OK;
        else if ( pElement2->_etag == ETAG_TXTSLAVE && pElement1 == pElement2->MarkupMaster() )
            hrEqual = S_OK;

    }
    
Cleanup:    
    ReleaseInterface( pIIdent );

    RRETURN1( hrEqual , S_FALSE );
}

//+====================================================================================
//
// Method: IsContainedBy
//
// Synopsis: See if The given InnerContainer is contained by the outer container.
//
//           return S_OK if it is contained
//                  S_FALSE if Not contained
//                  E_INVALIDARG if they are in the same markup
//                  E_FAIL for otheer errors
//------------------------------------------------------------------------------------


HRESULT
CDoc::IsContainedBy( IMarkupContainer* pIOuterContainer, 
                     IMarkupContainer* pIInnerContainer )
{
    HRESULT hr;
    CMarkup* pOuterMarkup = NULL;
    CMarkup* pInnerMarkup = NULL;

    if ( ! pIOuterContainer || ! pIInnerContainer )
    {
        hr = E_FAIL;
        goto Cleanup;
    }    
    
    hr = THR( pIOuterContainer->QueryInterface( CLSID_CMarkup, (void**) & pOuterMarkup ));
    if ( hr )
        goto Cleanup;

    hr = THR( pIInnerContainer->QueryInterface( CLSID_CMarkup, (void**) & pInnerMarkup));
    if ( hr )
        goto Cleanup;

    if ( pOuterMarkup && pInnerMarkup )
    {
        if ( pInnerMarkup == pOuterMarkup )
        {
            hr = E_INVALIDARG;
            goto Cleanup;            
        }            
        
        while (pInnerMarkup )
        {
            if ( pInnerMarkup == pOuterMarkup )
            {
                hr = S_OK;
                goto Cleanup;
            }
            if ( pInnerMarkup->Master())
                pInnerMarkup = pInnerMarkup->Master()->GetMarkup();            
            else
            {
                hr = S_FALSE;
                goto Cleanup;
            }                
        }
        hr = S_FALSE;
    }
    else
        hr = E_FAIL;
        
Cleanup:
    return ( hr );
}


//+====================================================================================
//
// Method: GetElemnetForSelection
//
// Synopsis: Gets the Master if the element given is a slave. Otherwise gets the element
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::GetElementForSelection( 
        IHTMLElement * pIElement, 
        IHTMLElement** ppISiteSelectableElement )
{
    HRESULT hr = S_OK;
    CElement* pElement = NULL;

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;

    if ( pElement->_etag == ETAG_TXTSLAVE )
    {
        hr = THR( pElement->MarkupMaster()->QueryInterface( IID_IHTMLElement, ( void**) ppISiteSelectableElement ));
    }
    else
    {
        ReplaceInterface( ppISiteSelectableElement , pIElement );
    }
Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: GetCursorForHTC
//
// Synopsis: Gets the Cursor for an HTC value 
//
//------------------------------------------------------------------------------------

LPCTSTR
CDoc::GetCursorForHTC( HTC inHTC )
{
    Assert( inHTC >= HTC_TOPBORDER );

    return astrCursor[inHTC - HTC_TOPBORDER ];
}

//+====================================================================================
//
// Method: CurrentScopeOrSlave
//
// Synopsis: Returns the current scope for a pointer, like CurrentScope; but
//           can also return the TXTSLAVE element at the top of an INPUT subtree.
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CDoc::CurrentScopeOrSlave ( IMarkupPointer * pPointer, IHTMLElement ** ppElemCurrent )
{
    HRESULT hr = S_OK;
    CMarkupPointer *    pmp = NULL;
    CTreeNode * pNode;

    if (!ppElemCurrent)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppElemCurrent = NULL;
    
    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pmp ) );
    if (hr)
        goto Cleanup;
        
    pNode = pmp->CurrentScope(MPTR_SHOWSLAVE);
    
    if (pNode) // (dbau: unlike IMarkupPointer::CurrentScope, don't check for ETAG_TXTSLAVE)
    {
        hr = THR(
            pNode->GetElementInterface(
                IID_IHTMLElement, (void **) ppElemCurrent ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

//+====================================================================================
//
// Method:   LeftOrSlave
//
// Synopsis: Like IMarkupPointer::Left, but doesn't hide textslave
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CDoc::LeftOrSlave (
        IMarkupPointer * pPointer,
        BOOL fMove,
        MARKUP_CONTEXT_TYPE *pContext,
        IHTMLElement** ppElement,
        long *pcch,
        OLECHAR* pchText)
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pmp = NULL;
    DWORD            dwFlags = MPTR_SHOWSLAVE;

    if (!pPointer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pmp ) );
    if (hr)
        goto Cleanup;
        
    hr = THR( pmp->There( TRUE, fMove, pContext, ppElement, pcch, pchText, & dwFlags ) );

Cleanup:

    RRETURN( hr );
}

//+====================================================================================
//
// Method:   RightOrSlave
//
// Synopsis: Like IMarkupPointer::Right, but doesn't hide textslave
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CDoc::RightOrSlave (
        IMarkupPointer * pPointer,
        BOOL fMove,
        MARKUP_CONTEXT_TYPE *pContext,
        IHTMLElement** ppElement,
        long *pcch,
        OLECHAR* pchText)
{
    HRESULT          hr;
    CMarkupPointer * pmp = NULL;
    DWORD            dwFlags = MPTR_SHOWSLAVE;

    if (!pPointer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pmp ) );
    if (hr)
        goto Cleanup;
        
    hr = THR( pmp->There( FALSE, fMove, pContext, ppElement, pcch, pchText, & dwFlags ) );

Cleanup:

    RRETURN( hr );
}

//+====================================================================================
//
// Method: MoveToContainerOrSlave
//
// Synopsis: Returns the current scope for a pointer, like CurrentScope; but
//           can also return the TXTSLAVE element at the top of an INPUT subtree.
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CDoc::MoveToContainerOrSlave (
        IMarkupPointer *pPointer,
        IMarkupContainer* pContainer,
        BOOL fAtStart)
{
    HRESULT hr;
    CMarkupPointer *    pmp = NULL;
    CMarkup *pmu = NULL;
    
    if (!pPointer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pContainer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pmp ) );
    if (hr)
        goto Cleanup;
        
    hr = THR( pPointer->QueryInterface( CLSID_CMarkup, (void **) & pmu ) );
    if (hr)
        goto Cleanup;
        
    hr = THR(pmp->MoveToContainer(pmu, fAtStart, MPTR_SHOWSLAVE ));

Cleanup:

    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsEnabled
//
// Synopsis: Is this element enabled
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::IsEnabled( IHTMLElement* pIElement, BOOL *pfEnabled )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;
    BOOL fEnabled = FALSE;
    
    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;

    if ( pElement )
    {
        fEnabled = pElement->IsEnabled();
    }
    else
    {
        hr = E_FAIL;
        goto Cleanup;
    }        
    
Cleanup:
    if ( pfEnabled )
        *pfEnabled = fEnabled;
        
    RRETURN( hr );
}


//
// IEditDebugServices Methods.
//
#if DBG == 1

//+====================================================================================
//
// Method: GetCp
//
// Synopsis: Gets the CP of a pointer. -1 if it's unpositioned
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::GetCp( IMarkupPointer* pIPointer, long* pcp)
{
    HRESULT hr = S_OK;
    CMarkupPointer* pPointer = NULL;
    long cp = 0;
    
    hr = THR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointer ));
    if ( hr ) 
        goto Cleanup;

    cp = pPointer->GetCp();
        
Cleanup:
    if ( pcp )
        *pcp = cp;
        
    RRETURN ( hr );
}


//+====================================================================================
//
// Method: SetDebugName
//
// Synopsis: Allows setting of Debug Name on an IMarkupPointer. This name then shows up
//           in DumpTree's.
//
//------------------------------------------------------------------------------------

HRESULT
CDoc::SetDebugName( IMarkupPointer* pIPointer, LPCTSTR strDebugName )
{
    HRESULT hr = S_OK;
    CMarkupPointer* pPointer = NULL;

    
    hr = THR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointer ));
    if ( hr ) 
        goto Cleanup;

    pPointer->SetDebugName( strDebugName);
    
Cleanup:

    RRETURN ( hr );

}


//+====================================================================================
//
// Method: DumpTree
//
// Synopsis: Calls dumptree on an IMarkupPointer.
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::DumpTree( IMarkupPointer* pIPointer)
{
    HRESULT hr = S_OK;
    CMarkupPointer* pPointer = NULL;
    CMarkup * pMarkup = NULL;

    if (pIPointer)
    {
        hr = THR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointer ));
        
        if ( hr ) 
            goto Cleanup;

        pMarkup = pPointer->Markup();
    }
    
    if (!pMarkup)
        pMarkup = _pPrimaryMarkup;

    if (pMarkup)
        pMarkup->DumpTree();
    
Cleanup:

    RRETURN ( hr );
}

#endif    

//+====================================================================================
//
// Method: FindUrl
//
// Synopsis: Finds the next url in the specified range
//
//------------------------------------------------------------------------------------
HRESULT
CDoc::FindUrl(
    IMarkupPointer *pmpStart, 
    IMarkupPointer *pmpEnd, 
    IMarkupPointer *pmpUrlStart, 
    IMarkupPointer *pmpUrlEnd)
{
    HRESULT         hr;
    CMarkupPointer  *pmp;
    long            cpStart, cpEnd;
    long            cpUrlStart, cpUrlEnd;
    CMarkup         *pMarkup;

    //
    // Get the start and end cp's
    //
    
    hr = THR(pmpEnd->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pmp) );
    if (hr) 
        goto Cleanup;
        
    cpEnd = pmp->GetCp();

    hr = THR(pmpStart->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pmp) );
    if (hr) 
        goto Cleanup;
        
    cpStart = pmp->GetCp();

    if (cpStart >= cpEnd)
    {
        hr = S_FALSE; // nothing to do
        goto Cleanup;
        
    }

    pMarkup = pmp->Markup();
    
    //
    // Move through the range
    //

    {
        CTxtPtr tp(pMarkup, cpStart);
        
        for (tp.AdvanceCp(2);
             long(tp.GetCp()) < cpEnd;
             tp.AdvanceCp(1))
        {
            switch (tp.GetChar())
            {
                case '.':       
                case '@':       
                case ':':       
                case '\\':       
                case '/':       
                {
                    if (tp.IsInsideUrl(&cpUrlStart, &cpUrlEnd) && cpUrlStart >= cpStart)
                    {
                        hr = THR(pmpUrlStart->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pmp));
                        if (hr) 
                            goto Cleanup;                    

                        hr = THR(pmp->MoveToCp(cpUrlStart, pMarkup));
                        if (hr)
                            goto Cleanup;

                        hr = THR(pmpUrlEnd->QueryInterface(CLSID_CMarkupPointer, (LPVOID *)&pmp));
                        if (hr) 
                            goto Cleanup;                    

                        hr = THR(pmp->MoveToCp(cpUrlEnd, pMarkup));
                        if (hr)
                            goto Cleanup;

                        hr = S_OK;
                        goto Cleanup;                        
                    }
                }
            }
        }
    }    

    // Can't find URL, so return S_FALSE
    hr = S_FALSE;

Cleanup:    
    RRETURN1(hr, S_FALSE);
}

//+====================================================================================
//
// Method: AllowSelection
//
// Synopsis: IsSelection Allowed in the current elemnet. Check for if we're in an HTML Dialog etc.
//              RETURN S_OK if selection allowed
//                     S_FALSE if selection not allowed
//------------------------------------------------------------------------------------

HRESULT 
CDoc::AllowSelection( 
    IHTMLElement *  pIElement,
    SelectionMessage* peMessage )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;
    CTreeNode* pTreeNode;
    CRect myRect;
    
    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;

    if (pElement->Tag() == ETAG_TXTSLAVE)
    {
        pElement = pElement->MarkupMaster();
        if (!pElement)
        {
            goto Cleanup;
        }
    }

    hr = pElement->DisallowSelection() ? S_FALSE : S_OK ;


    if ( hr == S_OK )
    {
        //
        // BUGBUG - 5.x - I'd like to do the check for context text length - 
        // by adding an additional flag to AllowSelection ( or expose GetContextTextLength).
        // However to avoid changes for 5.x - I will do the additional check for ContextTextLength -
        // iff the Message is null. The message will not be null when called from ShouldStartTracker
        // it will only be null when called from the SelectAll command.
        //
        // We must fix this when we "clean up" Viewservices
        //
        if ( peMessage )
        {
            //
            // Only allow a selection to begin in the content area of a flow layout
            // ie do not begin text selection on borders, scroll bars etc.
            //
            pTreeNode = static_cast<CTreeNode*>( (void *)(peMessage->elementCookie) );
            if ( pTreeNode )
            {
                CFlowLayout* pFlowLayout = GetFlowLayoutForSelection( pTreeNode );
                if ( pFlowLayout )
                {
                    //
                    // BUGBUG - use a different rect for TD's. WHy ? because
                    // GetClientRect doesn't quite give us what we want. pursue
                    // this with Don sometime.

                    //
                    // If the message is over a TD and we do this we're ok - because
                    // if we click on the border of a Table - the node is the TABLE.
                    //
                    if ( IsTablePart( pFlowLayout->ElementOwner() ))
                        pFlowLayout->GetRect( & myRect, COORDSYS_CONTENT );
                    else                    
                        pFlowLayout->GetClientRect( & myRect, COORDSYS_CONTENT );

                    hr =  ::PtInRect( & myRect, peMessage->ptContent ) ? S_OK : S_FALSE;

                }
            }            
        }
        else if ( pElement->GetMarkup()->GetContentTextLength() == 0 )
        {
                hr = S_FALSE;
        }
    }

Cleanup:
    RRETURN1 ( hr, S_FALSE );
}    



//+====================================================================================
//
// Method: MoveWord
//
// Synopsis: Provide IE 4 level word moving.
//             MoveWord() is a wrapper for MoveUnit(). It stops at IE4 word breaks
//              that MoveUnit() ignores such as:
//              <BR>, Block Break, TSB, TSE, and Intrinsics 
//              The only muActions supported are MOVEUNIT_PREVWORDBEGIN and 
//              MOVEUNIT_NEXTWORDBEGIN
//------------------------------------------------------------------------------------
HRESULT
CDoc::MoveWord( IMarkupPointer * pPointerToMove,
               MOVEUNIT_ACTION  muAction,
               IMarkupPointer * pLeftBoundary,
               IMarkupPointer * pRightBoundary )
{
    HRESULT hr;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;
    CTreeNode       *   pNode;
    DWORD               dwBreaks;
    BOOL                fResult;
    BOOL                fPassedText;
    IHTMLElement    *   pIElement = NULL;
    CMarkupPointer  pointerSource( this );
    CMarkupPointer  pointerDestination( this );
    

    extern BOOL IsIntrinsicTag( ELEMENT_TAG eTag );


    Assert ( muAction == MOVEUNIT_PREVWORDBEGIN || 
             muAction == MOVEUNIT_NEXTWORDBEGIN );

    if (  muAction != MOVEUNIT_PREVWORDBEGIN &&
          muAction != MOVEUNIT_NEXTWORDBEGIN )
    {
        hr = E_INVALIDARG;
        goto Cleanup;        
    }
    
    //
    // pPointerDestination is where MoveUnit() would have positioned us, however, 
    // since MoveUnit() does not account for IE4 word breaks like intrinsics,
    // Block Breaks, text site begin/ends, and Line breaks, we use another pointer
    // called pPointerSource. This pointer walks towards pPointerDestination
    // to detect IE4 word breaking characters that MoveUnit() does not catch.
    //


    hr = THR( 
            pointerSource.MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;

    hr = THR( 
            pointerDestination.MoveToPointer( pPointerToMove ) );
    if (hr)
        goto Cleanup;

    hr = THR(
            pointerDestination.MoveUnit( muAction ) );
    if (hr)
        goto Cleanup;

    //
    // MoveUnit() may place the destination outside the range boundary. 
    // First make sure that the destination is within range boundaries.
    //
    if ( pLeftBoundary && muAction == MOVEUNIT_PREVWORDBEGIN )
    {
        hr = THR( pointerDestination.IsLeftOf( pLeftBoundary, & fResult ) );
        if (hr)
            goto Cleanup;

        if ( fResult )
        {
            hr = THR( pointerDestination.MoveToPointer( pLeftBoundary ) );
            if (hr)
                goto Cleanup;
        }
    }
    else if( pRightBoundary && muAction == MOVEUNIT_NEXTWORDBEGIN )
    {
        hr = THR( pointerDestination.IsRightOf( pRightBoundary, & fResult ) );
        if (hr)
            goto Cleanup;

        if ( fResult )
        {
            hr = THR( pointerDestination.MoveToPointer( pRightBoundary ) );
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Walk pointerSource towards pointerDestination, looking
    // for word breaks that MoveUnit() might have missed.
    //
    
    fPassedText = FALSE;

    for ( ; ; )
    {
        if ( muAction == MOVEUNIT_PREVWORDBEGIN )
        {
            hr = THR( pointerSource.Left( TRUE, & context, & pNode , NULL, NULL, NULL ));
        }
        else
        {
            hr = THR( pointerSource.Right( TRUE, & context, & pNode , NULL, NULL, NULL ));
        }
        if ( hr )
            goto Cleanup;

        switch( context )
        {
        case CONTEXT_TYPE_Text:
            fPassedText = TRUE;
            break;

        case CONTEXT_TYPE_NoScope:
        case CONTEXT_TYPE_EnterScope:
        case CONTEXT_TYPE_ExitScope:
            if ( !pNode )
                break;

            if ( IsIntrinsicTag( pNode->Tag() ) )
            {
                // Move over intrinsics (e.g. <BUTTON>), don't go inside them like MoveUnit() does.
                // Here the passed in pointer is set before or after the intrinsic based
                // on our direction and whether or not we've travelled over text before.
                ClearInterface( & pIElement );
                hr = THR( pNode->Element()->QueryInterface( IID_IHTMLElement, (void **) & pIElement ) );
                if (hr)
                    goto Cleanup;

                if ( muAction == MOVEUNIT_NEXTWORDBEGIN )
                {
                    hr = THR( pPointerToMove->MoveAdjacentToElement( pIElement, 
                                fPassedText ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ) );
                }
                else
                {
                    hr = THR( pPointerToMove->MoveAdjacentToElement( pIElement, 
                                fPassedText ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ) );
                }
                //
                // We're done
                //
                goto Cleanup;
            }

            //
            // Special case Anchors - treat them as word breaks.
            //
            if ( pNode->Tag() == ETAG_A )
            {
                if ( fPassedText && muAction == MOVEUNIT_NEXTWORDBEGIN )
                {
                    // If we're travelling right and have passed some text, backup
                    // before the last BR, we've gone too far.
                    hr = THR( pointerSource.Left( TRUE, NULL, NULL, NULL, NULL, NULL ));
                    goto Done;
                }
                else if ( fPassedText && muAction == MOVEUNIT_PREVWORDBEGIN )
                {
                    // Travelling left we are at the right place: we're at the beginning of <BR>
                    // which is a valid word break.
                    goto Done;
                }                 
            }
#if 0 
        //
        // This code was special code for the range move word that selection doesn't need
        // ifdef'ed out for now.
        //
            // <BR> is a word break 
            if ( pNode->Tag() == ETAG_BR )
            {
                if ( fPassedText && muAction == MOVEUNIT_NEXTWORDBEGIN )
                {
                    // If we're travelling right and have passed some text, backup
                    // before the last BR, we've gone too far.
                    hr = THR( pointerSource.Left( TRUE, NULL, NULL, NULL, NULL, NULL ));
                    goto Done;
                }
                else if ( muAction == MOVEUNIT_PREVWORDBEGIN )
                {
                    // Travelling left we are at the right place: we're at the beginning of <BR>
                    // which is a valid word break.
                    goto Done;
                }                 
                else
                {
                    fPassedText = TRUE;
                }           
            }
#endif

            if ( pNode->Element()->IsBlockElement() ||
                      pNode->Element()->HasLayout()      ||
                      context == CONTEXT_TYPE_NoScope)
            {
                fPassedText = TRUE;
            }           
            break;
            
        }

        //
        // If we are at or beyond the destination point where MoveUnit() took us, 
        // set the passed in pointer to the destination and we're outta here
        //
        if ( muAction == MOVEUNIT_PREVWORDBEGIN )
        {            
            if ( pointerSource.IsLeftOfOrEqualTo( & pointerDestination ) )
            {                
                hr = THR( 
                        pPointerToMove->MoveToPointer( & pointerDestination ) );
                goto Cleanup;
            }
        }
        else
        {
            if ( pointerSource.IsRightOfOrEqualTo( & pointerDestination ) )
            {                
                hr = THR( 
                        pPointerToMove->MoveToPointer( & pointerDestination ) );
                goto Cleanup;
            }
        }

        //
        // Detect Block break, Text site begin or text site end
        //
        hr = THR( pointerSource.QueryBreaks( & dwBreaks ) );
        if (hr)
            goto Cleanup;

        // We hit a break before reaching our destination, time to stop...
        if ( dwBreaks != BREAK_NONE )
        {
            if ( fPassedText )
            {
                //BUGBUG: Must cling to text here

                goto Done;
            }
            else
            {
                fPassedText = TRUE;
            }
        }
    }

Done:
    hr = THR( 
            pPointerToMove->MoveToPointer( & pointerSource ) );
    if (hr)
        goto Cleanup;
    
Cleanup:
    ReleaseInterface( pIElement );
    RRETURN( hr );
}


//+====================================================================================
//
// Method: GetAdjacencyForPoint
//
// Synopsis: Given an elemnet and a point - return the closest edge to that element in the
//           form of an ELEMENT_ADJACENCY
//
//          Used by selection to determine how to "skip over" sites.
// 
//------------------------------------------------------------------------------------



HRESULT 
CDoc::GetAdjacencyForPoint( IHTMLElement* pIElement, 
                            POINT ptGlobal, 
                            ELEMENT_ADJACENCY *peAdjacent  )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;
    CFlowLayout* pFlowLayout = NULL;
    CRect rectGlobal;
    ELEMENT_ADJACENCY eAdj = ELEM_ADJ_BeforeBegin;
    int midX;
    
    if (! pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void**) & pElement ));
    if ( hr )
        goto Cleanup;

    if ( ! pElement->GetFirstBranch())
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    pFlowLayout = GetFlowLayoutForSelection( pElement->GetFirstBranch());
    if ( ! pFlowLayout )
    {
        hr = E_FAIL;
        goto Cleanup;        
    }

    pFlowLayout->GetRect( & rectGlobal, COORDSYS_GLOBAL );

    midX = rectGlobal.left + ( rectGlobal.right - rectGlobal.left) / 2;

    if ( ::PtInRect( & rectGlobal, ptGlobal ))
    {
        if ( ptGlobal.x <= midX )
            eAdj = ELEM_ADJ_AfterBegin;
        else
            eAdj = ELEM_ADJ_BeforeEnd;
    }
    else
    {
        if ( ptGlobal.x <= midX )
            eAdj = ELEM_ADJ_BeforeBegin;
        else
            eAdj = ELEM_ADJ_AfterEnd;
    }
    
Cleanup:

    if ( peAdjacent )
        *peAdjacent = eAdj;

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: SaveSegmentsToClipboard
//
// Synopsis: Saves a SegmentList to the clipboard
//
//------------------------------------------------------------------------------------

HRESULT
#ifndef UNIX
CDoc::SaveSegmentsToClipboard( ISegmentList * pSegmentList )
#else
CDoc::SaveSegmentsToClipboard( ISegmentList * pSegmentList, 
                               VARIANTARG *pvarargOut)
#endif
{
    CMarkup      *      pMarkup;
    BOOL                fEqual;
    CTextXBag    *      pBag = NULL;
    IDataObject  *      pDO = NULL;
    HRESULT             hr = S_OK;
    BOOL                fSupportsHTML = FALSE;
    IMarkupPointer *    pStart  = NULL;
    IMarkupPointer *    pEnd = NULL;
    CMarkupPointer *    pointerStart = NULL;
    CMarkupPointer *    pointerEnd = NULL;
    CTreeNode *         pNodeStart;
    CTreeNode *         pNodeEnd;
    CTreeNode *         pAncestor;

    hr = THR( CreateMarkupPointer( & pStart ));
    if ( hr )
        goto Cleanup;

    hr = THR( CreateMarkupPointer( & pEnd ));
    if ( hr )
        goto Cleanup;

    hr = THR( pSegmentList->MovePointersToSegment( 0, pStart, pEnd ));
    if ( hr )
        goto Cleanup;

    hr = THR( pStart->IsEqualTo ( pEnd, & fEqual ) );
    if (hr)
        goto Cleanup;

    if (fEqual)
    {
        // There is nothing to save
        hr = S_OK;
        goto Cleanup;
    }
    
    hr = THR( pStart->QueryInterface( CLSID_CMarkupPointer, ( void** ) & pointerStart));
    if ( hr )
        goto Cleanup;

    hr = THR( pEnd->QueryInterface( CLSID_CMarkupPointer, ( void** ) & pointerEnd));
    if ( hr )
        goto Cleanup;

    Assert( pointerStart->IsPositioned() && pointerEnd->IsPositioned() );

    pMarkup = pointerStart->Markup();
    if ( pMarkup != pointerEnd->Markup() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pNodeStart = pointerStart->CurrentScope( MPTR_SHOWSLAVE );
    pNodeEnd   = pointerEnd->CurrentScope( MPTR_SHOWSLAVE );
    
    if ( !pNodeStart || !pNodeEnd )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    pAncestor = pNodeStart->GetFirstCommonAncestor( pNodeEnd, NULL );

    fSupportsHTML = ( pAncestor && pAncestor->SupportsHtml() );

    hr = THR( CTextXBag::Create( 
            pMarkup, fSupportsHTML, pSegmentList, FALSE, & pBag) );
                                                                  
    if (hr)
        goto Cleanup;

#ifdef UNIX // This is used for MMB-paste to save selected text to buffer.
    if (pvarargOut)
    {
        if (pBag->_hUnicodeText)
        {
            hr = THR(g_uxQuickCopyBuffer.GetTextSelection(pBag->_hUnicodeText,
                                                          TRUE,
                                                          pvarargOut));
        }
        else
        {
            hr = THR(g_uxQuickCopyBuffer.GetTextSelection(pBag->_hText,
                                                          FALSE,
                                                          pvarargOut));
        }
        goto Cleanup;
    }
#endif // UNIX

    hr = THR(pBag->QueryInterface(IID_IDataObject, (void **) & pDO ));
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->Doc()->SetClipboard( pDO ) );
    if (hr)
        goto Cleanup;

Cleanup:
    if( pBag )
    {
        pBag->Release();
    }
    ReleaseInterface( pDO );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );

    RRETURN( hr );
}


HRESULT 
CDoc::InsertMaximumText ( OLECHAR * pstrText, 
                          LONG cch,
                          IMarkupPointer * pIMarkupPointer )
{   
    HRESULT             hr = S_OK ;
    ELEMENT_TAG         eTag = ETAG_NULL;
    CTreeNode *         pNode = NULL;
    CMarkupPointer *    pPointer = NULL;
    LONG                lActualLen = cch;
    
    if (! pIMarkupPointer)
    {
        hr = E_FAIL;
        goto Cleanup;
    }                

    hr = THR( 
            pIMarkupPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointer ) );

    if (hr)
        goto Cleanup;

    pNode = pPointer->CurrentScope( MPTR_SHOWSLAVE );
    if ( !pNode )
    {
        hr = E_FAIL;
        goto Cleanup;
    }        

    eTag = pNode->Tag();    
    
    if( eTag == ETAG_INPUT || eTag == ETAG_TXTSLAVE )
    {
        CFlowLayout * pLayout = GetFlowLayoutForSelection( pNode );
        Assert( pLayout );
        if ( ! pLayout )
        {
            hr = E_FAIL;
            goto Cleanup;
        }
   
        if( lActualLen < 0 )
            lActualLen = pstrText ? _tcslen( pstrText ) : 0;
            
        LONG lMaxLen = pLayout->GetMaxLength();
        LONG lContentLen = pLayout->GetContentTextLength();
        LONG lCharsAllowed = lMaxLen - lContentLen;

        if( lActualLen > lCharsAllowed )
        {
            lActualLen = lCharsAllowed;
        }

        if( lActualLen <= 0 )
            goto Cleanup;
          
    }
   
    hr = InsertText( pPointer, pstrText, lActualLen );
    if( hr )
        goto Cleanup;
    
Cleanup:
    RRETURN( hr );
}


HRESULT
CDoc::IsInsideURL (
    IMarkupPointer * pStart,
    IMarkupPointer * pRight,
    BOOL * pfRet )
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pmp;

    hr = THR( pStart->QueryInterface( CLSID_CMarkupPointer, (void **) & pmp ) );

    if (hr)
        goto Cleanup;

    hr = THR( pmp->IsInsideURL( pRight, pfRet ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CDoc::GetDocHostUIHandler ( IDocHostUIHandler * * ppIHandler )
{
    Assert( ppIHandler );

    *ppIHandler = _pHostUIHandler;

    if (*ppIHandler)
        (*ppIHandler)->AddRef();

    return S_OK;
}



//+====================================================================================
//
// Method: IsElementSized
//
// Synopsis: Does this element have a width or height ?
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::IsElementSized( IHTMLElement* pIElement, BOOL* pfSized )
{
    HRESULT hr = S_OK;
    BOOL fWidthSet = FALSE;
    BOOL fHeightSet = FALSE;
    CUnitValue  uvWidth;
    CUnitValue  uvHeight;
    CElement* pElement = NULL;

    hr = THR( pIElement->QueryInterface( CLSID_CElement, ( void** ) & pElement ));
    if ( hr )
        goto Cleanup;

    if (! pElement->GetFirstBranch())
    {
        hr = E_FAIL;
        goto Cleanup ;
    }
    
    uvWidth         = pElement->GetFirstBranch()->GetCascadedwidth();
    uvHeight        = pElement->GetFirstBranch()->GetCascadedheight();
    fWidthSet       = ! uvWidth.IsNull();
    fHeightSet      = ! uvHeight.IsNull();

    if ( pfSized )
        *pfSized = ( fWidthSet || fHeightSet );
    
Cleanup:
    RRETURN( hr );
}

// Get the line direction at the current markup pointer

HRESULT
CDoc::GetLineDirection(IMarkupPointer *pPointer, BOOL fAtEndOfLine, long *peHTMLDir)
{
    HRESULT             hr = S_OK;
    CMarkupPointer *    pPointerInternal;
    CFlowLayout *       pFL;
    CTreeNode *         pNode = NULL;
    CCharFormat const * pCharFormat = NULL;
    LONG                cp = 0;
    BOOL                fRTLLine = FALSE;

    if(!peHTMLDir)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if (hr)
        goto Cleanup;

    Assert( pPointerInternal->IsPositioned() );
    pNode = pPointerInternal->CurrentScope(MPTR_SHOWSLAVE); 
    if( pNode == NULL )
    {
        hr = OLE_E_BLANK;
        goto Cleanup;
    }

    pCharFormat = pNode->GetCharFormat();
    pFL = GetFlowLayoutForSelection(pNode);

    if(!pFL)
    {
        hr = OLE_E_BLANK;
        goto Cleanup;
    }
    
    //
    // fill in the structure with metrics from current position
    //

    {
        CDisplay * pdp = pFL->GetDisplay();
        BOOL fRTLDisplay = pdp->IsRTL();
        CLinePtr   rp(pdp);
        cp = pPointerInternal->GetCp();

        if(!rp.RpSetCp(cp, fAtEndOfLine))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (rp.IsValid())
            fRTLLine = rp->_fRTL;
        else
            fRTLLine = fRTLDisplay;
    }

Cleanup:
    *peHTMLDir = fRTLLine ? htmlDirRightToLeft : htmlDirLeftToRight;

    RRETURN(hr);    
}
