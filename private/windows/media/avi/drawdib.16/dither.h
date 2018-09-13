//////////////////////////////////////////////////////////////////////////////
//
//  this code dither a 256 color DIB bitmap down to the VGA 16 colors.
//
//////////////////////////////////////////////////////////////////////////////

typedef void (FAR PASCAL *DITHERPROC)(
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
    LPVOID             lpDitherTable);  // dither table.

//
//  call DitherInit() to set up the 16k dither table.
//  you need to call DitherInit() whenever the source colors
//  change.  DitherInit() returns a pointer to the dither table
//
extern LPVOID VFWAPI DitherInit(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable);
extern void   VFWAPI DitherTerm(LPVOID lpDitherTable);

extern LPVOID FAR PASCAL Dither8Init(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable);
extern void   FAR PASCAL Dither8Term(LPVOID lpDitherTable);

extern LPVOID FAR PASCAL DitherDeviceInit(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable);

extern LPVOID FAR PASCAL Dither16Init(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable);
extern void   FAR PASCAL Dither16Term(LPVOID lpDitherTable);

extern LPVOID FAR PASCAL Dither24Init(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable);
extern void   FAR PASCAL Dither24Term(LPVOID lpDitherTable);

extern LPVOID FAR PASCAL Dither32Init(LPBITMAPINFOHEADER lpbi, LPBITMAPINFOHEADER lpbiOut, DITHERPROC FAR *lpDitherProc, LPVOID lpDitherTable);
extern void   FAR PASCAL Dither32Term(LPVOID lpDitherTable);

extern void   FAR PASCAL DitherTableFree(void);

//extern HPALETTE FAR CreateDith775Palette(void); // (in dith775.c)

//
//  call this to actualy do the dither. (in dither8.asm)
//
extern void FAR PASCAL Dither8(
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
    LPVOID             lpDitherTable);  // dither table.

//
//  call this to actualy do the dither. (in dither.c)
//
extern void FAR PASCAL Dither8C(
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
    LPVOID             lpDitherTable);  // dither table.


//
//  call this to actualy do the dither. (in dith775.c)
//
extern void FAR PASCAL Dither16(
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
    LPVOID             lpDitherTable);  // dither table.

//
//  call this to actualy do the dither. (in dith775.c)
//
extern void FAR PASCAL Dither24(
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
    LPVOID             lpDitherTable);  // dither table.

//
//  call this to map 16 bpp DIBs to 24bit (in mapa.asm)
//
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
    LPVOID             lpDitherTable);  // dither table.


//
//  call this to map 32 bpp DIBs to 24bit (in mapa.asm)
//
extern void FAR PASCAL Map32to24(
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
    LPVOID             lpDitherTable);  // dither table.
