
#ifndef __WIN32K_BITMAPS_H
#define __WIN32K_BITMAPS_H

#include <win32k/dc.h>
#include <win32k/gdiobj.h>

typedef struct _DDBITMAP
{
  const PDRIVER_FUNCTIONS  pDriverFunctions;
/*  DHPDEV  PDev; */
/*  HSURF  Surface; */
} DDBITMAP;

/* GDI logical bitmap object */
typedef struct _BITMAPOBJ
{
  BITMAP      bitmap;
  SIZE        dimension;   /* For SetBitmapDimension(), do NOT use
                              to get width/height of bitmap, use
                              bitmap.bmWidth/bitmap.bmHeight for
                              that */

  DDBITMAP   *DDBitmap;

  /* For device-independent bitmaps: */
  DIBSECTION *dib;
  RGBQUAD *ColorMap;
} BITMAPOBJ, *PBITMAPOBJ;

/*  Internal interface  */

#define  BITMAPOBJ_AllocBitmap()  \
  ((HBITMAP) GDIOBJ_AllocObj (sizeof (BITMAPOBJ), GDI_OBJECT_TYPE_BITMAP, (GDICLEANUPPROC) Bitmap_InternalDelete))
#define  BITMAPOBJ_FreeBitmap(hBMObj)  \
  GDIOBJ_FreeObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP, GDIOBJFLAG_DEFAULT)
#define  BITMAPOBJ_LockBitmap(hBMObj) GDIOBJ_LockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP)
#define  BITMAPOBJ_UnlockBitmap(hBMObj) GDIOBJ_UnlockObj((HGDIOBJ) hBMObj, GDI_OBJECT_TYPE_BITMAP)

INT     FASTCALL BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp);
HBITMAP FASTCALL BITMAPOBJ_CopyBitmap (HBITMAP  hBitmap);
INT     FASTCALL DIB_GetDIBWidthBytes (INT  width, INT  depth);
int     STDCALL  DIB_GetDIBImageBytes (INT  width, INT  height, INT  depth);
INT     FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
INT     STDCALL  BITMAP_GetObject(BITMAPOBJ * bmp, INT count, LPVOID buffer);
BOOL    FASTCALL Bitmap_InternalDelete( PBITMAPOBJ pBmp );
HBITMAP FASTCALL BitmapToSurf(PBITMAPOBJ BitmapObj, HDEV GDIDevice);

/*  User Entry Points  */
BOOL
STDCALL
NtGdiBitBlt (
	HDC	hDCDest,
	INT	XDest,
	INT	YDest,
	INT	Width,
	INT	Height,
	HDC	hDCSrc,
	INT	XSrc,
	INT	YSrc,
	DWORD	ROP
	);
HBITMAP
STDCALL
NtGdiCreateBitmap (
	INT		Width,
	INT		Height,
	UINT		Planes,
	UINT		BitsPerPel,
	CONST VOID	* Bits
	);
HBITMAP
STDCALL
NtGdiCreateCompatibleBitmap (
	HDC	hDC,
	INT	Width,
	INT	Height
	);
HBITMAP
STDCALL
NtGdiCreateBitmapIndirect (
	CONST BITMAP	* BM
	);
HBITMAP
STDCALL
NtGdiCreateDIBitmap (
	HDC			hDC,
	CONST BITMAPINFOHEADER	* bmih,
	DWORD			Init,
	CONST VOID		* bInit,
	CONST BITMAPINFO	* bmi,
	UINT			Usage
	);
HBITMAP
STDCALL
NtGdiCreateDIBSection (
	HDC			hDC,
	CONST BITMAPINFO	* bmi,
	UINT			Usage,
	VOID			* Bits,
	HANDLE			hSection,
	DWORD			dwOffset
	);
HBITMAP
STDCALL
NtGdiCreateDiscardableBitmap (
	HDC	hDC,
	INT	Width,
	INT	Height
	);
BOOL
STDCALL
NtGdiExtFloodFill (
	HDC		hDC,
	INT		XStart,
	INT		YStart,
	COLORREF	Color,
	UINT		FillType
	);
