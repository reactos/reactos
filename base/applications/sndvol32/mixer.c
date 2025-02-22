/*
 * ReactOS Sound Volume Control
 * Copyright (C) 2004-2005 Thomas Weidenmueller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        base/applications/sndvol32/mixer.c
 * PROGRAMMERS: Thomas Weidenmueller <w3seek@reactos.com>
 */

#include "sndvol32.h"

#define NO_MIXER_SELECTED ((UINT)(~0))

static VOID
ClearMixerCache(PSND_MIXER Mixer)
{
    PSND_MIXER_DESTINATION Line, NextLine;
    PSND_MIXER_CONNECTION Con, NextCon;

    for (Line = Mixer->Lines; Line != NULL; Line = NextLine)
    {
        if (Line->Controls != NULL)
        {
            HeapFree(GetProcessHeap(),
                     0,
                     Line->Controls);
        }

        for (Con = Line->Connections; Con != NULL; Con = NextCon)
        {
            if (Con->Controls != NULL)
            {
                HeapFree(GetProcessHeap(),
                         0,
                         Con->Controls);
            }

            NextCon = Con->Next;
            HeapFree(GetProcessHeap(),
                     0,
                     Con);
        }

        NextLine = Line->Next;
        HeapFree(GetProcessHeap(),
                 0,
                 Line);
    }
    Mixer->Lines = NULL;
}

PSND_MIXER
SndMixerCreate(HWND hWndNotification, UINT MixerId)
{
    PSND_MIXER Mixer = (PSND_MIXER) HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(SND_MIXER));
    if (Mixer != NULL)
    {
        Mixer->hWndNotification = hWndNotification;
        Mixer->MixersCount = mixerGetNumDevs();
        Mixer->MixerId = NO_MIXER_SELECTED;

        if (Mixer->MixersCount > 0)
        {
            /* select the first mixer by default */
            SndMixerSelect(Mixer, MixerId);
        }
    }

    return Mixer;
}

VOID
SndMixerDestroy(PSND_MIXER Mixer)
{
    ClearMixerCache(Mixer);
    SndMixerClose(Mixer);
    HeapFree(GetProcessHeap(),
             0,
             Mixer);
}

VOID
SndMixerClose(PSND_MIXER Mixer)
{
    if (Mixer->hmx != NULL)
    {
      mixerClose(Mixer->hmx);
      Mixer->hmx = NULL;
      Mixer->MixerId = NO_MIXER_SELECTED;
    }
}

