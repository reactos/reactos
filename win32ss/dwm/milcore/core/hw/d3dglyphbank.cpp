// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Implementation of classes to serve HW text rendering.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#define MAX_TANK_WIDTH 2048
#define MAX_TANK_HEIGHT 256
#define MAX_TANK_NUM 10

MtDefine(CD3DGlyphBank, CD3DDeviceLevel1, "CD3DGlyphBank");
MtDefine(D3DResource_GlyphTank, MILHwMetrics, "Approximate glyph tank size");
MtDefine(D3DResource_GlyphBankTempSurface, MILHwMetrics, "Approximate glyph bank temporary surface size");

#define GLYPHRUNCOUNT_THRESHOLD 20

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::Init
//
//  Synopsis:
//      Initialize the instance. Store given pointers to CD3DDeviceLevel1 and
//      CD3DResourceManager.
//
//  Note:
//      m_pDevice and m_pResourceManager are never changed after this call.
//      However we can't declare them "const" because the instance of the bank
//      is the member of CD3DDeviceLevel1 and should be constructed first.
//      Compiler complaints against using uninitialized objects so we can't pass
//      CD3DDeviceLevel1* to CD3DGlyphBank constructor.
//
//------------------------------------------------------------------------------
HRESULT
CD3DGlyphBank::Init(
    __in_ecount(1) CD3DDeviceLevel1* pDevice,
    __in_ecount(1) CD3DResourceManager *pResourceManager
    )
{
    HRESULT hr = S_OK;

    m_pDevice = pDevice;  // No AddRef because it would be a circular reference.
    m_pResourceManager = pResourceManager;

    m_uMaxTankWidth = pDevice->GetMaxTextureWidth();
    if (m_uMaxTankWidth > MAX_TANK_WIDTH)
        m_uMaxTankWidth = MAX_TANK_WIDTH;

    m_uMaxTankHeight = pDevice->GetMaxTextureHeight();
    if (m_uMaxTankHeight > MAX_TANK_HEIGHT)
        m_uMaxTankHeight = MAX_TANK_HEIGHT;

    m_maxSubGlyphWid = m_uMaxTankWidth - m_uMaxTankWidth%3;

    if (m_uMaxTankWidth < 256 || m_uMaxTankHeight < 64) {hr = E_FAIL; goto Cleanup;}

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::~CD3DGlyphBank
//
//  Synopsis:
//      destructor
//
//------------------------------------------------------------------------------
CD3DGlyphBank::~CD3DGlyphBank()
{
    // Don't release m_pDevice since it's not AddRef'd.

    //
    // Release tanks
    //
    // Do not DestroyAndRelease them since the resource manager is responsible
    // for that during cleanup.
    //

    while (m_pTanks)
    {
        CD3DGlyphTank* pTank = m_pTanks;
        m_pTanks = pTank->m_pNext;
        pTank->Release();
        D3DLOG_INC(tanksDestroyedOnDestruction)
    }

    if (m_pTempTank)
    {
        m_pTempTank->Release();
        D3DLOG_INC(tmpTanksDestroyed)
    }

    if (m_pTempSurface)
    {
        m_pTempSurface->Release();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::CollectGarbage
//
//  Synopsis:
//      Release the memory that we don't need much.
//
//------------------------------------------------------------------------------
void
CD3DGlyphBank::CollectGarbage()
{
    ReleaseStubs();
    ReleaseLazyTanks();

    for (CD3DGlyphTank* pTank = m_pTanks; pTank; pTank = pTank->m_pNext)
    {
        pTank->NewFrame();
    }
    D3DLOG_SET(tanksTotal, CountTanks())

    m_glyphPainterMemory.CleanHuge();

    // Free system memory occupied by m_pTempSurface,
    // if it is "too big" (yet another heuristic).
    if (m_pTempSurface != NULL && m_pTempSurface->IsExpensive())
    {
        m_pTempSurface->DestroyAndRelease();
        m_pTempSurface = NULL;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::AllocRect
//
//  Synopsis:
//      Allocate rectangular space for single subglyph
//
//------------------------------------------------------------------------------
HRESULT
CD3DGlyphBank::AllocRect(
    UINT uWidth,
    UINT uHeight,
    BOOL fPersistent,
    __deref_out_ecount(1) CD3DGlyphTank** ppTank,
    __out_ecount(1) POINT* pptLocation
    )
{
    HRESULT hr = S_OK;

    if (!fPersistent)
    {
        if (!m_pTempTank || m_pTempTank->GetHeight() < uHeight)
        {
            IFC( CreateTank(RoundToPow2(uHeight), FALSE));
        }
        Assert(m_pTempTank);
        MIL_THR(m_pTempTank->AllocRect(uWidth, uHeight, pptLocation));
        Assert(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            *ppTank = m_pTempTank;
        }
        goto Cleanup;
    }

    // check whether we have available "current" tank
    while (m_pTanks && !m_pTanks->IsValid())
    {
        CD3DGlyphTank* pTank = m_pTanks;
        m_pTanks = pTank->m_pNext;
        pTank->Release();
        D3DLOG_INC(stubsDestroyed)
    }

    if (m_pTanks)
    {
        // yes we have; try to allocate inside it
retry:
        hr = m_pTanks->AllocRect(uWidth, uHeight, pptLocation);
        if (SUCCEEDED(hr))
        {
            *ppTank = m_pTanks;
            goto Cleanup;
        }
        else
        {
            UINT uTankHeight = m_pTanks->GetHeight();
            if (uTankHeight != m_uMaxTankHeight)
            {
                CD3DGlyphTank* pTank = m_pTanks;
                m_pTanks = pTank->m_pNext;
                pTank->DestroyAndRelease();
                D3DLOG_INC(smallPersTanksDestroyed)
                uTankHeight *= 2;
                if (uTankHeight < uHeight) uTankHeight = RoundToPow2(uHeight);
                IFC(CreateTank(uTankHeight, TRUE));
                goto retry;
            }
        }
    }

    // need to create another tank.
    // We permitted to handle MAX_TANK_NUM tanks.

    ReleaseStubs();

    CD3DGlyphTank* pTankReuse = NULL;

    if (CountTanks() == MAX_TANK_NUM)
    {
        // We already have allocated too many tanks and so need
        // to throw away one of them.
        // Here can be different heuristics;
        // the currently used one chooses the least loaded tank.
        CD3DGlyphTank** ppVictimTank = &m_pTanks;
        UINT uMinLoad = m_pTanks->GetUsefulLoad();

        for (CD3DGlyphTank** pp = &m_pTanks->m_pNext; *pp; pp = &(*pp)->m_pNext)
        {
            UINT uLoad = (*pp)->GetUsefulLoad();
            if (uLoad < uMinLoad)
            {
                uMinLoad = uLoad;
                ppVictimTank = pp;
            }
        }

        // Destroy choosen tank.
        CD3DGlyphTank* pVictimTank = *ppVictimTank;
        *ppVictimTank = pVictimTank->m_pNext;
        pTankReuse = pVictimTank->StubifyForReuseAndRelease();
        D3DLOG_INC(tanksReused)
    }

    if (pTankReuse)
    {
        pTankReuse->AddRef();
        if (pTankReuse->GetHeight() < uHeight)
        {   // We can't reuse this tank because it is too small.
            // Destroy it.
            pTankReuse->DestroyAndRelease();
            D3DLOG_INC(smallReuseTanksDestroyed)
            pTankReuse = 0;
        }
    }

    if (pTankReuse)
    {
        pTankReuse->m_pNext = m_pTanks;
        m_pTanks = pTankReuse;
    }
    else
    {
        UINT uHeightToAlloc;

        // Heuristic: if there exists another tank, or
        // we have already many glyphruns in scope, then don't
        // waste the time by allocating small tanks.
        if (m_pTanks || g_uiCMILGlyphRunCount > GLYPHRUNCOUNT_THRESHOLD)
            uHeightToAlloc = m_uMaxTankHeight;
        else
            uHeightToAlloc = RoundToPow2(uHeight);

        IFC(CreateTank(uHeightToAlloc, TRUE));
    }

    // Now we have new, perfectly empty tank

    MIL_THR(m_pTanks->AllocRect(uWidth, uHeight, pptLocation));
    Assert(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
        *ppTank = m_pTanks;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::RectFillAlpha
//
//  Synopsis:
//      Fill rectangular space in shared glyph bank storage with given alpha
//      data. Given pSrcData and fullDataRect represent the whole data array,
//      where pSrcData[0] corresponds to left top corner of fullDataRect and
//      pitch = fullDataRect's width. Only part of these data is moved to
//      destination; this part is limited with srcRect. The srcRect not
//      necessary lays inside fullDataRect; if some texels of srcRect are out of
//      fullDataRect, then zeroes are moved to corresponding destination texels.
//
//------------------------------------------------------------------------------
HRESULT CD3DGlyphBank::RectFillAlpha(
    __inout_ecount(1) CD3DGlyphTank* pTank,
    __in_ecount(1) const POINT& dstPoint,
    __in_ecount(1) const BYTE* pSrcData,
    __in_ecount(1) const RECT& fullDataRect,
    __in_ecount(1) const RECT& srcRect
    )
{
    HRESULT hr = S_OK;
    D3DLOCKED_RECT lockedRect;

    UINT uWidth = srcRect.right - srcRect.left;
    UINT uHeight = srcRect.bottom - srcRect.top;

    RECT rcTemp = {0, 0, uWidth, uHeight};
    IDirect3DSurface9* pTankSurface = pTank->GetSurfaceNoAddref();
    IDirect3DSurface9* pTempSurface = NULL;

    IFC( EnsureTempSurface(uWidth, uHeight, &pTempSurface) );

    int srcPitch = fullDataRect.right - fullDataRect.left;

    IFC(pTempSurface->LockRect(&lockedRect, &rcTemp, 0 ) );

    BYTE *pDst00 = (BYTE*)lockedRect.pBits
                 - lockedRect.Pitch*srcRect.top
                 - srcRect.left;

    // pDst00 points to the texel in destination that corresponds
    // to point (x,y) = (0,0) in source array

    const BYTE *pSrc00 = pSrcData
                        - srcPitch*fullDataRect.top
                        - fullDataRect.left;
    // pSrc00 points to (x,y) = (0,0) in given data array

    int y;
    // fill top edge
    for (y = srcRect.top; y < fullDataRect.top; y++)
    {
        BYTE *pDstRow = pDst00 + lockedRect.Pitch*y;
        memset(pDstRow + srcRect.left, 0, srcRect.right - srcRect.left);
    }

    // fill real data and side edges
    int ymax = min(fullDataRect.bottom, srcRect.bottom);
    int xmin = max(fullDataRect.left,   srcRect.left  );
    int xmax = min(fullDataRect.right,  srcRect.right );
    if (xmax > xmin)
    {
        for (; y < ymax; y++)
        {
                  BYTE* pDstRow = pDst00 + lockedRect.Pitch*y;
            const BYTE* pSrcRow = pSrc00 +         srcPitch*y;

            memset(pDstRow + srcRect.left, 0, xmin - srcRect.left);
            memcpy(pDstRow + xmin, pSrcRow + xmin, xmax - xmin);
            memset(pDstRow + xmax, 0, srcRect.right - xmax);
        }
    }
    else
    {
        // degenerate case: srcRect and fullDataRect have empty intersection
        for (; y < ymax; y++)
        {
            BYTE* pDstRow = pDst00 + lockedRect.Pitch*y;
            memset(pDstRow + srcRect.left, 0, srcRect.right - srcRect.left);
        }
    }

    // fill bottom edge
    for (; y < srcRect.bottom; y++)
    {
        unsigned char *pDstRow = pDst00 + lockedRect.Pitch*y;
        memset(pDstRow + srcRect.left, 0, srcRect.right - srcRect.left);
    }

    IFC( pTempSurface->UnlockRect() );

    Assert(pTankSurface);

    // Transfer data from system memory to video memory.
    IFC( m_pDevice->UpdateSurface(
        pTempSurface,
        &rcTemp,
        pTankSurface,
        &dstPoint
        ));

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::EnsureTempSurface
//
//  Synopsis:
//      Check whether temporary surface exists, is valid and big enough to serve
//      pumping data to video memory. Create new one if necessary.
//
//------------------------------------------------------------------------------
HRESULT CD3DGlyphBank::EnsureTempSurface(
    UINT uWidth,
    UINT uHeight,
    __deref_out_ecount(1) IDirect3DSurface9 **ppTempSurface
    )
{
    Assert(ppTempSurface);

    HRESULT hr = S_OK;
    IDirect3DTexture9 *pTexture = NULL;
    IDirect3DSurface9 *pSurface = NULL;

    if (m_pTempSurface == NULL
        || !m_pTempSurface->IsValid()
        || m_pTempSurface->GetWidth() < uWidth
        || m_pTempSurface->GetHeight() < uHeight
        )
    {
        D3DSURFACE_DESC sd;

        if (m_pTempSurface)
        {
            m_pTempSurface->DestroyAndRelease();
            m_pTempSurface = NULL;
        }

        uWidth  = RoundToPow2(uWidth);
        uHeight = RoundToPow2(uHeight);

        sd.Format = m_pDevice->GetAlphaTextureFormat();
        sd.Type = D3DRTYPE_TEXTURE;
        sd.Usage = 0;
        sd.Pool = D3DPOOL_SYSTEMMEM;
        sd.MultiSampleType = D3DMULTISAMPLE_NONE;
        sd.MultiSampleQuality = 0;
        sd.Width = uWidth;
        sd.Height = uHeight;

        IFC( m_pDevice->CreateTexture(&sd, 1, &pTexture) );

        IFC( pTexture->GetSurfaceLevel(0, &pSurface) );

        m_pTempSurface = new CD3DGlyphBankTemporarySurface(
            pSurface,
            uWidth,
            uHeight,
            m_pResourceManager
            );

        IFCOOM(m_pTempSurface);

        m_pTempSurface->AddRef();
    }

    *ppTempSurface = m_pTempSurface->GetSurfaceNoAddref();

Cleanup:
    ReleaseInterfaceNoNULL(pTexture);
    ReleaseInterfaceNoNULL(pSurface);
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::CreateTank
//
//  Synopsis:
//      Allocate new tank of given height and width == m_uMaxTankWidth
//
//------------------------------------------------------------------------------
HRESULT
CD3DGlyphBank::CreateTank(UINT uHeight, BOOL fPersistent)
{
    HRESULT hr = S_OK;

    Assert(uHeight <= m_uMaxTankHeight);
    AssertMsg(!(uHeight & (uHeight-1)), "CreateTank: height is not a power of two");

    IDirect3DSurface9 *pSurface = NULL;
    IDirect3DTexture9 *pTexture = NULL;
    CD3DGlyphTank* pTank = 0;

    D3DSURFACE_DESC sd;
    sd.Format = m_pDevice->GetAlphaTextureFormat();
    sd.Type = D3DRTYPE_TEXTURE;
    sd.Usage = 0;
    sd.Pool = D3DPOOL_DEFAULT;
    sd.MultiSampleType = D3DMULTISAMPLE_NONE;
    sd.MultiSampleQuality = 0;
    sd.Width = m_uMaxTankWidth;
    sd.Height = uHeight;

    IFC( m_pDevice->CreateTexture(&sd, 1, &pTexture) );

    IFC( pTexture->GetSurfaceLevel(0, &pSurface) );

    pTank = new CD3DGlyphTank(
        pTexture,
        pSurface,
        m_uMaxTankWidth,
        uHeight,
        m_pResourceManager
        );
    IFCOOM(pTank);

    pTank->AddRef();

    if (fPersistent)
    {
        Assert(CountTanks() < MAX_TANK_NUM);
        pTank->m_pNext = m_pTanks;
        m_pTanks = pTank;
    }
    else
    {
        if (m_pTempTank)
        {
            m_pTempTank->DestroyAndRelease();
            D3DLOG_INC(lazyTanksDestroyed)
        }
        m_pTempTank = pTank;
    }

    pTank = 0;
    D3DLOG_INC(tanksCreated);

Cleanup:
    ReleaseInterfaceNoNULL(pTank);
    ReleaseInterfaceNoNULL(pSurface);
    ReleaseInterfaceNoNULL(pTexture);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::ReleaseStubs
//
//  Synopsis:
//      Release the tanks that became stubified due to external reasons.
//
//------------------------------------------------------------------------------
void
CD3DGlyphBank::ReleaseStubs()
{
    for (CD3DGlyphTank** pp = &m_pTanks; *pp; )
    {
        CD3DGlyphTank* p = *pp;
        if (p->IsValid())
        {
            pp = &p->m_pNext;
        }
        else
        {
            *pp = p->m_pNext;
            p->Release();
            D3DLOG_INC(stubsDestroyed)
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBank::ReleaseLazyTanks
//
//  Synopsis:
//      Destroy (stubify) the tanks that are loaded ineffectively.
//
//  The heuristic implemented in this routine maybe not the best one, however it
//  appeared to reduce working set size for text editing scenario. 
// The overall approach assumes that tanks are never reused.
//  The tank is filled sequentially, run by run and stripe by stripe, in the
//  same sequence as rendering happens. This is made on purpose, in order to
//  reduce amount of IDirect3DDevice9::SetTexture() calls that are costly
//  (AshrafM's investigation says 100 switshing per frame are acceptable and
//  1000 too much). Even if some glyphruns in the tank are released, there are
//  no attempts to reuse freed space.
//
//  This routine fires at the moment of Present, i.e. at the end of each frame rendering.
//  It inspects each tank and releases it if it became filled loosely. The criteria
//  is when amount of released texels exceeds the half of amount of allocated texels.
//  It is possible to change condition below to
//        if (p->GetLostLoad()== p->GetPeakLoad())
//  so the tank will be released only when all the glyphruns will gone away -
//  but this appeared not to provide enough working set size reducing.
//  (9/5/2002 mikhaill)
//
//------------------------------------------------------------------------------
void
CD3DGlyphBank::ReleaseLazyTanks()
{
    // don't be aggressive for current tank
    if (!m_pTanks) return;

    for (CD3DGlyphTank** pp = &m_pTanks->m_pNext; *pp; )
    {
        CD3DGlyphTank* p = *pp;
        Assert(p->IsValid());

        if (p->GetLostLoad()*2 >= p->GetPeakLoad())
        {
            *pp = p->m_pNext;
            p->DestroyAndRelease();
            D3DLOG_INC(lazyTanksDestroyed)
        }
        else
        {
            pp = &p->m_pNext;
        }
    }
}

//-------------------------------------------------------- CD3DGlyphTank

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphTank::CD3DGlyphTank
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CD3DGlyphTank::CD3DGlyphTank(
    __in_ecount(1) IDirect3DTexture9* pTexture,
    __in_ecount(1) IDirect3DSurface9* pSurface,
    UINT widTank,
    UINT heiTank,
    __in_ecount(1) IMILPoolManager *pManager
    )
    : m_pTexture(pTexture), m_pSurface(pSurface)
{
    Assert(pTexture);
    Assert(pSurface);
    Assert(pManager);

    m_pTexture->AddRef();
    m_pSurface->AddRef();
    m_uWidth = widTank;
    m_uHeight = heiTank;
    m_rWidthReciprocal = 1.f/float(widTank);
    m_rHeightReciprocal = 1.f/float(heiTank);
    
    //DECLARE_METERHEAP_CLEAR assumed
    //m_uX = 0;
    //m_uY = 0;
    //m_uBandHeight = 0;
    //m_nPeakLoad = 0;
    //m_nLostLoad = 0;
    //m_nThisFrameLoad = 0;
    //m_nPrevFrameLoad = 0;
    //m_pNext = 0;

    InitResource(pManager);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphTank::StubifyForReuseAndRelease
//
//  Synopsis:
//      Create another instance of CD3DGlyphTank that will inherit D3D resources
//      of this one. Then destroy (stubify) this instance.
//
//------------------------------------------------------------------------------
CD3DGlyphTank*
CD3DGlyphTank::StubifyForReuseAndRelease()
{
    CD3DGlyphTank *pNewTank = NULL;

    if (IsValid())
    {
        Assert(m_pManager);
        pNewTank = new CD3DGlyphTank(m_pTexture, m_pSurface,
                                     m_uWidth, m_uHeight,
                                     m_pManager);
    }

    DestroyAndRelease();
    return pNewTank;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphTank::AllocRect
//
//  Synopsis:
//      Allocate rectangular space.
//
//------------------------------------------------------------------------------
HRESULT
CD3DGlyphTank::AllocRect(
    UINT uWidth,
    UINT uHeight,
    __out_ecount(1) POINT* pptLocation)
{
    if (uWidth > m_uWidth || uHeight > m_uHeight)
        return E_FAIL;

    if (m_uBandHeight)
    {   // 1. Try use current band
        if (m_uWidth - m_uX >= uWidth)
        {
            if (m_uBandHeight < uHeight && m_uHeight - m_uY >= uHeight)
            {   // band height is too small;
                // detect whether it would be reasonable to enlarge it
                if ((m_uWidth - m_uX)*m_uBandHeight > m_uX*(uHeight - m_uBandHeight))
                {   // enlarge
                    m_uBandHeight = uHeight;
                }
            }
            
            if (m_uBandHeight >= uHeight)
            {   // place in current band
                pptLocation->x = m_uX;
                pptLocation->y = m_uY;
                m_uX += uWidth;
                AddLoad(uWidth*uHeight);
                return S_OK;
            }
        }

        // close this band
        m_uY += m_uBandHeight;
        m_uBandHeight = 0;
        m_uX = 0;
    }

    {   // 2. Try to alloc another band
        if (m_uHeight - m_uY < uHeight)
        {   // fatal failure - not enough space in tank
            return E_FAIL;
        }

        m_uBandHeight = uHeight;
    }

    {   // 3. Use new band
        pptLocation->x = m_uX;
        pptLocation->y = m_uY;
        m_uX += uWidth;
        AddLoad(uWidth*uHeight);
        return S_OK;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphTank::FreeRect
//
//  Synopsis:
//      Free rectangular space obtained by AllocRect.
//
//  Basically, we don't care about reusing freed surface area, supposing that
//  detecting loosely populated tanks and re-populating them from scratch serves
//  better. However there is special case: temporary tank. It never contains
//  more than one allocated rectangle, and we need it to get really empty after
//  FreeRect(). To provide it, we are comparing the rectangle requested to free
//  with current allocation position defined by m_uX, m_uY and m_uBandHeight. If
//  the rectangle turns to be the only allocated one, all these variables become
//  zeros as if the tank was just created.
//
//------------------------------------------------------------------------------
void
CD3DGlyphTank::FreeRect(
    UINT uWidth,
    UINT uHeight,
    __out_ecount(1) POINT ptLocation
    )
{
    //
    // Check whether given rectangle is the last allocated one.
    // Last allocated should be most right in the current band.
    // See following codes in AllocRect:
    //    pptLocation->x = m_uX;
    //    pptLocation->y = m_uY;
    //    m_uX += uWidth;
    //
    if (static_cast<UINT>(ptLocation.x) + uWidth == m_uX &&
        static_cast<UINT>(ptLocation.y) == m_uY)
    {
        //
        // Given rectangle is the last allocated one,
        // so we can gracefully reclaim used memory.
        //
        m_uX -= uWidth;
        if (m_uX == 0)
        {
            // Current band became empty, so we can close it:
            m_uBandHeight = 0;
        }

        ReclaimLoad(uWidth*uHeight);
    }
    else
    {
        //
        // Given rectangle appeared to be somewhere inside tank surface;
        // don't care about reusing this space, just count lost area.
        SubLoad(uWidth*uHeight);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphTank::InitResource
//
//  Synopsis:
//      Inits the CD3DResource base class
//
//------------------------------------------------------------------------------
void
CD3DGlyphTank::InitResource(
    IMILPoolManager *pManager
    )
{
    //
    // Possible texture formats are D3DFMT_A8, D3DFMT_L8 and D3DFMT_P8
    // (see CD3DRenderState::InitAlphaTextures()); all of them spend
    // 1 byte per pixel so we need not multiply by pixel size.
    //
    UINT uResourceSize = m_uWidth * m_uHeight;

    //
    // Init the base class
    //

    CD3DResource::Init(pManager, uResourceSize);

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphTank::ReleaseD3DResources
//
//  Synopsis:
//      Release video memory
//
//      This method may only be called by CD3DResourceManager and potentially
//      the desctructor because there are various restrictions around when a
//      release is okay.
//
//------------------------------------------------------------------------------
void CD3DGlyphTank::ReleaseD3DResources()
{
    // This resource should have been marked invalid already or at least be out
    // of use.
    Assert(!m_fResourceValid || (m_cRef == 0));
    Assert(IsValid() == m_fResourceValid);

    // This context is protected so it is safe to release the D3D resources
    ReleaseInterface((*const_cast<IDirect3DSurface9 **>(&m_pSurface)));
    ReleaseInterface((*const_cast<IDirect3DTexture9 **>(&m_pTexture)));

    return;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBankTemporarySurface::CD3DGlyphBankTemporarySurface
//
//  Synopsis:
//      Constructor.
//
//------------------------------------------------------------------------------
CD3DGlyphBankTemporarySurface::CD3DGlyphBankTemporarySurface(
    __in_ecount(1) IDirect3DSurface9* pSurface,
    UINT uWidth,
    UINT uHeight,
    __in_ecount(1) IMILPoolManager *pManager
    )
    : m_pSurface(pSurface), m_uWidth(uWidth), m_uHeight(uHeight)
{
    Assert(pSurface);
    Assert(pManager);

    m_pSurface->AddRef();

    CD3DResource::Init(pManager, uWidth * uHeight);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBankTemporarySurface::~CD3DGlyphBankTemporarySurface
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DGlyphBankTemporarySurface::~CD3DGlyphBankTemporarySurface()
{
    ReleaseD3DResources();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphBankTemporarySurface::ReleaseD3DResources
//
//  Synopsis:
//      Release video memory
//
//------------------------------------------------------------------------------
void CD3DGlyphBankTemporarySurface::ReleaseD3DResources()
{
    ReleaseInterface((*const_cast<IDirect3DSurface9 **>(&m_pSurface)));
}



