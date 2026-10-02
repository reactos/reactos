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
//      Constructing new shapes with Boolean operations
//
//  $ENDTAG
//
//  Classes:
//      COutline, CBoolean, CBooleanClassifier
//
//  A note about curve retrieval:
//      The core scanner architecture can currently only handle polygonal
//      geometries -- when passed a geometry containing curves, we must flatten
//      the geometry as a pre-process step. This, unfortunately, makes all
//      scanner operations resolution-dependent. To help mitigate this, we offer
//      Bezier reconstruction, which attempts to reconstruct the Beziers from
//      the scanner output. We do this by tagging segments resulting from
//      flattening with:
//
//          1) The Bezier from which they came
//          2) The start and end parameter values (between 0 and 1) for that segment.
//
//      This information is preserved during the scanner operation. After the
//      operation is complete, we search the output for tags and reconstruct the
//      Beziers.
//
//      See BezierReconstruction.docx in this directory for details.
//
//------------------------------------------------------------------------------

#if DBG
    void DumpRelation(MilPathsRelation::Enum eResult);
#endif

class COutline  :   public CScanner
{
public:
                                    // Helper classes

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CPreFigure
    //
    //  Synopsis:
    //      A list of chains that will eventually form a figure
    //
    //--------------------------------------------------------------------------
    class CPreFigure
    {
        public:

        // The class is only instantiated by the memory pool, so no constructor/destructor necessary

        // Methods
        void Initialize(
            __inout_ecount(1) CChain *pFirst,
                // The starting chain
            __inout_ecount(1) CChain *pLast);
                // The ending chain

        void Assume(
            __inout_ecount(1) CPreFigure *pOther);  // The figure to assume

        HRESULT AddToShape(
            __inout_ecount(1) COutline &refOutline);
                // The outline generator

        void AssumeAsFirst(__inout_ecount(1) CChain *pChain)
        {
            pChain->SetTaskData(this);
            m_pFirst = pChain;
        }

        void AssumeAsLast(__inout_ecount(1) CChain *pChain)
        {
            pChain->SetTaskData(this);
            m_pLast = pChain;
        }

        protected:

        void LinkChainTo(
            __inout_ecount(1) CChain      *pChain,
                // The chain to be linked.
            __in_ecount(1) CChain      *pNextChain)
                // The chain to link to.
        {
            Assert(pChain);
            pChain->SetTaskData2(pNextChain);
        }

        CChain *GetNextChain(
            __in_ecount(1) CChain  *pChain)
                // A chain
        {
            Assert(pChain);
            return static_cast<CChain *>(pChain->GetTaskData2());
        }

        // Data
        protected:
            CChain         *m_pFirst;      // The first chain
            CChain         *m_pLast;       // The last chain
            CPreFigure     *m_pPrevious;   // Previous prefigure in the list
            CPreFigure     *m_pNext;       // Next prefigure in the list
    };  // End of definition of CPreFigure

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CPreFigurePool
    //
    //  Synopsis:
    //      A memory pool for pre-figures
    //
    //  Notes:
    //
    //--------------------------------------------------------------------------
    class CPreFigurePool   :   public TMemBlockBase<CPreFigure>
    {
    public:
        CPreFigurePool()
        {
        }

        virtual ~CPreFigurePool()
        {
        }

        __ecount_opt(1) CPreFigure * AllocatePreFigure(
            __inout_ecount(1) CChain *pFirst,
                // The starting chain
            __inout_ecount(1) CChain *pLast);
                // The ending chain

    };  // End of definition of CPreFigurePool


    //----------------------------------------------------------------------------------------------
    //
    //      COutline methods and data
    //
    //----------------------------------------------------------------------------------------------

    public:

    // Constructor/destructor
    COutline(
        __inout_ecount_opt(1) IShapeBuilder *pResult,
            // The recepient of the resulting shape (NULL okay)
        __in bool fRetrieveCurves=true,
            // Retrieve curves if true
        __in double rTolerance=0);
            // Curve retrieval error tolerance

    virtual ~COutline()
    {
    }

    // CScanner override
    virtual HRESULT ProcessTheJunction();

