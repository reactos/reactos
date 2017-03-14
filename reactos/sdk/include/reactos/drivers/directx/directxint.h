
#ifndef _DXINTERNEL_
#define _DXINTERNEL_

#ifdef __W32K_H
#define PDD_BASEOBJECT POBJ
#define DD_BASEOBJECT BASEOBJECT
#endif

/* _EDD_DIRECTDRAW_LOCAL is 0x54 bytes long on Windows XP */
typedef struct _EDD_DIRECTDRAW_LOCAL
{
    //
    // GDI Object Header
    //
/* 0x00 */    DD_BASEOBJECT pobj; // verified to match Windows XP
/* 0x10 */    struct _EDD_DIRECTDRAW_GLOBAL * peDirectDrawGlobal;    // verified to match Windows XP
/* 0x14 */    struct _EDD_SURFACE * peSurface_DdList;
/* 0x18 */    ULONG unk_018;
/* 0x1C */    ULONG unk_01c;
/* 0x20 */    ULONG unk_020;
/* 0x24 */    struct _EDD_DIRECTDRAW_GLOBAL * peDirectDrawGlobal2;   // verified to match Windows XP
/* 0x28 */    FLATPTR fpProcess;
/* 0x2C */    FLONG fl;
/* 0x30 */    struct _EDD_DIRECTDRAW_LOCAL *peDirectDrawLocal_prev;  // verified to match Windows XP,
                                                                     // points to the old DDLocal when new handle is created.
/* 0x34 */    ULONG unk_034;
/* 0x38 */    ULONG unk_038;
/* 0x3C */    HANDLE UniqueProcess;
/* 0x40 */    PEPROCESS Process;
/* 0x44 */    VOID *unk_044;
/* 0x48 */    ULONG unk_048;
/* 0x4C */    ULONG unk_04C;
/* 0x50 */    ULONG unk_050;
} EDD_DIRECTDRAW_LOCAL, *PEDD_DIRECTDRAW_LOCAL;


