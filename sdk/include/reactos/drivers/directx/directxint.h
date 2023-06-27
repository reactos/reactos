
#ifndef _DXINTERNEL_
#define _DXINTERNEL_

#ifdef __W32K_H
#define PDD_BASEOBJECT POBJ
#define DD_BASEOBJECT BASEOBJECT
#endif

// Size 0x190
typedef struct _EDD_MOTIONCOMP
{
    /* 0x00 */ DD_BASEOBJECT pobj;

    /* 0x54 */ struct _EDD_MOTIONCOMP *peDirectDrawMotionNext;

    /* 0x18C */ DWORD dwLastMember;
} EDD_MOTIONCOMP, *PEDD_MOTIONCOMP;

/* _EDD_DIRECTDRAW_LOCAL is 0x54 bytes long on Windows XP */
typedef struct _EDD_DIRECTDRAW_LOCAL
{
    /* 0x00 */    DD_BASEOBJECT pobj;                                   // verified to match Windows XP
    /* 0x10 */    struct _EDD_DIRECTDRAW_GLOBAL *peDirectDrawGlobal;    // verified to match Windows XP
    /* 0x14 */    ULONG hRefCount;                                      // How many handles exist to this local
    /* 0x18 */    ULONG cActiveSurface;                                 // How many handles of this local are active
    /* 0x1C */    struct _EDD_SURFACE *peSurface_DdList;                // List of other surfaces?
    /* 0x20 */    ULONG hSurface;                                       // EDD_SURFACE object of this local
    /* 0x24 */    struct _EDD_DIRECTDRAW_GLOBAL *peDirectDrawGlobal2;   // verified to match Windows XP
    /* 0x28 */    struct _EDD_VIDEOPORT* peDirectDrawVideoport;          // Videoport
    /* 0x2C */    struct _EDD_MOTIONCOMP *peDirectDrawMotion;            // Should be a list of MotionComp objects
    /* 0x30 */    struct _EDD_DIRECTDRAW_LOCAL *peDirectDrawLocal_prev;  // verified to match Windows XP,
                                                                         // points to the old DDLocal when new handle is created.
    /* 0x34 */    FLATPTR fpProcess2;                                    // surface memory address returned by graphic driver
    /* 0x38 */    ULONG isMemoryMapped;                                  // Is memory mapped to fpProcess2
    /* 0x3C */    HANDLE hProcess;                                       // Current Thread Process 
    /* 0x40 */    HANDLE hCreatorProcess;                                // A handle to the process that created this local
    /* 0x44 */    PVOID *ppvHeaps;                                         // Some kind of heaps that should be allocated based on DirectDrawGlobal->dwNumHeaps
    /* 0x48 */    DWORD dwNumHeaps;
    /* 0x4C */    FLATPTR peMapCurrent;
    /* 0x50 */    ULONG unk_050;
} EDD_DIRECTDRAW_LOCAL, *PEDD_DIRECTDRAW_LOCAL;

typedef struct _EDD_VIDEOPORT 
{
    /* 0x00 */ DD_BASEOBJECT pobj;
    /* 0x10 */ DD_VIDEOPORT_LOCAL ddvVideoportLocal;
    /* 0xB8 */ ULONG unk_0b8;
    /* 0xBC */ struct _EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal;
    /* 0xB8 */ struct _EDD_VIDEOPORT* peDirectDrawVideoport; 

    /* 0xEC */ ULONG unk_DxObj;
    /* 0xEC */ HANDLE hVideoPort;
} EDD_VIDEOPORT, *PEDD_VIDEOPORT;

// This struct appears to almost be entirely D3DNTHAL_CONTEXTCREATEDATA
// with two additional members
typedef struct _D3D_CREATECONTEXT_INT
{
    /* 0x00 */ union
    {
        PDD_DIRECTDRAW_GLOBAL   lpDDGbl;
        PDD_DIRECTDRAW_LOCAL    lpDDLcl;
    };
    /* 0x04 */ union
    {
        PDD_SURFACE_LOCAL       lpDDS;
        PDD_SURFACE_LOCAL       lpDDSLcl;
    };
    /* 0x08 */ union
    {
        PDD_SURFACE_LOCAL       lpDDSZ;
        PDD_SURFACE_LOCAL       lpDDSZLcl;
    };
    /* 0x0C */ DWORD dwPID;
    /* 0x10 */ DWORD dwhContext;
    /* 0x14 */ HRESULT ddrval;
    /* 0x18 */ DWORD dwAlignment;    // Offset address for page alignment
    /* 0x1C */ DWORD dwRegionSize; // This is the actual size of data requested/allocated
} D3D_CREATECONTEXT_INT, *PD3D_CREATECONTEXT_INT;

typedef struct _EDD_CONTEXT 
{
    /* 0x00 */ DD_BASEOBJECT pobj;
    /* 0x10 */ BOOL bTexture; 
    /* 0x14 */ ULONG_PTR dwhContext;
    /* 0x18 */ struct _EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal;
    /* 0x1C */ DWORD unk_01c;
    /* 0x20 */ struct _EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal;
    /* 0x24 */ struct _EDD_SURFACE *peSurface_Target; // Target Surface?
    /* 0x28 */ struct _EDD_SURFACE *peSurface_Z; // Z Surface?
    /* 0x2C */ PVOID  BaseAddress; // Base address of primitive buffer
    /* 0x30 */ DWORD dwAlignment;    // Offset address for page alignment
    /* 0x34 */ DWORD dwRegionSize;  // This is the total size allocated paged aligned
    /* 0x38 */ HANDLE hSecure;

    // Whole hell of a lot of IDK between here

    /* 0x138 */ ULONG_PTR dwhContextPrev;
    /* 0x13C */ DWORD unk_13C;

} EDD_CONTEXT, *PEDD_CONTEXT;

