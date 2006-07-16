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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* Here we put in all 2d api for directdraw and redirect some of them to GDI api */

#include "framebuf.h"


DWORD CALLBACK 
DdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA pccsd)
{
	 	  	  	 	
	 /* We do not support 3d buffer so we fail here */
	 if ((pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) && 
		(pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
	 {
		pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
        return DDHAL_DRIVER_HANDLED;	
	 }


	 /* Check if another pixel format or not, we fail for now */
	 if (pccsd->bIsDifferentPixelFormat)
     {
		/* check the fourcc diffent FOURCC, but we only support BMP for now */
		//if(pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
        //{   	 
		//	/* We do not support other pixel format */
		//	switch (pccsd->lpDDSurfaceDesc->ddpfPixelFormat.dwFourCC)
		//	{         
		//		default:                
		//			pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
		//			return DDHAL_DRIVER_HANDLED;
		//	}
		//}
		// /* check the texture support, we do not support testure for now */
		//else if((pccsd->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_TEXTURE))
		//{
		//	/* We do not support texture surface */
		//	pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
		//	return DDHAL_DRIVER_HANDLED;            
		//}

		/* Fail */
		pccsd->ddRVal = DDERR_INVALIDPIXELFORMAT;
		return DDHAL_DRIVER_HANDLED;
    }

	 pccsd->ddRVal = DD_OK;    
	 return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK 
DdCreateSurface(PDD_CREATESURFACEDATA lpCreateSurface)
{
	lpCreateSurface->ddRVal = DDERR_GENERIC;
    return DDHAL_DRIVER_NOTHANDLED;
}

