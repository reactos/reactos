//+---------------------------------------------------------------------------
//
//  Maintained by: istvanc
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       src\ddoc\datadoc\matrix.cxx
//
//  Contents:   CMatrix
//
//  History:    2/10/95     istvanc created
//
//----------------------------------------------------------------------------

#define _MATRIX_CXX_   1



#include "headers.hxx"
#include "dfrm.hxx"

#if PRODUCT_97

#include "matrix.hxx"

extern TAG tagPropose;

BOOL    CMatrix::s_fDoGridResize = TRUE; // do global grid resize
BOOL    CMatrix::s_fDoGridResizeSnapped = FALSE;  // go grid resize only for snapped
BOOL    CMatrix::s_fDoCloseResizeSnap = FALSE;  // do snap only for the neighbor
int CMatrix::s_iSnapDelta[2] =
{
    5, 5
};


HRESULT CMatrix::CFrame::MoveToProposed(DWORD dwFlags)
{
    HRESULT hr;

#if DBG == 1
    if (_pSite)
    {
        TraceTag((tagPropose, "%ls/%d CMatrix::CFrame::MoveToProposed",
            _pSite->TBag()->_cstrName, _pSite->TBag()->_ID));
    }
    else
    {
        TraceTag((tagPropose, "CMatrix::CFrame::MoveToProposed (fake)"));
    }
#endif

    if (_pSite)
    {
        hr = _pSite->MoveToProposed(dwFlags);
    }
    else
    {
        _rcl = _rclPropose;
        hr = S_OK;
    }

    RRETURN (hr);
}

void CMatrix::CFrame::SetDirtyRectangle(BOOL fDirty)
{
    if (_pSite)
    {
        _pSite->_fIsDirtyRectangle = fDirty;
    }
}

void CMatrix::CFrame::DrawFeedbackRect(CDoc * pDoc, HDC hDC)
{
    if (_pSite)
    {
        _pSite->DrawFeedbackRect(hDC);
    }
    else
    {
        RECT rc;

        pDoc->DeviceFromHimetric(&rc, &_rclPropose);
        DrawDefaultFeedbackRect(hDC, &rc);
    }
}

