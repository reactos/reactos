
#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

#include <windows.h>
#include <win32k/driver.h>
#include <win32k/gdiobj.h>
#include <win32k/path.h>

typedef struct _WIN_DC_INFO
{
  int  flags;
  HRGN  hClipRgn;     /* Clip region (may be 0) */
  HRGN  hVisRgn;      /* Visible region (must never be 0) */
  HRGN  hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
  HPEN  hPen;
  HBRUSH  hBrush;
  HFONT  hFont;
  HBITMAP  hBitmap;
  HBITMAP  hFirstBitmap; /* Bitmap selected at creation of the DC */

/* #if 0 */
    HANDLE      hDevice;
    HPALETTE    hPalette;

    GdiPath       path;
/* #endif */

  WORD  ROPmode;
  WORD  polyFillMode;
  WORD  stretchBltMode;
  WORD  relAbsMode;
  WORD  backgroundMode;
  COLORREF  backgroundColor;
  COLORREF  textColor;

  short  brushOrgX;
  short  brushOrgY;

  WORD  textAlign;         /* Text alignment from SetTextAlign() */
  short  charExtra;         /* Spacing from SetTextCharacterExtra() */
  short  breakTotalExtra;   /* Total extra space for justification */
  short  breakCount;        /* Break char. count */
  short  breakExtra;        /* breakTotalExtra / breakCount */
  short  breakRem;          /* breakTotalExtra % breakCount */

  RECT   totalExtent;
  BYTE   bitsPerPixel;

  INT  MapMode;
  INT  GraphicsMode;      /* Graphics mode */
  INT  DCOrgX;            /* DC origin */
  INT  DCOrgY;

#if 0
    FARPROC     lpfnPrint;         /* AbortProc for Printing */
#endif

  INT  CursPosX;          /* Current position */
  INT  CursPosY;
  INT  ArcDirection;

  XFORM  xformWorld2Wnd;    /* World-to-window transformation */
  XFORM  xformWorld2Vport;  /* World-to-viewport transformation */
  XFORM  xformVport2World;  /* Inverse of the above transformation */
  BOOL  vport2WorldValid;  /* Is xformVport2World valid? */
} WIN_DC_INFO;

  /* DC flags */
#define DC_MEMORY     0x0001   /* It is a memory DC */
#define DC_SAVED      0x0002   /* It is a saved DC */
#define DC_DIRTY      0x0004   /* hVisRgn has to be updated */
#define DC_THUNKHOOK  0x0008   /* DC hook is in the 16-bit code */

#define  GDI_DC_TYPE  (1)

typedef struct _DC
{
  HDC  hSelf;
  HDC  hNext;
  DHPDEV  PDev;
  DEVMODEW  DMW;
  HSURF  FillPatternSurfaces[HS_DDI_MAX];
  PGDIINFO  GDIInfo;
  PDEVINFO  DevInfo;
  HDEV   GDIDevice;

  DRIVER_FUNCTIONS  DriverFunctions;
  UNICODE_STRING    DriverName;
  HANDLE  DeviceDriver;

  INT  wndOrgX;          /* Window origin */
  INT  wndOrgY;
  INT  wndExtX;          /* Window extent */
  INT  wndExtY;
  INT  vportOrgX;        /* Viewport origin */
  INT  vportOrgY;
  INT  vportExtX;        /* Viewport extent */
  INT  vportExtY;

  CLIPOBJ *CombinedClip;

  INT  saveLevel;

  WIN_DC_INFO  w;
} DC, *PDC;

typedef struct
{
  HANDLE Handle;
  DHPDEV PDev;
  DEVMODEW DMW;
  HSURF FillPatterns[HS_DDI_MAX];
  GDIINFO GDIInfo;
  DEVINFO DevInfo;
  DRIVER_FUNCTIONS DriverFunctions;
  PFILE_OBJECT VideoFileObject;

  struct {
     BOOL Enable;
     LONG Column;
     LONG Row;
     LONG Width;
     LONG Height;
  } PointerAttributes;
  XLATEOBJ *PointerXlateObject;
  HSURF PointerColorSurface;
  HSURF PointerMaskSurface;
  HSURF PointerSaveSurface;
  POINTL PointerHotSpot;
  ULONG PointerStatus;
} GDIDEVICE;

/*  Internal functions  */

