/* 
 * DirectDraw NT driver interface
 */

#ifndef __DD_INCLUDED__
#define __DD_INCLUDED__

#include <ddraw.h>
#include <ole32/guiddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* pointer to video memory */
typedef ULONG_PTR FLATPTR;

// FIXME: These should have been defined in other header files!
typedef struct _DDVIDEOPORTCAPS      *LPDDVIDEOPORTCAPS; /* should be in dvp.h */
typedef struct _DDVIDEOPORTDESC      *LPDDVIDEOPORTDESC; /* should be in dvp.h */
typedef struct _DDVIDEOPORTBANDWIDTH *LPDDVIDEOPORTBANDWIDTH; /* should be in dvp.h */
typedef struct _DDVIDEOPORTCONNECT   *LPDDVIDEOPORTCONNECT; /* should be in dvp.h */
typedef struct _DDVIDEOPORTINFO      *LPDDVIDEOPORTINFO; /* should be in dvp.h */
typedef struct _DDKERNELCAPS         *LPDDKERNELCAPS; /* should be in ddkernel.h */
typedef struct _VMEMHEAP             *LPVMEMHEAP; /* should be in dmemmgr.h */
typedef struct _DD_VIDEOPORT_LOCAL   *PDD_VIDEOPORT_LOCAL; /* should be defined here once we have dvp.h */

/************************************************************************/
/* Video memory info structures                                         */
/************************************************************************/

typedef struct
{
	DWORD          dwFlags;
	FLATPTR        fpStart;
	union
	{
		FLATPTR    fpEnd;
		DWORD      dwWidth;
	};
	DDSCAPS        ddsCaps;
	DDSCAPS        ddsCapsAlt;
	union
	{
		LPVMEMHEAP lpHeap;
		DWORD      dwHeight;
	};
} VIDEOMEMORY, *PVIDEOMEMORY;

#define VIDMEM_ISLINEAR      (1<<0)
#define VIDMEM_ISRECTANGULAR (1<<1)
#define VIDMEM_ISHEAP        (1<<2)
#define VIDMEM_ISNONLOCAL    (1<<3)
#define VIDMEM_ISWC          (1<<4)
#define VIDMEM_ISDISABLED    (1<<5)

typedef struct
{
	FLATPTR       fpPrimary;
	DWORD         dwFlags;
	DWORD         dwDisplayWidth;
	DWORD         dwDisplayHeight;
	LONG          lDisplayPitch;
	DDPIXELFORMAT ddpfDisplay;
	DWORD         dwOffscreenAlign;
	DWORD         dwOverlayAlign;
	DWORD         dwTextureAlign;
	DWORD         dwZBufferAlign;
	DWORD         dwAlphaAlign;
	PVOID         pvPrimary;
} VIDEOMEMORYINFO;
typedef VIDEOMEMORYINFO *LPVIDEOMEMORYINFO;

/************************************************************************/
/* DDI representation of the DirectDraw object                          */
/************************************************************************/

typedef struct
{
	PVOID             dhpdev;
	ULONG_PTR         dwReserved1;
	ULONG_PTR         dwReserved2;
	LPDDVIDEOPORTCAPS lpDDVideoPortCaps;
} DD_DIRECTDRAW_GLOBAL, *PDD_DIRECTDRAW_GLOBAL;

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL lpGbl;
} DD_DIRECTDRAW_LOCAL, *PDD_DIRECTDRAW_LOCAL;

/************************************************************************/
/* DDI representation of the DirectDrawSurface object                   */
/************************************************************************/

typedef struct
{
	union 
	{
		DWORD        dwBlockSizeY;
		LONG         lSlicePitch;
	};

	union 
	{
		PVIDEOMEMORY lpVidMemHeap;
		DWORD        dwBlockSizeX;
		DWORD        dwUserMemSize;
	};

	FLATPTR          fpVidMem;
	union
	{
		LONG         lPitch;
		DWORD        dwLinearSize;
	};
	LONG             yHint;
	LONG             xHint;
	DWORD            wHeight;
	DWORD            wWidth;
	ULONG_PTR        dwReserved1;
	DDPIXELFORMAT    ddpfSurface;
	FLATPTR          fpHeapOffset;
	HANDLE           hCreatorProcess;
} DD_SURFACE_GLOBAL, *PDD_SURFACE_GLOBAL;

typedef struct
{
	DWORD               dwMipMapCount;
	PDD_VIDEOPORT_LOCAL lpVideoPort;
	DWORD               dwOverlayFlags;
	DDSCAPSEX           ddsCapsEx;
	DWORD               dwSurfaceHandle;
} DD_SURFACE_MORE, *PDD_SURFACE_MORE;

typedef struct _DD_ATTACHLIST *PDD_ATTACHLIST;

typedef struct
{
	PDD_SURFACE_GLOBAL lpGbl;
	DWORD              dwFlags;
	DDSCAPS            ddsCaps;
	ULONG_PTR          dwReserved1;
	union
	{
		DDCOLORKEY     ddckCKSrcOverlay;
		DDCOLORKEY     ddckCKSrcBlt;
	};
	union
	{
		DDCOLORKEY     ddckCKDestOverlay;
		DDCOLORKEY     ddckCKDestBlt;
	};
	PDD_SURFACE_MORE   lpSurfMore;
	PDD_ATTACHLIST     lpAttachList;
	PDD_ATTACHLIST     lpAttachListFrom;
	RECT               rcOverlaySrc;
} DD_SURFACE_LOCAL, *PDD_SURFACE_LOCAL;

#define DDRAWISURF_HASCKEYSRCBLT  0x00000800L
#define DDRAWISURF_HASPIXELFORMAT 0x00002000L
#define DDRAWISURF_HASOVERLAYDATA 0x00004000L
#define DDRAWISURF_FRONTBUFFER    0x04000000L
#define DDRAWISURF_BACKBUFFER     0x08000000L
#define DDRAWISURF_INVALID        0x10000000L
#define DDRAWISURF_DRIVERMANAGED  0x40000000L

typedef struct _DD_ATTACHLIST
{
	PDD_ATTACHLIST     lpLink;
	PDD_SURFACE_LOCAL  lpAttached;
} DD_ATTACHLIST;

