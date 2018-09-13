/*----------------------------------------------------------------------------*\
|   Routines for dealing with Device independent bitmaps                       |
|									       |
|   History:                                                                   |
|       06/23/89 toddla     Created                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>
#include <ntavi.h>
#include "ddtest.h"
#include "dib.h"

HANDLE CreateLogicalDib(HBITMAP hbm, UINT biBits, HPALETTE hpal);

static DWORD NEAR PASCAL lread(int fh, VOID FAR *pv, DWORD ul);
static DWORD NEAR PASCAL lwrite(int fh, VOID FAR *pv, DWORD ul);

/* flags for _lseek */
#define  SEEK_CUR 1
#define  SEEK_END 2
#define  SEEK_SET 0

/*
 *   Open a DIB file and return a MEMORY DIB, a memory handle containing..
 *
 *   BITMAP INFO    bi
 *   palette data
 *   bits....
 *
 */
HANDLE OpenDIB(LPTSTR szFile, int fh)
{
    BITMAPINFOHEADER	bi;
    LPBITMAPINFOHEADER  lpbi;
    DWORD		dwLen;
    DWORD		dwBits;
    HANDLE		hdib;
    HANDLE              h;
#ifndef UNICODE
    OFSTRUCT            of;
#endif
    BOOL fOpened = FALSE;

    if (szFile != NULL) {
#ifdef UNICODE
        DPF(("Opening DIB %ls", szFile));
        fh = (int)CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
        fh = OpenFile(szFile, &of, OF_READ);
#endif
	fOpened = TRUE;
    }

    if (fh == -1)
	return NULL;

    hdib = ReadDibBitmapInfo(fh);

    if (!hdib)
	return NULL;

    DibInfo((LPBITMAPINFOHEADER)GlobalLock(hdib),&bi);  GlobalUnlock(hdib);

    /* How much memory do we need to hold the DIB */

    dwBits = bi.biSizeImage;
    dwLen  = bi.biSize + PaletteSize(&bi) + dwBits;

    /* Can we get more memory? */

    h = GlobalReAlloc(hdib,dwLen,0);

    if (!h)
    {
	GlobalFree(hdib);
	hdib = NULL;
    }
    else
    {
	hdib = h;
    }

    if (hdib)
    {
        lpbi = (VOID FAR *)GlobalLock(hdib);

	/* read in the bits */
        lread(fh, (LPBYTE)lpbi + (UINT)lpbi->biSize + PaletteSize(lpbi), dwBits);

	GlobalUnlock(hdib);
    }

    if (fOpened)
        _lclose(fh);

    return hdib;
}

/*
 *   Write a global handle in CF_DIB format to a file.
 *
 */
BOOL WriteDIB(LPTSTR szFile,int fh, HANDLE hdib)
{
    BITMAPFILEHEADER	hdr;
    LPBITMAPINFOHEADER  lpbi;
    BITMAPINFOHEADER    bi;
    DWORD               dwSize;
    BOOL fOpened = FALSE;

    if (!hdib)
	return FALSE;

    if (szFile != NULL) {	
#ifdef UNICODE
        DPF(("Writing DIB %ls", szFile));
        fh = (int)CreateFile(szFile,GENERIC_WRITE | GENERIC_READ, 0,
                             NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#else
        OFSTRUCT            of;
        fh = OpenFile(szFile,&of,OF_CREATE|OF_READWRITE);
#endif
        fOpened = TRUE;
    }

    if (fh == -1)
        return FALSE;

    lpbi = (VOID FAR *)GlobalLock(hdib);
    DibInfo(lpbi,&bi);

    dwSize = bi.biSize + PaletteSize(&bi) + bi.biSizeImage;


    hdr.bfType		= BFT_BITMAP;
    hdr.bfSize          = dwSize + sizeof(BITMAPFILEHEADER);
    hdr.bfReserved1     = 0;
    hdr.bfReserved2     = 0;
    hdr.bfOffBits       = (DWORD)sizeof(BITMAPFILEHEADER) + lpbi->biSize +
                          PaletteSize(lpbi);

    _lwrite(fh,(LPVOID)&hdr,sizeof(BITMAPFILEHEADER));
    lwrite(fh,(LPVOID)lpbi,dwSize);

    GlobalUnlock(hdib);

    if (fOpened)
        _lclose(fh);

    return TRUE;
}

/*
 *  DibInfo(hbi, lpbi)
 *
 *  retrives the DIB info associated with a CF_DIB format memory block.
 */
BOOL  DibInfo(LPBITMAPINFOHEADER lpbiSource, LPBITMAPINFOHEADER lpbiTarget)
{
    if (lpbiSource)
    {
	*lpbiTarget = *lpbiSource;

        if (lpbiTarget->biSize == sizeof(BITMAPCOREHEADER))
        {
            BITMAPCOREHEADER bc;

            bc = *(LPBITMAPCOREHEADER)lpbiTarget;

            lpbiTarget->biSize               = sizeof(BITMAPINFOHEADER);
            lpbiTarget->biWidth              = (DWORD)bc.bcWidth;
            lpbiTarget->biHeight             = (DWORD)bc.bcHeight;
            lpbiTarget->biPlanes             =  (UINT)bc.bcPlanes;
            lpbiTarget->biBitCount           =  (UINT)bc.bcBitCount;
            lpbiTarget->biCompression        = BI_RGB;
            lpbiTarget->biSizeImage          = 0;
            lpbiTarget->biXPelsPerMeter      = 0;
            lpbiTarget->biYPelsPerMeter      = 0;
            lpbiTarget->biClrUsed            = 0;
            lpbiTarget->biClrImportant       = 0;
        }

        /*
         * fill in the default fields
         */
        if (lpbiTarget->biSize != sizeof(BITMAPCOREHEADER))
        {
            if (lpbiTarget->biSizeImage == 0L)
                lpbiTarget->biSizeImage = (DWORD)DIBWIDTHBYTES(*lpbiTarget) * lpbiTarget->biHeight;

            if (lpbiTarget->biClrUsed == 0L)
                lpbiTarget->biClrUsed = DibNumColors(lpbiTarget);
        }
	return TRUE;
    }
    return FALSE;
}

HPALETTE CreateColorPalette()
{
    LOGPALETTE          *pPal;
    HPALETTE            hpal = NULL;
    UINT                nNumColors;
    BYTE                red;
    BYTE                green;
    BYTE                blue;
    int                 i;

    nNumColors = MAXPALETTE;
    pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));

    if (!pPal)
	goto exit;

    pPal->palNumEntries = nNumColors;
    pPal->palVersion	= PALVERSION;

    red = green = blue = 0;

    for (i = 0; i < pPal->palNumEntries; i++)
    {
	pPal->palPalEntry[i].peRed   = red;
	pPal->palPalEntry[i].peGreen = green;
	pPal->palPalEntry[i].peBlue  = blue;
	pPal->palPalEntry[i].peFlags = (BYTE)0;

	if (!(red += 32))
	    if (!(green += 32))
		blue += 64;
    }

    hpal = CreatePalette(pPal);
    LocalFree((HANDLE)pPal);

