
#include "headers.hxx"

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__RANGE_H_
#define X__RANGE_H_
#include "_range.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_RANGE_H_
#define X_RANGE_H_
#include "range.h"
#endif

#ifndef X_EFONT_HXX_
#define X_EFONT_HXX_
#include "efont.hxx"
#endif

#ifndef X_DOBJ_HXX_
#define X_DOBJ_HXX_
#include "dobj.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#include "quxcopy.hxx"

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_EPARA_HXX_
#define X_EPARA_HXX_
#include "epara.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

// externs
MtDefine(CTxtRangeAutoUrl_UpdateAnchorFromHRefIfSynched_aryNewAnchorGA_pv, Locals, "CTxtRange::AutoUrl_UpdateAnchorFromHRefIfSynched aryNewAnchorGA::_pv")
MtDefine(CTxtRangeAutoUrl_UpdateAnchorFromHRefIfSynched_pszAnchorText, Locals, "CTxtRange::AutoUrl_UpdateAnchorFromHRefIfSynched pszAnchorText")
MtDefine(CTxtRangeAutoUrl_SetUrl_aryHRefGA_pv, Locals, "CTxtRange::AutoUrl_SetUrl aryHRefGA::_pv")
MtDefine(CTxtRangeUrlAutodetector_aryWordGA_pv, Locals, "CTxtRange::UrlAutodetector aryWordGA::_pv")
MtDefine(CTxtRangeExecFormatAnchor_arypElements_pv, Locals, "CTxtRange::ExecFormatAnchor arypElements::_pv")
MtDefine(CTxtRangeExecRemoveAnchor_arypElements_pv, Locals, "CTxtRange::ExecRemoveAnchor arypElements::_pv")
MtDefine(CTxtRangeQueryStatusAnchor_arypAnchor_pv, Locals, "CTxtRange::QueryStatusAnchor arypAnchor::_pv")

HRESULT ConvertVariantFromHTMLToTwips(VARIANTARG *pvar)
{
    HRESULT hr = S_FALSE;
    long offset = 0;

    Assert(pvar);
    if (((CVariant *)pvar)->IsEmpty())
        goto Cleanup;

    if (V_VT(pvar) == VT_BSTR)
    {
        if (   *V_BSTR(pvar) == _T('+')
            || *V_BSTR(pvar) == _T('-')
           )
        {
            offset = FONT_INDEX_SHIFT;
        }
    }
    hr = THR(VariantChangeTypeSpecial(pvar, pvar, VT_I4));
    if (hr)
        goto Cleanup;
    V_I4(pvar) += offset;

    if (V_I4(pvar) < 1 || V_I4(pvar) > 7)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    V_I4(pvar) = ConvertHtmlSizeToTwips(V_I4(pvar));

Cleanup:
    RRETURN1(hr, S_FALSE);
}

void ConvertVariantFromTwipsToHTML(VARIANTARG *pvar)
{
    Assert(pvar);
    Assert(V_VT(pvar) == VT_I4 || V_VT(pvar) == VT_NULL);

    if (V_VT(pvar) == VT_I4)
    {
        long htmlSize = ConvertTwipsToHtmlSize(V_I4(pvar));

        if (ConvertHtmlSizeToTwips(htmlSize) == V_I4(pvar))
            V_I4(pvar) = htmlSize;
        else
            V_VT(pvar) = VT_NULL;
    }
}


HRESULT 
CTxtRange::GetLeft( IMarkupPointer * ptp )
{
    HRESULT  hr = THR( ptp->MoveToPointer( _pLeft ) );
    RRETURN( hr );
}

HRESULT
CTxtRange::GetLeft( CMarkupPointer * ptp )
{
    HRESULT  hr = THR( ptp->MoveToPointer( _pLeft  ) );
    RRETURN( hr );
}

HRESULT 
CTxtRange::GetRight( IMarkupPointer * ptp )
{
    HRESULT  hr = THR( ptp->MoveToPointer( _pRight ) );
    RRETURN( hr );
}

HRESULT 
CTxtRange::GetRight( CMarkupPointer * ptp )
{
    HRESULT  hr = THR( ptp->MoveToPointer( _pRight ) );
    RRETURN( hr );
}

HRESULT 
CTxtRange::SetLeft( IMarkupPointer * ptp )
{
    HRESULT  hr;
    
    // BUGBUG: aren't we guarranteed to have _pLeft and _pRight
    if (! _pLeft )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    
    hr = THR( _pLeft->MoveToPointer( ptp ) );
    if (hr)
        goto Cleanup;

    hr = THR( ValidatePointers() );

Cleanup:
    RRETURN( hr );
}


HRESULT 
CTxtRange::SetRight( IMarkupPointer * ptp )
{
    HRESULT  hr;
    
    if (! _pRight )
    {
        hr = E_FAIL;
        goto Cleanup;
    }


    hr = THR( _pRight->MoveToPointer( ptp ) );
    if (hr)
        goto Cleanup;

    hr = THR( ValidatePointers() );

Cleanup:
    RRETURN( hr );
}


HRESULT 
CTxtRange::SetLeftAndRight( 
                                IMarkupPointer * pLeft, 
                                IMarkupPointer * pRight, 
                                BOOL fAdjustPointers /*=TRUE*/ )
{
    HRESULT  hr;
    
    if (! _pRight || !_pLeft)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( _pRight->MoveToPointer( pRight ) );
    if (hr)
        goto Cleanup;

    hr = THR( _pLeft->MoveToPointer( pLeft ) );
    if (hr)
        goto Cleanup;
            
    if ( fAdjustPointers )
    {
        hr = THR( ValidatePointers() );
    }
Cleanup:
    RRETURN( hr );
}

HRESULT 
CTxtRange::GetLeftAndRight( CMarkupPointer * pLeft, CMarkupPointer * pRight )
{
    HRESULT  hr;

    // internal function...
    Assert( pRight );
    Assert( pLeft );
   
    hr = THR( pRight->MoveToPointer( _pRight ) );
    if (hr)
        goto Cleanup;

    hr = THR( pLeft->MoveToPointer( _pLeft ) );

Cleanup:
    RRETURN( hr );
}

CTreeNode * 
CTxtRange::GetNode(BOOL fLeft)
{   
    CMarkupPointer * pmp = NULL;
    HRESULT          hr = S_OK;

    if (fLeft)
        hr = THR_NOTRACE( _pLeft->QueryInterface(CLSID_CMarkupPointer, (void **) & pmp) );
    else
        hr = THR_NOTRACE( _pRight->QueryInterface(CLSID_CMarkupPointer, (void **) & pmp) );
    
    if (hr)
    {
        return NULL;
    }

    return ( pmp && pmp->IsPositioned() ) ? pmp->CurrentScope() : NULL;
}


CTreeNode * 
CTxtRange::LeftNode()
{   
    return ( GetNode( TRUE ) );
}


CTreeNode * 
CTxtRange::RightNode()
{   
    return ( GetNode( FALSE ) );
}