typedef struct
{
	PDD_SURFACE_LOCAL lpLcl;
} DD_SURFACE_INT, *PDD_SURFACE_INT;

/************************************************************************/
/* DDI representation of the DirectDrawPalette object                   */
/************************************************************************/

typedef struct
{
    ULONG_PTR Reserved1;
} DD_PALETTE_GLOBAL, *PDD_PALETTE_GLOBAL;

/************************************************************************/
/* DDI representation of the DirectDrawVideo object                     */
/************************************************************************/

typedef struct
{
	PDD_DIRECTDRAW_LOCAL lpDD;
	GUID                 guid;
	DWORD                dwUncompWidth;
	DWORD                dwUncompHeight;
	DDPIXELFORMAT        ddUncompPixelFormat;
	DWORD                dwDriverReserved1;
	DWORD                dwDriverReserved2;
	DWORD                dwDriverReserved3;
	LPVOID               lpDriverReserved1;
	LPVOID               lpDriverReserved2;
	LPVOID               lpDriverReserved3;
} DD_MOTIONCOMP_LOCAL, *PDD_MOTIONCOMP_LOCAL;

/************************************************************************/
/* IDirectDrawSurface callbacks                                         */
/************************************************************************/

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	DWORD                         bHasRect;
	RECTL                         rArea;
	LPVOID                        lpSurfData;
	HRESULT                       ddRVal;
	PVOID                         Lock;
	DWORD                         dwFlags;
	FLATPTR                       fpProcess;
} DD_LOCKDATA, *PDD_LOCKDATA;
typedef DWORD (STDCALL *PDD_SURFCB_LOCK)(PDD_LOCKDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	HRESULT                       ddRVal;
	PVOID                         Unlock;
} DD_UNLOCKDATA, *PDD_UNLOCKDATA;
typedef DWORD (STDCALL *PDD_SURFCB_UNLOCK)(PDD_UNLOCKDATA);

#define DDABLT_SRCOVERDEST        0x00000001
#define DDBLT_AFLAGS              0x80000000

typedef struct
{
	BYTE blue, green, red, alpha;
} DDARGB, *PDDARGB;

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDDestSurface;
	RECTL                         rDest;
	PDD_SURFACE_LOCAL             lpDDSrcSurface;
	RECTL                         rSrc;
	DWORD                         dwFlags;
	DWORD                         dwROPFlags;
	DDBLTFX                       bltFX;
	HRESULT                       ddRVal;
	PVOID                         Blt;
	BOOL                          IsClipped;
	RECTL                         rOrigDest;
	RECTL                         rOrigSrc;
	DWORD                         dwRectCnt;
	LPRECT                        prDestRects;
	DWORD                         dwAFlags;
	DDARGB                        ddargbScaleFactors;
} DD_BLTDATA, *PDD_BLTDATA;
typedef DWORD (STDCALL *PDD_SURFCB_BLT)(PDD_BLTDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDDestSurface;
	RECTL                         rDest;
	PDD_SURFACE_LOCAL             lpDDSrcSurface;
	RECTL                         rSrc;
	DWORD                         dwFlags;
	DDOVERLAYFX                   overlayFX;
	HRESULT                       ddRVal;
	PVOID                         UpdateOverlay;
} DD_UPDATEOVERLAYDATA, *PDD_UPDATEOVERLAYDATA;
typedef DWORD (STDCALL *PDD_SURFCB_UPDATEOVERLAY)(PDD_UPDATEOVERLAYDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSrcSurface;
	PDD_SURFACE_LOCAL             lpDDDestSurface;
	LONG                          lXPos;
	LONG                          lYPos;
	HRESULT                       ddRVal;
	PVOID                         SetOverlayPosition;
} DD_SETOVERLAYPOSITIONDATA, *PDD_SETOVERLAYPOSITIONDATA;
typedef DWORD (STDCALL *PDD_SURFCB_SETOVERLAYPOSITION)(PDD_SETOVERLAYPOSITIONDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	PDD_PALETTE_GLOBAL            lpDDPalette;
	HRESULT                       ddRVal;
	PVOID                         SetPalette;
	BOOL                          Attach;
} DD_SETPALETTEDATA, *PDD_SETPALETTEDATA;
typedef DWORD (STDCALL *PDD_SURFCB_SETPALETTE)(PDD_SETPALETTEDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpSurfCurr;
	PDD_SURFACE_LOCAL             lpSurfTarg;
	DWORD                         dwFlags;
	HRESULT                       ddRVal;
	PVOID                         Flip;
	PDD_SURFACE_LOCAL             lpSurfCurrLeft;
	PDD_SURFACE_LOCAL             lpSurfTargLeft;
} DD_FLIPDATA, *PDD_FLIPDATA;
typedef DWORD (STDCALL *PDD_SURFCB_FLIP)(PDD_FLIPDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	HRESULT                       ddRVal;
	PVOID                         DestroySurface;
} DD_DESTROYSURFACEDATA, *PDD_DESTROYSURFACEDATA;
typedef DWORD (STDCALL *PDD_SURFCB_DESTROYSURFACE)(PDD_DESTROYSURFACEDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	HRESULT                       ddRVal;
	PVOID                         SetClipList;
} DD_SETCLIPLISTDATA, *PDD_SETCLIPLISTDATA;
typedef DWORD (STDCALL *PDD_SURFCB_SETCLIPLIST)(PDD_SETCLIPLISTDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	PDD_SURFACE_LOCAL             lpSurfAttached;
	HRESULT                       ddRVal;
	PVOID                         AddAttachedSurface;
} DD_ADDATTACHEDSURFACEDATA, *PDD_ADDATTACHEDSURFACEDATA;
typedef DWORD (STDCALL *PDD_SURFCB_ADDATTACHEDSURFACE)(PDD_ADDATTACHEDSURFACEDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	DWORD                         dwFlags;
	DDCOLORKEY                    ckNew;
	HRESULT                       ddRVal;
	PVOID                         SetColorKey;
} DD_SETCOLORKEYDATA, *PDD_SETCOLORKEYDATA;
typedef DWORD (STDCALL *PDD_SURFCB_SETCOLORKEY)(PDD_SETCOLORKEYDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	DWORD                         dwFlags;
	HRESULT                       ddRVal;
	PVOID                         GetBltStatus;
} DD_GETBLTSTATUSDATA, *PDD_GETBLTSTATUSDATA;
typedef DWORD (STDCALL *PDD_SURFCB_GETBLTSTATUS)(PDD_GETBLTSTATUSDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL         lpDD;
	PDD_SURFACE_LOCAL             lpDDSurface;
	DWORD                         dwFlags;
	HRESULT                       ddRVal;
	PVOID                         GetFlipStatus;
} DD_GETFLIPSTATUSDATA, *PDD_GETFLIPSTATUSDATA;
typedef DWORD (STDCALL *PDD_SURFCB_GETFLIPSTATUS)(PDD_GETFLIPSTATUSDATA);

