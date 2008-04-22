
#ifndef _DMUSICS_
#define _DMUSICS_

#include "dmusicc.h"

#define REGSTR_PATH_SOFTWARESYNTHS  "Software\\Microsoft\\DirectMusic\\SoftwareSynths"

interface IDirectMusicSynth;
interface IDirectMusicSynthSink;

#ifndef __cplusplus
    typedef interface IDirectMusicSynth IDirectMusicSynth;
    typedef interface IDirectMusicSynthSink IDirectMusicSynthSink;
#endif

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
    STDMETHOD(SetSynthSink) (THIS_ IDirectMusicSynthSink *pSynthSink) PURE;
    STDMETHOD(Render) (THIS_ short *pBuffer, DWORD dwLength,  LONGLONG llPosition) PURE;
    STDMETHOD(SetChannelPriority)   (THIS_ DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority) PURE;
    STDMETHOD(GetChannelPriority) (THIS_ DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority) PURE;
    STDMETHOD(GetFormat) (THIS_ LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize) PURE;
    STDMETHOD(GetAppend) (THIS_ DWORD* pdwAppend) PURE;
};

#undef  INTERFACE

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
    STDMETHOD(SetSynthSink) (THIS_ IDirectMusicSynthSink *pSynthSink) PURE;
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
