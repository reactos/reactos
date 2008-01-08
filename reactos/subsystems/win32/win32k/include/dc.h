
#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

#include "driver.h"

  /* DC flags */
#define DC_SAVED      0x0002   /* It is a saved DC */
#define DC_DIRTY      0x0004   /* hVisRgn has to be updated */

// GDIDEVICE flags
#define PDEV_DISPLAY             0x00000001 // Display device
#define PDEV_HARDWARE_POINTER    0x00000002 // Supports hardware cursor
#define PDEV_SOFTWARE_POINTER    0x00000004
#define PDEV_GOTFONTS            0x00000040 // Has font driver
#define PDEV_PRINTER             0x00000080
#define PDEV_ALLOCATEDBRUSHES    0x00000100
#define PDEV_HTPAL_IS_DEVPAL     0x00000200
#define PDEV_DISABLED            0x00000400
#define PDEV_SYNCHRONIZE_ENABLED 0x00000800
#define PDEV_FONTDRIVER          0x00002000 // Font device
#define PDEV_GAMMARAMP_TABLE     0x00004000
#define PDEV_UMPD                0x00008000
#define PDEV_SHARED_DEVLOCK      0x00010000
#define PDEV_META_DEVICE         0x00020000
#define PDEV_DRIVER_PUNTED_CALL  0x00040000 // Driver calls back to GDI engine
#define PDEV_CLONE_DEVICE        0x00080000

// Graphics Device structure.
typedef struct _GRAPHICS_DEVICE
{
  CHAR szNtDeviceName[CCHDEVICENAME];           // Yes char AscII
  CHAR szWinDeviceName[CCHDEVICENAME];          // <- chk GetMonitorInfoW MxIxEX.szDevice
  struct _GRAPHICS_DEVICE * pNextGraphicsDevice;
  DWORD StateFlags;                             // See DISPLAY_DEVICE_*
} GRAPHICS_DEVICE, *PGRAPHICS_DEVICE;

typedef struct _GDIPOINTER /* should stay private to ENG? No, part of GDIDEVICE aka HDEV aka PDEV. */
{
  /* private GDI pointer handling information, required for software emulation */
  BOOL     Enabled;
  POINTL   Pos;
  SIZEL    Size;
  POINTL   HotSpot;
  XLATEOBJ *XlateObject;
  HSURF    ColorSurface;
  HSURF    MaskSurface;
  HSURF    SaveSurface;
  int      ShowPointer; /* counter negtive  do not show the mouse postive show the mouse */

  /* public pointer information */
  RECTL    Exclude; /* required publicly for SPS_ACCEPT_EXCLUDE */
  PGD_MOVEPOINTER MovePointer;
  ULONG    Status;
} GDIPOINTER, *PGDIPOINTER;

typedef struct _GDIDEVICE
{
  HANDLE        hHmgr;
  ULONG         csCount;
  ULONG         lucExcLock;
  PVOID         Tid;

  struct _GDIDEVICE *ppdevNext;
  INT           cPdevRefs;
  INT           cPdevOpenRefs;
  struct _GDIDEVICE *ppdevParent;
  FLONG         flFlags;
  PERESOURCE    hsemDevLock;    // Device lock.

  PVOID         pvGammaRamp;    // Gamma ramp pointer.

  DHPDEV        hPDev;          // DHPDEV for device.

  HSURF         FillPatterns[HS_DDI_MAX];

  ULONG         DxDd_nCount;

  DEVINFO       DevInfo;
  GDIINFO       GDIInfo;
  HSURF         pSurface;       // SURFACE for this device.
  HANDLE        hSpooler;       // Handle to spooler, if spooler dev driver.
  ULONG         DisplayNumber;
  PVOID         pGraphicsDev;   // PGRAPHICS_DEVICE

  DEVMODEW      DMW;
  PVOID         pdmwDev;        // Ptr->DEVMODEW.dmSize + dmDriverExtra == alloc size.

  FLONG         DxDd_Flags;     // DxDD active status flags.

  PFILE_OBJECT  VideoFileObject;
  BOOLEAN       PreparedDriver;
  GDIPOINTER    Pointer;
  /* Stuff to keep track of software cursors; win32k gdi part */
  UINT SafetyRemoveLevel; /* at what level was the cursor removed?
			     0 for not removed */
  UINT SafetyRemoveCount;

  DRIVER_FUNCTIONS DriverFunctions;
  struct _EDD_DIRECTDRAW_GLOBAL * pEDDgpl;
} GDIDEVICE, *PGDIDEVICE;