BOOL
SndMixerQueryControls(PSND_MIXER Mixer,
                      PUINT DisplayControls,
                      LPMIXERLINE LineInfo,
                      LPMIXERCONTROL *Controls)
{
    if (LineInfo->cControls > 0)
    {
        *Controls = (MIXERCONTROL*) HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              LineInfo->cControls * sizeof(MIXERCONTROL));
        if (*Controls != NULL)
        {
            MIXERLINECONTROLS LineControls;
            MMRESULT Result;
            UINT j;

            LineControls.cbStruct = sizeof(LineControls);
            LineControls.dwLineID = LineInfo->dwLineID;
            LineControls.cControls = LineInfo->cControls;
            LineControls.cbmxctrl = sizeof(MIXERCONTROL);
            LineControls.pamxctrl = (MIXERCONTROL*)(*Controls);

            Result = mixerGetLineControls((HMIXEROBJ)Mixer->hmx,
                                          &LineControls,
                                          MIXER_GETLINECONTROLSF_ALL);
            if (Result == MMSYSERR_NOERROR)
            {
                for (j = 0; j < LineControls.cControls; j++)
                {
                    if (SndMixerIsDisplayControl(Mixer,
                                                 &(*Controls)[j]))
                    {
                        (*DisplayControls)++;
                    }

                    DPRINT("Line control: %ws (0x%x, 0x%x)\n", (*Controls)[j].szName, (*Controls)[j].fdwControl, (*Controls)[j].dwControlType);
                }

                return TRUE;
            }
            else
            {
                HeapFree(GetProcessHeap(),
                         0,
                         *Controls);
                *Controls = NULL;
                DPRINT("Failed to get line (ID: 0x%x) controls: %d\n", LineInfo->dwLineID, Result);
            }
        }
        else
        {
            DPRINT("Failed to allocate memory for %d line (ID: 0x%x) controls!\n", LineInfo->dwLineID, LineInfo->cControls);
        }

        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

static BOOL
SndMixerQueryConnections(PSND_MIXER Mixer,
                         PSND_MIXER_DESTINATION Line)
{
    UINT i, DispControls;
    MIXERLINE LineInfo;
    MMRESULT Result;
    BOOL Ret = TRUE;

    LineInfo.cbStruct = sizeof(LineInfo);
    for (i = Line->Info.cConnections; i > 0; i--)
    {
        LineInfo.dwDestination = Line->Info.dwDestination;
        LineInfo.dwSource = i - 1;
        Result = mixerGetLineInfo((HMIXEROBJ)Mixer->hmx,
                                  &LineInfo,
                                  MIXER_GETLINEINFOF_SOURCE);
        if (Result == MMSYSERR_NOERROR)
        {
            LPMIXERCONTROL Controls = NULL;
            PSND_MIXER_CONNECTION Con;

            DPRINT("++ Source: %ws\n", LineInfo.szName);

            DispControls = 0;

            if (!SndMixerQueryControls(Mixer,
                                       &DispControls,
                                       &LineInfo,
                                       &Controls))
            {
                DPRINT("Failed to query connection controls\n");
                Ret = FALSE;
                break;
            }

            Con = (SND_MIXER_CONNECTION*) HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            sizeof(SND_MIXER_CONNECTION));
            if (Con != NULL)
            {
                Con->Info = LineInfo;
                Con->Controls = Controls;
                Con->DisplayControls = DispControls;
                Con->Next = Line->Connections;
                Line->Connections = Con;
            }
            else
            {
                HeapFree(GetProcessHeap(),
                         0,
                         Controls);
            }
        }
        else
        {
            DPRINT("Failed to get connection information: %d\n", Result);
            Ret = FALSE;
            break;
        }
    }

    return Ret;
}

static BOOL
SndMixerQueryDestinations(PSND_MIXER Mixer)
{
    UINT i;
    BOOL Ret = TRUE;

    for (i = Mixer->Caps.cDestinations; i > 0; i--)
    {
        PSND_MIXER_DESTINATION Line;

        Line = (SND_MIXER_DESTINATION*) HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         sizeof(SND_MIXER_DESTINATION));
        if (Line != NULL)
        {
            Line->Info.cbStruct = sizeof(Line->Info);
            Line->Info.dwDestination = i - 1;
            if (mixerGetLineInfo((HMIXEROBJ)Mixer->hmx,
                                 &Line->Info,
                                 MIXER_GETLINEINFOF_DESTINATION) == MMSYSERR_NOERROR)
            {
                DPRINT("+ Destination: %ws (0x%x, %d)\n", Line->Info.szName, Line->Info.dwLineID, Line->Info.dwComponentType);

                if (!SndMixerQueryControls(Mixer,
                                           &Line->DisplayControls,
                                           &Line->Info,
                                           &Line->Controls))
                {
                    DPRINT("Failed to query mixer controls!\n");
                    Ret = FALSE;
                    break;
                }

                if (!SndMixerQueryConnections(Mixer, Line))
                {
                    DPRINT("Failed to query mixer connections!\n");
                    Ret = FALSE;
                    break;
                }

                Line->Next = Mixer->Lines;
                Mixer->Lines = Line;
            }
            else
            {
                DPRINT("Failed to get line information for id %d!\n", i);
                HeapFree(GetProcessHeap(),
                         0,
                         Line);
                Ret = FALSE;
                break;
            }
        }
        else
        {
            DPRINT("Allocation of SND_MIXER_DEST structure for id %d failed!\n", i);
            Ret = FALSE;
            break;
        }
    }

    return Ret;
}

