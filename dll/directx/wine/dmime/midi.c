/*
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

#include "dmusic_midi.h"
#include "dmime_private.h"
#include "dmusic_band.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmime);

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) RtlUshortByteSwap(x)
#define GET_BE_DWORD(x) RtlUlongByteSwap(x)
#endif

struct midi_event
{
    MUSIC_TIME delta_time;
    BYTE status;
    union
    {
        struct
        {
            BYTE data[2];
        };
        struct
        {
            BYTE meta_type;
            ULONG tempo;
        };
    };
};

struct midi_seqtrack_item
{
    struct list entry;
    DMUS_IO_SEQ_ITEM item;
};

struct midi_parser
{
    IDirectMusicTrack8 *seqtrack;
    ULONG seqtrack_items_count;
    struct list seqtrack_items;
    /* Track the initial note on event generated for a note that is currently on. NULL if the note is off. */
    struct midi_seqtrack_item *note_states[128 * 16];

    IDirectMusicTrack *chordtrack;
    IDirectMusicTrack *bandtrack;
    IDirectMusicTrack *tempotrack;
    MUSIC_TIME time;
    IStream *stream;
    DWORD division;
};

enum meta_event_type
{
    MIDI_META_SEQUENCE_NUMBER = 0x00,
    MIDI_META_TEXT_EVENT = 0x01,
    MIDI_META_COPYRIGHT_NOTICE = 0x02,
    MIDI_META_TRACK_NAME = 0x03,
    MIDI_META_INSTRUMENT_NAME = 0x04,
    MIDI_META_LYRIC = 0x05,
    MIDI_META_MARKER = 0x06,
    MIDI_META_CUE_POINT = 0x07,
    MIDI_META_CHANNEL_PREFIX_ASSIGNMENT = 0x20,
    MIDI_META_END_OF_TRACK = 0x2f,
    MIDI_META_SET_TEMPO = 0x51,
    MIDI_META_SMPTE_OFFSET = 0x54,
    MIDI_META_TIME_SIGNATURE = 0x58,
    MIDI_META_KEY_SIGNATURE = 0x59,
    MIDI_META_SEQUENCER_SPECIFIC = 0x7f,
};

static HRESULT stream_read_at_most(IStream *stream, void *buffer, ULONG size, ULONG *bytes_left)
{
    HRESULT hr;
    ULONG read = 0;
    if (size > *bytes_left) hr = IStream_Read(stream, buffer, *bytes_left, &read);
    else hr = IStream_Read(stream, buffer, size, &read);
    if (hr != S_OK) return hr;
    *bytes_left -= read;
    if (read < size) return S_FALSE;
    return S_OK;
}

static HRESULT read_variable_length_number(IStream *stream, DWORD *out, ULONG *bytes_left)
{
    BYTE byte;
    HRESULT hr = S_OK;

    *out = 0;
    do
    {
        hr = stream_read_at_most(stream, &byte, 1, bytes_left);
        if (hr != S_OK) return hr;
        *out = (*out << 7) | (byte & 0x7f);
    } while (byte & 0x80);
    return S_OK;
}

static HRESULT read_midi_event(IStream *stream, struct midi_event *event, BYTE *last_status, ULONG *bytes_left)
{
    BYTE byte, status_type;
    DWORD length;
    BYTE data[3];
    LARGE_INTEGER offset;
    HRESULT hr = S_OK;
    DWORD delta_time;

    if ((hr = read_variable_length_number(stream, &delta_time, bytes_left)) != S_OK) return hr;
    event->delta_time = delta_time;

    if ((hr = stream_read_at_most(stream, &byte, 1, bytes_left)) != S_OK) return hr;

    if (byte & 0x80)
    {
        event->status = *last_status = byte;
        if ((hr = stream_read_at_most(stream, &byte, 1, bytes_left)) != S_OK) return hr;
    }
    else event->status = *last_status;

    if (event->status == MIDI_META)
    {
        event->meta_type = byte;

        if ((hr = read_variable_length_number(stream, &length, bytes_left)) != S_OK) return hr;

        switch (event->meta_type)
        {
        case MIDI_META_SET_TEMPO:
            if (length != 3)
            {
                ERR("Invalid MIDI meta event length %lu for set tempo event.\n", length);
                return E_FAIL;
            }
            if (FAILED(hr = stream_read_at_most(stream, data, 3, bytes_left))) return hr;
            event->tempo = (data[0] << 16) | (data[1] << 8) | data[2];
            break;
        default:
            if (*bytes_left < length) return S_FALSE;
            offset.QuadPart = length;
            if (FAILED(hr = IStream_Seek(stream, offset, STREAM_SEEK_CUR, NULL))) return hr;
            FIXME("MIDI meta event type %#02x, length %lu, time +%lu. not supported\n",
                    event->meta_type, length, delta_time);
            *bytes_left -= length;
            event->tempo = 0;
        }
        TRACE("MIDI meta event type %#02x, length %lu, time +%lu\n", event->meta_type, length, delta_time);
        return S_OK;
    }
    else if (event->status == MIDI_SYSEX1 || event->status == MIDI_SYSEX2)
    {
        if (byte & 0x80)
        {
            if ((hr = read_variable_length_number(stream, &length, bytes_left)) != S_OK) return hr;
            length = length << 8 | (byte & 0x7f);
        }
        else length = byte;

        if (*bytes_left < length) return S_FALSE;
        offset.QuadPart = length;
        if (FAILED(hr = IStream_Seek(stream, offset, STREAM_SEEK_CUR, NULL))) return hr;
        *bytes_left -= length;
        FIXME("MIDI sysex event type %#02x, length %lu, time +%lu. not supported\n", event->status,
                length, delta_time);
        return S_OK;
    }

    status_type = event->status & 0xf0;
    event->data[0] = byte;
    if (status_type != MIDI_PROGRAM_CHANGE && status_type != MIDI_CHANNEL_PRESSURE &&
            (hr = stream_read_at_most(stream, &byte, 1, bytes_left)) != S_OK)
        return hr;
    event->data[1] = byte;
    TRACE("MIDI event status %#02x, data: %#02x, %#02x, time +%lu\n", event->status, event->data[0],
            event->data[1], delta_time);

    return S_OK;
}

