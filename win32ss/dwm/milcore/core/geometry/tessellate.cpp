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
//      Tessellating a shape into triangles
//
//  $ENDTAG
//
//  Classes:
//      CTessellator, CVertexRef, CVertexRefPool
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#ifdef DBG
    bool g_fTesselatorTrace = false;
    #define TESSELLATOR_VALIDATE_COINCIDENCE(p,q) (p)->CoincidesWith(q)
#else
    #define TESSELLATOR_VALIDATE_COINCIDENCE(p,q)
#endif

    

inline bool 
IsLeftTurn(
    __in_ecount(1) const GpPointR &A,
        // First point
    __in_ecount(1) const GpPointR &B,
        // Second point
    __in_ecount(1) const GpPointR &C)
        // Third point
{
    return Determinant(B - A, C - B) > 0;
}

//--------------------------------------------------------------------------------------------------

//  Implementation of  CVertexRef
    
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CVertexRef::operator =
//
//  Synopsis:
//      Assignment operator
//
//------------------------------------------------------------------------------
void
CTessellator::CVertexRef::operator = (
    __in_ecount(1) const CVertexRef &other)
        // The other object to copy
{
    m_pVertex = other.m_pVertex;
    m_pLeft = other.m_pLeft;
    m_pRight = other.m_pRight;
    m_wIndex = other.m_wIndex;

    Assert(m_pVertex);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CVertexRef::Initialize
//
//  Synopsis:
//      Initialize a newly allocated vertexRef
//
//------------------------------------------------------------------------------
void 
CTessellator::CVertexRef::Initialize(
    __in_ecount(1) const CVertex *pVertex,
        // The vertex to initialize to
    IN WORD wIndex)
        // Trianguation vertex index
{
    Assert(pVertex);

    m_pVertex = pVertex;
    m_pLeft = m_pRight = NULL;
    m_wIndex = wIndex;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CVertexRef::LinkTo
//
//  Synopsis:
//      Link to a vertex on the right
//
//------------------------------------------------------------------------------
void 
CTessellator::CVertexRef::LinkTo(
    __inout_ecount_opt(1) CVertexRef  *pRight)
        // The vertex on the right to link to (NULL OK)
{
    Assert(pRight != this);  // Don't link a vertex ref to itself!

    m_pRight = pRight;

    if (pRight)
    {
        pRight->m_pLeft = this;
    }
}
#ifdef DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CVertexRef::AssertNoLeftDuplicate
//
//  Synopsis:
//      Verify that this vertex is not duplicated on its left
//
//------------------------------------------------------------------------------
void
CTessellator::CVertexRef::AssertNoLeftDuplicate()
{
    for (CVertexRef *pvr = m_pLeft;  pvr;  pvr = pvr->m_pLeft)
    {
        Assert(pvr != this);
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CVertexRef::AssertNoRightDuplicate
//
//  Synopsis:
//      Verify that this vertex is not duplicated on its right
//
//------------------------------------------------------------------------------
void
CTessellator::CVertexRef::AssertNoRightDuplicate()
{
    for (CVertexRef *pvr = m_pRight;  pvr;  pvr = pvr->m_pRight)
    {
        Assert(pvr != this);
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CVertexRef::Dump
//
//  Synopsis:
//      Debug dump
//
//------------------------------------------------------------------------------
void
CTessellator::CVertexRef::Dump() const
{
    MILDebugOutput(L"id=%d", m_id);
    MILDebugOutput(L" Point: (%f, %f)",  m_pVertex->GetPoint().X, m_pVertex->GetPoint().Y);
    MILDebugOutput(L"\n");
}
#endif
//--------------------------------------------------------------------------------------------------

//  Implementation of  CVertexRefPool
    
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CVertexRefPool::AllocateVertexRef
//
//  Synopsis:
//      Allocate a new vertex at a given point
//
//  Return:
//      A pointer to the new vertex, NULL if allocation failed
//
//  Notes:
//
//------------------------------------------------------------------------------
__out_ecount(1) CTessellator::CVertexRef *
CTessellator::CVertexRefPool::AllocateVertexRef(
    __in_ecount(1) const CVertex *pVertex,
        // The vertex to reference
    IN WORD wIndex)
        // Index of triangulation vertex
{    
    Assert(pVertex);

    HRESULT hr = S_OK;
    CTessellator::CVertexRef *pNew = NULL;
    IFC(Allocate(&pNew));

    pNew->Initialize(pVertex, wIndex);

#ifdef DBG
    pNew->m_id = m_id++;
#endif

Cleanup:
    return pNew;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CVertexRef::Split
//
//  Synopsis:
//      Split the ceiling at a given vertex ref
//
//  Return:
//      A pointer to the leftmost vertex of the right piece, NULL if allocation
//      failed
//
//  Notes:
//      This vertex will remain the rightmost vertex of the left piece
//
//------------------------------------------------------------------------------
__out_ecount_opt(1) CTessellator::CVertexRef *
CTessellator::CVertexRef::Split(
    __inout_ecount(1) CVertexRefPool &mem)
        // The memory pool to allocate from
{
    HRESULT hr = S_OK;
    CVertexRef *pNew = NULL;
    
    IFC(mem.Allocate(&pNew));

    Assert(m_pVertex);
    pNew->m_pVertex = m_pVertex;
    pNew->m_wIndex = m_wIndex;

    // Assume the right link
    pNew->LinkTo(m_pRight);
    
    // Sever the chain
    pNew->m_pLeft = NULL;
    m_pRight = NULL;

#ifdef DBG
    pNew->m_id = mem.m_id++;
#endif

Cleanup:
    return pNew;
}

//--------------------------------------------------------------------------------------------------
//
//  Implementation of  CTessellator
//
//--------------------------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::ProcessTheJunction
//
//  Synopsis:
//      Adjust the band structure with the heads and tails at the current
//      junction
//
//  Notes:
//      This method is at the very core of the tessellation algorithm. It is
//      very difficult to understand without Scanner.doc - the illustrated
//      document that describes the algorithm.  The various configurations are
//      illustrated in the figures in that document; these figures are
//      referenced in the comments.
//
//      We are at a junction which is the common tail of some chains and the
//      common head of some other chains.  Depending on whether the junction
//      lies in the fill set or not, and on the numbers of head and tail chains,
//      various actions are taken that modify the band structure of the fill
//      set.  New bands may be formed with pairs of head chains, existing bands
//      may be merged or split, and new head chains may assume the roles of
//      existing tails in these bands.
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::ProcessTheJunction()
{
    HRESULT hr = S_OK;
    WORD wIndex;
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

    IFC(AddVertex(m_oJunction.GetPoint(), wIndex));

    if (pLeftmostTail)
    {
        // If there's a left-most, there must be a right-most.
        QUIT_IF_NOT(NULL != pRightmostTail);

        // Process all the tails in the junction
        IFC(ProcessAllTails(wIndex, pLeftmostTail, pRightmostTail));
    }
    
    if (pLeftmostHead) // There is at least one head
    {
        CChain *pFrom = NULL;
        CChain *pTo = NULL;

        if (!pLeftmostHead->IsSideRight())  // The leftmost head is a left chain
        {
            // Figure 7 or figure 9 - we'll pair all the heads into bands in CreateBands below.
            // The extension of the rightmost tail (Fig 4) will happen inside CreateBands
            pFrom = pLeftmostHead;
            pTo = pRightmostHead;
        }
        else    // the leftmost head is a right chain
        {
            if (pLeftmostTail)  // The junction has at least one tail
            {
                // Figure 6 or Figure 8 - extend the leftmost tail with the leftmost head 
                // The extension of the rightmost tail (Fig 5) will happen inside CreateBands
                pLeftmostHead->AssumeTask(pLeftmostTail);

                if (pLeftmostHead != pRightmostHead)
                {
                    // We'll pair the rest of the heads into bands in CreateBands below.
                    pFrom = pLeftmostHead->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
                    pTo = pRightmostHead;
                }
            }
            else // there is no tail, only a bunch of heads (even number, at least 2)
            {
                // Figure 10 - split the band between the chains on both sides of the junction
                IFC(SplitTheBand(pLeftmostHead, pRightmostHead, wIndex));

                // That took care of the leftmost and rightmost heads
                pFrom = pLeftmostHead->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
                if (pFrom != pRightmostHead)
                {
                    Assert(pRightmostHead); // Because there is a leftmost head
                    // We'll pair the rest of the heads into bands in CreateBands below.
                    pTo = pRightmostHead->GetRelevantLeft(CHAIN_REDUNDANT_OR_CANCELLED);
                }            
            }   // End if there is no tail
            
        } // End if the leftmost head is a right chain

        if (pFrom && pTo)   // We have some bands to create
        {
            IFC(CreateBands(pFrom, pTo, wIndex));
        }
    }
    else // There is no head, only a bunch of tails - Figure 11.
    {
        if (pLeftmostTail->IsSideRight()) // We're inside the fill set
        {
            // Merge the two bands on both sides of the junction
            IFC(MergeTheBands(pLeftmostTail, pRightmostTail));
        }
        // else We're outside the fill set, we're done with the current bands.
        
    }   // End if there is no head

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::ProcessAllTails
//
//  Synopsis:
//      Process the tail vertex on all the chains that end at current junction.
//
//  Notes:
//      Some or all thses tail chains may have been grabbed by the junction
//      before their turn has come for processing.  Their cursors may still be
//      above their tail vertices, but we won't bother moving them as they are
//      deactivated by the junction.
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::ProcessAllTails(
    IN WORD    wIndex,
        // Trianguation index of the common tail vertex
    __inout_ecount(1) CChain *pLeftmost,
        // The leftmost tail
    __inout_ecount(1) const CChain *pRightmost)
        // The rightmost tail
{
    HRESULT hr = S_OK;
    
    CChain *pTail = pLeftmost;
    Assert(pTail);      // Otherwise why were we called?

    while (pTail)
    {
        // Allocate a reference to the vertex
        CVertexRef *pvr = m_oMem.AllocateVertexRef(pTail->GetTail(), wIndex);
        IFCOOM(pvr);

        if (pTail->IsSideRight())  // This is a right chain, process the ceiling from the right
        {
            hr = THR(ProcessAsRight(pTail, pvr));
        }
        else                        // This is a left chain, process the ceiling from the left
        {
            hr = THR(ProcessAsLeft(pTail, pvr));
        }
        
        if (pRightmost == pTail) // This is the last tail in the junction
            break;
        
        // Move on the the next band
        pTail = pTail->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::CreateBands
//
//  Synopsis:
//      Create bands from pairs of heads in the current junction
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::CreateBands(
    __inout_ecount(1) CChain *pFirst,
        // The first left chain
    __inout_ecount(1) const CChain *pLast,
        // The last right chain
    IN WORD wIndex)
        // Trianguation index of the common head vertex
{
    HRESULT hr = S_OK;
    CChain *pLeft = pFirst;
    bool fOdd = true;

    Assert(pLast);

    while (pLeft != pLast)
    {
        Assert(pLeft);                  // Should be a non null left chain
        QUIT_IF_NOT(!pLeft->IsSideRight());
        
        CChain *pRight = pLeft->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
        
        Assert(pRight);
        QUIT_IF_NOT(pRight->IsSideRight());  // Should be a non null right chain
        
        // Create a band from this pair of left and right chains
        CVertexRef *pvr = m_oMem.AllocateVertexRef(pLeft->GetHead(), wIndex);
        IFCOOM(pvr);       
        SetCeiling(pLeft, pvr);
        SetCeiling(pRight, pvr);

        if (pRight == pLast)    // All chains have been paired
        {
            fOdd = false;
            break;
        }

        pLeft = pRight->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
    }

    if (fOdd)  // There is a last odd head, attach it to the junction's rightmost tail
    {
        CChain *pRightTail = m_oJunction.GetRightmostTail(CHAIN_REDUNDANT_OR_CANCELLED);

        // Because something is wrong.  The total count of heads & tails must be even.  We have an 
        // odd head count, so there should be at least one tail
        QUIT_IF_NOT(NULL != pRightTail);

        pLeft->AssumeTask(pRightTail);
    }
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::MergeTheBands
//
//  Synopsis:
//      Merge the bands on the two sides of the current junction
//
//  Notes:
//      The junction is inside the fill set, and it has tail chains only.  So
//      there is a band on its left and a band on its right, and here they merge
//      into one band as we are about to terminate these tail chains.
//
//              |  Left |  | Right  |
//              |  Band  \/  Band   |
//              |                   |
//              |   Merged Band     |
//              |                   |
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::MergeTheBands(
    __inout_ecount(1) CChain *pLeftmostTail,
        // The leftmost tail
    __inout_ecount(1) CChain *pRightmostTail)
        // The rightmost tail
{
    HRESULT hr = S_OK;

    // We should not be here unless ---
    
    // We were called on a nonempty junction with no head, so it should have at least 2 tails
    QUIT_IF_NOT(pLeftmostTail && pRightmostTail  && (pLeftmostTail != pRightmostTail));

    // Get the ceiling links of the leftmost and rightmost tail
    CVertexRef *pLeftCeiling = GetCeiling(pLeftmostTail);
    CVertexRef *pRightCeiling = GetCeiling(pRightmostTail);

    // The right ceiling and left ceiling should meet at the junction point
    QUIT_IF_NOT(pLeftCeiling  && pRightCeiling);
    TESSELLATOR_VALIDATE_COINCIDENCE(pLeftCeiling, pRightCeiling);

    // Hook the two ceilings together, skipping the duplicate vertex
    if (pLeftCeiling->GetLeft())
    {
        // Connect the left ceiling to the right ceiling, removing the duplicate vertex
        pLeftCeiling->GetLeft()->LinkTo(pRightCeiling);
        m_oMem.Free(pLeftCeiling);
    }
    else
    {
        // The left ceiling consisted of a single vertex, which duplicates the leftmost vertex
        // of the right ceiling.  So we hook up the right ceiling directly to the left chain
        CChain *p = m_oJunction.GetLeft();
        if (p)
        {
            if (GetCeiling(p)) // Which should actually always be true, but not worth the risk
            {
                m_oMem.Free(GetCeiling(p));
            }
            SetCeiling(p, pRightCeiling);
            pRightCeiling->SetAsLeftmost();
        }
        else
        {
            // Something is wrong, we should be inside a band, with a chain on our left
            TEST_ALARM;
            hr = THR(WGXERR_SCANNER_FAILED);
        }
    }
    // The tails are on their way out, so we won't bother detaching them from the ceiling

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::SplitTheBand
//
//  Synopsis:
//      Split the band at the current junction
//
//  Notes:
//      The junction is inside the fill set, and it has a bunch of heads but no
//      tail.  So here the band splits into two bands by the head chains at the
//      junction
//
//              |                   |
//              |       Band        |
//              |                   |
//              |  Left  /\  Right  |
//              |  Band |  | Band   |
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::SplitTheBand(
    __inout_ecount(1) CChain *pLeftmostHead,
        // The leftmost head
    __inout_ecount(1) CChain *pRightmostHead,
        // The rightmost head
    IN WORD    wIndex)
        // Trianguation index of the current junction vertex
{
    HRESULT hr = S_OK;

    // We should be here only if we have at least two heads 
    QUIT_IF_NOT(pLeftmostHead  && pRightmostHead  && (pLeftmostHead != pRightmostHead));

    CChain *pFrom;
    CChain *pTo;
    CVertexRef *pRightCeiling;
    CChain *pLeft = m_oJunction.GetLeft();

    QUIT_IF_NOT(NULL != pLeft); 
    // Because we know that the leftmost head is a right chain, otherwise we wouldn't be here

    // Find the lowest vertex in the ceiling above this junction
    CVertexRef *pLeftCeiling = GetCeiling(pLeft);
    while (pLeftCeiling->GetRight() && pLeftCeiling->GetRight()->IsLowerThan(pLeftCeiling))
    {
        pLeftCeiling = pLeftCeiling->GetRight();
    }
    
    // Split the ceiling there
    IFCOOM(pRightCeiling = pLeftCeiling->Split(m_oMem));

    if (!pRightCeiling->GetRight())
    {
        // We rely on the ceiling links to connect the right ceiling to the right chain, but
        // here the lowest vertex is the rightmost one, so it is linked to nowhere. So we 
        // need to hook it to the right chain manually as a single-vertex ceiling.
        pTo = m_oJunction.GetRight();
        QUIT_IF_NOT(NULL != pTo);  // because we're supposed to be inside a band

        // This is the right end of the ceiling, we need to reattach the copy to the right chain
        SetCeiling(pTo, pRightCeiling); 
    }

    // Connect the leftmost head chain to the left side of the ceiling
    IFC(Connect(pLeftmostHead, pLeftCeiling, wIndex));

    // Connect the rightmost head chain to the right side of the ceiling
    IFC(Connect(pRightmostHead, pRightCeiling, wIndex));

    // Construct bands from all the pairs of heads in between
    pFrom = pLeftmostHead->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
    QUIT_IF_NOT(NULL != pFrom); // because we expect at least 2 heads

    if (pFrom != pRightmostHead)
    {
        pTo = pRightmostHead->GetRelevantLeft(CHAIN_REDUNDANT_OR_CANCELLED);
        QUIT_IF_NOT(NULL != pTo);   // because we expect at least 2 heads
        
        IFC(CreateBands(pFrom, pTo, wIndex)); 
    }
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::Connect
//
//  Synopsis:
//      Connect a chain to the ceiling and process its head
//
//  Notes:
//      The head of the chain is lower than the end of the ceiling, so here we
//      connect the head to the ceiling and process it.
//
//                            /
//                        ___/ Ceiling
//                      */
//             Head *          
//                  |
//          Chain   |
//                  |
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::Connect(
    __inout_ecount(1) CChain     *pChain,
        // A chain, to which a vertex is added here
    __in_ecount(1) CVertexRef *pCeiling,
        // Ceiling end vertex
    IN WORD        wIndex)
        // Trianguation index of the current junction vertex
{
    // We should be here only if:
    Assert(pChain);
    Assert(pCeiling);

    HRESULT hr = S_OK;
    CVertexRef *pHead; 
    
    // Allocate an additional ceiling vertex at the chain's head
    pHead = m_oMem.AllocateVertexRef(pChain->GetHead(), wIndex);
    IFCOOM(pHead);

    // Connect the chain to the ceiling, and then process it
    SetCeiling(pChain, pCeiling);
    if (pChain->IsSideRight())
    {
        IFC(ProcessAsRight(pChain, pHead));
    }
    else
    {
        IFC(ProcessAsLeft(pChain, pHead));
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::ProcessCurrentVertex
//
//  Synopsis:
//      Process the current vertex on a given chain
//
//  Notes:
//      We add the vertex the ceiling, and then as long as the ceiling is not
//      concave we carve triangles out of it.
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::ProcessCurrentVertex(
    __inout_ecount(1) CChain *pChain)
        // The chain to process the current vertex on
{
    Assert(pChain);
    Assert(pChain->GetCurrentVertex());   // Otherwise we should not be called
    Assert(!pChain->IsAtTail());          // Should not be called at a tail
    
    HRESULT hr = S_OK;
        
    // Allocate a reference to the vertex
    WORD wIndex;
    CVertexRef *pvr; 
    IFC(AddVertex(pChain->GetCurrentApproxPoint(), wIndex));
    pvr = m_oMem.AllocateVertexRef(pChain->GetCurrentVertex(), wIndex);
    IFCOOM(pvr);

    if (pChain->IsSideRight())  // This is a right chain, process the ceiling from the right
    {
        hr = THR(ProcessAsRight(pChain, pvr));
    }
    else                        // This is a left chain, process the ceiling from the left
    {
        hr = THR(ProcessAsLeft(pChain, pvr));
    }

    VALIDATE_BANDS;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::ProcessAsRight
//
//  Synopsis:
//      Process a chain as a right chain in a band. As long as the ceiling is
//      not concave we carve triangles out of it from the left.
//
//  Notes:
//      If you make any changes to this method, make sure to change
//      ProcessAsLeft as well
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::ProcessAsRight(
    __inout_ecount(1) CChain *pChain,
        // The chain to process
    __in_ecount(1) CVertexRef *pNext)
        // Reference to the next vertex down the chain
{
    HRESULT hr = S_OK;
    GpPointR pt(pNext->GetPoint());
    CVertexRef *pLeftLeft;

    // Here we are processing a non-head vertex.  This chains should have already been set up with
    // a ceiling when the junction containing its head was flushed
    CVertexRef *pLeft = GetCeiling(pChain);
    QUIT_IF_NOT(pNext  &&  pLeft);                 // Something is wrong
    
    // Insert the next vertex as the rightmost ceiling vertex
    pLeft->LinkTo(pNext);
    SetCeiling(pChain, pNext);

    for (pLeft = pNext->GetLeft();  pLeft != NULL;  pLeft = pLeftLeft)
    {
        pLeftLeft = pLeft->GetLeft();
        if (!pLeftLeft  || IsLeftTurn(pLeftLeft->GetPoint(), pLeft->GetPoint(), pt))
            break;

        // Cut the corner - create a triangle and remove pLeft from the ceiling
        IFC(CreateTriangle(*pNext, *pLeft, *pLeftLeft));
        pLeftLeft->LinkTo(pNext);
        m_oMem.Free(pLeft);
    }
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::ProcessAsLeft
//
//  Synopsis:
//      Process a chain as a left chain in a band. As long as the ceiling is not
//      concave we carve triangles out of it from the left.
//
//  Notes:
//      If you make any changes to this method, make sure to change
//      ProcessAsRight as well
//
//------------------------------------------------------------------------------
HRESULT
CTessellator::ProcessAsLeft(
    __inout_ecount(1) CChain       *pChain,
        // The chain to process
    __in_ecount(1) CVertexRef   *pNext)
        // Reference to the next vertex down the chain
{
    HRESULT hr = S_OK;
    GpPointR pt = pNext->GetPoint();
    CVertexRef *pRightRight;

    // Here we are processing a non-head vertex.  This chains should have already been set up with
    // a ceiling when the junction containing its head was flushed
    CVertexRef *pRight = GetCeiling(pChain);
    QUIT_IF_NOT(pNext  &&  pRight);                 // Something is wrong
    
    // Insert the next vertex as the leftmost ceiling vertex
    pNext->LinkTo(pRight);
    SetCeiling(pChain, pNext);

    for (pRight = pNext->GetRight();  pRight != NULL;   pRight = pRightRight)
    {
        pRightRight = pRight->GetRight();
        if (!pRightRight  || IsLeftTurn(pt, pRight->GetPoint(), pRightRight->GetPoint()))
            break;

        // Cut the corner - create a triangle and remove pRight from the ceiling
        IFC(CreateTriangle(*pNext, *pRight, *pRightRight));
        pNext->LinkTo(pRightRight);
        m_oMem.Free(pRight);
    }
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::AddVertex
//
//  Synopsis:
//      Add a tesellation vertex
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE HRESULT
CTessellator::AddVertex(
    __in_ecount(1) const GpPointR &ptR,
        // The vertex location
    __out_ecount(1) WORD &wIndex)
        // Triangulation vertex index
{
    HRESULT hr;
    GpPointR ptOut = ptR * m_rInverseScale + m_ptCenter;
    MilPoint2F ptF;

    ptOut.Set(OUT ptF);
    hr = THR(m_refSink.AddVertex(ptF, &wIndex));

    RRETURN(hr);
}
#ifdef DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::ValidateBands
//
//  Synopsis:
//      Validate the band structure
//
//  Notes:
//      Do not call this method at the end of ProcessTheJunction, because at
//      that stage the new bands have been created on the junctions's
//      head-chains, but they have not yet entered the active list, and you are
//      likely to get a false alarm.
//
//------------------------------------------------------------------------------
void
CTessellator::ValidateBands()
{
    CChain *pRightChain = NULL;
    CChain *pLeftChain;

    for (pLeftChain = m_oActive.GetLeftmost()->GoRightWhileRedundant(CHAIN_REDUNDANT_OR_CANCELLED);
        pLeftChain;
        pLeftChain = pRightChain->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED))
    {
        pRightChain = pLeftChain->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
        Assert(pRightChain);    // The band should have a left and a right chain
        
        CVertexRef *pvrFirst = GetCeiling(pLeftChain);
        Assert(pvrFirst);       // The band should have a ceiling
        Assert(!pvrFirst->GetLeft());
        
        // Traverse the ceiling and validate the links
        CVertexRef *pvr = pvrFirst;

        for ( ; ; )
        {
            pvr->AssertNoLeftDuplicate();
            CVertexRef *pvrNext = pvr->GetRight();
            if (!pvrNext)
            {
                break;
            }
            Assert(pvrNext->GetLeft() == pvr);
            pvr = pvrNext;
        }

        // Now pvr is the rightmost vertex in the ceiling, so
        Assert(!pvr->GetRight());
        Assert(pvr == GetCeiling(pRightChain));
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CTessellator::DumpBands
//
//  Synopsis:
//      Dump the band structure
//
//------------------------------------------------------------------------------
void
CTessellator::DumpBands()
{
    CChain *pRightChain = NULL;
    CChain *pLeftChain = m_oActive.GetLeftmost();

    if (pLeftChain)
    {
        OutputDebugString(L"Bands:\n");
        for (pLeftChain = pLeftChain->GoRightWhileRedundant(CHAIN_REDUNDANT_OR_CANCELLED);
            pLeftChain;
            pLeftChain = pRightChain)
        {
            pRightChain = pLeftChain->GetRelevantRight(CHAIN_REDUNDANT_OR_CANCELLED);
            
            OutputDebugString(L"Left chain: ");
            pLeftChain->Dump();

            OutputDebugString(L"Right chain: ");
            if (pRightChain)
            {
                pRightChain->Dump();
            }
            else
            {
                OutputDebugString(L"Right chain: NULL\n");
            }

            for (CVertexRef *pvr = GetCeiling(pLeftChain);  pvr;   pvr = pvr->GetRight())
            {
                pvr->Dump();
            }
        }
    }
}
#endif

