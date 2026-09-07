// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//
//      Software Rasterizer - polygon scan conversion.
//

struct CInitializeEdgesContext;


// Define our on-stack storage use.  The 'free' versions are nicely tuned
// to avoid allocations in most common scenarios, while at the same time
// not chewing up toooo much stack space.
//
// We make the debug versions small so that we hit the 'grow' cases more
// frequently, for better testing:

#if DBG
    #define EDGE_STORE_STACK_NUMBER 10
    #define EDGE_STORE_ALLOCATION_NUMBER 11
    #define INACTIVE_LIST_NUMBER 12
    #define ENUMERATE_BUFFER_NUMBER 15

    // Must be at least 4
    #define NOMINAL_FILL_POINT_NUMBER 4
#else
    #define EDGE_STORE_STACK_NUMBER (1600 / sizeof(CEdge))
    #define EDGE_STORE_ALLOCATION_NUMBER (4032 / sizeof(CEdge))
    #define INACTIVE_LIST_NUMBER EDGE_STORE_STACK_NUMBER
    #define ENUMERATE_BUFFER_NUMBER 32
    #define NOMINAL_FILL_POINT_NUMBER 32
#endif

//
// Rasterization helpers that are also needed by the hardware rasterizer
// in hwrasterizer.cpp.
//

// 'CEdge' is our classic data structure for tracking an edge:

struct CEdge
{
    CEdge *Next;                // Next active edge (don't check for NULL,
                                //   look for tail sentinel instead)
    INT X;                      // Current X location
    INT Dx;                     // X increment
    INT Error;                  // Current DDA error
    INT ErrorUp;                // Error increment
    INT ErrorDown;              // Error decrement when the error rolls over
    INT StartY;                 // Y-row start
    INT EndY;                   // Y-row end
    INT WindingDirection;       // -1 or 1
};

// We the inactive-array separate from the edge allocations so that
// we can more easily do in-place sorts on it:

struct CInactiveEdge
{
    CEdge *Edge;                // Associated edge
    LONGLONG Yx;                // Sorting key, StartY and X packed into an lword
};

// We allocate room for our edge datastructures in batches:

struct CEdgeAllocation
{
    CEdgeAllocation *Next;     // Next allocation batch (may be NULL)
    __field_range(<=, EDGE_STORE_ALLOCATION_NUMBER) UINT Count;
    CEdge EdgeArray[EDGE_STORE_STACK_NUMBER];
};

