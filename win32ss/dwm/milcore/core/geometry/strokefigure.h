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
//      Classes used for stroking a figure
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//                             WIDENING DESIGN 
// 
//  CPen represents the elliptical pen tip.  It is instantiated from the
//  nominal pen and is hooked up with a sink upon construction.  It generates
//  offset points for a given direction and sends them to the sink.  CSimplePen
//  is currently the only derived class of CPen.  Additional classes may be
//  derived in the future for compound lines or for variable width. 
//  
//  Like CPen, the dash generator supports CPenInterface. The CWiden
//  instantiates one, hook it to the widener as the widening target, and hook a
//  CPen for it to call with its flattened segments. 
//  
//  The pen is defined with a "nominal" circle and possibly a transformation
//  matrix that maps it to an ellipse. Intuitively, the widened path is the
//  locus of points defined by the ellipse as it traces out the original path
//  (the "spine").  This definition is augmented to account for features such
//  as caps, joins (rounded, mitered or beveled), dashes and gaps.
//  
//  In practice, the widened path is constructed from a finite number of
//  instances of the ellipse, with straight line segments connecting them.
//  Curved segments on the spine are approximated by polygonal paths augmented
//  with tangency information for that purpose. 
//  
//  A vector from the origin to a point on the pen's nominal circle is called
//  "radius vector".  For every spine segment, we compute the radius vector
//  whose image (under the pen's matrix) is in the direction of that segment.
//  This vector is used to find the tip of a triangular or round cap on a
//  segment.  The radius vector perpendicular to it is used to find the "offset
//  points", which are points on the theoretical widened outline.  Between them
//  we draw straight lines, but this is only an approximation of the widened
//  path. When the widened path is very curved and wide, this is a poor
//  approximation, and the outline would look jagged. To prevent that, we apply
//  a (cheap) test to the radius vector of the a new point on a curve and
//  previous one radius vector.  When this test fails, we switch to a different
//  model, approximating the path with straight segments and rounded corners.
//  This is an analytical version of the Hobby algorithm.  It is not cheap -
//  that's why reserve it to extreme cases.
//
//------------------------------------------------------------------------------

//
// A pen is considered empty if one of its dimensions is less than tolerance /
// 128.  Since we are dealing with radii = 1/2 dimensions, we'll approximate
// tolerance/256 with tolerance * 0.004.
//
//  I'm not really sure where the 128 comes from. We
// should revisit this.
//
#define EMPTY_PEN_FACTOR 0.004

//
// The following is a lower bound on the length of a dash sequence. We are in
// device space, so anything smaller than ~1/8 is invisible with antialiasing.
// Moreover, the rasterizer becomes quadratic when the number of edges per
// sample is > 1.
//
#define MIN_DASH_ARRAY_LENGTH .1f

//
// We are implicitly assuming that we are working with a left handed coordinate
// system (which is the most common case, with x pointing right and y pointing
// down).  That's the case with GpPointR.TurnRight(), and say that the path is
// turning right when the determinant is positive.  The algorithms do not
// depend on this assumption. The only effect of a right hand coordinate system
// is on the orientation of the resulting path outline: It will go clockwise in
// a left handed system and counterclockwise in a right handed one.
//

enum _RAIL_SIDE
{
    RAIL_LEFT=0,
    RAIL_RIGHT=1
};

typedef __range(0,1) _RAIL_SIDE RAIL_SIDE;

enum _RAIL_TERMINAL
{
    RAIL_START=0,
    RAIL_END=1
};

typedef __range(0,1) _RAIL_TERMINAL RAIL_TERMINAL;

MIL_FORCEINLINE RAIL_SIDE TERMINAL2SIDE(RAIL_TERMINAL x) { return static_cast<RAIL_SIDE>(x); }
MIL_FORCEINLINE RAIL_SIDE OPPOSITE_SIDE(RAIL_SIDE x) { return static_cast<RAIL_SIDE>(1-x); }

//+-----------------------------------------------------------------------------
//
//  Class:
//      CMatrix22
//
//  Synopsis:
//      Implements a 2x2 matrix class 
//
//------------------------------------------------------------------------------
class CMatrix22
{
public:
    CMatrix22()
    {
        Reset();
    }

    CMatrix22(
        __in_ecount(1) const CMatrix22 &other
            // In The matrix to copy
        );

    CMatrix22(
        __in_ecount(1) const CMILMatrix &oMatrix
            // In: The CMILMatrix to copy from
        )
        : m_rM11(oMatrix.GetM11()), m_rM12(oMatrix.GetM12()),
          m_rM21(oMatrix.GetM21()), m_rM22(oMatrix.GetM22())
    {
    }

    ~CMatrix22() {}

    void Reset();

    void CMatrix22::Set(
        GpReal rM11,
            // In: The value to set for M11
        GpReal rM12,
            // In: The value to set for M12
        GpReal rM21,
            // In: The value to set for M21
        GpReal rM22
            // In: The value to set for M22
        );

    void Prepend(
        __in_ecount_opt(1) const CMILMatrix *pMatrix
        );

    bool Finalize(
        GpReal rEmptyThresholdSquared,
            // Lower bound for |determinant(this)|
        __out_ecount(1) CMatrix22 &oInverse
            // The inverse of this matrix
        );

    bool IsIsotropic(
        __out_ecount(1) GpReal &rSqMax
        ) const;

    // Apply the transformation to a vector
    void Transform(
        __inout_ecount(1) GpPointR &P
        ) const;

    void TransformColumn(
        __inout_ecount(1) GpPointR &P
        ) const;

    void PreFlipX();

    HRESULT Invert(); // Return InvalidParameter if not invertable

    HRESULT GetInverseQuadratic(
        __out_ecount(1) GpReal &rCxx,
            // Out: Coefficient of x*x
        __out_ecount(1) GpReal &rCxy,
            // Out: Coefficient of x*y
        __out_ecount(1) GpReal &rCyy
            // Out: Coefficient of y*y
        );

protected:
    // 4 matrix entries
    GpReal m_rM11;
    GpReal m_rM12;
    GpReal m_rM21;
    GpReal m_rM22;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CPenInterface
//  
//  Synopsis:
//      CPenInterface is an abstract class representing a pen or a dasher.  It
//      extends the CFlatteningSink interface.
//
//------------------------------------------------------------------------------
class CPenInterface     :   public CFlatteningSink
{
public:
    virtual HRESULT StartFigure(
        __in_ecount(1) const GpPointR &pt,
            // In: Figure's first point
        __in_ecount(1) const GpPointR &vec,
            // In: First segment's (non-zero) direction vector
        bool fClosed,
            // In: =true if we're starting a closed figure
        MilPenCap::Enum eCapType
            // In: The start cap type
        )=0;

    virtual HRESULT DoCorner(
        __in_ecount(1) const GpPointR &ptCenter,
            // Corner center point
        __in_ecount(1) const GpPointR &vecIn,
            // Vector in the direction coming in
        __in_ecount(1) const GpPointR &vecOut,
            // Vector in the direction going out
        MilLineJoin::Enum eLineJoin,
            // Corner type
        bool fSkipped,
            // =true if this corner straddles a degenerate segment 
        bool fRound,
            // Enforce rounded corner if true 
        bool fClosing 
            // This is the last corner in a closed figure if true
        )=0;

    virtual HRESULT EndStrokeOpen(
        bool fStarted,
            // = true if the widening has started
        __in_ecount(1) const GpPointR &ptEnd,
            // Figure's endpoint
        __in_ecount(1) const GpPointR &vecEnd,
            // Direction vector there
        MilPenCap::Enum eEndCap,
            // The type of the end cap
        MilPenCap::Enum eStartCap=MilPenCap::Flat
            // The type of start cap (optional)
        )=0;

    virtual HRESULT EndStrokeClosed(
        __in_ecount(1) const GpPointR &ptEnd,
            // Figure's endpoint
        __in_ecount(1) const GpPointR &vecEnd
            // Direction vector there
        )=0;

    virtual HRESULT AcceptCurvePoint(
        __in_ecount(1) const GpPointR &point,
            // In: The point
        __in_ecount(1) const GpPointR &vec,
            // In: The tangent there
        bool fLast = false
            // In: Is this the last point on the curve? (optional)
        )=0;   

    virtual HRESULT AcceptLinePoint(
        __in_ecount(1) const GpPointR &point
            // In: The point
        )=0;  