typedef struct
{
	DWORD                         dwSize;
	DWORD                         dwFlags;
	PDD_SURFCB_DESTROYSURFACE     DestroySurface;
	PDD_SURFCB_FLIP               Flip;
	PDD_SURFCB_SETCLIPLIST        SetClipList;
	PDD_SURFCB_LOCK               Lock;
	PDD_SURFCB_UNLOCK             Unlock;
	PDD_SURFCB_BLT                Blt;
	PDD_SURFCB_SETCOLORKEY        SetColorKey;
	PDD_SURFCB_ADDATTACHEDSURFACE AddAttachedSurface;
	PDD_SURFCB_GETBLTSTATUS       GetBltStatus;
	PDD_SURFCB_GETFLIPSTATUS      GetFlipStatus;
	PDD_SURFCB_UPDATEOVERLAY      UpdateOverlay;
	PDD_SURFCB_SETOVERLAYPOSITION SetOverlayPosition;
	PVOID                         Reserved;
	PDD_SURFCB_SETPALETTE         SetPalette;
} DD_SURFACECALLBACKS, *PDD_SURFACECALLBACKS;

enum
{
	DDHAL_SURFCB32_DESTROYSURFACE     = 1<<0,
	DDHAL_SURFCB32_FLIP               = 1<<1,
	DDHAL_SURFCB32_SETCLIPLIST        = 1<<2,
	DDHAL_SURFCB32_LOCK               = 1<<3,
	DDHAL_SURFCB32_UNLOCK             = 1<<4,
	DDHAL_SURFCB32_BLT                = 1<<5,
	DDHAL_SURFCB32_SETCOLORKEY        = 1<<6,
	DDHAL_SURFCB32_ADDATTACHEDSURFACE = 1<<7,
	DDHAL_SURFCB32_GETBLTSTATUS       = 1<<8,
	DDHAL_SURFCB32_GETFLIPSTATUS      = 1<<9,
	DDHAL_SURFCB32_UPDATEOVERLAY      = 1<<10,
	DDHAL_SURFCB32_SETOVERLAYPOSITION = 1<<11,
	DDHAL_SURFCB32_SETPALETTE         = 1<<13,
};

/************************************************************************/
/* IDirectDraw callbacks                                                */
/************************************************************************/

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	DDSURFACEDESC           *lpDDSurfaceDesc;
	PDD_SURFACE_LOCAL       *lplpSList;
	DWORD                    dwSCnt;
	HRESULT                  ddRVal;
	PVOID                    CreateSurface;
} DD_CREATESURFACEDATA, *PDD_CREATESURFACEDATA;
typedef DWORD (STDCALL *PDD_CREATESURFACE)(PDD_CREATESURFACEDATA);

typedef struct
{
	PDD_SURFACE_LOCAL        lpDDSurface;
	DWORD                    dwFlags;
	DDCOLORKEY               ckNew;
	HRESULT                  ddRVal;
	PVOID                    SetColorKey;
} DD_DRVSETCOLORKEYDATA, *PDD_DRVSETCOLORKEYDATA;
typedef DWORD (STDCALL *PDD_SETCOLORKEY)(PDD_DRVSETCOLORKEYDATA);

#define DDWAITVB_I_TESTVB    0x80000006

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	DWORD                    dwFlags;
	DWORD                    bIsInVB;
	DWORD                    hEvent;
	HRESULT                  ddRVal;
	PVOID                    WaitForVerticalBlank;
} DD_WAITFORVERTICALBLANKDATA, *PDD_WAITFORVERTICALBLANKDATA;
typedef DWORD (STDCALL *PDD_WAITFORVERTICALBLANK)(PDD_WAITFORVERTICALBLANKDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	DDSURFACEDESC           *lpDDSurfaceDesc;
	DWORD                    bIsDifferentPixelFormat;
	HRESULT                  ddRVal;
	PVOID                    CanCreateSurface;
} DD_CANCREATESURFACEDATA, *PDD_CANCREATESURFACEDATA;
typedef DWORD (STDCALL *PDD_CANCREATESURFACE)(PDD_CANCREATESURFACEDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	PDD_PALETTE_GLOBAL       lpDDPalette;
	LPPALETTEENTRY           lpColorTable;
	HRESULT                  ddRVal;
	PVOID                    CreatePalette;
	BOOL                     is_excl;
} DD_CREATEPALETTEDATA, *PDD_CREATEPALETTEDATA;
typedef DWORD (STDCALL *PDD_CREATEPALETTE)(PDD_CREATEPALETTEDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	DWORD                    dwScanLine;
	HRESULT                  ddRVal;
	PVOID                    GetScanLine;
} DD_GETSCANLINEDATA, *PDD_GETSCANLINEDATA;
typedef DWORD (STDCALL *PDD_GETSCANLINE)(PDD_GETSCANLINEDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	BOOL                     bMap;
	HANDLE                   hProcess;
	FLATPTR                  fpProcess;
	HRESULT                  ddRVal;
} DD_MAPMEMORYDATA, *PDD_MAPMEMORYDATA;
typedef DWORD (STDCALL *PDD_MAPMEMORY)(PDD_MAPMEMORYDATA);

typedef struct
{
	DWORD                    dwSize;
	DWORD                    dwFlags;
	PVOID                    Reserved1;
	PDD_CREATESURFACE        CreateSurface;
	PDD_SETCOLORKEY          SetColorKey;
	PVOID                    Reserved2;
	PDD_WAITFORVERTICALBLANK WaitForVerticalBlank;
	PDD_CANCREATESURFACE     CanCreateSurface;
	PDD_CREATEPALETTE        CreatePalette;
	PDD_GETSCANLINE          GetScanLine;
	PDD_MAPMEMORY            MapMemory;
} DD_CALLBACKS, *PDD_CALLBACKS;

