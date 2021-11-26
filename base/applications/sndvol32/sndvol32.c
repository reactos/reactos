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
 * FILE:        base/applications/sndvol32/sndvol32.c
 * PROGRAMMERS: Thomas Weidenmueller <w3seek@reactos.com>
 */

#include "sndvol32.h"

#include <shellapi.h>

HINSTANCE hAppInstance;
ATOM MainWindowClass;
HWND hMainWnd;
HANDLE hAppHeap;
LPTSTR lpAppTitle;
PREFERENCES_CONTEXT Preferences;

#define GetDialogData(hwndDlg, type) \
    ( P##type )GetWindowLongPtr((hwndDlg), DWLP_USER)
#define GetWindowData(hwnd, type) \
    ( P##type )GetWindowLongPtr((hwnd), GWL_USERDATA)

/******************************************************************************/



typedef struct _PREFERENCES_FILL_DEVICES
{
    PPREFERENCES_CONTEXT PrefContext;
    HWND hComboBox;
    UINT Selected;
} PREFERENCES_FILL_DEVICES, *PPREFERENCES_FILL_DEVICES;

static BOOL CALLBACK
FillDeviceComboBox(PSND_MIXER Mixer,
                   UINT Id,
                   LPCTSTR ProductName,
                   PVOID Context)
{
    LRESULT lres;
    PPREFERENCES_FILL_DEVICES FillContext = (PPREFERENCES_FILL_DEVICES)Context;

    UNREFERENCED_PARAMETER(Mixer);

    lres = SendMessage(FillContext->hComboBox,
                       CB_ADDSTRING,
                       0,
                       (LPARAM)ProductName);
    if (lres != CB_ERR)
    {
        /* save the index so we don't screw stuff when the combobox is sorted... */
        SendMessage(FillContext->hComboBox,
                    CB_SETITEMDATA,
                    (WPARAM)lres,
                    Id);

        if (Id == FillContext->Selected)
        {
            SendMessage(FillContext->hComboBox,
                        CB_SETCURSEL,
                        (WPARAM)lres,
                        0);
        }
    }

    return TRUE;
}

static BOOL CALLBACK
PrefDlgAddLine(PSND_MIXER Mixer,
               LPMIXERLINE Line,
               UINT DisplayControls,
               PVOID Context)
{
    PPREFERENCES_CONTEXT PrefContext = (PPREFERENCES_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Mixer);
    UNREFERENCED_PARAMETER(DisplayControls);

    switch (Line->dwComponentType)
    {
        case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
            if (PrefContext->PlaybackID == (DWORD)-1)
            {
                PrefContext->PlaybackID = Line->dwLineID;

                if (PrefContext->SelectedLine == (DWORD)-1)
                {
                    PrefContext->SelectedLine = Line->dwLineID;
                }
            }
            else
                goto AddToOthersLines;

            break;

        case MIXERLINE_COMPONENTTYPE_DST_WAVEIN:
            if (PrefContext->RecordingID == (DWORD)-1)
            {
                PrefContext->RecordingID = Line->dwLineID;

                if (PrefContext->SelectedLine == (DWORD)-1)
                {
                    PrefContext->SelectedLine = Line->dwLineID;
                }
            }
            else
                goto AddToOthersLines;

            break;

        default:
        {
            LRESULT lres;
            HWND hwndCbOthers;

            if (PrefContext->SelectedLine == (DWORD)-1)
            {
                PrefContext->SelectedLine = Line->dwLineID;
            }

AddToOthersLines:
            hwndCbOthers = GetDlgItem(PrefContext->hwndDlg,
                                      IDC_LINE);

            lres = SendMessage(hwndCbOthers,
                               CB_ADDSTRING,
                               0,
                               (LPARAM)Line->szName);
            if (lres != CB_ERR)
            {
                SendMessage(hwndCbOthers,
                            CB_SETITEMDATA,
                            (WPARAM)lres,
                            Line->dwLineID);

                PrefContext->OtherLines++;
            }
            break;
        }
    }

    return TRUE;
}

static BOOL CALLBACK
PrefDlgAddConnection(PSND_MIXER Mixer,
                     DWORD LineID,
                     LPMIXERLINE Line,
                     PVOID Context)
{
    PPREFERENCES_CONTEXT PrefContext = (PPREFERENCES_CONTEXT)Context;
    HWND hwndControls;
    LVITEM lvi;
    UINT i;

    UNREFERENCED_PARAMETER(Mixer);
    UNREFERENCED_PARAMETER(LineID);

    if (Line->cControls != 0)
    {
        hwndControls = GetDlgItem(PrefContext->hwndDlg,
                                  IDC_CONTROLS);

        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = PrefContext->tmp++;
        lvi.iSubItem = 0;
        lvi.pszText = Line->szName;
        lvi.lParam = (LPARAM)Line->dwSource;

        i = SendMessage(hwndControls,
                        LVM_INSERTITEM,
                        0,
                        (LPARAM)&lvi);
        if (i != (UINT)-1)
        {
            TCHAR LineName[MIXER_LONG_NAME_CHARS];
            DWORD Flags;
            BOOL SelLine = FALSE;

            if (SndMixerGetLineName(PrefContext->Mixer,
                                    PrefContext->SelectedLine,
                                    LineName,
                                    MIXER_LONG_NAME_CHARS,
                                    TRUE) == -1)
            {
                LineName[0] = TEXT('\0');
            }

            if (ReadLineConfig(PrefContext->DeviceName,
                               LineName,
                               Line->szName,
                               &Flags))
            {
                if (Flags != 0x4)
                {
                    SelLine = TRUE;
                }
            }

            ListView_SetCheckState(hwndControls,
                                   i,
                                   SelLine);
        }
    }

    return TRUE;
}

static VOID
UpdatePrefDlgControls(PPREFERENCES_CONTEXT Context,
                      DWORD LineID)
{
    INT OldID, MixerID = 0;
    INT DeviceCbIndex;

    /* select the mixer */
    DeviceCbIndex = SendDlgItemMessage(Context->hwndDlg,
                                       IDC_MIXERDEVICE,
                                       CB_GETCURSEL,
                                       0,
                                       0);
    if (DeviceCbIndex != CB_ERR)
    {
        MixerID = SendDlgItemMessage(Context->hwndDlg,
                                     IDC_MIXERDEVICE,
                                     CB_GETITEMDATA,
                                     DeviceCbIndex,
                                     0);
        if (MixerID == CB_ERR)
        {
            MixerID = 0;
        }
    }

    OldID = Context->Selected;
    if (MixerID != OldID &&
        SndMixerSelect(Context->Mixer,
                       MixerID))
    {
        Context->Selected = SndMixerGetSelection(Context->Mixer);

        /* update the controls */
        Context->PlaybackID = (DWORD)-1;
        Context->RecordingID = (DWORD)-1;
        Context->OtherLines = 0;
        Context->SelectedLine = (DWORD)-1;

        SndMixerGetProductName(Context->Mixer,
                               Context->DeviceName,
                               sizeof(Context->DeviceName) / sizeof(Context->DeviceName[0]));

        if (SndMixerEnumLines(Context->Mixer,
                              PrefDlgAddLine,
                              Context))
        {
            UINT SelBox = 0;

            /* enable/disable controls and make default selection */
            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDC_PLAYBACK),
                         Context->PlaybackID != (DWORD)-1);
            CheckDlgButton(Context->hwndDlg,
                           IDC_PLAYBACK,
                           (Context->PlaybackID != (DWORD)-1 && SelBox++ == 0) ?
                               BST_CHECKED : BST_UNCHECKED);

            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDC_RECORDING),
                         Context->RecordingID != (DWORD)-1);
            CheckDlgButton(Context->hwndDlg,
                           IDC_RECORDING,
                           (Context->RecordingID != (DWORD)-1 && SelBox++ == 0) ?
                               BST_CHECKED : BST_UNCHECKED);

            if (Context->OtherLines != 0)
            {
                /* select the first item in the other lines combo box by default */
                SendDlgItemMessage(Context->hwndDlg,
                                   IDC_LINE,
                                   CB_SETCURSEL,
                                   0,
                                   0);
            }
            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDC_LINE),
                         FALSE);
            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDC_OTHER),
                         Context->OtherLines != 0);
            CheckDlgButton(Context->hwndDlg,
                           IDC_LINE,
                           (Context->OtherLines != 0 && SelBox++ == 0) ?
                               BST_CHECKED : BST_UNCHECKED);

            /* disable the OK button if the device doesn't have any lines */
            EnableWindow(GetDlgItem(Context->hwndDlg,
                                    IDOK),
                         Context->PlaybackID != (DWORD)-1 ||
                         Context->RecordingID != (DWORD)-1 ||
                         Context->OtherLines != 0);

            LineID = Context->SelectedLine;
        }
    }

    /* update the line sources list */
    if ((MixerID != OldID && Context->SelectedLine != (DWORD)-1) ||
        (Context->SelectedLine != LineID && LineID != (DWORD)-1))
    {
        Context->SelectedLine = LineID;

        (void)ListView_DeleteAllItems(GetDlgItem(Context->hwndDlg,
                                      IDC_CONTROLS));

        Context->tmp = 0;
        SndMixerEnumConnections(Context->Mixer,
                                LineID,
                                PrefDlgAddConnection,
                                Context);
    }
}