    //
    // CFlatteningSink override
    //
    // Here is the reason for using different names for the same thing.
    // CPenInterface distinguishes between points on a line segment and points
    // on a curve.  On line segment the direction is known, and there is no
    // need for a tangent.  On the other hand, ALL the points sent to
    // CFlatteningSink are on a curve, whether they come with or without
    // tangents.  GetPointOnTangent is called if fWithTangent=true.
    // CPenInterface happens to need tangents for points that come from a
    // curve, so the call from the flattener is routed as a point from a curve.
    // There should be no cost for this routing.
    //
    virtual HRESULT AcceptPointAndTangent(
        __in_ecount(1) const GpPointR &pt,
            // The point
        __in_ecount(1) const GpPointR &vec,
            // The tangent there
        bool fLast
            // Is this the last point on the curve?
        )
    {
        RRETURN(AcceptCurvePoint(pt, vec, fLast));
    }

    virtual bool Aborted()=0;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSegment
//  
//  Synopsis:
//      This class abstracts the concepts of line and cubic curve.  A segment
//      is a parametric mapping C(t) from the interval [0,1] to the plane. For
//      a line the mapping is  C(t) = P + t*V, where P is a point and V is a
//      vector. For a cubic curve the mapping is C(t) = C0 + C1*t + C[2]*t^2 +
//      C[3]*t^3. where C[i] are 2D coefficients.
//
//------------------------------------------------------------------------------
class CSegment
{
public:
    CSegment()
    {}

    virtual ~CSegment() {}

    virtual HRESULT Widen(
        __out_ecount(1) GpPointR &ptEnd,
            // End point
        __out_ecount(1) GpPointR &vecEnd
            // End direction vector
        )=0;

    virtual HRESULT GetFirstTangent(
        __out_ecount(1) GpPointR &vecTangent
            // Out: Nonzero direction vector
        ) const=0;

    // No data

};  // End of definition of the class CSegment

MtExtern(CLineSegment);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CLineSegment
//  
//  Synopsis:
//      Representation of a line segment.
//
//------------------------------------------------------------------------------
class CLineSegment  :   public CSegment
{
public:
    CLineSegment() 
    {
        m_rFuzz = MIN_TOLERANCE * SQ_LENGTH_FUZZ;
        m_pTarget = NULL;
    }

    CLineSegment( 
        GpReal rTolerance
            // Widening tolerance       
        )
    {
        m_pTarget = NULL;
        m_rFuzz = rTolerance * SQ_LENGTH_FUZZ;
    }

    virtual ~CLineSegment() {}

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CLineSegment));

    void SetTarget(
        __in_ecount_opt(1) CPenInterface *pTarget
        )
    {
        m_pTarget = pTarget;
    }

    void Set(
        double rStart,
            // Start parameter
        double rEnd,
            // End parameter
        __inout_ecount(1) GpPointR &ptFirst,
            // First point, transformed, possibly modified here
        __in_ecount(1) const MilPoint2F &ptLast,
            // Last point (raw)
        __in_ecount_opt(1) const CMILMatrix *pMatrix
            // Transformation matrix (NULL OK)
        );

    virtual HRESULT Widen(
        __out_ecount(1) GpPointR &ptEnd,
            // End point
        __out_ecount(1) GpPointR &vecEnd
            // End direction vector
        );

    virtual HRESULT GetFirstTangent(
        __out_ecount(1) GpPointR &vecTangent
            // Nonzero direction vector
        ) const;

    // Data
protected:
    GpPointR        m_ptEnd;         // End point
    GpPointR        m_vecDirection;  // Direction vector
    GpReal          m_rFuzz;         // Zero length fuzz
    CPenInterface   *m_pTarget;      // The widening target

};   // End of definition of the class CLineSegment

//+-----------------------------------------------------------------------------
//
//  Class:
//      CCubicSegment
//  
//  Synopsis:
//      Representation of a cubic Bezier segment.
//
//------------------------------------------------------------------------------
class CCubicSegment  :   public CSegment
{
public:
    CCubicSegment() 
    : m_oBezier(NULL, MIN_TOLERANCE)
    {
    }

    CCubicSegment(
        GpReal rTolerance
            // Widening tolerance       
        )
    : m_oBezier(NULL, rTolerance)
    {
    }

    virtual ~CCubicSegment() {}

    void SetTarget(
        __in_ecount_opt(1) CPenInterface *pTarget
        )
    {
        m_oBezier.SetTarget(pTarget);
    }

    void Set(
        double rStart,
            // Start parameter
        double rEnd,
            // End parameter
        __inout_ecount(1) GpPointR &ptFirst,
            // First point, transformed, possibly modified here
        __in_ecount(3) const MilPoint2F *ppt,
            // The rest of the points (raw)
        __in_ecount_opt(1) const CMILMatrix *pMatrix
            // Transformation matrix (NULL OK)
        );

    virtual HRESULT Widen(
        __out_ecount(1) GpPointR &ptEnd,
            // End point
        __out_ecount(1) GpPointR &vecEnd
          // End direction vector
        );

    virtual HRESULT GetFirstTangent(
        __out_ecount(1) GpPointR &vecTangent
              // Tangent vector there
        ) const
    {
        return m_oBezier.GetFirstTangent(vecTangent);
    }

    // Data
protected:
    CMILBezierFlattener     m_oBezier;

};  // End of definition of the class CCubicSegment

//+-----------------------------------------------------------------------------
//
//  Class:
//      CPen
//  
//  Synopsis:
//      Implements an (undashed) pen.
//
//------------------------------------------------------------------------------
class CPen  :   public CPenInterface
{
    // Construction/destruction

public:
    CPen();

    virtual ~CPen() {}

    bool Set(
        __in_ecount(1) const CPenGeometry &geom,
            // In: The pen's geometry information
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // In: W to D transformation matrix (NULL OK)
        GpReal rTolerance,
            // In: Ignored here
        __in_ecount_opt(1) const MilRectF *prcViewableInflated
            // In: Viewable region (inflated by stroke properties) (NULL okay)
        );

    void Copy(
        __in_ecount(1) const CPen &pen
            // A pen to copy basic properties from
        );

    HRESULT UpdateOffset(
        __in_ecount(1) const GpPointR &vecDirection
            // In: A nonzero direction vector
        );

    GpReal GetRadius() const
    {
        return m_rRadius;
    }

    void GetSqWidth(
        __in_ecount(1) const GpPointR &vec,
            // In: Direction vector
        __out_ecount(1) GpReal &rNum,
            // In: The result's numerator
        __out_ecount(1) GpReal &rDenom
            // In: The result's denominator
        ) const;

    HRESULT ComputeRadiusVector(     // Return InvalidParameter if vecDirection=0
        __in_ecount(1) const GpPointR &vecDirection,
            // In: A not necessarily unit vector
        __out_ecount(1) GpPointR &vecRad
            // Out: Radius vector on the pen circle
        ) const;     

    void SetRadiusVector(
        __in_ecount(1) const GpPointR &vecRad
            // In: A Given radius vector
        );

    virtual HRESULT AcceptCurvePoint(
        __in_ecount(1) const GpPointR &point,
            // In: The point
        __in_ecount(1) const GpPointR &vec,
            // In: The tangent there
        bool fLast = false
            // In: Is this the last point on the curve?
        );

    virtual HRESULT RoundTo(
        __in_ecount(1) const GpPointR &vecRad,
            // In: Radius vector of the outgoing segment
        __in_ecount(1) const GpPointR &ptCenter,
            // In: Corner center point
        __in_ecount(1) const GpPointR &vecIn,
            // In: Vector in the direction coming in
        __in_ecount(1) const GpPointR &vecOut
            // In: Vector in the direction going out
        )=0;

protected:
    bool SetPenShape(
        __in_ecount(1) const CPenGeometry &geom,
            // The pen's geometry information
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Render transform (NULL OK)
        GpReal rTolerance
            // Approximation tolerance
        );

    static GpReal ComputeRefinementThreshold(
        GpReal rMaxRadiusBound,
            // Bound on the maximum radius of the pen
        GpReal rTolerance
            // Approximation tolerance
        );

    void GetOffsetVector(
        __in_ecount(1) const GpPointR &vecRad,
            // In: A radius vector
        __out_ecount(1) GpPointR &vecOffset
            // Out: The corresponding offset vector
        ) const;

    GpPointR GetPenVector(
        __in_ecount(1) const GpPointR &vecRad
            // In: A radius vector
        ) const;   

    bool GetTurningInfo(
        __in_ecount(1) const GpPointR &vecIn,
            // In: Vector in the direction comin in
        __in_ecount(1) const GpPointR &vecOut,
            // In: Vector in the direction going out
        __out_ecount(1) GpReal &rDet,
            // Out: The determinant of the vectors
        __out_ecount(1) GpReal &rDot,
            // Out: The dot product of the vectors
        __out_ecount(1) RAIL_SIDE &side,
            // The outer side of the turn
        __out_ecount(1) bool &f180Degrees
            // Out: =true if this is a 180 degrees turn
            ) const;

