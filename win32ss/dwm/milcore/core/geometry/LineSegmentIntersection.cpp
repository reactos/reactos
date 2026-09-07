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
//      CLineSegmentIntersection implementation.
//
//  $ENDTAG
//
//  Classes:
//      CLineSegmentIntersection
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

using namespace RobustIntersections;

// Utilities.

//+-----------------------------------------------------------------------------
//
//  Function:
//      DoubleIsInteger
//
//  Synopsis:
//      Finds if the argument is an integer
//
//  Returns:
//      true if the argument is an integer and false otherwise
//
//------------------------------------------------------------------------------
inline 
bool 
DoubleIsInteger(double v)
{
    double integerPart;
    return modf(v, &integerPart) == 0.0;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      DeterminantIsExactDouble
//
//  Synopsis:
//      Finds if the determinant a*b - c*d can be surely computed exactly using
//      floating point arithmetic.
//
//  Returns:
//      true if the determinant can be computed exactly and false otherwise
//
//------------------------------------------------------------------------------
inline 
bool 
DeterminantIsExactDouble(double a, double b, double c, double d)
{
    // Return true iff a*d - b*c can be computed exactly as a double.
    // We know that the arguments are integral.
    Assert(DoubleIsInteger(a) && DoubleIsInteger(b) &&
           DoubleIsInteger(c) && DoubleIsInteger(d));

    return abs(a) <= LARGESTINTEGER26 && abs(b) <= LARGESTINTEGER26 && 
           abs(c) <= LARGESTINTEGER26 && abs(d) <= LARGESTINTEGER26;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ComputeDeterminantExactSign
//
//  Synopsis:
//      Computes the sign of the determinant a*d - b*c, sets *r to an
//      approximate value of the result.
//
//  Returns:
//      the determinant sign.
//
//------------------------------------------------------------------------------
inline 
SIGNINDICATOR 
ComputeDeterminantExactSign(double a, double b, double c, double d, __out_ecount(1) double* r)
{
    // Return the exact sign of a*d - b*c.
    // Set *r to an approximate value of the result.
    // We know that a, b, c, and d are exactly represented and are in the [-2^31, +2^31] range.

    Assert(r);
    Assert(IsValidInteger31(a) && IsValidInteger31(b) && 
           IsValidInteger31(c) && IsValidInteger31(d));

#ifdef LSI_AUDITING
    CLineSegmentIntersection::IncrementFpa();
#endif

    double ad = a * d;
    double bc = b * c;
    SIGNINDICATOR eResult = SI_ZERO;

    *r = ad - bc;
    if (ad != bc || abs(ad) <= LARGESTINTEGER53)
    {
        // If ad != bc or if the product is exact we can safely conclude.
        Assert((ad != bc) || (ad == bc && 
            abs(ad) <= LARGESTINTEGER53 && abs(bc) <= LARGESTINTEGER53));
        eResult = ad > bc ? SI_STRICTLY_POSITIVE : (ad < bc ? SI_STRICTLY_NEGATIVE : SI_ZERO);
    }
    else
    {
        // Use exact integer arithmetic, interval arithmetic is useless as ad == bc.
        Assert(ad == bc);
#ifdef LSI_AUDITING
        CLineSegmentIntersection::DecrementFpa();
        CLineSegmentIntersection::IncrementEa();
#endif
        CZ64 aCZ(a);
        CZ64 bCZ(b);
        CZ64 cCZ(c);
        CZ64 dCZ(d);
        aCZ.Multiply(dCZ);
        bCZ.Multiply(cCZ);

        eResult = static_cast<SIGNINDICATOR>(aCZ.Compare(bCZ));
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ComputeDeterminantExactSign
//
//  Synopsis:
//      Computes the sign of the determinant a*d - b*c.
//
//  Returns:
//      the determinant sign.
//
//------------------------------------------------------------------------------
inline 
SIGNINDICATOR 
ComputeDeterminantExactSign(double a, double b, double c, double d)
{
    // Return the exact sign of a*d - b*c.
    // We know that a, b, c, and d are exactly represented 
    // and are in the [-2^33, +2^33] closed range.

    Assert(IsValidInteger33(a) && IsValidInteger33(b) && 
           IsValidInteger33(c) && IsValidInteger33(d));

#ifdef LSI_AUDITING
    CLineSegmentIntersection::IncrementFpa();
#endif

    SIGNINDICATOR eResult = SI_ZERO;
    double ad = a * d;
    double bc = b * c;
    if (ad != bc)
    {
        // We can conclude if the products have different values.
        // An IEEE 754 multiplication returns the representable 
        // double closest to the true value.
        eResult = ad > bc ? SI_STRICTLY_POSITIVE : SI_STRICTLY_NEGATIVE;
    }
    else if (abs(ad) <= LARGESTINTEGER53)
    {
        Assert(ad == bc && abs(ad) <= LARGESTINTEGER53 && abs(bc) <= LARGESTINTEGER53);
        eResult = SI_ZERO;
    }
    else
    {
        // Use exact integer arithmetic, interval arithmetic is useless as ad == bc.
        Assert(ad == bc);
#ifdef LSI_AUDITING
        CLineSegmentIntersection::DecrementFpa();
        CLineSegmentIntersection::IncrementEa();
#endif
        CZ128 aCZ(a);
        CZ128 bCZ(b);
        CZ128 cCZ(c);
        CZ128 dCZ(d);
        aCZ.Multiply(dCZ);
        bCZ.Multiply(cCZ);

        eResult = static_cast<SIGNINDICATOR>(aCZ.Compare(bCZ));
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ClampToZeroOne
//
//  Synopsis:
//      Clamps the argument in the closed interval [0.0, 1.0]
//
//  Returns:
//      the clamped value
//
//------------------------------------------------------------------------------
double 
ClampToZeroOne(double v)
{
    return max(0.0, min(v, 1.0));
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      YXComparePoints
//
//  Synopsis:
//      Compares two points in y then x order.
//
//  Returns:
//      the comparison result.
//
//------------------------------------------------------------------------------
COMPARISON 
YXComparePoints(double xA, double yA, double xB, double yB)
{
    return yA > yB ? C_STRICTLYGREATERTHAN : (yA < yB ? C_STRICTLYLESSTHAN : 
           (xA > xB ? C_STRICTLYGREATERTHAN : (xA < xB ? C_STRICTLYLESSTHAN : C_EQUAL)));
}

// CLineSegmentIntersection Implementation of public methods.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::CLineSegmentIntersection
//
//  Synopsis:
//      Default constructor.
//
//------------------------------------------------------------------------------
CLineSegmentIntersection::CLineSegmentIntersection()
{
    Initialize();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::Initialize
//
//  Synopsis:
//      Initializes this instance.
//
//  Note:
//      This method must be called when a CLineSegmentIntersection is allocated
//      using a memory allocator. Use placement new as an alternative.
//
//------------------------------------------------------------------------------
void
CLineSegmentIntersection::Initialize()
{
    C_ASSERT(C_STRICTLYLESSTHAN == static_cast<COMPARISON>(SI_STRICTLY_NEGATIVE) &&
             C_EQUAL == static_cast<COMPARISON>(SI_ZERO) &&
             C_STRICTLYGREATERTHAN == static_cast<COMPARISON>(SI_STRICTLY_POSITIVE) &&
             SI_STRICTLY_NEGATIVE == -1 &&
             SI_ZERO == 0 &&
             SI_STRICTLY_POSITIVE == 1 &&
             CLineSegmentIntersection::SIDE_RIGHT == 
                static_cast<CLineSegmentIntersection::SIDEINDICATOR>(SI_STRICTLY_NEGATIVE) &&
             CLineSegmentIntersection::SIDE_INCIDENT == 
                static_cast<CLineSegmentIntersection::SIDEINDICATOR>(SI_ZERO) &&
             CLineSegmentIntersection::SIDE_LEFT == 
                static_cast<CLineSegmentIntersection::SIDEINDICATOR>(SI_STRICTLY_POSITIVE));

    m_eKind = CLineSegmentIntersection::KIND_UNDEFINED;
    m_eLocationAB = m_eLocationCD = CLineSegmentIntersection::LOCATION_UNDEFINED;

#ifdef LSI_AUDITING
    IncrementTotalInstanceCount();
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::~CLineSegmentIntersection
//
//  Synopsis:
//      Non virtual destructor.
//
//  Note:
//      CLineSegmentIntersection instances might be deallocated without this
//      destructor being called. If this destructor is modified to actually do
//      something, a DeInitialize method should be added to this class.  Use
//      placement delete as an alternative.
//
//------------------------------------------------------------------------------
CLineSegmentIntersection::~CLineSegmentIntersection()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::PairwiseIntersect
//
//  Synopsis:
//      Computes the intersection context for the line segment pair (AB, CD).
//
//  Returns:
//      The intersection kind.
//
//------------------------------------------------------------------------------
CLineSegmentIntersection::KIND 
CLineSegmentIntersection::PairwiseIntersect(
    __in_ecount(4) const Integer30 ab[4],
        // xA, yA, xB, yB 
    __in_ecount(4) const Integer30 cd[4],
        // xC, yC, xD, yD
    __out_ecount(1) LOCATION& eLocationAB,
        // The location of I, the tranvserse intersection
        // of AB and CD, wrt AB.
    __out_ecount(1) LOCATION& eLocationCD
        // The location of I, the tranvserse intersection
        // of AB and CD, wrt CD. 
)
{
#if DBG
    // Check input validity in debug version.
    Assert(ab[0] != ab[2] || ab[1] != ab[3]);
    Assert(cd[0] != cd[2] || cd[1] != cd[3]);
    for (int i = 0; i < 4; i++)
    {
        Assert(IsValidInteger30(ab[i]) && IsValidInteger30(cd[i]));
    }
#endif

#ifdef LSI_AUDITING
    IncrementPairwiseIntersectCount();
#endif

    Reset();
    eLocationAB = eLocationCD = LOCATION_UNDEFINED;

    if (min(ab[0], ab[2]) > max(cd[0], cd[2]) || 
        max(ab[0], ab[2]) < min(cd[0], cd[2]) ||
        min(ab[1], ab[3]) > max(cd[1], cd[3]) || 
        max(ab[1], ab[3]) < min(cd[1], cd[3]))
    {
        // Extents do not intersect.
        m_eKind = KIND_EMPTY;

#ifdef LSI_AUDITING
        IncrementPairwiseIntersectNoIntersection();
#endif
    }
    else
    {

        // Compute the differences, they are always exact.
        m_xAB = ab[2] - ab[0];
        m_yAB = ab[3] - ab[1];
        m_xDC = cd[0] - cd[2];
        m_yDC = cd[1] - cd[3];
        m_xAC = cd[0] - ab[0];
        m_yAC = cd[1] - ab[1];
        Assert(IsValidInteger31(m_xAB) && IsValidInteger31(m_yAB) && 
            IsValidInteger31(m_xDC) && IsValidInteger31(m_yDC) &&
            IsValidInteger31(m_xAC) && IsValidInteger31(m_yAC));

        // Store the coordinates of the first point.
        m_xA = ab[0];
        m_yA = ab[1];

        // Compute Determinant(AB, DC).
        m_fExactABDC = DeterminantIsExactDouble(m_xAB, m_yAB, m_xDC, m_yDC);
        if (m_fExactABDC)
        {
            m_rDeterminantABDC = (m_xAB * m_yDC) - (m_yAB * m_xDC);
            m_eSignABDC = m_rDeterminantABDC > 0 ? SI_STRICTLY_POSITIVE : 
                (m_rDeterminantABDC < 0 ? SI_STRICTLY_NEGATIVE : SI_ZERO);
        }
        else
        {
            m_eSignABDC = ComputeDeterminantExactSign(
                m_xAB, m_yAB, m_xDC, m_yDC, &m_rDeterminantABDC);
        }

        if (m_eSignABDC == SI_ZERO)
        {
            // Lines AB and CD are parallel.

            // Compute Determinant(AB, AC)
            m_fExactABAC = DeterminantIsExactDouble(m_xAB, m_yAB, m_xAC, m_yAC);
            if (m_fExactABAC)
            {
                m_rDeterminantABAC = (m_xAB * m_yAC) - (m_yAB * m_xAC);
                m_eSignABAC = m_rDeterminantABAC > 0 ? SI_STRICTLY_POSITIVE : 
                    (m_rDeterminantABAC < 0 ? SI_STRICTLY_NEGATIVE : SI_ZERO);
            }
            else
            {
                m_eSignABAC = ComputeDeterminantExactSign(
                    m_xAB, m_yAB, m_xAC, m_yAC, &m_rDeterminantABAC);
            }

            if (m_eSignABAC == 0)
            {
                // Find the region for C.
                REGION eRegionC = ComputeRegionWhenPointPIsOnAB(m_xAB, m_yAB, m_xAC, m_yAC);

                // Find the region for D. The subtraction is exact.
                REGION eRegionD = ComputeRegionWhenPointPIsOnAB(
                    m_xAB, m_yAB, cd[2] - ab[0], cd[3] - ab[1]);

                // Conclude.
                if ((eRegionC == REGION_HALFLINE_BEFOREFIRST &&
                     eRegionD == REGION_HALFLINE_BEFOREFIRST) ||
                    (eRegionC == REGION_HALFLINE_AFTERLAST && 
                     eRegionD == REGION_HALFLINE_AFTERLAST))
                {
                    m_eKind = KIND_EMPTY;
                }
                else
                {
                    m_eKind = KIND_NONTRANSVERSE;
                }
            }
            else
            {
                m_eKind = KIND_EMPTY;
            }
        }
        else
        {
            Assert(m_eSignABDC == SI_STRICTLY_NEGATIVE || m_eSignABDC == SI_STRICTLY_POSITIVE);

            // Compute Determinant(AC, DC)
            m_fExactACDC = DeterminantIsExactDouble(m_xAC, m_yAC, m_xDC, m_yDC);
            if (m_fExactACDC)
            {
                m_rDeterminantACDC = (m_xAC * m_yDC) - (m_yAC * m_xDC);
                m_eSignACDC = m_rDeterminantACDC > 0 ? SI_STRICTLY_POSITIVE : 
                    (m_rDeterminantACDC < 0 ? SI_STRICTLY_NEGATIVE : SI_ZERO);
            }
            else
            {
                m_eSignACDC = ComputeDeterminantExactSign(
                    m_xAC, m_yAC, m_xDC, m_yDC, &m_rDeterminantACDC);
            }

            // Test lambda >= 0.
            if ((m_eSignABDC == SI_STRICTLY_NEGATIVE && m_eSignACDC == SI_STRICTLY_POSITIVE) ||
                (m_eSignABDC == SI_STRICTLY_POSITIVE && m_eSignACDC == SI_STRICTLY_NEGATIVE))
            {
                m_eKind = KIND_EMPTY;
            }
            else
            {
                // Test lambda <= 1.
                COMPARISON eCompareLambdaAndOne = CompareDeterminantABDCandDeterminantACDC();
                if (m_eSignABDC == SI_STRICTLY_POSITIVE)
                {
                    eCompareLambdaAndOne = OppositeComparison(eCompareLambdaAndOne);
                }
                if (eCompareLambdaAndOne == C_STRICTLYGREATERTHAN)
                {
                    m_eKind = KIND_EMPTY;
                }
                else
                {
                    // Compute Determinant(AB, AC)
                    m_fExactABAC = DeterminantIsExactDouble(m_xAB, m_yAB, m_xAC, m_yAC);
                    if (m_fExactABAC)
                    {
                        m_rDeterminantABAC = (m_xAB * m_yAC) - (m_yAB * m_xAC);
                        m_eSignABAC = m_rDeterminantABAC > 0 ? SI_STRICTLY_POSITIVE :
                            (m_rDeterminantABAC < 0 ? SI_STRICTLY_NEGATIVE : SI_ZERO);
                    }
                    else
                    {
                        m_eSignABAC = ComputeDeterminantExactSign(
                            m_xAB, m_yAB, m_xAC, m_yAC, &m_rDeterminantABAC);
                    }

                    // Test mu >= 0.
                    if ((m_eSignABDC == SI_STRICTLY_NEGATIVE &&
                         m_eSignABAC == SI_STRICTLY_POSITIVE) ||
                        (m_eSignABDC == SI_STRICTLY_POSITIVE && 
                         m_eSignABAC == SI_STRICTLY_NEGATIVE))
                    {
                        m_eKind = KIND_EMPTY;
                    }
                    else
                    {
                        // Test mu <= 1.
                        COMPARISON eCompareMuAndOne = CompareDeterminantABDCandDeterminantABAC();
                        if (m_eSignABDC == SI_STRICTLY_POSITIVE)
                        {
                            eCompareMuAndOne = OppositeComparison(eCompareMuAndOne);
                        }
                        if (eCompareMuAndOne == C_STRICTLYGREATERTHAN)
                        {
                            m_eKind = KIND_EMPTY;
                        }
                        else
                        {
                            // The intersection is transverse.
                            m_eKind = KIND_TRANSVERSE;

                            // Set the location wrt AB.
                            if (m_eSignACDC == SI_ZERO)
                            {
                                // lambda = 0, intersection is in A.
                                m_eLocationAB = LOCATION_AT_FIRST_POINT;
                            }
                            else if (eCompareLambdaAndOne == C_STRICTLYLESSTHAN)
                            {
                                // 0 < lambda < 1, intersection is between A and B.
                                m_eLocationAB = LOCATION_ON_OPEN_SEGMENT;
                            }
                            else
                            {
                                // lambda = 1, intersection is in B.
                                m_eLocationAB = LOCATION_AT_LAST_POINT;
                            }

                            // Set the location wrt CD.
                            if (m_eSignABAC == SI_ZERO)
                            {
                                // mu = 0
                                m_eLocationCD = LOCATION_AT_FIRST_POINT;
                            }
                            else if (eCompareMuAndOne == C_STRICTLYLESSTHAN)
                            {
                                // 0 < mu < 1
                                m_eLocationCD = LOCATION_ON_OPEN_SEGMENT;
                            }
                            else
                            {
                                // mu = 1
                                m_eLocationCD = LOCATION_AT_LAST_POINT;
                            }                        
                        }
                    }
                }
            }
        }
    }
    #if DBG
    Assert(KindAndLocationsAreValid());
    #endif
    eLocationAB = m_eLocationAB;
    eLocationCD = m_eLocationCD;
    return m_eKind; 
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::IsEqual
//
//  Synopsis:
//      Tests this line segment intersection for equality with the argument.
//
//  Returns:
//      true if the two line segment intersections have the same input points
//      and false otherwise.
//
//  Note:
//      Points ABCD mut be equal to points EFGH and the order must be the same,
//      we do not test for permutations.
//
//------------------------------------------------------------------------------
bool 
CLineSegmentIntersection::IsEqual(
    __in_ecount(1) const CLineSegmentIntersection& efgh
) const
{
    #if DBG
    Assert(KindAndLocationsAreValid());
    Assert(efgh.KindAndLocationsAreValid());
    #endif

    return m_xA == efgh.m_xA && m_yA == efgh.m_yA && 
           m_xAB == efgh.m_xAB && m_yAB == efgh.m_yAB &&
           m_xAC == efgh.m_xAC && m_yAC == efgh.m_yAC &&
           m_xDC == efgh.m_xDC && m_yDC == efgh.m_yDC;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::ParameterAlongAB
//
//  Synopsis:
//      When the intersection between AB and CD exists and is transverse, the
//      intersection point I verifies AI = lambda * AB, with 0 <= lambda <= 1.
//
//  Returns:
//      An approximation of lambda if the conditions above are true, and -1
//      otherwise.
//
//------------------------------------------------------------------------------
double 
CLineSegmentIntersection::ParameterAlongAB() const
{
    Assert(IntersectionIsTransverse() && m_rDeterminantABDC != 0.0);

#ifdef LSI_AUDITING
    IncrementParameterAlongAB();
#endif

    double rResult = -1.0;

    if (IntersectionIsTransverse() && m_rDeterminantABDC != 0.0)
    {
        if (m_eLocationAB == LOCATION_AT_FIRST_POINT)
        {
            rResult = 0.0;
        }
        else if (m_eLocationAB == LOCATION_AT_LAST_POINT)
        {
            rResult = 1.0;
        }
        else
        {
            rResult = ClampToZeroOne(m_rDeterminantACDC / m_rDeterminantABDC);
        }
    }
    return rResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::ParameterAlongCD
//
//  Synopsis:
//      When the intersection between AB and CD exists and is transverse, the
//      intersection point I verifies CI = lambda * CD, with 0 <= lambda <= 1.
//
//  Returns:
//      An approximation of lambda if the conditions above are true, and -1
//      otherwise.
//
//------------------------------------------------------------------------------
double 
CLineSegmentIntersection::ParameterAlongCD() const
{

    Assert(IntersectionIsTransverse() && m_rDeterminantABDC != 0.0);

#ifdef LSI_AUDITING
    IncrementParameterAlongCD();
#endif

    double rResult = -1.0;

    if (IntersectionIsTransverse() && m_rDeterminantABDC != 0.0)
    {
        if (m_eLocationCD == LOCATION_AT_FIRST_POINT)
        {
            rResult = 0.0;
        }
        else if (m_eLocationCD == LOCATION_AT_LAST_POINT)
        {
            rResult = 1.0;
        }
        else
        {
            rResult = ClampToZeroOne(m_rDeterminantABAC / m_rDeterminantABDC);
        }
    }
    return rResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::LocateTransverseIntersectionRelativeToLine
//
//  Synopsis:
//      Assumes that the intersection between AB and CD exists and is transverse
//      on the open segments AB and CD, Locates the intersection point I wrt the
//      line defined by the argument. The line is oriented from E to F.
//
//  Returns:
//      SIDE_LEFT when I lies in the open half plane left of the line EF.
//      SIDE_INCIDENT when I is incident to the line EF.
//      SIDE_RIGHT when I lies in the open half plane right of the line EF.
//
//  Note:
//      This specification assumes a right-handed coordinate system. Exchange
//      right and left for a left-handed coordinate system.
//
//------------------------------------------------------------------------------
CLineSegmentIntersection::SIDEINDICATOR 
CLineSegmentIntersection::LocateTransverseIntersectionRelativeToLine(
    __in_ecount(4) const Integer30 ef[4]
        // The line defined by the two distinct points E and F
) const
{
    // Assume that PairwiseIntersect has been called and that the intersection between 
    // AB and CD exists and is transverse on the open segments AB and CD. 
    // Let I be the intersection point.

    Assert(IntersectionIsTransverse() && 
        m_eLocationAB == LOCATION_ON_OPEN_SEGMENT &&
        m_eLocationCD == LOCATION_ON_OPEN_SEGMENT);

    Assert(ef[0] != ef[2] || ef[1] != ef[3]);

#ifdef LSI_AUDITING
    IncrementLocateTransverseIntersectionRelativeToLine();
    IncrementFpa();
#endif

    SIGNINDICATOR eResult = SI_ZERO;

    // If either A and B or C and D are left of the line EF, then I is left of EF.

    // Compute sign of Determinant(EF, EA) and sign of Determinant(EF, EB)
    // The arguments are exact as they need at most 33 bits.
    SIGNINDICATOR eSignA = ComputeDeterminantExactSign(
        ef[2] - ef[0], ef[3] - ef[1], m_xA - ef[0], m_yA - ef[1]);
    SIGNINDICATOR eSignB = ComputeDeterminantExactSign(
        ef[2] - ef[0], ef[3] - ef[1], m_xAB + (m_xA - ef[0]), m_yAB + (m_yA - ef[1]));

    if (eSignA == eSignB)
    {
        eResult = eSignA;
    }
    else if (eSignA == SI_ZERO)
    {
        // We know that I is not in A.
        eResult = eSignB;
    }
    else if (eSignB == SI_ZERO)
    {
        // We know that I is not in B.
        eResult = eSignA;
    }
    else
    {
        Assert((eSignA == SI_STRICTLY_NEGATIVE && eSignB == SI_STRICTLY_POSITIVE) || 
               (eSignB == SI_STRICTLY_NEGATIVE && eSignA == SI_STRICTLY_POSITIVE));

        // Sign of Determinant(EF, EC), the arguments need at most 32 bits.
        SIGNINDICATOR eSignC = ComputeDeterminantExactSign(
            ef[2] - ef[0], 
            ef[3] - ef[1], 
            m_xAC + (m_xA - ef[0]), 
            m_yAC + (m_yA - ef[1])
            );

        // Sign of Determinant(EF, ED), the arguments need at most 33 bits.
        SIGNINDICATOR eSignD = ComputeDeterminantExactSign(
            ef[2] - ef[0], 
            ef[3] - ef[1], 
            (m_xAC - m_xDC) + (m_xA - ef[0]), 
            (m_yAC - m_yDC) + (m_yA - ef[1])
            );

        if (eSignC == eSignD)
        {
            eResult = eSignC;
        }
        else if (eSignC == SI_ZERO)
        {
            // We know that I is not in C.
            eResult = eSignD;
        }
        else if (eSignD == SI_ZERO)
        {
            // We know that I is not in D.
            eResult = eSignC;
        }
        else
        {
            Assert((eSignC == SI_STRICTLY_NEGATIVE && eSignD == SI_STRICTLY_POSITIVE) ||
                   (eSignD == SI_STRICTLY_NEGATIVE && eSignC == SI_STRICTLY_POSITIVE));

            // We know that the intersection I is transverse on open segments AB and CD.
            // I = A + lambda1 * AB and lambda1 = Determinant(AC, DC) / Determinant(AB, DC)
            // Because eSignA is different from eSignB, the line segment AB intersects the line EF
            // and we can write the intersection point J as:
            // J = A + lambda2 * AB and lambda2 = Determinant(AE, FE) / Determinant(AB, FE)
            // If lambda1 < lambda2, I is on A's side of the line EF,
            // if lambda1 == lambda2, I is on the line EF,
            // if lambda1 > lambda2, I is on B's side of the line EF.

#ifdef LSI_AUDITING
            DecrementFpa();
            IncrementIa();
#endif

            // Try using interval arithmetic.
            CIntegralInterval oDetABDC(m_xAB, m_yAB, m_xDC, m_yDC);
            CIntegralInterval oDetACDC(m_xAC, m_yAC, m_xDC, m_yDC);
            CIntegralInterval oDetABFE(m_xAB, m_yAB, ef[2] - ef[0], ef[3] - ef[1]);
            CIntegralInterval oDetAEFE(ef[0] - m_xA, ef[1] - m_yA, ef[2] - ef[0], ef[3] - ef[1]);
            SIGNINDICATOR eSignABDC = oDetABDC.GetSign();
            SIGNINDICATOR eSignABFE = oDetABFE.GetSign();
            COMPARISON eComparison = C_UNDEFINED;
            if (eSignABDC != SI_ZERO && eSignABFE != SI_ZERO)
            {
                if (eSignABDC * eSignABFE == 1)
                {
                    eComparison = oDetACDC.Multiply(oDetABFE).Compare(oDetABDC.Multiply(oDetAEFE));
                }
                else
                {
                    eComparison = oDetABDC.Multiply(oDetAEFE).Compare(oDetACDC.Multiply(oDetABFE));
                }
                if (eComparison != C_UNDEFINED)
                {
                    eResult = eComparison == C_STRICTLYLESSTHAN ? eSignA : 
                        (eComparison == C_STRICTLYGREATERTHAN ? eSignB : SI_ZERO);
                }
            }

            if (eComparison == C_UNDEFINED)
            {
                // Use exact integer arithmetic.
                CZ192 z1(m_xAB);
                CZ192 z2(m_yAB);
                CZ192 z3(m_xDC);
                CZ192 z4(m_yDC);
                z1.Multiply(z4);
                z2.Multiply(z3);
                z1.Subtract(z2);
                // z1 equals Determinant(AB, DC)

                CZ192 z5(m_xAC);
                CZ192 z6(m_yAC);
                z5.Multiply(z4);
                z6.Multiply(z3);
                z5.Subtract(z6);
                // z5 equals Determinant(AC, DC)

                CZ192 zz1(m_xAB);
                CZ192 zz2(m_yAB);
                CZ192 zz3(ef[2] - ef[0]);
                CZ192 zz4(ef[3] - ef[1]);
                zz1.Multiply(zz4);
                zz2.Multiply(zz3);
                zz1.Subtract(zz2);
                // zz1 equals Determinant(AB, FE)

                CZ192 zz5(ef[0] - m_xA);
                CZ192 zz6(ef[1] - m_yA);
                zz5.Multiply(zz4);
                zz6.Multiply(zz3);
                zz5.Subtract(zz6);
                // zz5 equals Determinant(AE, FE)

                Assert(z1.GetSign() == -1 || z1.GetSign() == 1);
                Assert(zz1.GetSign() == -1 || zz1.GetSign() == 1);
                if (z1.GetSign() * zz1.GetSign() == 1)
                {
                    eComparison = z5.Multiply(zz1).Compare(z1.Multiply(zz5));
                }
                else
                {
                   eComparison = z1.Multiply(zz5).Compare(z5.Multiply(zz1));
                }
                Assert(eComparison != C_UNDEFINED);
                eResult = eComparison == C_STRICTLYLESSTHAN ? eSignA : 
                            (eComparison == C_STRICTLYGREATERTHAN ? eSignB : SI_ZERO);

#ifdef LSI_AUDITING
                DecrementIa();
                IncrementEa();
#endif
            }
        }
    }
    return static_cast<CLineSegmentIntersection::SIDEINDICATOR>(eResult);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::YXSortTransverseIntersectionPair
//
//  Synopsis:
//      Sorts two intersection points.
//
//  Returns:
//      The order of the intersection points AB ^ CD and EF ^ GH in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::YXSortTransverseIntersectionPair(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line segment intersection for (AB, CD). 
    __in_ecount(1) const CLineSegmentIntersection& efgh
        // Line segment intersection for (EF, GH). 
    )
{
    Assert(abcd.IntersectionIsTransverse() && efgh.IntersectionIsTransverse());

#ifdef LSI_AUDITING
    IncrementYXSortTransverseIntersectionPair();
    IncrementFpa();
#endif

    COMPARISON eResult = C_UNDEFINED;

#if DBG
    // Debug version test. This function should not be called when the two arguments
    // have exactly the same geometry. This function will succeed but it has to use 
    // exact arithmetic in some cases.
    if (abcd.IsEqual(efgh))
    {
#ifdef LSI_AUDITING
        DecrementFpa();
        IncrementEqualArgumentsToYXSortTransverseIntersectionPair();
#endif
    }
#endif

    // If the Y spans for the two intersections do not intersect we can conclude.
    Integer30 v1[2], v2[2];
    abcd.GetTransverseIntersectionYSpan(v1);
    efgh.GetTransverseIntersectionYSpan(v2);
    if (v1[1] < v2[0])
    {
        eResult = C_STRICTLYLESSTHAN;
    }
    else if (v1[0] > v2[1])
    {
        eResult = C_STRICTLYGREATERTHAN;
    }
    else if (abcd.IntersectionIsTransverseOnOpenSegments() && 
             efgh.IntersectionIsTransverseOnOpenSegments())
    {
        // Let I = AB ^ CD, J = EF ^ GH
        // I = A + lambda1 * AB, J = E + lambda2 * EF

        // lambda1 = Determinant(AC, DC) / Determinant(AB, DC)
        // lambda2 = Determinant(EG, HG) / Determinant(EF, HG)
        // 0 < lambda1, lambda2 < 1

        if (abcd.DeterminantACDCIsExact() && abcd.DeterminantABDCIsExact() && 
            efgh.DeterminantACDCIsExact() && efgh.DeterminantABDCIsExact())
        {
            double rMin1, rMax1, rMin2, rMax2;
            abcd.ComputeIntersectionPointYCoordinateInterval(rMin1, rMax1);
            efgh.ComputeIntersectionPointYCoordinateInterval(rMin2, rMax2);
            if (rMin1 > rMax2)
            {
                eResult = C_STRICTLYGREATERTHAN;
            }
            else if (rMax1 < rMin2)
            {
                eResult = C_STRICTLYLESSTHAN;
            }
        }
        if (eResult == C_UNDEFINED)
        {

#ifdef LSI_AUDITING
            DecrementFpa();
            IncrementIa();
#endif

            eResult = YXSortTransverseIntersectionPairUsingIntervalArithmetic(abcd, efgh);
            if (eResult == C_UNDEFINED)
            {
                eResult = YXSortTransverseIntersectionPairUsingExactArithmetic(abcd, efgh);
                Assert(eResult != C_UNDEFINED);

#ifdef LSI_AUDITING
                DecrementIa();
                IncrementEa();
#endif

            }
        }
    }
    else
    {
        // One intersection point is a line segment endpoint.
        eResult = YXSortSpecificPosition(abcd, efgh);

#ifdef LSI_AUDITING
        DecrementFpa();
#endif

    }

    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::YXSortTransverseIntersectionAndPoint
//
//  Synopsis:
//      Sorts an intersection point and a point.
//
//  Returns:
//      The order of the intersection point AB ^ CD and of the point E in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::YXSortTransverseIntersectionAndPoint(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // The line segment intersection AB ^ CD 
    __in_ecount(2) const Integer30 e[2]
        // The (x, y) coordinates for a point
)
{
    // Let AB and CD be two line segments such that the pair (AB, CD) has a transverse intersection
    // and let E be an input point.
    // Assume that the method PairwiseIntersect(AB, CD) has been called on the first argument.
    // Compare the intersection point AB ^ CD and E in the y coordinate then x coordinate ordering
    // and retun the result.

    Assert(abcd.IntersectionIsTransverse());

#ifdef LSI_AUDITING
    IncrementYXSortTransverseIntersectionAndPoint();
    IncrementFpa();
#endif

    // Let I = AB ^ CD
    // I = A + lambda1 * AB
    // lambda1 = Determinant(AC, DC) / Determinant(AB, DC)

    COMPARISON eResult = C_UNDEFINED;
    bool fXComparisonOnly = false;
    Integer30 v[2];

    abcd.GetTransverseIntersectionYSpan(v);
    if (e[1] < v[0] )
    {
        eResult = C_STRICTLYGREATERTHAN;
    }
    else if (e[1] > v[1])
    {
        eResult = C_STRICTLYLESSTHAN;
    }
    else if (abcd.DeterminantACDCIsExact() && abcd.DeterminantABDCIsExact())
    {
        // Compute the 2 numbers we need to compare.
        // The coordinates of AE are exact.
        double yLHS = abcd.DeterminantACDC() * abcd.m_yAB;
        double yRHS = abcd.DeterminantABDC() * (e[1] - abcd.m_yA);

        if (yLHS != yRHS)
        {
            // We can safely conclude.
            if (abcd.SignDeterminantABDC() == SI_STRICTLY_POSITIVE)
            {
                eResult = yLHS > yRHS ? C_STRICTLYGREATERTHAN : C_STRICTLYLESSTHAN;
            }
            else
            {
                Assert(abcd.SignDeterminantABDC() == SI_STRICTLY_NEGATIVE);
                eResult = yLHS > yRHS ? C_STRICTLYLESSTHAN : C_STRICTLYGREATERTHAN;
            }
        }
        else if (abs(abcd.DeterminantACDC()) <= LARGESTINTEGER26 && 
                 abs(abcd.m_yAB) <= LARGESTINTEGER26 &&
                 abs(abcd.DeterminantABDC()) <= LARGESTINTEGER26 && 
                 abs(e[1] - abcd.m_yA) <= LARGESTINTEGER26)
        {
            // yLHS and yRHS are truly equal. Compare the X values.
            Assert(yLHS == yRHS);
            double xLHS = abcd.DeterminantACDC() * abcd.m_xAB;
            double xRHS = abcd.DeterminantABDC() * (e[0] - abcd.m_xA);
            fXComparisonOnly = true;
            if (xLHS != xRHS)
            {
                // We can safely conclude.
                if (abcd.SignDeterminantABDC() == SI_STRICTLY_POSITIVE)
                {
                    eResult = xLHS > xRHS ? C_STRICTLYGREATERTHAN : C_STRICTLYLESSTHAN;
                }
                else
                {
                    Assert(abcd.SignDeterminantABDC() == SI_STRICTLY_NEGATIVE);
                    eResult = xLHS > xRHS ? C_STRICTLYLESSTHAN : C_STRICTLYGREATERTHAN;
                }
            }
            else if (abs(abcd.m_xAB) <= LARGESTINTEGER26 &&
                     abs(e[0] - abcd.m_xA) <= LARGESTINTEGER26)
            {
                eResult = C_EQUAL;
            }
        }
    }

    if (eResult == C_UNDEFINED)
    {
        eResult = YXSortTransverseIntersectionAndPointUsingIntervalArithmetic(
            abcd, e, fXComparisonOnly);

#ifdef LSI_AUDITING
        DecrementFpa();
        IncrementIa();
#endif

        if (eResult == C_UNDEFINED)
        {
            eResult = YXSortTransverseIntersectionAndPointUsingExactArithmetic(
                abcd, e, fXComparisonOnly);

#ifdef LSI_AUDITING
            DecrementIa();
            IncrementEa();
#endif

        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::LocatePointRelativeToLine
//
//  Synopsis:
//      Locates a point wrt a line.
//
//  Returns:
//      SIDE_LEFT when C lies in the open half plane left of the line AB.
//      SIDE_INCIDENT when C is incident to the line AB.
//      SIDE_RIGHT when C lies in the open half plane right of the line AB.  
//
//  Note:
//      The line AB is oriented from A to B. This is a static member. This
//      specification assumes a right-handed coordinate system. Exchange right
//      and left for a left-handed coordinate system.
//
//------------------------------------------------------------------------------
CLineSegmentIntersection::SIDEINDICATOR 
CLineSegmentIntersection::LocatePointRelativeToLine(
    __in_ecount(2) const Integer30 c[2],
        // The point C
    __in_ecount(4) const Integer30 ab[4]
        // The line through A and B, oriented from A to B
)
{
    // Assume that A and B are distinct.
    Assert(ab[0] != ab[2] || ab[1] != ab[3]);

#ifdef LSI_AUDITING
    IncrementLocatePointRelativeToLine();
    IncrementFpa();
#endif

    SIDEINDICATOR eResult = SIDE_INCIDENT;

    // C is left of AB iff Determinant(AB, AC) is > 0.
    double xAB = ab[2] - ab[0], yAB = ab[3] - ab[1];
    double xAC = c[0] - ab[0], yAC = c[1] - ab[1];
    if (DeterminantIsExactDouble(xAB, yAB, xAC, yAC))
    {
        eResult = xAB * yAC - yAB * xAC > 0 ? SIDE_LEFT : 
            (xAB * yAC - yAB * xAC < 0 ? SIDE_RIGHT : SIDE_INCIDENT);
    }
    else
    {
#ifdef LSI_AUDITING
        DecrementFpa();
#endif
        eResult = static_cast<CLineSegmentIntersection::SIDEINDICATOR>(
            ComputeDeterminantExactSign(xAB, yAB, xAC, yAC));
    }
    return eResult;
}

// CLineSegmentIntersection Implementation of private methods.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::Reset
//
//  Synopsis:
//      Resets this instance data members.
//
//------------------------------------------------------------------------------
void 
CLineSegmentIntersection::Reset()
{
    // Reset some of our data members.
    m_eKind = KIND_UNDEFINED;
    m_eLocationAB = m_eLocationCD = LOCATION_UNDEFINED;
    m_fExactABDC = m_fExactACDC = m_fExactABAC = false;
    m_eSignABDC = m_eSignACDC = m_eSignABAC = SI_ZERO;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::ComputeIntersectionPointXCoordinateInterval
//
//  Synopsis:
//      Assumes that this instance is transverse on open segments and that the
//      determinants ACDC and ABDC are exact. Computes an interval containing
//      the x coordinate of the intersection point.
//
//  Returns:
//      The interval bounds.
//
//------------------------------------------------------------------------------
void 
CLineSegmentIntersection::ComputeIntersectionPointXCoordinateInterval(
    __out_ecount(1) double& rMin, 
    __out_ecount(1) double& rMax
) const
{
    // We know that the intersection is transverse on open segments.
    // I = A + lambda * AB
    // lambda = Determinant(AC, DC) / Determinant(AB, DC)

    Assert(DeterminantACDCIsExact() && DeterminantABDCIsExact());
    Assert(DeterminantABDC() != 0 && DeterminantABDC() != 0);

    // lambda cannot be s.t. PreviousDouble(lambda) is denormalized.
    double lambda = abs(DeterminantACDC() / DeterminantABDC());
    rMin = PreviousDouble(lambda);
    rMax = NextDouble(lambda); 

    // The equal signs are necessary because of the PreviousDouble and NextDouble calls.
    Assert(rMin >= 0 && rMax <= 1);

    if (m_xAB > 0)
    {
        rMin = PreviousDouble(PreviousDouble(rMin * m_xAB) + m_xA);
        rMax = NextDouble(NextDouble(rMax * m_xAB) + m_xA);
    }
    else if (m_xAB == 0)
    {
        rMin = rMax = m_xA;
    }
    else
    {
        double rMinAux = rMin;
        rMin = PreviousDouble(PreviousDouble(rMax * m_xAB) + m_xA);
        rMax = NextDouble(NextDouble(rMinAux * m_xAB) + m_xA);
    }
    Assert(rMin <= rMax);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::ComputeIntersectionPointYCoordinateInterval
//
//  Synopsis:
//      Assumes that this instance is transverse on open segments and that the
//      determinants ACDC and ABDC are exact. Computes an interval containing
//      the y coordinate of the intersection point.
//
//  Returns:
//      The interval bounds.
//
//------------------------------------------------------------------------------
void 
CLineSegmentIntersection::ComputeIntersectionPointYCoordinateInterval(
    __out_ecount(1) double& rMin, 
    __out_ecount(1) double& rMax
) const
{
    // We know that the intersection is transverse on open segments.
    // I = A + lambda * AB
    // lambda = Determinant(AC, DC) / Determinant(AB, DC)

    Assert(DeterminantACDCIsExact() && DeterminantABDCIsExact());
    Assert(DeterminantABDC() != 0 && DeterminantABDC() != 0);

    // lambda cannot be s.t. PreviousDouble(lambda) is denormalized.
    double lambda = abs(DeterminantACDC() / DeterminantABDC());
    rMin = PreviousDouble(lambda);
    rMax = NextDouble(lambda);  

    // The equal signs are necessary because of the PreviousDouble and NextDouble calls.
    Assert(rMin >= 0 && rMax <= 1);

    if (m_yAB > 0)
    {
        rMin = PreviousDouble(PreviousDouble(rMin * m_yAB) + m_yA);
        rMax = NextDouble(NextDouble(rMax * m_yAB) + m_yA);
    }
    else if (m_yAB == 0)
    {
        rMin = rMax = m_yA;
    }
    else
    {
        double rMinAux = rMin;
        rMin = PreviousDouble(PreviousDouble(rMax * m_yAB) + m_yA);
        rMax = NextDouble(NextDouble(rMinAux * m_yAB) + m_yA);
    }
    Assert(rMin <= rMax);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::CompareDeterminantABDCandDeterminantACDC
//
//  Synopsis:
//      Compares det(AB, DC) and det(AC, DC).
//
//  Returns:
//      The result of the comparison.
//
//------------------------------------------------------------------------------
COMPARISON  
CLineSegmentIntersection::CompareDeterminantABDCandDeterminantACDC()
{

#ifdef LSI_AUDITING
    CLineSegmentIntersection::IncrementFpa();
#endif

    COMPARISON eResult = C_UNDEFINED;

    if (m_fExactABDC && m_fExactACDC)
    {
        eResult = m_rDeterminantABDC < m_rDeterminantACDC ? C_STRICTLYLESSTHAN : 
            (m_rDeterminantABDC > m_rDeterminantACDC ? C_STRICTLYGREATERTHAN : C_EQUAL);
    }
    else
    {
        // Compare xDC * (yAC - yAB) and yDC * (xAC - xAB) The differences are exact.
        double dy = m_yAC - m_yAB;
        double dx = m_xAC - m_xAB;
        if ((m_xDC == 0 && dx == 0) || (dy == 0 && m_yDC == 0) || (dy == 0 && dx == 0))
        {
            eResult = C_EQUAL;
        }
        else
        {
            // Here, the sign of a possibly inexact product is exact. The operands are integers,
            // therefore only values greater or smaller than 2^53 can be inexact. However, as 
            // an IEEE 754 multiplication returns the representable double closest to the true 
            // result, the signs of the products p1 and p2 are exact.

            double p1 = m_xDC * dy;
            double p2 = m_yDC * dx;
            if (p1 >= 0 && p2 <= 0)
            {
                // Both products can't be 0.
                Assert(p1 >= 0 && p2 < 0 || p1 > 0 && p2 <= 0);
                eResult = C_STRICTLYGREATERTHAN;
            }
            else if (p1 <= 0 && p2 >= 0)
            {
                // Both products can't be 0.
                Assert(p1 <= 0 && p2 > 0 || p1 < 0 && p2 >= 0);
                eResult = C_STRICTLYLESSTHAN;
            }
            else
            {
                // p1 and p2 have the same sign and none of them is 0.
                Assert(p1 * p2 > 0);

                // We can conclude if the products have different values.
                // An IEEE 754 multiplication returns the representable 
                // double closest to the true value.
                if (p1 != p2)
                {
                    eResult = p1 > p2 ? C_STRICTLYGREATERTHAN : C_STRICTLYLESSTHAN;
                }
                else
                {
                    // Use exact integer arithmetic. Interval arithmetic is useless.
                    CZ64 z1(m_xDC);
                    CZ64 z2(m_yDC);
                    CZ64 z3(dx);
                    CZ64 z4(dy);
                    return z1.Multiply(z4).Compare(z2.Multiply(z3));

#ifdef LSI_AUDITING
                    CLineSegmentIntersection::DecrementFpa();
                    CLineSegmentIntersection::IncrementEa();
#endif
                }
            }
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::CompareDeterminantABDCandDeterminantABAC
//
//  Synopsis:
//      Compares det(AB, DC) and det(AB, AC).
//
//  Returns:
//      The result of the comparison.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::CompareDeterminantABDCandDeterminantABAC()
{

#ifdef LSI_AUDITING
    CLineSegmentIntersection::IncrementFpa();
#endif

    COMPARISON eResult = C_UNDEFINED;
    if (m_fExactABDC && m_fExactABAC)
    {
        eResult = m_rDeterminantABDC < m_rDeterminantABAC ? C_STRICTLYLESSTHAN : 
            (m_rDeterminantABDC > m_rDeterminantABAC ? C_STRICTLYGREATERTHAN : C_EQUAL);
    }
    else
    {
        // Compare xAB * (yDC - yAC) and yAB * (xDC - xAC), the differences are exact.
        double dy = m_yDC - m_yAC;
        double dx = m_xDC - m_xAC;
        if ((m_xAB == 0 && dx == 0) || (dy == 0 && m_yAB == 0) || (dy == 0 && dx == 0))
        {
            eResult = C_EQUAL;
        }
        else
        {
            // Here, the sign of a possibly inexact product is exact. The operands are integers,
            // therefore only values greater or smaller than 2^53 can be inexact. However, as 
            // an IEEE 754 multiplication returns the representable double closest to the true 
            // result, the signs of the products p1 and p2 are exact. 
    
            double p1 = m_xAB * dy;
            double p2 = m_yAB * dx;
            if (p1 >= 0 && p2 <= 0)
            {
                // Both products can't be 0.
                Assert(p1 >= 0 && p2 < 0 || p1 > 0 && p2 <= 0);
                eResult = C_STRICTLYGREATERTHAN;
            }
            else if (p1 <= 0 && p2 >= 0)
            {
                // Both products can't be 0.
                Assert(p1 <= 0 && p2 > 0 || p1 < 0 && p2 >= 0);
                eResult = C_STRICTLYLESSTHAN;
            }
            else
            {
                // p1 and p2 have the same sign and none of them is 0.
                Assert(p1 * p2 > 0);

                // We can conclude if the products have different values.
                // An IEEE 754 multiplication returns the representable 
                // double closest to the true value.
                if (p1 != p2)
                {
                    eResult = p1 > p2 ? C_STRICTLYGREATERTHAN : C_STRICTLYLESSTHAN;
                }
                else
                {
                    // Use exact integer arithmetic, interval arithmetic is useless
                    // in this case.
                    CZ64 z1(m_xAB);
                    CZ64 z2(m_yAB);
                    CZ64 z3(dx);
                    CZ64 z4(dy);
                    eResult = z1.Multiply(z4).Compare(z2.Multiply(z3));

#ifdef LSI_AUDITING
                    CLineSegmentIntersection::DecrementFpa();
                    CLineSegmentIntersection::IncrementEa();
#endif
                }
            }
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::ComputeRegionWhenPointPIsOnAB
//
//  Synopsis:
//      Computes the region where P lies when P is on the line through A and B
//
//  Returns:
//      The region.
//
//------------------------------------------------------------------------------
CLineSegmentIntersection::REGION 
CLineSegmentIntersection::ComputeRegionWhenPointPIsOnAB(
    IN double xAB,              // The x component of the vector AB
    IN double yAB,              // The y component of the vector AB
    IN double xAP,              // The x component of the vector AP
    IN double yAP               // The y component of the vector AP
)
{
    // We know that point P is on the line AB.
    // xAB and yAB are the components of the vector AB.
    // xAP and yAP are the components of the vector AP.
    // Return the region where P lies on line AB.
    // We know that A and B are different.
    
    REGION eResult = REGION_UNDEFINED;
    if (xAB != 0)
    {
        // Use the X axis projection.
        if (xAB > 0)
        {
            if (xAP < 0)
            {
                eResult = REGION_HALFLINE_BEFOREFIRST;
            }
            else if (xAP == 0)
            {
                eResult = REGION_EQUAL_TO_FIRST;
            }
            else if (xAP < xAB)
            {
                eResult = REGION_OPEN_LINESEGMENT;
            }
            else if (xAP == xAB)
            {
                eResult = REGION_EQUAL_TO_LAST;
            }
            else
            {
                eResult = REGION_HALFLINE_AFTERLAST;
            }
        }
        else
        {
            if (xAP > 0)
            {
                eResult = REGION_HALFLINE_BEFOREFIRST;
            }
            else if (xAP == 0)
            {
                eResult = REGION_EQUAL_TO_FIRST;
            }
            else if (xAP > xAB)
            {
                eResult = REGION_OPEN_LINESEGMENT;
            }
            else if (xAP == xAB)
            {
                eResult = REGION_EQUAL_TO_LAST;
            }
            else
            {
                eResult = REGION_HALFLINE_AFTERLAST;
            }
        }
    }
    else
    {
        // Use the Y axis projection.
        Assert(yAB != 0);
        if (yAB > 0)
        {
            if (yAP < 0)
            {
                eResult = REGION_HALFLINE_BEFOREFIRST;
            }
            else if (yAP == 0)
            {
                eResult = REGION_EQUAL_TO_FIRST;
            }
            else if (yAP < yAB)
            {
                eResult = REGION_OPEN_LINESEGMENT;
            }
            else if (yAP == yAB)
            {
                eResult = REGION_EQUAL_TO_LAST;
            }
            else
            {
                eResult = REGION_HALFLINE_AFTERLAST;
            }
        }
        else
        {
            if (yAP > 0)
            {
                eResult = REGION_HALFLINE_BEFOREFIRST;
            }
            else if (yAP == 0)
            {
                eResult = REGION_EQUAL_TO_FIRST;
            }
            else if (yAP > yAB)
            {
                eResult = REGION_OPEN_LINESEGMENT;
            }
            else if (yAP == yAB)
            {
                eResult = REGION_EQUAL_TO_LAST;
            }
            else
            {
                eResult = REGION_HALFLINE_AFTERLAST;
            }
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::YXSortTransverseIntersectionPairUsingIntervalArithmetic
//
//  Synopsis:
//      Sorts two intersection points using interval arithmetic.
//
//  Returns:
//      The order of the intersection points AB ^ CD and EF ^ GH in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::YXSortTransverseIntersectionPairUsingIntervalArithmetic(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line segment intersection for (AB, CD). 
    __in_ecount(1) const CLineSegmentIntersection& efgh
        // Line segment intersection for (EF, GH). 
)
{
    COMPARISON eResult = C_UNDEFINED;

    // z1 equals abcd.DeterminantABDC()
    CIntegralInterval z1(abcd.m_xAB, abcd.m_yAB, abcd.m_xDC, abcd.m_yDC);
    CIntegralInterval z7(abcd.m_yAB);

    // z5 equals abcd.DeterminantACDC()
    CIntegralInterval z5(abcd.m_xAC, abcd.m_yAC, abcd.m_xDC, abcd.m_yDC);

    // zz1 equals efgh.DeterminantABDC()
    CIntegralInterval zz1(efgh.m_xAB, efgh.m_yAB, efgh.m_xDC, efgh.m_yDC);
    CIntegralInterval zz7(efgh.m_yAB);

    // zz5 equals efgh.DeterminantACDC()
    CIntegralInterval zz5(efgh.m_xAC, efgh.m_yAC, efgh.m_xDC, efgh.m_yDC);

    z5.Multiply(zz1);
    zz5.Multiply(z1);
    z1.Multiply(zz1);
    if (z1.GetSign() != SI_ZERO)
    {
        CIntegralInterval yLHS(abcd.m_yA);
        CIntegralInterval yRHS(efgh.m_yA);
        yLHS.Multiply(z1).Add(z7.Multiply(z5));       
        yRHS.Multiply(z1).Add(zz7.Multiply(zz5));

        if (z1.GetSign() == SI_STRICTLY_NEGATIVE)
        {
            if ((eResult = yRHS.Compare(yLHS)) == C_EQUAL)
            {
                // Compare the X coordinate.
                CIntegralInterval z8(abcd.m_xAB);
                CIntegralInterval xLHS(abcd.m_xA);
                CIntegralInterval zz8(efgh.m_xAB);
                CIntegralInterval xRHS(efgh.m_xA);
                xLHS.Multiply(z1).Add(z8.Multiply(z5));
                xRHS.Multiply(z1).Add(zz8.Multiply(zz5));
                eResult = xRHS.Compare(xLHS);
            }
        }
        else
        {
            if ((eResult = yLHS.Compare(yRHS)) == C_EQUAL)
            {
                // Compare the X coordinate.
                CIntegralInterval z8(abcd.m_xAB);
                CIntegralInterval xLHS(abcd.m_xA);
                CIntegralInterval zz8(efgh.m_xAB);
                CIntegralInterval xRHS(efgh.m_xA);
                xLHS.Multiply(z1).Add(z8.Multiply(z5));
                xRHS.Multiply(z1).Add(zz8.Multiply(zz5));
                eResult = xLHS.Compare(xRHS);
            }
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::YXSortTransverseIntersectionPairUsingExactArithmetic
//
//  Synopsis:
//      Sorts two intersection points using exact arithmetic.
//
//  Returns:
//      The order of the intersection points AB ^ CD and EF ^ GH in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON CLineSegmentIntersection::YXSortTransverseIntersectionPairUsingExactArithmetic(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line segment intersection for (AB, CD). 
    __in_ecount(1) const CLineSegmentIntersection& efgh
        // Line segment intersection for (EF, GH). 
    )
{
    COMPARISON eResult = C_UNDEFINED;

    CZ192 z1(abcd.m_xAB);
    CZ192 z2(abcd.m_yAB);
    CZ192 z7(abcd.m_yAB);
    CZ192 z3(abcd.m_xDC);
    CZ192 z4(abcd.m_yDC);
    CZ192 z5(abcd.m_xAC);
    CZ192 z6(abcd.m_yAC);

    CZ192 zz1(efgh.m_xAB);
    CZ192 zz2(efgh.m_yAB);
    CZ192 zz7(efgh.m_yAB);
    CZ192 zz3(efgh.m_xDC);
    CZ192 zz4(efgh.m_yDC);
    CZ192 zz5(efgh.m_xAC);
    CZ192 zz6(efgh.m_yAC);

    CZ192 yLHS(abcd.m_yA);
    CZ192 yRHS(efgh.m_yA);

    z1.Multiply(z4);
    z2.Multiply(z3);
    z1.Subtract(z2);
    // z1 equals abcd.DeterminantABDC()

    z5.Multiply(z4);
    z6.Multiply(z3);
    z5.Subtract(z6);
    // z5 equals abcd.DeterminantACDC()

    zz1.Multiply(zz4);
    zz2.Multiply(zz3);
    zz1.Subtract(zz2);
    // zz1 equals efgh.DeterminantABDC()

    zz5.Multiply(zz4);
    zz6.Multiply(zz3);
    zz5.Subtract(zz6);
    // zz5 equals efgh.DeterminantACDC()

    z5.Multiply(zz1);
    zz5.Multiply(z1);
    z1.Multiply(zz1);
    Assert(z1.GetSign() == -1 || z1.GetSign() == 1);

    yLHS.Multiply(z1).Add(z7.Multiply(z5));
    yRHS.Multiply(z1).Add(zz7.Multiply(zz5));

    if (z1.GetSign() == -1)
    {
        if ((eResult = yRHS.Compare(yLHS)) == C_EQUAL)
        {
            // Compare the X coordinate.
            CZ192 z8(abcd.m_xAB);
            CZ192 xLHS(abcd.m_xA);
            CZ192 zz8(efgh.m_xAB);
            CZ192 xRHS(efgh.m_xA);

            xLHS.Multiply(z1).Add(z8.Multiply(z5));
            xRHS.Multiply(z1).Add(zz8.Multiply(zz5));
            eResult = xRHS.Compare(xLHS);
        }
    }
    else
    {
        if ((eResult = yLHS.Compare(yRHS)) == C_EQUAL)
        {
            // Compare the X coordinate.
            CZ192 z8(abcd.m_xAB);
            CZ192 xLHS(abcd.m_xA);
            CZ192 zz8(efgh.m_xAB);
            CZ192 xRHS(efgh.m_xA);

            xLHS.Multiply(z1).Add(z8.Multiply(z5));
            xRHS.Multiply(z1).Add(zz8.Multiply(zz5));
            eResult = xLHS.Compare(xRHS);
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::GetTransverseIntersectionYSpan
//
//  Synopsis:
//      Applies to a transverse intersection. Gets the inerval containing the
//      intersection point y coordinate. Set y to {yMin, yMax} where [yMin,
//      yMax] is the interval containing the intersection point y coordinate.
//
//------------------------------------------------------------------------------
void 
CLineSegmentIntersection::GetTransverseIntersectionYSpan(
    __out_ecount_full(2) Integer30 y[2]
    ) const
{
    Assert(IntersectionIsTransverse());

    // The intersection point interval is the intersection of the intervals.

    Integer30 yA = m_yA;
    Integer30 yB = yA + m_yAB;
    Integer30 yC = yA + m_yAC;
    Integer30 yD = yC - m_yDC;
    y[0] = max(min(yA, yB), min(yC, yD));
    y[1] = min(max(yA, yB), max(yC, yD));
    Assert(IsValidInteger30(y[0]) && IsValidInteger30(y[1]));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::GetTransverseIntersectionWhenNotOnOpenSegments
//
//  Synopsis:
//      Applies to a transverse intersection. Gets the coordinate of the
//      intersection point when this point is one of the four input points A, B,
//      C, or D.
//
//  Returns:
//      True iff this intersection is transverse and that the intersection point
//      equals A, B, C, or D and false otherwise.
//
//------------------------------------------------------------------------------
bool
CLineSegmentIntersection::GetTransverseIntersectionWhenNotOnOpenSegments(
    __out_ecount_full(2) Integer30 p[2]
    ) const
{
    bool eResult = false;
    if (IntersectionIsTransverse())
    {
        eResult = true;
        if (m_eLocationAB == LOCATION_AT_FIRST_POINT)
        {
            p[0] = m_xA;
            p[1] = m_yA;
        }
        else if (m_eLocationAB == LOCATION_AT_LAST_POINT)
        {
            p[0] = m_xA + m_xAB;
            p[1] = m_yA + m_yAB;
        }
        else if (m_eLocationCD == LOCATION_AT_FIRST_POINT)
        {
            p[0] = m_xA + m_xAC;
            p[1] = m_yA + m_yAC;
        }
        else if (m_eLocationCD == LOCATION_AT_LAST_POINT)
        {
            p[0] = m_xA + m_xAC - m_xDC;
            p[1] = m_yA + m_yAC - m_yDC;
        }
        else
        {
            eResult = false;
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::YXSortSpecificPosition
//
//  Synopsis:
//      Sorts two transverse intersections when one or both of them are not in
//      general position.
//
//  Returns:
//      The order of the intersection points AB ^ CD and EF ^ GH in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::YXSortSpecificPosition(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // A transverse line segment intersection
    __in_ecount(1) const CLineSegmentIntersection& efgh
        // A transverse line segment intersection
)
{
    // Sort these two transverse intersections when one or both of them are not in general position.

    Assert(abcd.IntersectionIsTransverse() && efgh.IntersectionIsTransverse());
    Assert(!(abcd.IntersectionIsTransverseOnOpenSegments() && 
             efgh.IntersectionIsTransverseOnOpenSegments()));

    COMPARISON eResult = C_UNDEFINED;

    if (abcd.IntersectionIsTransverse() && efgh.IntersectionIsTransverse())
    {
        Integer30 p[2], q[2];
        if (abcd.GetTransverseIntersectionWhenNotOnOpenSegments(p))
        {
            if (efgh.GetTransverseIntersectionWhenNotOnOpenSegments(q))
            {
                eResult = YXComparePoints(p[0], p[1], q[0], q[1]);
            }
            else
            {
                eResult = OppositeComparison(YXSortTransverseIntersectionAndPoint(efgh, p));
            }
        }
        else
        {
            if (efgh.GetTransverseIntersectionWhenNotOnOpenSegments(q))
            {
                eResult = YXSortTransverseIntersectionAndPoint(abcd, q);
            }
            else
            {
                // We shouldn't be here.
                Assert(false);
            }
        }     
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::
//                  YXSortTransverseIntersectionAndPointUsingIntervalArithmetic
//
//  Synopsis:
//      Sorts an intersection point and a point.
//
//  Returns:
//      The order of the intersection point AB ^ CD and of the point E in the y
//      coordinate then x coordinate ordering or C_UNDEFINED.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::YXSortTransverseIntersectionAndPointUsingIntervalArithmetic(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // The line segment intersection AB ^ CD 
    __in_ecount(2) const Integer30 e[2],
        // The (x, y) coordinates for a point
    bool fXComparisonOnly                                    
)
{
    COMPARISON eResult = C_UNDEFINED;
    CIntegralInterval z1(abcd.m_xAB, abcd.m_yAB, abcd.m_xDC, abcd.m_yDC);

    if (z1.GetSign() != SI_ZERO)
    {
        CIntegralInterval z5(abcd.m_xAC, abcd.m_yAC, abcd.m_xDC, abcd.m_yDC);
        if (fXComparisonOnly == false)
        {
            // Compare the Y coordinates.
            CIntegralInterval yRHS(e[1] - abcd.m_yA);
            CIntegralInterval yLHS(abcd.m_yAB);

            yRHS.Multiply(z1);
            yLHS.Multiply(z5);
            eResult = yLHS.Compare(yRHS);
            if (eResult == C_STRICTLYLESSTHAN || eResult == C_STRICTLYGREATERTHAN)
            {
                if (z1.GetSign() == SI_STRICTLY_NEGATIVE)
                {
                    eResult = (eResult == C_STRICTLYLESSTHAN) ? C_STRICTLYGREATERTHAN
                        : C_STRICTLYLESSTHAN;
                }
            }
        }

        if (eResult == C_EQUAL || fXComparisonOnly == true)
        {
            // Compare the X coordinates.
            CIntegralInterval xRHS(e[0] - abcd.m_xA);
            CIntegralInterval xLHS(abcd.m_xAB);

            xRHS.Multiply(z1);
            xLHS.Multiply(z5);
            if (z1.GetSign() == SI_STRICTLY_POSITIVE)
            {
                eResult = xLHS.Compare(xRHS);
            }
            else
            {
                Assert(z1.GetSign() == SI_STRICTLY_NEGATIVE);
                eResult = xRHS.Compare(xLHS);
            }
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::YXSortTransverseIntersectionAndPointUsingExactArithmetic
//
//  Synopsis:
//      Sorts an intersection point and a point.
//
//  Returns:
//      The order of the intersection point AB ^ CD and of the point E in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::YXSortTransverseIntersectionAndPointUsingExactArithmetic(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // The line segment intersection AB ^ CD 
    __in_ecount(2) const Integer30 e[2],
        // The (x, y) coordinates for a point
    bool fXComparisonOnly
)
{
    COMPARISON eResult = C_UNDEFINED;
    CZ192 z1(abcd.m_xAB);
    CZ192 z2(abcd.m_yAB);
    CZ192 z3(abcd.m_xDC);
    CZ192 z4(abcd.m_yDC);
    CZ192 z5(abcd.m_xAC);
    CZ192 z6(abcd.m_yAC);

    // Compute z1 equal to abcd.DeterminantABDC().
    z1.Multiply(z4);
    z2.Multiply(z3);
    z1.Subtract(z2);
    Assert(z1.GetSign() != SI_ZERO);

    // Compute z5 equal to abcd.DeterminantACDC()   
    z5.Multiply(z4);
    z6.Multiply(z3);
    z5.Subtract(z6);

    if (fXComparisonOnly == false)
    {
        // Compare the Y coordinates.
        CZ192 yRHS(e[1] - abcd.m_yA);
        CZ192 yLHS(abcd.m_yAB);
        yRHS.Multiply(z1);
        yLHS.Multiply(z5);

        if ((eResult = yLHS.Compare(yRHS)) != C_EQUAL)
        {
            if (z1.GetSign() != SI_STRICTLY_POSITIVE)
            {
                eResult = (eResult == C_STRICTLYLESSTHAN) ? C_STRICTLYGREATERTHAN 
                    : C_STRICTLYLESSTHAN;
            }
        }
        // else, we must compare the X coordinates.
    }

    if (eResult == C_EQUAL || eResult == C_UNDEFINED)
    {
        CZ192 xRHS(e[0] - abcd.m_xA);
        CZ192 xLHS(abcd.m_xAB);
        xRHS.Multiply(z1);
        xLHS.Multiply(z5);
        if (z1.GetSign() == SI_STRICTLY_POSITIVE)
        {
            eResult = xLHS.Compare(xRHS);
        }
        else
        {
            // We know that z1.GetSign() == SI_STRICTLY_NEGATIVE
            eResult = xRHS.Compare(xLHS);
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::SortTransverseIntersectionsAlongCommonLineSegment
//
//  Synopsis:
//      Sorts two intersection points on the same line segment.
//
//  Returns:
//      The order of the intersection points AB ^ CD and EF ^ GH on the common
//      line segment.
//
//  Note:
//      This is a static member. The common line segment must have the same
//      orientation in both abcd and efgh.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::SortTransverseIntersectionsAlongCommonLineSegment(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line intersection AB ^ CD 
    __in_ecount(1) const CLineSegmentIntersection& efgh,
        // Line intersection EF ^ GH
    PAIRING pairing
        // Specifies the common line segment.
)
{
    Assert(abcd.IntersectionIsTransverse() && efgh.IntersectionIsTransverse());

    // Check that the arguments are correct.
    Assert((pairing == PAIRING_FIRST_FIRST && 
            abcd.m_xAB == efgh.m_xAB && abcd.m_yAB == efgh.m_yAB &&
            abcd.m_xA == efgh.m_xA && abcd.m_yA == efgh.m_yA) ||
           (pairing == PAIRING_FIRST_LAST && 
            abcd.m_xAB == - efgh.m_xDC && abcd.m_yAB == - efgh.m_yDC &&
            abcd.m_xA == efgh.m_xA + efgh.m_xAC && abcd.m_yA == efgh.m_yA + efgh.m_yAC) ||
           (pairing == PAIRING_LAST_FIRST &&
            efgh.m_xAB == - abcd.m_xDC && efgh.m_yAB == - abcd.m_yDC &&
            efgh.m_xA == abcd.m_xA + abcd.m_xAC && efgh.m_yA == abcd.m_yA + abcd.m_yAC) ||
           (pairing == PAIRING_LAST_LAST && 
            abcd.m_xDC == efgh.m_xDC && abcd.m_yDC == efgh.m_yDC &&
            abcd.m_xA + abcd.m_xAC == efgh.m_xA + efgh.m_xAC && 
            abcd.m_yA + abcd.m_yAC == efgh.m_yA + efgh.m_yAC));

#ifdef LSI_AUDITING
    IncrementFpa();
#endif

    COMPARISON eResult = C_UNDEFINED;

    switch (pairing)
    {
    case PAIRING_FIRST_FIRST:
        eResult = LambdaABSortTransverseIntersectionPair(abcd, efgh);
        break;

    case PAIRING_FIRST_LAST:
        {
            CLineSegmentIntersection swapped;
            swapped.SetToSwappedTransverseIntersection(efgh);
            eResult = LambdaABSortTransverseIntersectionPair(abcd, swapped);
        }
        break;

    case PAIRING_LAST_FIRST:
        {
            CLineSegmentIntersection swapped;
            swapped.SetToSwappedTransverseIntersection(efgh);
            eResult = LambdaCDSortTransverseIntersectionPair(abcd, swapped);
        }
        break;

    case PAIRING_LAST_LAST:
        eResult = LambdaCDSortTransverseIntersectionPair(abcd, efgh);
        break;
    }

    Assert(eResult != C_UNDEFINED);
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::LambdaABSortTransverseIntersectionPair
//
//  Synopsis:
//      Sorts two intersection points on the same line segment.
//
//  Returns:
//      The order of the intersection points AB ^ CD and AB ^ EF on the line
//      segment AB oriented from A to B.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::LambdaABSortTransverseIntersectionPair(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line intersection AB ^ CD 
    __in_ecount(1) const CLineSegmentIntersection& abef
        // Line intersection AB ^ EF 
    )
{
    // lambda1 = Determinant(AC, DC) / Determinant(AB, DC)
    // lambda2 = Determinant(AE, FE) / Determinant(AB, FE)
    // Compare lambda1 and lambda2 and return the result.

    Assert(abcd.IntersectionIsTransverse() && abef.IntersectionIsTransverse());

#ifdef LSI_AUDITING
    IncrementFpa();
#endif

    COMPARISON eResult = C_UNDEFINED;

    // Easy cases first.
    if (abcd.LambdaIsZero())
    {
        eResult = abef.LambdaIsZero() ? C_EQUAL : C_STRICTLYLESSTHAN;
    }
    else if (abcd.LambdaIsOne())
    {
        eResult = abef.LambdaIsOne() ? C_EQUAL : C_STRICTLYGREATERTHAN;
    }
    else if (abef.LambdaIsZero())
    {
        eResult = C_STRICTLYGREATERTHAN;
    }
    else if (abef.LambdaIsOne())
    {
        eResult = C_STRICTLYLESSTHAN;
    }
    else if (abcd.DeterminantABDCIsExact() && abcd.DeterminantACDCIsExact() &&
             abef.DeterminantABDCIsExact() && abef.DeterminantACDCIsExact())
    {
        // We know that lambda1 and lambda2 are in the [0, 1] interval
        Assert(abcd.DeterminantABDC() != 0.0 && abef.DeterminantABDC() != 0.0); 

        double d1 = abs(abcd.DeterminantACDC());
        double d2 = abs(abcd.DeterminantABDC());
        double d3 = abs(abef.DeterminantACDC());
        double d4 = abs(abef.DeterminantABDC());
        double d1d4 = d1 * d4;
        double d2d3 = d2 * d3;
        if (d1 < LARGESTINTEGER26 && d2 < LARGESTINTEGER26 &&
            d3 < LARGESTINTEGER26 && d4 < LARGESTINTEGER26)
        {
            eResult = d1d4 > d2d3 ? C_STRICTLYGREATERTHAN : 
                (d1d4 < d2d3 ? C_STRICTLYLESSTHAN : C_EQUAL);
        }
        else
        {
            // Compare the two products.
            if (d1d4 != d2d3)
            {
                eResult =  d1d4 > d2d3 ? C_STRICTLYGREATERTHAN : 
                    (d1d4 < d2d3 ? C_STRICTLYLESSTHAN : C_EQUAL);
            }
            else
            {
                // Use exact integer arithmetic. There is no point in using interval arithmetic.
                CZ128 z1(d1);
                CZ128 z2(d2);
                CZ128 z3(d3);
                CZ128 z4(d4);
                eResult = z1.Multiply(z4).Compare(z3.Multiply(z2));

#ifdef LSI_AUDITING
                DecrementFpa();
                IncrementEa();
#endif
            }
        }
    }
    else
    {
        eResult = LambdaABSortTransverseIntersectionPairUsingIntervalArithmetic(abcd, abef);

#ifdef LSI_AUDITING
        DecrementFpa();
        IncrementIa();
#endif

        if (eResult == C_UNDEFINED)
        {
            eResult = LambdaABSortTransverseIntersectionPairUsingExactArithmetic(abcd, abef);

#ifdef LSI_AUDITING
            DecrementIa();
            IncrementEa();
#endif

        }
    }
    Assert(eResult != C_UNDEFINED);
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::LambdaCDSortTransverseIntersectionPair
//
//  Synopsis:
//      Sorts two intersection points on the same line segment.
//
//  Returns:
//      The order of the intersection points AB ^ CD and EF ^ CD on the line
//      segment CD oriented from C to D.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::LambdaCDSortTransverseIntersectionPair(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line segment intersection AB ^ CD
    __in_ecount(1) const CLineSegmentIntersection& efcd
        // Line segment intersection EF ^ CD
    )
{
    // mu1 = Determinant(AB, AC) / Determinant(AB, DC)
    // mu2 = Determinant(EF, EC) / Determinant(EF, DC)
    // Compare mu1 and mu2 and return the result.

    Assert(abcd.IntersectionIsTransverse() && efcd.IntersectionIsTransverse());

#ifdef LSI_AUDITING
    IncrementFpa();
#endif

    COMPARISON eResult = C_UNDEFINED;

    // Easy cases first.
    if (abcd.MuIsZero())
    {
        eResult = efcd.MuIsZero() ? C_EQUAL : C_STRICTLYLESSTHAN;
    }
    else if (abcd.MuIsOne())
    {
        eResult = efcd.MuIsOne() ? C_EQUAL : C_STRICTLYGREATERTHAN;
    }
    else if (efcd.MuIsZero())
    {
        eResult = C_STRICTLYGREATERTHAN;
    }
    else if (efcd.MuIsOne())
    {
        eResult = C_STRICTLYLESSTHAN;
    }
    else if (abcd.DeterminantABDCIsExact() && abcd.DeterminantABACIsExact() &&
        efcd.DeterminantABDCIsExact() && efcd.DeterminantABACIsExact())
    {
        // We know that mu1 and mu2 are in the [0, 1] interval.
        Assert(abcd.DeterminantABDC() != 0.0 && efcd.DeterminantABDC() != 0.0); 

        double d1 = abs(abcd.DeterminantABAC());
        double d2 = abs(abcd.DeterminantABDC());
        double d3 = abs(efcd.DeterminantABAC());
        double d4 = abs(efcd.DeterminantABDC());
        double d1d4 = d1 * d4;
        double d2d3 = d2 * d3;
        if (d1 < LARGESTINTEGER26 && d2 < LARGESTINTEGER26 &&
            d3 < LARGESTINTEGER26 && d4 < LARGESTINTEGER26)
        {
            eResult = d1d4 > d2d3 ? C_STRICTLYGREATERTHAN : 
                (d1d4 < d2d3 ? C_STRICTLYLESSTHAN : C_EQUAL);
        }
        else
        {
            // Compare the two products.
            if (d1d4 != d2d3)
            {
                eResult = d1d4 > d2d3 ? C_STRICTLYGREATERTHAN : 
                    (d1d4 < d2d3 ? C_STRICTLYLESSTHAN : C_EQUAL);
            }
            else
            {
                // Use exact integer arithmetic. There is no point in using interval arithmetic.
                CZ128 z1(d1);
                CZ128 z2(d2);
                CZ128 z3(d3);
                CZ128 z4(d4);
                eResult = z1.Multiply(z4).Compare(z3.Multiply(z2));

#ifdef LSI_AUDITING
                DecrementFpa();
                IncrementEa();
#endif
            }
        }
    }
    else
    {
        eResult = LambdaCDSortTransverseIntersectionPairUsingIntervalArithmetic(abcd, efcd);

#ifdef LSI_AUDITING
        DecrementFpa();
        IncrementIa();
#endif
        if (eResult == C_UNDEFINED)
        {
            eResult = LambdaCDSortTransverseIntersectionPairUsingExactArithmetic(abcd, efcd);

#ifdef LSI_AUDITING
            DecrementIa();
            IncrementEa();
#endif
        }
    }
    Assert(eResult != C_UNDEFINED);
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::SortCDAlongAB
//
//  Synopsis:
//      Assumes that the intersection between AB and CD exists and is non
//      transverse. Computes the order of C and D along the line AB oriented
//      from A to B.
//
//  Returns:
//      the order of C and D along the line AB oriented from A to B. If the
//      intersection doesn't exist or is transverse, return C_UNDEFINED.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::SortCDAlongAB() const
{
    // Assume that PairwiseIntersect has been called.
    // and that the intersection between AB and CD exists and is non transverse.
    // Return the order of C and D with respect to line AB oriented from A to B.
    // If the intersection doesn't exist or is transverse, return C_UNDEFINED.

    if (m_eKind != KIND_NONTRANSVERSE || m_eSignABDC != 0)
    {
        return C_UNDEFINED;
    }

    // We know AC and DC, AD = AC + CD.
    double xAD = m_xAC - m_xDC;
    double yAD = m_yAC - m_yDC;
 
    // We know that points C and D are on the line AB.
    // m_xAC and m_yAC are the components of the vector AC.
    // xAD and yAD are the components of the vector AD.
    // We know that A and B are different.

    if (m_xAC == xAD && m_yAC == yAD)
    {
        return C_EQUAL;
    }

    Assert(m_xAB != 0 || m_yAB != 0);

    if (m_xAB != 0)
    {
        // Use the X axis projection.

        // If m_xAC == xAD then m_yAC == yAD.
        Assert(m_xAC != xAD);

        if (m_xAB > 0)
        {
            return m_xAC > xAD ? C_STRICTLYGREATERTHAN : C_STRICTLYLESSTHAN;
        }
        else
        {
            return m_xAC > xAD ? C_STRICTLYLESSTHAN : C_STRICTLYGREATERTHAN;
        }
    }
    else
    {
        // Use the Y axis projection.

        // If m_yAC == yAD then m_xAC == xAD.
        Assert(m_yAC != yAD);

        if (m_yAB > 0)
        {
            return m_yAC > yAD ? C_STRICTLYGREATERTHAN : C_STRICTLYLESSTHAN;
        }
        else
        {
            return m_yAC > yAD ? C_STRICTLYLESSTHAN : C_STRICTLYGREATERTHAN;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::
//                      LambdaABSortTransverseIntersectionPairUsingIntervalArithmetic
//
//  Synopsis:
//      Sorts two intersection points using interval arithmetic.
//
//  Returns:
//      The order of the intersection points AB ^ CD and AB ^ EF in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::LambdaABSortTransverseIntersectionPairUsingIntervalArithmetic(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line segment intersection for (AB, CD). 
    __in_ecount(1) const CLineSegmentIntersection& abef
        // Line segment intersection for (AB, EF). 
)
{
    COMPARISON eResult = C_UNDEFINED;

    // z1 equals abcd.DeterminantACDC()
    CIntegralInterval z1(abcd.m_xAC, abcd.m_yAC, abcd.m_xDC, abcd.m_yDC);

    // z2 equals abcd.DeterminantABDC()
    CIntegralInterval z2(abcd.m_xAB, abcd.m_yAB, abcd.m_xDC, abcd.m_yDC);

    // z3 equals abef.DeterminantACDC()
    CIntegralInterval z3(abef.m_xAC, abef.m_yAC, abef.m_xDC, abef.m_yDC);

    // z4 equals abef.DeterminantABDC()
    CIntegralInterval z4(abef.m_xAB, abef.m_yAB, abef.m_xDC, abef.m_yDC);

    if (z2.GetSign() != SI_ZERO && z4.GetSign() != SI_ZERO)
    {
        eResult = z1.Multiply(z4).Compare(z3.Multiply(z2));
        if (z2.GetSign() * z4.GetSign() == SI_STRICTLY_NEGATIVE)
        {
            eResult = OppositeComparison(eResult);
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::LambdaABSortTransverseIntersectionPairUsingExactArithmetic
//
//  Synopsis:
//      Sorts two intersection points using exact arithmetic.
//
//  Returns:
//      The order of the intersection points AB ^ CD and AB ^ EF in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::LambdaABSortTransverseIntersectionPairUsingExactArithmetic(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line segment intersection for (AB, CD). 
    __in_ecount(1) const CLineSegmentIntersection& abef
        // Line segment intersection for (AB, EF). 
)
{
    COMPARISON eResult = C_UNDEFINED;
    CZ192 z1(abcd.m_xAB);
    CZ192 z2(abcd.m_yAB);
    CZ192 z3(abcd.m_xDC);
    CZ192 z4(abcd.m_yDC);
    CZ192 z5(abcd.m_xAC);
    CZ192 z6(abcd.m_yAC);
    CZ192 zz1(abef.m_xAB);
    CZ192 zz2(abef.m_yAB);
    CZ192 zz3(abef.m_xDC);
    CZ192 zz4(abef.m_yDC);
    CZ192 zz5(abef.m_xAC);
    CZ192 zz6(abef.m_yAC);

    // abcd.DeterminantABDC()
    z1.Multiply(z4).Subtract(z2.Multiply(z3));
    if (z1.GetSign() == SI_STRICTLY_NEGATIVE)
    {
        z1.Negate();
    }

    // abcd.DeterminantACDC()  
    z5.Multiply(z4).Subtract(z6.Multiply(z3));
    if (z5.GetSign() == SI_STRICTLY_NEGATIVE)
    {
        z5.Negate();
    }

    // abef.DeterminantABDC()
    zz1.Multiply(zz4).Subtract(zz2.Multiply(zz3));
    if (zz1.GetSign() == SI_STRICTLY_NEGATIVE)
    {
        zz1.Negate();
    }

    // abef.DeterminantACDC()
    zz5.Multiply(zz4).Subtract(zz6.Multiply(zz3));
    if (zz5.GetSign() == SI_STRICTLY_NEGATIVE)
    {
        zz5.Negate();
    }

    if (z1.Compare(zz1) == C_STRICTLYLESSTHAN && z5.Compare(zz5) == C_STRICTLYGREATERTHAN)
    {
        eResult = C_STRICTLYGREATERTHAN;
    }
    else if (z1.Compare(zz1) == C_STRICTLYGREATERTHAN && z5.Compare(zz5) == C_STRICTLYLESSTHAN)
    {
        eResult = C_STRICTLYLESSTHAN;
    }
    else
    {
        eResult = z5.Multiply(zz1).Compare(z1.Multiply(zz5));
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::
//                  LambdaCDSortTransverseIntersectionPairUsingIntervalArithmetic
//
//  Synopsis:
//      Sorts two intersection points using interval arithmetic.
//
//  Returns:
//      The order of the intersection points AB ^ CD and EF ^ CD in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::LambdaCDSortTransverseIntersectionPairUsingIntervalArithmetic(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line segment intersection for (AB, CD). 
    __in_ecount(1) const CLineSegmentIntersection& efcd
        // Line segment intersection for (EF, CD). 
)
{
    // mu1 = Determinant(AB, AC) / Determinant(AB, DC)
    // mu2 = Determinant(EF, EC) / Determinant(EF, DC)
    // Compare mu1 and mu2 and return the result.

    COMPARISON eResult = C_UNDEFINED;

    // z1 equals abcd.DeterminantABAC()
    CIntegralInterval z1(abcd.m_xAB, abcd.m_yAB, abcd.m_xAC, abcd.m_yAC);

    // z2 equals abcd.DeterminantABDC()
    CIntegralInterval z2(abcd.m_xAB, abcd.m_yAB, abcd.m_xDC, abcd.m_yDC);

    // z3 equals efcd.DeterminantABAC()
    CIntegralInterval z3(efcd.m_xAB, efcd.m_yAB, efcd.m_xAC, efcd.m_yAC);

    // z4 equals efcd.DeterminantABDC()
    CIntegralInterval z4(efcd.m_xAB, efcd.m_yAB, efcd.m_xDC, efcd.m_yDC);

    if (z2.GetSign() != SI_ZERO && z4.GetSign() != SI_ZERO)
    {
        eResult = z1.Multiply(z4).Compare(z3.Multiply(z2));
        if (z2.GetSign() * z4.GetSign() == SI_STRICTLY_NEGATIVE)
        {
            eResult = OppositeComparison(eResult);
        }
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::
//                      LambdaABSortTransverseIntersectionPairUsingExactArithmetic
//
//  Synopsis:
//      Sorts two intersection points using exact arithmetic.
//
//  Returns:
//      The order of the intersection points AB ^ CD and EF ^ CD in the y
//      coordinate then x coordinate ordering.
//
//  Note:
//      This is a static member.
//
//------------------------------------------------------------------------------
COMPARISON 
CLineSegmentIntersection::LambdaCDSortTransverseIntersectionPairUsingExactArithmetic(
    __in_ecount(1) const CLineSegmentIntersection& abcd,
        // Line segment intersection for (AB, CD). 
    __in_ecount(1) const CLineSegmentIntersection& efcd
        // Line segment intersection for (EF, CD). 
)
{
    // mu1 = Determinant(AB, AC) / Determinant(AB, DC)
    // mu2 = Determinant(EF, EC) / Determinant(EF, DC)
    // Compare mu1 and mu2 and return the result.

    COMPARISON eResult = C_UNDEFINED;
    CZ192 z1(abcd.m_xAB);
    CZ192 z2(abcd.m_yAB);
    CZ192 z3(abcd.m_xAC);
    CZ192 z4(abcd.m_yAC);
    CZ192 z5(abcd.m_xDC);
    CZ192 z6(abcd.m_yDC);
    CZ192 z7(efcd.m_xAB);
    CZ192 z8(efcd.m_yAB);
    CZ192 z9(efcd.m_xAC);
    CZ192 z10(efcd.m_yAC);
    CZ192 z11(efcd.m_xDC);
    CZ192 z12(efcd.m_yDC);    
    
    // z4 equals abcd.DeterminantABAC()
    z4.Multiply(z1).Subtract(z3.Multiply(z2));   

    // z6 equals abcd.DeterminantABDC()
    z6.Multiply(z1).Subtract(z5.Multiply(z2)); 

    // z10 equals efcd.DeterminantABAC()
    z10.Multiply(z7).Subtract(z9.Multiply(z8)); 

    // z12 equals efcd.DeterminantABDC()
    z12.Multiply(z7).Subtract(z11.Multiply(z8));

    Assert(z6.GetSign() != SI_ZERO && z12.GetSign() != SI_ZERO);

    if (z6.GetSign() * z12.GetSign() == SI_STRICTLY_NEGATIVE)
    {
        eResult = OppositeComparison(z4.Multiply(z12).Compare(z10.Multiply(z6)));
    }
    else
    {
        eResult = z4.Multiply(z12).Compare(z10.Multiply(z6));
    }
    return eResult;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CLineSegmentIntersection::SetToSwappedTransverseIntersection
//
//  Synopsis:
//      Sets this intersection to a copy of the argument where the role of AB
//      and CD have been swapped.
//
//  Note:
//      The argument must be a transverse intersection.
//
//------------------------------------------------------------------------------
void
CLineSegmentIntersection::SetToSwappedTransverseIntersection(
    __in_ecount(1) const CLineSegmentIntersection& other
)
{
    Assert(other.IntersectionIsTransverse());

#if DBG
    // The id is not relevant, we use an unlikely value.  
    m_id = UINT_MAX;
#endif

    m_xAB = - other.m_xDC;
    m_yAB = - other.m_yDC;
    m_xDC = - other.m_xAB;
    m_yDC = - other.m_yAB;
    m_xAC = - other.m_xAC;
    m_yAC = - other.m_yAC;
    m_xA = other.m_xA + other.m_xAC;
    m_yA = other.m_yA + other.m_yAC;

    m_rDeterminantABDC = - other.m_rDeterminantABDC;
    m_rDeterminantACDC = - other.m_rDeterminantABAC;
    m_rDeterminantABAC = - other.m_rDeterminantACDC;
    m_eSignABDC = static_cast<SIGNINDICATOR>(- other.m_eSignABDC);
    m_eSignACDC = static_cast<SIGNINDICATOR>(- other.m_eSignABAC);
    m_eSignABAC = static_cast<SIGNINDICATOR>(- other.m_eSignACDC);
    m_fExactABDC = other.m_fExactABDC;
    m_fExactACDC = other.m_fExactABAC;
    m_fExactABAC = other.m_fExactACDC;

    m_eKind = other.m_eKind;
    m_eLocationAB = other.m_eLocationCD;
    m_eLocationCD = other.m_eLocationAB;

    Assert(IntersectionIsTransverse());
}

#ifdef LSI_AUDITING

int CLineSegmentIntersection::s_counters[AUDIT_LAST];

const char* CLineSegmentIntersection::s_countersNames[AUDIT_LAST] = 
{
    "  CLineSegmentIntersection instances                    ",
    "  PairwiseIntersect calls                               ",
    "  Empty intersections found in PairwiseIntersect        ",
    "  YXSortTransverseIntersectionPair calls                ",
    "  Equal arguments to YXSortTransverseIntersectionPair   ",
    "  YXSortTransverseIntersectionAndPoint calls            ",
    "  LocatePointRelativeToLine calls                       ",
    "  LocateTransverseIntersectionRelativeToLine calls      ",
    "  Floating point arithmetic concludes                   ",
    "  Interval arithmetic concludes                         ",       
    "  Exact integer arithmetic concludes                    ",
    "  ParameterAlongAB calls                                ",
    "  ParameterAlongCD calls                                " 
};

void 
CLineSegmentIntersection::ResetAuditCounters()
{
    for (int i = 0; i < AUDIT_LAST; i++)
    {
        s_counters[i] = 0;
    }
}

void 
CLineSegmentIntersection::ReportAuditCounters(
    __in PCSTR reportTitle,
    __in PCSTR reportPrefix,
    __in PCSTR outputFileName
    )
{
    ReportAudit(reportTitle, reportPrefix, outputFileName, s_countersNames, s_counters)
}

#endif // LSI_AUDITING

