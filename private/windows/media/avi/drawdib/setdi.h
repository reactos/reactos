#ifndef _SETDI_H_
#define _SETDI_H_

typedef void (FAR PASCAL CONVERTPROC)(
        LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc);             // pixel convert table.

struct SETDI;

typedef CONVERTPROC *PCONVERTPROC;
typedef BOOL (INITPROC)(struct SETDI *psd);
typedef BOOL (FREEPROC)(struct SETDI *psd);

typedef INITPROC *PINITPROC;
typedef FREEPROC *PFREEPROC;

typedef struct SETDI
{
        LONG         size;           // for sanity checks.

        HDC          hdc;
        HPALETTE     hpal;
        HBITMAP      hbm;
        UINT         DibUsage;

        IBITMAP      bmDst;
        IBITMAP      bmSrc;

        LPVOID       color_convert;  // dither/color convert table.
        PCONVERTPROC convert;        // convert function
        PINITPROC    init;
        PFREEPROC    free;
} SETDI, *PSETDI;

BOOL FAR SetBitmapBegin(
        PSETDI   psd,
        HDC      hdc,
        HBITMAP  hbm,               //  bitmap to set into
        LPBITMAPINFOHEADER lpbi,    //  --> BITMAPINFO of source
        UINT     DibUsage);

void FAR SetBitmapColorChange(PSETDI psd, HDC hdc, HPALETTE hpal);
void FAR SetBitmapEnd(PSETDI psd);
BOOL FAR SetBitmap(PSETDI psd, int DstX, int DstY, int DstDX, int DstDY, LPVOID lpBits, int SrcX, int SrcY, int SrcDX, int SrcDY);

BOOL GetPhysDibPaletteMap(HDC hdc, LPBITMAPINFOHEADER lpbi, UINT Usage, LPBYTE pb);
BOOL GetDibPaletteMap    (HDC hdc, LPBITMAPINFOHEADER lpbi, UINT Usage, LPBYTE pb);
#endif //_SETDI_H_