exit:
    return hpal;
}

/*
 *  CreateBIPalette()
 *
 *  Given a Pointer to a BITMAPINFO struct will create a
 *  a GDI palette object from the color table.
 *
 *  works with "old" and "new" DIB's
 *
 */
HPALETTE CreateBIPalette(LPBITMAPINFOHEADER lpbi)
{
    LOGPALETTE          *pPal;
    HPALETTE            hpal = NULL;
    UINT                nNumColors;
    UINT                i;
    RGBQUAD        FAR *pRgb;
    BOOL                fCoreHeader;

    if (!lpbi)
	return NULL;

    nNumColors = DibNumColors(lpbi);

    if (nNumColors)
    {
		fCoreHeader = (lpbi->biSize == sizeof(BITMAPCOREHEADER));

		pRgb = (RGBQUAD FAR *)((LPBYTE)lpbi + (UINT)lpbi->biSize);
		pPal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + nNumColors * sizeof(PALETTEENTRY));

		if (!pPal)
			goto exit;

		pPal->palNumEntries = nNumColors;
		pPal->palVersion    = PALVERSION;

		for (i = 0; i < nNumColors; i++)
		{
			pPal->palPalEntry[i].peRed	 = pRgb->rgbRed;
			pPal->palPalEntry[i].peGreen = pRgb->rgbGreen;
			pPal->palPalEntry[i].peBlue  = pRgb->rgbBlue;
			pPal->palPalEntry[i].peFlags = (BYTE)0;

			if (fCoreHeader)
				((LPBYTE)pRgb) += sizeof(RGBTRIPLE) ;
			else
				pRgb++;
		}

		hpal = CreatePalette(pPal);
		LocalFree((HANDLE)pPal);
    }
    else if (lpbi->biBitCount == 24)
    {
	hpal = CreateColorPalette();
    }

exit:
    return hpal;
}


/*
 *  CreateDibPalette()
 *
 *  Given a Global HANDLE to a BITMAPINFO Struct
 *  will create a GDI palette object from the color table.
 *
 *  works with "old" and "new" DIB's
 *
 */
HPALETTE CreateDibPalette(HANDLE hbi)
{
    HPALETTE hpal;

    if (!hbi)
	return NULL;

    hpal = CreateBIPalette((LPBITMAPINFOHEADER)GlobalLock(hbi));
    GlobalUnlock(hbi);
    return hpal;
}

HPALETTE CreateExplicitPalette()
{
    PLOGPALETTE ppal;
    HPALETTE    hpal;
    int         i;
    #define     NPAL   256

    ppal = (LOGPALETTE*)LocalAlloc(LPTR,sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * NPAL);

    ppal->palVersion    = 0x300;
    ppal->palNumEntries = NPAL;

    for (i=0; i<NPAL; i++)
    {
        ppal->palPalEntry[i].peFlags = (BYTE)PC_EXPLICIT;
        ppal->palPalEntry[i].peRed   = (BYTE)i;
        ppal->palPalEntry[i].peGreen = (BYTE)0;
        ppal->palPalEntry[i].peBlue  = (BYTE)0;
    }

    hpal = CreatePalette(ppal);
    LocalFree((HANDLE)ppal);
    return hpal;
}



/*
 *  ReadDibBitmapInfo()
 *
 *  Will read a file in DIB format and return a global HANDLE to it's
 *  BITMAPINFO.  This function will work with both "old" and "new"
 *  bitmap formats, but will always return a "new" BITMAPINFO
 *
 */