HRESULT
CTxtRange::collapse ( VARIANT_BOOL fStart )
{
    HRESULT hr = S_OK;

    //BUGBUG: DO_SANITY_CHECK

    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if  (fStart)
    {
        hr = THR( _pRight->MoveToPointer( _pLeft ) );
    }
    else
    {
        hr = THR( _pLeft->MoveToPointer( _pRight ) );
    }

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::SetSelectionBlockDirection
//
//  Synopsis:   Sets the text direction in a selection. This is similar to
//              AlignSelection, except we dont do the DIV thing and we add
//              or remove a DIR attribute to the block level element found
//              and then remove any DIRs from the element's children.
//
//  Arguments:  atDir               new direction type
//
//  Returns:    HRESULT             S_OK if succesfull or an error
//
//----------------------------------------------------------------------------

HRESULT
CTxtRange::SetSelectionBlockDirection(htmlDir atDir)
{
#ifdef MERGEFUN // rtp
    CElement *      pElementDiv = NULL;
    HRESULT         hr = S_OK;
    long            iRun,
                    iRunFinish,
                    icpStart,
                    icpEnd,
                    iLastRangeRun,
                    iFirstRangeRun;
    CTreePosList &  eruns = GetList();
    CNotification   nf;
    CPedUndo        pu( GetPed() );
    CTreeNode *     pNodeFlowLayout;
    CFlowLayout *   pFlowLayout;
    CElement *      pContainer = GetCommonContainer();
    htmlDir         dirParent;
    CStr            cstrDir, cstrDirNotSet;

    cstrDirNotSet.Set ( _T("") );
    cstrDir.Set(atDir == htmlDirRightToLeft ? _T("rtl") : _T("ltr"));

    pu.Start( IDS_UNDOBLOCKDIR, this, pContainer->IsEditable() );
    pu.MarkForClearRunCaches();

    GetEndRuns(&iFirstRangeRun, &iLastRangeRun);

    Assert( iFirstRangeRun >= GetFirstRun() && iFirstRangeRun <= GetLastRun() );
    Assert( iLastRangeRun  >= GetFirstRun() && iLastRangeRun  <= GetLastRun() );

    pNodeFlowLayout = eruns.GetFlowLayoutNodeOnBranch( iFirstRangeRun );

    if ( pNodeFlowLayout->IsPed() && DifferentScope(pNodeFlowLayout, GetPed()) )
    {
        pNodeFlowLayout = pNodeFlowLayout->Parent()->GetFlowLayoutNode();
        Assert( pNodeFlowLayout->GetPed() == GetPed() );
    }

    pFlowLayout = DYNCAST(CFlowLayout, pNodeFlowLayout->GetLayout());

    Assert(pFlowLayout);

#if DBG==1
    CTreeNode * pNodeFlowLayoutEnd = eruns.GetFlowLayoutNodeOnBranch( iLastRangeRun );

    if ( pNodeFlowLayoutEnd->IsPed() && DifferentScope(pNodeFlowLayoutEnd, GetPed()) )
    {
        pNodeFlowLayoutEnd = pNodeFlowLayoutEnd->Parent()->GetFlowLayoutNode();
        Assert( pNodeFlowLayoutEnd->GetPed() == GetPed() );
    }

    Assert( SameScope(pNodeFlowLayout, pNodeFlowLayoutEnd) );
#endif

    // if the current selection, selects the first and the last outermost
    // block partially, then update the scope extents
    hr = THR(eruns.ExtendScopeToEndBlocksDirectScope(NULL,
                                    &iFirstRangeRun, &iLastRangeRun));
    if(hr)
        goto Cleanup;

    iRun = iRunFinish = iFirstRangeRun;

    while(iRun <= iLastRangeRun)
    {
        CTreeNode *     pNodeTemp;
        CTreeNode *     pNodeCurrBlock = NULL;
        CElement *      pElementCurrBlock = NULL;
        CTreeNode *     pNode = SearchBranchForBlockElement( iRun, pFlowLayout );
        CElement *      pElementParent;
        CElement *      pElementTemp;

        // Find the topmost block  whose scope is completely
        // in the given range that aligns a block.
        while(!pNode->HasFlowLayout())
        {
            if(!eruns.IsCompleteScopeOfElement(pNode->Element(), iRun, iLastRangeRun))
            {
                break;
            }

            pNodeCurrBlock = pNode;

            pNode = eruns.SearchBranchForBlockElement( pNode->Parent(), pFlowLayout );
        }

        pElementCurrBlock = pNodeCurrBlock->SafeElement();

        // Do we really need to change direction?
        // We will need to know the parent's direction
        if(!pNodeCurrBlock)
            pNodeTemp = GetBranchAbs(iRun);
        else
            pNodeTemp = pNodeCurrBlock;

        pElementParent = pNodeTemp->Parent()->Element();
        pElementTemp = pNodeTemp->SafeElement();
        dirParent = (pNodeTemp->Parent()->GetParaFormat()->HasRTL(TRUE)) ? 
                        htmlDirRightToLeft : htmlDirLeftToRight;

        if(!pElementTemp->IsBlockElement())
        {
            continue;
        }

        if(dirParent == atDir)
        {
            // we we want to remove the dir property so we inherit from the parent
            hr = THR(pElementTemp->put_StringHelper((BSTR)(LPTSTR)cstrDirNotSet, 
                                                    (const PROPERTYDESC *)&s_propdescCElementdir));
        }
        else
        {
            // add the direction flag
            hr = THR(pElementTemp->put_StringHelper((BSTR)(LPTSTR)cstrDir, 
                                                    (const PROPERTYDESC *)&s_propdescCElementdir));
        }

        if(pElementCurrBlock)
        {
            // Found a block element

            hr = THR(eruns.GetElementScope(iRun, pElementCurrBlock,
                                                NULL, &iRunFinish));
            if(hr)
                goto Cleanup;
            // now remove the alignment below the current block, not including it
            hr = THR(eruns.RemoveDirection(pElementCurrBlock,
                                iRun, iRunFinish));
            if(hr)
                goto Cleanup;

            iRun = iRunFinish + 1;
        }
        else
        {
            iRun++;
        }

    }

    pu.ClearRunCaches( iFirstRangeRun, iLastRangeRun );
    eruns.ClearRunCaches(iFirstRangeRun, iLastRangeRun);
    icpStart = eruns.CpAtStartOfRun(iFirstRangeRun);
    icpEnd   = eruns.CpAtStartOfRun(iLastRangeRun + 1);

    Assert( icpStart >= GetFirstCp() && icpStart <= GetLastCp() );
    Assert( icpEnd   >= GetFirstCp() && icpEnd   <= GetLastCp() );
    Assert( iFirstRangeRun >= GetFirstRun() && iFirstRangeRun <= GetLastRun() );
    Assert( iLastRangeRun  >= GetFirstRun() && iLastRangeRun  <= GetLastRun() );

    nf.CharsResize(
        icpStart, icpEnd - icpStart, iFirstRangeRun,
        iLastRangeRun - iFirstRangeRun + 1,
        pFlowLayout->GetFirstBranch());
    _snLast = nf.SerialNumber();
    eruns.Notify( nf );

Cleanup:

    pu.Finish( hr );

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}
    
//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::SetSelectionInlineDirection
//
//  Synopsis:   Sets the text direction in an inline selection. This is similar to
//              SetSelectionBlockDirection. However we place a <SPAN DIR=atDir>
//              around the selection and then remove any DIRs from the element's
//              children.
//
//  Arguments:  atDir               new direction type
//
//  Returns:    HRESULT             S_OK if succesfull or an error
//
//----------------------------------------------------------------------------

HRESULT
CTxtRange::SetSelectionInlineDirection(htmlDir atDir)
{
    HRESULT hr = S_OK;
    // Complex Text - Plumbing for Beta 2 work. Needed for OM Testing

    return hr;
}
    

#ifdef WIN16
#pragma code_seg ( "RANGE2_3_TEXT" )
#endif

//+---------------------------------------------------------------------------

AUTOURL_TAG CTxtRange::s_urlTags[] = {
    { FALSE, {_T("www."),         _T("http://www.")}},
    { FALSE, {_T("http://"),      _T("http://")}},
    { FALSE, {_T("https://"),     _T("https://")}},
    { FALSE, {_T("ftp."),         _T("ftp://ftp.")}},
    { FALSE, {_T("ftp://"),       _T("ftp://")}},
    { FALSE, {_T("gopher."),      _T("gopher://gopher.")}},
    { FALSE, {_T("gopher://"),    _T("gopher://")}},
    { FALSE, {_T("mailto:"),      _T("mailto:")}},
    { FALSE, {_T("news:"),        _T("news:")}},
    { FALSE, {_T("snews:"),       _T("snews:")}},
    { FALSE, {_T("telnet:"),      _T("telnet:")}},
    { FALSE, {_T("wais:"),        _T("wais:")}},
    { FALSE, {_T("file://"),      _T("file://")}},
    { FALSE, {_T("file:\\\\"),    _T("file:///\\\\")}},
    { FALSE, {_T("nntp://"),      _T("nntp://")}},
    { FALSE, {_T("newsrc:"),      _T("newsrc:")}},
    { FALSE, {_T("ldap://"),      _T("ldap://")}},
    { FALSE, {_T("ldaps://"),     _T("ldaps://")}},
    { FALSE, {_T("outlook:"),     _T("outlook:")}},
    { FALSE, {_T("mic://"),       _T("mic://")}},

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // N.B. The following have wildcard characters.
    // If you change \b to something else make sure you also change
    // the AUTOURL_WILDCARD_CHAR macro defined in _range.h!.
    //
    // Note that there should be the same number of wildcards in the left and right strings.
    // Also, all characters in in both strings must be identical after the FIRST wildcard.
    // For example: LEGAL:   {_T("\b@\b"),   _T("mailto:\b@\b")},     [since @\b == @\b]
    //              ILLEGAL: {_T("\b@hi\b"), _T("mailto:\b@there\b")} [since @hi != @there]

    { TRUE,  {_T("\\\\\b"),        _T("file://\\\\\b")}},
    { TRUE,  {_T("//\b"),          _T("file://\b")}},
    { TRUE,  {_T("\b@\b.\b"),      _T("mailto:\b@\b.\b")}},

};

LONG
CTxtRange::AutoUrl_FindWordBreak(int action)
{
#ifdef MERGEFUN // rtp
    LONG  cch = 0;
    LONG  cchTmp = 0;
    LONG  cchTotal = 0;

    Assert(action == WB_MOVEURLLEFT || action == WB_MOVEURLRIGHT);

    do
    {
        cchTmp = FindWordBreak(action);

        // have we gone too far or reached the beginning or end of the file?
                cchTotal += abs(cchTmp);
        if (cchTotal > MAX_URL_WORD_BREAK_CHAR_COUNT || !cchTmp)
            break;

        cch += cchTmp;
    }
    while (AutoUrl_IgnoreBoundary(_rpTX.GetPrevChar(), _rpTX.GetChar()));

    return cch;
#else
    return 0;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     AutoUrl_ApplyPattern
//
//  Synopsis:   This function creates a destination string corresponding to
//              the 'pattern' of a source string.  For example, given the
//              ptag {_T("\b@"),_T("mailto:\b@")}, and the source string
//              x@y.com, this function can generate mailto:x@y.com.  Similarly,
//              given mailto:x@y.com, it can generate x@y.com.
//
//
//  Arguments:  ppstrDest           where we should allocate memory to hold the
//                                  destination string.
//
//              iIndexDest          the ptag pattern index for the destination
//
//              pszSourceText       the source string (which must have been matched
//                                  via the AutoUrl_IsAutodetectable function).
//
//              iIndexSrc           the ptag pattern index for the source
//
//              ptag                the ptag (returned via the AutoUrl_IsAutodetectable
//                                  function).
//
//----------------------------------------------------------------------------
void
CTxtRange::AutoUrl_ApplyPattern(
    CStackDataAry<TCHAR, AUTOURL_MAXBUF>*   paryDestGA,
    int                                     iIndexDest,
    const TCHAR *                           pszSourceText,
    int                                     iIndexSrc,
    AUTOURL_TAG *                           ptag )
{
    TCHAR* pstrDest;

    if( ptag->fWildcard )
    {
        const TCHAR* pszPrefixEndSrc, * pszPrefixEndDest;
        int   iPrefixLengthSrc, iPrefixLengthDest;

        // Note: There MUST be a wildcard in both the source and destination
        //  patterns, or we will overflow into infinity.
        pszPrefixEndSrc = ptag->pszPattern[iIndexSrc];
        while( *pszPrefixEndSrc != AUTOURL_WILDCARD_CHAR )
            ++pszPrefixEndSrc;

        pszPrefixEndDest = ptag->pszPattern[iIndexDest];
        while( *pszPrefixEndDest != AUTOURL_WILDCARD_CHAR )
            ++pszPrefixEndDest;

        iPrefixLengthDest= pszPrefixEndDest - ptag->pszPattern[iIndexDest];
        iPrefixLengthSrc=  pszPrefixEndSrc  - ptag->pszPattern[iIndexSrc];

        if (paryDestGA->Grow(iPrefixLengthDest +
                                      _tcslen(pszSourceText +
                                      iPrefixLengthSrc) + 1) != S_OK)
        {
            return;
        }

        pstrDest = *paryDestGA;

        memcpy( pstrDest, ptag->pszPattern[iIndexDest], iPrefixLengthDest*sizeof(TCHAR) );
        _tcscpy( pstrDest + iPrefixLengthDest, pszSourceText + iPrefixLengthSrc );
    }
    else
    {
        int iTotalLength = _tcslen(ptag->pszPattern[iIndexDest]) +
                           _tcslen(pszSourceText + _tcslen(ptag->pszPattern[iIndexSrc])) +
                           1;

        if (paryDestGA->Grow( iTotalLength ) != S_OK )
            return;

        pstrDest = *paryDestGA;

        _tcscpy( pstrDest, ptag->pszPattern[iIndexDest] );
        _tcscat( pstrDest, pszSourceText + _tcslen(ptag->pszPattern[iIndexSrc]) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_MakeLink
//
//  Synopsis:   AutoUrl_MakeLink formats a specified range as an anchor
//              element.  It does not set the href.
//
//  Arguments:  ppAnchorElement        pointer to where the new element will
//                                     be returned.
//
//              cpStart, cpEnd         the start and end cp positions.
//
//----------------------------------------------------------------------------
HRESULT
CTxtRange::AutoUrl_MakeLink( CAnchorElement** ppAnchorElement, DWORD cpStart, DWORD cpEnd )
{
#ifdef MERGEFUN // rtp
    HRESULT        hr;
    long           lWidth;
    CElement*      pElement;

    hr = S_OK;

    Assert( cpStart >= (DWORD)GetFirstCp() && cpStart <= (DWORD)GetLastCp() );
    Assert( cpEnd   >= (DWORD)GetFirstCp() && cpEnd   <= (DWORD)GetLastCp() );

    // Select the range
    lWidth = cpEnd - cpStart;
    Set(cpStart, -lWidth);

    // Create the anchor element and format the range as part of that element
    hr = THR(CreateElement(ETAG_A, &pElement, GetPed()->Doc() ));
    if (hr != S_OK)
        goto Cleanup;

    pElement->_fExplicitEndTag = TRUE;

    hr = THR(FormatRange(pElement, NULL));
    if (hr != S_OK)
        goto Cleanup;

    *ppAnchorElement = DYNCAST(CAnchorElement, pElement);

    _cch = 0;

Cleanup:
    CElement::ClearPtr(&pElement);
    RRETURN(hr);
#else
    RRETURN(E_FAIL);
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_EstablishSearchRange
//
//  Synopsis:   EstablishSearchRange takes the range present when
//              UrlAutodetector was called and expands it to contain any
//              leading and trailing pieces of potential Url.  This is the
//              range through which UrlAutodetector should scan for tag
//              matches.
//
//  Arguments:  pcch                pointer UrlAutodetector's _cch
//              pcpSearchStart      pointer to UrlAutodetector's cpSearchStart
//              pcpSearchStop       pointer to UrlAutodetector's cpSearchStop
//              fUserEnteredChar    whether the user triggered UrlAutodetector
//                                  by entering a char (as opposed to jumping
//                                  the caret with something such as a mouse
//                                  click).
//
//----------------------------------------------------------------------------
void
CTxtRange::AutoUrl_EstablishSearchRange(
        long *  pcch,
        DWORD * pcpSearchStart,
        DWORD * pcpSearchStop,
        BOOL    fUserEnteredChar,
        BOOL    fBlockInsert)
{
#ifdef MERGEFUN // rtp
    DWORD   cpSearchStop;

    Assert( pcch );
    Assert( pcpSearchStart );
    Assert( pcpSearchStop );

    cpSearchStop = max(long(GetCp() - _cch), long(GetFirstCp()));
    if (fUserEnteredChar && cpSearchStop)
    {
        Advance(-1);
    }

    *pcch = 0;
    AutoUrl_FindWordBreak(WB_MOVEURLLEFT);

    *pcpSearchStart = GetCp();

    // jump to the right side of the original range if the user did not enter
    //  a character
    if (!fUserEnteredChar && !fBlockInsert)
    {
        SetCp(cpSearchStop);
        AutoUrl_FindWordBreak(WB_MOVEURLRIGHT);
        cpSearchStop = GetCp();
    }

    *pcpSearchStop = cpSearchStop;

    Assert( *pcpSearchStart >= (DWORD)GetFirstCp() && *pcpSearchStart <= (DWORD)GetLastCp() );
    Assert( *pcpSearchStop  >= (DWORD)GetFirstCp() && *pcpSearchStop  <= (DWORD)GetLastCp() );
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_UpdateAnchorFromHRefIfSynched
//
//  Synopsis:   If both the href of the supplied anchor and the anchor text
//              are autodetectable, we update the anchor text from the href
//              using the AutoUrl_ApplyPattern function.
//
//  Arguments:  pstrHref            a null terminated string containing the href
//
//              pAnchorElement      a pointer to the anchor element
//
//----------------------------------------------------------------------------

BOOL
CTxtRange::AutoUrl_UpdateAnchorFromHRefIfSynched( LPCTSTR pstrHref, CAnchorElement* pAnchorElement )
{
#ifdef MERGEFUN // rtp
    BOOL  fUpdated = FALSE;
    AUTOURL_TAG* ptag, *ptagHref;
    DWORD cpStart, cpEnd;
    TCHAR* pszAnchorText = NULL, *pszNewAnchorText = NULL;
    DWORD  iLength;
    CStackDataAry<TCHAR, AUTOURL_MAXBUF> aryNewAnchorGA(Mt(CTxtRangeAutoUrl_UpdateAnchorFromHRefIfSynched_aryNewAnchorGA_pv));

    if (!pstrHref)
    {
        goto Cleanup;
    }

        AutoUrl_GrabAnchorDimensions( pAnchorElement, &cpStart, &cpEnd );

    Assert( cpStart >= (DWORD)GetFirstCp() && cpStart <= (DWORD)GetLastCp() );
    Assert( cpEnd   >= (DWORD)GetFirstCp() && cpEnd   <= (DWORD)GetLastCp() );

    // Check the text in the buffer to make sure it is autodetectable
    iLength = cpEnd - cpStart;
    pszAnchorText = new(Mt(CTxtRangeAutoUrl_UpdateAnchorFromHRefIfSynched_pszAnchorText)) TCHAR[ iLength + 1 ];
    if( NULL == pszAnchorText )
        goto Cleanup;

    SetCp( cpStart );
    _rpTX.GetText( iLength, pszAnchorText );
    pszAnchorText[ iLength ] = 0;

    if( !AutoUrl_IsAutodetectable( pszAnchorText, AUTOURL_TEXT_PREFIX, &ptag ) )
        goto Cleanup;

    if( !AutoUrl_IsAutodetectable( pstrHref, AUTOURL_TEXT_PREFIX, &ptagHref ) )
        goto Cleanup;       // href must be autodetectable

    if( ptagHref != ptag )
    {
        // If the tags are different, our best bet is to copy the entire text
        //  from the anchor's href.
        pszNewAnchorText = (TCHAR *) pstrHref;
    }
    else
    {
        AutoUrl_ApplyPattern( &aryNewAnchorGA, AUTOURL_TEXT_PREFIX,
                                pstrHref, AUTOURL_HREF_PREFIX,
                                ptag );

        pszNewAnchorText = aryNewAnchorGA;

        // If tags are the same, copy the prefix from the original anchor text
        //  in order to preserve case.

        memcpy( pszNewAnchorText, pszAnchorText,
                    _tcslen(ptag->pszPattern[AUTOURL_TEXT_PREFIX])*sizeof(TCHAR) );
    }

    SetCp( cpStart );
    _cch = cpStart - cpEnd; // cchs are negative
    IGNORE_HR( ReplaceRange( _tcslen(pszNewAnchorText), pszNewAnchorText ) );
    _cch = 0;

    fUpdated = TRUE;        // we made it

Cleanup:

    delete [] pszAnchorText;

    return fUpdated;
#else
    return FALSE;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_CloseQuotes
//
//  Synopsis:   If we are currently under the scope of an anchor element, and
//              the anchor element is awaiting a closing quote, we set the
//              need closing quote flag to FALSE, and remove the first quote,
//              and return TRUE.  Otherwise, we return false.
//
//----------------------------------------------------------------------------

BOOL
CTxtRange::AutoUrl_CloseQuotes( )
{
#ifdef MERGEFUN // rtp
    CAnchorElement* pAnchorElement;
    CElement* pElement;

    pElement = SearchBranchForAnchor( )->SafeElement();
    if( NULL == pElement )
        return FALSE;           // no anchor element, nothing to do

    pAnchorElement = DYNCAST( CAnchorElement, pElement );

    if( pAnchorElement->GetNeedClosingQuote() )
    {
        DWORD cpStart, cpEnd;

        AutoUrl_GrabAnchorDimensions( pAnchorElement, &cpStart, &cpEnd );

        Assert( cpStart >= (DWORD)GetFirstCp() && cpStart <= (DWORD)GetLastCp() );
        Assert( cpEnd   >= (DWORD)GetFirstCp() && cpEnd   <= (DWORD)GetLastCp() );

        SetCp( cpStart - 1 );
        _cch = -1;
        Assert(GetChar()==_T('"'));
        IGNORE_HR( ReplaceRange( 0, NULL ) );
        _cch = 0;
        SetCp( cpEnd-1 );

        pAnchorElement->SetNeedClosingQuote( FALSE );
        return TRUE;
    }
#endif
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_DoAutoDetectOnInsert
//
//  Synopsis:   Returns TRUE if the autodetector should be called before
//              chNew is inserted (used during key handling)
//
//----------------------------------------------------------------------------

BOOL
CTxtRange::AutoUrl_DoAutoDetectOnInsert(TCHAR chPrev, TCHAR chNew)
{
    return chPrev != chNew && (chNew == _T(' ') || chNew == _T('"'));
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_IgnoreBoundary
//
//  Synopsis:   Returns TRUE if the boundary between two characters should
//              not be considered as a boundary for the purposes of URL
//              autodetection.
//
//----------------------------------------------------------------------------

inline BOOL
AutoUrl_IsAllowedDbcsNeighbor(TCHAR ch)
{
    return ch == _T('\\') ||
           ch == _T('/');
}

BOOL
CTxtRange::AutoUrl_IgnoreBoundary(TCHAR ch1, TCHAR ch2)
{
    BOOL IsFEChar(TCHAR); // in intl.cxx

    //
    // the word break isn't supposed to break on these boundaries
    // during URL expansion (unlike normal word expansion)
    //

    return IsFEChar(ch1) && AutoUrl_IsAllowedDbcsNeighbor(ch2) ||
           IsFEChar(ch2) && AutoUrl_IsAllowedDbcsNeighbor(ch1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_SetUrl
//
//  Synopsis:   This function either creates a new link, or updates the href
//              of an existing link given a string of anchor text.
//
//  Arguments:  pszAnchorText       the anchor text to make the href from
//              cpWordStart, cpWordEnd
//                                  the cps where the word was taken from
//              pAnchorElement      NULL to create a new anchor, or a pointer
//                                  to an anchor element to update
//              ptag                the search tag to used to build the href
//              fQuote              whether we should set the need closing
//                                  quote flag on a new anchor
//----------------------------------------------------------------------------

void
CTxtRange::AutoUrl_SetUrl(
    const TCHAR *    pszAnchorText,
    DWORD            cpWordStart,
    DWORD            cpWordEnd,
    CAnchorElement * pAnchorElement,
    AUTOURL_TAG *    ptag,
    BOOL             fQuote )
{
#ifdef MERGEFUN // rtp
    TCHAR* pszHref;
    DWORD dwFlags = ELEMCHNG_CLEARCACHES;
    CStackDataAry<TCHAR, AUTOURL_MAXBUF> aryHRefGA(Mt(CTxtRangeAutoUrl_SetUrl_aryHRefGA_pv));

    Assert( cpWordStart >= (DWORD)GetFirstCp() && cpWordStart <= (DWORD)GetLastCp() );
    Assert( cpWordEnd   >= (DWORD)GetFirstCp() && cpWordEnd   <= (DWORD)GetLastCp() );

    AutoUrl_ApplyPattern( &aryHRefGA, AUTOURL_HREF_PREFIX,
                          pszAnchorText, AUTOURL_TEXT_PREFIX,
                          ptag );

    pszHref = aryHRefGA;

    if( NULL == pAnchorElement )
    {
        AutoUrl_MakeLink( &pAnchorElement, cpWordStart, cpWordEnd );
        pAnchorElement->SetNeedClosingQuote( fQuote );
    }

    THR( pAnchorElement->SetAAhref( pszHref ) );
    THR( pAnchorElement->EnsureFormatCacheChange(&dwFlags));
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_GetWord
//
//  Synopsis:   This function a word from the text buffer.  Anchors are treated
//              as words, so that <A>this is anchor text</A> will return just
//              one word.
//
//  Arguments:  cpStart, cpEnd      the search range
//              pWordGA             a growable array to hold the word
//              pcpWordStart, pcpWordEnd
//                                  where we found the word
//              ppAnchorElement     if we found a word not part of an anchor,
//                                  this will be set to null.  otherwise, it
//                                  will be the anchor we used to get the word.
//              pfQuote             set to true if we found a leading quote
//              pcpNext             set to the cp position we should use to
//                                  search for the next word
//              pfMoreWords         false iff this is the last word available
//----------------------------------------------------------------------------

void
CTxtRange::AutoUrl_GetWord(
    DWORD                                   cpStart,
    DWORD                                   cpEnd,
    CStackDataAry<TCHAR, AUTOURL_MAXBUF> *  paryWordGA,
    DWORD *                                 pcpWordStart,
    DWORD *                                 pcpWordEnd,
    CAnchorElement **                       ppAnchorElement,
    BOOL *                                  pfQuote,
    DWORD *                                 pcpNext,
    BOOL *                                  pfMoreWords)
{
#ifdef MERGEFUN // rtp
    BOOL      fQuote = FALSE;
    CElement* pElement;
    TCHAR*    pszWord;
    DWORD     cpCompletelyDone;

    Assert( cpStart >= (DWORD)GetFirstCp() && cpStart <= (DWORD)GetLastCp() );
    Assert( cpEnd   >= (DWORD)GetFirstCp() && cpEnd   <= (DWORD)GetLastCp() );

    cpCompletelyDone = cpEnd;   // so we know when we're doing the last word

    SetCp( cpStart );

    // Skip leading quote characters if we have any
    while( cpStart < cpCompletelyDone && GetChar() == _T('"') )
    {
        fQuote = TRUE;
        Advance( 1 );
        cpStart ++;
    }

    // make sure we didn't go out of our range
    if (cpStart >= cpCompletelyDone)
    {
        *pfMoreWords = FALSE;
        *pcpWordStart = *pcpWordEnd = cpStart;  // give them something nondetectable
        return;
    }

    // If we are not within an anchor, get the word extent.
    AdvanceToNonEmpty();
    pElement = SearchBranchForAnchor( )->SafeElement();
    if (!pElement)
    {
        // find the right side
        AutoUrl_FindWordBreak(WB_MOVEURLRIGHT);
        cpEnd = min(cpEnd, GetCp());    // stay inside intended range

        // See if moving to the right has brought us under the influence of
        // an anchor.
        SetCp(cpEnd);
        Advance(-1);
        pElement = SearchBranchForAnchor()->SafeElement();
    }

    if (pElement)
    {
        // Use the anchor element's dimensions
        *ppAnchorElement = DYNCAST(CAnchorElement, pElement);
        AutoUrl_GrabAnchorDimensions( *ppAnchorElement, &cpStart, &cpEnd );
        *pfQuote = FALSE;   // anchors should never set closing quote flag
    }
    else
    {
        *ppAnchorElement = NULL;     // we do not have an anchor element
        *pfQuote = fQuote;
    }

    if (paryWordGA->Grow( cpEnd - cpStart + 2 ) != S_OK)
    {
        // If we cannot allocate memory to hold the word,
        // send back an empty buffer
        *pcpWordStart = *pcpWordEnd = cpEnd;
        (*paryWordGA)[0] = 0;
    }
    else
    {
        int iLength = cpEnd - cpStart;
        TCHAR   chLast;

        pszWord = *paryWordGA;

        SetCp( cpStart );
        _rpTX.GetText( cpEnd - cpStart, pszWord );
        pszWord[ cpEnd - cpStart ] = 0;

        *pcpWordStart = cpStart;
        *pcpWordEnd   = cpEnd;

        // Certain punctuation characters and sequences of dissimilar
        // characters should not be included if they are at the end
        // of the word.

        Assert(iLength >= 0);
        if (!fQuote && iLength > 1)
        {
            chLast = pszWord[iLength-1];
            if (chLast == _T('.') || chLast == _T('"')
                || chLast == _T('?') || chLast == _T(','))
            {
                while (iLength-- && chLast == pszWord[iLength])
                {
                    --(*pcpWordEnd);
                }

                pszWord[iLength+1] = _T('\0');
                Assert(*pcpWordEnd >= *pcpWordStart);
            }
        }
    }

    *pcpNext = cpEnd;
    *pfMoreWords = cpEnd < cpCompletelyDone;

    Assert( *pcpWordStart >= (DWORD)GetFirstCp() && *pcpWordStart <= (DWORD)GetLastCp() );
    Assert( *pcpWordEnd   >= (DWORD)GetFirstCp() && *pcpWordEnd   <= (DWORD)GetLastCp() );
    Assert( *pcpNext      >= (DWORD)GetFirstCp() && *pcpNext      <= (DWORD)GetLastCp() );
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_IsAutodetectable
//
//  Synopsis:   Returns TRUE if the supplied text is autodetectable.
//
//  Arguments:  pszString           the word in question.
//              iIndex              which pattern to use in the s_urlTags array
//              ppTag               if successful, will point to the matching
//                                  s_urlTag.
//----------------------------------------------------------------------------

BOOL
CTxtRange::AutoUrl_IsAutodetectable( const TCHAR* pszString, int iIndex,
                                         AUTOURL_TAG** ppTag )
{
#ifdef MERGEFUN // rtp
    const TCHAR *   pch;
    const TCHAR* pszPattern;
    register int i;
    BOOL     fMatch = FALSE;

    //
    // make sure there aren't any undesirable characters
    // trying to slip into our little hrefs (so young and
    // innocent, they are)
    //

    for (pch = pszString; *pch != _T('\0'); pch++)
    {
        if (*pch == WCH_EMBEDDING)
        {
            *ppTag = NULL;
            return FALSE;
        }
    }

    for (i = 0; i < ARRAY_SIZE(s_urlTags); i++)
    {
        pszPattern = s_urlTags[i].pszPattern[iIndex];

        if( !s_urlTags[i].fWildcard )
        {
            DWORD iLen = _tcslen( pszPattern );
#ifdef UNIX
	    //IEUNIX: We need a correct count for UNIX version.
	    DWORD iStrLen = _tcslen( pszString );
            if (!_tcsnicmp(pszPattern, iLen, pszString, iStrLen) && (iStrLen != iLen))
#else
            if (!StrCmpNIC(pszPattern, pszString, iLen) && pszString[iLen])
#endif
                fMatch = TRUE;
        }
        else
        {
            const TCHAR* pSource = pszString;
            const TCHAR* pMatch  = pszPattern;

            while( *pSource )
            {
                if( *pMatch == AUTOURL_WILDCARD_CHAR )
                {
                    // N.B. (johnv) Never detect a slash at the
                    //  start of a wildcard (so \\\ won't autodetect).
                    if (*pSource == _T('\\') || *pSource == _T('/'))
                        break;

                    if( pMatch[1] == 0 )
                        // simple optimization: wildcard at end we just need to
                        //  match one character
                        fMatch = TRUE;
                    else
                    {
                        while( *pSource && *(++pSource) != pMatch[1] )
                            ;
                        if( *pSource )
                            pMatch++;       // we skipped wildcard here
                        else
                            continue;   // no match
                    }
                }
                else if( *pSource != *pMatch )
                    break;

                if( *(++pMatch) == 0 )
                    fMatch = TRUE;

                pSource++;
            }
        }

        if( fMatch )
        {
            *ppTag = &s_urlTags[i];
            return TRUE;
        }
    }

#endif

    *ppTag = NULL;
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::AutoUrl_GrabAnchorDimensions
//
//  Synopsis:   Fills out the pcpStart and pcpEnd vars based on info returned
//              from pAnchorElement.
//
//  Arguments:  pAnchorElement      a pointer to an anchor element to use in
//                                  calculating the dimensions.
//
//              pcpStart            the address of a word to hold the start pos
//
//              pcpEnd              the address of a word to hold the end pos
//
//----------------------------------------------------------------------------

BOOL
CTxtRange::AutoUrl_GrabAnchorDimensions( CAnchorElement* pAnchorElement, DWORD* pcpStart,
                                            DWORD* pcpEnd )
{
#ifdef MERGEFUN // rtp
    long iRunStart, iRunFinish, cch;

    GetList().GetElementScope( pAnchorElement, &iRunStart, &iRunFinish,
                                      (long *)pcpStart, &cch );

    if (iRunStart < 0)
        return FALSE;

    *pcpEnd = *pcpStart + cch;

    Assert( *pcpStart >= (DWORD)GetFirstCp() && *pcpStart <= (DWORD)GetLastCp() );
    Assert( *pcpEnd   >= (DWORD)GetFirstCp() && *pcpEnd   <= (DWORD)GetLastCp() );

    return TRUE;
#endif

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTxtRange::UrlAutodetector
//
//  Synopsis:   UrlAutodetector scans a given range for Url-like stretches of
//              text and upon identifying such text formats it with an
//              appropriate anchor element.
//
//  Arguments:  fUserEnteredChar    whether the user triggered UrlAutodetector
//                                  with a space, return, or tab
//              fUserPressedEnter   whether the user triggered UrlAutodetector
//                                  with a return
//              pfChangedText       pointer to where UrlAutodetector specifies
//                                  whether any changes were made to the text
//              fAllowInBrowseMode  whether we allow autodetection to occurr
//                                  when we're in browse mode (default FALSE)
//
//----------------------------------------------------------------------------
HRESULT
CTxtRange::UrlAutodetector(
        BOOL    fUserEnteredChar,
        BOOL    fBlockInsert,
        BOOL *  pfChangedText,
        BOOL    fAllowInBrowseMode)
{
#ifdef MERGEFUN // rtp
    HRESULT         hr = S_OK;
    DWORD           cpSearchStart;
    DWORD           cpSearchStop;
    DWORD           cpScan;
    CTxtRange       oldRange(*this);
    BOOL            fChangedText = FALSE;
    DWORD           iExtendEntry = _fExtend;
    CPedUndo        pu( GetPed() );
    CElement *      pContainer = GetCommonContainer();
    BOOL            fConainterEditable = pContainer->IsEditable();

    WHEN_DBG( if (IsTagEnabled(tagDisableAutodetector)) goto Cleanup );

    //
    // we don't want to autodetect when we're:
    // 1) not allowing detection in browse mode and are in browse mode
    // 2) not a real text object (no input buttons, etc)
    //

    //
    // BUGBUG - marka I wanted to have another tagdesc here for AcceptURL 
    // ( and get away from using ACCEPT_HTML here ) - but it required some ugly
    // casts (as we're out of TAGDESC's). Instead I check for buttons 
    // Fix in NEW_EDIT !!
    //
    
    if (!fAllowInBrowseMode && !fConainterEditable ||
        !GetCurrFlowLayout()->Element()->HasFlag(TAGDESC_ACCEPTHTML )
        || GetCurrFlowLayout()->Element()->_etag == ETAG_BUTTON )
    {
        goto Cleanup;
    }

    pu.Start( IDS_UNDOGENERICTEXT, this, fConainterEditable );

    _fDisableWordLikeTxtRange = TRUE;

    SetExtend(FALSE);

    AutoUrl_EstablishSearchRange(
            &_cch,
            &cpSearchStart,
            &cpSearchStop,
            fUserEnteredChar,
            fBlockInsert);

    Assert( cpSearchStart >= (DWORD)GetFirstCp() && cpSearchStart <= (DWORD)GetLastCp() );
    Assert( cpSearchStop  >= (DWORD)GetFirstCp() && cpSearchStop  <= (DWORD)GetLastCp() );

    // Read a word at a time, and see if the word is autodetectable.  If so,
    //  either create a new anchor, or update the href of an existing anchor.
    {
        TCHAR*          pszWord;
        AUTOURL_TAG*    ptag;
        BOOL            fQuote;
        DWORD           cpWordStart, cpWordEnd;
        CAnchorElement* pAnchorElement;
        CStackDataAry<TCHAR, AUTOURL_MAXBUF> aryWordGA(Mt(CTxtRangeUrlAutodetector_aryWordGA_pv));
        BOOL            fMoreWords;

        cpScan = cpSearchStart;

	// PORT QSY	SUNPRO	zero the string
#ifdef UNIX
	aryWordGA[0] = 0 ;
#endif
        do
        {
            AutoUrl_GetWord(cpScan, cpSearchStop, &aryWordGA, &cpWordStart,
                            &cpWordEnd, &pAnchorElement, &fQuote, &cpScan, &fMoreWords);

            Assert(cpWordEnd > cpWordStart || cpWordEnd == cpWordStart && !fMoreWords);

            pszWord = aryWordGA;

            if( AutoUrl_IsAutodetectable( pszWord, AUTOURL_TEXT_PREFIX, &ptag ) )
            {
                // quotes are not special cased when pasting
                // and if the user clicked or arrowed away quoting
                // benefits go away
                if( fBlockInsert || !fUserEnteredChar)
                    fQuote = FALSE;

                AutoUrl_SetUrl( pszWord, cpWordStart, cpWordEnd, pAnchorElement,
                                    ptag, fQuote );

                fChangedText = TRUE;
            }
        } while (fMoreWords);
    }

    if( !fChangedText )
    {
        // If we didn't do anything at all, it is safe to go back to the original
        //  state, including the irun.
        *this = oldRange;
    }
    else
    {
        // We should not play with the irun, since we may have added a run.
        // The caller is responsible for adjusting to the proper run.
        SetCp(oldRange.GetCp());
        _cch = oldRange._cch;
    }

    // BUGBUG (johnv) To work around bug in CTxtRange copy operator
    SetExtend(iExtendEntry);

    pu.Finish( hr );

Cleanup:

    if( pfChangedText )
        *pfChangedText = fChangedText;

    _fDisableWordLikeTxtRange = oldRange._fDisableWordLikeTxtRange;

    RRETURN(hr);
#else
    RRETURN(E_FAIL);
#endif
}

//+----------------------------------------------------------------------
//
//  Member:     CWordLikeTxtRange::CWordLikeTxtRange
//
//  Synopsis:   A temporary text range is used for formatting operations
//              which require that for the duration of the operation the
//              range should encompass the word in which the IP is
//              situated.  This is to mimic Microsoft Word's behavior.
//
//              The constructor may modify the CTxtRange variable.
//              Similarly, the destructor may also
//
//-----------------------------------------------------------------------

CWordLikeTxtRange::CWordLikeTxtRange(
    CTxtRange * pRange )
{
#ifdef MERGEFUN // GetCp()
    // Cache a pointer for the destructor's use.
    _pRange = pRange;

    if(_pRange->_fDisableWordLikeTxtRange)
        return;

    _cch = _pRange->_cch;

    if (_cch)
    {
        CTextSelectionRecord * pTSR;

        IGNORE_HR(_pRange->GetPed()->GetSel(&pTSR, FALSE));
        // Here's whats bogus about Word's selection model:  If the selection
        // was auto extended in some fashion, the trailing spaces are not
        // subject to (un)formatting.  If the user deliberately selected
        // trailing spaces (which is possible only by shift-clicking), it
        // will be subject to (un)formatting.

        if (pTSR && pTSR->WasAutoSel())
        {
            _cp = _pRange->GetCp();

            if (_cch < 0)
                _pRange->FlipRange();

            _pRange->SetExtend(TRUE);
            _pRange->FindWordBreak(WB_MOVEWORDLEFT);
            _pRange->FindWordBreak(WB_RIGHTBREAK);
            _pRange->SetExtend(FALSE);
        }
        else
        {
            _cp = -1;
        }
    }
    else
    {
        // If we have a singular IP, we do one of two things:
        // (a) If we are in mid-word, we expand the range to encompass
        //     the word, or
        // (b) If we are not mid-word, and we are inserting, we split the
        //     run and make our TxtRange point to that.

        if (_pRange->IsAtMidWord())
        {
            _cp = _pRange->GetCp();

            _pRange->SetExtend(FALSE);
            _pRange->FindWordBreak(WB_MOVEWORDLEFT); // Go to start of word
            _pRange->SetExtend(TRUE);
            _pRange->FindWordBreak(WB_RIGHTBREAK); // Extend to end of word
            _pRange->SetExtend(FALSE);
        }
        else
        {
            _cp = -1;                   // do not adjust later.

            // If we aren't already point at a null run, create one at
            // the current point and make the CTxtRange point to it.

            if (_pRange->GetCchRun())
            {
                _pRange->SplitRun( );
                _pRange->AdjustForward();
            }
        }
    }
#endif
}

//+----------------------------------------------------------------------
//
//  Member:     CWordLikeTxtRange::~CWordLikeTxtRange
//
//  Synopsis:   Restores the cached CTxtRange to it's original (or a
//              close facsimile thereof because runs may have been split)
//              state in the event that it was modified by the constructor.
//
//-----------------------------------------------------------------------

CWordLikeTxtRange::~CWordLikeTxtRange()
{
#ifdef MERGEFUN // SetCp()
    if(_pRange->_fDisableWordLikeTxtRange)
        return;

    // If we've mangled _pRange in our constructor, restore it.

    if (-1 != _cp)
    {
        _pRange->SetCp( _cp );
        _pRange->_cch = _cch;

        _pRange->AdjustForInsert();
    }
#endif
}


//+------------------------------------------------------------------------
//
//  Member:     ExecFormatAnchor
//
//  Synopsis:   Creates/Edits the anchor of the current selection.
//
//  Comment:    This text handles three different scenarios:
//
//              (a) A single anchor is selected
//              (b) No anchor currently in the selection
//              (c) A single site is selected
//
//              In the case (a), the user is allowed to edit the anchor in
//              in the selection.  In the other two cases, a new anchor is
//              created, and the placed in the proper position.
//
//
//              Parameter fLink is True when the anchor is treated as Link
//                  when it is False a bookmark is assumed.
//-------------------------------------------------------------------------

HRESULT
CTxtRange::ExecFormatAnchor (
    CLayout * pLayoutSelected, BOOL fShowUI, VARIANT * pvarArg, BOOL fLink )
{
#ifdef MERGEFUN // rtp
    CElement          * pElementAnchor = NULL;
    CAnchorElement    * pAnchor;
    CPtrAry<CElement *> arypElements(Mt(CTxtRangeExecFormatAnchor_arypElements_pv));
    long                iRun, cp;
    CTreeNode *         pNodeTxtSite = NULL;
    HRESULT             hr = S_FALSE;
    BOOL                fInsert = FALSE;
    CNotification       nf;
    CPedUndo            pu ( GetPed() );
    CElement *          pContainer = GetCommonContainer();

    pu.Start( IDS_UNDOGENERICTEXT, this, pContainer->IsEditable() );

    // fShowUI case will be handled later in CRootSite::Exec
    if(fShowUI)
    {
        hr = MSOCMDERR_E_NOTSUPPORTED;
        goto Cleanup;
    }

    if (IsVariantOmitted(pvarArg) || V_VT(pvarArg) != VT_BSTR)
    {
        hr = MSOCMDERR_E_NOTSUPPORTED;
        goto Cleanup;
    }

    // NB: (cthrash) GetAnchorElements will return the inner-most
    // anchor if anchors are completely nested.  This will be tolerable
    // once we have the mechanism to remove anchors.  Until then, we
    // simple cannot edit outer anchors if they have identical scope
    // as the inner one.

    hr = THR(GetAnchorElements(pLayoutSelected, & arypElements));

    if (FAILED( hr ))
        goto Cleanup;

    if (1 < arypElements.Size())
    {
        // multiple anchors selected, can't really edit them.
        // BUGBUG (cthrash) Provide feedback to user?
        hr = S_OK;
        goto Cleanup;
    }

    if (0 == arypElements.Size())
    {
        pNodeTxtSite = GetCurrFlowLayoutNodeInContext();

        hr = THR( CreateElement( ETAG_A, &pElementAnchor, GetPed()->Doc() ) );
        if (hr)
            goto Cleanup;

        pElementAnchor->_fExplicitEndTag = TRUE;
        arypElements.Append(pElementAnchor);

        fInsert = TRUE;
    }

    pAnchor = static_cast<CAnchorElement *>((CElement*)arypElements[0]);

    // Set the name or href property of the anchor element to the value of
    //  the function parameter
    if(fLink)
    {
        hr = pAnchor->SetAAhref(V_BSTR(pvarArg));
        if (hr)
            goto Cleanup;
        hr = THR(pAnchor->OnPropertyChange(DISPID_CAnchorElement_href, 0));
    }
    else
    {
        hr = pAnchor->SetAAname(V_BSTR(pvarArg));
    }

    if(hr)
        goto Cleanup;

    if(fInsert)
    {
        //
        // If a site was selected, insert the anchor above it.
        // Otherwise, place the newly created anchor above the
        // text selection.
        //

        if(pLayoutSelected)
        {
            Assert( pNodeTxtSite );

            // BUGBUG (cthrash) this is not optimal code.
            CRchTxtPtr rtp( pNodeTxtSite->HasFlowLayout() ? pNodeTxtSite->Element() : NULL );

            cp = rtp.CpFromSite(pLayoutSelected);

            iRun = rtp.GetIRun();

            hr = THR(rtp.GetList().InsertElement(
                                    pLayoutSelected->Element(),
                                    iRun,
                                    iRun,
                                    pElementAnchor));

            if ( hr )
                goto Cleanup;

            nf.CharsResize(
                              cp, rtp.GetCchRun(), iRun, 1,
                              pNodeTxtSite );
            _snLast = nf.SerialNumber();
            rtp.GetList().Notify( nf );
        }
        else
        {
            hr = THR( FormatRange(pElementAnchor, NULL ) );
        }
    }

Cleanup:
    CElement::ClearPtr( & pElementAnchor );

    pu.Finish( hr );

    // Even though S_FALSE is a legitimate return value, our callers
    // want none of it.  We'll just lie and say everything is S_OK.
    if(S_FALSE == hr)
    {
        hr = S_OK;
    }

    if(!hr)
        hr = THR( AdjustForInsert() );

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     ExecRemoveAnchor
//
//  Synopsis:   Removes the anchor in the current selection.
//
//  Arguments:  none.
//
//-------------------------------------------------------------------------

HRESULT
CTxtRange::ExecRemoveAnchor(CLayout * pLayoutSelected)
{
#ifdef MERGEFUN // iRuns
    CPtrAry<CElement *> arypElements(Mt(CTxtRangeExecRemoveAnchor_arypElements_pv));
    CElement * *        ppElement;
    long                iRun, iRunLeft, iRunRight, iRunScopeLeft, iRunScopeRight;
    int                 nIndex;
    HRESULT             hr;
    CPedUndo            pu ( GetPed() );
    CElement *          pContainer = GetCommonContainer();

    //
    // BUGBUG (cthrash) Once again, I should mention that GetAnchorElements
    // will only get the lowest anchor element in a given run.  This means
    // in fully nested anchors (e.g. <a><a>foo</a></a>) only the innermost
    // anchor will be subject to any operations, which in this case is
    // removal.
    //
    // This could be considered a feature, since currently there is no
    // other way to edit the outer anchor.  On the other hand, the user
    // will have to 'unlink' twice in order to fully remove anchor-ness.
    // This might be considered confusing UI.
    //

    hr = THR(GetAnchorElements(pLayoutSelected, & arypElements));

    if (FAILED( hr ))
        goto Cleanup;

    // If hr == S_FALSE, it means we didn't have any anchors.
    Assert( !hr );

    nIndex = arypElements.Size();

    // If, for some reason, we have no anchors leave!
    if (!nIndex)
        goto Cleanup;

    pu.Start( IDS_UNDOGENERICTEXT, this, pContainer->IsEditable() );

    // BUGBUG (cthrash) If we have sites selected, locating the runs in which
    // these sites reside is a slow and inefficient process since we don't
    // have a clue as to where they are.

    if (pLayoutSelected)
    {
        GetRunRange( & iRunLeft, & iRunRight );
    }
    else
    {
        GetEndRuns( & iRunLeft, & iRunRight );
    }

    iRun = iRunLeft;

    for ( ppElement = arypElements; nIndex--; ppElement++)
    {
        // TotallyRemoveElement requires the caller to pass in a run
        // that contains the element.

        while (iRun <= iRunRight)
        {
            if (SearchBranchForScope( *ppElement, iRun ))
                break;
            iRun++;
        }

        Assert( iRun <= iRunRight );

        THR(
            GetList().GetElementScope(
                iRun, * ppElement, & iRunScopeLeft, & iRunScopeRight ) );

        // Remove wholly.

        hr = THR(
            GetList().TotallyRemoveElement(
                iRun, * ppElement, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    pu.Finish( hr );

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}

void
CTxtRange::QueryStatusAnchor(CLayout * pLayoutSelected, MSOCMD * pCMD, DWORD nCmdID )
{
    Assert( IDM_HYPERLINK == nCmdID ||
            IDM_BOOKMARK == nCmdID  ||
            IDM_UNBOOKMARK == nCmdID ||
            IDM_UNLINK == nCmdID );

    CPtrAry<CElement *> arypAnchor(Mt(CTxtRangeQueryStatusAnchor_arypAnchor_pv));
    int                 cAnchors;
    HRESULT             hr;
    int                 cSites = pLayoutSelected ? 1 : 0;

    hr = THR(GetAnchorElements(pLayoutSelected, & arypAnchor));

    if (hr)
    {
        pCMD->cmdf = MSOCMDSTATE_DISABLED;
        return;
    }

    cAnchors = arypAnchor.Size();

    switch ( nCmdID )
    {
    case IDM_HYPERLINK:
        // We can always create and edit a hyperlink. (The dialog knows what
        // to do if there's no text selected.)
        pCMD->cmdf = MSOCMDSTATE_UP;
        break;

    case IDM_BOOKMARK:
        // We can create a bookmark if there are zero or one anchors
        // currently selected.
        pCMD->cmdf = cAnchors <= 1 ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
        break;

    case IDM_UNBOOKMARK:
    case IDM_UNLINK:
        // We must have exactly one anchor selected for unlinking to be
        // legal.  Why?  That's not for me to decide.
        pCMD->cmdf = cAnchors ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
        break;
    }
}

#ifdef WIN16
#pragma code_seg ("RANGE2_2_TEXT")
#endif

//+----------------------------------------------------------------------------
//
//  Member:     Exec
//
//-----------------------------------------------------------------------------

EXTERN_C const GUID CLSID_Img;
EXTERN_C const GUID CLSID_HRElement;
EXTERN_C const GUID CLSID_DivPosition;
EXTERN_C const GUID CLSID_DivFixed;

extern BOOL IsIDMSuperscript(CTreeNode *);
extern BOOL IsIDMSubscript(CTreeNode *);
extern BOOL IsIDMBold(CTreeNode *);
extern BOOL IsIDMItalic(CTreeNode *);
extern BOOL IsIDMUnderlined(CTreeNode *);
extern BOOL IsIDMCharAttr(CTreeNode *);


struct BlockFmtRec {
    ELEMENT_TAG   Etag;
    UINT          NameIDS;
};
static BlockFmtRec BlockFmts[] = {
    { ETAG_NULL,    IDS_BLOCKFMT_NORMAL }, // depends on the current default tag
    { ETAG_PRE,     IDS_BLOCKFMT_PRE },
    { ETAG_ADDRESS, IDS_BLOCKFMT_ADDRESS },
    { ETAG_H1,      IDS_BLOCKFMT_H1 },
    { ETAG_H2,      IDS_BLOCKFMT_H2 },
    { ETAG_H3,      IDS_BLOCKFMT_H3 },
    { ETAG_H4,      IDS_BLOCKFMT_H4 },
    { ETAG_H5,      IDS_BLOCKFMT_H5 },
    { ETAG_H6,      IDS_BLOCKFMT_H6 },
    { ETAG_OL,      IDS_BLOCKFMT_OL },
    { ETAG_UL,      IDS_BLOCKFMT_UL },
    { ETAG_DIR,     IDS_BLOCKFMT_DIR },
    { ETAG_MENU,    IDS_BLOCKFMT_MENU },
    { ETAG_DT,      IDS_BLOCKFMT_DT },
    { ETAG_DD,      IDS_BLOCKFMT_DD },
    { ETAG_P,       IDS_BLOCKFMT_P }
};

HRESULT
CTxtRange::Exec (
    GUID *       pguidCmdGroup,
    DWORD        nCmdID,
    DWORD        nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut,
    BOOL fStopBobble)
{
    HRESULT     hr = MSOCMDERR_E_NOTSUPPORTED;
    ULONG       cmdID;
    LONG        cpMin, cpMost;

    // artakka - showhelp is not implemented (v2?)
    if(nCmdexecopt == MSOCMDEXECOPT_SHOWHELP)
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    GetRange(cpMin, cpMost);

    Assert( CBase::IsCmdGroupSupported( pguidCmdGroup ) );

    cmdID = CBase::IDMFromCmdID( pguidCmdGroup, nCmdID );

    // Perform indirection if it is appropriate:
    if(pvarargIn && (V_VT(pvarargIn) == (VT_BYREF | VT_VARIANT)))
        pvarargIn = V_VARIANTREF(pvarargIn);

    // Some commands only make sense when selection is owned or in one text site
    if (CheckOwnerSiteOrSelection(cmdID))
    {
        goto Cleanup;
    }

    switch ( cmdID )
    {
    case IDM_AUTODETECT:
        hr = THR(UrlAutodetector(FALSE, FALSE, NULL, TRUE));
        break;

    case IDM_SELECTALL:
    case IDM_CLEARSELECTION:
        // Delegate this command to document
        hr = GetMarkup()->Doc()->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut, TRUE);
        break;

    case IDM_OVERWRITE:
#ifdef MERGEFUN
        hr = THR(GetMarkup()->ExecOverwriteCommand(pvarargIn, pvarargOut));
#endif
        break;

    case IDM_DELETEWORD:
        hr = THR( Delete( TRUE ) );
        LaunderSpacesForRange(cpMin, cpMost);
        break;

    case IDM_UNORDERLIST:
    case IDM_ORDERLIST:
    {
        MSOCMD msocmd = { cmdID, 0 };

        hr = THR(QueryStatus(const_cast < GUID * > ( & CGID_MSHTML ), 1, & msocmd, NULL, FALSE ) );
        if (hr)
            goto Cleanup;

        if (msocmd.cmdf == MSOCMDSTATE_UP)
        {
            CElement *      pContainer = GetCommonContainer();

            CParentUndo     pu( GetMarkup()->Doc() );
            if( pContainer->IsEditable() )
                pu.Start( IDS_UNDO );

            hr = THR(ApplyBlockFormat(nCmdID == IDM_UNORDERLIST ? ETAG_UL : ETAG_OL ) );

            pu.Finish( hr );
        }
        else
        {
            hr = OutdentSelection( TRUE );
        }

        if (hr)
            goto Cleanup;

        LaunderSpacesForRange(cpMin, cpMost);
        break;
    }

    case IDM_LINEBREAKNORMAL:
    case IDM_LINEBREAKLEFT:
    case IDM_LINEBREAKRIGHT:
    case IDM_LINEBREAKBOTH:
    {
        CParentUndo pu( GetMarkup()->Doc() );
        CElement *  pContainer = GetCommonContainer();

        if( pContainer->IsEditable() )
            pu.Start( IDS_UNDOTYPING );

        // if there is a selection, then delete it
        AssertSz( 0, "_cch no longer exists" );
//        if (_cch)
        {
            hr = THR( Delete( FALSE ) );
            if (hr != S_OK)
                goto Cleanup;
        }


        hr = SplitLine();

        if (hr == S_OK)
        {
            Update( TRUE, FALSE );
        }

        pu.Finish( hr );

        LaunderSpacesForRange(cpMin, cpMost);
        break;
    }

    case IDM_BLOCKDIRLTR:
    case IDM_BLOCKDIRRTL:
        {
            hr = THR(SetSelectionBlockDirection((cmdID == IDM_BLOCKDIRLTR) ?
                              htmlDirLeftToRight :
                              htmlDirRightToLeft));
        }
        break;

    case IDM_INLINEDIRLTR:
    case IDM_INLINEDIRRTL:
        {
            hr = THR(SetSelectionInlineDirection((cmdID == IDM_INLINEDIRLTR) ?
                              htmlDirLeftToRight :
                              htmlDirRightToLeft));
        }
        break;

    case IDM_TABLE:
    {
#if DBG==1
        GetMarkup()->Doc()->DoEnableModeless( FALSE );
        MessageBox(
                GetFocus(),
                TEXT("First Toolbar in HtmlForm"),
                TEXT("HtmlForm"),
                MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
        GetMarkup()->Doc()->DoEnableModeless( TRUE );
#endif
        hr = S_OK;

        break;
    }

    case IDM_BOOKMARK:
    case IDM_HYPERLINK:
        hr = THR( ExecFormatAnchor( NULL,
                    (nCmdexecopt != MSOCMDEXECOPT_DONTPROMPTUSER),
                    pvarargIn, (cmdID == IDM_HYPERLINK)));
        break;

    case IDM_UNBOOKMARK:
    case IDM_UNLINK:
        hr = THR( ExecRemoveAnchor( NULL ) );
        break;

    }

Cleanup:

    RRETURN_NOTRACE( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     QueryStatus
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::QueryStatus (
    GUID *       pguidCmdGroup,
    ULONG        cCmds,
    MSOCMD       rgCmds [ ],
    MSOCMDTEXT * pcmdtext,
    BOOL         fStopBobble )
{
    MSOCMD * pCmd = & rgCmds[ 0 ];
    ULONG    cmdID;
    HRESULT hr = S_OK;

    Assert( CBase::IsCmdGroupSupported( pguidCmdGroup ) );
    Assert( cCmds == 1 );

    cmdID = CBase::IDMFromCmdID( pguidCmdGroup, pCmd->cmdID );

    // Some commands only make sense when selection is owned or in one text site
    hr = CheckOwnerSiteOrSelection(cmdID);
    if(hr == S_FALSE)
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        hr = S_OK;
        goto Cleanup;
    }

    switch ( cmdID )
    {
    case IDM_IMAGE:
    case IDM_PARAGRAPH:
    case IDM_IFRAME:
    case IDM_TEXTBOX:
    case IDM_TEXTAREA:
    case IDM_HTMLAREA:
    case IDM_CHECKBOX:
    case IDM_RADIOBUTTON:
    case IDM_DROPDOWNBOX:
    case IDM_LISTBOX:
    case IDM_BUTTON:
    case IDM_MARQUEE:
    case IDM_1D:
    case IDM_BLOCKFMT:
#ifdef NEVER_NEW_EDIT
    case IDM_INDENT:
#endif   
    case IDM_OUTDENT:
    case IDM_LINEBREAKNORMAL:
    case IDM_LINEBREAKLEFT:
    case IDM_LINEBREAKRIGHT:
    case IDM_LINEBREAKBOTH:
    case IDM_HORIZONTALLINE:
    case IDM_INSINPUTBUTTON:
    case IDM_INSINPUTIMAGE:
    case IDM_INSINPUTRESET:
    case IDM_INSINPUTSUBMIT:
    case IDM_INSINPUTUPLOAD:
    case IDM_INSFIELDSET:
    case IDM_INSINPUTHIDDEN:
    case IDM_INSINPUTPASSWORD:
    case IDM_NONBREAK:
        pCmd->cmdf = MSOCMDSTATE_UP;
        break;

    case IDM_FIND:
    case IDM_REPLACE:
    case IDM_FONT:
    case IDM_GOTO:
       pCmd->cmdf = MSOCMDSTATE_UP;
       break;

    case IDM_SIZETOCONTROL:
    case IDM_SIZETOCONTROLHEIGHT:
    case IDM_SIZETOCONTROLWIDTH:
    case IDM_DYNSRCPLAY:
    case IDM_DYNSRCSTOP:

    case IDM_BROWSEMODE:
    case IDM_EDITMODE:
    case IDM_REFRESH:
    case IDM_REDO:
    case IDM_UNDO:
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        break;

    case IDM_COPY:
    case IDM_DELETEWORD:
    case IDM_CUT:
        {
            if (    (cmdID == IDM_CUT && !GetCommonNode()->Element()->Fire_onbeforecut())
                ||  (cmdID == IDM_COPY && !GetCommonNode()->Element()->Fire_onbeforecopy()))
                pCmd->cmdf = MSOCMDSTATE_UP;

            // when sites are selected, selectr should handle it.
        }
        break;

    case IDM_JUSTIFYLEFT:
    case IDM_JUSTIFYRIGHT:
    case IDM_JUSTIFYCENTER:
    case IDM_JUSTIFYFULL:
        QueryStatusJustify( pCmd, cmdID, pcmdtext );
        break;

    case IDM_BLOCKDIRLTR:
    case IDM_BLOCKDIRRTL:
        QueryStatusBlockDirection(pCmd, cmdID, pcmdtext);
        break;

    case IDM_INLINEDIRLTR:
    case IDM_INLINEDIRRTL:
        {
            // this is only valid if some text is selected

        }
        break;

    case IDM_UNORDERLIST:
    case IDM_ORDERLIST:
    {
        CElement      * pElementList;
        ELEMENT_TAG     etagList;
        ELEMENT_TAG     etagCB;

        pElementList = NULL;

        etagList = (cmdID == IDM_UNORDERLIST) ? ETAG_UL : ETAG_OL;

        etagCB = GetBlockFormat( TRUE );

        if (etagCB == ETAG_MENU || etagCB == ETAG_DIR)
            etagCB = ETAG_UL;

        if (etagCB == etagList)
            pCmd->cmdf = MSOCMDSTATE_DOWN;
        else
            pCmd->cmdf = MSOCMDSTATE_UP;

        break;
    }

    case IDM_BOOKMARK:
    case IDM_UNBOOKMARK:
    case IDM_HYPERLINK:
    case IDM_UNLINK:
        QueryStatusAnchor(
            NULL, & rgCmds[0],
            CBase::IDMFromCmdID( pguidCmdGroup, pCmd->cmdID ) );

        break;

    case IDM_SELECTALL:
    case IDM_CLEARSELECTION:
        // Delegate this command to document
        {
            CMarkup * pMarkup = GetMarkup();

            if(pMarkup != NULL && pMarkup->Doc() != NULL)
            {
                hr = pMarkup->Doc()->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
            }
            break;
        }

    case IDM_OVERWRITE:
        pCmd->cmdf = GetMarkup()->GetOverwriteStateFlags();
        break;

    default:
        hr = MSOCMDERR_E_NOTSUPPORTED;
        break;
    }

Cleanup:
    RRETURN( hr );
}

void
CTxtRange::QueryStatusJustify (
    MSOCMD *pCmd, DWORD nCmdID, MSOCMDTEXT * pcmdtext )
{
    STATUS status;

    Assert( nCmdID == IDM_JUSTIFYLEFT || nCmdID == IDM_JUSTIFYRIGHT || nCmdID == IDM_JUSTIFYGENERAL ||
            nCmdID == IDM_JUSTIFYCENTER || nCmdID == IDM_JUSTIFYFULL);

    // Get the alignment button status

    status = GetCurrentAlignment( nCmdID );

    if (status == STATUS_ON)
        pCmd->cmdf = MSOCMDSTATE_DOWN;
    else
        pCmd->cmdf = MSOCMDSTATE_UP;
}

void
CTxtRange::QueryStatusBlockDirection(
    MSOCMD *pCmd, DWORD nCmdID, MSOCMDTEXT * pcmdtext )
{
    STATUS status;

    Assert( nCmdID == IDM_BLOCKDIRLTR || nCmdID == IDM_BLOCKDIRRTL );

    status = GetCurrentBlockDirection( nCmdID );

    if (status == STATUS_ON)
        pCmd->cmdf = MSOCMDSTATE_DOWN;
    else
        pCmd->cmdf = MSOCMDSTATE_UP;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetCurrentAlignment
//
//  Synopsis:   Determines the alignment type of selection using the info. in
//              run
//
//  Arguments:  nCmdID - one of the following IDM_JUSTIFYLEFT,IDM_JUSTIFYRIGHT,
//                  or IDM_JUSTIFYCENTER:
//
//  Returns:    STATUS_ON - if the button must be on, STATUS_OFF if OFF,
//              STATUS_PARTIAL if the format changes in selection
//
//-----------------------------------------------------------------------------

STATUS
CTxtRange::GetCurrentAlignment( int nCmdID )
{
#ifdef MERGEFUN // rtp
    htmlBlockAlign  bAlign;
    BOOL fRTL;
    CRchTxtPtr      rtp(*this);

    rtp.SetCp(GetCpMin());
    const CParaFormat *pPF = rtp.GetPF();
    bAlign = (htmlBlockAlign)pPF->GetBlockAlign(TRUE);
    fRTL = pPF->HasRTL(TRUE);

    // if the block is not set, check for reading order 
    if (bAlign == htmlBlockAlignNotSet  &&
        (fRTL ? nCmdID == IDM_JUSTIFYRIGHT : nCmdID == IDM_JUSTIFYLEFT) ||
        bAlign == htmlBlockAlignLeft    && nCmdID == IDM_JUSTIFYLEFT || 
        bAlign == htmlBlockAlignRight   && nCmdID == IDM_JUSTIFYRIGHT ||
        bAlign == htmlBlockAlignCenter  && nCmdID == IDM_JUSTIFYCENTER ||
        bAlign == htmlBlockAlignJustify && nCmdID == IDM_JUSTIFYFULL)
    {
        return STATUS_ON;
    }

    return STATUS_OFF;

    // BUGBUG the code below works, but is left out because all the other
    //        related behaviors (bold/italic/underline for example) don't
    //        work this way, and instead always report the format of a selection
    //        as whatever the format for leading edge is.  If we ever get around
    //        to fixing the other stuff, this code should be allowed to play again.

#if 0
    long    iRunStart, iRunFinish, iRun;
    htmlBlockAlign    bAlignment = htmlBlockAlignNotSet;
    BOOL    fFirstRun = TRUE;
    BOOL    fPartial = FALSE;
    CRchTxtPtr  rtp(*this);

    // Get the first and last runs in the selected range ignoring leading
    //  and trailing spaces
    // BUGBUG we probably don't want to ignore spaces because it allows a cross
    //        format selection to be reported as single format (which directly
    //        contradict information conveyed by page layout)
    IGNORE_HR( GetEndRunsIgnoringSpaces( & iRunStart, & iRunFinish ) );
    Assert( GetList().GetTxtSite( iRunStart ) ==
            GetList().GetTxtSite( iRunFinish ) );

    Assert( iRunStart >= GetFirstRun() && iRunFinish <= GetLastRun() );

    for ( iRun = iRunStart; iRun <= iRunFinish ; iRun++ )
    {
        // BUGBUG: If the selection extends into a nested CTxtSite, then simply requesting
        //         the PF from the element which owns the run is incorrect. It will need
        //         to use GetRunOwnerPF. (brendand)
        rtp.GotoRunAndIch(iRun, 0);
        const CParaFormat * pPF = rtp.GetPF();

        if(fFirstRun)
        {
            bAlignment = (htmlBlockAlign)pPF->_bBlockAlign;
            fFirstRun = FALSE;
        }
        else
        {
            // Check different fields of the format and reset the mask bits
            if(bAlignment != pPF->_bBlockAlign)
            {
                fPartial = TRUE;
                break;
            }
        }
    }

    // If selection has more than 1 type of justification

    if(fPartial)
        return STATUS_PARTIAL;

    if (bAlignment == htmlBlockAlignLeft   && nCmdID == IDM_JUSTIFYLEFT ||
        bAlignment == htmlBlockAlignNotSet && nCmdID == IDM_JUSTIFYLEFT ||
        bAlignment == htmlBlockAlignRight  && nCmdID == IDM_JUSTIFYRIGHT ||
        bAlignment == htmlBlockAlignCenter && nCmdID == IDM_JUSTIFYCENTER)
    {
        return STATUS_ON;
    }

    return STATUS_OFF;
#endif // #if 0
#else
    return STATUS_OFF;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     GetCurrentBlockDirection
//
//  Synopsis:   Determines the direction type of selection using the info. in
//              run
//
//  Arguments:  nCmdID - one of the following IDM_BLOCKDIRLTR or IDM_BLOCKDIRRTL
//
//  Returns:    STATUS_ON - if the button must be on, STATUS_OFF if OFF,
//              STATUS_PARTIAL if the format changes in selection
//
//-----------------------------------------------------------------------------

STATUS
CTxtRange::GetCurrentBlockDirection( int nCmdID )
{
#ifdef MERGEFUN // rtp
    htmlDir bDir;
    CRchTxtPtr rtp(*this);

    rtp.SetCp(GetCpMin());
    const CParaFormat *pPF = rtp.GetPF();

    bDir = pPF->HasRTL(TRUE) ? htmlDirRightToLeft : htmlDirLeftToRight;

    if(bDir == htmlDirRightToLeft && nCmdID == IDM_BLOCKDIRRTL ||
       bDir == htmlDirLeftToRight && nCmdID == IDM_BLOCKDIRLTR)
    {
        return STATUS_ON;
    }
#endif
    return STATUS_OFF;
}

/*
 *  Delete(fCtrl, publdr)
 *
 *  @mfunc
 *      Delete the selection. If fCtrl is true, this methods delete from
 *      min of selection to end of word
 *
 *  @rdesc
 *      TRUE if successful
 */

HRESULT
CTxtRange::Delete ( DWORD fCtrl )
{
#ifdef MERGEFUN // rtp
    HRESULT hr = S_OK;
    CPedUndo pu( GetPed() );
    
    if (IsOrphaned())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (IsDocLoading())
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    if(fCtrl)
    {
        TCHAR ch;
                                            // Delete to word end right from CpMin
        SetExtend(FALSE);                   //  (won't necessarily repaint, since
        *this = GetCpMin();                 //  won't delete it)

        // NB (cthrash) MSWORD compat:

        for (;;)
        {
            ch = _rpTX.GetPrevChar();

            if (ch == L' ' || ch == WCH_NBSP)
            {
                Advance(-1);
            }
            else
            {
                break;
            }
        }

        SetExtend(TRUE);

        ch = GetChar();
        if (ch == L' ' || ch == WCH_NBSP)
        {
            FindWordBreak(WB_MOVEWORDRIGHT);
            FindWordBreak(WB_MOVEWORDRIGHT);
        }
        else
        {
            FindWordBreak(WB_RIGHTEDGE);
        }

        // Give back a space if is last char of selection.

        if (_cch)
        {
            Assert(_cch > 0);
            ch = _rpTX.GetPrevChar();
            if (ch == L' ' || ch == WCH_NBSP)
            {
                Advance(-1);
            }
        }
    }
    else
    {
        SetExtend(TRUE);                    // Setup to change selection
        
        if(IsClusterTypeChar(GetChar()))
        {
            CTextSelectionRecord * pTSR;

            IGNORE_HR(GetPed()->GetSel(&pTSR, FALSE));
            pTSR->MoveToClusterBreak(TRUE, TRUE); // move forward and delete the char
        }
    }

    {
        CElement *      pContainer = GetCommonContainer();

        pu.Start( IDS_UNDOTEXTDELETE, this, pContainer->IsEditable() );
    }

    if(!_cch)                               // No selection
    {
        DTSB_RET dtsbRet;

        hr = THR(DeleteTxtSiteBreak(TRUE, dtsbRet));
        if (hr)
            goto Cleanup;
        else if (DTSB_NODELETE == dtsbRet)
        {
            // Found a TSB, but cannot delete
            GetPed()->Layout()->Sound();
            goto Cleanup;
        }
        else if (DTSB_DELETED == dtsbRet)
        {
            // Found a TSB and had it deleted
            goto Cleanup;
        }
        // else not found a TSB, so go ahead with normal processing

        //BUGBUG!!
        if(FAILED(AdjustCRLF( 1 )))         // Try to select char at IP
        {
            GetPed()->Layout()->Sound();              // End of text, nothing to delete
            goto Cleanup;
        }
    }
    else
    {
        if(_cch < 0)                        // Ensure active end is cpMost
            FlipRange();
        if(_cch > cchCRLF && _rpTX.IsAfterEOP())    // Selection ends with para mark
        {
            CRchTxtPtr tp = *this;          // Don't delete para mark unless
            tp -= _cch;                     //  selection starts at BOL

            BOOL after = tp._rpTX.IsAfterEOP();
            if ( ! after )
            {
                //
                // BUGBUG marka - Bug # 21688 - IsAfterEOP() will fail at the first 
                // char position in the document 
                // and will actually be at a TextSiteBreak from the Root
                // this is a very ugly hack - but all this should be reworked for Beta 2 anyway
                //
                
                TCHAR curChar = tp._rpTX.GetChar();
                TCHAR prevChar = tp._rpTX.PrevChar();
                if (( IsTxtSiteBreak(curChar)) ||(IsTxtSiteBreak(prevChar)))
                {
                    CFlowLayout* pFlow = tp._rpTX._ped->GetFlowLayoutForCp( tp._rpTX.GetCp());
                    if (( pFlow->Element()->_etag ) == ETAG_ROOT )
                        after = TRUE;
                }
            }
            if((int)tp > 0 && ! after )
                AdjustCRLF( -1 );
        }
    }
#ifdef MOVE_TO_RICH
    if(ped->fObSelected && !CmdFUnselectOb(GetCurrTxtSite()))
    {                                       // Couldn't unselect object
        ped->Sound();
        goto Cleanup;
    }
#endif

    hr = THR( ValidateReplace( FALSE, TRUE ) );

    if (hr == S_FALSE)
    {
        GetPed()->Layout()->Sound();
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR( ReplaceRange( 0, NULL ) );

    if (hr)
        goto Cleanup;

    hr = THR( AdjustForInsert() );

    if (hr)
        goto Cleanup;

Cleanup:
    pu.Finish( hr );

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::PasteFileToRange
//
//  Synopsis:   Open a text file and insert it into document
//
//  BUGBUG (johnv) What is paryElems used for?
//-----------------------------------------------------------------------------

extern HRESULT CreateStreamOnFile(
        LPCTSTR lpstrFile,
        DWORD dwSTGM,
        LPSTREAM * ppstrm);             // not prototyped in any header file...

HRESULT
CTxtRange::PasteFileToRange (const TCHAR * pszTextFilePath)
{
#ifdef MERGEFUN // rtp
    HRESULT hr = S_OK;
    BOOL    fCreatedElement = FALSE;        // TRUE if we had to create an XMP element
    IStream* pInputStm = NULL;
    CTreeNode* pNode = NULL;

    Assert(pszTextFilePath && pszTextFilePath[0]);

    // Create a stream on the input file
    hr = THR(CreateStreamOnFile(
            pszTextFilePath,
            STGM_READ | STGM_SHARE_DENY_WRITE,
            &pInputStm));
    if( hr )
        goto Error;

    // BUGBUG: johnv For now we are creating an XMP element, and adding the text from the file
    //               into it.  Always inserts in the current run. What about editing?

    // First, make sure we are not already within an XMP tag...
    pNode = SearchBranchForTag( ETAG_XMP );
    if( NULL == pNode )
    {
        CElement *pElementNew;
        hr = THR( GetList().CreateAndInsertElement( NULL, GetIRun(), GetIRun(), ETAG_XMP, &pElementNew ) );
        if( hr )
            goto Error;
        fCreatedElement = TRUE;

        pNode = SearchBranchForScopeInStory(pElementNew);

        Assert(pNode);

        pNode->AddRef();
        pElementNew->Release();
    }

    // Read line by line, and add to our XMP element
    {
        TCHAR buffer[ 512 ];
        long    cbBuffer, linesRead = 0;
        CStreamReadBuff streamReader( pInputStm );

        while( streamReader.GetLine( buffer, ARRAY_SIZE(buffer)-1 ) == S_OK )
        {
            cbBuffer = _tcslen( buffer );
            _tcscpy(buffer+cbBuffer, _T("\r"));

            hr = InsertChars( cbBuffer + 1, buffer, pNode );

            if( hr )
                goto Error;

        }

    }

Cleanup:

    // Make sure we only to a ClearPtr if we have actually created a new element
    if( fCreatedElement )
        CTreeNode::ClearPtr(&pNode);

    ReleaseInterface( pInputStm );      // these functions also check for null

    RRETURN1(hr, S_FALSE);

Error:
    goto Cleanup;
#else
    RRETURN(E_FAIL);
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     SetTextRangeToElement
//
//  Synopsis:   Have this range select all text under the influence of the
//              given element.  If the element is a text site with a text
//              site end character, this character is excluded from the
//              range.
//
//  Return:     S_FALSE if the element cannot be located in the tree
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::SetTextRangeToElement ( CElement * pElement )
{
    IHTMLElement * pHTMLElement = NULL;
    HRESULT        hr = S_OK;

    if (! pElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // BUGBUG - for inputs - we want to drill in to the inner slave element
    //
    if ( pElement->HasSlaveMarkupPtr() )
        pElement = pElement->GetSlaveMarkupPtr()->FirstElement();
        
    hr = THR( pElement->QueryInterface( IID_IHTMLElement, (void **) & pHTMLElement ) );
    if (hr)
        goto Cleanup;

    if (! pElement->IsNoScope() )
    {
        hr = THR( _pLeft->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_AfterBegin ) );
        if (hr)
            goto Cleanup;

        hr = THR( _pRight->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_BeforeEnd ) );
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR( _pLeft->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_BeforeBegin ) );
        if (hr)
            goto Cleanup;

        hr = THR( _pRight->MoveAdjacentToElement( pHTMLElement, ELEM_ADJ_AfterEnd ) );
        if (hr)
            goto Cleanup;

    }

    hr = THR( ValidatePointers() );

Cleanup:
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   AdjustFragment
//
//  Synopsis:   Helper function to workaround a problem in ParseString()
//              ParseString() currently does not position the start fragment
//              under the body. AdjustFragment() move the passed in Fragment
//              pointer under the body.
//
//-----------------------------------------------------------------------------
HRESULT
AdjustFragment( IMarkupPointer * pFragment, IMarkupContainer * pContainer )
{
    HRESULT                hr = S_OK;
    IHTMLElement        *  pIElement = NULL;
    IMarkupPointer      *  pStart = NULL;
    ELEMENT_TAG_ID         tagId;
    IMarkupServices     *  pMarkupServices = NULL;
    IHTMLDocument2      *  pIDoc = NULL;
    MARKUP_CONTEXT_TYPE    context = CONTEXT_TYPE_EnterScope;

    hr = THR( pContainer->OwningDoc( & pIDoc ) );
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE( pIDoc->QueryInterface( IID_IMarkupServices,
            (void**) & pMarkupServices) );
    if (hr)
        goto Cleanup;

    hr = THR( pMarkupServices->CreateMarkupPointer( & pStart ) );
    if (hr)
        goto Cleanup;

    hr = THR( pStart->MoveToPointer( pFragment ) );
    if (hr)
        goto Cleanup;

    hr = THR( pStart->CurrentScope( & pIElement ) );
    if (hr)
        goto Cleanup;

    for( ; ; )
    {
        if ( pIElement && (context == CONTEXT_TYPE_EnterScope) )
        {
            hr = THR( pMarkupServices->GetElementTagId( pIElement, &tagId ) );
            if (hr)
                goto Cleanup;

            if (tagId == TAGID_BODY)
            {
                hr = THR( pFragment->MoveToPointer( pStart ) );
                if (hr)
                    goto Cleanup;

                break;
            }

        }
        else if ( context == CONTEXT_TYPE_None)
        {    
            hr = E_FAIL;
            break;
        }
        ClearInterface( & pIElement );
        hr = THR( pStart->Right(TRUE, & context, & pIElement, NULL, NULL) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface( pIElement );
    ReleaseInterface( pStart );
    ReleaseInterface( pMarkupServices );
    ReleaseInterface( pIDoc );
 
    RRETURN( hr );
}
//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::PasteHTMLToRange
//
//  Synopsis:   Helper function to parse HTML text and insert the result at the
//              CElement associated with this range.  Note, the HTML text is
//              passed as unicode which is then converted to multibyte prior to
//              parsing.
//
//  Arguments   [in] pStr       HTML text
//              [in] dwCount    number of chars in pStr (not including NULL)
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::PasteHTMLToRange (
    const TCHAR * pStr, long cch )
{
#ifdef MERGEFUN // rtp
    HRESULT hr = S_OK;
    HGLOBAL hHtmlText = NULL;
    LONG cpMin, cpMost;
    LONG cCharsPasted = _rpTX.GetTextLength();

    GetRange(cpMin, cpMost);

    if (!GetPed()->HasFlag( TAGDESC_ACCEPTHTML ))
        goto Cleanup;

    hr = THR( HtmlStringToSignaturedHGlobal( & hHtmlText, pStr, cch ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        GetPed()->GetDTE()->HandlePasteHTMLToRange(
            this, hHtmlText, NULL, hps ) );

    if (hr == S_FALSE)
        goto Cleanup;

    if (hr)
        goto Cleanup;

Cleanup:

    if (hr == S_OK)
    {
        cCharsPasted = _rpTX.GetTextLength() - cCharsPasted;

        // Just launder the 2 edges of the pasted in range. The intermediate
        // stuff is already taken care of by the parser.
        LaunderSpaces(cpMin, 1);
        LaunderSpaces(max(cpMost, cpMin + cCharsPasted) - 1, 1);
    }

    if (hHtmlText)
        GlobalFree( hHtmlText );

    RRETURN1( hr, S_FALSE );
#else

#if 1
    
    HRESULT hr = S_OK;
    CMarkupPointer * pMpLeft, * pMpRight;

    extern HRESULT HandleUIPasteHTML (
        CMarkupPointer *, CMarkupPointer *, const TCHAR *, long );

    hr = THR( _pLeft->QueryInterface( CLSID_CMarkupPointer, (void **) & pMpLeft ) );

    if (hr)
        goto Cleanup;

    hr = THR( _pRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pMpRight ) );

    if (hr)
        goto Cleanup;

    hr = THR( HandleUIPasteHTML( pMpLeft, pMpRight, pStr, cch ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
    
#else
    
    HRESULT     hr;
    IMarkupContainer    *  pContainer = NULL;
    IMarkupPointer      *  pStart = NULL;
    IMarkupPointer      *  pEnd = NULL;
    CDoc                *  pDoc;
    BOOL                   fAreEqual;

    if (! pStr)
    { 
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pDoc = _pMarkup->Doc();
    Assert( pDoc );

    hr = THR( pDoc->ParseString ( const_cast<OLECHAR *> (pStr), hps, & pContainer, & pStart, & pEnd ) );
    if (hr)
        goto Cleanup;

    // BUGBUG:  raminh
    //          AdjustFragment() is used as a workaround because
    //          ParseString() does not position the StartFrag under the body
    //
    hr = THR( AdjustFragment( pStart, pContainer ) );
    if (hr)
        goto Cleanup;

    hr = THR( _pLeft->Equal( _pRight, & fAreEqual ) );
    if (hr)
        goto Cleanup;

    if (! fAreEqual)
    {
        hr = THR( pDoc->Remove( _pLeft, _pRight ) );
        if (hr)
            goto Cleanup;
    }
    
    hr = THR( pDoc->Move( pStart, pEnd, _pRight ) );
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    ReleaseInterface( pContainer );
    RRETURN( hr );

#endif

#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::GetBstrHelper
//
//  Synopsis:   Gets text from the range into a given bstr in a specified
//              save mode.
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::GetBstrHelper(BSTR * pbstr, DWORD dwSaveMode, DWORD dwStrWrBuffFlags)
{
    HRESULT  hr;
    LPSTREAM pIStream = NULL;

    *pbstr = NULL;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pIStream);
    if (hr)
        goto Cleanup;

    //
    // Use a scope to clean up the StreamWriteBuff
    //
    {
        CStreamWriteBuff StreamWriteBuff(pIStream, CP_UCS_2);

        StreamWriteBuff.SetFlags(dwStrWrBuffFlags);

        //
        // BUGBUG (johnbed) RaminH: make this call the CAutoRange::SaveHTMLToStream
        //
        
        hr = THR( SaveHTMLToStream( &StreamWriteBuff, dwSaveMode ));
        if (hr)
            goto Cleanup;

        StreamWriteBuff.Terminate();    // appends a null character
    }

    hr = GetBStrFromStream(pIStream, pbstr, TRUE);

Cleanup:

    ReleaseInterface(pIStream);

    RRETURN(hr);
}
 
//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::GetBstrHTML
//
//  Synopsis:   Gets a BSTR holding the HTML text from the range
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::GetBstrHTML(BSTR * pbstr)
{
    RRETURN(GetBstrHelper(pbstr, RSF_FRAGMENT, 0));
}

//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::GetBstrSimpleText
//
//  Synopsis:   Gets a BSTR holding the simple text from the range
//
//-----------------------------------------------------------------------------

HRESULT
CTxtRange::GetBstrSimpleText(BSTR * pbstr)
{
    RRETURN(GetBstrHelper(pbstr, RSF_SELECTION, WBF_SAVE_PLAINTEXT|WBF_NO_WRAP));  // new
}

//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::GetCommonElement
//
//  Synopsis:
//
//  Notes:      GetMinBranch and GetMostBranch are helper functions.
//-----------------------------------------------------------------------------

CTreeNode * GetMinBranch(CTxtRange *ptr)
{
#ifdef MERGEFUN // rtp 
    long cpMin, cpMost;
    CRchTxtPtr rtp(*ptr);
    ptr->GetRange (cpMin, cpMost);
    rtp.SetCp (cpMin);
    return rtp.CurrBranch();
#else
    return NULL;
#endif
}

CTreeNode * GetMostBranch(CTxtRange *ptr)
{
#ifdef MERGEFUN // rtp
    long cpMin, cpMost;
    ptr->GetRange (cpMin, cpMost);
    if (cpMost - cpMin <= 1)
    {
        // Our ranges are not inclusive at cpMost
        Assert((cpMost - cpMin) == (ptr->GetCch() < 0 ? -ptr->GetCch() : ptr->GetCch()));
        return (GetMinBranch(ptr));
    }
    else
    {
        CRchTxtPtr rtp(*ptr);
        rtp.SetCp (cpMost-1);
        return rtp.CurrBranch();
    }
#else
    return(NULL);
#endif
}

CTreeNode *
CTxtRange::GetCommonNode()
{
    CTreeNode * ptnLeft  = LeftNode();
    CTreeNode * ptnRight = RightNode();
    
    if (ptnLeft && ptnRight)
    {
        return ptnLeft->GetFirstCommonAncestor( ptnRight, NULL);
    }
    else
    {
        return NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::GetCommonContainer
//
//  Synopsis:   Find the common container of the range, if the range is inside
//              the same textsite, we return the textsite, otherwise we return
//              the ped.
//
//-----------------------------------------------------------------------------

CElement *CTxtRange::GetCommonContainer()
{
    return GetCommonNode()->GetContainer();
}

//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::OwnedBySingleTxtSite
//
//  Synopsis:   Function to determine if a single txt site owns the complete
//              range.
//
//  Arguments   None
//
//-----------------------------------------------------------------------------

BOOL
CTxtRange::OwnedBySingleFlowLayout()
{
    HRESULT     hr;
    BOOL        bResult = FALSE;
    CTreeNode   *ptnLeft  = LeftNode();
    CTreeNode   *ptnRight = RightNode();
    CElement    *pFlowLeft;
    CElement    *pFlowRight;
    IObjectIdentity *pIdentLeft = NULL;
    IObjectIdentity *pUnkRight = NULL;
    
    if (ptnLeft && ptnRight)
    {        
        pFlowLeft = ptnLeft->GetFlowLayout()->ElementOwner();
        pFlowRight = ptnRight->GetFlowLayout()->ElementOwner();

        hr = THR(pFlowLeft->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdentLeft));
        if (FAILED(hr))
            goto Cleanup;

        hr = THR(pFlowRight->QueryInterface(IID_IUnknown, (LPVOID *)&pUnkRight));
        if (FAILED(hr))
            goto Cleanup;

        hr = THR(pIdentLeft->IsEqualObject(pUnkRight));
        bResult = (hr == S_OK);
    }

Cleanup:
    ReleaseInterface(pIdentLeft);
    ReleaseInterface(pUnkRight);
    return bResult;
}

//+----------------------------------------------------------------------------
//
//  Member:     CTxtRange::SelectionInOneTxtSite
//
//  Synopsis:   Function to determine if a single txt site owns both the
//              begin and end of a selection.
//
//  Arguments   None
//
//-----------------------------------------------------------------------------

BOOL
CTxtRange::SelectionInOneFlowLayout()
{
#ifdef MERGEFUN // rtp
#if 0
    // bugbug: why does doing it this way break om3.htm? (johnbed)
    HRESULT hr = ValidateReplace( FALSE, FALSE );
    return( hr == S_OK );
#endif

    BOOL fRet = TRUE;
    long cpMin, cpMost;
    CFlowLayout * pFromOFL;
    CFlowLayout * pToOFL;
    GetRange(cpMin, cpMost);
    
    if (cpMost - cpMin > 1)
    {
        CRchTxtPtr rtpFrom(GetPed(), cpMin);
        CRchTxtPtr rtpTo(GetPed(), cpMost);

        while (rtpFrom.AdjustForward());
        while (rtpTo.AdjustBackward());

        pFromOFL = rtpFrom.GetOwningFlowLayout();
        pToOFL = rtpTo.GetOwningFlowLayout();

        fRet = pFromOFL == pToOFL;

        if (!fRet)
        {
            while (rtpFrom.AdjustBackward());
            while (rtpTo.AdjustForward());

            pFromOFL = rtpFrom.GetOwningFlowLayout();
            pToOFL = rtpTo.GetOwningFlowLayout();

            if( pFromOFL->Element()->Tag() == ETAG_ROOT )
                pFromOFL = pFromOFL->Doc()->GetElementClient()->GetFlowLayout();

            if( pToOFL->Element()->Tag() == ETAG_ROOT )
                pToOFL = pFromOFL->Doc()->GetElementClient()->GetFlowLayout();

            fRet = pFromOFL == pToOFL;

        }
    }
    return fRet;
#else // MERGEFUN
    BOOL fRet = TRUE;
    CFlowLayout * pFromOFL;
    CFlowLayout * pToOFL;
    CTreeNode * ptnLeft  = LeftNode();
    CTreeNode * ptnRight = RightNode();
    
    if (ptnLeft && ptnRight) // BUGBUG: check if (cpMost - cpMin > 1)
    {
        pFromOFL = ptnLeft->GetFlowLayout();
        pToOFL   = ptnRight->GetFlowLayout();

        fRet = pFromOFL == pToOFL;

        /* BUGBUG: CAutoRange TODO (raminh) 
        if (!fRet)
        {
            while (rtpFrom.AdjustBackward());
            while (rtpTo.AdjustForward());

            pFromOFL = rtpFrom.GetOwningFlowLayout();
            pToOFL = rtpTo.GetOwningFlowLayout();

            if( pFromOFL->Element()->Tag() == ETAG_ROOT )
                pFromOFL = pFromOFL->Doc()->GetElementClient()->GetFlowLayout();

            if( pToOFL->Element()->Tag() == ETAG_ROOT )
                pToOFL = pFromOFL->Doc()->GetElementClient()->GetFlowLayout();

            fRet = pFromOFL == pToOFL;
        }
        */
    }
    return fRet;
#endif
}

//+----------------------------------------------------------------------------
//
//  Class:      CRangeRestorer
//
//  Purpose:    Manipulates the document this range lives in such that
//              the range can survive complex document operations.
//
//-----------------------------------------------------------------------------

CRangeRestorer::CRangeRestorer ( CTxtRange * pRange )
{
    Assert( pRange );

    _pRange = pRange;
    _fMarked = FALSE;
}

HRESULT
CRangeRestorer::Set ( )
{
#ifdef MERGEFUN // rtp
    HRESULT hr = S_OK;

    //
    // Don't support non-degenerate ranges ... yet
    //

    if (_pRange->GetCch())
        goto Cleanup;

    //
    // Place the range marker character into the document
    //

    hr = THR( _pRange->InsertCharInCurrRun( WCH_RANGE_MARKER ) );

    if (hr)
        goto Cleanup;

    _fMarked = TRUE;

Cleanup:

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}

HRESULT
CRangeRestorer::Restore ( )
{
#ifdef MERGEFUN // rtp
    HRESULT hr = S_OK;
    const TCHAR * pch;
    long cpFound = -1;
    long cchValidBackward, cchValidForward;

    //
    // Don;t attempt to locate marker unless we put one in.
    //

    if (!_fMarked)
        goto Cleanup;

    //
    // First, attempt to locate the marker quickly by examining the text
    // immediatly around the current range.
    //

    pch = _pRange->_rpTX.GetPchReverse( cchValidBackward, & cchValidForward );

    if (pch)
    {
        long ich, ichMax;

        //
        // First, look to the left.  We should find it immediately there most
        // of the time (because thats where we placed it in the first place).
        //

        ichMax = - min( 100L, cchValidBackward );

        for ( ich = -1 ; ich >= ichMax ; ich-- )
        {
            if (pch[ich] == WCH_RANGE_MARKER)
            {
                cpFound = long( _pRange->GetCp() ) + ich;
                goto FoundIt;
            }
        }

        ichMax = min( 100L, cchValidForward );

        for ( ich = 0 ; ich <= ichMax ; ich++ )
        {
            if (pch[ich] == WCH_RANGE_MARKER)
            {
                cpFound = long( _pRange->GetCp() ) + ich;
                goto FoundIt;
            }
        }
    }

    //
    // Locate the marker the HARD (expensive) way.
    //

    {
        CTxtPtr tp ( _pRange->_rpTX._ped, 0 );

        pch = tp.GetPch( cchValidForward );

        if (!pch || cchValidForward == 0)
            goto Cleanup;

        tp.AdvanceCp( cchValidForward );

        for ( cchValidForward-- ; cchValidForward >= 0 ; cchValidForward-- )
        {
            if (pch[cchValidForward] == WCH_RANGE_MARKER)
            {
                cpFound = long( tp.GetCp() ) + cchValidForward;
                goto FoundIt;
            }
        }
    }

    //
    // Can't find it ..... Oh Well! ...
    //

Cleanup:

    RRETURN( hr );


FoundIt:

    Assert( cpFound >= 0 );

    //
    // Position the range before the marker, and remove it.
    //

    Verify( long( _pRange->SetCp( cpFound ) ) == cpFound );
    _pRange->AdvanceToNonEmpty();

    Assert( _pRange->_rpTX.GetChar() == WCH_RANGE_MARKER );

    hr = THR( _pRange->DeleteCurrentCharInRun() );

    if (hr)
        goto Cleanup;

    goto Cleanup;
#else
    RRETURN(E_FAIL);
#endif
}

//+----------------------------------------------------------------------------
//
//  Function:  DeleteTxtSiteBreak
//
//  Purpose:   Deletes the site associated with a txtsitebreak character
//
//  Params:    fDirForward: specifies delete or backspace
//             dtsbRet: the return value:
//                      DTSB_NODELETE: Break char found but cannot delete
//                      DTSB_NOBREAKCHAR: No break char at current cp
//                                        go ahead with normal deletion
//                      DTSB_DELETED: Break char found and deleted associated site
//
//-----------------------------------------------------------------------------
HRESULT
CTxtRange::DeleteTxtSiteBreak(BOOL fDirForward, DTSB_RET &dtsbRet)
{
#ifdef MERGEFUN // rtp
    CRchTxtPtr rtp(*this);
    TCHAR ch;
    HRESULT hr = S_OK;
    CLayout     *  pLayout;
    CElement    *  pElementFL;
    CFlowLayout *  pFlowLayout;
    long cchSite, cpSite;

    Assert(_cch == 0);

    dtsbRet = DTSB_NODELETE;

    // Get the run owner of where the cp is. Advancing to nonempty for
    // both forward and reverse directions is correct. In the forward
    // direction, we would end up in the run containing the TSB. In the
    // reverse direction, we would end up in a run outside the scope
    // of the txtsite whose end is signified by the TSE character.
    rtp.AdvanceToNonEmpty();
    pFlowLayout = rtp.GetRunOwnerBranch(NULL)->GetFlowLayout();
    pElementFL  = pFlowLayout->Element();
    Assert(pFlowLayout);

    if (fDirForward)
    {
        ch = rtp._rpTX.GetChar();
        if (0 == ch ||                   // Happens if positioned after last char
            WCH_TXTSITEBREAK != ch
           )
        {
            if (WCH_TXTSITEEND == ch)
                dtsbRet = DTSB_NODELETE;
            else
                dtsbRet = DTSB_NOBREAKCHAR;
            goto Cleanup;
        }

        // Now go to the next character. It definitely lives under
        // a site other than pFlowLayoutContainer. This site is the one
        // which we have to delete.
        rtp.Advance(1);
    }
    else
    {
        if (rtp.GetCp() == 0)
        {
            dtsbRet = DTSB_NOBREAKCHAR;
            goto Cleanup;
        }

        // Go back once so we can see the character at that position
        rtp.Advance(-1);

        ch = rtp._rpTX.GetChar();
        if (WCH_TXTSITEEND != ch)
        {
            if (WCH_TXTSITEBREAK == ch)
                dtsbRet = DTSB_NODELETE;
            else
                dtsbRet = DTSB_NOBREAKCHAR;
            goto Cleanup;
        }
    }

    rtp.AdvanceToNonEmpty();
    pLayout = rtp.GetRunOwner(pFlowLayout);

    // The character is not within this txtsite. Probably under some
    // other txtsite. In which case we cannot delete it.
    if (!pLayout)
    {
        dtsbRet = DTSB_NODELETE;
        goto Cleanup;
    }

    // We should end up in a site under the scope of the pTxtSiteContainer
    // else something is wrong with the tree.
    Assert(pLayout != pFlowLayout);

    // We now have have pSite which is to be deleted.
    cchSite = pFlowLayout->GetList().GetElementCch(pLayout->Element());
    // Should atleast contain the TxtSiteEnd character
    Assert(cchSite > 0);

    rtp.SetCp(pElementFL->GetFirstCp());
    cpSite = rtp.CpFromSite(pLayout);

    // Following assert checks > FirstCp because even if pSite were the
    // first one in pTxtSiteContainer, the TSB would be at the firstcp pos.
    Assert(cpSite > pElementFL->GetFirstCp());
    Assert(cpSite < pElementFL->GetLastCp());

    Set(cpSite, -cchSite);

    hr = THR(ReplaceRange(0, NULL));
    if (hr)
        goto Cleanup;

    // Everything's fine ...
    dtsbRet = DTSB_DELETED;
    hr = S_OK;

Cleanup:
    RRETURN(hr);
#else
    RRETURN(E_FAIL);
#endif
}


//+----------------------------------------------------------------------------
//
//  Method:     VariantCompareBSTRS, local helper
//
//  Synopsis:   compares 2 btrs
//
//-----------------------------------------------------------------------------

BOOL VariantCompareBSTRS(VARIANT * pvar1, VARIANT * pvar2)
{
    BOOL    fResult;
    TCHAR  *pStr1;
    TCHAR  *pStr2;

    if (V_VT(pvar1) == VT_BSTR && V_VT(pvar2) == VT_BSTR)
    {
        pStr1 = V_BSTR(pvar1) ? V_BSTR(pvar1) : g_Zero.ach;
        pStr2 = V_BSTR(pvar2) ? V_BSTR(pvar2) : g_Zero.ach;

        fResult = StrCmpC(pStr1, pStr2) == 0;
    }
    else
    {
        fResult = FALSE;
    }

    return fResult;
}


//+----------------------------------------------------------------------------
//
//  Method:     VariantCompareFontSize, local helper
//
//  Synopsis:   compares font size
//
//-----------------------------------------------------------------------------

BOOL VariantCompareFontSize(VARIANT * pvarSize1, VARIANT * pvarSize2)
{
    CVariant    convVar1;
    CVariant    convVar2;
    BOOL        fResult;

    Assert(pvarSize1);
    Assert(pvarSize2);

    if (   V_VT(pvarSize1) == VT_NULL
        || V_VT(pvarSize2) == VT_NULL
       )
    {
        fResult = V_VT(pvarSize1) == V_VT(pvarSize2);
        goto Cleanup;
    }

    if (VariantChangeTypeSpecial(&convVar1, pvarSize1, VT_I4))
        goto Error;

    if (VariantChangeTypeSpecial(&convVar2, pvarSize2, VT_I4))
        goto Error;

    fResult = V_I4(&convVar1) == V_I4(&convVar2);

Cleanup:
    return fResult;

Error:
    fResult = FALSE;
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Method:     VariantCompareColor, local helper
//
//  Synopsis:   compares color
//
//-----------------------------------------------------------------------------

BOOL VariantCompareColor(VARIANT * pvarColor1, VARIANT * pvarColor2)
{
    BOOL        fResult;
    CVariant    var;
    COLORREF    color1;
    COLORREF    color2;

    if (   V_VT(pvarColor1) == VT_NULL
        || V_VT(pvarColor2) == VT_NULL
       )
    {
        fResult = V_VT(pvarColor1) == V_VT(pvarColor2);
        goto Cleanup;
    }

    if (VariantChangeTypeSpecial(&var, pvarColor1,  VT_I4))
        goto Error;

    color1 = (COLORREF)V_I4(&var);

    if (VariantChangeTypeSpecial(&var, pvarColor2, VT_I4))
        goto Error;

    color2 = (COLORREF)V_I4(&var);

    fResult = color1 == color2;

Cleanup:
    return fResult;

Error:
    fResult = FALSE;
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Method:     VariantCompareFontName, local helper
//
//  Synopsis:   compares font names
//
//-----------------------------------------------------------------------------

BOOL VariantCompareFontName(VARIANT * pvarName1, VARIANT * pvarName2)
{
    return VariantCompareBSTRS(pvarName1, pvarName2);
}


/*
 *  CTxtRange::GetCurrentFontHeight(pyDescent)
 *
 *  @rdesc
 *      Return the current font height, taking the spring loaded
 *      font into acount
 */

// TODO: Bug 39010: Move this to mshtmled!!!

INT CTxtRange::GetCurrentFontHeight(
    INT *pyDescent,
    BOOL fTrueHeight) const        //@parm Out parm to receive caret descent
{
#ifdef MERGEFUN // rtp
    CCharFormat cfLocal = *((CRchTxtPtr *)this)->GetCF();
    INT         CaretHeight = -1;
    CCcs *      pccs;
    CCalcInfo   CI;
    long lSize;

    if ( GetFormatInfoFontSize(&lSize) == S_OK )
    {
        // If we just cached the format info, use the size stored there.
        // Modify local size.

        // Only use cached format if font new font is smaller.
        if (lSize < cfLocal._yHeight || fTrueHeight)
        {
            cfLocal.SetHeightInTwips( lSize );
        }

        AssertSz (V_BSTR(&_FormatInfo._varName),
                  "spring loaded fontname should never be null");
        cfLocal.SetFaceName( (const TCHAR *)V_BSTR(&_FormatInfo._varName) );

        // Do this to suppress assert in GetCcs.
        cfLocal._bCrcFont = cfLocal.ComputeFontCrc();
    }

    CI.Init(GetPed()->GetLayout());

    pccs = fc().GetCcs(CI._hdc, &CI, &cfLocal);

    if(!pccs)
        goto ret;

    if(pyDescent)
        *pyDescent = (INT) pccs->_yDescent;

    CaretHeight = pccs->_yHeight + pccs->_yOffset;

    pccs->Release();

ret:

    return CaretHeight;
#else
    return 0;
#endif
}


/*
 *  CTxtRange::LaunderSpacesForRange
 *
 *  @rdesc
 */

void
CTxtRange::LaunderSpacesForRange(LONG cpMin, LONG cpMost)
{
    LONG cpMinAfter, cpMostAfter;

    GetRange(cpMinAfter, cpMostAfter);
    cpMin = min(cpMin, cpMinAfter);
    cpMost = max(cpMost, cpMostAfter);
    LaunderSpaces(cpMin, cpMost - cpMin);
}


/*
 *  CTxtRange::CheckOwnerSiteOrSelection
 *
 *  @rdesc
 */

HRESULT
CTxtRange::CheckOwnerSiteOrSelection(ULONG cmdID)
{
    HRESULT     hr = S_OK;

    switch ( cmdID )
    {
    case IDM_BLOCKFMT:
    case IDM_UNORDERLIST:
    case IDM_ORDERLIST:
    case IDM_INDENT:
    case IDM_OUTDENT:
    case IDM_JUSTIFYLEFT:
    case IDM_JUSTIFYRIGHT:
    case IDM_JUSTIFYCENTER:
    case IDM_JUSTIFYFULL:
    case IDM_JUSTIFYGENERAL:
    case IDM_BOOKMARK:
    case IDM_UNBOOKMARK:
    case IDM_HYPERLINK:
    case IDM_UNLINK:
    case IDM_OVERWRITE:
        if (!OwnedBySingleFlowLayout())
        {
            hr = S_FALSE;
        }
        break;

    case IDM_IMAGE:
    case IDM_PARAGRAPH:
    case IDM_IFRAME:
    case IDM_TEXTBOX:
    case IDM_TEXTAREA:
    case IDM_HTMLAREA:
    case IDM_CHECKBOX:
    case IDM_RADIOBUTTON:
    case IDM_DROPDOWNBOX:
    case IDM_LISTBOX:
    case IDM_BUTTON:
    case IDM_MARQUEE:
    case IDM_1D:
    case IDM_LINEBREAKNORMAL:
    case IDM_LINEBREAKLEFT:
    case IDM_LINEBREAKRIGHT:
    case IDM_LINEBREAKBOTH:
    case IDM_HORIZONTALLINE:
    case IDM_INSINPUTBUTTON:
    case IDM_INSINPUTIMAGE:
    case IDM_INSINPUTRESET:
    case IDM_INSINPUTSUBMIT:
    case IDM_INSINPUTUPLOAD:
    case IDM_INSFIELDSET:
    case IDM_INSINPUTHIDDEN:
    case IDM_INSINPUTPASSWORD:

    case IDM_GETBLOCKFMTS:
    case IDM_TABLE:

    case IDM_CUT:
    case IDM_PASTE:

        if (!SelectionInOneFlowLayout())
        {
            hr = S_FALSE;
        }
        break;
    }

    RRETURN1(hr, S_FALSE);
}


BOOL
CTxtRange::FSupportsHTML()
{
    if (_pMarkup)
    {
        CDoc        *pDoc   = _pMarkup->Doc();
        CTreeNode   *pNode  = GetCommonNode();

        if (pDoc && pNode)
        {
            CFlowLayout *pFlowLayout = pDoc->GetFlowLayoutForSelection(pNode);

            if (pFlowLayout)
            {
                CElement *pElement = pFlowLayout->ElementOwner();

                if (pElement)
                    return pElement->HasFlag(TAGDESC_ACCEPTHTML);
            }
        }
        

        
    }

    return FALSE;
}


/*
 *  CTxtRange::ExecBlockFormat
 *
 *  @rdesc
 *  Executes IDM_FORMATBLOCK command
 */

HRESULT
CTxtRange::ExecBlockFormat(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    long        l;
    ELEMENT_TAG etagCur;
    TCHAR       szBuffer [256];
    BSTR        bstrBuf;
    HRESULT     hr = MSOCMDERR_E_NOTSUPPORTED;

    if (pvarargIn && V_VT(pvarargIn) == VT_BSTR)
    {
        TCHAR* pArg = V_BSTR(pvarargIn);

        // Look for a block tag match for the input string.
        for (l = 0; l < ARRAY_SIZE(BlockFmts); ++l)
        {
            if( pArg[0] == _T('<') )
            {
                // If we start with angle brackets, see if we match a tag name.
                const TCHAR* pTagName = NameFromEtag( BlockFmts[l].Etag );
                int          iTagLength = _tcslen( pTagName );

                if( !_tcsnicmp(pArg + 1, iTagLength, pTagName, iTagLength) &&
                    !StrCmpC(pArg + 1 + iTagLength, _T(">")) )
                    break;
            }

            Verify(LoadString(g_hInstResource, BlockFmts[l].NameIDS, szBuffer, ARRAY_SIZE(szBuffer)));

            // Try comparing the name from the resource file
            if ( !_tcsicmp(pArg, szBuffer) )
            {
                break;
            }
        }

        // If none found, return error
        if (l >= ARRAY_SIZE(BlockFmts))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        // Get the block type.
        etagCur = BlockFmts[l].Etag;
        if (etagCur == ETAG_NULL)
        {
            etagCur = GetDefaultBlockTag();
        }

        // Set the new tag.
        hr = ApplyBlockFormat(etagCur);
    }
    if (pvarargOut )
    {
        ELEMENT_TAG etagCurrent = GetBlockFormat();

        if (etagCurrent == GetDefaultBlockTag())
            etagCurrent = ETAG_NULL;    // normal

        // Look for a string match to the current block tag.
        for (l = 0; l < ARRAY_SIZE(BlockFmts); ++l)
        {
            if (etagCurrent == BlockFmts[l].Etag)
            {
                Verify(LoadString(g_hInstResource, BlockFmts[l].NameIDS,
                                        szBuffer, ARRAY_SIZE(szBuffer)));
                break;
            }
        }

        // If none found, return an empty string.
        if (l >= ARRAY_SIZE(BlockFmts))
        {
            wcscpy(szBuffer, TEXT(""));
        }

        // Allocate and return a copy of the string.
        bstrBuf = SysAllocString(szBuffer);
        hr = bstrBuf ? S_OK : E_OUTOFMEMORY;
        V_VT(pvarargOut) = VT_BSTR;
        V_BSTR(pvarargOut) = bstrBuf;
    }

Cleanup:
    RRETURN(hr);
}

// Helper functions so that measurer can make empty lines
// have the same height they will have after we spring
// load them and put text in them.

long
GetSpringLoadedHeight(IMarkupPointer * pmpPosition, CFlowLayout * pFlowLayout, short * pyDescentOut)
{
    CDoc         * pDoc = pFlowLayout->Doc();
    CTreeNode    * pNode = pFlowLayout->GetFirstBranch();
    CCcs         * pccs;
    CCharFormat    cfLocal = *(pNode->GetCharFormat());
    CCalcInfo      CI;
    int            yHeight = -1;
    CVariant       varIn, varOut;
    GUID           guidCmdGroup = CGID_MSHTML;
    HRESULT        hr;

    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = pmpPosition;

    hr = THR(pDoc->Exec(&guidCmdGroup, IDM_COMPOSESETTINGS, 0, &varIn, &varOut));
    V_VT(&varIn) = VT_NULL;
    if (hr || V_VT(&varOut) == VT_NULL)
        goto Cleanup;

    // We now know that we have to apply compose settings on this line.

    // Get font size.
    V_VT(&varIn) = VT_I4;
    V_I4(&varIn) = IDM_FONTSIZE;
    hr = THR(pDoc->Exec(&guidCmdGroup, IDM_COMPOSESETTINGS, 0, &varIn, &varOut));
    if (hr)
        goto Cleanup;

    // If we got a valid font size, apply it to the local charformat.
    if (V_VT(&varOut) == VT_I4 && V_I4(&varOut) != -1)
    {
        int iFontSize = ConvertHtmlSizeToTwips(V_I4(&varOut));

        if (iFontSize < cfLocal._yHeight)
            cfLocal.SetHeightInTwips(iFontSize);
    }

    // Get font name.
    V_VT(&varIn) = VT_I4;
    V_I4(&varIn) = IDM_FONTNAME;
    hr = THR(pDoc->Exec(&guidCmdGroup, IDM_COMPOSESETTINGS, 0, &varIn, &varOut));
    if (hr)
        goto Cleanup;

    // If we got a valid font name, apply it to the local charformat.
    if (V_VT(&varOut) == VT_BSTR)
    {
        TCHAR * pstrFontName = V_BSTR(&varOut);
        cfLocal.SetFaceName(pstrFontName);
    }

    cfLocal._bCrcFont = cfLocal.ComputeFontCrc();

    CI.Init(pFlowLayout);

    pccs = fc().GetCcs(CI._hdc, &CI, &cfLocal);

    if (pccs)
    {
        yHeight = pccs->_yHeight + pccs->_yOffset;

        if (pyDescentOut)
            *pyDescentOut = (INT) pccs->_yDescent;

        pccs->Release();
    }

Cleanup:

    return yHeight;
}


long
GetSpringLoadedHeight(CMarkup * pMarkup, CFlowLayout * pFlowLayout, long cp, short * pyDescentOut)
{
    CDoc         * pDoc = pMarkup->Doc();
    CMarkupPointer mpComposeFont(pDoc);
    int            yHeight = -1;
    HRESULT        hr;

    hr = THR(mpComposeFont.MoveToCp(cp, pMarkup, FALSE));
    if (hr)
        goto Cleanup;

    yHeight = GetSpringLoadedHeight(&mpComposeFont, pFlowLayout, pyDescentOut);

Cleanup:

    return yHeight;
}


