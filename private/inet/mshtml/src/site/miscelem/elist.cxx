//+---------------------------------------------------------------------
//
//   File:      eolist.cxx
//
//  Contents:   List Element class (OL, DL, UL, MENU, DIR
//
//  Classes:    CListElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_EDLIST_HXX_
#define X_EDLIST_HXX_
#include "edlist.hxx"
#endif

#ifndef X_ELI_HXX_
#define X_ELI_HXX_
#include "eli.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#define _cxx_
#include "list.hdl"

MtDefine(CListElement, Elements, "CListElement")

const CElement::CLASSDESC CListElement::s_classdesc =
{
    {
        &CLSID_HTMLListElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLListElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLListElement,         // apfnTearOff

    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


HRESULT CListElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(pht->Is(ETAG_OL)   || pht->Is(ETAG_UL) ||
           pht->Is(ETAG_MENU) || pht->Is(ETAG_DIR));
    // N.B. DL's have their own class
    Assert(ppElementResult);
    *ppElementResult = new CListElement(pht->GetTag(), pDoc);
    return *ppElementResult ? S_OK : E_OUTOFMEMORY;
}

BOOL
IsListElement(CTreeNode * pNode)
{
    return pNode->Element()->HasFlag(TAGDESC_LIST);
}

//+------------------------------------------------------------------------
//
//  Member:     CListElement::ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CListElement::ApplyDefaultFormat (CFormatInfo *pCFI)
{
    HRESULT hr = S_OK;
    BOOL fInList = pCFI->_ppf->_cListing.IsInList();
    WORD wLevel = (WORD)pCFI->_ppf->_cListing.GetLevel();

    // BUGBUG -- Setup default indents BEFORE calling ApplyDefaultFormat (lylec)

    // We need to do this first for list elements in order to determine if
    // this is an inside or outside bullet style.  We carefully check before
    // overriding margins or anything else, to see if the properties have already
    // been set by stylesheets, et al.
    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    pCFI->PrepareParaFormat();
    // BUGBUG(paulnel) we need direction added here because ApplyInnerOuterFormat
    // is not applied until later.
    pCFI->_pf()._fRTLInner = pCFI->_pcf->_fRTL;

    // Other tags interfere with level for DLs. This is necessary for Netscape
    // compatibility because <UL><LI><DL> only causes one level of indentation
    // instead of two. Basically, we reset the level for DLs whenever they're
    // the first nested tag under another type of nested list.
    if (ETAG_DL == Tag() && pCFI->_pf()._fResetDLLevel)
        wLevel = 0;

    // Note that we DO need to indent for DLs after the first one.
    if (ETAG_DL != Tag() || wLevel > 0)
    {
        // Don't inherit any numbering attributes.
        pCFI->_pf()._cListing.Reset();

        pCFI->PrepareFancyFormat();
        
        if (!pCFI->_pf().HasRTL(TRUE))
        {
            // Has the left margin already been set by stylesheets?
            if (pCFI->_ff()._cuvMarginLeft.IsNull())
            {
                pCFI->_ff()._cuvMarginLeft.SetPoints(LIST_INDENT_POINTS); // No
                pCFI->_ff()._fHasMargins = TRUE;
            }
        }
        else
        {
            // Has the right margin already been set by stylesheets?
            if (pCFI->_ff()._cuvMarginRight.IsNull())
            {
                pCFI->_ff()._cuvMarginRight.SetPoints(LIST_INDENT_POINTS); // No
                pCFI->_ff()._fHasMargins = TRUE;
            }
        }
        
        if (++wLevel < CListing::MAXLEVELS)
        {
            pCFI->_pf()._cListing.SetLevel( wLevel );
        }

        // Default index style.

        // BUGBUG (cthrash) Obviously, we're ignoring the TYPE attribute
        // of MENUs and DIRs here.  Let this go fine until we resolve the
        // issue of creating CUlistElements for these instead.
        pCFI->_pf()._cListing.SetStyle( FilterHtmlListType( styleListStyleTypeNotSet,
                                                     wLevel) );
    }

    if(Tag() != ETAG_DL)
    {
        // all lists other than DL cause some indent by default. So, if there is
        // an li in the the list, then the bullet is drawn in the indent. For DL
        // there is no indent so do not set offset. This case is handled in
        // MeasureListIndent.
        if (pCFI->_pf()._bListPosition != styleListStylePositionInside)
            pCFI->_pf()._cuvOffsetPoints.SetPoints(LIST_FIRST_REDUCTION_POINTS);
    }

    if (ETAG_DL == Tag())
    {
        // DLs have a level, but our normal mechanism above is short
        // circuited because we've combined it with indentation. More
        // code weirdness for Netscape compatibility.
        if (!wLevel)
        {
            // Paranoid assumption that the maximum allowable levels
            // might actually be zero.
            if (++wLevel < CListing::MAXLEVELS)
            {
                pCFI->_pf()._cListing.SetLevel( wLevel );
            }

            pCFI->_pf()._fResetDLLevel = FALSE;
        }

        // Check to see if the compact flag is set.  If so, set a bit in the para format.
        VARIANT_BOOL fCompact = FALSE;
        IGNORE_HR( DYNCAST(CDListElement, this)->get_PropertyHelper( &fCompact, (PROPERTYDESC *)&s_propdescCDListElementcompact ) );
        pCFI->_pf()._fCompactDL = fCompact;
    }
    else
    {
        pCFI->_pf()._fResetDLLevel = TRUE;
    }

    pCFI->UnprepareForDebug();

    // Spacing is different within lists than without.
    if (fInList)
        ApplyListFormat(this, pCFI);
    else
    {
        pCFI->PrepareFancyFormat();
        ApplyDefaultVerticalSpace(&pCFI->_ff());
        pCFI->UnprepareForDebug();
    }

    pCFI->PrepareParaFormat();

    pCFI->_pf()._cListing.SetInList();

    // set up for potential EMs, ENs, and ES Conversions
    pCFI->_pf()._lFontHeightTwips = pCFI->_pcf->GetHeightInTwips(Doc());
    if (pCFI->_pf()._lFontHeightTwips <=0)
        pCFI->_pf()._lFontHeightTwips = 1;

    pCFI->UnprepareForDebug();

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CListElement::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------

HRESULT
CListElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    if(!fEnd)
        _nOuterListItemIndex = pStreamWrBuff->SetCurListItemIndex(1);
    else
        pStreamWrBuff->SetCurListItemIndex(_nOuterListItemIndex);

    RRETURN(super::Save(pStreamWrBuff, fEnd));
}

/* static */
void CListElement::ApplyListFormat (CElement *pElement, CFormatInfo *pCFI)
{
    if (!pCFI->_pff->_fExplicitTopMargin)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._cuvSpaceBefore.SetValue(0, CUnitValue::UNIT_POINT);
        pCFI->UnprepareForDebug();
    }
    if (!pCFI->_pff->_fExplicitBottomMargin)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._cuvSpaceAfter.SetValue(0, CUnitValue::UNIT_POINT);
        pCFI->UnprepareForDebug();
    }
}