HANDLE ReadDibBitmapInfo(int fh)
{
    DWORD     off;
    HANDLE    hbi = NULL;
    int       size;
    int       i;
    UINT      nNumColors;

    RGBQUAD FAR       *pRgb;
    BITMAPINFOHEADER   bi;
    BITMAPCOREHEADER   bc;
    LPBITMAPINFOHEADER lpbi;
    BITMAPFILEHEADER   bf;

    if (fh == -1)
        return NULL;

    off = _llseek(fh,0L,SEEK_CUR);

    if (sizeof(bf) != _lread(fh,(LPBYTE)&bf,sizeof(bf)))
        return FALSE;

    /*
     *  do we have a RC HEADER?
     */
    if (!ISDIB(bf.bfType))
    {
        bf.bfOffBits = 0L;
        _llseek(fh,off,SEEK_SET);
    }

    if (sizeof(bi) != _lread(fh,(LPBYTE)&bi,sizeof(bi)))
        return FALSE;

    nNumColors = DibNumColors(&bi);

    /*
     *  what type of bitmap info is this?
     */
    switch (size = (int)bi.biSize)
    {
        case sizeof(BITMAPINFOHEADER):
            break;

        case sizeof(BITMAPCOREHEADER):
            bc = *(BITMAPCOREHEADER*)&bi;
            bi.biSize               = sizeof(BITMAPINFOHEADER);
            bi.biWidth              = (DWORD)bc.bcWidth;
            bi.biHeight             = (DWORD)bc.bcHeight;
            bi.biPlanes             =  (UINT)bc.bcPlanes;
            bi.biBitCount           =  (UINT)bc.bcBitCount;
            bi.biCompression        = BI_RGB;
            bi.biSizeImage          = 0;
            bi.biXPelsPerMeter      = 0;
            bi.biYPelsPerMeter      = 0;
            bi.biClrUsed            = nNumColors;
            bi.biClrImportant       = nNumColors;

            _llseek(fh,(LONG)sizeof(BITMAPCOREHEADER)-sizeof(BITMAPINFOHEADER),SEEK_CUR);

            break;

        default:
            return NULL;       /* not a DIB */
    }

    /*
     *	fill in some default values!
     */
    if (bi.biSizeImage == 0)
    {
        bi.biSizeImage = (DWORD)DIBWIDTHBYTES(bi) * bi.biHeight;
    }

    if (bi.biXPelsPerMeter == 0)
    {
	bi.biXPelsPerMeter = 0; 	// ??????????????
    }

    if (bi.biYPelsPerMeter == 0)
    {
	bi.biYPelsPerMeter = 0; 	// ??????????????
    }

    if (bi.biClrUsed == 0)
    {
	bi.biClrUsed = DibNumColors(&bi);
    }

    hbi = GlobalAlloc(GMEM_MOVEABLE,(LONG)bi.biSize + nNumColors * sizeof(RGBQUAD));
    if (!hbi)
        return NULL;

    lpbi = (VOID FAR *)GlobalLock(hbi);
    *lpbi = bi;

    pRgb = (RGBQUAD FAR *)((LPBYTE)lpbi + bi.biSize);

    if (nNumColors)
    {
        if (size == sizeof(BITMAPCOREHEADER))
        {
            /*
             * convert a old color table (3 byte entries) to a new
             * color table (4 byte entries)
             */
            _lread(fh,(LPBYTE)pRgb,nNumColors * sizeof(RGBTRIPLE));

            for (i=nNumColors-1; i>=0; i--)
            {
                RGBQUAD rgb;

                rgb.rgbRed      = ((RGBTRIPLE FAR *)pRgb)[i].rgbtRed;
                rgb.rgbBlue     = ((RGBTRIPLE FAR *)pRgb)[i].rgbtBlue;
                rgb.rgbGreen    = ((RGBTRIPLE FAR *)pRgb)[i].rgbtGreen;
                rgb.rgbReserved = (BYTE)0;

                pRgb[i] = rgb;
            }
        }
        else
        {
            _lread(fh,(LPBYTE)pRgb,nNumColors * sizeof(RGBQUAD));
        }
    }

    if (bf.bfOffBits != 0L)
        _llseek(fh,off + bf.bfOffBits,SEEK_SET);

    GlobalUnlock(hbi);
    return hbi;
}

/*  How big is the palette? if bits per pel not 24
 *  no of bytes to read is 6 for 1 bit, 48 for 4 bits
 *  256*3 for 8 bits and 0 for 24 bits
 */
UINT PaletteSize(VOID FAR * pv)
{
    #define lpbi ((LPBITMAPINFOHEADER)pv)
    #define lpbc ((LPBITMAPCOREHEADER)pv)

    UINT    NumColors;

    NumColors = DibNumColors(lpbi);

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        return NumColors * sizeof(RGBTRIPLE);
    else
        return NumColors * sizeof(RGBQUAD);

    #undef lpbi
    #undef lpbc
}

/*  How Many colors does this DIB have?
 *  this will work on both PM and Windows bitmap info structures.
 */
UINT DibNumColors(VOID FAR * pv)
{
    #define lpbi ((LPBITMAPINFOHEADER)pv)
    #define lpbc ((LPBITMAPCOREHEADER)pv)

    int bits;

    /*
     *  with the new format headers, the size of the palette is in biClrUsed
     *  else is dependent on bits per pixel
     */
    if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
    {
        if (lpbi->biClrUsed != 0)
            return (UINT)lpbi->biClrUsed;

        bits = lpbi->biBitCount;
    }
    else
    {
        bits = lpbc->bcBitCount;
    }

    switch (bits)
    {
    case 1:
            return 2;
    case 4:
            return 16;
    case 8:
            return 256;
    default:
            return 0;
    }

    #undef lpbi
    #undef lpbc
}

