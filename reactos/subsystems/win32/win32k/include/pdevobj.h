#ifndef __WIN32K_PDEVOBJ_H
#define __WIN32K_PDEVOBJ_H

/* PDEVOBJ flags */
#define PDEV_DISPLAY             0x00000001 /* Display device */
#define PDEV_HARDWARE_POINTER    0x00000002 /* Supports hardware cursor */
#define PDEV_SOFTWARE_POINTER    0x00000004
#define PDEV_GOTFONTS            0x00000040 /* Has font driver */
#define PDEV_PRINTER             0x00000080
#define PDEV_ALLOCATEDBRUSHES    0x00000100
#define PDEV_HTPAL_IS_DEVPAL     0x00000200
#define PDEV_DISABLED            0x00000400
#define PDEV_SYNCHRONIZE_ENABLED 0x00000800
#define PDEV_FONTDRIVER          0x00002000 /* Font device */
#define PDEV_GAMMARAMP_TABLE     0x00004000
#define PDEV_UMPD                0x00008000
#define PDEV_SHARED_DEVLOCK      0x00010000
#define PDEV_META_DEVICE         0x00020000
#define PDEV_DRIVER_PUNTED_CALL  0x00040000 /* Driver calls back to GDI engine */
#define PDEV_CLONE_DEVICE        0x00080000

/* Type definitions ***********************************************************/

typedef struct _GDIPOINTER /* should stay private to ENG? No, part of PDEVOBJ aka HDEV aka PDEV. */
{
  /* private GDI pointer handling information, required for software emulation */
  BOOL     Enabled;
  SIZEL    Size;
  POINTL   HotSpot;
  XLATEOBJ *XlateObject;
  HSURF    ColorSurface;
  HSURF    MaskSurface;
  HSURF    SaveSurface;
  int      ShowPointer; /* counter negtive  do not show the mouse postive show the mouse */

  /* public pointer information */
  RECTL    Exclude; /* required publicly for SPS_ACCEPT_EXCLUDE */
  PFN_DrvMovePointer MovePointer;
  ULONG    Status;
} GDIPOINTER, *PGDIPOINTER;

typedef struct _GRAPHICS_DEVICE
{
  CHAR szNtDeviceName[CCHDEVICENAME];           /* Yes char AscII */
  CHAR szWinDeviceName[CCHDEVICENAME];          /* <- chk GetMonitorInfoW MxIxEX.szDevice */
  struct _GRAPHICS_DEVICE * pNextGraphicsDevice;
  DWORD StateFlags;                             /* See DISPLAY_DEVICE_* */
} GRAPHICS_DEVICE, *PGRAPHICS_DEVICE;

typedef struct _PDEVOBJ
{
    BASEOBJECT                BaseObject;

    struct _PDEVOBJ *         ppdevNext;
    INT                       cPdevRefs;
    INT                       cPdevOpenRefs;
    struct _PDEVOBJ *         ppdevParent;
    FLONG                     flFlags;  // flags
//  FLONG                     flAccelerated;
    PERESOURCE                hsemDevLock;    /* Device lock. */
//  HSEMAPHORE                hsemPointer;
//  POINTL                    ptlPointer;
//  SIZEL                     szlPointer;
//  SPRITESTATE               SpriteState;
//  HFONT                     hlfntDefault;
//  HFONT                     hlfntAnsiVariable;
//  HFONT                     hlfntAnsiFixed;
    HSURF                     FillPatterns[HS_DDI_MAX]; // ahsurf[HS_DDI_MAX];
//  PUNICODE_STRING           pusPrtDataFileName;
//  PVOID                     pDevHTInfo;
//  RFONT *                   prfntActive;
//  RFONT *                   prfntInactive;
//  ULONG                     cInactive;
//  BYTE                      ajbo[0x5C];
//  ULONG                     cDirectDrawDisableLocks;
//  PVOID                     TypeOneInfo;
    PVOID                     pvGammaRamp;    /* Gamma ramp pointer. */
//  PVOID                     RemoteTypeOne;
//  ULONG                     ulHorzRes;
//  ULONG                     ulVertRes;
//  PFN_DrvSetPointerShape    pfnDrvSetPointerShape;
//  PFN_DrvMovePointer        pfnDrvMovePointer;
//  PFN_DrvMovePointer        pfnMovePointer;
//  PFN_DrvSynchronize        pfnDrvSynchronize;
//  PFN_DrvSynchronizeSurface pfnDrvSynchronizeSurface;
//  PFN_DrvSetPalette         pfnDrvSetPalette;
//  PFN_DrvNotify             pfnDrvNotify;
//  ULONG                     TagSig;
//  PLDEVOBJ                  pldev;
    DHPDEV                    hPDev;          /* dhpdev, DHPDEV for device. */
    PVOID                     ppalSurf;       /* PEPALOBJ/PPALGDI for this device. */
    DEVINFO                   DevInfo; // devinfo
    GDIINFO                   GDIInfo; // gdiinfo
    HSURF                     pSurface;       /* SURFACE for this device., FIXME: PSURFACE */
//  HANDLE                    hSpooler;       /* Handle to spooler, if spooler dev driver. */
//  PVOID                     pDesktopId;
    PGRAPHICS_DEVICE          pGraphicsDev;   /* pGraphicsDevice */
//  POINTL                    ptlOrigion;
    PVOID                     pdmwDev;        /* Ptr->DEVMODEW.dmSize + dmDriverExtra == alloc size. */
//  DWORD                     Unknown3;
    FLONG                     DxDd_Flags;     /* DxDD active status flags. */
//  LONG                      devAttr;
//  PVOID                     WatchDogContext;
//  ULONG                     WatchDogs;
    union
    {
      DRIVER_FUNCTIONS        DriverFunctions;
      PVOID                   apfn[INDEX_LAST];         // B8C 0x0598
    };

    /* ros specific */
    ULONG         DxDd_nCount;
    ULONG         DisplayNumber;
    DEVMODEW      DMW;
    PFILE_OBJECT  VideoFileObject;
    BOOLEAN       PreparedDriver;
    GDIPOINTER    Pointer;
    /* Stuff to keep track of software cursors; win32k gdi part */
    UINT SafetyRemoveLevel; /* at what level was the cursor removed?
                              0 for not removed */
    UINT SafetyRemoveCount;
    struct _EDD_DIRECTDRAW_GLOBAL * pEDDgpl;
} PDEVOBJ, *PPDEVOBJ;

#endif /* !__WIN32K_PDEVOBJ_H */
