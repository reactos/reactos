#ifndef __WIN32K_PDEVOBJ_H
#define __WIN32K_PDEVOBJ_H

/* PDEVOBJ flags */
enum _PDEVFLAGS
{
    PDEV_DISPLAY             = 0x00000001, /* Display device */
    PDEV_HARDWARE_POINTER    = 0x00000002, /* Supports hardware cursor */
    PDEV_SOFTWARE_POINTER    = 0x00000004,
    PDEV_GOTFONTS            = 0x00000040, /* Has font driver */
    PDEV_PRINTER             = 0x00000080,
    PDEV_ALLOCATEDBRUSHES    = 0x00000100,
    PDEV_HTPAL_IS_DEVPAL     = 0x00000200,
    PDEV_DISABLED            = 0x00000400,
    PDEV_SYNCHRONIZE_ENABLED = 0x00000800,
    PDEV_FONTDRIVER          = 0x00002000, /* Font device */
    PDEV_GAMMARAMP_TABLE     = 0x00004000,
    PDEV_UMPD                = 0x00008000,
    PDEV_SHARED_DEVLOCK      = 0x00010000,
    PDEV_META_DEVICE         = 0x00020000,
    PDEV_DRIVER_PUNTED_CALL  = 0x00040000, /* Driver calls back to GDI engine */
    PDEV_CLONE_DEVICE        = 0x00080000
};

/* Type definitions ***********************************************************/

typedef struct _GDIPOINTER /* should stay private to ENG? No, part of PDEVOBJ aka HDEV aka PDEV. */
{
  /* Private GDI pointer handling information, required for software emulation */
  BOOL     Enabled;
  SIZEL    Size;
  POINTL   HotSpot;
  SURFACE  *psurfColor;
  SURFACE  *psurfMask;
  SURFACE  *psurfSave;
  FLONG    flags;

  /* Public pointer information */
  RECTL    Exclude; /* Required publicly for SPS_ACCEPT_EXCLUDE */
} GDIPOINTER, *PGDIPOINTER;

typedef struct _DEVMODEINFO
{
    struct _DEVMODEINFO *pdmiNext;
    struct _LDEVOBJ *pldev;
    ULONG cbdevmode;
    DEVMODEW adevmode[1];
} DEVMODEINFO, *PDEVMODEINFO;

typedef struct _DEVMODEENTRY
{
    DWORD dwFlags;
    PDEVMODEW pdm;

} DEVMODEENTRY, *PDEVMODEENTRY;

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
    PDEVMODEINFO     pdevmodeInfo;
    ULONG            cDevModes;
    PDEVMODEENTRY    pDevModeList;
    LPWSTR           pDiplayDrivers;
    LPWSTR           pwszDescription;
    DWORD            dwUnknown;
    PVOID            pUnknown;
    PFILE_OBJECT     FileObject;
    DWORD            ProtocolType;
    ULONG            iDefaultMode;
    ULONG            iCurrentMode;
} GRAPHICS_DEVICE, *PGRAPHICS_DEVICE;

typedef struct _PDEVOBJ
{
    BASEOBJECT                BaseObject;

    struct _PDEVOBJ *         ppdevNext;
    LONG                      cPdevRefs;
    LONG                      cPdevOpenRefs;
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
    struct _LDEVOBJ *         pldev;
    DHPDEV                    dhpdev;         /* DHPDEV for device. */
    struct _PALETTE*          ppalSurf;       /* PEPALOBJ/PPALETTE for this device. */
    DEVINFO                   devinfo;
    GDIINFO                   gdiinfo;
    PSURFACE                  pSurface;       /* SURFACE for this device. */
    HANDLE                    hSpooler;       /* Handle to spooler, if spooler dev driver, DeviceObject if graphics device */
//  PVOID                     pDesktopId;
    PGRAPHICS_DEVICE          pGraphicsDevice;
    POINTL                    ptlOrigion;
    PDEVMODEW                 pdmwDev;        /* Ptr->DEVMODEW.dmSize + dmDriverExtra == alloc size. */
//  DWORD                     Unknown3;
    FLONG                     DxDd_Flags;     /* DxDD active status flags. */
//  LONG                      devAttr;
//  PVOID                     WatchDogContext;
//  ULONG                     WatchDogs;
    union
    {
      DRIVER_FUNCTIONS        DriverFunctions;
      DRIVER_FUNCTIONS        pfn;
      PVOID                   apfn[INDEX_LAST];         // B8C 0x0598
    };

    /* ros specific */
    ULONG         DxDd_nCount;
    GDIPOINTER    Pointer;
    /* Stuff to keep track of software cursors; win32k gdi part */
    UINT SafetyRemoveLevel; /* at what level was the cursor removed?
                              0 for not removed */
    UINT SafetyRemoveCount;
    struct _EDD_DIRECTDRAW_GLOBAL * pEDDgpl;
} PDEVOBJ, *PPDEVOBJ;

/* Globals ********************************************************************/

extern PPDEVOBJ gppdevPrimary;


/* Function prototypes ********************************************************/

PPDEVOBJ
NTAPI
EngpGetPDEV(
    _In_opt_ PUNICODE_STRING pustrDevice);

FORCEINLINE
VOID
PDEVOBJ_vReference(
    _In_ PPDEVOBJ ppdev)
{
    ASSERT(ppdev);

    /* Fail if the PDEV is being destroyed */
    if (ppdev->cPdevRefs == 0)
    {
        ASSERT(FALSE);
        return;
    }
    ASSERT(ppdev->cPdevRefs > 0);

    InterlockedIncrement(&ppdev->cPdevRefs);
}

VOID
NTAPI
PDEVOBJ_vRelease(
    _Inout_ PPDEVOBJ ppdev);

PSURFACE
NTAPI
PDEVOBJ_pSurface(
    _In_ PPDEVOBJ ppdev);

VOID
NTAPI
PDEVOBJ_vGetDeviceCaps(
    _In_ PPDEVOBJ ppdev,
    _Out_ PDEVCAPS pDevCaps);

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitPDEVImpl(VOID);

PSIZEL
FASTCALL
PDEVOBJ_sizl(
    _In_ PPDEVOBJ ppdev,
    _Out_ PSIZEL psizl);

BOOL
NTAPI
PDEVOBJ_bSwitchMode(
    PPDEVOBJ ppdev,
    PDEVMODEW pdm);

#endif /* !__WIN32K_PDEVOBJ_H */
