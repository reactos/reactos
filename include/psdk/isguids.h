/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#ifndef _ISGUIDS_H_
#define _ISGUIDS_H_

DEFINE_GUID(CLSID_InternetShortcut, 0xfbf23b40,0xe3f0,0x101b,0x84,0x88,0x00,0xaa,0x00,0x3e,0x56,0xf8);

DEFINE_GUID(IID_IUniformResourceLocatorA, 0xfbf23b80,0xe3f0,0x101b,0x84,0x88,0x00,0xaa,0x00,0x3e,0x56,0xf8);
DEFINE_GUID(IID_IUniformResourceLocatorW, 0xcabb0da0,0xda57,0x11cf,0x99,0x74,0x00,0x20,0xaf,0xd7,0x97,0x62);
#define IID_IUniformResourceLocator WINELIB_NAME_AW(IID_IUniformResourceLocator)

#endif