#define  DC_LockDc(hDC)  \
  ((PDC) GDIOBJ_LockObj ((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC))
#define  DC_UnlockDc(hDC)  \
  GDIOBJ_UnlockObj ((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC)

HDC  FASTCALL RetrieveDisplayHDC(VOID);
HDC  FASTCALL DC_AllocDC(PUNICODE_STRING  Driver);
VOID FASTCALL DC_InitDC(HDC  DCToInit);
HDC  FASTCALL DC_FindOpenDC(PUNICODE_STRING  Driver);
VOID FASTCALL DC_FreeDC(HDC  DCToFree);
HDC  FASTCALL DC_GetNextDC (PDC pDC);
VOID FASTCALL DC_SetNextDC (PDC pDC, HDC hNextDC);
BOOL FASTCALL DC_InternalDeleteDC( PDC DCToDelete );
VOID FASTCALL DC_SetOwnership(HDC DC, PEPROCESS Owner);

VOID FASTCALL DC_UpdateXforms(PDC  dc);
BOOL FASTCALL DC_InvertXform(const XFORM *xformSrc, XFORM *xformDest);

/*  User entry points */

BOOL STDCALL  NtGdiCancelDC(HDC  hDC);
HDC STDCALL  NtGdiCreateCompatableDC(HDC  hDC);
HDC STDCALL  NtGdiCreateDC(PUNICODE_STRING Driver,
                           PUNICODE_STRING Device,
                           PUNICODE_STRING Output,
                           CONST PDEVMODEW  InitData);
HDC STDCALL NtGdiCreateIC(PUNICODE_STRING Driver,
                          PUNICODE_STRING Device,
                          PUNICODE_STRING Output,
                          CONST PDEVMODEW  DevMode);
BOOL STDCALL  NtGdiDeleteDC(HDC  hDC);
BOOL STDCALL  NtGdiDeleteObject(HGDIOBJ hObject);
INT STDCALL  NtGdiDrawEscape(HDC  hDC,
                            INT  nEscape,
                            INT  cbInput,
                            LPCSTR  lpszInData);

#ifndef __USE_W32API
/* FIXME: this typedef should go somewhere else...  */
typedef VOID (*GOBJENUMPROC)(PVOID, LPARAM);
#endif

INT STDCALL  NtGdiEnumObjects(HDC  hDC,
                             INT  ObjectType,
                             GOBJENUMPROC  ObjectFunc,
                             LPARAM  lParam);

COLORREF STDCALL  NtGdiGetBkColor(HDC  hDC);
INT STDCALL  NtGdiGetBkMode(HDC  hDC);
BOOL STDCALL  NtGdiGetBrushOrgEx(HDC  hDC, LPPOINT brushOrg);
HRGN STDCALL  NtGdiGetClipRgn(HDC  hDC);
HGDIOBJ STDCALL  NtGdiGetCurrentObject(HDC  hDC, UINT  ObjectType);
VOID FASTCALL IntGetCurrentPositionEx (PDC  dc,  LPPOINT currentPosition);
BOOL STDCALL  NtGdiGetCurrentPositionEx(HDC  hDC, LPPOINT currentPosition);
BOOL STDCALL  NtGdiGetDCOrgEx(HDC  hDC, LPPOINT  Point);
HDC STDCALL  NtGdiGetDCState(HDC  hDC);
INT STDCALL  NtGdiGetDeviceCaps(HDC  hDC, INT  Index);
INT STDCALL  NtGdiGetMapMode(HDC  hDC);
INT STDCALL  NtGdiGetObject(HGDIOBJ  hGDIObj,
                           INT  BufSize,
                           LPVOID  Object);
DWORD STDCALL  NtGdiGetObjectType(HGDIOBJ  hGDIObj);
INT STDCALL  NtGdiGetPolyFillMode(HDC  hDC);
INT STDCALL  NtGdiGetRelAbs(HDC  hDC);
INT STDCALL  NtGdiGetROP2(HDC  hDC);
HGDIOBJ STDCALL  NtGdiGetStockObject(INT  Object);
INT STDCALL  NtGdiGetStretchBltMode(HDC  hDC);
COLORREF STDCALL  NtGdiGetTextColor(HDC  hDC);
UINT STDCALL  NtGdiGetTextAlign(HDC  hDC);
BOOL STDCALL  NtGdiGetViewportExtEx(HDC  hDC, LPSIZE viewportExt);
BOOL STDCALL  NtGdiGetViewportOrgEx(HDC  hDC, LPPOINT viewportOrg);
BOOL STDCALL  NtGdiGetWindowExtEx(HDC  hDC, LPSIZE windowExt);
BOOL STDCALL  NtGdiGetWindowOrgEx(HDC  hDC, LPPOINT windowOrg);
HDC STDCALL  NtGdiResetDC(HDC  hDC, CONST DEVMODEW  *InitData);
BOOL STDCALL  NtGdiRestoreDC(HDC  hDC, INT  SavedDC);
INT STDCALL  NtGdiSaveDC(HDC  hDC);
HGDIOBJ STDCALL  NtGdiSelectObject(HDC  hDC, HGDIOBJ  hGDIObj);
INT STDCALL  NtGdiSetBkMode(HDC  hDC, INT  backgroundMode);
VOID STDCALL NtGdiSetDCState ( HDC hDC, HDC hDCSave );
WORD STDCALL NtGdiSetHookFlags(HDC hDC, WORD Flags);
INT STDCALL  NtGdiSetPolyFillMode(HDC  hDC, INT polyFillMode);
INT STDCALL  NtGdiSetRelAbs(HDC  hDC, INT  relAbsMode);
INT STDCALL  NtGdiSetROP2(HDC  hDC, INT  ROPmode);
INT STDCALL  NtGdiSetStretchBltMode(HDC  hDC, INT  stretchBltMode);
COLORREF STDCALL  NtGdiSetTextColor(HDC hDC, COLORREF color);

#endif
