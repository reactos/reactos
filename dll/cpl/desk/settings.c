/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/settings.c
 * PURPOSE:         Settings property page
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "desk.h"

typedef struct _SETTINGS_DATA
{
    PDISPLAY_DEVICE_ENTRY DisplayDeviceList;
    PDISPLAY_DEVICE_ENTRY CurrentDisplayDevice;
    HBITMAP hSpectrumBitmaps[NUM_SPECTRUM_BITMAPS];
    int cxSource[NUM_SPECTRUM_BITMAPS];
    int cySource[NUM_SPECTRUM_BITMAPS];
} SETTINGS_DATA, *PSETTINGS_DATA;

typedef struct _TIMEOUTDATA
{
    TCHAR szRawBuffer[256];
    TCHAR szCookedBuffer[256];
    INT nTimeout;
} TIMEOUTDATA, *PTIMEOUTDATA;

static VOID
UpdateDisplay(IN HWND hwndDlg, PSETTINGS_DATA pData, IN BOOL bUpdateThumb)
{
    TCHAR Buffer[64];
    TCHAR Pixel[64];
    DWORD index;
    HWND hwndMonSel;
    MONSL_MONINFO info;

    LoadString(hApplet, IDS_PIXEL, Pixel, sizeof(Pixel) / sizeof(TCHAR));
    _stprintf(Buffer, Pixel, pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth, pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight, Pixel);
    SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION_TEXT, WM_SETTEXT, 0, (LPARAM)Buffer);

    for (index = 0; index < pData->CurrentDisplayDevice->ResolutionsCount; index++)
    {
        if (pData->CurrentDisplayDevice->Resolutions[index].dmPelsWidth == pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth &&
            pData->CurrentDisplayDevice->Resolutions[index].dmPelsHeight == pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight)
        {
            if (bUpdateThumb)
                SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION, TBM_SETPOS, TRUE, index);
            break;
        }
    }
    if (LoadString(hApplet, (2900 + pData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel), Buffer, sizeof(Buffer) / sizeof(TCHAR)))
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)Buffer);

    hwndMonSel = GetDlgItem(hwndDlg, IDC_SETTINGS_MONSEL);
    index = (INT)SendMessage(hwndMonSel, MSLM_GETCURSEL, 0, 0);
    if (index != (DWORD)-1 && SendMessage(hwndMonSel, MSLM_GETMONITORINFO, index, (LPARAM)&info))
    {
        info.Size.cx = pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth;
        info.Size.cy = pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight;
        SendMessage(hwndMonSel, MSLM_SETMONITORINFO, index, (LPARAM)&info);
    }
}

static int
CompareSettings(PSETTINGS_ENTRY Entry, DWORD dmPelsWidth, DWORD dmPelsHeight,
                DWORD dmBitsPerPel, DWORD dmDisplayFrequency)
{
    if (Entry->dmPelsWidth  == dmPelsWidth  &&
        Entry->dmPelsHeight == dmPelsHeight &&
        Entry->dmBitsPerPel == dmBitsPerPel &&
        Entry->dmDisplayFrequency == dmDisplayFrequency)
    {
        return 0;
    }
    else
    if ((Entry->dmPelsWidth  < dmPelsWidth) ||
        (Entry->dmPelsWidth == dmPelsWidth && Entry->dmPelsHeight < dmPelsHeight) ||
        (Entry->dmPelsWidth == dmPelsWidth && Entry->dmPelsHeight == dmPelsHeight &&
         Entry->dmBitsPerPel < dmBitsPerPel))
    {
        return 1;
    }
    return -1;
}

