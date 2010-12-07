/*
 * ReactOS Generic Framebuffer display driver directdraw interface
 *
 * Copyright (C) 2006 Magnus Olsen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Here we put in all 2d api for directdraw and redirect some of them to GDI api */

#include "framebuf.h"


DWORD CALLBACK
DdCanCreateSurface( PDD_CANCREATESURFACEDATA pccsd)
{
    /* We do not needit if we need it here it is 
       PPDEV ppdev=(PPDEV)pccsd->lpDD->dhpdev;
    */

    pccsd->ddRVal = DD_OK;
    /* We do not support 3d buffer in video ram so we fail here */
    if (pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
    {
        pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
    }

    /* We do not support texture yet so we fail here */
    if (pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSD_TEXTURESTAGE)
    {
        pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
    }

    /* Check if another pixel format or not, we fail for now */
    if (pccsd->bIsDifferentPixelFormat)
     {
        /* We do not support FOUR_CC */
        pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
    }

    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdCreateSurface( PDD_CREATESURFACEDATA pcsd )
{
    PDD_SURFACE_LOCAL   lpSurfaceLocal;
    PDD_SURFACE_GLOBAL   lpSurfaceGlobal;
    LPDDSURFACEDESC     lpSurfaceDesc;

    /* Driver DdCreateSurface should only support to create one surface not more that */
    if (pcsd->dwSCnt != 1)
    {
        pcsd->ddRVal = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    lpSurfaceLocal = pcsd->lplpSList[0];
    lpSurfaceGlobal = lpSurfaceLocal->lpGbl;
    lpSurfaceDesc   = pcsd->lpDDSurfaceDesc;


    /* ReactOS / Windows NT is supposed to guarantee that ddpfSurface.dwSize is valid */
    if ( lpSurfaceGlobal->ddpfSurface.dwSize == sizeof(DDPIXELFORMAT) )
    {
        pcsd->ddRVal = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    /* We do not have any private surface data for dx */
    lpSurfaceGlobal->dwReserved1 = 0;


    /* Support diffent Bpp deep */
    if (lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4)
    {
        lpSurfaceGlobal->lPitch = ((lpSurfaceGlobal->wWidth/2) + 31) & ~31;
    }
    else if (lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
    {
        lpSurfaceGlobal->lPitch = (lpSurfaceGlobal->wWidth + 31) & ~31;
    }
    else
    {
        lpSurfaceGlobal->lPitch = lpSurfaceGlobal->wWidth*(lpSurfaceDesc->ddpfPixelFormat.dwRGBBitCount/8);
    }

    if ( lpSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        /* We maybe should alloc it with EngAlloc
           for now we trusting ddraw alloc it        */
        lpSurfaceGlobal->fpVidMem = 0;
    }
    else
    {
        /* We maybe should alloc it with EngAlloc
            for now we trusting ddraw alloc it        */

        lpSurfaceGlobal->fpVidMem = 0;

        if ( (lpSurfaceLocal->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE) )
        {
            if (lpSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
            {
                lpSurfaceGlobal->lPitch = ((lpSurfaceDesc->ddpfPixelFormat.dwRGBBitCount * lpSurfaceGlobal->wWidth+31)/32)*4;  //make it DWORD aligned
                lpSurfaceGlobal->dwUserMemSize = lpSurfaceGlobal->wWidth * lpSurfaceGlobal->wHeight * lpSurfaceGlobal->lPitch;
                lpSurfaceGlobal->fpVidMem = DDHAL_PLEASEALLOC_USERMEM;
            }
        } 
        else
        {
            lpSurfaceGlobal->dwBlockSizeX = lpSurfaceGlobal->wWidth;
            lpSurfaceGlobal->dwBlockSizeY = lpSurfaceGlobal->wHeight;
            lpSurfaceGlobal->fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE;
        }
    }

    pcsd->lpDDSurfaceDesc->lPitch = lpSurfaceGlobal->lPitch;
    pcsd->lpDDSurfaceDesc->dwFlags |= DDSD_PITCH;

    pcsd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK
DdMapMemory(PDD_MAPMEMORYDATA lpMapMemory)
{
    
    VIDEO_SHARE_MEMORY              ShareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  ShareMemoryInformation;
    DWORD                           ReturnedDataLength;
    PPDEV                           ppdev = (PPDEV) lpMapMemory->lpDD->dhpdev;

    lpMapMemory->ddRVal = DD_OK;

    if (lpMapMemory->bMap)
    {
        ShareMemory.ProcessHandle = lpMapMemory->hProcess;
        ShareMemory.RequestedVirtualAddress = 0;
        ShareMemory.ViewOffset = 0;
        ShareMemory.ViewSize = ppdev->ScreenHeight * ppdev->ScreenDelta;

        if (EngDeviceIoControl(ppdev->hDriver,
                       IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                       &ShareMemory,
                       sizeof(VIDEO_SHARE_MEMORY),
                       &ShareMemoryInformation,
                       sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                       &ReturnedDataLength))
        {
            lpMapMemory->ddRVal = DDERR_GENERIC;
        }
        else
        {
            lpMapMemory->fpProcess = (FLATPTR) ShareMemoryInformation.VirtualAddress;
        }
    }
    else
    {
        ShareMemory.ProcessHandle           = lpMapMemory->hProcess;
        ShareMemory.ViewOffset              = 0;
        ShareMemory.ViewSize                = 0;
        ShareMemory.RequestedVirtualAddress = (VOID*) lpMapMemory->fpProcess;

        if (EngDeviceIoControl(ppdev->hDriver,
                       IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                       &ShareMemory,
                       sizeof(VIDEO_SHARE_MEMORY),
                       NULL,
                       0,
                       &ReturnedDataLength))
        {
            lpMapMemory->ddRVal = DDERR_GENERIC;
        }
    }

    
    return(DDHAL_DRIVER_HANDLED);
}
