#include "precomp.hxx" // PCH
#pragma hdrstop

#include "gdiobj.h"

CGDIObject::CGDIObject(
    HANDLE handle
    ) : m_handle(handle)
{

}

CGDIObject::~CGDIObject(
    VOID
    )
{
    if (NULL != m_handle)
    {
        if (0 == DeleteObject(m_handle))
        {
            DebugMsg(DM_ERROR, TEXT("CGDIObject::~CGDIObject, DeleteObject failed. obj: 0x%08X"), m_handle);
        }
    }
}


CFont::CFont(
    const LOGFONT& lf
    )
{
    *this = lf;
}


CFont& 
CFont::operator = (
    const LOGFONT& lf
    )
{
    if (NULL != m_handle)
    {
        if (0 == DeleteObject(m_handle))
            DebugMsg(DM_ERROR, TEXT("CFont::operator = , DeleteObject failed. obj: 0x%08X"), m_handle);

        m_handle = NULL;
    }
    if (NULL == (m_handle = CreateFontIndirect(&lf)))
        throw CGDIObject::Exception();

    return *this;
}

BOOL
CFont::GetLogFont(
    LPLOGFONT lpf
    ) const
{
    return (0 != ::GetObject(m_handle, sizeof(*lpf), lpf));
}


CDC::CDC(
    HWND hwnd
    ) throw(CGDIObject::Exception, OutOfMemory) :
      m_pRealDC(NULL)
{
    m_pRealDC = new WindowDC(m_handle, hwnd);
}

CDC::CDC(
    HDC hdc
    ) throw(CGDIObject::Exception, OutOfMemory) :
      m_pRealDC(NULL)
{
    m_pRealDC = new CompatibleDC(m_handle, hdc);
}

CDC::~CDC(
    VOID
    )
{
    delete m_pRealDC;
}


CDC::WindowDC::WindowDC(
    HANDLE& refHandle,
    HWND hwnd
    ) : RealDC(refHandle),
        m_hwnd(hwnd)
{
    if (NULL == (refHandle = (HANDLE)GetDC(m_hwnd)))
        throw CGDIObject::Exception();
}

CDC::CompatibleDC::CompatibleDC(
    HANDLE& refHandle,
    HDC hdc
    ) : RealDC(refHandle)
{
    if (NULL == (refHandle = (HANDLE)CreateCompatibleDC(hdc)))
        throw CGDIObject::Exception();
}

CDC::WindowDC::~WindowDC(
    VOID
    )
{
    if (NULL != m_refHandle)
    {
        ReleaseDC(m_hwnd, (HDC)m_refHandle);
        m_refHandle = NULL;
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("CDC::WindowDC::~WindowDC.  Null hdc.  obj: 0x%08X"), this);
    }
}

CDC::CompatibleDC::~CompatibleDC(
    VOID
    )
{
    if (NULL != m_refHandle)
    {
        DeleteDC((HDC)m_refHandle);
        m_refHandle = NULL;
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("CDC::CompatibleDC::~CompatibleDC.  Null hdc.  obj: 0x%08X"), this);
    }
}

CBrush::CBrush(
    VOID
    )
{

}
        
CBrush::CBrush(
    COLORREF clrRGB
    )
{
    *this = clrRGB;
}


CBrush& CBrush::operator = (
    COLORREF clrRGB
    )
{
    if (NULL != m_handle)
    {
        if (0 == DeleteObject(m_handle))
            DebugMsg(DM_ERROR, TEXT("CBrush::operator = , DeleteObject failed. obj: 0x%08X"), m_handle);
        m_handle = NULL;
    }
    m_handle = CreateSolidBrush(clrRGB);

    if (NULL == m_handle)
        throw CGDIObject::Exception();

    return *this;
}            

CDIB::CDIB(
    VOID
    ) : m_hPalette(NULL)
{
    ZeroMemory(&m_bitmap, sizeof(m_bitmap));
}


CDIB::CDIB(
    HINSTANCE hInstance,
    LPCTSTR pszResource
    ) : m_hPalette(NULL)
{
    Load(hInstance, pszResource);
}


CDIB::~CDIB(
    VOID
    )
{
    if (NULL != m_hPalette)
    {
        if (0 == DeleteObject(m_hPalette))
            DebugMsg(DM_ERROR, TEXT("CDIB::~CDIB, DeleteObject failed. obj: 0x%08X"), m_hPalette);
    }
}


