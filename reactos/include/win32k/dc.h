
#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

#include <windows.h>
#include <win32k/driver.h>
#include <win32k/gdiobj.h>

/*  (RJJ) Taken from WINE  */
typedef struct _WIN_DC_INFO
{
  int  flags;
#if 0
    const DeviceCaps *devCaps;
#endif

  HRGN  hClipRgn;     /* Clip region (may be 0) */
  
#if 0
    HRGN        hVisRgn;      /* Visible region (must never be 0) */
    HRGN        hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
#endif
  
  HPEN  hPen;
  HBRUSH  hBrush;
  HFONT  hFont;
  HBITMAP  hBitmap;
  HBITMAP  hFirstBitmap; /* Bitmap selected at creation of the DC */

#if 0
    HANDLE      hDevice;
    HPALETTE    hPalette;
    
    GdiPath       path;
#endif

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
  
#if 0
    XFORM         xformWorld2Wnd;    /* World-to-window transformation */
    XFORM         xformWorld2Vport;  /* World-to-viewport transformation */
    XFORM         xformVport2World;  /* Inverse of the above transformation */
    BOOL        vport2WorldValid;  /* Is xformVport2World valid? */  
#endif
} WIN_DC_INFO;

  /* DC flags */
#define DC_MEMORY     0x0001   /* It is a memory DC */
#define DC_SAVED      0x0002   /* It is a saved DC */
#define DC_DIRTY      0x0004   /* hVisRgn has to be updated */
#define DC_THUNKHOOK  0x0008   /* DC hook is in the 16-bit code */ 

#define  GDI_DC_TYPE  (1)

typedef struct _DC
{
  GDIOBJHDR  header;
  HDC  hSelf;
  DHPDEV  PDev;  
  DEVMODEW  DMW;
  HSURF  FillPatternSurfaces[HS_DDI_MAX];
  GDIINFO  GDIInfo;
  DEVINFO  DevInfo;
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

  INT  saveLevel;

  WIN_DC_INFO  w;
} DC, *PDC;

/*  Internal functions  */

PDC  DC_AllocDC(LPCWSTR  Driver);
void  DC_InitDC(PDC  DCToInit);
PDC  DC_FindOpenDC(LPCWSTR  Driver);
void  DC_FreeDC(PDC  DCToFree);
HDC  DC_PtrToHandle(PDC  pDC);
PDC  DC_HandleToPtr(HDC  hDC);
BOOL DC_LockDC(HDC  hDC);
BOOL DC_UnlockDC(HDC  hDC);

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
HDC STDCALL  W32kResetDC(HDC  hDC, CONST DEVMODE  *InitData);
BOOL STDCALL  W32kRestoreDC(HDC  hDC, INT  SavedDC);
INT STDCALL  W32kSaveDC(HDC  hDC);
HGDIOBJ STDCALL  W32kSelectObject(HDC  hDC, HGDIOBJ  GDIObj);
INT STDCALL  W32kSetBkMode(HDC  hDC, INT  backgroundMode);
INT STDCALL  W32kSetPolyFillMode(HDC  hDC, INT polyFillMode);
INT STDCALL  W32kSetRelAbs(HDC  hDC, INT  relAbsMode);
INT STDCALL  W32kSetROP2(HDC  hDC, INT  ROPmode);
INT STDCALL  W32kSetStretchBltMode(HDC  hDC, INT  stretchBltMode);
COLORREF STDCALL  W32kSetTextColor(HDC hDC, COLORREF color);

#endif