BOOL
SndMixerSelect(PSND_MIXER Mixer,
               UINT MixerId)
{
    if (MixerId >= Mixer->MixersCount)
    {
        return FALSE;
    }

    SndMixerClose(Mixer);

    if (mixerOpen(&Mixer->hmx,
                  MixerId,
                  (DWORD_PTR)Mixer->hWndNotification,
                  0,
                  CALLBACK_WINDOW | MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR ||
        mixerOpen(&Mixer->hmx,
                  MixerId,
                  (DWORD_PTR)Mixer->hWndNotification,
                  0,
                  CALLBACK_WINDOW) == MMSYSERR_NOERROR ||
        mixerOpen(&Mixer->hmx,
                  MixerId,
                  0,
                  0,
                  0) == MMSYSERR_NOERROR)
    {
        if (mixerGetDevCaps(MixerId,
                            &Mixer->Caps,
                            sizeof(Mixer->Caps)) == MMSYSERR_NOERROR)
        {
            BOOL Ret = FALSE;

            Mixer->MixerId = MixerId;

            ClearMixerCache(Mixer);

            Ret = SndMixerQueryDestinations(Mixer);

            if (!Ret)
            {
                ClearMixerCache(Mixer);
            }

            return Ret;
        }
        else
        {
            mixerClose(Mixer->hmx);
        }
    }

    Mixer->hmx = NULL;
    Mixer->MixerId = NO_MIXER_SELECTED;
    return FALSE;
}

UINT
SndMixerGetSelection(PSND_MIXER Mixer)
{
    return Mixer->MixerId;
}

INT
SndMixerGetProductName(PSND_MIXER Mixer,
                       LPTSTR lpBuffer,
                       UINT uSize)
{
    if (Mixer->hmx)
    {
        UINT lnsz = (UINT) lstrlen(Mixer->Caps.szPname);
        if(lnsz + 1 > uSize)
        {
            return lnsz + 1;
        }
        else
        {
            memcpy(lpBuffer, Mixer->Caps.szPname, lnsz * sizeof(TCHAR));
            lpBuffer[lnsz] = _T('\0');
            return lnsz;
        }
    }

    return -1;
}

INT
SndMixerGetLineName(PSND_MIXER Mixer,
                    DWORD LineID,
                    LPTSTR lpBuffer,
                    UINT uSize,
                    BOOL LongName)
{
    if (Mixer->hmx)
    {
        UINT lnsz;
        PSND_MIXER_DESTINATION Line;
        LPMIXERLINE lpl = NULL;

        for (Line = Mixer->Lines; Line != NULL; Line = Line->Next)
        {
            if (Line->Info.dwLineID == LineID)
            {
                lpl = &Line->Info;
                break;
            }
        }

        if (lpl != NULL)
        {
            lnsz = (UINT) lstrlen(LongName ? lpl->szName : lpl->szShortName);
            if(lnsz + 1 > uSize)
            {
                return lnsz + 1;
            }
            else
            {
                memcpy(lpBuffer, LongName ? lpl->szName : lpl->szShortName, lnsz * sizeof(TCHAR));
                lpBuffer[lnsz] = _T('\0');
                return lnsz;
            }
        }
    }

    return -1;
}

BOOL
SndMixerEnumProducts(PSND_MIXER Mixer,
                     PFNSNDMIXENUMPRODUCTS EnumProc,
                     PVOID Context)
{
    MIXERCAPS Caps;
    HMIXER hMixer;
    UINT i;
    BOOL Ret = TRUE;

    for (i = 0; i < Mixer->MixersCount; i++)
    {
        if (mixerOpen(&hMixer,
                      i,
                      0,
                      0,
                      0) == MMSYSERR_NOERROR)
        {
            if (mixerGetDevCaps(i,
                                &Caps,
                                sizeof(Caps)) == MMSYSERR_NOERROR)
            {
                if (!EnumProc(Mixer,
                              i,
                              Caps.szPname,
                              Context))
                {
                    mixerClose(hMixer);
                    Ret = FALSE;
                    break;
                }
            }
            else
            {
                DPRINT("Failed to get device capabilities for mixer id %d!\n", i);
            }
            mixerClose(hMixer);
        }
    }

    return Ret;
}

INT
SndMixerSetVolumeControlDetails(PSND_MIXER Mixer, DWORD dwControlID, DWORD cChannels, DWORD cbDetails, LPVOID paDetails)
{
    MIXERCONTROLDETAILS MixerDetails;

    if (Mixer->hmx)
    {
        MixerDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
        MixerDetails.dwControlID = dwControlID;
        MixerDetails.cChannels = cChannels;
        MixerDetails.cMultipleItems = 0;
        MixerDetails.cbDetails = cbDetails;
        MixerDetails.paDetails = paDetails;

        if (mixerSetControlDetails((HMIXEROBJ)Mixer->hmx, &MixerDetails, MIXER_SETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER) == MMSYSERR_NOERROR)
        {
            return 1;
        }
    }

    return -1;
}


INT
SndMixerGetVolumeControlDetails(PSND_MIXER Mixer, DWORD dwControlID, DWORD cChannels, DWORD cbDetails, LPVOID paDetails)
{
    MIXERCONTROLDETAILS MixerDetails;

    if (Mixer->hmx)
    {
        MixerDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
        MixerDetails.dwControlID = dwControlID;
        MixerDetails.cChannels = cChannels;
        MixerDetails.cMultipleItems = 0;
        MixerDetails.cbDetails = cbDetails;
        MixerDetails.paDetails = paDetails;

        if (mixerGetControlDetails((HMIXEROBJ)Mixer->hmx, &MixerDetails, MIXER_GETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER) == MMSYSERR_NOERROR)
        {
            return 1;
        }
    }
    return -1;
}

INT
SndMixerGetDestinationCount(PSND_MIXER Mixer)
{
    return (Mixer->hmx ? (INT)Mixer->Caps.cDestinations : -1);
}

BOOL
SndMixerEnumLines(PSND_MIXER Mixer,
                  PFNSNDMIXENUMLINES EnumProc,
                  PVOID Context)
{
    if (Mixer->hmx)
    {
        PSND_MIXER_DESTINATION Line;

        for (Line = Mixer->Lines; Line != NULL; Line = Line->Next)
        {
            if (!EnumProc(Mixer,
                          &Line->Info,
                          Line->DisplayControls,
                          Context))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    return FALSE;
}

BOOL
SndMixerEnumConnections(PSND_MIXER Mixer,
                        DWORD LineID,
                        PFNSNDMIXENUMCONNECTIONS EnumProc,
                        PVOID Context)
{
    if (Mixer->hmx)
    {
        PSND_MIXER_DESTINATION Line;

        for (Line = Mixer->Lines; Line != NULL; Line = Line->Next)
        {
            if (Line->Info.dwLineID == LineID)
            {
                PSND_MIXER_CONNECTION Connection;

                if (Line->DisplayControls != 0)
                {
                    if (!EnumProc(Mixer,
                                  LineID,
                                  &Line->Info,
                                  Context))
                    {
                        return FALSE;
                    }
                }

                for (Connection = Line->Connections; Connection != NULL; Connection = Connection->Next)
                {
                    if (!EnumProc(Mixer,
                                  LineID,
                                  &Connection->Info,
                                  Context))
                    {
                        return FALSE;
                    }
                }

                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL
SndMixerIsDisplayControl(PSND_MIXER Mixer,
                         LPMIXERCONTROL Control)
{
    if (Mixer->hmx && !(Control->fdwControl & MIXERCONTROL_CONTROLF_DISABLED))
    {
        switch (Control->dwControlType & MIXERCONTROL_CT_CLASS_MASK)
        {
            case MIXERCONTROL_CT_CLASS_FADER:
            case MIXERCONTROL_CT_CLASS_SWITCH:
                return TRUE;
        }
    }

    return FALSE;
}

LPMIXERLINE
SndMixerGetLineByName(PSND_MIXER Mixer,
                      DWORD LineID,
                      LPWSTR LineName)
{
    PSND_MIXER_DESTINATION Line;
    PSND_MIXER_CONNECTION Connection;

    if (Mixer->hmx == 0)
        return NULL;

    for (Line = Mixer->Lines; Line != NULL; Line = Line->Next)
    {
        if (Line->Info.dwLineID == LineID)
        {
            if (Line->DisplayControls != 0)
            {
                if (_wcsicmp(Line->Info.szName, LineName) == 0)
                {
                    return &Line->Info;
                }
            }

            for (Connection = Line->Connections; Connection != NULL; Connection = Connection->Next)
            {
                if (_wcsicmp(Connection->Info.szName, LineName) == 0)
                {
                    return &Connection->Info;
                }
            }

            return NULL;
        }
    }

    return NULL;
}
