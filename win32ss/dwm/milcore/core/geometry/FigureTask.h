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
//      Helper classes for traversing a figure for various computations
//
//  $ENDTAG
//
//  Classes:
//      CFlatteningSink, CFigureTask and its subclasses;  CBezier
//
//------------------------------------------------------------------------------



//+-----------------------------------------------------------------------------
//
//  Class:
//      CMILBezierFlattener
//
//  Synopsis:
//      Generates a polygonal apprximation to a given Bezier curve
//
//  Note:
//      This class adds some MIL specific methods to construct CBezierFlattener
//
//------------------------------------------------------------------------------
class CMILBezierFlattener  :   public CBezierFlattener
{
public:
    CMILBezierFlattener(
        __in_ecount_opt(1) CFlatteningSink *pSink,
            // The reciptient of the flattened data
        GpReal rTolerance)
            // Flattening tolerance
        :
    CBezierFlattener(pSink, rTolerance)
    {
    }

    CMILBezierFlattener(
        __in_ecount(1) const GpPointR  &ptFirst,
            // First point (transformed)
        __in_ecount(3) const MilPoint2F *pt,
            // The last 3 points (raw)
        __in_ecount_opt(1) CFlatteningSink *pSink,
            // The reciptient of the flattened data
        GpReal rTolerance,
            // Flattening tolerance 
        __in_ecount_opt(1) const CMILMatrix  *pMatrix)
            // Transformation matrix (NULL OK)
        : CBezierFlattener(pSink, rTolerance)
    {
        SetPoints(0, 1, ptFirst, pt, pMatrix);
    }


    CMILBezierFlattener(
        __in_ecount(1) const GpPointR       &ptFirst,
            // First point (transformed)
        __in_ecount(1) const GpPointR       &ptControl1,
            // First control point
        __in_ecount(1) const GpPointR       &ptControl2,
            // Second control point
        __in_ecount(1) const GpPointR       &ptEnd,
            // Last point
        __in_ecount_opt(1) CFlatteningSink   *pSink,
            // Flattening sink
        __in_ecount(1) const CMILMatrix     &matrix);
            // Transformation matrix
    
    void SetPoints(
        double rStart,
            // Start parameter
        double rEnd,
            // End parameter
        __in_ecount(1) const GpPointR  &ptFirst,
            // First point (transformed)
        __in_ecount(3) const MilPoint2F *pt,
            // The last 3 points (raw)
        __in_ecount_opt(1) const CMILMatrix  *pMatrix);
            // Transformation matrix (NULL OK)
    
    // No data
};

///////////////////////////////////////////////////////////////////////////////
//      Definition of CFigureTask
/* This class implements traversing a figure and defines a callback interface 
 for performing a task on every line or curve segment. The callback will be 
 from TraverseForward/TraverseBackward.
 
 Here is how you'd possibly  use it for your task:

    * Derive and implement your subclass
    * Override the methods with what you want done on every line and curve
    * If you choose to instantiate a CBezier and call Flatten from your
      DoBezier then you need to override the AcceptPoint method.
    * Have the caller do what needs to be done at the figure's start (or end
      if going backward).
    * Call TraverseForward or TraverseBackward

    Look at CBoundsTask as an example */

class CFigureTask   :   public CFlatteningSink
{
public:

    CFigureTask()
    {
        m_fAborted = false;
    }

    virtual ~CFigureTask()
    {
    }

    // Interface for traversal forward/backword callback

    virtual HRESULT DoLine(
        __in_ecount(1) const MilPoint2F &ptEnd
            // The line's end point
        )=0;

    virtual HRESULT DoBezier(
        __in_ecount(3) const MilPoint2F *pt
            // The missing 3 Bezier points
        )=0;

    // Traversal implementation.
    HRESULT TraverseForward(
        __in_ecount(1) const IFigureData &figure
            // The traversed figure
        );

#ifdef LINE_SHAPES_ENABLED
    HRESULT TraverseBackward(
        __in_ecount(1) const IFigureData &figure
            // The traversed figure
        );
#endif

