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
//      Getting points on a path at a given fraction of path length
//
//  $ENDTAG
//
//  Classes:
//      CAnimationPath and supporting classes
//
//------------------------------------------------------------------------------

MtExtern(CAnimationPath);
MtExtern(CAnimationSegment);


//+-----------------------------------------------------------------------------
//
//  Class:
//      CAnimationSegment
//
//  Synopsis:
//      Knows how to produce points on a segment at a given portion of its
//      length
//
//------------------------------------------------------------------------------
class CAnimationSegment :   public CIncreasingFunction
{
                        // H E L P E R   C L A S S E S
protected:

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CSquaredSpeedDerivative
    //
    //  Synopsis:
    //      Represents the squared derivative of the speed on a Bezier curve
    //
    //--------------------------------------------------------------------------
    class CSquaredSpeedDerivative :   public CRealFunction
    {
    public:
        CSquaredSpeedDerivative(
            __in_ecount(1) const CAnimationSegment &curve)
                // The Bezier curve
            : m_refCurve(curve)
        {
        }
        virtual ~CSquaredSpeedDerivative()
        {
        }

        virtual void GetValueAndDerivative(
            __in double t,
                // Where on the curve
            __out_ecount(1) double &f,
                // The derivative of the distance at t
            __out_ecount(1) double &df) const;
                // The derivative of f there

    protected:
        // Data
        const CAnimationSegment &m_refCurve;  // The Bezier curve on which the speed is defined
    };

public:
    // Constructor/destructor
    CAnimationSegment()
        :m_rBaseLength(0), m_uiCurrentSpan(0), m_rLatest(0)
        {
        }

    ~CAnimationSegment()
        {
        }

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CAnimationSegment));

    // Methods
    HRESULT InitAsLine(
        __in_ecount(2) const CMilPoint2F *ppt,
            // Line's points
        __inout_ecount(1) REAL &rLength);
            // Path length so far, updated here
    
    HRESULT InitAsCurve(
        __in_ecount(4) const CMilPoint2F *ppt,
            // Curve's points
        __inout_ecount(1) REAL &rLength);
            // Path length so far, updated here

    void GetPointAndTangentOnLine(
        IN REAL         rLength,
            // Length along the segment
        __out_ecount(1) MilPoint2F  &pt,
            // Point there
        __out_ecount_opt(1) MilPoint2F  *vecTangent) const;
            // Unit tangent there (NULL OK)

    void GetPointAndTangentOnCurve(
        IN REAL       rLength,
            // Length along the curve segment
        __out_ecount(1) MilPoint2F  &pt,
            // Point there
        __out_ecount_opt(1) MilPoint2F  *vecTangent) const;
            // Unit tangent there (NULL OK)

    REAL GetLength()
    {
        return m_rgLength[m_cBreaks - 1];
    }
    
    REAL GetBaseLength() const
    {
        return m_rBaseLength;
    }
    
    void GetPointAtLength(
        IN REAL       s,
            // Fraction of length (between 0 and 1 (or NaN))
        __out_ecount(1) MilPoint2F  &pt,
            // Point on the segment there
        __out_ecount_opt(1) MilPoint2F  *pvecTangent);
            // Unit tangent there (NULL OK)