static PSETTINGS_ENTRY
GetPossibleSettings(IN LPCTSTR DeviceName, OUT DWORD* pSettingsCount, OUT PSETTINGS_ENTRY* CurrentSettings)
{
    DEVMODE devmode;
    DWORD NbSettings = 0;
    DWORD iMode = 0;
    DWORD dwFlags = 0;
    PSETTINGS_ENTRY Settings = NULL;
    HDC hDC;
    PSETTINGS_ENTRY Current;
    DWORD bpp, xres, yres;
    DWORD curDispFreq;

    /* Get current settings */
    *CurrentSettings = NULL;
    hDC = CreateIC(NULL, DeviceName, NULL, NULL);
    bpp = GetDeviceCaps(hDC, PLANES);
    bpp *= GetDeviceCaps(hDC, BITSPIXEL);
    xres = GetDeviceCaps(hDC, HORZRES);
    yres = GetDeviceCaps(hDC, VERTRES);
    DeleteDC(hDC);

    /* List all settings */
    devmode.dmSize = (WORD)sizeof(devmode);
    devmode.dmDriverExtra = 0;

    if (!EnumDisplaySettingsEx(DeviceName, ENUM_CURRENT_SETTINGS, &devmode, dwFlags))
        return NULL;

    curDispFreq = devmode.dmDisplayFrequency;

    while (EnumDisplaySettingsEx(DeviceName, iMode, &devmode, dwFlags))
    {
        iMode++;

        if (devmode.dmPelsWidth < 640 ||
            devmode.dmPelsHeight < 480 ||
            devmode.dmDisplayFrequency != curDispFreq ||
            (devmode.dmBitsPerPel != 4 &&
             devmode.dmBitsPerPel != 8 &&
             devmode.dmBitsPerPel != 16 &&
             devmode.dmBitsPerPel != 24 &&
             devmode.dmBitsPerPel != 32))
        {
            continue;
        }

        Current = HeapAlloc(GetProcessHeap(), 0, sizeof(SETTINGS_ENTRY));
        if (Current != NULL)
        {
            /* Sort resolutions by increasing height, and BPP */
            PSETTINGS_ENTRY Previous = NULL;
            PSETTINGS_ENTRY Next = Settings;
            Current->dmPelsWidth = devmode.dmPelsWidth;
            Current->dmPelsHeight = devmode.dmPelsHeight;
            Current->dmBitsPerPel = devmode.dmBitsPerPel;
            Current->dmDisplayFrequency = devmode.dmDisplayFrequency;
            while (Next != NULL &&
                   CompareSettings(Next, devmode.dmPelsWidth,
                                   devmode.dmPelsHeight, devmode.dmBitsPerPel,
                                   devmode.dmDisplayFrequency) > 0)
            {
                Previous = Next;
                Next = Next->Flink;
            }
            Current->Blink = Previous;
            Current->Flink = Next;
            if (Previous == NULL)
                Settings = Current;
            else
                Previous->Flink = Current;
            if (Next != NULL)
                Next->Blink = Current;
            if (devmode.dmPelsWidth == xres && devmode.dmPelsHeight == yres && devmode.dmBitsPerPel == bpp)
            {
                *CurrentSettings = Current;
            }
            NbSettings++;
        }
    }

    *pSettingsCount = NbSettings;
    return Settings;
}

static BOOL
AddDisplayDevice(IN PSETTINGS_DATA pData, IN const DISPLAY_DEVICE *DisplayDevice)
{
    PDISPLAY_DEVICE_ENTRY newEntry = NULL;
    LPTSTR description = NULL;
    LPTSTR name = NULL;
    LPTSTR key = NULL;
    LPTSTR devid = NULL;
    SIZE_T descriptionSize, nameSize, keySize, devidSize;
    PSETTINGS_ENTRY Current;
    DWORD ResolutionsCount = 1;
    DWORD i;

    newEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DISPLAY_DEVICE_ENTRY));
    if (!newEntry) goto ByeBye;

    newEntry->Settings = GetPossibleSettings(DisplayDevice->DeviceName, &newEntry->SettingsCount, &newEntry->CurrentSettings);
    if (!newEntry->Settings) goto ByeBye;

    newEntry->InitialSettings.dmPelsWidth = newEntry->CurrentSettings->dmPelsWidth;
    newEntry->InitialSettings.dmPelsHeight = newEntry->CurrentSettings->dmPelsHeight;
    newEntry->InitialSettings.dmBitsPerPel = newEntry->CurrentSettings->dmBitsPerPel;
    newEntry->InitialSettings.dmDisplayFrequency = newEntry->CurrentSettings->dmDisplayFrequency;

    /* Count different resolutions */
    for (Current = newEntry->Settings; Current != NULL; Current = Current->Flink)
    {
        if (Current->Flink != NULL &&
            ((Current->dmPelsWidth != Current->Flink->dmPelsWidth) ||
             (Current->dmPelsHeight != Current->Flink->dmPelsHeight)))
        {
            ResolutionsCount++;
        }
    }

    newEntry->Resolutions = HeapAlloc(GetProcessHeap(), 0, ResolutionsCount * sizeof(RESOLUTION_INFO));
    if (!newEntry->Resolutions) goto ByeBye;

    newEntry->ResolutionsCount = ResolutionsCount;

    /* Fill resolutions infos */
    for (Current = newEntry->Settings, i = 0; Current != NULL; Current = Current->Flink)
    {
        if (Current->Flink == NULL ||
            (Current->Flink != NULL &&
            ((Current->dmPelsWidth != Current->Flink->dmPelsWidth) ||
             (Current->dmPelsHeight != Current->Flink->dmPelsHeight))))
        {
            newEntry->Resolutions[i].dmPelsWidth = Current->dmPelsWidth;
            newEntry->Resolutions[i].dmPelsHeight = Current->dmPelsHeight;
            i++;
        }
    }
    descriptionSize = (_tcslen(DisplayDevice->DeviceString) + 1) * sizeof(TCHAR);
    description = HeapAlloc(GetProcessHeap(), 0, descriptionSize);
    if (!description) goto ByeBye;

    nameSize = (_tcslen(DisplayDevice->DeviceName) + 1) * sizeof(TCHAR);
    name = HeapAlloc(GetProcessHeap(), 0, nameSize);
    if (!name) goto ByeBye;

    keySize = (_tcslen(DisplayDevice->DeviceKey) + 1) * sizeof(TCHAR);
    key = HeapAlloc(GetProcessHeap(), 0, keySize);
    if (!key) goto ByeBye;

    devidSize = (_tcslen(DisplayDevice->DeviceID) + 1) * sizeof(TCHAR);
    devid = HeapAlloc(GetProcessHeap(), 0, devidSize);
    if (!devid) goto ByeBye;

    memcpy(description, DisplayDevice->DeviceString, descriptionSize);
    memcpy(name, DisplayDevice->DeviceName, nameSize);
    memcpy(key, DisplayDevice->DeviceKey, keySize);
    memcpy(devid, DisplayDevice->DeviceID, devidSize);
    newEntry->DeviceDescription = description;
    newEntry->DeviceName = name;
    newEntry->DeviceKey = key;
    newEntry->DeviceID = devid;
    newEntry->DeviceStateFlags = DisplayDevice->StateFlags;
    newEntry->Flink = pData->DisplayDeviceList;
    pData->DisplayDeviceList = newEntry;
    return TRUE;