    bool WasAborted() const
    {
        return m_fAborted;
    }

private:
    // Disallow assignment
    __out_ecount(1) const CFigureTask & operator=(
        __in_ecount(1) const CFigureTask &other
        )
    {
        RIP("Assignment of a CFigureTask not allowed");
        return *this;
    }

protected:
    // Data
    bool m_fAborted;  // Set to true if traversal was aborted
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBoundsTask
//
//  Synopsis:
//      Helper class for hit computing bounds
//
//------------------------------------------------------------------------------
class CBoundsTask   :   public CFigureTask
{
public:
    CBoundsTask(
        __in_ecount(1) CBounds &bounds,
            // Bounds, updated here
        __in_ecount(1) const MilPoint2F &pt,
            // The first point
        __in_ecount_opt(1) const CMILMatrix *pMatrix)
            // Transformation (optional, NULL OK)
    :   m_oBounds(bounds), m_pMatrix(pMatrix)
    {
        DoLineNoHRESULT(pt);
    }

    virtual ~CBoundsTask() {}
    
    void DoLineNoHRESULT(
        __in_ecount(1) const MilPoint2F &ptEnd
        // The line's end point
        );

    virtual HRESULT DoLine(
        __in_ecount(1) const MilPoint2F &ptEnd
        // The line's end point
        )
    {
        DoLineNoHRESULT(ptEnd);
        return S_OK;
    }

    virtual HRESULT DoBezier(
        __in_ecount(3) const MilPoint2F *pt
            // The missing 3 Bezier points
        );

// Data
protected:
    CBounds             &m_oBounds;     // The bounds we are updating
    const CMILMatrix    *m_pMatrix;     // Transformation matrix
    GpPointR            m_ptCurrent;    // The current point
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHitTest
//
//  Synopsis:
//      Helper class for hit testing
//
//  Notes:
//      This class checks if a given point is in the filled area of a path or
//      near its boundary.  It does the former by counting intersections of the
//      path with a horizontal ray emanating from the hit point. It bails out
//      without completing the count if a near-boundary hit is detected.
//
//      The computation is done under a transformation that takes the hit point
//      to the origin.  The horizontal ray is then the positive half of the x
//      axis.
//
//------------------------------------------------------------------------------
class CHitTest   :   public CFigureTask
{
public:

    // Constructor / destructor
    CHitTest(
        __in_ecount(1) const GpPointR &ptHit,
            // The hit test point (in transformed space)
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation to apply to the figure (NULL OK)
        double rThreshold
            // Distance to be considered a hit
        );
        
    virtual ~CHitTest() {}
    
    // CFigureTask overrides
    virtual HRESULT DoLine(
        __in_ecount(1) const MilPoint2F &ptEnd
            // The line's end point
        )
    {
        GpPointR pt(ptEnd, &m_oMatrix);
        
        return AcceptPoint(pt, 1, m_fAborted);
    }

    virtual HRESULT DoBezier(
        __in_ecount(3) const MilPoint2F *pt
            // The missing 3 Bezier points
        )
    {
        CMILBezierFlattener curve(m_ptCurrent, pt, this, DEFAULT_FLATTENING_TOLERANCE, &m_oMatrix);
        RRETURN(curve.Flatten(false));
    }

    void AcceptPointNoHRESULT(
        __in_ecount(1) const GpPointR &ptEnd,
            // The segment's endpoint
        GpReal,
            // Ignored here
        __out_ecount(1) bool &fHit
            // Set to true if there is a hit
        );

    virtual HRESULT AcceptPoint(
        __in_ecount(1) const GpPointR &ptEnd,
            // The segment's endpoint
        GpReal t,
            // Ignored here
        __out_ecount(1) bool &fHit
            // Set to true if there is a hit
        )
    {
        AcceptPointNoHRESULT(
            ptEnd,
            t,
            fHit);

        return S_OK;
    }

