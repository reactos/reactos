//////////////////////////////////////////////////////////////////////////////
//
//  DITH666.C  - full color dither (to a palette with 6 red, 6 green 6 blue
//               levels)
//
//  NOTE this file contains the 'C' code and DITH666A.ASM has the ASM code.
//
//  This file does the following dithering
//
//      32bpp   -> 8bpp
//      24bpp   -> 8bpp
//      16bpp   -> 8bpp
//
//      8bpp    -> 4bpp     N/I
//      16bpp   -> 4bpp     N/I
//      24bpp   -> 4bpp     N/I
//      32bpp   -> 4bpp     N/I
//
//////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <windowsx.h>
#include "drawdibi.h"
#include "dither.h"

#if defined(WIN32) || defined(WANT_286) // 'C' code for Win32
#define USE_C
#endif

#include "dith666.h"

int         giDitherTableUsage = 0;
LPVOID      glpDitherTable;

static void Get666Colors(LPBITMAPINFOHEADER lpbi);

//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////

void FAR PASCAL Dither16(LPBITMAPINFOHEADER,LPVOID,int,int,int,int,LPBITMAPINFOHEADER,LPVOID,int,int,LPVOID);
void FAR PASCAL Dither24(LPBITMAPINFOHEADER,LPVOID,int,int,int,int,LPBITMAPINFOHEADER,LPVOID,int,int,LPVOID);
void FAR PASCAL Dither32(LPBITMAPINFOHEADER,LPVOID,int,int,int,int,LPBITMAPINFOHEADER,LPVOID,int,int,LPVOID);

//////////////////////////////////////////////////////////////////////////////
//
//   DitherTableInit()
//
//////////////////////////////////////////////////////////////////////////////

