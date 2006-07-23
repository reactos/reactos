/* $Id: surface_hel.c 21519 2006-04-08 21:05:49Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/soft/surface.c
 * PURPOSE:              DirectDraw Software Implementation 
 * PROGRAMMER:           Magnus Olsen
 *
 */

#include "rosdraw.h"


extern  DDPIXELFORMAT pixelformats[];
extern DWORD pixelformatsCount;

/*
 * Direct Draw Callbacks 
 */
 
DWORD CALLBACK  HelDdDestroyDriver(LPDDHAL_DESTROYDRIVERDATA lpDestroyDriver)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdCreateSurface(LPDDHAL_CREATESURFACEDATA lpCreateSurface)
{
   	DX_STUB;
}


DWORD CALLBACK  HelDdSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKey)
{
   	DX_STUB;
}


DWORD CALLBACK  HelDdSetMode(LPDDHAL_SETMODEDATA SetMode)
{	
	// change the resolution using normal WinAPI function
	DEVMODE mode;  
	mode.dmSize = sizeof(DEVMODE);
	mode.dmPelsWidth = 0;
	mode.dmPelsHeight = 0;
	mode.dmBitsPerPel = 0;

	/* FIXME use emuate display to get the res */

	/* 320x200   15, 16, 32 */
	if (SetMode->dwModeIndex == 0x10d)  
	{    
		mode.dmPelsWidth = 320;
		mode.dmPelsHeight = 200;
        mode.dmBitsPerPel = 15;	    
	}

	if (SetMode->dwModeIndex == 0x10e)  
	{    
		mode.dmPelsWidth = 320;
		mode.dmPelsHeight = 200;
        mode.dmBitsPerPel = 16;	    
	}

	if (SetMode->dwModeIndex == 0x10f)  
	{    
		mode.dmPelsWidth = 320;
		mode.dmPelsHeight = 200;
        mode.dmBitsPerPel = 32;	    
	}
	
	/* 640x400   8 */
	if (SetMode->dwModeIndex == 0x100)  
	{    
		mode.dmPelsWidth = 640;
		mode.dmPelsHeight = 400;
        mode.dmBitsPerPel = 8;	    
	}

    /* 640x480   8, 15, 16 , 32*/
	if (SetMode->dwModeIndex == 0x101)  
	{    
		mode.dmPelsWidth = 640;
		mode.dmPelsHeight = 480;
        mode.dmBitsPerPel = 8;	    
	}

	if (SetMode->dwModeIndex == 0x110)  
	{    
		mode.dmPelsWidth = 640;
		mode.dmPelsHeight = 480;
        mode.dmBitsPerPel = 15;	    
	}

	if (SetMode->dwModeIndex == 0x111)  
	{    
		mode.dmPelsWidth = 640;
		mode.dmPelsHeight = 480;
        mode.dmBitsPerPel = 16;	    
	}
	
	if (SetMode->dwModeIndex == 0x112)  
	{    
		mode.dmPelsWidth = 640;
		mode.dmPelsHeight = 480;
        mode.dmBitsPerPel = 32;	    
	}

	/* 800x600  4, 8, 15, 16 , 32*/
	if (SetMode->dwModeIndex == 0x102)  
	{    
		mode.dmPelsWidth = 800;
		mode.dmPelsHeight = 600;
        mode.dmBitsPerPel = 4;	    
	}

	if (SetMode->dwModeIndex == 0x103)  
	{    
		mode.dmPelsWidth = 800;
		mode.dmPelsHeight = 600;
        mode.dmBitsPerPel = 8;	    
	}

	if (SetMode->dwModeIndex == 0x113)  
	{    
		mode.dmPelsWidth = 800;
		mode.dmPelsHeight = 600;
        mode.dmBitsPerPel = 15;	    
	}


	if (SetMode->dwModeIndex == 0x114)  
	{    
		mode.dmPelsWidth = 800;
		mode.dmPelsHeight = 600;
        mode.dmBitsPerPel = 16;	    
	}

	if (SetMode->dwModeIndex == 0x115)  
	{    
		mode.dmPelsWidth = 800;
		mode.dmPelsHeight = 600;
        mode.dmBitsPerPel = 32;	    
	}

    /* 1024x768 8, 15, 16 , 32*/

	if (SetMode->dwModeIndex == 0x104)  
	{    
		mode.dmPelsWidth = 1024;
		mode.dmPelsHeight = 768;
        mode.dmBitsPerPel = 4;	    
	}

	if (SetMode->dwModeIndex == 0x105)  
	{    
		mode.dmPelsWidth = 1024;
		mode.dmPelsHeight = 768;
        mode.dmBitsPerPel = 8;	    
	}

	if (SetMode->dwModeIndex == 0x116)  
	{    
		mode.dmPelsWidth = 1024;
		mode.dmPelsHeight = 768;
        mode.dmBitsPerPel = 15;	    
	}

	if (SetMode->dwModeIndex == 0x117)  
	{    
		mode.dmPelsWidth = 1024;
		mode.dmPelsHeight = 768;
        mode.dmBitsPerPel = 16;	    
	}
	
	if (SetMode->dwModeIndex == 0x118)  
	{    
		mode.dmPelsWidth = 1024;
		mode.dmPelsHeight = 768;
        mode.dmBitsPerPel = 32;	    
	}
         
	//mode.dmDisplayFrequency = dwRefreshRate;
	mode.dmFields = 0;

    DX_STUB_str("in hel");

	if(mode.dmPelsWidth != 0)
		mode.dmFields |= DM_PELSWIDTH;
	if(mode.dmPelsHeight != 0)
		mode.dmFields |= DM_PELSHEIGHT;
	if( mode.dmBitsPerPel != 0)
		mode.dmFields |= DM_BITSPERPEL;
    /*
	if(dwRefreshRate)
		mode.dmFields |= DM_DISPLAYFREQUENCY;
    */
	
	DX_WINDBG_trace_res((int)mode.dmPelsWidth, (int)mode.dmPelsHeight, (int)mode.dmBitsPerPel );
		
	if (ChangeDisplaySettings(&mode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
		DX_STUB_str("FAIL");
		SetMode->ddRVal = DDERR_UNSUPPORTEDMODE;		
	}
	else
	{
		DX_STUB_str("OK");
		SetMode->ddRVal = DD_OK;
	}
			
	return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK  HelDdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurface)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdCreatePalette(LPDDHAL_CREATEPALETTEDATA lpCreatePalette)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdGetScanLine(LPDDHAL_GETSCANLINEDATA lpGetScanLine)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdSetExclusiveMode(LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveMode)
{
	 DX_WINDBG_trace();
	 DX_STUB_str("Not implement yet, return DD_OK for not bsod\n");
	 lpSetExclusiveMode->ddRVal = DD_OK;
	 
	 return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK  HelDdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurface)
{
   	DX_STUB;
}

