/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2020 Jacek Caban for CodeWeavers
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

#if 0
#pragma makedep implib
#endif

#include "windows.h"
#include "ocidl.h"
#include "propidl.h"
#include "initguid.h"
#include "wincodec.h"
#include "wincodecsdk.h"

DEFINE_GUID(IID_IMILBitmap,0xb1784d3f,0x8115,0x4763,0x13,0xaa,0x32,0xed,0xdb,0x68,0x29,0x4a);
DEFINE_GUID(IID_IMILBitmapSource,0x7543696a,0xbc8d,0x46b0,0x5f,0x81,0x8d,0x95,0x72,0x89,0x72,0xbe);
DEFINE_GUID(IID_IMILBitmapLock,0xa67b2b53,0x8fa1,0x4155,0x8f,0x64,0x0c,0x24,0x7a,0x8f,0x84,0xcd);
DEFINE_GUID(IID_IMILBitmapScaler,0xa767b0f0,0x1c8c,0x4aef,0x56,0x8f,0xad,0xf9,0x6d,0xcf,0xd5,0xcb);
DEFINE_GUID(IID_IMILFormatConverter,0x7e2a746f,0x25c5,0x4851,0xb3,0xaf,0x44,0x3b,0x79,0x63,0x9e,0xc0);
DEFINE_GUID(IID_IMILPalette,0xca8e206f,0xf22c,0x4af7,0x6f,0xba,0x7b,0xed,0x5e,0xb1,0xc9,0x2f);
