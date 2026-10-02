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
//------------------------------------------------------------------------------

#pragma once

using namespace RobustIntersections;


#if DBG
    extern int g_iTestCount;
    extern bool g_fScannerTrace;
    #define SET_DBG_ID(id) pNew->m_id=(id)++
    #define SCAN_TEST_INIT g_iTestCount++
    #define SCAN_TRACE(x, y) Trace((x), (y))
#else
    #define SCAN_TEST_INIT
    #define SET_DBG_ID(id)
    #define SCAN_TRACE(x, y)
#endif


//--------------------------------------------------------------------------------------------------

//  Private debug mode with special debugging utilities

//--------------------------------------------------------------------------------------------------

// The scanner relies on various combinatorial assumptions, e.g. that the total number of
// chains at every junction must be even.  Some of them may not be crucial, so you may want to
// know when they fail when you are testing the code, but not cause assertions to trip in stress
// tests.  Others are severe, and should never happen.  But this is a complex algorithm, so instead
// of asserting we check the assumptions at runtime, and if the check fails we quit and return
// an error that will cause us to fall back to software rasterizing. But when testing the code,
// you may want to be alerted when that happen.  To do that, uncomment the following and recompile.
// In addition, this will enable various consistency checks that may detect early on when the system
// gets into an inconsistent state.

//#define SCAN_TESTING

#ifdef SCAN_TESTING
    // Additional validity checks for test runs
    #define TEST_ALARM Assert(false)
#   ifndef ASSERT_VALID
    #define ASSERT_VALID AssertValid()
#   endif
    #define VALIDATE_AT(p) {if (SUCCEEDED(hr)) ValidateAt(p);}
#else
    #define TEST_ALARM
#   ifndef ASSERT_VALID
    #define ASSERT_VALID
#   endif
    #define VALIDATE_AT(y)
#endif

#define QUIT_IF_NOT(x)\
    if (!(x))\
    {\
        TEST_ALARM;\
        IFC(WGXERR_SCANNER_FAILED);\
    }


// Chain flag values
extern const WORD CHAIN_REVERSED;
extern const WORD CHAIN_COINCIDENT;

extern const WORD CHAIN_SIDE_RIGHT;
extern const WORD CHAIN_SELF_REDUNDANT;
extern const WORD CHAIN_CANCELLED;

extern const WORD CHAIN_SHAPE_MASK;
extern const WORD CHAIN_BOOL_FLIP_SIDE;
extern const WORD CHAIN_BOOL_REDUNDANT;

extern const WORD CHAIN_REDUNDANT_MASK;
extern const WORD CHAIN_REDUNDANT_OR_CANCELLED;
extern const WORD CHAIN_SELF_TYPE_MASK;
extern const WORD CHAIN_INHERITTED_MASK;

// Location of a point relative to a line segment. 
// Assuming a right-handed coordinate system, CLineSegmentIntersection defines sides for the
// obserever along the line's direction - from the first defining point toward the second one.
// The scanner defines sides for the observer of a page where y points up.  Since scanner edges,
// left of the line is right for the observer. 
#define SCANNER_LOCATION   CLineSegmentIntersection::SIDEINDICATOR
#define SCANNER_LEFT       CLineSegmentIntersection::SIDE_RIGHT
#define SCANNER_INCIDENT   CLineSegmentIntersection::SIDE_INCIDENT
#define SCANNER_RIGHT      CLineSegmentIntersection::SIDE_LEFT