    virtual HRESULT ProcessCurvePoint(
        __in_ecount(1) const GpPointR &point,
            // In: Point to draw to
        __in_ecount(1) const GpPointR &vecSeg
            // In: Direction of segment
        )=0;

protected:
    // Nominal pen data
    MilLineJoin::Enum  m_eLineJoin;        // Line join style (TO DO: Do we need this?)

    // Derived data - fixed
    CMatrix22   m_oMatrix;              // Pen's transformation matrix
    CMatrix22   m_oInverse;             // Inverse of m_oMatrix
    CMatrix22   m_oWToDMatrix;          // The world to device matrix (ignoring translation)
    GpReal      m_rRadius;              // Radius in pen coordinate space
    GpReal      m_rRadSquared;          // The above squared
    GpReal      m_rNominalMiterLimit;   // Nominal API miter limit
    GpReal      m_rMiterLimit;          // Miter limit multiplied by radius
    GpReal      m_rMiterLimitSquared;   // The above squared
    GpReal      m_rRefinementThreshold; // Stroke outline refinement threshold
    bool        m_fCircular;            // =true if the pen is circular
    bool        m_fViewableSpecified;   // =true m_rcViewableInflated should be used.
    CMilRectF   m_rcViewableInflated;   // The clip rect (if any), inflated to account for 
                                        // stroke width, miter-limit, dash/line caps.

    // Working variable data
    GpPointR    m_vecRad;           // Current radius vector
    GpPointR    m_vecOffset;        // Current offset vector
    GpPointR    m_ptPrev;           // Previous point
    GpPointR    m_vecPrev;          // Previous tangent vector
};

MtExtern(CSimplePen);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSimplePen
//  
//  Synopsis:
//      Implements an (undashed, simple) pen.
//
//  Notes:
//      The alternative is a (speced, but not implemented) complex pen, which
//      would allow multiple "prongs" on the pen (like a rake).
//
//      Some of the methods should be pushed up to the base class CPen.
//
//------------------------------------------------------------------------------
class CSimplePen    :   public CPen
{
public:
    CSimplePen()
        : m_pSink(NULL)
    {
    }

    bool Initialize(
        __in_ecount(1) const CPenGeometry &geom,
            // In: The pen's geometry information
        __in_ecount(1) const CMILMatrix *pMatrix,
            // In: W to D transformation matrix (NULL OK)
        GpReal rTolerance,
            // In: Ignored here
        __in_ecount_opt(1) const MilRectF *prcViewableInflated,
            // In: The viewableregion
        __inout_ecount(1) CWideningSink *pSink
            // In/out: The recipient of the results
        )
    {
        m_pSink = pSink;

        return Set(geom, pMatrix, rTolerance, prcViewableInflated);
    }

    // Constructor for stroking line shapes 

    virtual ~CSimplePen() {}

    void SetFrom(
        __in_ecount(1) const CPen &pen,
            // A pen to inherit some properties from
        __in_ecount(1) CWideningSink *pSink
            // The recipient of the results
        )
    {
        Copy(pen);
        m_pSink = pSink;
    }

    // CPenInterface override
    HRESULT StartFigure(
        __in_ecount(1) const GpPointR &pt,
            // In: Figure's firts point
        __in_ecount(1) const GpPointR &vec,
            // In: First segment's (non-zero) direction vector
        bool fClosed,
            // In: =true if we're starting a closed figure
        MilPenCap::Enum eCapType
            // In: The start cap type
        );

    virtual HRESULT DoCorner(
        __in_ecount(1) const GpPointR &ptCenter,
            // Corner center point
        __in_ecount(1) const GpPointR &vecIn,
            // Vector in the direction comin in
        __in_ecount(1) const GpPointR &vecOut,
            // Vector in the direction going out
        MilLineJoin::Enum eLineJoin,
            // Corner type
        bool fSkipped,
            // =true if this corner straddles a degenerate segment 
        bool fRound,
            // Enforce rounded corner if true 
        bool fClosing
            // This is the last corner in a closed figure if true
        );

    virtual HRESULT EndStrokeOpen(
        bool fStarted,
            // = true if the widening has started
        __in_ecount(1) const GpPointR &ptEnd,
            // Figure's endpoint
        __in_ecount(1) const GpPointR &vecEnd,
            // Direction vector there
        MilPenCap::Enum eEndCap,
            // The type of the end cap
        MilPenCap::Enum eStartCap=MilPenCap::Flat
            // The type of start cap (optional)
        );


    virtual HRESULT EndStrokeClosed(
        __in_ecount(1) const GpPointR &ptEnd,
            // Figure's endpoint
        __in_ecount(1) const GpPointR &vecEnd
            // Direction vector there
        );

    HRESULT AcceptLinePoint(
        __in_ecount(1) const GpPointR &point
            // In: Point to stop the pen at
        );

    virtual HRESULT RoundTo(
        __in_ecount(1) const GpPointR &vecRad,
            // In: Radius vector of the outgoing segment
        __in_ecount(1) const GpPointR &ptCenter,
            // In: Corner center point
        __in_ecount(1) const GpPointR &vecIn,
            // In: Vector in the direction comin in
        __in_ecount(1) const GpPointR &vecOut
            // In: Vector in the direction going out
        );

    virtual bool Aborted();

protected:
    // CPen overrides
    HRESULT ProcessCurvePoint(
        __in_ecount(1) const GpPointR &point,
            // In: Point to draw to
        __in_ecount(1) const GpPointR &vecSeg
            // In: Direction of segment we're coming along
        );

    // Private methods
    HRESULT DoInnerCorner(
        RAIL_SIDE side,
            // In: The side of the inner corner
        __in_ecount(1) const GpPointR &ptCenter,
            // In: The corner's center
        const GpPointR *ptOffset
            // In: The offset points of the new segment
        );

    HRESULT DoBaseCap(
        RAIL_TERMINAL whichEnd,
            // In: START or END
        __in_ecount(1) const GpPointR &ptCenter,
            // In: Cap's center
        __in_ecount(1) const GpPointR &vec,
            // In: Vector pointing out along the segment
        MilPenCap::Enum type
            // In: The type of cap
        );

    HRESULT DoSquareCap(
        RAIL_TERMINAL whichEnd,
            // START or END
        __in_ecount(1) const GpPointR &ptCenter
            // Cap's center
        );

    HRESULT DoRoundCap(
        RAIL_TERMINAL whichEnd,
            // In: START or END
        __in_ecount(1) const GpPointR &ptCenter
            // In: Cap's center
        );

    bool GetMiterPoint(
        __in_ecount(1) const GpPointR &vecRad,
            // In: Radius vector for the outgoing segment
        GpReal rDet,
            // In: The determinant of vecIn and vecOut
        __in_ecount(1) const GpPointR &ptIn,
            // In: Offset point of the incoming segment
        __in_ecount(1) const GpPointR &vecIn,
            // In: Vector in the direction comin in
        __in_ecount(1) const GpPointR &ptNext,
            // In: Offset point of the outgoing segment
        __in_ecount(1) const GpPointR &vecOut,
            // In: Vector in the direction going out
        __out_ecount(1) GpReal &rDot,
            // Out: The dot product of the 2 radius vectors
        __out_ecount(1) GpPointR &ptMiter
            // Out: The outer miter point, if within limit
        );

    HRESULT DoLimitedMiter(
        __in_ecount(1) const GpPointR &ptIn,
            // In: Outer offset of incoming segment
        __in_ecount(1) const GpPointR &ptNext,
            // In: Outer offset of outgoing segment
        GpReal rDot,
            // In:  -(m_vecRad * vecRadNext)
        __in_ecount(1) const GpPointR &vecRadNext,
            // In: Radius vector of outgoing segment
        RAIL_SIDE side
            // In: Turn's outer side, RAIL_LEFT or RAIL_RIGHT
        );

    HRESULT Do180DegreesMiter();

    HRESULT BevelCorner(
        RAIL_SIDE side,
            // In: The side of the outer corner
        __in_ecount(1) const GpPointR &ptNext
            // In: The bevel's endpoint
        );

    HRESULT RoundCorner(
        __in_ecount(1) const GpPointR &ptCenter,
            // In: Corner point on the spine
        __in_ecount(1) const GpPointR &ptIn,
            // In: Outer offset points of incoming segment
        __in_ecount(1) const GpPointR &ptNext,
            // In: Outer offset points of outgoing segment
        __in_ecount(1) const GpPointR &vecRad,
            // In: New value of m_vecRad
        RAIL_SIDE side
            // In: Side to be rounded, RAIL_LEFT or RAIL_RIGHT
        );

    HRESULT SetCurrentPoints(
        __in_ecount(1) const GpPointR &ptLeft,
            // In: Left point
        __in_ecount(1) const GpPointR &ptRight
            // In: Right point
        );