static
VOID
WriteLineSettings(PPREFERENCES_CONTEXT Context, HWND hwndDlg)
{
    HWND hwndControls;
    INT Count, Index;
    WCHAR LineName[MIXER_LONG_NAME_CHARS];
    WCHAR DestinationName[MIXER_LONG_NAME_CHARS];
    DWORD Flags;
    PSNDVOL_REG_LINESTATE LineStates;

    /* get list view */
    hwndControls = GetDlgItem(hwndDlg, IDC_CONTROLS);

    /* get list item count */
    Count = ListView_GetItemCount(hwndControls);

    /* sanity check */
    assert(Count);

    if (SndMixerGetLineName(Context->Mixer, Context->SelectedLine, DestinationName, MIXER_LONG_NAME_CHARS, TRUE) == -1)
    {
        /* failed to get destination line name */
        return;
    }

    /* allocate line states array */
    LineStates = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SNDVOL_REG_LINESTATE) * Count);
    if (LineStates == NULL)
    {
        /* failed to allocate line states array */
        return;
    }


    for(Index = 0; Index < Count; Index++)
    {
        /* set to empty */
        LineName[0] = L'\0';

        /* get item text */
        ListView_GetItemText(hwndControls, Index, 0, LineName, MIXER_LONG_NAME_CHARS);

        /* make sure it is null terminated */
        LineName[MIXER_LONG_NAME_CHARS-1] = L'\0';

        /* get check state */
        Flags = (ListView_GetCheckState(hwndControls, Index) == 0 ? 0x4 : 0);

        /* copy line name */
        wcscpy(LineStates[Index].LineName, LineName);

        /* store flags */
        LineStates[Index].Flags = Flags;
    }

    /* now write the line config */
    WriteLineConfig(Context->DeviceName, DestinationName, LineStates, sizeof(SNDVOL_REG_LINESTATE) * Count);

    /* free line states */
    HeapFree(GetProcessHeap(), 0, LineStates);
}