//--------------------------------------------------------------------------------------------------
//  Utilities
//--------------------------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Function:
//      AreAscending
//
//  Synopsis:
//      Determines whether ptFirst <= ptSecond
//
//  Returns:
//      true if ptFirst <= ptSecond
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE bool
AreAscending(
    __in_ecount(1) const GpPointR &ptFirst,
        // First point
    __in_ecount(1) const GpPointR &ptSecond
        // Second point
    )
{
    return ((ptFirst.Y < ptSecond.Y) || ((ptFirst.Y == ptSecond.Y) && (ptFirst.X < ptSecond.X)));
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ComparePoints
//
//  Synopsis:
//      Compares two points
//
//  Returns:
//      C_STRICTLYESSTHAN     if ptFirst < ptSecond
//      C_EQUAL               if ptFirst = ptSecond
//      C_STRICTLYGREATERTHAN if ptFirst > ptSecond
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE COMPARISON
ComparePoints(
    __in_ecount(1) const GpPointR &ptFirst,
        // First point
    __in_ecount(1) const GpPointR &ptSecond
        // Second point
    )
{
    COMPARISON compare;

    if (ptFirst.Y < ptSecond.Y)
    {
        compare = C_STRICTLYLESSTHAN;
    }
    else if (ptFirst.Y == ptSecond.Y)
    {
        if (ptFirst.X < ptSecond.X)
        {
            compare = C_STRICTLYLESSTHAN;
        }
        else if (ptFirst.X == ptSecond.X)
        {
            compare = C_EQUAL;
        }
        else
        {
            compare = C_STRICTLYGREATERTHAN;
        }
    }
    else
    {
        compare = C_STRICTLYGREATERTHAN;
    }

    return compare;
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CScanner
//
//  Synopsis:
//      Base class for various scan operations
//
//  Notes:
//
//------------------------------------------------------------------------------
class CScanner : public IPopulationSink
{
public:
                        // Helper classes

class CChainPool;
class CActiveList;
class CClassifier;
class CVertex;

    
    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CIntersectionPool
    //
    //  Synopsis:
    //      A memory pool for intersection records
    //
    //--------------------------------------------------------------------------
    class CIntersectionPool   :   public TMemBlockBase<CLineSegmentIntersection>
    {
    public:
        CIntersectionPool()
        {
#if DBG
            m_id = 1;
#endif
        }

        virtual ~CIntersectionPool()
        {
        }

        HRESULT AllocateIntersection(
            __deref_out_ecount(1) CLineSegmentIntersection *&pNew);
                // The allocated record

#if DBG
        UINT    m_id;  // Debugging ID
#endif
    };  // End of definition of CIntersectionPool

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CEdgeIntersection
    //
    //  Synopsis:
    //      Adds bookeeping utilities to the CLineSegmentIntersection class.
    //
    //  Notes:
    //      The class CLineSegmentIntersection is very non-symmetrical; knowing
    //      how to interpret an intersection depends on knowing which of the two
    //      intersecting line segment it is associated with. This class is
    //      adding bookeeping facilities to help with that tedium.
    //
    //--------------------------------------------------------------------------
    class CEdgeIntersection
    {
    public:
        // Construction
        void Initialize();

        void Copy(
            __in_ecount(1) const CEdgeIntersection &other);

        // Setting edge info
        void SetEdgeLocation(CLineSegmentIntersection::LOCATION loc)
        {
            m_eLocationOnEdge = loc;
        }

        // Queries:        
        CLineSegmentIntersection::LOCATION GetEdgeLocation() const
        {
            return m_eLocationOnEdge;
        }

        __outro_ecount(1) const CLineSegmentIntersection& GetIntersection() const
        {
            Assert(m_pIntersection);
            return *m_pIntersection;
        }

        bool IsUnderlyingSegmentAB() const
        {
            Assert(m_eFlavor != eSegmentUnknown);
            return m_eFlavor == eSegmentAB;
        }

        __outro_ecount(1) const CVertex* GetCrossSegmentBase() const
        {
            Assert(m_pCrossSegmentBase);
            return m_pCrossSegmentBase;
        }

        double GetParameterAlongSegment() const;

        COMPARISON CompareWithIntersection(__in_ecount(1) const CEdgeIntersection &other) const
        {
            if (m_pIntersection == other.m_pIntersection)
                return C_EQUAL;     
            else
                return CLineSegmentIntersection::YXSortTransverseIntersectionPair(
                                                                        *m_pIntersection, 
                                                                        *other.m_pIntersection);
        }

        COMPARISON CompareWithPoint(__in_ecount(1) const GpPointR &pt) const
        {
            double e[2] = {pt.X, pt.Y};
            return CLineSegmentIntersection::YXSortTransverseIntersectionAndPoint(
                                                                        *m_pIntersection,
                                                                        e);
        }

        COMPARISON CompareWithSameSegmentIntersection(
            __in_ecount(1) const CEdgeIntersection &other
                // Other intersection to compare with
            ) const;

    protected:

        // Accomodate CLineSegmentIntersection segment bias
        typedef enum
        {
            eSegmentAB,         // Intersection was evaluated with underlying segment as "ab"
            eSegmentCD,         // Intersection was evaluated with underlying segment as "cd"
            eSegmentUnknown     // We don't know yet
        }   Flavor;

        // Data
        Flavor                              m_eFlavor;             // Flavor (AB or CD)
        CLineSegmentIntersection::LOCATION  m_eLocationOnEdge;     // Location on the edge
        const CVertex                       *m_pCrossSegmentBase;  // Base vertex for cross-segment
        CLineSegmentIntersection            *m_pIntersection;      // The intersection record
    };


    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CIntersectionResult
    //
    //  Synopsis:
    //      CEdgeIntersection with information about the outcome of an
    //      intersection
    //
    //  Notes:
    //      This class contains a nested segment intersection record.
    //
    //--------------------------------------------------------------------------
   class CIntersectionResult : public CEdgeIntersection
   {
   private:
       // Disallow construction without a line segment intersection record
       CIntersectionResult() {}

   public:
        CIntersectionResult(__in_ecount(1) CLineSegmentIntersection *pIntersection)
        {
            Assert(pIntersection);
            m_pIntersection = pIntersection;
        }

        CIntersectionResult(
            __in_ecount(1) const CIntersectionResult &other);
                // Object to copy

        bool IntersectSegments(
            __in_ecount(1) const CVertex *pABBase,
                // Base vertex for segment ab
            __in_ecount(1) const CVertex *pCDBase,
                // Base vertex for segment cd
            __out_ecount(1) CLineSegmentIntersection::LOCATION &eLocationOnAB,
                // Intersection location wrt AB
            __out_ecount(1) CLineSegmentIntersection::LOCATION &eLocationOnCD
                // Intersection location wrt CD
            );

        void FormDualIntersectionOnCD(
            __in_ecount(1) const CIntersectionResult &refOnAB,
                // Intersection on AB
            __in_ecount(1) const CVertex *pABBase);
                // Base vertex for segment ab

        bool IsExact() const { return m_fIsExact; }

        GpPointR GetExactCoordinates() const
        {
            Assert (m_fIsExact);
            return m_pt;
        }

   private:
        bool                     m_fIsExact;  // =true if intersection is at exact
        GpPointR                 m_pt;        // Intersection point if exact
   };

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CCurvePool
    //
    //  Synopsis:
    //      Memory pool for curve records
    //
    //--------------------------------------------------------------------------
   class CCurvePool   :   public TMemBlockBase<CBezier>
    {
    public:
        CCurvePool()
        {
            m_pCurrentCurve = NULL;
        }

        virtual ~CCurvePool()
        {
        }

        void SetNoCurve()
        {
            m_pCurrentCurve = NULL;
        }

        HRESULT AddCurve(
            __in_ecount(1) const GpPointR &ptFirst,
                // The curve's first point
            __in_ecount(3) const GpPointR *pPt);
                // The curve's remaining 3 points

        __outro_ecount(1) const CBezier *GetCurrentCurve() const
        {
            return m_pCurrentCurve;
        }

    // Data
    protected:
        CBezier *m_pCurrentCurve;
            // The current curve whose vertices are being entered

    };  // End of definition of CCurvePool


    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CVertex
    //
    //  Synopsis:
    //      Vertex based line-segment and edge information; used to build chains
    //
    //  Notes:
    //      Although CVertex instances individually represent single 2D points,
    //      they are linked together to form chains of edges. So CVertex
    //      sometimes represents the edge between it and the next verted down
    //      the chain, to which its m_pNext points.
    //
    //      The coordinates of the input geometry are rounded upon CVertrx
    //      construction and are considered exact.  The edges between them are
    //      called "segments".  Additional vertices are constructed as
    //      intersections of edges.  As an edge, an intersection CVertex has a
    //      "Base segment" (of which the edge is a part) and a "Cross Segment"
    //      That defines the extent of that part.  An intersection is thus
    //      always defined in terms the segments that underly the intersecting
    //      edges.
    //
    //      There are three types of CVertex: 
    //          * Original input "Endpoint" vertex, which is the "base" or the "tip of its
    //            undrlying segment.
    //          * Intersection vertex, which is the intersection of two segments.
    //          * Intersection vertex that falls on one one of its cross segment endpoints;
    //            it is still an intersection, but it is "exact" because its coordinates
    //            are known exactly.  
    //
    //      So the terminology is:
    //
    //      Segment:        an edge connecting original input vertices.
    //      Edge:           a portion of, or possible the whole segment between two
    //                      successive vertices.
    //      Base vertex:    the top-most (in Y-then-X order) vertex of an edge/segment.
    //      Tip:            the bottom-most, (in Y-then-X order) vertex of an edge/segment.
    //      Edge base:      the top-most, in YX order, vertex of an edge.
    //      Edge tip:       the bottom-most, in YX order, vertex of an edge.
    //
    //      Exact:          A vertex whose coordinates are origianl input, one of the 
    //      Approximate:    A vertex whose only exact definition is in terms of two
    //                      intersectinf segments.
    //
    //      Vertices are doubly-linked together into chains of edges in
    //      YX-decreasing order. A secondary set of parallel links are used to
    //      keep track of segments., Specifically each segment base vertex
    //      points to the segment's tip vertex, and each intersection vertex
    //      points to the segment's base vertex. These link make it possible to
    //      quickly access underlying-segment information from any edge along
    //      the chain.
    //
    //--------------------------------------------------------------------------
    class CVertex
    {

    public:
        // The class is only instantiated by the memory pool, so no constructor/destructor necessary
        void InitializeAtPoint(
            __in_ecount(1) const GpPointR &pt,
                // Vertex coordinates
            __in bool fIsEndpoint
                // true ==> endpoint, false ==> exact intersection
            );

        void InitializeAtIntersection(
            __in_ecount(1) const CEdgeIntersection  &refEdgeIntersect,
                // Edges intersection info
            __in_ecount(1) const GpPointR           &pt);
                // Approximate vertex coordinates

        void InitializeAsCopy(
            __in_ecount(1) const CVertex &pOther);
                // Copy construction

        ~CVertex()
        {
        }

        // Chain topology:
        void InsertAsHead(
            __in_pcount_inout_ecount(1,1) CVertex *&pHead);
                // Chain head, updated in-place

        void InsertAsTail(
            __in_pcount_inout_ecount(1,1) CVertex *&pTail);
                // Chain's tail, updated in-place

        void LinkEdgeTo(
            __inout_ecount_opt(1) CVertex *pNext);
            // The vertex/edge to link to (NULL okay)

        void Attach(
            __inout_ecount(1) CVertex *pHead);
                // Other chain's head

        // Edge and segment topology:
        __out_ecount(1) CVertex* GetNext() const
        {
            return m_pNext;
        }

        __out_ecount(1) CVertex* GetPrevious() const
        {
            return m_pPrevious;
        }

        __outro_ecount(1) const CVertex* GetSegmentBase() const
        {
            return IsSegmentEndpoint() ? this : m_oSegment.pBase;
        }

        __outro_ecount(1) const CVertex* GetSegmentTip() const
        {
            return IsSegmentEndpoint() ? m_oSegment.pTip : m_oSegment.pBase->m_oSegment.pTip;
        }

        // For intersections only; return cross segment
        __outro_ecount(1) const CVertex* GetCrossSegmentBase() const
        {
            Assert(!IsExact());
            return m_intersection.GetCrossSegmentBase();
        }

        // Smoothness
        bool IsSmoothJoin() const
        {
            return m_fSmoothJoin;
        }

        void SetSmoothJoin(bool val)
        {
            m_fSmoothJoin = val;
        }

        // Retrieving 2D geometry info:
        // Valid for any vertex
        __outro_ecount(1) const GpPointR &GetApproxCoordinates() const    
        {
            return m_pt;
        }

        // Valid for endpoints only
        __outro_ecount(1) const GpPointR &GetExactCoordinates() const
        {
            Assert(IsExact());
            return m_pt;
        }
        
        GpPointR GetSegmentBasePoint() const
        {
            return GetSegmentBase()->GetExactCoordinates();
        }

        GpPointR GetSegmentTipPoint() const
        {
            Assert(GetSegmentTip());
            return GetSegmentTip()->GetExactCoordinates();
        }

        // Return true if vertex is an endpoint
        bool IsSegmentEndpoint() const
        {
            return m_eType == eVTypeEndpoint;
        }

        // Return true if intersection at an original input point
        bool IsExactIntersection() const
        {
            return m_eType == eVTypeExactIntersect;
        }

        // Return true if vertex has exact coordinates
        bool IsExact() const
        {
            return IsSegmentEndpoint() || IsExactIntersection();
        }

        void SetCurveInfo(
            __in_ecount_opt(1) const CBezierFragment *pFragment
            )
        {
            // Must only ever be called once.
            Assert(!m_bezierFragment.Assigned());

            m_bezierFragment = *pFragment;
        }

        // Curve information
        bool HasCurve() const
        {
            return (m_bezierFragment.Assigned());
        }

        __outro const CBezierFragment &GetCurve() const
        {
            return m_bezierFragment;
        }

        void ClearCurve()
        {
            m_bezierFragment.Clear();
        }

        // The contribution of the this edge to the area integral (could be negative)
        double GetAreaContribution() const
        {
            Assert(m_pNext);    // Should not be called on a tail vertex
            return m_pt.X * m_pNext->m_pt.Y - m_pt.Y * m_pNext->m_pt.X;
        }

        // Comparisons, either along a line or around a fan:
        COMPARISON CompareWith(
            __in_ecount(1) const CVertex *pOther) const;
                // The other vertex to compare with

        // Effectively answers <this> > pOther ?
        bool IsHigherThan(
            __in_ecount(1) const CVertex *pOther) const
                // The other vertex to compare YX-order with
        {
            return (CompareWith(pOther) == C_STRICTLYGREATERTHAN);
        }

        bool CoincidesWith(
            __in_ecount(1) const CVertex *pOther) const         
                // The other vertex to check coincidence with
        {
            return (CompareWith(pOther) == C_EQUAL);
        }

        SCANNER_LOCATION LocateVertex(
            __in_ecount(1) const CVertex *v) const;    
                // The vertex to locate

        // Intersecting edges:
        HRESULT Intersect(
            __in_ecount(1) const CVertex *pOther,
                // The vertex starting the other edge
            __out_ecount(1) bool &fIntersect,
                // The edges intersect if true
            __out_ecount(1) CIntersectionResult &isectOnThis,
                // Intersection info on this edge
            __out_ecount(1) CIntersectionResult &isectOnOther
                // Intersection info on edge pOther
            ) const;

        HRESULT IntersectWithSegment(
            __in_ecount(1) const CVertex *pOther,
                // The vertex starting the segment
            __out_ecount(1) bool &fIntersect,
                // =true if a transverse intersection was found 
            __out_ecount(1) CIntersectionResult &isect) const;
                // Intersection info on this edge

        bool QueryAndSetEdgeIntersection(
            CLineSegmentIntersection::LOCATION eLocation,
                // Location on supporting segment
            __inout_ecount(1) CIntersectionResult &result
                // intersection result, updated here.
        ) const;

        GpPointR EvalIntersectApproxCoordinates(
            __in_ecount(1) const CEdgeIntersection &isect) const;
                // Intersection info

        __outro_ecount(1) const GpPointR &GetPoint() const
        {
            return m_pt;
        }

#if DBG
       GpPointR GetSegmentVector() const
       {
           return GpPointR(GetSegmentBasePoint(), GetSegmentTipPoint());
       }

        void AssertValid() const;
#endif


    protected:

        // non-const version
        __out_ecount(1) CVertex* GetSegmentBase()
        {
            return IsSegmentEndpoint() ? this : m_oSegment.pBase;
        }


    // Data
    protected:
        typedef enum  
        {
            eVTypeEndpoint,         // vertex is a line segment endpoint
            eVTypeIntersection,     // vertex results from an intersection with another segment
            eVTypeExactIntersect,   // vertex is an intersection at an exact point (eg. intersect
                                    // with another segment's endpoint)
            eVTypeUnknown
        }   EVertexType;

        EVertexType     m_eType;        // Type of vertex - intersection or exact point
        GpPointR        m_pt;           // Vertex coordinates; exact for endpoints and exact
                                        // intersections, approximate for generic intersections

        // Recording smoothness stipulation
        bool            m_fSmoothJoin; // Smooth join at this vertex, if true

        // Chain topology:
        CVertex         *m_pNext;       // Link to the next vertex/edge
        CVertex         *m_pPrevious;   // Link to the previous vertex/edge

        // Segment topology:
        union {
        CVertex        *pBase;      // For intersections: link to base-vertex of underlying segment
        CVertex        *pTip;       // For endpoints: link to tip-vertex of underlying segment
        }               m_oSegment;

        // Information about the source of this vertex

        // For intersection:  Intersection info; unused for exact intersection
        CEdgeIntersection m_intersection;  

        //
        // Curve information
        // Note: This information is stored on the vertex, but applies to the edge that this vertex
        // terminates (i.e. the edge above).
        //
        CBezierFragment m_bezierFragment;

   };  // End of definition of CVertex


    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CVertexPool
    //
    //  Synopsis:
    //      A memory pool for vertices
    //
    //  Notes:
    //      The memory pool will not allow allocating more than MAX_VERTEX_COUNT
    //      vertices, so that we fail early if we're going to produce an
    //      unusable number of vertices.
    //
    //--------------------------------------------------------------------------
    class CVertexPool   :   public TMemBlockBase<CVertex>
    {
    public:
        CVertexPool(
            __in_ecount(1) CCurvePool &pool)
                // Memory pool for curve records
        : m_refCurvePool(pool)
        {
            m_cVertices = 0;
        }

        virtual ~CVertexPool()
        {
        }

        HRESULT AllocateVertex(
            __deref_out_ecount(1) CVertex *&pNew);      // The allocated vertex

        HRESULT AllocateVertexAtPoint(
            __in_ecount(1) const GpPointR &pt,
                // Vertex coordinates
            __in bool fEndpoint,
                // true ==> endpoint, false ==> intersection
            __deref_out_ecount(1) CVertex *&pNew);
                // The allocated vertex

        HRESULT AllocateVertexAtIntersection(
            __in_ecount(1) const CEdgeIntersection &isect,
                // Edge intersection info
            __in_ecount(1) const GpPointR &pt,
                // Vertex coordinates
            __deref_out_ecount(1) CVertex *&pNew);
                // The allocated vertex

        HRESULT CopyVertex(
            __in_ecount(1) const CVertex *pvt,
                // Vertex to copy
            __deref_out_ecount(1) CVertex *&pNew);
                // The allocated vertex

        // Data
        int         m_cVertices;        // The number of allocted vertices
        CCurvePool  &m_refCurvePool;    // Memory pool for curve records
    };  // End of definition of CVertexPool

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CChain
    //
    //  Synopsis:
    //      Represents a chain of vertices/edges used by CScanner
    //
    //  Notes:
    //      During chain construction, the cursor is the latest vertex entered
    //      (which is the head if the chain is constructed in reverse).  During
    //      scan the cursor is the base of the current edge on the chain.
    //
    //--------------------------------------------------------------------------

    class CChain
    {
    private:
        // Disallows the copy constructor & assignment
        CChain(const CChain &)
        {
            Assert(false);
        }

        void operator = (const CChain&)
        {
            Assert(false);
        }

    public:
        CChain() {}
        ~CChain() {}

        void Initialize(
            __in_ecount(1) CVertexPool *pVertexPool,
                // The memory pool for vertices
            __in_ecount(1) CChainPool *pChainPool,
                // The memory pool for vertices
            MilFillMode::Enum eFillMode,
                // The fill mode
            WORD wFlags=0);
                // Initial flags settings

        HRESULT StartWith(
            __in_ecount(1) const GpPointR &pt);
                // Coordinates of the chain's first vertex

        HRESULT StartWithCopyOf(__in_ecount(1) const CVertex *pVertex);

        HRESULT InsertVertexAt(
            __in_ecount(1) const GpPointR &pt,
                // Coordinates of the new vertex
            __in_ecount_opt(1) const CBezierFragment *pFragment
                // Info about the associated curve (or NULL)
            );

        HRESULT TryAdd(
            __in_ecount(1) const GpPointR &ptNew,
                // Coordinates of the new vertex we're trying to add
            __in_ecount_opt(1) const CBezierFragment *pFragment,
                // Info about the associated curve (or NULL)
            __out_ecount(1) bool &fAscending,
                // =true if the new point is ascending from the previous
            __out_ecount(1) bool &fAdded);
                // =true if the new edge has been added

        __out_ecount(1) CVertex *GetHead() const
        {
            return m_pHead;
        }

        __out_ecount(1) CVertex *GetTail() const
        {
            return m_pTail;
        }

        __outro_ecount(1) const GpPointR &GetCurrentExactPoint() const
        {
            return m_pCursor->GetExactCoordinates();
        }

        __outro_ecount(1) const GpPointR &GetCurrentApproxPoint() const
        {
            return m_pCursor->GetApproxCoordinates();
        }

        GpPointR GetCurrentSegmentTipPoint() const
        {
            Assert(m_pCursor);
            Assert(m_pCursor->GetSegmentTip());
            return m_pCursor->GetSegmentTipPoint();
        }

        GpPointR GetCurrentEdgeApproxTipPoint() const
        {
            Assert(m_pCursor);
            Assert(m_pCursor->GetNext());
            return m_pCursor->GetNext()->GetApproxCoordinates();
        }

        __outro_ecount(1) const CVertex *GetCurrentVertex() const
        {
            return m_pCursor;
        }

        __outro_ecount(1) const CVertex *GetPreviousVertex() const
        {
            Assert(m_pCursor);
            return m_pCursor->GetPrevious();
        }

        __outro_ecount(1) const CVertex *GetCurrentSegmentBase() const
        {
            Assert(m_pCursor);
            return const_cast<const CVertex*>(m_pCursor)->GetSegmentBase();
        }

        __outro_ecount(1) const CVertex *GetCurrentSegmentTip() const
        {
            Assert(m_pCursor);
            return m_pCursor->GetSegmentTip();
        }

        __outro_ecount(1) const CVertex *GetCurrentEdgeBase() const
        {
            Assert(m_pCursor);
            return m_pCursor;
        }

        __outro_ecount(1) const CVertex *GetCurrentEdgeTip() const
        {
            Assert(m_pCursor);
            return m_pCursor->GetNext();
        }

        bool IsReversed() const
        {
            return (0 != (m_wFlags & CHAIN_REVERSED));
        }

        __out_ecount(1) CChain *GetLeft() const
        {
            return m_pLeft;
        }

        __out_ecount(1) CChain *GetRight() const
        {
            return m_pRight;
        }

        __out_ecount_opt(1) CChain *GoLeftWhileRedundant(
            WORD wRedundantMask);
                // Defining what is redundant - 
                // may or may not include coincidence

        __out_ecount_opt(1) CChain *GoRightWhileRedundant(
            WORD wRedundantMask);
                // Defining what is redundant

        __out_ecount_opt(1) CChain *GetRelevantLeft(
            WORD wRedundantMask) const
                // Defining what is redundant - 
                // may or may not include coincidence
        {
            return m_pLeft->GoLeftWhileRedundant(wRedundantMask);
        }

        __out_ecount_opt(1) CChain *GetRelevantRight(
            WORD wRedundantMask) const
                // Defining what is redundant
                // may or may not include coincidence
        {
            return m_pRight->GoRightWhileRedundant(wRedundantMask);
        }

        bool IsSideRight() const
        {
            return ((0 == (m_wFlags & CHAIN_SIDE_RIGHT)) !=
                    (0 == (m_wFlags & CHAIN_BOOL_FLIP_SIDE)));
        } const

        bool IsSelfSideRight() const
        {
            return (0 != (m_wFlags & CHAIN_SIDE_RIGHT));
        } const

        void SetSideRight()
        {
            m_wFlags |= CHAIN_SIDE_RIGHT;
        }

        // Set the Boolean side the opposite of the self side
        void FlipBoolSide() 
        {
            m_wFlags ^= CHAIN_BOOL_FLIP_SIDE;
        }

        bool IsRedundant(
            WORD wRedundantMask) const
                // Defining what is redundant - 
                // may or may not include coincidence
        {
            return (0 != (m_wFlags & wRedundantMask));
        }

        bool IsSelfRedundant() const
        {
            return (0 != (m_wFlags & CHAIN_SELF_REDUNDANT));
        }

        void SetRedundant()
        {
            m_wFlags |= CHAIN_SELF_REDUNDANT;
        }

        void CancelWith(__inout_ecount(1) CChain* pOther)
        {
            Assert(pOther);
            m_wFlags |= CHAIN_CANCELLED;
            pOther->m_wFlags |= CHAIN_CANCELLED;
        }

        void SetCoincidentWithRight()
        {
            Assert(m_pRight);
            m_wFlags |= CHAIN_COINCIDENT;
        }

        bool CoincidesWithRight() const
        {
            return (0 != (m_wFlags & CHAIN_COINCIDENT));
        }

        void SetBoolRedundant()
        {
            m_wFlags |= CHAIN_BOOL_REDUNDANT;
        }

        bool IsBoolRedundant() const
        {
            return (0 != (m_wFlags & CHAIN_BOOL_REDUNDANT));
        }

        WORD GetShape() const
        {
            return m_wFlags & CHAIN_SHAPE_MASK;
        }

        MIL_FORCEINLINE void SetReversed(
            bool reversed);     // The value to set to

        void MoveOn();

        static void LinkLeftRight(
            __inout_ecount_opt(1) CChain *pLeft,
                // The left chain (NULL OK)
            __inout_ecount_opt(1) CChain *pRight);
                // The right chain (NULL OK)

        void InsertBetween(
            __inout_ecount_opt(1) CChain *pLeft,
                // The left chain (NULL OK)
            __inout_ecount_opt(1) CChain *pRight);
                // The right chain (NULL OK)

        SCANNER_LOCATION LocateVertex(
            __in_ecount(1) const CVertex *pVt) const
                // The vertex to locate
        {
            return (m_pCursor->LocateVertex(pVt));
        }

        bool IsVertexOnRight(
            __in_ecount(1) const CVertex *pvt) const
                // A vertex
        {
            Assert(m_pCursor->GetSegmentTip());     // Should have an active edge
            return (m_pCursor->LocateVertex(pvt) == SCANNER_RIGHT);
        }

        bool IsVertexOnChain(
            __in_ecount(1) const CVertex *pvt) const
                // A vertex
        {
            Assert(m_pCursor->GetSegmentTip());     // Should have an active edge
            return (m_pCursor->LocateVertex(pvt) == SCANNER_INCIDENT);
        }

        void Append(
            __in_ecount(1) CChain *pOther);
                // The other chain to append

        HRESULT SplitAtVertex(
            __inout_ecount(1) CVertex *pvt,
                // A vertex on the chain to split at
            __deref_out_ecount(1) CChain *&pSplit);
                // The bottom chain after the split

        HRESULT SplitAtCurrentEdgeTip(
            __deref_out_ecount(1) CChain *&pSplit)
                // The bottom chain after the split
        {
            Assert(m_pCursor->GetNext());
            return SplitAtVertex(m_pCursor->GetNext(), pSplit);
        }

        HRESULT SplitAtIntersection(
            __in_ecount(1) const CIntersectionResult &isect,
                // Intersection along current edge
            __deref_out_ecount(1) CChain *&pSplit);
                // The remaining piece

        HRESULT SplitAtExactPoint(
            __in_ecount(1) const GpPointR &pt,
                // Intersection point along current edge
            __deref_out_ecount(1) CChain *&pSplit);
                // The remaining piece

        HRESULT SplitAtIncidentVertex(
            __in_ecount(1) const CVertex *pVertex,
                // The incident vertex
            __inout_ecount(1) CIntersectionPool &refPool,
                // Memory pool for allocating intersections
            __deref_out_ecount(1) CChain *&pSplit);
                // A (possibly NULL) piece split from this chain

        void ClassifyWinding(
            __in_ecount_opt(1) const CChain *pLeft);
                // Immediate left neighbor (NULL OK)

        void ClassifyAlternate(
            __in_ecount_opt(1) const CChain *pLeft)
                // Immediate left neighbor (NULL OK)
        {
            if (pLeft && !pLeft->IsSelfSideRight())
            {
                SetSideRight();
            }
        }

        void ContinueWinding(
            __in_ecount(1) const CChain *pChain);
                // The chain above it

        void ContinueAlternate(
            __in_ecount(1) const CChain *pChain)
                // The chain above it
        {
            Assert(pChain);   // Don't call with a null chain
            m_wFlags |= (pChain->m_wFlags & CHAIN_SELF_TYPE_MASK);
        }

        void Reset();

        void SetRight(
            __in_ecount_opt(1) CChain *pChain)       
                // The next chain to the right
        {
            m_pRight = pChain;
        }

        void SetLeft(
            __in_ecount_opt(1) CChain *pChain)       
                // The next chain to the left
        {
            m_pLeft = pChain;
        }

        void *GetTaskData()
        {
            return m_pTaskData;
        }

        void SetTaskData(void *pData)
        {
            m_pTaskData = pData;
        }

        void *GetTaskData2()
        {
            return m_pTaskData2;
        }

        void SetTaskData2(void *pData)
        {
            m_pTaskData2 = pData;
        }

        void AssumeTask(__inout_ecount(1) CChain *pOther)
        {
            Assert(pOther);
            m_pTaskData = pOther->m_pTaskData;
            pOther->m_pTaskData = NULL;
        }

        HRESULT Intersect(
            __in_ecount(1) const CChain *pOther,
                // The other chain to intersect
            __out_ecount(1) bool &fIntersect,
                // The edges intersect if true
            __out_ecount(1) CIntersectionResult &isectOnThis,
                // Intersection info on this chain
            __out_ecount(1) CIntersectionResult &isectOnOther
                // Intersection info on chain pOther
            ) const
        {
            return m_pCursor->Intersect(
                pOther->m_pCursor, fIntersect, isectOnThis, isectOnOther);
        }

        HRESULT IntersectWithSegment(
            __in_ecount(1) const CVertex *pOther,
                // The vertex starting the segment
            __out_ecount(1) bool &fIntersect,
                // =true if a transverse intersection was found 
            __out_ecount(1) CIntersectionResult &isect) const
                // Intersection info on this edge
        {
            return m_pCursor->IntersectWithSegment(pOther, fIntersect, isect);
        }

        bool IsAtTail() const
        {
            return m_pCursor == m_pTail;
        }

        bool IsAtItsLastEdge() const
        {
            Assert(m_pCursor);
            return GetCurrentEdgeTip() == m_pTail;
        }

        bool IsATailIntersection(const CIntersectionResult &result) const
        {
            return (result.GetEdgeLocation() == CLineSegmentIntersection::LOCATION_AT_LAST_POINT) 
                && IsAtItsLastEdge();
        }

        bool CoincidesWith(
            __in_ecount(1) const CChain *pOther) const;   // Another chain

        void Classify(__inout_ecount(1) CChain *pLeft)
        {
            (this->*m_pClassifyMethod)(pLeft);
        }

        void Continue(__inout_ecount(1) CChain *pChain)
        {
            (this->*m_pContinueMethod)(pChain);
        }

        void SetCurrentVertexSmooth(bool val)
        {
            Assert(m_pCursor);
            m_pCursor->SetSmoothJoin(val);
        }

        __outro_ecount(1) const GpPointR &GetTailPoint() const
        {
            Assert(m_pTail);
            return m_pTail->GetApproxCoordinates();
        }

        __outro_ecount(1) const GpPointR &GetHeadPoint() const
        {
            Assert(m_pHead);
            return m_pHead->GetApproxCoordinates();
        }

        UINT GetCandidateHeapIndex() const
        {
            return m_candidateHeapIndex;
        }

        void SetCandidateHeapIndex(UINT index)
        {
            m_candidateHeapIndex = index;
        }

        // Debug
#if DBG
        void ValidateActiveLinks() const;

        void Dump(
            bool fBooleanOperation=false
                // = true if called from a boolean operation
            ) const;

        void Validate() const;
        UINT              m_id;                 // Debugging ID
#endif

    protected:
        // Vertices
        CVertex           *m_pHead;             // The chain's first vertex.
        CVertex           *m_pCursor;           // The chain's current vertex
        CVertex           *m_pTail;             // The chain's last vertex

        // Links
        CChain            *m_pRight;            // The next chain to the right
        CChain            *m_pLeft;             // The next chain to the left
        CVertexPool       *m_pVertexPool;       // Chain list's memory pool for vertices
        CChainPool        *m_pChainPool;        // Chain list's memory pool for chains
        void              *m_pTaskData;         // Task specific data
        void              *m_pTaskData2;        // Task specific data (some tasks, like boolean, need 2)

        // Other data
        WORD              m_wFlags;               // Misc attributes
        WORD              m_wWinding;             // Winding number (used for winding fill mode)

        UINT              m_candidateHeapIndex;   // Index of this chain in the candidate heap.

        // Chains cannot have virtual functions because they are allocated by a pool allocator,
        // which cannot set a v-table pointer. The following is a substitute for virtual methods
        // for classifying, to differentiate between the 2 fill modes
        void (CChain:: *m_pClassifyMethod)(const CChain *pLeft);
        void (CChain:: *m_pContinueMethod)(const CChain *pChain);

    };  // End of definition of CChain

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CChainPool
    //
    //  Synopsis:
    //      A memory pool for chains
    //
    //  Notes:
    //
    //--------------------------------------------------------------------------
    class CChainPool   :   public TMemBlockBase<CChain>
    {
    public:
        CChainPool()
        {
            m_eFillMode = MilFillMode::Winding;
            m_wShapeIndex = 0;
#if DBG
            m_id = 0;
#endif
        }

        virtual ~CChainPool()
        {
        }

        __ecount_opt(1) CChain *AllocateChain(
            __inout_ecount(1) CVertexPool &refVertexPool);
            // The memory pool for vertices

        void SetFillMode(
            MilFillMode::Enum eFillMode)  // The mode that defines the fill set
        {
            m_eFillMode = eFillMode;
        }

        HRESULT SetNext();

#if DBG
        int m_id;
#endif
        // Data
        MilFillMode::Enum     m_eFillMode;     // The fill mode (to be set in allocated chains)
        WORD            m_wShapeIndex;   // The index of the current shape (for Boolean ops)
    };  // End of definition of CChainPool

    class CChainHolder
    {
    public:
        CChainHolder(__in_ecount(1) CChain * pChain)
        {
            m_pChain = pChain;
        }

        __outro_ecount(1) operator CChain const*() const
        {
            return m_pChain;
        }

        __out_ecount(1) operator CChain*()
        {
            return m_pChain;
        }

        bool operator==(__in_ecount(1) CChainHolder const & other) const
        {
            return m_pChain == other.m_pChain;
        }

    protected:
        CChain * m_pChain;
    };

    class CCandidateChain : public CChainHolder
    {
    public:
        CCandidateChain(__in_ecount(1) CChain * pChain) : CChainHolder(pChain) {}

        bool IsGreaterThan(
            __in_ecount(1) const CCandidateChain &other
            ) const
        {
            return m_pChain->GetCurrentEdgeTip()->IsHigherThan(other.m_pChain->GetCurrentEdgeTip());
        }

        UINT GetIndex() const
        {
            return m_pChain->GetCandidateHeapIndex();
        }

        void SetIndex(UINT index)
        {
            return m_pChain->SetCandidateHeapIndex(index);
        }

        void Dump() const
        {
#if DBG
            m_pChain->Dump();
#endif
        }
    };

    class CMasterChain : public CChainHolder
    {
    public:
        CMasterChain(__in_ecount(1) CChain * pChain) : CChainHolder(pChain) {}

        bool IsGreaterThan(
            __in_ecount(1) const CMasterChain &other
            ) const
        {
            return m_pChain->GetHead()->IsHigherThan(other.m_pChain->GetHead());
        }

        void Dump() const
        {
#if DBG
            m_pChain->Dump();
#endif
        }
    };

    class CMasterHeap : public CHeap<CMasterChain, 6>
        // Boolean operations on convex shapes will only require 6 chains.
    {
    public:
        HRESULT Insert(__in_ecount(1) CChain * pChain)
        {
            return InsertElement(CMasterChain(pChain));
        }

        __out_ecount_opt(1) CChain *GetTop()
        {
            CChain *pChain = NULL;
            if (!IsEmpty())
            {
                pChain = GetTopElement();
            }
            return pChain;
        }

    };

    class CCandidateHeap : public CHeap<CCandidateChain, 6>
        // Boolean operations on convex shapes will only require 6 chains.
    {
    public:
        HRESULT Insert(__in_ecount(1) CChain * pChain)
        {
            return InsertElement(CCandidateChain(pChain));
        }

        __out_ecount_opt(1) CChain *GetTop()
        {
            CChain *pChain = NULL;
            if (!IsEmpty())
            {
                pChain = GetTopElement();
            }
            return pChain;
        }

    };

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CChainList
    //
    //  Synopsis:
    //      The main list of chains that CScanner operates on
    //
    //  Notes:
    //
    //--------------------------------------------------------------------------

    class CChainList : public CFlatteningSink
    {
    public:

        CChainList();

        void SetFillMode(
            MilFillMode::Enum eFillMode)
                // The mode that defines the fill set
        {
            m_oChainPool.SetFillMode(eFillMode);
        }

        HRESULT SetNext()
        {
            return m_oChainPool.SetNext();
        }

        __out_ecount_opt(1) CChain *GetNextChain()
        {
            CChain *pChain = NULL;
            if (!m_chainHeap.IsEmpty())
            {
                pChain = m_chainHeap.GetTop();
            }
            return pChain;
        }

        void Pop() { m_chainHeap.Pop(); }

        HRESULT StartFigure(
            __in_ecount(1) const GpPointR &pt);
                // Figure's first point

        HRESULT AddVertex(
            __in_ecount(1) const GpPointR &ptNew,
                // The new vertex to add
            __in_ecount_opt(1) const CBezierFragment *pFragment = NULL
                // Info about the associated curve (or NULL)
            );

        // CFlatteningSink override
        HRESULT AcceptPoint(
            __in_ecount(1) const GpPointR &ptNew,
                // The new vertex to add
            __in GpReal t,
                // Parameter value on the curve)
            __out_ecount(1) bool &fAbort);
                // Ignored here

        void SetCurrentVertexSmooth(bool val)
        {
            Assert(m_pCurrent);
            m_pCurrent->SetCurrentVertexSmooth(val);
        }

        HRESULT EndFigure(
            __in_ecount(1) const GpPointR &ptCurrent,
                // The most recently added vertex
            __in bool fClosed);
                // =true if the figure is closed

        HRESULT Insert(
            __inout_ecount(1) CChain *pNew);
                // The chain to insert

        __outro_ecount(1) const CBezier *GetCurrentCurve() const
        {
            return m_oCurvePool.GetCurrentCurve();
        }

        __outro_ecount(1) const GpPointR &GetCurrentPoint() const
        {
            return m_ptCurrent;
        }

        void SetNoCurve()
        {
            m_oCurvePool.SetNoCurve();
        }

        HRESULT AddCurve(
            __in_ecount(1) const GpPointR &ptFirst,
                // The curve's first point
            __in_ecount(3) const GpPointR *pPt)
                // The curve's remaining 3 points
        {
            m_previousT = 0.0;
            return m_oCurvePool.AddCurve(ptFirst, pPt);
        }

        // Debug
#if DBG
        void Dump() const;
        void Validate() const;
#endif
    protected:
        CChain            *m_pCurrent;            // Current chain under construction
        CChain            *m_pFiguresFirstChain;  // First chain in the figure currently organized

        CCurvePool        m_oCurvePool;           // The memory pool for curve records
        CVertexPool       m_oVertexPool;          // The memory pool for vertices
        CChainPool        m_oChainPool;           // The memory pool for chains

        GpPointR          m_ptFirst;              // The first point of the figure in processing
        GpPointR          m_ptCurrent;            // The most recent point (scanner coordinates)

        CMasterHeap       m_chainHeap;            // Heap of chains from highest to lowest.

        double            m_previousT;            // Last encountered parameter on the current Bezier
    };  // End of definition of CChainList

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CClassifier
    //
    //  Synopsis:
    //      Classifies chains as left/right/redundant
    //
    //--------------------------------------------------------------------------

    class CClassifier
    {
    public:
        CClassifier()
        {
        }

        virtual ~CClassifier()
        {
        }

        virtual void Classify(
            __inout_ecount(1) CChain *pLeftmostTail,
                // The junction's leftmost tail
            __inout_ecount(1) CChain *pLeftmostHead,
                // The junction's leftmost head
            __inout_ecount_opt(1) CChain *pLeft);
                // The chain to the left of the junction (possibly NULL)
    };

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CJunction
    //
    //  Synopsis:
    //      The junction of coincident heads and tails at the current vertex
    //
    //  Notes:
    //      The junction keeps track of all the active chains whose heads and
    //      tails are at the currently processed vertex.
    //
    //      This object holds a pointer to the owning scanner. It is used to
    //      invoke the scanner's virtual ProcessJunction method when it's time
    //      to flush the junction.
    //
    //--------------------------------------------------------------------------
    class CJunction
    {
    public:
        CJunction(CClassifier *pClassifier, CIntersectionPool &refIntersectionPool)
            :m_refIntersectionPool(refIntersectionPool)
        {
            Initialize();
            m_pClassifier = pClassifier;
        }

        ~CJunction()
        {
        }

        void SetOwner(
            __in_ecount(1) CScanner *pScanner)
                // Owning scanner
        {
            m_pOwner = pScanner;
        }

        void Initialize();

        // Access methods
        __out_ecount_opt(1) CChain *GetLeftmostHead(WORD wRedundantMask)
        {
            return m_pLeftmostHead->GoRightWhileRedundant(wRedundantMask);
        }

        __out_ecount_opt(1) CChain *GetRightmostHead(WORD wRedundantMask)
        {
            return m_pRightmostHead->GoLeftWhileRedundant(wRedundantMask);
        }

        __out_ecount_opt(1) CChain *GetLeftmostTail(WORD wRedundantMask)
        {
            return m_pLeftmostTail->GoRightWhileRedundant(wRedundantMask);
        }

        __out_ecount_opt(1) CChain *GetRightmostTail(WORD wRedundantMask)
        {
            return m_pRightmostTail->GoLeftWhileRedundant(wRedundantMask);
        }

        __out_ecount_opt(1) CChain *GetLeft() const
        {
            return m_pLeft->GoLeftWhileRedundant(CHAIN_REDUNDANT_OR_CANCELLED);
        }

        __out_ecount_opt(1) CChain *GetRight() const
        {
            return m_pRight->GoRightWhileRedundant(CHAIN_REDUNDANT_OR_CANCELLED);
        }

        bool IsEmpty() const
        {
            return (NULL == m_pLeftmostHead)  &&  (NULL == m_pLeftmostTail);
        }

        const GpPointR &GetPoint() const
        {
            return m_pRepVertex->GetApproxCoordinates();
        }

        void SetClassifier(CClassifier *pClassifier)
        {
            m_pClassifier = pClassifier;
        }

        // Action
        void InsertHead(
            __inout_ecount(1) CChain *pNew);
                // The new chain to insert

        HRESULT Flush();

        HRESULT ProcessAtHead(
            __inout_ecount(1) CChain *pHead,
                // The chain to become the first head
            __inout_ecount_opt(1) CChain *pLeft,
                // The junction's left neighbor (NULL OK)
            __inout_ecount_opt(1) CChain *pRight,
                // The junction's right neighbor (NULL OK)
            bool fIsOnRightChain); // =true if the junction lies on the right chain

        HRESULT ProcessAtTail(
            __inout_ecount(1) CChain *pTail,
                // The chain to become the first tail
            __inout_ecount_opt(1) CChain *pLeft,
                // Its left neighbor (NULL OK)
            __inout_ecount_opt(1) CChain *pRight);
                // Its right neighbor (NULL OK)

        HRESULT Populate(
            __inout_ecount_opt(1) CChain *pLeft,
                // The junction's left neighbor (NULL OK)
            __inout_ecount_opt(1) CChain *pRight);
                // The junction's right neighbor (NULL OK)

        void Classify();
// Debug
#if DBG
        void Dump() const;
#endif
    // Data
    protected:
        const CVertex     *m_pRepVertex;          // A representent vertex
        CChain            *m_pLeftmostTail;       // The leftomst junction tail chain
        CChain            *m_pRightmostTail;      // The rightmost junction tail chain
        CChain            *m_pLeftmostHead;       // The leftmost junction head chain
        CChain            *m_pRightmostHead;      // The rightmost junction tail chain
        CChain            *m_pLeft;               // The junction's left neighbot
        CChain            *m_pRight;              // The junction's right neighbor
        CScanner          *m_pOwner;              // Owning scanner reference
        CClassifier       *m_pClassifier;         // The classifier in use for the current operation
        CIntersectionPool &m_refIntersectionPool; // Memory pool for intersections

    };  // End of definition of CJunction

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CActiveList
    //
    //  Synopsis:
    //      The list of active chains
    //
    //  Notes:
    //      Since we may have to dynamically remove entries anywhere in the
    //      list, it is implemented as a doubly linked list.
    //
    //--------------------------------------------------------------------------
    class CActiveList
    {
        public:

        CActiveList()
        {
            m_pLeftmost = NULL;
        }

        ~CActiveList()
        {
        }

        void InsertHead(
            __inout_ecount(1) CChain *pNew);
                // The new head to insert

        void MoveOn();

        void Remove(
            __inout_ecount(1) CChain *pFirst,
                    // The first chain to remove
            __inout_ecount(1) CChain *pLast); 
                    // The last chain to remove

        bool Locate(
            __in_ecount(1) const CChain *pNew,
                // The new chain whose position we need to locate
            __deref_out_ecount(1) CChain *&pLeft,
                // The chain on the left of the location
            __deref_out_ecount(1) CChain *&pRight);
                // The chain on the left of the location

        void Insert(
            __inout_ecount(1) CChain *pLeft,
                // The leftmost chain to insert
            __inout_ecount(1) CChain *pRight,
                // The rightmost chain to insert
            __inout_ecount_opt(1) CChain *pPrevious,
                // The chain to insert after (NULL OK)
            __inout_ecount_opt(1) CChain *pNext);
                // The chain to insert before (NULL OK)

        __out_ecount(1) CChain *GetLeftmost()
        {
            return m_pLeftmost;
        }

        // Debug
#if DBG
        void Dump(
            bool fBooleanOperation=false // = true if called from a boolean operation
            ) const;

        void Validate(
            __in_ecount(1) const CVertex *pV) const;
                // The current event vertex

        bool Includes(
            __in_ecount(1) const CChain *pChain) const;
                // A chain

        void AssertConsistentWith(
            __in_ecount(1) const CCandidateHeap &list) const;
                // The candidate list
#endif
    protected:
        CChain      *m_pLeftmost;        // The leftmost active chain

    };  // End of definition of CActiveList

    //+---------------------------------------------------------------------------------------------

    //      End of helper classes

    //----------------------------------------------------------------------------------------------
    // CScanner methods

private:
    CScanner();

public:
    CScanner(double rTolerance);

    virtual ~CScanner()
    {
    }

    HRESULT SetWorkspaceTransform(
        __in_ecount(1) const CMilRectF &rect,
            // Geometry bounds
        __out_ecount(1) bool &fDegenerate);
            // =true if there is nothing to scan

    // IPopulationSink methods

    virtual void SetFillMode(
        MilFillMode::Enum eFillMode)
            // The mode that defines the fill set
    {
        m_oChains.SetFillMode(eFillMode);
    }

    virtual HRESULT StartFigure(
        __in_ecount(1) const GpPointR &pt)
            // Figure's first point
    {
        HRESULT hr = S_OK;
        GpPointR ptLocal;

        IFC(ConvertToInteger30(pt, ptLocal));
        IFC(m_oChains.StartFigure(ptLocal));

        m_ptLastInput = pt;

    Cleanup:
        RRETURN(hr);
    }

    virtual HRESULT AddLine(
        __in_ecount(1) const GpPointR &ptNew)
            // The line's endpoint
    {
        HRESULT hr = S_OK;
        GpPointR ptLocal;

        IFC(ConvertToInteger30(ptNew, ptLocal));
        IFC(m_oChains.AddVertex(ptLocal));

        m_ptLastInput = ptNew;

    Cleanup:
        RRETURN(hr);
    }

    HRESULT AddCurve(
        __in_ecount(3) const GpPointR *ptNew);
            // The last 3 Bezier points of the curve (we already have the first one)

    virtual void SetCurrentVertexSmooth(bool val)
    {
        m_oChains.SetCurrentVertexSmooth(val);
    }

    virtual void SetStrokeState(bool val)
    {
        // Ignored
    }

    virtual HRESULT EndFigure(
        bool fClosed)
            // =true if the figure is closed
    {
        return m_oChains.EndFigure(m_oChains.GetCurrentPoint(), fClosed);
    }

    HRESULT Scan();

protected:
    HRESULT 
    ConvertToInteger30(
        __in_ecount(1) const GpPointR &ptIn,
            // Input point
        __out_ecount(1) GpPointR &ptOut)
            // The point converted to scanner workspace coordinates
    {
        HRESULT hr = S_OK;

        ptOut.X = CDoubleFPU::Round((ptIn.X - m_ptCenter.X) * m_rScale);
        ptOut.Y = CDoubleFPU::Round((ptIn.Y - m_ptCenter.Y) * m_rScale);

        //
        // Since m_ptCenter and m_rScale were computed using the bounds of the geometry
        // ptOut should be valid. Bounds computations aren't exact, though when dealing with
        // Beziers, though, so we should double check.
        // 

        if(!IsValidInteger30(ptOut.X) ||
            !IsValidInteger30(ptOut.Y))
        {
            IFC(WGXERR_BADNUMBER);
        }

    Cleanup:
        RRETURN(hr);
    }


    HRESULT MoveOn();

    void TerminateBatch(
        __inout_ecount(1) CChain *pFirst,
            // The first chain
        __inout_ecount(1) CChain *pLast);
            // The last chain

    HRESULT ProcessCandidate(
        __inout_ecount(1) CChain *pChain);
            // The chain whose cursor we're processing

    HRESULT Activate(
        __inout_ecount(1) CChain *pChain);
            // The chain to activate

    void InsertCandidate(
        __inout_ecount(1) CChain *pChain);
            // The chain to activate

    __out_ecount_opt(1) CChain* GrabInactiveCoincidentChain(
        __in_ecount(1) const CVertex *pV);
            // The current junction vertex

    HRESULT SplitAtIntersections(
        __inout_ecount(1) CChain *pLeft,
            // The leftmost chain
        __inout_ecount(1) CChain *pRight,
            // The rightmost chain
        __inout_ecount_opt(1) CChain *pPrevious,
            // The chain that will become pLeft's left neighbor (NULL OK)
        __inout_ecount_opt(1) CChain *pNext);
            // The chain that will become pRight's right neighbor (NULL OK)

    void ActivateBatch(
        __inout_ecount(1) CChain *pLeft,    
            // The leftmost chain to insert
        __inout_ecount(1) CChain *pRight,   
            // The rightmost chain to insert
        __inout_ecount_opt(1) CChain *pPrevious,
            // The chain to insert after (NULL OK)
        __inout_ecount_opt(1) CChain *pNext);
            // The chain to insert before (NULL OK)

    HRESULT SplitChainAtIntersection(
        __inout_ecount(1) CChain *pChain,
            // A chain to split
        __in_ecount(1) const CIntersectionResult &isect);
            // Intersection info

    HRESULT SplitChainAtCurrentEdgeTip(
        __inout_ecount(1) CChain *pChain); 
            // The chain to split

    HRESULT SplitChainAtIncidentVertex(
        __inout_ecount(1) CChain *pChain,
            // The chain to split
        __in_ecount(1) const CVertex *pVertex);
            // The incident vertex

    HRESULT SplitChainAtSegmentIntersection(
        __inout_ecount(1) CChain *pChain,
            // The chain to split
        __in_ecount(1) const CVertex *pSegmentBase);
            // The segment's base point

    HRESULT SplitCandidate(
        __inout_ecount(1) CChain *pChain,
            // A chain to split
        __in_ecount(1) const CIntersectionResult &isect);
            // The split point

    HRESULT SplitNeighbor(
        __inout_ecount(1) CChain *pChain,
            // A chain that is not in the candidate list
        __inout_ecount_opt(1) CChain *pNeighbor,
            // Its neighbor that is in the candidate list (NULL OK)
        __out_ecount(1) bool &fSplitNeighbor);
            // =true if pNeighbor was split

    HRESULT SplitCoincidentChainsLeftOf(
        __inout_ecount(1) CChain *pChain);
            // A that has just been split

    HRESULT SplitCoincidentChainsRightOf(
        __inout_ecount(1) CChain *pChain);
            // A that has just been split

    HRESULT SplitPairAtIntersection(
        __inout_ecount(1) CChain *pLeft,
            // The left chain
        __inout_ecount(1) CChain *pRight);
            // The right chain

    HRESULT SplitAtCoincidentIntersection(
        __inout_ecount(1) CChain *pChain);
            // A chain

    // Pure methods
    virtual HRESULT ProcessTheJunction()=0;

    virtual HRESULT ProcessCurrentVertex(
        __inout_ecount(1) CChain *pChain)=0;
            // The chain whose cursor we're processing

    // Debug
#if DBG
    void Trace(
    __in PCWSTR pStr,
        // String to dump
    int id) const;
        // Chain ID

    void ValidateAt(
        __in_ecount(1) const CVertex *pVertex) const;
            // The recently processed vertex

    virtual bool IsBooleanOperation() const
    {
        return false;
    }
#endif

    // Data
protected:
    CChainList        m_oChains;           // The master list of chains to scan
    CActiveList       m_oActive;           // The active chains sorted from left to right
    CJunction         m_oJunction;         // The current junction
    CCandidateHeap    m_oCandidates;       // Active chains sorted in by their candidate vertices
    CClassifier       m_oClassifier;       // The standard left/right/redundant classifier
    CIntersectionPool m_oIntersectionPool; // The memofy pool for intersections
    GpPointR          m_ptLastInput;       // Last input point in caller coordinates

    // Transformation: caller space to and from scanner workspace
    double            m_rTolerance;        // Polygonal approximation (flattening) error tolerance
    GpPointR          m_ptCenter;          // Input geometry's bounding box center
    double            m_rScale;            // Scale factor to apply to the input
    double            m_rInverseScale;     // Scale factor to apply to the output

    bool              m_fCachingCurves;    // = true if we are caching input curves
    bool              m_fDone;             // Done flag
};




