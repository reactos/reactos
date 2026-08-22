// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains classes that waffle triangles and lines.
//      See waffling-and-packing.txt for detailed description of algorithm.
//

#include "precomp.hpp"

#if DBG

static UINT s_uInCountTri = 0;
static UINT s_uOutCountTri = 0;
static UINT s_uInCountLine = 0;
static UINT s_uOutCountLine = 0;

#endif

//+----------------------------------------------------------------------------
//
//  Function:  Score
//
//  Synopsis:  Evaluates the equation ax + by + c for a PointXYA
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE 
static float Score(
    __in_ecount(1) const PointXYA &vertex,  // Input point to score against.
    float a, float b, float c               // Parameters defining waffle
    )
{
    return vertex.x * a + vertex.y * b + c;
}

//+----------------------------------------------------------------------------
//
//  Function:  Interpolate
//
//  Synopsis:  Interpolates between two PointXYA
//

//+----------------------------------------------------------------------------
//
//  Function:  Interpolate
//
//  Synopsis:  Interpolates between two PointXYA
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE
static void Interpolate(
    __in_ecount(1) const PointXYA &begin,
    __in_ecount(1) const PointXYA &end,
    float u,                    // Parameter distance from begin to end.
    __out_ecount(1) PointXYA &output
    )
{
    float v = 1-u;
    output.x = v * begin.x + u * end.x;
    output.y = v * begin.y + u * end.y;
    output.a = v * begin.a + u * end.a;

    // Clamp the split/interpolated point to the extent of the original points.
    output.x = ClampValueUnordered(output.x, begin.x, end.x);
    output.y = ClampValueUnordered(output.y, begin.y, end.y);
    output.a = ClampValueUnordered(output.a, begin.a, end.a);
}

