//+---------------------------------------------------------------------------
//
//  Maintained by:  istvanc
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       matrix.hxx
//
//  Contents:   CMatrix
//
//  History:    2/10/95     istvanc created
//
//----------------------------------------------------------------------------

#ifndef _MATRIX_HXX_
#define _MATRIX_HXX_   1


#ifdef PRODUCT_97


#ifndef _XYL_HXX_
#   include "xyl.hxx"
#endif

const int BITMASK = 0xF;

/*
    The following matrix is an abstract representation of a grid of sites which we
    obtain by drawing imaginary long lines for every control edge and take all the cells
    these line create.
*/
class CMatrix
{
public:
    CMatrix (CMatrix **ppOwner);
private:
    ~CMatrix ();
    CMatrix() {};

public:

    #if DBG==1
    ULONG AddRef(void);
    ULONG Release(void);
    #else
    ULONG AddRef(void) { return ++_ulRefs; }
    ULONG Release(void)
    {
        if (--_ulRefs == 0)
        {
            _ulRefs++;
            delete this;
            return 0;
        }
        return _ulRefs;
    }
    #endif

    void Detach(void) { _ppOwner = 0;};
    // initialize the matrix, now can be called more than once
    HRESULT Init (CParentSite * pOuter);

    void SetDirty() { _fDirty = TRUE; }

    // add new site
    HRESULT AddSite (CSite * pSite, BOOL fSort);

    // remove site
    HRESULT RemoveSite (CSite * pSite, BOOL fSort);

    // set/get proposed rect for site
    HRESULT SetProposed (CSite * pSite, const CRectl * prcl);
    HRESULT GetProposed (CSite * pSite, CRectl * prcl);

    void MarkForMove(CSite * pSite);
    BOOL IsSiteMarkedForMove(CSite * pSite);


    // calculate new positions for sites
    HRESULT Calculate (Edge e, DWORD dwFlags);
    HRESULT CalculateUp (Edge e, DWORD dwFlags);

    CParentSite * getOuter() { return _pOuter; }

    // set proposed point and size for grid resize
    void PrepareGridResize
    (
        CParentSite * pOuter
    );

    // do grid resize on matrix
    HRESULT DoGridResize (CParentSite * pOuter, DWORD dwFlags);

    // move the sites
    HRESULT MoveSites(DWORD dwFlags = CSite::SITEMOVE_NOFIREEVENT | CSite::SITEMOVE_NOINVALIDATE);

    Edge EdgeTouched(CRectl * prcl, POINTL ptl);

    // find where site would move
    HRESULT FindMovedRect
    (
        CSite * pSiteMoved,     // site to move
        POINTL ptlMoveTo,       // new rect topleft
        POINTL ptlCursor,       // cursor position
        POINTL ptlLastCursor,   // last cursor position
        CRectl * prclMoved       // insertion rect (out)
    );

    // do the grid move
    HRESULT CMatrix::DoGridMove
    (
        CSite * pSite,
        CRectl * prclNew,
        CRectl * prclOld,
        DWORD dwFlags = CSite::SITEMOVE_NOFIREEVENT | CSite::SITEMOVE_NOINVALIDATE
    );

    HRESULT MoveRelatedCells(CDetailFrame *pRelated, BOOL fVertical);

    // snap edge to grid lines
    BOOL SnapEdge(LONG * pCoord, CSite * pSite, Edge e);

    // draw XOR rectangles
    void DrawGhosts(CDoc * pDoc, HDC hDC);

    // set _rcl rects to proposed
    void SetRectangles();
    // restore _rcl rects
    void ResetRectangles();

        /*
                This class contains the bag of data resize/move uses for deferred
                positioning and layout
        */
        class CFrame
        {
        public:

            CFrame(CSite * pSite)
            {
                _pSite = pSite;
                _uMoveBits = _uMoveMask = BITMASK;
                _fMarkForMove = 0;
            }

