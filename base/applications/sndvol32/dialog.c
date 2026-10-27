/*
 * PROJECT:     ReactOS Sound Volume Control
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Dialog control routines
 * COPYRIGHT:   Copyright 2011 Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2018-2019 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2026 Vitaly Orekhov <vkvo2000@vivaldi.net>
 */

#include "sndvol32.h"


VOID
ConvertRect(LPRECT lpRect, UINT xBaseUnit, UINT yBaseUnit)
{
    lpRect->left = MulDiv(lpRect->left, xBaseUnit, 4);
    lpRect->right = MulDiv(lpRect->right, xBaseUnit, 4);
    lpRect->top = MulDiv(lpRect->top, yBaseUnit, 8);
    lpRect->bottom = MulDiv(lpRect->bottom, yBaseUnit, 8);
}

LPVOID
LoadDialogResource(
    IN HMODULE hModule,
    IN LPCWSTR ResourceName,
    OUT LPDWORD ResourceLength)
{
    HRSRC hSrc;
    HGLOBAL hRes;
    PVOID Result;

    /* find resource */
    hSrc = FindResourceW(hModule, ResourceName, (LPCWSTR)RT_DIALOG);

    if (!hSrc)
    {
        /* failed to find resource */
        return NULL;
    }

    /* now load the resource */
    hRes = LoadResource(hAppInstance, hSrc);
    if (!hRes)
    {
        /* failed to load resource */
        return NULL;
    }

    /* now lock the resource */
    Result = LockResource(hRes);

    if (!Result)
    {
        /* failed to lock resource */
        return NULL;
    }

    if (ResourceLength)
    {
        /* store output length */
        *ResourceLength = SizeofResource(hAppInstance, hSrc);
    }

    /* done */
    return Result;
}