static LPVOID DitherTableInit()
{
#ifdef DEBUG
    DWORD time = timeGetTime();
#endif

#ifdef XDEBUG
    int X,Y;
    char aBuffer[100];
    char far *pBuffer = aBuffer;

    GetProfileString("DrawDib", "Matrix5", "", aBuffer, sizeof(aBuffer));

    if (aBuffer[0])
    {
        for(Y = 0;Y < 4;Y++)
        {
            for(X = 0;X < 4;X++)
            {
                while(!isdigit(*pBuffer))
                {
                        pBuffer++;
                }

                aHalftone4x4_5[X][Y] = *pBuffer - '0';
                pBuffer++;
            }
        }
    }
#endif

    if (aHalftone8[0][0][0][0] == (BYTE)-1)
    {
        int i,x,y;

        for (x=0; x<4; x++)
            for (y=0; y<4; y++)
                for (i=0; i<256; i++)
                    aHalftone8[0][x][y][i] = (i/51 + (i%51 > aHalftone4x4[x][y]));

        for (x=0; x<4; x++)
            for (y=0; y<4; y++)
                for (i=0; i<256; i++)
                    aHalftone8[1][x][y][i] = 6 * (i/51 + (i%51 > aHalftone4x4[x][y]));

        for (x=0; x<4; x++)
            for (y=0; y<4; y++)
                for (i=0; i<256; i++)
                    aHalftone8[2][x][y][i] = 36 * (i/51 + (i%51 > aHalftone4x4[x][y]));
    }

#ifdef USE_C
    if (aHalftone5[0][0][0][0] == (BYTE)-1)
    {
        int i,x,y,z,n;

        for (x=0; x<4; x++)
            for (y=0; y<4; y++)
                for (z=0; z<256; z++) {
                    n = (z >> 2) & 0x1F;
                    i = n > 0 ? n-1 : 0;
                    aHalftone5[0][x][y][z] = (i/6 + (i%6 > aHalftone4x4_5[x][y]));
                }

        for (x=0; x<4; x++)
            for (y=0; y<4; y++)
                for (z=0; z<256; z++) {
                    n = (z & 0x1F);
                    i = n > 0 ? n-1 : 0;
                    aHalftone5[1][x][y][z] = 6 * (i/6 + (i%6 > aHalftone4x4_5[x][y]));
                }

        for (x=0; x<4; x++)
            for (y=0; y<4; y++)
                for (z=0; z<256; z++) {
                    n = z & 0x1F;
                    i = n > 0 ? n-1 : 0;
                    aHalftone5[2][x][y][z] = 36 * (i/6 + (i%6 > aHalftone4x4_5[x][y]));
                }
    }
#endif

    DPF(("DitherTableInit() took %ldms", timeGetTime() - time));

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
//   DitherInit()
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR PASCAL Dither8Init(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable)
{
    UINT x,y,i,r,g,b;
    BYTE FAR *pb;

    Get666Colors(lpbiOut);

    DitherTableInit();

    if (lpDitherTable == NULL)
        lpDitherTable = GlobalAllocPtr(GHND, 256*8*8);

    if (lpDitherTable == NULL)
        return (LPVOID)-1;

    pb = (LPBYTE)lpDitherTable;

    for (y=0; y<8; y++)
    {
        for (i=0; i<256; i++)
        {
            r = ((LPRGBQUAD)(lpbi+1))[i].rgbRed;
            g = ((LPRGBQUAD)(lpbi+1))[i].rgbGreen;
            b = ((LPRGBQUAD)(lpbi+1))[i].rgbBlue;

            for (x=0; x<8; x++)
            {
                *pb++ = DITH8(x,y,r,g,b);
            }
        }
    }

    *lpDitherProc = Dither8;

    return lpDitherTable;
}

//////////////////////////////////////////////////////////////////////////////
//
//   DitherInit()
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR PASCAL Dither16Init(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable)
{
    Get666Colors(lpbiOut);

    *lpDitherProc = Dither16;

    DitherTableInit();

#ifndef USE_C
    //
    // we dont need to re-init the dither table, unless it is not ours then
    // we should free it.
    //
    if (lpDitherTable && lpDitherTable != glpDitherTable)
    {
        DitherTerm(lpDitherTable);
        lpDitherTable = NULL;
    }

    //
    // we dont need to re-init table
    //
    if (lpDitherTable != NULL)
        return lpDitherTable;

    if (glpDitherTable)
    {
        giDitherTableUsage++;
        return glpDitherTable;
    }
    else
    {
        //
        //  build a table that maps a RGB555 directly to a palette index
        //  we actualy build 4 tables, we assume a 2x2 dither and build
        //  a table for each position in the matrix.
        //

        UINT x,y,r,g,b;
        BYTE FAR *pb;

#ifdef DEBUG
        DWORD time = timeGetTime();
#endif
        lpDitherTable = GlobalAllocPtr(GMEM_MOVEABLE|GMEM_SHARE, 32768l*4);

        if (lpDitherTable == NULL)
            return (LPVOID)-1;

        glpDitherTable = lpDitherTable;
        giDitherTableUsage = 1;

        for (y=0; y<2; y++)
        {
            if (y == 0)
                pb = (BYTE FAR *)lpDitherTable;
            else
                pb = (BYTE FAR *)((BYTE _huge *)lpDitherTable + 65536);

            for (r=0; r<32; r++)
                for (g=0; g<32; g++)
                    for (b=0; b<32; b++)
                        for (x=0; x<2; x++)
                            *pb++ = DITH31(x,y,r,g,b);
        }

        DPF(("Dither16Init() took %ldms", timeGetTime() - time));
    }
#endif
    return lpDitherTable;
}

//////////////////////////////////////////////////////////////////////////////
//
//   DitherTerm()
//
//////////////////////////////////////////////////////////////////////////////

void FAR PASCAL Dither16Term(LPVOID lpDitherTable)
{
    if (giDitherTableUsage == 0 || --giDitherTableUsage > 0)
        return;

    if (glpDitherTable)
    {
        GlobalFreePtr(glpDitherTable);
        glpDitherTable = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//   Dither24Init()
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR PASCAL Dither24Init(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable)
{
    Get666Colors(lpbiOut);

    *lpDitherProc = Dither24;

    return DitherTableInit();
}

//////////////////////////////////////////////////////////////////////////////
//
//   Dither24Term()
//
//////////////////////////////////////////////////////////////////////////////

void FAR PASCAL Dither24Term(LPVOID lpDitherTable)
{
}

//////////////////////////////////////////////////////////////////////////////
//
//   Dither32Init()
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR PASCAL Dither32Init(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable)
{
    Get666Colors(lpbiOut);

    *lpDitherProc = Dither32;

    return DitherTableInit();
}

//////////////////////////////////////////////////////////////////////////////
//
//   Dither32Term()
//
//////////////////////////////////////////////////////////////////////////////

void FAR PASCAL Dither32Term(LPVOID lpDitherTable)
{
}

//////////////////////////////////////////////////////////////////////////////
//
//   GetDithColors() get the dither palette
//
//////////////////////////////////////////////////////////////////////////////

static void Get666Colors(LPBITMAPINFOHEADER lpbi)
{
    RGBQUAD FAR *prgb = (RGBQUAD FAR *)(((LPBYTE)lpbi) + (UINT)lpbi->biSize);
    int i;

    for (i=0; i<256; i++)
    {
        prgb[i].rgbRed   = pal666[i][0];
        prgb[i].rgbGreen = pal666[i][1];
        prgb[i].rgbBlue  = pal666[i][2];
        prgb[i].rgbReserved = 0;
    }

    lpbi->biClrUsed = 256;
}

#ifdef USE_C

//////////////////////////////////////////////////////////////////////////////
//
//  Dither24   - dither from 24 to 8 using the Table method in 'C' Code
//
//////////////////////////////////////////////////////////////////////////////

void FAR PASCAL Dither24(
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
    BYTE r,g,b;
    UINT wWidthSrc;
    UINT wWidthDst;
    BYTE _huge *pbS;
    BYTE _huge *pbD;

    if (biDst->biBitCount != 8 || biSrc->biBitCount != 24)
        return;

    wWidthSrc = ((UINT)biSrc->biWidth*3+3)&~3;
    wWidthDst = ((UINT)biDst->biWidth+3)&~3;

    pbD = (BYTE _huge *)lpDst + DstX   + (DWORD)(UINT)DstY * (DWORD)wWidthDst;
    pbS = (BYTE _huge *)lpSrc + SrcX*3 + (DWORD)(UINT)SrcY * (DWORD)wWidthSrc;

    wWidthSrc -= DstXE*3;
    wWidthDst -= DstXE;

#define GET24() \
    b = *pbS++; \
    g = *pbS++; \
    r = *pbS++;

    for (y=0; y<DstYE; y++) {

        for (x=0; x<DstXE; x++) {
            GET24(); *pbD++ = DITH8(x,y,r,g,b);
        }

        pbS += wWidthSrc;
        pbD += wWidthDst;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Dither32  - dither from 32 to 8 using the Table method in 'C' Code
//
//////////////////////////////////////////////////////////////////////////////

void FAR PASCAL Dither32(
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
    BYTE r,g,b;
    UINT wWidthSrc;
    UINT wWidthDst;
    BYTE _huge *pbS;
    BYTE _huge *pbD;

    if (biDst->biBitCount != 8 || biSrc->biBitCount != 32)
        return;

    wWidthSrc = ((UINT)biSrc->biWidth*4+3)&~3;
    wWidthDst = ((UINT)biDst->biWidth+3)&~3;

    pbD = (BYTE _huge *)lpDst + DstX   + (DWORD)(UINT)DstY * (DWORD)wWidthDst;
    pbS = (BYTE _huge *)lpSrc + SrcX*4 + (DWORD)(UINT)SrcY * (DWORD)wWidthSrc;

    wWidthSrc -= DstXE*4;
    wWidthDst -= DstXE;

#define GET32() \
    b = *pbS++; \
    g = *pbS++; \
    r = *pbS++; \
    pbS++;

    for (y=0; y<DstYE; y++) {

        for (x=0; x<DstXE; x++)
        {
            GET32();
            *pbD++ = DITH8(x,y,r,g,b);
        }

        pbS += wWidthSrc;
        pbD += wWidthDst;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Dither16  - dither from 16 to 8 using the Table method in 'C' Code
//
//////////////////////////////////////////////////////////////////////////////

void FAR PASCAL Dither16(
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
    WORD w;
    UINT wWidthSrc;
    UINT wWidthDst;
    BYTE _huge *pbS;
    BYTE _huge *pbD;

    if (biDst->biBitCount != 8 || biSrc->biBitCount != 16)
        return;

    wWidthSrc = ((UINT)biSrc->biWidth*2+3)&~3;
    wWidthDst = ((UINT)biDst->biWidth+3)&~3;

    pbD = (BYTE _huge *)lpDst + DstX   + (DWORD)(UINT)DstY * (DWORD)wWidthDst;
    pbS = (BYTE _huge *)lpSrc + SrcX*2 + (DWORD)(UINT)SrcY * (DWORD)wWidthSrc;

    wWidthSrc -= DstXE*2;
    wWidthDst -= DstXE;

#define GET16() \
    w = *((WORD _huge *)pbS)++;

    for (y=0; y<DstYE; y++) {

        for (x=0; x<DstXE; x++)
        {
            GET16();
            *pbD++ = DITH5(x,y,w);
        }

        pbS += wWidthSrc;
        pbD += wWidthDst;
    }
}

#endif
