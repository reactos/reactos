
#ifndef __WIN32K_BITMAPS_H
#define __WIN32K_BITMAPS_H

#include <win32k/dc.h>
#include <win32k/gdiobj.h>

typedef struct _DDBITMAP
{
  const PDRIVER_FUNCTIONS  pDriverFunctions;
//  DHPDEV  PDev;
//  HSURF  Surface;
} DDBITMAP;

/* GDI logical bitmap object */
typedef struct _BITMAPOBJ
{
  BITMAP      bitmap;
  SIZE        size;   /* For SetBitmapDimension() */

  DDBITMAP   *DDBitmap;

  /* For device-independent bitmaps: */
  DIBSECTION *dib;
  RGBQUAD *ColorMap;
} BITMAPOBJ, *PBITMAPOBJ;

/*  Internal interface  */

#define  BITMAPOBJ_AllocBitmap()  \
  ((HBITMAP) GDIOBJ_AllocObj (sizeof (BITMAPOBJ), GO_BITMAP_MAGIC))
#define  BITMAPOBJ_FreeBitmap(hBMObj)  \
  GDIOBJ_FreeObj((HGDIOBJ) hBMObj, GO_BITMAP_MAGIC, GDIOBJFLAG_DEFAULT)
#define  BITMAPOBJ_HandleToPtr(hBMObj)  \
  ((PBITMAPOBJ) GDIOBJ_LockObj ((HGDIOBJ) hBMObj, GO_BITMAP_MAGIC))
#define  BITMAPOBJ_ReleasePtr(hBMObj)  \
  GDIOBJ_UnlockObj ((HGDIOBJ) hBMObj, GO_BITMAP_MAGIC)

/*#define  BITMAPOBJ_PtrToHandle(hBMObj)  \
  ((HBITMAP) GDIOBJ_PtrToHandle ((PGDIOBJ) hBMObj, GO_BITMAP_MAGIC))*/
#define  BITMAPOBJ_LockBitmap(hBMObj) GDIOBJ_LockObject ((HGDIOBJ) hBMObj)
#define  BITMAPOBJ_UnlockBitmap(hBMObj) GDIOBJ_UnlockObject ((HGDIOBJ) hBMObj)

INT     FASTCALL BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp);
HBITMAP FASTCALL BITMAPOBJ_CopyBitmap (HBITMAP  hBitmap);
INT     FASTCALL DIB_GetDIBWidthBytes (INT  width, INT  depth);
int     STDCALL  DIB_GetDIBImageBytes (INT  width, INT  height, INT  depth);
int     FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
INT     STDCALL  BITMAP_GetObject(BITMAPOBJ * bmp, INT count, LPVOID buffer);
BOOL    FASTCALL Bitmap_InternalDelete( PBITMAPOBJ pBmp );
HBITMAP FASTCALL BitmapToSurf(PBITMAPOBJ BitmapObj);

/*  User Entry Points  */
BOOL
STDCALL
W32kBitBlt (
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
W32kCreateBitmap (
	INT		Width,
	INT		Height,
	UINT		Planes,
	UINT		BitsPerPel,
	CONST VOID	* Bits
	);
HBITMAP
STDCALL
W32kCreateCompatibleBitmap (
	HDC	hDC,
	INT	Width,
	INT	Height
	);
HBITMAP
STDCALL
W32kCreateBitmapIndirect (
	CONST BITMAP	* BM
	);
HBITMAP
STDCALL
W32kCreateDIBitmap (
	HDC			hDC,
	CONST BITMAPINFOHEADER	* bmih,
	DWORD			Init,
	CONST VOID		* bInit,
	CONST BITMAPINFO	* bmi,
	UINT			Usage
	);
HBITMAP
STDCALL
W32kCreateDIBSection (
	HDC			hDC,
	CONST BITMAPINFO	* bmi,
	UINT			Usage,
	VOID			* Bits,
	HANDLE			hSection,
	DWORD			dwOffset
	);
HBITMAP
STDCALL
W32kCreateDiscardableBitmap (
	HDC	hDC,
	INT	Width,
	INT	Height
	);
BOOL
STDCALL
W32kExtFloodFill (
	HDC		hDC,
	INT		XStart,
	INT		YStart,
	COLORREF	Color,
	UINT		FillType
	);
BOOL
STDCALL
W32kFloodFill (
	HDC		hDC,
	INT		XStart,
	INT		YStart,
	COLORREF	Fill
	);
LONG
STDCALL
W32kGetBitmapBits (
	HBITMAP	hBitmap,
	LONG	Buffer,
	LPVOID	Bits
	);
BOOL
STDCALL
W32kGetBitmapDimensionEx (
	HBITMAP	hBitmap,
	LPSIZE	Dimension
	);
UINT
STDCALL
W32kGetDIBColorTable (
	HDC	hDC,
	UINT	StartIndex,
	UINT	Entries,
	RGBQUAD	* Colors
	);
INT
STDCALL
W32kGetDIBits (
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
W32kGetPixel (
	HDC	hDC,
	INT	XPos,
	INT	YPos
	);
BOOL
STDCALL
W32kMaskBlt (
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
W32kPlgBlt (
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
W32kSetBitmapBits (
	HBITMAP		hBitmap,
	DWORD		Bytes,
	CONST VOID	* Bits
	);
BOOL
STDCALL
W32kSetBitmapDimensionEx (
	HBITMAP	hBitmap,
	INT	Width,
	INT	Height,
	LPSIZE	Size
	);
UINT
STDCALL
W32kSetDIBColorTable (
	HDC		hDC,
	UINT		StartIndex,
	UINT		Entries,
	CONST RGBQUAD	* Colors
	);
INT
STDCALL
W32kSetDIBits (
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
W32kSetDIBitsToDevice (
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
W32kSetPixel (
	HDC		hDC,
	INT		X,
	INT		Y,
	COLORREF	Color
	);
BOOL
STDCALL
W32kSetPixelV (
	HDC		hDC,
	INT		X,
	INT		Y,
	COLORREF	Color
	);
BOOL
STDCALL
W32kStretchBlt (
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
W32kStretchDIBits (
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

