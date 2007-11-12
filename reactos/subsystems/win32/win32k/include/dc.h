
#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

#include "driver.h"

  /* DC flags */
#define DC_MEMORY     0x0001   /* It is a memory DC */
#define DC_SAVED      0x0002   /* It is a saved DC */
#define DC_DIRTY      0x0004   /* hVisRgn has to be updated */
#define DC_THUNKHOOK  0x0008   /* DC hook is in the 16-bit code */

#define  GDI_DC_TYPE  (1)



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
  PVOID  pvEntry;
  ULONG  lucExcLock;
  ULONG  Tid;

  PERESOURCE hsemDevLock;

  PVOID  pfnSync;

  DHPDEV PDev;
  DEVMODEW DMW;
  HSURF FillPatterns[HS_DDI_MAX];
  DEVINFO DevInfo;
  GDIINFO GDIInfo;

  HANDLE hSpooler;
  ULONG DisplayNumber;

  PFILE_OBJECT VideoFileObject;
  BOOLEAN PreparedDriver;
  GDIPOINTER Pointer;

  /* Stuff to keep track of software cursors; win32k gdi part */
  UINT SafetyRemoveLevel; /* at what level was the cursor removed?
			     0 for not removed */
  UINT SafetyRemoveCount;

  struct _EDD_DIRECTDRAW_GLOBAL * pEDDgpl;

  DRIVER_FUNCTIONS DriverFunctions;
} GDIDEVICE, *PGDIDEVICE;

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
VOID FASTCALL DC_LockDisplay(PERESOURCE);
VOID FASTCALL DC_UnlockDisplay(PERESOURCE);
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
