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
//      Processing a polygonal CShape by scanning its vertices
//
//  $ENDTAG
//
//  Classes:
//      CScanner with supporting classes CChain, CChainList, CActiveList
//      CJunction, CVertex, CVertexPool.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


using namespace RobustIntersections;


#if DBG
    int g_iTestCount = 0;
    bool g_fScannerTrace = false;
#endif


const WORD CHAIN_REVERSED =         0x0010;    // Reversed upon creation
const WORD CHAIN_COINCIDENT =       0X0020;    // Coinicides with active chain on its right

const WORD CHAIN_SIDE_RIGHT =       0x0100;    // Classified as right chain
const WORD CHAIN_SELF_REDUNDANT =   0x0200;    // Classified as redundant in its own shape
const WORD CHAIN_CANCELLED =        0x0400;    // Cancelled with a non-redundant coincident chain

// Boolean stuff
const WORD CHAIN_SHAPE_MASK =       0x0001;   // Shape index (0 or 1) in a Boolean operation
const WORD CHAIN_BOOL_FLIP_SIDE =   0x1000;   // Boolean operation flipped the side
const WORD CHAIN_BOOL_REDUNDANT =   0x2000;   // Classified as redundant by a Boolean operations

// Combinations
const WORD CHAIN_REDUNDANT_MASK = CHAIN_SELF_REDUNDANT | CHAIN_BOOL_REDUNDANT;
const WORD CHAIN_REDUNDANT_OR_CANCELLED = CHAIN_REDUNDANT_MASK | CHAIN_CANCELLED;
const WORD CHAIN_SELF_TYPE_MASK = CHAIN_SIDE_RIGHT | CHAIN_SELF_REDUNDANT; 
const WORD CHAIN_INHERITTED_MASK = CHAIN_REVERSED | CHAIN_SHAPE_MASK; 

const int MAX_VERTEX_COUNT =        0xfffe;


//  Implementation of helper classes


// Implementation of CScanner::CIntersectionPool

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CIntersectionPool::AllocateIntersection
//
//  Synopsis:
//      Allocate and set a copy of a given intersection
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CIntersectionPool::AllocateIntersection(
    __deref_out_ecount(1) CLineSegmentIntersection *&pNew)
        // The allocated record
{
    HRESULT hr = S_OK;

    IFC(Allocate(&pNew));
    Assert(pNew);

    pNew->Initialize();

#if DBG
    pNew->m_id = m_id++;
#endif

Cleanup:
    RRETURN(hr);
}

