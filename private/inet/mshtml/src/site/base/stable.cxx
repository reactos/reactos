//+---------------------------------------------------------------------------
//
//  Microsoft IE4/Trident
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       stable.cxx
//
//  Contents:   Implementation of validity/stablity of the tree
//
//  Classes:    CMarkup
//
//  Author:     alexa
//
//----------------------------------------------------------------------------

#include <headers.hxx>

// Right now we don't use the stability code.  I'm sure we will need
// this eventually.
#ifdef MARKUP_STABLE

#ifndef X_ELEMENT_HXX
#define X_ELEMENT_HXX
#include "element.hxx"
#endif

#ifndef X_TREEPOS_HXX
#define X_TREEPOS_HXX
#include "treepos.hxx"
#endif

#if DBG == 1
    char * gs_unstable[] = 
        {
            "No violation - tree/html is stable",
            "Violation of NESTED containers rule",
            "Violation of TEXTSCOPE rule",
            "Violation of OVERLAPING tags rule",
            "Violation of MASKING container rule",
            "Violation of PROHIBITED container rule",
            "Violation of REQUIRED container rule",
            "Violation of IMPLICITCHILD rule",
            "Violation of LITERALTAG rule",
            "Violation of TREE rules",
            "Vialotion of EMPTY TAG rule",
            "Vialotion of the tree pointer rule",
            "Can not determine if stable or not due to the other probelms (OUT_OF_MEMEORY)"
        };
#endif

MtDefine(Stability_aryScopeNodes_pv, Locals, "CRootSite::ValidateParserRules aryScopeNodes::_pv")

//+---------------------------------------------------------------------------
//
//  Member:     CRootSite::IsStable
//
//  Synopsis:   returns TRUE if tree is stable.
//
//----------------------------------------------------------------------------

