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
//      Robust line segment intersection computation.
//
//  $ENDTAG
//
//  Classes:
//      CLineSegmentIntersection
//
//------------------------------------------------------------------------------

// Define LSI_AUDITING to enable line segment intersection monitoring.
// If LSI_AUDITING is defined, we monitor the calls to this module and report the results
// in a text file, see ReportAuditCounters below.
// #define LSI_AUDITING

namespace RobustIntersections
{
    
    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CLineSegmentIntersection
    //
    //  Synopsis:
    //      A CLineSegmentIntersection models the intersection between two line
    //      segments.
    //
    //  Notes:
    //      Each instance of this class holds the data relative to the
    //      intersection between two line segments.  The PairwiseIntersect
    //      method computes the actual intersection whose description can be
    //      retrieved using the query methods. Static methods let the client
    //      sort intersection points along a line segment and line segments
    //      incident to the same intersection point. Other static methods let
    //      the client sort points on a line, this is useful when the input line
    //      segments are part of the same line.
    //
    //      A, B, C, D, E, F, G, H are input points with integer coordinates in
    //      the interval R = [-2^30, +2^30].  Coordinates are represented by
    //      double precision floating point numbers.
    //
    //      AB is the closed line segment from A to B and ABOpen is the open line
    //      segment from A to B.
    //      Each input line segment MUST have distinct endpoints.
    //      The intersection between AB and CD is transverse if it exists 
    //      and the lines AB and CD are not parallel. 
    //      Note that a non-transverse intersection can be reduced to a single
    //      point which has to be equal to one of the endpoints.
    //
    //--------------------------------------------------------------------------
    class CLineSegmentIntersection
    {
    public:

        // Describes the nature of the intersection between the line segments AB and CD. 
        enum KIND
        {
            KIND_EMPTY = 0,                // The intersection is empty.
            KIND_NONTRANSVERSE = 1,        // AB and CD are parallel and AB intersects CD.
            KIND_TRANSVERSE = 2,           // AB and CD are not parallel and AB intersects CD.
            KIND_UNDEFINED = 3             // The intersection is undefined, either because the 
                                           // input points were invalid or the computation failed.
        };

        // Describes the various line segment pairs association flavors.
        // Do not modify the enumeration member values.
        enum PAIRING
        {
            PAIRING_FIRST_FIRST = 0,        // The first line segment of both pairs. 
            PAIRING_FIRST_LAST = 1,         // First on the first pair and last on the second pair.
            PAIRING_LAST_FIRST = 2,         // Last on the first pair and first on the second pair.
            PAIRING_LAST_LAST = 3           // The last line segment on both pairs.
        };

        // Describes the location of the intersection point I with respect to the line segment
        // AB (resp. CD) when the intersection exists and is transverse.
        enum LOCATION
        {
            LOCATION_AT_FIRST_POINT = 0,      // I equals A (resp. C)  
            LOCATION_ON_OPEN_SEGMENT = 1,     // I is in the open line segment AB (resp. CD)     
            LOCATION_AT_LAST_POINT = 2,       // I equals B (resp. D)  
            LOCATION_UNDEFINED = 3            // Either the intersection is empty, 
                                              // or not transverse, or the input points were  
                                              // invalid, or the computation failed.
        };

        // Describes the position of a point relative to an oriented line.
        // This definition assumes a right-handed coordinate system. Client
        // applications should swap rigt and left if they use a left-handed coordinate system.
        // Do not modify the enumeration member values.
        enum SIDEINDICATOR
        {
            SIDE_RIGHT = -1,   // The point lies in the open half-plane right of the oriented line.
            SIDE_INCIDENT = 0, // The point is incident to the oriented line.
            SIDE_LEFT = 1      // The point lies in the open half-plane left of the oriented line.
        };

