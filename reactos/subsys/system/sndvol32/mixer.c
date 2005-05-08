/*
 * ReactOS Sound Volume Control
 * Copyright (C) 2004 Thomas Weidenmueller
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * VMware is a registered trademark of VMware, Inc.
 */
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        subsys/system/sndvol32/mixer.c
 * PROGRAMMERS: Thomas Weidenmueller <w3seek@reactos.com>
 */
#include "sndvol32.h"

#define NO_MIXER_SELECTED (~0)

static VOID
ClearMixerCache(PSND_MIXER Mixer)
{
  PSND_MIXER_DESTINATION Line, NextLine;
  PSND_MIXER_CONNECTION Con, NextCon;

  for(Line = Mixer->Lines; Line != NULL; Line = NextLine)
  {
    if(Line->Controls != NULL)
    {
      HeapFree(GetProcessHeap(), 0, Line->Controls);
    }

    for(Con = Line->Connections; Con != NULL; Con = NextCon)
    {
      if(Con->Controls != NULL)
      {
        HeapFree(GetProcessHeap(), 0, Con->Controls);
      }

      NextCon = Con->Next;
      HeapFree(GetProcessHeap(), 0, Con);
    }

    NextLine = Line->Next;
    HeapFree(GetProcessHeap(), 0, Line);
  }
  Mixer->Lines = NULL;
}

PSND_MIXER
SndMixerCreate(HWND hWndNotification)
{
  PSND_MIXER Mixer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SND_MIXER));
  if(Mixer != NULL)
  {
    Mixer->hWndNotification = hWndNotification;
    Mixer->MixersCount = mixerGetNumDevs();
    Mixer->MixerId = NO_MIXER_SELECTED;

    if(Mixer->MixersCount > 0)
    {
      /* select the first mixer by default */
      SndMixerSelect(Mixer, 0);
    }
  }

  return Mixer;
}

VOID
SndMixerDestroy(PSND_MIXER Mixer)
{
  SndMixerClose(Mixer);
  HeapFree(GetProcessHeap(), 0, Mixer);
}

VOID
SndMixerClose(PSND_MIXER Mixer)
{
  if(Mixer->hmx != NULL)
  {
    mixerClose(Mixer->hmx);
    Mixer->hmx = NULL;
    Mixer->MixerId = NO_MIXER_SELECTED;
  }
}

