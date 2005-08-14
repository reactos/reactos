#ifndef __SNDVOL32_H
#define __SNDVOL32_H

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include "resources.h"

typedef struct _MIXER_WINDOW
{
  HWND hWnd;
  HWND hStatusBar;
  struct _SND_MIXER *Mixer;
  UINT SelectedLine;
} MIXER_WINDOW, *PMIXER_WINDOW;

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL RebuildMixerWindowControls(PMIXER_WINDOW MixerWindow);

extern HINSTANCE hAppInstance;
extern ATOM MainWindowClass;
extern HWND hMainWnd;
extern HANDLE hAppHeap;

#define SZ_APP_CLASS TEXT("Volume Control")

#define DBG DbgPrint("SNDVOL32: %s:%i: ", __FILE__, __LINE__); DbgPrint


/*
 * MIXER
 */

typedef struct _SND_MIXER_CONNECTION
{
  struct _SND_MIXER_CONNECTION *Next;
  MIXERLINE Info;
  LPMIXERCONTROL Controls;
} SND_MIXER_CONNECTION, *PSND_MIXER_CONNECTION;


typedef struct _SND_MIXER_DESTINATION
{
  struct _SND_MIXER_DESTINATION *Next;
  MIXERLINE Info;
  LPMIXERCONTROL Controls;
  PSND_MIXER_CONNECTION Connections;
} SND_MIXER_DESTINATION, *PSND_MIXER_DESTINATION;

typedef struct _SND_MIXER
{
  UINT MixersCount;
  HWND hWndNotification;
  UINT MixerId;
  HMIXER hmx;
  MIXERCAPS Caps;
  PSND_MIXER_DESTINATION Lines;
} SND_MIXER, *PSND_MIXER;

typedef BOOL (CALLBACK *PFNSNDMIXENUMLINES)(PSND_MIXER Mixer, LPMIXERLINE Line, PVOID Context);
typedef BOOL (CALLBACK *PFNSNDMIXENUMPRODUCTS)(PSND_MIXER Mixer, UINT Id, LPCTSTR ProductName, PVOID Context);

PSND_MIXER SndMixerCreate(HWND hWndNotification);
VOID SndMixerDestroy(PSND_MIXER Mixer);
VOID SndMixerClose(PSND_MIXER Mixer);
BOOL SndMixerSelect(PSND_MIXER Mixer, UINT MixerId);
UINT SndMixerGetSelection(PSND_MIXER Mixer);
INT SndMixerGetProductName(PSND_MIXER Mixer, LPTSTR lpBuffer, UINT uSize);
BOOL SndMixerEnumProducts(PSND_MIXER Mixer, PFNSNDMIXENUMPRODUCTS EnumProc, PVOID Context);
INT SndMixerGetDestinationCount(PSND_MIXER Mixer);
BOOL SndMixerEnumDestinationLines(PSND_MIXER Mixer, PFNSNDMIXENUMLINES EnumProc, PVOID Context);

#endif /* __SNDVOL32_H */
