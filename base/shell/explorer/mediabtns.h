/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Backend class for media buttons (header)
 * COPYRIGHT:   Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

typedef MMRESULT (WINAPI* pmxControlDetails)(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
typedef MMRESULT (WINAPI* pmxLine)(HMIXEROBJ hmxobj, LPMIXERLINEW pmxcd, DWORD fdwDetails);
typedef MMRESULT (WINAPI* pmxLineControls)(HMIXEROBJ hmxobj, LPMIXERLINECONTROLSW pmxcd, DWORD fdwDetails);
typedef MMRESULT (WINAPI* pmxOpen)(LPHMIXER phmx, UINT uMxId, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
typedef MMRESULT (WINAPI* pmxClose)(HMIXER hmx);
typedef MMRESULT (WINAPI* pwoGetErrText)(MMRESULT mmrError, LPWSTR pszText, UINT cchText);
#define WINMM_PROC(func, type) ((func) = (type)GetProcAddress(m_hWinMM, #func))

/* Media volume buttons must be synchronized with sndvol32 meters */
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

    void GetMasterMixer();
    bool Mute();
    bool AdjustVolume(_In_ UINT direction, _In_ HANDLE hProcessHeap);

private:
    pmxControlDetails mixerSetControlDetails;
    pmxControlDetails mixerGetControlDetailsW;
    pmxLine mixerGetLineInfoW;
    pmxLineControls mixerGetLineControlsW;
    pmxOpen mixerOpen;
    pmxClose mixerClose;
    pwoGetErrText waveOutGetErrorTextW;
 
    HMODULE m_hWinMM;
    HMIXER m_hMixer = NULL;
    DWORD m_dwMasterMuteControlID;
    DWORD m_dwMasterVolumeID;
    DWORD m_dwMasterChannels;
    DWORD m_dwMasterRanges[2];
    DWORD m_dwVolumeStep;
    bool m_isWinMMAvailable;    
};