#define DECLARE_MEMCLEAR_NEW_DELETE \
    void * __cdecl operator new(size_t cb) { void *pv = malloc(cb); if (pv) ZeroMemory(pv, cb); return pv; } \
    void __cdecl operator delete(void * pv) { free(pv); }

class CVoid
{
};

class CBaseFT : public CVoid
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE

    ULONG               AddRef()               { return((ULONG)InterlockedIncrement((LONG *)&_ulRefs)); }
    ULONG               Release();
    ULONG               SubAddRef()            { return((ULONG)InterlockedIncrement((LONG *)&_ulAllRefs)); }
    ULONG               SubRelease();
    CRITICAL_SECTION *  GetPcs() { return(_pcs); }
    void                SetPcs(CRITICAL_SECTION *pcs) { _pcs = pcs; }
    ULONG               GetRefs()              { return(_ulRefs); }
    ULONG               GetAllRefs()           { return(_ulAllRefs); }

    void                EnterCriticalSection() { if (_pcs) ::EnterCriticalSection(_pcs); }
    void                LeaveCriticalSection() { if (_pcs) ::LeaveCriticalSection(_pcs); }

protected:

                        CBaseFT(CRITICAL_SECTION * pcs = NULL);
    virtual            ~CBaseFT();
    virtual void        Passivate();
    ULONG               InterlockedRelease()   { return((ULONG)InterlockedDecrement((LONG *)&_ulRefs)); }

private:

    CRITICAL_SECTION *  _pcs;
    ULONG               _ulRefs;
    ULONG               _ulAllRefs;
};


class CDDrawWrapper : public CBaseFT, public IDirectDrawSurface, public IDirectDrawPalette
{
    typedef CBaseFT super;
    
public:
    CDDrawWrapper(HBITMAP hbmDib);
    ~CDDrawWrapper();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD(QueryInterface)(REFIID iid, void** ppInterface);

    // IDirectDrawSurface
    STDMETHOD(AddAttachedSurface)(LPDIRECTDRAWSURFACE lpdds);
    STDMETHOD(AddOverlayDirtyRect)(LPRECT lprc);
    STDMETHOD(Blt)(LPRECT lprcDest, LPDIRECTDRAWSURFACE lpdds, LPRECT lprcSrc, DWORD dw, LPDDBLTFX lpfx);
    STDMETHOD(BltBatch)(LPDDBLTBATCH lpBlt, DWORD dwCount, DWORD dwFlags);
    STDMETHOD(BltFast)(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpdds, LPRECT lprcSrc, DWORD dwTrans);
    STDMETHOD(DeleteAttachedSurface)(DWORD dwFlags, LPDIRECTDRAWSURFACE lpdds);
    STDMETHOD(EnumAttachedSurfaces)(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfn);
    STDMETHOD(EnumOverlayZOrders)(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfn);
    STDMETHOD(Flip)(LPDIRECTDRAWSURFACE lpdds, DWORD dwFlags);
    STDMETHOD(GetAttachedSurface)(LPDDSCAPS lpCaps, LPDIRECTDRAWSURFACE FAR * lpdds);
    STDMETHOD(GetBltStatus)(DWORD dw);
    STDMETHOD(GetCaps)(LPDDSCAPS lpCaps);
    STDMETHOD(GetClipper)(LPDIRECTDRAWCLIPPER FAR* lpClipper);
    STDMETHOD(GetColorKey)(DWORD dw, LPDDCOLORKEY lpKey);
    STDMETHOD(GetDC)(HDC FAR * lphdc);
    STDMETHOD(GetFlipStatus)(DWORD dw);
    STDMETHOD(GetOverlayPosition)(LPLONG lpl1, LPLONG lpl2);
    STDMETHOD(GetPalette)(LPDIRECTDRAWPALETTE FAR* ppPal);
    STDMETHOD(GetPixelFormat)(LPDDPIXELFORMAT pPixelFormat);
    STDMETHOD(GetSurfaceDesc)(LPDDSURFACEDESC pSurfaceDesc);
    STDMETHOD(Initialize)(LPDIRECTDRAW pDD, LPDDSURFACEDESC pSurfaceDesc);
    STDMETHOD(IsLost)();
    STDMETHOD(Lock)(LPRECT pRect, LPDDSURFACEDESC pSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
    STDMETHOD(ReleaseDC)(HDC hdc);
    STDMETHOD(Restore)();
    STDMETHOD(SetClipper)(LPDIRECTDRAWCLIPPER pClipper);
    STDMETHOD(SetColorKey)(DWORD dwFlags, LPDDCOLORKEY pDDColorKey);
    STDMETHOD(SetOverlayPosition)(LONG x, LONG y);
    STDMETHOD(SetPalette)(LPDIRECTDRAWPALETTE pDDPal);
    STDMETHOD(Unlock)(LPVOID pBits);
    STDMETHOD(UpdateOverlay)(LPRECT prc, LPDIRECTDRAWSURFACE pdds, LPRECT prc2, DWORD dw, LPDDOVERLAYFX pfx);
    STDMETHOD(UpdateOverlayDisplay)(DWORD dw);
    STDMETHOD(UpdateOverlayZOrder)(DWORD dw, LPDIRECTDRAWSURFACE pdds);

    // IDirectDrawPalette
    STDMETHOD(SetEntries)(DWORD dwFlags, DWORD dwStart, DWORD dwCount, LPPALETTEENTRY pEntries);
    STDMETHOD(GetCaps)(LPDWORD lpdw);
    STDMETHOD(GetEntries)(DWORD dwFlags, DWORD dwStart, DWORD dwCount, LPPALETTEENTRY pEntries);
    STDMETHOD(Initialize)(LPDIRECTDRAW lpdd, DWORD dwCount, LPPALETTEENTRY pEntries);

protected:
    HBITMAP         m_hbmDib;
    DIBSECTION      m_dsSurface;
    DDCOLORKEY      m_ddColorKey;
    LONG            m_lPitch;
    RECT            m_rcSurface;
    BYTE *          m_pbBits;
};

HBITMAP ImgCreateDib(LONG xWid, LONG yHei, BOOL fPal, int cBitsPerPix,
    int cEnt, PALETTEENTRY * ppe, BYTE ** ppbBits, int * pcbRow);
HBITMAP ImgCreateDibFromInfo(BITMAPINFO * pbmi, UINT wUsage, BYTE ** ppbBits, int * pcbRow);
    