enum
{
	DDHAL_CB32_CREATESURFACE        = 1<<1,
	DDHAL_CB32_SETCOLORKEY          = 1<<2,
	DDHAL_CB32_WAITFORVERTICALBLANK = 1<<4,
	DDHAL_CB32_CANCREATESURFACE     = 1<<5,
	DDHAL_CB32_CREATEPALETTE        = 1<<6,
	DDHAL_CB32_GETSCANLINE          = 1<<7,
	DDHAL_CB32_MAPMEMORY            = 1<<31,
};

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	DDSCAPS                  DDSCaps;
	DWORD                    dwTotal;
	DWORD                    dwFree;
	HRESULT                  ddRVal;
	PVOID                    GetAvailDriverMemory;
} DD_GETAVAILDRIVERMEMORYDATA, *PDD_GETAVAILDRIVERMEMORYDATA;
typedef DWORD (STDCALL *PDD_GETAVAILDRIVERMEMORY)(PDD_GETAVAILDRIVERMEMORYDATA);

DEFINE_GUID(GUID_MiscellaneousCallbacks, 0xEFD60CC0, 0x49E7, 0x11D0, 0x88, 0x9D, 0x00, 0xAA, 0x00, 0xBB, 0xB7, 0x6A);

typedef struct
{
	DWORD                    dwSize;
	DWORD                    dwFlags;
	PDD_GETAVAILDRIVERMEMORY GetAvailDriverMemory;
} DD_MISCELLANEOUSCALLBACKS, *PDD_MISCELLANEOUSCALLBACKS;

enum
{
	DDHAL_MISCCB32_GETAVAILDRIVERMEMORY = 1<<0,
};

typedef DWORD (STDCALL *PDD_ALPHABLT)(PDD_BLTDATA);

typedef struct
{
	DWORD                     dwFlags;
	PDD_DIRECTDRAW_LOCAL      lpDDLcl;
	PDD_SURFACE_LOCAL         lpDDSLcl;
	HRESULT                   ddRVal;
} DD_CREATESURFACEEXDATA, *PDD_CREATESURFACEEXDATA;
typedef DWORD (STDCALL *PDD_CREATESURFACEEX)(PDD_CREATESURFACEEXDATA);

typedef struct
{
	DWORD                     dwFlags;
	union
	{
		PDD_DIRECTDRAW_GLOBAL lpDD;
		DWORD_PTR             dwhContext;
	};
	LPDWORD                   lpdwStates;
	DWORD                     dwLength;
	HRESULT                   ddRVal;
} DD_GETDRIVERSTATEDATA, *PDD_GETDRIVERSTATEDATA;
typedef DWORD (STDCALL *PDD_GETDRIVERSTATE)(PDD_GETDRIVERSTATEDATA);

typedef struct
{
	DWORD                     dwFlags;
	PDD_DIRECTDRAW_LOCAL      pDDLcl;
	HRESULT                   ddRVal;
} DD_DESTROYDDLOCALDATA, *PDD_DESTROYDDLOCALDATA;
typedef DWORD (STDCALL *PDD_DESTROYDDLOCAL)(PDD_DESTROYDDLOCALDATA);

DEFINE_GUID(GUID_Miscellaneous2Callbacks, 0x406B2F00, 0x3E5A, 0x11D1, 0xB6, 0x40, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x6A);

typedef struct
{
	DWORD                     dwSize;
	DWORD                     dwFlags;
	PDD_ALPHABLT              AlphaBlt;
	PDD_CREATESURFACEEX       CreateSurfaceEx;
	PDD_GETDRIVERSTATE        GetDriverState;
	PDD_DESTROYDDLOCAL        DestroyDDLocal;
} DD_MISCELLANEOUS2CALLBACKS, *PDD_MISCELLANEOUS2CALLBACKS;

enum
{
	DDHAL_MISC2CB32_ALPHABLT        = 1<<0,
	DDHAL_MISC2CB32_CREATESURFACEEX = 1<<1,
	DDHAL_MISC2CB32_GETDRIVERSTATE  = 1<<2,
	DDHAL_MISC2CB32_DESTROYDDLOCAL  = 1<<3,
};

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL lpDD;
	PDD_SURFACE_LOCAL     lpDDSurface;
	HRESULT               ddRVal;
	PVOID                 FreeDriverMemory;
} DD_FREEDRIVERMEMORYDATA, *PDD_FREEDRIVERMEMORYDATA;
typedef DWORD (STDCALL *PDD_FREEDRIVERMEMORY)(PDD_FREEDRIVERMEMORYDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL lpDD;
	DWORD                 dwEnterExcl;
	DWORD                 dwReserved;
	HRESULT               ddRVal;
	PVOID                 SetExclusiveMode;
} DD_SETEXCLUSIVEMODEDATA, *PDD_SETEXCLUSIVEMODEDATA;
typedef DWORD (STDCALL *PDD_SETEXCLUSIVEMODE)(PDD_SETEXCLUSIVEMODEDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL lpDD;
	DWORD                 dwToGDI;
	DWORD                 dwReserved;
	HRESULT               ddRVal;
	PVOID                 FlipToGDISurface;
} DD_FLIPTOGDISURFACEDATA, *PDD_FLIPTOGDISURFACEDATA;
typedef DWORD (STDCALL *PDD_FLIPTOGDISURFACE)(PDD_FLIPTOGDISURFACEDATA);

DEFINE_GUID(GUID_NTCallbacks, 0x6FE9ECDE, 0xDF89, 0x11D1, 0x9D, 0xB0, 0x00, 0x60, 0x08, 0x27, 0x71, 0xBA);

typedef struct
{
	DWORD                 dwSize;
	DWORD                 dwFlags;
	PDD_FREEDRIVERMEMORY  FreeDriverMemory;
	PDD_SETEXCLUSIVEMODE  SetExclusiveMode;
	PDD_FLIPTOGDISURFACE  FlipToGDISurface;
} DD_NTCALLBACKS, *PDD_NTCALLBACKS;