LPWORD
AddDialogControl(
    IN HWND hwndDialog,
    OUT HWND *OutWnd,
    IN LPRECT DialogOffset,
    IN PDLGITEMTEMPLATE DialogItem,
    IN DWORD DialogIdMultiplier,
    IN HFONT hFont,
    IN UINT xBaseUnit,
    IN UINT yBaseUnit,
    IN UINT MixerId)
{
    RECT rect;
    LPWORD Offset;
    LPWSTR ClassName, WindowName;
    WCHAR WindowIdBuf[sizeof("#65535")];
    HWND hwnd;
    DWORD wID;
    INT nSteps, i;

    /* initialize client rectangle */
    rect.left = DialogItem->x;
    rect.top = DialogItem->y;
    rect.right = DialogItem->x + DialogItem->cx;
    rect.bottom = DialogItem->y + DialogItem->cy;

    /* Convert Dialog units to pixes */
    ConvertRect(&rect, xBaseUnit, yBaseUnit);

    rect.left += DialogOffset->left;
    rect.right += DialogOffset->left;
    rect.top += DialogOffset->top;
    rect.bottom += DialogOffset->top;

    /* move offset after dialog item */
    Offset = (LPWORD)(DialogItem + 1);

    if (*Offset == 0xFFFF)
    {
        /* class is encoded as type */
        Offset++;

        /* get control type */
        switch(*Offset)
        {
            case 0x80:
                ClassName = L"button";
                break ;
            case 0x82:
                ClassName = L"static";
                break;
            default:
               /* FIXME */
               assert(0);
               ClassName = NULL;
        }
        Offset++;
    }
    else
    {
        /* class name is encoded as string */
        ClassName = (LPWSTR)(Offset);

        /* move offset to the end of class string */
        Offset += wcslen(ClassName) + 1;
    }

    if (*Offset == 0xFFFF)
    {
        /* Window name is encoded as ordinal */
        Offset++;
        wsprintf(WindowIdBuf, L"#%u", (DWORD)*Offset);
        WindowName = WindowIdBuf;
        Offset++;
    }
    else
    {
        /* window name is encoded as string */
        WindowName = (LPWSTR)(Offset);

        /* move offset to the end of class string */
        Offset += wcslen(WindowName) + 1;
    }

    if (DialogItem->id == MAXWORD)
    {
        /* id is not important */
        wID = DialogItem->id;
    }
    else
    {
        /* calculate id */
        wID = DialogItem->id * (DialogIdMultiplier + 1);

    }

    /* now create the window */
    hwnd = CreateWindowExW(DialogItem->dwExtendedStyle,
                           ClassName,
                           WindowName,
                           DialogItem->style,
                           rect.left,
                           rect.top,
                           rect.right - rect.left,
                           rect.bottom - rect.top,
                           hwndDialog,
                           UlongToPtr(wID),
                           hAppInstance,
                           NULL);

    /* sanity check */
    assert(hwnd);

    /* store window */
    *OutWnd = hwnd;

    /* check if this the track bar */
    if (!_wcsicmp(ClassName, L"msctls_trackbar32"))
    {
        if (DialogItem->style & TBS_VERT)
        {
            /* Vertical trackbar: Volume */

            /* Disable the volume trackbar by default */
            EnableWindow(hwnd, FALSE);

            /* set up range */
            SendMessage(hwnd, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(VOLUME_MIN, VOLUME_MAX));

            /* set up page size */
            SendMessage(hwnd, TBM_SETPAGESIZE, 0, (LPARAM)VOLUME_PAGE_SIZE);

            /* set position */
            SendMessage(hwnd, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);

            /* Calculate and set ticks */
            nSteps = (VOLUME_MAX / (VOLUME_TICKS + 1));
            if (VOLUME_MAX % (VOLUME_TICKS + 1) != 0)
                nSteps++;
            for (i = nSteps; i < VOLUME_MAX; i += nSteps)
                SendMessage(hwnd, TBM_SETTIC, 0, (LPARAM)i);
        }
        else
        {
            /* Horizontal trackbar: Balance */

            /* Disable the balance trackbar by default */
            EnableWindow(hwnd, FALSE);

            /* set up range */
            SendMessage(hwnd, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, BALANCE_STEPS));

            /* set up page size */
            SendMessage(hwnd, TBM_SETPAGESIZE, 0, (LPARAM)BALANCE_PAGE_SIZE);

            /* set position */
            SendMessage(hwnd, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)BALANCE_STEPS / 2);

            /* Calculate and set ticks */
            nSteps = (BALANCE_STEPS / (BALANCE_TICKS + 1));
            if (BALANCE_STEPS % (BALANCE_TICKS + 1) != 0)
                nSteps++;
            for (i = nSteps; i < BALANCE_STEPS; i += nSteps)
                SendMessage(hwnd, TBM_SETTIC, 0, (LPARAM)i);
        }
    }
    else if (!_wcsicmp(ClassName, L"static"))
    {
        /* Set font */
        SendMessageW(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    else if (!_wcsicmp(ClassName, L"button"))
    {
        if (DialogItem->id == IDC_LINE_SWITCH)
        {
            if (MixerId == PLAY_MIXER)
            {
                /* Disable checkboxes by default, if we are in play mode */
                EnableWindow(hwnd, FALSE);
            }
        }
        else if (DialogItem->id == IDC_LINE_ADVANCED)
        {
            ShowWindow(hwnd, SW_HIDE);
        }

        /* Set font */
        SendMessageW(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
    }


    /* check if there is additional data */
    if (*Offset == 0)
    {
        /* no additional data */
        Offset++;
    }
    else
    {
        /* FIXME: Determine whether this should be "Offset += 1 + *Offset" to explicitly skip the data count too. */
        /* skip past additional data */
        Offset += *Offset;
    }

    /* make sure next template is word-aligned */
    Offset = (LPWORD)(((ULONG_PTR)Offset + 3) & ~3);

    /* done */
    return Offset;
}

VOID
LoadDialogControls(
    IN PMIXER_WINDOW MixerWindow,
    LPRECT DialogOffset,
    WORD ItemCount,
    PDLGITEMTEMPLATE DialogItem,
    DWORD DialogIdMultiplier,
    UINT xBaseUnit,
    UINT yBaseUnit)
{
    LPWORD Offset;
    WORD Index;

    /* sanity check */
    assert(ItemCount);

    if (MixerWindow->Window)
        MixerWindow->Window = (HWND*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MixerWindow->Window, (MixerWindow->WindowCount + ItemCount) * sizeof(HWND));
    else
        MixerWindow->Window = (HWND*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ItemCount * sizeof(HWND));
    if (!MixerWindow->Window)
    {
        /* no memory */
        return;
    }

    /* enumerate now all controls */
    for (Index = 0; Index < ItemCount; Index++)
    {
        /* add controls */
        Offset = AddDialogControl(MixerWindow->hWnd,
                                  &MixerWindow->Window[MixerWindow->WindowCount],
                                  DialogOffset,
                                  DialogItem,
                                  DialogIdMultiplier,
                                  MixerWindow->hFont,
                                  xBaseUnit,
                                  yBaseUnit,
                                  MixerWindow->MixerId);

        /* sanity check */
        assert(Offset);

        /* move dialog item to new offset */
        DialogItem =(PDLGITEMTEMPLATE)Offset;

        /* increment window count */
        MixerWindow->WindowCount++;
    }
}

VOID
LoadDialog(
    IN HMODULE hModule,
    IN PMIXER_WINDOW MixerWindow,
    IN LPCWSTR DialogResId,
    IN DWORD Index)
{
    LPDLGTEMPLATE DlgTemplate;
    PDLGITEMTEMPLATE DlgItem;
    RECT dialogRect;
    LPWORD Offset;
    WORD FontSize;
    WCHAR FontName[100];
    SIZE_T Length;
    int width;

    DWORD units = GetDialogBaseUnits();
    UINT xBaseUnit = LOWORD(units);
    UINT yBaseUnit = HIWORD(units);

    /* first load the dialog resource */
    DlgTemplate = (LPDLGTEMPLATE)LoadDialogResource(hModule, DialogResId, NULL);
    if (!DlgTemplate)
    {
        /* failed to load resource */
        return;
    }

    /* Now walk past the dialog header */
    Offset = (LPWORD)(DlgTemplate + 1);

    /* FIXME: support menu */
    assert(*Offset == 0);
    Offset++;

    /* FIXME: support classes */
    assert(*Offset == 0);
    Offset++;

    /* FIXME: support titles */
    assert(*Offset == 0);
    Offset++;

    /* get font size */
    FontSize = *Offset;
    Offset++;

    /* calculate font length */
    Length = wcslen((LPWSTR)Offset) + 1;
    assert(Length < (sizeof(FontName) / sizeof(WCHAR)));

    /* copy font */
    wcscpy(FontName, (LPWSTR)Offset);

    if (DlgTemplate->style & DS_SETFONT)
    {
        HDC hDC;

        hDC = GetDC(0);

        if (!MixerWindow->hFont)
        {
            int pixels = MulDiv(FontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
            MixerWindow->hFont = CreateFontW(-pixels, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, FontName);
        }

        if (MixerWindow->hFont)
        {
            SIZE charSize;
            HFONT hOldFont;

            hOldFont = SelectObject(hDC, MixerWindow->hFont);
            charSize.cx = GdiGetCharDimensions(hDC, NULL, &charSize.cy);
            if (charSize.cx)
            {
                xBaseUnit = charSize.cx;
                yBaseUnit = charSize.cy;
            }
            SelectObject(hDC, hOldFont);

            MixerWindow->baseUnit.cx = charSize.cx;
            MixerWindow->baseUnit.cy = charSize.cy;
        }

        ReleaseDC(NULL, hDC);
    }

//    assert(MixerWindow->hFont);

    /* move offset after font name */
    Offset += Length;

    /* offset is now at first dialog item control */
    DlgItem = (PDLGITEMTEMPLATE)Offset;

    dialogRect.left = 0;
    dialogRect.right = DlgTemplate->cx;
    dialogRect.top = 0;
    dialogRect.bottom = DlgTemplate->cy;

    ConvertRect(&dialogRect, xBaseUnit, yBaseUnit);

    width = dialogRect.right - dialogRect.left;

    dialogRect.left += MixerWindow->rect.right;
    dialogRect.right += MixerWindow->rect.right;
    dialogRect.top += MixerWindow->rect.top;
    dialogRect.bottom += MixerWindow->rect.top;

    MixerWindow->rect.right += width;
    if ((dialogRect.bottom - dialogRect.top) > (MixerWindow->rect.bottom - MixerWindow->rect.top))
        MixerWindow->rect.bottom = MixerWindow->rect.top + dialogRect.bottom - dialogRect.top;

    /* now add the controls */
    LoadDialogControls(MixerWindow, &dialogRect, DlgTemplate->cdit, DlgItem, Index, xBaseUnit, yBaseUnit);
}

BOOL
CALLBACK
EnumConnectionsCallback(
    PSND_MIXER Mixer,
    DWORD LineID,
    LPMIXERLINE Line,
    PVOID Context)
{
    WCHAR LineName[MIXER_LONG_NAME_CHARS];
    DWORD Flags;
    DWORD wID;
    UINT ControlCount = 0, Index;
    LPMIXERCONTROL Control = NULL;
    HWND hDlgCtrl;
    PMIXERCONTROLDETAILS_UNSIGNED pVolumeDetails = NULL;
    PPREFERENCES_CONTEXT PrefContext = (PPREFERENCES_CONTEXT)Context;

    if (Line->cControls == 0)
        return TRUE;

    /* get line name */
    if (SndMixerGetLineName(PrefContext->MixerWindow->Mixer, PrefContext->SelectedLine, LineName, MIXER_LONG_NAME_CHARS, TRUE) == -1)
    {
        /* failed to get line name */
        LineName[0] = L'\0';
    }

    pVolumeDetails = HeapAlloc(GetProcessHeap(),
                               0,
                               Line->cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
    if (pVolumeDetails == NULL)
        goto done;

    /* check if line is found in registry settings */
    if (ReadLineConfig(PrefContext->DeviceName,
                       LineName,
                       Line->szName,
                       &Flags))
    {
          /* is it selected */
          if (Flags != 0x4)
          {
              int dlgId;

              if ((Line->dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS) ||
                  (Line->dwComponentType == MIXERLINE_COMPONENTTYPE_DST_HEADPHONES))
                  dlgId = (PrefContext->MixerWindow->Mode == SMALL_MODE) ? IDD_SMALL_MASTER : IDD_NORMAL_MASTER;
              else
                  dlgId = (PrefContext->MixerWindow->Mode == SMALL_MODE) ? IDD_SMALL_LINE : IDD_NORMAL_LINE;

              /* load dialog resource */
              LoadDialog(hAppInstance, PrefContext->MixerWindow, MAKEINTRESOURCE(dlgId), PrefContext->MixerWindow->DialogCount);

              /* get id */
              wID = (PrefContext->MixerWindow->DialogCount + 1) * IDC_LINE_NAME;

              /* set line name */
              SetDlgItemTextW(PrefContext->MixerWindow->hWnd, wID, Line->szName);

              /* query controls */
              if (SndMixerQueryControls(Mixer, &ControlCount, Line, &Control) != FALSE)
              {
                  /* now go through all controls and update their states */
                  for (Index = 0; Index < Line->cControls; Index++)
                  {
                      if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE)
                      {
                          MIXERCONTROLDETAILS_BOOLEAN Details;

                          /* get volume control details */
                          if (SndMixerGetVolumeControlDetails(Mixer, Control[Index].dwControlID, 1, sizeof(MIXERCONTROLDETAILS_BOOLEAN), (LPVOID)&Details) != -1)
                          {
                              /* update dialog control */
                              wID = (PrefContext->MixerWindow->DialogCount + 1) * IDC_LINE_SWITCH;

                              /* get dialog control */
                              hDlgCtrl = GetDlgItem(PrefContext->MixerWindow->hWnd, wID);

                              if (hDlgCtrl != NULL)
                              {
                                  /* Enable the 'Mute' checkbox, if we are in play mode */
                                  if (Mixer->MixerId == PLAY_MIXER)
                                      EnableWindow(hDlgCtrl, TRUE);

                                  /* check state */
                                  if (SendMessageW(hDlgCtrl, BM_GETCHECK, 0, 0) != Details.fValue)
                                  {
                                      /* update control state */
                                      SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)Details.fValue, 0);
                                  }
                              }
                          }
                      }
                      else if (Control[Index].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME)
                      {
                          /* get volume control details */
                          if (SndMixerGetVolumeControlDetails(Mixer, Control[Index].dwControlID, Line->cChannels, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID)pVolumeDetails) != -1)
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

                              /* Set the volume trackbar */
                              wID = (PrefContext->MixerWindow->DialogCount + 1) * IDC_LINE_SLIDER_VERT;

                              /* get dialog control */
                              hDlgCtrl = GetDlgItem(PrefContext->MixerWindow->hWnd, wID);

                              if (hDlgCtrl != NULL)
                              {
                                  /* check state */
                                  LRESULT OldPosition = SendMessageW(hDlgCtrl, TBM_GETPOS, 0, 0);

                                  /* Enable the volume trackbar */
                                  EnableWindow(hDlgCtrl, TRUE);

                                  if (OldPosition != (VOLUME_MAX - volumePosition))
                                  {
                                      /* update control state */
                                      SendMessageW(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, VOLUME_MAX - volumePosition);
                                  }
                              }

                              if (Line->cChannels == 2)
                              {
                                  /* Set the balance trackbar */
                                  wID = (PrefContext->MixerWindow->DialogCount + 1) * IDC_LINE_SLIDER_HORZ;

                                  /* get dialog control */
                                  hDlgCtrl = GetDlgItem(PrefContext->MixerWindow->hWnd, wID);

                                  if (hDlgCtrl != NULL)
                                  {
                                      /* check state */
                                      LRESULT OldPosition = SendMessageW(hDlgCtrl, TBM_GETPOS, 0, 0);

                                      /* Enable the balance trackbar */
                                      EnableWindow(hDlgCtrl, TRUE);

                                      if (OldPosition != balancePosition)
                                      {
                                          /* update control state */
                                          SendMessageW(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, balancePosition);
                                      }
                                  }
                              }
                          }
                      }
                      else
                      {
                          if (PrefContext->MixerWindow->Mode == NORMAL_MODE)
                          {
                              PrefContext->MixerWindow->bHasExtendedControls = TRUE;

                              wID = (PrefContext->MixerWindow->DialogCount + 1) * IDC_LINE_ADVANCED;

                              /* get dialog control */
                              hDlgCtrl = GetDlgItem(PrefContext->MixerWindow->hWnd, wID);
                              if (hDlgCtrl != NULL)
                              {
                                  ShowWindow(hDlgCtrl,
                                             PrefContext->MixerWindow->bShowExtendedControls ? SW_SHOWNORMAL : SW_HIDE);
                              }
                          }
                      }
                  }

                  /* free controls */
                  HeapFree(GetProcessHeap(), 0, Control);
              }

              /* increment dialog count */
              PrefContext->MixerWindow->DialogCount++;
          }
    }

