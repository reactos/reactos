
#include "stdafx.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgwell.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

#define DSx     0x00660046L
#define DSna    0x00220326L


CImageWell::CImageWell()
    : m_bitmap(), m_dc(), m_mask(), m_maskDC(), m_imageSize(0, 0)
{
    m_nBitmapID = 0;
}


CImageWell::CImageWell(UINT nBitmapID, CSize imageSize)
    : m_bitmap(), m_dc(), m_mask(), m_maskDC(), m_imageSize(imageSize)
{
    m_nBitmapID = nBitmapID;
}


CImageWell::~CImageWell()
{
    // Destructor needed to ensure the DC is deleted before the bitmap
    m_dc.DeleteDC();
    m_bitmap.DeleteObject();
    m_maskDC.DeleteDC();
    m_mask.DeleteObject();
}


BOOL CImageWell::Load(UINT nBitmapID, CSize imageSize)
{
    ASSERT(m_bitmap.m_hObject == NULL);

    if (!m_bitmap.LoadBitmap(nBitmapID))
    {
        TRACE1("Could not load image well %d\n", nBitmapID);
        return FALSE;
    }

    m_nBitmapID = nBitmapID;
    m_imageSize = imageSize;

    return TRUE;
}


void CImageWell::Unload()
{
    ASSERT(m_dc.m_hDC == NULL); // can't unload if it's open!
    ASSERT(m_maskDC.m_hDC == NULL);
    ASSERT(m_bitmap.m_hObject != NULL);

    m_bitmap.DeleteObject();
    m_mask.DeleteObject();
}


BOOL CImageWell::CalculateMask()
{
    ASSERT(m_maskDC.m_hDC == NULL);
    ASSERT(m_dc.m_hDC != NULL);
    ASSERT(m_bitmap.m_hObject != NULL);

    if (!m_maskDC.CreateCompatibleDC(NULL))
    {
        theApp.SetGdiEmergency(FALSE);
        return FALSE;
    }

    if (m_mask.m_hObject != NULL)
    {
        VERIFY(m_maskDC.SelectObject(&m_mask) != NULL);
        return TRUE;
    }

    BITMAP bmp;
    m_bitmap.GetObject(sizeof (BITMAP), &bmp);

    if (!m_mask.CreateBitmap(bmp.bmWidth, bmp.bmHeight*2, 1, 1, NULL))
    {
        m_maskDC.DeleteDC();
        theApp.SetMemoryEmergency(FALSE);
        return FALSE;
    }

    VERIFY(m_maskDC.SelectObject(&m_mask) != NULL);

    COLORREF oldBkColor = m_dc.SetBkColor(m_dc.GetPixel(0, 0));
    m_maskDC.BitBlt(0, 0, bmp.bmWidth, bmp.bmHeight,
        &m_dc, 0, 0, NOTSRCCOPY);
    
    // store the pixels in "button text" color in the second part of the mask
    
    m_dc.SetBkColor(RGB(0, 0, 0));
    m_maskDC.BitBlt(0, bmp.bmHeight, bmp.bmWidth, bmp.bmHeight,
        &m_dc, 0, 0, SRCCOPY);
    
    m_dc.SetBkColor(oldBkColor);

    return TRUE;
}


BOOL CImageWell::Open()
{
    ASSERT(m_dc.m_hDC == NULL); // make sure this is not already open
    ASSERT(m_nBitmapID != 0);

    if (m_bitmap.m_hObject == NULL && !Load(m_nBitmapID, m_imageSize))
        return FALSE;

    if (!m_dc.CreateCompatibleDC(NULL))
    {
        theApp.SetGdiEmergency(FALSE);
        return FALSE;
    }

    VERIFY(m_dc.SelectObject(&m_bitmap));

    return TRUE;
}


void CImageWell::Close()
{
    ASSERT(m_dc.m_hDC != NULL);

    m_dc.DeleteDC();
    m_maskDC.DeleteDC();
}

BOOL CImageWell::DrawImage(CDC* pDestDC, CPoint destPoint,
    UINT nImage, DWORD rop)
{
    BOOL bClose = FALSE;
    if (m_dc.m_hDC == NULL)
    {
        ASSERT(rop != NULL); // must open first and calc mask for rop==0!

        if (!Open())
        {
            TRACE(TEXT("Could not open image well!\n"));
            return FALSE;
        }

        bClose = TRUE;
    }

    if (rop == 0)
    {
        if (m_maskDC.m_hDC == NULL && !CalculateMask())
            return FALSE;

        COLORREF oldBkColor = pDestDC->SetBkColor(RGB(255, 255, 255));
        COLORREF oldTextColor = pDestDC->SetTextColor(RGB(0, 0, 0));

        pDestDC->BitBlt(destPoint.x, destPoint.y,
            m_imageSize.cx, m_imageSize.cy,
            &m_dc, m_imageSize.cx * nImage, 0, DSx);

        pDestDC->BitBlt(destPoint.x, destPoint.y,
            m_imageSize.cx, m_imageSize.cy,
            &m_maskDC, m_imageSize.cx * nImage, 0, DSna);

        pDestDC->BitBlt(destPoint.x, destPoint.y,
            m_imageSize.cx, m_imageSize.cy,
            &m_dc, m_imageSize.cx * nImage, 0, DSx);

        pDestDC->SetBkColor(GetSysColor(COLOR_BTNTEXT));

        pDestDC->BitBlt(destPoint.x, destPoint.y,
            m_imageSize.cx, m_imageSize.cy,
            &m_maskDC, m_imageSize.cx * nImage, m_imageSize.cy, SRCPAINT);

        pDestDC->SetBkColor(oldBkColor);
        pDestDC->SetTextColor(oldTextColor);
    }
    else
    {
        pDestDC->BitBlt(destPoint.x, destPoint.y,
            m_imageSize.cx, m_imageSize.cy,
            &m_dc, m_imageSize.cx * nImage, 0, rop);
    }

    if (bClose)
        Close();

    return TRUE;
}
