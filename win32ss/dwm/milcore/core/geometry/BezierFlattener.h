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
//      Definition of CBezierFlattener.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CFlatteningSink
//
//  Synopsis:
//      Callback interface for the results of curve flattening
//
//  Notes:
//      Methods are implemented rather than pure, for callers who do not use all
//      of them.
//
//------------------------------------------------------------------------------
//
//  Definition of CFlatteningSink
//
//------------------------------------------------------------------------------

class CFlatteningSink
{
public:
    CFlatteningSink() {}

    virtual ~CFlatteningSink() {}

    virtual HRESULT Begin(
        __in_ecount(1) const GpPointR &)
            // First point (transformed)
    {
        // Do nothing stub, should not be called
        RIP("Base class Begin called");
        return E_NOTIMPL;
    }

    virtual HRESULT AcceptPoint(
        __in_ecount(1) const GpPointR &pt,
            // The point
        IN GpReal t,
            // Parameter we're at
        __out_ecount(1) bool &fAborted)
            // Set to true to signal aborting
    {
        UNREFERENCED_PARAMETER(pt);
        UNREFERENCED_PARAMETER(t);
        UNREFERENCED_PARAMETER(fAborted);

        // Do nothing stub, should not be called
        RIP("Base class AcceptPoint called");
        return E_NOTIMPL;
    }

    virtual HRESULT AcceptPointAndTangent(
        __in_ecount(1) const GpPointR &,
            //The point
        __in_ecount(1) const GpPointR &,
            //The tangent there
        IN bool fLast)         // Is this the last point on the curve?
    {
        // Do nothing stub, should not be called
        RIP("Base class AcceptPointAndTangent called");
        return E_NOTIMPL;
    }
};
//+-----------------------------------------------------------------------------
//
//  Class:
//      CBezierFlattener
//
//  Synopsis:
//      Generates a polygonal apprximation to a given Bezier curve
//
//------------------------------------------------------------------------------
class CBezierFlattener  :   public CBezier
{
public:
    CBezierFlattener(
        __in_ecount_opt(1) CFlatteningSink *pSink,
            // The reciptient of the flattened data
        IN GpReal          rTolerance)
            // Flattening tolerance 
    {
        Initialize(pSink, rTolerance);
    }

    void SetTarget(__in_ecount_opt(1) CFlatteningSink *pSink)
    {
        m_pSink = pSink;
    }

    void Initialize(
        __in_ecount_opt(1) CFlatteningSink *pSink,
            // The reciptient of the flattened data
        IN GpReal rTolerance);
        // Flattening tolerance 

    void SetPoint(
        __in UINT i,
            // index of the point (must be between 0 and 3)
        __in_ecount(1) const GpPointR &pt)
            // point value
    {
        Assert(i < 4);
        m_ptB[i] = pt;
    }

    HRESULT GetFirstTangent(
        __out_ecount(1) GpPointR &vecTangent) const;
            // Tangent vector there

    GpPointR GetLastTangent() const;

    HRESULT Flatten( 
        IN bool fWithTangents);   // Return tangents with the points if true

private:
    // Disallow copy constructor
    CBezierFlattener(__in_ecount(1) const CBezierFlattener &)
    {
        RIP("CBezierFlattener copy constructor reached.");
    }

protected:
    HRESULT Step(
        __out_ecount(1) bool &fAbort);   // Set to true if flattening should be aborted

    void HalveTheStep();

    bool TryDoubleTheStep();

    // Flattening defining data
    CFlatteningSink *m_pSink;           // The recipient of the flattening data
    double          m_rTolerance;       // Prescribed tolerance
    bool            m_fWithTangents;    // Generate tangent vectors if true
    double          m_rQuarterTolerance;// Prescribed tolerance/4 (for doubling the step)
    double          m_rFuzz;            // Computational zero

    // Flattening working data
    GpPointR        m_ptE[4];           // The moving basis of the curve definition
    int             m_cSteps;           // The number of steps left to the end of the curve
    double          m_rParameter;       // Parameter value
    double          m_rStepSize;        // Steps size in parameter domain
};


