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
//      Definitions for the classes to serve HW text rendering.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CD3DGlyphBank);
MtExtern(D3DResource_GlyphTank);
MtExtern(D3DResource_GlyphBankTempSurface);

class CD3DDeviceLevel1;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DGlyphBankTemporarySurface
//
//  Synopsis:
//      Holds a D3DPOOL_SYSTEMMEM surface that's used to pump data to glyph
//      tanks.
//
//------------------------------------------------------------------------------
class CD3DGlyphBankTemporarySurface : public CD3DResource
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CD3DGlyphBank));

    CD3DGlyphBankTemporarySurface(
        __in_ecount(1) IDirect3DSurface9* pSurface,
        UINT uWidth,
        UINT uHeight,
        __in_ecount(1) IMILPoolManager *pManager
        );

    ~CD3DGlyphBankTemporarySurface();

    UINT GetWidth() const {return m_uWidth;}
    UINT GetHeight() const {return m_uHeight;}
    IDirect3DSurface9* GetSurfaceNoAddref() const {return m_pSurface;}

    bool IsExpensive() const
    {
        static const UINT sc_uCriticalHeight = 32;
        return m_uHeight > sc_uCriticalHeight;
    }

private:
    virtual void ReleaseD3DResources();

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_GlyphBankTempSurface);
    }
#endif

private:
    IDirect3DSurface9 * const m_pSurface;
    UINT const m_uWidth;
    UINT const m_uHeight;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DGlyphTank
//
//  Synopsis:
//      The instance of CD3DGlyphTank is a wrapper of D3D texture. It serves as
//      a placeholder for glyph run shape data. Many glyph runs can share the
//      same tank, thus reducing texture switching expences.
//
//      Instances of CD3DGlyphTank belong to CD3DGlyphBank that holds a list of
//      several tanks.
//
//------------------------------------------------------------------------------
class CD3DGlyphTank : public CD3DResource
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CD3DGlyphBank));

    CD3DGlyphTank(
        __in_ecount(1) IDirect3DTexture9* pTexture,
        __in_ecount(1) IDirect3DSurface9* pSurface,
        UINT uTankWidth,
        UINT uTankHeight,
        __in_ecount(1) IMILPoolManager *pManager
        );
    ~CD3DGlyphTank() { ReleaseD3DResources(); }

    CD3DGlyphTank* StubifyForReuseAndRelease();
    
    HRESULT AllocRect(
        UINT uWidth,
        UINT uHeight,
        __out_ecount(1) POINT* pptLocation
        );

    void FreeRect(
        UINT uWidth,
        UINT uHeight,
        __out_ecount(1) POINT ptLocation
        );

    //
    // accessors
    //

    IDirect3DTexture9* GetTextureNoAddref() const {return m_pTexture;}
    IDirect3DSurface9* GetSurfaceNoAddref() const {return m_pSurface;}
    float GetWidTextureRc() const {return m_rWidthReciprocal;}
    float GetHeiTextureRc() const {return m_rHeightReciprocal;}
    UINT GetLoad() const {return m_nPeakLoad - m_nLostLoad;}
    UINT GetPeakLoad() const {return m_nPeakLoad;}
    UINT GetLostLoad() const {return m_nLostLoad;}

    UINT GetUsefulLoad() const {return m_nThisFrameLoad > m_nPrevFrameLoad ? m_nThisFrameLoad : m_nPrevFrameLoad;}
    void NewFrame() {m_nPrevFrameLoad = m_nThisFrameLoad; m_nThisFrameLoad = 0;}
    void AddUsefulArea(UINT d) {m_nThisFrameLoad += d;}

private:

    void InitResource(
        IMILPoolManager *pManager
        );

    // Should only be called by CD3DResourceManager (destructor is okay, too)
    virtual void ReleaseD3DResources();

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_GlyphTank);
    }
#endif

    void AddLoad(UINT n) { m_nPeakLoad += n; }
    void SubLoad(UINT n) { InterlockedExchangeAdd(&m_nLostLoad, n); }
    void ReclaimLoad(UINT n) { m_nPeakLoad -= n; }

public:
    MIL_FORCEINLINE UINT GetHeight() const {return m_uHeight;}
    MIL_FORCEINLINE UINT GetFilledHeight() const {return m_uY+m_uBandHeight;}

    // tank list
    CD3DGlyphTank* m_pNext;

