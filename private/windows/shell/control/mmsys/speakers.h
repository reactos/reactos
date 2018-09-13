//--------------------------------------------------------------------------;
//
//  File: speakers.h
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved
//
//
//--------------------------------------------------------------------------;

#ifndef SPEAKERS_HEADER
#define SPEAKERS_HEADER


#define TYPE_HEADPHONES        0
#define TYPE_STEREODESKTOP     1
#define TYPE_MONOLAPTOP        2
#define TYPE_STEREOLAPTOP      3
#define TYPE_STEREOMONITOR     4
#define TYPE_STEREOCPU         5
#define TYPE_MOUNTEDSTEREO     6
#define TYPE_STEREOKEYBOARD    7
#define TYPE_QUADRAPHONIC      8
#define TYPE_SURROUND          9
#define TYPE_SURROUND_5_1      10
#define MAX_SPEAKER_TYPE       TYPE_SURROUND_5_1


BOOL CALLBACK SpeakerHandler(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void VerifySpeakerConfig(DWORD dwSpeakerConfig, LPDWORD pdwSpeakerType);
DWORD GetSpeakerConfigFromType(DWORD dwType);

#define SPEAKERS_DEFAULT_CONFIG      DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE)
#define SPEAKERS_DEFAULT_TYPE        TYPE_STEREODESKTOP


#endif // SPEAKERS_HEADER