    virtual HRESULT ProcessCurrentVertex(
        __inout_ecount(1) CChain *)
    {
        // Do nothing stub
        return S_OK;
    }

protected:
    //
    // Figure construction function pointers. They will be set by constructor
    // according to working mode - with or without curve-retrieval
    //
    typedef HRESULT (COutline::*PFNAddVertex)(__in_ecount(1) const CVertex *pVertex);
    typedef HRESULT (COutline::*PFNCloseFigure)();

    PFNAddVertex m_pfnAddVertex;
    PFNCloseFigure m_pfnCloseFigure;

    // Other methods
    __out_ecount(1) CPreFigure *GetOwnerOf(__in_ecount(1) CChain *pChain)
    {
        return static_cast<CPreFigure *>(pChain->GetTaskData());
    }

    HRESULT StartPreFigure(
        __inout_ecount(1) CChain *pFirst,
            // The first chain
        __inout_ecount(1) CChain *pLast);
        // The last (=second) chain

    HRESULT AppendPairs(
        __inout_ecount_opt(1) CChain *pLeftHead,
            // The leftmost head
        __inout_ecount_opt(1) CChain *pRightHead,
            // The rightmost head
        __inout_ecount_opt(1) CChain *pLeftTail,
            // The leftmost tail
        __inout_ecount_opt(1) CChain *pRightTail);
            // The rightmost tail

    HRESULT AppendHeadPairs(
        __inout_ecount(1) CChain *pLeftmost,
            // The leftmost head
        __inout_ecount(1) const CChain *pRightmost,
            // The rightmost head
        __out_ecount(1) bool  &fOddCount);
            // True if the number of heads is odd

    HRESULT AppendTailPairs(
        __inout_ecount(1) CChain *pLeftmost,
            // The leftmost tail
        __inout_ecount(1) const CChain *pRightmost,
            // The rightmost head
        __out_ecount(1) bool  &fOddCount);
            // True if the number of heads is odd

    HRESULT AppendTails(
        __inout_ecount(1) CChain *pLeader,
            // The leading chain
        __inout_ecount(1) CChain *pTrailer);
            // The trailing chain

    void Append(
        __inout_ecount(1) CChain *pHead,
            // The head chain
        __inout_ecount(1) CChain *pTail,
            // The tail chain
        IN bool   fReverse);
            // Append in reverse order if true

    HRESULT ResetLeft(
        __inout_ecount(1) CChain *&pLeftmost,
            // The leftmost head/tail, modified here
        __inout_ecount(1) CChain *&pRightmost);
            // The rightmost head/tail, modified here

    HRESULT ResetBoth(
        __inout_ecount(1) CChain *&pLeftmost,
            // The leftmost head/tail, modified here
        __inout_ecount(1) CChain *&pRightmost);
            // The rightmost head/tail, modified here

    void operator = (__in_ecount(1) const COutline &other) {other;}

    // Figure construction

    HRESULT StartFigure(__in_ecount(1) const CChain *pChain);

    HRESULT AddOutlineVertex(__in_ecount(1) const CVertex *pVertex)
    {
        // Calling the method-pointer set upon construction
        return (this->*m_pfnAddVertex)(pVertex);
    }

    HRESULT CloseFigure()
    {
        // Calling the method-pointer set upon construction
        return (this->*m_pfnCloseFigure)();
    }

    HRESULT AddCurveFragment(
        __in_ecount(1) const CBezierFragment *pFragment,
           // The fragment being added.
        __in_ecount(1) const CVertex *pVertex
           // The vertex that terminates that fragment.
        );

    HRESULT FlushCurve();

    HRESULT AddOutlineFigure();

    HRESULT AddChainToFigure(__in_ecount(1) CChain *pChain);

    HRESULT AddVertexSimple(__in_ecount(1) const CVertex *pVertex);

    HRESULT AddVertexWithCurves(__in_ecount(1) const CVertex *pVertex);

    HRESULT CloseFigureSimple();

    HRESULT CloseFigureWithCurves();

    HRESULT AddCurve(
        __in_ecount(1) const GpPointR &controlPoint1,
        __in_ecount(1) const GpPointR &controlPoint2,
        __in_ecount(1) const CVertex *pVertex
        );