ByeBye:
    if (newEntry != NULL)
    {
        if (newEntry->Settings != NULL)
        {
            Current = newEntry->Settings;
            while (Current != NULL)
            {
                PSETTINGS_ENTRY Next = Current->Flink;
                HeapFree(GetProcessHeap(), 0, Current);
                Current = Next;
            }
        }
        if (newEntry->Resolutions != NULL)
            HeapFree(GetProcessHeap(), 0, newEntry->Resolutions);
        HeapFree(GetProcessHeap(), 0, newEntry);
    }
    if (description != NULL)
        HeapFree(GetProcessHeap(), 0, description);
    if (name != NULL)
        HeapFree(GetProcessHeap(), 0, name);
    if (key != NULL)
        HeapFree(GetProcessHeap(), 0, key);
    return FALSE;
}

static VOID
OnDisplayDeviceChanged(IN HWND hwndDlg, IN PSETTINGS_DATA pData, IN PDISPLAY_DEVICE_ENTRY pDeviceEntry)
{
    PSETTINGS_ENTRY Current;
    DWORD index;

    pData->CurrentDisplayDevice = pDeviceEntry; /* Update variable */

    /* Fill color depths combo box */
    SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_RESETCONTENT, 0, 0);
    for (Current = pDeviceEntry->Settings; Current != NULL; Current = Current->Flink)
    {
        TCHAR Buffer[64];
        if (LoadString(hApplet, (2900 + Current->dmBitsPerPel), Buffer, sizeof(Buffer) / sizeof(TCHAR)))
        {
            index = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)Buffer);
            if (index == (DWORD)CB_ERR)
            {
                index = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_ADDSTRING, 0, (LPARAM)Buffer);
                SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_SETITEMDATA, index, Current->dmBitsPerPel);
            }
        }
    }

    /* Fill device description */
    SendDlgItemMessage(hwndDlg, IDC_SETTINGS_DEVICE, WM_SETTEXT, 0, (LPARAM)pDeviceEntry->DeviceDescription);

    /* Fill resolutions slider */
    SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION, TBM_CLEARTICS, TRUE, 0);
    SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION, TBM_SETRANGE, TRUE, MAKELONG(0, pDeviceEntry->ResolutionsCount - 1));

    UpdateDisplay(hwndDlg, pData, TRUE);
}

