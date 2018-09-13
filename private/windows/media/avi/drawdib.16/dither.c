#include <windows.h>
#include <windowsx.h>
#include "drawdibi.h"
#include "dither.h"

//#define GRAY_SCALE

extern BOOL gf286;
extern UINT gwRasterCaps;

void FAR PASCAL Map16to24(LPBITMAPINFOHEADER,LPVOID,int,int,int,int,LPBITMAPINFOHEADER,LPVOID,int,int,LPVOID);

extern LPVOID glpDitherTable;

//////////////////////////////////////////////////////////////////////////////
//
//   DitherInit()
//
//////////////////////////////////////////////////////////////////////////////

LPVOID VFWAPI
DitherInit(LPBITMAPINFOHEADER lpbiIn,
           LPBITMAPINFOHEADER lpbiOut,
           DITHERPROC FAR *   lpDitherProc,
           LPVOID             lpDitherTable)
{
    switch ((int)lpbiOut->biBitCount)
    {
        case 8:
            if ((int)lpbiIn->biBitCount == 8 && (gwRasterCaps & RC_PALETTE))
                return Dither8Init(lpbiIn, lpbiOut, lpDitherProc, lpDitherTable);

            if ((int)lpbiIn->biBitCount == 8 && !(gwRasterCaps & RC_PALETTE))
                return DitherDeviceInit(lpbiIn, lpbiOut, lpDitherProc, lpDitherTable);

            if ((int)lpbiIn->biBitCount == 16)
                return Dither16Init(lpbiIn, lpbiOut, lpDitherProc, lpDitherTable);

            if ((int)lpbiIn->biBitCount == 24)
                return Dither24Init(lpbiIn, lpbiOut, lpDitherProc, lpDitherTable);

            if ((int)lpbiIn->biBitCount == 32)
                return Dither32Init(lpbiIn, lpbiOut, lpDitherProc, lpDitherTable);

            return (LPVOID)-1;

        case 24:
            if (!gf286) {
		if (lpbiIn->biBitCount == 16) {
                    *lpDitherProc = Map16to24;
                    return NULL;
		} else if (lpbiIn->biBitCount == 32) {
		    *lpDitherProc = Map32to24;
		    return NULL;
		}
	    }

	    return (LPVOID)-1;

        default:
            return (LPVOID)-1;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//   DitherTerm()
//
//////////////////////////////////////////////////////////////////////////////

void VFWAPI
DitherTerm(LPVOID lpDitherTable)
{
    if (lpDitherTable == glpDitherTable)
        Dither16Term(lpDitherTable);
    else
        Dither8Term(lpDitherTable);
}

//////////////////////////////////////////////////////////////////////////////
//
//   DitherDeviceInit() - dither to the colors of the display driver
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR PASCAL DitherDeviceInit(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable)
{
    HBRUSH   hbr;
    HDC      hdcMem;
    HDC      hdc;
    HBITMAP  hbm;
    HBITMAP  hbmT;
    int      i;
    int      nColors;
    LPRGBQUAD prgb;
    BITMAPINFOHEADER biSave = *lpbiOut;

    //
    // we dont need to re-init the dither table, unless it is not ours then
    // we should free it.
    //
    if (lpDitherTable == glpDitherTable)
    {
        DitherTerm(lpDitherTable);
        lpDitherTable = NULL;
    }

    if (lpDitherTable == NULL)
    {
        lpDitherTable = GlobalAllocPtr(GHND, 256*8*8);
    }

    if (lpDitherTable == NULL)
        return (LPVOID)-1;

    hdc = GetDC(NULL);
    hdcMem = CreateCompatibleDC(hdc);

    hbm = CreateCompatibleBitmap(hdc, 256*8, 8);
    hbmT = SelectObject(hdcMem, hbm);

    if ((nColors = (int)lpbi->biClrUsed) == 0)
        nColors = 1 << (int)lpbi->biBitCount;

    prgb = (LPRGBQUAD)(lpbi+1);

    for (i=0; i<nColors; i++)
    {
        hbr = CreateSolidBrush(RGB(prgb[i].rgbRed,prgb[i].rgbGreen,prgb[i].rgbBlue));
        hbr = SelectObject(hdcMem, hbr);
        PatBlt(hdcMem, i*8, 0, 8, 8, PATCOPY);
        hbr = SelectObject(hdcMem, hbr);
        DeleteObject(hbr);
    }

#ifdef XDEBUG
    for (i=0; i<16; i++)
        BitBlt(hdc,0,i*8,16*8,8,hdcMem,i*(16*8),0,SRCCOPY);
#endif

    SelectObject(hdcMem, hbmT);
    DeleteDC(hdcMem);

    lpbiOut->biSize           = sizeof(BITMAPINFOHEADER);
    lpbiOut->biPlanes         = 1;
    lpbiOut->biBitCount       = 8;
    lpbiOut->biWidth          = 256*8;
    lpbiOut->biHeight         = 8;
    lpbiOut->biCompression    = BI_RGB;
    lpbiOut->biSizeImage      = 256*8*8;
    lpbiOut->biXPelsPerMeter  = 0;
    lpbiOut->biYPelsPerMeter  = 0;
    lpbiOut->biClrUsed        = 0;
    lpbiOut->biClrImportant   = 0;
    GetDIBits(hdc, hbm, 0, 8, lpDitherTable, (LPBITMAPINFO)lpbiOut, DIB_RGB_COLORS);

    i = (int)lpbiOut->biClrUsed;
    *lpbiOut = biSave;
    lpbiOut->biClrUsed = i;

    DeleteObject(hbm);
    ReleaseDC(NULL, hdc);

    *lpDitherProc = Dither8;

    return (LPVOID)lpDitherTable;
}


//////////////////////////////////////////////////////////////////////////////
//
//   DitherTerm()
//
//////////////////////////////////////////////////////////////////////////////

void FAR PASCAL Dither8Term(LPVOID lpDitherTable)
{
    if (lpDitherTable)
        GlobalFreePtr(lpDitherTable);
}

#ifdef WIN32

//
//  call this to actually do the dither.
//
void FAR PASCAL Dither8(
    LPBITMAPINFOHEADER biDst,           // --> BITMAPINFO of the dest
    LPVOID             lpDst,           // --> to destination bits
    int                DstX,            // Destination origin - x coordinate
    int                DstY,            // Destination origin - y coordinate
    int                DstXE,           // x extent of the BLT
    int                DstYE,           // y extent of the BLT
    LPBITMAPINFOHEADER biSrc,           // --> BITMAPINFO of the source
    LPVOID             lpSrc,           // --> to source bits
    int                SrcX,            // Source origin - x coordinate
    int                SrcY,            // Source origin - y coordinate
    LPVOID             lpDitherTable)   // dither table.
{
    int x,y;
    UINT wWidthSrc;
    UINT wWidthDst;
    BYTE _huge *pbS;
    BYTE _huge *pbD;
    DWORD dw;

    if (biDst->biBitCount != 8 || biSrc->biBitCount != 8)
        return;

    // tomor -- A little help! seems initialization is not done yet.
    if(!lpDitherTable)
        return;

    wWidthSrc = ((UINT)biSrc->biWidth+3)&~3;
    wWidthDst = ((UINT)biDst->biWidth+3)&~3;

    pbD = (BYTE _huge *)lpDst + DstX + DstY * wWidthDst;
    pbS = (BYTE _huge *)lpSrc + SrcX + SrcY * wWidthSrc;

    wWidthSrc -= DstXE;
    wWidthDst -= DstXE;

#define DODITH8(px, x, y)	((LPBYTE)lpDitherTable)[((y) & 7) * 256 * 8 + (px) * 8 + (x & 7)]

    for (y=0; y<DstYE; y++) {
	/* write two DWORDs (one dither cell horizontally) at once */
	for (x=0; x <= (DstXE - 8); x += 8) {

            dw = DODITH8(*pbS++, 0, y);
	    dw |= (DODITH8(*pbS++, 1, y) << 8);
	    dw |= (DODITH8(*pbS++, 2, y) << 16);
	    dw |= (DODITH8(*pbS++, 3, y) << 24);
            * ( (DWORD _huge UNALIGNED *) pbD)++ = dw;

            dw = DODITH8(*pbS++, 4, y);
	    dw |= (DODITH8(*pbS++, 5, y) << 8);
	    dw |= (DODITH8(*pbS++, 6, y) << 16);
	    dw |= (DODITH8(*pbS++, 7, y) << 24);
            * ( (DWORD _huge UNALIGNED *) pbD)++ = dw;
	}

	/* clean up remainder (less than 8 bytes per row) */
	for ( ; x < DstXE; x++) {
	    *pbD++ = DODITH8(*pbS++, x, y);
	}
	
        pbS += wWidthSrc;
        pbD += wWidthDst;
    }
#undef DODITH8
}

/*
 * C version of 16->24 mapping (in asm for win16)
 */
extern void FAR PASCAL Map16to24(
    LPBITMAPINFOHEADER biDst,           // --> BITMAPINFO of the dest
    LPVOID             lpDst,           // --> to destination bits
    int                DstX,            // Destination origin - x coordinate
    int                DstY,            // Destination origin - y coordinate
    int                DstXE,           // x extent of the BLT
    int                DstYE,           // y extent of the BLT
    LPBITMAPINFOHEADER biSrc,           // --> BITMAPINFO of the source
    LPVOID             lpSrc,           // --> to source bits
    int                SrcX,            // Source origin - x coordinate
    int                SrcY,            // Source origin - y coordinate
    LPVOID             lpDitherTable)   // dither table.
{

    int x,y;
    UINT wWidthSrc;
    UINT wWidthDst;
    BYTE _huge *pbS;
    BYTE _huge *pbD;
    WORD wRGB;

    if (biDst->biBitCount != 24 || biSrc->biBitCount != 16)
        return;

    /* width of one row is nr pixels * size of pixel rounded to 4-bytes */
    wWidthSrc = ((UINT) (biSrc->biWidth * 2) +3)&~3;
    wWidthDst = ((UINT) (biDst->biWidth * 3) +3)&~3;

    /* advance to start of source, dest rect within DIB */
    pbD = (BYTE _huge *)lpDst + (DstX * 3) + DstY * wWidthDst;
    pbS = (BYTE _huge *)lpSrc + (SrcX * 2) + SrcY * wWidthSrc;

    /* amount to advance pointer to next line from end of source, dest rect */
    wWidthSrc -= (DstXE * 2);
    wWidthDst -= (DstXE * 3);

    for (y=0; y<DstYE; y++) {
        for (x=0; x<DstXE; x++) {
	    wRGB = *((LPWORD)pbS)++;
	    *pbD++ = (wRGB << 3) & 0xf8;
	    *pbD++ = (wRGB >> 2) & 0xf8;
	    *pbD++ = (wRGB >> 7) & 0xf8;
	}

        pbS += wWidthSrc;
        pbD += wWidthDst;
    }
}

void FAR PASCAL Map32to24(
    LPBITMAPINFOHEADER biDst,           // --> BITMAPINFO of the dest
    LPVOID             lpDst,           // --> to destination bits
    int                DstX,            // Destination origin - x coordinate
    int                DstY,            // Destination origin - y coordinate
    int                DstXE,           // x extent of the BLT
    int                DstYE,           // y extent of the BLT
    LPBITMAPINFOHEADER biSrc,           // --> BITMAPINFO of the source
    LPVOID             lpSrc,           // --> to source bits
    int                SrcX,            // Source origin - x coordinate
    int                SrcY,            // Source origin - y coordinate
    LPVOID             lpDitherTable)   // dither table.
{
    return;
}
#endif
