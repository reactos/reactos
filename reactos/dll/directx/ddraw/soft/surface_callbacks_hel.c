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

DWORD CALLBACK HelDdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA pccsd)
{
    DWORD count;
    
     // FIXME check the HAL pixelformat table if it exists 

     // FIXME check how big the surface in byte and report it can be create or not 
     // if we got egunt with HEL memmory 

       
     // HEL only support 16bits & 15bits Z-Buffer
    if ((pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) && 
        (pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {        
        pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        
        if (DDSD_ZBUFFERBITDEPTH & pccsd->lpDDSurfaceDesc->dwFlags)
        {            
            if (pccsd->lpDDSurfaceDesc->dwZBufferBitDepth == 16)
            {
                pccsd->ddRVal = DD_OK;
            }
        }
        else
        {            
            if (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwZBufferBitDepth == 16)
            {
                pccsd->ddRVal = DD_OK;
            }
        }                        
        return DDHAL_DRIVER_HANDLED;
    }

    // Check diffent pixel format 
    if (pccsd->bIsDifferentPixelFormat)
    {
        //if(pccsd->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
        if(pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & 0)
        {                                    
            switch (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC)
            {
                //case FOURCC_YUV422:       
                case 0:       
                    // FIXME check if display is 8bmp or not if it return DDERR_INVALIDPIXELFORMAT
                    pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 16;
                    pccsd->ddRVal = DD_OK;                
                    return DDHAL_DRIVER_HANDLED;

                default:                
                    pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
                    return DDHAL_DRIVER_HANDLED;
            }

         }
         else if((pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_TEXTURE))
         {             
             for(count=0;count< pixelformatsCount ;count++)
             {
                 if (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags !=  pixelformats->dwFlags)
                 {
                     continue;
                 }

                 if (!(pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & (DDPF_YUV | DDPF_FOURCC)))
                 {
                     if (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != pixelformats->dwRGBBitCount )                     
                     {
                       continue;
                     }
                 }

                 if (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB)
                 {
                    if ((pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask != pixelformats->dwRBitMask) ||
                        (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask != pixelformats->dwGBitMask) ||
                        (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask != pixelformats->dwBBitMask) ||
                        (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwRGBAlphaBitMask != pixelformats->dwRGBAlphaBitMask))
                    { 
                       continue;
                    }
                 }

                 if (pccsd->lpDDSurfaceDesc->dwFlags & DDPF_YUV)	
                 {
                    if ((pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC != pixelformats->dwFourCC) ||
                        (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwYUVBitCount != pixelformats->dwYUVBitCount) ||
                        (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwYBitMask != pixelformats->dwYBitMask) ||
                        (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwUBitMask != pixelformats->dwUBitMask) ||
                        (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwVBitMask != pixelformats->dwVBitMask) ||
                        (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwYUVAlphaBitMask != pixelformats->dwYUVAlphaBitMask))
                        {
                           continue;
                        }
                 }
                 else if (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
                 {
    
                    if (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC != pixelformats->dwFourCC)
                    {
                        continue;
                    }
                }                 
                if (pccsd->lpDDSurfaceDesc->dwFlags & DDPF_ZPIXELS)
                {
                  if (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwRGBZBitMask != pixelformats->dwRGBZBitMask)
                  {
                     continue;
                  }
                }
                 
                pccsd->ddRVal = DD_OK;                
                return DDHAL_DRIVER_HANDLED;
             } 

             // for did not found a pixel format 
             pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;                
             return DDHAL_DRIVER_HANDLED;
          } 
    } 
    
    // no diffent pixel format was found so we return OK
    pccsd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;   
}

DWORD CALLBACK HelDdCreateSurface(LPDDHAL_CREATESURFACEDATA  lpCreateSurface)
{
	DX_STUB;
}
