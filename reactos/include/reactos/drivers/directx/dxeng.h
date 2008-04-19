
/************************************************************************/
/* This driver interface are exported from win32k.sys to dxg.sys        */
/************************************************************************/
#define DXENG_INDEX_Resverd0                            0x00
#define DXENG_INDEX_DxEngNUIsTermSrv                    0x01
#define DXENG_INDEX_DxEngScreenAccessCheck              0x02
#define DXENG_INDEX_DxEngRedrawDesktop                  0x03
#define DXENG_INDEX_DxEngDispUniq                       0x04
#define DXENG_INDEX_DxEngIncDispUniq                    0x05
#define DXENG_INDEX_DxEngVisRgnUniq                     0x06
#define DXENG_INDEX_DxEngLockShareSem                   0x07
#define DXENG_INDEX_DxEngUnlockShareSem                 0x08
#define DXENG_INDEX_DxEngEnumerateHdev                  0x09
#define DXENG_INDEX_DxEngLockHdev                       0x0A
#define DXENG_INDEX_DxEngUnlockHdev                     0x0B
#define DXENG_INDEX_DxEngIsHdevLockedByCurrentThread    0x0C
#define DXENG_INDEX_DxEngReferenceHdev                  0x0D
#define DXENG_INDEX_DxEngUnreferenceHdev                0x0E
#define DXENG_INDEX_DxEngGetDeviceGammaRamp             0x0F
#define DXENG_INDEX_DxEngSetDeviceGammaRamp             0x10
#define DXENG_INDEX_DxEngSpTearDownSprites              0x11
#define DXENG_INDEX_DxEngSpUnTearDownSprites            0x12
#define DXENG_INDEX_DxEngSpSpritesVisible               0x13
#define DXENG_INDEX_DxEngGetHdevData                    0x14
#define DXENG_INDEX_DxEngSetHdevData                    0x15
#define DXENG_INDEX_DxEngCreateMemoryDC                 0x16
#define DXENG_INDEX_DxEngGetDesktopDC                   0x17
#define DXENG_INDEX_DxEngDeleteDC                       0x18
#define DXENG_INDEX_DxEngCleanDC                        0x19
#define DXENG_INDEX_DxEngSetDCOwner                     0x1A
#define DXENG_INDEX_DxEngLockDC                         0x1B
#define DXENG_INDEX_DxEngUnlockDC                       0x1C
#define DXENG_INDEX_DxEngSetDCState                     0x1D
#define DXENG_INDEX_DxEngGetDCState                     0x1E
#define DXENG_INDEX_DxEngSelectBitmap                   0x1F
#define DXENG_INDEX_DxEngSetBitmapOwner                 0x20
#define DXENG_INDEX_DxEngDeleteSurface                  0x21
#define DXENG_INDEX_DxEngGetSurfaceData                 0x22
#define DXENG_INDEX_DxEngAltLockSurface                 0x23
#define DXENG_INDEX_DxEngUploadPaletteEntryToSurface    0x24
#define DXENG_INDEX_DxEngMarkSurfaceAsDirectDraw        0x25
#define DXENG_INDEX_DxEngSelectPaletteToSurface         0x26
#define DXENG_INDEX_DxEngSyncPaletteTableWithDevice     0x27
#define DXENG_INDEX_DxEngSetPaletteState                0x28
#define DXENG_INDEX_DxEngGetRedirectionBitmap           0x29
#define DXENG_INDEX_DxEngLoadImage                      0x2A

typedef enum _DXEGSHDEVDATA
{
  DxEGShDevData_Surface,
  DxEGShDevData_hSpooler,
  DxEGShDevData_DitherFmt,
  DxEGShDevData_FxCaps,
  DxEGShDevData_FxCaps2,
  DxEGShDevData_DrvFuncs,
  DxEGShDevData_dhpdev,
  DxEGShDevData_eddg,
  DxEGShDevData_dd_nCount,
  DxEGShDevData_dd_flags,
  DxEGShDevData_disable,
  DxEGShDevData_metadev,
  DxEGShDevData_display,
  DxEGShDevData_Parent,
  DxEGShDevData_OpenRefs,
  DxEGShDevData_palette,
  DxEGShDevData_ldev,
  DxEGShDevData_GDev,
  DxEGShDevData_clonedev,
} DXEGSHDEVDATA,*PDXEGSHDEVDATA;

/************************************************************************/
/* win32k.sys internal protypes for the driver functions it export      */
/************************************************************************/
BOOLEAN STDCALL DxEngNUIsTermSrv();
BOOLEAN DxEngRedrawDesktop();
ULONG DxEngDispUniq();
ULONG DxEngVisRgnUniq();
HDEV *DxEngEnumerateHdev(HDEV *hdev);
BOOL DxEngGetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp);
PDC STDCALL DxEngLockDC(HDC hDC);
BOOLEAN STDCALL DxEngUnlockDC(PDC pDC);
BOOLEAN DxEngSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp, BOOL Unuse);
BOOLEAN DxEngLockShareSem();
BOOLEAN DxEngUnlockShareSem();
BOOLEAN DxEngCleanDC(HDC hdc);
DWORD STDCALL DxEngGetHdevData(HDEV, DXEGSHDEVDATA);
BOOLEAN STDCALL DxEngSetHdevData(HDEV, DXEGSHDEVDATA, DWORD);
BOOLEAN DxEngIncDispUniq();
BOOLEAN STDCALL DxEngLockHdev(HDEV hdev);
BOOLEAN DxEngUnlockHdev(HDEV hdev);
DWORD STDCALL DxEngGetDCState(HDC hDC, DWORD type);

/* prototypes are not done yet, I need gather all my notes
 * to make them correct
 */
DWORD DxEngCreateMemoryDC(DWORD x1);
DWORD DxEngScreenAccessCheck();

DWORD DxEngReferenceHdev(DWORD x1);
DWORD DxEngIsHdevLockedByCurrentThread(DWORD x1);
DWORD DxEngUnreferenceHdev(DWORD x1);
DWORD DxEngSpTearDownSprites(DWORD x1, DWORD x2, DWORD x3);
DWORD DxEngSpUnTearDownSprites(DWORD x1, DWORD x2, DWORD x3);
DWORD DxEngSpSpritesVisible(DWORD x1);
DWORD DxEngGetDesktopDC(DWORD x1, DWORD x2, DWORD x3);
DWORD DxEngDeleteDC(DWORD x1, DWORD x2);
DWORD DxEngSetDCOwner(DWORD x1, DWORD x2);
DWORD DxEngSetDCState(DWORD x1, DWORD x2, DWORD x3);
DWORD DxEngSelectBitmap(DWORD x1, DWORD x2);
DWORD DxEngSetBitmapOwner(DWORD x1, DWORD x2);
DWORD DxEngDeleteSurface(DWORD x1);
DWORD DxEngGetSurfaceData(DWORD x1, DWORD x2);
DWORD DxEngAltLockSurface(DWORD x1);
DWORD DxEngUploadPaletteEntryToSurface(DWORD x1, DWORD x2,DWORD x3, DWORD x4);
DWORD DxEngMarkSurfaceAsDirectDraw(DWORD x1, DWORD x2);
DWORD DxEngSelectPaletteToSurface(DWORD x1, DWORD x2);
DWORD DxEngSyncPaletteTableWithDevice(DWORD x1, DWORD x2);
DWORD DxEngSetPaletteState(DWORD x1, DWORD x2, DWORD x3);
DWORD DxEngGetRedirectionBitmap(DWORD x1);
DWORD DxEngLoadImage(DWORD x1,DWORD x2);