        // Describes where a point M lies relative to an oriented line segment defined by 
        // its endpoints First and Last. We assume that First and Last are distinct. 
        // Do not modify the enumeration member values.
        enum REGION
        {
            REGION_HALFLINE_BEFOREFIRST = 0, // M is incident to the line and located before First.
            REGION_EQUAL_TO_FIRST = 1,       // M equals First.        
            REGION_OPEN_LINESEGMENT = 2,     // M is inside the open line segment (First, Last).        
            REGION_EQUAL_TO_LAST = 3,        // M equals Last.        
            REGION_HALFLINE_AFTERLAST = 4,   // M is incident to the line and located after Last.        
            REGION_LEFT_OPENHALFPLANE = 5,   // M is not incident to the line and lies          
                                             // in the open half plane left of (First, Last).        
            REGION_RIGHT_OPENHALFPLANE = 6,  // M is not incident to the line and lies         
                                             // in the open half plane right of (First, Last).        
            REGION_UNDEFINED                 // The region is undefined.        
        };

        // Constructor.
        CLineSegmentIntersection();

        // Copy constructor and assignment operator.
        // The compiler generated ones do the right thing.

        // Non virtual destructor.
        ~CLineSegmentIntersection();

        // Initialize this instance. Memory allocators bypassing the constructor must call 
        // this method.
        void Initialize();

        // Computes the intersection between the line segments AB and CD, 
        // A must be distinct from B, C must be distinct from D.
        // Coordinates are in order xA, yA, xB, yB, same thing for CD.
        // Returns the kind of the intersection.
        // Note that the last two arguments are set to LOCATION_UNDEFINED when the
        // intersection is not transverse
        KIND PairwiseIntersect(
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
        );

        // Queries.

        // Whether this intersection is equal to the argument.
        // PairwiseIntersect must have been called on both intersections.
        bool IsEqual(__in_ecount(1) const CLineSegmentIntersection& efgh) const;
        
        // Get the kind of the last computed intersection.
        KIND GetKind() const {return m_eKind;}

        // Get the location wrt AB of the last computed intersection.
        LOCATION GetLocationAB() const {return m_eLocationAB;}

        // Get the location wrt CD of the last computed intersection.
        LOCATION GetLocationCD() const {return m_eLocationCD;}

        // Whether the last computed intersection is not the empty set.
        bool IntersectionIsNotEmpty() const 
        {
            return m_eKind == KIND_NONTRANSVERSE || m_eKind == KIND_TRANSVERSE;
        }

        // Whether the last computed intersection is transverse.
        // When true, it implies that the intersection is not empty.
        bool IntersectionIsTransverse() const
        {
            return m_eKind == KIND_TRANSVERSE;
        }

        // Whether the last computed intersection is non transverse.
        // When true, it implies that the intersection is not empty.
        bool IntersectionIsNonTransverse() const 
        {
            return m_eKind == KIND_NONTRANSVERSE;
        }

        // Whether the last computed intersection is non transverse and
        // the intersection point is in AB open and in CD open.
        bool IntersectionIsTransverseOnOpenSegments() const
        {
            return m_eKind == KIND_TRANSVERSE && 
                m_eLocationAB == LOCATION_ON_OPEN_SEGMENT &&
                m_eLocationCD == LOCATION_ON_OPEN_SEGMENT;
        }

        // When the intersection between AB and CD exists and is transverse, 
        // the intersection point I verifies
        // AI = lambda * AB, with 0 <= lambda <= 1. 
        // Return an approximation of lambda if the conditions above are true, otherwise return -1.
        double ParameterAlongAB() const;

        // When the intersection between AB and CD exists and is transverse, 
        // the intersection point I verifies
        // CI = lambda * CD, with 0 <= lambda <= 1. 
        // Return an approximation of lambda if the conditions above are true, otherwise return -1.
        double ParameterAlongCD() const;

        // Assume that PairwiseIntersect has been called and that the intersection between 
        // AB and CD exists and is transverse on the open segments AB and CD. 
        // Let I be the intersection point.
        // Return SIDE_LEFT when I lies in the open half plane left 
        // of the line EF, oriented from E to F.
        // Return SIDE_INCIDENT when I is incident to the line EF.
        // Return SIDE_RIGHT when I lies in the open half plane 
        // right of the line EF, oriented from E to F.
        // This specification assumes a right-handed coordinate system.
        SIDEINDICATOR LocateTransverseIntersectionRelativeToLine(__in_ecount(4) const Integer30 ef[4]) const;

        // Assume that PairwiseIntersect has been called
        // and that the intersection between AB and CD exists and is transverse.
        // Compute the y coordinate interval containing the intersection point.
        // Set y to {yMin, yMax} where [yMin, yMax] is the interval
        // containing the intersection point y coordinate. 
        void GetTransverseIntersectionYSpan(__out_ecount_full(2) Integer30 y[2]) const;