protected:
    // Bezier curve methods
    void GetPointAndTangent(
        IN REAL       t,
            // Parameter in the curve domain
        __out_ecount(1) CMilPoint2F  &pt,
            // Point there
        __out_ecount_opt(1) CMilPoint2F  *vecTangent) const;
            // Unit tangent there (NULL OK)

    void Get2Derivatives(
        IN REAL   t,
            // Parameter
        __out_ecount(1) CMilPoint2F &vecD1,
            // The first derivative there
        __out_ecount(1) CMilPoint2F &vecD2) const;
            // The second derivative there

    REAL GetSpeed(
        IN REAL t) const;   // Parameter

    void GetSpeedAndDerivative(
        IN REAL   t,
            // Parameter
        __out_ecount(1) REAL  &speed,
            // The speed there
        __out_ecount(1) REAL  &derivative) const;
            // The derivative of the speed there
    
    REAL GetLength(
        IN REAL from,         // Parameter of segment start
        IN REAL to) const;    // Parameter of segment end
    
    REAL GetExtent() const;

    const CMilPoint2F &GetThirdDerivative() const
    {
        return m_vecD3;
    }

    REAL GetParameterFromLength(
        IN REAL rLength) const;   // The length on this curve

    void AcceptBreak(
        double t);   // The candidate for a break

    void SetBreaks();

    // CIncreasingFunction override
    void GetValueAndDerivative(
        __in double t,
            // Where on the curve
        __out_ecount(1) double &f,
            // The derivative of the distance at t
        __out_ecount(1) double &df) const;
            // The derivative of f there


    // Data

    // Line/curve segment
    BYTE                m_bType;       // Line or curve
    const CMilPoint2F    *m_ppt;        // The location (in CAnimationPath.m_pPoints) of the
                                       // first of the 2 or 4 defining points of this segment
    mutable CMilPoint2F  m_vecTangent;  // Last good tangent

    // For curve only
    CMilPoint2F          m_vecD1[3];    // First derivative's Bezier coefficients      
    CMilPoint2F          m_vecD2[2];    // Second derivative's Bezier coefficients      
    CMilPoint2F          m_vecD3;       // Constant third derivative

    // Breaks, lengths etc.
    __field_range(1, 5) UINT               m_cBreaks;         // Number of break points
    REAL              m_rgBreak[5];      // Break points for length approximation
    REAL              m_rgLength[5];     // Lengths there 
    REAL              m_rgMid[4];        // Midpoints between breaks
    REAL              m_rBaseLength;     // The path length at this segment's start

    // Computation variables
    mutable REAL      m_rTargetLength;     // used when solving the equation
    mutable UINT       m_uiCurrentSpan;      // The current length-span
    mutable REAL      m_rLatest;           // The latest solution of Length(t)=target
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CAnimationPath
//
//  Synopsis:
//      Knows how to produce points on a path at a given portion of its length
//
//  Notes:
//      Because this class holds pointers to CShape internals (points) it is
//      defined so that it can only be instantiated by a CShape.  This is not
//      foolproof, but reduces the chance for the pointers becoming bad.
//
//------------------------------------------------------------------------------
class CAnimationPath    :   public CFigureTask
{
public:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CAnimationPath));
    
    CAnimationPath()
        :m_pSegments(NULL), 
        m_pPoints(NULL), 
        m_cSegments(0), 
        m_uCurrentSegment(0),
        m_uCurrentPoint(0)
    {
    }

    virtual ~CAnimationPath()
    {
        delete [] m_pSegments;
        delete [] m_pPoints;
    }

    HRESULT SetUp(
        __in_ecount(1) const IShapeData &shape);
            // The path we to animate along

    REAL GetLength(UINT i) const
    {
        Assert(i <= m_cSegments);
        return (i < m_cSegments)? m_pSegments[i].GetBaseLength() : m_rTotalLength;
    }

    // CFigureTask overrides
    virtual HRESULT DoLine(
        __in_ecount(1) const MilPoint2F &ptEnd);
            // The line's end point

    virtual HRESULT DoBezier(
        __in_ecount(3) const MilPoint2F *ptBez);
            // The curve's 3 last Bezier points

    void GetPointAtLengthFraction(
        IN REAL        rFraction,
            // Fraction of length (between 0 and 1)
        __out_ecount(1) MilPoint2F  &pt,
            // Point on the segment there
        __out_ecount_opt(1) MilPoint2F  *pvecTangent);
            // Unit tangent there (NULL OK)

    void BinarySearch(
        IN REAL       rLength,          // The length to locate
        IN int          bottom,         // The bottom of the search interval
        IN int          top);           // The top of the search interval

protected:
    // Data
    CAnimationSegment   *m_pSegments;       // List of segments
    MilPoint2F           *m_pPoints;         // List of points
    UINT                m_cSegments;        // Number or segments
    REAL                m_rTotalLength;     // Path's total length
    UINT                m_uCurrentSegment;  // The current segment
    UINT                m_uCurrentPoint;    // The current point (during construction)
};

