/* DirectMusicBand Private Include
 *
 * Copyright (C) 2024 Yuxuan Shui for CodeWeavers
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

#include "dmusici.h"
#include "dmusicf.h"

extern HRESULT create_dmband(REFIID riid, void **ret_iface);
extern HRESULT band_connect_to_collection(IDirectMusicBand *iface, IDirectMusicCollection *collection);
extern HRESULT band_send_messages(IDirectMusicBand *iface, IDirectMusicPerformance *performance,
        IDirectMusicGraph *graph, MUSIC_TIME time, DWORD track_id);
HRESULT band_add_instrument(IDirectMusicBand *iface, DMUS_IO_INSTRUMENT *instrument);