        // Let AB, CD, EF, and GH be four line segments such that each pair (AB, CD) and (EF, GH)
        // has a transverse intersection. Assume that the method PairwiseIntersect(AB, CD)
        // (resp. PairwiseIntersect(EF, GH)) has been called on the first (resp. second) argument.
        // Return the order of the intersection points AB ^ CD and EF ^ GH 
        // in the y coordinate then x coordinate ordering.
        static COMPARISON YXSortTransverseIntersectionPair(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& efgh
            );

        // Let AB and CD be two line segments such that the pair (AB, CD) 
        // has a transverse intersection and let E be an input point.
        // Assume that the method PairwiseIntersect(AB, CD) has been called on the first argument.
        // Compare the intersection point AB ^ CD and E in the y coordinate 
        // then x coordinate ordering and retun the result.
        static COMPARISON YXSortTransverseIntersectionAndPoint(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(2) const Integer30 e[2]
            );

        // Assume that A and B are distinct.
        // Return SIDE_LEFT when C lies in the open half plane left of the line AB,
        // oriented from A to B.
        // Return SIDE_INCIDENT when C is incident to the line AB.
        // Return SIDE_RIGHT when C lies in the open half plane right of the line AB,
        // oriented from A to B.
        // This specification assumes a right-handed coordinate system.
        static SIDEINDICATOR LocatePointRelativeToLine(
            __in_ecount(2) const Integer30 c[2], 
            __in_ecount(4) const Integer30 ab[4]
            );

        // Assume that PairwiseIntersect has been called
        // and that the intersection between AB and CD exists and is non transverse.
        // Return the order of C and D with respect to line AB orientation.
        // If the intersection doesn't exist or is transverse, return C_UNDEFINED.
        COMPARISON SortCDAlongAB() const;

        // Let (AB, CD) and (EF, GH) be two transverse line segment intersections. Assume that
        // according to the third argument, these line segment pairs share a common line segment.
        // Assume that the method PairwiseIntersect(AB, CD) (resp. PairwiseIntersect(EF, GH)) 
        // has been called on the first (resp. second) argument.
        // Return the order of the intersection points AB ^ CD and EF ^ GH 
        // along the common line segment.
        // Note that the common line segment must have the same orientation in both abcd and efgh.
        static COMPARISON SortTransverseIntersectionsAlongCommonLineSegment(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& efgh,
            PAIRING ePairing 
            );

        // Let AB, CD, and EF be three line segments such that each pair (AB, CD) and (AB, EF)
        // has a transverse intersection. Assume that the method PairwiseIntersect(AB, CD)
        // (resp. PairwiseIntersect(AB, EF)) has been called on the first (resp. second) argument.
        // Return the order of the intersection points AB ^ CD and AB ^ EF along AB.
        static COMPARISON LambdaABSortTransverseIntersectionPair(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& abef
            );

        // Let AB, CD, and EF be three line segments such that each pair (AB, CD) and (EF, CD)
        // has a transverse intersection. Assume that the method PairwiseIntersect(AB, CD)
        // (resp. PairwiseIntersect(EF, CD)) has been called on the first (resp. second) argument.
        // Return the order of the intersection points AB ^ CD and EF ^ CD along CD.
        static COMPARISON LambdaCDSortTransverseIntersectionPair(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& efcd
            );

    private:

        // Member functions.

        bool LambdaIsZero() const
        {
            Assert(IntersectionIsTransverse());
            return m_eLocationAB == LOCATION_AT_FIRST_POINT;
        }
        bool LambdaIsOne() const
        {
            Assert(IntersectionIsTransverse());
            return m_eLocationAB == LOCATION_AT_LAST_POINT;
        }
        bool MuIsZero() const
        {
            Assert(IntersectionIsTransverse());
            return m_eLocationCD == LOCATION_AT_FIRST_POINT;
        }
        bool MuIsOne() const
        {
            Assert(IntersectionIsTransverse());
            return m_eLocationCD == LOCATION_AT_LAST_POINT;
        }

        bool DeterminantABDCIsExact() const {return m_fExactABDC;}
        bool DeterminantACDCIsExact() const {return m_fExactACDC;}
        bool DeterminantABACIsExact() const {return m_fExactABAC;}       

