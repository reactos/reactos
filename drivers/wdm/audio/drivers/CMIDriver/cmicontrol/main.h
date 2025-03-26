/*
Copyright (c) 2006-2008 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _MAIN_H_
#define _MAIN_H_

#include <math.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <devioctl.h>
#include <commctrl.h>
#include <setupapi.h>
#include <shellapi.h>
#include <cpl.h>

#include <ks.h>
#include <mmreg.h>
#include <ksmedia.h>
#include <mmsystem.h>


#define INITGUID
#define DIRECTSOUND_VERSION 0x800
#include <dsound.h>

#include "resource.h"
#include "property.h"

#define SAMPLE_RATE 44100

#define BASS_FREQUENCY 90
#define BASS_AMPLITUDE 0.5

#define SPEAKER_FREQUENCY 440
#define SPEAKER_AMPLITUDE 0.5

#define MAX_TOKEN_SIZE 128

typedef struct _CMIDEV  {
	HDEVINFO                          Info;
	SP_DEVINFO_DATA                   InfoData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA  InterfaceDetailData;
} CMIDEV;

static INT_PTR CALLBACK TabDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HINSTANCE hInst;
HWND      hWndMain;
HWND      hWndTab;
HWND      hWndChild[NUM_TABS];
LRESULT   currentTab;
CMIDEV    cmiTopologyDev;
CMIDATA   cmiData;
HWAVEOUT  hWave;
WAVEHDR   pwh;
int       currentChannelCount;
HFONT     hURLFont;

#endif //_MAIN_H_