// Implementation of CScanner::CEdgeIntersection

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CEdgeIntersection::Initialize
//
//  Synopsis:
//      Initialization
//
//  Notes:
//      The compiler will not allow a class in a union if it has a copy
//      constructor. That is why this is implementaed as Initialize rather than
//      as a constructor
//
//------------------------------------------------------------------------------
void 
CScanner::CEdgeIntersection::Initialize()
{
    m_eFlavor = eSegmentUnknown;
    m_eLocationOnEdge = CLineSegmentIntersection::LOCATION_UNDEFINED;
    m_pCrossSegmentBase = NULL;
    m_pIntersection = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CEdgeIntersection::Copy
//
//  Synopsis:
//      Copy
//
//  Notes:
//      The compiler will not allow a class in a union if it has a copy
//      constructor. That is why this is implementaed as a Copy method rather
//      than as a copy constructor
//
//------------------------------------------------------------------------------
void
CScanner::CEdgeIntersection::Copy(
    __in_ecount(1) const CEdgeIntersection &other)
        // Object to copy
{
    m_eFlavor = other.m_eFlavor;
    m_eLocationOnEdge = other.m_eLocationOnEdge;
    m_pCrossSegmentBase = other.m_pCrossSegmentBase;
    m_pIntersection = other.m_pIntersection;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CEdgeIntersection::GetParameterAlongSegment
//
//  Synopsis:
//      Return an approximate parameter on the segment for this intersection
//
//------------------------------------------------------------------------------
double
CScanner::CEdgeIntersection::GetParameterAlongSegment() const
{
    Assert(m_pIntersection);

    if (IsUnderlyingSegmentAB())
        return m_pIntersection->ParameterAlongAB();
    else
        return m_pIntersection->ParameterAlongCD();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CEdgeIntersection::CompareWithSameSegmentIntersection
//
//  Synopsis:
//      Compare this intersection against another that lies along the same
//      segment.
//
//  Returns:
//      C_STRICTLYESSTHAN     if this < other
//      C_EQUAL               if this = other
//      C_STRICTLYGREATERTHAN if this > other
//
//  Notes:
//      It is assumed without checking that the two intersections share a common
//      segment.
//
//------------------------------------------------------------------------------
COMPARISON
CScanner::CEdgeIntersection::CompareWithSameSegmentIntersection(
    __in_ecount(1) const CEdgeIntersection &other) const
        // Other intersection to compare with
{
    // Determine which (AB/CD) is the common edge on each intersection
    CLineSegmentIntersection::PAIRING pairing;
    if (m_eFlavor == eSegmentAB)
    {
        if (other.m_eFlavor == eSegmentAB)
        {
            pairing = CLineSegmentIntersection::PAIRING_FIRST_FIRST;
        }
        else
        {
            pairing = CLineSegmentIntersection::PAIRING_FIRST_LAST;
        }
    }
    else
    {
        if (other.m_eFlavor == eSegmentAB)
        {
            pairing = CLineSegmentIntersection::PAIRING_LAST_FIRST;
        }
        else
        {
            pairing = CLineSegmentIntersection::PAIRING_LAST_LAST;
        }
    }

    //
    // Call the line segment service
    //
    // Note that the sort method compares the lambdas of our segments.
    // Since all of our segments go from top down, though, this is the
    // opposite of what we want.
    //
    return OppositeComparison(
        CLineSegmentIntersection::
            SortTransverseIntersectionsAlongCommonLineSegment(
                *m_pIntersection, 
                *other.m_pIntersection,
                pairing
                )
        );
}
 
// Implementation of CIntersectionResult

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CIntersectionResult::IntersectSegments
//
//  Synopsis:
//      Form the intersection on AB given the base vertices for segments AB and
//      CD
//
//  Returns:
//      true if a transverse intersection between the segments was found. Here,
//      the intersection is performed as if the segments were half-open (base
//      excluded and tip included).
//
//  Notes:
//      In order to save time we abort without setting all the output parameter
//      once we determine that the segments do not intersect.  Only transverse
//      intersections count.
//
//------------------------------------------------------------------------------
bool
CScanner::CIntersectionResult::IntersectSegments(
    __in_ecount(1) const CVertex *pABBase,
        // Base vertex for segment ab
    __in_ecount(1) const CVertex *pCDBase,
        // Base vertex for segment cd
    __out_ecount(1) CLineSegmentIntersection::LOCATION &eLocationOnAB,
        // Intersection location wrt AB
    __out_ecount(1) CLineSegmentIntersection::LOCATION &eLocationOnCD
        // Intersection location wrt CD
    )
{
    Assert(pABBase);
    Assert(pABBase->GetSegmentTip());
    Assert(pCDBase);
    Assert(pCDBase->GetSegmentTip());
    Assert(m_pIntersection);
    

    // Gather edges base and tip coordinates and compute the intersection
    double  ab[4] = {pABBase->GetSegmentBasePoint().X, pABBase->GetSegmentBasePoint().Y,
                     pABBase->GetSegmentTipPoint().X,  pABBase->GetSegmentTipPoint().Y};
    double  cd[4] = {pCDBase->GetSegmentBasePoint().X, pCDBase->GetSegmentBasePoint().Y,
                     pCDBase->GetSegmentTipPoint().X,  pCDBase->GetSegmentTipPoint().Y};

    CLineSegmentIntersection::KIND kind = m_pIntersection->PairwiseIntersect(ab, 
                                                                             cd, 
                                                                             eLocationOnAB, 
                                                                             eLocationOnCD);
    Assert(kind != CLineSegmentIntersection::KIND_UNDEFINED);

    //
    // Interpret the result
    //

    bool fFound = 
        (kind == CLineSegmentIntersection::KIND_TRANSVERSE)
        &&
        (eLocationOnAB != CLineSegmentIntersection::LOCATION_AT_FIRST_POINT)
        &&
        (eLocationOnCD != CLineSegmentIntersection::LOCATION_AT_FIRST_POINT);

    if (!fFound)
        goto exit;  // No intersection, we are not interested in any additional information

    m_eFlavor = eSegmentAB;
    m_pCrossSegmentBase = pCDBase;
    
    if (eLocationOnAB == CLineSegmentIntersection::LOCATION_AT_LAST_POINT)
    {
        m_fIsExact = true;
        m_pt = pABBase->GetSegmentTipPoint();
    }
    else if (eLocationOnCD == CLineSegmentIntersection::LOCATION_AT_LAST_POINT)
    {
        m_fIsExact = true;
        m_pt = pCDBase->GetSegmentTipPoint();
    }
    else
    {
        // The intersection is at neither segment's endpoint, hence not exact
        m_fIsExact = false;
    }

exit:
    return fFound;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CIntersectionResult::FormDualIntersectionOnCD
//
//  Synopsis:
//      Form the intersection on CD given the intersection on AB and the base
//      vertex for AB.
//
//------------------------------------------------------------------------------
void
CScanner::CIntersectionResult::FormDualIntersectionOnCD(
    __in_ecount(1) const CIntersectionResult &refOnAB,
        // Intersection on AB
    __in_ecount(1) const CVertex *pABBase)
        // Base vertex for segment ab
{
    Assert(pABBase);

    *this = refOnAB;
    m_eFlavor = eSegmentCD;
    m_pCrossSegmentBase = pABBase;
}

// Implementation of CScanner::CVertex

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::InitializeAtPoint
//
//  Synopsis:
//      Constructor; construct an endpoint or exact intersection
//
//------------------------------------------------------------------------------
void
CScanner::CVertex::InitializeAtPoint(
    __in_ecount(1) const GpPointR &pt,
        // Vertex coordinates
    __in bool fIsEndpoint
        // true ==> endpoint, false ==> exact intersection
    )
{
    m_pt = pt;
    m_pNext = m_pPrevious = NULL;
    m_fSmoothJoin = false;
    
    if (fIsEndpoint)
    {
        m_eType = eVTypeEndpoint;
        m_oSegment.pTip = NULL;
    }
    else
    {
        m_intersection.SetEdgeLocation(CLineSegmentIntersection::LOCATION_UNDEFINED);
        m_eType = eVTypeExactIntersect;
        m_oSegment.pBase = NULL;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::InitializeAtIntersection
//
//  Synopsis:
//      Constructor; construct an non-exact intersection
//
//------------------------------------------------------------------------------
void
CScanner::CVertex::InitializeAtIntersection(
    __in_ecount(1) const CEdgeIntersection &refEdgeIntersect,
        // Edges intersection info
    __in_ecount(1) const GpPointR &pt)
        // Approximate vertex coordinates
{
    m_eType = eVTypeIntersection;
    m_intersection.Copy(refEdgeIntersect);
    m_intersection.SetEdgeLocation(CLineSegmentIntersection::LOCATION_UNDEFINED);
    m_pt = pt;
    m_pNext = m_pPrevious = NULL;
    m_oSegment.pTip = NULL;
    m_fSmoothJoin = false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::InitializeAsCopy
//
//  Synopsis:
//      Copy constructor
//
//------------------------------------------------------------------------------
void
CScanner::CVertex::InitializeAsCopy(
    __in_ecount(1) const CVertex &pOther)
        // Copy construction
{
    *this = pOther;
    m_pNext = m_pPrevious = NULL;

    // Curve retrieval information is edge-based, not vertex based.
    ClearCurve();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::InsertAsHead
//
//  Synopsis:
//      Set this vertex as chain head
//
//  Note:
//      both <this> and pHead must be endpoints
//
//------------------------------------------------------------------------------
void
CScanner::CVertex::InsertAsHead(
    __in_pcount_inout_ecount(1,1) CVertex *&pHead)
        // Current chain head, updated in-place
{
    Assert(pHead);
    Assert(pHead->IsSegmentEndpoint());
    Assert(IsSegmentEndpoint());
    
    m_pNext = pHead;
    m_oSegment.pTip = pHead;
    pHead->m_pPrevious = this;

    pHead = this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::InsertAsTail
//
//  Synopsis:
//      Set this vertex as chain tail
//
//  Note:
//      both <this> and pHead must be endpoints
//
//------------------------------------------------------------------------------
void
CScanner::CVertex::InsertAsTail(
    __in_pcount_inout_ecount(1,1) CVertex *&pTail)
        // Chain's tail, changed here
{
    Assert(pTail);
    Assert(pTail->IsSegmentEndpoint());
    Assert(IsSegmentEndpoint());

    pTail->m_pNext = this;
    pTail->m_oSegment.pTip = this;
    m_pPrevious = pTail;
    m_pNext = NULL;
    m_oSegment.pTip = NULL;
    
    pTail = this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::LinkEdgeTo
//
//  Synopsis:
//      Link <this> vertex to pNext in the chain.
//
//------------------------------------------------------------------------------
void
CScanner::CVertex::LinkEdgeTo(
    __inout_ecount_opt(1) CVertex *pNext)
        // The vertex to link to (NULL okay)
{
    m_pNext = pNext;
    if (pNext)
        pNext->m_pPrevious = this;

    // Fix the edge and segment links
    CVertex    *pBase = GetSegmentBase();
    CVertex    *pVtx = m_pNext;

    while (pVtx != NULL  &&  !pVtx->IsSegmentEndpoint())
    {
        pVtx->m_oSegment.pBase = pBase;
        pVtx = pVtx->GetNext();
    }

    // At this point, if PVtx is not NULL it points at the segment's tip
    if (pVtx  &&  pBase)
        pBase->m_oSegment.pTip = pVtx;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::Attach
//
//  Synopsis:
//      Attach the head of a chain to this vertex
//
//  Notes:
//      This method is called when this is the tail of a chain, and we are
//      appending another chain to it whose head coincides with this.
//
//------------------------------------------------------------------------------
void
CScanner::CVertex::Attach(
    __inout_ecount(1) CVertex *pHead)
        // Other chain's head
{
    Assert(pHead);                                // The head should be there
    Assert(pHead->IsSegmentEndpoint());           // Both head...
    Assert(IsSegmentEndpoint());                  // ... and tail vertices must be segment endpoints
    Assert(m_pt == pHead->GetExactCoordinates()); // And they must coincide

    // Since the head coincides with this, abandon it
    m_pNext = pHead->m_pNext;
    m_oSegment.pTip = pHead->m_oSegment.pTip;
    if (m_pNext)
        m_pNext->m_pPrevious = this;

    // Link any intersection vertex below head to new base
    CVertex     *pvi = m_pNext;
    while (pvi != NULL  &&  !pvi->IsSegmentEndpoint())
        pvi->m_oSegment.pBase = this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::CompareWith
//
//  Synopsis:
//      Compare the height of this vertex with that of another vertex using YX
//      order
//
//------------------------------------------------------------------------------
COMPARISON
CScanner::CVertex::CompareWith(
    __in_ecount(1) const CVertex *pOther) const
        // The other vertex to compare with
{
    Assert(pOther);

    COMPARISON  result = C_UNDEFINED;

    if (IsExact())
    {
        GpPointR    ptThis = GetExactCoordinates();

        if (pOther->IsExact())
        {
            // With two exact locations, we can compare coordinates directly
            GpPointR ptOther = pOther->GetExactCoordinates();

            result = ComparePoints(ptThis, ptOther);
        }
        else
        {
            result = pOther->m_intersection.CompareWithPoint(ptThis);
            result = OppositeComparison(result);
        }
    }
    else if (pOther->IsExact())
    {
        result = m_intersection.CompareWithPoint(pOther->GetExactCoordinates());
    }
    else
    {
        result = m_intersection.CompareWithIntersection(pOther->m_intersection);
    }

    return result;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::LocateVertex
//
//  Synopsis:
//      Determine which side of this edge a given vertex lies
//
//  Returns:
//      SCANNER_LEFT, SCANNER_INCIDENT or SCANNER_RIGHT
//
//  Notes:
//      This is really an edge method.  Coincidence is ignored. v must be an
//      endpoint.
//
//------------------------------------------------------------------------------
SCANNER_LOCATION
CScanner::CVertex::LocateVertex(
    __in_ecount(1) const CVertex *v) const
        // The vertex to locate
{
    Assert(v);
    Assert(GetSegmentTip());

    SCANNER_LOCATION location = SCANNER_INCIDENT;
    double ab[4];

    if (v->IsExact())
    {
        // v is an exact location; we can use its coordinates directly
        double c[2];
        c[0] = v->GetExactCoordinates().X;
        c[1] = v->GetExactCoordinates().Y;
        
        ab[0] = GetSegmentBasePoint().X;
        ab[1] = GetSegmentBasePoint().Y;
        ab[2] = GetSegmentTipPoint().X;
        ab[3] = GetSegmentTipPoint().Y;
        location = CLineSegmentIntersection::LocatePointRelativeToLine(c, ab);
    }
    else if (v->GetCrossSegmentBase() != this->GetSegmentBase())
    {
        // v is an intersection vertex and we must compare it with the line the expensive way.
        ab[0] = GetSegmentBasePoint().X;
        ab[1] = GetSegmentBasePoint().Y;
        ab[2] = GetSegmentTipPoint().X;
        ab[3] = GetSegmentTipPoint().Y;
        location = v->m_intersection.
            GetIntersection().LocateTransverseIntersectionRelativeToLine(ab);
    }
    // else, the trivial case : we're comparing against our own cross segment,
    // location was initialized as SCANNER_INCIDENT.

    return location;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::Intersect
//
//  Synopsis:
//      Intersect two edges
//
//  Notes:
//      This is really an edge method.  Coincident intersection is ignored.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CVertex::Intersect(
    __in_ecount(1) const CVertex *pOther,
        // The vertex starting the other edge's segment
    __out_ecount(1) bool &fIntersect,
        // The edges intersect if true
    __out_ecount(1) CIntersectionResult &refOnThis,
        // Intersection info on this edge
    __out_ecount(1) CIntersectionResult &refOnOther) const
        // Intersection info on edge pOther
{
    HRESULT hr = S_OK;
    CLineSegmentIntersection::LOCATION eLocationOnAB, eLocationOnCD;
    
    fIntersect = false;

    Assert(pOther);
    if (!GetSegmentTip()  ||  !pOther->GetSegmentTip())       // We need full edges
    {
        IFC(WGXERR_SCANNER_FAILED);
    }

    fIntersect = refOnThis.IntersectSegments(GetSegmentBase(), 
                                             pOther->GetSegmentBase(),
                                             OUT eLocationOnAB,
                                             OUT eLocationOnCD
                                             );

    if (!fIntersect)
        goto Cleanup;   // No intersection, we are not interested
        
    refOnOther.FormDualIntersectionOnCD(refOnThis, GetSegmentBase());

    // At this point we know that the support segments intersect. Now we need
    // to determine if the edges intersect.
    fIntersect = this->QueryAndSetEdgeIntersection(eLocationOnAB,
                                                   IN OUT refOnThis);
    if (!fIntersect)
        goto Cleanup;   // No intersection, we are not interested

    fIntersect = pOther->QueryAndSetEdgeIntersection(eLocationOnCD,
                                                     IN OUT refOnOther);

Cleanup:
    //
    // Intersection at the base of an edge should have been ruled out by
    // IntersectSegments.
    //
    Assert(!fIntersect  ||
           (refOnThis.GetEdgeLocation()  != CLineSegmentIntersection::LOCATION_AT_FIRST_POINT 
            &&
            refOnOther.GetEdgeLocation() != CLineSegmentIntersection::LOCATION_AT_FIRST_POINT));

    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::IntersectWithSegment
//
//  Synopsis:
//      Get the intersection on this edge with another segment
//
//  Notes:
//      This method is similar to Intersect; the differences are:
//      * This method does NOT check that the intersection point lies within the edge. It
//        should be used only when we know that there is an intersection. That intersection
//        may not be inside the other edge, but we want the intersection of its segment.
//      * This method only sets an intersection record for this edge.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CVertex::IntersectWithSegment(
    __in_ecount(1) const CVertex *pSegmentBase,
        // The vertex starting the other segment
    __out_ecount(1) bool &fIntersect,
        // =true if a transverse intersection was found 
    __out_ecount(1) CIntersectionResult &result) const
        // Intersection info on this edge
{
    HRESULT hr = S_OK;
    CLineSegmentIntersection::LOCATION eLocation, eOtherLocation;
    
    // It takes two edges to intersect
    Assert(pSegmentBase);
    Assert(GetSegmentTip());
    Assert(pSegmentBase->GetSegmentTip());

    // Intersect the segments
    fIntersect = result.IntersectSegments(GetSegmentBase(), 
                                          pSegmentBase,
                                          eLocation,    
                                          eOtherLocation
                                          );
    
    if (!fIntersect)
        goto Cleanup;

    // Set up the intersection record.  There is something wrong if the edges do not intersect.
    QUIT_IF_NOT(QueryAndSetEdgeIntersection(eLocation, result));

Cleanup:
    //
    // Intersection at the base of an edge should have been ruled out by
    // IntersectSegments.
    //
    Assert(!fIntersect  ||
           result.GetEdgeLocation() != CLineSegmentIntersection::LOCATION_AT_FIRST_POINT);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::QueryAndSetEdgeIntersection
//
//  Synopsis:
//      Check if intersection is within this edge and set its location there if
//      it is. Here, the edge is treated as half-open (base excluded and tip
//      included).
//
//  Returns:
//      true if it is
//
//  Notes:
//      Assumes, without verifying, that intersection result is along the
//      (half-open) supporting segment for the edge starting at this vertex.
//      Returns true if the intersection is within the half-open edge; updates
//      the edge-location info in the intersection result record if it is.
//
//------------------------------------------------------------------------------
bool
CScanner::CVertex::QueryAndSetEdgeIntersection(
    CLineSegmentIntersection::LOCATION eLocation,
        // Location on supporting segment
    __inout_ecount(1) CIntersectionResult &result) const
        // intersection result, updated here.
{
    // We need a full edge
    Assert(GetNext());

    Assert(eLocation != CLineSegmentIntersection::LOCATION_AT_FIRST_POINT);

    const CVertex  *pEdgeBase = this;
    const CVertex  *pEdgeTip = GetNext();
    bool            fOnEdge = false;
    COMPARISON      compare;

    //
    // Compare intersection location relative to pEdgeBase
    //

    if (pEdgeBase->IsSegmentEndpoint())
    {
        if (pEdgeTip->IsSegmentEndpoint())
        {
            //
            // This edge spans its entire segment, on which we already know the
            // location.
            //
            result.SetEdgeLocation(eLocation);

            fOnEdge = true;
            goto exit;
        }        
        else
        {
            //
            // We've determined EdgeBase == SegmentBase, and it's assumed that the
            // intersection location is below the segment base, so:
            //
            compare = C_STRICTLYLESSTHAN;
        }
    }
    else if (pEdgeBase->IsExact())
    {
        if (result.IsExact())
        {
            compare = ComparePoints(
                result.GetExactCoordinates(),
                pEdgeBase->GetExactCoordinates());
        }
        else
        {
            compare = result.CompareWithPoint(pEdgeBase->GetExactCoordinates());
        }
    }
    else
    {
        if (result.IsExact())
        {
            // "base < pt" means "pt > base"
            compare = OppositeComparison(
                pEdgeBase->m_intersection.CompareWithPoint(
                    result.GetExactCoordinates())
                );
        }
        else
        {
            compare =
                result.CompareWithSameSegmentIntersection(pEdgeBase->m_intersection);

            Assert(compare == result.CompareWithIntersection(pEdgeBase->m_intersection));
        }
    }

    Assert(compare != C_UNDEFINED);

    if ((compare == C_STRICTLYGREATERTHAN) || (compare == C_EQUAL))
    {
        // In YX-order, the intersection is at or above the edge base vertex
        fOnEdge = false;
        goto exit;
    }

    //
    // We know the intersection occurs somewhere below pEdgeBase.  Now compare
    // its location relative to pEdgeTip.
    //

    compare = C_UNDEFINED;
    if (pEdgeTip->IsSegmentEndpoint() &&
        eLocation == CLineSegmentIntersection::LOCATION_AT_LAST_POINT)
    {
        result.SetEdgeLocation(CLineSegmentIntersection::LOCATION_AT_LAST_POINT);
        fOnEdge = true;
        goto exit;
    }
    else if (pEdgeTip->IsExact())
    {
        if (result.IsExact())
        {
            compare = ComparePoints(
                result.GetExactCoordinates(),
                pEdgeTip->GetExactCoordinates()
                );
        }
        else
        {
            compare = result.CompareWithPoint(pEdgeTip->GetExactCoordinates());
        }
    }
    else
    {
        if (result.IsExact())
        {
            // "tip < pt" means "pt > tip"
            compare = OppositeComparison(
                pEdgeTip->m_intersection.CompareWithPoint(
                    result.GetExactCoordinates())
                );
        }
        else
        {
            compare =
                result.CompareWithSameSegmentIntersection(pEdgeTip->m_intersection);

            Assert(compare == result.CompareWithIntersection(pEdgeTip->m_intersection));
        }
    }

    Assert(compare != C_UNDEFINED);

    if (compare == C_STRICTLYLESSTHAN)
    {
        // In YX-order, the intersection is below the edge tip
        fOnEdge = false;
        goto exit;
    }
    else if (compare == C_EQUAL)
    {
        result.SetEdgeLocation(CLineSegmentIntersection::LOCATION_AT_LAST_POINT);
        fOnEdge = true;
        goto exit;
    }

    // The intersection is on the open segment between pEdgeBase and pEdgeTip
    result.SetEdgeLocation(CLineSegmentIntersection::LOCATION_ON_OPEN_SEGMENT);
    fOnEdge = true;

exit:
    return fOnEdge;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::EvalIntersectApproxCoordinates
//
//  Synopsis:
//      Evaluate the approximate coordinates of an intersection along the edge
//      headed by this vertex.
//
//  Returns:
//      The coordinates
//
//------------------------------------------------------------------------------
GpPointR
CScanner::CVertex::EvalIntersectApproxCoordinates(
    __in_ecount(1) const CEdgeIntersection &isect) const
        // Intersection info
{
    double      lambda = isect.GetParameterAlongSegment();
    GpPointR    ptBase = GetSegmentBasePoint();
    GpPointR    ptTip = GetSegmentTipPoint();

    return GpPointR(ptBase.X + lambda * (ptTip.X-ptBase.X), ptBase.Y + lambda * (ptTip.Y-ptBase.Y));
}

//--------------------------------------------------------------------------------------------------

                        // Implementation of CCurvePool

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CCurvePool::AddCurve
//
//  Synopsis:
//      Start a new cubic Bezier curve.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CCurvePool::AddCurve(
    __in_ecount(1) const GpPointR &ptFirst,
        // The curve's first point
    __in_ecount(3) const GpPointR *pPt)
        // The curve's remaining 3 points
{
    HRESULT hr = S_OK;

    Assert(pPt);

    // Cache a copy of the new curve
    IFC(Allocate(&m_pCurrentCurve));
    m_pCurrentCurve->Initialize(ptFirst, pPt);

Cleanup:
    RRETURN(hr);
}
//--------------------------------------------------------------------------------------------------

                        // Implementation of CVertex
#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertex::AssertValid
//
//  Synopsis:
//      Assert that the edge topology is consistent
//
//  Notes:
//      Debug utility
//
//------------------------------------------------------------------------------
void
CScanner::CVertex::AssertValid() const
{
    if (IsSegmentEndpoint())
    {
        // this is an endpoint; check the edge topology below it

        // Check chain topology down to tip
        const CVertex *p = GetNext();
        while (p  &&  !p->IsSegmentEndpoint())
        {
            Assert(p->m_oSegment.pBase == this);
            p = p->GetNext();
        }

        // p could be NULL if the chain was split. If not then p
        // should point at the tip now; check base to tip pointer
        Assert(p == NULL  ||  p == m_oSegment.pTip);
    }
    else
    {
        // <this> is an intersection vertex. Move up to the edge base and
        // check the topology from there.
        const CVertex *p = m_oSegment.pBase;
        Assert(p);
        if (p)
            p->AssertValid();
    }
}
#endif


//--------------------------------------------------------------------------------------------------

                        // Implementation of CVertexPool

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertexPool::AllocateVertex
//
//  Synopsis:
//      Allocate a new vertex
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CVertexPool::AllocateVertex(
    __deref_out_ecount(1) CVertex *&pNew)
        // The allocated vertex
{
    HRESULT hr = S_OK;

    if (m_cVertices < MAX_VERTEX_COUNT)
    {
        IFC(Allocate(&pNew));
    }
    else
    {
        pNew = NULL;
        hr = E_FAIL;
    }
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertexPool::AllocateVertexAtPoint
//
//  Synopsis:
//      Allocate a new vertex at given point
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CVertexPool::AllocateVertexAtPoint(
    __in_ecount(1) const GpPointR &pt,
        // Vertex coordinates
    __in bool fEndpoint,
        // true ==> endpoint, false ==> intersection
    __deref_out_ecount(1) CVertex *&pNew)
        // The allocated vertex
{
    HRESULT hr;

    IFC(AllocateVertex(pNew));
    Assert(pNew);

    pNew->InitializeAtPoint(pt, fEndpoint);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertexPool::AllocateVertexAtIntersection
//
//  Synopsis:
//      Allocate a new vertex at an intersection
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CVertexPool::AllocateVertexAtIntersection(
    __in_ecount(1) const CEdgeIntersection &isect,
        // Edge intersection info
    __in_ecount(1) const GpPointR &pt,
        // Vertex coordinates
    __deref_out_ecount(1) CVertex *&pNew)
        // The allocated vertex
{
    HRESULT hr;
    IFC(AllocateVertex(OUT pNew));
    if (pNew)
        pNew->InitializeAtIntersection(isect, pt);
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CVertexPool::CopyVertex
//
//  Synopsis:
//      Allocate a copy of a given vertex
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CVertexPool::CopyVertex(
    __in_ecount(1) const CVertex *pvt,
        // Vertex to copy
    __deref_out_ecount(1) CVertex *&pNew)
        // The allocated vertex
{    
    HRESULT hr = S_OK;

    if (m_cVertices < MAX_VERTEX_COUNT)
    {
        IFC(Allocate(&pNew));
        pNew->InitializeAsCopy(*pvt);
    }
    else
    {
        pNew = NULL;
        hr = E_FAIL;
    }
Cleanup:
    RRETURN(hr);
}

//--------------------------------------------------------------------------------------------------

                        // Implementation of CChain

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::Initialize
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
void
CScanner::CChain::Initialize(
    __in_ecount(1) CVertexPool *pVertexPool,
        // The memory pool for vertices
    __in_ecount(1) CChainPool *pChainPool,
        // The memory pool for vertices
    MilFillMode::Enum eFillMode,
        // The fill mode
    WORD wFlags)
        // Initial flags settings
{
    m_pVertexPool = pVertexPool;
    m_pChainPool = pChainPool;
    m_pLeft = m_pRight = NULL;
    m_pCursor = m_pHead = m_pTail = NULL;
    m_wFlags = wFlags;
    m_wWinding = 0;
    m_pTaskData = NULL;
    m_pTaskData2 = NULL;

    m_candidateHeapIndex = NULL_INDEX;

    if (MilFillMode::Winding == eFillMode)
    {
        m_pClassifyMethod = &CScanner::CChain::ClassifyWinding;
        m_pContinueMethod = &CScanner::CChain::ContinueWinding;
    }
    else
    {
        Assert(MilFillMode::Alternate == eFillMode);
        m_pClassifyMethod = &CScanner::CChain::ClassifyAlternate;
        m_pContinueMethod = &CScanner::CChain::ContinueAlternate;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::Reset
//
//  Synopsis:
//      Reset the chain (when activated)
//
//  Synopsis:
//      Limited initialization.
//
//------------------------------------------------------------------------------
void
CScanner::CChain::Reset()
{
    m_pCursor = m_pHead;
    m_pTaskData = NULL;
    m_pLeft = m_pRight = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::StartWith
//
//  Synopsis:
//      Start the chain with a given point
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChain::StartWith(
    __in_ecount(1) const GpPointR &pt)
        // Coordinates of the chain's first vertex
{
    HRESULT hr;
    Assert(!m_pTail);
    Assert(!m_pHead);
    Assert(!m_pCursor);

    IFC(m_pVertexPool->AllocateVertexAtPoint(
        pt, 
        true, // endpoint 
        OUT m_pCursor));

    m_pTail = m_pHead = m_pCursor;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::StartWithCopyOf
//
//  Synopsis:
//      Start the chain with a copy of a given vertex
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChain::StartWithCopyOf(__in_ecount(1) const CVertex *pVertex)
{
    HRESULT hr;
    Assert(!m_pTail);
    Assert(!m_pHead);
    Assert(!m_pCursor);

    IFC(m_pVertexPool->CopyVertex(pVertex, OUT m_pCursor));
    m_pTail = m_pHead = m_pCursor;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::InsertVertexAt
//
//  Synopsis:
//      Insert a vertex at a given point
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChain::InsertVertexAt(
    __in_ecount(1) const GpPointR &pt,
        // Coordinates of the new vertex
    __in_ecount_opt(1) const CBezierFragment *pFragment
        // Curve retrieval info (or NULL)
    )
{
    HRESULT hr;

    Assert(m_pHead); // Should not be called on an empty chain
        
    IFC(m_pVertexPool->AllocateVertexAtPoint(
        pt, 
        true, // endpoint
        OUT m_pCursor));

    if (IsReversed())
    {
        //
        // Curve information is defined per edge. Edge information is stored on
        // the vertex *below* the edge. In this case, the vertex is m_pHead.
        //

        if (pFragment)
        {
            m_pHead->SetCurveInfo(pFragment);
        }

        // This chain is reversed, so insert the new vertex ahead of the head
        m_pCursor->InsertAsHead(m_pHead);
    }
    else
    {
        //
        // Curve information is defined per edge. Edge information is stored on
        // the vertex *below* the edge. In this case, the vertex is m_pCursor.
        //

        if (pFragment)
        {
            m_pCursor->SetCurveInfo(pFragment);
        }

        // Insert after the last vertex
        m_pCursor->InsertAsTail(m_pTail);
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::TryAdd
//
//  Synopsis:
//      Try to add a vertex to this chain at a given point
//
//  Notes:
//      The point is skipped if it duplicates the current point. Otherwise it is
//      added if it continues the chain's ascend/descend trend.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChain::TryAdd(
    __in_ecount(1) const GpPointR &ptNew,
        // Coordinates of the new vertex we're trying to add
    __in_ecount_opt(1) const CBezierFragment *pFragment,
        // Info about the associated curve (or NULL)
    __out_ecount(1) bool &fAscending,
        // =true if the new point is ascending from the previous
    __out_ecount(1) bool &fAdded)
        // =true if the new edge has been added
{
    HRESULT hr = S_OK;

    // The chain is not empty
    Assert(m_pHead);
    Assert(m_pTail);
    Assert(m_pCursor);
    
    fAscending = AreAscending(GetCurrentExactPoint(), ptNew);
    if (m_pHead->GetNext()) // The chain has 2 vertices or more, its trend is already set
    {
        if (fAscending == IsReversed())
        {
            // The new point continues the chain's trend, so insert it in the chain
            IFC(InsertVertexAt(ptNew, pFragment));
            fAdded = true;
        }
        else
        {
            // The new points breaks the chain's trend, so reject it
            fAdded = false;
        }
    }
    else    // This is only the second point, let it set the chain's trend
    {
        SetReversed(fAscending);
        IFC(InsertVertexAt(ptNew, pFragment));
        fAdded = true;
    }
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::SetReversed
//
//  Synopsis:
//      Set the Reversed trend of this chain
//
//  Notes:
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void
CScanner::CChain::SetReversed(
    bool reversed)     // The value to set to
{
    Assert(m_pHead);              // Should be called when the chain has exactly one vertex
    Assert(!m_pHead->GetNext());

    if (reversed)
    {
        m_wFlags |= CHAIN_REVERSED;
    }
    else
    {
        m_wFlags &= (~CHAIN_REVERSED);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::MoveOn
//
//  Synopsis:
//      Move the to the next vertex down the chain
//
//  Notes:
//      This method is called just before the vertex is processed, so the
//      processed vertex is the cursor AFTER incrementing.
//
//------------------------------------------------------------------------------
void
CScanner::CChain::MoveOn()
{
    m_pCursor = m_pCursor->GetNext();    // Moving the cursor down the chain
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::LinkLeftRight
//
//  Synopsis:
//      Link two chains as left-right
//
//  Notes:
//
//------------------------------------------------------------------------------
void
CScanner::CChain::LinkLeftRight(
    __inout_ecount_opt(1) CChain *pLeft,
        // The left chain (NULL OK)
    __inout_ecount_opt(1) CChain *pRight)
        // The right chain (NULL OK)
{
    if (pLeft)
    {
        pLeft->m_pRight = pRight;
    }
    if (pRight)
    {
        pRight->m_pLeft = pLeft;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::InsertBetween
//
//  Synopsis:
//      Insert this chain horizonatlly between two given chains
//
//  Notes:
//
//------------------------------------------------------------------------------
void
CScanner::CChain::InsertBetween(
    __inout_ecount_opt(1) CChain *pLeft,
        // The chain on the left (NULL OK)
    __inout_ecount_opt(1) CChain *pRight)
        // The chain on the right (NULL OK)
{
    m_pLeft = pLeft;
    m_pRight = pRight;

    if (pLeft)
    {
        pLeft->m_pRight = this;
    }
    if (pRight)
    {
        pRight->m_pLeft = this;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::Append
//
//  Synopsis:
//      Append another chain to this one. Assumes both chains go in the same
//      direction.
//
//  Notes:
//
//------------------------------------------------------------------------------
void
CScanner::CChain::Append(
    __in_ecount(1) CChain *pOther)
        // The other chain to append
{
    Assert(pOther);
    Assert(IsReversed() == pOther->IsReversed()); 
    
    CVertex *pThisTail = GetTail();
    CVertex *pOtherHead = pOther->GetHead();

    Assert(pThisTail);  // Should not be called on empty chains
    Assert(pOtherHead);

    //
    // pThisTail and pOtherHead should be the same point, so when we attach the
    // two we can get rid of one of them.
    //
    // Note: It is important to free the head of the other chain and not the
    // tail of this one, since our tail contains information about the edge
    // that precedes it.
    //

    pThisTail->Attach(pOtherHead);
    m_pVertexPool->Free(pOtherHead); // Abandoned when attached, because it's a duplicate

    m_pTail = pOther->m_pTail;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::SplitAtVertex
//
//  Synopsis:
//      Split a chain at a given vertex
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChain::SplitAtVertex(
    __inout_ecount(1) CVertex *pvt,
        // A vertex on the chain to split at
    __deref_out_ecount(1) CChain *&pSplit)
        // The bottom chain after the split
{
    Assert(pvt);

    HRESULT hr= S_OK;
    CVertex *pCopy;

    pSplit = NULL;

    if (pvt == m_pHead  ||  !pvt->GetNext())     // No splitting at head or tail
        goto Cleanup;

    // Copy the vertex
    IFC(m_pVertexPool->CopyVertex(pvt, OUT pCopy));

    //
    // pCopy does not retain pvt's curve information.  This is what we want,
    // since pCopy will be used as the head of a chain.
    //

    // Start a new chain at the copied vertex
    pSplit = m_pChainPool->AllocateChain(*m_pVertexPool);
    IFCOOM(pSplit);
    pSplit->m_pTail = m_pTail;
    pSplit->m_wFlags = m_wFlags & CHAIN_INHERITTED_MASK;
    pSplit->m_pClassifyMethod = m_pClassifyMethod;
    pSplit->m_pContinueMethod = m_pContinueMethod;

    // Hand over the portion of the chain starting at pCopy to the new chain 
    pSplit->m_pHead = pSplit->m_pCursor = pCopy;
    pCopy->LinkEdgeTo(pvt->GetNext());
    pCopy->SetSmoothJoin(false);
    pvt->LinkEdgeTo(NULL);
    m_pTail = pvt;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::SplitAtIntersection
//
//  Synopsis:
//      Split a chain at a given intersection on the current edge
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChain::SplitAtIntersection(
    __in_ecount(1) const CIntersectionResult &result,
        // Intersection result
    __deref_out_ecount(1) CChain *&pSplit)
        // The remaining piece
{
    HRESULT hr = S_OK;
    const CVertex *pNextEdge = m_pCursor->GetNext();

    pSplit = NULL;

    if (pNextEdge)
    {
        // Intersection at edge-base should have been weeded out
        Assert(result.GetEdgeLocation() != CLineSegmentIntersection::LOCATION_AT_FIRST_POINT);
        
        if (result.GetEdgeLocation() == CLineSegmentIntersection::LOCATION_AT_LAST_POINT)
        {
            // Split at the tip of the current edge
            if (!IsAtItsLastEdge())
            {
                IFC(SplitAtVertex(m_pCursor->GetNext(), pSplit));
            }
        }
        else
        {
            // Insert a new vertex on the current edge
            CVertex     *pvt;
            if (result.IsExact())
            {
                IFC(m_pVertexPool->AllocateVertexAtPoint(
                    result.GetExactCoordinates(), 
                    false, // not an endpoint 
                    OUT pvt));
            }
            else
            {
                GpPointR  pt = m_pCursor->EvalIntersectApproxCoordinates(result);
                IFC(m_pVertexPool->AllocateVertexAtIntersection(result, pt, OUT pvt));
            }

            // 
            // The edge that we're on is being subdivided, and hence no longer
            // has the correct curve information. Clear it out.
            //
            // Future Consideration:  Instead of clearing out the Bezier, we
            // could update it with a best-guess of the new Bezier parameters.
            //
            m_pCursor->GetNext()->ClearCurve();

            // Sanity checks
            Assert(pvt->IsHigherThan(m_pCursor->GetNext()));
            Assert(m_pCursor->IsHigherThan(pvt));

            // Fix the chain
            pvt->LinkEdgeTo(m_pCursor->GetNext());
            m_pCursor->LinkEdgeTo(pvt);

            // Split the chain there
            IFC(SplitAtVertex(pvt, pSplit));
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::SplitAtExactPoint
//
//  Synopsis:
//      Split a chain at a given point on the current edge
//
//  Notes:
//      The point must be on the current edge.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChain::SplitAtExactPoint(
    __in_ecount(1) const GpPointR &pt,
        // Intersection point
    __deref_out_ecount(1) CChain *&pSplit)
        // The remaining piece
{
    HRESULT hr = S_OK;
    CVertex *pTip = m_pCursor->GetNext();
    CVertex *pSplitVertex = NULL;

    pSplit = NULL;

    if (pTip)
    {
        IFC(m_pVertexPool->AllocateVertexAtPoint(pt, 
                                                false,  // intersection
                                                OUT pSplitVertex));
        
        // The new vertex must be within this edge
        Assert(!pTip->IsHigherThan(pSplitVertex));
        Assert(m_pCursor->IsHigherThan(pSplitVertex));

        if (pSplitVertex->CoincidesWith(pTip))
        {
            // We don't need the new vertex
            m_pVertexPool->Free(pSplitVertex);

            if (pTip != m_pTail)
            {
                // The split point coincides with the current edge tip
                pSplitVertex = pTip;
            }
            else 
            {
                // There is no point in splitting at the tail
                goto Cleanup;
            }
        }
        else
        {
            // Insert the new vertex in the chain
            pSplitVertex->LinkEdgeTo(m_pCursor->GetNext());
            m_pCursor->LinkEdgeTo(pSplitVertex);
        }

        // Split the chain
        IFC(SplitAtVertex(pSplitVertex, pSplit));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::ClassifyWinding
//
//  Synopsis:
//      Classify in Winding mode as left/right/redundant, based on its left
//      neighbor
//
//  Notes:
//      The type of redundancy detected here is only the one caused by chains of
//      the same direction in winding mode.
//
//------------------------------------------------------------------------------
void
CScanner::CChain::ClassifyWinding(
    __in_ecount_opt(1) const CChain *pLeft)
        // Immediate left neighbor (NULL OK)
{
    // Set the side and the winding number
    WORD wLeftWinding = pLeft ? pLeft->m_wWinding : 0;
    {
        // This chain increments or decrements the winding number, depending on its direction
        if (IsReversed())
            m_wWinding = wLeftWinding - 1;
        else
            m_wWinding = wLeftWinding + 1;
    }
    
    if (0 != wLeftWinding)
    {
        if (0 == m_wWinding)
        {
            SetSideRight();
        }
        else
        {
            SetRedundant();
        }
    }
    else   // wLeftWinding == 0
    {
        if (0 == m_wWinding)
        {
            SetRedundant();
        }
        // else leave the default setting, which is Left
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::ContinueWinding
//
//  Synopsis:
//      Set the left/right/redundant as a continuation of a chain above it -
//      Winding mode
//
//  Notes:
//      This is called on the leftmost head chain in a junction, continuing the
//      leftmost tail chain in that junction, but we have no way of Asserting it
//      here.
//
//------------------------------------------------------------------------------
void
CScanner::CChain::ContinueWinding(
    __in_ecount(1) const CChain *pChain)
        // The chain above it
{
    Assert(pChain);   // Don't call with a null chain
    WORD wType;

    // Set the side/redundancy and the winding number
    if (pChain->IsReversed() == IsReversed())
    {
        // Slam dunk, this chain simply continues the chain above it
        m_wWinding = pChain->m_wWinding;
        m_wFlags |= (pChain->m_wFlags & CHAIN_SELF_TYPE_MASK);
    }
    else 
    {
        // This winding number differs from pChain's by +-2
        if (IsReversed())
        {
            m_wWinding = pChain->m_wWinding - 2;
        }
        else
        {
            m_wWinding = pChain->m_wWinding + 2;
        }

        // Setting the type/redundancy is a bit tricky for winding mode, because we have 
        // to deduce the winding number of our left neighbor from the state of pChain.
        wType = pChain->m_wFlags & CHAIN_SELF_TYPE_MASK;
        if (CHAIN_SIDE_RIGHT == wType)
        {
            Assert(0 == pChain->m_wWinding);

            // The chain left of us must have a non 0 winding number and so do we, hence 
            SetRedundant();
        }
        else if (CHAIN_SELF_REDUNDANT == wType)
        {
            // The chain left of us must have a non 0 winding number 
            if (0 == m_wWinding)    // our winding number is 0, hence
            {
                SetSideRight();
            }
            else    // our winding number is also nonzero, hence
            {
                SetRedundant();
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::GoLeftWhileRedundant
//
//  Synopsis:
//      Get the first non-redundant chain looking to the left, starting with
//      this chain
//
//------------------------------------------------------------------------------
__out_ecount_opt(1) CScanner::CChain *
CScanner::CChain::GoLeftWhileRedundant(
    WORD wRedundantMask) // Defining what is redundant
{
    CChain *pLeft = this;
    while (pLeft  &&  pLeft->IsRedundant(wRedundantMask))
    {   
        pLeft = pLeft->m_pLeft;
    }

    return pLeft;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::GoRightWhileRedundant
//
//  Synopsis:
//      Get the first non-redundant chain looking to the right, starting with
//      given this
//
//------------------------------------------------------------------------------
__out_ecount_opt(1) CScanner::CChain *
CScanner::CChain::GoRightWhileRedundant(
    WORD wRedundantMask) // Defining what is redundant
{
    CChain *pRight = this;
    while (pRight  &&  pRight->IsRedundant(wRedundantMask))
    {   
        pRight = pRight->m_pRight;
    }
    return pRight;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::CoincidesWith
//
//  Synopsis:
//      Return true if the chains' head-edges coincide exactly.
//
//  Notes:
//
//------------------------------------------------------------------------------
bool
CScanner::CChain::CoincidesWith(
     __in_ecount(1) const CChain *pOther) const
        // Another chain
{
    // This check is invoked between two chains with a common head
    Assert(m_pHead->CoincidesWith(pOther->m_pHead));

    // This method is called after chains have been split at coincident vertices, so
    // coincident chains should have exactly two vertices each, and they should
    // coincide.  We have asserted the heads coincidence, so we check the tails:
    return (m_pHead->GetNext() == m_pTail)                    &&
            (pOther->m_pHead->GetNext() == pOther->m_pTail)   &&
            (m_pTail->CoincidesWith(pOther->m_pTail));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::SplitAtIncidentVertex
//
//  Synopsis:
//      Split a chain at a vertex from another chain that lies on it
//
//  Notes:
//      The chain is split in place
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChain::SplitAtIncidentVertex(
    __in_ecount(1) const CVertex *pVertex,
        // The incident vertex
    __inout_ecount(1) CIntersectionPool &refPool,
        // Memory pool for allocating intersections
    __deref_out_ecount(1) CChain *&pSplit)
        // A (possibly NULL) piece split from this chain 
{
    // The chain should have a valid current edge.
    Assert(GetCurrentEdgeTip());
    Assert(pVertex);

    HRESULT hr = S_OK;

    if (pVertex->IsExact())
    {
        IFC(SplitAtExactPoint(pVertex->GetExactCoordinates(), pSplit));
    }
    else
    {
        // The incident vertex, which is the some other segments, lies on this chain.  
        // We need to find the intersection of one of those segments with this edge.
        // We look for one that is not collinear with this edge.
        bool fIntersect;
        CLineSegmentIntersection *pIntersection;
        IFC(refPool.AllocateIntersection(pIntersection));

        CIntersectionResult result(pIntersection);

        IFC(IntersectWithSegment(pVertex->GetCrossSegmentBase(), fIntersect, result));
        if (!fIntersect)
        {
            // The current edge is collinear with one of the cross segment,
            // try the supporting segment
            IFC(IntersectWithSegment(pVertex->GetSegmentBase(), fIntersect, result));
            QUIT_IF_NOT(fIntersect);
            // One of the intersecting segments must traverse this chain
        }

        IFC(SplitAtIntersection(result, pSplit));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::GrabInactiveCoincidentChain
//
//  Synopsis:
//      Return the next inactive chain if its head vertex coincides with pV.
//
//------------------------------------------------------------------------------
__out_ecount_opt(1) CScanner::CChain *
CScanner::GrabInactiveCoincidentChain(
    __in_ecount(1) const CVertex *pV)
        // The current junction point
{
    CChain *pChain = m_oChains.GetNextChain();
    if (pChain)
    {
        Assert(pChain->GetHead());
        if (pV->CoincidesWith(pChain->GetHead()))
        {
            m_oChains.Pop();
        }
        else
        {
            pChain = NULL;
        }
    }

    return pChain;
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CChain::ValidateActiveLinks
//
//  Synopsis:
//      Debug utility - validate links in the active list
//
//------------------------------------------------------------------------------
void
CScanner::CChain::ValidateActiveLinks() const
{
    if (m_pLeft)
    {
        Assert(m_pLeft->m_pRight == this);
    }
    if (m_pRight)
    {
        Assert(m_pRight->m_pLeft == this);
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::Dump
//
//  Synopsis:
//      Dump the list
//
//  Notes:
//      A debugging utility
//
//------------------------------------------------------------------------------
void
CScanner::CChain::Dump(
    bool fBooleanOperation   // = true if called from a boolean operation
    ) const
{
    // Dump the id and points
    if (GetCurrentEdgeTip())
    {
        MILDebugOutput(L"id=%d Points: (%f, %f), (%f, %f)", 
                       m_id,
                       GetCurrentApproxPoint().X,
                       GetCurrentApproxPoint().Y,
                       GetCurrentEdgeApproxTipPoint().X,
                       GetCurrentEdgeApproxTipPoint().Y);
    }
    else
    {
        MILDebugOutput(L"id=%d Points: (%f, %f)", 
                       m_id,
                       GetCurrentApproxPoint().X,
                       GetCurrentApproxPoint().Y);
    }

    // Winding number
    MILDebugOutput(L" winding=%d", m_wWinding);

    // Self Left/right/redundant
    if (IsSelfRedundant())
        MILDebugOutput(L" U");
    else if (IsSelfSideRight())
        MILDebugOutput(L" R");
    else
        MILDebugOutput(L" L");
        
    if (fBooleanOperation)
    {
        // Left/right/redundant in the result of a Boolean operation
        MILDebugOutput(L" shape(%d)", GetShape());
        
        if (IsRedundant(CHAIN_REDUNDANT_MASK))
            MILDebugOutput(L" U");
        else if (IsSideRight())
            MILDebugOutput(L" R");
        else
            MILDebugOutput(L" L");
    }
    MILDebugOutput(L"\n");
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChain::Validate
//
//  Synopsis:
//      Verify that the chain is still valid
//
//  Notes:
//      A debugging utility
//
//------------------------------------------------------------------------------
void
CScanner::CChain::Validate() const
{
    // Must have at least 2 vertices
    CVertex *p = m_pHead;
    Assert(p);
    CVertex *pNext = m_pHead->GetNext();
    Assert(pNext);
    
    // Verify the descending order
    while (pNext)
    {
        Assert(p->IsHigherThan(pNext));
        p = pNext;
        pNext = p->GetNext();
    }
}
#endif
//--------------------------------------------------------------------------------------------------

                        // Implementation of CChainPool

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainPool::AllocateChain
//
//  Synopsis:
//      Allocate a new chain
//
//  Return:
//      A pointer to the new chain, NULL if allocation failed
//
//  Notes:
//
//------------------------------------------------------------------------------
__ecount_opt(1) CScanner::CChain *
CScanner::CChainPool::AllocateChain(
    __inout_ecount(1) CVertexPool &refVertexPool)
        // The memory pool for vertices
{
    HRESULT hr = S_OK;
    CChain *pNew = NULL;

    IFC(Allocate(&pNew));
    pNew->Initialize(&refVertexPool, this, m_eFillMode, m_wShapeIndex);

#if DBG
    pNew->m_id = m_id++;
#endif

Cleanup:
    return pNew;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainPool::SetNext
//
//  Synopsis:
//      Set up for a new shape (in a 2-shape operation)
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChainPool::SetNext()
{
    HRESULT hr = S_OK;

    if (0 == m_wShapeIndex)
    {
        m_wShapeIndex = 1;
    }
    else
    {
        // We can only operate on 2 shapes
        hr = E_UNEXPECTED;
    }

    RRETURN(hr);
}

//--------------------------------------------------------------------------------------------------

                        // Implementation of CChainList

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainList::CChainList
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CScanner::CChainList::CChainList()
    : m_pFiguresFirstChain(NULL),
      m_pCurrent(NULL),
      m_oVertexPool(m_oCurvePool)
{
    SCAN_TEST_INIT; // Start tracking the test case # ifdef DBG
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainList::StartFigure
//
//  Synopsis:
//      Start the chains of a new figure
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChainList::StartFigure(
    __in_ecount(1) const GpPointR &pt)
        // Figure's first point
{
    HRESULT hr = S_OK;

    m_ptFirst = m_ptCurrent = pt;
    
    // Start a new chain in the list, starting with the first point
    IFCOOM(m_pCurrent = m_pFiguresFirstChain = m_oChainPool.AllocateChain(m_oVertexPool)); 
    hr = THR(m_pCurrent->StartWith(m_ptFirst));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainList::AddVertex
//
//  Synopsis:
//      Add a vertex, coming from AddLine or from a flattened AddCurve.
//
//  Notes:
//      Try to add it to the current chain. If it breaks the chain's trend,
//      start a new chain from the end of the current chain
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChainList::AddVertex(
    __in_ecount(1) const GpPointR &ptNew,
        // The new vertex to add
    __in_ecount_opt(1) const CBezierFragment *pFragment
        // Info about the associated curve (or NULL)
    )
{
    HRESULT hr = S_OK;
    bool fAscending, fAddedToCurrentChain;

    // Avoid duplicate vertices
    if (ptNew == m_ptCurrent)
    {
        goto Cleanup;
    }
    
    // We should have a current chain, and it mustn't be empty (after StartFigure).
    Assert(m_pCurrent);
    Assert(m_pCurrent->GetHead());

    // Try to add it to the chain
    IFC(m_pCurrent->TryAdd(ptNew, pFragment, OUT fAscending, OUT fAddedToCurrentChain));

    if (!fAddedToCurrentChain)    // No, we are at a turning point
    {
        const CVertex *pLast = m_pCurrent->GetCurrentVertex();

        // Insert the current chain if it's not the figure's first chain
        if (m_pCurrent != m_pFiguresFirstChain)
        {
            IFC(Insert(m_pCurrent));
        }

        // Start a new chain from the last point of the current chain
        IFCOOM(m_pCurrent = m_oChainPool.AllocateChain(m_oVertexPool));
        IFC(m_pCurrent->StartWithCopyOf(pLast));

        // Add the second point and set the chain's trend
        m_pCurrent->SetReversed(fAscending);
        IFC(m_pCurrent->InsertVertexAt(ptNew, pFragment));
    }

Cleanup:
    m_ptCurrent = ptNew;
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainList::AcceptPoint
//
//  Synopsis:
//      CFlatteningSink override. Called for each point of a flattened Bezier.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChainList::AcceptPoint(
    __in_ecount(1) const GpPointR &ptNew,
    // The new vertex to add
    __in GpReal t,
    // Parameter value on the curve)
    __out_ecount(1) bool &fAbort)
    // Ignored here
{
    HRESULT hr = S_OK;
    GpPointR ptRounded;
    ptRounded.X = CDoubleFPU::Round(ptNew.X);
    ptRounded.Y = CDoubleFPU::Round(ptNew.Y);

    //
    // Theoretically, because a Bezier satisfies the convex hull
    // property, ptRounded should always be valid. Perform a
    // double-check just in case.
    //

    if(!IsValidInteger30(ptRounded.X) ||
        !IsValidInteger30(ptRounded.Y))
    {
        IFC(WGXERR_BADNUMBER);
    }

    fAbort = false;

    {
        CBezierFragment fragment(
            *GetCurrentCurve(),
            m_previousT,
            t
            );

        IFC(AddVertex(ptRounded, &fragment));
    }

Cleanup:
    m_previousT = t;

    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainList::EndFigure
//
//  Synopsis:
//      Close the figure and wrap up the last chain
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChainList::EndFigure(
    __in_ecount(1) const GpPointR &ptCurrent,
        // The most recently added vertex
    __in bool fClosed)
        // =true if the figure is closed
{
    HRESULT hr = S_OK;
    
    if (!fClosed)
    {
        // Add a line segment to the figure's first point
        IFC(AddVertex(m_ptFirst));
    }

    if (m_pFiguresFirstChain == m_pCurrent) // The figure has only one chain, must be degenerate
        goto Cleanup;

    // In general a figure does not start at the top of a chain, and the figure's first chain 
    // and last chain are two parts of one chain.  So now we try to consolidate the figure start
    // with the current (last) chain.
    if (m_pFiguresFirstChain->IsReversed() == m_pCurrent->IsReversed())
    {
        if (m_pFiguresFirstChain->IsReversed())
        {
            m_pFiguresFirstChain->Append(m_pCurrent);
            IFC(Insert(m_pFiguresFirstChain));
            m_oChainPool.Free(m_pCurrent);
        }
        else
        {
            m_pCurrent->Append(m_pFiguresFirstChain);
            IFC(Insert(m_pCurrent));
            m_oChainPool.Free(m_pFiguresFirstChain);
        }
    }
    else
    {
        // No, this is a separate chain, insert both
        IFC(Insert(m_pFiguresFirstChain));
        IFC(Insert(m_pCurrent));
    }

    // In any case
    m_pFiguresFirstChain = m_pCurrent = NULL;
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainList::Insert
//
//  Synopsis:
//      Insert the chain in the list according to its head's height
//
//  Notes:
//      pBelow != NULL indicates that the new chain should fit somewhere below
//      that.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CChainList::Insert(
    __inout_ecount(1) CChain *pNew)
        // The chain to insert
{
    HRESULT hr = S_OK;

    Assert(pNew);             // No point inserting a null chain;
    Assert(pNew->GetHead());  // No point inserting an empty chain

    IFC(m_chainHeap.Insert(pNew));
    
    pNew->Reset();
    
Cleanup:
    RRETURN(hr);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainList::Dump
//
//  Synopsis:
//      Dump the list
//
//  Notes:
//      A debugging utility
//
//------------------------------------------------------------------------------
void
CScanner::CChainList::Dump() const
{
    MILDebugOutput(L"Master list:\n");

    m_chainHeap.Dump();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CChainList::Validate
//
//  Synopsis:
//      Validate that the chains are in the right order
//
//  Notes:
//      A debugging utility
//
//------------------------------------------------------------------------------
void
CScanner::CChainList::Validate() const
{
    m_chainHeap.Validate();
}
#endif

//--------------------------------------------------------------------------------------------------

                        // Implementation of CActiveList


//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CActiveList::Locate
//
//  Synopsis:
//      Locate the position of a new chain in the active list.
//
//  Returns:
//      True if the new chain's head lies on pRight
//
//------------------------------------------------------------------------------
bool
CScanner::CActiveList::Locate(
    __in_ecount(1) const CChain *pNew,
        // The new chain whose position we need to locate
    __deref_out_ecount(1) CChain *&pLeft,
        // The chain on the left of the location (possibly NULL)
    __deref_out_ecount(1) CChain *&pRight)
        // The chain on the right of or at the location
{
    Assert(pNew);                        // No point inserting a null or empty chain;
    Assert(pNew->GetHead());
        
    const CVertex *pNewHead = pNew->GetHead();
    bool fIsOnChain = false;
    pLeft = NULL;
    pRight = m_pLeftmost;
    
    while (pRight)
    {
        SCANNER_LOCATION location = pRight->LocateVertex(pNewHead);
        if (location != SCANNER_RIGHT)
        {
            fIsOnChain = (location == SCANNER_INCIDENT);
            break;
        }
        pLeft = pRight;
        pRight = pRight->GetRight();
    }
    
    return fIsOnChain;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CActiveList::Insert
//
//  Synopsis:
//      Insert a segment of chains in the active and candidate lists
//
//  Notes:
//      It is assumed that the segment is already linked finternally rom left to
//      right, but the leftmost and rightmost are not linked out.
//
//------------------------------------------------------------------------------
void
CScanner::CActiveList::Insert(
    __inout_ecount(1) CChain *pLeft,
        // The leftmost chain to insert
    __inout_ecount(1) CChain *pRight,
        // The rightmost chain to insert
    __inout_ecount_opt(1) CChain *pPrevious,
        // The chain to insert after (NULL OK)
    __inout_ecount_opt(1) CChain *pNext)
        // The chain to insert before (NULL OK)
{
    // Not yet linked
    Assert(!pLeft->GetLeft());
    Assert(!pRight->GetRight());

    CChain::LinkLeftRight(pPrevious, pLeft);
    CChain::LinkLeftRight(pRight, pNext);
    
    if (!pPrevious)
    {
        m_pLeftmost = pLeft;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CActiveList::Remove
//
//  Synopsis:
//      Remove a segment of the active chain list
//
//  Notes:
//      The Right and Left pointers of the removed chains are not being reset
//      here, they will be reset when the chain is activated
//
//------------------------------------------------------------------------------
void
CScanner::CActiveList::Remove(
    __inout_ecount(1) CChain *pFirst,
        // The first chain to remove
    __inout_ecount(1) CChain *pLast)
        // The last chain to remove
{
    Assert(pFirst);
    Assert(pLast);

    CChain *pPrevious = pFirst->GetLeft();
    CChain *pNext = pLast->GetRight();

    if (NULL == pPrevious)
    {
        m_pLeftmost = pNext;
    }
    else
    {
        pPrevious->SetRight(pNext);
    }

    if (NULL != pNext)
    {
        pNext->SetLeft(pPrevious);
    }

    pFirst->SetLeft(NULL);
    pLast->SetRight(NULL);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CActiveList::Includes
//
//  Synopsis:
//      Check if the list includes a given entry
//
//  Notes:
//      A debugging utility
//
//------------------------------------------------------------------------------
bool
CScanner::CActiveList::Includes(
    __in_ecount(1) const CChain *pChain) const // A chain
{
    for (CChain *p = m_pLeftmost;  p;  p = p->GetRight())
    {
        if (p == pChain)
            return true;
    }
    return false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CActiveList::AssertConsistentWith
//
//  Synopsis:
//      Verify that this list is consistent with the candidate list
//
//  Notes:
//      A debugging utility
//
//------------------------------------------------------------------------------
void
CScanner::CActiveList::AssertConsistentWith(
    __in_ecount(1) const CCandidateHeap &list) const // The candidate list
{

    for (CChain *p = m_pLeftmost;  p;  p = p->GetRight())
    {
        if (!list.Includes(p))
        {
            MILDebugOutput(L"CActiveList::AssertConsistentWith falied\n");
            list.Dump();
            Dump();

            Assert(false);
            break;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CActiveList::Dump
//
//  Synopsis:
//      Debug dump
//
//  Notes:
//      If called from a Booelanoperation, dump the Boolean left/right/redundant
//      as well
//
//------------------------------------------------------------------------------
void
CScanner::CActiveList::Dump(
    bool fBooleanOperation
        // = true if called from a boolean operation
    ) const
{
    MILDebugOutput(L"Active list:\n");
    for (CChain *pChain = m_pLeftmost;  pChain;  pChain = pChain->GetRight())
    {
        pChain->Dump(fBooleanOperation);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CActiveList::Validate
//
//  Synopsis:
//      Check if the list is in a valid state
//
//  Notes:
//      A debugging utility
//
//------------------------------------------------------------------------------
void
CScanner::CActiveList::Validate(__in_ecount(1) const CVertex *pV)  const
{
    CChain  *pChain;
    const CChain  *pLeft = m_pLeftmost? m_pLeftmost->GetLeft() : NULL;

    for (pChain = m_pLeftmost;   pChain;  pChain = pChain->GetRight())
    {
        // Validate the chain
        pChain->Validate();

        // Validate links
        Assert(pChain->GetLeft() == pLeft);
        if (pLeft)
        {
            Assert(pLeft->GetRight() == pChain);
        }

        // Validate chain's current edge given vertex pV
        // The chain's current edge should bracket the current vertex position
        const CVertex  *pVBase = pChain->GetCurrentEdgeBase();
        const CVertex  *pVTip = pChain->GetCurrentEdgeTip();
        bool            fIncludeVertex = true;
        COMPARISON      comp;

        Assert(pVBase);
        if (pVBase)
        {
            comp = pVBase->CompareWith(pV);
            fIncludeVertex = (comp == C_STRICTLYGREATERTHAN  ||  comp == C_EQUAL);
        }

        Assert(pVTip);
        if (pVTip)
        {
            comp = pVTip->CompareWith(pV);
            fIncludeVertex = (comp == C_STRICTLYLESSTHAN ||  comp == C_EQUAL);
        }

        if (!fIncludeVertex)
        {
            Dump();
            Assert(false);
        }

        // Validate adjacent chains
        pLeft = pChain->GetLeft();
        if (pLeft  &&  
            pChain->GetCurrentEdgeBase()->GetSegmentVector().Y == 0  &&
            pLeft->GetCurrentEdgeBase()->GetSegmentVector().Y == 0)
        {
            // Two horizontal chains must coincide
            if (!pLeft->GetCurrentEdgeBase()->CoincidesWith(pChain->GetCurrentEdgeBase())  ||
                !pLeft->GetCurrentEdgeTip()->CoincidesWith(pChain->GetCurrentEdgeTip()))
            {
            Dump();
            Assert(false);
            }
        }
        else if (pLeft)
        {
            // Verify that pChain is collinear or to the right of pLeft
            if (pLeft->GetCurrentEdgeBase()->LocateVertex(pVTip) != SCANNER_RIGHT)
            {
               if (pLeft->GetCurrentEdgeBase()->LocateVertex(pVBase) != SCANNER_RIGHT)
                {
                    // Neither vertices of the current edge on pChain are to the
                    // right of the current edge on pLeft. These two edges are better
                    // be collinear.
                    bool fCollinear;

                    double ab[4] = {pLeft->GetCurrentSegmentBase()->GetExactCoordinates().X, 
                                    pLeft->GetCurrentSegmentBase()->GetExactCoordinates().Y,
                                    pLeft->GetCurrentSegmentTipPoint().X,
                                    pLeft->GetCurrentSegmentTipPoint().Y};
                    double c[2] = {pChain->GetCurrentSegmentBase()->GetExactCoordinates().X,
                                   pChain->GetCurrentSegmentBase()->GetExactCoordinates().Y};
                    double d[2] = {pChain->GetCurrentSegmentTipPoint().X, 
                                   pChain->GetCurrentSegmentTipPoint().Y};
                    fCollinear = (CLineSegmentIntersection::LocatePointRelativeToLine(c, ab) == 
                                  CLineSegmentIntersection::SIDE_INCIDENT)    &&
                                 (CLineSegmentIntersection::LocatePointRelativeToLine(d, ab) ==
                                  CLineSegmentIntersection::SIDE_INCIDENT);

                    if (!fCollinear)
                    {
                        // pChain is to the left of pLeft!!!
                        MILDebugOutput(L"Active chains are out of oder!\n");
                        Dump();
                        Assert(false);
                    }
                }
            }
        }

        pLeft = pChain;
    }
}
#endif

//--------------------------------------------------------------------------------------------------

                        // Implementation of Cjunction


//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CJunction::Initialize
//
//  Synopsis:
//      Reset to empty
//
//------------------------------------------------------------------------------
void
CScanner::CJunction::Initialize()
{
    m_pLeftmostHead = m_pRightmostHead = m_pLeftmostTail = m_pRightmostTail = NULL;
    m_pLeft = m_pRight = NULL;
    m_pRepVertex = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CJunction::Flush
//
//  Synopsis:
//      Process junction and reset it to empty
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CJunction::Flush()
{
    HRESULT hr;

    Assert(!IsEmpty());   // Should not be called on an empty junction

    // Deactivate the tails
    if (m_pLeftmostTail)
    {
        m_pOwner->TerminateBatch(m_pLeftmostTail, m_pRightmostTail);
    }

    if (m_pLeftmostHead)
    {
        // Split the new heads at intersections with their neighbors
        IFC(m_pOwner->SplitAtIntersections(m_pLeftmostHead, m_pRightmostHead, m_pLeft, m_pRight));

        // Classify the junction's heads
        Classify();
    }
    else
    {
        // There are no heads, so the chains on the left and right of the junction will become 
        // neighbors once we've flushed this junction, and we need to process them for intersection
        IFC(m_pOwner->SplitPairAtIntersection(m_pLeft, m_pRight));        
    }

    // Perform the application-specific task at this junction.
    IFC(m_pOwner->ProcessTheJunction());  // Virtual method

    if (m_pLeftmostHead)
    {
        // Insert the head chains in the active lists
        m_pOwner->ActivateBatch(m_pLeftmostHead, m_pRightmostHead, m_pLeft, m_pRight);
    }

    Initialize();
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CJunction::InsertHead
//
//  Synopsis:
//      Insert a chain as a head chain among the junction's other head chains
//
//  Notes:
//      Search for the position where to insert, between pPrevious and pCurrent.
//      The new chain's place will be determined by the direction of the first
//      edge relative to the edge directions of the head chains.  Since their
//      heads coincide, this is the same as examining the position of the second
//      vertex relative to the other chains' active edges.  Then insert pNew and
//      link it to its neighbors.
//
//------------------------------------------------------------------------------
void
CScanner::CJunction::InsertHead(
    __inout_ecount(1) CChain *pNew)
        // The new head to insert
{
    Assert(pNew);               // No point inserting a null or empty chain;
    Assert(pNew->GetHead());
    Assert(!IsEmpty());
    Assert(!pNew->GetLeft());   // Should be inactive chain
    Assert(!pNew->GetRight());
   
    if (m_pLeftmostHead)
    {
        Assert(m_pRightmostHead);

        // All heads are not yet active, they will be activated in a batch later
        Assert(!m_pLeftmostHead->GetLeft());
        Assert(!m_pRightmostHead->GetRight());

        // Find out where pNew fits among the current heads in the junction
        const CVertex *pSearchKey = pNew->GetCurrentEdgeTip(); // = chain's 2nd vertex
        Assert(pSearchKey);

        CChain *pPrevious = NULL;
        CChain *pCurrent = m_pLeftmostHead;
        while (pPrevious != m_pRightmostHead  &&  pCurrent->IsVertexOnRight(pSearchKey))
        {
            pPrevious = pCurrent;
            pCurrent = pCurrent->GetRight();
        }

        // Insert pNew between pPrevious and pCurrent
        pNew->InsertBetween(pPrevious, pCurrent);

        // Update the leftmost or rightmost head if necessary
        if (pCurrent == m_pLeftmostHead)         // Found to be left of the leftmost head
        {                   
            m_pLeftmostHead = pNew;
        }
        else if (pPrevious == m_pRightmostHead) // Found to be right of the rightmost head
        {
            m_pRightmostHead = pNew;
        }
    }
    else
    {
        // This is the first head in the junction
        Assert(m_pRightmostTail);   // Because the junction has no heads and is not empty
        m_pLeftmostHead = m_pRightmostHead = pNew;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::Cjunction::ProcessAtHead
//
//  Synopsis:
//      Construct an empty junction from its first head chain
//
//  Notes:
//      The fOnRight argument is there for performance, to eliminate test ing it
//      later
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CJunction::ProcessAtHead(
    __inout_ecount(1) CChain *pHead,
        // The chain to become the first head
    __inout_ecount_opt(1) CChain *pLeft,
        // The junction's left neighbor (NULL OK)
    __inout_ecount_opt(1) CChain *pRight,
        // The junction's right neighbor (NULL OK)
    bool fIsOnRightChain)  // =true if the junction lies on the right chain
{
    Assert(IsEmpty()); // Otherwise we shouldn't be called
    Assert(pHead);

    HRESULT hr;

    // Initialize the junction's point and first head chain
    m_pLeftmostHead = m_pRightmostHead = pHead;
    m_pRepVertex = pHead->GetHead();
    m_pRight = pRight;
    m_pLeft = pLeft;

    // Grab chains that pass through this junction
    
    // In CActiveList::Locate we made sure that there are none on the left, but
    // this junction may lie on one or more chains on its right
    if (fIsOnRightChain)
    {
        CChain *pNewHead = NULL;
        Assert(pRight);
        
        m_pRightmostTail = m_pLeftmostTail = pRight;

        while (m_pRightmostTail)
        {
            IFC(m_pRightmostTail->SplitAtIncidentVertex(m_pRepVertex, 
                                                        m_refIntersectionPool,
                                                        pNewHead));

            // We have been called from CScanner::Activate. the junction is at the head of a
            // newly activated chain, and it lies on pRight.  If it coincides with pRight's
            // tail then CScanner::MoveOn should have called ProcessCandidate on that insdead
            // of activating this head. A junction would have been created by that, grabbing this
            // chain, and this head would have been grabbed to that junction, and we wouldn't be
            // here.  So the junction is not at the tail of m_pRightmostTail, and it should have
            // been truly split.
            QUIT_IF_NOT(pNewHead);

            // Insert this new piece that was split from chain as a head
            InsertHead(pNewHead);
            if (m_pRightmostTail->CoincidesWithRight())
            {
                m_pRightmostTail = m_pRightmostTail->GetRight();
            }
            else
            {
                break;
            }
        }

        if (m_pRightmostTail)
        {
            m_pRight = m_pRightmostTail->GetRight();
        }
        else
        {
            m_pRight = NULL;
        }
    }

    
    // There should be no tails to grab.  Any edge whose tail coincides with
    // the junction should been processed prior to activating this head.  That would
    // have invoked ProcessAtTail, which should have grabbed this head and activated 
    // it, and we should not be here
    Assert(!m_pRight  ||  !m_pRight->GetTail()->CoincidesWith(m_pRepVertex));
    Assert(!m_pLeft   ||  !m_pLeft->GetTail()->CoincidesWith(m_pRepVertex));

    // Grab and activate yet inactive heads from the master chain list
    for ( ; ; )
    {
        pHead = m_pOwner->GrabInactiveCoincidentChain(m_pRepVertex);
        if (pHead)
        {
           InsertHead(pHead);
        }
        else
        {
           break;
        } 
    }

    // Flush
    hr = THR(Flush());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::Cjunction::ProcessAtTail
//
//  Synopsis:
//      Construct an empty junction from its first tail chain
//
//  Notes:
//
//------------------------------------------------------------------------------
HRESULT
CScanner::CJunction::ProcessAtTail(
    __inout_ecount(1) CChain *pTail,
        // The chain to become the first tail
    __inout_ecount_opt(1) CChain *pLeft,
        // Its left neighbor (NULL OK)
    __inout_ecount_opt(1) CChain *pRight)
        // Its right neighbor (NULL OK)
{
    Assert(IsEmpty()); // Otherwise we shouldn't be called
    Assert(pTail);

    HRESULT hr;
    CChain *pHead;

    // Initialize the junction's point and first head chain
    m_pLeftmostTail = m_pRightmostTail = pTail;
    m_pRepVertex = pTail->GetTail();

    // If any one of pTail's neighbors goes through the junction then it should have been split by
    // pTail by now, and the same holds for their neighbors. Any active chains that goes through
    // the junction must therefore have its tail there, so here we grab it

    // Grab tails on the left
    for (m_pLeft = pLeft;
        m_pLeft  &&  m_pLeft->GetTail()->CoincidesWith(m_pRepVertex);
        m_pLeft = m_pLeft->GetLeft())
    {
        // Insert this chain at the junction as the leftmost tail
        m_pLeftmostTail = m_pLeft;
    }

    // Grab tails on the right
    for (m_pRight = pRight;
        m_pRight  &&  m_pRight->GetTail()->CoincidesWith(m_pRepVertex);
        m_pRight = m_pRight->GetRight())
    {
        // Insert this chain at the junction as the rightmost tail
        m_pRightmostTail = m_pRight;
    }

    // Grab and activate yet inactive heads from the master chain list
    for ( ; ; )
    {
        pHead = m_pOwner->GrabInactiveCoincidentChain(m_pRepVertex);
        if (pHead)
        {
           InsertHead(pHead);
        }
        else
        {
           break;
        } 
    }

    // Flush
    hr = THR(Flush());

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::Cjunction::Classify
//
//  Synopsis:
//      Classify all the heads and tails in the junction
//
//  Notes:
//      All the junction's chains are classified here as left, right or
//      redundant.  Classification depends on the fill rule (Alternate/winding).
//
//------------------------------------------------------------------------------
void
CScanner::CJunction::Classify()
{
    CChain *pChain = m_pLeftmostHead;

    // First pass, task-specific classifying (virtual)
    Assert(m_pClassifier);
    m_pClassifier->Classify(m_pLeftmostTail, m_pLeftmostHead, m_pLeft);

    // The leftmost head may now be redundant, so get the leftmost non-redundant chain
    pChain = GetLeftmostHead(CHAIN_REDUNDANT_MASK);

    // Second pass, mark non-redundant coincident chain pairs as redundant
    while (pChain)
    {
        CChain *pNext = pChain->GetRelevantRight(CHAIN_REDUNDANT_MASK);
        if (!pNext)
        {
            break;
        }
        if (pChain->CoincidesWith(pNext))
        {
            // Found a pair, mark both as coincident
            pChain->CancelWith(pNext);
            pChain = pNext->GetRelevantRight(CHAIN_REDUNDANT_MASK);
        }
        else
        {
            pChain = pNext;
        }
    }
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CJunction::Dump
//
//  Synopsis:
//      Debug Dump
//
//------------------------------------------------------------------------------
void
CScanner::CJunction::Dump() const
{
    MILDebugOutput(L"Junction Heads:\n");
    for (CChain *pChain = m_pLeftmostHead;    pChain;   pChain = pChain->GetRight())
    {
        MILDebugOutput(L"id=%d Head point: (%f, %f)\n", 
                       pChain->m_id,
                       pChain->GetHead()->GetApproxCoordinates().X,
                       pChain->GetHead()->GetApproxCoordinates().Y);

        if (pChain == m_pRightmostHead)
        {
            break;
        }
    }

    MILDebugOutput(L"Junction Heads:\n");
    for (CChain *pChain = m_pLeftmostHead;  pChain;  pChain = pChain->GetRight())
    {
        MILDebugOutput(L"id=%d Head point: (%f, %f)\n", 
                       pChain->m_id,
                       pChain->GetHead()->GetApproxCoordinates().X,
                       pChain->GetHead()->GetApproxCoordinates().Y);

        if (pChain == m_pRightmostHead)
        {
            break;
        }
    }
}
#endif
//--------------------------------------------------------------------------------------------------

//                          Implementation of CClassifier

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CClassifier::Classify
//
//  Synopsis:
//      First pass of classifying the heads in the junction.
//
//  Notes:
//      All the junction's head chains are classified here as left, right or
//      redundant.  Classification is based strictly on the fill rule
//      (Alternate/winding), ignoring redundancy that stems from coincidence.
//
//------------------------------------------------------------------------------
void
CScanner::CClassifier::Classify(
    __inout_ecount(1) CChain *pLeftmostTail,
        // The junction's leftmost tail
    __inout_ecount(1) CChain *pLeftmostHead,
        // The junction's leftmost head
    __inout_ecount_opt(1) CChain *pLeft)
        // The chain to the left of the junction (possibly NULL)
{
    Assert(pLeftmostHead);    // Shouldn't be called oterhwise
        
    CChain *pChain = pLeftmostTail;

    // Tail chains have already been classified when they were heads. So if the junction
    // has tail chain then the leftmost head must be of the same type as the leftmost tail
    if (pChain)  // There is a tail chain 
    {
        // Copy its type 
        pLeftmostHead->Continue(pChain);

        // Move to the next head chain
        pLeft = pLeftmostHead;
        pChain = pLeftmostHead->GetRight();
    }
    else // No tail to copy from, we'll decide according to the chain on our left (or its absence)
    {
        pChain = pLeftmostHead;
    }

    // Classify the remaining heads
    while (pChain)
    {
        pChain->Classify(pLeft);

        pLeft = pChain;
        pChain = pChain->GetRight();
    }
}
//--------------------------------------------------------------------------------------------------

                        // Implementation of CScanner

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CScanner
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CScanner::CScanner()
    :m_oJunction(&m_oClassifier,
     m_oIntersectionPool),
     m_fDone(false),
     m_ptCenter(0.0, 0.0),
     m_fCachingCurves(false),
     m_rTolerance(DEFAULT_FLATTENING_TOLERANCE)
{
    m_oJunction.SetOwner(this);
    m_rScale = m_rInverseScale = 1.0;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::CScanner
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CScanner::CScanner(double rTolerance)
    :m_oJunction(&m_oClassifier,
     m_oIntersectionPool),
     m_fDone(false),
     m_ptCenter(0.0, 0.0),
     m_fCachingCurves(false),
    m_rTolerance(rTolerance)
{
    m_oJunction.SetOwner(this);
    m_rScale = m_rInverseScale = 1.0;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SetWorkspaceTransform
//
//  Synopsis:
//      Set up the transform to and from the scanner workspace
//
//  Notes:
//      The input points are converted to integers represented as doubles.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SetWorkspaceTransform(
    __in_ecount(1) const CMilRectF &rect,// Geometry bounds
    __out_ecount(1) bool &fDegenerate)      // =true if there is nothing to scan
{
    HRESULT hr = S_OK;

    Assert(rect.IsWellOrdered());

    // convert to double before finding width and height to avoid overflow
    double rGeometryWidth = static_cast<double>(rect.right) - rect.left;
    double rGeometryHeight = static_cast<double>(rect.bottom) - rect.top;

    double rGeometryExtents = max(rGeometryWidth, rGeometryHeight);

    if (!_finite(rGeometryExtents))
    {
        IFC(WGXERR_BADNUMBER);
    }

    // Set up a transformation to the scanner's Integer30 space
    fDegenerate = (rGeometryExtents < FLT_MIN);
    if (!fDegenerate)   // This shape degenerates to a point
    {
        m_rScale = RobustIntersections::LARGESTINTEGER26 / rGeometryExtents;
        m_rInverseScale = 1.0 / m_rScale;

        m_ptCenter.X = (static_cast<GpReal>(rect.left) + rect.right) / 2;
        m_ptCenter.Y = (static_cast<GpReal>(rect.top) + rect.bottom) / 2;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::Scan
//
//  Synopsis:
//      The main scanning loop
//
//------------------------------------------------------------------------------
HRESULT
CScanner::Scan()

{
    HRESULT hr = S_OK;
    m_fDone = false;
 
//  To investigate a problem, record the value of g_iTestCount when the problem was detected, and
//  then uncomment the following 2 lines, set the value, build and run again.

    // if (g_iTestCount == the value);
    //     g_fScannerTrace = true;

    // Scan all the vertices
    while (!m_fDone)
    {
        IFC(MoveOn());
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::ProcessCandidate
//
//  Synopsis:
//      Move the cursor down the chain and process the vertex there.
//
//  Notes:
//      We move first and then process.  Thus the cursor of an active chain is
//      always on the the lowest vertex that has already been processed.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::ProcessCandidate(
    __inout_ecount(1) CChain *pChain)
        // The chain whose current edge we're processing
{
    Assert(pChain);           // We checked before invoking this method
    
    HRESULT hr;
    bool fNeighborWasSplit;

    SCAN_TRACE(L"\nStart ProcessCandidate ", pChain->m_id);

    if (pChain->IsAtItsLastEdge())
    {
        hr = THR(m_oJunction.ProcessAtTail(pChain, pChain->GetLeft(), pChain->GetRight()));
        goto Cleanup;
    }

    m_oCandidates.Pop();
    pChain->MoveOn(); 
    
    // Look for intersection with pChain's current edge.
    
    // On the left
    IFC(SplitNeighbor(pChain, pChain->GetLeft(), fNeighborWasSplit));
    if (fNeighborWasSplit)
    {
        IFC(SplitCoincidentChainsLeftOf(pChain->GetLeft()));
    }

    // On the right
    IFC(SplitNeighbor(pChain, pChain->GetRight(), fNeighborWasSplit));
    if (fNeighborWasSplit)
    {
        IFC(SplitCoincidentChainsRightOf(pChain->GetRight()));
    }

    // Do your thing on this chain at its current edge (virtual method)
    if (!pChain->IsRedundant(CHAIN_REDUNDANT_MASK))
    {
        IFC(ProcessCurrentVertex(pChain));
    }

    // Re-insert chain in candidate list
    InsertCandidate(pChain);
        
Cleanup:
    SCAN_TRACE(L"\nEnd PrcessCandidate ", pChain->m_id);
    VALIDATE_AT(pChain->GetCurrentEdgeBase());

    RRETURN(hr);
}

///+-------------------------------------------------------------------------------------------------
//
//  Member:     CScanner::TerminateBatch
//
//  Synopsis:   Remove a left-to-right active segment from the active and candidate lists
//
//  Notes:      
//
//--------------------------------------------------------------------------------------------------
void
CScanner::TerminateBatch(
    __inout_ecount(1) CChain *pLeft,
        // The leftmost chain to terminate
    __inout_ecount(1) CChain *pRight)
        // The rightmost chain to terminate
{
    m_oActive.Remove(pLeft, pRight);
    
    CChain *pChain = pLeft;  
    while (pChain)  
    {
        m_oCandidates.Remove(pChain);
        if (pChain == pRight)
        {
            break;
        }
        pChain = pChain->GetRight();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::Activate
//
//  Synopsis:
//      Activate a chain
//
//  Notes:
//      Insert it into the Active and Candidate lists
//
//------------------------------------------------------------------------------
HRESULT
CScanner::Activate(
    __inout_ecount(1) CChain *pChain)
        // The chain to activate
{
    HRESULT hr;
    CChain *pRight = NULL;
    CChain *pLeft = NULL;
    bool fJunctionIsOnRightChain;

    Assert(pChain);
    Assert(m_oJunction.IsEmpty());

    SCAN_TRACE(L"\nStart Activate ", pChain->m_id);
    
    fJunctionIsOnRightChain = m_oActive.Locate(pChain, pLeft, pRight);

    hr = THR(m_oJunction.ProcessAtHead(pChain, pLeft, pRight, fJunctionIsOnRightChain));

    SCAN_TRACE(L"\nExit Activate ", pChain->m_id);
    VALIDATE_AT(pChain->GetHead());
   
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::MoveOn
//
//  Synopsis:
//      Move to the next highest vertex unprocessed vertex
//
//  Notes:
//                    The next vertex in line for processing is the highest unprocessed vertex. It may be 
//                    the highest head of an unprocessed chain, or the highest current-edge-tip among the   
//                    the active chains, which is picked from the candidate list.
//      //
//
//------------------------------------------------------------------------------
HRESULT
CScanner::MoveOn()
{
    HRESULT hr = S_OK;
    
    CChain *pTopInactive = m_oChains.GetNextChain();
    CChain *pCandidate = m_oCandidates.GetTop();
    Assert(!m_fDone);
     
    if (pTopInactive)  // There is an unprocessed chain
    {
        if (pCandidate) // There is an active candidate
        {
            // Choose which one to process first.
            COMPARISON pos = pCandidate->GetCurrentEdgeTip()->CompareWith(pTopInactive->GetHead());
            if (pos == C_STRICTLYGREATERTHAN)
            {
                // The candidate is higher than the top head
                IFC(ProcessCandidate(pCandidate));
            }
            else if ((pos == C_EQUAL)  &&  pCandidate->IsAtItsLastEdge())
            {
                // They coincide, and either one will generate a junction that will eventually 
                // do the same thing, but populating a junction is more efficient when it starts
                // from a tail, so give preference to that
                IFC(ProcessCandidate(pCandidate));
            }
            else
            {
                // The top head is higher than or coincident with the top candidate
                m_oChains.Pop();
                IFC(Activate(pTopInactive));
            }
        }
        else
        {
            // There are no active chains, so activate one
            m_oChains.Pop();
            IFC(Activate(pTopInactive));
        }
    }
    else if (pCandidate)
    {
        // There are no new chains to activate, process the top candidate
        IFC(ProcessCandidate(pCandidate));
    }
    else    // There is nothing left to process, we're done
    {
        m_fDone = true;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::InsertCandidate
//
//  Synopsis:
//      Insert a chain in the candidate list
//
//  Notes:
//      THIS SHOULD BE CALLED ONLY ON A CHAIN THAT IS IN THE ACTIVE LIST!!!
//      Otherwise just call m_oCandidates.Insert
//
//------------------------------------------------------------------------------
void
CScanner::InsertCandidate(
    __inout_ecount(1) CChain *pChain)
        // The chain to activate
{
    if (FAILED(m_oCandidates.Insert(pChain)))
    {
        // This shouldn't happen, so
        TEST_ALARM;

        // But we need to clean up
        m_oActive.Remove(pChain, pChain);   
        // otherwise the candidate list will be inconsitent with the active list
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitChainAtIntersection
//
//  Synopsis:
//      Split a chain at a given intersection
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitChainAtIntersection(
        __inout_ecount(1) CChain *pChain,
            // A chain to split
        __in_ecount(1) const CIntersectionResult &result)
            // Intersection info
{
    HRESULT hr = E_FAIL;
    CChain  *pSplit = NULL;

    // Don't call me if if the intersection is at my tail (and there is nothing to split
    Assert(!pChain->IsATailIntersection(result));

    // Split the chain
    IFC(pChain->SplitAtIntersection(result, pSplit));
    Assert(pSplit);
    
    // Insert the bottom portion in the main chain list
    IFC(m_oChains.Insert(pSplit));
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitChainAtCurrentEdgeTip
//
//  Synopsis:
//      Split a chain at the tip of the current edge
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitChainAtCurrentEdgeTip(
    __inout_ecount(1) CChain *pChain)
        // The chain to split
{
    HRESULT hr;

    CChain *pSplit = NULL;

    // Split the chain
    IFC(pChain->SplitAtCurrentEdgeTip(pSplit));

    if (pSplit) // Insert the bottom portion in the main chain list
    {        
        IFC(m_oChains.Insert(pSplit));
    }
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitChainAtIncidentVertex
//
//  Synopsis:
//      Split a chain at a vertex from another chain that lies on it
//
//  Notes:
//      The chain is split in place
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitChainAtIncidentVertex(
    __inout_ecount(1) CChain *pChain,
        // The chain to split
    __in_ecount(1) const CVertex *pVertex)
        // The incident vertex
{
    HRESULT hr;

    CChain *pSplit = NULL;

    // Split the chain
    IFC(pChain->SplitAtIncidentVertex(pVertex, m_oIntersectionPool, pSplit));

    if (pSplit) // Insert the bottom portion in the main chain list
    {        
        IFC(m_oChains.Insert(pSplit));
    }
    
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitChainAtSegmentIntersection
//
//  Synopsis:
//      Split a chain where a given segment intersects it
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitChainAtSegmentIntersection(
    __inout_ecount(1) CChain *pChain,
        // The chain to split
    __in_ecount(1) const CVertex *pSegmentBase)
        // The segment's base point
{
    HRESULT hr;
    CLineSegmentIntersection *pIntersection;
    IFC(m_oIntersectionPool.AllocateIntersection(pIntersection));
    
    {
        // Separate scope needed because of the IFC abouve
        CIntersectionResult result(pIntersection);
        bool fIntersect;

        IFC(pChain->IntersectWithSegment(pSegmentBase, fIntersect, result));
        Assert(fIntersect);

        if(!pChain->IsATailIntersection(result))
        {
            IFC(SplitChainAtIntersection(pChain, result));
        }
    }
Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitCandidate
//
//  Synopsis:
//      Split a chain that is in the candidate list
//
//  Notes:
//      This should only be called on a chain that is in the candidate and
//      active lists. Once split, the chain split point will be the chain's a
//      new candidate vertex. The chain's position in the candidate list
//      therefore needs to be updated accordingly to its new candidate's height.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitCandidate(
        __inout_ecount(1) CChain *pChain,
            // A chain to split
        __in_ecount(1) const CIntersectionResult &result)
            // The split point
{
    HRESULT hr;

    m_oCandidates.Remove(pChain);
    IFC(SplitChainAtIntersection(pChain, result));
    InsertCandidate(pChain);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitNeighbor
//
//  Synopsis:
//      Split a pair of chains if they intersect, 1 of whom is in the candidate
//      list
//
//  Notes:
//      This method is for the case that one of the chains is in the candidate
//      list and other is not.  (When split, a chain needs to be repositioned in
//      the candidate list if there, because after the split its candidate has
//      changed.)
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitNeighbor(
    __inout_ecount(1) CChain *pChain,
        // A chain that is not in the candidate list
    __inout_ecount_opt(1) CChain *pNeighbor,  
        // Its neighbor that is in the candidate list (NULL OK)
    __out_ecount(1) bool &fSplitNeighbor)
        // =true if pNeighbor was split
{
    HRESULT hr = S_OK;
    bool fIntersect = false;
    bool fSplitChain = false;
    fSplitNeighbor = false;

    Assert(pChain);

    if (pNeighbor)
    {
        CLineSegmentIntersection *pIntersection;
        IFC(m_oIntersectionPool.AllocateIntersection(pIntersection));

        CIntersectionResult oResultOnChain(pIntersection);
        CIntersectionResult oResultOnNeighbor(pIntersection);

        IFC(pChain->Intersect(pNeighbor, fIntersect, oResultOnChain, oResultOnNeighbor));
        if (fIntersect)
        {
            fSplitChain = !pChain->IsATailIntersection(oResultOnChain);
            if (fSplitChain)
            {
                IFC(SplitChainAtIntersection(pChain, oResultOnChain));
            }

            fSplitNeighbor = !pNeighbor->IsATailIntersection(oResultOnNeighbor);
            if (fSplitNeighbor)
            {
                hr = THR(SplitCandidate(pNeighbor, oResultOnNeighbor));
            }
        }
        
        if (!fSplitChain  &&  !fSplitNeighbor)
        {
            m_oIntersectionPool.Free(pIntersection);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitCoincidentChainsLeftOf
//
//  Synopsis:
//      Split chains that are coincident with pChain on the left
//
//  Notes:
//      This method is called after pChain has been split to accordingly split
//      all chains further left that were coincident with pChain before the
//      split.  It assumes that these chains are in the candidate list.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitCoincidentChainsLeftOf(
    __inout_ecount(1) CChain *pChain)
        // A that has just been split
{
    HRESULT hr = S_OK;
    CChain *pLeft = pChain->GetLeft();

    while (pLeft &&  pLeft->CoincidesWithRight())
    {
        m_oCandidates.Remove(pLeft);
        IFC(SplitChainAtIncidentVertex(pLeft, pChain->GetTail()));
        InsertCandidate(pLeft);
        
        pLeft = pLeft->GetLeft();
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitCoincidentChainsRightOf
//
//  Synopsis:
//      Split chains that are coincident with pChain on the right
//
//  Notes:
//      This method is called after pChain has been split to accordingly split
//      all chains further right that were coincident with pChain before the
//      split. It assumes that these chains are in the candidate list.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitCoincidentChainsRightOf(
    __inout_ecount(1) CChain *pChain)
        // A chain that has just been split
{
    HRESULT hr = S_OK;
    CChain *pRight = pChain;

    while (pRight->CoincidesWithRight())
    {
        pRight = pRight->GetRight();

        m_oCandidates.Remove(pRight);
        IFC(SplitChainAtIncidentVertex(pRight, pChain->GetTail()));
        InsertCandidate(pRight);
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitPairAtIntersection
//
//  Synopsis:
//      Split a pair of chains if they intersect, when both are in the candidate
//      list
//
//  Notes:
//      This method is for the case where both chain are in the candidate list. 
//      (When split, a chain needs to be repositioned there, because after the
//      split its candidate has changed.)
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitPairAtIntersection(
    __inout_ecount(1) CChain *pLeft,
        // The left chain
    __inout_ecount(1) CChain *pRight)
        // The right chain
{
    HRESULT hr = S_OK;

    if (pLeft  &&  pRight)
    {
        bool fIntersect, fLeftIsSplit, fRightIsSplit;
        CLineSegmentIntersection *pIntersection;
        IFC(m_oIntersectionPool.AllocateIntersection(pIntersection));

        CIntersectionResult oResultOnLeft(pIntersection);
        CIntersectionResult oResultOnRight(pIntersection);

        IFC(pLeft->Intersect(pRight, fIntersect, oResultOnLeft, oResultOnRight));
        if (fIntersect)
        {
            // The new neighbors do intersect, split both of them at their intersection if
            // there is something to split

            fLeftIsSplit = !pLeft->IsATailIntersection(oResultOnLeft);
            if (fLeftIsSplit)
            {
                IFC(SplitCandidate(pLeft, oResultOnLeft));
                IFC(SplitCoincidentChainsLeftOf(pLeft));
            }

            fRightIsSplit = !pRight->IsATailIntersection(oResultOnRight);
            if (fRightIsSplit)
            {
                IFC(SplitCandidate(pRight, oResultOnRight));
                IFC(SplitCoincidentChainsRightOf(pRight));
            }

            if (!fLeftIsSplit  &&  !fRightIsSplit)
            {
                m_oIntersectionPool.Free(pIntersection);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitAtIntersections
//
//  Synopsis:
//      Split the new head-chains with their left- and right-bracketing
//      neighbors, and among themselves where they are collinear.
//
//  Notes:
//      This is a contiguous left-to-right list of chains with coincident heads,
//      about to be activated.  Where a pair of these edges are collinear, we
//      split the longer one so that it will fully coincides with the shorter
//      one.
//
//  Warning:
//      This method is called only from CJunction::Flush. Calling it from
//      elsewhere should be done with caution!!!  pLeft, pRight and all the
//      chains between them are presumed not to be active yet. It would be wrong
//      to call it otherwise because if they are active then their location in
//      the Candidate list should be adjusted after splitting.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitAtIntersections(
    __inout_ecount(1) CChain *pLeft,
        // The leftmost chain
    __inout_ecount(1) CChain *pRight,
        // The rightmost chain
    __inout_ecount_opt(1) CChain *pPrevious,
        // The chain that will become pLeft's left neighbor (NULL OK)
    __inout_ecount_opt(1) CChain *pNext)
        // The chain that will become pRight's right neighbor (NULL OK)
{
    HRESULT hr;
    CChain  *pChain;
    bool fNeighborWasSplit;

    Assert(pLeft);
    Assert(pRight);

    // Split where the leftmost chain intersects its left neighbor
    IFC(SplitNeighbor(pLeft, pPrevious, fNeighborWasSplit));
    if (fNeighborWasSplit)
    {
        IFC(SplitCoincidentChainsLeftOf(pPrevious));
    }

    // Split where the rightmost new chain intersects its right neighbor
    IFC(SplitNeighbor(pRight, pNext, fNeighborWasSplit));
    if (fNeighborWasSplit)
    {
        IFC(SplitCoincidentChainsRightOf(pNext));
    }

    // Split at coincident intersections of the new chains among themselves.
    for (pChain = pLeft;  pChain  &&  pChain != pRight;  pChain = pChain->GetRight())
    {
        IFC(SplitAtCoincidentIntersection(pChain));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::ActivateBatch
//
//  Synopsis:
//      Insert all the head-chains in the current junction in the active and
//      candidate lists
//
//------------------------------------------------------------------------------
void
CScanner::ActivateBatch(
    __inout_ecount(1) CChain *pLeft,
        // The leftmost chain to insert
    __inout_ecount(1) CChain *pRight,
        // The rightmost chain to insert
    __inout_ecount_opt(1) CChain *pPrevious,
        // The chain to insert after (NULL OK)
    __inout_ecount_opt(1) CChain *pNext)
        // The chain to insert before (NULL OK)
{
    // Should not be called with an empty list
    Assert(pLeft);

    // Insert in the active list
    m_oActive.Insert(pLeft, pRight, pPrevious, pNext);

    // Insert in the candidate list
    while (pLeft)
    {
        InsertCandidate(pLeft);
        if (pLeft == pRight)
        {
            break;
        }
        pLeft = pLeft->GetRight();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::SplitAtCoincidentIntersection
//
//  Synopsis:
//      Split 2 head-coincident chains at the end of their overlap if they are
//      collinear
//
//  Notes:
//      If the edges are collinear then we split both chains. The one with the
//      shorter edge is split at the tip of that edge. The other is split at
//      that tip on its If both edges are of the same length then both chains
//      are split at their tips.
//
//  Warning:
//      This method is called only from SplitAtIntersections.  Calling it from
//      elsewhere should be done with caution!!! The chains that are split here
//      are presumed not to be active yet. It would be wrong to call it
//      otherwise, because if they are active then their location in the
//      Candidate list should be adjusted after splitting.
//
//------------------------------------------------------------------------------
HRESULT
CScanner::SplitAtCoincidentIntersection(
    __inout_ecount(1) CChain *pChain)
        // A chain
{
    Assert(pChain);

    HRESULT hr = S_OK;
    CChain *pRight = pChain->GetRight();
    
    if (!pRight)
        goto Cleanup;

    // This method is eventually called after processing a junction. Both chains
    // point to vertices that generated that junction. Therefore they share
    // a common point.
    Assert(pChain->GetCurrentEdgeBase()->CoincidesWith(pRight->GetCurrentEdgeBase()));

    // First, let's find out if the support segments for the two edges are collinear.
    double ab[4] = {pChain->GetCurrentSegmentBase()->GetExactCoordinates().X,
                    pChain->GetCurrentSegmentBase()->GetExactCoordinates().Y,
                    pChain->GetCurrentSegmentTip()->GetExactCoordinates().X,
                    pChain->GetCurrentSegmentTip()->GetExactCoordinates().Y};

    double c[2] = {pRight->GetCurrentSegmentTip()->GetExactCoordinates().X,
                   pRight->GetCurrentSegmentTip()->GetExactCoordinates().Y};

    if (CLineSegmentIntersection::LocatePointRelativeToLine(c, ab) == 
        CLineSegmentIntersection::SIDE_INCIDENT)
    {
        // We know the support segments for the edges are collinear. We also know
        // that they share a common vertex at the junction. Therefore we also
        // know that we have overlapping collinear edges. Now we need to do a little 
        // more work to find where to split these edges.
        COMPARISON  compare = pChain->GetCurrentEdgeTip()->CompareWith(pRight->GetCurrentEdgeTip());
        if (compare == C_STRICTLYGREATERTHAN)
        {
            // The edge on the left is shorter, split both chains at its edge tip
            IFC(SplitChainAtCurrentEdgeTip(pChain));
            IFC(SplitChainAtIncidentVertex(pRight, pChain->GetCurrentEdgeTip()));
        }
        else if (compare == C_STRICTLYLESSTHAN)
        {
            CChain *pLeft = pChain;

            // The edge on the right is shorter, split it at its edge tip 
            IFC(SplitChainAtCurrentEdgeTip(pRight));

            // Split the left one there, as well as all new chains on the left that 
            // were coincident with it.  
            do
            {
                IFC(SplitChainAtIncidentVertex(pLeft, pRight->GetTail()));
                pLeft = pLeft->GetLeft();
            }
            while (pLeft  &&  pLeft->CoincidesWithRight());

        }
        else
        {
            // The edges overlap exactly, base and tip
            Assert(pChain->GetCurrentEdgeTip()->CoincidesWith(pRight->GetCurrentEdgeTip()));

            // Split both chains at the tips of their current edges
            IFC(SplitChainAtCurrentEdgeTip(pChain));
            IFC(SplitChainAtCurrentEdgeTip(pRight));
        }

        pChain->SetCoincidentWithRight();
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::AddCurve
//
//  Synopsis:
//      Accept an input curve
//
//------------------------------------------------------------------------------
HRESULT
CScanner::AddCurve(
    __in_ecount(3) const GpPointR *ptNew)
        // The last 3 Bezier points of the curve (we already have the first one)
{
    HRESULT hr;
    CBezierFlattener flattener(&m_oChains, m_rTolerance * m_rScale);

    if (m_fCachingCurves)
    {
        // Cache the input curve (in caller coordinates) for retrieval
        IFC(m_oChains.AddCurve(m_ptLastInput, ptNew));
    }


    //
    // Flatten the curve in scanner workspace coordinates.
    // The coordinates of the curve are not Integer30s, because flattening would not preserve 
    // that.  We must round after flattening (in the AcceptPoint callback).
    //

    flattener.SetPoint(0, m_oChains.GetCurrentPoint());

    for (UINT i = 0;  i < 3;  i++)
    {
        GpPointR pt;
        IFC(ConvertToInteger30(ptNew[i], OUT pt));
        flattener.SetPoint(i + 1, pt);
    }

    IFC(flattener.Flatten(false /* => no tangents */));

    m_ptLastInput = ptNew[2];

Cleanup:
    m_oChains.SetNoCurve();
    
    RRETURN(hr);
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::Trace
//
//  Synopsis:
//      Debug tracing which chain is being processed
//
//------------------------------------------------------------------------------
void
CScanner::Trace(
    __in PCWSTR pStr,
        // String to dump
    int id) const
        // Chain ID
{
    if (g_fScannerTrace)
    {
        OutputDebugString(pStr);
        MILDebugOutput(L"id=%d\n", id); 

        m_oChains.Dump();
        MILDebugOutput(L"\n"); 
        m_oActive.Dump(IsBooleanOperation());
        MILDebugOutput(L"\n"); 
        m_oCandidates.Dump();
        MILDebugOutput(L"\n"); 
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanner::ValidateAt
//
//  Synopsis:
//      Verify that we are at a consistent state at the current level of
//      processing
//
//------------------------------------------------------------------------------
void
CScanner::ValidateAt(
    __in_ecount(1) const CVertex *pVertex) const
        // The recently processed vertex
{
    m_oChains.Validate();
    if (pVertex)
    {
        m_oActive.Validate(pVertex);
    }
    m_oCandidates.Validate();
    m_oActive.AssertConsistentWith(m_oCandidates);

    for (UINT i = 0; i < m_oCandidates.GetCount(); ++i)
    {
        if (!m_oActive.Includes(m_oCandidates[i]))
        {
            MILDebugOutput(L"Consistency check failed on %d.\n", i);

            m_oCandidates.Dump();
            m_oActive.Dump();

            Assert(false);
            break;
        }
    }
}
#endif


