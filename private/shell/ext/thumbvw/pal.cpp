#include "thlibpch.h"
#include "pal.h"
#include "palpriv.h"

//
// Inverse Palette using uniform partitioning
//

#define qStepR 1

class CInvPalUP : public CInvPal
{
    LOGPALETTE *pLPal;

    HRESULT
    BuildQuantTbl(void);

public:
    UCHAR quantTblR[256], quantTblG[256], quantTblB[256];
    UCHAR errTblR[256], errTblG[256], errTblB[256];
    int numcycles_red, numcycles_green, numcycles_blue;
    UCHAR qStepG, qStepB;

    CInvPalUP()
    {
        pLPal = NULL;
    }

    ~CInvPalUP()
    {
        if (pLPal)
            free(pLPal);
    }

    HRESULT
    Init(int ncycles_red, int ncycles_green, int ncycles_blue,
         const DitherInfo *const pDitherInfo);

    STDMETHODIMP_(LOGPALETTE *)
    GetLOGPALETTE();
};

//
// Build the quantization tables based on the current dither type:
//  SOLID       midpoint
//  ORDERED     floor
//  ERRDIFFUSED midpoint
//
HRESULT
CInvPalUP::BuildQuantTbl(void)
{
    int i;

    switch (ditherInfo.ditherType)
    {
    default:
        return E_FAIL;

    //
    // For solid dither, calculate the quantized midpoints.
    // No error needed.
    //
    case DITHER_SOLID:
        for (i = 0; i < 256; ++i)
        {
            quantTblR[i] = CYC_QUANT_MID(i, numcycles_red) *
                qStepR;

            quantTblG[i] = CYC_QUANT_MID(i, numcycles_green) *
                qStepG;

            quantTblB[i] = CYC_QUANT_MID(i, numcycles_blue) *
                qStepB;
        }
        break;

    //
    // For ordered dither, calculate the quantized floor value. The error
    // will be scaled to (0 .. 255) for comparison against the dither table
    //
    case DITHER_ORDERED:
        for (i = 0; i < 256; ++i)
        {
            quantTblR[i] = CYC_QUANT_FLR(i, numcycles_red);
            errTblR[i] = (numcycles_red - 1) *
                (i - CYC_EXPAND(quantTblR[i],
                        numcycles_red));
            quantTblR[i] *= qStepR;

            quantTblG[i] = CYC_QUANT_FLR(i, numcycles_green);
            errTblG[i] = (numcycles_green - 1) *
                (i - CYC_EXPAND(quantTblG[i],
                        numcycles_green));
            quantTblG[i] *= qStepG;

            quantTblB[i] = CYC_QUANT_FLR(i, numcycles_blue);
            errTblB[i] = (numcycles_blue - 1) *
                (i - CYC_EXPAND(quantTblB[i],
                        numcycles_blue));
            quantTblB[i] *= qStepB;
        }
        break;

    //
    // For error diffused dither, calculate the quantized midpoint. The
    // error is kept as-is, to be dispersed to the neighbors.
    //
    case DITHER_ERRDIFFUSED:
        for (i = 0; i < 256; ++i)
        {
            quantTblR[i] = CYC_QUANT_MID(i, numcycles_red);
            errTblR[i] = i - CYC_EXPAND(quantTblR[i],
                            numcycles_red);
            quantTblR[i] *= qStepR;

            quantTblG[i] = CYC_QUANT_MID(i, numcycles_green);
            errTblG[i] = i - CYC_EXPAND(quantTblG[i],
                            numcycles_green);
            quantTblG[i] *= qStepG;

            quantTblB[i] = CYC_QUANT_MID(i, numcycles_blue);
            errTblB[i] = i - CYC_EXPAND(quantTblB[i],
                            numcycles_blue);
            quantTblB[i] *= qStepB;
        }
        break;
    }

    return S_OK;
}

HRESULT
CInvPalUP::Init(int ncycles_red, int ncycles_green, int ncycles_blue,
        const DitherInfo *const pDitherInfo)
{
    HRESULT hr;

    hr = CInvPal::Init(pDitherInfo);
    if (FAILED(hr))
        return hr;

    numcycles_red = ncycles_red;
    numcycles_green = ncycles_green;
    numcycles_blue = ncycles_blue;

    qStepG = (UCHAR)numcycles_red;
    qStepB = (UCHAR)(numcycles_red * numcycles_green);

    return BuildQuantTbl();
}

STDMETHODIMP_(LOGPALETTE *)
CInvPalUP::GetLOGPALETTE()
{
    PALETTEENTRY *pPalEnt;
    int num_colors;
    UCHAR qR, qG, qB;

    if (pLPal != NULL)
        return pLPal;

    num_colors = numcycles_red * numcycles_green * numcycles_blue;

    pLPal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) +
                     (num_colors - 1) * sizeof(PALETTEENTRY));
    if (pLPal == NULL)
        return NULL;

    pLPal->palVersion = 0x300;
    pLPal->palNumEntries = (WORD)num_colors;

    pPalEnt = &pLPal->palPalEntry[0];

    for (qB = 0; qB < numcycles_blue; ++qB)
    {
        UCHAR iB = CYC_EXPAND(qB, numcycles_blue);

        for (qG = 0; qG < numcycles_green; ++qG)
        {
            UCHAR iG = CYC_EXPAND(qG, numcycles_green);

            for (qR = 0; qR < numcycles_red; ++qR)
            {
                pPalEnt->peRed = CYC_EXPAND(qR, numcycles_red);
                pPalEnt->peGreen = iG;
                pPalEnt->peBlue = iB;
                pPalEnt->peFlags = 0;

                ++pPalEnt;
            }
        }
    }

    return pLPal;
}


class CInvPalUP_Solid : public CInvPalUP
{
public:
    STDMETHODIMP
    Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    QuantizeCOLORREF(COLORREF rgb, int x, int y, int num_pixels,
             UCHAR *pucIdx);

    STDMETHODIMP
    ResetDither(int max_width)
    {
        return S_OK;
    }
};

STDMETHODIMP
CInvPalUP_Solid::Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
              UCHAR *pucIdx)
{
    while (num_pixels--)
    {
        *pucIdx++ = CYC_PACKSCALEDQUANT(quantTblR[pucBGR[2]],
                        quantTblG[pucBGR[1]],
                        quantTblB[pucBGR[0]]);
        pucBGR += 3;
    }

    return S_OK;
}

STDMETHODIMP
CInvPalUP_Solid::QuantizeCOLORREF(COLORREF rgb, int x, int y,
                  int num_pixels, UCHAR *pucIdx)
{
    UCHAR ucIdx;

    ucIdx = CYC_PACKSCALEDQUANT(quantTblR[GetRValue(rgb)],
                    quantTblG[GetGValue(rgb)],
                    quantTblB[GetBValue(rgb)]);

    memset(pucIdx, ucIdx, num_pixels);

    return S_OK;
}


class CInvPalUP_Ordered : public CInvPalUP
{
public:
    STDMETHODIMP
    Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    QuantizeCOLORREF(COLORREF rgb, int x, int y, int num_pixels,
             UCHAR *pucIdx);

    STDMETHODIMP
    ResetDither(int max_width)
    {
        return S_OK;
    }
};

STDMETHODIMP
CInvPalUP_Ordered::Quantize(UCHAR *pucBGR, int x, int y,
                int num_pixels, UCHAR *pucIdx)
{
    UCHAR *pucDith;
    int num_dither, num_dither_left;

    num_dither = ditherInfo.diOrdered.width;
    num_dither_left = x % num_dither;
    pucDith = ditherInfo.diOrdered.pucDitherTable +
        (y % ditherInfo.diOrdered.height) * num_dither +
            num_dither_left;
    num_dither_left = num_dither - num_dither_left;

    while (num_pixels--)
    {
        UCHAR v, q, thresh;

        thresh = *pucDith++;

        v = pucBGR[2];
        q = quantTblR[v];
        if (errTblR[v] >= thresh)
            q += qStepR;

        v = pucBGR[1];
        q += quantTblG[v];
        if (errTblG[v] >= thresh)
            q += qStepG;

        v = pucBGR[0];
        q += quantTblB[v];
        if (errTblB[v] >= thresh)
            q += qStepB;

        pucBGR += 3;

        *pucIdx++ = q;

        if (--num_dither_left == 0)
        {
            num_dither_left = num_dither;
            pucDith -= num_dither;
        }
    }

    return S_OK;
}