static VOID
SettingsOnInitDialog(IN HWND hwndDlg)
{
    BITMAP bitmap;
    DWORD Result = 0;
    DWORD iDevNum = 0;
    DWORD i;
    DISPLAY_DEVICE displayDevice;
    PSETTINGS_DATA pData;

    pData = HeapAlloc(GetProcessHeap(), 0, sizeof(SETTINGS_DATA));
    if (pData == NULL)
        return;

    SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pData);

    /* Get video cards list */
    pData->DisplayDeviceList = NULL;
    displayDevice.cb = sizeof(displayDevice);
    while (EnumDisplayDevices(NULL, iDevNum, &displayDevice, 0x1))
    {
        if ((displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0)
        {
            if (AddDisplayDevice(pData, &displayDevice))
                Result++;
        }
        iDevNum++;
    }

    if (Result == 0)
    {
        /* No adapter found */
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_BPP), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_RESOLUTION), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_RESOLUTION_TEXT), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_ADVANCED), FALSE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SPECTRUM), SW_HIDE);

        /* Do not initialize the color spectrum bitmaps */
        memset(pData->hSpectrumBitmaps, 0, sizeof(pData->hSpectrumBitmaps));
        return;
    }
    else if (Result == 1)
    {
        MONSL_MONINFO monitors;

        /* Single video adapter */
        OnDisplayDeviceChanged(hwndDlg, pData, pData->DisplayDeviceList);

        monitors.Position.x = monitors.Position.y = 0;
        monitors.Size.cx = pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth;
        monitors.Size.cy = pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight;
        monitors.Flags = 0;
        SendDlgItemMessage(hwndDlg,
                   IDC_SETTINGS_MONSEL,
                   MSLM_SETMONITORSINFO,
                   1,
                   (LPARAM)&monitors);
    }
    else /* FIXME: Incomplete! */
    {
        PMONSL_MONINFO pMonitors;
        DWORD i;

        OnDisplayDeviceChanged(hwndDlg, pData, pData->DisplayDeviceList);

        pMonitors = (PMONSL_MONINFO)HeapAlloc(GetProcessHeap(), 0, sizeof(MONSL_MONINFO) * Result);
        if (pMonitors)
        {
            DWORD hack = 1280;
            for (i = 0; i < Result; i++)
            {
                pMonitors[i].Position.x = hack * i;
                pMonitors[i].Position.y = 0;
                pMonitors[i].Size.cx = pData->DisplayDeviceList->CurrentSettings->dmPelsWidth;
                pMonitors[i].Size.cy = pData->DisplayDeviceList->CurrentSettings->dmPelsHeight;
                pMonitors[i].Flags = 0;
            }

            SendDlgItemMessage(hwndDlg,
                       IDC_SETTINGS_MONSEL,
                       MSLM_SETMONITORSINFO,
                       Result,
                       (LPARAM)pMonitors);

            HeapFree(GetProcessHeap(), 0, pMonitors);
        }
    }

    /* Initialize the color spectrum bitmaps */
    for (i = 0; i < NUM_SPECTRUM_BITMAPS; i++)
    {
        pData->hSpectrumBitmaps[i] = LoadImageW(hApplet, MAKEINTRESOURCEW(IDB_SPECTRUM_4 + i), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

        if (pData->hSpectrumBitmaps[i] != NULL)
        {
            if (GetObjectW(pData->hSpectrumBitmaps[i], sizeof(BITMAP), &bitmap) != 0)
            {
                pData->cxSource[i] = bitmap.bmWidth;
                pData->cySource[i] = bitmap.bmHeight;
            }
            else
            {
                pData->cxSource[i] = 0;
                pData->cySource[i] = 0;
            }
        }
    }
}

/* Get the ID for SETTINGS_DATA::hSpectrumBitmaps */
static VOID
ShowColorSpectrum(IN HDC hDC, IN LPRECT client, IN DWORD BitsPerPel, IN PSETTINGS_DATA pData)
{
    HDC hdcMem;
    INT iBitmap;

    hdcMem = CreateCompatibleDC(hDC);

    if (!hdcMem)
        return;

    switch(BitsPerPel)
    {
        case 4:  iBitmap = 0; break;
        case 8:  iBitmap = 1; break;
        default: iBitmap = 2;
    }

    if (SelectObject(hdcMem, pData->hSpectrumBitmaps[iBitmap]))
    {
        StretchBlt(hDC,
               client->left, client->top,
               client->right - client->left,
               client->bottom - client->top,
               hdcMem, 0, 0,
               pData->cxSource[iBitmap],
               pData->cySource[iBitmap], SRCCOPY);
    }

    DeleteDC(hdcMem);
}

