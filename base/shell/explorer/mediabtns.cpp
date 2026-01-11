/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Backend class for media buttons
 * COPYRIGHT:   Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "precomp.h"
#include "mediabtns.h"

CMultimediaBackend::CMultimediaBackend()
{
    m_isWinMMAvailable = false;

    m_hWinMM = LoadLibraryW(L"winmm.dll");
    if (!m_hWinMM)
        return;

    if (!WINMM_PROC(mixerOpen, pmxOpen))
        return;

    if (!WINMM_PROC(mixerClose, pmxClose))
        return;

    if (!WINMM_PROC(mixerSetControlDetails, pmxControlDetails))
        return;

    if (!WINMM_PROC(mixerGetControlDetailsW, pmxControlDetails))
        return;

    if (!WINMM_PROC(mixerGetLineInfoW, pmxLine))
        return;

    if (!WINMM_PROC(mixerGetLineControlsW, pmxLineControls))
        return;
    
    if (!WINMM_PROC(waveOutGetErrorTextW, pwoGetErrText))
        return;
};

CMultimediaBackend::~CMultimediaBackend()
{
    if (m_isWinMMAvailable)
        mixerClose(m_hMixer);

    CloseHandle(m_hWinMM);
}

void
CMultimediaBackend::GetMasterMixer()
{
    if (m_hMixer != NULL)
        mixerClose(m_hMixer);

    WCHAR pszWinMMErrText[MAXERRORLENGTH] = {0};

    MMRESULT mmres = mixerOpen(&m_hMixer, 0, NULL, 0, MIXER_OBJECTF_MIXER);
    if (mmres != MMSYSERR_NOERROR)
    {
        waveOutGetErrorTextW(mmres, pszWinMMErrText, _countof(pszWinMMErrText));
        ERR("GetMasterMixer: mixerOpen failed: %lu (%S)\n", mmres, pszWinMMErrText);
        return;
    }

    MIXERLINEW mxln;
    mxln.cbStruct = sizeof(mxln);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    mmres = mixerGetLineInfoW((HMIXEROBJ)m_hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (mmres != MMSYSERR_NOERROR)
    {
        waveOutGetErrorTextW(mmres, pszWinMMErrText, _countof(pszWinMMErrText));
        ERR("GetMasterMixer: mixerGetLineInfoW failed: %lu (%S)\n", mmres, pszWinMMErrText);
        return;
    }

    m_dwMasterChannels = mxln.cChannels;        

    MIXERLINECONTROLS mxlctrl;
    MIXERCONTROLW mxc;
    mxlctrl.cbStruct = sizeof(mxlctrl);
    mxlctrl.dwLineID = mxln.dwLineID;
    mxlctrl.dwControlID = MIXERCONTROL_CONTROLTYPE_MUTE;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(mxc);
    mxlctrl.pamxctrl = &mxc;

    mmres = mixerGetLineControlsW((HMIXEROBJ)m_hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmres != MMSYSERR_NOERROR)
    {
        waveOutGetErrorTextW(mmres, pszWinMMErrText, _countof(pszWinMMErrText));
        ERR("GetMasterMixer: mixerGetLineControlsW failed retrieving Master Mute control: %lu (%S)\n", mmres, pszWinMMErrText);
        return;
    }

    m_dwMasterMuteControlID = mxc.dwControlID;

    mxlctrl.dwControlID = MIXERCONTROL_CONTROLTYPE_VOLUME;

    mmres = mixerGetLineControlsW((HMIXEROBJ)m_hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmres != MMSYSERR_NOERROR)
    {
        waveOutGetErrorTextW(mmres, pszWinMMErrText, _countof(pszWinMMErrText));
        ERR("GetMasterMixer: mixerGetLineControlsW failed retrieving Master Volume control: %lu (%S)\n", mmres, pszWinMMErrText);
        return;
    }

    m_dwMasterVolumeID = mxc.dwControlID;
    m_dwMasterRanges[0] = mxc.Bounds.dwMinimum;
    m_dwMasterRanges[1] = mxc.Bounds.dwMaximum;
    m_dwVolumeStep = (m_dwMasterRanges[1] - m_dwMasterRanges[0]) / VOLUME_STEPS_COUNT;

    TRACE("GetMasterMixer: Master controls configured successfully:\n"
          "   Master Mute dwControlID: %lu; Master Volume dwControlID %lu\n"
          "   Master Volume channels: %lu; Master Volume value range: %lu-%lu\n"
          "   Master Volume update step: %lu\n",
          m_dwMasterMuteControlID, m_dwMasterVolumeID,
          m_dwMasterChannels, m_dwMasterRanges[0], m_dwMasterRanges[1],
          m_dwVolumeStep);

    m_isWinMMAvailable = true;
}

bool
CMultimediaBackend::Mute()
{
    if (!m_isWinMMAvailable)
    {
        WARN("CMultimediaBackend: Mixer is not available; cannot (un)mute sound. Check the prior class constructor calls\n");
        return false;
    }

    MIXERCONTROLDETAILS mxcd;
    MIXERCONTROLDETAILS_BOOLEAN mxcdMute;

    mxcd.cbStruct = sizeof(mxcd);
    mxcd.dwControlID = m_dwMasterMuteControlID;
    mxcd.cChannels = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails = sizeof(mxcdMute);
    mxcd.paDetails = &mxcdMute;

    mixerGetControlDetailsW((HMIXEROBJ)m_hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);

    mxcdMute.fValue = !mxcdMute.fValue;

    mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);

    return true;
}