//+----------------------------------------------------------------------------
//
//  Member:    SplitEdge
//
//  Synopsis:  Given vertices v0 and v1 with scores s0 and s1 respectively
//             return the interpolated vertex with score
//             splitScore.
//
//-----------------------------------------------------------------------------
static void SplitEdge(
    float s0,
    __in_ecount(1) const PointXYA &v0,
    float s1,
    __in_ecount(1) const PointXYA &v1,
    float splitScore,
    __out_ecount(1) PointXYA &out
    )
{
    float d0 = +(splitScore-s0);
    float d1 = -(splitScore-s1);

    // This function needs to be PERFECTLY symmetric with respect to
    // s0,v0 vs s1,v1.  Therefore I invert the calculation based on
    // the relative values of d0 and d1.
    if (d0 < d1)
    {
        d0 /= d0+d1;

        // We should be splitting edges at a point inbetween the endpoints
        // so clamp d0 to [0,1] in case of numerical error.
        d0 = ClampReal(d0, 0.f, 1.f);

        Interpolate(v0,v1,d0,out);
    }
    else
    {
        d1 /= d0+d1;

        // We should be splitting edges at a point inbetween the endpoints
        // so clamp d0 to [0,1] in case of numerical error.
        d1 = ClampReal(d1, 0.f, 1.f);

        Interpolate(v1,v0,d1,out);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:  SortByScore
//
//  Synopsis:  Given three vertices and corresponding scores.  This function sorts
//             them by scores (and sorts the scores too!)
//
//  Note:      Returns failure if we failed to sort which should only happen if
//             one of the inputs is nan.
//
//-----------------------------------------------------------------------------
template<typename T>
HRESULT SortByScore(
    __inout_ecount(1) T &v0,    // Vertices
    __inout_ecount(1) T &v1,
    __inout_ecount(1) T &v2,
    __inout_ecount(1) float &s0, // Scores
    __inout_ecount(1) float &s1,
    __inout_ecount(1) float &s2
    )
{
    T tempT;
    float tempFloat;
    
    // Swap a-th and b-th vertex and score
#define SWAP(a,b)     \
    tempFloat = s##a; \
    s##a = s##b;      \
    s##b = tempFloat; \
    tempT = v##a;     \
    v##a = v##b;      \
    v##b = tempT;

    if (s1 < s0)
    {
        SWAP(0,1);
    }
    if (s2 < s1)
    {
        SWAP(1,2);
    }
    if (s1 < s0)
    {
        SWAP(0,1);
    }

    if (!(s1 >= s0) || !(s2 >= s1))
    {
        // This should only happen if we encountered NaN.
        Assert(_isnan(s0) || _isnan(s1) || _isnan(s2));
        RRETURN(WGXERR_BADNUMBER);
    }
    else
    {
        return S_OK;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    TriangleWaffler<T>::AddTriangle
//
//  Synopsis:  Break the given triangle up according to our partition and sends
//             the generated triangles to our sink.
//

template<typename T>
HRESULT TriangleWaffler<T>::AddTriangle(
    __in_ecount(1) const T &v0_,
    __in_ecount(1) const T &v1_,
    __in_ecount(1) const T &v2_
    )
{
#if DBG
    ++s_uInCountTri;
#endif
    
    HRESULT hr = S_OK;
    
    // Score the vertices & put them in order.
    T v0 = v0_;
    T v1 = v1_;
    T v2 = v2_;
    float s[3];
    s[0] = Score(v0, m_a, m_b, m_c);
    s[1] = Score(v1, m_a, m_b, m_c);
    s[2] = Score(v2, m_a, m_b, m_c);

    IFC(SortByScore(v0,v1,v2,s[0],s[1],s[2]));
    
    Assert(s[0] <= s[1] && s[1] <= s[2]);
    
    //
    // Convert from floating point scores to cells using GpFloorSat. These
    // floating-point scores are guaranteed to be small enough not to
    // overflow, since:
    //      a^2+b^2 is small (see code in BuildWafflePipeline),
    //      -1 <= c <= 1, and
    //      vertex comes from a geometry clipped to safe device bounds.
    //
    int cellNums[3];
    for (int i = 0; i < 3; ++i)
    {
        cellNums[i] = static_cast<int>(GpFloorSat(s[i]));
    }
    Assert(cellNums[0] <= cellNums[1] && cellNums[1] <= cellNums[2]);
    
    // This should never happen, but just to be sure...
    if (cellNums[2] == INT_MAX)
    {
        IFC(WGXERR_BADNUMBER);
    }

    // Step through each cell containing a vertex and output triangles
    for (int i = cellNums[0]; i <= cellNums[2]; ++i)
    {
        // Figure out which case we're in, defined by number of vertices
        // in left and right tridents.
        UINT uLeftCount=0;
        UINT uRightCount=0;
        for (int j = 0; j < 3; ++j)
        {
            uLeftCount += cellNums[j] < i;
            uRightCount += cellNums[j] > i;
        }

        UINT configuration = uLeftCount << 4 | uRightCount;

// EDGE(a,b,o)
// Defines a variable of type T with name eabo that is the vertex
// on the edge between a and b intersecting the i+o-th
// gridline.
// This macros is symmetric about it's first two arguments, e.g.
//        EDGE(a,b,o) = EDGE(b,a,o)
//
// And, o is either 0 or 1 indicating whether the edge should be
// intersected with the boundary between the this cell and the PRIOR
// region or the NEXT region respectively.  In other words which of the two
// pipes in my shorthand state notation e.g. for 0|1|2
// EDGE(0,1,0) is the vertex between 0 and 1 on the first pipe
// EDGE(1,2,1) is the vertex between 1 and 2 on the second pipe
// EDGE(0,2,0) is the vertex between 0 and 2 on the first pipe (and so on)
             
#define EDGE(a,b,o)                                                     \
        T e##a##b##o;                                                   \
        SplitEdge(s[##a],v##a,s[##b],v##b,static_cast<float>(i+o),e##a##b##o) \

        switch (configuration)
        {
        case 0x00: // |012|
        {
            IFC(SendTriangle(v0,v1,v2));
            break;
        }
        case 0x01: // |01|2
        {
            EDGE(1,2,1);
            EDGE(0,2,1);
            IFC(SendQuad(v0,v1,e121,e021));
            break;
        }
        case 0x02: // |0|12
        {
            EDGE(0,1,1);
            EDGE(0,2,1);
            IFC(SendTriangle(v0,e011,e021));
            break;
        }

        case 0x10: // 0|12|
        {
            EDGE(0,1,0);
            EDGE(2,0,0);
            IFC(SendQuad(e010,v1,v2,e200));
            break;
        }
        case 0x11: // 0|1|2
        {
            EDGE(0,1,0);
            EDGE(1,2,1);
            EDGE(2,0,1);
            EDGE(2,0,0);
            IFC(SendPent(e010,v1,e121,e201,e200));
            break;
        }
        case 0x12: // 0||12
        {
            EDGE(0,1,0);
            EDGE(0,2,0);
            EDGE(0,1,1);
            EDGE(0,2,1);
            IFC(SendQuad(e010,e011,e021,e020));
            break;
        }
        case 0x20: // 01|2|
        {
            EDGE(1,2,0);
            EDGE(2,0,0);
            IFC(SendTriangle(e120,v2,e200));
            break;
        }
        case 0x21: // 01||2
        {
            EDGE(1,2,0);
            EDGE(1,2,1);
            EDGE(2,0,1);
            EDGE(2,0,0);
            IFC(SendQuad(e120,e121,e201,e200));
            break;
        }

        // Shouldn't be in an interval that generates no triangles.
        //case 0x03: // ||012
        //case 0x30: // 012||
        default:
            Assert(false);
        }
    }
    
  Cleanup:
    RRETURN(hr);
}

// OK this is a bit of a cheat.  We know that the wafflers in the
// pipeline are in an array so we check to see if the sink for this
// waffler is the next element in the array.  That way we measure
// "into the pipeline" and "out to the vertex buffer".
#if DBG
#define DBG_SEND(incr,type)         \
    if (m_consumer != &this[1])     \
    {                               \
        s_uOutCount##type += incr;  \
    }                               \
    else                            \
    {                               \
        s_uInCount##type -= incr;   \
    }
#else
#define DBG_SEND(incr,type)         \
    {                               \
    }
#endif

//+----------------------------------------------------------------------------
//
//  Member:    TriangleWaffler<T>::SendTriangle
//             TriangleWaffler<T>::SendQuad
//             TriangleWaffler<T>::SendPent
//
//  Synopsis:  Send a triangle (or quad or pentagon) to this waffler's sink.
//
    
template<typename T>
HRESULT TriangleWaffler<T>::SendTriangle(
    __in_ecount(1) const T &v0,
    __in_ecount(1) const T &v1,
    __in_ecount(1) const T &v2
    )
{
    HRESULT hr = S_OK;
    
    DBG_SEND(1,Tri);

    IFC(m_consumer->AddTriangle(v0,v1,v2));
  Cleanup:
    RRETURN(hr);
}

template<typename T>
HRESULT TriangleWaffler<T>::SendQuad(
    __in_ecount(1) const T &v0,
    __in_ecount(1) const T &v1,
    __in_ecount(1) const T &v2,
    __in_ecount(1) const T &v3
    )
{
    HRESULT hr = S_OK;
    
    DBG_SEND(2,Tri);
    
    IFC(m_consumer->AddTriangle(v0,v1,v2));
    IFC(m_consumer->AddTriangle(v0,v2,v3));
  Cleanup:
    RRETURN(hr);
}

template<typename T>
HRESULT TriangleWaffler<T>::SendPent(
    __in_ecount(1) const T &v0,
    __in_ecount(1) const T &v1,
    __in_ecount(1) const T &v2,
    __in_ecount(1) const T &v3,
    __in_ecount(1) const T &v4
    )
{
    HRESULT hr = S_OK;
    
    DBG_SEND(3,Tri);

    IFC(m_consumer->AddTriangle(v0,v1,v2));
    IFC(m_consumer->AddTriangle(v0,v2,v3));
    IFC(m_consumer->AddTriangle(v0,v3,v4));
  Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    LineWaffler<T>::AddLine
//
//  Synopsis:  Split line v0, v1 according to the subdivision
//
//-----------------------------------------------------------------------------
template<typename T>
HRESULT
LineWaffler<T>::AddLine(
    __in_ecount(1) const T &v0,
    __in_ecount(1) const T &v1)
{
#if DBG
    ++s_uInCountLine;
#endif
    
    HRESULT hr = S_OK;
    
    float score0 = Score(v0, m_a, m_b, m_c);
    float score1 = Score(v1, m_a, m_b, m_c);
    
    if (score0 > score1)
    {
        // Signs don't matter so let's ensure that score0 < score1
        score0 = -score0;
        score1 = -score1;
    }

    if (!(score1 >= score0))
    {
        // We had a nan
        Assert(_isnan(score0) || _isnan(score1));
        IFC(E_FAIL);
    }

    int cellNum0 = GpFloorSat(score0);
    int cellNum1 = GpFloorSat(score1);

    Assert(cellNum1 >= cellNum0);

    DBG_SEND(cellNum1-cellNum0+1,Line);
    
    if (cellNum0 == cellNum1)
    {
        IFC(m_consumer->AddLine(v0,v1));
    }
    else
    {
        // The line looks like
        //
        // ..cellNum0....----cellNum0+1-----------+2---------+3-------- /// ----cellNum1----........
        //                   #####################
        //
        // where . is empty, - is line and the numbers are the dividors between cells.  Note that
        // the cells extend to the right of the number so cellNum0 is the hash marks
        //
        // There are cellNum1-cellNum0+1 generated line segments.
        
        T lastPoint = v0;
        for (int i = cellNum0+1; i < cellNum1+1; ++i)
        {
            T nextPoint;
            SplitEdge(score0, v0, score1, v1, static_cast<float>(i), nextPoint);
            IFC(m_consumer->AddLine(lastPoint,nextPoint));
            lastPoint = nextPoint;
        }
        IFC(m_consumer->AddLine(lastPoint,v1));
    }

  Cleanup:
    RRETURN(hr);
}

// Explicit template instantiation
template class TriangleWaffler<PointXYA>;
template class LineWaffler<PointXYA>;



