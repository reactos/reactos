
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

// #if 0
    HANDLE      hDevice;
    HPALETTE    hPalette;

    GdiPath       path;
// #endif

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
  HSURF  Surface;

  DRIVER_FUNCTIONS  DriverFunctions;
  PWSTR  DriverName;
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
  HANDLE DisplayDevice;
} GDIDEVICE;

/*  Internal functions  */

/*
#define  DC_PtrToHandle(pDC)  \
  ((HDC) GDIOBJ_PtrToHandle ((PGDIOBJ) pDC, GO_DC_MAGIC))
*/

#define  DC_HandleToPtr(hDC)  \
  ((PDC) GDIOBJ_LockObj ((HGDIOBJ) hDC, GO_DC_MAGIC))
#define  DC_ReleasePtr(hDC)  \
  GDIOBJ_UnlockObj ((HGDIOBJ) hDC, GO_DC_MAGIC)

HDC  FASTCALL RetrieveDisplayHDC(VOID);
HDC  FASTCALL DC_AllocDC(LPCWSTR  Driver);
VOID FASTCALL DC_InitDC(HDC  DCToInit);
HDC  FASTCALL DC_FindOpenDC(LPCWSTR  Driver);
VOID FASTCALL DC_FreeDC(HDC  DCToFree);
HDC  FASTCALL DC_GetNextDC (PDC pDC);
VOID FASTCALL DC_SetNextDC (PDC pDC, HDC hNextDC);
BOOL FASTCALL DC_InternalDeleteDC( PDC DCToDelete );

VOID FASTCALL DC_UpdateXforms(PDC  dc);
BOOL FASTCALL DC_InvertXform(const XFORM *xformSrc, XFORM *xformDest);

/*  User entry points */

BOOL STDCALL  W32kCancelDC(HDC  hDC);
HDC STDCALL  W32kCreateCompatableDC(HDC  hDC);
HDC STDCALL  W32kCreateDC(LPCWSTR  Driver,
                          LPCWSTR  Device,
                          LPCWSTR  Output,
                          CONST PDEVMODEW  InitData);
HDC STDCALL W32kCreateIC(LPCWSTR  Driver,
                         LPCWSTR  Device,
                         LPCWSTR  Output,
                         CONST PDEVMODEW  DevMode);
BOOL STDCALL  W32kDeleteDC(HDC  hDC);
BOOL STDCALL  W32kDeleteObject(HGDIOBJ hObject);
INT STDCALL  W32kDrawEscape(HDC  hDC,
                            INT  nEscape,
                            INT  cbInput,
                            LPCSTR  lpszInData);

/* FIXME: this typedef should go somewhere else...  */
typedef VOID (*GOBJENUMPROC)(PVOID, LPARAM);

INT STDCALL  W32kEnumObjects(HDC  hDC,
                             INT  ObjectType,
                             GOBJENUMPROC  ObjectFunc,
                             LPARAM  lParam);

COLORREF STDCALL  W32kGetBkColor(HDC  hDC);
INT STDCALL  W32kGetBkMode(HDC  hDC);
BOOL STDCALL  W32kGetBrushOrgEx(HDC  hDC, LPPOINT brushOrg);
HRGN STDCALL  W32kGetClipRgn(HDC  hDC);
HGDIOBJ STDCALL  W32kGetCurrentObject(HDC  hDC, UINT  ObjectType);
BOOL STDCALL  W32kGetCurrentPositionEx(HDC  hDC, LPPOINT currentPosition);
BOOL STDCALL  W32kGetDCOrgEx(HDC  hDC, LPPOINT  Point);
HDC STDCALL  W32kGetDCState16(HDC  hDC);
INT STDCALL  W32kGetDeviceCaps(HDC  hDC, INT  Index);
INT STDCALL  W32kGetMapMode(HDC  hDC);
INT STDCALL  W32kGetObject(HGDIOBJ  hGDIObj,
                           INT  BufSize,
                           LPVOID  Object);
DWORD STDCALL  W32kGetObjectType(HGDIOBJ  hGDIObj);
INT STDCALL  W32kGetPolyFillMode(HDC  hDC);
INT STDCALL  W32kGetRelAbs(HDC  hDC);
INT STDCALL  W32kGetROP2(HDC  hDC);
HGDIOBJ STDCALL  W32kGetStockObject(INT  Object);
INT STDCALL  W32kGetStretchBltMode(HDC  hDC);
COLORREF STDCALL  W32kGetTextColor(HDC  hDC);
UINT STDCALL  W32kGetTextAlign(HDC  hDC);
BOOL STDCALL  W32kGetViewportExtEx(HDC  hDC, LPSIZE viewportExt);
BOOL STDCALL  W32kGetViewportOrgEx(HDC  hDC, LPPOINT viewportOrg);
BOOL STDCALL  W32kGetWindowExtEx(HDC  hDC, LPSIZE windowExt);
BOOL STDCALL  W32kGetWindowOrgEx(HDC  hDC, LPPOINT windowOrg);
HDC STDCALL  W32kResetDC(HDC  hDC, CONST DEVMODEW  *InitData);
BOOL STDCALL  W32kRestoreDC(HDC  hDC, INT  SavedDC);
INT STDCALL  W32kSaveDC(HDC  hDC);
HGDIOBJ STDCALL  W32kSelectObject(HDC  hDC, HGDIOBJ  hGDIObj);
INT STDCALL  W32kSetBkMode(HDC  hDC, INT  backgroundMode);
INT STDCALL  W32kSetPolyFillMode(HDC  hDC, INT polyFillMode);
INT STDCALL  W32kSetRelAbs(HDC  hDC, INT  relAbsMode);
INT STDCALL  W32kSetROP2(HDC  hDC, INT  ROPmode);
INT STDCALL  W32kSetStretchBltMode(HDC  hDC, INT  stretchBltMode);
COLORREF STDCALL  W32kSetTextColor(HDC hDC, COLORREF color);

#endif
