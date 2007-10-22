
#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

#include "driver.h"

typedef enum tagGdiPathState
{
   PATH_Null,
   PATH_Open,
   PATH_Closed
} GdiPathState;

typedef struct tagGdiPath
{
   GdiPathState state;
   POINT      *pPoints;
   BYTE         *pFlags;
   int          numEntriesUsed, numEntriesAllocated;
   BOOL       newStroke;
} GdiPath;

typedef struct _WIN_DC_INFO
{
  int  flags;
  HRGN  hClipRgn;     /* Clip region (may be 0) */
  HRGN  hVisRgn;      /* Visible region (must never be 0) */
  HRGN  hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
  HBITMAP  hBitmap;
  HBITMAP  hFirstBitmap; /* Bitmap selected at creation of the DC */

/* #if 0 */
    HANDLE      hDevice;
    HPALETTE    hPalette;

    GdiPath       path;
/* #endif */

  RECT   totalExtent;
  BYTE   bitsPerPixel;

  INT  DCOrgX;            /* DC origin */
  INT  DCOrgY;
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
  HGDIOBJ     hHmgr;
  PVOID       pvEntry;
  ULONG       lucExcLock;
  ULONG       Tid;

  DHPDEV      PDev;
  INT         DC_Type;
  INT         DC_Flags;
  PDC_ATTR    pDc_Attr;
  DC_ATTR     Dc_Attr;

  HDC         hSelf;
  HDC         hNext;
  HSURF       FillPatternSurfaces[HS_DDI_MAX];
  PGDIINFO    GDIInfo;
  PDEVINFO    DevInfo;
  HDEV        GDIDevice;

  DRIVER_FUNCTIONS  DriverFunctions;
  UNICODE_STRING    DriverName;
  HANDLE      DeviceDriver;

  CLIPOBJ     *CombinedClip;

  XLATEOBJ    *XlateBrush;
  XLATEOBJ    *XlatePen;

  INT         saveLevel;
  BOOL        IsIC;

  HPALETTE    PalIndexed;

  WIN_DC_INFO w;
  
  HANDLE      hFile;  
  LPENHMETAHEADER emh;
} DC, *PDC;

typedef struct _GDIPOINTER /* should stay private to ENG */
{
  /* private GDI pointer handling information, required for software emulation */
  BOOL Enabled;
  POINTL Pos;
  SIZEL Size;
  POINTL HotSpot;
  XLATEOBJ *XlateObject;
  HSURF ColorSurface;
  HSURF MaskSurface;
  HSURF SaveSurface;
  int  ShowPointer; /* counter negtive  do not show the mouse postive show the mouse */
  
  /* public pointer information */
  RECTL Exclude; /* required publicly for SPS_ACCEPT_EXCLUDE */
  PGD_MOVEPOINTER MovePointer;
  ULONG Status;
} GDIPOINTER, *PGDIPOINTER;

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
  BOOLEAN PreparedDriver;
  ULONG DisplayNumber;

  GDIPOINTER Pointer;

  /* Stuff to keep track of software cursors; win32k gdi part */
  UINT SafetyRemoveLevel; /* at what level was the cursor removed?
			     0 for not removed */
  UINT SafetyRemoveCount;
} GDIDEVICE;

/*  Internal functions  */

#define  DC_LockDc(hDC)  \
  ((PDC) GDIOBJ_LockObj (GdiHandleTable, (HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC))
#define  DC_UnlockDc(pDC)  \
  GDIOBJ_UnlockObjByPtr (GdiHandleTable, pDC)

NTSTATUS FASTCALL InitDcImpl(VOID);
HDC  FASTCALL RetrieveDisplayHDC(VOID);
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
VOID FASTCALL IntGdiCopyFromSaveState(PDC, PDC, HDC);
VOID FASTCALL IntGdiCopyToSaveState(PDC, PDC);

VOID FASTCALL DC_UpdateXforms(PDC  dc);
BOOL FASTCALL DC_InvertXform(const XFORM *xformSrc, XFORM *xformDest);

BOOL FASTCALL DCU_UpdateUserXForms(PDC, ULONG);
BOOL FASTCALL DCU_SyncDcAttrtoUser(PDC, FLONG);
BOOL FASTCALL DCU_SynchDcAttrtoUser(HDC, FLONG);
BOOL FASTCALL DCU_SyncDcAttrtoW32k(PDC, FLONG);
BOOL FASTCALL DCU_SynchDcAttrtoW32k(HDC, FLONG);

VOID FASTCALL IntGetViewportExtEx(PDC dc, LPSIZE pt);
VOID FASTCALL IntGetViewportOrgEx(PDC dc, LPPOINT pt);
VOID FASTCALL IntGetWindowExtEx(PDC dc, LPSIZE pt);
VOID FASTCALL IntGetWindowOrgEx(PDC dc, LPPOINT pt);

NTSTATUS STDCALL NtGdiFlushUserBatch(VOID);

/* For Metafile and MetaEnhFile not in windows this struct taken from wine cvs 15/9-2006*/
typedef struct
{
  LPENHMETAHEADER  emh;
  BOOL    on_disk;   /* true if metafile is on disk */
} DD_ENHMETAFILEOBJ, *PDD_ENHMETAFILEOBJ;

#endif /* __WIN32K_DC_H */
