// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Coverage buffer implementation
//

struct CEdge;
struct CInactiveEdge;

//-------------------------------------------------------------------------
//
// TrapezoidalAA only supports 8x8 mode, so the shifts/masks are all
// constants.  Also, since we must be symmetrical, x and y shifts are
// merged into one shift unlike the implementation in aarasterizer.
//
//-------------------------------------------------------------------------

const INT c_nShift = 3; 
const INT c_nShiftSize = 8; 
const INT c_nShiftSizeSquared = c_nShiftSize * c_nShiftSize; 
const INT c_nHalfShiftSize = 4; 
const INT c_nShiftMask = 7; 
const float c_rShiftSize = 8.0f;
const float c_rHalfShiftSize = 4.0f;
const float c_rInvShiftSize = 1.0f/8.0f;
const MilAntiAliasMode::Enum c_antiAliasMode = MilAntiAliasMode::EightByEight;

//
// Interval coverage descriptor for our antialiased filler
//

struct CCoverageInterval
{
    CCoverageInterval *m_pNext; // m_pNext interval (look for sentinel, not NULL)
    INT m_nPixelX;              // Interval's left edge (m_pNext->X is the right edge)
    INT m_nCoverage;            // Pixel coverage for interval
};

// Define our on-stack storage use.  The 'free' versions are nicely tuned
// to avoid allocations in most common scenarios, while at the same time
// not chewing up toooo much stack space.  
//
// We make the debug versions small so that we hit the 'grow' cases more
// frequently, for better testing:

#if DBG
    // Must be at least 6 now: 4 for the "minus4" logic in hwrasterizer.*, and then 
    // 1 each for the head and tail sentinels (since their allocation doesn't use Grow).
    #define INTERVAL_BUFFER_NUMBER 8        
#else    
    #define INTERVAL_BUFFER_NUMBER 32
#endif

//
// Allocator structure for the antialiased fill interval data
//

struct CCoverageIntervalBuffer
{
    CCoverageIntervalBuffer *m_pNext;
    CCoverageInterval m_interval[INTERVAL_BUFFER_NUMBER];
};

//------------------------------------------------------------------------------
//
//  Class: CCoverageBuffer
//
//  Description:
//      Coverage buffer implementation that maintains coverage information
//      for one scanline.  
//
//      This implementation will maintain a linked list of intervals consisting
//      of x value in pixel space and a coverage value that applies for all pixels
//      between pInterval->X and pInterval->Next->X.
//
//      For example, if we add the following interval (assuming 8x8 anti-aliasing)
//      to the coverage buffer:
//       _____ _____ _____ _____
//      |     |     |     |     |
//      |  -------------------  |
//      |_____|_____|_____|_____|
//    (0,0) (1,0) (2,0) (3,0) (4,0)
//
//      Then we will get the following coverage buffer:
//
//     m_nPixelX: INT_MIN  |  0  |  1  |  3  |  4  | INT_MAX
//   m_nCoverage: 0        |  4  |  8  |  4  |  0  | 0xdeadbeef
//       m_pNext: -------->|---->|---->|---->|---->| NULL
//              
//------------------------------------------------------------------------------
class CCoverageBuffer
{
public:
    //
    // Init/Destroy methods
    //

    VOID Initialize();
    VOID Destroy();

    //
    // Setup the buffer so that it can accept another scanline
    //

    VOID Reset();

    //
    // Add a subpixel interval to the coverage buffer
    //

    HRESULT FillEdgesAlternating(
        __in_ecount(1) const CEdge *pEdgeActiveList,
        INT nSubpixelYCurrent
        );

    HRESULT FillEdgesWinding(
        __in_ecount(1) const CEdge *pEdgeActiveList,
        INT nSubpixelYCurrent
        );

    HRESULT AddInterval(INT nSubpixelXLeft, INT nSubpixelXRight);

private:

    HRESULT Grow(
        __deref_out_ecount(1) CCoverageInterval **ppIntervalNew, 
        __deref_out_ecount(1) CCoverageInterval **ppIntervalEndMinus4
        );

public:
    CCoverageInterval *m_pIntervalStart;           // Points to list head entry

private:
    CCoverageInterval *m_pIntervalNew;

    // The Minus4 in the below variable refers to the position at which
    // we need to Grow the buffer.  The buffer is grown once before an
    // AddInterval, so the Grow has to ensure that there are enough 
    // intervals for the AddInterval worst case which is the following:
    //
    //  1     2           3     4
    //  *_____*_____ _____*_____* 
    //  |     |     |     |     |
    //  |  ---|-----------|---  |
    //  |_____|_____|_____|_____|
    //
    // Note that the *'s above mark potentional insert points in the list,
    // so we need to ensure that at least 4 intervals can be allocated.
    //

    CCoverageInterval *m_pIntervalEndMinus4;