static VOID
OnBPPChanged(IN HWND hwndDlg, IN PSETTINGS_DATA pData)
{
    /* If new BPP is not compatible with resolution:
     * 1) try to find the nearest smaller matching resolution
     * 2) otherwise, get the nearest bigger resolution
     */
    PSETTINGS_ENTRY Current;
    DWORD dmNewBitsPerPel;
    DWORD index;
    HDC hSpectrumDC;
    HWND hSpectrumControl;
    RECT client;

    index = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_GETCURSEL, 0, 0);
    dmNewBitsPerPel = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_BPP, CB_GETITEMDATA, index, 0);

    /* Show a new spectrum bitmap */
    hSpectrumControl = GetDlgItem(hwndDlg, IDC_SETTINGS_SPECTRUM);
    hSpectrumDC = GetDC(hSpectrumControl);
    if (hSpectrumDC == NULL)
        return;

    GetClientRect(hSpectrumControl, &client);
    ShowColorSpectrum(hSpectrumDC, &client, dmNewBitsPerPel, pData);
    ReleaseDC(hSpectrumControl, hSpectrumDC);

    /* Find if new parameters are valid */
    Current = pData->CurrentDisplayDevice->CurrentSettings;
    if (dmNewBitsPerPel == Current->dmBitsPerPel)
    {
        /* No change */
        return;
    }

    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

    if (dmNewBitsPerPel < Current->dmBitsPerPel)
    {
        Current = Current->Blink;
        while (Current != NULL)
        {
            if (Current->dmBitsPerPel == dmNewBitsPerPel
             && Current->dmPelsHeight == pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight
             && Current->dmPelsWidth == pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth)
            {
                pData->CurrentDisplayDevice->CurrentSettings = Current;
                UpdateDisplay(hwndDlg, pData, TRUE);
                return;
            }
            Current = Current->Blink;
        }
    }
    else
    {
        Current = Current->Flink;
        while (Current != NULL)
        {
            if (Current->dmBitsPerPel == dmNewBitsPerPel
             && Current->dmPelsHeight == pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight
             && Current->dmPelsWidth == pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth)
            {
                pData->CurrentDisplayDevice->CurrentSettings = Current;
                UpdateDisplay(hwndDlg, pData, TRUE);
                return;
            }
            Current = Current->Flink;
        }
    }

    /* Search smaller resolution compatible with current color depth */
    Current = pData->CurrentDisplayDevice->CurrentSettings->Blink;
    while (Current != NULL)
    {
        if (Current->dmBitsPerPel == dmNewBitsPerPel)
        {
            pData->CurrentDisplayDevice->CurrentSettings = Current;
            UpdateDisplay(hwndDlg, pData, TRUE);
            return;
        }
        Current = Current->Blink;
    }

    /* Search bigger resolution compatible with current color depth */
    Current = pData->CurrentDisplayDevice->CurrentSettings->Flink;
    while (Current != NULL)
    {
        if (Current->dmBitsPerPel == dmNewBitsPerPel)
        {
            pData->CurrentDisplayDevice->CurrentSettings = Current;
            UpdateDisplay(hwndDlg, pData, TRUE);
            return;
        }
        Current = Current->Flink;
    }

    /* We shouldn't go there */
}

static VOID
OnResolutionChanged(IN HWND hwndDlg, IN PSETTINGS_DATA pData, IN DWORD NewPosition,
                    IN BOOL bUpdateThumb)
{
    /* If new resolution is not compatible with color depth:
     * 1) try to find the nearest bigger matching color depth
     * 2) otherwise, get the nearest smaller color depth
     */
    PSETTINGS_ENTRY Current;
    DWORD dmNewPelsHeight = pData->CurrentDisplayDevice->Resolutions[NewPosition].dmPelsHeight;
    DWORD dmNewPelsWidth = pData->CurrentDisplayDevice->Resolutions[NewPosition].dmPelsWidth;
    DWORD dmBitsPerPel;
    DWORD dmDisplayFrequency;

    /* Find if new parameters are valid */
    Current = pData->CurrentDisplayDevice->CurrentSettings;
    if (dmNewPelsHeight == Current->dmPelsHeight && dmNewPelsWidth == Current->dmPelsWidth)
    {
        /* No change */
        return;
    }

    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

    dmBitsPerPel = Current->dmBitsPerPel;
    dmDisplayFrequency = Current->dmDisplayFrequency;

    if (CompareSettings(Current, dmNewPelsWidth,
                        dmNewPelsHeight, dmBitsPerPel,
                        dmDisplayFrequency) < 0)
    {
        Current = Current->Blink;
        while (Current != NULL)
        {
            if (Current->dmPelsHeight == dmNewPelsHeight
             && Current->dmPelsWidth  == dmNewPelsWidth
             && Current->dmBitsPerPel == dmBitsPerPel)
            {
                pData->CurrentDisplayDevice->CurrentSettings = Current;
                UpdateDisplay(hwndDlg, pData, bUpdateThumb);
                return;
            }
            Current = Current->Blink;
        }
    }
    else
    {
        Current = Current->Flink;
        while (Current != NULL)
        {
            if (Current->dmPelsHeight == dmNewPelsHeight
             && Current->dmPelsWidth  == dmNewPelsWidth
             && Current->dmBitsPerPel == dmBitsPerPel)
            {
                pData->CurrentDisplayDevice->CurrentSettings = Current;
                UpdateDisplay(hwndDlg, pData, bUpdateThumb);
                return;
            }
            Current = Current->Flink;
        }
    }

    /* Search bigger color depth compatible with current resolution */
    Current = pData->CurrentDisplayDevice->CurrentSettings->Flink;
    while (Current != NULL)
    {
        if (dmNewPelsHeight == Current->dmPelsHeight && dmNewPelsWidth == Current->dmPelsWidth)
        {
            pData->CurrentDisplayDevice->CurrentSettings = Current;
            UpdateDisplay(hwndDlg, pData, bUpdateThumb);
            return;
        }
        Current = Current->Flink;
    }

    /* Search smaller color depth compatible with current resolution */
    Current = pData->CurrentDisplayDevice->CurrentSettings->Blink;
    while (Current != NULL)
    {
        if (dmNewPelsHeight == Current->dmPelsHeight && dmNewPelsWidth == Current->dmPelsWidth)
        {
            pData->CurrentDisplayDevice->CurrentSettings = Current;
            UpdateDisplay(hwndDlg, pData, bUpdateThumb);
            return;
        }
        Current = Current->Blink;
    }

    /* We shouldn't go there */
}