static HRESULT midi_parser_handle_set_tempo(struct midi_parser *parser, struct midi_event *event)
{
    DMUS_TEMPO_PARAM tempo;
    MUSIC_TIME dmusic_time = (ULONGLONG)parser->time * DMUS_PPQ / parser->division;
    HRESULT hr;

    if (!parser->tempotrack && FAILED(hr = CoCreateInstance(&CLSID_DirectMusicTempoTrack, NULL, CLSCTX_INPROC_SERVER,
                                              &IID_IDirectMusicTrack, (void **)&parser->tempotrack)))
        return hr;

    tempo.mtTime = dmusic_time;
    tempo.dblTempo = 60 * 1000000.0 / event->tempo;
    TRACE("Adding tempo at time %lu, tempo %f\n", dmusic_time, tempo.dblTempo);
    return IDirectMusicTrack_SetParam(parser->tempotrack, &GUID_TempoParam, dmusic_time, &tempo);
}

static HRESULT midi_parser_handle_program_change(struct midi_parser *parser, struct midi_event *event)
{
    HRESULT hr;
    DMUS_IO_INSTRUMENT instrument;
    IDirectMusicBand *band;
    DMUS_BAND_PARAM band_param;
    MUSIC_TIME dmusic_time = (ULONGLONG)parser->time * DMUS_PPQ / parser->division;
    instrument.dwPChannel = event->status & 0xf;
    instrument.dwFlags = DMUS_IO_INST_PATCH;
    instrument.dwPatch = event->data[0];
    if (FAILED(hr = CoCreateInstance(&CLSID_DirectMusicBand, NULL, CLSCTX_INPROC_SERVER,
                       &IID_IDirectMusicBand, (void **)&band)))
        return hr;
    hr = band_add_instrument(band, &instrument);

    if (SUCCEEDED(hr))
    {
        TRACE("Adding band at time %lu\n", dmusic_time);
        band_param.pBand = band;
        band_param.mtTimePhysical = dmusic_time;
        hr = IDirectMusicTrack_SetParam(parser->bandtrack, &GUID_BandParam, dmusic_time, &band_param);
    }
    else WARN("Failed to add instrument to band\n");

    IDirectMusicBand_Release(band);
    return hr;
}