bool
CMultimediaBackend::AdjustVolume(_In_ UINT direction, _In_ HANDLE hProcessHeap)
{
    if (!m_isWinMMAvailable)
    {
        WARN("CMultimediaBackend: Mixer is not available; cannot %s volume. Check the prior class constructor calls\n",
              direction == APPCOMMAND_VOLUME_DOWN ? "decrease" : "increase");
        return FALSE;
    }

    MIXERCONTROLDETAILS mxcd;
    mxcd.cbStruct = sizeof(mxcd);
    mxcd.cMultipleItems = 0;

    /* If sound is muted and we want to increase volume, we are unmuting it. */
    if (direction == APPCOMMAND_VOLUME_UP)
    {
        MIXERCONTROLDETAILS_BOOLEAN mxcdMute;
        
        mxcd.cChannels = 1;
        mxcd.dwControlID = m_dwMasterMuteControlID;
        mxcd.cbDetails = sizeof(mxcdMute);
        mxcd.paDetails = &mxcdMute;
        mxcdMute.fValue = false;

        mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
    }

    PMIXERCONTROLDETAILS_UNSIGNED mxcdVolume =
        (PMIXERCONTROLDETAILS_UNSIGNED)HeapAlloc(hProcessHeap,
                                                 0,
                                                 m_dwMasterChannels * sizeof(*mxcdVolume));

    if (mxcdVolume == NULL)
    {
        ERR("CMultimediaBackend: HeapAlloc failed, requested %lu bytes", m_dwMasterChannels * sizeof(*mxcdVolume));
        return FALSE;
    }

    mxcd.cChannels = m_dwMasterChannels;
    mxcd.dwControlID = m_dwMasterVolumeID;
    mxcd.cbDetails = sizeof(*mxcdVolume);
    mxcd.paDetails = mxcdVolume;

    mixerGetControlDetailsW((HMIXEROBJ)m_hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);

    /* TODO: implement restoring balance between channels when all channels were put to 0 */
    for (DWORD i = 0; i < m_dwMasterChannels; ++i)
    {                            
        if (direction == APPCOMMAND_VOLUME_DOWN)
        {
            /* Protecting from underflow when decreasing volume */
            if (mxcdVolume[i].dwValue < m_dwVolumeStep)
                mxcdVolume[i].dwValue = 0;
            else
                mxcdVolume[i].dwValue -= m_dwVolumeStep;
        }
        else
        {
            /* Protecting from overflow when increasing volume */
            if (m_dwMasterRanges[1] - mxcdVolume[i].dwValue < m_dwVolumeStep)
                mxcdVolume[i].dwValue = m_dwMasterRanges[1];
            else
                mxcdVolume[i].dwValue += m_dwVolumeStep;
        }
    }

    mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
    HeapFree(hProcessHeap, 0, mxcdVolume);
    return TRUE;
}