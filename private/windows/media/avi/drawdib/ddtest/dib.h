/*----------------------------------------------------------------------------*\
|   Routines for dealing with Device independent bitmaps                       |
|									       |
|   History:                                                                   |
|       06/23/89 toddla     Created                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/

HANDLE      OpenDIB(LPTSTR szFile, int fh);
BOOL        WriteDIB(LPTSTR szFile, int fh, HANDLE hdib);
UINT        PaletteSize(VOID FAR * pv);
UINT        DibNumColors(VOID FAR * pv);
HPALETTE    CreateDibPalette(HANDLE hdib);
HPALETTE    CreateBIPalette(LPBITMAPINFOHEADER lpbi);
HPALETTE    CreateExplicitPalette(void);
HPALETTE    CreateColorPalette(void);
HANDLE      DibFromBitmap(HBITMAP hbm, DWORD biStyle, UINT biBits, HPALETTE hpal, UINT wUsage);
HANDLE      DibFromDib(HANDLE hdib, DWORD biStyle, UINT biBits, HPALETTE hpal, UINT wUsage);
HBITMAP     BitmapFromDib(HANDLE hdib, HPALETTE hpal, UINT wUsage);
BOOL        SetDibUsage(HANDLE hdib, HPALETTE hpal,UINT wUsage);
BOOL        DibInfo(LPBITMAPINFOHEADER lpbiSource, LPBITMAPINFOHEADER lpbiTarget);
HANDLE      ReadDibBitmapInfo(int fh);
BOOL        SetPalFlags(HPALETTE hpal, int iIndex, int cntEntries, UINT wFlags);

BOOL        DrawBitmap(HDC hdc, int x, int y, HBITMAP hbm, DWORD rop);
BOOL        StretchBitmap(HDC hdc, int x, int y, int dx, int dy, HBITMAP hbm, int x0, int y0, int dx0, int dy0, DWORD rop);

BOOL        DibBlt(HDC hdc, int x0, int y0, int dx, int dy, HANDLE hdib, int x1, int y1, LONG rop, UINT wUsage);
BOOL        StretchDibBlt(HDC hdc, int x, int y, int dx, int dy, HANDLE hdib, int x0, int y0, int dx0, int dy0, LONG rop,  UINT wUsage);

LPVOID      DibLock(HANDLE hdib,int x, int y);
VOID        DibUnlock(HANDLE hdib);
LPVOID      DibXY(LPBITMAPINFOHEADER lpbi,int x, int y);
HANDLE      CreateDib(int bits, int dx, int dy);

#define BFT_ICON   0x4349   /* 'IC' */
#define BFT_BITMAP 0x4d42   /* 'BM' */
#define BFT_CURSOR 0x5450   /* 'PT' */

#define ISDIB(bft) ((bft) == BFT_BITMAP)
#define ALIGNULONG(i)     ((i+3)/4*4)        /* ULONG aligned ! */
#define WIDTHBYTES(i)     ((i+31)/32*4)      /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)

#define PALVERSION      0x300
#define MAXPALETTE      256
