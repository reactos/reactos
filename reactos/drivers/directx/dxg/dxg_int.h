#include <ntifs.h>

/* Win32 Headers */
#define WINBASEAPI
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#define INTERNAL_CALL NTAPI
#define NT_BUILD_ENVIRONMENT

#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winddi.h>

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

#define ObjType_DDSURFACE_TYPE    2
#define ObjType_DDVIDEOPORT_TYPE  4
#define ObjType_DDMOTIONCOMP_TYPE 5

 typedef struct _DD_ENTRY
{
    union
    {
        PDD_BASEOBJECT pobj;
        HANDLE hFree;
    };
    union
    {
         ULONG ulObj;
         struct
         {
                USHORT Count;
                USHORT Lock;
                HANDLE Pid;
         };
    } ObjectOwner;
    USHORT FullUnique;
    UCHAR Objt;
    UCHAR Flags;
    PVOID pUser;
} DD_ENTRY, *PDD_ENTRY;

typedef struct _EDD_SURFACE_LOCAL
{
     DD_BASEOBJECT Object;
     DD_SURFACE_LOCAL Surfacelcl;
} EDD_SURFACE_LOCAL, *PEDD_SURFACE_LOCAL;

/* exported functions */
NTSTATUS NTAPI DriverEntry(IN PVOID Context1, IN PVOID Context2);
NTSTATUS NTAPI GsDriverEntry(IN PVOID Context1, IN PVOID Context2);
NTSTATUS APIENTRY DxDdCleanupDxGraphics(VOID);

/* Global pointers */
extern ULONG gcSizeDdHmgr;
extern PDD_ENTRY gpentDdHmgr;
extern ULONG gcMaxDdHmgr;
extern PDD_ENTRY gpentDdHmgrLast;
extern HANDLE ghFreeDdHmgr;
extern HSEMAPHORE ghsemHmgr;
extern LONG gcDummyPageRefCnt;
extern HSEMAPHORE ghsemDummyPage;
extern VOID *gpDummyPage;
extern PEPROCESS gpepSession;
extern PLARGE_INTEGER gpLockShortDelay;

/* Driver list export functions */
DWORD NTAPI DxDxgGenericThunk(ULONG_PTR ulIndex, ULONG_PTR ulHandle, SIZE_T *pdwSizeOfPtr1, PVOID pvPtr1, SIZE_T *pdwSizeOfPtr2, PVOID pvPtr2);
DWORD NTAPI DxDdIoctl(ULONG ulIoctl, PVOID pBuffer, ULONG ulBufferSize);
PDD_SURFACE_LOCAL NTAPI DxDdLockDirectDrawSurface(HANDLE hDdSurface);
BOOL NTAPI DxDdUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface);