// BUGBUG (cthrash) This code is only necessary because MENU and DIR tags
// don't create CUlistElement as they should.  The only reason I didn't
// change the parser was because there's a bug in the style dropdown which
// prevents the creation of MENU and DIR items (they all become ULs).

//+-----------------------------------------------------------------------
//
//  Member:     FilterHtmlListType()
//
//  Returns:    Return the perferred htmlListType for unordered lists.
//
//------------------------------------------------------------------------

styleListStyleType
CListElement::FilterHtmlListType( styleListStyleType type, WORD wLevel )
{
    // BUGBUG (cthrash) see BUGBUG above.  Nobody should be instantiating
    // CListElements directly.  Assume that only MENU and DIR types do
    // at the moment, so this is cut-and-paste code from CUListElement.

    return ( styleListStyleTypeDisc == type ||
             styleListStyleTypeCircle == type ||
             styleListStyleTypeSquare == type ) ? type :
            ((wLevel == 1) ? styleListStyleTypeDisc :
             (wLevel == 2) ? styleListStyleTypeCircle : styleListStyleTypeSquare );
}

//+-----------------------------------------------------------------------
//
//  Member:     Notify()
//
//  Returns:    Trap exit and enter tree's to invalidate the index caches
//
//------------------------------------------------------------------------
void
CListElement::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    NOTIFYTYPE  ntype = pNF->Type();

    if (ntype == NTYPE_ELEMENT_EXITTREE_1)
    {
        if (!(pNF->DataAsDWORD() & EXITTREE_DESTROY))
        {
            CMarkup *pMarkup = GetMarkup();
            Assert (pMarkup);
            if (!pMarkup)
                goto Cleanup;
            
            CTreeNode *pListNode = pMarkup->FindMyListContainer(GetFirstBranch());
            if (pListNode)
            {
                CListElement *pListElement = DYNCAST(CListElement, pListNode->Element());

                // Invalidate my container so that it has 1 + max of my version and its
                // version. This way we are sure that all my containing LI's will
                // surely be invalid in my container.
                pListElement->_dwVersion = max(_dwVersion, pListElement->_dwVersion) + 1;
            }
        }
    }
    else if (ntype == NTYPE_ELEMENT_ENTERTREE)
    {
        CMarkup *pMarkup = GetMarkup();
        Assert (pMarkup);
        if (!pMarkup)
            goto Cleanup;
        
        CTreeNode *pListNode = pMarkup->FindMyListContainer(GetFirstBranch());
        if (pListNode)
        {
            CListElement *pListElement = DYNCAST(CListElement, pListNode->Element());

            // Update my version number to be the version number of the parent OL + 1
            // so that both, LI's inside me and inside my containing OL are invalidated.
            pListElement->UpdateVersion();
            _dwVersion = pListElement->_dwVersion;
        }
        else
        {
            // If we have _thrown_ an OL around existing LI's then we have to nuke
            // the version numbers of all such LI's since they are invalid now.
            CListItemIterator ci(this, NULL);
            CTreeNode *pNode;
            
            while ((pNode = ci.NextChild()) != NULL)
            {
                CLIElement *pLIElement = DYNCAST(CLIElement, pNode->Element());
                pLIElement->_ivIndex._dwVersion = 0;
            }

            // Finally nuke the OL's version number too!
            _dwVersion = 0;
        }
    }

Cleanup:
    return;
}


static ELEMENT_TAG g_etagChildrenNoRecurse[] = {ETAG_OL, ETAG_UL, ETAG_DL, ETAG_DIR, ETAG_MENU, ETAG_LI};
static ELEMENT_TAG g_etagInterestingChildren[] = {ETAG_LI};
    // Removed CHILDITERATOR_DEEP since USETAGS implies deep and giving both _USETAGS+_DEEP confuses
    // the iterator.
static const DWORD LI_ITERATE_FLAGS=(CHILDITERATOR_USETAGS); // Use the lists to stop recursion
CListItemIterator::CListItemIterator(CListElement *pElementContainer, CElement *pElementStart)
            :CChildIterator(pElementContainer,
                            pElementStart,
                            LI_ITERATE_FLAGS,
                            &g_etagChildrenNoRecurse[0],     // Do NOT recurse into these children
                            sizeof(g_etagChildrenNoRecurse) / sizeof(g_etagChildrenNoRecurse[0]),
                            &g_etagInterestingChildren[0],   // Return all of these kinds of children to me
                            sizeof(g_etagInterestingChildren) / sizeof(g_etagInterestingChildren[0])
                           )
{
}