/* Property sheet page callback */
UINT CALLBACK
SettingsPageCallbackProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    UINT Ret = 0;

    switch (uMsg)
    {
        case PSPCB_CREATE:
            Ret = RegisterMonitorSelectionControl(hApplet);
            break;

        case PSPCB_RELEASE:
            UnregisterMonitorSelectionControl(hApplet);
            break;
    }

    return Ret;
}

static INT_PTR CALLBACK
ConfirmDlgProc(IN HWND hwndDlg, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam)
{
    PTIMEOUTDATA pData;

    pData = (PTIMEOUTDATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            /* Allocate the local dialog data */
            pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TIMEOUTDATA));
            if (pData == NULL)
                return FALSE;

            /* Link the dialog data to the dialog */
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pData);

            /* Timeout in seconds */
            pData->nTimeout = 15;

            /* Load the raw timeout string */
            LoadString(hApplet, IDS_TIMEOUTTEXT, pData->szRawBuffer, ARRAYSIZE(pData->szRawBuffer));

            /* Cook the timeout string and show it */
            _stprintf(pData->szCookedBuffer, pData->szRawBuffer, pData->nTimeout);
            SetDlgItemText(hwndDlg, IDC_TIMEOUTTEXT, pData->szCookedBuffer);

            /* Start the timer (ticks every second)*/
            SetTimer(hwndDlg, 1, 1000, NULL);
            break;

        case WM_TIMER:
            /* Update the timepout value */
            pData->nTimeout--;

            /* Update the timeout text */
            _stprintf(pData->szCookedBuffer, pData->szRawBuffer, pData->nTimeout);
            SetDlgItemText(hwndDlg, IDC_TIMEOUTTEXT, pData->szCookedBuffer);

            /* Kill the timer and return a 'No', if we ran out of time */
            if (pData->nTimeout == 0)
            {
                KillTimer(hwndDlg, 1);
                EndDialog(hwndDlg, IDNO);
            }
            break;

        case WM_COMMAND:
            /* Kill the timer and return the clicked button id */
            KillTimer(hwndDlg, 1);
            EndDialog(hwndDlg, LOWORD(wParam));
            break;

        case WM_DESTROY:
            /* Free the local dialog data */
            HeapFree(GetProcessHeap(), 0, pData);
            break;
    }

    return FALSE;
}

