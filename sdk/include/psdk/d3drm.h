/*
 * Copyright (C) 2005 Peter Berg Larsen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __D3DRM_H__
#define __D3DRM_H__

#include <ddraw.h>
/* #include <d3drmobj.h> */


/* Direct3DRM Object CLSID */
DEFINE_GUID(CLSID_CDirect3DRM,              0x4516ec41, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);

/* Direct3DRM Interface GUIDs */
DEFINE_GUID(IID_IDirect3DRM,                0x2bc49361, 0x8327, 0x11cf, 0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRM2,               0x4516ecc8, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRM3,               0x4516ec83, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);

#endif /* __D3DRM_H__ */