done:
    /* Free the volume details */
    if (pVolumeDetails)
        HeapFree(GetProcessHeap(), 0, pVolumeDetails);

    return TRUE;
}

/*
 * Returns the width that is available for the mixer window on the monitor
 * the window is currently placed on. The work area of the monitor is used,
 * so that the window is not larger than the usable screen area and the
 * horizontal scrollbar is only shown when it is really needed.
 */
static INT
GetAvailableClientWidth(
    _In_ HWND hWnd)
{
    HMONITOR hMonitor;
    MONITORINFO MonitorInfo;
    RECT WorkArea, FrameRect;
    INT Width;

    /* Fall back to the work area of the primary monitor */
    WorkArea.right = GetSystemMetrics(SM_CXSCREEN);

    hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    if (hMonitor != NULL)
    {
        MonitorInfo.cbSize = sizeof(MonitorInfo);
        if (GetMonitorInfoW(hMonitor, &MonitorInfo))
        {
            WorkArea = MonitorInfo.rcWork;
        }
        else
        {
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &WorkArea, 0);
        }
    }
    else
    {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &WorkArea, 0);
    }

    /* Get the size of the window frame */
    FrameRect.left = 0;
    FrameRect.top = 0;
    FrameRect.right = 0;
    FrameRect.bottom = 0;
    AdjustWindowRect(&FrameRect,
                     WS_DLGFRAME | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
                     FALSE);

    /* Leave some space so that the window frame does not touch the screen edges */
    Width = (WorkArea.right - WorkArea.left) -
            (FrameRect.right - FrameRect.left) - 4;

    return max(Width, MIXER_WINDOW_MIN_WIDTH);
}

