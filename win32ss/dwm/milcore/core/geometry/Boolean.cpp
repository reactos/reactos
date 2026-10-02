// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      Constructing new shapes with Boolean operations
//
//  $ENDTAG
//
//  Classes:
//      - COutline and supporting classes CPreFigure and CFigureList
//      - CBoolean and CBooleanClassifier.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//--------------------------------------------------------------------------------------------------

                        // Implementation of CPreFigure

// CPrefigure holds a list of contiguous chains on their way to become a figure.  It points to the
// first and last chain in the list.  When 2 pre-figures get attached, one of them yields its chains
// to the other and goes out of business.  When the first and last pointers hold the same chain, 
// the pre-figure creates a closed figure from its list, adds it to the shape, and goes out of 
// business.

//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::CPreFigure::Initialize
//
//  Synopsis:
//      Initializee with first and last chains
//
//------------------------------------------------------------------------------
void
COutline::CPreFigure::Initialize(
    __inout_ecount(1) CChain *pFirst,
        // The starting chain
    __inout_ecount(1) CChain *pLast)
        // The ending chain
{
    AssumeAsFirst(pFirst);
    AssumeAsLast(pLast);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::CPreFigure::Assume
//
//  Synopsis:
//      Assume the chains owned by another pre-figure
//
//  Notes:
//      This method is invoked when the chains of the other pre-figure are
//      appended trailing the chains of this prefigure
//
//------------------------------------------------------------------------------
void
COutline::CPreFigure::Assume(
    __inout_ecount(1) CPreFigure *pOther)
        // The figure to assume
{
    // Hook up the last chain to its new owner; the remaining chains don't matter
    AssumeAsLast(pOther->m_pLast);
    pOther->m_pFirst = pOther->m_pLast = NULL;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::CPreFigure::AddToShape
//
//  Synopsis:
//      Create a figure from the chains and add it to the shape
//
//------------------------------------------------------------------------------
HRESULT
COutline::CPreFigure::AddToShape(
    __inout_ecount(1) COutline &refOutline)
        // The outline generator
{
    HRESULT hr;
    CChain *pChain;

    Assert(m_pFirst);
    
    // Start a new figure to the shape
    IFC(refOutline.AddOutlineFigure());
    IFC(refOutline.StartFigure(m_pFirst));
    
    // Add the chains to the figure
    for (pChain = m_pFirst;  pChain;  pChain = GetNextChain(pChain))
    {
        IFC(refOutline.AddChainToFigure(pChain));
    }

    // Disown the chains
    m_pFirst = m_pLast = NULL;

Cleanup:
    RRETURN(hr);
}
//--------------------------------------------------------------------------------------------------

                        // Implementation of CFigurePool

// This is the memory pool from which pre-figures are allocated.

//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::CPreFigurePool::AllocatePreFigure
//
//  Synopsis:
//      Allocate a new pre-figure with first and last chains
//
//------------------------------------------------------------------------------
__ecount_opt(1) COutline::CPreFigure *
COutline::CPreFigurePool::AllocatePreFigure(
    __inout_ecount(1) CChain *pFirst,
        // The starting chain
    __inout_ecount(1) CChain *pLast)
        // The ending chain
{
    HRESULT hr = S_OK;
    CPreFigure *pNew = NULL;
    IFC(Allocate(&pNew));
    pNew->Initialize(pFirst, pLast);

Cleanup:
    return pNew;
}
   
//--------------------------------------------------------------------------------------------------

                        // Implementation of COutline

// COutline overrides CScanner::ProcessTheJunction, picking up chains and stringing them together to
// form figures in a shape.  The override of CScanner::ProcessCandidate is a do-nothing stub.

//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::COutline
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
COutline::COutline(
    __inout_ecount_opt(1) IShapeBuilder *pResult,
        // The recepient of the resulting shape
    __in bool fRetrieveCurves,
            // Retrieve curves if true
    __in double rTolerance)
            // Curve retrieval error tolerance
    :CScanner(rTolerance),
     m_pShape(pResult), 
     m_pCurrentFigure(NULL)
{
    m_fCachingCurves = fRetrieveCurves;

    if (fRetrieveCurves)
    {
        m_pfnAddVertex = &COutline::AddVertexWithCurves;
        m_pfnCloseFigure = &COutline::CloseFigureWithCurves;
    }
    else
    {
        m_pfnAddVertex = &COutline::AddVertexSimple;
        m_pfnCloseFigure = &COutline::CloseFigureSimple;
    }

    m_pCurrentCurveVertex = NULL;
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::ProcessTheJunction
//
//  Synopsis:
//      Process the junction - CScanner method override
//
//  Notes:
//      The choice of actions depend on the following factors:
//      * Presence or absence of any head / tail chains
//      * Parity of the head & tail counts (if nonzero)
//      * Whether the area on the left of this junction filled or void 
//
//      The sitation in each case is illustrated in Scanner.doc.  The figure
//      numbers are those of the figures in that document.
//
//------------------------------------------------------------------------------
HRESULT
COutline::ProcessTheJunction()
{
    HRESULT hr = S_OK;
    bool fOdd;
    CChain *pLeftmostHead = m_oJunction.GetLeftmostHead(CHAIN_REDUNDANT_OR_CANCELLED);
    CChain *pRightmostHead = m_oJunction.GetRightmostHead(CHAIN_REDUNDANT_OR_CANCELLED);
    CChain *pLeftmostTail = m_oJunction.GetLeftmostTail(CHAIN_REDUNDANT_OR_CANCELLED);
    CChain *pRightmostTail = m_oJunction.GetRightmostTail(CHAIN_REDUNDANT_OR_CANCELLED);

    if (!pLeftmostHead  &&  !pLeftmostTail)
    {
        // This may happen on a onempty junction, if all its chains are redundant,
        Assert(!pRightmostHead);
        Assert(!pRightmostTail);
        goto Cleanup;
    }

    if (pLeftmostHead) // There is at least one head
    {
        // If there's a left-most, there must be a right-most.
        QUIT_IF_NOT(NULL != pRightmostHead);

        if (!pLeftmostHead->IsSideRight())  // Our left is void
        {
            // Figure 7 or figure 9 - append heads and tails pairwise.  The appending 
            // of the rightmost tail to the rightmost head (Fig 2) will happen inside
            IFC(AppendPairs(pLeftmostHead, pRightmostHead, pLeftmostTail, pRightmostTail));
        }
        else    // the leftmost head is a right chain - our left is filled
        {
            if (pLeftmostTail)  // We have at least one tail
            {
                // Figure 6 or Figure 8 - append the leftmost head to the leftmost tail 
                Append(pLeftmostHead, pLeftmostTail, false);

                IFC(ResetLeft(pLeftmostHead, pRightmostHead)); 
                IFC(ResetLeft(pLeftmostTail, pRightmostTail));
                if (pLeftmostHead  ||  pLeftmostTail)
                {
                    // Pair the remaining heads and/or tails. The extension of the rightmost 
                    // tail with the rightmost head (Fig 5) will happen inside AppendPairs
                    IFC(AppendPairs(pLeftmostHead, pRightmostHead, 
                                    pLeftmostTail, pRightmostTail));
                }
            }
            else // there is no tail, only a bunch of heads (even number, at least 2)
            {
                // Figure 10 - Start a new pre-figure with the leftmost head and the rightmost head
                IFC(StartPreFigure(pLeftmostHead, pRightmostHead));

                // Append the remaining the heads pairwise
                IFC(ResetBoth(pLeftmostHead, pRightmostHead));
                if (pLeftmostHead)
                {
                    // There are more than 2 heads (must be at least 4)
                    // Append the remaining heads pairwise.
                    IFC(AppendHeadPairs(pLeftmostHead, pRightmostHead, fOdd));
                    QUIT_IF_NOT(!fOdd);
                }            
            }   // End if there is no tail
            
        } // End if the leftmost head is a right chain
    }
    else // There is no head, only a bunch of tails.
    {
        // We have checked above that heads & tails are not both null
        Assert(pLeftmostTail);
        Assert(pRightmostTail);
        if (pLeftmostTail->IsSideRight()) // Our left is filled - Figure 11
        {
            // Append the leftmost tail to the rightmost tail
            IFC(AppendTails(pRightmostTail, pLeftmostTail));
            
            IFC(ResetBoth(pLeftmostTail, pRightmostTail));
            if (pLeftmostTail)
            {
                // There are more than 2 tails (must be at least 4)
                // Append the remaining tails pairwise.
                IFC(AppendTailPairs(pLeftmostTail, pRightmostTail, fOdd));
                QUIT_IF_NOT(!fOdd);
            }   
        }
        else // Our left is void
        {
            // Append all the tails pairwise.
            IFC(AppendTailPairs(pLeftmostTail, pRightmostTail, fOdd));
            QUIT_IF_NOT(!fOdd);
        }        
    }   // End if there is no head


Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::ResetLeft
//
//  Synopsis:
//      Set the remaining heads or tails after removing the leftmost one
//
//------------------------------------------------------------------------------
HRESULT
COutline::ResetLeft(
    __inout_ecount(1) CChain *&pLeftmost,
        // The leftmost head/tail, modified here
    __inout_ecount(1) CChain *&pRightmost)
        // The rightmost head/tail, modified here
{
    HRESULT hr = S_OK;

    if (pLeftmost != pRightmost)
    {
        pLeftmost = pLeftmost->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
    }
    else
    {
        pLeftmost = pRightmost = NULL;
    }

    QUIT_IF_NOT((NULL == pLeftmost) == (NULL == pRightmost));
Cleanup:
    return hr;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::ResetBoth
//
//  Synopsis:
//      Set the remaining heads or tails after removing the leftmost and
//      rightmost ones
//
//------------------------------------------------------------------------------
HRESULT
COutline::ResetBoth(
    __inout_ecount(1) CChain *&pLeftmost,
        // The leftmost head/tail, modified here
    __inout_ecount(1) CChain *&pRightmost)
        // The rightmost head/tail, modified here
{
    HRESULT hr = S_OK;
    
    if (pLeftmost->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED) != pRightmost)
    {
        pLeftmost = pLeftmost->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
        if (NULL != pLeftmost)
        {
            pRightmost = pRightmost->GetRelevantLeft(CHAIN_REDUNDANT_OR_CANCELLED);
            QUIT_IF_NOT(pRightmost);
        }
        else
        {
            pLeftmost = pRightmost = NULL;
        }
    }
    else
    {
        pLeftmost = pRightmost = NULL;
    }
Cleanup:
    return hr;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::StartPreFigure
//
//  Synopsis:
//      Start a new segment of the figure with two head chains
//
//------------------------------------------------------------------------------
HRESULT
COutline::StartPreFigure(
    __inout_ecount(1) CChain *pFirst,
        // The first chain
    __inout_ecount(1) CChain *pLast)
        // The last (=second) chain
{
    HRESULT hr = S_OK;
    const CPreFigure *pNew;

    Assert(pFirst);
    Assert(pLast);

    // Starting a pre-figure requires 2 different chains.  If we have only one then we are in an
    // inconsistent state.  Worse than that, the loop that traverses the chains when creating figures
    // assumes null at the end of the list.  If there is only a single chain then it will be linked to
    // itself, and that will cause an infinite loop when creating figures.
    QUIT_IF_NOT(pFirst != pLast)

    IFCOOM(pNew = m_oMem.AllocatePreFigure(pFirst, pLast));
    LinkChainTo(pFirst, pLast);

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AppendPairs
//
//  Synopsis:
//      Append heads and tails pairwise, hooking up the odd remaining head &
//      tail
//
//------------------------------------------------------------------------------
HRESULT
COutline::AppendPairs(
    __inout_ecount_opt(1) CChain *pLeftHead,
        // The leftmost head
    __inout_ecount_opt(1) CChain *pRightHead,
        // The rightmost head
    __inout_ecount_opt(1) CChain *pLeftTail,
        // The leftmost tail
    __inout_ecount_opt(1) CChain *pRightTail)
        // The rightmost tail
{
    HRESULT hr = S_OK;

    bool fOddHeadCount = false;
    bool fOddTailCount = false;

    if (pLeftHead)
    {
        Assert(pRightHead);
        IFC(AppendHeadPairs(pLeftHead, pRightHead, fOddHeadCount));
    }
    
    if (pLeftTail)
    {
        Assert(pRightTail);
        IFC(AppendTailPairs(pLeftTail, pRightTail, fOddTailCount));
    }

    QUIT_IF_NOT(fOddHeadCount == fOddTailCount); // The total count must be even
        
    if (fOddHeadCount)
    {
        // There is one unpaired head and one unpaired tail, attach them together, tail first
        Append(pRightHead, pRightTail, true);
    }
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AppendHeadPairs
//
//  Synopsis:
//      Append heads pairwise
//
//------------------------------------------------------------------------------
HRESULT
COutline::AppendHeadPairs(
    __inout_ecount(1) CChain *pLeftmost,
        // The leftmost head
    __inout_ecount(1) const CChain *pRightmost,
        // The rightmost head
    __out_ecount(1) bool  &fOddCount)
        // True if the number of heads is odd
{
    HRESULT hr = S_OK;
    CChain *pLeft = pLeftmost;

    // Should not be called with null chains
    Assert(pLeftmost);
    Assert(pRightmost); 

    fOddCount = (NULL != pLeftmost);
    while (pLeft != pRightmost)
    {
        Assert(pLeft);                  // Should be a non null left chain
        QUIT_IF_NOT(!pLeft->IsSideRight());
        
        CChain *pRight = pLeft->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
        
        QUIT_IF_NOT(pRight && pRight->IsSideRight());  // Should be a non null right chain
        
        // Append this pair of left and right head chains
        IFC(StartPreFigure(pRight, pLeft));

        if (pRight == pRightmost)    // All chains have been paired
        {
            fOddCount = false;
            break;
        }

        pLeft = pRight->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
        fOddCount = true;
    }
        
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AppendTailPairs
//
//  Synopsis:
//      Append tails pairwise
//
//------------------------------------------------------------------------------
HRESULT
COutline::AppendTailPairs(
    __inout_ecount(1) CChain *pLeftmost,
        // The leftmost tail
    __inout_ecount(1) const CChain *pRightmost,
        // The rightmost head
    __out_ecount(1) bool  &fOddCount)
        // True if the number of heads is odd
{
    HRESULT hr = S_OK;
    CChain *pLeft = pLeftmost;

    // Should not be called with null chains
    Assert(pLeftmost);
    Assert(pRightmost); 

    fOddCount = true;
    
    while (pLeft != pRightmost)
    {
        Assert(pLeft);                        // Should be a non null left chain
        QUIT_IF_NOT(!pLeft->IsSideRight());
        
        CChain *pRight = pLeft->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
        
        QUIT_IF_NOT(pRight && pRight->IsSideRight());  // Should be a non null right chain
        
        // Append this pair of left and right tail chains
        IFC(AppendTails(pLeft, pRight));

        if (pRight == pRightmost)    // All chains have been paired
        {
            fOddCount = false;
            break;
        }

        pLeft = pRight->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
        fOddCount = true;
    }
        
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AppendTails
//
//  Synopsis:
//      String together two tail chains
//
//  Notes:
//      This method should be called only on chains that belong to existing
//      pre-figures. Since every chain is assigned a pre-figure when activated,
//      tt should be safe to assume that on tail chains.
//
//------------------------------------------------------------------------------
HRESULT
COutline::AppendTails(
    __inout_ecount(1) CChain *pLeader,
        // The leading chain
    __inout_ecount(1) CChain *pTrailer)
        // The trailing chain
{
    HRESULT hr = S_OK;
    
    Assert(pLeader);
    Assert(pTrailer);
    
    CPreFigure *pLeaderFigure = GetOwnerOf(pLeader);
    CPreFigure *pTrailerFigure = GetOwnerOf(pTrailer);
    
    QUIT_IF_NOT(pLeaderFigure  &&  pTrailerFigure);

    if (pTrailerFigure == pLeaderFigure)
    {
        // We have closed a figure, so add it to the shape
        IFC(pLeaderFigure->AddToShape(*this));
        IFC(CloseFigure());
    }
    else
    {
        LinkChainTo(pLeader, pTrailer);
        pLeaderFigure->Assume(pTrailerFigure);
    }
    // In any case:
    m_oMem.Free(pTrailerFigure);

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::Append
//
//  Synopsis:
//      Attach a head chain to a tail chain
//
//------------------------------------------------------------------------------
void
COutline::Append(
    __inout_ecount(1) CChain *pHead,
        // The head chain
    __inout_ecount(1) CChain *pTail,
        // The tail chain
    IN bool   fReverse)
        // Append in reverse order if true
{
    Assert(pHead);
    Assert(pTail);    
    Assert(GetOwnerOf(pTail));
    Assert(!GetOwnerOf(pHead));
    
    if (fReverse)
    {
        LinkChainTo(pTail, pHead);
        GetOwnerOf(pTail)->AssumeAsLast(pHead);
    }
    else
    {
        LinkChainTo(pHead, pTail);
        GetOwnerOf(pTail)->AssumeAsFirst(pHead);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AddOutlineFigure
//
//  Synopsis:
//      Add the current figure to m_pShape
//
//------------------------------------------------------------------------------
HRESULT 
COutline::AddOutlineFigure()
{
    return m_pShape->AddNewFigure(m_pCurrentFigure);
}
    
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AddChainToFigure
//
//  Synopsis:
//      Add the vertices of a chain to the current figure
//
//------------------------------------------------------------------------------
HRESULT
COutline::AddChainToFigure(__in_ecount(1) CChain *pChain)
{
    HRESULT hr = S_OK;
    CVertex *pVt;
    
    Assert(pChain);

    //
    // Right chains will be traversed in reverse.
    // m_segmentReversed captures the flow-direction of the result relative to the original direction.
    // It's the combination of the chain IsReversed property and this traversal direction
    //

    if (pChain->IsSideRight())
    {
        m_downwardTraversal = false;
        m_segmentReversed = !pChain->IsReversed();

        // Add the vertices while traversing upwards
        for (pVt = pChain->GetTail()->GetPrevious();  pVt;  pVt = pVt->GetPrevious())
        {
            IFC(AddOutlineVertex(pVt));
        }
    }
    else
    {
        m_downwardTraversal = true;
        m_segmentReversed = pChain->IsReversed();

        // Add the vertices while traversing downwards 
        for (pVt = pChain->GetHead()->GetNext();  pVt;  pVt = pVt->GetNext())
        {
            IFC(AddOutlineVertex(pVt));
        }
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::StartFigure
//
//  Synopsis:
//      Start a new figure in the resulting shape. Currently, both the
//      curve-retrieval and non-curve-retrival paths use this code.
//
//------------------------------------------------------------------------------
HRESULT
COutline::StartFigure(__in_ecount(1) const CChain *pChain)
{
    HRESULT hr = S_OK;
    GpPointR pt;

    if (pChain->IsSideRight())
    {
        // A right chain is traversed backwards
        pt = pChain->GetTailPoint() * m_rInverseScale + m_ptCenter;
    }
    else
    {
        pt = pChain->GetHeadPoint() * m_rInverseScale + m_ptCenter;
    }

    IFC(m_pCurrentFigure->StartAt(
        static_cast<FLOAT>(pt.X),
        static_cast<FLOAT>(pt.Y)));

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AddCurveFragment
//
//  Synopsis:
//      Add the curve fragment to the current figure. This may involve
//      coallescing the fragment on to the end of a pre-existing Bezier.
//
//------------------------------------------------------------------------------
HRESULT
COutline::AddCurveFragment(
    __in_ecount(1) const CBezierFragment *pFragment,
        // The fragment being added.
    __in_ecount(1) const CVertex *pVertex
        // The vertex that terminates that fragment.
    )
{
    HRESULT hr = S_OK;

    Assert(pFragment->Assigned());

    //
    // If there's a pre-existing curve, and it abuts the curve fragment,
    // then simply extend the curve to include the fragment.
    //
    if (!m_curve.TryExtend(*pFragment, !m_segmentReversed))
    {
        //
        // We didn't extend the curve, so output the old one and 
        // start anew.
        //

        if (m_curve.Assigned())
        {
            IFC(FlushCurve());
        }

        m_curve = *pFragment;
        m_curveReversed = m_segmentReversed;
    }

    //
    // After an extension attempt (that either succeeds or fails) the current
    // curve should be in the same direction as our last fragment.
    //
    Assert(m_segmentReversed == m_curveReversed);

    m_pCurrentCurveVertex = pVertex;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::FlushCurve
//
//  Synopsis:
//      Assumes there is a currently active BezierFragment. Constructs the
//      corresponding Bezier and adds it to the current figure.
//
//------------------------------------------------------------------------------
HRESULT
COutline::FlushCurve()
{
    HRESULT hr = S_OK;

    Assert(m_curve.Assigned());

    CBezier bezier;

    bool fNotDegenerate = m_curve.ConstructBezier(&bezier);

    if (fNotDegenerate)
    {
        GpPointR pt1 = bezier.GetControlPoint(1);
        GpPointR pt2 = bezier.GetControlPoint(2);

        if (m_curveReversed)
        {
            GpPointR tmp;
            tmp = pt1; pt1 = pt2; pt2 = tmp;
        }

        IFC(AddCurve(
                pt1,
                pt2,
                m_pCurrentCurveVertex
                ));
    }
    // Else: since it's degenerate we might as well ignore it.

    m_curve.Clear();

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AddVertexSimple
//
//  Synopsis:
//      Add a vertex to the current figure under construction, no curve
//      retrieval
//
//------------------------------------------------------------------------------
HRESULT
COutline::AddVertexSimple(__in_ecount(1) const CVertex *pVertex)
 {
    HRESULT hr;

    GpPointR pt = pVertex->GetPoint() * m_rInverseScale + m_ptCenter;

    Assert(m_pCurrentFigure);
    IFC(m_pCurrentFigure->LineTo(
        static_cast<REAL>(pt.X),
        static_cast<REAL>(pt.Y),
        pVertex->IsSmoothJoin()));

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AddVertexWithCurves
//
//  Synopsis:
//      Add a vertex to the current figure under construction, with curve
//      retrieval
//
//------------------------------------------------------------------------------
HRESULT
COutline::AddVertexWithCurves(__in_ecount(1) const CVertex *pVertex)
{
    HRESULT hr = S_OK;

    //
    // The vertex containing information about the current edge.
    // If we're travelling up a chain, this is *not* the same
    // as pVertex.
    //
    const CVertex *pEdgeVertex = m_downwardTraversal ? pVertex : pVertex->GetNext();

    if (pEdgeVertex->HasCurve())
    {
        IFC(AddCurveFragment(&pEdgeVertex->GetCurve(), pVertex));
    }
    else
    {
        if (m_curve.Assigned())
        {
            IFC(FlushCurve());
        }

        IFC(AddVertexSimple(pVertex));
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::CloseFigureSimple
//
//  Synopsis:
//      Close the current figure under construction
//
//------------------------------------------------------------------------------
HRESULT
COutline::CloseFigureSimple()
{
    HRESULT hr = S_OK;

    IFC(m_pCurrentFigure->Close());
    m_pCurrentFigure = NULL;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::CloseFigureWithCurves
//
//  Synopsis:
//      Close the current figure under construction
//
//------------------------------------------------------------------------------
HRESULT
COutline::CloseFigureWithCurves()
{
    HRESULT hr = S_OK;

    if (m_curve.Assigned())
    {
        IFC(FlushCurve());
    }

    IFC(m_pCurrentFigure->Close());
    m_pCurrentFigure = NULL;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      COutline::AddCurve
//
//  Synopsis:
//      Add a curve segment to the result
//
//------------------------------------------------------------------------------
HRESULT
COutline::AddCurve(
    __in_ecount(1) const GpPointR &controlPoint1,
    __in_ecount(1) const GpPointR &controlPoint2,
    __in_ecount(1) const CVertex *pVertex
    )
{
    HRESULT hr = S_OK;

    Assert(m_pCurrentFigure);

    GpPointR pt = pVertex->GetPoint() * m_rInverseScale + m_ptCenter;

    // Add a curve to the current figure
    IFC(m_pCurrentFigure->BezierTo(
         static_cast<REAL>(controlPoint1.X),
         static_cast<REAL>(controlPoint1.Y),
         static_cast<REAL>(controlPoint2.X),
         static_cast<REAL>(controlPoint2.Y),
         static_cast<REAL>(pt.X),
         static_cast<REAL>(pt.Y),
         pVertex->IsSmoothJoin()));

Cleanup:
    RRETURN(hr);
}
//--------------------------------------------------------------------------------------------------

//                          Implementation of CBooleanClassifier

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBoolean::CBooleanClassifier::CBooleanClassifier
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CBoolean::CBooleanClassifier::CBooleanClassifier(
    IN MilCombineMode::Enum eOperation)   // The Boolean operation
    {
        m_eOperation = eOperation;
        m_pTail[0] = m_pTail[1] = m_pLeft[0] = m_pLeft[1] = NULL;
        m_fIsInside[0] = m_fIsInside[1] = false;
    }

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBoolean::CBooleanClassifier::Classify
//
//  Synopsis:
//      First pass of classifying the heads in the junction.
//
//  Notes:
//      All the junction's head chains are classified here as left, right or
//      redundant.  Classification is based on the fill rule (Alternate/winding)
//      and Boolean operation, ignoring redundancy that stems from Boolean
//      operation.
//
//------------------------------------------------------------------------------
void
CBoolean::CBooleanClassifier::Classify(
    __inout_ecount(1) CChain *pLeftmostTail,
        // The junction's leftmost tail
    __inout_ecount(1) CChain *pLeftmostHead,
        // The junction's leftmost head
    __inout_ecount_opt(1) CChain *pLeft)
    // The chain to the left of the junction (possibly NULL)
{        
    Assert(pLeftmostHead);    // Shouldn't be called oterhwise

    CChain *pChain;

    // Identify the first tail in each shape, looking right
    m_pTail[0] = pLeftmostTail;
    while (m_pTail[0]  &&  (1 == m_pTail[0]->GetShape()))
    {
        m_pTail[0] = m_pTail[0]->GetRight();
    }

    m_pTail[1] = pLeftmostTail;
    while (m_pTail[1]  &&  (0 == m_pTail[1]->GetShape()))
    {
        m_pTail[1] = m_pTail[1]->GetRight();
    }

    // Identify the first (possibly redundant) chain left of the junction in each shape
    m_pLeft[0] = pLeft;
    while (m_pLeft[0]  &&  (1 == m_pLeft[0]->GetShape()))
    {
        m_pLeft[0] = m_pLeft[0]->GetLeft();
    }

    m_pLeft[1] = pLeft;
    while (m_pLeft[1]  &&  (0 == m_pLeft[1]->GetShape()))
    {
        m_pLeft[1] = m_pLeft[1]->GetLeft();
    }

    // Figure out where we are relative to both shapes
    m_fIsInside[0] = (m_pLeft[0]  &&  !m_pLeft[0]->IsSelfSideRight());
    m_fIsInside[1] = (m_pLeft[1]  &&  !m_pLeft[1]->IsSelfSideRight());

    // Traverse the junction's heads
    for (pChain = pLeftmostHead;   pChain;   pChain = pChain->GetRight())
    {
        ClassifyChain(pChain);
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CBoolean::CBooleanClassifier::ClassifyChain
//
//  Synopsis:
//      Classify one head-chain in the junction.
//
//------------------------------------------------------------------------------
void
CBoolean::CBooleanClassifier::ClassifyChain(
    __inout_ecount(1) CChain *pChain)
        // The chain being classified
{        

    UINT uiWhich = pChain->GetShape();
    Assert((0 == uiWhich)  ||  (1 == uiWhich));
    UINT uiOther = 1 - uiWhich;


    // Classify the chain in its own shape
    if (m_pTail[uiWhich])
    {
        // This is the first head chain belonging to this Shape, and we have a tail that
        // belongs to the same shape, so key off that to classify the chain
        pChain->Continue(m_pTail[uiWhich]);
        m_pTail[uiWhich] = NULL;
    }
    else
    {
        // Key off the previous chain to our left (or its absence)
        pChain->Classify(m_pLeft[uiWhich]);
    }
    
    m_fIsInside[uiWhich] = !pChain->IsSelfSideRight();
    m_pLeft[uiWhich] = pChain;

    // Mark Boolean operation redundancy
    if (!pChain->IsSelfRedundant())
    {
        switch (m_eOperation)
        {
        case MilCombineMode::Union:
            if (m_fIsInside[uiOther])
            {
                pChain->SetBoolRedundant();
            }
            break;

        case MilCombineMode::Intersect:
            if (!m_fIsInside[uiOther])
            {
                pChain->SetBoolRedundant();
            }
            break;

        // Subtract and Xor may also flip left/right side

        case MilCombineMode::Exclude:
            if (0 == uiWhich)
            {
                if (m_fIsInside[1])
                {
                    pChain->SetBoolRedundant();
                }
            }
            else if(1 == uiWhich)// uiWhich = 1
            {
                if (m_fIsInside[0])
                {
                    pChain->FlipBoolSide();
                }
                else
                {
                    pChain->SetBoolRedundant();
                }
            }
            break;

        case MilCombineMode::Xor:
            if (m_fIsInside[uiOther])
            {
                pChain->FlipBoolSide();
            }
            break;

        default:
            Assert(false);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------

                        // Implementation of CRelation

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRelation::CRelation
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CRelation::CRelation(double rTolerance)
    :CBoolean(NULL, MilCombineMode::Intersect, false, rTolerance),
     m_eResult(MilPathsRelation::Unknown)
{
    m_fInside[0] =  m_fInside[1] = false;
    m_fOutside[0] = m_fOutside[1] = false;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CRelation::ProcessTheJunction
//
//  Synopsis:
//      Process the junction - CScanner method override
//
//  Notes:
//      This methods updates the result of detecting the location of edges of
//      one shape relative to the other.
//
//------------------------------------------------------------------------------
HRESULT
CRelation::ProcessTheJunction()
{
    HRESULT hr = S_OK;

    CChain *pLeftmostHead = m_oJunction.GetLeftmostHead(CHAIN_SELF_REDUNDANT);
    CChain *pChain = pLeftmostHead;

    while (pChain)
    {
        // At this stage the head chains of this junction have been classified for the
        // Intersection Boolean operation.  A chain is therefore BoolRedundant
        // if and only if it lies outside the other shape.

        Assert((0 == pChain->GetShape())  ||  (1 == pChain->GetShape()));
        
        if (pChain->IsBoolRedundant())
        {
            // This chain lies outside the other shape
            m_fOutside[pChain->GetShape()] = true;
        }
        else
        {
            // This chain lies inside the other shape
            m_fInside[pChain->GetShape()] = true;
        }

        // See if we can early out
        if ((m_fInside[0]  &&  m_fOutside[0])  ||  (m_fInside[1]  &&  m_fOutside[1]))
        {
            m_eResult = MilPathsRelation::Overlap;
            m_fDone = true;
            break;
        }

        pChain = pChain->GetRelevantRight(CHAIN_SELF_REDUNDANT);
    }

    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CRelation::GetResult
//
//  Synopsis:
//      Get the result of detecting the relation between shapes
//
//  Notes:
//      The decision is based on the presence of edges of one shape inside the
//      other, which have been tallied when we scanned the shapes.
//
//------------------------------------------------------------------------------
MilPathsRelation::Enum
CRelation::GetResult()
{
    if (MilPathsRelation::Overlap == m_eResult)
    {
        // We have already detected an overlap, we're done
        goto exit;
    }

    if (m_fInside[0])
    {
        // Shape[0] has some edges inside.  If it had any edges outside then the result would have
        // been set earlier to Overlap, and we wouldn't be here.  so all the edges of Shape[0] are
        // inside Shape[1], hence:
        Assert(!m_fOutside[0]);
        m_eResult = MilPathsRelation::IsContained;
    }
    else if (m_fInside[1])
    {
        // Shape[1] has some edges inside.  If it had any edges outside then the result would have
        // been set earlier to Overlap, and we wouldn't be here.  so all the edges of Shape[1] are
        // inside Shape[0], hence:
        Assert(!m_fOutside[1]);
        m_eResult = MilPathsRelation::Contains;
    }
    else
    {
        // No shape contains any edge of the other, so they are disjoint
        m_eResult = MilPathsRelation::Disjoint;
    }

exit:
    return m_eResult;
}
#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CRelation::Dump
//
//  Synopsis:
//      Dump the current state
//
//------------------------------------------------------------------------------
void
CRelation::Dump()
{
    MILDebugOutput(L"m_fInside=%d, %d.  m_fOutside=%d, %d\n",
                   m_fInside[0],  m_fInside[1], 
                   m_fOutside[0], m_fOutside[1]);

    DumpRelation(m_eResult);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      DumpRelation
//
//  Synopsis:
//      Dump a relation enum
//
//------------------------------------------------------------------------------
void
DumpRelation(MilPathsRelation::Enum eResult)
{
    switch (eResult)
    {
    case MilPathsRelation::Unknown:
        MILDebugOutput(L"UnKnown\n");
        break;

    case MilPathsRelation::Overlap:
        MILDebugOutput(L"Overlap\n");
        break;

    case MilPathsRelation::Contains:
        MILDebugOutput(L"Contains\n");
        break;

    case MilPathsRelation::IsContained:
        MILDebugOutput(L"IsContained\n");
        break;

    case MilPathsRelation::Disjoint:
        MILDebugOutput(L"Disjoint\n");
        break;

    default:
        MILDebugOutput(L"Problem!!!\n");
        break;
    }
}
#endif // DBG