    void LinkChainTo(
        __in_ecount(1) CChain      *pChain,
            // The chain to be linked.
        __in_ecount(1) CChain      *pNextChain)
            // The chain to link to.
    {
        Assert(pChain);
        pChain->SetTaskData2(pNextChain);
    }

    __out_ecount(1) CChain *GetNextChain(
        __in_ecount(1) CChain  *pChain)
            // A chain
    {
        Assert(pChain);
        return static_cast<CChain *>(pChain->GetTaskData2());
    }

    // Data
    CPreFigurePool m_oMem;
        // Memory pool for prefigures
    IShapeBuilder *m_pShape;
        // The recepient outline shape
    IFigureBuilder *m_pCurrentFigure;
        // The current figure added to m_pShape

    // Bezier reconstruction data
    bool m_segmentReversed;
        // = true if the current output segment is reversed relative to to its original.
    bool m_curveReversed;
        // = true if the current curve is reversed relative to to its original.
    bool m_downwardTraversal;
        // = true if we are traversing the current segment from the top down.

    CBezierFragment m_curve;
        // The Bezier being reconstructed.
    const CVertex *m_pCurrentCurveVertex;
        // The last vertex on the curve we have proccessed.
};
//--------------------------------------------------------------------------------------------------

class CBoolean  :   public COutline
{
public:
    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CBooleanClassifier
    //
    //  Synopsis:
    //      Classifies chains as left/right/redundant
    //
    //--------------------------------------------------------------------------

    class CBooleanClassifier    :   public CClassifier
    {
    private:
        CBooleanClassifier()
        {
        }

    public:
        CBooleanClassifier(
            IN MilCombineMode::Enum m_eOperation);   // The Boolean operation

        virtual ~CBooleanClassifier()
        {
        }

        virtual void  Classify(
            __inout_ecount(1) CChain *pLeftmostTail,
                // The junction's leftmost tail
            __inout_ecount(1) CChain *pLeftmostHead,
                // The junction's leftmost head
            __inout_ecount_opt(1) CChain *pLeft);
                // The chain to the left of the junction (possibly NULL)

        void ClassifyChain(
            __inout_ecount(1) CChain *pChain);
                // The chain being classified

    // Data
    protected:
        MilCombineMode::Enum    m_eOperation;   // The type of operation we're performing
        CChain              *m_pTail[2];     // The 2 shapes' leftmost tail chains
        CChain              *m_pLeft[2];     // The 2 shapes' left-of-the-junction chains
        bool                m_fIsInside[2];  // State: where we are relative to the 2 shapes
    };

    // CBoolean methods & data
    CBoolean(
        __inout_ecount_opt(1) IShapeBuilder *pResult,
            // The recepient of the result of the oeration (NULL okay)
        __in MilCombineMode::Enum  eOperation,
            // The Boolean operation
        __in bool fRetrieveCurves=true,
            // Retrieve curves if true
        __in double rTolerance=0)
            // Curve retrieval error tolerance
        :COutline(pResult, fRetrieveCurves, rTolerance),
         m_oBoolClassifier(eOperation)
    {
        m_oJunction.SetClassifier(&m_oBoolClassifier);
    }

    virtual ~CBoolean()
    {
    }

    HRESULT SetNext()
    {
        return m_oChains.SetNext();
    }
#if DBG
    virtual bool IsBooleanOperation() const
    {
        return true;
    }
#endif

protected:
    // Data
    CBooleanClassifier   m_oBoolClassifier;   // A Boolean left/right/redundant classifier
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CRelation
//
//  Synopsis:
//      Classifies 2 shapes as intersecting/overlapping/disjoint
//
//------------------------------------------------------------------------------
class CRelation     :   public CBoolean
{
public:
    CRelation(double rTolerance);

    virtual ~CRelation()
    {
    }

    // CScanner override
    virtual HRESULT ProcessTheJunction();

    MilPathsRelation::Enum GetResult();

#if DBG
    void Dump();
#endif

protected:
    // Data
    bool                 m_fInside[2];        // some edges of shape[i] are inside the other
    bool                 m_fOutside[2];       // some edges of shape[i] are outside the other
    MilPathsRelation::Enum     m_eResult;           // The result
};