        double DeterminantABDC() const {return m_rDeterminantABDC;}
        double DeterminantACDC() const {return m_rDeterminantACDC;}
        double DeterminantABAC() const {return m_rDeterminantABAC;}

        SIGNINDICATOR SignDeterminantABDC() const {return m_eSignABDC;}
        SIGNINDICATOR SignDeterminantACDC() const {return m_eSignACDC;}
        SIGNINDICATOR SignDeterminantABAC() const {return m_eSignABAC;}

        void Reset();

        COMPARISON CompareDeterminantABDCandDeterminantACDC();

        COMPARISON CompareDeterminantABDCandDeterminantABAC();

        void ComputeIntersectionPointXCoordinateInterval(
            __out_ecount(1) double& rMin, 
            __out_ecount(1) double& rMax
            ) const;

        void ComputeIntersectionPointYCoordinateInterval(
            __out_ecount(1) double& rMin, 
            __out_ecount(1) double& rMax
            ) const;

        void SetToSwappedTransverseIntersection(
            __in_ecount(1) const CLineSegmentIntersection& other
            );

        bool GetTransverseIntersectionWhenNotOnOpenSegments(
            __out_ecount_full(2) Integer30 p[2]
            ) const;

        static REGION ComputeRegionWhenPointPIsOnAB(
            double xAB, 
            double yAB, 
            double xAP, 
            double yAP
            );

        static COMPARISON YXSortTransverseIntersectionPairUsingIntervalArithmetic(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& efgh
            );

        static COMPARISON YXSortTransverseIntersectionPairUsingExactArithmetic(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& efgh
            );

        static COMPARISON YXSortSpecificPosition(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& efgh
            );

        static COMPARISON YXSortTransverseIntersectionAndPointUsingIntervalArithmetic(
            __in_ecount(1) const CLineSegmentIntersection& abcd,                    
            __in_ecount(2) const Integer30 e[2],
            bool fXComparisonOnly
            );

        static COMPARISON YXSortTransverseIntersectionAndPointUsingExactArithmetic(
            __in_ecount(1) const CLineSegmentIntersection& abcd,                    
            __in_ecount(2) const Integer30 e[2],
            bool fXComparisonOnly
            );

        static COMPARISON LambdaABSortTransverseIntersectionPairUsingIntervalArithmetic(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& abef
            );

        static COMPARISON LambdaABSortTransverseIntersectionPairUsingExactArithmetic(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& abef
            );

        static COMPARISON LambdaCDSortTransverseIntersectionPairUsingIntervalArithmetic(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& efcd
            );

        static COMPARISON LambdaCDSortTransverseIntersectionPairUsingExactArithmetic(
            __in_ecount(1) const CLineSegmentIntersection& abcd, 
            __in_ecount(1) const CLineSegmentIntersection& efcd
            );

#if DBG
        bool KindAndLocationsAreValid() const
        {
            return m_eKind != KIND_UNDEFINED && 
                ((m_eKind == KIND_TRANSVERSE && 
                m_eLocationAB != LOCATION_UNDEFINED && m_eLocationCD != LOCATION_UNDEFINED) ||
                (m_eKind != KIND_TRANSVERSE && 
                m_eLocationAB == LOCATION_UNDEFINED && m_eLocationCD == LOCATION_UNDEFINED));
        }
#endif

        // Data members.

        // The components of the 3 vectors AB, DC, and AC. 
        // We know that they are exactly represented.
        // We know that A is not equal to B and that C is not equal to D.
        double m_xAB, m_yAB, m_xDC, m_yDC, m_xAC, m_yAC;

        // The coordinates of A. We need them for 2 of the comparison functions.
        double m_xA, m_yA;

        // Exact or approximate value of the determinants ABDC, ACDC, and ABAC
        double m_rDeterminantABDC, m_rDeterminantACDC, m_rDeterminantABAC;

        // The intersection kind.
        KIND m_eKind;

        // The next two members are valid iff the intersection is transverse.
        // The location of the intersection point, if any, wrt AB and CD
        LOCATION m_eLocationAB, m_eLocationCD;

        // Sign of the determinants, must be exact.
        SIGNINDICATOR m_eSignABDC, m_eSignACDC, m_eSignABAC;

