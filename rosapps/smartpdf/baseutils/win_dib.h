#ifndef __DIB_h__
#define __DIB_h__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// CDIB.h : header file
//

// Copyright © Dundas Software Ltd. 1999, All Rights Reserved

// //////////////////////////////////////////////////////////////////////////

// Properties:
//	NO	Abstract class (does not have any objects)
//	NO	Derived from CWnd
//	NO	Is a CWnd.
//	NO	Two stage creation (constructor & Create())
//	NO	Has a message map
//	NO 	Needs a resource (template)
//	YES	Persistent objects (saveable on disk)
//	YES	Uses exceptions

// //////////////////////////////////////////////////////////////////////////

// Desciption :

// CDIBSectionLite is DIBSection wrapper class for win32 and WinCE platforms.
// This class provides a simple interface to DIBSections including loading,
// saving and displaying DIBsections.
//
// Full palette support is provided for Win32 and CE 2.11 and above.

// Using CDIBSectionLite :

// This class is very simple to use. The bitmap can be set using either SetBitmap()
// (which accepts either a Device dependant or device independant bitmap, or a
// resource ID) or by using Load(), which allows an image to be loaded from disk.
// To display the bitmap simply use Draw or Stretch.
//
// eg.
//
//      CDIBsection dibsection;
//      dibsection.Load(_T("image.bmp"));
//      dibsection.Draw(pDC, CPoint(0,0));  // pDC is of type CDC*
//
//      CDIBsection dibsection;
//      dibsection.SetBitmap(IDB_BITMAP);
//      dibsection.Draw(pDC, CPoint(0,0));  // pDC is of type CDC*
//
// The CDIBsection API includes many methods to extract information about the
// image, as well as palette options for getting and setting the current palette.
//
// Author   : Chris Maunder (cmaunder@mail.com)
// Date     : 12 April 1999

// Modified : Kenny Goers (kennyg@magenic.com)
// Date     : 12 December 2000
// Why      : Remove all MFC bloat

// CDIB.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// defines

//#define DIBSECTION_NO_DITHER          // Disallow dithering via DrawDib functions
#define DIBSECTION_NO_MEMDC_REUSE       // Disallow the reuse of memory DC's
//#define DIBSECTION_NO_PALETTE         // Remove palette support

// Only provide palette support for non-CE platforms, or for CE 2.11 and above
#define DIBSECTION_NO_DITHER            // DrawDib not supported on CE
#if (_WIN32_WCE < 211)
#  define DIBSECTION_NO_PALETTE           // No palette support on early CE devices
#endif

#define DS_BITMAP_FILEMARKER  ((WORD) ('M' << 8) | 'B')    // is always "BM" = 0x4D42

/////////////////////////////////////////////////////////////////////////////
// BITMAPINFO wrapper

struct DIBINFO : public BITMAPINFO
{
	RGBQUAD	 arColors[255];    // Color table info - adds an extra 255 entries to palette

	operator LPBITMAPINFO()          { return (LPBITMAPINFO) this; }
	operator LPBITMAPINFOHEADER()    { return &bmiHeader;          }
	RGBQUAD* ColorTable()            { return bmiColors;           }
};

/////////////////////////////////////////////////////////////////////////////
// LOGPALETTE wrapper

#ifndef DIBSECTION_NO_PALETTE
struct PALETTEINFO : public LOGPALETTE
{
    PALETTEENTRY arPalEntries[255];               // Palette entries

    PALETTEINFO()
    {
        palVersion    = (WORD) 0x300;
        palNumEntries = 0;
        ::memset(palPalEntry, 0, 256*sizeof(PALETTEENTRY));
    }

    operator LPLOGPALETTE()   { return (LPLOGPALETTE) this;            }
    operator LPPALETTEENTRY() { return (LPPALETTEENTRY) (palPalEntry); }
};
#endif // DIBSECTION_NO_PALETTE


/////////////////////////////////////////////////////////////////////////////
// CeDIB object

class CeDIB
{
// Construction
public:
	CeDIB();
	virtual ~CeDIB();

// static helpers
public:
    static int BytesPerLine(int nWidth, int nBitsPerPixel);
    static int NumColorEntries(int nBitsPerPixel, int nCompression);