/*
 * Returns the size of the window frame, that is the space that is used by the
 * borders, the caption and the menu bar of the window.
 *
 * The size of the menu bar is not known in advance, so it is measured.
 * The height of the horizontal scroll bar is not part of the frame, because it
 * is not part of the client rectangle either.
 */
static VOID
GetWindowFrameSize(
    _In_ HWND hWnd,
    _Out_ INT *FrameWidth,
    _Out_ INT *FrameHeight)
{
    RECT WindowRect, ClientRect;
    POINT Origin;
    INT BorderWidth;

    GetWindowRect(hWnd, &WindowRect);
    GetClientRect(hWnd, &ClientRect);
    Origin.x = 0;
    Origin.y = 0;
    ClientToScreen(hWnd, &Origin);

    /* The space used by the left and the right border */
    *FrameWidth = (WindowRect.right - WindowRect.left) -
                  (ClientRect.right - ClientRect.left);
    BorderWidth = *FrameWidth / 2;

    /*
     * The space used by the top border, the caption and the menu bar, plus
     * the space used by the bottom border.
     */
    *FrameHeight = (Origin.y - WindowRect.top) + BorderWidth;
}

/*
 * Enables or disables the horizontal scrollbar of the mixer window.
 * Returns TRUE if the state of the scrollbar has been changed.
 */
