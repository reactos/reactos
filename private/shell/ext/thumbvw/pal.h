#ifndef _PAL_H_
#define _PAL_H_

#include <objbase.h>

#ifndef EXTERN_C_BEGIN
#ifdef __cplusplus
#define EXTERN_C_BEGIN      extern "C" {
#define EXTERN_C_END        }
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif
#endif

#define _PAL_EXPORT __declspec(dllexport)

typedef enum
{
    DITHER_SOLID,
    DITHER_ORDERED,
    DITHER_ERRDIFFUSED
} dither_t;

typedef struct DitherInfo
{
    dither_t ditherType;

    // PAL_DITHER_NONE
    //  (no information required)

    // PAL_DITHER_ORDERED
    struct
    {
        UCHAR *pucDitherTable;
        int width, height;
    } diOrdered;

    // PAL_DITHER_ERRDIFFUSED
    //  (no information required)

    void (*pfnDestroyDither)(struct DitherInfo *pDith);
    void *udata;
} DitherInfo;

//
// Convenience structures.
//
//EXTERN_C _PAL_EXPORT DitherInfo diSolid;      // solid dither

//EXTERN_C _PAL_EXPORT DitherInfo diOrderedBayer16x16;  // 16x16 Bayer dither
//EXTERN_C _PAL_EXPORT DitherInfo diOrderedStochastic256x256;   // 256x256 Stochastic dither

//EXTERN_C _PAL_EXPORT DitherInfo diErrDiffused;    // FloydSteinberg err diffused

//
// IInvPalette interface
//
// This interface provides an inverse mapping function from 24bpp RGB space
// to 8bpp indexed RGB space. It performs the quantization and disperses the
// quantization error according to the dither option selected.
//

#undef INTERFACE
#define INTERFACE IInvPalette

DECLARE_INTERFACE_(IInvPalette, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD_(LOGPALETTE *, GetLOGPALETTE)(THIS) PURE;
    STDMETHOD_(HPALETTE, GetHPalette)(THIS) PURE;
    STDMETHOD(Quantize)(THIS_ UCHAR *pucBGR,
                int x, int y, int num_pixels,
                UCHAR *pucIdx) PURE;
    STDMETHOD(QuantizeCOLORREF)(THIS_ COLORREF rgb,
                    int x, int y, int num_pixels,
                    UCHAR *pucIdx) PURE;
    STDMETHOD(ResetDither)(THIS_ int maxwidth) PURE;
};

EXTERN_C _PAL_EXPORT IInvPalette *
InvPalette_CreateUniformPaletteInv(int numcycles_red,
                   int numcycles_green,
                   int numcycles_blue,
                   DitherInfo *dither_info);

EXTERN_C _PAL_EXPORT IInvPalette *
InvPalette_CreateSpecifiedPaletteInv(LOGPALETTE *pLOGPALETTE,
                     int numbits_red,
                     int numbits_green,
                     int numbits_blue,
                     DitherInfo *dither_info);

//
// IColorReduce interface
//
// This interface produces an 8bpp indexed RGB color palette that approximates
// the optimal palette for some set of 24bpp RGB inputs.
//

#undef INTERFACE
#define INTERFACE IColorReduce

DECLARE_INTERFACE_(IColorReduce, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD(AddPixels)(THIS_ UCHAR *pucBGR, int num_pixels) PURE;
    STDMETHOD(AddPixelCounts)(THIS_ UCHAR *pucBGR, USHORT *pCnts,
                  int num_pixels) PURE;
    STDMETHOD_(LOGPALETTE *, GenerateLOGPALETTE)(THIS) PURE;
    STDMETHOD(Reset)(THIS) PURE;
};

EXTERN_C _PAL_EXPORT IColorReduce *
ColorReduce_CreateMedianCut(int num_colors,
                int numbits_red,
                int numbits_green,
                int numbits_blue);


//
// Helper functions.
//
EXTERN_C _PAL_EXPORT HRESULT
ConvertDIBToDIB8(BITMAPINFO *pBMISrc, UCHAR *pucBitsSrc,
         IInvPalette *pInvPal, BITMAPINFO **ppBMIDst);


//////////////////////////////////////////////////////////////////////////////////////
class CInvPal : public IInvPalette
{
    DWORD dwRefcnt;
    HPALETTE hPal;

public:
    DitherInfo ditherInfo;

    CInvPal()
    {
        dwRefcnt = 0;
        ditherInfo.pfnDestroyDither = NULL;
        hPal = NULL;
    }

    virtual
    ~CInvPal()
    {
        if (ditherInfo.pfnDestroyDither)
            ditherInfo.pfnDestroyDither(&ditherInfo);
        if (hPal)
            DeleteObject(hPal);
    }

    HRESULT
    Init(const DitherInfo *const pDitherInfo)
    {
        ditherInfo = *pDitherInfo;

        return S_OK;
    }

    STDMETHODIMP
    QueryInterface(REFIID riid, LPVOID *ppvObj)
    {
        if (riid == IID_IUnknown)
            *ppvObj = (LPVOID)this;
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }

        ((LPUNKNOWN)*ppvObj)->AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        return ++dwRefcnt;
    }

    STDMETHODIMP_(ULONG)
    Release()
    {
        if (dwRefcnt > 1)
            return --dwRefcnt;
        delete this;
        return 0;
    }

    STDMETHODIMP_(HPALETTE)
    GetHPalette()
    {
        LOGPALETTE *pLPal;

        if (hPal)
            return hPal;

        pLPal = GetLOGPALETTE();
        if (pLPal == NULL)
            return NULL;

        hPal = CreatePalette(pLPal);

        return hPal;
    }
};

class CColRed : public IColorReduce
{
    DWORD dwRefcnt;

public:
    CColRed()
    {
        dwRefcnt = 0;
    }

    virtual
    ~CColRed()
    {
    }

    HRESULT
    Init(void)
    {
        return S_OK;
    }

    STDMETHODIMP
    QueryInterface(REFIID riid, LPVOID *ppvObj)
    {
        if (riid == IID_IUnknown)
            *ppvObj = (LPVOID)this;
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }

        ((LPUNKNOWN)*ppvObj)->AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        return ++dwRefcnt;
    }

    STDMETHODIMP_(ULONG)
    Release()
    {
        if (dwRefcnt > 1)
            return --dwRefcnt;
        delete this;
        return 0;
    }
};

#endif /* _PAL_H_ */
