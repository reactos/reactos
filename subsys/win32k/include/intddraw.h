#ifndef _INT_W32k_DDRAW
#define _INT_W32k_DDRAW

#define GDI_OBJECT_TYPE_DIRECTDRAW    0x00600000
#define GDI_OBJECT_TYPE_DD_SURFACE    0x00610000
#define GDI_OBJECT_TYPE_DD_VIDEOPORT  0x00620000
#define GDI_OBJECT_TYPE_DD_PALETTE    0x00630000
#define GDI_OBJECT_TYPE_DD_CLIPPER    0x00640000
#define GDI_OBJECT_TYPE_DD_MOTIONCOMP 0x00650000

typedef struct
{
	DD_SURFACE_LOCAL Local;
	DD_SURFACE_MORE More;
	DD_SURFACE_GLOBAL Global;
	DD_ATTACHLIST AttachList;
	DD_ATTACHLIST AttachListFrom;
	BOOL bComplete;
} DD_SURFACE, *PDD_SURFACE;

typedef struct
{
	DD_DIRECTDRAW_LOCAL Local;
	DD_DIRECTDRAW_GLOBAL Global;
	// Drv callbacks
	PGD_GETDIRECTDRAWINFO           DrvGetDirectDrawInfo;
	PGD_DISABLEDIRECTDRAW           DrvDisableDirectDraw;
	// DD callbacks
	PDD_CREATESURFACE               DdCreateSurface;
	PDD_SETCOLORKEY                 DdDrvSetColorKey; // ?????
	PDD_WAITFORVERTICALBLANK        DdWaitForVerticalBlank;
	PDD_CANCREATESURFACE            DdCanCreateSurface;
	PDD_CREATEPALETTE               DdCreatePalette;
	PDD_GETSCANLINE                 DdGetScanLine;
	PDD_MAPMEMORY                   DdMapMemory;
	// Surface callbacks
	PDD_SURFCB_DESTROYSURFACE	    DdDestroySurface;
	PDD_SURFCB_FLIP                 DdFlip;
	PDD_SURFCB_SETCLIPLIST          DdSetClipList;
	PDD_SURFCB_LOCK                 DdLock;
	PDD_SURFCB_UNLOCK               DdUnlock;
	PDD_SURFCB_BLT                  DdBlt;
	PDD_SURFCB_SETCOLORKEY          DdSetColorKey;
	PDD_SURFCB_ADDATTACHEDSURFACE   DdAddAttachedSurface;
	PDD_SURFCB_GETBLTSTATUS         DdGetBltStatus;
	PDD_SURFCB_GETFLIPSTATUS        DdGetFlipStatus;
	PDD_SURFCB_UPDATEOVERLAY        DdUpdateOverlay;
	PDD_SURFCB_SETOVERLAYPOSITION   DdSetOverlayPosition;
	PDD_SURFCB_SETPALETTE           DdSetPalette;
	// Palette callbacks
	PDD_PALCB_DESTROYPALETTE        DdDestroyPalette;
	PDD_PALCB_SETENTRIES            DdSetEntries;
	// D3D Device context callbacks
	PD3DNTHAL_CONTEXTCREATECB       D3dContextCreate;
	PD3DNTHAL_CONTEXTDESTROYCB      D3dContextDestroy;
	// D3D Buffer callbacks
	PDD_CANCREATESURFACE            DdCanCreateD3DBuffer;
	PDD_CREATESURFACE               DdCreateD3DBuffer;
	PDD_SURFCB_DESTROYSURFACE       DdDestroyD3DBuffer;
	PDD_SURFCB_LOCK                 DdLockD3DBuffer;
	PDD_SURFCB_UNLOCK               DdUnlockD3DBuffer;
} DD_DIRECTDRAW, *PDD_DIRECTDRAW;

BOOL FASTCALL DD_Cleanup(PDD_DIRECTDRAW pDD);
BOOL FASTCALL DDSURF_Cleanup(PDD_SURFACE pDDSurf);

#endif /* _INT_W32k_DDRAW */
