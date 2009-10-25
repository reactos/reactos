
#ifndef _DMUSICS_
#define _DMUSICS_

#include "dmusicc.h"

#define REGSTR_PATH_SOFTWARESYNTHS  "Software\\Microsoft\\DirectMusic\\SoftwareSynths"

interface IDirectMusicSynth;
interface IDirectMusicSynthSink;

#ifndef _DMUS_VOICE_STATE_DEFINED
#define _DMUS_VOICE_STATE_DEFINED


DEFINE_GUID(IID_IDirectMusicSynth,          0x9823661,  0x5C85, 0x11D2, 0xAF, 0xA6, 0x00, 0xAA, 0x00, 0x24, 0xD8, 0xB6);
DEFINE_GUID(IID_IDirectMusicSynth8,         0x53CAB625, 0x2711, 0x4C9F, 0x9D, 0xE7, 0x1B, 0x7F, 0x92, 0x5F, 0x6F, 0xC8);
DEFINE_GUID(IID_IDirectMusicSynthSink,      0x09823663, 0x5C85, 0x11D2, 0xAF, 0xA6, 0x00, 0xAA, 0x00, 0x24, 0xD8, 0xB6);
DEFINE_GUID(GUID_DMUS_PROP_SetSynthSink,    0x0A3A5BA5, 0x37B6, 0x11D2, 0xB9, 0xF9, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12);
DEFINE_GUID(GUID_DMUS_PROP_SinkUsesDSound,  0xBE208857, 0x8952, 0x11D2, 0xBA, 0x1C, 0x00, 0x00, 0xF8, 0x75, 0xAC, 0x12);

#define REFRESH_F_LASTBUFFER        0x00000001

typedef struct _DMUS_VOICE_STATE
{
    BOOL bExists;
    SAMPLE_POSITION spPosition;
} DMUS_VOICE_STATE;

