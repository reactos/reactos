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
//      Geometry: Some 2D geometry helper routines.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

extern const float FLOAT_QNAN;
extern const UINT FLOAT_QNAN_UINT;

typedef DOUBLE GpReal;

// Some of these constants need further thinking

const GpReal FUZZ = 1.e-6;           // Relative 0
// Minimum allowed tolerance - should probably be adjusted to the size of the 
// geometry we are rendering, but for now ---

const GpReal FUZZ_DOUBLE = 1.e-12;           // Double-precision relative 0 

const GpReal MIN_TOLERANCE = 1.e-6;

const GpReal DEFAULT_FLATTENING_TOLERANCE = .25;

const GpReal TWICE_MIN_BEZIER_STEP_SIZE = 1.e-3; // The step size in the Bezier flattener should
                                                 // never go below half this amount.

// We should probably document bounds for geometry we'll render reliably, but
// for now ---
const GpReal MIN_GPREAL = 1.e-30;
const GpReal MAX_GPREAL = 1.e+30;

// Relative to this is relative to the tolerance squared. In other words, a vector
// whose length is less than .01*tolerance will be considered 0
const GpReal SQ_LENGTH_FUZZ = 1.E-4;

// Approximating a 1/4 circle with a Bezier curve          _
const GpReal ARC_AS_BEZIER = 0.5522847498307933984; // =(\/2 - 1)*4/3

const GpReal ONE_THIRD = 0.33333333333333333; // = 1/3

const GpReal TWO_THIRDS = 0.66666666666666666; // = 2/3

const GpReal FOUR_THIRDS = 1.33333333333333333; // = 4/3

const REAL DEFAULT_TENSION = 0.5;

const GpReal PI_OVER_180 = 0.0174532925199432957692;  // PI/180

const GpReal TWO_PI = 6.2831853071795865;    // 2PI

//                                                _
const GpReal SQRT_2 = 1.4142135623730950;    // \/2

inline BOOL IsStartType(BYTE type)
{
    return ((type & PathPointTypePathTypeMask) ==
               PathPointTypeStart);
}

inline BOOL IsClosedType(BYTE type)
{
    return ((type & PathPointTypeCloseSubpath) ==
               PathPointTypeCloseSubpath);
}

GpReal GetBezierDistance(// Return the distance as a fraction of the radius
    GpReal rDot,         // In: The dot product of the two radius vectors
    GpReal rRadius=1);   // In: The radius of the arc's circle

#define HYPOT(x,y) sqrt((x)*(x) + (y)*(y))

class GpPointR
{
    public:

    // Constructors
    GpPointR()
    {
    }

    GpPointR(
        __in_ecount(1) const MilPoint2F &point)
        :X(point.X), Y(point.Y)
    {
    } 
    
    GpPointR(
        __in_ecount(1) const MilPoint2F &point, 
        __in_ecount_opt(1) const CMILMatrix* pMatrix);
    
    GpPointR(
        __in_ecount(1) const     GpPointR &point,       // Raw point
        __in_ecount_opt(1) const CMILMatrix* pMatrix);  // transformation matrix

    GpPointR(__in_ecount(1) const GpPointR &point)
        :X(point.X), Y(point.Y)
    {
    } 

    GpPointR(IN GpReal x, IN GpReal y)
        :X(x), Y(y)
    {
    }

    // Construct as the difference vector
    GpPointR(IN const GpPointR &A, IN const GpPointR &B)
        :X(B.X - A.X), Y(B.Y - A.Y)
    {
    } 
    
    __outro_ecount(1) const GpPointR & operator=(__in_ecount(1) const GpPointR &P)
    {
        X = P.X;
        Y = P.Y;
        return *this;
    }
    
    // Conversion to and from MilPoint2F
    __outro_ecount(1) const GpPointR & operator=(__in_ecount(1) const MilPoint2F &P)
    {
        X = P.X;
        Y = P.Y;
        return *this;
    }

    void Set(__out_ecount(1) MilPoint2F &P) const
    {
        P.X = (REAL)X;
        P.Y = (REAL)Y;
    }

    // Overloaded arithmetic operations
    
    // Scale.
    GpPointR operator*(IN GpReal k) const
    {
        return GpPointR(X*k, Y*k);
    }
    
    VOID operator*=(IN const GpReal k)
    {
        X *= k;
        Y *= k;
    } 

    GpPointR operator/(IN GpReal k) const;
    
    VOID operator/=(IN const GpReal k);

    // Dot Product
    GpReal operator*(__in_ecount(1) const GpPointR &V) const
    {
        return (X*V.X+Y*V.Y);
    }
    
   
    GpPointR operator+(__in_ecount(1) const GpPointR &P) const
    {
        return GpPointR(X+P.X, Y+P.Y);
    } 
    