            BOOL IsNormalFrame() { return (_uMoveBits & _uMoveMask) == BITMASK; }

            HRESULT MoveToProposed(DWORD dwFlags);
            void SetDirtyRectangle(BOOL fDirty);
            void DrawFeedbackRect(CDoc * pDoc, HDC hDC);
            HRESULT CalcProposedPositions(CMatrix * pMatrix);

            CSite * _pSite;             // Pointer to site if exists
            Edge    _edgeTouched;   // Edge touched last time
            CRectl  _rclGrid;       // Grid coordinates for site (template)
            CRectl  _rcl;                   // Original position
            CRectl  _rclPropose;    // Proposed new rectangle

            unsigned _uMoveBits : 4;    // any bit set means participates in move/resize
            unsigned _uMoveMask : 4;    // mask used to and _uMoveBits
            unsigned _fMarkForMove : 1; // mark the frame to be moved instead of resized
        };

    friend CFrame;

    DECLARE_FORMSDATAARY(CAryFrame, CFrame, CFrame*);

        // find frame for site
    CFrame * FindSite (CSite * pSite);

    HRESULT AddFakeFrameForMove(CFrame * pFrame);
    void RemoveFakeFrames();


        /*
            This is a little helper class to keep track of Frame edges. We use CFrame
            pointers and mark the edges with bit 0..1 set (since all the pointers
            are 32 bit aligned...)
        */

        class CEdge
        {
        public:

            CEdge (CFrame * pFrame, Edge e)
            {
                Assert(pFrame);
                Assert(((ULONG)pFrame & 3) == 0);
                Assert(e >= edgeLeft && e <= edgeBottom);
                _ulFrame = (ULONG)pFrame | e;
            }

            CFrame * Frame () { return (CFrame *)(_ulFrame & 0xFFFFFFFC); }
            int Pos () { return ((CFrame *)(_ulFrame & 0xFFFFFFFC))->_rcl[(Edge)(_ulFrame & 3)]; }
            int PosPropose () { return ((CFrame *)(_ulFrame & 0xFFFFFFFC))->_rclPropose[(Edge)(_ulFrame & 3)]; }

            Edge Side () { return (Edge)(_ulFrame & 3); }

            void SetSide (Edge e) { Assert(e >= edgeLeft && e <= edgeBottom); _ulFrame = (_ulFrame & 0xFFFFFFFC) | e; }
            void Set (CFrame * pFrame, Edge e)
            {
                Assert(pFrame);
                Assert(((ULONG)pFrame & 3) == 0);
                Assert(e >= edgeLeft && e <= edgeBottom);
                _ulFrame = (ULONG)pFrame | e;
            }
            void SetNull () { _ulFrame = 0; }

            CEdge() {}    // private constructor not doing anything, only CMatrix is using it !

            ULONG _ulFrame;
        };


private:
    ULONG   _ulRefs;
    unsigned _uEdges;       // number of sites * 2
    CEdge * _pceVertical;   // sites sorted vertical
    CEdge * _pceHorizontal; // sites sorted horizontal
    unsigned _uRows;        // number of rows
    unsigned _uCols;        // number of columns
    CAryFrame _aryFrames;   // frame array matrix uses
    BOOL _fResizeOther;     // true if we have to call Recalc for the opposite direction
    BOOL    _fDirty;        // matrix sort order is bad
    CParentSite * _pOuter;  // outer site to resize
    CMatrix **_ppOwner;     // member of creator
    BOOL    _fHasFake;      // fake frame(s) added

    static  BOOL    s_fDoGridResize; // do global grid resize
    static  BOOL    s_fDoGridResizeSnapped;  // go grid resize only for snapped
    static  BOOL    s_fDoCloseResizeSnap;   // do snap only for the neighbor
    static int s_iSnapDelta[2]; // snap size in pixels x, y
};

#endif PRODUCT_97

#endif _MATRIX_HXX_
