/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 *              or BSD-3-Clause (https://spdx.org/licenses/BSD-3-Clause)
 * PURPOSE:     Backend class for media buttons
 * COPYRIGHT:   Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "precomp.h"
#include <math.h>
#include "mediabtns.h"

CMultimediaBackend::CMultimediaBackend()
{
    HMODULE hWinMM = LoadLibraryW(L"winmm.dll");
    bool bFailedGetProcAddress = false;

    if (!hWinMM)
        return;

    bFailedGetProcAddress |= !WINMM_PROC(mixerOpen, pmxOpen);
    bFailedGetProcAddress |= !WINMM_PROC(mixerClose, pmxClose);
    bFailedGetProcAddress |= !WINMM_PROC(mixerSetControlDetails, pmxControlDetails);
    bFailedGetProcAddress |= !WINMM_PROC(mixerGetControlDetailsW, pmxControlDetails);
    bFailedGetProcAddress |= !WINMM_PROC(mixerGetID, pmxGetID);
    bFailedGetProcAddress |= !WINMM_PROC(mixerGetLineInfoW, pmxLine);
    bFailedGetProcAddress |= !WINMM_PROC(mixerGetLineControlsW, pmxLineControls);
    bFailedGetProcAddress |= !WINMM_PROC(waveOutGetErrorTextW, pwoGetErrText);
    bFailedGetProcAddress |= !WINMM_PROC(waveOutGetNumDevs, pwoGetNumDevs);
    bFailedGetProcAddress |= !WINMM_PROC(waveOutMessage, pwoMessage);

    if (bFailedGetProcAddress)
    {
        ERR("CMultimediaBackend::ctor: One or more entry points to WinMM functions not found\n");
        FreeLibrary(hWinMM);
        return;
    }

    m_hWinMM = hWinMM;
};

CMultimediaBackend::~CMultimediaBackend()
{
    if (m_hWinMM)
        mixerClose(m_hMixer);

    HeapFree(hProcessHeap, 0, m_pdwChannelSteps);

    FreeLibrary(m_hWinMM);
}

void
CMultimediaBackend::Initialize()
{
    if (m_hMixer != NULL)
        mixerClose(m_hMixer);

    if (waveOutGetNumDevs() == 0)
    {
        ERR("CMultimediaBackend::Initialize: No waveform-audio output devices detected\n");
        return;
    }

    DWORD dwWaveOutDeviceID;
    DWORD dw2 = 0;

    MMRESULT mmres = waveOutMessage((HWAVEOUT)UlongToHandle(WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&dwWaveOutDeviceID, (DWORD_PTR)&dw2);
    if (mmres != MMSYSERR_NOERROR)
        LogError(__func__, "waveOutMessage failed", mmres);

    if (dwWaveOutDeviceID == (DWORD)-1)
    {
        TRACE("CMultimediaBackend::Initialize: No default output device was assigned, falling back to the first device\n");
        dwWaveOutDeviceID = 0;
    }

    UINT mxID;
    mmres = mixerGetID((HMIXEROBJ)UlongToHandle(dwWaveOutDeviceID), &mxID, MIXER_OBJECTF_WAVEOUT);
    if (mmres != MMSYSERR_NOERROR)
    {
        LogError(__func__, "mixerGetID failed", mmres);

        if (mmres == MMSYSERR_NODRIVER)
        {
            ERR("    Not providing fallback to the first mixer device.\n");
            return;
        }

        mxID = 0;
    }

    mmres = mixerOpen(&m_hMixer, mxID, NULL, 0, MIXER_OBJECTF_MIXER);
    if (mmres != MMSYSERR_NOERROR)
    {
        m_hMixer = NULL;
        LogError(__func__, "mixerOpen failed", mmres);
        return;
    }

    MIXERLINEW mxln;
    mxln.cbStruct = sizeof(mxln);
    mxln.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    mmres = mixerGetLineInfoW((HMIXEROBJ)m_hMixer, &mxln, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (mmres != MMSYSERR_NOERROR)
    {
        LogError(__func__, "mixerGetLineInfoW failed", mmres);
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
        LogError(__func__, "mixerGetLineControlsW failed retrieving Master Mute control", mmres);
        return;
    }

    m_dwMasterMuteControlID = mxc.dwControlID;

    mxlctrl.dwControlID = MIXERCONTROL_CONTROLTYPE_VOLUME;

    mmres = mixerGetLineControlsW((HMIXEROBJ)m_hMixer, &mxlctrl, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmres != MMSYSERR_NOERROR)
    {
        LogError(__func__, "mixerGetLineControlsW failed retrieving Master Volume control", mmres);
        return;
    }

    m_dwMasterVolumeID = mxc.dwControlID;
    m_dwMasterRanges.dwMinimum = mxc.Bounds.dwMinimum;
    m_dwMasterRanges.dwMaximum = mxc.Bounds.dwMaximum;

    m_pdwChannelSteps = (PDWORD)HeapAlloc(hProcessHeap, 0, m_dwMasterChannels * sizeof(*m_pdwChannelSteps));

    TRACE("CMultimediaBackend::Initialize: Master controls configured successfully:\n"
          "    Master Mute dwControlID: %lu; Master Volume dwControlID %lu\n"
          "    Master Volume channels: %lu; Master Volume value range: %lu-%lu\n",
          m_dwMasterMuteControlID, m_dwMasterVolumeID,
          m_dwMasterChannels, m_dwMasterRanges.dwMinimum, m_dwMasterRanges.dwMaximum);
}

bool
CMultimediaBackend::Mute()
{
    if (!m_hWinMM)
    {
        WARN("CMultimediaBackend::Mute: WinMM functions unavailable; cannot (un)mute sound. Check the prior class constructor calls\n");
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
    if (!m_hWinMM)
    {
        WARN("CMultimediaBackend: WinMM functions unavailable; cannot %s volume. Check the prior class constructor calls\n",
             direction == APPCOMMAND_VOLUME_DOWN ? "decrease" : "increase");
        return FALSE;
    }

    MMRESULT mmres;

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

        mmres = mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
        if (mmres != MMSYSERR_NOERROR)
            LogError(__func__, "mixerSetControlDetails failed unmuting Master Volume", mmres);
    }

    if (GetCurrentVolume(&mxcd) != MMSYSERR_NOERROR)
        return FALSE;

    PMIXERCONTROLDETAILS_UNSIGNED mxcdVolume = (PMIXERCONTROLDETAILS_UNSIGNED)mxcd.paDetails;

    bool bEitherCeiling = [this](_In_ UINT direction, _Inout_ PMIXERCONTROLDETAILS_UNSIGNED mxcdVolume) -> bool
    {
        DWORD dwValue = m_dwMasterRanges.dwMinimum;

        for (DWORD i = 0; i < m_dwMasterChannels; ++i)
        {
            if (mxcdVolume[i].dwValue > dwValue)
                dwValue = mxcdVolume[i].dwValue;
        }

        return direction == APPCOMMAND_VOLUME_DOWN
            ? dwValue == m_dwMasterRanges.dwMinimum
            : dwValue == m_dwMasterRanges.dwMaximum;
    }(direction, mxcdVolume);

    if (bEitherCeiling)
    {
        TRACE("CMultimediaBackend::AdjustVolume: Volume boundary hit, no action\n");
        goto cleanup;
    }

    CalculateSteps(mxcdVolume);

    for (DWORD i = 0; i < m_dwMasterChannels; ++i)
    {
        if (direction == APPCOMMAND_VOLUME_DOWN)
        {
            /* Protecting from underflow when decreasing volume */
            if (mxcdVolume[i].dwValue < m_pdwChannelSteps[i])
                mxcdVolume[i].dwValue = m_dwMasterRanges.dwMinimum;
            else
                mxcdVolume[i].dwValue -= m_pdwChannelSteps[i];
        }
        else if (direction == APPCOMMAND_VOLUME_UP)
        {
            /* Protecting from overflow when increasing volume */
            if (m_dwMasterRanges.dwMaximum - mxcdVolume[i].dwValue < m_pdwChannelSteps[i])
                mxcdVolume[i].dwValue = m_dwMasterRanges.dwMaximum;
            else
                mxcdVolume[i].dwValue += m_pdwChannelSteps[i];
        }
    }

    mmres = mixerSetControlDetails((HMIXEROBJ)m_hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE);
    if (mmres == MMSYSERR_NOERROR)
    {
        TRACE("CMultimediaBackend::AdjustVolume: Volume values were updated:\n");

        for (DWORD i = 0; i < m_dwMasterChannels; ++i)
            TRACE("    Channel %lu volume: %lu\n", i, mxcdVolume[i].dwValue);
    }
    else
    {
        LogError(__func__, "mixerSetControlDetails failed updating Master Volume control details", mmres);
    }

cleanup:
    HeapFree(hProcessHeap, 0, mxcdVolume);
    return TRUE;
}

void
CMultimediaBackend::CalculateSteps(_Inout_ PMIXERCONTROLDETAILS_UNSIGNED pmxcdVolume)
{
    DWORD dwHighestVolume = m_dwMasterRanges.dwMinimum;
    DWORD dwLoudestChIdx = 0;

    /* Find the loudest channel of all available */
    for (DWORD i = 0; i < m_dwMasterChannels; ++i)
    {
        if (pmxcdVolume[i].dwValue < dwHighestVolume)
            continue;

        dwLoudestChIdx = i;
        dwHighestVolume = pmxcdVolume[dwLoudestChIdx].dwValue;
    }

    /* Calculate steps for uneven channels */
    for (DWORD i = 0; i < m_dwMasterChannels; ++i)
    {
        /* As we rely on MIXERLINEW.cChannels, there may be a case of multiple channels having equal high volume. */
        if (dwHighestVolume == m_dwMasterRanges.dwMinimum)
        {
            TRACE("CMultimediaBackend::CalculateSteps: All channels were at minimum volume, steps will be unchanged\n");
            break;
        }
        else if (i == dwLoudestChIdx || (pmxcdVolume[i].dwValue == dwHighestVolume))
        {
            m_pdwChannelSteps[i] = (m_dwMasterRanges.dwMaximum - m_dwMasterRanges.dwMinimum) / VOLUME_STEPS_COUNT;
            TRACE("CMultimediaBackend::CalculateSteps: Channel %lu is the loudest, volume step will be %lu\n", i, m_pdwChannelSteps[i]);
        }
        else
        {
            if (dwHighestVolume > 0)
            {
                float dwStepRatio = (float)pmxcdVolume[i].dwValue / dwHighestVolume;
                m_pdwChannelSteps[i] = ceil(dwStepRatio * m_pdwChannelSteps[dwLoudestChIdx]);
            }
            else
            {
                m_pdwChannelSteps[i] = 0;
            }
            TRACE("CMultimediaBackend::CalculateSteps: Channel %lu volume step will be %lu\n", i, m_pdwChannelSteps[i]);
        }
    }
}

void
CMultimediaBackend::LogError(_In_ const char* pszFunctionName, _In_ const char* pszMessage, _In_ MMRESULT mmres)
{
    static WCHAR pszWinMMErrText[MAXERRORLENGTH] = {0};
    waveOutGetErrorTextW(mmres, pszWinMMErrText, _countof(pszWinMMErrText));    
    ERR("CMultimediaBackend::%s: %: %lu (%S)\n", pszFunctionName, pszMessage, mmres, pszWinMMErrText);
}

MMRESULT
CMultimediaBackend::GetCurrentVolume(_Inout_ PMIXERCONTROLDETAILS pmxcd)
{
    PMIXERCONTROLDETAILS_UNSIGNED mxcdVolume =
        (PMIXERCONTROLDETAILS_UNSIGNED)HeapAlloc(hProcessHeap,
                                                 0,
                                                 m_dwMasterChannels * sizeof(*mxcdVolume));

    if (mxcdVolume == NULL)
    {
        ERR("CMultimediaBackend::GetCurrentVolume: HeapAlloc failed, requested %lu bytes\n", m_dwMasterChannels * sizeof(*mxcdVolume));
        return MMSYSERR_NOMEM;
    }

    pmxcd->cChannels = m_dwMasterChannels;
    pmxcd->dwControlID = m_dwMasterVolumeID;
    pmxcd->cbDetails = sizeof(*mxcdVolume);
    pmxcd->paDetails = mxcdVolume;

    return mixerGetControlDetailsW((HMIXEROBJ)m_hMixer, pmxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
}