/*
 *  DibFromBitmap()
 *
 *  Will create a global memory block in DIB format that represents the DDB
 *  passed in
 *
 */
HANDLE DibFromBitmap(HBITMAP hbm, DWORD biStyle, UINT biBits, HPALETTE hpal, UINT wUsage)
{
    BITMAP               bm;
    BITMAPINFOHEADER     bi;
    BITMAPINFOHEADER FAR *lpbi;
    DWORD                dwLen;
    int                  nColors;
    HANDLE               hdib;
    HANDLE               h;
    HDC                  hdc;

    if (wUsage == 0)
        wUsage = DIB_RGB_COLORS;

    if (!hbm)
        return NULL;
#if 0
    if (biStyle == BI_RGB && wUsage == DIB_RGB_COLORS)
        return CreateLogicalDib(hbm,biBits,hpal);
#endif

    if (hpal == NULL)
        hpal = GetStockObject(DEFAULT_PALETTE);

    GetObject(hbm,sizeof(bm),(LPBYTE)&bm);
#ifdef WIN32
    nColors = 0;  // GetObject only stores two bytes
#endif
    GetObject(hpal,sizeof(nColors),(LPBYTE)&nColors);

    if (biBits == 0)
        biBits = bm.bmPlanes * bm.bmBitsPixel;

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = biBits;
    bi.biCompression        = biStyle;
    bi.biSizeImage          = 0;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = 0;
    bi.biClrImportant       = 0;

    dwLen  = bi.biSize + PaletteSize(&bi);

    hdc = CreateCompatibleDC(NULL);
    hpal = SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);  // why is this needed on a MEMORY DC? GDI bug??

    hdib = GlobalAlloc(GMEM_MOVEABLE,dwLen);

    if (!hdib)
        goto exit;

    lpbi = (VOID FAR *)GlobalLock(hdib);

    *lpbi = bi;

    /*
     *  call GetDIBits with a NULL lpBits param, so it will calculate the
     *  biSizeImage field for us
     */
    GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
        NULL, (LPBITMAPINFO)lpbi, wUsage);

    bi = *lpbi;
    GlobalUnlock(hdib);

    /*
     * HACK! if the driver did not fill in the biSizeImage field, make one up
     */
    if (bi.biSizeImage == 0)
    {
        bi.biSizeImage = (DWORD)WIDTHBYTES(bm.bmWidth * biBits) * bm.bmHeight;

        if (biStyle != BI_RGB)
            bi.biSizeImage = (bi.biSizeImage * 3) / 2;
    }

    /*
     *  realloc the buffer big enough to hold all the bits
     */
    dwLen = bi.biSize + PaletteSize(&bi) + bi.biSizeImage;
    if (h = GlobalReAlloc(hdib,dwLen,0))
    {
        hdib = h;
    }
    else
    {
        GlobalFree(hdib);
        hdib = NULL;
        goto exit;
    }

    /*
     *  call GetDIBits with a NON-NULL lpBits param, and actualy get the
     *  bits this time
     */
    lpbi = (VOID FAR *)GlobalLock(hdib);

    GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
        (LPBYTE)lpbi + (UINT)lpbi->biSize + PaletteSize(lpbi),
        (LPBITMAPINFO)lpbi, wUsage);

    bi = *lpbi;
    lpbi->biClrUsed = DibNumColors(lpbi) ;
    GlobalUnlock(hdib);

exit:
    SelectPalette(hdc,hpal,FALSE);
    DeleteDC(hdc);
    return hdib;
}

/*
 *  BitmapFromDib()
 *
 *  Will create a DDB (Device Dependent Bitmap) given a global handle to
 *  a memory block in CF_DIB format
 *
 */
HBITMAP BitmapFromDib(HANDLE hdib, HPALETTE hpal, UINT wUsage)
{
    LPBITMAPINFOHEADER lpbi;
    HPALETTE    hpalT;
    HDC         hdc;
    HBITMAP     hbm;
#if 0
    UINT        dx,dy,bits;
#endif

    if (!hdib)
        return NULL;

    if (wUsage == 0)
        wUsage = DIB_RGB_COLORS;

    lpbi = (VOID FAR *)GlobalLock(hdib);

    if (!lpbi)
	return NULL;

    hdc = GetDC(NULL);
//    hdc = CreateCompatibleDC(NULL);

    if (hpal)
    {
        hpalT = SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);  // why is this needed on a MEMORY DC? GDI bug??
    }

#if 0
    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
    {
        dx   = ((LPBITMAPCOREHEADER)lpbi)->bcWidth;
        dy   = ((LPBITMAPCOREHEADER)lpbi)->bcHeight;
        bits = ((LPBITMAPCOREHEADER)lpbi)->bcBitCount;
    }
    else
    {
        dx   = (UINT)lpbi->biWidth;
        dy   = (UINT)lpbi->biHeight;
        bits = (UINT)lpbi->biBitCount;
    }

    if (bMonoBitmap /* || bits == 1 */)
    {
        hbm = CreateBitmap(dx,dy,1,1,NULL);
    }
    else
    {
        HDC hdcScreen = GetDC(NULL);
        hbm = CreateCompatibleBitmap(hdcScreen,dx,dy);
        ReleaseDC(NULL,hdcScreen);
    }

    if (hbm)
    {
        if (fErrProp)
            SetDIBitsErrProp(hdc,hbm,0,dy,
               (LPBYTE)lpbi + lpbi->biSize + PaletteSize(lpbi),
               (LPBITMAPINFO)lpbi,wUsage);
        else
            SetDIBits(hdc,hbm,0,dy,
               (LPBYTE)lpbi + lpbi->biSize + PaletteSize(lpbi),
               (LPBITMAPINFO)lpbi,wUsage);
    }

