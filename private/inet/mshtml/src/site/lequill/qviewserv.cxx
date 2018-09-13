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

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
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

#ifndef X_QUILGLUE_H_
#define X_QUILGLUE_H_
#include "quilglue.hxx"
#endif

// Externs
void    ConvertVariantFromTwipsToHTML(VARIANT *);
HRESULT ConvertVariantFromHTMLToTwips(VARIANT *);

MtDefine(CDocRegionFromMarkupPointers_aryRects_pv, Locals, "CDoc::RegionFromMarkupPointers aryRects::_pv")

DeclareTag(tagSelectionTimer, "Selection", "Selection Timer Actions in CDoc")
DeclareTag(tagViewServicesErrors, "ViewServices", "Show Viewservices errors")

////////////////////////////////////////////////////////////////
//    IHTMLViewServices methods


HRESULT
CDoc::MoveMarkupPointerToPoint( 
    POINT               pt, 
    IMarkupPointer *    pPointer, 
    BOOL *              pfNotAtBOL, 
    BOOL *              pfRightOfCp, 
    BOOL                fScrollIntoView )
{
    RRETURN( THR( MoveMarkupPointerToPointEx( pt, pPointer, TRUE, pfNotAtBOL, pfRightOfCp, fScrollIntoView ))); // Default to global coordinates
}

HRESULT 
CDoc::MoveMarkupPointerToPointEx(
    POINT               pt,
    IMarkupPointer *    pPointer,
    BOOL                fGlobalCoordinates,
    BOOL *              pfNotAtBOL,
    BOOL *              pfRightOfCp,
    BOOL                fScrollIntoView )
{
    HRESULT hr = E_FAIL;
    CMarkupPointer * pPointerInternal = NULL;
    POINT ptContent;
    
    CTreeNode * pTreeNode = GetNodeFromPoint( pt, fGlobalCoordinates, &ptContent );
    
    if( pTreeNode == NULL )
        goto Cleanup;

    hr = THR( pPointer->QueryInterface( CLSID_CMarkupPointer, (void **) &pPointerInternal ));
    if( FAILED( hr ))
        goto Cleanup;

    hr = THR( MovePointerToPointInternal( ptContent, pTreeNode, pPointerInternal, pfNotAtBOL, pfRightOfCp, fScrollIntoView ));

Cleanup:
    RRETURN( hr );
}


