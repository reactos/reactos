//
// Surface Object Flags
//
#define DDPF_ALPHAPIXELS        0x0001
#define DDPF_ALPHA              0x0002
#define DDPF_FOURCC             0x0004
#define DDPF_PALETTEINDEXED4    0x0008
#define DDPF_PALETTEINDEXEDTO8  0x0010
#define DDPF_PALETTEINDEXED8    0x0020
#define DDPF_RGB                0x0040
#define DDPF_COMPRESSED         0x0080
#define DDPF_RGBTOYUV           0x0100
#define DDPF_YUV                0x0200
#define DDPF_ZBUFFER            0x0400
#define DDPF_PALETTEINDEXED1    0x0800
#define DDPF_PALETTEINDEXED2    0x1000
#define DDPF_ZPIXELS            0x2000

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
    DD_SURCFACE_MORE ddsSurfaceMore;
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
    ERECTL rclLock;
    ULONG field_FC[2];
} EDD_SURFACE, *PEDD_SURFACE;


typedef struct _EDD_DIRECTDRAW_GLOBAL
{
    PVOID dhpdev;
    DWORD dwReserved1;
    DWORD dwReserved2;
    ULONG unk_000c[3];
    LONG cDriverReferences;
    ULONG unk_01c[3];
    LONGLONG llAssertModeTimeout;
    DWORD dwNumHeaps;
    VIDEOMEMORY *pvmList;
    DWORD dwNumFourCC;
    PDWORD pdwFourCC;
    DD_HHALINFO ddHalInfo;
    ULONG unk_1e0[44];
    DD_CALLBACKS ddCallbacks;
    DD_SURFACECALLBACKS ddSurfaceCallbacks;
    DD_PALETTE_CALLBACKS ddPaletteCallbacks;
    ULONG unk_314[48];
    D3DNTHAL_CALLBACKS d3dNtHalCallbacks;
    ULONG unk_460[7];
    D3DNTHAL_CALLBACKS2 d3dNtHalCallbacks2;
    ULONG unk_498[18];
    DD_MISCELLANEOUSCALLBACKS ddMiscellanousCallbacks;
    ULONG unk_4ec[18];
    D3DNTHAL_CALLBACKS3 d3dNtHalCallbacks3;
    ULONG unk_54c[23];
    EDD_DIRECTDRAW_LOCAL* peDirectDrawLocalList;
    EDD_SURFACE* peSurface_LockList;
    FLONG fl;
    ULONG cSurfaceLocks;
    PKEVENT pAssertModeEvent;
    EDD_SURFACE *peSurfaceCurrent;
    EDD_SURFACE *peSurfacePrimary;
    BOOL bSuspended;
    ULONG unk_5c8[12];
    RECTL rcbounds;
    HDEV hDev;
    ULONG unk_60c;
} EDD_DIRECTDRAW_GLOBAL, *PEDD_DIRECTDRAW_GLOBAL;

