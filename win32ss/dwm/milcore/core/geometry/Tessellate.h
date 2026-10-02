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
//      Tessellate a shape
//
//  $ENDTAG
//
//  Classes:
//      CTessellator, CVertexRef, CVertexRefPool
//
//------------------------------------------------------------------------------
#if DBG
    extern bool g_fTesselatorTrace;
#endif

#ifdef SCAN_TESTING
    #define VALIDATE_BANDS ValidateBands()
#else
    #define VALIDATE_BANDS
#endif

//+-----------------------------------------------------------------------------
//
//  Class:
//      CTessellator
//
//  Synopsis:
//      Tessellates the fill bands defined by a list of chains
//
//  Notes:
//
//------------------------------------------------------------------------------

#pragma warning( push )
#pragma warning( disable : 4512 )   // Disable automatic copy constructor

class CTessellator :   public CScanner
{

protected:

    // Helper classes
    
    class CVertexRefPool;

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CVertexRef
    //
    //  Synopsis:
    //      A Vertex reference in a doubly linked list
    //
    //  Notes:
    //
    //--------------------------------------------------------------------------
    class CVertexRef
    {
    public:
        CVertexRef() {}
        ~CVertexRef() {}

        void Initialize(
            __in_ecount(1) const CVertex *pVertex,   // The vertex to initialize to
            IN WORD wIndex);             // Trianguation vertex index

        void operator = (
            __in_ecount(1) const CVertexRef &other);   // The other object to copy

        __out_ecount(1) CVertexRef *GetLeft() const
        {
            return m_pLeft;
        }

        __out_ecount(1) CVertexRef *GetRight() const
        {
            return m_pRight;
        }

        __outro_ecount(1) const WORD &Index() const
        {
            return m_wIndex;
        }

        WORD HasIndex() const
        {
            return (m_wIndex != 0xffff);
        }

        void SetAsLeftmost()
        {
            m_pLeft = NULL;
        }
        
        void SetAsRightmost()
        {
            m_pRight = NULL;
        }
        
        __outro_ecount(1) const GpPointR &GetPoint() const
        {
            Assert(m_pVertex);
            return m_pVertex->GetPoint();
        }

        bool IsLowerThan(__in_ecount(1) const CVertexRef *pOther) const
        {
            return AreAscending(GetPoint(), pOther->GetPoint());
        }

        void LinkTo(
            __inout_ecount_opt(1) CVertexRef  *pRight);
                // The vertex on the right to link to (NULL OK)

        __out_ecount_opt(1) CVertexRef *Split(
            __inout_ecount(1) CVertexRefPool &mem); // The memory pool to allocate from

#ifdef DBG
        bool CoincidesWith(__in_ecount(1) const CVertexRef *pOther) const
        {
            return m_pVertex->CoincidesWith(pOther->m_pVertex);
        }

        void AssertNoLeftDuplicate();
        void AssertNoRightDuplicate();
        void Dump() const;
        int m_id;                    // For debug tracking
#endif

    protected:
        const CVertex   *m_pVertex;  // The referenced vertex
        WORD            m_wIndex;    // Vertex index in the triangle buffer
        CVertexRef      *m_pLeft;    // Link to vertex on the left
        CVertexRef      *m_pRight;   // Link to vertex on the right
    };

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CVertexRefPool
    //
    //  Synopsis:
    //      A memory pool for vertices
    //
    //  Notes:
    //      I am setting this up as a class to support type-checking when passed
    //      as an argument.
    //
    //--------------------------------------------------------------------------
    class CVertexRefPool   :   public TMemBlockBase<CVertexRef>
    {
    public:
        CVertexRefPool()
        {
#ifdef DBG
        m_id = 0;
#endif
        }

        virtual ~CVertexRefPool()
        {
        }

        __out_ecount(1) CVertexRef *AllocateVertexRef(
            __in_ecount(1) const CVertex *pVertex,
                // Vertex to reference
            IN WORD wIndex);
                // Trianguation vertex index
    
#ifdef DBG
    public:
        int m_id;
#endif
    };  // End of definition of CVertexRefPool
    //----------------------------------------------------------------------------------------------
    
