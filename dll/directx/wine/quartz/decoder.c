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

HRESULT mpeg_audio_codec_create(IUnknown *outer, IUnknown **out)
{
    static const GUID CLSID_wg_mpeg_audio_decoder = {0xc9f285f8,0x4380,0x4121,{0x97,0x1f,0x49,0xa9,0x53,0x16,0xc2,0x7b}};
    return CoCreateInstance(&CLSID_wg_mpeg_audio_decoder, outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)out);
}

HRESULT mpeg_video_codec_create(IUnknown *outer, IUnknown **out)
{
    static const GUID CLSID_wg_mpeg_video_decoder = {0x5ed2e5f6,0xbf3e,0x4180,{0x83,0xa4,0x48,0x47,0xcc,0x5b,0x4e,0xa3}};
    return CoCreateInstance(&CLSID_wg_mpeg_video_decoder, outer, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)out);
}