#else
    hbm = CreateDIBitmap(hdc,
                (LPBITMAPINFOHEADER)lpbi,
                (LONG)CBM_INIT,
                (LPBYTE)lpbi + lpbi->biSize + PaletteSize(lpbi),
                (LPBITMAPINFO)lpbi,
                wUsage );
#endif

    if (hpal && hpalT)
        SelectPalette(hdc,hpalT,FALSE);

//    DeleteDC(hdc);
    ReleaseDC(NULL,hdc);

    GlobalUnlock(hdib);
    return hbm;
}

/*
 *  DibFromDib()
 *
 *  Will convert a DIB in 1 format to a DIB in the specifed format
 *
 */
HANDLE DibFromDib(HANDLE hdib, DWORD biStyle, UINT biBits, HPALETTE hpal, UINT wUsage)
{
    BITMAPINFOHEADER bi;
    HBITMAP     hbm;
    BOOL        fKillPalette=FALSE;

    if (!hdib)
        return NULL;

    DibInfo((LPBITMAPINFOHEADER)GlobalLock(hdib),&bi); GlobalUnlock(hdib);

    /*
     *  do we have the requested format already?
     */
    if (bi.biCompression == biStyle && (UINT)bi.biBitCount == biBits)
        return hdib;

    if (hpal == NULL)
    {
        hpal = CreateDibPalette(hdib);
        fKillPalette++;
    }

    hbm = BitmapFromDib(hdib,hpal,wUsage);

    if (hbm == NULL)
    {
        hdib = NULL;
    }
    else
    {
        hdib = DibFromBitmap(hbm,biStyle,biBits,hpal,wUsage);
        DeleteObject(hbm);
    }

    if (fKillPalette && hpal)
        DeleteObject(hpal);

    return hdib;
}

#define MAKEP(sel,off)  ((VOID FAR *)MAKELONG(off,sel))

/*
 *  CreateLogicalDib
 *
 *  Given a DDB and a HPALETTE create a "logical" DIB
 *
 *  if the HBITMAP is NULL create a DIB from the system "stock" bitmap
 *      This is used to save a logical palette to a disk file as a DIB
 *
 *  if the HPALETTE is NULL use the system "stock" palette (ie the
 *      system palette)
 *
 *  a "logical" DIB is a DIB where the DIB color table *exactly* matches
 *  the passed logical palette.  There will be no system colors in the DIB
 *  block, and a pixel value of <n> in the DIB will correspond to logical
 *  palette index <n>.
 *
 *  This is accomplished by doing a GetDIBits() with the DIB_PAL_COLORS
 *  option then converting the palindexes returned in the color table
 *  from palette indexes to logical RGB values.  The entire passed logical
 *  palette is always copied to the DIB color table.
 *
 *  The DIB color table will have exactly the same number of entries as
 *  the logical palette.  Normaly GetDIBits() will always set biClrUsed to
 *  the maximum colors supported by the device regardless of the number of
 *  colors in the logical palette
 *
 *  Why would you want to do this?  The major reason for a "logical" DIB
 *  is so when the DIB is written to a disk file then reloaded the logical
 *  palette created from the DIB color table will be the same as one used
 *  originaly to create the bitmap.  It also will prevent GDI from doing
 *  nearest color matching on PC_RESERVED palettes.
 *
 *  ** What do we do if the logical palette has more than 256 entries!!!!!
 *  ** GetDIBits() may return logical palette index's that are greater than
 *  ** 256, we cant represent these colors in the "logical" DIB
 *  **
 *  ** for now hose the caller?????
 *
 */

