

/************************************************************************/
/* These functions are imported from win32k.sys by dxg.sys              */
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
/* win32k.sys internal protypes for driver functions it exports         */
/************************************************************************/
BOOLEAN NTAPI DxEngNUIsTermSrv(VOID);
PDC NTAPI DxEngLockDC(HDC hDC);
BOOLEAN NTAPI DxEngUnlockDC(PDC pDC);
DWORD_PTR NTAPI DxEngGetHdevData(HDEV, DXEGSHDEVDATA);
BOOLEAN NTAPI DxEngSetHdevData(HDEV, DXEGSHDEVDATA, DWORD_PTR);
BOOLEAN NTAPI DxEngLockHdev(HDEV hdev);
BOOLEAN NTAPI DxEngUnlockHdev(HDEV hdev);
DWORD_PTR NTAPI DxEngGetDCState(HDC hDC, DWORD type);
BOOLEAN NTAPI DxEngReferenceHdev(HDEV hdev);
BOOLEAN NTAPI DxEngLockShareSem(VOID);
BOOLEAN NTAPI DxEngUnlockShareSem(VOID);
DWORD NTAPI DxEngScreenAccessCheck(VOID);
BOOL NTAPI DxEngSetDCOwner(HGDIOBJ hObject, DWORD OwnerMask);

/* Prototypes for the following functions are not yet finished */
BOOLEAN NTAPI DxEngRedrawDesktop(VOID);
ULONG NTAPI DxEngDispUniq(VOID);
ULONG NTAPI DxEngVisRgnUniq(VOID);
HDEV* NTAPI DxEngEnumerateHdev(HDEV *hdev);
BOOL NTAPI DxEngGetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp);
BOOLEAN NTAPI DxEngSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp, BOOL Unuse);
BOOLEAN NTAPI DxEngCleanDC(HDC hdc);
BOOLEAN NTAPI DxEngIncDispUniq(VOID);

HDC NTAPI DxEngCreateMemoryDC(HDEV hDev);

BOOLEAN NTAPI DxEngIsHdevLockedByCurrentThread(HDEV hDev);
BOOLEAN NTAPI DxEngUnreferenceHdev(HDEV hDev);
DWORD NTAPI DxEngSpTearDownSprites(DWORD x1, DWORD x2, DWORD x3);
DWORD NTAPI DxEngSpUnTearDownSprites(DWORD x1, DWORD x2, DWORD x3);
DWORD NTAPI DxEngSpSpritesVisible(DWORD x1);
HDC NTAPI DxEngGetDesktopDC(ULONG DcType, BOOL EmptyDC, BOOL ValidatehWnd);
BOOLEAN NTAPI DxEngDeleteDC(HDC hdc, BOOL Force);
BOOLEAN NTAPI DxEngSetDCState(HDC hDC, DWORD SetType, DWORD Set);
HBITMAP APIENTRY DxEngSelectBitmap(HDC hdc, HBITMAP hbmp);
BOOLEAN APIENTRY DxEngSetBitmapOwner(HBITMAP hbmp, ULONG ulOwner);
BOOLEAN APIENTRY DxEngDeleteSurface(HSURF hsurf);
DWORD NTAPI DxEngGetSurfaceData(DWORD x1, DWORD x2);
DWORD NTAPI DxEngAltLockSurface(DWORD x1);
DWORD NTAPI DxEngUploadPaletteEntryToSurface(DWORD x1, DWORD x2,DWORD x3, DWORD x4);
DWORD NTAPI DxEngMarkSurfaceAsDirectDraw(DWORD x1, DWORD x2);
DWORD NTAPI DxEngSelectPaletteToSurface(DWORD x1, DWORD x2);
DWORD NTAPI DxEngSyncPaletteTableWithDevice(DWORD x1, DWORD x2);
DWORD NTAPI DxEngSetPaletteState(DWORD x1, DWORD x2, DWORD x3);
DWORD NTAPI DxEngGetRedirectionBitmap(DWORD x1);
DWORD NTAPI DxEngLoadImage(DWORD x1,DWORD x2);