static VOID
ApplyDisplaySettings(HWND hwndDlg, PSETTINGS_DATA pData)
{
    TCHAR Message[1024], Title[256];
    DEVMODE devmode;
    LONG rc;

    RtlZeroMemory(&devmode, sizeof(devmode));
    devmode.dmSize = (WORD)sizeof(devmode);
    devmode.dmPelsWidth = pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth;
    devmode.dmPelsHeight = pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight;
    devmode.dmBitsPerPel = pData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel;
    devmode.dmDisplayFrequency = pData->CurrentDisplayDevice->CurrentSettings->dmDisplayFrequency;
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

    rc = ChangeDisplaySettingsEx(pData->CurrentDisplayDevice->DeviceName,
                                 &devmode,
                                 NULL,
                                 CDS_UPDATEREGISTRY,
                                 NULL);
    switch (rc)
    {
        case DISP_CHANGE_SUCCESSFUL:
            break;

        case DISP_CHANGE_RESTART:
            LoadString(hApplet, IDS_DISPLAY_SETTINGS, Title, sizeof(Title) / sizeof(TCHAR));
            LoadString(hApplet, IDS_APPLY_NEEDS_RESTART, Message, sizeof(Message) / sizeof (TCHAR));
            MessageBox(hwndDlg, Message, Title, MB_OK | MB_ICONINFORMATION);
            return;

        case DISP_CHANGE_FAILED:
        default:
            LoadString(hApplet, IDS_DISPLAY_SETTINGS, Title, sizeof(Title) / sizeof(TCHAR));
            LoadString(hApplet, IDS_APPLY_FAILED, Message, sizeof(Message) / sizeof (TCHAR));
            MessageBox(hwndDlg, Message, Title, MB_OK | MB_ICONSTOP);
            return;
    }

    if (DialogBox(hApplet, MAKEINTRESOURCE(IDD_CONFIRMSETTINGS), hwndDlg, ConfirmDlgProc) == IDYES)
    {
        pData->CurrentDisplayDevice->InitialSettings.dmPelsWidth = pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth;
        pData->CurrentDisplayDevice->InitialSettings.dmPelsHeight = pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight;
        pData->CurrentDisplayDevice->InitialSettings.dmBitsPerPel = pData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel;
        pData->CurrentDisplayDevice->InitialSettings.dmDisplayFrequency = pData->CurrentDisplayDevice->CurrentSettings->dmDisplayFrequency;
    }
    else
    {
        devmode.dmPelsWidth = pData->CurrentDisplayDevice->InitialSettings.dmPelsWidth;
        devmode.dmPelsHeight = pData->CurrentDisplayDevice->InitialSettings.dmPelsHeight;
        devmode.dmBitsPerPel = pData->CurrentDisplayDevice->InitialSettings.dmBitsPerPel;
        devmode.dmDisplayFrequency = pData->CurrentDisplayDevice->InitialSettings.dmDisplayFrequency;

        rc = ChangeDisplaySettingsEx(pData->CurrentDisplayDevice->DeviceName,
                                     &devmode,
                                     NULL,
                                     CDS_UPDATEREGISTRY,
                                     NULL);
        switch (rc)
        {
            case DISP_CHANGE_SUCCESSFUL:
                pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth = pData->CurrentDisplayDevice->InitialSettings.dmPelsWidth;
                pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight = pData->CurrentDisplayDevice->InitialSettings.dmPelsHeight;
                pData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel = pData->CurrentDisplayDevice->InitialSettings.dmBitsPerPel;
                pData->CurrentDisplayDevice->CurrentSettings->dmDisplayFrequency = pData->CurrentDisplayDevice->InitialSettings.dmDisplayFrequency;
                UpdateDisplay(hwndDlg, pData, TRUE);
                break;

            case DISP_CHANGE_RESTART:
                LoadString(hApplet, IDS_DISPLAY_SETTINGS, Title, sizeof(Title) / sizeof(TCHAR));
                LoadString(hApplet, IDS_APPLY_NEEDS_RESTART, Message, sizeof(Message) / sizeof (TCHAR));
                MessageBox(hwndDlg, Message, Title, MB_OK | MB_ICONINFORMATION);
                return;

            case DISP_CHANGE_FAILED:
            default:
                LoadString(hApplet, IDS_DISPLAY_SETTINGS, Title, sizeof(Title) / sizeof(TCHAR));
                LoadString(hApplet, IDS_APPLY_FAILED, Message, sizeof(Message) / sizeof (TCHAR));
                MessageBox(hwndDlg, Message, Title, MB_OK | MB_ICONSTOP);
                return;
        }
    }
}

