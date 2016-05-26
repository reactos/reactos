/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/sndrec32.h
 * PURPOSE:         Sound recording
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#pragma once

#include "resource.h"
#include "audio_api.hpp"

#define MAX_LOADSTRING 100

#define MAINWINDOW_W 350
#define MAINWINDOW_H 190

#define CONTROLS_CX 10

#define INITIAL_BUFREC_SECONDS 30.0f

#define BUTSTART_ID 0
#define BUTEND_ID 1
#define BUTPLAY_ID 2
#define BUTSTOP_ID 3
#define BUTREC_ID 4
#define SLIDER_ID 5
#define WAVEBAR_ID 6

#define BUTTONS_H 30
#define BUTTONS_W 60
#define BUTTONS_CY 100
#define BUTTONS_CX (CONTROLS_CX)
#define BUTTONS_SPACE 5

#define SLIDER_CX CONTROLS_CX
#define SLIDER_CY 65
#define SLIDER_H 30
#define SLIDER_W 320

#define STRPOS_X 240
#define STRPOS_Y 5

#define STRDUR_X (STRPOS_X)
#define STRDUR_Y (STRPOS_Y + 13)

#define STRBUF_X (STRDUR_X)
#define STRBUF_Y (STRDUR_Y + 13)

#define STRFMT_X 10
#define STRFMT_Y (STRPOS_Y)

#define STRCHAN_X (STRFMT_X)
#define STRCHAN_Y (STRFMT_Y + 13)

#define WAVEBAR_X (CONTROLS_CX + 90)
#define WAVEBAR_Y (STRPOS_Y)
#define WAVEBAR_CX 130
#define WAVEBAR_CY 50

#define WAVEBAR_TIMERID 2
#define WAVEBAR_TIMERTIME 80

#define WAVEBAR_COLOR (RGB(0, 0, 255))

#define REFRESHA_X (STRPOS_X)
#define REFRESHA_Y (STRPOS_Y)
#define REFRESHA_CX (REFRESHA_X + 100)
#define REFRESHA_CY (REFRESHA_Y + 55)

#define REFRESHB_X (STRFMT_X)
#define REFRESHB_Y (STRFMT_Y)
#define REFRESHB_CX (REFRESHB_X + 85)
#define REFRESHB_CY (REFRESHB_Y + 55)

struct riff_hdr
{
    DWORD magic;
    DWORD chunksize;
    DWORD format;
};

struct wave_hdr
{
    DWORD Subchunkid;
    DWORD Subchunk1Size;
    WORD AudioFormat;
    WORD NumChannels;
    DWORD SampleRate;
    DWORD ByteRate;
    WORD BlockAlign;
    WORD BitsPerSample;
};

struct data_chunk
{
    DWORD subc;
    DWORD subc_size;
    //unsigned char data[];
};

/* Functions prototypes */

LRESULT CALLBACK Buttons_proc(HWND, UINT, WPARAM, LPARAM);

BOOL write_wav(TCHAR *);
BOOL open_wav(TCHAR *);
VOID enable_but(DWORD);
VOID disable_but(DWORD);

void l_play_finished(void);
void l_audio_arrival(unsigned int);
void l_buffer_resized(unsigned int);