VOID 
CDIB::Load(
    HINSTANCE hInstance,
    LPCTSTR pszResource
    )
{
    //
    // Delete palette and bitmap if they exist.
    //
    if (NULL != m_hPalette)
    {
        if (0 == DeleteObject(m_hPalette))
            DebugMsg(DM_ERROR, TEXT("CDIB::Load, DeleteObject failed. obj: 0x%08X"), m_hPalette);
        m_hPalette = NULL;
    }
    if (NULL != m_handle)
    {
        if (0 == DeleteObject(m_handle))
            DebugMsg(DM_ERROR, TEXT("CDIB::Load, DeleteObject failed. obj: 0x%08X"), m_handle);
        m_handle = NULL;
    }

    //
    // Load the palette and bitmap.
    //
    m_handle = LoadResourceBitmap(hInstance,
                                  pszResource,
                                  &m_hPalette);

    //
    // If something failed, leave the object in a known state and throw an exception.
    //
    if (NULL == m_handle || NULL == m_hPalette)
    {
        if (NULL != m_handle)
        {
            if (0 == DeleteObject(m_handle))
                DebugMsg(DM_ERROR, TEXT("CDIB::Load, DeleteObject failed. obj: 0x%08X"), m_handle);
            m_handle = NULL;
        }
        if (NULL != m_hPalette)
        {
            if (0 == DeleteObject(m_hPalette))
                DebugMsg(DM_ERROR, TEXT("CDIB::Load, DeleteObject failed. obj: 0x%08X"), m_hPalette);
            m_hPalette = NULL;
        }
        throw CGDIObject::Exception();
    }

    //
    // We have a bitmap and palette.  Now cache the bitmap info.
    //
    GetObject(m_handle, sizeof(m_bitmap), &m_bitmap);
}


VOID 
CDIB::GetRect(
    LPRECT prc
    )
{
    prc->left   = prc->top = 0;
    prc->right  = m_bitmap.bmWidth;
    prc->bottom = m_bitmap.bmHeight;
}


VOID 
CDIB::GetBitmapInfo(
    LPBITMAP pbm
    )
{
    *pbm = m_bitmap;
}


HPALETTE
CDIB::CreateDIBPalette(
    LPBITMAPINFO lpbmi, 
    LPINT lpiNumColors
    )
{
    LPBITMAPINFOHEADER  lpbi;
    LPLOGPALETTE     lpPal;
    HANDLE           hLogPal;
    HPALETTE         hPal = NULL;
    int              i;
 
    lpbi = (LPBITMAPINFOHEADER)lpbmi;
    if (lpbi->biBitCount <= 8)
        *lpiNumColors = (1 << lpbi->biBitCount);
    else
        *lpiNumColors = 0;  // No palette needed for 24 BPP DIB
 
    if (*lpiNumColors)
    {
        hLogPal = GlobalAlloc(GHND, 
                             sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * (*lpiNumColors));
        lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
        lpPal->palVersion    = 0x300;
        lpPal->palNumEntries = (unsigned short)(*lpiNumColors);
 
        for (i = 0;  i < *lpiNumColors;  i++)
        {
            lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
            lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
            lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
            lpPal->palPalEntry[i].peFlags = 0;
        }
        hPal = CreatePalette (lpPal);
        GlobalUnlock (hLogPal);
        GlobalFree   (hLogPal);
    }
    return hPal;
}


HBITMAP
CDIB::LoadResourceBitmap(
    HINSTANCE hInstance, 
    LPCTSTR lpString,
    HPALETTE *lphPalette
    )
{
    HRSRC  hRsrc;
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    int iNumColors;
 
    if (NULL != (hRsrc = FindResource(hInstance, lpString, RT_BITMAP)))
    {
        hGlobal = LoadResource(hInstance, hRsrc);
        lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);
 
        CDC dc;
        HPALETTE hpalOld = NULL;

        *lphPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
        if (*lphPalette)
        {
            hpalOld = SelectPalette(dc, *lphPalette, TRUE);
            RealizePalette(dc);
        }
 
        hBitmapFinal = CreateDIBitmap(dc,
                                      (LPBITMAPINFOHEADER)lpbi,
                                      (LONG)CBM_INIT,
                                      (LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD),
                                      (LPBITMAPINFO)lpbi,
                                      DIB_RGB_COLORS);
 
        if (NULL != hpalOld)
            SelectPalette(dc, hpalOld, TRUE);

        UnlockResource(hGlobal);
        FreeResource(hGlobal);
    }
    return (hBitmapFinal);
}


    