    CCoverageIntervalBuffer m_pIntervalBufferBuiltin;
    CCoverageIntervalBuffer *m_pIntervalBufferCurrent;
       
    // Disable instrumentation checks within all methods of this class
    SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_DONOTHING);
};


//
// Inlines
//

//-------------------------------------------------------------------------
//
//  Function:   CCoverageBuffer::AddInterval
//
//  Synopsis:   Add a subpixel resolution interval to the coverage buffer
// 
//-------------------------------------------------------------------------
MIL_FORCEINLINE HRESULT
CCoverageBuffer::AddInterval(INT nSubpixelXLeft, INT nSubpixelXRight)
{
    HRESULT hr = S_OK;
    INT nPixelXNext;
    INT nPixelXLeft;
    INT nPixelXRight;
    INT nCoverageLeft;  // coverage from right edge of pixel for interval start
    INT nCoverageRight; // coverage from left edge of pixel for interval end

    CCoverageInterval *pInterval = m_pIntervalStart;
    CCoverageInterval *pIntervalNew = m_pIntervalNew;
    CCoverageInterval *pIntervalEndMinus4 = m_pIntervalEndMinus4;

    // Make sure we have enough room to add two intervals if
    // necessary:

    if (pIntervalNew >= pIntervalEndMinus4)
    {
        IFC(Grow(&pIntervalNew, &pIntervalEndMinus4));
    }

    // Convert interval to pixel space so that we can insert it 
    // into the coverage buffer

    Assert(nSubpixelXLeft < nSubpixelXRight);
    nPixelXLeft = nSubpixelXLeft >> c_nShift;
    nPixelXRight = nSubpixelXRight >> c_nShift; 

    // Skip any intervals less than 'nPixelLeft':

    while ((nPixelXNext = pInterval->m_pNext->m_nPixelX) < nPixelXLeft)
    {
        pInterval = pInterval->m_pNext;
    }

    // Insert a new interval if necessary:

    if (nPixelXNext != nPixelXLeft)
    {
        pIntervalNew->m_nPixelX = nPixelXLeft;
        pIntervalNew->m_nCoverage = pInterval->m_nCoverage;

        pIntervalNew->m_pNext = pInterval->m_pNext;
        pInterval->m_pNext = pIntervalNew;

        pInterval = pIntervalNew;

        pIntervalNew++;
    }
    else
    {
        pInterval = pInterval->m_pNext;
    }

    //
    // Compute coverage for left segment as shown by the *'s below
    //
    //  |_____|_____|_____|_
    //  |     |     |     |
    //  |  ***----------  |
    //  |_____|_____|_____|
    //

    nCoverageLeft = c_nShiftSize - (nSubpixelXLeft & c_nShiftMask);

    // If nCoverageLeft == 0, then the value of nPixelXLeft is wrong
    // and should have been equal to nPixelXLeft+1.
    Assert(nCoverageLeft > 0);

    // If we have partial coverage, then ensure that we have a position
    // for the end of the pixel 

    if ((nCoverageLeft < c_nShiftSize || (nPixelXLeft == nPixelXRight))
        && nPixelXLeft + 1 != pInterval->m_pNext->m_nPixelX)
    {
        pIntervalNew->m_nPixelX = nPixelXLeft + 1;
        pIntervalNew->m_nCoverage = pInterval->m_nCoverage;

        pIntervalNew->m_pNext = pInterval->m_pNext;
        pInterval->m_pNext = pIntervalNew;

        pIntervalNew++;
    }
    
    //
    // If the interval only includes one pixel, then the coverage is
    // nSubpixelXRight - nSubpixelXLeft
    //

    if (nPixelXLeft == nPixelXRight)
    {
        pInterval->m_nCoverage += nSubpixelXRight - nSubpixelXLeft;
        Assert(pInterval->m_nCoverage <= c_nShiftSize*c_nShiftSize);
        goto Cleanup;
    }

    // Update coverage of current interval
    pInterval->m_nCoverage += nCoverageLeft;
    Assert(pInterval->m_nCoverage <= c_nShiftSize*c_nShiftSize);

    // Increase the coverage for any intervals between 'nPixelXLeft'
    // and 'nPixelXRight':

    while ((nPixelXNext = pInterval->m_pNext->m_nPixelX) < nPixelXRight)
    {
        pInterval = pInterval->m_pNext;
        pInterval->m_nCoverage += c_nShiftSize;
        Assert(pInterval->m_nCoverage <= c_nShiftSize*c_nShiftSize);
    }

    // Insert another new interval if necessary:

    if (nPixelXNext != nPixelXRight)
    {
        pIntervalNew->m_nPixelX = nPixelXRight;
        pIntervalNew->m_nCoverage = pInterval->m_nCoverage - c_nShiftSize;

        pIntervalNew->m_pNext = pInterval->m_pNext;
        pInterval->m_pNext = pIntervalNew;

        pInterval = pIntervalNew;

        pIntervalNew++;
    }
    else
    {
        pInterval = pInterval->m_pNext;
    }

    //
    // Compute coverage for right segment as shown by the *'s below
    //
    //  |_____|_____|_____|_
    //  |     |     |     |
    //  |  ---------****  |
    //  |_____|_____|_____|
    //

    nCoverageRight = nSubpixelXRight & c_nShiftMask;
    if (nCoverageRight > 0)
    {
        if (nPixelXRight + 1 != pInterval->m_pNext->m_nPixelX)
        {
            pIntervalNew->m_nPixelX = nPixelXRight + 1;
            pIntervalNew->m_nCoverage = pInterval->m_nCoverage;

            pIntervalNew->m_pNext = pInterval->m_pNext;
            pInterval->m_pNext = pIntervalNew;

            pIntervalNew++;
        }

        pInterval->m_nCoverage += nCoverageRight;
        Assert(pInterval->m_nCoverage <= c_nShiftSize*c_nShiftSize);
    }

Cleanup:
    // Update the coverage buffer new interval

    m_pIntervalNew = pIntervalNew;

    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Function:   CCoverageBuffer::FillEdgesAlternating
//
//  Synopsis:   
//      Given the active edge list for the current scan, do an alternate-mode
//      antialiased fill.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE HRESULT 
CCoverageBuffer::FillEdgesAlternating(
    __in_ecount(1) const CEdge *pEdgeActiveList,
    INT nSubpixelYCurrent
    )
{
    HRESULT hr = S_OK;
    const CEdge *pEdgeStart = pEdgeActiveList->Next;
    const CEdge *pEdgeEnd;
    INT nSubpixelXLeft;
    INT nSubpixelXRight;

    ASSERTACTIVELIST(pEdgeActiveList, nSubpixelYCurrent);

    while (pEdgeStart->X != INT_MAX)
    {
        pEdgeEnd = pEdgeStart->Next;

        // We skip empty pairs:

        if ((nSubpixelXLeft = pEdgeStart->X) != pEdgeEnd->X)
        {
            // We now know we have a non-empty interval.  Skip any
            // empty interior pairs:

            while ((nSubpixelXRight = pEdgeEnd->X) == pEdgeEnd->Next->X)
            {
                pEdgeEnd = pEdgeEnd->Next->Next;
            }

            Assert((nSubpixelXLeft < nSubpixelXRight) && (nSubpixelXRight < INT_MAX));

            IFC(AddInterval(nSubpixelXLeft, nSubpixelXRight));
        }

        // Prepare for the next iteration:
        pEdgeStart = pEdgeEnd->Next;
    } 

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CCoverageBuffer::FillEdgesWinding
//
//  Synopsis:   
//      Given the active edge list for the current scan, do an alternate-mode
//      antialiased fill.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE HRESULT 
CCoverageBuffer::FillEdgesWinding(
    __in_ecount(1) const CEdge *pEdgeActiveList,
    INT nSubpixelYCurrent
    )
{
    HRESULT hr = S_OK;
    const CEdge *pEdgeStart = pEdgeActiveList->Next;
    const CEdge *pEdgeEnd;
    INT nSubpixelXLeft;
    INT nSubpixelXRight;
    INT nWindingValue;

    ASSERTACTIVELIST(pEdgeActiveList, nSubpixelYCurrent);

    while (pEdgeStart->X != INT_MAX)
    {
        pEdgeEnd = pEdgeStart->Next;

        nWindingValue = pEdgeStart->WindingDirection;
        while ((nWindingValue += pEdgeEnd->WindingDirection) != 0)
        {
            pEdgeEnd = pEdgeEnd->Next;
        }

        Assert(pEdgeEnd->X != INT_MAX);

        // We skip empty pairs:

        if ((nSubpixelXLeft = pEdgeStart->X) != pEdgeEnd->X)
        {
            // We now know we have a non-empty interval.  Skip any
            // empty interior pairs:

            while ((nSubpixelXRight = pEdgeEnd->X) == pEdgeEnd->Next->X)
            {
                pEdgeStart = pEdgeEnd->Next;
                pEdgeEnd = pEdgeStart->Next;

                nWindingValue = pEdgeStart->WindingDirection;
                while ((nWindingValue += pEdgeEnd->WindingDirection) != 0)
                {
                    pEdgeEnd = pEdgeEnd->Next;
                }
            }

            Assert((nSubpixelXLeft < nSubpixelXRight) && (nSubpixelXRight < INT_MAX));

            IFC(AddInterval(nSubpixelXLeft, nSubpixelXRight));
        }

        // Prepare for the next iteration:

        pEdgeStart = pEdgeEnd->Next;
    } 

Cleanup:
    RRETURN(hr);
}





