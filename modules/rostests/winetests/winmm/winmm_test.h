/*
 * Copyright (c) 2002 Francois Gouget
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

#ifndef WAVE_FORMAT_48M08
#define WAVE_FORMAT_48M08      0x00001000    /* 48     kHz, Mono,   8-bit  */
#define WAVE_FORMAT_48S08      0x00002000    /* 48     kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_48M16      0x00004000    /* 48     kHz, Mono,   16-bit */
#define WAVE_FORMAT_48S16      0x00008000    /* 48     kHz, Stereo, 16-bit */
#define WAVE_FORMAT_96M08      0x00010000    /* 96     kHz, Mono,   8-bit  */
#define WAVE_FORMAT_96S08      0x00020000    /* 96     kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_96M16      0x00040000    /* 96     kHz, Mono,   16-bit */
#define WAVE_FORMAT_96S16      0x00080000    /* 96     kHz, Stereo, 16-bit */
#endif

#ifndef DRV_QUERYDEVICEINTERFACE
#define DRV_QUERYDEVICEINTERFACE     (DRV_RESERVED + 12)
#endif
#ifndef DRV_QUERYDEVICEINTERFACESIZE
#define DRV_QUERYDEVICEINTERFACESIZE (DRV_RESERVED + 13)
#endif

static const unsigned int win_formats[][4] = {
    {0,                  8000,  8, 1},
    {0,                  8000,  8, 2},
    {0,                  8000, 16, 1},
    {0,                  8000, 16, 2},
    {WAVE_FORMAT_1M08,  11025,  8, 1},
    {WAVE_FORMAT_1S08,  11025,  8, 2},
    {WAVE_FORMAT_1M16,  11025, 16, 1},
    {WAVE_FORMAT_1S16,  11025, 16, 2},
    {0,                 12000,  8, 1},
    {0,                 12000,  8, 2},
    {0,                 12000, 16, 1},
    {0,                 12000, 16, 2},
    {0,                 16000,  8, 1},
    {0,                 16000,  8, 2},
    {0,                 16000, 16, 1},
    {0,                 16000, 16, 2},
    {WAVE_FORMAT_2M08,  22050,  8, 1},
    {WAVE_FORMAT_2S08,  22050,  8, 2},
    {WAVE_FORMAT_2M16,  22050, 16, 1},
    {WAVE_FORMAT_2S16,  22050, 16, 2},
    {WAVE_FORMAT_4M08,  44100,  8, 1},
    {WAVE_FORMAT_4S08,  44100,  8, 2},
    {WAVE_FORMAT_4M16,  44100, 16, 1},
    {WAVE_FORMAT_4S16,  44100, 16, 2},
    {WAVE_FORMAT_48M08, 48000,  8, 1},
    {WAVE_FORMAT_48S08, 48000,  8, 2},
    {WAVE_FORMAT_48M16, 48000, 16, 1},
    {WAVE_FORMAT_48S16, 48000, 16, 2},
    {WAVE_FORMAT_96M08, 96000,  8, 1},
    {WAVE_FORMAT_96S08, 96000,  8, 2},
    {WAVE_FORMAT_96M16, 96000, 16, 1},
    {WAVE_FORMAT_96S16, 96000, 16, 2}
};

extern const char* dev_name(int);
extern const char* wave_open_flags(DWORD);
extern const char* mmsys_error(MMRESULT);
extern const char* wave_out_error(MMRESULT);
extern const char* get_format_str(WORD format);
extern const char* wave_time_format(UINT type);
extern DWORD bytes_to_samples(DWORD bytes, LPWAVEFORMATEX pwfx);
extern DWORD bytes_to_ms(DWORD bytes, LPWAVEFORMATEX pwfx);
extern DWORD time_to_bytes(LPMMTIME mmtime, LPWAVEFORMATEX pwfx);