    static RGBQUAD ms_StdColors[];
#ifndef DIBSECTION_NO_PALETTE
    static BOOL UsesPalette(HDC hDC)
		{ return (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE); }
    static BOOL CreateHalftonePalette(HPALETTE palette, int nNumColors);
#endif // DIBSECTION_NO_PALETTE

// Attributes
public:
    HBITMAP      GetSafeHandle() const       { return (this)? m_hBitmap : NULL;        }
    operator     HBITMAP() const             { return GetSafeHandle();                 }
	void		 GetSize(SIZE& size) const   { size.cx = GetWidth(); size.cy = GetHeight(); }
    int          GetHeight() const           { return m_DIBinfo.bmiHeader.biHeight;    }
    int          GetWidth() const            { return m_DIBinfo.bmiHeader.biWidth;     }
    int          GetPlanes() const           { return m_DIBinfo.bmiHeader.biPlanes;    }
    int          GetBitCount() const         { return m_DIBinfo.bmiHeader.biBitCount;  }
    LPVOID       GetDIBits()                 { return m_ppvBits;                       }
    LPBITMAPINFO GetBitmapInfo()             { return  (BITMAPINFO*) m_DIBinfo;        }
    DWORD        GetImageSize() const        { return m_DIBinfo.bmiHeader.biSizeImage; }
    LPBITMAPINFOHEADER GetBitmapInfoHeader() { return (BITMAPINFOHEADER*) m_DIBinfo;   }

// Operations (Palette)
public:
    LPRGBQUAD GetColorTable()				{ return m_DIBinfo.ColorTable();          }
    BOOL      SetColorTable(UINT nNumColors, RGBQUAD *pColors);
    int       GetColorTableSize()			{ return m_iColorTableSize;               }
#ifndef DIBSECTION_NO_PALETTE
    HPALETTE  GetPalette()					{ return m_hPal; }
    BOOL      SetPalette(HPALETTE pPalette);
    BOOL      SetLogPalette(LOGPALETTE* pLogPalette);
#endif // DIBSECTION_NO_PALETTE

// Operations (Setting the bitmap)
public:
    BOOL SetBitmap(UINT nIDResource, HINSTANCE hInst = NULL);
    BOOL SetBitmap(LPCTSTR lpszResourceName, HINSTANCE hInst = NULL);
    BOOL SetBitmap(HBITMAP hBitmap, HPALETTE hPal = NULL);
    BOOL SetBitmap(LPBITMAPINFO lpBitmapInfo, LPVOID lpBits);

    BOOL Load(LPCTSTR lpszFileName);
    BOOL Save(LPCTSTR lpszFileName);
    BOOL Copy(CeDIB& Bitmap);

// Operations (Display)
public:
    BOOL Draw(HDC hDC, POINT& ptDest, BOOL bForceBackground = FALSE);
    BOOL Stretch(HDC hDC, POINT& ptDest, SIZE& size, BOOL bForceBackground = FALSE);

    HDC GetMemoryDC(HDC hDC = NULL, BOOL bSelectPalette = TRUE);
    BOOL ReleaseMemoryDC(BOOL bForceRelease = FALSE);

// Overrideables

// Implementation
public:

// Implementation
protected:
#ifndef DIBSECTION_NO_PALETTE
    BOOL CreatePalette();
    BOOL FillDIBColorTable(UINT nNumColors, RGBQUAD *pRGB);
#endif // DIBSECTION_NO_PALETTE
    UINT GetColorTableEntries(HDC hdc, HBITMAP hBitmap);

protected:
    HBITMAP  m_hBitmap;          // Handle to DIBSECTION
    DIBINFO  m_DIBinfo;          // Bitmap header & color table info
    VOID    *m_ppvBits;          // Pointer to bitmap bits
    UINT     m_iColorDataType;   // color data type (palette or RGB values)
    UINT     m_iColorTableSize;  // Size of color table

    HDC      m_hMemDC;            // Memory DC for drawing on bitmap

#ifndef DIBSECTION_NO_MEMDC_REUSE
    BOOL     m_bReuseMemDC;      // Reeuse the memory DC? (Quicker, but not fully tested)
#endif

#ifndef DIBSECTION_NO_PALETTE
    HPALETTE m_hPal;         // Color palette
    HPALETTE m_hOldPal;
#endif // DIBSECTION_NO_PALETTE

private:
    HBITMAP  m_hOldBitmap;      // Storage for previous bitmap in Memory DC
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CeDIB_H__35D9F3D4_B960_11D2_A981_2C4476000000__INCLUDED_)
