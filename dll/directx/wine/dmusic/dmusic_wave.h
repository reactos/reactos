/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stddef.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "objbase.h"
#include "dmusici.h"

struct soundfont;
struct chunk_entry;

extern HRESULT wave_create(IDirectMusicObject **ret_iface);
extern HRESULT wave_create_from_soundfont(struct soundfont *soundfont, UINT index, IDirectMusicObject **out);
extern HRESULT wave_create_from_chunk(IStream *stream, struct chunk_entry *parent, IDirectMusicObject **out);
extern HRESULT wave_download_to_port(IDirectMusicObject *iface, IDirectMusicPortDownload *port, DWORD *id);
extern HRESULT wave_download_to_dsound(IDirectMusicObject *iface, IDirectSound *dsound,
        IDirectSoundBuffer **ret_iface);
extern HRESULT wave_get_duration(IDirectMusicObject *iface, REFERENCE_TIME *duration);