    VOID operator+=(__in_ecount(1) const GpPointR &V)
    {
        X += V.X;
        Y += V.Y;
    } 
    
    GpPointR operator-(__in_ecount(1) const GpPointR &P) const
    {
        return GpPointR(X-P.X, Y-P.Y);
    }

    VOID operator-=(__in_ecount(1) const GpPointR &V)
    {
        X -= V.X;
        Y -= V.Y;
    }

    // Unary -
    friend GpPointR operator-(__in_ecount(1) const GpPointR &P)
    {
        return GpPointR(-P.X, -P.Y);
    }
        
    // Equal
    bool operator == (__in_ecount(1) const GpPointR &P) const
    {
        return P.X == X  &&  P.Y == Y;
    }
        
    // Not equal
    bool operator != (__in_ecount(1) const GpPointR &P) const
    {
        return P.X != X  ||  P.Y != Y;
    }
        
    // Length
    GpReal Norm() const;
    GpReal ApproxNorm() const
        {
            return max(fabs(X), fabs(Y));
        }
    
    // Right turn in a left handed coordinate system (left otherwise)
    void TurnRight()    
    {
        GpReal r = -Y;
        Y = X;
        X = r;
    }

    // For debug purposes
#ifdef DBG
    bool DbgIsOfLength(
        GpReal r,                   // In: The expected length
        GpReal rTolerance) const;   // In: Tolerated relatove errpr

#endif

    void AssertEqualOrNaN(__in_ecount(1) const GpPointR &other) const
    {
        Assert((X == other.X) || (_isnan(X) && _isnan(other.X)));
        Assert((Y == other.Y) || (_isnan(Y) && _isnan(other.Y)));
    }

    // Data - intentially public
    GpReal X;
    GpReal Y;
};

