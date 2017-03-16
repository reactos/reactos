#ifndef _DXG_PCH_
#define _DXG_PCH_

#include <ntifs.h>

/* Win32 Headers */
#define WINBASEAPI
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#define INTERNAL_CALL NTAPI
#define NT_BUILD_ENVIRONMENT

#define DDHMG_HANDLE_LIMIT 0x200000
#define DDHMG_HTOI(DdHandle) ((DWORD)DdHandle & (DDHMG_HANDLE_LIMIT-1))


#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>
#include <ddkernel.h>
#include <initguid.h>
#include <ddrawi.h>
#include <ntgdityp.h>
#include <psfuncs.h>

DEFINE_GUID(GUID_NTCallbacks,             0x6fe9ecde, 0xdf89, 0x11d1, 0x9d, 0xb0, 0x00, 0x60, 0x08, 0x27, 0x71, 0xba);
DEFINE_GUID(GUID_DDMoreCaps,              0x880baf30, 0xb030, 0x11d0, 0x8e, 0xa7, 0x00, 0x60, 0x97, 0x97, 0xea, 0x5b);
DEFINE_GUID(GUID_NTPrivateDriverCaps,     0xfad16a23, 0x7b66, 0x11d2, 0x83, 0xd7, 0x00, 0xc0, 0x4f, 0x7c, 0xe5, 0x8c);

/* DXG treats this as opaque */
typedef PVOID PDC;
typedef PVOID PW32THREAD;

typedef struct _DD_BASEOBJECT
{
  HGDIOBJ     hHmgr;
  ULONG       ulShareCount;
  USHORT      cExclusiveLock;
  USHORT      BaseFlags;
  PW32THREAD  Tid;
} DD_BASEOBJECT, *PDD_BASEOBJECT;

#include <drivers/directx/directxint.h>
#include <drivers/directx/dxg.h>
#include <drivers/directx/dxeng.h>

#include "tags.h"

#define CapOver_DisableAccel      0x1
#define CapOver_DisableD3DDDAccel 0x2
#define CapOver_DisableD3DAccel   0x4
#define CapOver_DisableOGL        0x8
#define CapOver_DisableEscapes    0x10

#define ObjType_DDLOCAL_TYPE      1
#define ObjType_DDSURFACE_TYPE    2
#define ObjType_DDCONTEXT_TYPE    3
#define ObjType_DDVIDEOPORT_TYPE  4
#define ObjType_DDMOTIONCOMP_TYPE 5

typedef struct _DD_ENTRY
{
    union
    {
        PDD_BASEOBJECT pobj;
        ULONG NextFree;
    };
    HANDLE Pid;
    USHORT FullUnique;
    UCHAR Objt;
} DD_ENTRY, *PDD_ENTRY;

typedef struct _EDD_SURFACE_LOCAL
{
     DD_BASEOBJECT Object;
     DD_SURFACE_LOCAL Surfacelcl;
} EDD_SURFACE_LOCAL, *PEDD_SURFACE_LOCAL;