NTSTATUS NTAPI DxDdEnableDirectDraw(PVOID pDev, BOOLEAN Enable);
NTSTATUS NTAPI DxD3dContextCreate(DWORD x1, DWORD x2, DWORD x3, DWORD x4);
NTSTATUS NTAPI DxD3dContextDestroy(DWORD x1);
NTSTATUS NTAPI DxD3dContextDestroyAll(DWORD x1);
NTSTATUS NTAPI DxD3dValidateTextureStageState(DWORD x1);
NTSTATUS NTAPI DxD3dDrawPrimitives2(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7);
NTSTATUS NTAPI DxDdGetDriverState(DWORD x1);
NTSTATUS NTAPI DxDdAddAttachedSurface(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdAlphaBlt(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdAttachSurface(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdBeginMoCompFrame(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdBlt(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdCanCreateSurface(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdCanCreateD3DBuffer(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdColorControl(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdCreateDirectDrawObject(DWORD x1);
NTSTATUS NTAPI DxDdCreateSurface(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7, DWORD x8);
NTSTATUS NTAPI DxDdCreateMoComp(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdCreateSurfaceObject(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6);
NTSTATUS NTAPI DxDdDeleteDirectDrawObject(DWORD x1);
NTSTATUS NTAPI DxDdDeleteSurfaceObject(DWORD x1);
NTSTATUS NTAPI DxDdDestroyMoComp(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdDestroySurface(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdDestroyD3DBuffer(DWORD x1);
NTSTATUS NTAPI DxDdEndMoCompFrame(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdFlip(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5);
NTSTATUS NTAPI DxDdFlipToGDISurface(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetAvailDriverMemory(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetBltStatus(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetDC(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetDriverInfo(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetDxHandle(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdGetFlipStatus(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetInternalMoCompInfo(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetMoCompBuffInfo(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetMoCompGuids(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetMoCompFormats(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetScanLine(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdLock(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdLockD3D(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdQueryDirectDrawObject(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5, DWORD x6, DWORD x7, DWORD x8, DWORD x9 , DWORD x10 , DWORD x11);
NTSTATUS NTAPI DxDdQueryMoCompStatus(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdReenableDirectDrawObject(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdReleaseDC(DWORD x1);
NTSTATUS NTAPI DxDdRenderMoComp(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdResetVisrgn(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdSetColorKey(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdSetExclusiveMode(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdSetGammaRamp(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdCreateSurfaceEx(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdSetOverlayPosition(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdUnattachSurface(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdUnlock(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdUpdateOverlay(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdWaitForVerticalBlank(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpCanCreateVideoPort(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpColorControl(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpCreateVideoPort(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpDestroyVideoPort(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpFlipVideoPort(DWORD x1, DWORD x2, DWORD x3, DWORD x4);
NTSTATUS NTAPI DxDvpGetVideoPortBandwidth(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpGetVideoPortField(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpGetVideoPortFlipStatus(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpGetVideoPortInputFormats(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpGetVideoPortLine(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpGetVideoPortOutputFormats(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpGetVideoPortConnectInfo(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpGetVideoSignalStatus(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpUpdateVideoPort(DWORD x1, DWORD x2, DWORD x3, DWORD x4);
NTSTATUS NTAPI DxDvpWaitForVideoPortSync(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDvpAcquireNotification(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDvpReleaseNotification(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdHeapVidMemAllocAligned(DWORD x1, DWORD x2, DWORD x3, DWORD x4, DWORD x5);
NTSTATUS NTAPI DxDdHeapVidMemFree(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdDisableDirectDraw(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdSuspendDirectDraw(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdResumeDirectDraw(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdDynamicModeChange(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdCloseProcess(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdGetDirectDrawBound(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdEnableDirectDrawRedirection(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdAllocPrivateUserMem(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdFreePrivateUserMem(DWORD x1, DWORD x2);
NTSTATUS NTAPI DxDdSetAccelLevel(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdGetSurfaceLock(DWORD x1);
NTSTATUS NTAPI DxDdEnumLockedSurfaceRect(DWORD x1, DWORD x2, DWORD x3);
NTSTATUS NTAPI DxDdEnableDirectDraw(PVOID pDev, BOOLEAN Enable);


/* Internal functions */
BOOL FASTCALL VerifyObjectOwner(PDD_ENTRY pEntry);
BOOL FASTCALL DdHmgCreate(VOID);
BOOL FASTCALL DdHmgDestroy(VOID);
PVOID FASTCALL DdHmgLock(HANDLE DdHandle, UCHAR ObjectType, BOOLEAN LockOwned);

/* define stuff */
#define drvDxEngLockDC          gpEngFuncs[DXENG_INDEX_DxEngLockDC]
#define drvDxEngGetDCState      gpEngFuncs[DXENG_INDEX_DxEngGetDCState]
#define drvDxEngGetHdevData     gpEngFuncs[DXENG_INDEX_DxEngGetHdevData]
#define drvDxEngUnlockDC        gpEngFuncs[DXENG_INDEX_DxEngUnlockDC]
#define drvDxEngUnlockHdev      gpEngFuncs[DXENG_INDEX_DxEngUnlockHdev]
#define drvDxEngLockHdev        gpEngFuncs[DXENG_INDEX_DxEngLockHdev]
