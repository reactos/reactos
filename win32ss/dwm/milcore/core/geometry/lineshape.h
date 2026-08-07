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
//      Classes used for modeling and positioning line shapes
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CLineShape
//
//  Synopsis:
//      Captures the settings and shape of a line shape
//
//  Notes:
//      A line shape is a shape that adorns either one of the two tips of every
//      open figure in a path.
//
//------------------------------------------------------------------------------

// Since thefeture is not exposed and has no test coverage for them, line shapes are currently
// mothballed in milcore.dll. But they are exposed in milcoretest.dll.  To achieve that, their
// definition and implementation is switched on #ifdef LINE_SHAPES_ENABLED.  the source code
// in the geometry directory is compiled into two separate libraries: coregeometry.lib - 
// - linked into milcore.dll - and testgeoemtry.lib - linked into milcoretest.dll. The 
// LINE_SHAPES_ENABLED #define is only seen by geometry source inside geometry\test.  It will
// NOT be seen by files from other directories even if their .lib files are linked into 
// milcoretest.dll.  To avoid a mismatch between the actual definition of geometry classes and
// the way they are seen from source outside the geometry directory (e.g. api), switcing on
// LINE_SHAPES_ENABLED must not be done in include files.  Here is the only exception, where
// the entire definition of classes is switched off.  The compiler will complain if these
// classes are used in any code outside the geometry directory.

#ifdef LINE_SHAPES_ENABLED

MtExtern(CLineShape);

class CLineShape
{
public:
    CLineShape(
        IN REAL      rInset,
            // Figure inset distance
        IN REAL      rAnchor,
            // In: -y coordinate of the anchor point
        IN bool      fFill,
            // Fill the shape if true 
        IN bool      fStroke,
            // Stroke the shape if true
        __in_ecount_opt(1) CPlainPen *pPen)
            // An overriding pen for strokes
    : m_rInset(rInset), m_rAnchor(rAnchor), m_fStroke(fStroke),  m_fFill(fFill)
    {
        if (pPen)
        {
            m_fOverrideThePen = true;
            m_oPenGeom = pPen->GetGeometry();
        }
        else
        {
            m_fOverrideThePen = false;
        }

    }

    virtual ~CLineShape()
    {
    }

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CLineShape));

    HRESULT SetPath(
        __in_ecount(1) const CShape &oShape)
            // The line shape's geometry
    {
        return m_oPath.Copy(oShape);
    }

    __outro_ecount(1) const CShape &GetPath() const
        // Returned: the line shape's geometry
    {
        return m_oPath;
    }

    const bool IsStroked() const
    {
        return m_fStroke;
    }

    const bool IsFilled() const
    {
        return m_fFill;
    }

    REAL GetAnchor() const
    {
        return m_rAnchor;
    }

    REAL GetInset() const
    {
        return m_rInset;
    }

    bool OverridesThePen() const
    {
        return m_fOverrideThePen;
    }

    __outro_ecount(1) const CPenGeometry &GetPenGeometry() const
    {
        return m_oPenGeom;
    }

    HRESULT Clone(
        __deref_out_ecount(1) CLineShape *&pClone) const;
            // The cloned shape

    // Used for construction of the canned shapes
    HRESULT AddPolygon(
        __in_ecount(count) const MilPoint2F *pPoints,
        int count)
    {
        return m_oPath.AddPolygon(pPoints, count);
    }

    HRESULT AddEllipse(
        REAL centerX, REAL centerY, // The center of the ellipse
        REAL radiusX,               // The X-radius of the ellipse
        REAL radiusY,               // The Y-radius of the ellipse
        CR crParameters
        )
    {
        return m_oPath.AddEllipse(centerX, centerY, radiusX, radiusY, crParameters);
    }

    HRESULT AddEllipse(     // The following parameters refer to the rectangular bounds of the ellipse:
        REAL x, REAL y,     // The upper left hand corner.
        REAL width,         // Rectangle width
        REAL height,        // Rectangle height
        OWH owhParameters
        )
    {
        return m_oPath.AddEllipse(x, y, width, height, owhParameters);
    }

    HRESULT GetExtents(
        __in REAL rOwnersThickness,
            // The extents of the owner pen
        __in REAL rOwnerExtents,
            // The owner's extents
        __out_ecount(1) REAL &rExtents) const;
            // The extents

private:
    // Disallow assignment
    CLineShape(__in_ecount(1) const CLineShape* other)
    {
        other;
    }


protected:
    CShape       m_oPath;           // The line shape's geometry
    bool         m_fOverrideThePen; // = true if we use our own pen for stroking
    CPenGeometry m_oPenGeom;        // Geometry of the pen used for the above
    bool         m_fStroke;         // Stroke the shape if true
    bool         m_fFill;           // Fill the shape if true

    /* The line shape will be placed so that its anchor point (0, -m_rAnchor)
     is on the figure.  The figure is trimmed to a point whose distance from
     the tip is m_rInset */

    REAL         m_rInset;   // The figure inset distance when applied
    REAL         m_rAnchor;  // Distance to anchor point
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CMarker
//
//  Synopsis:
//      Helper class for positioning a line shape
//
//  Notes:
//      Computes the transformation for scaling and positioning the line shape,
//      and computes where the figure is to be trimmed to accommodate that.
//
//------------------------------------------------------------------------------

class CMarker   : public CFigureTask
{
public:
    CMarker(
        __in_ecount(1) const CPen &pen,
            // The owning figure's widening pen
        __in_ecount(1) const CLineShape &shape,
            // The line shape we're processing
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation matrix (NULL OK)
        __in_ecount(1) CWideningSink    &sink,
            // The widening sink
        IN double           rTolerance);
            // Error tolerance

