#include "rosdraw.h"

HRESULT Hel_DirectDraw_Initialize (LPDIRECTDRAW7 iface)
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	This->HELMemoryAvilable = HEL_GRAPHIC_MEMORY_MAX;

    This->mCallbacks.HELDD.dwFlags = DDHAL_CB32_DESTROYDRIVER;
    This->mCallbacks.HELDD.DestroyDriver = HelDdDestroyDriver;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CREATESURFACE; 
    This->mCallbacks.HELDD.CreateSurface = HelDdCreateSurface;
    
    // DDHAL_CB32_
    //This->mCallbacks.HELDD.SetColorKey = HelDdSetColorKey;
   
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_SETMODE;
    This->mCallbacks.HELDD.SetMode = HelDdSetMode;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_WAITFORVERTICALBLANK;     
    This->mCallbacks.HELDD.WaitForVerticalBlank = HelDdWaitForVerticalBlank;
        
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CANCREATESURFACE;
    This->mCallbacks.HELDD.CanCreateSurface = HelDdCanCreateSurface;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CREATEPALETTE;
    This->mCallbacks.HELDD.CreatePalette = HelDdCreatePalette;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_GETSCANLINE;
    This->mCallbacks.HELDD.GetScanLine = HelDdGetScanLine;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_SETEXCLUSIVEMODE;
    This->mCallbacks.HELDD.SetExclusiveMode = HelDdSetExclusiveMode;

    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_FLIPTOGDISURFACE;
    This->mCallbacks.HELDD.FlipToGDISurface = HelDdFlipToGDISurface;
   
	return DD_OK;
}