    HRESULT MiterTo(
        RAIL_SIDE side,
            // Which side to set the point
        __in_ecount(1) const GpPointR &ptMiter,
            // Miter corner
        __in_ecount(1) const GpPointR &ptNextStart,
            // The starting point of the next segment's offset
        bool fExtended
            // Extend all the way to ptNextStart if true
        );

// Data
protected:
    CWideningSink *m_pSink;        // The sink that accepts the results
    GpPointR       m_ptCurrent[2]; // The current left & right points
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CDasher
//  
//  Synopsis:
//      Adapter to a CPen that provides dashing functionality.
//
//  Notes:
//
//      DASHED LINES DESIGN
//                  
//      There are several widening scenarios.  The following crude diagrams
//      depict the flow of information:
//      
//      Simple pen without dashes: CWidener --> CSimplePen --> CWideningSink
//          
//      Simple pen with dashes: CWidener --> CDasher --> CSimplePen -->
//      CWideningSink
//              
//      Glossary An EDGE is a smooth piece of the figure between corners or
//      start and end.  The edge is a sequence of SEGMENTS.  If the edge is a
//      straight line then it comprises one segment.  If it is a curve then the
//      segments are the result of its flattening.
//      
//      CDasher accumulates segments with the information needed for widening
//      and accumulated length. At every corner (between edges) and at the
//      figure end, the Dasher flushes the segments buffer and sends the dashes
//      to the pen to draw.
//      
//      The buffer must contain all the information needed for the pen at flush
//      time, so we record points, tangents, and a flag indicating whether the
//      segment came from a line segment (rather than from curve flattening)
//      
//      If the figure is closed then the first dash may have to be the second
//      half of the last dash.  So if it starts on a dash, we'll start it with
//      a flat cap.  After the last dash we'll do the corner (between figure
//      end and start) and exit with a flat cap, that will abut with the flat
//      cap of the first dash.  If there is no end dash then we'll append a
//      0-length segment with the right cap.
//      
//      Some of the functionality of CDasher is delegated to its class members
//      CSegments, capturing the data and behavior of the segments buffer, and
//      CDashSequence, encapsulating the dash sequence.
//      
//      We dash one edge at a time. We try to dash it in a synchronized mode,
//      i.e.  always ending at the same point (=DashOffset) in the dash
//      sequence. For that we tweak the sequence length. But if the edge is
//      substantially shorter than one full instance then we dash in
//      unsynchronized mode. For the canned dash styles the offset is set to
//      half the first dash. 
//
//------------------------------------------------------------------------------
class CDasher   :   public CPenInterface
{

// Disallow instantiation without a pen
private:
    CDasher()
    : m_pPen(NULL)
    {
    }

public:
    CDasher(
        __in_ecount(1) CPen *pPen
            // In: The internal widening pen
        );

    virtual ~CDasher()
    {
    }

    HRESULT Initialize(
        __in_ecount(1) const CPlainPen &pen,
            // In: The pen we stroke with
        __in_ecount_opt(1) CMILMatrix const *pMatrix,
            // In: Render transform (NULL OK)
        __in_ecount_opt(1) const MilRectF *prcViewableInflated
            // In: The viewable region, inflated by the stroke properties (NULL
            // OK)
        );

    // CPenInterface override
    virtual HRESULT StartFigure(
        __in_ecount(1) const GpPointR &pt,
            // In: Figure's firts point
        __in_ecount(1) const GpPointR &vec,
            // In: First segment's (non-zero) direction vector
        bool fClosed,
            // In: =true if we're starting a closed figure
        MilPenCap::Enum eCapType
            // In: The start cap type
        );

    virtual HRESULT AcceptLinePoint(
        __in_ecount(1) const GpPointR &point
            // In: Point to draw to
        );

    virtual HRESULT AcceptCurvePoint(
        __in_ecount(1) const GpPointR &point,
            // In: The point
        __in_ecount(1) const GpPointR &vec,
            // In: The tangent there
        bool fLast
            // In: Is this the last point on the curve?
        );

    virtual HRESULT DoCorner(
        __in_ecount(1) const GpPointR &ptCenter,
            // Corner center point
        __in_ecount(1) const GpPointR &vecIn,
            // Vector in the direction comin in
        __in_ecount(1) const GpPointR &vecOut,
            // Vector in the direction going out
        MilLineJoin::Enum eLineJoin,
            // Corner type
        bool fSkipped,
            // =true if this corner straddles a degenerate segment 
        bool fRound,
            // Enforce rounded corner if true 
        bool fClosing
            // This is the last corner in a closed figure if true
        );

    virtual HRESULT EndStrokeOpen(
        bool fStarted,
            // = true if the widening has started
        __in_ecount(1) const GpPointR &ptEnd,
            // Figure's endpoint
        __in_ecount(1) const GpPointR &vecEnd,
            // Direction vector there
        MilPenCap::Enum eEndCap,
            // The type of the end cap
        MilPenCap::Enum eStartCap=MilPenCap::Flat
            // The type of start cap (optional)
        );

    virtual HRESULT EndStrokeClosed(
        __in_ecount(1) const GpPointR &ptEnd,
            // Figure's endpoint
        __in_ecount(1) const GpPointR &vecEnd
            // Direction vector there
        );


    virtual bool Aborted()
    {
        return m_pPen->Aborted();
    }

protected:
    // Private methods
    HRESULT StartANewDash(
        GpReal rLoc,
            // In: The location
        GpReal rWorldSpaceLength,
            // In: The dash's length in world-space
        bool fAtVertex
            // In: =true if we're at a segment start or end
        );

    HRESULT ExtendCurrentDash(
        GpReal rLoc,
            // In: The location
        bool fAtVertex
            // In: =true if we're at a segment start or end
        );

    HRESULT TerminateCurrentDash(
        GpReal rLoc,
            // In: The location
        bool fAtVertex
            // In: =true if we're at a segment start or end
        );

    HRESULT Flush(
        bool fLastEdge
            // =true if this is the figure's last edge
        );

    HRESULT DoDashOrGapEndAtEdgeEnd(
        bool fLastEdge,
            // =true if this is the figure's last edge
        bool fIsOnDash
            // =true if we are on a dash    
        );

//+-----------------------------------------------------------------------------
//
//  Struct:
//      CDasher::CSegData
//  
//  Synopsis:
//      Stores information about a given segment.
//
//------------------------------------------------------------------------------
    class CSegData
    {
    public:
        CSegData()
        : m_rLocation(0), m_fIsALine(false)
        {
        }

        CSegData(
            bool fIsALine,
                // In: =true if this is a line segment
            __in_ecount(1) const GpPointR &ptEnd,
                // In: Segment endpoint
            __in_ecount(1) const GpPointR &vecTangent,
                // In: The tangent vector there
            __in_ecount(1) const GpPointR &vecSeg,
                // In: The segment direction unit vector
            GpReal rLocation,
                // In: Accummulated length so far
            GpReal rDashScaleFactor,
                // In: Amount by which dashes will be scaled along this segment
            bool fBezierEnd
                // In: True if this is the last segment on a Bezier
            );

        // Really a data structure, not a class, so data is left public
        GpPointR            m_ptEnd;      // The segment endpoint
        GpPointR            m_vecTangent; // Tangent vector there
        GpPointR            m_vecSeg;     // The segment direction unit vector
        GpReal              m_rLocation;  // Accummulated length to this point
        GpReal              m_rDashScaleFactor;// Amount by which dashes will 
                                               // be scaled along this segment
        bool                m_fIsALine;   // =true if this is a line segment
        bool                m_fBezierEnd; // =true if this the last segment in a Bezier
    };

//+-----------------------------------------------------------------------------
//
//  Class:
//      CDasher::CSegments
//  
//  Synopsis:
//      Stores the yet-to-be processed segments.
//
//------------------------------------------------------------------------------
    class CSegments
    {
    public:
        CSegments()
            : m_uCurrentSegment(1), m_rCxx(1), m_rCxy(0), m_rCyy(1) {}

        HRESULT Initialize(
            __in_ecount_opt(1) const CMILMatrix *pMatrix
                // In: Transformation matrix (NULL OK)
            );

        HRESULT StartWith(
            __in_ecount(1) const GpPointR &ptStart,
                // In: Starting point
            __in_ecount(1) const GpPointR &vecTangent
                // In: The tangent vector there
            );

        HRESULT Add(
            __in_ecount(1) const GpPointR &ptEnd,
                // In: Segment endpoint
            __in_ecount_opt(1) const GpPointR *pTangent=NULL,
                // Optional: Tangent vector there
            bool fBezierEnd=false
                // Optional: true if this is the last segment on a Bezier
            );