STDMETHODIMP
CInvPalUP_Ordered::QuantizeCOLORREF(COLORREF rgb, int x, int y,
                    int num_pixels, UCHAR *pucIdx)
{
    UCHAR *pucDith;
    int num_dither, num_dither_left;

    num_dither = ditherInfo.diOrdered.width;
    num_dither_left = x % num_dither;
    pucDith = ditherInfo.diOrdered.pucDitherTable +
        (y % ditherInfo.diOrdered.height) * num_dither +
            num_dither_left;
    num_dither_left = num_dither - num_dither_left;

    while (num_pixels--)
    {
        UCHAR v, q, thresh;

        thresh = *pucDith++;

        v = GetRValue(rgb);
        q = quantTblR[v];
        if (errTblR[v] >= thresh)
            q += qStepR;

        v = GetGValue(rgb);
        q += quantTblG[v];
        if (errTblG[v] >= thresh)
            q += qStepG;

        v = GetBValue(rgb);
        q += quantTblB[v];
        if (errTblB[v] >= thresh)
            q += qStepB;

        *pucIdx++ = q;

        if (--num_dither_left == 0)
        {
            num_dither_left = num_dither;
            pucDith -= num_dither;
        }
    }

    return S_OK;
}


class CInvPalUP_OrderedM : public CInvPalUP_Ordered
{
private:
    UCHAR mask_x, mask_y;

public:
    CInvPalUP_OrderedM(UCHAR maskx, UCHAR masky)
    {
        mask_x = maskx;
        mask_y = masky;
    }

    STDMETHODIMP
    Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    QuantizeCOLORREF(COLORREF rgb, int x, int y, int num_pixels,
             UCHAR *pucIdx);
};


STDMETHODIMP
CInvPalUP_OrderedM::Quantize(UCHAR *pucBGR, int x, int y,
                 int num_pixels, UCHAR *pucIdx)
{
    UCHAR *pucDith;
    int i, i3, n3;

    i = x & mask_x;
    pucDith = ditherInfo.diOrdered.pucDitherTable +
        (y & mask_y) * (mask_x + 1);
    pucIdx -= i;

    i3 = i;
    pucBGR -= i3;
    n3 = i3 + num_pixels * 3;

    while (i3 < n3)
    {
        UCHAR v, q, thresh;

        thresh = pucDith[i & mask_x];

        v = pucBGR[i3++];
        q = quantTblB[v];
        if (errTblB[v] >= thresh)
            q += qStepB;

        v = pucBGR[i3++];
        q += quantTblG[v];
        if (errTblG[v] >= thresh)
            q += qStepG;

        v = pucBGR[i3++];
        q += quantTblR[v];
        if (errTblR[v] >= thresh)
            q += qStepR;

        pucIdx[i++] = q;
    }

    return S_OK;
}

STDMETHODIMP
CInvPalUP_OrderedM::QuantizeCOLORREF(COLORREF rgb, int x, int y,
                        int num_pixels, UCHAR *pucIdx)
{
    UCHAR *pucDith, r, g, b;
    int i;

    i = x & mask_x;
    pucDith = ditherInfo.diOrdered.pucDitherTable +
        (y & mask_y) * (mask_x + 1);
    pucIdx -= i;

    r = GetRValue(rgb);
    g = GetGValue(rgb);
    b = GetBValue(rgb);

    while (num_pixels--)
    {
        UCHAR q, thresh;

        thresh = pucDith[i & mask_x];

        q = quantTblR[r] + quantTblG[g] + quantTblB[b];
        if (errTblR[r] >= thresh)
            q += qStepR;
        if (errTblG[g] >= thresh)
            q += qStepG;
        if (errTblB[b] >= thresh)
            q += qStepB;

        pucIdx[i++] = q;
    }

    return S_OK;
}


#define qStepR666 1
#define qStepG666 6
#define qStepB666 36

class CInvPalUP_OrderedM255C666 : public CInvPalUP_Ordered
{
public:
    STDMETHODIMP
    Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    QuantizeCOLORREF(COLORREF rgb, int x, int y, int num_pixels,
             UCHAR *pucIdx);
};

STDMETHODIMP
CInvPalUP_OrderedM255C666::Quantize(UCHAR *pucBGR, int x, int y,
                    int num_pixels, UCHAR *pucIdx)
{
    UCHAR *pucDith;
    int i, i3;

    i = x & 255;
    pucDith = ditherInfo.diOrdered.pucDitherTable + (y & 255) * 256;
    pucIdx -= i;
    i += num_pixels;

    i3 = 3 * num_pixels;

    while (i3)
    {
        UCHAR v, q, thresh;

        thresh = pucDith[--i & 255];
        v = pucBGR[--i3];
        q = (errTblR[v] < thresh) ?
            quantTblR[v] : quantTblR[v] + qStepR666;
        v = pucBGR[--i3];
        q += (errTblG[v] < thresh) ?
            quantTblG[v] : quantTblG[v] + qStepG666;
        v = pucBGR[--i3];
        q += (errTblB[v] < thresh) ?
            quantTblB[v] : quantTblB[v] + qStepB666;
        pucIdx[i] = q;
    }

    return S_OK;
}

STDMETHODIMP
CInvPalUP_OrderedM255C666::QuantizeCOLORREF(COLORREF rgb, int x, int y,
                        int num_pixels, UCHAR *pucIdx)
{
    UCHAR *pucDith, r, g, b;
    int i;

    i = x & 255;
    pucDith = ditherInfo.diOrdered.pucDitherTable + (y & 255) * 256;
    pucIdx -= i;

    r = GetRValue(rgb);
    g = GetGValue(rgb);
    b = GetBValue(rgb);

    while (num_pixels--)
    {
        UCHAR q, thresh;

        thresh = pucDith[i & 255];

        q = quantTblR[r] + quantTblG[g] + quantTblB[b];
        if (errTblR[r] >= thresh)
            q += qStepR666;
        if (errTblG[g] >= thresh)
            q += qStepG666;
        if (errTblB[b] >= thresh)
            q += qStepB666;

        pucIdx[i++] = q;
    }

    return S_OK;
}


class CInvPalUP_ErrDiffused : public CInvPalUP
{
    Err *errBuf, *errBase[2];
    int last_max_width;
    int last_y;

public:
    CInvPalUP_ErrDiffused()
    {
        errBuf = NULL;
    }

    ~CInvPalUP_ErrDiffused()
    {
        if (errBuf)
            free(errBuf);
    }

    STDMETHODIMP
    Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    QuantizeCOLORREF(COLORREF rgb, int x, int y, int num_pixels,
             UCHAR *pucIdx);

    STDMETHODIMP
    ResetDither(int max_width);
};

STDMETHODIMP
CInvPalUP_ErrDiffused::Quantize(UCHAR *pucBGR, int x, int y,
                int num_pixels, UCHAR *pucIdx)
{
    Err *errs[2];
    BOOL reverse;

    //
    // Assume if this y isn't the previous y that it's within 1 of
    // the previous, so we step the error scanlines. For any larger
    // jump, the user should call ResetDither()
    //
    if (last_y != y)
    {
        errs[0] = errBase[0];
        errBase[0] = errBase[1];
        errBase[1] = errs[0];

        memset(errBase[1], 0, sizeof(Err) * last_max_width);
    }

    reverse = (BOOL)(y & 1);

    errs[0] = errBase[0];
    errs[1] = errBase[1];

    if (reverse)
    {
        errs[0] += num_pixels - 1;
        errs[1] += num_pixels - 1;
        pucBGR += 3 * (num_pixels - 1);
        pucIdx += num_pixels - 1;
    }

    while (num_pixels--)
    {
        short iR, iG, iB;
        Err err;

        iR = pucBGR[2] + DIV16(errs[0][0].r);
        iG = pucBGR[1] + DIV16(errs[0][0].g);
        iB = pucBGR[0] + DIV16(errs[0][0].b);

        iR = (iR < 0 ? 0 : iR > 255 ? 255 : iR);
        iG = (iG < 0 ? 0 : iG > 255 ? 255 : iG);
        iB = (iB < 0 ? 0 : iB > 255 ? 255 : iB);

        *pucIdx = CYC_PACKSCALEDQUANT(quantTblR[iR],
                          quantTblG[iG],
                          quantTblB[iB]);

        err.r = (short)(char)errTblR[iR];
        err.g = (short)(char)errTblG[iG];
        err.b = (short)(char)errTblB[iB];

        //
        // Disperse the error with Floyd-Steinberg weights:
        //
        //      *    7/16
        // 1/16 5/16 3/16
        //
        // The weights are reflected in the reverse direction.
        //

        if (reverse)
        {
            errs[0][-1].r += MUL7(err.r);
            errs[1][-1].r += MUL3(err.r);
            errs[1][0].r += MUL5(err.r);
            errs[1][1].r += err.r;

            errs[0][-1].g += MUL7(err.g);
            errs[1][-1].g += MUL3(err.g);
            errs[1][0].g += MUL5(err.g);
            errs[1][1].g += err.g;

            errs[0][-1].b += MUL7(err.b);
            errs[1][-1].b += MUL3(err.b);
            errs[1][0].b += MUL5(err.b);
            errs[1][1].b += err.b;

            pucBGR -= 3;
            pucIdx--;
            errs[0]--;
            errs[1]--;
        }
        else
        {
            errs[0][1].r += MUL7(err.r);
            errs[1][1].r += MUL3(err.r);
            errs[1][0].r += MUL5(err.r);
            errs[1][-1].r += err.r;

            errs[0][1].g += MUL7(err.g);
            errs[1][1].g += MUL3(err.g);
            errs[1][0].g += MUL5(err.g);
            errs[1][-1].g += err.g;

            errs[0][1].b += MUL7(err.b);
            errs[1][1].b += MUL3(err.b);
            errs[1][0].b += MUL5(err.b);
            errs[1][-1].b += err.b;

            pucBGR += 3;
            pucIdx++;
            errs[0]++;
            errs[1]++;
        }
    }

    return S_OK;
}