/* Property page dialog callback */
INT_PTR CALLBACK
SettingsPageProc(IN HWND hwndDlg, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam)
{
    PSETTINGS_DATA pData;

    pData = (PSETTINGS_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            SettingsOnInitDialog(hwndDlg);
            break;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;

            if (lpDrawItem->CtlID == IDC_SETTINGS_SPECTRUM)
                ShowColorSpectrum(lpDrawItem->hDC, &lpDrawItem->rcItem, pData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel, pData);
            break;
        }
        case WM_COMMAND:
        {
            DWORD controlId = LOWORD(wParam);
            DWORD command   = HIWORD(wParam);

            if (controlId == IDC_SETTINGS_ADVANCED && command == BN_CLICKED)
                DisplayAdvancedSettings(hwndDlg, pData->CurrentDisplayDevice);
            else if (controlId == IDC_SETTINGS_BPP && command == CBN_SELCHANGE)
                OnBPPChanged(hwndDlg, pData);
            break;
        }
        case WM_HSCROLL:
        {
            switch (LOWORD(wParam))
            {
                case TB_LINEUP:
                case TB_LINEDOWN:
                case TB_PAGEUP:
                case TB_PAGEDOWN:
                case TB_TOP:
                case TB_BOTTOM:
                case TB_ENDTRACK:
                {
                    DWORD newPosition = (DWORD) SendDlgItemMessage(hwndDlg, IDC_SETTINGS_RESOLUTION, TBM_GETPOS, 0, 0);
                    OnResolutionChanged(hwndDlg, pData, newPosition, TRUE);
                    break;
                }

                case TB_THUMBTRACK:
                    OnResolutionChanged(hwndDlg, pData, HIWORD(wParam), FALSE);
                    break;
            }
            break;
        }
        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            if (lpnm->code == (UINT)PSN_APPLY)
            {
                if (pData->CurrentDisplayDevice->CurrentSettings->dmPelsWidth != pData->CurrentDisplayDevice->InitialSettings.dmPelsWidth
                 || pData->CurrentDisplayDevice->CurrentSettings->dmPelsHeight != pData->CurrentDisplayDevice->InitialSettings.dmPelsHeight
                 || pData->CurrentDisplayDevice->CurrentSettings->dmBitsPerPel != pData->CurrentDisplayDevice->InitialSettings.dmBitsPerPel)
                {
                    /* Apply new settings */
                    ApplyDisplaySettings(hwndDlg, pData);
                }
            }
            else if (lpnm->code == MSLN_MONITORCHANGED)
            {
                PMONSL_MONNMMONITORCHANGING lpnmi = (PMONSL_MONNMMONITORCHANGING)lParam;
                PDISPLAY_DEVICE_ENTRY Current = pData->DisplayDeviceList;
                ULONG i;
                for (i = 0; i < lpnmi->hdr.Index; i++)
                    Current = Current->Flink;
                OnDisplayDeviceChanged(hwndDlg, pData, Current);
            }
            break;
        }

        case WM_CONTEXTMENU:
        {
            HWND hwndMonSel;
            HMENU hPopup;
            UINT uiCmd;
            POINT pt, ptClient;
            INT Index;

            pt.x = (SHORT)LOWORD(lParam);
            pt.y = (SHORT)HIWORD(lParam);

            hwndMonSel = GetDlgItem(hwndDlg,
                                    IDC_SETTINGS_MONSEL);
            if ((HWND)wParam == hwndMonSel)
            {
                if (pt.x == -1 && pt.y == -1)
                {
                    RECT rcMon;

                    Index = (INT)SendMessage(hwndMonSel,
                                             MSLM_GETCURSEL,
                                             0,
                                             0);

                    if (Index >= 0 &&
                        (INT)SendMessage(hwndMonSel,
                                         MSLM_GETMONITORRECT,
                                         Index,
                                         (LPARAM)&rcMon) > 0)
                    {
                        pt.x = rcMon.left + ((rcMon.right - rcMon.left) / 2);
                        pt.y = rcMon.top + ((rcMon.bottom - rcMon.top) / 2);
                    }
                    else
                        pt.x = pt.y = 0;

                    MapWindowPoints(hwndMonSel,
                                    NULL,
                                    &pt,
                                    1);
                }
                else
                {
                    ptClient = pt;
                    MapWindowPoints(NULL,
                                    hwndMonSel,
                                    &ptClient,
                                    1);

                    Index = (INT)SendMessage(hwndMonSel,
                                             MSLM_HITTEST,
                                             (WPARAM)&ptClient,
                                             0);
                }

                if (Index >= 0)
                {
                    hPopup = LoadPopupMenu(hApplet,
                                           MAKEINTRESOURCE(IDM_MONITOR_MENU));
                    if (hPopup != NULL)
                    {
                        /* FIXME: Enable/Disable menu items */
                        EnableMenuItem(hPopup,
                                       ID_MENU_ATTACHED,
                                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                        EnableMenuItem(hPopup,
                                       ID_MENU_PRIMARY,
                                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                        EnableMenuItem(hPopup,
                                       ID_MENU_IDENTIFY,
                                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                        EnableMenuItem(hPopup,
                                       ID_MENU_PROPERTIES,
                                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

                        uiCmd = (UINT)TrackPopupMenu(hPopup,
                                                     TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                                     pt.x,
                                                     pt.y,
                                                     0,
                                                     hwndDlg,
                                                     NULL);

                        switch (uiCmd)
                        {
                            case ID_MENU_ATTACHED:
                            case ID_MENU_PRIMARY:
                            case ID_MENU_IDENTIFY:
                            case ID_MENU_PROPERTIES:
                                /* FIXME: Implement */
                                break;
                        }

                        DestroyMenu(hPopup);
                    }
                }
            }
            break;
        }

        case WM_DESTROY:
        {
            DWORD i;
            PDISPLAY_DEVICE_ENTRY Current = pData->DisplayDeviceList;

            while (Current != NULL)
            {
                PDISPLAY_DEVICE_ENTRY Next = Current->Flink;
                PSETTINGS_ENTRY CurrentSettings = Current->Settings;
                while (CurrentSettings != NULL)
                {
                    PSETTINGS_ENTRY NextSettings = CurrentSettings->Flink;
                    HeapFree(GetProcessHeap(), 0, CurrentSettings);
                    CurrentSettings = NextSettings;
                }
                HeapFree(GetProcessHeap(), 0, Current);
                Current = Next;
            }

            for (i = 0; i < NUM_SPECTRUM_BITMAPS; i++)
            {
                if (pData->hSpectrumBitmaps[i])
                    DeleteObject(pData->hSpectrumBitmaps[i]);
            }

            HeapFree(GetProcessHeap(), 0, pData);
        }
    }
    return FALSE;
}
