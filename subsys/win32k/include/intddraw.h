#ifndef _INT_W32k_DDRAW
#define _INT_W32k_DDRAW

#include <ddkernel.h>

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
	PGD_GETDIRECTDRAWINFO            DrvGetDirectDrawInfo; 	
	PGD_DISABLEDIRECTDRAW            DrvDisableDirectDraw;	

	// DD callbacks
	DD_CALLBACKS                     DD;

	// Surface callbacks
	DD_SURFACECALLBACKS              Surf;

	// Palette callbacks
	DD_PALETTECALLBACKS              Pal;

	// HAL 
	DD_HALINFO                       Hal;

	// Color Control Callback 
	DD_COLORCONTROLCALLBACKS         Color;

	// D3DHAL_CALLBACKS
	// D3DHAL_CALLBACKS                 D3dHal;
	// D3DHAL_CALLBACKS3                D3dHal3;
	// D3DHAL_D3DEXTENDEDCAPS           D3dHal3Ext;

	// Heap Callback
	PDD_GETHEAPALIGNMENTDATA         HeapData;
	
	// Kernel Callback
	DD_KERNELCALLBACKS               Kernel;
    DDKERNELCAPS                     KernelCaps;

	// Miscellaneous Callback
	DD_MISCELLANEOUSCALLBACKS        Misc;


    
	// NT-based Callback 
	PDD_FLIPTOGDISURFACE             DdFlipToGDISurface; 
	PDD_FREEDRIVERMEMORY             DdFreeDriverMemory; 
	PDD_SETEXCLUSIVEMODE             DdSetExclusiveMode; 
	// Motion Compensation
    PDD_MOCOMPCB_BEGINFRAME          DdMoCompBeginFrame; 
    PDD_MOCOMPCB_CREATE              DdMoCompCreate; 
	PDD_MOCOMPCB_DESTROY             DdMoCompDestroy; 
	PDD_MOCOMPCB_ENDFRAME            DdMoCompEndFrame;
	PDD_MOCOMPCB_GETCOMPBUFFINFO     DdMoCompGetBuffInfo; 
	PDD_MOCOMPCB_GETFORMATS          DdMoCompGetFormats;
	PDD_MOCOMPCB_GETGUIDS            DdMoCompGetGuids; 
	PDD_MOCOMPCB_GETINTERNALINFO     DdMoCompGetInternalInfo; 
	PDD_MOCOMPCB_QUERYSTATUS         DdMoCompQueryStatus; 
	PDD_MOCOMPCB_RENDER              DdMoCompRender; 
	// Video Port Callback 
    PDD_VPORTCB_CANCREATEVIDEOPORT   DdVideoPortCanCreate;
    PDD_VPORTCB_COLORCONTROL         DdVideoPortColorControl;
    PDD_VPORTCB_CREATEVIDEOPORT      DdVideoPortCreate;
    PDD_VPORTCB_DESTROYVPORT         DdVideoPortDestroy;
    PDD_VPORTCB_FLIP                 DdVideoPortFlip;
    PDD_VPORTCB_GETBANDWIDTH         DdVideoPortGetBandwidth;
    PDD_VPORTCB_GETVPORTCONNECT      DdVideoPortGetConnectInfo;
    PDD_VPORTCB_GETFIELD             DdVideoPortGetField;
    PDD_VPORTCB_GETFLIPSTATUS        DdVideoPortGetFlipStatus;
    PDD_VPORTCB_GETINPUTFORMATS      DdVideoPortGetInputFormats;
    PDD_VPORTCB_GETLINE              DdVideoPortGetLine;
    PDD_VPORTCB_GETOUTPUTFORMATS     DdVideoPortGetOutputFormats;
    PDD_VPORTCB_GETSIGNALSTATUS      DdVideoPortGetSignalStatus;
    PDD_VPORTCB_UPDATE               DdVideoPortUpdate;
    PDD_VPORTCB_WAITFORSYNC          DdVideoPortWaitForSync;
    // Notify Callback 
    //LPDD_NOTIFYCALLBACK NotifyCallback
	

	
	
	           
} DD_DIRECTDRAW, *PDD_DIRECTDRAW;

BOOL INTERNAL_CALL DD_Cleanup(PVOID pDD);
BOOL INTERNAL_CALL DDSURF_Cleanup(PVOID pDDSurf);

#endif /* _INT_W32k_DDRAW */