    //                  CTessellator members

public:
    CTessellator(
        __inout_ecount(1) IGeometrySink &sink,
            // Tessellation sink
        __in double rTolerance)
            // Flattening tolerance
            
        : CScanner(rTolerance),
          m_refSink(sink)
    {
    }

    virtual ~CTessellator()
    {
    }
    
public:

    // Inline methods
    void SetCeiling(
        __inout_ecount(1) CChain *pChain,
            // The chain to attach the ceiling to
        __in_ecount(1) CVertexRef  *pCeiling)
            // The ceiling end
    {
        Assert(pChain);
        pChain->SetTaskData(pCeiling);
    }

    __out_ecount(1) CVertexRef *GetCeiling(
        __inout_ecount(1) CChain  *pChain)
            // The chain to get the ceiling from
    {
        return static_cast<CVertexRef *>(pChain->GetTaskData());
    }

    // CScanner overrides
    virtual HRESULT ProcessTheJunction();

    virtual HRESULT ProcessCurrentVertex(
        __inout_ecount(1) CChain *pChain);
            // The chain whose current vertex we're processing

    // Methods supporting these overrides
    HRESULT CreateBands(
        __inout_ecount(1) CChain *pFirst,
            // The first left chain
        __inout_ecount(1) const CChain *pLast,
            // The last right chain
        IN WORD wIndex);
            // Trianguation index of the common head vertex
    
    HRESULT ProcessAllTails(
        IN WORD    wIndex,
            // Trianguation index of the common tail vertex
        __inout_ecount(1) CChain *pLeftmost,
            // The leftmost tail
        __inout_ecount(1) const CChain *pRightmost);
            // The rightmost tail

    HRESULT MergeTheBands(
        __inout_ecount(1) CChain *pLeftmostTail,
            // The leftmost tail
        __inout_ecount(1) CChain *pRightmostTail);
            // The rightmost tail
        
    HRESULT SplitTheBand(
        __inout_ecount(1) CChain *pLeftmostHead,
            // The leftmost head
        __inout_ecount(1) CChain *pRightmostHead,
            // The rightmost head
        IN WORD    wIndex);
            // Trianguation index of the current junction vertex
    
    HRESULT Connect(
        __inout_ecount(1) CChain     *pChain,
            // A chain, to which a vertex is added here
        __in_ecount(1) CVertexRef *pCeiling,
            // Ceiling end vertex
        IN WORD        wIndex);
            // Trianguation index of the current junction vertex
    
    HRESULT ProcessAsRight(
        __inout_ecount(1) CChain       *pChain,
            // The chain to process
        __in_ecount(1) CVertexRef   *pNext);
            // Reference to the next vertex down the chain
    
    HRESULT ProcessAsLeft(
        __inout_ecount(1) CChain       *pChain,
            // The chain to process
        __in_ecount(1) CVertexRef   *pNext);
            // Reference to the next vertex down the chain
    
    MIL_FORCEINLINE HRESULT AddVertex(
        __in_ecount(1) const GpPointR &ptR,
            // The vertex location
        __out_ecount(1) WORD &wIndex);
            // Triangulation vertex index
    
    MIL_FORCEINLINE HRESULT CreateTriangle(
        __in_ecount(1) const CVertexRef &vr1,
            // First vertex
        __in_ecount(1) const CVertexRef &vr2,
            // Second vertex
        __in_ecount(1) const CVertexRef &vr3)
            // Third vertex
    {
#ifdef DBG
         if (g_fTesselatorTrace)
        {
            OutputDebugString(L"Triangle\n");
            vr1.Dump();
            vr2.Dump();
            vr3.Dump();
        }
#endif
        return m_refSink.AddTriangle(vr1.Index(), vr2.Index(), vr3.Index());
    }


    // Debug
#ifdef DBG
    void ValidateBands();
    void DumpBands();
#endif

// Data
private:
    IGeometrySink   &m_refSink;         // Geometry recipient
    CVertexRefPool  m_oMem;             // Memory pool for ceiling vertices
};

#pragma warning( pop )