        GpReal GetLength() const
        {
            return m_rgSegments.Last().m_rLocation;
        }

        bool IsAtALine() const
        {
            return m_rgSegments[m_uCurrentSegment].m_fIsALine;
        }

        bool IsAtBezierEnd() const
        {
            return m_rgSegments[m_uCurrentSegment].m_fBezierEnd;
        }

        bool IsLast() const
        {
            return m_rgSegments.GetCount() - 1 == m_uCurrentSegment;
        }

        bool IsEmpty() const
        {
            // The first entry is just the starting point.
            return m_rgSegments.GetCount() < 2;
        }

        GpReal GetCurrentEnd() const
        {
            return m_rgSegments[m_uCurrentSegment].m_rLocation;
        }

        GpReal GetCurrentDashScaleFactor() const
        {
            return m_rgSegments[m_uCurrentSegment].m_rDashScaleFactor;
        }

        GpPointR GetCurrentDirection() const
        {
            return m_rgSegments[m_uCurrentSegment].m_vecTangent;
        }

        bool Increment()
        {
            m_uCurrentSegment++;
            return m_uCurrentSegment >= m_rgSegments.GetCount();
        }

        void Reset();

        void ProbeAt(
            GpReal rLoc,
                // In: The location (lengthwise) to probe at
            __out_ecount(1) GpPointR &pt,
                // Out: Point there
            __out_ecount(1) GpPointR &vecTangent,
                // Out: Tangent vector at segment end
            bool fAtSegEnd
                // In: At segment end if true
            ) const;

    protected:
        // Data
        UINT                        m_uCurrentSegment; // Current segment index
        DynArrayIA<CSegData, 16>    m_rgSegments;    // Segments buffer
                                // The first entry is the first segment's start

        // Coefficients of the pre-transform length quadratic form
        GpReal                      m_rCxx;          // Coefficient of x*x
        GpReal                      m_rCxy;          // Coefficient of x*y
        GpReal                      m_rCyy;          // Coefficient of y*y
    };

//+-----------------------------------------------------------------------------
//
//  Class:
//      CDasher::CDashSequence
//  
//  Synopsis:
//      Stores the sequence of dashes to apply to the stroke.
//
//  Notes:
//      All APIs take arguments in edge space (edge space records how far we
//      have travelled along an edge). Internally, all our computations are
//      done in dash space (how far along in the dash array we are). Note that
//      dash space is calculated modulo the length of the dash array, so in
//      order to convert from dash space to edge space we have to keep track of
//      how many times we've iterated over the dash array
//      (m_uCurrentIteration).
//
//------------------------------------------------------------------------------
    class CDashSequence
    {
    public:
        // Constructor
        CDashSequence();

        HRESULT Initialize(
            __in_ecount(1) const CPlainPen &pen
                // In: The nominal pen
            );

        bool IsOnDash() const
        {
            return 0 != (m_uCurrentDash & 1);
        }

        GpReal GetStep() const
        {
            return m_rgDashes[m_uCurrentDash] - m_rCurrentLoc;
        }

        GpReal GetNextEndpoint() const
        {
            return DashToEdgeSpace(m_rgDashes[m_uCurrentDash]);
        }

        void PrepareForNewEdge()
        {
            // Should be checked at ::Set, otherwise flushing an edge may result
            // in an infinite loop.
            Assert(2 * m_rLength >= MIN_DASH_ARRAY_LENGTH);

            m_uCurrentIteration = 0;
            m_rEdgeSpace0 = m_rCurrentLoc;
        }

        void Reset();

        void AdvanceTo(
            GpReal rEdgeSpaceLoc
                // New location (in edge space)
            )
        {
            m_rCurrentLoc = EdgeToDashSpace(rEdgeSpaceLoc);
            //
            // Future Consideration:  Ideally, we should be able to assert
            // that m_rCurrentLoc <= m_rgDashes[m_uCurrentDash], but this isn't
            // necessarily true, since floating point error can cause this
            // class and CDasher::Flush to get out of sync (in particular,
            // m_rInverseFactor is computed using only single precision). This
            // is no biggie, though, since this just means that the dash will
            // be a little longer than it should be. Still, it'd be nice to
            // clean up this inconsistency.
            //
        }

        void Increment();

        GpReal GetLengthOfNextDash() const
        {
            UINT iStart;

            if (m_uCurrentDash == m_rgDashes.GetCount() - 1)
            {
                iStart = 0;
            }
            else
            {
                iStart = m_uCurrentDash;
            }

            return m_rgDashes[iStart+1] - m_rgDashes[iStart];
        }

    protected:

        GpReal DashToEdgeSpace(GpReal rDashSpaceLoc) const
        {
            //
            // If the dash array has very long dashes, it's easy to get NaNs,
            // even though the behavior may be well-defined. To guard against
            // this, we special case the m_uCurrentIteration == 0 case.
            //

            if (m_uCurrentIteration == 0)
            {
                return rDashSpaceLoc - m_rEdgeSpace0;
            }
            else
            {
                return rDashSpaceLoc - m_rEdgeSpace0 +
                    m_uCurrentIteration * m_rLength;
            }
        }

        GpReal EdgeToDashSpace(GpReal rEdgeSpaceLoc) const
        {
            //
            // If the dash array has very long dashes, it's easy to get NaNs,
            // even though the behavior may be well-defined. To guard against
            // this, we special case the m_uCurrentIteration == 0 case.
            //

            if (m_uCurrentIteration == 0)
            {
                return rEdgeSpaceLoc + m_rEdgeSpace0;
            }
            else
            {
                return rEdgeSpaceLoc
                    - m_uCurrentIteration * m_rLength
                    + m_rEdgeSpace0;
            }

        }

    protected:

        // Data
        UINT            m_uCurrentDash;      // Current dash/space end index
        UINT            m_uCurrentIteration; // Number of times we've wrapped around since
                                             // the last call to PrepareForNewEdge()
        GpReal          m_rCurrentLoc;       // Current location in the dash sequence
                                             // (in dash coordinates)
        GpReal          m_rEdgeSpace0;       // The value of m_rCurrentLoc at the time
                                             // of the last PrepareForNewEdge()
        GpReal          m_rLength;           // Sequence's total length
        UINT            m_uStartDash;        // The dash/space where the dash sequence starts
        DynArrayIA<GpReal, 16> m_rgDashes;   // Dash/space ends array
    };

    // Data
    CSegments       m_oSegments;            // Segments buffer
    CDashSequence   m_oDashes;              // The dash sequence
    CPen            *m_pPen;                // The widening pen
    CMilRectF       m_rcViewableInflated;   // The viewable bounds (if any), inflated to account for 
                                            // stroke width, miter-limit, dash/line caps.
    bool            m_fViewableSpecified;   // =true m_rcViewableInflated should be used.

    MilPenCap::Enum m_eDashCap;             // Dash cap
    bool            m_fIsPenDown;           // =true when we're on a dash
    bool            m_fIsFirstCapPending;   // =true if the first cap is yet to be set
    bool            m_fIgnoreDash;          // =true Dash creation instructions should be ignored.
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CWidener
//
//  Synopsis:
//      Generates a figure that approximates the stroke of a pen along a
//      figure.
//
//------------------------------------------------------------------------------
class CWidener
{
public:
    CWidener(
        GpReal rTolerance = MIN_TOLERANCE
            // In: Approximation tolerance
        );

    HRESULT Initialize(
        __in_ecount(1) const CPlainPen &pen,
            // The stroking pen
        __in_ecount(1) CWideningSink *pSink,
            // The widening sink
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Render transform (NULL OK)
        __in_ecount_opt(1) const CMILSurfaceRect *prcViewable,
            // The viewable region (NULL OK)
        __out_ecount(1) bool &fEmpty
            // The pen is empty if true
        );

    bool SetSegmentForWidening(
        __in_ecount(1) const IFigureData &oFigure,
            // The figure
        __inout_ecount(1) GpPointR &ptFirst,
            // First point, transformed, possibly modified here
        __in_ecount_opt(1) const CMILMatrix  *pMatrix
            // Transformation matrix (NULL OK)
        ) const;

    HRESULT Widen(
        __in_ecount(1) const IFigureData &oFigure,
            // The figure
        __in_ecount_opt(1) CStartMarker *pStartMarker,
            // Start shape marker (NULL OK)
        __in_ecount_opt(1) CEndMarker *pEndMarker
            // End shape marker (NULL OK)
        ) const;

    HRESULT WidenClosedFigure(
        __in_ecount(1) const IFigureData &oFigure
            // The figure
        ) const;

    HRESULT WidenOpenFigure(
        __in_ecount(1) const IFigureData &oFigure,
            // The figure
        __in_ecount_opt(1) CStartMarker *pStartMarker,
            // Start shape marker (NULL OK)
        __in_ecount_opt(1) CEndMarker *pEndMarker
            // End shape marker (NULL OK)
        ) const;