static BOOL
SetHorizontalScrollBarEnabled(
    _In_ PMIXER_WINDOW MixerWindow,
    _In_ BOOL bEnabled)
{
    LONG_PTR Style;

    Style = GetWindowLongPtr(MixerWindow->hWnd, GWL_STYLE);
    if (bEnabled == ((Style & WS_HSCROLL) != 0))
    {
        /* Nothing to do */
        return FALSE;
    }

    if (bEnabled)
        Style |= WS_HSCROLL;
    else
        Style &= ~WS_HSCROLL;

    SetWindowLongPtr(MixerWindow->hWnd, GWL_STYLE, Style);
    SetWindowPos(MixerWindow->hWnd,
                 NULL,
                 0,
                 0,
                 0,
                 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    return TRUE;
}

/*
 * Updates the range and the position of the horizontal scrollbar.
 */
static VOID
UpdateHorizontalScrollBar(
    _In_ PMIXER_WINDOW MixerWindow,
    _In_ INT ClientWidth)
{
    SCROLLINFO ScrollInfo;

    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = max(0, MixerWindow->ContentWidth - 1);
    ScrollInfo.nPage = ClientWidth;
    ScrollInfo.nPos = MixerWindow->ScrollPos;

    SetScrollInfo(MixerWindow->hWnd, SB_HORZ, &ScrollInfo, TRUE);
}

/*
 * Moves the mixer controls by the given number of pixels.
 */
VOID
MoveMixerControls(
    _In_ PMIXER_WINDOW MixerWindow,
    _In_ INT Delta)
{
    UINT i;

    /* Nothing to do */
    if (Delta == 0)
        return;

    for (i = 0; i < MixerWindow->WindowCount; i++)
    {
        HWND hCtrl = MixerWindow->Window[i];
        RECT rc;

        if (!GetWindowRect(hCtrl, &rc))
            continue;

        MapWindowPoints(NULL, MixerWindow->hWnd, (LPPOINT)&rc, 2);
        SetWindowPos(hCtrl,
                     NULL,
                     rc.left + Delta,
                     rc.top,
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

/*
 * Checks if the horizontal scrollbar is still needed and updates its range.
 * The scrollbar is only shown if the mixer controls do not fit into the
 * client area of the window.
 *
 * Returns TRUE if the scrollbar has been enabled or disabled.
 */
BOOL
UpdateMixerWindowScrollBar(
    _In_ PMIXER_WINDOW MixerWindow)
{
    RECT ClientRect;
    INT ClientWidth;
    BOOL bChanged;

    GetClientRect(MixerWindow->hWnd, &ClientRect);
    ClientWidth = ClientRect.right - ClientRect.left;

    /* The scrollbar is only needed if the mixer controls do not fit */
    bChanged = SetHorizontalScrollBarEnabled(MixerWindow,
                                             MixerWindow->ContentWidth > ClientWidth);

    /* Update the range of the scrollbar */
    UpdateHorizontalScrollBar(MixerWindow, ClientWidth);

    return bChanged;
}

/*
 * Adjusts the size of the mixer window to the size of the mixer controls,
 * moves the status bar and updates the horizontal scrollbar.
 */
VOID
UpdateMixerWindowLayout(
    _In_ PPREFERENCES_CONTEXT PrefContext)
{
    PMIXER_WINDOW MixerWindow = PrefContext->MixerWindow;
    RECT StatusRect, ClientRect, ContentRect;
    INT ContentWidth, ContentHeight, ClientWidth, StatusHeight, ScrollHeight;
    INT FrameWidth, FrameHeight, NewPos;
    BOOL bHasScrollBar;

    MixerWindow->bUpdatingLayout = TRUE;

    /* Get the size of the mixer controls */
    ContentWidth = MixerWindow->rect.right - MixerWindow->rect.left;
    ContentHeight = MixerWindow->rect.bottom - MixerWindow->rect.top;

    /* Get the height of the status bar */
    StatusHeight = 0;
    if (MixerWindow->hStatusBar)
    {
        GetWindowRect(MixerWindow->hStatusBar, &StatusRect);
        StatusHeight = StatusRect.bottom - StatusRect.top;
    }

    /* The window is limited to the width of the work area of the monitor */
    ClientWidth = min(ContentWidth, GetAvailableClientWidth(MixerWindow->hWnd));

    /* Store the sizes of the mixer controls */
    MixerWindow->ContentWidth = ContentWidth;
    MixerWindow->ClientWidth = ClientWidth;

    /*
     * The horizontal scrollbar is only needed if the mixer controls do not
     * fit into the client area. It has to be switched on or off before the
     * size of the window is calculated, because it uses space of the client
     * area.
     */
    bHasScrollBar = (ContentWidth > ClientWidth);
    SetHorizontalScrollBarEnabled(MixerWindow, bHasScrollBar);

    /* The horizontal scrollbar uses space below the client area */
    ScrollHeight = bHasScrollBar ? GetSystemMetrics(SM_CYHSCROLL) : 0;

    /*
     * Get the size of the window frame. It is measured instead of being
     * calculated, because the space used by the menu bar is not known in
     * advance.
     */
    GetWindowFrameSize(MixerWindow->hWnd, &FrameWidth, &FrameHeight);

    /* Resize the window */
    SetWindowPos(MixerWindow->hWnd,
                 HWND_TOP,
                 0,
                 0,
                 ClientWidth + FrameWidth,
                 ContentHeight + StatusHeight + ScrollHeight + FrameHeight,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    /* Update the range of the horizontal scrollbar */
    UpdateHorizontalScrollBar(MixerWindow, ClientWidth);

    /*
     * The scroll position might have been changed by SetScrollInfo(), e.g.
     * because the mixer controls fit into the client area now. The mixer
     * controls have to be moved accordingly.
     */
    if (bHasScrollBar)
    {
        NewPos = GetScrollPos(MixerWindow->hWnd, SB_HORZ);
    }
    else
    {
        NewPos = 0;
    }

    if (NewPos != MixerWindow->ScrollPos)
    {
        MoveMixerControls(MixerWindow, MixerWindow->ScrollPos - NewPos);
        MixerWindow->ScrollPos = NewPos;
    }

    /*
     * Move the status bar below the mixer controls. It has to stay above the
     * horizontal scrollbar, otherwise the scrollbar would be hidden by the
     * status bar.
     */
    if (MixerWindow->hStatusBar)
    {
        GetClientRect(MixerWindow->hWnd, &ClientRect);
        SetWindowPos(MixerWindow->hStatusBar,
                     HWND_TOP,
                     0,
                     ContentHeight,
                     ClientRect.right - ClientRect.left,
                     StatusHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    /*
     * Repaint the mixer controls. The controls that have been clipped
     * before, because they did not fit into the client area, have to be
     * painted now.
     */
    ContentRect.left = 0;
    ContentRect.top = 0;
    ContentRect.right = ClientWidth;
    ContentRect.bottom = ContentHeight;
    RedrawWindow(MixerWindow->hWnd,
                 &ContentRect,
                 NULL,
                 RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    MixerWindow->bUpdatingLayout = FALSE;
}

VOID
LoadDialogCtrls(
    PPREFERENCES_CONTEXT PrefContext)
{
    WCHAR szBuffer[64];
    HWND hDlgCtrl;
    RECT statusRect;
    UINT i;
    LONG dy;

    /* set dialog count to zero */
    PrefContext->MixerWindow->DialogCount = 0;
    PrefContext->MixerWindow->bHasExtendedControls = FALSE;
    PrefContext->MixerWindow->ScrollPos = 0;
    SetRectEmpty(&PrefContext->MixerWindow->rect);

    /* enumerate controls */
    SndMixerEnumConnections(PrefContext->MixerWindow->Mixer, PrefContext->SelectedLine, EnumConnectionsCallback, (PVOID)PrefContext);

    /* Update the 'Advanced Controls' menu item */
    EnableMenuItem(GetMenu(PrefContext->MixerWindow->hWnd),
                   IDM_ADVANCED_CONTROLS,
                   MF_BYCOMMAND | (PrefContext->MixerWindow->bHasExtendedControls ? MF_ENABLED : MF_GRAYED));

    /* Add height of the 'Advanced' button */
    dy = MulDiv(ADVANCED_BUTTON_HEIGHT, PrefContext->MixerWindow->baseUnit.cy, 8);
    if (PrefContext->MixerWindow->bShowExtendedControls && PrefContext->MixerWindow->bHasExtendedControls)
        PrefContext->MixerWindow->rect.bottom += dy;

    /* Resize the window and update the horizontal scrollbar */
    UpdateMixerWindowLayout(PrefContext);

    if (PrefContext->MixerWindow->MixerId == RECORD_MIXER)
        LoadStringW(hAppInstance, IDS_SELECT, szBuffer, ARRAYSIZE(szBuffer));

    for (i = 0; i < PrefContext->MixerWindow->DialogCount; i++)
    {
        if (PrefContext->MixerWindow->MixerId == RECORD_MIXER)
        {
            hDlgCtrl = GetDlgItem(PrefContext->MixerWindow->hWnd, (i + 1) * IDC_LINE_SWITCH);

            /* Turn the autocheckbox into a checkbox */
            SetWindowLongPtr(hDlgCtrl, GWL_STYLE, (GetWindowLongPtr(hDlgCtrl, GWL_STYLE) & ~BS_AUTOCHECKBOX) | BS_CHECKBOX);

            /* Change text from 'Mute' to 'Select' */
            SetWindowTextW(hDlgCtrl, szBuffer);
        }

        /* Resize the vertical line separator */
        hDlgCtrl = GetDlgItem(PrefContext->MixerWindow->hWnd, (i + 1) * IDC_LINE_SEP);
        if (hDlgCtrl != NULL)
        {
            GetWindowRect(hDlgCtrl, &statusRect);
            if (PrefContext->MixerWindow->bShowExtendedControls && PrefContext->MixerWindow->bHasExtendedControls)
                statusRect.bottom += dy;

            SetWindowPos(hDlgCtrl,
                         HWND_TOP,
                         0,
                         0,
                         statusRect.right - statusRect.left,
                         statusRect.bottom - statusRect.top,
                         SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    /* Hide the last line separator */
    hDlgCtrl = GetDlgItem(PrefContext->MixerWindow->hWnd, IDC_LINE_SEP * PrefContext->MixerWindow->DialogCount);
    if (hDlgCtrl != NULL)
    {
        ShowWindow(hDlgCtrl, SW_HIDE);
    }
}

HWND
GetLineDialogControl(
    PPREFERENCES_CONTEXT PrefContext,
    LPMIXERLINE Line,
    DWORD dwDialogID)
{
    DWORD Index;

    /* find the index of this line */
    for (Index = 0; Index < PrefContext->MixerWindow->DialogCount; Index++)
    {
        /* get id */
        WCHAR LineName[MIXER_LONG_NAME_CHARS];
        DWORD wID = (Index + 1) * IDC_LINE_NAME;

        if (GetDlgItemText(PrefContext->MixerWindow->hWnd, wID, LineName, MIXER_LONG_NAME_CHARS) == 0)
        {
            /* failed to retrieve id */
            continue;
        }

        /* check if the line name matches */
        if (!_wcsicmp(LineName, Line->szName))
        {
            /* found matching line name */
            wID = (Index + 1) * dwDialogID;

            return GetDlgItem(PrefContext->MixerWindow->hWnd, wID);
        }
    }

    return NULL;
}

VOID
UpdateDialogLineSwitchControl(
    PPREFERENCES_CONTEXT PrefContext,
    LPMIXERLINE Line,
    LONG fValue)
{
    HWND hDlgCtrl = GetLineDialogControl(PrefContext, Line, IDC_LINE_SWITCH);

    if (hDlgCtrl != NULL && SendMessageW(hDlgCtrl, BM_GETCHECK, 0, 0) != fValue)
        SendMessageW(hDlgCtrl, BM_SETCHECK, (WPARAM)fValue, 0);
}

DWORD
GetDialogLineSliderCurrentPosition(
    PPREFERENCES_CONTEXT PrefContext,
    LPMIXERLINE Line,
    DWORD dwDialogID)
{
    HWND hDlgCtrl = GetLineDialogControl(PrefContext, Line, dwDialogID);

    return hDlgCtrl != NULL
        ? SendMessageW(hDlgCtrl, TBM_GETPOS, 0, 0)
        : 0;
}

VOID
UpdateDialogLineSliderControl(
    PPREFERENCES_CONTEXT PrefContext,
    LPMIXERLINE Line,
    DWORD dwDialogID,
    DWORD Position)
{
    HWND hDlgCtrl = GetLineDialogControl(PrefContext, Line, dwDialogID);
    BOOL isPositionChanged = GetDialogLineSliderCurrentPosition(PrefContext, Line, dwDialogID) != Position;

    if (hDlgCtrl != NULL && isPositionChanged)
        SendMessageW(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, Position);
}
