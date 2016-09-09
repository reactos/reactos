#ifndef _DXG_PCH_
#define _DXG_PCH_

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
BOOL NTAPI DxDdEnableDirectDraw(PVOID arg1, BOOL arg2);

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

#endif /* _DXG_PCH_ */