private:

    // Pointers to the actual D3D resources.
    //  These pointers are constant to help enforce the modification
    //  restrictions of CD3DResource objects.
    IDirect3DTexture9 * const m_pTexture;
    IDirect3DSurface9 * const m_pSurface;

    UINT m_uWidth, m_uHeight;
    float m_rWidthReciprocal, m_rHeightReciprocal;

    // allocation variables
    UINT m_uX, m_uY, m_uBandHeight;

    // load count: shows how many pixels occupied by consumers
    // who called AllocRect. When the consumer detaches from
    // the tank, it should call CD3DGlyphTank::SubLoad(), then
    // CD3DGlyphTank::Release(). In particular, this can happen
    // when the consumer is being deleted, maybe on another
    // thread (managed garbage collector's one). Therefore it is
    // extremely important to make any changes of m_nLoad
    // only by interlocked AddLoad(n) and SubLoad() routines.

    volatile LONG m_nPeakLoad;
    volatile LONG m_nLostLoad;

    // Following two variables intended to estimate
    // "useful load" of the tank. The matter is that
    // some consumer can exist but actually not used for
    // scene rendering. In constrated video memory condition,
    // we need to choose "least useful" tank and evit it.
    // Current criteria chooses the tank with minimal
    // "useful load" that is nothing but amount of texels
    // used dirung frame rendering. As far as in the middle
    // of frame rendering we don't yet know what will happen
    // before finish, the "useful load" is calculated for
    // the current frame in m_nThisFrameLoad that is
    // then copied into m_nPrevFrameLoad at the end
    // of frame rendering

    LONG m_nThisFrameLoad;
    LONG m_nPrevFrameLoad;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DGlyphBank
//
//  Synopsis:
//      The instance of CD3DGlyphBank exists as CD3DDeviceLevel1 satellite. It
//      concentrates all the data required for text rendering, related not to
//      particular glyph run but to device, and that should be kept from frame
//      to frame.
//
//      CD3DGlyphBank manipulates with a number of CD3DGlyphTanks. Each tank is
//      a wrapper of D3D texture and serves as a placeholder for glyph run shape
//      data. Many glyph runs can share the same tank, thus reducing texture
//      switching expences.
//
//------------------------------------------------------------------------------
class CD3DGlyphBank
{
public:
    //CD3DGlyphBank(); no ctor - DECLARE_METERHEAP_CLEAR assumed
    ~CD3DGlyphBank();
    HRESULT Init(
        __in_ecount(1) CD3DDeviceLevel1*,
        __in_ecount(1) CD3DResourceManager*
        );

    void CollectGarbage();
    HRESULT AllocRect(
        UINT uWidth,
        UINT uHeight,
        BOOL fPersistent,
        __deref_out_ecount(1) CD3DGlyphTank** ppTank,
        __out_ecount(1) POINT* pptLocation
        );
    HRESULT RectFillAlpha(
        __inout_ecount(1) CD3DGlyphTank* pTank,
        __in_ecount(1) const POINT& dstPoint,
        __in_ecount(1) const BYTE* pSrcData,
        __in_ecount(1) const RECT& fullDataRect,
        __in_ecount(1) const RECT& srcRect
        );

    UINT GetMaxTankWidth() const { return m_uMaxTankWidth; }
    UINT GetMaxTankHeight() const { return m_uMaxTankHeight; }
    UINT GetMaxSubGlyphWidth() const { return m_maxSubGlyphWid; }
    UINT GetMaxSubGlyphHeight() const { return m_uMaxTankHeight; }

    CGlyphPainterMemory* GetGlyphPainterMemory()
    {
        return &m_glyphPainterMemory;
    }

private:
    HRESULT EnsureTempSurface(
        UINT uWidth,
        UINT uHeight,
        __deref_out_ecount(1) IDirect3DSurface9 **ppTempSurface
        );

private:
    friend class CD3DGlyphRunPainter;
    friend class CD3DGlyphTank;

    CD3DDeviceLevel1 *m_pDevice;

    CD3DResourceManager *m_pResourceManager;

    CD3DGlyphTank* m_pTanks;    // list of persistent tanks
    CD3DGlyphTank* m_pTempTank; // single tank for short-living runs

    UINT m_uMaxTankWidth;
    UINT m_uMaxTankHeight;
    UINT m_maxSubGlyphWid;

    void ReleaseStubs();
    void ReleaseLazyTanks();
    HRESULT CreateTank(UINT minHei, BOOL fPersistent);
    UINT CountTanks()
    {
        UINT n = 0;
        for (CD3DGlyphTank* p = m_pTanks; p; p = p->m_pNext) n++;
        return n;
    }

    CGlyphPainterMemory m_glyphPainterMemory;

    CD3DGlyphBankTemporarySurface *m_pTempSurface;
};