STDMETHODIMP
CInvPalUP_ErrDiffused::QuantizeCOLORREF(COLORREF rgb, int x, int y,
                    int num_pixels, UCHAR *pucIdx)
{
    Err *errs[2];
    BOOL reverse;

    //
    // Assume if this y isn't the previous y that it's within 1 of
    // the previous, so we step the error scanlines. For any larger
    // jump, the user should call ResetDither()
    //
    if (last_y != y)
    {
        errs[0] = errBase[0];
        errBase[0] = errBase[1];
        errBase[1] = errs[0];

        memset(errBase[1], 0, sizeof(Err) * last_max_width);
    }

    reverse = (BOOL)(y & 1);

    errs[0] = errBase[0];
    errs[1] = errBase[1];

    if (reverse)
    {
        errs[0] += num_pixels - 1;
        errs[1] += num_pixels - 1;
        pucIdx += num_pixels - 1;
    }

    while (num_pixels--)
    {
        short iR, iG, iB;
        Err err;

        iR = GetRValue(rgb) + DIV16(errs[0][0].r);
        iG = GetGValue(rgb) + DIV16(errs[0][0].g);
        iB = GetBValue(rgb) + DIV16(errs[0][0].b);

        iR = (iR < 0 ? 0 : iR > 255 ? 255 : iR);
        iG = (iG < 0 ? 0 : iG > 255 ? 255 : iG);
        iB = (iB < 0 ? 0 : iB > 255 ? 255 : iB);

        *pucIdx = CYC_PACKSCALEDQUANT(quantTblR[iR],
                          quantTblG[iG],
                          quantTblB[iB]);

        err.r = (short)(char)errTblR[iR];
        err.g = (short)(char)errTblG[iG];
        err.b = (short)(char)errTblB[iB];

        //
        // Disperse the error with Floyd-Steinberg weights:
        //
        //      *    7/16
        // 1/16 5/16 3/16
        //
        // The weights are reflected in the reverse direction.
        //

        if (reverse)
        {
            errs[0][-1].r += MUL7(err.r);
            errs[1][-1].r += MUL3(err.r);
            errs[1][0].r += MUL5(err.r);
            errs[1][1].r += err.r;

            errs[0][-1].g += MUL7(err.g);
            errs[1][-1].g += MUL3(err.g);
            errs[1][0].g += MUL5(err.g);
            errs[1][1].g += err.g;

            errs[0][-1].b += MUL7(err.b);
            errs[1][-1].b += MUL3(err.b);
            errs[1][0].b += MUL5(err.b);
            errs[1][1].b += err.b;

            pucIdx--;
            errs[0]--;
            errs[1]--;
        }
        else
        {
            errs[0][1].r += MUL7(err.r);
            errs[1][1].r += MUL3(err.r);
            errs[1][0].r += MUL5(err.r);
            errs[1][-1].r += err.r;

            errs[0][1].g += MUL7(err.g);
            errs[1][1].g += MUL3(err.g);
            errs[1][0].g += MUL5(err.g);
            errs[1][-1].g += err.g;

            errs[0][1].b += MUL7(err.b);
            errs[1][1].b += MUL3(err.b);
            errs[1][0].b += MUL5(err.b);
            errs[1][-1].b += err.b;

            pucIdx++;
            errs[0]++;
            errs[1]++;
        }
    }

    return S_OK;
}

STDMETHODIMP
CInvPalUP_ErrDiffused::ResetDither(int max_width)
{
    Err *err;

    if (errBuf == NULL)
    {
        err = (Err *)malloc(sizeof(Err) * (max_width + 2) * 2);
        if (err == NULL)
            return NULL;

        errBuf = err;
        last_max_width = max_width;
    }

    else if (last_max_width < max_width)
    {
        err = (Err *)realloc(errBuf, sizeof(Err) *
                     (max_width + 2) * 2);
        if (err == NULL)
            return NULL;

        errBuf = err;
        last_max_width = max_width;
    }

    errBase[0] = errBuf + 1;
    errBase[1] = errBuf + last_max_width + 3;

    memset(errBuf, 0, sizeof(Err) * (last_max_width + 2) * 2);

    last_y = (int)~0;

    return S_OK;
}


extern "C" IInvPalette *
InvPalette_CreateUniformPaletteInv(int numcycles_red,
                   int numcycles_green,
                   int numcycles_blue,
                   DitherInfo *ditherInfo)
{
    CInvPalUP *pInvPalUP;
    DitherInfo di;

    Assert(numcycles_red > 0 && numcycles_green > 0 &&
           numcycles_blue > 0);
    if (!(numcycles_red > 0 && numcycles_green > 0 &&
          numcycles_blue > 0))
        return NULL;

    Assert (numcycles_red * numcycles_green * numcycles_blue <= 256);
    if (numcycles_red * numcycles_green * numcycles_blue > 256)
        return NULL;

    if (ditherInfo == NULL)
    {
        di.ditherType = DITHER_SOLID;
        di.pfnDestroyDither = NULL;
    }
    else
        di = *ditherInfo;

    switch (di.ditherType)
    {
    default:
        Assert(FALSE);
        return NULL;

    case DITHER_SOLID:
        pInvPalUP = new CInvPalUP_Solid;
        break;

    case DITHER_ORDERED:
    {
        int mask_x, mask_y;

        for (mask_x = 2; mask_x < di.diOrdered.width; mask_x <<= 1);
        for (mask_y = 2; mask_y < di.diOrdered.height; mask_y <<= 1);

        if (di.diOrdered.pucDitherTable == NULL)
            return NULL;

        if (mask_x == di.diOrdered.width &&
            mask_y == di.diOrdered.height)
        {
            --mask_x;
            --mask_y;

            if (mask_x == 255 && mask_y == 255 &&
                numcycles_red == 6 &&
                numcycles_green == 6 &&
                numcycles_blue == 6)
                pInvPalUP = new CInvPalUP_OrderedM255C666;
            else
                pInvPalUP = new CInvPalUP_OrderedM((UCHAR)mask_x,
                                   (UCHAR)mask_y);
        }
        else
            pInvPalUP = new CInvPalUP_Ordered;
        break;
    }

    case DITHER_ERRDIFFUSED:
        pInvPalUP = new CInvPalUP_ErrDiffused;
        break;
    }

    if (pInvPalUP == NULL)
        return NULL;

    if (FAILED(pInvPalUP->Init(numcycles_red, numcycles_green,
                   numcycles_blue, &di)))
    {
        delete pInvPalUP;
        return NULL;
    }

    pInvPalUP->AddRef();

    return (IInvPalette *)pInvPalUP;
}


//
// Inverse Palette using specified LOGPALETTE
//
// Based on "Efficient Inverse Color Map Computation", Spencer W. Thomas,
// Graphics Gems II, p.116 (and code presented)
//
//

class CInvPalSP : public CInvPal
{
    virtual HRESULT
    BuildInvMap(void);

    HRESULT
    BuildQuantTbl(void);

public:
    LOGPALETTE *pLPal;
    int numbits_red, numbits_green, numbits_blue;
    int numshift_red, numshift_green, numshift_blue;
    int numcycles_red, numcycles_green, numcycles_blue;
    int numfrac_red, numfrac_green, numfrac_blue;
    UCHAR *pInvMap;
    USHORT quantTblR[256], quantTblG[256], quantTblB[256];
    UCHAR errTblR[256], errTblG[256], errTblB[256];
    int qStepG, qStepB;

    CInvPalSP()
    {
        pLPal = NULL;
        pInvMap = NULL;
    }

    ~CInvPalSP()
    {
        if (pLPal)
            free(pLPal);
        if (pInvMap)
            free(pInvMap);
    }

    HRESULT
    Init(LOGPALETTE *pLogPal,
         int nbits_red, int nbits_green, int nbits_blue,
         const DitherInfo *const pDitherInfo);

    STDMETHODIMP_(LOGPALETTE *)
    GetLOGPALETTE()
    {
        return pLPal;
    }
};

typedef ULONG dist_t;

