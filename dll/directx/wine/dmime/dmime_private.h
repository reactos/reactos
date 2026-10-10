/* DirectMusicInteractiveEngine Private Include
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
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

#ifndef __WINE_DMIME_PRIVATE_H
#define __WINE_DMIME_PRIVATE_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "winreg.h"
#include "objbase.h"

#include "dmusici.h"
#include "dmusicf.h"
#include "dmusics.h"
#include "dmusicc.h"

#include "dmobject.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicAudioPathImpl IDirectMusicAudioPathImpl;
struct midi_parser;

/*****************************************************************************
 * ClassFactory
 */
extern HRESULT create_dmperformance(REFIID riid, void **ret_iface);
extern HRESULT create_dmsegment(REFIID riid, void **ret_iface);
extern HRESULT create_dmsegmentstate(REFIID riid, void **ret_iface);
extern HRESULT create_dmgraph(REFIID riid, void **ret_iface);
extern HRESULT create_dmaudiopath(REFIID riid, void **ret_iface);
extern HRESULT create_dmaudiopath_config(REFIID riid, void **ret_iface);

extern HRESULT create_dmlyricstrack(REFIID riid, void **ret_iface);
extern HRESULT create_dmmarkertrack(REFIID riid, void **ret_iface);
extern HRESULT create_dmparamcontroltrack(REFIID riid, void **ret_iface);
extern HRESULT create_dmsegtriggertrack(REFIID riid, void **ret_iface);
extern HRESULT create_dmseqtrack(REFIID riid, void **ret_iface);
extern HRESULT create_dmsysextrack(REFIID riid, void **ret_iface);
extern HRESULT create_dmtempotrack(REFIID riid, void **ret_iface);
extern HRESULT create_dmtimesigtrack(REFIID riid, void **ret_iface);
extern HRESULT create_dmwavetrack(REFIID riid, void **ret_iface);

/* Parse a MIDI file. Note the stream might still be modified even when this function fails. */
extern HRESULT parse_midi(IStream *stream, IDirectMusicSegment8 *segment);

extern void set_audiopath_perf_pointer(IDirectMusicAudioPath*,IDirectMusicPerformance8*);
extern void set_audiopath_dsound_buffer(IDirectMusicAudioPath*,IDirectSoundBuffer*);
extern void set_audiopath_primary_dsound_buffer(IDirectMusicAudioPath*,IDirectSoundBuffer*);

extern HRESULT segment_state_create(IDirectMusicSegment *segment, MUSIC_TIME start_time,
        IDirectMusicPerformance8 *performance, IDirectMusicSegmentState **ret_iface);
extern HRESULT segment_state_play(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance);
extern HRESULT segment_state_tick(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance);
extern HRESULT segment_state_stop(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance);
extern HRESULT segment_state_end_play(IDirectMusicSegmentState *iface, IDirectMusicPerformance8 *performance);
extern BOOL segment_state_has_segment(IDirectMusicSegmentState *iface, IDirectMusicSegment *segment);
extern BOOL segment_state_has_track(IDirectMusicSegmentState *iface, DWORD track_id);

extern HRESULT wave_track_create_from_chunk(IStream *stream, struct chunk_entry *parent,
        IDirectMusicTrack8 **ret_iface);

extern void sequence_track_set_items(IDirectMusicTrack8 *track, DMUS_IO_SEQ_ITEM *items, unsigned int count);

extern HRESULT performance_get_dsound(IDirectMusicPerformance8 *iface, IDirectSound **dsound);
extern HRESULT performance_send_segment_start(IDirectMusicPerformance8 *iface, MUSIC_TIME music_time,
        IDirectMusicSegmentState *state);
extern HRESULT performance_send_segment_tick(IDirectMusicPerformance8 *iface, MUSIC_TIME music_time,
        IDirectMusicSegmentState *state);
extern HRESULT performance_send_segment_end(IDirectMusicPerformance8 *iface, MUSIC_TIME music_time,
        IDirectMusicSegmentState *state, BOOL abort);

HRESULT path_config_get_audio_path_params(IUnknown *iface, WAVEFORMATEX *format, DSBUFFERDESC *desc, DMUS_PORTPARAMS *params);

/*****************************************************************************
 * Auxiliary definitions
 */
typedef struct _DMUS_PRIVATE_TEMPO_ITEM {
  struct list entry; /* for listing elements */
  DMUS_IO_TEMPO_ITEM item;
} DMUS_PRIVATE_TEMPO_ITEM, *LPDMUS_PRIVATE_TEMPO_ITEM;

typedef struct _DMUS_PRIVATE_TEMPO_PLAY_STATE {
  DWORD dummy;
} DMUS_PRIVATE_TEMPO_PLAY_STATE, *LPDMUS_PRIVATE_TEMPO_PLAY_STATE;

/*****************************************************************************
 * Misc.
 */

#endif	/* __WINE_DMIME_PRIVATE_H */