#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicSynth
DECLARE_INTERFACE_(IDirectMusicSynth, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(Open) (THIS_ LPDMUS_PORTPARAMS pPortParams) PURE;
    STDMETHOD(Close) (THIS) PURE;
    STDMETHOD(SetNumChannelGroups) (THIS_ DWORD dwGroups) PURE;
    STDMETHOD(Download) (THIS_ LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree ) PURE;
    STDMETHOD(Unload) (THIS_ HANDLE hDownload, HRESULT ( CALLBACK *lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData ) PURE;
    STDMETHOD(PlayBuffer) (THIS_ REFERENCE_TIME rt,LPBYTE pbBuffer, DWORD cbBuffer) PURE;
    STDMETHOD(GetRunningStats) (THIS_ LPDMUS_SYNTHSTATS pStats) PURE;
    STDMETHOD(GetPortCaps) (THIS_ LPDMUS_PORTCAPS pCaps) PURE;
    STDMETHOD(SetMasterClock) (THIS_ IReferenceClock *pClock) PURE;
    STDMETHOD(GetLatencyClock) (THIS_ IReferenceClock **ppClock) PURE;
    STDMETHOD(Activate) (THIS_ BOOL fEnable) PURE;
    STDMETHOD(SetSynthSink) (THIS_ struct IDirectMusicSynthSink *pSynthSink) PURE;
    STDMETHOD(Render) (THIS_ short *pBuffer, DWORD dwLength,  LONGLONG llPosition) PURE;
    STDMETHOD(SetChannelPriority)   (THIS_ DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority) PURE;
    STDMETHOD(GetChannelPriority) (THIS_ DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority) PURE;
    STDMETHOD(GetFormat) (THIS_ LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize) PURE;
    STDMETHOD(GetAppend) (THIS_ DWORD* pdwAppend) PURE;
};
#undef  INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectMusicSynth_QueryInterface(p, a, b)           (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectMusicSynth_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define IDirectMusicSynth_Release(p)                        (p)->lpVtbl->Release(p)
#define IDirectMusicSynth_Open(p, a)                        (p)->lpVtbl->Open(p, a)
#define IDirectMusicSynth_Close(p)                          (p)->lpVtbl->Close(p)
#define IDirectMusicSynth_SetNumChannelGroups(p, a)         (p)->lpVtbl->SetNumChannelGroups(p, a)
#define IDirectMusicSynth_Download(p, a, b, c)              (p)->lpVtbl->Download(p, a, b, c)
#define IDirectMusicSynth_Unload(p, a, b, c)                (p)->lpVtbl->Unload(p, a, b, c)
#define IDirectMusicSynth_PlayBuffer(p, a, b, c)            (p)->lpVtbl->PlayBuffer(p, a, b, c)
#define IDirectMusicSynth_GetRunningStats(p, a)             (p)->lpVtbl->GetRunningStats(p, a)
#define IDirectMusicSynth_GetPortCaps(p, a)                 (p)->lpVtbl->GetPortCaps(p, a)
#define IDirectMusicSynth_SetMasterClock(p, a)              (p)->lpVtbl->SetMasterClock((p, a)
#define IDirectMusicSynth_GetLatencyClock(p, a)             (p)->lpVtbl->GetLatencyClock(p, a)
#define IDirectMusicSynth_Activate(p, a)                    (p)->lpVtbl->Activate((p, a)
#define IDirectMusicSynth_SetSynthSink(p, a)                (p)->lpVtbl->SetSynthSink(p, a)
#define IDirectMusicSynth_Render(p, a, b, c)                (p)->lpVtbl->Render(p, a, b, c)
#define IDirectMusicSynth_SetChannelPriority(p, a, b, c)    (p)->lpVtbl->SetChannelPriority(p, a, b, c)
#define IDirectMusicSynth_GetChannelPriority(p, a, b, c)    (p)->lpVtbl->GetChannelPriority(p, a, b, c)
#define IDirectMusicSynth_GetFormat(p, a, b)                (p)->lpVtbl->GetFormat(p, a, b)
#define IDirectMusicSynth_GetAppend(p, a)                   (p)->lpVtbl->GetAppend(p, a)
#endif

#define INTERFACE  IDirectMusicSynth8
DECLARE_INTERFACE_(IDirectMusicSynth8, IDirectMusicSynth)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(Open) (THIS_ LPDMUS_PORTPARAMS pPortParams) PURE;
    STDMETHOD(Close) (THIS) PURE;
    STDMETHOD(SetNumChannelGroups) (THIS_ DWORD dwGroups) PURE;
    STDMETHOD(Download) (THIS_ LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree ) PURE;
    STDMETHOD(Unload) (THIS_ HANDLE hDownload, HRESULT ( CALLBACK *lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData ) PURE;
    STDMETHOD(PlayBuffer) (THIS_ REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer) PURE;
    STDMETHOD(GetRunningStats) (THIS_ LPDMUS_SYNTHSTATS pStats) PURE;
    STDMETHOD(GetPortCaps) (THIS_ LPDMUS_PORTCAPS pCaps) PURE;
    STDMETHOD(SetMasterClock) (THIS_ IReferenceClock *pClock) PURE;
    STDMETHOD(GetLatencyClock) (THIS_ IReferenceClock **ppClock) PURE;
    STDMETHOD(Activate) (THIS_ BOOL fEnable) PURE;
    STDMETHOD(SetSynthSink) (THIS_ struct IDirectMusicSynthSink *pSynthSink) PURE;
    STDMETHOD(Render) (THIS_ short *pBuffer, DWORD dwLength, LONGLONG llPosition) PURE;
    STDMETHOD(SetChannelPriority) (THIS_ DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority) PURE;
    STDMETHOD(GetChannelPriority) (THIS_ DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority) PURE;
    STDMETHOD(GetFormat) (THIS_ LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize) PURE;
    STDMETHOD(GetAppend) (THIS_ DWORD* pdwAppend) PURE;
    STDMETHOD(PlayVoice) (THIS_ REFERENCE_TIME rt, DWORD dwVoiceId, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwDLId, long prPitch, long vrVolume, SAMPLE_TIME stVoiceStart, SAMPLE_TIME stLoopStart, SAMPLE_TIME stLoopEnd) PURE;
    STDMETHOD(StopVoice) (THIS_ REFERENCE_TIME rt, DWORD dwVoiceId ) PURE;
    STDMETHOD(GetVoiceState) (THIS_ DWORD dwVoice[], DWORD cbVoice, DMUS_VOICE_STATE dwVoiceState[] ) PURE;
    STDMETHOD(Refresh) (THIS_ DWORD dwDownloadID, DWORD dwFlags) PURE;
    STDMETHOD(AssignChannelToBuses) (THIS_ DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwBuses, DWORD cBuses) PURE;
};
#undef  INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectMusicSynth8_QueryInterface(p, a, b)                       (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectMusicSynth8_AddRef(p)                                     (p)->lpVtbl->AddRef(p)
#define IDirectMusicSynth8_Release(p)                                    (p)->lpVtbl->Release(p)
#define IDirectMusicSynth8_Open(p, a)                                    (p)->lpVtbl->Open(p, a)
#define IDirectMusicSynth8_Close(p)                                      (p)->lpVtbl->Close(p)
#define IDirectMusicSynth8_SetNumChannelGroups(p, a)                     (p)->lpVtbl->SetNumChannelGroups(p, a)
#define IDirectMusicSynth8_Download(p, a, b, c)                          (p)->lpVtbl->Download(p, a, b, c)
#define IDirectMusicSynth8_Unload(p, a, b, c)                            (p)->lpVtbl->Unload(p, a, b, c)
#define IDirectMusicSynth8_PlayBuffer(p, a, b, c)                        (p)->lpVtbl->PlayBuffer(p, a, b, c)
#define IDirectMusicSynth8_GetRunningStats(p, a)                         (p)->lpVtbl->GetRunningStats(p, a)
#define IDirectMusicSynth8_GetPortCaps(p, a)                             (p)->lpVtbl->GetPortCaps(p, a)
#define IDirectMusicSynth8_SetMasterClock(p, a)                          (p)->lpVtbl->SetMasterClock((p, a)
#define IDirectMusicSynth8_GetLatencyClock(p, a)                         (p)->lpVtbl->GetLatencyClock(p, a)
#define IDirectMusicSynth8_Activate(p, a)                                (p)->lpVtbl->Activate((p, a)
#define IDirectMusicSynth8_SetSynthSink(p, a)                            (p)->lpVtbl->SetSynthSink(p, a)
#define IDirectMusicSynth8_Render(p, a, b, c)                            (p)->lpVtbl->Render(p, a, b, c)
#define IDirectMusicSynth8_SetChannelPriority(p, a, b, c)                (p)->lpVtbl->SetChannelPriority(p, a, b, c)
#define IDirectMusicSynth8_GetChannelPriority(p, a, b, c)                (p)->lpVtbl->GetChannelPriority(p, a, b, c)
#define IDirectMusicSynth8_GetFormat(p, a, b)                            (p)->lpVtbl->GetFormat(p, a, b)
#define IDirectMusicSynth8_GetAppend(p, a)                               (p)->lpVtbl->GetAppend(p, a)
#define IDirectMusicSynth8_PlayVoice(p, a, b, c, d, e, f, g, h, i, j)    (p)->lpVtbl->PlayVoice(p, a, b, c, d, e, f, g, h, i, j)
#define IDirectMusicSynth8_StopVoice(p, a, b)                            (p)->lpVtbl->StopVoice(p, a, b)
#define IDirectMusicSynth8_GetVoiceState(p, a, b, c)                     (p)->lpVtbl->GetVoiceState(p, a, b, c)
#define IDirectMusicSynth8_Refresh(p, a, b)                              (p)->lpVtbl->Refresh(p, a, b)
#define IDirectMusicSynth8_AssignChannelToBuses(p, a, b, c, d)           (p)->lpVtbl->AssignChannelToBuses(p, a, b, c, d)
#endif

#define INTERFACE  IDirectMusicSynthSink
DECLARE_INTERFACE_(IDirectMusicSynthSink, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    STDMETHOD(Init) (THIS_ IDirectMusicSynth *pSynth) PURE;
    STDMETHOD(SetMasterClock) (THIS_ IReferenceClock *pClock) PURE;
    STDMETHOD(GetLatencyClock) (THIS_ IReferenceClock **ppClock) PURE;
    STDMETHOD(Activate) (THIS_ BOOL fEnable) PURE;
    STDMETHOD(SampleToRefTime) (THIS_ LONGLONG llSampleTime, REFERENCE_TIME *prfTime) PURE;
    STDMETHOD(RefTimeToSample) (THIS_ REFERENCE_TIME rfTime, LONGLONG *pllSampleTime) PURE;
    STDMETHOD(SetDirectSound) (THIS_ LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer) PURE;
    STDMETHOD(GetDesiredBufferSize) (THIS_ LPDWORD pdwBufferSizeInSamples) PURE;
};

#endif