//
// Surface Object
//
typedef struct _EDD_SURFACE
{
    //
    // GDI Object Header
    //
    DD_BASEOBJECT pobj;

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


/* NOTE :
 * if any of these flags are set in dwCallbackFlags (struct EDD_DIRECTDRAW_GLOBAL),
 * it means that the respective callback member for it has been filled in by a graphic driver
 */
#define EDDDGBL_MISCCALLBACKS           0x001 // ddMiscellanousCallbacks
#define EDDDGBL_VIDEOPORTCALLBACKS      0x002 // ddVideoPortCallback
#define EDDDGBL_COLORCONTROLCALLBACKS   0x004 // ddColorControlCallbacks
#define EDDDGBL_MOTIONCOMPCALLBACKS     0x040 // ddMotionCompCallbacks
#define EDDDGBL_MISC2CALLBACKS          0x080 // ddMiscellanous2Callbacks
#define EDDDGBL_DDMORECAPS              0x100 // ddMorecaps
#define EDDDGBL_D3DCALLBACKS3           0x200 // d3dNtHalCallbacks3
#define EDDDGBL_NTCALLBACKS             0x400 // ddNtCallbacks
#define EDDDGBL_PRIVATEDRIVERCAPS       0x800 // ddNtPrivateDriverCaps


typedef struct _EDD_DIRECTDRAW_GLOBAL
{
/* 0x000 */    PVOID dhpdev;           // 0x000 <-- verified to match Windows XP, dhpdev, the drv hPDev --> 
/* 0x004 */    DWORD dwReserved1;
/* 0x008 */    DWORD dwReserved2;
/* 0x00C */    ULONG unk_000c[3];
/* 0x018 */    LONG cDriverReferences;
/* 0x01C */    ULONG unk_01c;
/* 0x020 */    DWORD dwCallbackFlags; /* 0x020 <-- verified to match Windows XP, dwCallbackFlags
                                         Flags value
                                         0x0002 = ddVideoPortCallback and GUID_VideoPortCaps
                                         0x0004 = GUID_ColorControlCallbacks
                                         0x0040 = GUID_MotionCompCallbacks
                                         0x0080 = GUID_Miscellaneous2Callbacks
                                         0x0100 = GUID_DDMoreCaps
                                         0x0200 = GUID_D3DCallbacks3
                                         0x0400 = GUID_NTCallbacks
                                       */

/* 0x024 */    ULONG unk_024;

/* 0x028 */    LARGE_INTEGER   llAssertModeTimeout;                    /* 0x028 <-- verified to match Windows XP, llAssertModeTimeout, it
                                                                          using regkey 
                                                                          HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers\DCI
                                                                          Specifies how long a DirectDraw application can keep a graphics-device frame-buffer locked
                                                                          in second, if this value are set to 0 it disable directdraw acclatrions.
                                                                          it is normal set to 7 (7 sec in windwos xp/2003)
                                                                        */

/* 0x030 */    DWORD dwNumHeaps;                                       // 0x030 <-- verified to match Windows XP, dwNumHeaps
/* 0x034 */    VIDEOMEMORY *pvmList;                                   // 0x034 <-- verified to match Windows XP, pvmList
/* 0x038 */    DWORD dwNumFourCC;                                      // 0x038 <-- verified to match Windows XP, dwNumFourCC
/* 0x03C */    PDWORD pdwFourCC;                                       // 0x03C <-- verified to match Windows XP, pdwFourCC
/* 0x040 */    DD_HALINFO ddHalInfo;                                   // 0x040 <-- verified to match Windows XP, ddHalInfo
/* 0x1E0 */    ULONG unk_1e0[17];                                      // DxApi interface (size 0x44)
/* 0x224 */    ULONG unk_224;                                          // 
/* 0x228 */    ULONG unk_228[14];                                      // AGP interface (size 0x38)
/* 0x260 */    DDKERNELCAPS ddKernelCaps;                              // 0x260 <-- verified to match Windows Server 2003
/* 0x26C */    DD_MORECAPS ddMoreCaps;                                 // 0x26C <-- verified to match Windows Server 2003
/* 0x290 */    DD_NTPRIVATEDRIVERCAPS ddNtPrivateDriverCaps;           // 0x290 <-- verified to match Windows Server 2003
/* 0x298 */    DD_CALLBACKS ddCallbacks;                               // 0x298 <-- verified to match Windows XP, ddCallbacks
/* 0x2C4 */    DD_SURFACECALLBACKS ddSurfaceCallbacks;                 // 0x2C4 <-- verified to match Windows XP, ddSurfaceCallbacks
/* 0x304 */    DD_PALETTECALLBACKS ddPaletteCallbacks;                 // 0x304 <-- verified to match Windows XP, ddPaletteCallbacks
/* 0x314 */    D3DNTHAL_GLOBALDRIVERDATA d3dNtGlobalDriverData;
/* 0x3D4 */    D3DNTHAL_CALLBACKS d3dNtHalCallbacks;
/* 0x460 */    DD_D3DBUFCALLBACKS d3dBufCallbacks;
/* 0x47C */    D3DNTHAL_CALLBACKS2 d3dNtHalCallbacks2;
/* 0x498 */    DD_VIDEOPORTCALLBACKS ddVideoPortCallback;              // 0x498 <-- verified to match Windows XP, ddVideoPortCallback
/* 0x4E0 */    DD_MISCELLANEOUSCALLBACKS ddMiscellanousCallbacks;      // 0x4E0 <-- verified to match Windows XP, ddMiscellanousCallbacks
/* 0x4EC */    DD_MISCELLANEOUS2CALLBACKS ddMiscellanous2Callbacks;    // 0x4EC <-- verified to match Windows XP, ddMiscellanous2Callbacks
/* 0x504 */    DD_NTCALLBACKS ddNtCallbacks;                           // 0x504 <-- verified to match Windows Server 2003
/* 0x518 */    DD_COLORCONTROLCALLBACKS ddColorControlCallbacks;       // 0x518 <-- verified to match Windows Server 2003
/* 0x524 */    DD_KERNELCALLBACKS ddKernelCallbacks;                   // 0x524 <-- verified to match Windows Server 2003
/* 0x534 */    D3DNTHAL_CALLBACKS3 d3dNtHalCallbacks3;                 // 0x524 <-- verified to match Windows Server 2003
/* 0x54C */    DD_MOTIONCOMPCALLBACKS ddMotionCompCallbacks;           // 0x54C <-- verified to match Windows Server 2003
/* 0x57C */    DDMORESURFACECAPS ddMoreSurfaceCaps;                    // 0x57C <-- verified to match Windows Server 2003
/* 0x5A8 */    EDD_DIRECTDRAW_LOCAL* peDirectDrawLocalList; // 0x5A8 <-- verified to match Windows XP, it is a current local struct, not a list, peDirectDrawLocalList Current
/* 0x5AC */    EDD_SURFACE* peSurface_LockList;
/* 0x5B0 */    FLONG fl;
/* 0x5B4 */    ULONG cSurfaceLocks;
/* 0x5B8 */    PKEVENT pAssertModeEvent;
/* 0x5BC */    EDD_SURFACE *peSurfaceCurrent;
/* 0x5C0 */    EDD_SURFACE *peSurfacePrimary;
/* 0x5C4 */    BOOL bSuspended;                             // 0x5C4 <-- verified to match Windows XP, tells dxg to use driver's own api or return error code instead 
/* 0x5C8 */    ULONG unk_5c8[12];
/* 0x5F8 */    RECTL rcbounds;
/* 0x608 */    ULONG unk_608;
/* 0x60C */    HDEV hDev;                                   // 0x60c <-- verified to match Windows XP, The real Pdev, hDev

/* Windows XP and higher */
/* 0x610 */    ULONG unk_610[63];
/* 0x70C */    ULONG unk_70C;
} EDD_DIRECTDRAW_GLOBAL, *PEDD_DIRECTDRAW_GLOBAL;

#endif
