/*--------------------------------------------------------------------------*\
|									     |
|   MSRLE.H								     |
\*--------------------------------------------------------------------------*/

 /**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1991 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/

#ifdef _WIN32
#define huge

// NT and WIN95 use different debug switches.
// NT defines DBG to be 0 or 1
// Win95 defines DEBUG, or does not define it
// Map one methodology to the other

#ifndef DBG
#ifdef DEBUG
    #define DBG 1
#else
    #define DBG 0
#endif
#endif

#undef DEBUG

#if DBG
    #define DEBUG
    #define STATICFN
    #define STATICDT
#else
    #define STATICFN static
    #define STATICDT static
#endif
#endif

//
// compressor state info
//
typedef struct {
    long	lMinFrameSize;
    long        lMaxFrameSize;
    long        tolTemporal;
    long        tolSpatial;
    long        tolMax;
    int         iMaxRunLen;
} RLESTATE, FAR *LPRLESTATE, *PRLESTATE;

typedef struct {
    LPVOID	lpbiPrev;
    int         iStart;
    long        lLastParm;
    long	lFrames;
    RLESTATE    RleState;
    BOOL        fCompressBegin;
    BOOL        fDecompressBegin;
} RLEINST, *PRLEINST;

#define BI_DIBX     0x78626964l     // 'dibx'
#define BI_DIBC     0x63626964l     // 'dibc'

#define QUALITY_DEFAULT     8500

#define IDS_DESCRIPTION 42
#define IDS_NAME        43

extern HMODULE ghModule;

/****************************************************************************
 ***************************************************************************/

#ifdef _INC_COMPDDK

void       NEAR PASCAL RleLoad(void);
void       NEAR PASCAL RleFree(void);
PRLEINST   NEAR PASCAL RleOpen(void);
DWORD      NEAR PASCAL RleClose(PRLEINST pri);
DWORD      NEAR PASCAL RleGetState(PRLEINST pri, LPVOID pv, DWORD dwSize);
DWORD      NEAR PASCAL RleSetState(PRLEINST pri, LPVOID pv, DWORD dwSize);

DWORD      NEAR PASCAL RleGetInfo(PRLEINST pri, ICINFO FAR *icinfo, DWORD dwSize);

