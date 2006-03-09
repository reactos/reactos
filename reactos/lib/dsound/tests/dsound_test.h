/*
 * Unit tests for dsound functions
 *
 * Copyright (c) 2004 Francois Gouget
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

static const unsigned int formats[][4]={
    { 8000,  8, 1, 0 },
    { 8000,  8, 2, 0 },
    { 8000, 16, 1, 0 },
    { 8000, 16, 2, 0 },
    {11025,  8, 1, WAVE_FORMAT_1M08 },
    {11025,  8, 2, WAVE_FORMAT_1S08 },
    {11025, 16, 1, WAVE_FORMAT_1M16 },
    {11025, 16, 2, WAVE_FORMAT_1S16 },
    {22050,  8, 1, WAVE_FORMAT_2M08 },
    {22050,  8, 2, WAVE_FORMAT_2S08 },
    {22050, 16, 1, WAVE_FORMAT_2M16 },
    {22050, 16, 2, WAVE_FORMAT_2S16 },
    {44100,  8, 1, WAVE_FORMAT_4M08 },
    {44100,  8, 2, WAVE_FORMAT_4S08 },
    {44100, 16, 1, WAVE_FORMAT_4M16 },
    {44100, 16, 2, WAVE_FORMAT_4S16 },
    {48000,  8, 1, WAVE_FORMAT_48M08 },
    {48000,  8, 2, WAVE_FORMAT_48S08 },
    {48000, 16, 1, WAVE_FORMAT_48M16 },
    {48000, 16, 2, WAVE_FORMAT_48S16 },
    {96000,  8, 1, WAVE_FORMAT_96M08 },
    {96000,  8, 2, WAVE_FORMAT_96S08 },
    {96000, 16, 1, WAVE_FORMAT_96M16 },
    {96000, 16, 2, WAVE_FORMAT_96S16 }
};
#define NB_FORMATS (sizeof(formats)/sizeof(*formats))

/* The time slice determines how often we will service the buffer */
#define TIME_SLICE     31
#define BUFFER_LEN    400

extern char* wave_generate_la(WAVEFORMATEX*,double,DWORD*);
extern HWND get_hwnd(void);
extern void init_format(WAVEFORMATEX*,int,int,int,int);
extern void test_buffer(LPDIRECTSOUND,LPDIRECTSOUNDBUFFER,
                        BOOL,BOOL,LONG,BOOL,LONG,BOOL,double,BOOL,
                        LPDIRECTSOUND3DLISTENER,BOOL,BOOL,BOOL,DWORD);
extern void test_buffer8(LPDIRECTSOUND8,LPDIRECTSOUNDBUFFER,
                         BOOL,BOOL,LONG,BOOL,LONG,BOOL,double,BOOL,
                         LPDIRECTSOUND3DLISTENER,BOOL,BOOL);
extern const char * getDSBCAPS(DWORD xmask);
extern int align(int length, int align);
extern const char * get_file_version(const char * file_name);