    // Other public methods
    bool StartAt(
        __in_ecount(1) const MilPoint2F &ptFirst
            // The figure's first point
        )
    {
        m_ptCurrent = GpPointR(ptFirst, &m_oMatrix);
        return (m_fAborted = (m_ptCurrent * m_ptCurrent < m_rSquaredThreshold));
    }

    // Exactly the same as StartAt, but with a double-precision point
    // To be consolidated when we move to double precision points throughout
    HRESULT StartAtR(
        __in_ecount(1) const GpPointR &ptFirst
            // The figure's first point
        )
    {
        m_iWinding = 0;
        m_ptCurrent = GpPointR(ptFirst, &m_oMatrix);
        m_fAborted = (m_ptCurrent * m_ptCurrent < m_rSquaredThreshold);

        return S_OK;
    }

    // Exactly the same as DoLine, but with a double-precision point
    // To be consolidated when we move to double precision points throughout
    virtual HRESULT DoLineR(
        __in_ecount(1) const GpPointR &ptEnd
            // The line's end point
        )
    {
        GpPointR pt(ptEnd, &m_oMatrix);       
        return AcceptPoint(pt, 1, m_fAborted);
    }

    // Same as DoBezier, but with individual double-precision points
    // May be consolidated when we move to double precision points throughout
    HRESULT DoBezierR(
        __in_ecount(1) const GpPointR &ptControl1,
            // First control point
        __in_ecount(1) const GpPointR &ptControl2,
            // Second control point
        __in_ecount(1) const GpPointR &ptEnd
            // Last point
        )
    {
        CMILBezierFlattener curve(m_ptCurrent, 
                      ptControl1,
                      ptControl1,
                      ptEnd,
                      this,
                      m_oMatrix);
        return curve.Flatten(false);
    }

    bool EndAt(
        __in_ecount(1) const MilPoint2F &ptFirst
            // The figure's starting point
        );

    int GetWindingNumber() const
    {
        Assert(!m_fAborted);  // Otherwise the number may be bogus due to early out
        return m_iWinding;
    }

    __outro_ecount(1) const CMILMatrix &GetTransform() const
    {
        return m_oMatrix;
    }

    void SetTransform(
        __in_ecount(1) const CMILMatrix &matrix
        )
    {
        m_oMatrix = matrix;

        // Set the transformation to shift the hit point to the origin
        m_oMatrix.Translate(static_cast<REAL>(-m_ptHit.X), static_cast<REAL>(-m_ptHit.Y));
    }
                  
    // Private methods
    protected:

    MIL_FORCEINLINE void CheckIfNearTheOrigin(
        __in_ecount(1) const GpPointR &ptEnd
            // The segment's endpoint
        );
    
    MIL_FORCEINLINE void UpdateWith(
        __in_ecount(1) const GpPointR &ptEnd
            // The segment's endpoint
        );
    
// Data
protected:
    GpPointR       m_ptHit;              // The hitting point
    CMILMatrix     m_oMatrix;            // Transformation matrix
    double         m_rSquaredThreshold;  // Squared hit distance
    GpPointR       m_ptCurrent;          // The current point
    int            m_iWinding;           // The winding number
};


#ifdef DBG
//+-----------------------------------------------------------------------------
//
//  Class:
//      CFigureDumper
//
//  Synopsis:
//      Debug dumper
//
//------------------------------------------------------------------------------
class CFigureDumper :   public CFigureTask
{
public:
    CFigureDumper()
    {
    }

    // CFigureTask overrides
    virtual HRESULT DoLine(
        __in_ecount(1) const MilPoint2F &ptEnd
            // The line's end point
        );

    virtual HRESULT DoBezier(
        __in_ecount(3) const MilPoint2F *pt
            // The curve's 3 last Bezier points
        );

    // No data
};
#endif // def DBG