HRESULT
CDoc::MoveMarkupPointerToMessage( 
    IMarkupPointer *    pPointer ,
    SelectionMessage *  pMessage,
    BOOL *              pfNotAtBOL,
    BOOL *              pfRightOfCp,
    BOOL *              pfValidTree,
    BOOL                fScrollIntoView )
{
    HRESULT             hr = S_OK;
    BOOL                fValidTree = FALSE;
    CTreeNode *         pTreeNode = NULL;
    CTreePos*           ptp = NULL;
    CMarkupPointer *    pPointerInternal = NULL;
    CMarkup *           pPointerMarkup = NULL;
    CMarkup *           pCookieMarkup = NULL;

    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if( FAILED( hr ))
        goto Cleanup;
        
    pTreeNode = static_cast<CTreeNode*>( pMessage->elementCookie );

    //
    // HACK : If pTreeNode is NULL or is the ROOT, we assume we are doing a virtual hit
    // test and the mouse is out of bounds. Instead of doing the fValidTree junk,
    // we use the global point translated to local coordinates on the passed in Markup
    // Pointer's layout. This is a complete hack for moving the end point of selection
    // around correctly. (johnbed)
    // marka - just using the layout of the passed in pointer won't work !
    //

    // Known Issues
    // 1) We are using flow layout boundaries here. We should be making this determination
    //    in the editor, not way down here. This breaks the ability to virtual hit test
    //    in tables, for instance.

    if( pTreeNode == NULL || pTreeNode->Element()->Tag() == ETAG_ROOT  )
    {
        CFlowLayout * pLayout = NULL;

        pTreeNode = GetNodeFromPoint( pMessage->pt , TRUE, NULL, NULL);
        if ( ! pTreeNode )
            pTreeNode = PrimaryMarkup()->GetElementClient()->GetFirstBranch();

        Assert( pTreeNode );
        // Stuff the resolved tree node back into the message...
        pMessage->elementCookie = static_cast<VOID *>( pTreeNode );

        pLayout = GetFlowLayoutForSelection( pTreeNode );
        if( pLayout == NULL )
            goto Cleanup;
        
        CPoint myPt( pMessage->pt );
        pLayout->TransformPoint( &myPt, COORDSYS_GLOBAL, COORDSYS_CONTENT, NULL );
        
        // Stuff the resolved point back into the message...
        pMessage->ptContent.x = myPt.x;
        pMessage->ptContent.y = myPt.y;
        
        fValidTree = TRUE; // 'cause we are forcing it to be true...
    }
    else
    {

        //
        // Figure out if we are in the same tree as the passed in pointer. Fail if the 
        // passed in pointer is unpositioned. Note that we have to do this BEFORE we
        // move the tree pointer.
        //

        pCookieMarkup = pTreeNode->GetMarkup();
        ptp = pPointerInternal->TreePos();

        if( ptp )
        {
            pPointerMarkup = ptp->GetMarkup();
            fValidTree = ( pCookieMarkup == pPointerMarkup );
        }
        else
        {
            fValidTree = FALSE;
        }
    }
    
    hr = THR( MovePointerToPointInternal( pMessage->ptContent, pTreeNode, pPointerInternal, pfNotAtBOL, pfRightOfCp, fScrollIntoView ));
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
    BOOL *              pfRightOfCp,
    BOOL                fScrollIntoView )
{
    HRESULT         hr = S_OK;
    LONG            cp = 0;
    BOOL            fNotAtBOL;
    BOOL            fRightOfCp;
    BOOL            fPsuedoHit = TRUE;
    CFlowLayout *   pFlowLayout = NULL;
    CMarkup *       pMarkup = NULL;
    CTreePos *      ptp = NULL;
    BOOL            fPtNotAtBOL = FALSE;
    
    pFlowLayout = GetFlowLayoutForSelection( pNode );
    if(!pFlowLayout)
        goto Error;

    {
        LONG cpMin = pFlowLayout->GetContentFirstCp();
        LONG cpMax = pFlowLayout->GetContentLastCp();
        CLinePtr rp( pFlowLayout->GetDisplay() );

        //
        // BUGBUG : (johnbed) this is a completely arbitrary hack, but I have to
        // improvise since I don't know what line I'm on
        //
        fPtNotAtBOL = tContentPoint.x > 15;   

        if (!FExternalLayout())
		{
			cp = pFlowLayout->GetDisplay()->CpFromPoint( tContentPoint, // Point
														 &rp,           // Line Pointer
														 &ptp,          // Tree Pos
														 NULL,          // Flow Layout
														 FALSE,         // stuff...
														 FALSE,
														 FALSE,
														 &fRightOfCp,
														 NULL, 
														 NULL );
		}
		else
		{
			if (pFlowLayout->GetQuillGlue())
			{
				cp = pFlowLayout->GetQuillGlue()->CpFromPoint( tContentPoint, // Point
														 &rp,           // Line Pointer
														 &ptp,          // Tree Pos
														 NULL,          // Flow Layout
														 FALSE,         // stuff...
														 FALSE,
														 FALSE,
														 &fRightOfCp,
														 NULL, 
														 NULL );
			}
#if DBG == 1 // Debug Only
			else
			{
				Assert(pFlowLayout->GetQuillGlue() != NULL);
			}
#endif
		}

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

    if( pfRightOfCp )
        *pfRightOfCp = fRightOfCp;

    pMarkup = pFlowLayout->GetContentMarkup();
    if( ! pMarkup )
        goto Error;

    hr = pPointer->MoveToCp( cp, pMarkup );
    if( hr )
        goto Error;
        
    Assert( pPointer->TreePos()->GetCp( FALSE ) == cp );
    
#if DBG == 1 // Debug Only

    //
    // Test that the pointer is in a valid position
    //
    
    {
        CTreeNode * pTst = pPointer->CurrentScope();
        
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
        LONG d = 5;
        CRect r( tContentPoint.x - d, tContentPoint.y - d, tContentPoint.x + d, tContentPoint.y + d );
        pFlowLayout->ScrollRectIntoView( r, SP_MINIMAL, SP_MINIMAL );
    }

    goto Cleanup;
    

Error:
    TraceTag( ( tagViewServicesErrors, " MovePointerToPointInternal --- Failed "));
    return( E_UNEXPECTED );
    
Cleanup:
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

    pNode = pPointerInternal->CurrentScope();
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
        pInfo->wFontSize = pCFormat->GetHeightInTwips( this );
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

HRESULT 
CDoc::ConvertVariantFromHTMLToTwips(VARIANT *pvarargIn)
{
    return ::ConvertVariantFromHTMLToTwips(pvarargIn);
}

HRESULT
CDoc::ConvertVariantFromTwipsToHTML(VARIANT *pvarargIn)
{
    ::ConvertVariantFromTwipsToHTML(pvarargIn);
    return S_OK;
}

HRESULT
CDoc::GetLineInfo(IMarkupPointer *pPointer, BOOL fAtEndOfLine, HTMLPtrDispInfoRec *pInfo)
{
    HRESULT             hr = S_OK;
    CMarkupPointer *    pPointerInternal;
    CFlowLayout *       pFlowLayout;
    POINT               pt;
    CTreePos *          ptp = NULL;
    CTreeNode *         pNode = NULL;
    CCharFormat const * pCharFormat = NULL;

    LONG cp;

    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if (hr)
        goto Cleanup;

    Assert( pPointerInternal->IsPositioned() );
    
    ptp = pPointerInternal->TreePos();
    pNode = pPointerInternal->CurrentScope();
   
    pCharFormat = pNode->GetCharFormat();
    pFlowLayout = GetFlowLayoutForSelection(pNode);

    if(!pFlowLayout)
    {
        hr = OLE_E_BLANK;
        goto Cleanup;
    }
    
	if (FExternalLayout())
	{
		if (pFlowLayout->GetQuillGlue())
		{
			hr = pFlowLayout->GetQuillGlue()->GetLineInfo(ptp, pFlowLayout, fAtEndOfLine, pInfo);
		}
		else
		{
			Assert(("No Quill !!!", pFlowLayout->GetQuillGlue() != NULL));
			hr = OLE_E_BLANK;
			goto Cleanup;
		}
	}
	else

    //
    // fill in the structure with metrics from current position
    //

    {
        CLinePtr   rp(pFlowLayout->GetDisplay());
        cp = ptp->GetCp();
        CCalcInfo CI;
        CI.Init(pFlowLayout);
        BOOL fComplexLine;

        //
        // Query Position Info
        //
        
        if (-1 == pFlowLayout->GetDisplay()->PointFromTp( cp, NULL, fAtEndOfLine, pt, &rp, TA_BASELINE, &CI, &fComplexLine ))
        {
            hr = OLE_E_BLANK;
            goto Cleanup;
        }
        pInfo->lXPosition = pt.x;
        pInfo->lBaseline = pt.y;
        pInfo->fRightToLeft = rp->_fRTL;

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
            CCcs *              pccs;

            pccs = fc().GetCcs( CI._hdc, &CI, pCharFormat );

            if(!pccs)
                goto Cleanup;

            pInfo->lTextHeight = pccs->_yHeight + pccs->_yOffset;
            pccs->Release();
        }
    }

Cleanup:
    RRETURN(hr);    
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
CDoc::MoveMarkupPointer(
    IMarkupPointer *    pPointer, 
    LAYOUT_MOVE_UNIT    eUnit, 
    LONG                lXCurReally, 
    BOOL *              pfNotAtBOL )
{
    HRESULT hr = S_OK;
    CFlowLayout *pFlowLayout;
    CElement *pElemInternal = NULL;
    CTreeNode   *pNode;
    CMarkup * pMarkup = NULL;
    CMarkupPointer *pPointerInternal = NULL;
    LONG cp;

    // InterfacePointers

    
    hr = pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal);
    if (hr)
        goto Cleanup;

    // get element for current position so we can get it's flow layout    
    pNode = pPointerInternal->CurrentScope();
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
    cp = pPointerInternal->TreePos()->GetCp();

    //
    // use layout to get new position
    //

    pMarkup = pNode->GetMarkup();
    // Get the line where we are positioned.

    if (FExternalLayout())
    {
       	if (eUnit != LAYOUT_MOVE_UNIT_TopOfWindow && 
       		eUnit != LAYOUT_MOVE_UNIT_BottomOfWindow)
       	{
			if (pFlowLayout->GetQuillGlue())
				hr = pFlowLayout->GetQuillGlue()->MoveMarkupPointer(pPointerInternal, 
					eUnit, lXCurReally, *pfNotAtBOL);
			else
			{
				Assert(("No Quill !!!", pFlowLayout->GetQuillGlue() != NULL));
				hr = OLE_E_BLANK;
			}
			goto Cleanup;
       	}
    }

    switch (eUnit)
    {
        case LAYOUT_MOVE_UNIT_CurrentLineStart:
        {
            CLinePtr rp(pFlowLayout->GetDisplay());
            rp.RpSetCp( cp , *pfNotAtBOL );
            cp = cp - rp.GetIch();
            // Check that we are sane
            if( cp < 1 || cp > pMarkup->Cch() )
                goto Cleanup;            
            *pfNotAtBOL = FALSE;
            // move pointer to new position
            hr = pPointerInternal->MoveToCp(cp, pMarkup , *pfNotAtBOL );
            break;
        }
        
        case LAYOUT_MOVE_UNIT_CurrentLineEnd:
        {
            CLinePtr rp(pFlowLayout->GetDisplay());
            rp.RpSetCp( cp , *pfNotAtBOL );
            cp = (cp - rp.GetIch()) + rp.GetAdjustedLineLength();
            // Check that we are sane
            if( cp < 1 || cp > pMarkup->Cch() )
                goto Cleanup;            
            *pfNotAtBOL = TRUE ;
            // move pointer to new position
            hr = pPointerInternal->MoveToCp(cp, pMarkup , *pfNotAtBOL );
            break;
        }
        
        case LAYOUT_MOVE_UNIT_TopOfWindow:
        {
            //
            // BUGBUG: We need to be a bit more precise about where the
            // top of the first line is
            //
            
            // Go to top of window, not container...
            POINT pt;
            pt.x = 12;
            pt.y = 12;
            
            hr = THR( MoveMarkupPointerToPointEx( pt, pPointer, TRUE, pfNotAtBOL, NULL, FALSE ));

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

            CView * pView = GetActiveView();

            if( pView == NULL )
                goto Cleanup;
            
            pView->GetViewSize( &szScreen );
            pt = szScreen.AsPoint();

            Assert( pt.x > 0 && pt.y > 0 );
            pt.x -= 25;
            pt.y -= 25;
            hr = THR( MoveMarkupPointerToPointEx( pt, pPointer, TRUE, pfNotAtBOL, NULL, FALSE ));

            goto Cleanup;                
        }
        
        case LAYOUT_MOVE_UNIT_PreviousLine:
        {
            CLinePtr rp(pFlowLayout->GetDisplay());        
            // Move one line up. This may cause the txt site to be different.
            rp.RpSetCp( cp , *pfNotAtBOL );
            pFlowLayout = pFlowLayout->GetDisplay()->MoveLineUpOrDown(NAVIGATE_UP, rp, lXCurReally, &cp, pfNotAtBOL);
            // Check that we are sane
            if( cp < 1 || cp > pMarkup->Cch() )
                goto Cleanup;

            // move pointer to new position
            hr = pPointerInternal->MoveToCp(cp, pMarkup , *pfNotAtBOL );
            break;
        }
        
        case LAYOUT_MOVE_UNIT_NextLine:
        {
            CLinePtr rp(pFlowLayout->GetDisplay());
            // Move down line up. This may cause the txt site to be different.
            rp.RpSetCp( cp , *pfNotAtBOL );
            pFlowLayout = pFlowLayout->GetDisplay()->MoveLineUpOrDown(NAVIGATE_DOWN, rp, lXCurReally, &cp, pfNotAtBOL);
            // Check that we are sane
            if( cp < 1 || cp > pMarkup->Cch() )
                goto Cleanup;
                
            // move pointer to new position
            hr = pPointerInternal->MoveToCp(cp, pMarkup , *pfNotAtBOL );
            break;
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
    cpStart = pStart->TreePos()->GetCp();
    cpEnd = pEnd->TreePos()->GetCp();

    //Get the flow layout that the markup pointer is placed in.
    pTreeNode = pStart->CurrentScope();
    if ( !pTreeNode )
        goto Error;

    pFlowLayout = GetFlowLayoutForSelection(pTreeNode);
    if ( !pFlowLayout )
        goto Error;

    // get the element we are in.
    pElem = pTreeNode->Element();
    
    // Get the rectangles.
    pFlowLayout->GetDisplay()->RegionFromElement( pElem, paryRects,  
                                                    NULL, NULL, 0 , cpStart, 
                                                    cpEnd, pBoundingRect );

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

    pLayout = pElement->GetLayout();

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
CDoc::IsCaretVisible()
{
    BOOL fVisible = FALSE;
    
    if ( _pCaret )
    {
        _pCaret->IsVisible( & fVisible );
    }
    
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
    
    hr = _pCaret->QueryInterface( IID_IHTMLCaret, (void **) ppCaret );

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

    *fResult = pElement->HasLayout();

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
        *pfHTML = pElement->HasFlag(TAGDESC_ACCEPTHTML);

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

    if (! pIPointer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointer->IsPositioned( & fPositioned, NULL ) );
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

    hr = S_FALSE;

    pTreeNode = (pMarkupPointer->IsPositioned() ) ? pMarkupPointer->CurrentScope() : NULL;
    if (! pTreeNode)
        goto Cleanup;

    pFlowLayout = GetFlowLayoutForSelection(pTreeNode);
    if (! pFlowLayout)
        goto Cleanup;

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
    
    EnsureTotalOrder ( pPointerStart, pPointerFinish );

    HRESULT
    HandleUIPasteHTML (
        CMarkupPointer * pPointerTargetStart,
        CMarkupPointer * pPointerTargetFinish,
        HGLOBAL          hglobal );

    hr = THR( HandleUIPasteHTML( pPointerStart, pPointerFinish, hGlobalHtml ) );

Cleanup:

    RRETURN( hr );
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
    if (SUCCEEDED(hr))
        pElement->_fBreakOnEmpty = TRUE;
        
    RRETURN ( hr );
} 

//+====================================================================================
//
// Method:InflateBlockElement
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
        pLayout = pElement->GetFlowLayout();
        if (pLayout)
            *pfMultiLine = pLayout->GetMultiLine();
    }
    
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
CDoc::FireOnSelectStartFromMessage(
        SelectionMessage *pSelectionMessage )
{
    CTreeNode*  pNode = static_cast< CTreeNode * >( pSelectionMessage->elementCookie );
    
    if( pNode == NULL )
        return( E_FAIL );
    else
        return( THR( pNode->Element()->Fire_onselectstart(pNode)));
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
    void** ppeCookie)
{
    HRESULT hr = S_OK;
    CElementAdorner * pAdorner = NULL;
    CElement* pElement = NULL ;
    CView* pView;

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
     
    pView = GetActiveView() ;

    if ( !pView )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pAdorner = DYNCAST( CElementAdorner, pView->CreateAdorner( pElement ) );

    if ( !pAdorner )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pAdorner->SetSite( pIElementAdorner );
    pAdorner->AddRef();

    *ppeCookie = static_cast<void*> ( pAdorner ) ;

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
    void* peCookie)
{
    HRESULT hr;

    if ( !peCookie )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CElementAdorner* pAdorner = static_cast<CElementAdorner*> (peCookie);
        CView* pView = pAdorner->GetView();
        if ( ( pView ) && ( _ulRefs != ULREF_IN_DESTRUCTOR ) && ( pView->Doc() != NULL ) )
        {
            pView->OpenView();
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
    void* peAdornerCookie,
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
        pAdorner = static_cast<CElementAdorner*> ( peAdornerCookie);
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
            if ( pCurNode->Element()->HasFlag(TAGDESC_CONTAINER) )
            {
                pOuterElement = pCurNode->Element();
                break;
            }
            pCurNode = pCurNode->Parent();
        }

        if (( pOuterElement->_etag == ETAG_BODY ) && ( fIgnoreOutermostContainer )) 
            pOuterElement = FALSE;
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

    pDispNode = pFlowLayout->GetElementDispNode( NULL, TRUE );

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

    pNode = pPtr->CurrentScope();

    while( pNode && ! fDone )
    {
        pTest = pNode->Element();
        if( pTest == NULL )
            goto Cleanup;
            
        pLayout = pTest->GetLayout();
        pDispNode = (pLayout == NULL ? NULL : pLayout->GetElementDispNode( NULL, TRUE ));

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
    LONG *          plCpMaybe /* = NULL */ )
{   
    CTreeNode *         pTreeNode = NULL;
    
    POINT               ptContent;
    CDispNode *         pDispNode = NULL;
    DWORD               dwCoordSys;
    CView *             pView =     NULL;
    HITTESTRESULTS      HTRslts;
    HTC                 htcResult = HTC_NO;
    DWORD               dwHTFlags = HT_VIRTUALHITTEST |       // Turn on virtual hit testing
                                    HT_NOGRABTEST |           // Ignore grab handles if they exist
                                    HT_IGNORESCROLL;          // Ignore testing scroll bars                                  

    if( fGlobalCoordinates )
        dwCoordSys = CDispNode::COORDSYS_GLOBAL;    
    else
        dwCoordSys = CDispNode::COORDSYS_CONTENT;

    //
    //  Do a hit test to figure out what node would be hit by this point.
    //  I know this seems like a lot of work to just get the TreeNode,
    //  but we have to do this. Also, we can't trust the cp returned in
    //  HTRslts. Some day, perhaps we can.
    //

    pView = GetActiveView();
    if( pView == NULL ) 
        goto Cleanup;

    htcResult = pView->HitTestPoint( pt, 
                                     dwCoordSys, 
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
        *plCpMaybe = HTRslts.cpHit;
    }
    
    return pTreeNode;
}


CFlowLayout *
CDoc::GetFlowLayoutForSelection( CTreeNode * pNode )
{
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
                     FALSE,
                     FALSE );

    _fInEditCapture = TRUE ;
    RRETURN ( hr );
}



HRESULT
CDoc::HTMLEditorReleaseCapture( )
{
    HRESULT hr = S_OK;

    Assert( _fInEditCapture );
    SetMouseCapture(NULL, NULL);
    _fInEditCapture = FALSE ; 

    RRETURN ( hr );
}


//+====================================================================================
//
// Method: AdjustEditContext
//
// Synopsis: External Adjust EditContext Routine.
//
//------------------------------------------------------------------------------------


HRESULT
CDoc::AdjustEditContext (   IHTMLElement* pStartElement, 
                            IHTMLElement** ppEditThisElement, 
                            IMarkupPointer* pIStart,
                            IMarkupPointer* pIEnd , 
                            BOOL * pfEditThisEditable,
                            BOOL * pfEditParentEditable,
                            BOOL * pfNoScopeElement )
{
    HRESULT hr = S_OK;
    CElement* pElementInternal = NULL;
    CElement* pOuterElement = NULL;
    
    hr = THR( pStartElement->QueryInterface( CLSID_CElement , ( void**) & pElementInternal ));
    if ( hr )
        goto Cleanup;

    hr = THR( AdjustEditContext(
                                pElementInternal,
                                & pOuterElement,
                                pIStart,
                                pIEnd,
                                pfEditThisEditable,
                                pfEditParentEditable,
                                pfNoScopeElement,
                                TRUE ));
    if ( hr )
        goto Cleanup;

    if ( ( !hr ) && ( ppEditThisElement ) )
    {
        Assert( pOuterElement );
        hr = THR( pOuterElement->QueryInterface( IID_IHTMLElement, (void**) ppEditThisElement ));
    }

Cleanup:

    RRETURN( hr );    
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
                            BOOL fSetCurrencyIfInvalid /* = FALSE*/)
{
    HRESULT hr = S_OK;
    CElement* pEditableElement = NULL;
    CElement* pCurElement = NULL;
    IHTMLElement* pICurElement = NULL;
    CTreeNode* pCurNode = FALSE;
    BOOL fNoScope = FALSE;
    
    if ( pStartElement->HasSlaveMarkupPtr())
    {
        pEditableElement = pStartElement->GetSlaveMarkupPtr()->FirstElement();
    }
    else if ( _fDesignMode )
    {
        //
        // Look for the first container. This is our editable element
        //
        
        if ( pStartElement->IsNoScope() )
        {
            pEditableElement = pStartElement;
            fNoScope = TRUE;
        }            
        else
        {
            pCurNode = pStartElement->GetFirstBranch();
            while ( pCurNode )
            {
                pCurElement = pCurNode->Element();

                //
                // We stop on Containers, Absolute or Relative Positioned objects
                // or objects that have a width or a height
                //

                // BUGBUG: this logic is probably still wrong according to JohnBed, 
                //         but works better than the code under ifdef NEVER
                //
                if ( ( pCurElement->HasFlowLayout() ) ||
                     ( pCurElement->_etag == ETAG_TXTSLAVE ) )
#ifdef NEVER
                     ( pCurElement->HasFlag(TAGDESC_CONTAINER) ) ||
                     ( pCurElement->HasFlag(TAGDESC_ACCEPTHTML) ) ||            
                     ( pCurNode->IsAbsolute() ) ||
                     ( pCurNode->IsRelative() ) ||
                     ( ! (pCurNode->GetCascadedwidth()).IsNull() ) ||
                     ( ! (pCurNode->GetCascadedheight()).IsNull() ) )
#endif            
                {
                    pEditableElement = pCurElement;
                    break;
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
        BOOL fFoundEditableElement = pStartElement->_fEditAtBrowse;

        pCurNode = pStartElement->GetFirstBranch();
        while ( pCurNode )
        {
            pCurElement = pCurNode->Element();
            
            if ( ( pCurElement->_fEditAtBrowse ) || 
                 ( pCurElement->_etag == ETAG_TXTSLAVE ) )
            {
                pEditableElement = pCurElement;          
                fFoundEditableElement = TRUE;
                if ( pCurElement->_etag == ETAG_TXTSLAVE )
                    break;
            }
            
            if ( fFoundEditableElement && ( !pCurElement->_fEditAtBrowse ) )
            {
                break;
            }
            pCurNode = pCurNode->Parent();
        }
        
        if ( ! fFoundEditableElement )
        {
            //
            // We're in browse mode, there is nothing editable.
            // assume this is a selection in the body. Make the context the body.
            //
            pEditableElement = PrimaryMarkup()->GetElementClient();
        }    
        
    }

    // Clear the old selection, if the edit context is changing
    if (_pElemEditContext != pEditableElement)
    {
        hr =  THR(NotifySelection(SELECT_NOTIFY_DESTROY_SELECTION, NULL));
        if (hr)
            goto Cleanup;
    }

    if ( pEditableElement )
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

    _pElemEditContext = pEditableElement;

    
    if ( ppEditThisElement )
    {

        *ppEditThisElement = pEditableElement;
    }

    if ( pfEditThisEditable )
    {
        *pfEditThisEditable =  pEditableElement->IsEditable(FALSE);
    }

    if ( pfEditParentEditable )
    {
        //
        // BUGBUG - more master/slave bizzaritude ! Getting the parent on the slave
        // goes to the root.
        //
        
        if ( pEditableElement->_etag != ETAG_TXTSLAVE )       
            pCurNode = pEditableElement->GetFirstBranch()->Parent();
        else
            pCurNode = pEditableElement->MarkupMaster()->GetFirstBranch()->Parent();

        if ( pCurNode )
            * pfEditParentEditable = pCurNode->Element()->IsEditable(FALSE);
    }

    if ( pfNoScopeElement )
    {
        *pfNoScopeElement = fNoScope;
        
    }
Cleanup:

    ReleaseInterface( pICurElement );
    RRETURN ( hr );
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
    HTMLPtrDispInfoRec liStart, liEnd;        
    int deltaY = 0;
    CRect rc;
    
    hr = THR(GetLineInfo( pStart, FALSE, &liStart ));
    if ( hr )
        goto Cleanup;
        
    hr = THR( GetLineInfo( pEnd, FALSE, & liEnd ));
    if ( hr )
        goto Cleanup;
        

    
    deltaY =  2 * max( liStart.lLineHeight, liEnd.lLineHeight );     // I want at least 2 lines of visiblity in all directions - may take tweaking to get right
    rc.left = min( liStart.lXPosition, liEnd.lXPosition);
    rc.top = min( liStart.lBaseline, liEnd.lBaseline) - deltaY ;
    rc.right = rc.left + abs( liEnd.lXPosition - liStart.lXPosition ) ;
    rc.bottom = rc.top + abs( liEnd.lBaseline - liStart.lBaseline ) + deltaY ;


    hr = THR( pStart->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if ( hr )
        goto Cleanup;
        
    pNode = pPointerInternal->CurrentScope();
    pFlowLayout = GetFlowLayoutForSelection( pNode );
    pFlowLayout->TransformRect( & rc, COORDSYS_GLOBAL, COORDSYS_CONTENT, FALSE, NULL );
    pFlowLayout->ScrollRectIntoView( rc, SP_BOTTOMRIGHT , SP_BOTTOMRIGHT );
    
Cleanup:
    RRETURN ( hr );
}

HRESULT
CDoc::ScrollPointerIntoView(
    IMarkupPointer *    pPointer,
    POINTER_SCROLLPIN   eScrollAmount )
{
    // TODO: Implement

    HRESULT hr = S_OK;
    CMarkupPointer * pPointerInternal = NULL;
    CTreeNode * pNode = NULL;
    CFlowLayout * pFlowLayout = NULL;
    HTMLPtrDispInfoRec LineInfo;
    SCROLLPIN ePin = SP_MINIMAL;
    GetLineInfo( pPointer, FALSE, &LineInfo );
    LONG x, y, delta;
    x = LineInfo.lXPosition;
    y = LineInfo.lBaseline;
    delta = 2 * LineInfo.lLineHeight;     // I want at least 2 lines of visiblity in all directions - may take tweaking to get right
    
    CRect rc( x - delta, y - delta, x + delta, y + delta );

    hr = THR( pPointer->QueryInterface(CLSID_CMarkupPointer, (void **)&pPointerInternal ));
    if (hr)
       return E_FAIL;

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

    pNode = pPointerInternal->CurrentScope();
    pFlowLayout = GetFlowLayoutForSelection( pNode );
    pFlowLayout->ScrollRectIntoView( rc, ePin , ePin );
    return hr;
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
    *pfSameMarkup = ( pM1 != NULL && ((DWORD) pM1 == (DWORD) pM2 ));
    
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
        
    pElement->DragElement(
        GetFlowLayoutForSelection( _pElemEditContext->GetFirstBranch() ),
        dwKeyState, 
        NULL);
        
Cleanup:
    RRETURN ( hr );
    
}