static BOOL
SndMixerQueryControls(PSND_MIXER Mixer, LPMIXERLINE LineInfo, LPMIXERCONTROL *Controls)
{
  if(LineInfo->cControls > 0)
  {
    *Controls = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, LineInfo->cControls * sizeof(MIXERCONTROL));
    if(*Controls != NULL)
    {
      MIXERLINECONTROLS LineControls;
      UINT j;

      LineControls.cbStruct = sizeof(LineControls);
      LineControls.dwLineID = LineInfo->dwLineID;
      LineControls.cControls = LineInfo->cControls;
      LineControls.cbmxctrl = sizeof(MIXERCONTROL);
      LineControls.pamxctrl = (PVOID)(*Controls);

      for(j = 0; j < LineInfo->cControls; j++)
      {
        (*Controls)[j].cbStruct = sizeof(MIXERCONTROL);
      }

      if(mixerGetLineControls((HMIXEROBJ)Mixer->hmx, &LineControls, MIXER_GETLINECONTROLSF_ALL) == MMSYSERR_NOERROR)
      {
        for(j = 0; j < LineInfo->cControls; j++)
        {
          DBG("Line control: %ws", (*Controls)[j].szName);
        }

        return TRUE;
      }
      else
      {
        HeapFree(GetProcessHeap(), 0, *Controls);
        *Controls = NULL;
        DBG("Failed to get line controls!\n");
      }
    }
    else
    {
      DBG("Failed to allocate memory for %d line controls!\n", LineInfo->cControls);
    }

    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

static BOOL
SndMixerQueryConnections(PSND_MIXER Mixer, PSND_MIXER_DESTINATION Line)
{
  UINT i;
  MIXERLINE LineInfo;
  BOOL Ret = TRUE;

  LineInfo.cbStruct = sizeof(LineInfo);
  LineInfo.dwDestination = Line->Info.dwDestination;
  for(i = Line->Info.cConnections; i > 0; i--)
  {
    LineInfo.dwSource = i - 1;
    if(mixerGetLineInfo((HMIXEROBJ)Mixer->hmx, &LineInfo, MIXER_GETLINEINFOF_SOURCE) == MMSYSERR_NOERROR)
    {
      LPMIXERCONTROL Controls;
      PSND_MIXER_CONNECTION Con;

      if(!SndMixerQueryControls(Mixer, &LineInfo, &Controls))
      {
        DBG("Failed to query connection controls\n");
        Ret = FALSE;
        break;
      }

      Con = HeapAlloc(GetProcessHeap(), 0, sizeof(SND_MIXER_CONNECTION));
      if(Con != NULL)
      {
        Con->Info = LineInfo;
        Con->Controls = Controls;
        Con->Next = Line->Connections;
        Line->Connections = Con;
      }
      else
      {
        HeapFree(GetProcessHeap(), 0, Controls);
      }
    }
    else
    {
      DBG("Failed to get connection information!\n");
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

  for(i = Mixer->Caps.cDestinations; i > 0; i--)
  {
    PSND_MIXER_DESTINATION Line;

    Line = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SND_MIXER_DESTINATION));
    if(Line != NULL)
    {
      Line->Info.cbStruct = sizeof(Line->Info);
      Line->Info.dwDestination = i - 1;
      if(mixerGetLineInfo((HMIXEROBJ)Mixer->hmx, &Line->Info, MIXER_GETLINEINFOF_DESTINATION) == MMSYSERR_NOERROR)
      {
        if(!SndMixerQueryConnections(Mixer, Line))
        {
          DBG("Failed to query mixer connections!\n");
          Ret = FALSE;
          break;
        }
        if(!SndMixerQueryControls(Mixer, &Line->Info, &Line->Controls))
        {
          DBG("Failed to query mixer controls!\n");
          Ret = FALSE;
          break;
        }

        Line->Next = Mixer->Lines;
        Mixer->Lines = Line;
      }
      else
      {
        DBG("Failed to get line information for id %d!\n", i);
        HeapFree(GetProcessHeap(), 0, Line);
        Ret = FALSE;
        break;
      }
    }
    else
    {
      DBG("Allocation of SND_MIXER_DEST structure for id %d failed!\n", i);
      Ret = FALSE;
      break;
    }
  }

  return Ret;
}

BOOL
SndMixerSelect(PSND_MIXER Mixer, UINT MixerId)
{
  if(MixerId >= Mixer->MixersCount)
  {
    return FALSE;
  }

  SndMixerClose(Mixer);

  if(mixerOpen(&Mixer->hmx, MixerId, (DWORD_PTR)Mixer->hWndNotification, 0, CALLBACK_WINDOW | MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR ||
     mixerOpen(&Mixer->hmx, MixerId, (DWORD_PTR)Mixer->hWndNotification, 0, CALLBACK_WINDOW) == MMSYSERR_NOERROR ||
     mixerOpen(&Mixer->hmx, MixerId, 0, 0, 0) == MMSYSERR_NOERROR)
  {
    if(mixerGetDevCaps(MixerId, &Mixer->Caps, sizeof(Mixer->Caps)) == MMSYSERR_NOERROR)
    {
      BOOL Ret = FALSE;

      Mixer->MixerId = MixerId;

      ClearMixerCache(Mixer);

      Ret = SndMixerQueryDestinations(Mixer);

      if(!Ret)
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
SndMixerGetProductName(PSND_MIXER Mixer, LPTSTR lpBuffer, UINT uSize)
{
  if(Mixer->hmx)
  {
    int lnsz = lstrlen(Mixer->Caps.szPname);
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

BOOL
SndMixerEnumProducts(PSND_MIXER Mixer, PFNSNDMIXENUMPRODUCTS EnumProc, PVOID Context)
{
  MIXERCAPS Caps;
  HMIXER hMixer;
  UINT i;
  BOOL Ret = TRUE;

  for(i = 0; i < Mixer->MixersCount; i++)
  {
    if(mixerOpen(&hMixer, i, 0, 0, 0) == MMSYSERR_NOERROR)
    {
      if(mixerGetDevCaps(i, &Caps, sizeof(Caps)) == MMSYSERR_NOERROR)
      {
        if(!EnumProc(Mixer, i, Caps.szPname, Context))
        {
          mixerClose(hMixer);
          Ret = FALSE;
          break;
        }
      }
      else
      {
        DBG("Failed to get device capabilities for mixer id %d!\n", i);
      }
      mixerClose(hMixer);
    }
  }

  return Ret;
}

INT
SndMixerGetDestinationCount(PSND_MIXER Mixer)
{
  return (Mixer->hmx ? Mixer->Caps.cDestinations : -1);
}

BOOL
SndMixerEnumLines(PSND_MIXER Mixer, PFNSNDMIXENUMLINES EnumProc, PVOID Context)
{
  if(Mixer->hmx)
  {
    PSND_MIXER_DESTINATION Line;

    for(Line = Mixer->Lines; Line != NULL; Line = Line->Next)
    {
      if(!EnumProc(Mixer, &Line->Info, Context))
      {
        return FALSE;
      }
    }

    return TRUE;
  }

  return FALSE;
}

