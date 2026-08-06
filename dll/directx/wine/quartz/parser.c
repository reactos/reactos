/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#include "quartz_private.h"

HRESULT avi_splitter_create(IUnknown *outer, IUnknown **out)
{
    static const GUID CLSID_wg_avi_splitter = {0x272bfbfb,0x50d0,0x4078,{0xb6,0x00,0x1e,0x95,0x9c,0x30,0x13,0x37}};
    return CoCreateInstance(&CLSID_wg_avi_splitter, outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)out);
}

HRESULT mpeg1_splitter_create(IUnknown *outer, IUnknown **out)
{
    static const GUID CLSID_wg_mpeg1_splitter = {0xa8edbf98,0x2442,0x42c5,{0x85,0xa1,0xab,0x05,0xa5,0x80,0xdf,0x53}};
    return CoCreateInstance(&CLSID_wg_mpeg1_splitter, outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)out);
}

HRESULT wave_parser_create(IUnknown *outer, IUnknown **out)
{
    static const GUID CLSID_wg_wave_parser = {0x3f839ec7,0x5ea6,0x49e1,{0x80,0xc2,0x1e,0xa3,0x00,0xf8,0xb0,0xe0}};
    return CoCreateInstance(&CLSID_wg_wave_parser, outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)out);
}