/**************************************************************************\
*
* Class Description:
*
*  'CEdgeStore' is used by 'InitializeEdges' as its repository for
*   all the edge data:
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

class CEdgeStore
{
private:
    __field_range(<=, UINT_MAX - 2) UINT TotalCount;                       // Total edge count in store
    __field_range(<=, CurrentBuffer->Count) UINT CurrentRemaining; // How much room remains in current buffer
    CEdgeAllocation *CurrentBuffer;  // Current buffer
    CEdge *CurrentEdge;              // Current edge in current buffer
    CEdgeAllocation *Enumerator;     // For enumerating all the edges
    CEdgeAllocation EdgeHead;        // Our built-in allocation

public:

    CEdgeStore()
    {
        TotalCount = 0;
        CurrentBuffer = &EdgeHead;
        CurrentEdge = &EdgeHead.EdgeArray[0];
        CurrentRemaining = EDGE_STORE_STACK_NUMBER;

        EdgeHead.Count = EDGE_STORE_STACK_NUMBER;
        EdgeHead.Next = NULL;
    }

    ~CEdgeStore()
    {
        // Free our allocation list, skipping the head, which is not
        // dynamically allocated:

        CEdgeAllocation *allocation = EdgeHead.Next;
        while (allocation != NULL)
        {
            CEdgeAllocation *next = allocation->Next;
            GpFree(allocation);
            allocation = next;
        }
    }

    __range(<=, UINT_MAX - 2) UINT StartEnumeration()
    {
        Enumerator = &EdgeHead;

        // Update the count and make sure nothing more gets added (in
        // part because this Count would have to be re-computed):

        CurrentBuffer->Count -= CurrentRemaining;

        // This will never overflow because NextAddBuffer always ensures that TotalCount has
        // space remaining to describe the capacity of all new buffers added to the edge list.
        TotalCount += CurrentBuffer->Count;

        // Prevent this from being called again, because bad things would
        // happen:

        CurrentBuffer = NULL;

        return TotalCount;
    }

    BOOL Enumerate(
        __deref_out_ecount(*ppEndEdge - *ppStartEdge) CEdge** ppStartEdge,
        __deref_out_ecount(0) CEdge** ppEndEdge
        )
    {
        CEdgeAllocation *enumerator = Enumerator;

        // Might return startEdge == endEdge:

        *ppStartEdge = &enumerator->EdgeArray[0];
        *ppEndEdge   = *ppStartEdge + enumerator->Count;

        return((Enumerator = enumerator->Next) != NULL);
    }

    VOID StartAddBuffer(
        __deref_out_ecount(*puRemaining) CEdge **ppCurrentEdge,
        __deref_out_range(==, (this->CurrentRemaining)) UINT *puRemaining
        )
    {
        *ppCurrentEdge = CurrentEdge;
        *puRemaining = CurrentRemaining;
    }

    VOID EndAddBuffer(
        __in_ecount(remaining) CEdge *pCurrentEdge,
        __range(0, (this->CurrentBuffer->Count)) UINT remaining
        )
    {
        CurrentEdge = pCurrentEdge;
        CurrentRemaining = remaining;
    }

    HRESULT NextAddBuffer(
        __deref_out_ecount(*puRemaining) CEdge **ppCurrentEdge,
        __inout_ecount(1) UINT *puRemaining
        );

    // Disable instrumentation checks within all methods of this class
    SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_DONOTHING);
};

// The following is effectively the paramter list for 'InitializeEdges',
// which takes a run of points and sets up the initial edge list:

struct CInitializeEdgesContext
{
    INT MaxY;                      // Maximum 'y' found, should be INT_MIN on
                                   //   first call to 'InitializeEdges'
    RECT* ClipRect;                // Bounding clip rectangle in 28.4 format
    CEdgeStore *Store;             // Where to stick the edges
    MilAntiAliasMode::Enum AntiAliasMode;
};

//
// Useful methods for rasterization
//

HRESULT
InitializeEdges(
    __inout_ecount(1) CInitializeEdgesContext *pEdgeContext,
    __inout_ecount(vertexCount) POINT *pointArray,    // Points to a 28.4 array of size 'vertexCount'
    __in_range(>=, 2) UINT vertexCount
    );

VOID
InsertNewEdges(
    __inout_ecount(1) CEdge *pActiveList,
    INT iCurrentY,
    __deref_inout_xcount(array terminated by an edge with StartY != iCurrentY)
        CInactiveEdge **ppInactiveEdge,
    __out_ecount(1) INT *pYNextInactive
        // will be INT_MAX when no more
    );

VOID
FASTCALL
SortActiveEdges(
    __inout_ecount(1) CEdge *list
    );

#if DBG
    #define ASSERTACTIVELIST(list, y) AssertActiveList(list, y)
    #define ASSERTACTIVELISTORDER(list) AssertActiveListOrder(list)
    #define ASSERTINACTIVEARRAY(list, count) AssertInactiveArray(list, count)
    #define ASSERTPATH(rgTypes, cPoints) AssertPath(rgTypes, cPoints)

   BOOL AssertActiveList(
       __in_ecount(1) const CEdge *list,
       INT yCurrent
       );

   VOID AssertActiveListOrder(
       __in_ecount(1) const CEdge *list
       );

   VOID AssertInactiveArray(
       __in_ecount(1) const CInactiveEdge *inactive,
       INT count
       );

   VOID AssertPath(
       __in_ecount(cPoints) const BYTE *rgTypes,
       const UINT cPoints
       );

#else
    #define ASSERTACTIVELIST(list, y)
    #define ASSERTACTIVELISTORDER(list)
    #define ASSERTINACTIVEARRAY(list, count)
    #define ASSERTPATH(rgTypes, cPoints)
#endif

VOID
AppendScaleToMatrix(
    __inout_ecount(1) CMILMatrix *pmat,
    REAL scaleX,
    REAL scaleY
    );

INT
InitializeInactiveArray(
    __in_ecount(1) CEdgeStore *pEdgeStore,
    __in_ecount(count+2) CInactiveEdge *rgInactiveArray,
    UINT count,
    __in_ecount(1) CEdge *tailEdge                    // Tail sentinel for inactive list
    );


HRESULT
FixedPointPathEnumerate(
    __in_ecount(cPoints) const MilPoint2F *rgpt,
    __in_ecount(cPoints) const BYTE *rgTypes,
    UINT cPoints,
    __in_ecount(1) const CMILMatrix *matrix,
    __in_ecount(1) const RECT *clipRect,
    __inout_ecount(1) CInitializeEdgesContext *enumerateContext
    );

//+------------------------------------------------------------------------
//
//  Function:  RasterizePath
//
//  Synopsis:  Rasterize a path, or optionally the complement of the path
//             within some bounds (and still rendering the original
//             interior with alpha.)
//
//                 1          +---------------+
//                            |               |
//  NORMAL                    |               |
//  RENDERING                 |   INSIDE OF   |
//                            |   THE SHAPE   |
//                            |               |
//                 0 ---------+               +------------
//
//
//
//
//                 1 ---------+               +------------
//  COMPLEMENTED              |   ORIGINAL    |
//  RENDERING                 |    INSIDE     |
//                 1-factor.. +---------------+
//
//
//                 0 . . . . . . . . . . . . . . . . . . . .
//
//-------------------------------------------------------------------------
HRESULT
RasterizePath(
    __in_ecount(cPoints)   const MilPoint2F *rgPoints,      // Points of the path to stroke/fill
    __in_ecount(cPoints)   const BYTE *rgTypes,            // Types array of the path
    __in_range(>=, 2) const UINT cPoints,                  // Number of points in the path
    __in_ecount(1) const CBaseMatrix *pMatPointsToDevice,
    MilFillMode::Enum fillMode,
    MilAntiAliasMode::Enum antiAliasMode,
    __inout_ecount(1) CSpanSink *pSpanSink,                // The sink for the spans produced by the
                                                           // rasterizer. For AA, this sink must
                                                           // include an operation to apply the AA
                                                           // coverage.
    __in_ecount(1) CSpanClipper *pClipper,                 // Clipper.
    __in_ecount(1) const MilPointAndSizeL *prcBounds,               // Bounding rectangle of the path points.
    float rComplementFactor = -1,
    __in_ecount_opt(1) const CMILSurfaceRect *prcComplementBounds = NULL
    );

// Functions supporting per-primitive antialiasing (PPAA).

ScanOpFunc GetOp_ScalePPAACoverage(
    MilPixelFormat::Enum fmtColorData,
        // Color data format, either 32bppPARGB, 32bppRGB, or 128bppPABGR.
    bool fComplementAlpha,
        // Should the operation support using complement factor to rescale
        // coverage values
    __out_ecount(1) MilPixelFormat::Enum *pFmtColorOut
        // Color data format after operation
    );

BOOL IsPPAAMode(
    MilAntiAliasMode::Enum aam
    );

// Helper function to downcast a CAntialiasedFiller without having to see its definition
__ecount(1) OpSpecificData *DowncastFiller(
    __in_ecount(1) CAntialiasedFiller *pFiller
    );

//
// Inlines
//

/**************************************************************************\
*
* Function Description:
*
* Advance DDA and update active edge list
*
* Created:
*
*   06/20/2003 ashrafm
*
\**************************************************************************/
MIL_FORCEINLINE VOID
AdvanceDDAAndUpdateActiveEdgeList(
    INT nSubpixelYCurrent,
    __inout_ecount(1) CEdge *pEdgeActiveList
    )
{
    INT nOutOfOrderCount = 0;
    CEdge *pEdgePrevious = pEdgeActiveList;
    CEdge *pEdgeCurrent = pEdgeActiveList->Next;

    // Advance DDA and update edge list

    for (;;)
    {
        if (pEdgeCurrent->EndY <= nSubpixelYCurrent)
        {
            // If we've hit the sentinel, our work here is done:

            if (pEdgeCurrent->EndY == INT_MIN)
                break;              // ============>

            // This edge is stale, remove it from the list:

            pEdgeCurrent = pEdgeCurrent->Next;
            pEdgePrevious->Next = pEdgeCurrent;
            continue;               // ============>
        }

        // Advance the DDA:

        pEdgeCurrent->X += pEdgeCurrent->Dx;
        pEdgeCurrent->Error += pEdgeCurrent->ErrorUp;
        if (pEdgeCurrent->Error >= 0)
        {
            pEdgeCurrent->Error -= pEdgeCurrent->ErrorDown;
            pEdgeCurrent->X++;
        }

        // Is this entry out-of-order with respect to the previous one?

        nOutOfOrderCount += (pEdgePrevious->X > pEdgeCurrent->X);

        // Advance:

        pEdgePrevious = pEdgeCurrent;
        pEdgeCurrent = pEdgeCurrent->Next;
    }

    // It turns out that having any out-of-order edges at this point
    // is extremely rare in practice, so only call the bubble-sort
    // if it's truly needed.
    //
    // NOTE: If you're looking at this code trying to fix a bug where
    //       the edges are out of order when the filler is called, do
    //       NOT simply change the code to always do the bubble-sort!
    //       Instead, figure out what caused our 'outOfOrder' logic
    //       above to get messed up.

    if (nOutOfOrderCount)
    {
        SortActiveEdges(pEdgeActiveList);
    }

    ASSERTACTIVELISTORDER(pEdgeActiveList);
}