enum
{
	DDHAL_NTCB32_FREEDRIVERMEMORY = 1<<0,
	DDHAL_NTCB32_SETEXCLUSIVEMODE = 1<<1,
	DDHAL_NTCB32_FLIPTOGDISURFACE = 1<<2,
};

/************************************************************************/
/* IDirectDrawPalette callbacks                                         */
/************************************************************************/

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	PDD_PALETTE_GLOBAL       lpDDPalette;
	HRESULT                  ddRVal;
	PVOID                    DestroyPalette;
} DD_DESTROYPALETTEDATA, *PDD_DESTROYPALETTEDATA;
typedef DWORD (STDCALL *PDD_PALCB_DESTROYPALETTE)(PDD_DESTROYPALETTEDATA);

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	PDD_PALETTE_GLOBAL       lpDDPalette;
	DWORD                    dwBase;
	DWORD                    dwNumEntries;
	LPPALETTEENTRY           lpEntries;
	HRESULT                  ddRVal;
	PVOID                    SetEntries;
} DD_SETENTRIESDATA, *PDD_SETENTRIESDATA;
typedef DWORD (STDCALL *PDD_PALCB_SETENTRIES)(PDD_SETENTRIESDATA);

typedef struct
{
	DWORD                    dwSize;
	DWORD                    dwFlags;
	PDD_PALCB_DESTROYPALETTE DestroyPalette;
	PDD_PALCB_SETENTRIES     SetEntries;
} DD_PALETTECALLBACKS, *PDD_PALETTECALLBACKS;

enum
{
	DDHAL_PALCB32_DESTROYPALETTE = 1<<0,
	DDHAL_PALCB32_SETENTRIES     = 1<<1,
};

/************************************************************************/
/* IDirectDrawVideoport callbacks                                       */
/************************************************************************/

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	LPDDVIDEOPORTDESC              lpDDVideoPortDesc;
	HRESULT                        ddRVal;
	PVOID                          CanCreateVideoPort;
} DD_CANCREATEVPORTDATA, *PDD_CANCREATEVPORTDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_CANCREATEVIDEOPORT)(PDD_CANCREATEVPORTDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	LPDDVIDEOPORTDESC              lpDDVideoPortDesc;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	HRESULT                        ddRVal;
	PVOID                          CreateVideoPort;
} DD_CREATEVPORTDATA, *PDD_CREATEVPORTDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_CREATEVIDEOPORT)(PDD_CREATEVPORTDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	PDD_SURFACE_LOCAL              lpSurfCurr;
	PDD_SURFACE_LOCAL              lpSurfTarg;
	HRESULT                        ddRVal;
	PVOID                          FlipVideoPort;
} DD_FLIPVPORTDATA, *PDD_FLIPVPORTDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_FLIP)(PDD_FLIPVPORTDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	LPDDPIXELFORMAT                lpddpfFormat;
	DWORD                          dwWidth;
	DWORD                          dwHeight;
	DWORD                          dwFlags;
	LPDDVIDEOPORTBANDWIDTH         lpBandwidth;
	HRESULT                        ddRVal;
	PVOID                          GetVideoPortBandwidth;
} DD_GETVPORTBANDWIDTHDATA, *PDD_GETVPORTBANDWIDTHDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_GETBANDWIDTH)(PDD_GETVPORTBANDWIDTHDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	DWORD                          dwFlags;
	LPDDPIXELFORMAT                lpddpfFormat;
	DWORD                          dwNumFormats;
	HRESULT                        ddRVal;
	PVOID                          GetVideoPortInputFormats;
} DD_GETVPORTINPUTFORMATDATA, *PDD_GETVPORTINPUTFORMATDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_GETINPUTFORMATS)(PDD_GETVPORTINPUTFORMATDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	DWORD                          dwFlags;
	LPDDPIXELFORMAT                lpddpfInputFormat;
	LPDDPIXELFORMAT                lpddpfOutputFormats;
	DWORD                          dwNumFormats;
	HRESULT                        ddRVal;
	PVOID                          GetVideoPortInputFormats;
} DD_GETVPORTOUTPUTFORMATDATA, *PDD_GETVPORTOUTPUTFORMATDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_GETOUTPUTFORMATS)(PDD_GETVPORTOUTPUTFORMATDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	BOOL                           bField;
	HRESULT                        ddRVal;
	PVOID                          GetVideoPortField;
} DD_GETVPORTFIELDDATA, *PDD_GETVPORTFIELDDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_GETFIELD)(PDD_GETVPORTFIELDDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	DWORD                          dwLine;
	HRESULT                        ddRVal;
	PVOID                          GetVideoPortLine;
} DD_GETVPORTLINEDATA, *PDD_GETVPORTLINEDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_GETLINE)(PDD_GETVPORTLINEDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	DWORD                          dwPortId;
	LPDDVIDEOPORTCONNECT           lpConnect;
	DWORD                          dwNumEntries;
	HRESULT                        ddRVal;
	PVOID                          GetVideoPortConnectInfo;
} DD_GETVPORTCONNECTDATA, *PDD_GETVPORTCONNECTDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_GETVPORTCONNECT)(PDD_GETVPORTCONNECTDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	HRESULT                        ddRVal;
	PVOID                          DestroyVideoPort;
} DD_DESTROYVPORTDATA, *PDD_DESTROYVPORTDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_DESTROYVPORT)(PDD_DESTROYVPORTDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	FLATPTR                        fpSurface;
	HRESULT                        ddRVal;
	PVOID                          GetVideoPortFlipStatus;
} DD_GETVPORTFLIPSTATUSDATA, *PDD_GETVPORTFLIPSTATUSDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_GETFLIPSTATUS)(PDD_GETVPORTFLIPSTATUSDATA);

