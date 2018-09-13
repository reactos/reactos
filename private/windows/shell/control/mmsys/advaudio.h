//--------------------------------------------------------------------------;
//
//  File: advaudio.h
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved
//
//
//--------------------------------------------------------------------------;

#ifndef ADVAUDIO_HEADER
#define ADVAUDIO_HEADER

typedef struct CPLDATA
{
	DWORD dwHWLevel;
	DWORD dwSRCLevel;
	DWORD dwSpeakerConfig;
	DWORD dwSpeakerType;
} CPLDATA, *LPCPLDATA;


typedef struct AUDDATA
{
	GUID		devGuid;
	BOOL		fValid;
    BOOL        fRecord;
	CPLDATA		stored;
	CPLDATA		current;
} AUDDATA, *LPAUDDATA;


STDAPI_(void) AdvancedAudio(HWND hWnd, HINSTANCE hInst, const TCHAR *szHelpFile, LPTSTR szDeviceName, BOOL fRecord);
STDAPI_(void) ToggleApplyButton(HWND hWnd);
STDAPI_(void) ApplyCurrentSettings(LPAUDDATA pAD);

extern AUDDATA		gAudData;
extern HINSTANCE	ghInst;
extern const TCHAR*	gszHelpFile;

#endif // ADVAUDIO_HEADER