HRESULT
CInvPalSP::BuildInvMap(void)
{
    dist_t *pDistBuf;
    ULONG numcells;
    ULONG stepR, stepG, stepB;
    ULONG incR, incG, incB;
    ULONG baseincR, baseincG, baseincB;
    ULONG incincR, incincG, incincB;
    dist_t distR, distG, distB;
    PALETTEENTRY *pPalEnt;
    int i;

    numcells = 1 << (numbits_red + numbits_green + numbits_blue);

    pInvMap = (UCHAR *)malloc(numcells);
    if (pInvMap == NULL)
        return E_OUTOFMEMORY;

    pDistBuf = (dist_t *)malloc(sizeof(dist_t) * numcells);
    if (pDistBuf == NULL)
        return E_OUTOFMEMORY;

    memset(pDistBuf, 255, sizeof(dist_t) * numcells);

    stepR = numfrac_red;
    stepG = numfrac_green;
    stepB = numfrac_blue;

    incincR = (2 * stepR) << numshift_red;
    incincG = (2 * stepG) << numshift_green;
    incincB = (2 * stepB) << numshift_blue;

    for (i = pLPal->palNumEntries, pPalEnt = &pLPal->palPalEntry[i-1];
         i--; pPalEnt--)
    {
        dist_t *pDist;
        UCHAR *pIm;
        UCHAR qR, qG, qB;

        //
        // Start at the center of the first cell.
        //
        distR = pPalEnt->peRed - stepR / 2;
        distG = pPalEnt->peGreen - stepG / 2;
        distB = pPalEnt->peBlue - stepB / 2;

        baseincR = stepR * (stepR - 2 * distR);
        baseincG = stepG * (stepG - 2 * distG);
        baseincB = stepB * (stepB - 2 * distB);

        pDist = pDistBuf;
        pIm = pInvMap;

        distB = distR * distR + distG * distG + distB * distB;
        incB = baseincB;

        for (qB = 0; qB < numcycles_blue; ++qB)
        {
            distG = distB;
            incG = baseincG;

            for (qG = 0; qG < numcycles_green; ++qG)
            {
                distR = distG;
                incR = baseincR;

                for (qR = 0; qR < numcycles_red; ++qR)
                {
                    if (distR < *pDist)
                    {
                        *pDist = distR;
                        *pIm = (UCHAR)i;
                    }

                    ++pDist;
                    ++pIm;

                    distR += incR;
                    incR += incincR;
                }

                distG += incG;
                incG += incincG;
            }

            distB += incB;
            incB += incincB;
        }
    }

    free(pDistBuf);

    return S_OK;
}

HRESULT
CInvPalSP::BuildQuantTbl(void)
{
    int i;

    switch (ditherInfo.ditherType)
    {
    default:
        return E_FAIL;

    case DITHER_SOLID:
        for (i = 0; i < 256; ++i)
        {
            quantTblR[i] = SHF_QUANT_MID(i, numshift_red) * qStepR;
            quantTblG[i] = SHF_QUANT_MID(i, numshift_green) * qStepG;
            quantTblB[i] = SHF_QUANT_MID(i, numshift_blue) * qStepB;
        }
        break;

    case DITHER_ORDERED:
    case DITHER_ERRDIFFUSED:
        for (i = 0; i < 256; ++i)
        {
            quantTblR[i] = SHF_QUANT_MID(i, numshift_red);
            quantTblG[i] = SHF_QUANT_MID(i, numshift_green);
            quantTblB[i] = SHF_QUANT_MID(i, numshift_blue);
        }
        break;
    }

    return S_OK;
}

HRESULT
CInvPalSP::Init(LOGPALETTE *pLogPal,
        int nbits_red, int nbits_green, int nbits_blue,
        const DitherInfo *const pDitherInfo)
{
    HRESULT hr;

    hr = CInvPal::Init(pDitherInfo);
    if (FAILED(hr))
        return hr;

    numbits_red = nbits_red;
    numbits_green = nbits_green;
    numbits_blue = nbits_blue;

    numcycles_red = 1 << numbits_red;
    numcycles_green = 1 << numbits_green;
    numcycles_blue = 1 << numbits_blue;

    numshift_red = 8 - numbits_red;
    numshift_green = 8 - numbits_green;
    numshift_blue = 8 - numbits_blue;

    numfrac_red = 1 << numshift_red;
    numfrac_green = 1 << numshift_green;
    numfrac_blue = 1 << numshift_blue;

    qStepG = 1 << numbits_red;
    qStepB = 1 << (numbits_red + numbits_blue);

    pLPal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) +
                     (pLogPal->palNumEntries - 1) *
                     sizeof(PALETTEENTRY));
    if (pLPal == NULL)
        return E_OUTOFMEMORY;

    memcpy(pLPal, pLogPal, sizeof(LOGPALETTE) +
           (pLogPal->palNumEntries - 1) * sizeof(PALETTEENTRY));

    hr = BuildInvMap();
    if (FAILED(hr))
        return hr;

    return BuildQuantTbl();
}


class CInvPalSP_Solid : public CInvPalSP
{
public:
    STDMETHODIMP
    Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    QuantizeCOLORREF(COLORREF rgb, int x, int y, int num_pixels,
             UCHAR *pucIdx);

    STDMETHODIMP
    ResetDither(int max_width)
    {
        return S_OK;
    }
};

STDMETHODIMP
CInvPalSP_Solid::Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
              UCHAR *pucIdx)
{
    while (num_pixels--)
    {
        *pucIdx++ = pInvMap[BIT_PACKSCALEDQUANT(quantTblR[pucBGR[2]],
                            quantTblG[pucBGR[1]],
                            quantTblB[pucBGR[0]])];

        pucBGR += 3;
    }

    return S_OK;
}

STDMETHODIMP
CInvPalSP_Solid::QuantizeCOLORREF(COLORREF rgb, int x, int y,
                  int num_pixels, UCHAR *pucIdx)
{
    UCHAR ucIdx;

    ucIdx = pInvMap[BIT_PACKSCALEDQUANT(quantTblR[GetRValue(rgb)],
                        quantTblG[GetGValue(rgb)],
                        quantTblB[GetBValue(rgb)])];

    memset(pucIdx, ucIdx, num_pixels);

    return S_OK;
}


struct OrderedInvMap
{
    UCHAR id0, id1, pct;
};

class CInvPalSP_Ordered : public CInvPalSP
{
private:
    OrderedInvMap *pOrderedInvMap;

    void
    BuildOrderedInvMapEntry(UCHAR r, UCHAR g, UCHAR b,
                OrderedInvMap *pIM);

    virtual HRESULT
    BuildInvMap__2(void);

public:
    CInvPalSP_Ordered()
    {
        pOrderedInvMap = NULL;
    }

    ~CInvPalSP_Ordered()
    {
        if (pOrderedInvMap)
            free(pOrderedInvMap);
    }