/*  Internal functions  */

#define  DC_LockDc(hDC)  \
  ((PDC) GDIOBJ_LockObj (GdiHandleTable, (HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC))
#define  DC_UnlockDc(pDC)  \
  GDIOBJ_UnlockObjByPtr (GdiHandleTable, pDC)

NTSTATUS FASTCALL InitDcImpl(VOID);
HDC  FASTCALL RetrieveDisplayHDC(VOID);
PGDIDEVICE FASTCALL IntEnumHDev(VOID);
HDC  FASTCALL DC_AllocDC(PUNICODE_STRING  Driver);
VOID FASTCALL DC_InitDC(HDC  DCToInit);
HDC  FASTCALL DC_FindOpenDC(PUNICODE_STRING  Driver);
VOID FASTCALL DC_FreeDC(HDC);
VOID FASTCALL DC_AllocateDcAttr(HDC);
VOID FASTCALL DC_FreeDcAttr(HDC);
BOOL INTERNAL_CALL DC_Cleanup(PVOID ObjectBody);
HDC  FASTCALL DC_GetNextDC (PDC pDC);
VOID FASTCALL DC_SetNextDC (PDC pDC, HDC hNextDC);
VOID FASTCALL DC_SetOwnership(HDC DC, PEPROCESS Owner);
VOID FASTCALL DC_LockDisplay(HDC);
VOID FASTCALL DC_UnlockDisplay(HDC);
VOID FASTCALL IntGdiCopyFromSaveState(PDC, PDC, HDC);
VOID FASTCALL IntGdiCopyToSaveState(PDC, PDC);
BOOL FASTCALL IntGdiDeleteDC(HDC, BOOL);

VOID FASTCALL DC_UpdateXforms(PDC  dc);
BOOL FASTCALL DC_InvertXform(const XFORM *xformSrc, XFORM *xformDest);

BOOL FASTCALL DCU_SyncDcAttrtoUser(PDC);
BOOL FASTCALL DCU_SynchDcAttrtoUser(HDC);
VOID FASTCALL DCU_SetDcUndeletable(HDC);

VOID FASTCALL IntGetViewportExtEx(PDC dc, LPSIZE pt);
VOID FASTCALL IntGetViewportOrgEx(PDC dc, LPPOINT pt);
VOID FASTCALL IntGetWindowExtEx(PDC dc, LPSIZE pt);
VOID FASTCALL IntGetWindowOrgEx(PDC dc, LPPOINT pt);

NTSTATUS STDCALL NtGdiFlushUserBatch(VOID);

COLORREF FASTCALL NtGdiSetBkColor (HDC hDC, COLORREF Color);
INT FASTCALL NtGdiSetBkMode(HDC  hDC, INT  backgroundMode);
COLORREF STDCALL  NtGdiGetBkColor(HDC  hDC);
INT STDCALL  NtGdiGetBkMode(HDC  hDC);
COLORREF FASTCALL  NtGdiSetTextColor(HDC hDC, COLORREF color);
UINT FASTCALL NtGdiSetTextAlign(HDC  hDC, UINT  Mode);
UINT STDCALL  NtGdiGetTextAlign(HDC  hDC);
COLORREF STDCALL  NtGdiGetTextColor(HDC  hDC);
INT STDCALL  NtGdiSetStretchBltMode(HDC  hDC, INT  stretchBltMode);

/* For Metafile and MetaEnhFile not in windows this struct taken from wine cvs 15/9-2006*/
typedef struct
{
  LPENHMETAHEADER  emh;
  BOOL    on_disk;   /* true if metafile is on disk */
} DD_ENHMETAFILEOBJ, *PDD_ENHMETAFILEOBJ;

#endif /* __WIN32K_DC_H */