static INT_PTR CALLBACK
DlgPreferencesProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PPREFERENCES_CONTEXT Context;

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            Context = GetDialogData(hwndDlg,
                                    PREFERENCES_CONTEXT);
            switch (LOWORD(wParam))
            {
                case IDC_MIXERDEVICE:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        UpdatePrefDlgControls(Context,
                                              (DWORD)-1);
                    }
                    break;
                }

                case IDC_LINE:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        INT LineID;
                        INT Index;

                        Index = SendDlgItemMessage(hwndDlg,
                                                   IDC_LINE,
                                                   CB_GETCURSEL,
                                                   0,
                                                   0);
                        if (Index != CB_ERR)
                        {
                            LineID = SendDlgItemMessage(hwndDlg,
                                                        IDC_LINE,
                                                        CB_GETITEMDATA,
                                                        Index,
                                                        0);
                            if (LineID != CB_ERR)
                            {
                                UpdatePrefDlgControls(Context,
                                                      LineID);
                            }
                        }
                    }
                    break;
                }

                case IDC_PLAYBACK:
                {
                    UpdatePrefDlgControls(Context,
                                          Context->PlaybackID);
                    EnableWindow(GetDlgItem(hwndDlg,
                                            IDC_LINE),
                                 FALSE);
                    break;
                }

                case IDC_RECORDING:
                {
                    UpdatePrefDlgControls(Context,
                                          Context->RecordingID);
                    EnableWindow(GetDlgItem(hwndDlg,
                                            IDC_LINE),
                                 FALSE);
                    break;
                }

                case IDC_OTHER:
                {
                    INT LineCbIndex;
                    INT LineID;

                    EnableWindow(GetDlgItem(hwndDlg,
                                            IDC_LINE),
                                 TRUE);

                    LineCbIndex = SendDlgItemMessage(hwndDlg,
                                                     IDC_LINE,
                                                     CB_GETCURSEL,
                                                     0,
                                                     0);
                    if (LineCbIndex != CB_ERR)
                    {
                        LineID = SendDlgItemMessage(hwndDlg,
                                                    IDC_LINE,
                                                    CB_GETITEMDATA,
                                                    LineCbIndex,
                                                    0);
                        if (LineID != CB_ERR)
                        {
                            UpdatePrefDlgControls(Context,
                                                  LineID);
                        }
                    }
                    break;
                }

                case IDOK:
                {
                    /* write line settings */
                    WriteLineSettings(Context, hwndDlg);

                    /* fall through */
                }
                case IDCANCEL:
                {
                    EndDialog(hwndDlg,
                              LOWORD(wParam));
                    break;
                }
            }
            break;
        }

        case WM_INITDIALOG:
        {
            PREFERENCES_FILL_DEVICES FillDevContext;
            LVCOLUMN lvc;
            RECT rcClient;
            HWND hwndControls;

            SetWindowLongPtr(hwndDlg,
                             DWLP_USER,
                             (LONG_PTR)lParam);
            Context = (PPREFERENCES_CONTEXT)((LONG_PTR)lParam);
            Context->hwndDlg = hwndDlg;
            Context->Mixer = SndMixerCreate(hwndDlg, Context->MixerWindow->MixerId);
            Context->Selected = (UINT)-1;

            FillDevContext.PrefContext = Context;
            FillDevContext.hComboBox = GetDlgItem(hwndDlg,
                                                  IDC_MIXERDEVICE);
            FillDevContext.Selected = SndMixerGetSelection(Context->Mixer);
            SndMixerEnumProducts(Context->Mixer,
                                 FillDeviceComboBox,
                                 &FillDevContext);

            /* initialize the list view control */
            hwndControls = GetDlgItem(hwndDlg,
                                      IDC_CONTROLS);
            (void)ListView_SetExtendedListViewStyle(hwndControls,
                                                    LVS_EX_CHECKBOXES);

            GetClientRect(hwndControls,
                          &rcClient);
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = TEXT("");
            lvc.cx = rcClient.right;
            SendMessage(hwndControls,
                        LVM_INSERTCOLUMN,
                        0,
                        (LPARAM)&lvc);

            /* update all controls */
            UpdatePrefDlgControls(Context,
                                  (DWORD)Context->SelectedLine);
            return TRUE;
        }

        case WM_CLOSE:
        {
            EndDialog(hwndDlg,
                      IDCANCEL);
            break;
        }

        case WM_SYSCOLORCHANGE:
        {
            HWND hwndControls;

            /* Forward WM_SYSCOLORCHANGE */
            hwndControls = GetDlgItem(hwndDlg, IDC_CONTROLS);
            SendMessage(hwndControls, WM_SYSCOLORCHANGE, 0, 0);
            break;
        }
    }

    return 0;
}

/******************************************************************************/

static VOID
DeleteMixerWindowControls(PMIXER_WINDOW MixerWindow)
{
    DWORD Index;

    for(Index = 0; Index < MixerWindow->WindowCount; Index++)
    {
        /* destroys the window */
        DestroyWindow(MixerWindow->Window[Index]);
    }

    /* free memory */
    HeapFree(GetProcessHeap(), 0, MixerWindow->Window);

    /* set to null */
    MixerWindow->Window = NULL;
    MixerWindow->WindowCount = 0;
}

static BOOL
RebuildMixerWindowControls(PPREFERENCES_CONTEXT PrefContext)
{
    /* delete existing mixer controls */
    DeleteMixerWindowControls(PrefContext->MixerWindow);

    /* load new mixer controls */
    LoadDialogCtrls(PrefContext);

    return TRUE;
}