typedef BOOLEAN   (APIENTRY* PFN_DxEngNUIsTermSrv)(VOID);
typedef DWORD     (APIENTRY* PFN_DxEngScreenAccessCheck)(VOID);
typedef BOOLEAN   (APIENTRY* PFN_DxEngRedrawDesktop)(VOID);
typedef ULONG     (APIENTRY* PFN_DxEngDispUniq)(VOID);
typedef BOOLEAN   (APIENTRY* PFN_DxEngIncDispUniq)(VOID);
typedef ULONG     (APIENTRY* PFN_DxEngVisRgnUniq)(VOID);
typedef BOOLEAN   (APIENTRY* PFN_DxEngLockShareSem)(VOID);
typedef BOOLEAN   (APIENTRY* PFN_DxEngUnlockShareSem)(VOID);
typedef HDEV*     (APIENTRY* PFN_DxEngEnumerateHdev)(HDEV*);
typedef BOOLEAN   (APIENTRY* PFN_DxEngLockHdev)(HDEV);
typedef BOOLEAN   (APIENTRY* PFN_DxEngUnlockHdev)(HDEV);
typedef BOOLEAN   (APIENTRY* PFN_DxEngIsHdevLockedByCurrentThread)(HDEV);
typedef BOOLEAN   (APIENTRY* PFN_DxEngReferenceHdev)(HDEV);
typedef BOOLEAN   (APIENTRY* PFN_DxEngUnreferenceHdev)(HDEV);
typedef BOOL      (APIENTRY* PFN_DxEngGetDeviceGammaRamp)(HDEV, PGAMMARAMP);
typedef BOOLEAN   (APIENTRY* PFN_DxEngSetDeviceGammaRamp)(HDEV, PGAMMARAMP, BOOL);
typedef DWORD     (APIENTRY* PFN_DxEngSpTearDownSprites)(DWORD, DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngSpUnTearDownSprites)(DWORD, DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngSpSpritesVisible)(DWORD);
typedef DWORD_PTR (APIENTRY* PFN_DxEngGetHdevData)(HDEV, DXEGSHDEVDATA);
typedef BOOLEAN   (APIENTRY* PFN_DxEngSetHdevData)(HDEV, DXEGSHDEVDATA, DWORD_PTR);
typedef HDC       (APIENTRY* PFN_DxEngCreateMemoryDC)(HDEV);
typedef HDC       (APIENTRY* PFN_DxEngGetDesktopDC)(ULONG, BOOL, BOOL);
typedef BOOLEAN   (APIENTRY* PFN_DxEngDeleteDC)(HDC, BOOL);
typedef BOOLEAN   (APIENTRY* PFN_DxEngCleanDC)(HDC hdc);
typedef BOOL      (APIENTRY* PFN_DxEngSetDCOwner)(HGDIOBJ, DWORD);
typedef PDC       (APIENTRY* PFN_DxEngLockDC)(HDC);
typedef BOOLEAN   (APIENTRY* PFN_DxEngUnlockDC)(PDC);
typedef BOOLEAN   (APIENTRY* PFN_DxEngSetDCState)(HDC, DWORD, DWORD);
typedef DWORD_PTR (APIENTRY* PFN_DxEngGetDCState)(HDC, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngSelectBitmap)(DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngSetBitmapOwner)(DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngDeleteSurface)(DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngGetSurfaceData)(DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngAltLockSurface)(DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngUploadPaletteEntryToSurface)(DWORD, DWORD, DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngMarkSurfaceAsDirectDraw)(DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngSelectPaletteToSurface)(DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngSyncPaletteTableWithDevice)(DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngSetPaletteState)(DWORD, DWORD, DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngGetRedirectionBitmap)(DWORD);
typedef DWORD     (APIENTRY* PFN_DxEngLoadImage)(DWORD, DWORD);


typedef struct _DXENG_FUNCTIONS
{
    PVOID                                   Reserved;
    PFN_DxEngNUIsTermSrv                    DxEngNUIsTermSrv;
    PFN_DxEngScreenAccessCheck              DxEngScreenAccessCheck;
    PFN_DxEngRedrawDesktop                  DxEngRedrawDesktop;
    PFN_DxEngDispUniq                       DxEngDispUniq;
    PFN_DxEngIncDispUniq                    DxEngIncDispUniq;
    PFN_DxEngVisRgnUniq                     DxEngVisRgnUniq;
    PFN_DxEngLockShareSem                   DxEngLockShareSem;
    PFN_DxEngUnlockShareSem                 DxEngUnlockShareSem;
    PFN_DxEngEnumerateHdev                  DxEngEnumerateHdev;
    PFN_DxEngLockHdev                       DxEngLockHdev;
    PFN_DxEngUnlockHdev                     DxEngUnlockHdev;
    PFN_DxEngIsHdevLockedByCurrentThread    DxEngIsHdevLockedByCurrentThread;
    PFN_DxEngReferenceHdev                  DxEngReferenceHdev;
    PFN_DxEngUnreferenceHdev                DxEngUnreferenceHdev;
    PFN_DxEngGetDeviceGammaRamp             DxEngGetDeviceGammaRamp;
    PFN_DxEngSetDeviceGammaRamp             DxEngSetDeviceGammaRamp;
    PFN_DxEngSpTearDownSprites              DxEngSpTearDownSprites;
    PFN_DxEngSpUnTearDownSprites            DxEngSpUnTearDownSprites;
    PFN_DxEngSpSpritesVisible               DxEngSpSpritesVisible;
    PFN_DxEngGetHdevData                    DxEngGetHdevData;
    PFN_DxEngSetHdevData                    DxEngSetHdevData;
    PFN_DxEngCreateMemoryDC                 DxEngCreateMemoryDC;
    PFN_DxEngGetDesktopDC                   DxEngGetDesktopDC;
    PFN_DxEngDeleteDC                       DxEngDeleteDC;
    PFN_DxEngCleanDC                        DxEngCleanDC;
    PFN_DxEngSetDCOwner                     DxEngSetDCOwner;
    PFN_DxEngLockDC                         DxEngLockDC;
    PFN_DxEngUnlockDC                       DxEngUnlockDC;
    PFN_DxEngSetDCState                     DxEngSetDCState;
    PFN_DxEngGetDCState                     DxEngGetDCState;
    PFN_DxEngSelectBitmap                   DxEngSelectBitmap;
    PFN_DxEngSetBitmapOwner                 DxEngSetBitmapOwner;
    PFN_DxEngDeleteSurface                  DxEngDeleteSurface;
    PFN_DxEngGetSurfaceData                 DxEngGetSurfaceData;
    PFN_DxEngAltLockSurface                 DxEngAltLockSurface;
    PFN_DxEngUploadPaletteEntryToSurface    DxEngUploadPaletteEntryToSurface;
    PFN_DxEngMarkSurfaceAsDirectDraw        DxEngMarkSurfaceAsDirectDraw;
    PFN_DxEngSelectPaletteToSurface         DxEngSelectPaletteToSurface;
    PFN_DxEngSyncPaletteTableWithDevice     DxEngSyncPaletteTableWithDevice;
    PFN_DxEngSetPaletteState                DxEngSetPaletteState;
    PFN_DxEngGetRedirectionBitmap           DxEngGetRedirectionBitmap;
    PFN_DxEngLoadImage                      DxEngLoadImage;
} DXENG_FUNCTIONS, *PDXENG_FUNCTIONS;