HANDLE CreateLogicalDib(HBITMAP hbm, UINT biBits, HPALETTE hpal)
{
    BITMAP              bm;
    BITMAPINFOHEADER    bi;
    LPBITMAPINFOHEADER  lpDib;      // pointer to DIB
    LPBITMAPINFOHEADER  lpbi;       // temp pointer to BITMAPINFO
    DWORD               dwLen;
    DWORD               dw;
    int                 n;
    int                 nColors;
    HANDLE              hdib;
    HDC                 hdc;
    BYTE FAR *          lpBits;
    UINT FAR *          lpCT;
    RGBQUAD FAR *       lpRgb;
    PALETTEENTRY        peT;
    HPALETTE            hpalT;

    if (hpal == NULL)
        hpal = GetStockObject(DEFAULT_PALETTE);

    if (hbm == NULL)
        hbm = NULL; // ????GetStockObject(STOCK_BITMAP);

#ifdef WIN32
    nColors = 0;  // GetObject only stores two bytes
#endif
    GetObject(hpal,sizeof(nColors),(LPBYTE)&nColors);
    GetObject(hbm,sizeof(bm),(LPBYTE)&bm);

    if (biBits == 0)
        biBits = nColors > 16 ? 8 : 4;

    if (nColors > 256)      // ACK!
        ;                   // How do we handle this????

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = biBits;
    bi.biCompression        = BI_RGB;
    bi.biSizeImage          = (DWORD)WIDTHBYTES(bm.bmWidth * biBits) * bm.bmHeight;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = nColors;
    bi.biClrImportant       = 0;

    dwLen = bi.biSize + PaletteSize(&bi) + bi.biSizeImage;

    hdib = GlobalAlloc(GMEM_MOVEABLE,dwLen);

    if (!hdib)
        return NULL;

    lpbi = MAKEP(GlobalAlloc(GMEM_FIXED,bi.biSize + 256 * sizeof(RGBQUAD)),0);

    if (!lpbi)
    {
        GlobalFree(hdib);
        return NULL;
    }

    hdc = GetDC(NULL);
    hpalT = SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);  // why is this needed on a MEMORY DC? GDI bug??

    lpDib = (LPVOID)GlobalLock(hdib);

    *lpbi  = bi;
    *lpDib = bi;
    lpCT   = (UINT FAR *)((LPBYTE)lpbi + (UINT)lpbi->biSize);
    lpRgb  = (RGBQUAD FAR *)((LPBYTE)lpDib + (UINT)lpDib->biSize);
    lpBits = (LPBYTE)lpDib + (UINT)lpDib->biSize + PaletteSize(lpDib);

    /*
     *  call GetDIBits to get the DIB bits and fill the color table with
     *  logical palette index's
     */
    GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
        lpBits,(LPBITMAPINFO)lpbi, DIB_PAL_COLORS);

    /*
     *  Now convert the DIB bits into "real" logical palette index's
     *
     *  lpCT        points to the DIB color table wich is a UINT array of
     *              logical palette index's
     *
     *  lpBits      points to the DIB bits, each DIB pixel is a index into
     *              the DIB color table.
     *
     */

    if (biBits == 8)
    {
        for (dw = 0; dw < bi.biSizeImage; dw++, ((BYTE HUGE_T *)lpBits)++)
            *lpBits = (BYTE)lpCT[*lpBits];
    }
    else // biBits == 4
    {
        for (dw = 0; dw < bi.biSizeImage; dw++, ((BYTE HUGE_T *)lpBits)++)
            *lpBits = lpCT[*lpBits & 0x0F] | (lpCT[(*lpBits >> 4) & 0x0F] << 4);
    }

    /*
     *  Now copy the RGBs in the logical palette to the dib color table
     */
    for (n=0; n<nColors; n++,lpRgb++)
    {
        GetPaletteEntries(hpal,n,1,&peT);

        lpRgb->rgbRed      = peT.peRed;
        lpRgb->rgbGreen    = peT.peGreen;
        lpRgb->rgbBlue     = peT.peBlue;
        lpRgb->rgbReserved = (BYTE)0;
    }

    GlobalUnlock(hdib);
#ifdef WIN32
    GlobalFree(GlobalHandle(lpbi));
#else
    GlobalFree(SELECTOROF(lpbi));
#endif

    SelectPalette(hdc,hpalT,FALSE);
    ReleaseDC(NULL,hdc);

    return hdib;
}

/*
 *  Draws bitmap <hbm> at the specifed position in DC <hdc>
 *
 */
BOOL StretchBitmap(HDC hdc, int x, int y, int dx, int dy, HBITMAP hbm, int x0, int y0, int dx0, int dy0, DWORD rop)
{
    HDC hdcBits;
    HPALETTE hpal,hpalT;
    BOOL f;

    if (!hdc || !hbm)
        return FALSE;

    hpal = SelectPalette(hdc,GetStockObject(DEFAULT_PALETTE),FALSE);
    SelectPalette(hdc,hpal,FALSE);

    hdcBits = CreateCompatibleDC(hdc);
    SelectObject(hdcBits,hbm);
    hpalT = SelectPalette(hdcBits,hpal,FALSE);
    RealizePalette(hdcBits);
    f = StretchBlt(hdc,x,y,dx,dy,hdcBits,x0,y0,dx0,dy0,rop);
    SelectPalette(hdcBits,hpalT,FALSE);
    DeleteDC(hdcBits);

    return f;
}

/*
 *  Draws bitmap <hbm> at the specifed position in DC <hdc>
 *
 */
BOOL DrawBitmap(HDC hdc, int x, int y, HBITMAP hbm, DWORD rop)
{
    HDC hdcBits;
    BITMAP bm;
    BOOL f;

    if (!hdc || !hbm)
        return FALSE;

    hdcBits = CreateCompatibleDC(hdc);
    GetObject(hbm,sizeof(BITMAP),(LPBYTE)&bm);
    SelectObject(hdcBits,hbm);
    f = BitBlt(hdc,x,y,bm.bmWidth,bm.bmHeight,hdcBits,0,0,rop);
    DeleteDC(hdcBits);

    return f;
}

/*
 *  SetDibUsage(hdib,hpal,wUsage)
 *
 *  Modifies the color table of the passed DIB for use with the wUsage
 *  parameter specifed.
 *
 *  if wUsage is DIB_PAL_COLORS the DIB color table is set to 0-256
 *  if wUsage is DIB_RGB_COLORS the DIB color table is set to the RGB values
 *      in the passed palette
 *
 */