        // Whether or not the associated determinant value is exact.
        bool m_fExactABDC, m_fExactACDC, m_fExactABAC;

#ifdef LSI_AUDITING

        // Audit counters name.
        enum AUDIT
        {
            AUDIT_INSTANCECOUNT = 0,
            AUDIT_PAIRWISEINTERSECT = 1,
            AUDIT_PAIRWISEINTERSECT_NOINTERSECTION = 2,
            AUDIT_YXSORTTRANSVERSEINTERSECTIONPAIR = 3,
            AUDIT_YXSORTTRANSVERSEINTERSECTIONPAIR_EQUALARGUMENTS = 4,
            AUDIT_YXSORTTRANSVERSEINTERSECTIONANDPOINT = 5,
            AUDIT_LOCATEPOINTRELATIVETOLINE = 6,
            AUDIT_LOCATETRANSVERSEINTERSECTIONRELATIVETOLINE = 7,
            AUDIT_FLOATINGPOINT_CONCLUDES = 8,
            AUDIT_INTERVAL_CONCLUDES = 9,
            AUDIT_EXACT_CONCLUDES = 10,
            AUDIT_PARAMETERALONGAB = 11,
            AUDIT_PARAMETERALONGCD = 12,
            AUDIT_LAST
        };

        // Auditing counters values and names.
        static int s_counters[AUDIT_LAST];
        static const char* s_countersNames[AUDIT_LAST];

        // Methods.
    public:

        static void ResetAuditCounters();

        // Where to report the audit.
        static void ReportAuditCounters(
            __in PCSTR reportTitle = " no title.",
            __in PCSTR reportPrefix = "Line Segment Intersection Audit: ",
            __in PCSTR outputFileName = ".\\LineSegmentIntersectionAuditReport.txt"             
        );

        static void IncrementFpa()
        {
            s_counters[AUDIT_FLOATINGPOINT_CONCLUDES]++; 
        }

        static void DecrementFpa()
        {
            s_counters[AUDIT_FLOATINGPOINT_CONCLUDES]--; 
        }

        static void IncrementIa()
        {
            s_counters[AUDIT_INTERVAL_CONCLUDES]++; 
        }

        static void DecrementIa()
        {
            s_counters[AUDIT_INTERVAL_CONCLUDES]--; 
        }

        static void IncrementEa()
        {
            s_counters[AUDIT_EXACT_CONCLUDES]++; 
        }

    private:

        static void IncrementTotalInstanceCount()
        {
            s_counters[AUDIT_INSTANCECOUNT]++;
        }

        static void IncrementPairwiseIntersectCount()
        {
            s_counters[AUDIT_PAIRWISEINTERSECT]++;
        }

        static void IncrementPairwiseIntersectNoIntersection()
        {
            s_counters[AUDIT_PAIRWISEINTERSECT_NOINTERSECTION]++;
        }

        static void IncrementParameterAlongAB()
        {
            s_counters[AUDIT_PARAMETERALONGAB]++;
        }

        static void IncrementParameterAlongCD()
        {
            s_counters[AUDIT_PARAMETERALONGCD]++;
        }

        static void IncrementLocateTransverseIntersectionRelativeToLine()
        {
            s_counters[AUDIT_LOCATETRANSVERSEINTERSECTIONRELATIVETOLINE]++;
        }

        static void IncrementYXSortTransverseIntersectionPair()
        {
            s_counters[AUDIT_YXSORTTRANSVERSEINTERSECTIONPAIR]++;
        }

        static void IncrementEqualArgumentsToYXSortTransverseIntersectionPair()
        {
            s_counters[AUDIT_YXSORTTRANSVERSEINTERSECTIONPAIR_EQUALARGUMENTS]++;
        }

        static void IncrementYXSortTransverseIntersectionAndPoint()
        {
            s_counters[AUDIT_YXSORTTRANSVERSEINTERSECTIONANDPOINT]++;
        }

        static void IncrementLocatePointRelativeToLine()
        {
            s_counters[AUDIT_LOCATEPOINTRELATIVETOLINE]++;
        }
#endif

#if DBG
    public:

        // Identifier used for debugging purposes.
        UINT m_id;
#endif
    };

}; // namespace RobustIntersections