/* exported functions */
NTSTATUS NTAPI DriverEntry(IN PVOID Context1, IN PVOID Context2);
NTSTATUS NTAPI GsDriverEntry(IN PVOID Context1, IN PVOID Context2);
NTSTATUS APIENTRY DxDdCleanupDxGraphics(VOID);
BOOL NTAPI DxDdEnableDirectDraw(HANDLE hDev, BOOL arg2);
DWORD NTAPI DxDdCreateDirectDrawObject(HDC hDC);

/* Global pointers */
extern ULONG gcSizeDdHmgr;
extern PDD_ENTRY gpentDdHmgr;
extern ULONG gcMaxDdHmgr;
extern PDD_ENTRY gpentDdHmgrLast;
extern ULONG ghFreeDdHmgr;
extern HSEMAPHORE ghsemHmgr;
extern LONG gcDummyPageRefCnt;
extern HSEMAPHORE ghsemDummyPage;
extern VOID *gpDummyPage;
extern PEPROCESS gpepSession;
extern PLARGE_INTEGER gpLockShortDelay;
extern DXENG_FUNCTIONS gpEngFuncs;

/* Driver list export functions */
DWORD NTAPI DxDxgGenericThunk(ULONG_PTR ulIndex, ULONG_PTR ulHandle, SIZE_T *pdwSizeOfPtr1, PVOID pvPtr1, SIZE_T *pdwSizeOfPtr2, PVOID pvPtr2);
DWORD NTAPI DxDdIoctl(ULONG ulIoctl, PVOID pBuffer, ULONG ulBufferSize);
PDD_SURFACE_LOCAL NTAPI DxDdLockDirectDrawSurface(HANDLE hDdSurface);
BOOL NTAPI DxDdUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface);
DWORD NTAPI DxDdGetDriverInfo(HANDLE DdHandle, PDD_GETDRIVERINFODATA drvInfoData);
BOOL NTAPI DxDdQueryDirectDrawObject(HANDLE DdHandle, DD_HALINFO* pDdHalInfo, DWORD* pCallBackFlags, LPD3DNTHAL_CALLBACKS pd3dNtHalCallbacks, 
                                     LPD3DNTHAL_GLOBALDRIVERDATA pd3dNtGlobalDriverData, PDD_D3DBUFCALLBACKS pd3dBufCallbacks, LPDDSURFACEDESC pTextureFormats,
                                     DWORD* p8, VIDEOMEMORY* p9, DWORD* pdwNumFourCC, DWORD* pdwFourCC);
DWORD NTAPI DxDdReenableDirectDrawObject(HANDLE DdHandle, PVOID p2);
DWORD NTAPI DxDdCanCreateSurface(HANDLE DdHandle, PDD_CANCREATESURFACEDATA SurfaceData);
DWORD NTAPI DxDdCanCreateD3DBuffer(HANDLE DdHandle, PDD_CANCREATESURFACEDATA SurfaceData);


/* Internal functions */
BOOL FASTCALL VerifyObjectOwner(PDD_ENTRY pEntry);
BOOL FASTCALL DdHmgCreate(VOID);
BOOL FASTCALL DdHmgDestroy(VOID);
PVOID FASTCALL DdHmgLock(HANDLE DdHandle, UCHAR ObjectType, BOOLEAN LockOwned);
HANDLE FASTCALL DdHmgAlloc(ULONG objSize, CHAR objType, BOOLEAN objLock);


#endif /* _DXG_PCH_ */