    HRESULT DoGap(
        __in_ecount(1) const IFigureData &oFigure
           // The figure
        ) const;

    HRESULT DoSegment(
        __in_ecount(1) const IFigureData &oFigure
            // The figure
        ) const;

    HRESULT SetForLineShape(
        __in_ecount(1) const CWidener &other,
            // The widener used for of the path to which  the line shape is attached
        __in_ecount(1) const CLineShape &shape,
            // The line shape we're attaching
        __in_ecount(1) CWideningSink *pSink,
            // The widening sink
        __out_ecount(1) bool &fEmpty
            // =true if the pen is empty
        );

    HRESULT WidenLineShape(
        __in_ecount(1) const CShape &shape,
            // The widened shape
        __in_ecount_opt(1) const CMILMatrix *pMatrix
            // The positioning matrix
        );

    __out_ecount(1) const CPen &GetPen() const
    {
        return m_pen;
    }

    void SetTarget(
        __in_ecount_opt(1) CPenInterface *pTarget
        )
    {
        m_pTarget = pTarget;
        m_oLine.SetTarget(m_pTarget);
        m_oCubic.SetTarget(m_pTarget);
    }

protected:
    HRESULT GetViewableInflated(
        __in_ecount(1) const CMILSurfaceRect *prcViewable,
            // The viewable region
        __in_ecount(1) const CPlainPen &pen,
            // The stroking pen
        __in_ecount_opt(1) const  CMILMatrix *pMatrix,
            // Transformation matrix (NULL OK)
        __out_ecount(1) CMilRectF *prcViewableInflated
            // The viewable region, inflated by the stroke properties
        )
    {
        HRESULT hr = S_OK;
        float pad;

        IFC(pen.GetExtents(OUT pad));

        {
            CMilPoint2F padVector(pad, pad);

            if (pMatrix)
            {
                pMatrix->TransformAsVectors(&padVector, &padVector, 1 /* one vector */);
                padVector.X = fabs(padVector.X);
                padVector.Y = fabs(padVector.Y);
            }

            prcViewableInflated->left = static_cast<FLOAT>(prcViewable->left) - padVector.X;
            prcViewableInflated->right = static_cast<FLOAT>(prcViewable->right) + padVector.X;
            prcViewableInflated->top = static_cast<FLOAT>(prcViewable->top) - padVector.Y;
            prcViewableInflated->bottom = static_cast<FLOAT>(prcViewable->bottom) + padVector.Y;
        }

    Cleanup:
        RRETURN(hr);
    }
        

    // Data
protected:

    //
    // Fixed data members, set for the life of the widener, which is one call
    // of CShapeBase::WidenToSink
    //

    const CMILMatrix  *m_pMatrix;       // Transformation matrix
    GpReal            m_rTolerance;     // Approximation tolerance

    //
    // The caps seem to duplicate the pen's start/end cap settings, but the
    // widener's target may be dasher.  In that case, the pen's caps will vary  
    // between original start/end cap and dash cap.  So here we keep track of
    // the original cap types
    //

    MilPenCap::Enum      m_eStartCap;        // Start cap style
    MilPenCap::Enum      m_eEndCap;          // End cap style
    MilPenCap::Enum      m_eDashCap;         // Dash cap style

    MilLineJoin::Enum     m_eLineJoin;        // Line join style
    CSimplePen      m_pen;              // Widening pen
    CDasher         m_dasher;
    CPenInterface   *m_pTarget;         // Pen or Dasher

    //
    // Working variables
    // Fixed for the duration of a figure
    //

    mutable GpPointR    m_vecStart;     // Figure's start direction vector
    mutable GpPointR    m_ptStart;      // Figure's first point
    mutable GpReal      m_rStartTrim;   // Trim parameter at figure start
    mutable GpReal      m_rEndTrim;     // Trim parameter at figure end

    // 
    // Changing between segments
    //

    mutable GpPointR    m_vecIn;        // Corner incoming direction vector
    mutable GpPointR    m_vecOut;       // Corner outgoing direction vector
    mutable GpPointR    m_pt;           // Current point
    mutable bool   m_fShouldPenBeDown;  // =true when we should be drawing
    mutable bool        m_fIsPenDown;   // =true when we are actually drawing
    mutable bool        m_fClosed;      // =true if widened as a closed figure
    mutable bool        m_fSkipped;     // =true if recent degenerate segment was skipped
    mutable bool        m_fSkippedFirst;// =true if first degenerate segment was skipped
    mutable MilPenCap::Enum  m_eCap;    // Current start-cap style (may be dash style by gaps)
    mutable bool  m_fNeedToRecordStart; // =true if we need to record the first point and vector
    mutable bool        m_fSmoothJoin;  // =true if the join is known to be smooth

    // 
    // The data of the current segment, set for widening
    //
    mutable CLineSegment  m_oLine;      // Line segment data
    mutable CCubicSegment m_oCubic;     // Cubic curve data
    mutable CSegment      *m_pSegment;  // The current choice between the above
};

MtExtern(CRail);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CRail
//  
//  Synopsis:
//      Stores edges belonging to one of the two "rails" (inner or outer)
//      belonging to the outline of the stroke.
//
//------------------------------------------------------------------------------
class CRail : public CFigureData
{
public:

    // Constructor/destructor
    CRail() {}
    virtual ~CRail(){}

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CRail));

    // Methods
    HRESULT ExtendTo(
        __in_ecount(1) const GpPointR &ptTo,
            // The target point
        __in_ecount(1) const GpPointR &vec,
            // The direction vector to test against
        __in_ecount(1) const GpPointR &pt1,
            // First point to draw to in case of gap
        __in_ecount(1) const GpPointR &pt2,
            // Second point to draw to in case of gap
        __in_ecount(1) const GpPointR &pt3,
            // Third point to draw to in case of gap
        __inout_ecount(1) CShape &shape
            // Shape we are widening to
        );

    HRESULT SetCurrentPoint(
        __in_ecount(1) const GpPointR &P
            // In: The point to set to
        );

    HRESULT ReverseJoin(
        __in_ecount(1) const CRail &other
            // In: The rail to concatinate
        );
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CShapeWideningSink
//  
//  Synopsis:
//      CWideningSink abstracts the recipient of the results of path widening.
//      The various implementations support generating the outline path,
//      computing bounds and hit testing
//
//------------------------------------------------------------------------------
class CWideningSink
{
public:

    CWideningSink() {}
    virtual ~CWideningSink() {}

    virtual HRESULT StartWith(
        __in_ecount(2) const GpPointR *ptOffset
            // Left and right offset points
        )=0;

    virtual HRESULT QuadTo(
        __in_ecount(2) const GpPointR *ptOffset
            // In: Left & right offset points
        )=0;

    virtual HRESULT QuadTo(
        __in_ecount(2) const GpPointR *ptOffset,
            // Left & right offset points thereof
        __in_ecount(1) const GpPointR &vecSeg,
            // Segment direction we're coming from 
        __in_ecount(1) const GpPointR &ptSpine,
            // The corresponding point on the stroke's spine
        __in_ecount(1) const GpPointR &ptSpinePrev
            // The previous point on the stroke's spine
        )=0;

    virtual HRESULT CurveWedge(
        RAIL_SIDE side,
            // Which side to add to
        __in_ecount(1) const GpPointR &ptBez_1,
            // First control point
        __in_ecount(1) const GpPointR &ptBez_2,
            // Second control point
        __in_ecount(1) const GpPointR &ptBez_3
            // Last point
        )=0;

    virtual HRESULT BezierCap(
        __in_ecount(1) const GpPointR &ptStart,
            // The cap's first point,
        __in_ecount(1) const GpPointR &pt0_1,
            // First arc's first control point
        __in_ecount(1) const GpPointR &pt0_2,
            // First arc's second control point
        __in_ecount(1) const GpPointR &ptMid,
            // The point separating the 2 arcs
        __in_ecount(1) const GpPointR &pt1_1,
            // Second arc's first control point
        __in_ecount(1) const GpPointR &pt1_2,
            // Second arc's second control point
        __in_ecount(1) const GpPointR &ptEnd
            // The cap's last point
        )=0;

    virtual HRESULT SetCurrentPoints(
        __in_ecount(2) const GpPointR *P
            // In: Array of 2 points
        )=0;

    virtual HRESULT DoInnerCorner(
        RAIL_SIDE side,
            // In: The side of the inner corner
        __in_ecount(1) const GpPointR &ptCenter,
            // In: The corner's center
        __in_ecount(2) const GpPointR *ptOffset
            // In: The offset points of new segment
        )=0;

