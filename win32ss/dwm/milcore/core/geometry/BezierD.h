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
//      Double precision Bezier curve with basic services
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBezier
//
//  Synopsis:
//      Data and basic services for a Bezier curve
//
//------------------------------------------------------------------------------
class CBezier
{
public:
    CBezier()
    {
    }

    CBezier(
        __in_ecount(4) const GpPointR *pPt)
            // The defining Bezier points
    {
        Assert(pPt);
        memcpy(&m_ptB, pPt, 4 * sizeof(GpPointR)); 
    }

    CBezier(
        __in_ecount(1) const CBezier &other)
            // Another Bezier to copy
    {
        Copy(other); 
    }

    void Copy(
        __in_ecount(1) const CBezier &other)
            // Another Bezier to copy
    {
        memcpy(&m_ptB, other.m_ptB, 4 * sizeof(GpPointR)); 
    }

    void Initialize(
        __in_ecount(1) const GpPointR &ptFirst,
            // The first Bezier point
        __in_ecount(3) const GpPointR *pPt)
            // The remaining 3 Bezier points
    {
        m_ptB[0] = ptFirst;
        memcpy(m_ptB + 1, pPt, 3 * sizeof(GpPointR)); 
    }

    __outro_ecount(1) const GpPointR &GetControlPoint(__range(0, 3) UINT i) const
    {
        Assert(i < 4);
        return m_ptB[i];
    }

    __outro_ecount(1) const GpPointR &GetFirstPoint() const
    {
        return m_ptB[0];
    }
    
    __outro_ecount(1) const GpPointR &GetLastPoint() const
    {
        return m_ptB[3];
    }

    void GetPoint(
        _In_ double t,
            // Parameter value
        __out_ecount(1) GpPointR &pt) const; 
            // Point there

    void GetPointAndDerivatives(
        __in double t,
            // Parameter value
        __out_ecount(3) GpPointR *pValues) const;
                // Point, first derivative and second derivative there

    void TrimToStartAt(
        IN double t);             // Parameter value
        
    void TrimToEndAt(
        IN double t);             // Parameter value

    bool TrimBetween(
        __in double rStart,
            // Parameter value for the new start, must be between 0 and 1
        __in double rEnd);
            // Parameter value for the new end, must be between 0 and 1

    bool operator ==(__in_ecount(1) const CBezier &other) const
    {
        return (m_ptB[0] == other.m_ptB[0]) &&
               (m_ptB[1] == other.m_ptB[1]) &&
               (m_ptB[2] == other.m_ptB[2]) &&
               (m_ptB[3] == other.m_ptB[3]);
    }

    void AssertEqualOrNaN(__in_ecount(1) const CBezier &other) const
    {
        m_ptB[0].AssertEqualOrNaN(other.m_ptB[0]);
        m_ptB[1].AssertEqualOrNaN(other.m_ptB[1]);
        m_ptB[2].AssertEqualOrNaN(other.m_ptB[2]);
        m_ptB[3].AssertEqualOrNaN(other.m_ptB[3]);
    }

protected:
    // Data
    GpPointR        m_ptB[4];
        // The defining Bezier points
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CBezierFragment
//
//  Synopsis:
//      Describes the piece of a Bezier delimited by a start and end parameter.
//
//  Note:
//      No reference counting is performed, so care must be taken to ensure the
//      parent Bezier exists during the lifetime of the fragment.
//
//------------------------------------------------------------------------------
class CBezierFragment
{
public:
    CBezierFragment()
        : m_pBezierNoRef(NULL), m_start(0.0), m_end(0.0)
    {}

    CBezierFragment(
        __in_ecount(1) const CBezier &bezier,
        double startParameter,
        double endParameter
        )
        : m_pBezierNoRef(&bezier),
          m_start(startParameter), m_end(endParameter)
    {}

    CBezierFragment(
        __in_ecount(1) const CBezierFragment &fragment
        )
        : m_pBezierNoRef(fragment.m_pBezierNoRef), m_start(fragment.m_start), 
            m_end(fragment.m_end)
    {}

    void Clear()
    {
        m_pBezierNoRef = NULL;
    }

    bool Assigned() const
    {
        return m_pBezierNoRef != NULL;
    }

    bool TryExtend(
        __in_ecount(1) const CBezierFragment &other,
            // The proposed extension
        bool fAppend
            // Should the extension be appended or prepended?
        );

    bool ConstructBezier(
        __out_ecount(1) CBezier *pBezier
        ) const;

private:
    const CBezier *m_pBezierNoRef;
        // Pointer to parent Bezier (weak reference)
    double m_start;
        // Start Bezier parameter
    double m_end;
        // End Bezier parameter
};