static
BOOL
CALLBACK
SetVolumeCallback(PSND_MIXER Mixer, DWORD LineID, LPMIXERLINE Line, PVOID Ctx)
{
    UINT ControlCount = 0, Index;
    LPMIXERCONTROL Control = NULL;
    PMIXERCONTROLDETAILS_UNSIGNED puDetails = NULL;
    MIXERCONTROLDETAILS_BOOLEAN bDetails;
    PSET_VOLUME_CONTEXT Context = (PSET_VOLUME_CONTEXT)Ctx;

    /* check if the line name is equal */
    if (wcsicmp(Line->szName, Context->LineName))
    {
        /* it is not */
        return TRUE;
    }

    /* query controls */
    if (SndMixerQueryControls(Mixer, &ControlCount, Line, &Control) == FALSE)
    {
        /* failed to query for controls */
        return FALSE;
    }

    puDetails = HeapAlloc(GetProcessHeap(), 0, Line->cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (puDetails == NULL)
        return FALSE;

    /* now go through all controls and compare control ids */
    for (Index = 0; Index < ControlCount; Index++)
    {
        if (Context->bVertical)
        {
            if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
            {
                DWORD LineOffset, volumePosition, balancePosition;
                DWORD volumeStep, balanceStep;

                LineOffset = Context->SliderPos;

                volumePosition = (DWORD)SendDlgItemMessage(Preferences.MixerWindow->hWnd, LineOffset * IDC_LINE_SLIDER_VERT, TBM_GETPOS, 0, 0);
                volumeStep = (Control[Index].Bounds.dwMaximum - Control[Index].Bounds.dwMinimum) / (VOLUME_MAX - VOLUME_MIN);

                if (Line->cChannels == 1)
                {
                    /* set up details */
                    puDetails[0].dwValue = ((VOLUME_MAX - volumePosition) * volumeStep) + Control[Index].Bounds.dwMinimum;
                }
                else if (Line->cChannels == 2)
                {
                    balancePosition = (DWORD)SendDlgItemMessage(Preferences.MixerWindow->hWnd, LineOffset * IDC_LINE_SLIDER_HORZ, TBM_GETPOS, 0, 0);
                    if (balancePosition == BALANCE_CENTER)
                    {
                        puDetails[0].dwValue = ((VOLUME_MAX - volumePosition) * volumeStep) + Control[Index].Bounds.dwMinimum;
                        puDetails[1].dwValue = ((VOLUME_MAX - volumePosition) * volumeStep) + Control[Index].Bounds.dwMinimum;
                    }
                    else if (balancePosition == BALANCE_LEFT)
                    {
                        puDetails[0].dwValue = ((VOLUME_MAX - volumePosition) * volumeStep) + Control[Index].Bounds.dwMinimum;
                        puDetails[1].dwValue = Control[Index].Bounds.dwMinimum;
                    }
                    else if (balancePosition == BALANCE_RIGHT)
                    {
                        puDetails[0].dwValue = Control[Index].Bounds.dwMinimum;
                        puDetails[1].dwValue = ((VOLUME_MAX - volumePosition) * volumeStep) + Control[Index].Bounds.dwMinimum;
                    }
                    else if (balancePosition < BALANCE_CENTER) // Left
                    {
                        puDetails[0].dwValue = ((VOLUME_MAX - volumePosition) * volumeStep) + Control[Index].Bounds.dwMinimum;

                        balanceStep = (puDetails[0].dwValue - Control[Index].Bounds.dwMinimum) / (BALANCE_STEPS / 2);

                        puDetails[1].dwValue = (balancePosition * balanceStep) + Control[Index].Bounds.dwMinimum;
                    }
                    else if (balancePosition > BALANCE_CENTER) // Right
                    {
                        puDetails[1].dwValue = ((VOLUME_MAX - volumePosition) * volumeStep) + Control[Index].Bounds.dwMinimum;

                        balanceStep = (puDetails[1].dwValue - Control[Index].Bounds.dwMinimum) / (BALANCE_STEPS / 2);

                        puDetails[0].dwValue = ((BALANCE_RIGHT - balancePosition) * balanceStep) + Control[Index].Bounds.dwMinimum;
                    }
                }
                else
                {
                    SndMixerGetVolumeControlDetails(Preferences.MixerWindow->Mixer, Control[Index].dwControlID, Line->cChannels, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID)puDetails);

                    /* FIXME */
                }

                /* set volume */
                SndMixerSetVolumeControlDetails(Preferences.MixerWindow->Mixer, Control[Index].dwControlID, Line->cChannels, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID)puDetails);

                /* done */
                break;
            }
        }
        else if (Context->bSwitch)
        {
            if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
            {
                /* set up details */
                bDetails.fValue = Context->SliderPos;

                /* set volume */
                SndMixerSetVolumeControlDetails(Preferences.MixerWindow->Mixer, Control[Index].dwControlID, 1, sizeof(MIXERCONTROLDETAILS_BOOLEAN), (LPVOID)&bDetails);

                /* done */
                break;
            }
        }
    }

    if (puDetails != NULL)
        HeapFree(GetProcessHeap(), 0, puDetails);

    /* free controls */
    HeapFree(GetProcessHeap(), 0, Control);


    /* done */
    return TRUE;
}