inline GpReal Determinant(
    __in_ecount(1) const GpPointR &a,
    __in_ecount(1) const GpPointR &b)
{
    return (a.X * b.Y - a.Y * b.X);
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CMilPoint2F, MilPoint2F
//
//  Synopsis:
//      An "energized" version of MilPoint2F, which adds members and operators.
//
//      Designed to have the same memory layout as MilPoint2F, so that you can
//      cast between them.
//
//------------------------------------------------------------------------------

class CMilPoint2F : public MilPoint2F
{
public:

    // Constructors
    CMilPoint2F()
    {
        // We require that you can typecast between MilPoint2F and CMilPoint2F.
        // To achieve this, CMilPoint2F must have no data members or virtual functions.

        Assert( sizeof(MilPoint2F) == sizeof(CMilPoint2F) );
    }

    CMilPoint2F(IN FLOAT x, IN FLOAT y)
    {
        X = x;
        Y = y;
    }

    CMilPoint2F(__in_ecount(1) const MilPoint2F &pt)
    {
        X = pt.X;
        Y = pt.Y;
    }

    CMilPoint2F(__in_ecount(1) const GpPointR &pt)
    {
        X = FLOAT(pt.X);
        Y = FLOAT(pt.Y);
    }

    // Overloaded arithmetic operations
    
    // Scale.
    CMilPoint2F operator*(IN FLOAT k) const
    {
        return CMilPoint2F(X*k, Y*k);
    }
    
    VOID operator*=(IN const FLOAT k)
    {
        X *= k;
        Y *= k;
    } 

    // Dot Product
    FLOAT operator*(__in_ecount(1) const MilPoint2F &V) const
    {
        return (X*V.X+Y*V.Y);
    }
    
   
    CMilPoint2F operator+(__in_ecount(1) const MilPoint2F &P) const
    {
        return CMilPoint2F(X+P.X, Y+P.Y);
    } 
    
    VOID operator+=(__in_ecount(1) const MilPoint2F &V)
    {
        X += V.X;
        Y += V.Y;
    } 
    
    CMilPoint2F operator-(__in_ecount(1) const MilPoint2F &P) const
    {
        return CMilPoint2F(X-P.X, Y-P.Y);
    }

    VOID operator-=(__in_ecount(1) const MilPoint2F &V)
    {
        X -= V.X;
        Y -= V.Y;
    }

    // Unary -
    friend CMilPoint2F operator-(__in_ecount(1) const MilPoint2F &P)
    {
        return CMilPoint2F(-P.X, -P.Y);
    }
        
    // Length
    float Norm() const
    {
        return FLOAT(HYPOT(X, Y));
    }

    // Right turn in a left handed coordinate system (left otherwise)
    void TurnRight()    
    {
        float r = -Y;
        Y = X;
        X = r;
    }

    HRESULT Unitize();
};

inline FLOAT Determinant(const MilPoint2F &a, const MilPoint2F &b)
{
    return (a.X * b.Y - a.Y * b.X);
}

inline BOOL MilPoint2LsEqual(
    __in_ecount(1) const MilPoint2F& p1,
    __in_ecount(1) const MilPoint2F& p2)
{
    return (p1.X == p2.X) && (p1.Y == p2.Y);
}

// Returns TRUE if numbers are equal or both NaNs
inline BOOL EqualOrNaNs(float a, float b)
{
    return (a == b) || (_isnan(a) && _isnan(b));
}

// Returns TRUE if coordinates are equal, NaNs considered equal
inline BOOL MilPoint2LsEqualOrNaNs(
    __in_ecount(1) const MilPoint2F& p1, 
    __in_ecount(1) const MilPoint2F& p2)
{
    return EqualOrNaNs(p1.X, p2.X) && EqualOrNaNs(p1.Y, p2.Y);
}

VOID
TransformPoint(
    __in_ecount(1) const CMILMatrix & mat,
    __in_ecount(1) const MilPoint2D & ptF,
    __out_ecount(1) MilPoint2D & ptR
    );

VOID
TransformPoint(
    __in_ecount(1) const CBaseMatrix &mat,
        // In: Transformation matrix
    __inout_ecount(1) MilPoint2F &pt
        // Out/In: Input point
    );  

VOID
TransformPoint(
    __in_ecount(1) const CMILMatrix & mat,
    __in_ecount(1) const MilPoint2F & ptF,
    __out_ecount(1) GpPointR & ptR
    );

VOID
TransformPoint(
    __in_ecount(1) const CMILMatrix &mat,  // Transformation matrix
    __in_ecount(1) const GpPointR &pt,     // Input point
    __out_ecount(1) GpPointR      &ptOut   // The transformed point 
    );

VOID
TransformPoints(
    __in_ecount(1) const CMILMatrix &mat,
        // In: Transformation matrix
    int count,
        // In: Number of points
    __in_ecount(count) const MilPoint2F *pptF,
        // In: Input points
    __out_ecount_full(count) GpPointR *pptR
        // Out: The transformed points 
    );

VOID
TransformPoints(
    __in_ecount(1) const CMILMatrix &mat,
        // In: Transformation matrix
    int count,
        // In: Number of points
    __inout_ecount_full(count) MilPoint2F *pptF
        // In/Out: Input points
    );  

REAL GetSqScaleBound(
    __in_ecount(1) const CMILMatrix & mat
    );

// Definition and implementation of CBounds
// 
// Note: It's important here that we don't ignore NaNs,
// because otherwise callers can have the false impression that
// the shape has valid bounds. To this end, we keep a running tally
// of whether we've seen any NaNs and report that back at the end.
// We do this in lieu of reporting the error immediately as
// we do not want to disrupt the flow of the common, no-NaN case.
//
class CBounds
{
public:
    CBounds()
    :   m_xMin(DBL_MAX), m_xMax(-DBL_MAX), 
        m_yMin(DBL_MAX), m_yMax(-DBL_MAX),
        m_fEncounteredNaN(false)
    {
    }

    ~CBounds() {}

    // Obtain the results
    HRESULT SetRect(
        __out_ecount(1) CMilRectF &rect);   // Out: The bounds as a RectF_RB

    BOOL NotUpdated() const { return (m_xMax < m_xMin) && (m_yMax < m_yMin); }

    void UpdateWithPoint(
        __in_ecount(1) const GpPointR & pt);
        // In: A point to update with

    void UpdateWithBezier(
        __in_ecount(1) const GpPointR &pt0,
            // In: First point
        __in_ecount(1) const GpPointR &pt1,
            // In: First control point
        __in_ecount(1) const GpPointR &pt2,
            // In: Second control point
        __in_ecount(1) const GpPointR &pt3);
            // In: Last point

    void UpdateWithArc(
        IN FLOAT   xStart,     // X coordinate of the last point
        IN FLOAT   yStart,     // Y coordinate of the last point
        IN FLOAT   xRadius,    // The ellipse's X radius
        IN FLOAT   yRadius,    // The ellipse's Y radius
        IN FLOAT   rRotation,  // Rotation angle of the ellipse's x axis
        IN BOOL    fLargeArc,  // Choose the larger of the 2 possible arcs if TRUE
        IN BOOL    fSweepUp,   // Sweep the arc while increasing the angle if TRUE
        IN FLOAT   xEnd,       // X coordinate of the last point
        IN FLOAT   yEnd);      // Y coordinate of the last point

protected:
    // Private utilities for UpdateWithBezier
    int SolveSpecialQuadratic( 
        GpReal a,      // In: Coefficient of x^2
        GpReal b,      // In: Coefficient of 2*x
        GpReal c,      // In: Constant term
        __out_ecount_part(2, return) GpReal * r);   // Out: An array of size 2 to receive the zeros
    
    int GetDerivativeZeros( 
        GpReal a,           // In: Bezier coefficient of (1-t)^3
        GpReal b,           // In: Bezier coefficient of 3t(1-t)^2
        GpReal c,           // In: Bezier coefficient of 3(1-t)t^2
        GpReal d,           // In: Bezier coefficient of t^3
        __out_ecount_part(2, return) GpReal * r); // Out: An array of size 2 to receive the zeros
    
    GpReal GetBezierPolynomValue(
        GpReal a,   // In: Coefficient of (1-t)^3
        GpReal b,   // In: Coefficient of 3t(1-t)^2
        GpReal c,   // In: Coefficient of 3(1-t)t^2
        GpReal d,   // In: Coefficient of t^3
        GpReal t);// In: Parameter value t

    void UpdateNaN(GpReal x)
    {
        m_fEncounteredNaN = m_fEncounteredNaN || _isnan(x);
    }

    void UpdateNaN(GpPointR pt)
    {
        m_fEncounteredNaN = m_fEncounteredNaN || _isnan(pt.X) || _isnan(pt.Y);
    }

    // Data
    GpReal m_xMin;
    GpReal m_xMax;
    GpReal m_yMin;
    GpReal m_yMax;
    bool   m_fEncounteredNaN;
};
//+-----------------------------------------------------------------------------
//
//  Class:
//      CRealFunction
//
//  Synopsis:
//      Abstract class for representing real-valued functions
//
//  Notes:
//      The main service is solving an equation f(t) = 0.  To do that: 
//          * Derive your function
//          * Implement GetValueAndDerivative
//          * Call SolveNewtonRaphson for the solution.
//
//------------------------------------------------------------------------------
class CRealFunction
{
public:
    CRealFunction()
    {
    }

    virtual ~CRealFunction()
    {
    }

    virtual void GetValueAndDerivative(
        __in double t,
            // Where on the curve
        __out_ecount(1) double &f,
            // The derivative of the distance at t
        __out_ecount(1) double &df) const=0;
            // The derivative of f there

    virtual bool SolveNewtonRaphson(
        __in double from,
            // The start of the search interval
        __in double to,
            // The end of the search interval
        __in double seed,
            // Initial guess
        __in double   delta,
            // Convergence is assumed when consecutive guesses are less than this
        __in double   epsilon,
            // Convergence is assumed when the function value is less than this
        __out_ecount(1) double &root
            // The root
        ) const;

    // No data
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CIncreasingFunction
//
//  Synopsis:
//      Abstract class for representing a increasing real-valued functions
//
//  Notes:
//      The main service is solving an equation f(t) = 0.  Look at the
//      definition of CRealFunction for instructions.  But be sure that your
//      function is indeed increasing, i.e.  s > t ==> f(s) > f(t), otherwise
//      you'll trip assertions, fail to converge, and get incorrect results!
//
//------------------------------------------------------------------------------
class CIncreasingFunction   :   public CRealFunction
{
public:
    CIncreasingFunction()
    {
    }

    virtual ~CIncreasingFunction()
    {
    }

    virtual bool SolveNewtonRaphson(
        __in double from,
            // The start of the search interval
        __in double to,
            // The end of the search interval
        __in double seed,
            // Initial guess
        __in double   delta,
            // Convergence is assumed when consecutive guesses are less than this
        __in double   epsilon,
            // Convergence is assumed when the function value is less than this
        __out_ecount(1) double &root
            // The root
        ) const;

    // No data
};

void
ArcToBezier(
    IN FLOAT        xStart,     // X coordinate of the last point
    IN FLOAT        yStart,     // Y coordinate of the last point
    IN FLOAT        xRadius,    // The ellipse's X radius
    IN FLOAT        yRadius,    // The ellipse's Y radius
    IN FLOAT        rRotation,  // Rotation angle of the ellipse's x axis
    IN BOOL         fLargeArc,  // Choose the larger of the 2 possible arcs if TRUE
    IN BOOL         fSweepUp,   // Sweep the arc while increasing the angle if TRUE
    IN FLOAT        xEnd,       // X coordinate of the last point
    IN FLOAT        yEnd,       // Y coordinate of the last point
    __out_ecount(12) MilPoint2F   *pPt,       // An array of size 12 receiving the Bezier points
    __deref_out_range(-1,4) int   &cPieces);  // The number of output Bezier curves