    STDMETHODIMP
    Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    QuantizeCOLORREF(COLORREF rgb, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    ResetDither(int max_width)
    {
        return S_OK;
    }
};

void
CInvPalSP_Ordered::BuildOrderedInvMapEntry(UCHAR r, UCHAR g, UCHAR b,
                       OrderedInvMap *pIM)
{
    PALETTEENTRY *pPalEnt0;
    int i;

    //
    // We want to locate the two palette entries that form a line that
    // lies closest to our target point. We favor points that lie
    // close to one another to help smooth the dither.
    //
    for (i = pLPal->palNumEntries-1, pPalEnt0 = &pLPal->palPalEntry[i-1];
         i--; pPalEnt0--)
    {
        PALETTEENTRY *pPalEnt1;
        int j;

        for (j = i - 1, pPalEnt1 = pPalEnt0 - 1; j--; pPalEnt1--)
        {
            int dr, dg, db;
            ULONG ldist2;

            dr = pPalEnt1->peRed - pPalEnt0->peRed;
            dg = pPalEnt1->peGreen - pPalEnt0->peGreen;
            db = pPalEnt1->peBlue - pPalEnt0->peBlue;

            ldist2 = dr*dr + dg*dg + db*db;
        }
    }
}

HRESULT
CInvPalSP_Ordered::BuildInvMap__2(void)
{
    ULONG numcells;
    OrderedInvMap *pIM;
    UCHAR qR, qG, qB;

    numcells = 1 << (numbits_red + numbits_green + numbits_blue);

    pOrderedInvMap = (OrderedInvMap *)malloc(numcells);
    if (pOrderedInvMap == NULL)
        return E_OUTOFMEMORY;

    pIM = pOrderedInvMap;

    for (qB = 0; qB < numcycles_blue; ++qB)
    {
        UCHAR b = BIT_EXPAND(qB, numbits_blue);

        for (qG = 0; qG < numcycles_green; ++qG)
        {
            UCHAR g = BIT_EXPAND(qG, numbits_green);

            for (qR = 0; qR < numcycles_red; ++qR)
            {
                UCHAR r = BIT_EXPAND(qR, numbits_red);

                BuildOrderedInvMapEntry(r, g, b,
                            pIM++);
            }
        }
    }

    return S_OK;
}

STDMETHODIMP
CInvPalSP_Ordered::Quantize(UCHAR *pucBGR, int x, int y,
                int num_pixels, UCHAR *pucIdx)
{
    UCHAR *pucDith;
    int num_dither, num_dither_left;

    num_dither = ditherInfo.diOrdered.width;
    num_dither_left = x % num_dither;
    pucDith = ditherInfo.diOrdered.pucDitherTable +
        (y % ditherInfo.diOrdered.height) * num_dither +
            num_dither_left;
    num_dither_left = num_dither - num_dither_left;

    while (num_pixels--)
    {
        UCHAR iR, iG, iB;
        UCHAR qR, qG, qB;
        UCHAR thresh;
        int err;

        thresh = *pucDith++;

        iR = pucBGR[2];
        iG = pucBGR[1];
        iB = pucBGR[0];

        qR = SHF_QUANT_FLR(iR, numshift_red);
        qG = SHF_QUANT_FLR(iG, numshift_green);
        qB = SHF_QUANT_FLR(iB, numshift_blue);

        err = iR - BIT_EXPAND(qR, numbits_red);
        if (err < 0)
        {
            --qR;
            err += numfrac_red;
            Assert(err >= 0);
        }
        err = CYC_EXPAND(err, numfrac_red);
        if (err >= thresh)
            ++qR;

        err = iG - BIT_EXPAND(qG, numbits_green);
        if (err < 0)
        {
            --qG;
            err += numfrac_green;
            Assert(err >= 0);
        }
        err = CYC_EXPAND(err, numfrac_green);
        if (err >= thresh)
            ++qG;

        err = iB - BIT_EXPAND(qB, numbits_blue);
        if (err < 0)
        {
            --qB;
            err += numfrac_blue;
            Assert(err >= 0);
        }
        err = CYC_EXPAND(err, numfrac_blue);
        if (err >= thresh)
            ++qB;

        Assert(qR < numcycles_red);
        Assert(qG < numcycles_green);
        Assert(qB < numcycles_blue);

        *pucIdx++ = pInvMap[BIT_PACKQUANT(qR, qG, qB, numbits_red,
                          numbits_green)];

        pucBGR += 3;

        if (--num_dither_left == 0)
        {
            num_dither_left = num_dither;
            pucDith -= num_dither;
        }
    }

    return S_OK;
}

STDMETHODIMP
CInvPalSP_Ordered::QuantizeCOLORREF(COLORREF rgb, int x, int y,
                    int num_pixels, UCHAR *pucIdx)
{
    UCHAR *pucDith;
    int num_dither, num_dither_left;

    num_dither = ditherInfo.diOrdered.width;
    num_dither_left = x % num_dither;
    pucDith = ditherInfo.diOrdered.pucDitherTable +
        (y % ditherInfo.diOrdered.height) * num_dither +
            num_dither_left;
    num_dither_left = num_dither - num_dither_left;

    while (num_pixels--)
    {
        UCHAR iR, iG, iB;
        UCHAR qR, qG, qB;
        UCHAR thresh;
        int err;

        thresh = *pucDith++;

        iR = GetRValue(rgb);
        iG = GetGValue(rgb);
        iB = GetBValue(rgb);

        qR = SHF_QUANT_FLR(iR, numshift_red);
        qG = SHF_QUANT_FLR(iG, numshift_green);
        qB = SHF_QUANT_FLR(iB, numshift_blue);

        err = iR - BIT_EXPAND(qR, numbits_red);
        if (err < 0)
        {
            --qR;
            err += numfrac_red;
            Assert(err >= 0);
        }
        err = CYC_EXPAND(err, numfrac_red);
        if (err >= thresh)
            ++qR;

        err = iG - BIT_EXPAND(qG, numbits_green);
        if (err < 0)
        {
            --qG;
            err += numfrac_green;
            Assert(err >= 0);
        }
        err = CYC_EXPAND(err, numfrac_green);
        if (err >= thresh)
            ++qG;

        err = iB - BIT_EXPAND(qB, numbits_blue);
        if (err < 0)
        {
            --qB;
            err += numfrac_blue;
            Assert(err >= 0);
        }
        err = CYC_EXPAND(err, numfrac_blue);
        if (err >= thresh)
            ++qB;

        Assert(qR < numcycles_red);
        Assert(qG < numcycles_green);
        Assert(qB < numcycles_blue);

        *pucIdx++ = pInvMap[BIT_PACKQUANT(qR, qG, qB, numbits_red,
                          numbits_green)];

        if (--num_dither_left == 0)
        {
            num_dither_left = num_dither;
            pucDith -= num_dither;
        }
    }

    return S_OK;
}


class CInvPalSP_ErrDiffused : public CInvPalSP
{
    Err *errBuf, *errBase[2];
    int last_max_width;
    int last_y;

public:
    CInvPalSP_ErrDiffused()
    {
        errBuf = NULL;
    }

    ~CInvPalSP_ErrDiffused()
    {
        if (errBuf)
            free(errBuf);
    }

    STDMETHODIMP
    Quantize(UCHAR *pucBGR, int x, int y, int num_pixels,
         UCHAR *pucIdx);

    STDMETHODIMP
    QuantizeCOLORREF(COLORREF rgb, int x, int y, int num_pixels,
             UCHAR *pucIdx);