static
BOOL
CALLBACK
MixerControlChangeCallback(PSND_MIXER Mixer, DWORD LineID, LPMIXERLINE Line, PVOID Context)
{
    PMIXERCONTROLDETAILS_UNSIGNED pVolumeDetails = NULL;
    UINT ControlCount = 0, Index;
    LPMIXERCONTROL Control = NULL;

    /* check if the line has controls */
    if (Line->cControls == 0)
    {
        /* no controls */
        return TRUE;
    }

    /* query controls */
    if (SndMixerQueryControls(Mixer, &ControlCount, Line, &Control) == FALSE)
    {
        /* failed to query for controls */
        return FALSE;
    }

    pVolumeDetails = HeapAlloc(GetProcessHeap(),
                               0,
                               Line->cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (pVolumeDetails == NULL)
        goto done;

    /* now go through all controls and compare control ids */
    for (Index = 0; Index < ControlCount; Index++)
    {
        if (Control[Index].dwControlID == PtrToUlong(Context))
        {
            if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
            {
                MIXERCONTROLDETAILS_BOOLEAN Details;

                /* get volume control details */
                if (SndMixerGetVolumeControlDetails(Preferences.MixerWindow->Mixer, Control[Index].dwControlID, 1, sizeof(MIXERCONTROLDETAILS_BOOLEAN), (LPVOID)&Details) != -1)
                {
                    /* update dialog control */
                    UpdateDialogLineSwitchControl(&Preferences, Line, Details.fValue);
                }
            }
            else if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
            {
                /* get volume control details */
                if (SndMixerGetVolumeControlDetails(Preferences.MixerWindow->Mixer, Control[Index].dwControlID, Line->cChannels, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID)pVolumeDetails) != -1)
                {
                    /* update dialog control */
                    DWORD volumePosition, volumeStep, maxVolume, i;
                    DWORD balancePosition, balanceStep;

                    volumeStep = (Control[Index].Bounds.dwMaximum - Control[Index].Bounds.dwMinimum) / (VOLUME_MAX - VOLUME_MIN);

                    maxVolume = 0;
                    for (i = 0; i < Line->cChannels; i++)
                    {
                        if (pVolumeDetails[i].dwValue > maxVolume)
                            maxVolume = pVolumeDetails[i].dwValue;
                    }

                    volumePosition = (maxVolume - Control[Index].Bounds.dwMinimum) / volumeStep;

                    if (Line->cChannels == 1)
                    {
                        balancePosition = BALANCE_CENTER;
                    }
                    else if (Line->cChannels == 2)
                    {
                        if (pVolumeDetails[0].dwValue == pVolumeDetails[1].dwValue)
                        {
                            balancePosition = BALANCE_CENTER;
                        }
                        else if (pVolumeDetails[0].dwValue == Control[Index].Bounds.dwMinimum)
                        {
                            balancePosition = BALANCE_RIGHT;
                        }
                        else if (pVolumeDetails[1].dwValue == Control[Index].Bounds.dwMinimum)
                        {
                            balancePosition = BALANCE_LEFT;
                        }
                        else
                        {
                            balanceStep = (maxVolume - Control[Index].Bounds.dwMinimum) / (BALANCE_STEPS / 2);

                            if (pVolumeDetails[0].dwValue < pVolumeDetails[1].dwValue)
                            {
                                balancePosition = (pVolumeDetails[0].dwValue - Control[Index].Bounds.dwMinimum) / balanceStep;
                                balancePosition = BALANCE_RIGHT - balancePosition;
                            }
                            else if (pVolumeDetails[1].dwValue < pVolumeDetails[0].dwValue)
                            {
                                balancePosition = (pVolumeDetails[1].dwValue - Control[Index].Bounds.dwMinimum) / balanceStep;
                                balancePosition = BALANCE_LEFT + balancePosition;
                            }
                        }
                    }

                    /* Update the volume control slider */
                    UpdateDialogLineSliderControl(&Preferences, Line, IDC_LINE_SLIDER_VERT, VOLUME_MAX - volumePosition);

                    /* Update the balance control slider */
                    UpdateDialogLineSliderControl(&Preferences, Line, IDC_LINE_SLIDER_HORZ, balancePosition);
                }
            }
            break;
        }
    }

done:
    /* Free the volume details */
    if (pVolumeDetails)
        HeapFree(GetProcessHeap(), 0, pVolumeDetails);

    /* free controls */
    HeapFree(GetProcessHeap(), 0, Control);

    /* done */
    return TRUE;
}