HRESULT CMatrix::CFrame::CalcProposedPositions(CMatrix * pMatrix)
{
    HRESULT hr;

#if DBG == 1
    if (_pSite)
    {
        TraceTag((tagPropose, "%ls/%d CMatrix::CFrame::CalcProposedPositions",
             _pSite->TBag()->_cstrName, _pSite->TBag()->_ID));
    }
    else
    {
        TraceTag((tagPropose, "CMatrix::CFrame::CalcProposedPositions (fake)"));
    }
#endif

    if (_pSite && _pSite != pMatrix->_pOuter)
    {
        hr = _pSite->CalcProposedPositions();
    }
    else
    {
        hr  = S_OK;
    }

    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CMatrix
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//----------------------------------------------------------------------------
CMatrix::CMatrix (CMatrix **ppOwner)
{
    _ulRefs = 1;
    _uEdges = 0;
    _pceVertical = _pceHorizontal = NULL;
    _uRows = _uCols = 0;
    _pOuter = NULL;
    _ppOwner = ppOwner;
    _fDirty = FALSE;
}



//+---------------------------------------------------------------------------
//
//  Member:     ~CMatrix
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//----------------------------------------------------------------------------
CMatrix::~CMatrix ()
{
    delete [] _pceVertical;
    delete [] _pceHorizontal;
    if (_ppOwner)
    {
        *_ppOwner = 0;  // null out owner member
    }
}



//+---------------------------------------------------------------------------
//
//  Function:     EdgeCompare
//
//  Synopsis:   Sort helper function to compare edges to qsort
//
//  Arguments:  pe1        pointer to edge
//                pe2        pointer to edge
//
//    Returns:    -1    if pe1 < pe2
//                0    if pe1 == pe2
//                1     if pe1 > pe2
//
//----------------------------------------------------------------------------
int __cdecl
EdgeCompare(CMatrix::CEdge * pe1, CMatrix::CEdge * pe2)
{
    int d = pe1->Pos() - pe2->Pos();
    if (!d)
    {
        // same coordinates
        d = (int)pe1->Side() - (int)pe2->Side();
        if (pe1->Frame() != pe2->Frame())
        {
            Edge e1 = pe1->Side();
            Edge e2 = pe2->Side();
            CMatrix::CFrame * pS1 = pe1->Frame();
            CMatrix::CFrame * pS2 = pe2->Frame();
            // different sites
            if (d || !(pS1->_rcl [e1] - pS1->_rcl [OPPOSITE(e1)]) || !(pS2->_rcl[e2] - pS2->_rcl[OPPOSITE(e2)]))
            {
                // different edges
                d = pS1->_rcl [OPPOSITE(e1)] - pS2->_rcl[OPPOSITE(e2)];
            }
            else
            {
                // same edges
                d = pS1->_rcl[(Edge )((e1 & 1) ^ 1)] -
                    pS2->_rcl[(Edge )((e2 & 1) ^ 1)];
            }
        }
    }
    return d;
}



//+---------------------------------------------------------------------------
//
//  Function:   BinLookup
//
//  Synopsis:   lookup the edge in array of edges and return position
//
//  Arguments:  uElems        number of elements in array
//              pceEdges        array if edges
//              ce             edge to look up
//
//
//    Returns:    ordinal number of integer closest to 'i' in array
//
//----------------------------------------------------------------------------
unsigned
BinLookup(unsigned uElems, CMatrix::CEdge * pceEdges, CMatrix::CEdge ce)
{
    unsigned ul, ur, um;

    Assert(uElems);

    if (uElems == 1)
        return 0;

    ul = 0;
    ur = uElems-1;

    while (ur - ul)
    {
        um = (ul + ur) / 2;
        int i = EdgeCompare (&ce, pceEdges + um);
        if (i < 0)
            ur = um;
        else if (i > 0)
            ul = um;
        else
            return um;
    }
    Assert (EdgeCompare (&ce, pceEdges + ul) == 0);
    return ul;
}




//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::Init
//
//  Synopsis:   Initialize matrix which can be called more than once
//                to reinitialize with a different site array or just
//                to resort the sites
//
//  Arguments:  parySites    Pointer to site array to add (can be NULL)
//                pOuter        Site provides 'outer' (boundary) rectangle
//                fFirst        TRUE if first time initialize matrix
//
//    Returns:    Success if all memory allocation worked
//
//----------------------------------------------------------------------------
HRESULT
CMatrix::Init (CParentSite * pOuter)
{
    HRESULT hr = S_OK;

    unsigned u;
    CMatrix::CEdge * pceV, * pceH;
    int x, y;
    int x1, y1;
    CFrame * pFrame;
    CFrame * pFrameEnd;
    CRectl rcl;
    BOOL fSizeChanged;
    CFrame * pFrameOuter;

    if (pOuter)
    {
        fSizeChanged = TRUE;
        u = pOuter->_arySites.Size();
        _pOuter = pOuter;
        u++;
    }
    else
    {
        fSizeChanged = FALSE;
        u = _aryFrames.Size();
        Assert(!pOuter);
    }

    u <<= 1;
    fSizeChanged |= u != _uEdges;

    // fSizeChanged
    // force change now
    _uEdges = 0;

    _fDirty = TRUE;

    if (pOuter)
    {
        CSite ** ppSite;
        CSite ** ppSiteEnd;

        _aryFrames.DeleteAll();
        hr = _aryFrames.EnsureSize(u >> 1);
        if (hr)
            goto Cleanup;
        for (ppSite = pOuter->_arySites, ppSiteEnd = ppSite + pOuter->_arySites.Size();
            ppSite < ppSiteEnd; ppSite++)
        {
            CFrame frm(*ppSite);
#if DBG==1
            frm._rclPropose.left = frm._rclPropose.top =
            frm._rclPropose.right = frm._rclPropose.bottom = 0xFEFEFEFE;
#endif
            hr = _aryFrames.AppendIndirect(&frm);
            if (hr)
                goto Cleanup;
        }
        CFrame frm(_pOuter);
#if DBG==1
        frm._rclPropose.left = frm._rclPropose.top =
        frm._rclPropose.right = frm._rclPropose.bottom = 0xFEFEFEFE;
#endif
        hr = _aryFrames.AppendIndirect(&frm);
        if (hr)
            goto Cleanup;
    }

    /*
        Allocate arrays for all the vertical and
        horizontal edges
    */
    _uEdges = u;

    delete [] _pceVertical;
    _pceVertical = new CEdge [_uEdges];
    if (!_pceVertical)
        goto MemoryError;

    delete [] _pceHorizontal;
    _pceHorizontal = new CEdge [_uEdges];
    if (!_pceHorizontal)
        goto MemoryError;

    /*
        Fill the array of edges from the site
        list and sort them
    */
    pceV = _pceVertical;
    pceH = _pceHorizontal;
    for (pFrame = _aryFrames, pFrameEnd = pFrame + _aryFrames.Size();
        pFrame < pFrameEnd; pFrame++)
    {
        CEdge ceLeft(pFrame, edgeLeft);
        *pceV++ = ceLeft;
        CEdge ceTop(pFrame, edgeTop);
        *pceH++ = ceTop;
        CEdge ceRight(pFrame, edgeRight);
        *pceV++ = ceRight;
        CEdge ceBottom(pFrame, edgeBottom);
        *pceH++ = ceBottom;
        /*
            Refetch _rcl
        */
        if (pFrame->_pSite && pFrame->IsNormalFrame())    // BUGBUG hack to avoid resetting in the move case !
        {
            pFrame->_rcl = pFrame->_pSite->_rcl;
        }
        /*
            Reset touch flags
        */
        pFrame->_edgeTouched = edgeInvalid;
        /*
            Reset proposed positions and size
        */
        if (pFrame->_pSite && !pFrame->_pSite->_fProposedSet)
        {
            pFrame->_rclPropose = pFrame->_rcl;
        }
    }

    /*
        We have to make sure somehow that the outer site edges
        are always at the end of the array !!!
    */

    if (_pOuter)
    {
        pFrameOuter = &_aryFrames[_aryFrames.Size() - 1];
        Assert(pFrameOuter->_pSite == _pOuter);
        rcl = pFrameOuter->_rcl;
        pFrameOuter->_rcl.left = pFrameOuter->_rcl.top = -0x3FFFFFFF;
        pFrameOuter->_rcl.right = pFrameOuter->_rcl.bottom = 0x3FFFFFFF;
    }

    qsort(_pceVertical, _uEdges, sizeof(CEdge),
        (int (__cdecl *) (const void *, const void *))EdgeCompare);
    qsort(_pceHorizontal, _uEdges, sizeof(CEdge),
        (int (__cdecl *) (const void *, const void *))EdgeCompare);

    if (_pOuter)
    {
        pFrameOuter->_rcl = rcl;
    }


    Assert(_pOuter ? _pceVertical[0].Frame()->_pSite == _pOuter &&
        _pceVertical[_uEdges - 1].Frame()->_pSite == _pOuter : TRUE);
    Assert(_pOuter ? _pceHorizontal[0].Frame()->_pSite == _pOuter &&
        _pceHorizontal[_uEdges - 1].Frame()->_pSite == _pOuter : TRUE);

    /*
        Now let's go thru the sorted arrays and count the
        number of distinct edge positions
    */
    _uRows = 0;
    _uCols = 0;
    pceV = _pceVertical;
    pceH = _pceHorizontal;
    x = pceV->Pos();
    y = pceH->Pos();
    for (u = 0; u < _uEdges; u++)
    {
        x1 = pceV->Pos();
        if (x != x1)
        {
            _uCols++;
        }
        x = x1;

        y1 = pceH->Pos();
        if (y != y1)
        {
            _uRows++;
        }
        y = y1;

        /*
            Set the _rclGrid of the Site to the row/column number
        */
        pceV->Frame()->_rclGrid[pceV->Side()] = _uCols;
        pceH->Frame()->_rclGrid[pceH->Side()] = _uRows;
        pceV++;
        pceH++;
    }

#if DBG==1
    if (_pOuter)
    {
        CFrame * pFrame = _pceVertical[0].Frame();
        Assert(pFrame->_rclGrid.left   == 0           &&
               pFrame->_rclGrid.top    == 0           &&
               pFrame->_rclGrid.right  == (int)_uCols &&
               pFrame->_rclGrid.bottom == (int)_uRows);
    }
#endif

    _fDirty = FALSE;

Cleanup:

    RRETURN(hr);

MemoryError:

    hr = E_OUTOFMEMORY;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::AddSite
//
//  Synopsis:   Add a site to matrix
//
//  Arguments:  pSite        Pointer to site to add
//              fSort        Sort matrix
//
//    Returns:    Success if all site added
//
//----------------------------------------------------------------------------
HRESULT
CMatrix::AddSite (CSite * pSite, BOOL fSort)
{
    HRESULT hr;

    Assert(pSite);
    int i = pSite->getIndex();
    // all the other sites had to be added already...
    Assert(i >= 0 && i <= _aryFrames.Size());
    CFrame frm(pSite);
    frm._rcl = pSite->_rcl;
    hr = _aryFrames.InsertIndirect(i, &frm);
    if (!hr)
    {
        _fDirty = TRUE;
        if (fSort)
            hr = Init(NULL);
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::RemoveSite
//
//  Synopsis:   Remove a site from matrix
//
//  Arguments:  pSite        Pointer to site to remove
//              fSort        Sort matrix
//
//    Returns:    Success if all site removed
//
//----------------------------------------------------------------------------
HRESULT
CMatrix::RemoveSite (CSite * pSite, BOOL fSort)
{
    HRESULT hr = S_OK;
    CFrame * pFrame;

    Assert(pSite);
    pFrame = FindSite(pSite);
    if (pFrame)
    {
        _aryFrames.Delete(pFrame - _aryFrames);
        _fDirty = TRUE;
        if (fSort)
            hr = Init(NULL);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::RemoveSite
//
//  Synopsis:   Find frame for a site in matrix
//
//  Arguments:  pSite        Pointer to site
//
//    Returns:    Frame pointer if found
//
//----------------------------------------------------------------------------
CMatrix::CFrame * CMatrix::FindSite (CSite * pSite)
{
    int i = pSite == _pOuter ? _aryFrames.Size() - 1 : pSite->getIndex();
    Assert(i >= 0 && i < _aryFrames.Size());

    return _aryFrames + i;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMatrix::SetProposed
//
//  Synopsis:   Remember the deferred move rectangle for this site
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CMatrix::SetProposed (CSite * pSite, const CRectl * prcl)
{
    HRESULT hr;
    CFrame * pFrame;

    TraceTag((tagPropose, "%ls/%d CMatrix::SetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        pSite->TBag()->_cstrName,
        pSite->TBag()->_ID,
        prcl->left - pSite->_rcl.left, prcl->top - pSite->_rcl.top, prcl->right - pSite->_rcl.right, prcl->bottom - pSite->_rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    if (_pOuter && pSite->_pParent != _pOuter)
    {
        PrepareGridResize((CParentSite *)pSite->_pParent);
    }

    if (_fDirty)
    {
        hr = Init(NULL);
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = S_OK;
    }
    pFrame = FindSite(pSite);
    Assert(pFrame);
    pFrame->_rclPropose = *prcl;

Cleanup:

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMatrix::GetProposed
//
//  Synopsis:   Get back the deferred move rectangle for this site
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CMatrix::GetProposed (CSite * pSite, CRectl * prcl)
{
    HRESULT hr;
    CFrame * pFrame;

#if DBG==1
    if (_pOuter)
    {
        Assert(_pOuter == pSite->_pParent);
    }
#endif

    if (_fDirty)
    {
        hr = Init(NULL);
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = S_OK;
    }
    pFrame = FindSite(pSite);
    Assert(pFrame);
    *prcl = pFrame->_rclPropose;

    TraceTag((tagPropose, "%ls/%d CMatrix::GetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        pSite->TBag()->_cstrName,
        pSite->TBag()->_ID,
        prcl->left - pSite->_rcl.left, prcl->top - pSite->_rcl.top, prcl->right - pSite->_rcl.right, prcl->bottom - pSite->_rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

Cleanup:

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMatrix::MarkForMove
//
//  Synopsis:   Mark the site for move in the later grid calculate
//
//--------------------------------------------------------------------------

void
CMatrix::MarkForMove(CSite * pSite)
{
    CFrame * pFrame = FindSite(pSite);
    Assert(pFrame);
    pFrame->_fMarkForMove = TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMatrix::IsSiteMarkedForMove
//
//  Synopsis:   checks if the site is marked for move
//
//--------------------------------------------------------------------------
BOOL
CMatrix::IsSiteMarkedForMove(CSite * pSite)
{
    CFrame * pFrame = FindSite(pSite);
    Assert(pFrame);
    return (pFrame->_fMarkForMove);
}





//+---------------------------------------------------------------------------
//
//  Member:     IntSet
//
//  Synopsis:   Set 'c' 4 byte integers to 'i' from 'p'
//
//  Arguments:  p            array of integers
//                i            __int32 value
//                c            count
//
//    Returns:
//
//----------------------------------------------------------------------------
#ifdef _X86_
void inline
IntSet(void * p, int i, size_t c)
{
    __asm
    {
        push EDI
        mov EDI, p
        mov EAX, i
        mov ECX, c
        cld
        repnz stosd
        pop EDI
    }
}
#else
void
IntSet(void * p, int i, size_t c)
{
        if ((int)p & 0x00000003)
        {
                union
                {
                        BYTE    ch[4];
                        int             i;
                } un;
                LPBYTE  pch;

                pch = (LPBYTE)p;
                un.i = i;
        while (c-- > 0)
        {
            *pch++ = un.ch[0];
            *pch++ = un.ch[1];
            *pch++ = un.ch[2];
            *pch++ = un.ch[3];
        }
        }
        else
        {
// MacNote: unsigned numbers are always >= zero...
        int *pT = (int* )p;
        while (c-- > 0)
        {
                *pT++ = i;
        }
        }

}
#endif

#define SITEMOVE_GRIDHORIZONTAL (CSite::SITEMOVE_NOMOVECHILDREN << 1)
#define SITEMOVE_GRIDVERTICAL (CSite::SITEMOVE_NOMOVECHILDREN << 2)


//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::Calculate
//
//  Synopsis:   Calculate new layout of the sites from proposed positions
//                left to right (or top to bottom)
//
//  Arguments:  e                edge (direction) to work with
//                fDoOutsideIn    TRUE if to go inside contained layouts
//
//    Returns:    Success if completed
//
//----------------------------------------------------------------------------
HRESULT
CMatrix::Calculate (Edge e, DWORD dwFlags)
{
    HRESULT hr;
    CFrame * pFrame, * pF;
    CEdge * pce, * pceDelta;
    unsigned u, uLine, uDeltas;
    int x, x1, x2;
    int yLine, yMin, yMax;
    Edge eLine, eOther, eOpposite;

    Assert(e == edgeLeft || e == edgeTop);

    if (_pOuter && _uEdges == 2)   // the outer is the only site
        return S_OK;

    eOther = OTHER(e);
    eOpposite = OPPOSITE(e);

    if (e & 1)
    {
        uDeltas = _uCols;
        pce = _pceHorizontal;
        dwFlags |= SITEMOVE_GRIDVERTICAL;
    }
    else
    {
        uDeltas = _uRows;
        pce = _pceVertical;
        dwFlags |= SITEMOVE_GRIDHORIZONTAL;
    }

    /*
        We walk thru the sites top to bottom and for every column we accumulate
        delta changes which will move the sites below. We allocate a helper array
        to keep track of the sites moving others.
    */
    pceDelta = new CEdge [uDeltas];
    if (!pceDelta)
        goto MemoryError;

    /*
        Swap outer site opposite sides so they behave correctly
    */
    if (_pOuter)
    {
        CEdge ce(&_aryFrames[_aryFrames.Size()-1], e);
        IntSet(pceDelta, *(int *)&ce, uDeltas);
    }
    else
    {
        memset(pceDelta, 0, sizeof(CEdge) * uDeltas);
    }

    hr = S_OK;

    uLine = 0;

    yMin = 0x7FFFFFFF;
    yMax = -0x7FFFFFFF;

    for (u = 1; u < _uEdges; u++)
    {
        pFrame = pce [u].Frame();
        eLine = pce [u].Side();

        CEdge ceFrameMoved;
        ceFrameMoved.SetNull();
        CEdge ceFrameBase;
        ceFrameBase.SetNull();

        x1 = pFrame->_rclGrid [eOther];
        x2 = pFrame->_rclGrid [OPPOSITE(eOther)];
        /*
            Let's find the maximum bottom and moved bottom
        */
        for (x = x1; x < x2; x++)
        {
            CEdge ce = pceDelta [x];

            CFrame * pF = ce.Frame();
            if (pF)
            {
                // check if this frame is supposed to move the other
                if (pF->_uMoveMask & pFrame->_uMoveBits)
                {
                    if (ceFrameBase.Frame())
                    {
                        if (ceFrameBase.Pos() < ce.Pos())
                            ceFrameBase = ce;
                    }
                    else
                        ceFrameBase = ce;
                    if (ceFrameMoved.Frame())
                    {
                        if (ceFrameMoved.PosPropose() < ce.PosPropose())
                            ceFrameMoved = ce;
                    }
                    else
                        ceFrameMoved = ce;
                }
            }

            /*
                Set this site as delta for sites below
            */
            pceDelta [x] = pce [u];
        }

        /*
            We want to move by keeping the distance to pFrameBase
            relative to pFrameMoved
        */
        if (ceFrameMoved.Frame())
        {
            if (ceFrameBase.Frame())
            {
                x = ceFrameMoved.PosPropose() - ceFrameBase.Pos();
                if (x)
                {
                    pFrame->_rclPropose [eLine] += x;
                    pFrame->_fIsDirtyRectangle = TRUE;
                }
            }
            if (pFrame->_rclPropose [eLine] < ceFrameMoved.PosPropose())
            {
                pFrame->_rclPropose [eLine] = ceFrameMoved.PosPropose();
            }
        }

        /*
            Remember maximum and minimum y position (for 'normal' frames)
        */
        if (pFrame->IsNormalFrame())
        {
            x = pFrame->_rclPropose [eLine];
            if (yMin > x)
                yMin = x;
            if (yMax < x)
                yMax = x;
        }
        /*
            If we do grid resize then check if everybody in this line
            was move with the same amount
        */
        if (s_fDoGridResize)
        {
            /*
                Check if we are entering a new line and set positions
                in previous line to min or max y position

                If frame is not 'normal' (a fake frame or the owner frame)
                we don't want to do the snap resize
            */

            yLine = pFrame->_rcl [eLine];
            if (u == _uEdges - 1 ||
                pce [u + 1].Side() != eLine || pce [u + 1].Pos() != yLine)
            {
                if (yMax > yLine)
                    yLine = yMax;
                else if (yMin < yLine)
                    yLine = yMin;
                else
                    uLine = u + 1;
                if (uLine < u)
                {
                    while (uLine <= u)
                    {
                        pF = pce [uLine].Frame();
                        if (pF->_pSite != _pOuter && pF->IsNormalFrame())
                        {
                            Assert(pce [uLine].Side() == eLine);
                            pF->_rclPropose [eLine] = yLine;
                            /*
                                If size is negative, move top and set size to 0
                            */
                            if (pF->_rclPropose[(Edge)(e + 2)] < pF->_rclPropose[e])
                            {
                                pF->_rclPropose[(Edge)(e + 2)] = pF->_rclPropose[e] = yLine;
                                _fResizeOther = TRUE;
                            }
                            pF->_fIsDirtyRectangle = TRUE;
                            if (pF != pFrame && eLine != e)
                            {
                                pF->CalcProposedPositions(this);
                            }
                        }
                        uLine++;
                    }
                }
                yMin = 0x7FFFFFFF;
                yMax = -0x7FFFFFFF;
                uLine = u + 1;
            }
        }
        if (eLine != e)    // do it at the bottom of the sites
        {
            pFrame->CalcProposedPositions(this);
        }
    }

Cleanup:

    delete [] pceDelta;

    RRETURN(hr);

MemoryError:

    hr = E_OUTOFMEMORY;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::CalculateUp
//
//  Synopsis:   Calculate new layout of the sites from proposed positions
//                right to left (or bottom to top)
//
//  Arguments:  e                edge (direction) to work with
//                fDoOutsideIn    TRUE if to go inside contained layouts
//
//    Returns:    Success if completed
//
//----------------------------------------------------------------------------

// BUGBUG this should be incorporated into Calculate !!!!

HRESULT
CMatrix::CalculateUp (Edge e, DWORD dwFlags)
{
    HRESULT hr;
    CFrame * pFrame, * pF;
    CEdge * pce, * pceDelta;
    unsigned u, uLine, uDeltas;
    int x, x1, x2;
    int yLine, yMin, yMax;
    Edge eLine, eOther, eOpposite;

    Assert(e == edgeLeft || e == edgeTop);

    if (_pOuter && _uEdges == 2)   // the outer is the only site
        return S_OK;

    eOther = OTHER(e);
    eOpposite = OPPOSITE(e);

    if (e & 1)
    {
        uDeltas = _uCols;
        pce = _pceHorizontal;
        dwFlags &= ~SITEMOVE_GRIDHORIZONTAL;
    }
    else
    {
        uDeltas = _uRows;
        pce = _pceVertical;
        dwFlags &= ~SITEMOVE_GRIDVERTICAL;
    }

    /*
        We walk thru the sites top to bottom and for every column we accumulate
        delta changes which will move the sites below. We allocate a helper array
        to keep track of the sites moving others.
    */
    pceDelta = new CEdge [uDeltas];
    if (!pceDelta)
        goto MemoryError;

    uLine = _uEdges - 1;

    if (_pOuter)
    {
        CEdge ce(&_aryFrames[_aryFrames.Size()-1], eOpposite);
        pFrame = pce [uLine - 1].Frame();
        x1 = pFrame->_rclGrid [eOther];
        x2 = pFrame->_rclGrid [OPPOSITE(eOther)];
        memset(pceDelta, 0, sizeof(CEdge) * uDeltas);
        IntSet(pceDelta + x1, *(int *)&ce, x2 - x1);
//        IntSet(pceDelta, *(int *)&ce, uDeltas);
    }
    else
        memset(pceDelta, 0, sizeof(CEdge) * uDeltas);

    hr = S_OK;

    yMin = 0x7FFFFFFF;
    yMax = -0x7FFFFFFF;

    for (u = _uEdges - 1; u + 1 > 0; u--)
    {
        pFrame = pce [u].Frame();
        eLine = pce [u].Side();

        CEdge ceFrameMoved;
        ceFrameMoved.SetNull();
        CEdge ceFrameBase;
        ceFrameBase.SetNull();

        x1 = pFrame->_rclGrid [eOther];
        x2 = pFrame->_rclGrid [OPPOSITE(eOther)];
        /*
            Let's find the maximum bottom and moved bottom
        */
        for (x = x1; x < x2; x++)
        {
            CEdge ce = pceDelta [x];

            CFrame * pF = ce.Frame();
            if (pF)
            {
                // check if this frame is supposed to move the other
                if (pF->_uMoveMask & pFrame->_uMoveBits)
                {
                    if (ceFrameBase.Frame())
                    {
                        if (ceFrameBase.Pos() > ce.Pos())
                            ceFrameBase = ce;
                    }
                    else
                        ceFrameBase = ce;
                    if (ceFrameMoved.Frame())
                    {
                        if (ceFrameMoved.PosPropose() > ce.PosPropose())
                            ceFrameMoved = ce;
                    }
                    else
                        ceFrameMoved = ce;
                }
            }

            /*
                Set this site as delta for sites below
            */
            pceDelta [x] = pce [u];
        }
        /*
            We want to move by keeping the distance to pFrameBase
            relative to pFrameMoved
        */
        if (ceFrameMoved.Frame())
        {
            /*
                No move if the bottom would push the top except if
                it would go beyond
            */
            if (ceFrameBase.Frame())
            {
                x = ceFrameMoved.PosPropose() - ceFrameBase.Pos();
                if (x)
                {
                    pFrame->_rclPropose [eLine] += x;
                    pFrame->_fIsDirtyRectangle = TRUE;
                }
            }
            if (pFrame->_rclPropose [eLine] > ceFrameMoved.PosPropose())
            {
                pFrame->_rclPropose [eLine] = ceFrameMoved.PosPropose();
            }
        }

        /*
            Remember maximum and minimum y position (for 'normal' frames)
        */
        if (pFrame->IsNormalFrame())
        {
            x = pFrame->_rclPropose [eLine];
            if (yMin > x)
                yMin = x;
            if (yMax < x)
                yMax = x;
        }
        /*
            If we do grid resize then check if everybody in this line
            was move with the same amount
        */
        if (s_fDoGridResize)
        {
            /*
                Check if we are entering a new line and set positions
                in previous line to min or max y position
            */

            yLine = pFrame->_rcl [eLine];
            if (u == 0 ||
                pce [u - 1].Side() != eLine || pce [u - 1].Pos() != yLine)
            {
                if (yMax > yLine)
                    yLine = yMax;
                else if (yMin < yLine)
                    yLine = yMin;
                else
                    uLine = u - 1;
                if (uLine + 1 > u + 1)
                {
                    while (uLine + 1 >= u + 1)
                    {
                        pF = pce [uLine].Frame();
                        if (pF->_pSite != _pOuter && pF->IsNormalFrame())
                        {
                            Assert(pce [uLine].Side() == eLine);
                            pF->_rclPropose [eLine] = yLine;
                            /*
                                If size is negative, move top and set size to 0
                            */
                            if (pF->_rclPropose[(Edge)(e + 2)] < pF->_rclPropose[e])
                            {
                                pF->_rclPropose[(Edge)(e + 2)] = pF->_rclPropose[e] = yLine;
                                _fResizeOther = TRUE;
                            }
                            pF->_fIsDirtyRectangle = TRUE;
                            if (pF != pFrame && eLine != e)
                            {
                                pF->CalcProposedPositions(this);
                            }
                        }
                        uLine--;
                    }
                }
                yMin = 0x7FFFFFFF;
                yMax = -0x7FFFFFFF;
                uLine = u - 1;
            }
        }
        if (eLine == e)    // do it at the top of the sites
        {
            pFrame->CalcProposedPositions(this);
        }
    }

Cleanup:

    delete [] pceDelta;

    RRETURN(hr);

MemoryError:

    hr = E_OUTOFMEMORY;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::SnapEdge
//
//  Synopsis:   Snap the edge of the site into passed pointer to coordinate
//
//  Arguments:  pCoord      Pointer to coordinate to return (out)
//              pSnapSite   Pointer to site
//              e           Edge of site to snap
//
//
//    Returns:    TRUE if snapped the edge
//
//----------------------------------------------------------------------------
BOOL
CMatrix::SnapEdge(LONG * pCoord, CSite * pSnapSite, Edge e)
{
    CFrame * pFrame;
    CFrame * pFrameEnd;
    LONGLONG llSnapDelta;
    LONGLONG llDelta;
    Edge eOpposite = OPPOSITE(e);
    Edge eOtherFirst = (Edge)((e & 1) ^ 1);
    Edge eOtherSecond = (Edge)(eOtherFirst + 2);
    CEdge ceClosest;
    CFrame * pSnapFrame;

    ceClosest.SetNull();
    if ((int)e & 1)
    {
        llSnapDelta = HimetricFromVPix(s_iSnapDelta [1]);
    }
    else
    {
        llSnapDelta = HimetricFromHPix(s_iSnapDelta [0]);
    }
    llSnapDelta *= llSnapDelta;
    pSnapFrame = FindSite(pSnapSite);
    Assert(pSnapFrame);
    /*
        BUGBUG: simple but slow implementation...
    */
    for (pFrame = _aryFrames, pFrameEnd = pFrame + _aryFrames.Size();
        pFrame < pFrameEnd; pFrame++)
    {
        if (pFrame == pSnapFrame)
            continue;
        if (pFrame->_rcl [e] == pSnapFrame->_rclPropose [e])
        {
            llDelta = pFrame->_rcl [e] - *pCoord;
            llDelta *= llDelta;
            if (llDelta < llSnapDelta)
            {
                llSnapDelta = llDelta;
                ceClosest.Set(pFrame, e);
            }
        }
        if (pSnapFrame->_rcl [eOtherFirst] < pFrame->_rcl [eOtherSecond] &&
            pSnapFrame->_rcl [eOtherSecond] > pFrame->_rcl [eOtherFirst])
        {
            if (pFrame->_rcl [eOpposite] == pSnapFrame->_rclPropose [eOpposite])
            {
                llDelta = pFrame->_rcl [eOpposite] - *pCoord;
                llDelta *= llDelta;
                if (llDelta < llSnapDelta)
                {
                    llSnapDelta = llDelta;
                    ceClosest.Set(pFrame, eOpposite);
                }
            }
        }
    }
    if (ceClosest.Frame())
    {
        *pCoord = ceClosest.Pos();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::DrawGhosts
//
//  Synopsis:   Draw xor-d feedback rectangles of resized sites in matrix
//
//  Arguments:  pForm       Form to operate on
//                hDC         HDC to use to draw
//
//
//    Returns:
//
//----------------------------------------------------------------------------
void
CMatrix::DrawGhosts(CDoc * pDoc, HDC hDC)
{
    CFrame * pFrame = _aryFrames;
    CFrame * pFrameEnd = pFrame + _aryFrames.Size();

    /*
        Go thru the sites and draw them if proposed position
        changed
    */
    for (; pFrame < pFrameEnd; pFrame++)
    {
        if (pFrame->_rcl != pFrame->_rclPropose)
        {
            if (pFrame->_pSite != _pOuter)   // outer will be drawed one level up !
            {
                pFrame->DrawFeedbackRect(pDoc, hDC);
            }
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::PrepareGridResize
//
//  Synopsis:   Prepare sites for the matrix layout calculation by
//              resetting proposed positions and setting new ones
//
//  Arguments:  prcDeltas       Pointer delta rectangle to adjust sites with
//                pSiteMoved      Pointer to site moved (NULL if array passed)
//              paryMoved       Pointer to array of sites moved (NULL if
//                              site passed)
//              fResetProposed  TRUE if to reset all proposed rects in sites
//
//
//    Returns:
//
//----------------------------------------------------------------------------
void
CMatrix::PrepareGridResize
(
    CParentSite * pOuter
)
{
    CFrame * pFrame;
    int cFrames;
    CSite ** ppSite;
    int cSites;
    CPointl ptl;

    if (pOuter && pOuter != _pOuter)
    {
        // replace the sites with the new instances
        pFrame = _aryFrames;
        CArySite * parySites = &pOuter->_arySites;
        if (parySites)
        {
            ppSite = *parySites;
            cSites = parySites->Size();
            for (; cSites; cSites--, pFrame++, ppSite++)
            {
                if (pFrame->_pSite)
                {
                    pFrame->_pSite = *ppSite;
                    pFrame->_rcl = (*ppSite)->_rcl;
                    pFrame->_edgeTouched = edgeInvalid;
                }
            }
        }
        Assert(pFrame->_pSite == _pOuter);
        pFrame->_pSite = pOuter;
        pFrame->_edgeTouched = edgeInvalid;
        _pOuter = pOuter;
    }
    else if (_pOuter)
    {
        pFrame = FindSite(_pOuter);
        Assert(pFrame);
    }
    cFrames = _aryFrames.Size();
    if (_pOuter)
    {
        _pOuter->GetProposed(_pOuter, &pFrame->_rclPropose);
        pFrame->_rcl = _pOuter->_rcl;
        // BUGBUG: Istvan, please review
        // pFrame->SetDirtyRectangle(FALSE);

        pFrame->_uMoveBits = BITMASK;
        ptl.x = pFrame->_rclPropose.left - pFrame->_rcl.left;
        ptl.y = pFrame->_rclPropose.top - pFrame->_rcl.top;
        cFrames--;  // to avoid setting outer later
    }
    else
    {
        ptl.x = ptl.y = 0;
    }


    // here we assume that on frames without a site the proposed
    // position is always correct !
    for (pFrame = _aryFrames; cFrames; cFrames--, pFrame++)
    {
        pFrame->_fIsDirtyRectangle = FALSE;
        pFrame->_uMoveBits = BITMASK;
        if (pFrame->_pSite)
        {
            pFrame->_rcl = pFrame->_pSite->_rcl;
            if (!pFrame->_pSite->_fProposedSet)
            {

                // BUGBUG: this is a shortcur to avoid the bubbling up
                //   of pFrame->_pSite->ProposedMove(&pFrame->_rcl)
                pFrame->_rclPropose = pFrame->_rcl;
                pFrame->_pSite->_fProposedSet = TRUE;

            }
            else if (pFrame->_fMarkForMove)
            {
                // add fake frame for collapse, this can change the array !
                IGNORE_HR(AddFakeFrameForMove(pFrame));
                pFrame->_fMarkForMove = FALSE;
            }
        }
        pFrame->_rclPropose += ptl;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::SetRectangles
//
//  Synopsis:   Set _rcl (rect positions) to proposed positions (temporarely)
//
//  Arguments:
//
//    Returns:
//
//----------------------------------------------------------------------------
void
CMatrix::SetRectangles()
{
    CFrame * pFrame = _aryFrames;
    CFrame * pFrameEnd = pFrame + _aryFrames.Size();

    for (; pFrame < pFrameEnd; pFrame++)
    {
        pFrame->_rcl = pFrame->_rclPropose;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::ResetRectangles
//
//  Synopsis:   Reset _rcl to real positions by refetching
//
//  Arguments:
//
//    Returns:
//
//----------------------------------------------------------------------------
void
CMatrix::ResetRectangles()
{
    CFrame * pFrame = _aryFrames;
    CFrame * pFrameEnd = pFrame + _aryFrames.Size();

    // BUGBUG doesn't work for 'empty' frames
    for (; pFrame < pFrameEnd; pFrame++)
    {
        if (pFrame->_pSite)
        {
            pFrame->_rcl = pFrame->_pSite->_rcl;
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::DoGridResize
//
//  Synopsis:   Do the grid resize in the matrix
//
//  Arguments:  pOuter  the container of the matrix
//              dwFlags SITEMOVE_FLAGS
//
//    Returns:    Success if resize worked
//
//----------------------------------------------------------------------------
HRESULT
CMatrix::DoGridResize
(
    CParentSite * pOuter,
    DWORD   dwFlags
)
{
    HRESULT hr = S_OK;

    if (!_uEdges)
        goto Cleanup;

    // this should happen before Init because of the adding of fake frames !
    PrepareGridResize(pOuter);

    if (_fDirty)
    {
        hr = Init(NULL);
        if (hr)
            goto Cleanup;
    }

    if (dwFlags & SITEMOVE_GRIDHORIZONTAL)
    {
        _fResizeOther = FALSE;
        hr = Calculate(edgeLeft, dwFlags);
        if (hr)
            goto Cleanup;
        if (_fResizeOther)
        {
            /*
                We moved edges left/up so we have to do CalculateUp.
                First we set the coordinates to the proposed so no more
                resize happens then necessary, we'll restore them later
            */
            SetRectangles();
            hr = CalculateUp(edgeLeft, dwFlags);
            ResetRectangles();
            if (hr)
                goto Cleanup;
        }
    }
    if (dwFlags & SITEMOVE_GRIDVERTICAL)
    {
        _fResizeOther = FALSE;
        hr = Calculate(edgeTop, dwFlags);
        if (hr)
            goto Cleanup;
        if (_fResizeOther)
        {
            /*
                We moved edges left/up so we have to do CalculateUp.
                First we set the coordinates to the proposed so no more
                resize happens then necessary, we'll restore them later
            */
            SetRectangles();
            hr = CalculateUp(edgeTop, dwFlags);
            ResetRectangles();
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:

    RemoveFakeFrames();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::MoveSites
//
//  Synopsis:   Gor thru all the sites in the matrix and move them to proposed
//              position
//
//  Arguments:  fDoOutsideIn    TRUE if to go inside
//              [dwFlags] -- Specifies flags
//
//                      SITEMOVE_NOFIREEVENT  -- Don't fire a move event
//                      SITEMOVE_NOINVALIDATE -- Don't invalidate the rect
//                      SITEMOVE_NOSETEXTENT  -- Don't call SetExtent on the
//                                                object
//
//
//    Returns:    Success if moved all sites
//
//----------------------------------------------------------------------------
HRESULT
CMatrix::MoveSites(DWORD dwFlags)
{
    HRESULT hr=S_OK;

    CFrame * pFrame = _aryFrames;
    CFrame * pFrameEnd = pFrame + _aryFrames.Size();

    /*
        Go thru the Frames and move them if proposed position
        changed
    */

    for (; pFrame < pFrameEnd; pFrame++)
    {
        if (pFrame->_pSite != _pOuter)
        {
            hr = THR(pFrame->MoveToProposed(dwFlags));
            Assert(hr == 0);
        }
    }

    _fDirty = TRUE;

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::EdgeTouched
//
//  Synopsis:   Find the edge of the rectangle we want to touch with
//              approaching site from the passed position
//
//  Arguments:  prcl        Pointer to rect
//              ptl         Position of cursor
//
//
//    Returns:    Edge to use
//
//----------------------------------------------------------------------------

const int VERTICALRATIO = 3;    // the ratio of the height of rectangles inside

Edge
CMatrix::EdgeTouched(CRectl * prcl, POINTL ptl)
{
    Edge e;

    /*
        We set up a rectangular region on the top and bottom edge which will
        restrict the vertical snapping to those when the cursor is inside

        *********************
        *  ***************  *
        *     *********     *
        *        ***        *
        *                   *
        *                   *
        *                   *
        *                   *
        *        ***        *
        *     *********     *
        *  ***************  *
        *********************

             (prcl->bottom - prcl->top)/VERTICALRATIO
        dy = ---------------------------------------- * (ptl.x - (*prcl) [v])
             (prcl->right - prcl->left)/2

        ptl.y - (*prcl) [h] < dy ?

        multiply with the dividend

        (ptl.y - (*prcl) [h]) * (prcl->right - prcl->left) * VERTICALRATIO <
            (prcl->bottom - prcl->top) * (ptl.x - (*prcl) [v]) * 2

    */

    Edge eL, eT, eR, eB;
    BOOL f;

    Assert(prcl->PtInRect(ptl));

    if (ptl.x * 2 < prcl->left + prcl->right)
    {
        eL = edgeLeft;
        eR = edgeRight;
        f = 1;
    }
    else
    {
        eL = edgeRight;
        eR = edgeLeft;
        f = 0;
    }
    if (ptl.y * 2 < prcl->top + prcl->bottom)
    {
        eT = edgeTop;
        eB = edgeBottom;
    }
    else
    {
        eT = edgeBottom;
        eB = edgeTop;
        f ^= 1;
    }

    if (((ptl.y - (*prcl) [eT]) * ((*prcl) [eR] - (*prcl) [eL]) * VERTICALRATIO <
        (ptl.x - (*prcl) [eL]) * ((*prcl) [eB] - (*prcl) [eT]) * 2) == f)
    {
        e = eT;
    }
    else
    {
        e = eL;
    }

    return e;
}

enum Sector
{
    sectorLeft = 1,
    sectorTop = 2,
    sectorRight = 4,
    sectorBottom = 8
};

/*
    This table will give us the edge the point is compared
    to a rectangle. The 'sectors' are arranged as the following:

     3|2|6
    ------
     1|0|4
    --------
     9|8|12
*/

Edge eLookup [16] =
{
    edgeInvalid,    // inside
    edgeLeft,
    edgeTop,
    edgeInvalid,
    edgeRight,
    edgeInvalid,
    edgeInvalid,
    edgeInvalid,
    edgeBottom,
    edgeInvalid,
    edgeInvalid,
    edgeInvalid,
    edgeInvalid,
    edgeInvalid,
    edgeInvalid,
    edgeInvalid,
};



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::FindMovedRect
//
//  Synopsis:   Find out where the moved site rectangle will snap to and
//              how big the insertion (starting size) rectangle will be
//              too resize from after inserting it in
//
//  Arguments:  pSite           Pointer to the site moved
//              ptlMoveTo       new top left position
//                ptlCursor       cursor position
//              ptlLastCursor   last cursor position
//              prclMoved       insertion rect (out)
//
//
//    Returns:    Success if found position
//
//----------------------------------------------------------------------------
// find where site would move
HRESULT
CMatrix::FindMovedRect
(
    CSite * pSiteMoved,     // site to move
    POINTL ptlMoveTo,       // new rect topleft
    POINTL ptlCursor,       // cursor position
    POINTL ptlLastCursor,   // last cursor position
    CRectl * prclMoved       // insertion rect (out)
)
{
    HRESULT hr = S_OK;

    CFrame * pFrame;
    CFrame * pFrameEnd = _aryFrames + _aryFrames.Size();
    CRectl rclM;    // moved rect
    CSizel sizelM;   // moved size
    CRectl rclI;    // intersect rect
    CRectl rclS;    // site rect
    Edge e;         // snapped edge
    unsigned uSector;
    Edge eH, eV;    // edges to check for collision
    int dx, dy;
    int x, y;
    CSite * pParent;
    BOOL fChanged[4];   // mark when rclM is changed
    BOOL fAgain = FALSE;

    sizelM = pSiteMoved->_rcl.Size();
    rclM.left = ptlMoveTo.x;
    rclM.top = ptlMoveTo.y;
    rclM.right = rclM.left + sizelM.cx;
    rclM.bottom = rclM.top + sizelM.cy;

    dx = ptlCursor.x - ptlLastCursor.x;
    dy = ptlCursor.y - ptlLastCursor.y;
#if DBG == 1
    int c = 0;
#endif

Again:
#if DBG == 1
    c++;
    Assert(c < 4);
#endif
    fChanged [0] = fChanged [1] = fChanged [2] = fChanged [3] = FALSE;
    /*
        Don't allow to go outside parent...
    */
    pParent = pSiteMoved->_pParent;
    if (rclM.left < pParent->_rcl.left)
    {
        rclM.left = pParent->_rcl.left;
        fChanged [edgeLeft] = TRUE;
    }
    else if (rclM.left > pParent->_rcl.right)
    {
        rclM.left = pParent->_rcl.right;
        fChanged [edgeLeft] = TRUE;
    }
    if (rclM.top < pParent->_rcl.top)
    {
        rclM.top = pParent->_rcl.top;
        fChanged [edgeTop] = TRUE;
    }
    else if (rclM.top > pParent->_rcl.bottom)
    {
        rclM.top = pParent->_rcl.bottom;
        fChanged [edgeTop] = TRUE;
    }
    if (rclM.right > pParent->_rcl.right)
    {
        rclM.right = pParent->_rcl.right;
        fChanged [edgeRight] = TRUE;
    }
    else if (rclM.right < pParent->_rcl.left)
    {
        rclM.right = pParent->_rcl.left;
        fChanged [edgeRight] = TRUE;
    }
    if (rclM.bottom > pParent->_rcl.bottom)
    {
        rclM.bottom = pParent->_rcl.bottom;
        fChanged [edgeBottom] = TRUE;
    }
    else if (rclM.bottom < pParent->_rcl.top)
    {
        rclM.bottom = pParent->_rcl.top;
        fChanged [edgeBottom] = TRUE;
    }

    /*
        Go thru sites and check for intersection
    */
    for (pFrame = _aryFrames; pFrame < pFrameEnd; pFrame++)
    {
        if (pFrame->_pSite == pSiteMoved)
            continue;
        if (pFrame->_pSite == _pOuter)
            continue;
        if (rclI.Intersect(&rclM, &pFrame->_rcl))
        {
            rclS = pFrame->_rcl;
            uSector = (ptlCursor.x < rclS.left) | ((ptlCursor.y < rclS.top) << 1) |
                ((ptlCursor.x > rclS.right) << 2) | ((ptlCursor.y > rclS.bottom) << 3);
            if (uSector == 0)
            {
                /*
                    If cursor is inside this site that takes precedense
                */
                e = EdgeTouched(&rclS, ptlCursor);
            }
            else
            {
                e = pFrame->_edgeTouched;
                /*
                    If touched edge is set we use the same edge unless
                    cursor moved to other side
                */
                if (e == edgeInvalid || (uSector & (1 << e)) == 0)
                {
                    /*
                        Check if cursor is right along of an edge of the site
                    */
                    e = eLookup [uSector];
                    if (e == edgeInvalid)
                    {
                        /*
                            Let's calculate the intersection of the edges closest
                            to the site
                        */
                        eV = (ptlLastCursor.x < rclS.left) ? edgeRight :
                            (ptlLastCursor.x > rclS.right) ? edgeLeft : edgeInvalid;
                        eH = (ptlLastCursor.y < rclS.top) ? edgeBottom :
                            (ptlLastCursor.y > rclS.bottom) ? edgeTop : edgeInvalid;
                        /*
                            The formula for a line going from the old corner to
                            the new corner and a site edge is:

                            Xnew - Xold   Xi - Xnew
                            ----------- = ---------
                            Ynew - Yold   Yi - Ynew

                            If we want to calculate the X position for a given Yi:

                                        Xnew - Xold
                            Xi = Xnew + ----------- * (Yi - Ynew)
                                        Ynew - Yold

                            If we substitute Dx = Xnew - Xold, Dy = Ynew - Yold which we
                            get by substracting the old cursor pos from the new cursor pos
                                        Dx
                            Xi = Xnew + -- * (Yi - Ynew)
                                        Dy

                            For an Yi
                                        Dy
                            Yi = Ynew + -- * (Xi - Xnew)
                                        Dx
                        */
                        if (eV != edgeInvalid && dx)
                        {
                            y = rclM [edgeTop] + (int)((((LONGLONG)rclS [OPPOSITE(eV)] -
                                (LONGLONG)rclM [eV]) * dy) / dx);
                            if (y <= rclS [edgeBottom] && y + sizelM.cy >= rclS [edgeTop])
                                e = OPPOSITE(eV);
                        }
                        if (eH != edgeInvalid && dy)
                        {
                            x = rclM [edgeLeft] + (int)((((LONGLONG)rclS [OPPOSITE(eH)] -
                                (LONGLONG)rclM [eH]) * dx) / dy);
                            if (x <= rclS [edgeRight] && x + sizelM.cx >= rclS [edgeLeft])
                                e = OPPOSITE(eH);
                        }
                        if (e == edgeInvalid)
                        {
                            e = rclI.right - rclI.left < rclI.bottom - rclI.top ?
                                OPPOSITE(eV) : OPPOSITE(eH);
                        }
                    }
                }
            }
            Assert (e != edgeInvalid);
            pFrame->_edgeTouched = e;
            if (e != edgeInvalid)
            {
                rclM [OPPOSITE(e)] = rclS [e];
                fChanged [OPPOSITE(e)] = TRUE;
                if (rclM [(Edge)(e & 1)] > rclM [(Edge)((e & 1) + 2)])
                {
                    rclM [e] = rclM [OPPOSITE(e)];
                    fChanged [e] = TRUE;
                }
            }
        }
        else    // reset touched edge
        {
            if (!fAgain)
                pFrame->_edgeTouched = edgeInvalid;
        }
    }

    /*
        Don't shrink left and top
    */
    if (fChanged [edgeLeft] && !fChanged [edgeRight])
    {
        rclM.right = rclM.left + sizelM.cx;
        fAgain = TRUE;
        goto Again;
    }
    if (fChanged [edgeTop] && !fChanged [edgeBottom])
    {
        rclM.bottom = rclM.top + sizelM.cy;
        fAgain = TRUE;
        goto Again;
    }

    *prclMoved = rclM;

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::AddFakeFrameForMove
//
//  Synopsis:   Add a fake frame to the old position of the site and move
//              the site to the new position
//
//  Arguments:  pFrame      frame of the site moved
//
//    Returns:    Success if frame added
//
//----------------------------------------------------------------------------
HRESULT
CMatrix::AddFakeFrameForMove(CFrame * pFrame)
{
    HRESULT hr;
    int i;

    /*
        Create fake frame to be at the OLD position of the site moved...
    */
    CFrame frm(NULL);
    i = _aryFrames.Size();
    if (_pOuter)
    {
        i--;
    }
    frm._rcl = pFrame->_rcl;
    /*
        Ask fake frame to shrink to 0 size
    */
    frm._rclPropose.left = frm._rclPropose.right = frm._rcl.left;
    // don't shrink vertical direction yet...
//  frm._rclPropose.top = frm._rclPropose.bottom = frm._rcl.top;
    frm._rclPropose.top = frm._rcl.top;
    frm._rclPropose.bottom = frm._rcl.bottom;

    // set move mask so it doesn't push pFrame
    frm._uMoveMask = 1;

    hr = _aryFrames.InsertIndirect(i, &frm);
    if (hr)
        goto Cleanup;

    /*
        Now put the site at the new position
    */
    pFrame->_rcl = pFrame->_rclPropose;
    /*
        Ask pSite to grow right and down
    */
    pFrame->_rclPropose.right = pFrame->_rclPropose.left + pFrame->_pSite->_rcl.right - pFrame->_pSite->_rcl.left;
    pFrame->_rclPropose.bottom = pFrame->_rclPropose.top + pFrame->_pSite->_rcl.bottom - pFrame->_pSite->_rcl.top;
    pFrame->_fIsDirtyRectangle = TRUE;

    // set move bits so fake frame doesn't push it
    pFrame->_uMoveBits = BITMASK ^ frm._uMoveMask;

    _fDirty = TRUE;
    _fHasFake = TRUE;

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::RemoveFakeFrames
//
//  Synopsis:   Remove all the fake frames added
//
//
//----------------------------------------------------------------------------
void
CMatrix::RemoveFakeFrames()
{
    if (_pOuter)
    {
        CFrame * pFrameOuter = FindSite(_pOuter);
        Assert(pFrameOuter);
#if 0   // BUGBUG to fix temporary disabled
        /*
            Make sure the edges of the outer are enclosing the sites inside
        */
        CFrame * pFrame = _aryFrames;
        CFrame * pFrameEnd = pFrame + (_aryFrames.Size() - 1);

        for (; pFrame < pFrameEnd; pFrame++)
        {
            if (pFrame->_rclPropose.right > pFrameOuter->_rclPropose.right)
            {
                pFrameOuter->_rclPropose.right = pFrame->_rclPropose.right;
            }
            if (pFrame->_rclPropose.bottom > pFrameOuter->_rclPropose.bottom)
            {
                pFrameOuter->_rclPropose.bottom = pFrame->_rclPropose.bottom;
            }
        }
#endif
        _pOuter->SetProposed(_pOuter, &pFrameOuter->_rclPropose);
    }
    if (_fHasFake)
    {
        for (int i = _aryFrames.Size() - 1; i >= 0; i--)
        {
            if (_aryFrames[i]._pSite == NULL)
            {
                _aryFrames.Delete(i);
            }
            else
            {
                _aryFrames[i]._fMarkForMove = FALSE;
            }
        }
        _fDirty = TRUE;
        _fHasFake = FALSE;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CMatrix::MoveRelatedCells
//
//  Synopsis:   walks over the detail sites and checks if they should have
//              a header. Assumes that the caller setup the _prelated pointers
//
//  Arguments:  pHeader:    the related header template
//
//              pBitAry     the BitAry to fill
//
//----------------------------------------------------------------------------
HRESULT CMatrix::MoveRelatedCells(CDetailFrame *pRelated, BOOL fVertical)
{

    Assert(pRelated);


    HRESULT  hr=S_OK;

    CBitAry  bitAry;
    CSite   *pSiteRelated;
    Edge    e = (Edge) (1 - fVertical);
    Assert(e == edgeLeft || e == edgeTop);
    Edge    eOther = (Edge)((e + 1) & 1);
    Edge    eOpposite = (Edge) (e+2);
    Edge    eOtherOp = (Edge) (eOther+2);
    CRectl  rclProp;
    CRectl  rclPropParent;
    CRectl  rcl;
    CFrame *pFrame;
    int     iBit;
    BOOL    fOccupied;



    if (_aryFrames.Size())
    {
        bitAry.SetSize(_uCols);

        hr = pRelated->GetProposed(pRelated, &rclPropParent);
        if (hr)
            goto Error;

        for (int i=0; i <(int)_uEdges;i++)
        {
            if (_pceHorizontal[i].Side() != eOther )
            {
                continue;
            }
            pFrame = _pceHorizontal[i].Frame();

            if (pFrame->_pSite == _pOuter)
            {
                continue;
            }

            pSiteRelated = ((COleDataSite*)(pFrame->_pSite))->TBag()->_pRelated;


            if (pSiteRelated)
            {

                for (fOccupied = FALSE, iBit = pFrame->_rclGrid[e]; iBit < pFrame->_rclGrid[eOpposite];iBit++)
                {
                    fOccupied = bitAry[iBit];
                    if (fOccupied)
                    {
                        break;
                    }
                }

                if (!fOccupied)
                {

                    // so there is free space, let's move the related cell there
                    hr = pFrame->_pSite->GetProposed(pFrame->_pSite, &rcl);
                    if (hr)
                        goto Error;

                    pSiteRelated->GetProposed(pSiteRelated, &rclProp);

                    rclProp[eOther]     = rclPropParent[eOther];
                    rclProp[eOtherOp]   = rclPropParent[eOtherOp];
                    rclProp[e]          = rcl[e];
                    rclProp[eOpposite]  = rcl[eOpposite];

                    // now set the bits in the array - all the way through
                    for (iBit = pFrame->_rclGrid[e]; iBit< pFrame->_rclGrid[eOpposite];iBit++)
                    {
                        bitAry.Set(iBit);
                    }
                }
                else
                {
                    // space is occupied, get rid of this guy -> move him in nirwana
                    rclProp[eOther] = rclPropParent[eOther] - 0xffff;
                    rclProp[eOtherOp]   = rclPropParent[eOtherOp] - 0xffff;
                }

                hr = pRelated->SetProposed(pRelated, &rclProp);
                if (hr)
                    goto Error;
                pSiteRelated->_fProposedSet = TRUE;
            }
        }
    }
Error:
    RRETURN(hr);

}
//-end-of-method--------------------------------------------------------------



#if DBG==1
ULONG CMatrix::AddRef(void)
{
    return ++_ulRefs;
}


ULONG CMatrix::Release(void)
{
    if (--_ulRefs == 0)
    {
        _ulRefs++;
        delete this;
        return 0;
    }
    return _ulRefs;
}

#endif // DBG


#endif PRODUCT_97