BOOL SetDibUsage(HANDLE hdib, HPALETTE hpal,UINT wUsage)
{
    LPBITMAPINFOHEADER lpbi;
    PALETTEENTRY       ape[MAXPALETTE];
    RGBQUAD FAR *      pRgb;
    UINT FAR *         pw;
    int                nColors;
    int                n;

    if (hpal == NULL)
        hpal = GetStockObject(DEFAULT_PALETTE);

    if (!hdib)
        return FALSE;

    lpbi = (VOID FAR *)GlobalLock(hdib);

    if (!lpbi)
	return FALSE;

    nColors = DibNumColors(lpbi);

    if (nColors > 0)
    {
        pRgb = (RGBQUAD FAR *)((LPBYTE)lpbi + (UINT)lpbi->biSize);

        switch (wUsage)
        {
            //
            // Set the DIB color table to palette indexes
            //
            case DIB_PAL_COLORS:
                for (pw = (UINT FAR*)pRgb,n=0; n<nColors; n++,pw++)
                    *pw = n;
                break;

            //
            // Set the DIB color table to RGBQUADS
            //
            default:
            case DIB_RGB_COLORS:
                nColors = min(nColors,MAXPALETTE);

                GetPaletteEntries(hpal,0,nColors,ape);

                for (n=0; n<nColors; n++)
                {
                    pRgb[n].rgbRed      = ape[n].peRed;
                    pRgb[n].rgbGreen    = ape[n].peGreen;
                    pRgb[n].rgbBlue     = ape[n].peBlue;
                    pRgb[n].rgbReserved = 0;
                }
                break;
        }
    }
    GlobalUnlock(hdib);
    return TRUE;
}

/*
 *  SetPalFlags(hpal,iIndex, cnt, wFlags)
 *
 *  Modifies the palette flags of all indexs in the range (iIndex - nIndex+cnt)
 *  to the parameter specifed.
 *
 */
BOOL SetPalFlags(HPALETTE hpal, int iIndex, int cntEntries, UINT wFlags)
{
    int     i;
    BOOL    f;
    HANDLE  hpe;
    PALETTEENTRY FAR *lppe;

    if (hpal == NULL)
        return FALSE;

    if (cntEntries < 0) {
#ifdef WIN32
        cntEntries = 0;  // GetObject only stores two bytes
#endif
        GetObject(hpal,sizeof(int),(LPBYTE)&cntEntries);
    }

    hpe = GlobalAlloc(GMEM_MOVEABLE,(LONG)cntEntries * sizeof(PALETTEENTRY));

    if (!hpe)
        return FALSE;

    lppe = (VOID FAR*)GlobalLock(hpe);

    GetPaletteEntries(hpal, iIndex, cntEntries, lppe);

    for (i=0; i<cntEntries; i++)
    {
        lppe[i].peFlags = (BYTE)wFlags;
    }

    f = SetPaletteEntries(hpal, iIndex, cntEntries, lppe);

    GlobalUnlock(hpe);
    GlobalFree(hpe);
    return f;
}

/*
 *  StretchDibBlt()
 *
 *  draws a bitmap in CF_DIB format, using StretchDIBits()
 *
 *  takes the same parameters as StretchBlt()
 */
BOOL StretchDibBlt(HDC hdc, int x, int y, int dx, int dy, HANDLE hdib, int x0, int y0, int dx0, int dy0, LONG rop, UINT wUsage)
{
    LPBITMAPINFOHEADER lpbi;
    LPBYTE        pBuf;
    BOOL         f;

    if (!hdib)
        return PatBlt(hdc,x,y,dx,dy,rop);

    if (wUsage == 0)
        wUsage = DIB_RGB_COLORS;

    lpbi = (VOID FAR *)GlobalLock(hdib);

    if (!lpbi)
        return FALSE;

    if (dx0 == -1 && dy0 == -1)
    {
        if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        {
            dx0 = ((LPBITMAPCOREHEADER)lpbi)->bcWidth;
            dy0 = ((LPBITMAPCOREHEADER)lpbi)->bcHeight;
        }
        else
        {
            dx0 = (int)lpbi->biWidth;
            dy0 = (int)lpbi->biHeight;
        }
    }

    if (dx < 0 && dy < 0)
    {
        dx = dx0 * (-dx);
        dy = dy0 * (-dy);
    }

    pBuf = (LPBYTE)lpbi + (UINT)lpbi->biSize + PaletteSize(lpbi);

    f = StretchDIBits (
        hdc,
        x,y,
        dx,dy,
        x0,y0,
        dx0,dy0,
        pBuf, (LPBITMAPINFO)lpbi,
        wUsage,
        rop);

    GlobalUnlock(hdib);
    return f;
}

/*
 *  DibBlt()
 *
 *  draws a bitmap in CF_DIB format, using SetDIBits to device.
 *
 *  takes the same parameters as BitBlt()
 */
