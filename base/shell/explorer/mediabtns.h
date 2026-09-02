/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Backend class for media buttons (header)
 * COPYRIGHT:   Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include <mmddk.h>

typedef MMRESULT (WINAPI* pmxControlDetails)(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
typedef MMRESULT (WINAPI* pmxGetID)(HMIXEROBJ hmxobj, UINT* puMxId, DWORD fdwId);
typedef MMRESULT (WINAPI* pmxLine)(HMIXEROBJ hmxobj, LPMIXERLINEW pmxcd, DWORD fdwDetails);
typedef MMRESULT (WINAPI* pmxLineControls)(HMIXEROBJ hmxobj, LPMIXERLINECONTROLSW pmxcd, DWORD fdwDetails);
typedef MMRESULT (WINAPI* pmxOpen)(LPHMIXER phmx, UINT uMxId, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
typedef MMRESULT (WINAPI* pmxClose)(HMIXER hmx);
typedef MMRESULT (WINAPI* pwoGetErrText)(MMRESULT mmrError, LPWSTR pszText, UINT cchText);
typedef MMRESULT (WINAPI* pwoGetNumDevs)(VOID);
typedef MMRESULT (WINAPI* pwoMessage)(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2);
#define WINMM_PROC(func, type) ((func) = (type)GetProcAddress(hWinMM, #func))

/* Media volume buttons are synchronized against sndvol32 meters */
#define VOLUME_STEPS_COUNT 25

class CMultimediaBackend
{
public:
    CMultimediaBackend();
    ~CMultimediaBackend();
    CMultimediaBackend(CMultimediaBackend&) = delete;
    CMultimediaBackend(CMultimediaBackend&&) = delete;
    CMultimediaBackend& operator=(CMultimediaBackend&) = delete;
    CMultimediaBackend& operator=(CMultimediaBackend&&) = delete;

    void Initialize();
    bool Mute();
    bool AdjustVolume(_In_ UINT direction, _In_ HANDLE hProcessHeap);

private:
    void CalculateSteps(_Inout_ PMIXERCONTROLDETAILS_UNSIGNED pmxcdVolume);
    void LogError(_In_ const char* pszFunctionName, _In_ const char* pszMessage, _In_ MMRESULT mmres);
    MMRESULT GetCurrentVolume(_Inout_ PMIXERCONTROLDETAILS pmxcd);

    pmxControlDetails mixerSetControlDetails;
    pmxControlDetails mixerGetControlDetailsW;
    pmxGetID mixerGetID;
    pmxLine mixerGetLineInfoW;
    pmxLineControls mixerGetLineControlsW;
    pmxOpen mixerOpen;
    pmxClose mixerClose;
    pwoGetErrText waveOutGetErrorTextW;
    pwoGetNumDevs waveOutGetNumDevs;
    pwoMessage waveOutMessage;

    HMODULE m_hWinMM;
    HMIXER m_hMixer = NULL;
    DWORD m_dwMasterMuteControlID;
    DWORD m_dwMasterVolumeID;
    DWORD m_dwMasterChannels;
    struct
    {
        DWORD dwMinimum;
        DWORD dwMaximum;
    } m_dwMasterRanges;
    PDWORD m_pdwChannelSteps = NULL;
};
