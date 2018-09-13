#ifndef _THUMPRIV_H_
#define _THUMPRIV_H_

// {54F08236-1290-11d1-9A1E-00C04FC2D6C1}
DEFINE_GUID(IID_IThumbnailMaker, 0x54f08236, 0x1290, 0x11d1, 0x9a, 0x1e, 0x0, 0xc0, 0x4f, 0xc2, 0xd6, 0xc1);

#undef INTERFACE
#define INTERFACE IThumbnailMaker

DECLARE_INTERFACE_(IThumbnailMaker, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD(Init)(THIS_ UINT uiDstWidth, UINT uiDstHeight,
            UINT uiSrcWidth, UINT uiSrcHeight) PURE;
    STDMETHOD(AddScanline)(THIS_ UCHAR *pucSrc, UINT uiY) PURE;
    STDMETHOD(AddDIB)(THIS_ BITMAPINFO *pBMI) PURE;
    STDMETHOD(AddDIBSECTION)( THIS_ BITMAPINFO * pBMI, LPVOID pBits ) PURE;
    STDMETHOD(GetBITMAPINFO)(THIS_ BITMAPINFO **ppBMInfo,
                 DWORD *pdwSize) PURE;
    STDMETHOD(GetSharpenedBITMAPINFO)(THIS_ UINT uiSharpPercentage,
                      BITMAPINFO **ppBMInfo,
                      DWORD *pdwSize) PURE;
};

HRESULT ThumbnailMaker_Create(IThumbnailMaker **ppThumbMaker);

typedef UCHAR BGR3[3];

class CThumbnailMaker : public IThumbnailMaker,
                        public CComObjectRoot
{
    BEGIN_COM_MAP( CThumbnailMaker )
        COM_INTERFACE_ENTRY( IThumbnailMaker )
    END_COM_MAP( )

    DECLARE_NOT_AGGREGATABLE( CThumbnailMaker )

private:
    UINT uiDstWidth, uiDstHeight;
    UINT uiSrcWidth, uiSrcHeight;
    BOOL fSharpen;
    BGR3 *pImH;

public:
    CThumbnailMaker();

    ~CThumbnailMaker();

    void
    Scale(BGR3 *pucDst, UINT uiDstWidth, int iDstStep,
          BGR3 *pucSrc, UINT uiSrcWidth, int iSrcStep);

    STDMETHODIMP
    Init(UINT uiDstWidth, UINT uiDstHeight,
         UINT uiSrcWidth, UINT uiSrcHeight);

    STDMETHODIMP AddScanline(UCHAR *pucSrc, UINT uiY);

    STDMETHODIMP AddDIB(BITMAPINFO *pBMI);

    STDMETHODIMP AddDIBSECTION( THIS_ BITMAPINFO * pBMI, LPVOID pBits );
    
    STDMETHODIMP GetBITMAPINFO(BITMAPINFO **ppBMInfo, DWORD *pdwSize);

    STDMETHODIMP GetSharpenedBITMAPINFO(UINT uiSharpPct,
                   BITMAPINFO **ppBMInfo, DWORD *pdwSize);
};

#endif // _THUMPRIV_H_