DWORD      NEAR PASCAL RleCompressBegin(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD      NEAR PASCAL RleCompressQuery(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn,LPBITMAPINFOHEADER lpbiOut);
DWORD      NEAR PASCAL RleCompressGetFormat(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD      NEAR PASCAL RleCompress(PRLEINST pri,ICCOMPRESS FAR *icinfo, DWORD dwSize);
DWORD      NEAR PASCAL RleCompressGetSize(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD      NEAR PASCAL RleCompressEnd(PRLEINST lpri);

DWORD      NEAR PASCAL RleDecompressBegin(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD      NEAR PASCAL RleDecompressQuery(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn,LPBITMAPINFOHEADER lpbiOut);
DWORD      NEAR PASCAL RleDecompressGetFormat(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD      NEAR PASCAL RleDecompress(PRLEINST pri,ICDECOMPRESS FAR *icinfo, DWORD dwSize);
DWORD      NEAR PASCAL RleDecompressEnd(PRLEINST pri);

#endif

/****************************************************************************
 ***************************************************************************/

BOOL FAR PASCAL CrunchDib(PRLEINST pri,
                            LPBITMAPINFOHEADER lpbiRle, LPVOID lpRle,
                            LPBITMAPINFOHEADER lpbiPrev,LPVOID lpPrev,
                            LPBITMAPINFOHEADER lpbiDib, LPVOID lpDib);

BOOL FAR PASCAL SplitDib (PRLEINST pri,
                            LPBITMAPINFOHEADER lpbiRle, LPVOID lpRle,
                            LPBITMAPINFOHEADER lpbiPrev,LPVOID lpPrev,
                            LPBITMAPINFOHEADER lpbiDib, LPVOID lpDib);

//#define MAXSUMSQUARES               195075L
#define MAXTOL                      0x00FFFFFFL
#define ADAPTIVE                    -1
#define NO_LIMIT                    -1

/****************************************************************************
 ***************************************************************************/

/* RgbTol Table structure */

typedef struct {
        int             ClrUsed;
        RGBQUAD         argbq[256];
	DWORD huge	*hpTable;
} RGBTOL;

extern RGBTOL gRgbTol;

/* Public functions anyone can call */

BOOL FAR PASCAL RleDeltaFrame(
        LPBITMAPINFOHEADER  lpbiRle,    LPBYTE pbRle,
        LPBITMAPINFOHEADER  lpbiPrev,   LPBYTE pbPrev,
        LPBITMAPINFOHEADER  lpbiDib,    LPBYTE pbDib,
        int         iStart,
        int         iLen,
        long        tolTemporal,
        long        tolSpatial,
        int         maxRun,
        int         minJump);

BOOL NEAR PASCAL MakeRgbTable(LPBITMAPINFOHEADER lpbi);

void NEAR PASCAL DecodeRle(LPBITMAPINFOHEADER lpbi, LPVOID pb, LPVOID prle);
#ifndef _WIN32
void NEAR PASCAL DecodeRle386(LPBITMAPINFOHEADER lpbi, LPVOID pb, LPVOID prle);
void NEAR PASCAL DecodeRle286(LPBITMAPINFOHEADER lpbi, LPVOID pb, LPVOID prle);
#endif

// in DF.ASM

#ifndef _WIN32
extern void FAR PASCAL DeltaFrame386(
    LPBITMAPINFOHEADER  lpbi,
    LPVOID              pbPrev,
    LPVOID              pbDib,
    LPVOID		pbRle,
    WORD		MaxRunLength,
    WORD		MinJumpLength,
    LPVOID              TolTable,
    DWORD               tolTemporal,
    DWORD               tolSpatial);
#else
extern void DeltaFrameC(
    LPBITMAPINFOHEADER  lpbi,
    LPBYTE              pbPrev,
    LPBYTE              pbDib,
    LPBYTE		pbRle,
    UINT		MaxRunLength,
    UINT		MinJumpLength,
    LPDWORD             TolTable,
    DWORD               tolTemporal,
    DWORD               tolSpatial);
#endif

/****************************************************************************
 DIB macros.
 ***************************************************************************/

#define WIDTHBYTES(i)           ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */

#define DibWidthBytesN(lpbi, n) (UINT)WIDTHBYTES((UINT)(lpbi)->biWidth * (UINT)(n))
#define DibWidthBytes(lpbi)     DibWidthBytesN(lpbi, (lpbi)->biBitCount)

#define DibSizeImage(lpbi)      ((lpbi)->biSizeImage == 0 \
                                    ? ((DWORD)(UINT)DibWidthBytes(lpbi) * (DWORD)(UINT)(lpbi)->biHeight) \
                                    : (lpbi)->biSizeImage)

// Use when biSizeImage is known to be correct (e.g. after FixBitmapInfo)
#define DibSizeImageX(lpbi)     ((lpbi)->biSizeImage)

#define DibSize(lpbi)           ((lpbi)->biSize + (lpbi)->biSizeImage + (int)(lpbi)->biClrUsed * sizeof(RGBQUAD))
#define DibPaletteSize(lpbi)    (DibNumColors(lpbi) * sizeof(RGBQUAD))

#define DibPtr(lpbi)            (LPVOID)(DibColors(lpbi) + (UINT)(lpbi)->biClrUsed)
#define DibColors(lpbi)         ((LPRGBQUAD)((LPBYTE)(lpbi) + (int)(lpbi)->biSize))

#define DibNumColors(lpbi)      ((lpbi)->biClrUsed == 0 && (lpbi)->biBitCount <= 8 \
                                    ? (int)(1 << (int)(lpbi)->biBitCount)          \
                                    : (int)(lpbi)->biClrUsed)

#define DibXYN(lpbi,pb,x,y,n)   (LPVOID)(                                     \
                                (LPBYTE)(pb) +                                 \
                                (UINT)((UINT)(x) * (UINT)(n) / 8u) +          \
                                ((DWORD)DibWidthBytesN(lpbi,n) * (DWORD)(UINT)(y)))

#define DibXY(lpbi,x,y)         DibXYN(lpbi,DibPtr(lpbi),x,y,(lpbi)->biBitCount)


#define FixBitmapInfo(lpbi)     if ((lpbi)->biSizeImage == 0)                 \
                                    (lpbi)->biSizeImage = DibSizeImage(lpbi); \
                                if ((lpbi)->biClrUsed == 0)                   \
                                    (lpbi)->biClrUsed = DibNumColors(lpbi);



/****************************************************************************
 ***************************************************************************/

#ifdef DEBUG
    extern void FAR CDECL dprintf(LPSTR, ...);
    #define DPF(_x_) dprintf _x_
#else
    #define DPF ; / ## /
#endif