    STDMETHODIMP
    ResetDither(int max_width);
};

STDMETHODIMP
CInvPalSP_ErrDiffused::Quantize(UCHAR *pucBGR, int x, int y,
                int num_pixels, UCHAR *pucIdx)
{
    Err *errs[2];
    BOOL reverse;
    PALETTEENTRY *pPalEnt;

    //
    // Assume if this y isn't the previous y that it's within 1 of
    // the previous, so we step the error scanlines. For any larger
    // jump, the user should call ResetDither()
    //
    if (last_y != y)
    {
        errs[0] = errBase[0];
        errBase[0] = errBase[1];
        errBase[1] = errs[0];

        memset(errBase[1], 0, sizeof(Err) * last_max_width);
    }

    reverse = (BOOL)(y & 1);

    errs[0] = errBase[0];
    errs[1] = errBase[1];

    if (reverse)
    {
        errs[0] += num_pixels - 1;
        errs[1] += num_pixels - 1;
        pucBGR += 3 * (num_pixels - 1);
        pucIdx += num_pixels - 1;
    }

    pPalEnt = pLPal->palPalEntry;

    while (num_pixels--)
    {
        short iR, iG, iB;
        int qR, qG, qB;
        Err err;

        iR = pucBGR[2] + DIV16(errs[0][0].r);
        iG = pucBGR[1] + DIV16(errs[0][0].g);
        iB = pucBGR[0] + DIV16(errs[0][0].b);

        iR = (iR < 0 ? 0 : iR > 255 ? 255 : iR);
        iG = (iG < 0 ? 0 : iG > 255 ? 255 : iG);
        iB = (iB < 0 ? 0 : iB > 255 ? 255 : iB);

        qR = quantTblR[iR];
        qG = quantTblG[iG];
        qB = quantTblB[iB];

        *pucIdx = pInvMap[BIT_PACKQUANT(qR, qG, qB, numbits_red,
                        numbits_green)];

        err.r = iR - pPalEnt[*pucIdx].peRed;
        err.g = iG - pPalEnt[*pucIdx].peGreen;
        err.b = iB - pPalEnt[*pucIdx].peBlue;

        //
        // Disperse the error with Floyd-Steinberg weights:
        //
        //      *    7/16
        // 1/16 5/16 3/16
        //
        // The weights are reflected in the reverse direction.
        //

        if (reverse)
        {
            errs[0][-1].r += MUL7(err.r);
            errs[1][-1].r += MUL3(err.r);
            errs[1][0].r += MUL5(err.r);
            errs[1][1].r += err.r;

            errs[0][-1].g += MUL7(err.g);
            errs[1][-1].g += MUL3(err.g);
            errs[1][0].g += MUL5(err.g);
            errs[1][1].g += err.g;

            errs[0][-1].b += MUL7(err.b);
            errs[1][-1].b += MUL3(err.b);
            errs[1][0].b += MUL5(err.b);
            errs[1][1].b += err.b;

            pucBGR -= 3;
            pucIdx--;
            errs[0]--;
            errs[1]--;
        }
        else
        {
            errs[0][1].r += MUL7(err.r);
            errs[1][1].r += MUL3(err.r);
            errs[1][0].r += MUL5(err.r);
            errs[1][-1].r += err.r;

            errs[0][1].g += MUL7(err.g);
            errs[1][1].g += MUL3(err.g);
            errs[1][0].g += MUL5(err.g);
            errs[1][-1].g += err.g;

            errs[0][1].b += MUL7(err.b);
            errs[1][1].b += MUL3(err.b);
            errs[1][0].b += MUL5(err.b);
            errs[1][-1].b += err.b;

            pucBGR += 3;
            pucIdx++;
            errs[0]++;
            errs[1]++;
        }
    }

    return S_OK;
}

STDMETHODIMP
CInvPalSP_ErrDiffused::QuantizeCOLORREF(COLORREF rgb, int x, int y,
                    int num_pixels, UCHAR *pucIdx)
{
    Err *errs[2];
    BOOL reverse;
    PALETTEENTRY *pPalEnt;

    //
    // Assume if this y isn't the previous y that it's within 1 of
    // the previous, so we step the error scanlines. For any larger
    // jump, the user should call ResetDither()
    //
    if (last_y != y)
    {
        errs[0] = errBase[0];
        errBase[0] = errBase[1];
        errBase[1] = errs[0];

        memset(errBase[1], 0, sizeof(Err) * last_max_width);
    }

    reverse = (BOOL)(y & 1);

    errs[0] = errBase[0];
    errs[1] = errBase[1];

    if (reverse)
    {
        errs[0] += num_pixels - 1;
        errs[1] += num_pixels - 1;
        pucIdx += num_pixels - 1;
    }

    pPalEnt = pLPal->palPalEntry;

    while (num_pixels--)
    {
        short iR, iG, iB;
        int qR, qG, qB;
        Err err;

        iR = GetRValue(rgb) + DIV16(errs[0][0].r);
        iG = GetGValue(rgb) + DIV16(errs[0][0].g);
        iB = GetBValue(rgb) + DIV16(errs[0][0].b);

        iR = (iR < 0 ? 0 : iR > 255 ? 255 : iR);
        iG = (iG < 0 ? 0 : iG > 255 ? 255 : iG);
        iB = (iB < 0 ? 0 : iB > 255 ? 255 : iB);

        qR = quantTblR[iR];
        qG = quantTblG[iG];
        qB = quantTblB[iB];

        *pucIdx = pInvMap[BIT_PACKQUANT(qR, qG, qB, numbits_red,
                        numbits_green)];

        err.r = iR - pPalEnt[*pucIdx].peRed;
        err.g = iG - pPalEnt[*pucIdx].peGreen;
        err.b = iB - pPalEnt[*pucIdx].peBlue;

        //
        // Disperse the error with Floyd-Steinberg weights:
        //
        //      *    7/16
        // 1/16 5/16 3/16
        //
        // The weights are reflected in the reverse direction.
        //

        if (reverse)
        {
            errs[0][-1].r += MUL7(err.r);
            errs[1][-1].r += MUL3(err.r);
            errs[1][0].r += MUL5(err.r);
            errs[1][1].r += err.r;

            errs[0][-1].g += MUL7(err.g);
            errs[1][-1].g += MUL3(err.g);
            errs[1][0].g += MUL5(err.g);
            errs[1][1].g += err.g;

            errs[0][-1].b += MUL7(err.b);
            errs[1][-1].b += MUL3(err.b);
            errs[1][0].b += MUL5(err.b);
            errs[1][1].b += err.b;

            pucIdx--;
            errs[0]--;
            errs[1]--;
        }
        else
        {
            errs[0][1].r += MUL7(err.r);
            errs[1][1].r += MUL3(err.r);
            errs[1][0].r += MUL5(err.r);
            errs[1][-1].r += err.r;

            errs[0][1].g += MUL7(err.g);
            errs[1][1].g += MUL3(err.g);
            errs[1][0].g += MUL5(err.g);
            errs[1][-1].g += err.g;

            errs[0][1].b += MUL7(err.b);
            errs[1][1].b += MUL3(err.b);
            errs[1][0].b += MUL5(err.b);
            errs[1][-1].b += err.b;

            pucIdx++;
            errs[0]++;
            errs[1]++;
        }
    }

    return S_OK;
}

STDMETHODIMP
CInvPalSP_ErrDiffused::ResetDither(int max_width)
{
    Err *err;

    if (errBuf == NULL)
    {
        err = (Err *)malloc(sizeof(Err) * (max_width + 2) * 2);
        if (err == NULL)
            return NULL;

        errBuf = err;
        last_max_width = max_width;
    }

    else if (last_max_width < max_width)
    {
        err = (Err *)realloc(errBuf, sizeof(Err) *
                     (max_width + 2) * 2);
        if (err == NULL)
            return NULL;

        errBuf = err;
        last_max_width = max_width;
    }

    errBase[0] = errBuf + 1;
    errBase[1] = errBuf + last_max_width + 3;

    memset(errBuf, 0, sizeof(Err) * (last_max_width + 2) * 2);

    last_y = (int)~0;

    return S_OK;
}


extern "C" IInvPalette *
InvPalette_CreateSpecifiedPaletteInv(LOGPALETTE *pLOGPALETTE,
                     int numbits_red, int numbits_green,
                     int numbits_blue,
                     DitherInfo *ditherInfo)
{
    CInvPalSP *pInvPalSP;
    DitherInfo di;

    if (ditherInfo == NULL)
    {
        di.ditherType = DITHER_SOLID;
        di.pfnDestroyDither = NULL;
    }
    else
        di = *ditherInfo;

    switch (di.ditherType)
    {
    default:
        return NULL;

    case DITHER_SOLID:
        pInvPalSP = new CInvPalSP_Solid;
        break;

    case DITHER_ORDERED:
        if (di.diOrdered.pucDitherTable == NULL)
            return NULL;
        pInvPalSP = new CInvPalSP_Ordered;
        break;

    case DITHER_ERRDIFFUSED:
        pInvPalSP = new CInvPalSP_ErrDiffused;
        break;
    }

    if (pInvPalSP == NULL)
        return NULL;

    if (FAILED(pInvPalSP->Init(pLOGPALETTE, numbits_red, numbits_green,
                   numbits_blue, &di)))
    {
        delete pInvPalSP;
        return NULL;
    }

    pInvPalSP->AddRef();

    return (IInvPalette *)pInvPalSP;
}


//
// Color Reduction using median cut
//

typedef struct
{
    UCHAR minR, maxR, minG, maxG, minB, maxB;
    ULONG sumR, sumG, sumB;
    ULONG cnt;
    int idx;
} Box;

typedef ULONG sum_t;
#define MAXCNT ((sum_t)0x001fffff)

class CColRedMC : public CColRed
{
    int numbits_red, numbits_green, numbits_blue;
    int numshift_red, numshift_green, numshift_blue;
    int numcycles_red, numcycles_green, numcycles_blue;
    int num_colors;
    sum_t *pHistogram;

    void
    ShrinkBox(Box *pBox);

    BOOL
    SplitBestBox(Box *pBoxes, int num_boxes);

    void
    SortBoxes(Box *pBoxes, int num_boxes);

public:
    CColRedMC()
    {
        pHistogram = NULL;
    }

    ~CColRedMC()
    {
        if (pHistogram)
            free(pHistogram);
    }

    HRESULT
    Init(int ncolors, int nbits_red, int nbits_green,
         int nbits_blue);

    STDMETHODIMP
    AddPixels(UCHAR *pucBGR, int num_pixels);

    STDMETHODIMP
    AddPixelCounts(UCHAR *pucBGR, USHORT *pCnts, int num_pixels);

    STDMETHODIMP_(LOGPALETTE *)
    GenerateLOGPALETTE();

