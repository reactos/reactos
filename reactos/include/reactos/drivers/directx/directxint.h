
#ifndef _DXINTERNEL_
#define _DXINTERNEL_

typedef struct _EDD_DIRECTDRAW_LOCAL
{
    //
    // GDI Object Header
    //
    HANDLE hHmgr;
    PVOID pEntry;
    INT cExcLock;
    HANDLE Tid;

    struct _EDD_DIRECTDRAW_GLOBAL * peDirectDrawGlobal;
    struct _EDD_DIRECTDRAW_GLOBAL * peDirectDrawGlobal2;
    struct _EDD_SURFACE * peSurface_DdList;
    ULONG unk_01c[2];
    struct _EDD_DIRECTDRAW_LOCAL * peDirectDrawLocalNext;
    FLATPTR fpProcess;
    FLONG fl;
    HANDLE UniqueProcess;
    PEPROCESS Process;
    ULONG unk_038[2];
    VOID *unk_040;
    VOID *unk_044;
} EDD_DIRECTDRAW_LOCAL, PEDD_DIRECTDRAW_LOCAL;


//
// Surface Object
//
typedef struct _EDD_SURFACE
{
    //
    // GDI Object Header
    //
    HANDLE hHmgr;
    PVOID pEntry;
    INT cExcLock;
    HANDLE Tid;

    //
    // Direct Draw Surface Data
    //
    DD_SURFACE_LOCAL ddsSurfaceLocal;
    DD_SURFACE_MORE ddsSurfaceMore;
    DD_SURFACE_GLOBAL ddsSurfaceGlobal;
    DD_SURFACE_INT ddsSurfaceInt;

    //
    // Surface pointers
    //
    struct _EDD_SURFACE *peSurface_DdNext;
    struct _EDD_SURFACE *peSurface_LockNext;

    //
    // Unknown
    //
    ULONG field_C0;

    //
    // Private Direct Draw Data
    //
    struct _EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal;
    struct _EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal;

    //
    // Flags
    //
    FLONG fl;

    //
    // Surface Attributes
    //
    ULONG cLocks;
    ULONG iVisRgnUniqueness;
    BOOL bLost;
    HANDLE hSecure;
    HANDLE hdc;
    HBITMAP hbmGdi;

    //
    // Unknown
    //
    ULONG field_E8;

    //
    // Surface Lock
    //
    RECTL rclLock;
    ULONG field_FC[2];
} EDD_SURFACE, *PEDD_SURFACE;


typedef struct _EDD_DIRECTDRAW_GLOBAL
{
/* 0x000 */    PVOID dhpdev;           /* The assign pdev */
/* 0x004 */    DWORD dwReserved1;
/* 0x008 */    DWORD dwReserved2;
/* 0x00C */    ULONG unk_000c[3];
/* 0x018 */    LONG cDriverReferences;
/* 0x01C */    ULONG unk_01c[3];
/* 0x028 */    LONGLONG llAssertModeTimeout;
/* 0x030 */    DWORD dwNumHeaps;
/* 0x034 */    VIDEOMEMORY *pvmList;
/* 0x038 */    DWORD dwNumFourCC;
/* 0x03C */    PDWORD pdwFourCC;
/* 0x040 */    DD_HALINFO ddHalInfo;
/* 0x1E0 */    ULONG unk_1e0[44];
/* 0x290 */    DD_CALLBACKS ddCallbacks;
/* 0x2C4 */    DD_SURFACECALLBACKS ddSurfaceCallbacks;
/* 0x304 */    DD_PALETTECALLBACKS ddPaletteCallbacks;
/* 0x314 */    ULONG unk_314[48];
/* 0x3D4 */    D3DNTHAL_CALLBACKS d3dNtHalCallbacks;
/* 0x460 */    ULONG unk_460[7];
/* 0x47C */    D3DNTHAL_CALLBACKS2 d3dNtHalCallbacks2;
/* 0x498 */    ULONG unk_498[18];
/* 0x4E0 */    DD_MISCELLANEOUSCALLBACKS ddMiscellanousCallbacks;
/* 0x4EC */    ULONG unk_4ec[18];
/* 0x534 */    D3DNTHAL_CALLBACKS3 d3dNtHalCallbacks3;
/* 0x54C */    ULONG unk_54c[23];
/* 0x5A8 */    EDD_DIRECTDRAW_LOCAL* peDirectDrawLocalList;
/* 0x5ac */    EDD_SURFACE* peSurface_LockList;
/* 0x5B0 */    FLONG fl;
/* 0x5B4 */    ULONG cSurfaceLocks;
/* 0x5B8 */    PKEVENT pAssertModeEvent;
/* 0x5Bc */    EDD_SURFACE *peSurfaceCurrent;
/* 0x5C0 */    EDD_SURFACE *peSurfacePrimary;
/* 0x5C4 */    BOOL bSuspended;
/* 0x5C8 */    ULONG unk_5c8[12];
/* 0x5F8 */    RECTL rcbounds;
/* 0x608 */    HDEV hDev;
/* 0x60c */    PVOID hPDev;  /* The real Pdev */

/* Windows XP and higher */
/* 0x610 */    ULONG unk_610[63];
/* 0x70C */    ULONG unk_70C;
} EDD_DIRECTDRAW_GLOBAL, *PEDD_DIRECTDRAW_GLOBAL;

#endif