//
// Surface Object
//
typedef struct _EDD_SURFACE
{
    //
    // GDI Object Header
    //

    /* 0x00 */ DD_BASEOBJECT pobj;

    //
    // Direct Draw Surface Data
    //

    /* 0x10 */ DD_SURFACE_LOCAL ddsSurfaceLocal;
    /* 0x4C */ DD_SURFACE_MORE ddsSurfaceMore;
    /* 0x68 */ DD_SURFACE_GLOBAL ddsSurfaceGlobal;
    /* 0xB4 */ DD_SURFACE_INT ddsSurfaceInt;

    //
    // Surface pointers
    //
    /* 0xB8 */ struct _EDD_SURFACE *peSurface_DdNext;
    /* 0xBC */ struct _EDD_SURFACE *peSurface_LockNext;

    //
    // Unknown
    //
    /* 0xC0 */ struct _EDD_SURFACE *field_C0;

    //
    // Private Direct Draw Data
    //
    /* 0xC4 */ struct _EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal;
    /* 0xC8 */ struct _EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobalNext;
    /* 0xCC */ struct _EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal;

    // I want to say anything before this point lines up with 
    // the expected structure of EDD_SURFACE
    // past this I have concerns that things are off and out of order

    //
    // Surface Attributes
    //
    /* 0xD0 */ ULONG unk_flag; 
    ULONG cLocks; 
    ULONG iVisRgnUniqueness; 
    BOOL bLost;
    HANDLE hSecure;
    HANDLE hdc;
    HBITMAP hbmGdi;
    HANDLE hGdiSurface; 

    //
    // Surface Lock
    //
    RECTL rclLock;
    ULONG field_FC;
    ULONG field_100;
    ULONG field_104;
    ULONG field_108;

    /* 0x10C */ struct _EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobalC;

    ULONG ldev;
    struct _EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal3;
    ULONG gdev;
    ULONG wWidth;
    ULONG wHeight;
} EDD_SURFACE, *PEDD_SURFACE;


typedef DWORD (APIENTRY *PDD_DXLOSEOBJECT)(PVOID, DWORD);

typedef struct _DXAPI_CALLBACKS
{
	DWORD DxAutoflipUpdate;
	PDD_DXLOSEOBJECT DxApiLoseObject;
	DWORD DxEnableIRQ;
	DWORD DxUpdateCapture;
	DWORD DxApi;
} DXAPI_CALLBACKS, *PDXAPI_CALLBACKS;

typedef struct _DEFERMEMORY_ENTRY
{
    PVOID pvMemory;
    PEDD_SURFACE peDdSurface;
    struct _DEFERMEMORY_ENTRY *pPrevEntry;
} DEFERMEMORY_ENTRY, *PDEFERMEMORY_ENTRY;

typedef struct _D3D_SURFACELOCK
{
    PEDD_SURFACE peDdSurf;       // 0
    BOOL bOptional;              // 1
    PEDD_SURFACE peDdLocked;     // 2
    PDD_SURFACE_LOCAL peDdLocal; // 3
} D3D_SURFACELOCK, *PD3D_SURFACELOCK;

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
    /* 0x00C */    LPDDVIDEOPORTCAPS lpDDVideoPortCaps;                    // 0x00C <-- verified to match Win2k3
    /* 0x010 */    ULONG unk_010;
    /* 0x014 */    ULONG unk_014;
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
    /* 0x5A4 */    ULONG unk_5a4;
    /* 0x5A8 */    EDD_DIRECTDRAW_LOCAL* peDirectDrawLocalList; // 0x5A8 <-- verified to match Windows XP, it is a current local struct, not a list, peDirectDrawLocalList Current
    /* 0x5AC */    EDD_SURFACE* peSurface_LockList;
    /* 0x5B0 */    FLONG fl;                                    // Acceleration Flags
    /* 0x5B4 */    ULONG cSurfaceLocks;
    /* 0x5B8 */    PKEVENT pAssertModeEvent;
    /* 0x5BC */    EDD_SURFACE *peSurfaceCurrent;
    /* 0x5C0 */    EDD_SURFACE *peSurfacePrimary;
    /* 0x5C4 */    BOOL bSuspended;                            // 0x5C4 <-- verified to match Windows XP, tells dxg to use driver's own api or return error code instead
    /* 0x5C8 */    DWORD dwMapCount;                           // How many times MapMemory has been called
    /* 0x5CC */    ULONG gSurfaceLocks;                        // Count of global surface locks
    /* 0x5D0 */    HANDLE hDxLoseObj;                          // DXOBJ list
    /* 0x5D4 */    HANDLE hDirectDraw;                         // DirectDraw Handle
    /* 0x5D8 */    DXAPI_CALLBACKS dxApiCallbacks;             // dxapi Function Pointers
    /* 0x5EC */    HANDLE hDxApi;                              // dxapi.sys Image
    /* 0x5F0 */    DWORD dwApiReferences;                      // How many times LoadDxApi has been called
    /* 0x5F4 */    ULONG unk_5f4;
    /* 0x5F8 */    RECTL rcbounds;
    /* 0x608 */    ULONG unk_608;                               // 0x608 Some kind of deferred free memory list 
    /* 0x60C */    HDEV hDev;                                   // 0x60c <-- verified to match Windows XP, The real Pdev, hDev

    /* Windows XP and higher */
    /* 0x610 */    HDC unk_hdc;                                 // 0x610 some unknown HDC
    /* 0x614 */    ULONG unk_614[62];
    /* 0x70C */    ULONG unk_70C;
} EDD_DIRECTDRAW_GLOBAL, *PEDD_DIRECTDRAW_GLOBAL;

#endif