#define DDRAWI_VPORTSTART          1
#define DDRAWI_VPORTSTOP           2
#define DDRAWI_VPORTUPDATE         3

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	PDD_SURFACE_INT               *lplpDDSurface;
	PDD_SURFACE_INT               *lplpDDVBISurface;
	LPDDVIDEOPORTINFO              lpVideoInfo;
	DWORD                          dwFlags;
	DWORD                          dwNumAutoflip;
	DWORD                          dwNumVBIAutoflip;
	HRESULT                        ddRVal;
	PVOID                          UpdateVideoPort;
} DD_UPDATEVPORTDATA, *PDD_UPDATEVPORTDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_UPDATE)(PDD_UPDATEVPORTDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	DWORD                          dwFlags;
	DWORD                          dwLine;
	DWORD                          dwTimeOut;
	HRESULT                        ddRVal;
	PVOID                          UpdateVideoPort;
} DD_WAITFORVPORTSYNCDATA, *PDD_WAITFORVPORTSYNCDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_WAITFORSYNC)(PDD_WAITFORVPORTSYNCDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	DWORD                          dwStatus;
	HRESULT                        ddRVal;
	PVOID                          GetVideoSignalStatus;
} DD_GETVPORTSIGNALDATA, *PDD_GETVPORTSIGNALDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_GETSIGNALSTATUS)(PDD_GETVPORTSIGNALDATA);

#define DDRAWI_VPORTGETCOLOR       1
#define DDRAWI_VPORTSETCOLOR       2

typedef struct
{
	PDD_DIRECTDRAW_LOCAL           lpDD;
	PDD_VIDEOPORT_LOCAL            lpVideoPort;
	DWORD                          dwFlags;
	LPDDCOLORCONTROL               lpColorData;
	HRESULT                        ddRVal;
	PVOID                          ColorControl;
} DD_VPORTCOLORDATA, *PDD_VPORTCOLORDATA;
typedef DWORD (STDCALL *PDD_VPORTCB_COLORCONTROL)(PDD_VPORTCOLORDATA);

DEFINE_GUID(GUID_VideoPortCallbacks, 0xEFD60CC1, 0x49E7, 0x11D0, 0x88, 0x9D, 0x00, 0xAA, 0x00, 0xBB, 0xB7, 0x6A);

typedef struct
{
	DWORD                          dwSize;
	DWORD                          dwFlags;
	PDD_VPORTCB_CANCREATEVIDEOPORT CanCreateVideoPort;
	PDD_VPORTCB_CREATEVIDEOPORT    CreateVideoPort;
	PDD_VPORTCB_FLIP               FlipVideoPort;
	PDD_VPORTCB_GETBANDWIDTH       GetVideoPortBandwidth;
	PDD_VPORTCB_GETINPUTFORMATS    GetVideoPortInputFormats;
	PDD_VPORTCB_GETOUTPUTFORMATS   GetVideoPortOutputFormats;
	PVOID                          Reserved;
	PDD_VPORTCB_GETFIELD           GetVideoPortField;
	PDD_VPORTCB_GETLINE            GetVideoPortLine;
	PDD_VPORTCB_GETVPORTCONNECT    GetVideoPortConnectInfo;
	PDD_VPORTCB_DESTROYVPORT       DestroyVideoPort;
	PDD_VPORTCB_GETFLIPSTATUS      GetVideoPortFlipStatus;
	PDD_VPORTCB_UPDATE             UpdateVideoPort;
	PDD_VPORTCB_WAITFORSYNC        WaitForVideoPortSync;
	PDD_VPORTCB_GETSIGNALSTATUS    GetVideoSignalStatus;
	PDD_VPORTCB_COLORCONTROL       ColorControl;
} DD_VIDEOPORTCALLBACKS, *PDD_VIDEOPORTCALLBACKS;

enum
{
	DDHAL_VPORT32_CANCREATEVIDEOPORT = 1<<0,
	DDHAL_VPORT32_CREATEVIDEOPORT    = 1<<1,
	DDHAL_VPORT32_FLIP               = 1<<2,
	DDHAL_VPORT32_GETBANDWIDTH       = 1<<3,
	DDHAL_VPORT32_GETINPUTFORMATS    = 1<<4,
	DDHAL_VPORT32_GETOUTPUTFORMATS   = 1<<5,
	DDHAL_VPORT32_GETFIELD           = 1<<7,
	DDHAL_VPORT32_GETLINE            = 1<<8,
	DDHAL_VPORT32_GETCONNECT         = 1<<9,
	DDHAL_VPORT32_DESTROY            = 1<<10,
	DDHAL_VPORT32_GETFLIPSTATUS      = 1<<11,
	DDHAL_VPORT32_UPDATE             = 1<<12,
	DDHAL_VPORT32_WAITFORSYNC        = 1<<13,
	DDHAL_VPORT32_GETSIGNALSTATUS    = 1<<14,
	DDHAL_VPORT32_COLORCONTROL       = 1<<15,
};

/************************************************************************/
/* IDirectDrawColorControl callbacks                                    */
/************************************************************************/

#define DDRAWI_GETCOLOR      1
#define DDRAWI_SETCOLOR      2

typedef struct
{
	PDD_DIRECTDRAW_GLOBAL    lpDD;
	PDD_SURFACE_LOCAL        lpDDSurface;
	LPDDCOLORCONTROL         lpColorData;
	DWORD                    dwFlags;
	HRESULT                  ddRVal;
	PVOID                    ColorControl;
} DD_COLORCONTROLDATA, *PDD_COLORCONTROLDATA;
typedef DWORD (STDCALL *PDD_COLORCB_COLORCONTROL)(PDD_COLORCONTROLDATA);

DEFINE_GUID(GUID_ColorControlCallbacks, 0xEFD60CC2, 0x49E7, 0x11D0, 0x88, 0x9D, 0x00, 0xAA, 0x00, 0xBB, 0xB7, 0x6A);

typedef struct
{
	DWORD                    dwSize;
	DWORD                    dwFlags;
	PDD_COLORCB_COLORCONTROL ColorControl;
} DD_COLORCONTROLCALLBACKS, *PDD_COLORCONTROLCALLBACKS;

enum
{
	DDHAL_COLOR_COLORCONTROL = 1<<0,
};