BOOL DibBlt(HDC hdc, int x0, int y0, int dx, int dy, HANDLE hdib, int x1, int y1, LONG rop, UINT wUsage)
{
    LPBITMAPINFOHEADER lpbi;
    LPBYTE       pBuf;
    BOOL        f;

    if (!hdib)
        return PatBlt(hdc,x0,y0,dx,dy,rop);

    if (wUsage == 0)
        wUsage = DIB_RGB_COLORS;

    lpbi = (VOID FAR *)GlobalLock(hdib);

    if (!lpbi)
        return FALSE;

    if (dx == -1 && dy == -1)
    {
        if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        {
            dx = ((LPBITMAPCOREHEADER)lpbi)->bcWidth;
            dy = ((LPBITMAPCOREHEADER)lpbi)->bcHeight;
        }
        else
        {
            dx = (int)lpbi->biWidth;
            dy = (int)lpbi->biHeight;
        }
    }

    pBuf = (LPBYTE)lpbi + (UINT)lpbi->biSize + PaletteSize(lpbi);

#if 0
    f = SetDIBitsToDevice(hdc, x0, y0, dx, dy,
        x1,y1,
        x1,
        dy,
        pBuf, (LPBITMAPINFO)lpbi,
        wUsage );
#else
    f = StretchDIBits (
        hdc,
        x0,y0,
        dx,dy,
        x1,y1,
        dx,dy,
        pBuf, (LPBITMAPINFO)lpbi,
        wUsage,
        rop);
#endif

    GlobalUnlock(hdib);
    return f;
}

LPVOID DibLock(HANDLE hdib,int x, int y)
{
    return DibXY((LPBITMAPINFOHEADER)GlobalLock(hdib),x,y);
}

VOID DibUnlock(HANDLE hdib)
{
    GlobalUnlock(hdib);
}

LPVOID DibXY(LPBITMAPINFOHEADER lpbi,int x, int y)
{
    BYTE HUGE_T *pBits;
    DWORD ulWidthBytes;

    pBits = (LPBYTE)lpbi + (UINT)lpbi->biSize + PaletteSize(lpbi);

    ulWidthBytes = DIBWIDTHBYTES(*lpbi);

    pBits += (ulWidthBytes * (long)y) + x;

    return (LPVOID)pBits;
}

    //
    // These are the standard VGA colors, we will be stuck with until the
    // end of time!
    //
    static DWORD CosmicColors[16] = {
         0x00000000        // 0000  black
        ,0x00800000        // 0001  dark red
        ,0x00008000        // 0010  dark green
        ,0x00808000        // 0011  mustard
        ,0x00000080        // 0100  dark blue
        ,0x00800080        // 0101  purple
        ,0x00008080        // 0110  dark turquoise
        ,0x00C0C0C0        // 1000  gray
        ,0x00808080        // 0111  dark gray
        ,0x00FF0000        // 1001  red
        ,0x0000FF00        // 1010  green
        ,0x00FFFF00        // 1011  yellow
        ,0x000000FF        // 1100  blue
        ,0x00FF00FF        // 1101  pink (magenta)
        ,0x0000FFFF        // 1110  cyan
        ,0x00FFFFFF        // 1111  white
        };

HANDLE CreateDib(int bits, int dx, int dy)
{
    HANDLE              hdib;
    BITMAPINFOHEADER    bi;
    LPBITMAPINFOHEADER  lpbi;
    DWORD FAR *         pRgb;
    UINT                i;


    bi.biSize           = sizeof(BITMAPINFOHEADER);
    bi.biPlanes         = 1;
    bi.biBitCount       = bits;
    bi.biWidth          = dx;
    bi.biHeight         = dy;
    bi.biCompression    = BI_RGB;
    bi.biSizeImage      = 0;
    bi.biXPelsPerMeter  = 0;
    bi.biYPelsPerMeter  = 0;
    bi.biClrUsed 		= 0;
    bi.biClrImportant   = 0;
    bi.biClrUsed	= DibNumColors(&bi);

    hdib = GlobalAlloc(GMEM_MOVEABLE,sizeof(BITMAPINFOHEADER) +
                + (long)bi.biClrUsed * sizeof(RGBQUAD)
                + (long)DIBWIDTHBYTES(bi) * (long)dy);

    if (hdib)
    {
        lpbi  = (LPVOID)GlobalLock(hdib);
        *lpbi = bi;

        pRgb  = (LPVOID)((LPBYTE)lpbi + lpbi->biSize);

        //
        //  setup the color table
        //
        if (bits == 1)
        {
            pRgb[0] = CosmicColors[0];
            pRgb[1] = CosmicColors[15];
        }
        else
        {
            for (i=0; i<bi.biClrUsed; i++)
				pRgb[i] = CosmicColors[i % 16];
        }

        GlobalUnlock(hdib);
    }

    return hdib;
}

/*
 * Private routines to read/write more than 64k
 */

#define MAXREAD (UINT)(32u * 1024)

static DWORD NEAR PASCAL lread(int fh, VOID FAR *pv, DWORD ul)
{
    DWORD  ulT = ul;
    BYTE HUGE_T *hp = pv;

    while (ul > MAXREAD) {
	if (_lread(fh, (LPBYTE)hp, MAXREAD) != MAXREAD)
		return 0;
	ul -= MAXREAD;
	hp += MAXREAD;
    }
    if (_lread(fh, (LPBYTE)hp, (UINT)ul) != (UINT)ul)
	return 0;
    return ulT;
}

static DWORD NEAR PASCAL lwrite(int fh, VOID FAR *pv, DWORD ul)
{
    DWORD  ulT = ul;
    BYTE HUGE_T *hp = pv;

    while (ul > MAXREAD) {
	if (_lwrite(fh, (LPBYTE)hp, MAXREAD) != MAXREAD)
		return 0;
	ul -= MAXREAD;
	hp += MAXREAD;
    }
    if (_lwrite(fh, (LPBYTE)hp, (UINT)ul) != (UINT)ul)
	return 0;
    return ulT;
}