BOOL
STDCALL
NtGdiFloodFill (
	HDC		hDC,
	INT		XStart,
	INT		YStart,
	COLORREF	Fill
	);
LONG
STDCALL
NtGdiGetBitmapBits (
	HBITMAP	hBitmap,
	LONG	Buffer,
	LPVOID	Bits
	);
BOOL
STDCALL
NtGdiGetBitmapDimensionEx (
	HBITMAP	hBitmap,
	LPSIZE	Dimension
	);
UINT
STDCALL
NtGdiGetDIBColorTable (
	HDC	hDC,
	UINT	StartIndex,
	UINT	Entries,
	RGBQUAD	* Colors
	);
INT
STDCALL
NtGdiGetDIBits (
	HDC		hDC,
	HBITMAP		hBitmap,
	UINT		StartScan,
	UINT		ScanLines,
	LPVOID		Bits,
	LPBITMAPINFO	bi,
	UINT		Usage
	);
COLORREF
STDCALL
NtGdiGetPixel (
	HDC	hDC,
	INT	XPos,
	INT	YPos
	);
BOOL
STDCALL
NtGdiMaskBlt (
	HDC	hDCDest,
	INT	XDest,
	INT	YDest,
	INT	Width,
	INT	Height,
	HDC	hDCSrc,
	INT	XSrc,
	INT	YSrc,
	HBITMAP	hMaskBitmap,
	INT	xMask,
	INT	yMask,
	DWORD	ROP
	);
BOOL
STDCALL
NtGdiPlgBlt (
	HDC		hDCDest,
	CONST POINT	* Point,
	HDC		hDCSrc,
	INT		XSrc,
	INT		YSrc,
	INT		Width,
	INT		Height,
	HBITMAP		hMaskBitmap,
	INT		xMask,
	INT		yMask
	);
LONG
STDCALL
NtGdiSetBitmapBits (
	HBITMAP		hBitmap,
	DWORD		Bytes,
	CONST VOID	* Bits
	);
BOOL
STDCALL
NtGdiSetBitmapDimensionEx (
	HBITMAP	hBitmap,
	INT	Width,
	INT	Height,
	LPSIZE	Size
	);
UINT
STDCALL
NtGdiSetDIBColorTable (
	HDC		hDC,
	UINT		StartIndex,
	UINT		Entries,
	CONST RGBQUAD	* Colors
	);
INT
STDCALL
NtGdiSetDIBits (
	HDC			hDC,
	HBITMAP			hBitmap,
	UINT			StartScan,
	UINT			ScanLines,
	CONST VOID		* Bits,
	CONST BITMAPINFO	* bmi,
	UINT			ColorUse
	);
INT
STDCALL
NtGdiSetDIBitsToDevice (
	HDC			hDC,
	INT			XDest,
	INT			YDest,
	DWORD			Width,
	DWORD			Height,
	INT			XSrc,
	INT			YSrc,
	UINT			StartScan,
	UINT			ScanLines,
	CONST VOID		* Bits,
	CONST BITMAPINFO	* bmi,
	UINT			ColorUse
	);
COLORREF
STDCALL
NtGdiSetPixel (
	HDC		hDC,
	INT		X,
	INT		Y,
	COLORREF	Color
	);
BOOL
STDCALL
NtGdiSetPixelV (
	HDC		hDC,
	INT		X,
	INT		Y,
	COLORREF	Color
	);
BOOL
STDCALL
NtGdiStretchBlt (
	HDC	hDCDest,
	INT	XOriginDest,
	INT	YOriginDest,
	INT	WidthDest,
	INT	HeightDest,
	HDC	hDCSrc,
	INT	XOriginSrc,
	INT	YOriginSrc,
	INT	WidthSrc,
	INT	HeightSrc,
	DWORD	ROP
	);
INT
STDCALL
NtGdiStretchDIBits (
	HDC			hDC,
	INT			XDest,
	INT			YDest,
	INT			DestWidth,
	INT			DestHeight,
	INT			XSrc,
	INT			YSrc,
	INT			SrcWidth,
	INT			SrcHeight,
	CONST VOID		* Bits,
	CONST BITMAPINFO	* BitsInfo,
	UINT			Usage,
	DWORD			ROP
	);
#endif