/************************************************************************/
/* IDirectDrawVideo callbacks                                           */
/************************************************************************/

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	DWORD                        dwNumGuids;
	GUID                        *lpGuids;
	HRESULT                      ddRVal;
} DD_GETMOCOMPGUIDSDATA, *PDD_GETMOCOMPGUIDSDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_GETGUIDS)(PDD_GETMOCOMPGUIDSDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	GUID                        *lpGuid;
	DWORD                        dwNumFormats;
	LPDDPIXELFORMAT              lpFormats;
	HRESULT                      ddRVal;
} DD_GETMOCOMPFORMATSDATA, *PDD_GETMOCOMPFORMATSDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_GETFORMATS)(PDD_GETMOCOMPFORMATSDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	PDD_MOTIONCOMP_LOCAL         lpMoComp;
	GUID                        *lpGuid;
	DWORD                        dwUncompWidth;
	DWORD                        dwUncompHeight;
	DDPIXELFORMAT                ddUncompPixelFormat;
	LPVOID                       lpData;
	DWORD                        dwDataSize;
	HRESULT                      ddRVal;
} DD_CREATEMOCOMPDATA, *PDD_CREATEMOCOMPDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_CREATE)(PDD_CREATEMOCOMPDATA);

typedef struct
{
	DWORD                        dwSize;
	DWORD                        dwNumCompBuffers;
	DWORD                        dwWidthToCreate;
	DWORD                        dwHeightToCreate;
	DWORD                        dwBytesToAllocate;
	DDSCAPS2                     ddCompCaps;
	DDPIXELFORMAT                ddPixelFormat;
} DDCOMPBUFFERINFO, *PDDCOMPBUFFERINFO;

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	GUID                        *lpGuid;
	DWORD                        dwWidth;
	DWORD                        dwHeight;
	DDPIXELFORMAT                ddPixelFormat;
	DWORD                        dwNumTypesCompBuffs;
	PDDCOMPBUFFERINFO            lpCompBuffInfo;
	HRESULT                      ddRVal;
} DD_GETMOCOMPCOMPBUFFDATA, *PDD_GETMOCOMPCOMPBUFFDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_GETCOMPBUFFINFO)(PDD_GETMOCOMPCOMPBUFFDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	GUID                        *lpGuid;
	DWORD                        dwWidth;
	DWORD                        dwHeight;
	DDPIXELFORMAT                ddPixelFormat;
	DWORD                        dwScratchMemAlloc;
	HRESULT                      ddRVal;
} DD_GETINTERNALMOCOMPDATA, *PDD_GETINTERNALMOCOMPDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_GETINTERNALINFO)(PDD_GETINTERNALMOCOMPDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	PDD_MOTIONCOMP_LOCAL         lpMoComp;
	PDD_SURFACE_LOCAL            lpDestSurface;
	DWORD                        dwInputDataSize;
	LPVOID                       lpInputData;
	DWORD                        dwOutputDataSize;
	LPVOID                       lpOutputData;
	HRESULT                      ddRVal;
} DD_BEGINMOCOMPFRAMEDATA, *PDD_BEGINMOCOMPFRAMEDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_BEGINFRAME)(PDD_BEGINMOCOMPFRAMEDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	PDD_MOTIONCOMP_LOCAL         lpMoComp;
	LPVOID                       lpInputData;
	DWORD                        dwInputDataSize;
	HRESULT                      ddRVal;
} DD_ENDMOCOMPFRAMEDATA, *PDD_ENDMOCOMPFRAMEDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_ENDFRAME)(PDD_ENDMOCOMPFRAMEDATA);

typedef struct
{
	DWORD                        dwSize;
	PDD_SURFACE_LOCAL            lpCompSurface;
	DWORD                        dwDataOffset;
	DWORD                        dwDataSize;
	LPVOID                       lpPrivate;
} DDMOCOMPBUFFERINFO, *PDDMOCOMPBUFFERINFO;

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	PDD_MOTIONCOMP_LOCAL         lpMoComp;
	DWORD                        dwNumBuffers;
	PDDMOCOMPBUFFERINFO          lpBufferInfo;
	DWORD                        dwFunction;
	LPVOID                       lpInputData;
	DWORD                        dwInputDataSize;
	LPVOID                       lpOutputData;
	DWORD                        dwOutputDataSize;
	HRESULT                      ddRVal;
} DD_RENDERMOCOMPDATA, *PDD_RENDERMOCOMPDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_RENDER)(PDD_RENDERMOCOMPDATA);

#define DDMCQUERY_READ 1

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	PDD_MOTIONCOMP_LOCAL         lpMoComp;
	PDD_SURFACE_LOCAL            lpSurface;
	DWORD                        dwFlags;
	HRESULT                      ddRVal;
} DD_QUERYMOCOMPSTATUSDATA, *PDD_QUERYMOCOMPSTATUSDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_QUERYSTATUS)(PDD_QUERYMOCOMPSTATUSDATA);

typedef struct
{
	PDD_DIRECTDRAW_LOCAL         lpDD;
	PDD_MOTIONCOMP_LOCAL         lpMoComp;
	HRESULT                      ddRVal;
} DD_DESTROYMOCOMPDATA, *PDD_DESTROYMOCOMPDATA;
typedef DWORD (STDCALL *PDD_MOCOMPCB_DESTROY)(PDD_DESTROYMOCOMPDATA);

DEFINE_GUID(GUID_MotionCompCallbacks, 0xB1122B40, 0x5DA5, 0x11D1, 0x8F, 0xCF, 0x00, 0xC0, 0x4F, 0xC2, 0x9B, 0x4E);

typedef struct
{
	DWORD                        dwSize;
	DWORD                        dwFlags;
	PDD_MOCOMPCB_GETGUIDS        GetMoCompGuids;
	PDD_MOCOMPCB_GETFORMATS      GetMoCompFormats;
	PDD_MOCOMPCB_CREATE          CreateMoComp;
	PDD_MOCOMPCB_GETCOMPBUFFINFO GetMoCompBuffInfo;
	PDD_MOCOMPCB_GETINTERNALINFO GetInternalMoCompInfo;
	PDD_MOCOMPCB_BEGINFRAME      BeginMoCompFrame;
	PDD_MOCOMPCB_ENDFRAME        EndMoCompFrame;
	PDD_MOCOMPCB_RENDER          RenderMoComp;
	PDD_MOCOMPCB_QUERYSTATUS     QueryMoCompStatus;
	PDD_MOCOMPCB_DESTROY         DestroyMoComp;
} DD_MOTIONCOMPCALLBACKS;
typedef DD_MOTIONCOMPCALLBACKS *PDD_MOTIONCOMPCALLBACKS;