static LRESULT CALLBACK
MainWindowProc(HWND hwnd,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PMIXER_WINDOW MixerWindow;
    DWORD CtrlID, LineOffset;
    BOOL bRet;
    LRESULT Result = 0;
    SET_VOLUME_CONTEXT Context;

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            MixerWindow = GetWindowData(hwnd,
                                        MIXER_WINDOW);

            switch (LOWORD(wParam))
            {
                case IDM_PROPERTIES:
                {
                    PREFERENCES_CONTEXT Pref;

                    Pref.MixerWindow = MixerWindow;
                    Pref.Mixer = NULL;
                    Pref.SelectedLine = Preferences.SelectedLine;

                    if (DialogBoxParam(hAppInstance,
                                       MAKEINTRESOURCE(IDD_PREFERENCES),
                                       hwnd,
                                       DlgPreferencesProc,
                                       (LPARAM)&Pref) == IDOK)
                    {
                        /* update window */
                        TCHAR szProduct[MAXPNAMELEN];

                        /* get mixer product name */
                        if (SndMixerGetProductName(Pref.Mixer,
                                                   szProduct,
                                                   sizeof(szProduct) / sizeof(szProduct[0])) == -1)
                        {
                            /* failed to get name */
                            szProduct[0] = L'\0';
                        }
                        else
                        {
                            /* copy product */
                            wcscpy(Preferences.DeviceName, szProduct);
                        }

                        /* destroy old status bar */
                        if (MixerWindow->Mode == NORMAL_MODE)
                            DestroyWindow(MixerWindow->hStatusBar);

                        /* update details */
                        Preferences.SelectedLine = Pref.SelectedLine;

                        /* destroy old mixer */
                        SndMixerDestroy(Preferences.MixerWindow->Mixer);

                        /* use new selected mixer */
                        Preferences.MixerWindow->Mixer = Pref.Mixer;

                        /* create status window */
                        if (MixerWindow->Mode == NORMAL_MODE)
                        {
                            MixerWindow->hStatusBar = CreateStatusWindow(WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS,
                                                                         NULL,
                                                                         hwnd,
                                                                         0);
                            if (MixerWindow->hStatusBar)
                            {
                                /* Set status bar */
                                SendMessage(MixerWindow->hStatusBar,
                                    WM_SETTEXT,
                                    0,
                                    (LPARAM)szProduct);
                            }
                        }

                        /* rebuild dialog controls */
                        RebuildMixerWindowControls(&Preferences);
                    }
                    break;
                }

                case IDM_ADVANCED_CONTROLS:
                    MixerWindow->bShowExtendedControls = !MixerWindow->bShowExtendedControls;
                    CheckMenuItem(GetMenu(hwnd),
                                  IDM_ADVANCED_CONTROLS,
                                  MF_BYCOMMAND | (MixerWindow->bShowExtendedControls ? MF_CHECKED : MF_UNCHECKED));
                    RebuildMixerWindowControls(&Preferences);
                    break;

                case IDM_EXIT:
                {
                    PostQuitMessage(0);
                    break;
                }

                case IDM_ABOUT:
                {
                    HICON hAppIcon = (HICON)GetClassLongPtrW(hwnd,
                                                             GCLP_HICON);
                    ShellAbout(hwnd,
                               lpAppTitle,
                               NULL,
                               hAppIcon);
                    break;
                }

                default:
                {
                    /* get button id */
                    CtrlID = LOWORD(wParam);

                    /* check if the message is from the line switch */
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        if (CtrlID % IDC_LINE_SWITCH == 0)
                        {
                            /* compute line offset */
                            LineOffset = CtrlID / IDC_LINE_SWITCH;

                            /* compute window id of line name static control */
                            CtrlID = LineOffset * IDC_LINE_NAME;

                            if (Preferences.MixerWindow->Mixer->MixerId == PLAY_MIXER)
                            {
                                /* get line name */
                                if (GetDlgItemTextW(hwnd, CtrlID, Context.LineName, MIXER_LONG_NAME_CHARS) != 0)
                                {
                                    /* setup context */
                                    Context.SliderPos = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                                    Context.bVertical = FALSE;
                                    Context.bSwitch = TRUE;

                                    /* set volume */
                                    SndMixerEnumConnections(Preferences.MixerWindow->Mixer, Preferences.SelectedLine, SetVolumeCallback, (LPVOID)&Context);
                                }
                            }
                            else if (Preferences.MixerWindow->Mixer->MixerId == RECORD_MIXER)
                            {
                                UINT i;

                                for (i = 0; i < Preferences.MixerWindow->DialogCount; i++)
                                {
                                    SendDlgItemMessageW(hwnd, (i + 1) * IDC_LINE_SWITCH, BM_SETCHECK, (WPARAM)((i + 1) == LineOffset), 0);
                                }
                            }
                        }
                        else if (CtrlID % IDC_LINE_ADVANCED == 0)
                        {
                            ADVANCED_CONTEXT AdvancedContext;

                            /* compute line offset */
                            LineOffset = CtrlID / IDC_LINE_ADVANCED;

                            /* compute window id of line name static control */
                            CtrlID = LineOffset * IDC_LINE_NAME;

                            /* get line name */
                            if (GetDlgItemTextW(hwnd, CtrlID, AdvancedContext.LineName, MIXER_LONG_NAME_CHARS) != 0)
                            {
                                AdvancedContext.MixerWindow = Preferences.MixerWindow;
                                AdvancedContext.Mixer = Preferences.MixerWindow->Mixer;
                                AdvancedContext.Line = SndMixerGetLineByName(Preferences.MixerWindow->Mixer,
                                                                             Preferences.SelectedLine,
                                                                             AdvancedContext.LineName);
                                if (AdvancedContext.Line)
                                {
                                    DialogBoxParam(hAppInstance,
                                                   MAKEINTRESOURCE(IDD_ADVANCED),
                                                   hwnd,
                                                   AdvancedDlgProc,
                                                   (LPARAM)&AdvancedContext);
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case MM_MIXM_LINE_CHANGE:
        {
            DPRINT("MM_MIXM_LINE_CHANGE\n");
            break;
        }

        case MM_MIXM_CONTROL_CHANGE:
        {
            DPRINT("MM_MIXM_CONTROL_CHANGE\n");

            /* get mixer window */
            MixerWindow = GetWindowData(hwnd,
                                        MIXER_WINDOW);

            /* sanity checks */
            assert(MixerWindow);
            assert(MixerWindow->Mixer->hmx == (HMIXER)wParam);

            SndMixerEnumConnections(MixerWindow->Mixer, Preferences.SelectedLine, MixerControlChangeCallback, (PVOID)lParam);
            break;
        }

        case WM_VSCROLL:
            switch (LOWORD(wParam))
            {
                case TB_THUMBTRACK:
                    /* get dialog item ctrl */
                    CtrlID = GetDlgCtrlID((HWND)lParam);

                    /* get line index */
                    LineOffset = CtrlID / IDC_LINE_SLIDER_VERT;

                    /* compute window id of line name static control */
                    CtrlID = LineOffset * IDC_LINE_NAME;

                    /* get line name */
                    if (GetDlgItemTextW(hwnd, CtrlID, Context.LineName, MIXER_LONG_NAME_CHARS) != 0)
                    {
                        /* setup context */
                        Context.SliderPos = LineOffset;
                        Context.bVertical = TRUE;
                        Context.bSwitch = FALSE;

                        /* set volume */
                        SndMixerEnumConnections(Preferences.MixerWindow->Mixer, Preferences.SelectedLine, SetVolumeCallback, (LPVOID)&Context);
                    }
                    break;

                case TB_ENDTRACK:
                    MixerWindow = GetWindowData(hwnd,
                                                MIXER_WINDOW);

                    /* get dialog item ctrl */
                    CtrlID = GetDlgCtrlID((HWND)lParam);

                    /* get line index */
                    LineOffset = CtrlID / IDC_LINE_SLIDER_VERT;

                    if (LineOffset == 1 && MixerWindow->Mixer->MixerId == 0)
                        PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ASYNC | SND_ALIAS_ID);
                    break;

                default:
                    break;
            }
            break;

        case WM_HSCROLL:
            switch (LOWORD(wParam))
            {
                case TB_THUMBTRACK:
                    /* get dialog item ctrl */
                    CtrlID = GetDlgCtrlID((HWND)lParam);

                    /* get line index */
                    LineOffset = CtrlID / IDC_LINE_SLIDER_HORZ;

                    /* compute window id of line name static control */
                    CtrlID = LineOffset * IDC_LINE_NAME;

                    /* get line name */
                    if (GetDlgItemTextW(hwnd, CtrlID, Context.LineName, MIXER_LONG_NAME_CHARS) != 0)
                    {
                        /* setup context */
                        Context.SliderPos = LineOffset;
                        Context.bVertical = TRUE;
                        Context.bSwitch = FALSE;

                        /* set volume */
                        SndMixerEnumConnections(Preferences.MixerWindow->Mixer, Preferences.SelectedLine, SetVolumeCallback, (LPVOID)&Context);
                    }
                    break;

                case TB_ENDTRACK:
                    MixerWindow = GetWindowData(hwnd,
                                                MIXER_WINDOW);

                    /* get dialog item ctrl */
                    CtrlID = GetDlgCtrlID((HWND)lParam);

                    /* get line index */
                    LineOffset = CtrlID / IDC_LINE_SLIDER_HORZ;

                    if (LineOffset == 1 && MixerWindow->Mixer->MixerId == 0)
                        PlaySound((LPCTSTR)SND_ALIAS_SYSTEMDEFAULT, NULL, SND_ASYNC | SND_ALIAS_ID);
                    break;

                default:
                    break;
            }
            break;


        case WM_CREATE:
        {
            MixerWindow = ((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd,
                             GWL_USERDATA,
                             (LONG_PTR)MixerWindow);
            MixerWindow->hWnd = hwnd;
            MixerWindow->Mixer = SndMixerCreate(MixerWindow->hWnd, MixerWindow->MixerId);
            if (MixerWindow->Mixer != NULL)
            {
                TCHAR szProduct[MAXPNAMELEN];

                /* get mixer product name */
                if (SndMixerGetProductName(MixerWindow->Mixer,
                                           szProduct,
                                           sizeof(szProduct) / sizeof(szProduct[0])) == -1)
                {
                    /* failed to get name */
                    szProduct[0] = L'\0';
                }


                /* initialize preferences */
                ZeroMemory(&Preferences, sizeof(Preferences));

                /* store mixer */
                Preferences.Mixer = MixerWindow->Mixer;

                /* store mixer window */
                Preferences.MixerWindow = MixerWindow;

                /* first destination line id */
                Preferences.SelectedLine = 0xFFFF0000;

                /* copy product */
                wcscpy(Preferences.DeviceName, szProduct);

                /* Disable the 'Advanced Controls' menu item */
                EnableMenuItem(GetMenu(hwnd), IDM_ADVANCED_CONTROLS, MF_BYCOMMAND | MF_GRAYED);

                /* Load the placement coordinate data of the window */
                bRet = LoadXYCoordWnd(&Preferences);
                if (bRet)
                {
                    /*
                     * LoadXYCoordWnd() might fail for the first time of opening the application which is normal as
                     * the Sound Control's applet settings haven't been saved yet to the Registry. At this point SetWindowPos()
                     * call is skipped.
                     */
                    SetWindowPos(hwnd, NULL, MixerWindow->WndPosX, MixerWindow->WndPosY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                }

                /* create status window */
                if (MixerWindow->Mode == NORMAL_MODE)
                {
                    MixerWindow->hStatusBar = CreateStatusWindow(WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS,
                                                                 NULL,
                                                                 hwnd,
                                                                 0);
                    if (MixerWindow->hStatusBar)
                    {
                        SendMessage(MixerWindow->hStatusBar,
                                    WM_SETTEXT,
                                    0,
                                   (LPARAM)szProduct);
                    }
                }

                if (!RebuildMixerWindowControls(&Preferences))
                {
                    DPRINT("Rebuilding mixer window controls failed!\n");
                    SndMixerDestroy(MixerWindow->Mixer);
                    MixerWindow->Mixer = NULL;
                    Result = -1;
                }
            }
            break;
        }

        case WM_DESTROY:
        {
            MixerWindow = GetWindowData(hwnd,
                                        MIXER_WINDOW);
            if (MixerWindow != NULL)
            {
                if (MixerWindow->Mixer != NULL)
                {
                    SndMixerDestroy(MixerWindow->Mixer);
                }
                if (MixerWindow->hFont)
                    DeleteObject(MixerWindow->hFont);
                HeapFree(hAppHeap, 0, MixerWindow);
            }
            break;
        }

        case WM_CLOSE:
        {
            SaveXYCoordWnd(hwnd, &Preferences);
            PostQuitMessage(0);
            break;
        }

        default:
        {
            Result = DefWindowProc(hwnd,
                                   uMsg,
                                   wParam,
                                   lParam);
            break;
        }
    }

    return Result;
}

static BOOL
RegisterApplicationClasses(VOID)
{
    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PMIXER_WINDOW);
    wc.hInstance = hAppInstance;
    wc.hIcon = LoadIcon(hAppInstance,
                        MAKEINTRESOURCE(IDI_MAINAPP));
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = SZ_APP_CLASS;
    wc.hIconSm = NULL;
    MainWindowClass = RegisterClassEx(&wc);

    return MainWindowClass != 0;
}

static VOID
UnregisterApplicationClasses(VOID)
{
    UnregisterClass(SZ_APP_CLASS,
                    hAppInstance);
}

static HWND
CreateApplicationWindow(
    WINDOW_MODE WindowMode,
    UINT MixerId)
{
    HWND hWnd;

    PMIXER_WINDOW MixerWindow = HeapAlloc(hAppHeap,
                                          HEAP_ZERO_MEMORY,
                                          sizeof(MIXER_WINDOW));
    if (MixerWindow == NULL)
    {
        return NULL;
    }

    MixerWindow->Mode = WindowMode;
    MixerWindow->MixerId = MixerId;

    if (mixerGetNumDevs() > 0)
    {
        hWnd = CreateWindowEx(WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT,
                              SZ_APP_CLASS,
                              lpAppTitle,
                              WS_DLGFRAME | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                              0, 0, 300, 315,
                              NULL,
                              LoadMenu(hAppInstance,
                                       MAKEINTRESOURCE(IDM_MAINMENU)),
                              hAppInstance,
                              MixerWindow);
    }
    else
    {
        LPTSTR lpErrMessage;

        /*
         * no mixer devices are available!
         */

        hWnd = NULL;
        if (AllocAndLoadString(&lpErrMessage,
                               hAppInstance,
                               IDS_NOMIXERDEVICES))
        {
            MessageBox(NULL,
                       lpErrMessage,
                       lpAppTitle,
                       MB_ICONINFORMATION);
            LocalFree(lpErrMessage);
        }
    }

    if (hWnd == NULL)
    {
        HeapFree(hAppHeap,
                 0,
                 MixerWindow);
    }

    return hWnd;
}

static
BOOL
HandleCommandLine(LPTSTR cmdline,
                  DWORD dwStyle,
                  PWINDOW_MODE pMode,
                  PUINT pMixerId)
{
    TCHAR option;

    *pMixerId = PLAY_MIXER;
    *pMode = (dwStyle & 0x20) ? SMALL_MODE : NORMAL_MODE;

    while (*cmdline == _T(' ') || *cmdline == _T('-') || *cmdline == _T('/'))
    {
        if (*cmdline++ == _T(' '))
            continue;

        option = *cmdline;
        if (option)
            cmdline++;
        while (*cmdline == _T(' '))
            cmdline++;

        switch (option)
        {
            case 'd': /* Device */
            case 'D':
                break;

            case 'n': /* Small size */
            case 'N':
                *pMode = NORMAL_MODE;
                break;

            case 's': /* Small size */
            case 'S':
                *pMode = SMALL_MODE;
                break;

            case 't': /* Tray size */
            case 'T':
                *pMode = TRAY_MODE;
                break;

            case 'p': /* Play mode */
            case 'P':
                *pMixerId = PLAY_MIXER;
                break;

            case 'r': /* Record mode */
            case 'R':
                *pMixerId = RECORD_MIXER;
                break;

            default:
                return FALSE;
        }
    }

    return TRUE;
}

int WINAPI
_tWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPTSTR lpszCmdLine,
          int nCmdShow)
{
    MSG Msg;
    int Ret = 1;
    INITCOMMONCONTROLSEX Controls;
    WINDOW_MODE WindowMode = SMALL_MODE;
    UINT MixerId = 0;
    DWORD dwStyle;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    hAppInstance = hInstance;
    hAppHeap = GetProcessHeap();

    if (InitAppConfig())
    {
        dwStyle = GetStyleValue();
        HandleCommandLine(lpszCmdLine, dwStyle, &WindowMode, &MixerId);

        /* load the application title */
        if (!AllocAndLoadString(&lpAppTitle,
                                hAppInstance,
                                IDS_SNDVOL32))
        {
            lpAppTitle = NULL;
        }

        Controls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        Controls.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;

        InitCommonControlsEx(&Controls);

        if (WindowMode == TRAY_MODE)
        {
            DialogBoxParam(hAppInstance,
                           MAKEINTRESOURCE(IDD_TRAY_MASTER),
                           NULL,
                           TrayDlgProc,
                           0);
        }
        else
        {
            if (RegisterApplicationClasses())
            {
                hMainWnd = CreateApplicationWindow(WindowMode, MixerId);
                if (hMainWnd != NULL)
                {
                    BOOL bRet;
                    while ((bRet =GetMessage(&Msg,
                                             NULL,
                                             0,
                                             0)) != 0)
                    {
                        if (bRet != -1)
                        {
                            TranslateMessage(&Msg);
                            DispatchMessage(&Msg);
                        }
                    }

                    DestroyWindow(hMainWnd);
                    Ret = 0;
                }
                else
                {
                    DPRINT("Failed to create application window (LastError: %d)!\n", GetLastError());
                }

                UnregisterApplicationClasses();
            }
            else
            {
                DPRINT("Failed to register application classes (LastError: %d)!\n", GetLastError());
            }
        }

        if (lpAppTitle != NULL)
        {
            LocalFree(lpAppTitle);
        }

        CloseAppConfig();
    }
    else
    {
        DPRINT("Unable to open the Volume Control registry key!\n");
    }

    return Ret;
}
