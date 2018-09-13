/////////////////////////////////////////////////////////////////////////////
// DIB.CPP
//
// Implementation of CDIB
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     02/03/97    Created
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "dib.h"

/////////////////////////////////////////////////////////////////////////////
// CDIB
/////////////////////////////////////////////////////////////////////////////
CDIB::CDIB
(
)
{
    m_pDIB = NULL;
    m_hPalette = NULL;
}

CDIB::~CDIB
(
)
{
    Cleanup();
}

/////////////////////////////////////////////////////////////////////////////
// CDIB::Cleanup
/////////////////////////////////////////////////////////////////////////////
void CDIB::Cleanup
(
)
{
    if (m_pDIB != NULL)
    {
        delete [] m_pDIB;
        m_pDIB = NULL;
    }

    if (m_hPalette != NULL)
    {
        EVAL(DeleteObject(m_hPalette));
        m_hPalette = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CDIB::LoadFromResource
/////////////////////////////////////////////////////////////////////////////
BOOL CDIB::LoadFromResource
(
    HINSTANCE   hInstance,
    WORD        wResID
)
{
    BOOL bResult = FALSE;

    for (;;)
    {
        ASSERT(m_pDIB == NULL);

        if ((m_pDIB = (BITMAP *)GetResource(hInstance,
                                            MAKEINTRESOURCE(wResID))) == NULL)
        {
            break;
        }

        ASSERT(m_hPalette == NULL);

        if ((m_hPalette = GetPalette()) == NULL)
            break;

        bResult = TRUE;
        break;
    }

    if (!bResult)
        Cleanup();

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CDIB::Draw
/////////////////////////////////////////////////////////////////////////////
void CDIB::Draw
(
    HDC     hDC,
    RECT *  prectSrc,
    RECT *  prectDest
)
{
    HPALETTE hPalOld = NULL;

    // Bail early if there is nothing to draw.
    if ((m_pDIB == NULL) || (prectDest == NULL))
        return;

    if (m_hPalette != NULL)
    {
        hPalOld = SelectPalette(hDC, m_hPalette, FALSE);
        RealizePalette(hDC);
    }

    SetStretchBltMode(hDC, COLORONCOLOR);
    SetDIBitsToDevice(  hDC, 
                        prectDest->left,
                        prectDest->top, 
                        ((prectSrc == NULL)
                            ? Width()
                            : (prectSrc->right - prectSrc->left)),
                        ((prectSrc == NULL)
                            ? Height()
                            : (prectSrc->bottom - prectSrc->top)),
                        ((prectSrc == NULL)
                            ? 0
                            : prectSrc->left),
                        ((prectSrc == NULL)
                            ? 0
                            : prectSrc->top),
                        0,
                        Height(),
                        GetBitsAddr(),
                        (BITMAPINFO *)m_pDIB,
                        DIB_RGB_COLORS);

    if (hPalOld != NULL)
        SelectPalette(hDC, hPalOld, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CDIB::GetResource
/////////////////////////////////////////////////////////////////////////////
void * CDIB::GetResource
(
    HINSTANCE       hInstance,
    const char *    pszBitmapName
)
{
    HRSRC   hResDIB = NULL;
    HGLOBAL hDIB = NULL;
    void *  pResource = NULL;
    void *  pDest = NULL;

    // We will attempt to load the DIB as a resource and
    // copy it to memory. This is done so we can free up
    // the resource and still have access to the information.
    if ((hResDIB = FindResource(hInstance,
                                pszBitmapName,
                                RT_BITMAP)) != NULL)
    {
        // We found the resource so load it up and save it for later.
        DWORD dwSize = SizeofResource(hInstance, hResDIB);
        if ((hDIB = LoadResource(hInstance, hResDIB)) != NULL)
        {
            // We have the bitmap. Make a copy and return the resource.
            pResource = (void *) LockResource(hDIB);

            if (pResource != NULL)
            {
                pDest = new BYTE [dwSize];

                if (pDest != NULL)
                    CopyMemory(pDest, pResource, dwSize);

                UnlockResource(hDIB);
            }

            FreeResource(hDIB);
        }
    }

    return pDest;
}

/////////////////////////////////////////////////////////////////////////////
// CDIB::GetPalette
/////////////////////////////////////////////////////////////////////////////
HPALETTE CDIB::GetPalette
(
)
{
    BITMAPINFOHEADER *  pBmpInfoHdr;
    HANDLE              hPalMem;
    LOGPALETTE *        pPal;
    HPALETTE            hPal;
    RGBQUAD *           pRGB;
    int                 iColors;

    pBmpInfoHdr = (BITMAPINFOHEADER *) m_pDIB;
    if (pBmpInfoHdr->biSize != sizeof(BITMAPINFOHEADER))
        return NULL;

    pRGB = (RGBQUAD *)((LPSTR)pBmpInfoHdr + (WORD)pBmpInfoHdr->biSize);
    if ((iColors = GetColorTableCount()) == 0)
        return NULL;

    hPalMem = LocalAlloc(LMEM_MOVEABLE,
                         sizeof(LOGPALETTE) + iColors * sizeof(PALETTEENTRY));
    if (!hPalMem)
        return NULL;

    pPal = (LOGPALETTE *) LocalLock(hPalMem);
    pPal->palVersion    = 0x300;
    pPal->palNumEntries = (WORD)iColors;

    for (int i=0; i < iColors; i++)
    {
        pPal->palPalEntry[i].peRed      = pRGB[i].rgbRed;
        pPal->palPalEntry[i].peGreen    = pRGB[i].rgbGreen;
        pPal->palPalEntry[i].peBlue     = pRGB[i].rgbBlue;
        pPal->palPalEntry[i].peFlags    = 0;
    }

    hPal = CreatePalette(pPal);
    LocalUnlock(hPalMem);
    LocalFree(hPalMem);

    return hPal;
}

/////////////////////////////////////////////////////////////////////////////
// CDIB::GetColorTableCount
/////////////////////////////////////////////////////////////////////////////
DWORD CDIB::GetColorTableCount
(
)
{
    DWORD dwNumColors;

    if (m_pDIB == NULL)
        return 0;

    if (GetInfoHeaderSize() >= 36)
        dwNumColors = ((BITMAPINFOHEADER *) m_pDIB)->biClrUsed;
    else
        dwNumColors = 0;

    if (dwNumColors == 0)
    {
        WORD wBitCount = ((BITMAPINFOHEADER *) m_pDIB)->biBitCount;

        if (wBitCount < 16)
            dwNumColors = 1L << wBitCount;
        else
            dwNumColors = 0; // For 16, 24 and 32 bits we return 0
    }

    return dwNumColors;
}

/////////////////////////////////////////////////////////////////////////////
// CDIB::GetBitsAddr
/////////////////////////////////////////////////////////////////////////////
void * CDIB::GetBitsAddr
(
)
{
    DWORD   dwColorTableSize;
    BYTE *  pVoid;

    dwColorTableSize = GetColorTableCount() * sizeof(RGBQUAD);
    
    // Compute the new addresse as a void so that we get true byte offsets
    pVoid = (BYTE *) m_pDIB;
    return (void *) (pVoid + GetInfoHeaderSize() + dwColorTableSize);
}
