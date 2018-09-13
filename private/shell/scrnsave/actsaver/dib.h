/////////////////////////////////////////////////////////////////////////////
// DIB.H
//
// Declaration of CDIB
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     02/03/97    Created
/////////////////////////////////////////////////////////////////////////////
#ifndef __DIB_H__
#define __DIB_H__

/////////////////////////////////////////////////////////////////////////////
// Macros
/////////////////////////////////////////////////////////////////////////////
#define IS_WIN30_DIB(lpbi)      ((*(DWORD *)(pbi)) == sizeof(BITMAPINFOHEADER))

#define QUAD_EQUAL(Q1, Q2)      ((Q1.rgbRed == Q2.rgbRed) && (Q1.rgbGreen == Q2.rgbGreen) && (Q1.rgbBlue == Q2.rgbBlue))

#define DIB_WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
    // DIB_WIDTHBYTES performs DWORD-aligning of DIB scanlines.  The "bits"
    // parameter is the bit count for the scanline (biWidth * biBitCount),
    // and this macro returns the number of DWORD-aligned bytes needed 
    // to hold those bits.

/////////////////////////////////////////////////////////////////////////////
// CDIB
/////////////////////////////////////////////////////////////////////////////
class CDIB
{
// Construction/destruction
public:
    CDIB();
    virtual ~CDIB();

// Functions
    BOOL LoadFromResource(HINSTANCE hInstance, WORD wResID);
    void Draw(HDC hDC, RECT * prectSrc, RECT * prectDest);

    DWORD Width()
    { return ((m_pDIB == NULL) ? 0 : (((BITMAPINFOHEADER *) m_pDIB)->biWidth)); }

    DWORD Height()
    { return ((m_pDIB == NULL) ? 0 : (((BITMAPINFOHEADER *) m_pDIB)->biHeight)); }

protected:
    void *      GetResource(HINSTANCE hInstance, const char * pszDIBName);
    DWORD       GetColorTableCount();
    void *      GetBitsAddr();

    DWORD       GetInfoHeaderSize()
    { return ((m_pDIB == NULL) ? NULL : ((BITMAPINFOHEADER *) m_pDIB)->biSize); }

    void *      GetColorTable()
    {
        return ((m_pDIB == NULL)
                    ? NULL
                    : (void *) (((BYTE *) m_pDIB) + GetInfoHeaderSize()));
    }

    HPALETTE    GetPalette();

    void        Cleanup();

// Data
public:
    void *      m_pDIB;
    HPALETTE    m_hPalette;
};

#endif // __DIB_H__