BOOL CMarkup::IsStable()
{
    if (_fUnstable || Doc()->_lTreeVersion != _lStableTreeVersion)
    {
        // 1. do we have a dirty range/for now do the whole tree
        // 2. Walk the subtree of the dirty range and determine if the tree is stable
        _fUnstable = ValidateParserRules() != UNSTABLE_NOT;
    }
    if (!_fUnstable)
    {
        UpdateStableTreeVersionNumber();
    }
    return !_fUnstable;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootSite::IsStable
//
//  Synopsis:   Returns TRUE if tree is stable.
//
//----------------------------------------------------------------------------

HRESULT CMarkup::MakeItStable()
{
    HRESULT hr = S_OK;

    if (_fUnstable)
    {
    }

    _fUnstable = FALSE;
    UpdateStableTreeVersionNumber();

    RRETURN (hr);
}

#if WILL_NEED_THIS_LATER_ON
//+---------------------------------------------------------------------------
//
//  Member:     CElement::CanPerformOMOperation
//
//  Synopsis:   Returns TRUE if OM operation can be performed on the element
//
//  Use:        This function should be called at the BEGINING of every OM 
//              operation which results in a modified tree.
//              At the END of such an operation UpdateStableTreeVersionNumber()
//              should be called.
//
//----------------------------------------------------------------------------

BOOL Element::CanPerformOMOperation()
{
    CRootSite *pRoot = SearchBranchToRootForTag(ETAG_ROOT); // BUGBUG: (alexa) this is extreemly not optimal
                                                            // we need to do : Doc()->_activeTree (or something like that).
    return GetMarkup()->GetLoaded() && pRoot->IsStable();
}
#endif WILL_NEED_THIS_LATER_ON


//+---------------------------------------------------------------------------
//
//  Member:     CRootSite::UpdateStableTreeVersionNumber
//
//  Synopsis:   Update stable tree version number with the new tree version number
//
//+---------------------------------------------------------------------------
void    
CMarkup::UpdateStableTreeVersionNumber()
{
    _lStableTreeVersion = Doc()? Doc()->_lTreeVersion : 1; 
    return; 
}


//+---------------------------------------------------------------------------
//
//  Member:     CRootSite::ValidateParserRules
//
//  Synopsis:   Validate html and tree rules for each element of the tree.
//
//+---------------------------------------------------------------------------

UNSTABLE_CODE 
CMarkup::ValidateParserRules()
{
    CStackPtrAry < CTreeNode *, 8 > aryScopeNodes(Mt(Stability_aryScopeNodes_pv));
    UNSTABLE_CODE   iReturnCode = UNSTABLE_NOT;
    HRESULT         hr = S_OK;
    long            iRun = 0;
    long            cpTotal = 0;
    long            cch = 0;
    CTreeNode     * pNode = NULL;
    CTreeNode     * pNodei;
    CTreeNode     * pNodeConflict = NULL;
    CTreeNode     * pNodeOverlap = NULL;// overlaping node (the first of the 2 overlaping nodes)
    CHtmlParseClass * phpc;
    CHtmlParseClass * phpcOverlap=NULL; // phpc of the overlaping node
    const CTagDesc  * ptd;
    int             i;
    int             cInclusion = 0;     // depth of inclusion (overalping case)
    BOOL            fLiteralTag = FALSE;
    BOOL            fEmptyTag = FALSE;
    ELEMENT_TAG     etagImplicitChild = ETAG_NULL;
    ELEMENT_TAG     etagCurNode;        // tag of the current node
    BOOL            fNeedToValidateTextScope;
    BOOL            fNeedToValidateMasking;
    BOOL            fNeedToValidateProhibited;
    BOOL            fNeedToValidateRequired;
    BOOL            fNeedToCheckBeginContainer;

    // We need to perform the following checking of each element of the tree
    //+---------------------------------------------------------------------------
    // 0. Tree should be stable
    //+---------------------------------------------------------------------------
    // 1. Check if the NESTED RULE is preserved.
    // IF   X is a NESTED element (_scope field of the parseclass is == SCOPE_NESTED)
    //      and Y is NESTED element
    // THEN if there is a position P, where P->X->Y, then for all the other positions
    //      if P'->X it should be also P'->X->Y
    //+---------------------------------------------------------------------------
    // 2. Check if TEXTSCOPE RULE is preserved
    // IF   T is a text (_textScope) or T is textLike (_fTextLike is TRUE)
    // THEN there should be a parent  element X with TEXTSCOPE_INCLUDE
    //      AND all the Y elements between the T and X should be TEXTSCOPE_NEUTRAL
    // ex: T->N->N->I is legal; T->N->E->I is illegal
    //+---------------------------------------------------------------------------
    // 3. Check the ENDCONTAINER RULE is preserved (NO OVERLAPPING relationship between 2 tags)
    // IF   Y is an endcontainer of X
    // THEN if X is inside Y then Y should enclose X completely
    //      else if Y is inside X then X should enclose Y completely
    // ex: <TD><B>...</B></TD> - legal; <TD><B>....</TD></B> - is illegal
    // ex: <B><TD>...</TD></B> - legal; <B><TD>....</B></TD> - is illegal
    //+---------------------------------------------------------------------------
    // 4. Check for MASKING rule.
    // IF   X is masked by Y
    // THEN if X->Y then it should be a B in between X->B->Y , 
    //      where B is a BEGIN container of X (X has to be protected by B)
    // EX: X->(NOT B)->Y is illegal (Y shouldn't be a masking container)
    //+---------------------------------------------------------------------------
    // 5: Check for PROHIBITED containers RULE
    // IF   X has a prohibited container Y
    // THEN X->Y means that there is another container in between X->B->Y
    // EX: <P><P> - is illegal because X->NOT B->Y is illegal , <P><TD><P> - ???
    //+---------------------------------------------------------------------------
    // 6. Check for REQUIRED container
    // IF X has a begin container B (so X->...->B
    // THEN should exists Y (required container of X) in between
    // X->Y->B(or ROOT)
    // EX: TR->TBODY(THEAD,TFOOT)->TABLE
    //+---------------------------------------------------------------------------
    // 7. Check for IMPLICIT CHILD rule.
    // IF   X has implicit child Y
    // THEN the first immediate child of X should be Y
    // ex: HTML-(has IC)->HEAD-(has IC)->TITLE
    //+---------------------------------------------------------------------------
    // 8. Check for LITERALTAG rule
    // IF   X is a literal tag
    // THEN only a TEXT should be it's child
    // ex: <IMG>-><LITERALTAG> - is illegal
    //+---------------------------------------------------------------------------

  if (GetElementClient()->Tag() != ETAG_BODY)   // BUGBUG: remove this when ericvas to checkin.
  {
    goto Conflict;
  }
  else
  {
    CTreePosGap     tpgWalker (FirstTreePos(), TPG_RIGHT, TPG_LEFT);

    tpgWalker.SetMoveDirection(TPG_RIGHT);
    do 
    {
        CTreePos *ptp = tpgWalker.AdjacentTreePos(TPG_LEFT);
        switch( ptp->Type() )
        {
        case CTreePos::NodeBeg:
        case CTreePos::NodeEnd:
            pNode = ptp->Branch();
            if(ptp->IsBeginNode())
            {
                // this is begin node

                // VALIDATE RULE #0: Tree should be stable
                if (!(aryScopeNodes.Size() == 0 || pNode->Parent() == aryScopeNodes[aryScopeNodes.Size() - 1]))
                {
                    Assert (FALSE && "The tree is totaly not safe");
                    iReturnCode = UNSTABLE_TREE;
                    goto Conflict;
                }

                etagCurNode = pNode->Tag();
                phpc = HpcFromEtag(etagCurNode);   // get a parser descriptor of the TAG

                fNeedToValidateTextScope  = (phpc->_texttype == TEXTTYPE_ALWAYS);
                fNeedToValidateMasking = phpc->_atagMaskingContainers != NULL;
                fNeedToValidateProhibited = phpc->_atagProhibitedContainers != NULL;
                fNeedToValidateRequired = phpc->_atagRequiredContainers != NULL;

                for ( pNodei = pNode->Parent();
                      (fNeedToValidateTextScope || fNeedToValidateMasking || fNeedToValidateProhibited || fNeedToValidateRequired) && pNodei;
                      pNodei = pNodei->Parent())                      
                {
                    fNeedToCheckBeginContainer = FALSE;
                    if (fNeedToValidateTextScope)
                    {
                        // VALIDATE RULE #2. Check if TEXTSCOPE RULE is preserved
                        CHtmlParseClass *phpci = HpcFromEtag(pNodei->Tag());
                        if (phpci->_textscope == TEXTSCOPE_INCLUDE)
                        {
                            fNeedToValidateTextScope = FALSE;   // means TEXTSCOPE is valid, there is an TEXTSCOPE_INCLUDE tag above
                        }
                        if (phpci->_textscope == TEXTSCOPE_EXCLUDE)
                        {
                            iReturnCode = UNSTABLE_TEXTSCOPE;
                            goto Conflict;
                        }
                    }
                    if (fNeedToValidateMasking)
                    {
                        // VALIDATE RULE #4: MASKING CONTAINERS RULE
                        if (IsEtagInSet(pNodei->Tag(), phpc->_atagMaskingContainers))
                        {
                            iReturnCode = UNSTABLE_MASKING;
                            pNodeConflict = pNodei;
                            goto Conflict;
                        }
                        fNeedToCheckBeginContainer = TRUE;
                    }
                    if (fNeedToValidateProhibited)
                    {
                        // VALIDATE RULE #5: PROHIBITED CONTAINERS RULE
                        if (IsEtagInSet(pNodei->Tag(), phpc->_atagProhibitedContainers))
                        {
                            iReturnCode = UNSTABLE_PROHIBITED;
                            pNodeConflict = pNodei;
                            goto Conflict;
                        }
                        fNeedToCheckBeginContainer = TRUE;
                    }
                    if (fNeedToValidateRequired)
                    {
                        // VALIDATE RULE #6: REQUIRED CONTAINERS RULE
                        if (IsEtagInSet(pNodei->Tag(), phpc->_atagRequiredContainers))
                        {
                            fNeedToValidateRequired = FALSE;// means REQUIRED rule is validated, there is a required container above
                        }
                        else
                        {
                            fNeedToCheckBeginContainer = TRUE;
                        }
                    }

                    if (fNeedToCheckBeginContainer && IsEtagInSet(pNodei->Tag(), phpc->_atagBeginContainers))
                    {
                        fNeedToValidateMasking = FALSE;     // means MASKING rule is valid, there is a BEGIN container in between
                        fNeedToValidateProhibited = FALSE;  // means PROHIBITED rule is valid, there is a BEGIN container in between
                        if (fNeedToValidateRequired)
                        {
                            iReturnCode = UNSTABLE_REQUIRED;
                            goto Conflict;
                        }
                    }
                }
                if (fNeedToValidateRequired)    // if still we need the required container after searching all the parenets, then
                {
                    iReturnCode = UNSTABLE_REQUIRED;
                    goto Conflict;
                }

                // VALIDATE RULE #7. IMPLICIT CHILD rule.
                if (etagImplicitChild && etagImplicitChild != etagCurNode)
                {
                    iReturnCode = UNSTABLE_IMPLICITCHILD;
                    goto Conflict;
                }

                etagImplicitChild = phpc->_etagImplicitChild;

                // VALIDATE RULE #8. Check for LITERALTAG rule
                if (fLiteralTag)
                {
                    iReturnCode = UNSTABLE_LITERALTAG;
                    goto Conflict;
                }
                ptd = TagDescFromEtag(pNode->Tag());
                fLiteralTag = ptd->HasFlag(TAGDESC_LITERALTAG) && 
                              etagCurNode != ETAG_GENERIC;  // generic tag can be both literal and not (alexz);

                // VALIDATE RULE #9: Empty tags
				if (fEmptyTag)
				{
					iReturnCode = UNSTABLE_EMPTYTAG;
					goto Conflict;
				}

                fEmptyTag = phpc->_scope == SCOPE_EMPTY;

                // this is a new element coming into scope. PUSH it to the aryScopeNodes (stack)
                hr = THR( aryScopeNodes.Append( pNode ) );
                if (hr)
                {
                    iReturnCode = UNSTABLE_CANTDETERMINE;
                    goto Conflict;
                }
                if (!ptp->IsEdgeScope())
                {
                    // this is the proxied branch (overlapping case)
                    Assert (cInclusion);
                    Assert (pNodeOverlap);

                    // VALIDATE RULE #3. (ILLEGAL OVERLAPPING relationship between 2 tags)
                    if (IsEtagInSet(etagCurNode, phpcOverlap->_atagEndContainers) ||
                        IsEtagInSet(pNodeOverlap->Tag(), phpc->_atagEndContainers))
                    {
                        iReturnCode = UNSTABLE_OVERLAPING;
                        goto Conflict;
                    }

                    // VALIDATE RULE #1. Check if the NESTED RULE is preserved.
                    if (phpcOverlap->_scope == SCOPE_NESTED && phpc->_scope == SCOPE_NESTED)
                    {
                        iReturnCode = UNSTABLE_NESTED;
						goto Conflict;
                    }

                    cInclusion--;
                    if (!cInclusion)
                    {
                        pNodeOverlap = NULL;
                        phpcOverlap = NULL;
                    }
                }
            }
            else
            {
                // this is the end node

				if (pNode != aryScopeNodes[aryScopeNodes.Size() - 1])
				{
					iReturnCode = UNSTABLE_TREE;
					goto Conflict;
				}


                if (ptp->IsEdgeScope())
                {
                    // this is the real end of the scope
                    if (cInclusion)
                    {
						Assert (pNodeOverlap == NULL);
                        pNodeOverlap = aryScopeNodes[aryScopeNodes.Size() - 1];
                        phpcOverlap = HpcFromEtag(pNodeOverlap->Tag());
                    }
                }
                else
                {
                    // this is temproary end of the scope (overlapping case)
					if (fLiteralTag)
					{
						iReturnCode = UNSTABLE_LITERALTAG;
						goto Conflict;
					}
					if (fEmptyTag)
					{
						iReturnCode = UNSTABLE_EMPTYTAG;
						goto Conflict;
					}

                    cInclusion++;
					Assert (pNodeOverlap == NULL);
                }

                fLiteralTag = FALSE;
				fEmptyTag = FALSE;

                // this element going out of scope. POP it from the aryScopeNodes (stack)
                aryScopeNodes.Delete(aryScopeNodes.Size() - 1);
            }
            break;
        case CTreePos::Text:
            cch = ptp->Cch();

            // VALIDATE RULE #2. Check if TEXTSCOPE RULE is preserved
            for ( i = aryScopeNodes.Size() - 1 ; cch && i >= 0 ; i-- )
            {
                pNode = aryScopeNodes[ i ];
                phpc = HpcFromEtag(pNode->Tag());
                if (phpc->_textscope == TEXTSCOPE_INCLUDE)
                    break;
                if (phpc->_textscope == TEXTSCOPE_EXCLUDE)
                {
                    iReturnCode = UNSTABLE_TEXTSCOPE;
                    goto Conflict;
                }
            }

            cpTotal += cch;
            iRun++;
            break;
        case CTreePos::Pointer:
            // make sure the tree pointer is not inside inclusion
			if (cInclusion)
			{
				Assert (0 && "We have an illegal tree pointer");
				iReturnCode = UNSTABLE_MARKUPPOINTER;
				goto Conflict;
			}
            break;
        }

        hr = THR_NOTRACE( tpgWalker.Move() );
    }
    while (!hr);

    if (cInclusion)
    {
        Assert (FALSE && "The tree is totaly not safe");
        iReturnCode = UNSTABLE_TREE;
        goto Conflict;
    }

    // BUGBUG: we don't walk past the last element for now, it will change with ericvas checkin
    Assert (aryScopeNodes.Size() == 1 && aryScopeNodes[0]->Tag() == ETAG_BODY);

  }

Conflict:
#ifdef NEVER
#if DBG == 1
    if (iReturnCode)
    {
        AssertSz(0, gs_unstable[iReturnCode]);
    }
#endif
#endif
    return iReturnCode;

}

#endif //MARKUP_STABLE