enum
{
	DDHAL_MOCOMP32_GETGUIDS        = 1<<0,
	DDHAL_MOCOMP32_GETFORMATS      = 1<<1,
	DDHAL_MOCOMP32_CREATE          = 1<<2,
	DDHAL_MOCOMP32_GETCOMPBUFFINFO = 1<<3,
	DDHAL_MOCOMP32_GETINTERNALINFO = 1<<4,
	DDHAL_MOCOMP32_BEGINFRAME      = 1<<5,
	DDHAL_MOCOMP32_ENDFRAME        = 1<<6,
	DDHAL_MOCOMP32_RENDER          = 1<<7,
	DDHAL_MOCOMP32_QUERYSTATUS     = 1<<8,
	DDHAL_MOCOMP32_DESTROY         = 1<<9,
};

/************************************************************************/
/* D3D buffer callbacks                                                 */
/************************************************************************/

typedef struct
{
	DWORD                     dwSize;
	DWORD                     dwFlags;
	PDD_CANCREATESURFACE      CanCreateD3DBuffer;
	PDD_CREATESURFACE         CreateD3DBuffer;
	PDD_SURFCB_DESTROYSURFACE DestroyD3DBuffer;
	PDD_SURFCB_LOCK           LockD3DBuffer;
	PDD_SURFCB_UNLOCK         UnlockD3DBuffer;
} DD_D3DBUFCALLBACKS, *PDD_D3DBUFCALLBACKS;

/************************************************************************/
/* DdGetDriverInfo callback                                             */
/************************************************************************/

typedef struct
{
	// Input:
	PVOID   dhpdev;
	DWORD   dwSize;
	DWORD   dwFlags;
	GUID    guidInfo;
	DWORD   dwExpectedSize;
	PVOID   lpvData;
	// Output:
	DWORD   dwActualSize;
	HRESULT ddRVal;
} DD_GETDRIVERINFODATA, *PDD_GETDRIVERINFODATA;
typedef DWORD (STDCALL *PDD_GETDRIVERINFO)(PDD_GETDRIVERINFODATA);

/************************************************************************/
/* Driver info structures                                               */
/************************************************************************/

typedef struct
{
	DWORD   dwSize;
	DWORD   dwCaps;
	DWORD   dwCaps2;
	DWORD   dwCKeyCaps;
	DWORD   dwFXCaps;
	DWORD   dwFXAlphaCaps;
	DWORD   dwPalCaps;
	DWORD   dwSVCaps;
	DWORD   dwAlphaBltConstBitDepths;
	DWORD   dwAlphaBltPixelBitDepths;
	DWORD   dwAlphaBltSurfaceBitDepths;
	DWORD   dwAlphaOverlayConstBitDepths;
	DWORD   dwAlphaOverlayPixelBitDepths;
	DWORD   dwAlphaOverlaySurfaceBitDepths;
	DWORD   dwZBufferBitDepths;
	DWORD   dwVidMemTotal;
	DWORD   dwVidMemFree;
	DWORD   dwMaxVisibleOverlays;
	DWORD   dwCurrVisibleOverlays;
	DWORD   dwNumFourCCCodes;
	DWORD   dwAlignBoundarySrc;
	DWORD   dwAlignSizeSrc;
	DWORD   dwAlignBoundaryDest;
	DWORD   dwAlignSizeDest;
	DWORD   dwAlignStrideAlign;
	DWORD   dwRops[DD_ROP_SPACE];
	DDSCAPS ddsCaps;
	DWORD   dwMinOverlayStretch;
	DWORD   dwMaxOverlayStretch;
	DWORD   dwMinLiveVideoStretch;
	DWORD   dwMaxLiveVideoStretch;
	DWORD   dwMinHwCodecStretch;
	DWORD   dwMaxHwCodecStretch;
	DWORD   dwReserved1;
	DWORD   dwReserved2;
	DWORD   dwReserved3;
	DWORD   dwSVBCaps;
	DWORD   dwSVBCKeyCaps;
	DWORD   dwSVBFXCaps;
	DWORD   dwSVBRops[DD_ROP_SPACE];
	DWORD   dwVSBCaps;
	DWORD   dwVSBCKeyCaps;
	DWORD   dwVSBFXCaps;
	DWORD   dwVSBRops[DD_ROP_SPACE];
	DWORD   dwSSBCaps;
	DWORD   dwSSBCKeyCaps;
	DWORD   dwSSBFXCaps;
	DWORD   dwSSBRops[DD_ROP_SPACE];
	DWORD   dwMaxVideoPorts;
	DWORD   dwCurrVideoPorts;
	DWORD   dwSVBCaps2;
} DDNTCORECAPS, *PDDNTCORECAPS;

typedef struct
{
	DWORD               dwSize;
	VIDEOMEMORYINFO     vmiData;
	DDNTCORECAPS        ddCaps;
	PDD_GETDRIVERINFO   GetDriverInfo;
	DWORD               dwFlags;
	PVOID               lpD3DGlobalDriverData;
	PVOID               lpD3DHALCallbacks;
	PDD_D3DBUFCALLBACKS lpD3DBufCallbacks;
} DD_HALINFO, *PDD_HALINFO;

DEFINE_GUID(GUID_NonLocalVidMemCaps, 0x86C4FA80, 0x8D84, 0x11D0, 0x94, 0xE8, 0x00, 0xC0, 0x4F, 0xC3, 0x41, 0x37);

typedef struct
{
	DWORD dwSize;
	DWORD dwNLVBCaps;
	DWORD dwNLVBCaps2;
	DWORD dwNLVBCKeyCaps;
	DWORD dwNLVBFXCaps;
	DWORD dwNLVBRops[DD_ROP_SPACE];
} DD_NONLOCALVIDMEMCAPS, *PDD_NONLOCALVIDMEMCAPS;

DEFINE_GUID(GUID_DDMoreSurfaceCaps, 0x3B8A0466, 0xF269, 0x11D1, 0x88, 0x0B, 0x00, 0xC0, 0x4F, 0xD9, 0x30, 0xC5);

typedef struct
{
	DWORD         dwSize;
	DDSCAPSEX     ddsCapsMore;
	struct
	{
		DDSCAPSEX ddsCapsEx;
		DDSCAPSEX ddsCapsExAlt;
	} ddsExtendedHeapRestrictions[1];
} DD_MORESURFACECAPS, *PDD_MORESURFACECAPS;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __DD_INCLUDED__ */