    GpReal FindCircleIntersection(       // Return chord end's parameter on the segment
        __in_ecount(1) const GpPointR &P,
            // In: A point outside the circle
        __in_ecount(1) const GpPointR &V,
            // In: Segment vector
        GpReal rNum,
            // In: The numerator of chord length^2
        GpReal rDenom) const;
            // In: The denominator of chord length^2

    HRESULT Process(
        __in_ecount(1) const IFigureData &oFigure,
            // The figure
        __inout_ecount(1) bool &fTrimmedAway,
            // Set to true if the figure was trimmed away
        __out_ecount(1) GpReal &rAt);
            // Parameter where a segment is to be trimmed

    // Figure task overrides
    virtual HRESULT DoLine(
        __in_ecount(1) const MilPoint2F &ptEnd);
            // The line's end point

    virtual HRESULT DoBezier(
        __in_ecount(3) const MilPoint2F *ptBez);
            // The curve's 3 last Bezier points

    virtual HRESULT AcceptPoint(
        __in_ecount(1) const GpPointR &point,
            // The new point
        IN GpReal rAt,
            // Parameter value there
        __out_ecount(1) bool &fDone);
            // Set to true if the task is done

    virtual HRESULT SetAnchorAndInset(
        IN GpReal   rSqFuzz,
            // Squared fuzz
        __inout_ecount(1) bool &fTrimmedAway)=0;
            // Set to true if the figure was trimmed away

    HRESULT SetForStroke(
        __in_ecount(1) const CWidener &other,
        __in_ecount(1) CWideningSink *pSink)
    {
        return m_oWidener.SetForLineShape(other, m_oLineShape, pSink, OUT m_fEmptyPen);       
    }

    virtual bool ThisIsAnEndMarker()const = 0;

protected:
    // Initial data
    const IFigureData   *m_pFigure;      // The figure to which we apply line shapes
    GpPointR            m_ptTip;         // Figure's tip
    const CPen          &m_refOwnersPen; // Owner's widening pen for for width computation
    const CMILMatrix    *m_pMatrix;      // Transformation matrix
    const CLineShape    &m_oLineShape;   // The line shape we're processing
    int                 m_iAnchor;       // Which one of the 2 is the anchor distance
    int                 m_iInset;        // Which one of the 2 is the inset distance
    CWideningSink       &m_refSink;      // The widening sink
    double              m_rSq0Length;    // Squared computational 0 distance

    // Widening
    CWidener    m_oWidener;        // A widener for stroking the line shape
    bool        m_fEmptyPen;       // The widening pen is too thin if true

    // Working variables for traversal
    GpPointR    m_ptPrevious;     // Previous point
    GpReal      m_rPrev;          // Previous parameter value

    // Bookkeeping - which chord is what
    int         m_iFrom;          // The first chord we'll locate
    int         m_iCurrent;       // The current chord we're working on
    int         m_iTo;            // The last chord to locate
    GpReal      m_rSqDist[2];     // The squared inset and anchor distances

    // Results
    GpPointR    m_vecAnchor;        // The location of the anchor point
    GpReal      m_rSqAnchorScale;   // The squared scale factor for the above
    GpReal      m_rTrim;            // Parameter value to trim at
    bool        m_fIsTrimDone;      // Trim point successfully located if true

    // Disallow assignment
private:
    void operator = (CMarker &) {}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CStartMarker
//
//  Synopsis:
//      Helper class for positioning a line shape at figure start
//
//------------------------------------------------------------------------------

MtExtern(CStartMarker);

class CStartMarker   :   public CMarker
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CStartMarker));

    CStartMarker(
        __in_ecount(1) const CPen &pen,
            // The owning figure's widening pen
        __in_ecount(1) const CLineShape &shape,
            // The line shape we're processing
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation matrix (NULL OK)
        __in_ecount(1) CWideningSink &sink,
            // The widening sink
        IN double rTolerance)
            // Error tolerance
    : CMarker(pen, shape, pMatrix, sink, rTolerance)
    {
    }

protected:
    virtual HRESULT SetAnchorAndInset(
        IN GpReal   rSqFuzz,
            // Squared fuzz
        __inout_ecount(1) bool &fTrimmedAway);
            // Set to true if the figure was trimmed away

    HRESULT Traverse(
        __inout_ecount(1) OUT bool &fTrimmedAway);
            // Set to true if the figure is trimmed away

    virtual bool ThisIsAnEndMarker() const
    {
        return false;
    }
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CEndMarker
//
//  Synopsis:
//      Helper class for positioning a line shape at figure end
//
//------------------------------------------------------------------------------

MtExtern(CEndMarker);

class CEndMarker   :   public CMarker
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CEndMarker));

    CEndMarker(
        __in_ecount(1) const CPen &pen,
            // The owning figure's widening pen
        __in_ecount(1) const CLineShape &shape,
            // The line shape we're processing
        __in_ecount_opt(1) const CMILMatrix *pMatrix,
            // Transformation matrix (NULL OK)
        __in_ecount(1) CWideningSink &sink,
            // The widening sink
        IN double rTolerance)
            // Error tolerance
    : CMarker(pen, shape, pMatrix, sink, rTolerance)
    {
    }

protected:
    virtual HRESULT SetAnchorAndInset(
        IN GpReal   rSqFuzz,
            // Squared fuzz
        __inout_ecount(1) bool &fTrimmedAway);
            // Set to true if the figure was trimmed away

    virtual bool ThisIsAnEndMarker() const
    {
        return true;
    }
};
#endif // LINE_SHAPES_ENABLED



