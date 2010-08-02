#pragma once

#include <drivers/directx/directxint.h>

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
  SURFACE  *psurfColor;
  SURFACE  *psurfMask;
  SURFACE  *psurfSave;

  /* public pointer information */
  RECTL    Exclude; /* required publicly for SPS_ACCEPT_EXCLUDE */
} GDIPOINTER, *PGDIPOINTER;

typedef struct _GRAPHICS_DEVICE
{
    WCHAR            szNtDeviceName[CCHDEVICENAME/2];
    WCHAR            szWinDeviceName[CCHDEVICENAME/2];
    struct _GRAPHICS_DEVICE * pNextGraphicsDevice;
    struct _GRAPHICS_DEVICE * pVgaDevice;
    PDEVICE_OBJECT   DeviceObject;
    PVOID            pDeviceHandle;
    DWORD            hkClassDriverConfig;
    DWORD            StateFlags;                     /* See DISPLAY_DEVICE_* */
    ULONG            cbdevmodeInfo;
    PVOID            devmodeInfo;
    DWORD            cbdevmodeInfo1;
    PVOID            devmodeInfo1;
    LPWSTR           pwszDeviceNames;
    LPWSTR           pwszDescription;
    DWORD            dwUnknown;
    PVOID            pUnknown;
    PFILE_OBJECT     FileObject;
    DWORD            ProtocolType;
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
    HSEMAPHORE                hsemDevLock;    /* Device lock. */
//  HSEMAPHORE                hsemPointer;
    POINTL                    ptlPointer;
//  SIZEL                     szlPointer;
//  SPRITESTATE               SpriteState;
//  HFONT                     hlfntDefault;
//  HFONT                     hlfntAnsiVariable;
//  HFONT                     hlfntAnsiFixed;
    HSURF                     ahsurf[HS_DDI_MAX];
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
    ULONG                     ulHorzRes;
    ULONG                     ulVertRes;
//  PFN_DrvSetPointerShape    pfnDrvSetPointerShape;
//  PFN_DrvMovePointer        pfnDrvMovePointer;
    PFN_DrvMovePointer        pfnMovePointer;
//  PFN_DrvSynchronize        pfnDrvSynchronize;
//  PFN_DrvSynchronizeSurface pfnDrvSynchronizeSurface;
//  PFN_DrvSetPalette         pfnDrvSetPalette;
//  PFN_DrvNotify             pfnDrvNotify;
//  ULONG                     TagSig;
//  PLDEVOBJ                  pldev;
    DHPDEV                    dhpdev;         /* DHPDEV for device. */
    PVOID                     ppalSurf;       /* PEPALOBJ/PPALETTE for this device. */
    DEVINFO                   devinfo;
    GDIINFO                   gdiinfo;
    HSURF                     pSurface;       /* SURFACE for this device., FIXME: PSURFACE */
//  HANDLE                    hSpooler;       /* Handle to spooler, if spooler dev driver. */
//  PVOID                     pDesktopId;
    PGRAPHICS_DEVICE          pGraphicsDevice;
    POINTL                    ptlOrigion;
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

/* PDEV and EDDX extra data container.*/
typedef struct _PDEVEDD
{
    PDEVOBJ pdevobj;
    EDD_DIRECTDRAW_GLOBAL EDDgpl;
} PDEVEDD, *PPDEVEDD;

PSIZEL FASTCALL PDEV_sizl(PPDEVOBJ, PSIZEL);

extern ULONG gdwDirectDrawContext;