    STDMETHODIMP
    Reset();
};

HRESULT
CColRedMC::Init(int ncolors, int nbits_red, int nbits_green,
        int nbits_blue)
{
    HRESULT hr;

    hr = CColRed::Init();
    if (FAILED(hr))
        return hr;

    num_colors = ncolors;

    numbits_red = nbits_red;
    numbits_green = nbits_green;
    numbits_blue = nbits_blue;

    numcycles_red = 1 << numbits_red;
    numcycles_green = 1 << numbits_green;
    numcycles_blue = 1 << numbits_blue;

    numshift_red = 8 - numbits_red;
    numshift_green = 8 - numbits_green;
    numshift_blue = 8 - numbits_blue;

    pHistogram = (sum_t *)malloc(sizeof(sum_t) * (1 << (numbits_red +
                                numbits_green +
                                numbits_blue)));
    if (pHistogram == NULL)
        return E_OUTOFMEMORY;

    return Reset();
}

HRESULT
CColRedMC::AddPixels(UCHAR *pucBGR, int num_pixels)
{
    while (num_pixels--)
    {
        UCHAR qR, qG, qB;
        int idx;

        qR = SHF_QUANT_MID(pucBGR[2], numshift_red);
        qG = SHF_QUANT_MID(pucBGR[1], numshift_green);
        qB = SHF_QUANT_MID(pucBGR[0], numshift_blue);

        idx = BIT_PACKQUANT(qR, qG, qB, numbits_red, numbits_green);

        if (pHistogram[idx] != MAXCNT)
            ++pHistogram[idx];

        pucBGR += 3;
    }

    return S_OK;
}

HRESULT
CColRedMC::AddPixelCounts(UCHAR *pucBGR, USHORT *pCnts, int num_pixels)
{
    while (num_pixels--)
    {
        UCHAR qR, qG, qB;
        int idx;

        qR = SHF_QUANT_MID(pucBGR[2], numshift_red);
        qG = SHF_QUANT_MID(pucBGR[1], numshift_green);
        qB = SHF_QUANT_MID(pucBGR[0], numshift_blue);

        idx = BIT_PACKQUANT(qR, qG, qB, numbits_red, numbits_green);

        if (pHistogram[idx] > MAXCNT - *pCnts)
            pHistogram[idx] = MAXCNT;
        else
            pHistogram[idx] += *pCnts;

        pucBGR += 3;
        pCnts++;
    }

    return S_OK;
}

void
CColRedMC::ShrinkBox(Box *pBox)
{
    Box minmax;
    UCHAR qR, qG, qB;
    int nR, nG, nB;

    Assert(pBox->minR <= pBox->maxR &&
           pBox->minG <= pBox->maxG &&
           pBox->minB <= pBox->maxB);
    Assert(pBox->maxR < numcycles_red &&
           pBox->maxG < numcycles_green &&
           pBox->maxB < numcycles_blue);

    minmax.minR = pBox->maxR;
    minmax.maxR = pBox->minR;
    minmax.minG = pBox->maxG;
    minmax.maxG = pBox->minG;
    minmax.minB = pBox->maxB;
    minmax.maxB = pBox->minB;
    minmax.cnt = 0;
    minmax.sumR = minmax.sumG = minmax.sumB = 0;

    nR = pBox->maxR - pBox->minR + 1;
    nG = pBox->maxG - pBox->minG + 1;
    nB = pBox->maxB - pBox->minB + 1;

    for (qB = pBox->minB; nB--; ++qB)
    {
        int nnG = nG;

        for (qG = pBox->minG; nnG--; ++qG)
        {
            int nnR = nR;

            for (qR = pBox->minR; nnR--; ++qR)
            {
                sum_t cnt;

                cnt = pHistogram[BIT_PACKQUANT(qR, qG, qB,
                                   numbits_red,
                                   numbits_green)];

                if (cnt > 0)
                {
                    if (qR < minmax.minR)
                        minmax.minR = qR;
                    if (qR > minmax.maxR)
                        minmax.maxR = qR;
                    if (qG < minmax.minG)
                        minmax.minG = qG;
                    if (qG > minmax.maxG)
                        minmax.maxG = qG;
                    if (qB < minmax.minB)
                        minmax.minB = qB;
                    if (qB > minmax.maxB)
                        minmax.maxB = qB;

                    minmax.sumR += qR * cnt;
                    minmax.sumG += qG * cnt;
                    minmax.sumB += qB * cnt;

                    minmax.cnt += cnt;
                }
            }
        }
    }

    pBox->minR = minmax.minR;
    pBox->maxR = minmax.maxR;
    pBox->minG = minmax.minG;
    pBox->maxG = minmax.maxG;
    pBox->minB = minmax.minB;
    pBox->maxB = minmax.maxB;
    pBox->sumR = minmax.sumR;
    pBox->sumG = minmax.sumG;
    pBox->sumB = minmax.sumB;
    pBox->cnt = minmax.cnt;

    return;
}

BOOL
CColRedMC::SplitBestBox(Box *pBoxes, int num_boxes)
{
    Box *pBox, *pBestBox;
    enum
    {
        DIM_R,
        DIM_G,
        DIM_B
    } dim;
    ULONG valBest;

    pBestBox = NULL;
    valBest = 0;

    for (pBox = pBoxes; pBox < pBoxes + num_boxes; ++pBox)
    {
        ULONG valR, valG, valB;

        if (pBox->cnt == 1)
            continue;

        valR = (pBox->maxR - pBox->minR) * pBox->cnt;
        valG = (pBox->maxG - pBox->minG) * pBox->cnt;
        valB = (pBox->maxB - pBox->minB) * pBox->cnt;

        if (valR >= valG && valR >= valB)
        {
            if (valR > valBest)
            {
                valBest = valR;
                pBestBox = pBox;
                dim = DIM_R;
            }
        }
        else if (valG >= valB)
        {
            if (valG > valBest)
            {
                valBest = valG;
                pBestBox = pBox;
                dim = DIM_G;
            }
        }
        else
        {
            if (valB > valBest)
            {
                valBest = valB;
                pBestBox = pBox;
                dim = DIM_B;
            }
        }
    }

    if (pBestBox == NULL)
        return FALSE;

    switch (dim)
    {
    case DIM_R:
        pBox->minR = pBestBox->minR;
        pBox->maxR = (UCHAR)(pBestBox->sumR / pBestBox->cnt);
        pBox->minG = pBestBox->minG;
        pBox->maxG = pBestBox->maxG;
        pBox->minB = pBestBox->minB;
        pBox->maxB = pBestBox->maxB;
        pBestBox->minR = pBox->maxR + 1;
        break;

    case DIM_G:
        pBox->minR = pBestBox->minR;
        pBox->maxR = pBestBox->maxR;
        pBox->minG = pBestBox->minG;
        pBox->maxG = (UCHAR)(pBestBox->sumG / pBestBox->cnt);
        pBox->minB = pBestBox->minB;
        pBox->maxB = pBestBox->maxB;
        pBestBox->minG = pBox->maxG + 1;
        break;

    case DIM_B:
        pBox->minR = pBestBox->minR;
        pBox->maxR = pBestBox->maxR;
        pBox->minG = pBestBox->minG;
        pBox->maxG = pBestBox->maxG;
        pBox->minB = pBestBox->minB;
        pBox->maxB = (UCHAR)(pBestBox->sumB / pBestBox->cnt);
        pBestBox->minB = pBox->maxB + 1;
        break;
    }

    ShrinkBox(pBestBox);
    ShrinkBox(pBox);

    return TRUE;
}

//
// Use an insertion sort over the box indices
//
void
CColRedMC::SortBoxes(Box *pBoxes, int num_boxes)
{
    Box *pBox = pBoxes;
    int i;

    for (i = 0; i < num_boxes; ++i)
        pBox[i].idx = i;

    for (i = 1; i < num_boxes; ++i)
    {
        int j;
        int x = pBox[i].idx;

        for (j = i; j > 0 &&
             pBox[pBox[j-1].idx].cnt < pBox[x].cnt; --j)
            pBox[j].idx = pBox[j-1].idx;
        pBox[j].idx = x;
    }
}

STDMETHODIMP_(LOGPALETTE *)
CColRedMC::GenerateLOGPALETTE()
{
    LOGPALETTE *pLPal;
    PALETTEENTRY *pPalEnt;
    Box *pBoxes, *pBox;
    int n, num_boxes;

    pBoxes = (Box *)malloc(sizeof(Box) * num_colors);
    if (pBoxes == NULL)
        return NULL;

    pLPal = (LOGPALETTE *)CoTaskMemAlloc(sizeof(LOGPALETTE) +
                         (num_colors - 1) *
                         sizeof(PALETTEENTRY));
    if (pLPal == NULL)
    {
        free(pBoxes);
        return NULL;
    }

    //
    // Create the first box, enclosing all points
    //
    pBox = pBoxes;

    pBox->minR = pBox->minG = pBox->minB = 0;
    pBox->maxR = numcycles_red - 1;
    pBox->maxG = numcycles_green - 1;
    pBox->maxB = numcycles_blue - 1;

    ShrinkBox(pBox);

    if (pBox->cnt == 0)
    {
        free(pBoxes);
        CoTaskMemFree(pLPal);
        return NULL;
    }

    for (num_boxes = 1; num_boxes < num_colors; ++num_boxes)
    {
        //
        // Split the best box into a new one, adding to the end
        // of the boxes array.
        //
        if (!SplitBestBox(pBoxes, num_boxes))
            break;
    }

    //
    // Now sort the boxes so that the more populated boxes get placed first
    //
    SortBoxes(pBoxes, num_boxes);

    pLPal->palVersion = 0x300;
    pLPal->palNumEntries = (WORD)num_boxes;

    for (pPalEnt = pLPal->palPalEntry, n = 0; n < num_boxes; ++n, ++pPalEnt)
    {
        pBox = &pBoxes[pBoxes[n].idx];

        pPalEnt->peRed = BIT_EXPAND(pBox->sumR / pBox->cnt,
                        numbits_red);
        pPalEnt->peGreen = BIT_EXPAND(pBox->sumG / pBox->cnt,
                          numbits_green);
        pPalEnt->peBlue = BIT_EXPAND(pBox->sumB / pBox->cnt,
                         numbits_blue);
        pPalEnt->peFlags = 0;
    }

    free(pBoxes);

    return pLPal;
}

STDMETHODIMP
CColRedMC::Reset()
{
    Assert(pHistogram != NULL);
    if (pHistogram == NULL)
        return E_FAIL;

    memset(pHistogram, 0, sizeof(sum_t) * (1 << (numbits_red +
                             numbits_green +
                             numbits_blue)));

    return S_OK;
}


extern "C" IColorReduce *
ColorReduce_CreateMedianCut(int num_colors, int numbits_red,
                int numbits_green, int numbits_blue)
{
    CColRedMC *pColRedMC;

    pColRedMC = new CColRedMC;
    if (pColRedMC == NULL)
        return NULL;

    if (FAILED(pColRedMC->Init(num_colors, numbits_red, numbits_green,
                   numbits_blue)))
    {
        delete pColRedMC;
        return NULL;
    }

    pColRedMC->AddRef();

    return (IColorReduce *)pColRedMC;
}


EXTERN_C _PAL_EXPORT HRESULT
ConvertDIBToDIB8(BITMAPINFO *pBMISrc, UCHAR *pucBitsSrc,
         IInvPalette *pInvPal, BITMAPINFO **ppBMIDst)
{
    BITMAPINFO *pBMIDst;
    UCHAR *pucBitsSrc0, *pBS, *pucBitsDst, *pucBits240, *pucBits24, *pB24;
    LOGPALETTE *pLogPal;
    PALETTEENTRY *pPalEnt;
    int bplSrc, bplDst;
    int ncolors;
    ULONG rmask, gmask, bmask;
    int rshift, gshift, bshift;
    int x, y;
    RGBQUAD *pRGBQ, *pQ;
    HRESULT hr;

    if (pBMISrc == NULL || pInvPal == NULL || ppBMIDst == NULL)
        return E_INVALIDARG;

    *ppBMIDst = NULL;

    //
    // Don't handle RLE.
    //
    if (pBMISrc->bmiHeader.biCompression != BI_RGB &&
        pBMISrc->bmiHeader.biCompression != BI_BITFIELDS)
        return E_INVALIDARG;

    //
    // Grab the LOGPALETTE and source we'll be using.
    //
    pLogPal = pInvPal->GetLOGPALETTE();
    if (pLogPal == NULL)
        return E_FAIL;

    bplSrc = ((pBMISrc->bmiHeader.biWidth *
           pBMISrc->bmiHeader.biBitCount + 31) >> 3) & ~3;

    pRGBQ = &pBMISrc->bmiColors[0];

    ncolors = pBMISrc->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBMISrc->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBMISrc->bmiHeader.biBitCount;

    //
    // Decode 16/32bpp with masks.
    //
    if (pBMISrc->bmiHeader.biBitCount == 16 ||
        pBMISrc->bmiHeader.biBitCount == 32)
    {
        if (pBMISrc->bmiHeader.biCompression == BI_BITFIELDS)
        {
            rmask = ((ULONG *)pRGBQ)[0];
            gmask = ((ULONG *)pRGBQ)[1];
            bmask = ((ULONG *)pRGBQ)[2];
            ncolors = 3;
        }
        else if (pBMISrc->bmiHeader.biBitCount == 16)
        {
            rmask = 0x7c00;
            gmask = 0x03e0;
            bmask = 0x001f;
        }
        else /* 32 */
        {
            rmask = 0xff0000;
            gmask = 0x00ff00;
            bmask = 0x0000ff;
        }

        for (rshift = 0; (rmask & 1) == 0; rmask >>= 1, ++rshift);
        if (rmask == 0)
            rmask = 1;
        for (gshift = 0; (gmask & 1) == 0; gmask >>= 1, ++gshift);
        if (gmask == 0)
            gmask = 1;
        for (bshift = 0; (bmask & 1) == 0; bmask >>= 1, ++bshift);
        if (bmask == 0)
            bmask = 1;
    }

    if (pucBitsSrc == NULL)
        pucBitsSrc = (UCHAR *)&pBMISrc->bmiColors[0] +
        ncolors * sizeof(RGBQUAD);
    pucBitsSrc0 = pucBitsSrc;

    //
    // Create the destination 8bpp DIB.
    //
    bplDst = (pBMISrc->bmiHeader.biWidth + 3) & ~3;

    pBMIDst = (BITMAPINFO *)CoTaskMemAlloc(sizeof(BITMAPINFOHEADER) +
                           pLogPal->palNumEntries *
                           sizeof(RGBQUAD) +
                           bplDst *
                           pBMISrc->bmiHeader.biHeight);
    if (pBMIDst == NULL)
        return E_OUTOFMEMORY;

    pBMIDst->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pBMIDst->bmiHeader.biWidth = pBMISrc->bmiHeader.biWidth;
    pBMIDst->bmiHeader.biHeight = pBMISrc->bmiHeader.biHeight;
    pBMIDst->bmiHeader.biPlanes = 1;
    pBMIDst->bmiHeader.biBitCount = 8;
    pBMIDst->bmiHeader.biCompression = BI_RGB;
    pBMIDst->bmiHeader.biXPelsPerMeter = 0;
    pBMIDst->bmiHeader.biYPelsPerMeter = 0;
    pBMIDst->bmiHeader.biSizeImage = bplDst * pBMIDst->bmiHeader.biHeight;
    pBMIDst->bmiHeader.biClrUsed = pLogPal->palNumEntries;
    pBMIDst->bmiHeader.biClrImportant = pLogPal->palNumEntries;

    for (y = 0, pQ = &pBMIDst->bmiColors[0],
             pPalEnt = &pLogPal->palPalEntry[0];
         y < pLogPal->palNumEntries; ++y, ++pQ, ++pPalEnt)
    {
        pQ->rgbRed = pPalEnt->peRed;
        pQ->rgbGreen = pPalEnt->peGreen;
        pQ->rgbBlue = pPalEnt->peBlue;
    }

    pucBitsDst = (UCHAR *)&pBMIDst->bmiColors[0] +
        pLogPal->palNumEntries * sizeof(RGBQUAD);

    //
    // If the source isn't 24bpp we need an intermediate buffer.
    //
    if (pBMISrc->bmiHeader.biBitCount == 24)
        pucBits240 = pucBitsSrc;
    else
    {
        pucBits240 =
            new UCHAR[(pBMISrc->bmiHeader.biWidth * 3 + 3) & ~3];

        if (pucBits240 == NULL)
        {
            CoTaskMemFree(pBMIDst);
            return E_OUTOFMEMORY;
        }
    }
    pucBits24 = pucBits240;

    //
    // Dither from the source into the dest.
    //
    pInvPal->ResetDither(pBMISrc->bmiHeader.biWidth);

    hr = S_OK;

    for (y = 0; y < pBMISrc->bmiHeader.biHeight; ++y)
    {
        pBS = pucBitsSrc;
        pB24 = pucBits24;

        switch (pBMISrc->bmiHeader.biBitCount)
        {
        case 1:
            for (x = pBMISrc->bmiHeader.biWidth; x >= 8; x -= 8)
            {
                pQ = &pRGBQ[(*pBS >> 7) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pBS >> 6) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pBS >> 5) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pBS >> 4) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pBS >> 3) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pBS >> 2) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pBS >> 1) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pBS++) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            if (x > 0)
            {
                int shf = 8;

                do
                {
                    pQ = &pRGBQ[(*pBS >> --shf) & 1];
                    *pB24++ = pQ->rgbBlue;
                    *pB24++ = pQ->rgbGreen;
                    *pB24++ = pQ->rgbRed;
                }
                while (--x);
            }

            break;

        case 4:
            for (x = pBMISrc->bmiHeader.biWidth; x >= 2; x -= 2)
            {
                pQ = &pRGBQ[(*pBS >> 4) & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[*pBS++ & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            if (x > 0)
            {
                pQ = &pRGBQ[(*pBS >> 4) & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                if (x > 1)
                {
                    pQ = &pRGBQ[*pBS & 0xf];
                    *pB24++ = pQ->rgbBlue;
                    *pB24++ = pQ->rgbGreen;
                    *pB24++ = pQ->rgbRed;
                }
            }

            break;

        case 8:
            for (x = pBMISrc->bmiHeader.biWidth; x--; )
            {
                pQ = &pRGBQ[*pBS++];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            break;

        case 16:
        {
            USHORT *pW;

            pW = (USHORT *)pucBitsSrc;

            for (x = pBMISrc->bmiHeader.biWidth; x--; )
            {
                ULONG w = *pW++;

                *pB24++ = (UCHAR)
                     ((((w >> bshift) & bmask) * 255) / bmask);
                *pB24++ = (UCHAR)
                     ((((w >> gshift) & gmask) * 255) / gmask);
                *pB24++ = (UCHAR)
                     ((((w >> rshift) & rmask) * 255) / rmask);
            }

            break;
        }

        case 24:
            pucBits24 = pucBitsSrc;
            break;

        case 32:
        {
            ULONG *pD;

            pD = (ULONG *)pucBitsSrc;

            for (x = pBMISrc->bmiHeader.biWidth; x--; )
            {
                ULONG d = *pD++;

                *pB24++ = (UCHAR)
                     ((((d >> bshift) & bmask) * 255) / bmask);
                *pB24++ = (UCHAR)
                     ((((d >> gshift) & gmask) * 255) / gmask);
                *pB24++ = (UCHAR)
                     ((((d >> rshift) & rmask) * 255) / rmask);
            }

            break;
        }

        default:
            delete[] pucBits24;
            return E_INVALIDARG;
        }

        hr = pInvPal->Quantize(pucBits24, 0, y,
                       pBMISrc->bmiHeader.biWidth,
                       pucBitsDst);
        if (FAILED(hr))
            break;

        pucBitsSrc += bplSrc;
        pucBitsDst += bplDst;
    }

    if (pucBits240 != pucBitsSrc0)
        delete[] pucBits240;

    if (FAILED(hr))
        CoTaskMemFree(pBMIDst);
    else
        *ppBMIDst = pBMIDst;

    return hr;
}