static HRESULT midi_parser_handle_note_on_off(struct midi_parser *parser, struct midi_event *event)
{
    BYTE new_velocity = (event->status & 0xf0) == MIDI_NOTE_OFF ? 0 : event->data[1]; /* DirectMusic doesn't have noteoff velocity */
    BYTE note = event->data[0], channel = event->status & 0xf;
    DWORD index = (DWORD)channel * 128 + note;
    MUSIC_TIME dmusic_time;
    struct midi_seqtrack_item *note_state = parser->note_states[index];
    DMUS_IO_SEQ_ITEM *seq_item;
    struct midi_seqtrack_item *item;

    /* Testing shows there are 2 cases to deal with here:
     *
     * 1. Got note on when the note is already on, generate a NOTE event with
     *    new velocity and duration 1.
     * 2. Got note on when the note is off, generate a NOTE event that lasts
     *    until the next note off event, intervening note on event doesn't matter.
     */
    if (new_velocity)
    {
        TRACE("Adding note event at time %lu, note %u, velocity %u\n", parser->time, note, new_velocity);

        dmusic_time = (ULONGLONG)parser->time * DMUS_PPQ / parser->division;

        item = calloc(1, sizeof(struct midi_seqtrack_item));
        if (!item) return E_OUTOFMEMORY;

        seq_item = &item->item;
        seq_item->mtTime = dmusic_time;
        seq_item->mtDuration = 1;
        seq_item->dwPChannel = channel;
        seq_item->bStatus = MIDI_NOTE_ON;
        seq_item->bByte1 = note;
        seq_item->bByte2 = new_velocity;
        list_add_tail(&parser->seqtrack_items, &item->entry);
        parser->seqtrack_items_count++;

        if (!note_state) parser->note_states[index] = item;
    }
    else if (note_state)
    {
        note_state->item.mtDuration = (ULONGLONG)parser->time * DMUS_PPQ / parser->division -
                                      note_state->item.mtTime;
        if (note_state->item.mtDuration == 0) note_state->item.mtDuration = 1;

        TRACE("Note off at time %lu, note %u, duration %ld\n", parser->time, note, note_state->item.mtDuration);

        parser->note_states[index] = NULL;
    }

    return S_OK;
}

static HRESULT midi_parser_handle_control(struct midi_parser *parser, struct midi_event *event)
{
    struct midi_seqtrack_item *item;
    DMUS_IO_SEQ_ITEM *seq_item;
    MUSIC_TIME dmusic_time = (ULONGLONG)parser->time * DMUS_PPQ / parser->division;

    if ((item = calloc(1, sizeof(struct midi_seqtrack_item))) == NULL) return E_OUTOFMEMORY;

    seq_item = &item->item;
    seq_item->mtTime = dmusic_time;
    seq_item->mtDuration = 0;
    seq_item->dwPChannel = event->status & 0xf;
    seq_item->bStatus = event->status & 0xf0;
    seq_item->bByte1 = event->data[0];
    seq_item->bByte2 = event->data[1];
    list_add_tail(&parser->seqtrack_items, &item->entry);
    parser->seqtrack_items_count++;

    return S_OK;
}

static int midi_seqtrack_item_compare(const void *a, const void *b)
{
    const DMUS_IO_SEQ_ITEM *item_a = a, *item_b = b;
    return item_a->mtTime - item_b->mtTime;
}

static HRESULT midi_parser_parse(struct midi_parser *parser, IDirectMusicSegment8 *segment)
{
    WORD i = 0;
    HRESULT hr;
    MUSIC_TIME music_length = 0;
    DMUS_IO_SEQ_ITEM *seq_items = NULL;
    struct midi_seqtrack_item *item;

    TRACE("(%p, %p): semi-stub\n", parser, segment);

    for (i = 0;; i++)
    {
        BYTE magic[4] = {0}, last_status = 0;
        DWORD length_be;
        ULONG length;
        ULONG read = 0;
        struct midi_event event = {0};

        TRACE("Start parsing track %u\n", i);
        if ((hr = IStream_Read(parser->stream, magic, sizeof(magic), &read)) != S_OK) break;
        if (read < sizeof(magic)) break;
        if (memcmp(magic, "MTrk", 4) != 0) break;

        if ((hr = IStream_Read(parser->stream, &length_be, sizeof(length_be), &read)) != S_OK)
            break;
        if (read < sizeof(length_be)) break;
        length = GET_BE_DWORD(length_be);
        TRACE("Track %u, length %lu bytes\n", i, length);

        while ((hr = read_midi_event(parser->stream, &event, &last_status, &length)) == S_OK)
        {
            parser->time += event.delta_time;
            if (event.status == 0xff && event.meta_type == MIDI_META_SET_TEMPO)
                hr = midi_parser_handle_set_tempo(parser, &event);
            else
            {
                switch (event.status & 0xf0)
                {
                case MIDI_NOTE_ON:
                case MIDI_NOTE_OFF:
                    hr = midi_parser_handle_note_on_off(parser, &event);
                    break;
                case MIDI_CHANNEL_PRESSURE:
                case MIDI_PITCH_BEND_CHANGE:
                case MIDI_CONTROL_CHANGE:
                    hr = midi_parser_handle_control(parser, &event);
                    break;
                case MIDI_PROGRAM_CHANGE:
                    hr = midi_parser_handle_program_change(parser, &event);
                    break;
                default:
                    FIXME("Unhandled MIDI event type %#02x at time +%lu\n", event.status, parser->time);
                    break;
                }
            }
            if (FAILED(hr)) break;
        }

        if (FAILED(hr)) break;

        TRACE("End of track %u\n", i);
        if (parser->time > music_length) music_length = parser->time;
        parser->time = 0;
        memset(parser->note_states, 0, sizeof(parser->note_states));
    }
    if (FAILED(hr)) return hr;

    TRACE("End of file\n");

    if ((seq_items = calloc(parser->seqtrack_items_count, sizeof(DMUS_IO_SEQ_ITEM))) == NULL)
        return E_OUTOFMEMORY;

    i = 0;
    LIST_FOR_EACH_ENTRY(item, &parser->seqtrack_items, struct midi_seqtrack_item, entry)
        seq_items[i++] = item->item;
    sequence_track_set_items(parser->seqtrack, seq_items, parser->seqtrack_items_count);
    qsort(seq_items, parser->seqtrack_items_count, sizeof(DMUS_IO_SEQ_ITEM), midi_seqtrack_item_compare);

    music_length = (ULONGLONG)music_length * DMUS_PPQ / parser->division + 1;
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment8_SetLength(segment, music_length);
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment8_InsertTrack(segment, parser->bandtrack, 0xffff);
    if (SUCCEEDED(hr)) hr = IDirectMusicSegment8_InsertTrack(segment, parser->chordtrack, 0xffff);
    if (SUCCEEDED(hr) && parser->tempotrack)
        hr = IDirectMusicSegment8_InsertTrack(segment, parser->tempotrack, 0xffff);
    if (SUCCEEDED(hr))
        hr = IDirectMusicSegment8_InsertTrack(segment, (IDirectMusicTrack *)parser->seqtrack, 0xffff);

    return hr;
}