    virtual HRESULT CapTriangle(
        __in_ecount(1) const GpPointR &ptStart,
            // Triangle base start point
        __in_ecount(1) const GpPointR &ptApex,
            // Triangle's apex
        __in_ecount(1) const GpPointR &ptEnd
            // Triangle base end point
        )=0;

    virtual HRESULT CapFlat(
        __in_ecount(1) const GpPointR *,
            // Ignored
        RAIL_SIDE
            // Ignored
        )
    {
        // Do nothing stub, OK to call
        return S_OK;
    }

    virtual HRESULT PolylineWedge(
        RAIL_SIDE side,
            // Which side to add to - RAIL_RIGHT or RAIL_LEFT
        UINT count,
            // Number of points
        __in_ecount(count) const GpPointR *pPoints
            // The polyline vertices
        )=0;

    virtual HRESULT AddFigure()
    {
        // Do nothing stub, OK to call
        return S_OK;
    }

    virtual HRESULT SwitchSides()=0;
   
    virtual HRESULT AddFill(
        __in_ecount(1) const CShape &oShape,
            // The shape to be filled
        __in_ecount(1) const CMILMatrix &matrix
            // Transformation
        )=0;

    virtual bool Aborted()
    {
        // Most sinks never abort
        return false;
    }

    // No data
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CShapeWideningSink
//  
//  Synopsis:
//      A sink for the widener that populates a new CShape.  It maintains two
//      rails, one for each side of the widened outline.  The right rail
//      accumulates the results.
//
//------------------------------------------------------------------------------
class CShapeWideningSink :   public CWideningSink
{
public:
    CShapeWideningSink(
        __in_ecount(1) CShape &oShape
            // The shape receiving the results
        );

    virtual ~CShapeWideningSink();

    // CwideningSink overrides
    virtual HRESULT StartWith(
        __in_ecount(2) const GpPointR *ptOffset
            // Left and right offset points
        );

    virtual HRESULT QuadTo(
        __in_ecount(2) const GpPointR *ptOffset
            // In: Left & right offset points
        );

    virtual HRESULT QuadTo(
        __in_ecount(2) const GpPointR *ptOffset,
            // Left & right offset points thereof
        __in_ecount(1) const GpPointR &vecSeg,
            // Segment direction we're coming from 
        __in_ecount(1) const GpPointR &ptSpine,
            // The corresponding point on the stroke's spine
        __in_ecount(1) const GpPointR &ptSpinePrev
            // The previous point on the stroke's spine
        );

    virtual HRESULT CurveWedge(
        RAIL_SIDE side,
            // Which side to add to
        __in_ecount(1) const GpPointR &ptBez_1,
            // First control point
        __in_ecount(1) const GpPointR &ptBez_2,
            // Second control point
        __in_ecount(1) const GpPointR &ptBez_3
            // Last point
        );

    virtual HRESULT BezierCap(
        __in_ecount(1) const GpPointR &,
            // Ignored here
        __in_ecount(1) const GpPointR &pt0_1,
            // First arc's first control point
        __in_ecount(1) const GpPointR &pt0_2,
            // First arc's second control point
        __in_ecount(1) const GpPointR &ptMid,
            // The point separating the 2 arcs
        __in_ecount(1) const GpPointR &pt1_1,
            // Second arc's first control point
        __in_ecount(1) const GpPointR &pt1_2,
            // Second arc's second control point
        __in_ecount(1) const GpPointR &ptEnd
            // The cap's last point
        );

    virtual HRESULT SetCurrentPoints(
        __in_ecount(2) const GpPointR *P
            // In: Array of 2 points
        );

    virtual HRESULT DoInnerCorner(
        RAIL_SIDE side,
            // In: The side of the inner corner
        __in_ecount(1) const GpPointR &ptCenter,
            // In: The corner's center
        __in_ecount(2) const GpPointR *ptOffset
            // In: The offset points of new segment
        );

    virtual HRESULT CapTriangle(
        __in_ecount(1) const GpPointR &ptStart,
            // Triangle base start point
        __in_ecount(1) const GpPointR &ptApex,
            // Triangle's apex
        __in_ecount(1) const GpPointR &ptEnd
            // Triangle base end point
        );

    virtual HRESULT CapFlat(
        __in_ecount(2) const GpPointR *ppt,
            // In: The base points
        RAIL_SIDE side
            // In: Which side the cap endpoint is
        );

    virtual HRESULT SwitchSides();

    virtual HRESULT PolylineWedge(
        RAIL_SIDE side,
            // Which side to add to - RAIL_RIGHT or RAIL_LEFT
        UINT count,
            // Number of points
        __in_ecount(count) const GpPointR *pPoints
            // The polyline vertices
        );
    
    virtual HRESULT AddFill(
        __in_ecount(1) const CShape &oShape,
            // The shape to be filled
        __in_ecount(1) const CMILMatrix &matrix
            // Transformation
        );

    virtual const GpPointR GetCurrentPoint(
        RAIL_SIDE side
            // In: Left or right
        )
    {
        return m_pRail[side]->GetEndPoint();
    }

    virtual HRESULT AddFigure();

// Data
protected:
    CRail    *m_pRail[2];   // The left & right rails
    bool     m_fFitCurves;  // Fit widened Bezier with curves if true
    GpReal   m_rTolerance;  // Approximation tolerance
    CShape   &m_oShape;     // Accumulates the final result
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CStrokeBoundsSink
//
//  Synopsis:
//      Helper class for computing the exact bounds of a widened stroke.
//
//------------------------------------------------------------------------------
class CStrokeBoundsSink   :   public CWideningSink
{
public:
    CStrokeBoundsSink()
    {
    }

    ~CStrokeBoundsSink() {}

    //
    // The following methods override CWideningSink methods.  They get 
    // called back when the path is widened with points on the boundary of 
    // the widened path.
    //

    virtual HRESULT StartWith(
        __in_ecount(2) const GpPointR *ptOffset
            // Left and right offset points
        )
    {
        m_oBounds.UpdateWithPoint(ptOffset[RAIL_RIGHT]);

        m_ptCurrent[RAIL_LEFT] = ptOffset[RAIL_RIGHT];  // Starting the cap
        m_ptCurrent[RAIL_RIGHT] = ptOffset[RAIL_RIGHT]; // Starting the right rail
        return S_OK;
    }

    virtual HRESULT QuadTo(
        __in_ecount(2) const GpPointR *ptOffset
            // In: Left & right offset points
        )
    {
        UpdateWith2Points(ptOffset);
        return S_OK;
    }

    virtual HRESULT QuadTo(
        __in_ecount(2) const GpPointR *ptOffset,
            // Left & right offset points thereof
        __in_ecount(1) const GpPointR &,
            // Ignored here
        __in_ecount(1) const GpPointR &,
            // Ignored here
        __in_ecount(1) const GpPointR &
            // Ignored here
        )
    {
        UpdateWith2Points(ptOffset);
        return S_OK;
    }

    virtual HRESULT CurveWedge(
        RAIL_SIDE side,
            // Which side to add to
        __in_ecount(1) const GpPointR &ptBez_1,
            // First control point
        __in_ecount(1) const GpPointR &ptBez_2,
            // Second control point
        __in_ecount(1) const GpPointR &ptBez_3
            // Last point
        )
    {
        m_oBounds.UpdateWithBezier(m_ptCurrent[side], 
                                   ptBez_1, 
                                   ptBez_2, 
                                   ptBez_3);
        
        m_ptCurrent[side] = ptBez_3;
        return S_OK;
    }

    virtual HRESULT BezierCap(
        __in_ecount(1) const GpPointR &ptStart,
            // The cap's start point
        __in_ecount(1) const GpPointR &pt0_1,
            // First arc's first control point
        __in_ecount(1) const GpPointR &pt0_2,
            // First arc's second control point
        __in_ecount(1) const GpPointR &ptMid,
            // The point separating the 2 arcs
        __in_ecount(1) const GpPointR &pt1_1,
            // Second arc's first control point
        __in_ecount(1) const GpPointR &pt1_2,
            // Second arc's second control point
        __in_ecount(1) const GpPointR &ptEnd
            // The cap's last point
        )
    {
        m_oBounds.UpdateWithBezier(m_ptCurrent[RAIL_LEFT], pt0_1, pt0_2, ptMid);
        m_oBounds.UpdateWithBezier(ptMid, pt1_1, pt1_2, ptEnd);
        m_ptCurrent[RAIL_LEFT] = ptEnd;
        return S_OK;
    }

    virtual HRESULT SetCurrentPoints(
        __in_ecount(2) const GpPointR *P
            // In: Array of 2 points
        )
    {
        UpdateWith2Points(P);
        return S_OK;
    }