static void midi_parser_destroy(struct midi_parser *parser)
{
    struct midi_seqtrack_item *item, *next_item;
    IStream_Release(parser->stream);
    if (parser->bandtrack) IDirectMusicTrack_Release(parser->bandtrack);
    if (parser->chordtrack) IDirectMusicTrack_Release(parser->chordtrack);
    if (parser->tempotrack) IDirectMusicTrack_Release(parser->tempotrack);
    if (parser->seqtrack) IDirectMusicTrack_Release(parser->seqtrack);
    LIST_FOR_EACH_ENTRY_SAFE(item, next_item, &parser->seqtrack_items, struct midi_seqtrack_item, entry)
    {
        list_remove(&item->entry);
        free(item);
    }
    free(parser);
}

static HRESULT midi_parser_new(IStream *stream, struct midi_parser **out_parser)
{
    LARGE_INTEGER offset;
    DWORD length;
    HRESULT hr;
    WORD format, number_of_tracks, division;
    struct midi_parser *parser;

    *out_parser = NULL;

    /* Skip over the 'MThd' magic number. */
    offset.QuadPart = 4;
    if (FAILED(hr = IStream_Seek(stream, offset, STREAM_SEEK_SET, NULL))) return hr;

    if (FAILED(hr = IStream_Read(stream, &length, sizeof(length), NULL))) return hr;
    length = GET_BE_DWORD(length);

    if (FAILED(hr = IStream_Read(stream, &format, sizeof(format), NULL))) return hr;
    format = GET_BE_WORD(format);

    if (FAILED(hr = IStream_Read(stream, &number_of_tracks, sizeof(number_of_tracks), NULL)))
        return hr;

    number_of_tracks = GET_BE_WORD(number_of_tracks);
    if (FAILED(hr = IStream_Read(stream, &division, sizeof(division), NULL))) return hr;
    division = GET_BE_WORD(division);
    if (division & 0x8000)
    {
        WARN("SMPTE time division not implemented yet\n");
        return DMUS_E_UNSUPPORTED_STREAM;
    }

    TRACE("MIDI format %u, %u tracks, division %u\n", format, number_of_tracks, division);

    parser = calloc(1, sizeof(struct midi_parser));
    if (!parser) return E_OUTOFMEMORY;
    list_init(&parser->seqtrack_items);
    IStream_AddRef(stream);
    parser->stream = stream;
    parser->division = division;
    hr = CoCreateInstance(&CLSID_DirectMusicBandTrack, NULL, CLSCTX_INPROC_SERVER,
            &IID_IDirectMusicTrack, (void **)&parser->bandtrack);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance(&CLSID_DirectMusicChordTrack, NULL, CLSCTX_INPROC_SERVER,
                &IID_IDirectMusicTrack, (void **)&parser->chordtrack);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance(&CLSID_DirectMusicSeqTrack, NULL, CLSCTX_INPROC_SERVER,
                &IID_IDirectMusicTrack, (void **)&parser->seqtrack);

    if (FAILED(hr)) midi_parser_destroy(parser);
    else *out_parser = parser;
    return hr;
}

HRESULT parse_midi(IStream *stream, IDirectMusicSegment8 *segment)
{
    struct midi_parser *parser;
    HRESULT hr;

    if (FAILED(hr = midi_parser_new(stream, &parser))) return hr;
    hr = midi_parser_parse(parser, segment);
    midi_parser_destroy(parser);
    return hr;
}