    virtual HRESULT DoInnerCorner(
        RAIL_SIDE side,
            // The side of the inner corner
        __in_ecount(1) const GpPointR &,
            // Ignored
        const GpPointR *ptOffset
            // The offset points of the new segment
        )
    {
        m_ptCurrent[side] = ptOffset[side];
        return S_OK;
    }
    
    virtual HRESULT CapFlat(
        __in_ecount(2) const GpPointR *pPt,
            // The base points
        RAIL_SIDE side
            // Which side the cap endpoint is
        )
    {
        m_oBounds.UpdateWithPoint(pPt[side]);
        m_ptCurrent[RAIL_LEFT] = pPt[side];
        return S_OK;
    }

    virtual HRESULT CapTriangle(
        __in_ecount(1) const GpPointR &ptStart,
            // Trainagle base start point
        __in_ecount(1) const GpPointR &ptApex,
            // Triangle's apex
        __in_ecount(1) const GpPointR &ptEnd
            // Trainagle base end point
        )
    {
        m_oBounds.UpdateWithPoint(ptApex);
        m_oBounds.UpdateWithPoint(ptEnd);
        m_ptCurrent[RAIL_LEFT] = ptEnd;
        return S_OK;
    }

    virtual HRESULT PolylineWedge(
        RAIL_SIDE side,
            // Which side to add to - RAIL_RIGHT or RAIL_LEFT
        UINT count,
            // Number of points
        __in_ecount(count) const GpPointR *pPoints
            // The polyline vertices
        )
    {
        for (UINT i = 0;  i < count;  i++)
        {
            m_oBounds.UpdateWithPoint(pPoints[i]);
        }
        if (count > 0)
        {
            m_ptCurrent[side] = pPoints[count-1];
        }
        return S_OK;
    }
    
    virtual HRESULT AddFill(
        __in_ecount(1) const CShape &oShape,
            // The shape to be filled
        __in_ecount(1) const CMILMatrix &matrix
            // Transformation
        )
    {
        RRETURN(oShape.UpdateBounds(m_oBounds, true /* fill only */, &matrix));
    }

    virtual HRESULT SwitchSides()
    {
        GpPointR ptTemp(m_ptCurrent[RAIL_LEFT]);
        m_ptCurrent[RAIL_LEFT] = m_ptCurrent[RAIL_RIGHT];
        m_ptCurrent[RAIL_RIGHT] = ptTemp;
        return S_OK;
    }

    // Other methods
    HRESULT SetRect(
        __out_ecount(1) CMilRectF &rect
            // Out: The bounds as a RectF
        )
    {
        RRETURN(m_oBounds.SetRect(OUT rect));
    }

    BOOL NotUpdated() const
    {
        return m_oBounds.NotUpdated();
    }

    // Private methods
protected:
    void UpdateWith2Points(
        __in_ecount(1) const GpPointR *P
            // In: Array of 2 points
        )
    {
        m_oBounds.UpdateWithPoint(P[RAIL_LEFT]);
        m_oBounds.UpdateWithPoint(P[RAIL_RIGHT]);
        m_ptCurrent[RAIL_LEFT] = P[RAIL_LEFT];
        m_ptCurrent[RAIL_RIGHT] = P[RAIL_RIGHT];
    }

    // Data
protected:
     CBounds    m_oBounds;          // The bounds computed here
     GpPointR   m_ptCurrent[2];     // The current widening points

     //
     // The current widening points are kept and updated because the method
     // CBounds::UpdateWithBezier requires the first point of the Bezier
     // segment. 
     //
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHitTestSink
//
//  Synopsis:
//      This class tests the widening information, for containing a given point
//
//  Notes:
//      Hit testing with this class is faster then constructing and testing the
//      widened path for 2 reasons:
//          * Widening into this sink does not involve any memory allocation.
//          * The widening is aborted when a hit is detected.
//
//      The widened geometry for this purpose is divided to small closed
//      regions:
//          * A quadrangle for every line segment
//          * A triangle, quadrangle or half ellipse for every cap. 
//          * A wedge with a polygonal or curved top for every corner.
//
//      There are 2 ways the widening may be aborted:
//          * The embedded hit-tester aborts when it determines that the hit
//            point is near the boundary of one of these regions.
//          * This class determines that the point lies inside one of these
//            regions by looking at the winding number computed by the embedded
//            hit-tester.
//
//      CHitTestSink reports a hit by returning true from its Aborted() method. 
//      This is monitored only once per (line or curve) segment.
//
//      CHitTestSink hit tests the widened shape without actually constructing
//      it.
//
//------------------------------------------------------------------------------
class CHitTestSink :   public CWideningSink
{
public:
    CHitTestSink(
        __inout_ecount(1) CHitTest &tester
            // A hit tester
        )
        : m_refTester(tester)
    {
        m_fHitInside = m_fHitNear = false;
    }

    virtual ~CHitTestSink()
    {
    }

    // CWideningSink overrides
    virtual HRESULT StartWith(
        __in_ecount(2) const GpPointR *ptOffset
            // Left and right offset points
        );

    virtual HRESULT QuadTo(
        __in_ecount(2) const GpPointR *ptOffset
            // Left & right offset points
        );

    virtual HRESULT QuadTo(
        __in_ecount(2) const GpPointR *ptOffset,
            // Left & right offset points thereof
        __in_ecount(1) const GpPointR &,
            // Ignored here 
        __in_ecount(1) const GpPointR &,
            // Ignored here
        __in_ecount(1) const GpPointR &
            // Ignored here
        )
    {
        // CShapeWideningSink as 2 versions of QuadTo (see implementation for
        // details).  For this sink they are the same.
        return QuadTo(ptOffset);
    }

    virtual HRESULT CurveWedge(
        RAIL_SIDE side,
            // Which side to add to - RAIL_LEFT or RAIL_RIGHT
        __in_ecount(1) const GpPointR &ptBez_1,
            // First control point
        __in_ecount(1) const GpPointR &ptBez_2,
            // Second control point
        __in_ecount(1) const GpPointR &ptBez_3
            // Last point
        );

    virtual HRESULT BezierCap(
        __in_ecount(1) const GpPointR &ptStart,
            // The cap's first point
        __in_ecount(1) const GpPointR &pt0_1,
            // First arc's first control point
        __in_ecount(1) const GpPointR &pt0_2,
            // First arc's second control point
        __in_ecount(1) const GpPointR &ptMid,
            // The point separating the 2 arcs
        __in_ecount(1) const GpPointR &pt1_1,
            // Second arc's first control point
        __in_ecount(1) const GpPointR &pt1_2,
            // Second arc's second control point
        __in_ecount(1) const GpPointR &ptEnd
            // The cap's last point
        );

    virtual HRESULT SetCurrentPoints(
        __in_ecount(2) const GpPointR *P
            // Array of 2 points
        );

    virtual HRESULT DoInnerCorner(
        RAIL_SIDE side,
            // The side of the inner corner
        __in_ecount(1) const GpPointR &ptCenter,
            // The corner's center
        __in_ecount(2) const GpPointR *ptOffset
            // The offset points of new segment
        );

    virtual HRESULT CapTriangle(
        __in_ecount(1) const GpPointR &ptStart,
            // Triangle base start point
        __in_ecount(1) const GpPointR &ptApex,
            // Triangle's apex
        __in_ecount(1) const GpPointR &ptEnd
            // Triangle base end point
        );

    virtual HRESULT CapFlat(
        __in_ecount(2) const GpPointR *ppt,
            // The base points
        RAIL_SIDE side
            // Which side the cap endpoint is
        );

    virtual HRESULT SwitchSides();

    virtual HRESULT PolylineWedge(
        RAIL_SIDE side,
            // Which side to add to - RAIL_RIGHT or RAIL_LEFT
        UINT count,
            // Number of points
        __in_ecount(count) const GpPointR *pPoints
            // The polyline vertices
        );

    virtual HRESULT AddFill(
        __in_ecount(1) const CShape &oShape,
            // The shape to be filled
        __in_ecount(1) const CMILMatrix &matrix
            // Transformation
        );

    virtual const GpPointR GetCurrentPoint(
        RAIL_SIDE side
            // Left or right
        ) const
    {
        return m_ptCurrent[side];
    }

    virtual bool Aborted() const
    {
        return m_fHitNear || m_fHitInside;  
    }

    // Other methods
    bool WasHit() const
    {
        return m_fHitNear || m_fHitInside;
    }

    bool WasHitNear() const
    {
        return m_fHitNear;
    }

// Data
protected:
    GpPointR    m_ptCurrent[2];     // The latest widening points
    bool        m_fHitInside;       // True the hit point is inside the widened path
    bool        m_fHitNear;         // True if a hit near the boundary was detected
    CHitTest    &m_refTester;       // A hit tester
};



