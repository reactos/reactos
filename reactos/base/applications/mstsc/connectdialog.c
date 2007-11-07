/*
   rdesktop: A Remote Desktop Protocol client.
   Connection settings dialog
   Copyright (C) Ged Murphy 2007

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <todo.h>
#include "resource.h"


#define MAX_KEY_NAME 255

/* As slider control can't contain user data, we have to keep an
 * array of RESOLUTION_INFO to have our own associated data.
 */
typedef struct _RESOLUTION_INFO
{
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
} RESOLUTION_INFO, *PRESOLUTION_INFO;

typedef struct _SETTINGS_ENTRY
{
    struct _SETTINGS_ENTRY *Blink;
    struct _SETTINGS_ENTRY *Flink;
    DWORD dmBitsPerPel;
    DWORD dmPelsWidth;
    DWORD dmPelsHeight;
} SETTINGS_ENTRY, *PSETTINGS_ENTRY;

typedef struct _DISPLAY_DEVICE_ENTRY
{
    struct _DISPLAY_DEVICE_ENTRY *Flink;
    LPTSTR DeviceDescription;
    LPTSTR DeviceName;
    LPTSTR DeviceKey;
    LPTSTR DeviceID;
    DWORD DeviceStateFlags;
    PSETTINGS_ENTRY Settings; /* sorted by increasing dmPelsHeight, BPP */
    DWORD SettingsCount;
    PRESOLUTION_INFO Resolutions;
    DWORD ResolutionsCount;
    PSETTINGS_ENTRY CurrentSettings; /* Points into Settings list */
    SETTINGS_ENTRY InitialSettings;
} DISPLAY_DEVICE_ENTRY, *PDISPLAY_DEVICE_ENTRY;

typedef struct _INFO
{
    PRDPSETTINGS pRdpSettings;
    PDISPLAY_DEVICE_ENTRY DisplayDeviceList;
    PDISPLAY_DEVICE_ENTRY CurrentDisplayDevice;
    HWND hSelf;
    HWND hTab;
    HWND hGeneralPage;
    HWND hDisplayPage;
    HBITMAP hHeader;
    BITMAP headerbitmap;
    HICON hLogon;
    HICON hConn;
    HICON hRemote;
    HICON hColor;
    HBITMAP hSpectrum;
    BITMAP bitmap;
} INFO, *PINFO;

HINSTANCE hInst;
extern char g_servername[];

static VOID
OnTabWndSelChange(PINFO pInfo)
{
    switch (TabCtrl_GetCurSel(pInfo->hTab))
    {
        case 0: //General
            ShowWindow(pInfo->hGeneralPage, SW_SHOW);
            ShowWindow(pInfo->hDisplayPage, SW_HIDE);
            BringWindowToTop(pInfo->hGeneralPage);
            break;
        case 1: //Display
            ShowWindow(pInfo->hGeneralPage, SW_HIDE);
            ShowWindow(pInfo->hDisplayPage, SW_SHOW);
            BringWindowToTop(pInfo->hDisplayPage);
            break;
    }
}


static VOID
FillServerAddesssCombo(PINFO pInfo)
{
    HKEY hKey;
    TCHAR KeyName[] = _T("Software\\Microsoft\\Terminal Server Client\\Default");
    TCHAR Name[MAX_KEY_NAME];
    LONG ret = ERROR_SUCCESS;
    DWORD size;
    INT i = 0;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     KeyName,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        while (ret == ERROR_SUCCESS)
        {
            size = MAX_KEY_NAME;
            ret = RegEnumValue(hKey,
                               i,
                               Name,
                               &size,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
            if (ret == ERROR_SUCCESS)
            {
                size = MAX_KEY_NAME;
                if (RegQueryValueEx(hKey,
                                    Name,
                                    0,
                                    NULL,
                                    NULL,
                                    &size) == ERROR_SUCCESS)
                {
                    LPTSTR lpAddress = HeapAlloc(GetProcessHeap(),
                                                 0,
                                                 size);
                    if (lpAddress)
                    {
                        if (RegQueryValueEx(hKey,
                                            Name,
                                            0,
                                            NULL,
                                            lpAddress,
                                            &size) == ERROR_SUCCESS)
                        {
                            SendDlgItemMessage(pInfo->hGeneralPage,
                                               IDC_SERVERCOMBO,
                                               CB_ADDSTRING,
                                               0,
                                               (LPARAM)lpAddress);
                        }

                        HeapFree(GetProcessHeap(),
                                 0,
                                 lpAddress);
                    }
                }
            }

            i++;
        }
    }

    if (LoadString(hInst,
                   IDS_BROWSESERVER,
                   Name,
                   sizeof(Name) / sizeof(TCHAR)))
    {
        SendDlgItemMessage(pInfo->hGeneralPage,
                           IDC_SERVERCOMBO,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)Name);
    }
}


static VOID
GeneralOnInit(PINFO pInfo)
{
    LPWSTR lpText;

    SetWindowPos(pInfo->hGeneralPage,
                 NULL, 
                 15, 
                 110, 
                 0, 
                 0, 
                 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

    pInfo->hLogon = LoadImage(hInst,
                              MAKEINTRESOURCE(IDI_LOGON),
                              IMAGE_ICON,
                              32,
                              32,
                              LR_DEFAULTCOLOR);
    if (pInfo->hLogon)
    {
        SendDlgItemMessage(pInfo->hGeneralPage,
                           IDC_LOGONICON,
                           STM_SETICON,
                           (WPARAM)pInfo->hLogon,
                           0);
    }

    pInfo->hConn = LoadImage(hInst,
                             MAKEINTRESOURCE(IDI_CONN),
                             IMAGE_ICON,
                             32,
                             32,
                             LR_DEFAULTCOLOR);
    if (pInfo->hConn)
    {
        SendDlgItemMessage(pInfo->hGeneralPage,
                           IDC_CONNICON,
                           STM_SETICON,
                           (WPARAM)pInfo->hConn,
                           0);
    }

    FillServerAddesssCombo(pInfo);

    /* add file address */
    lpText = GetStringFromSettings(pInfo->pRdpSettings,
                                   L"full address");
    if (lpText)
    {
        SetDlgItemTextW(pInfo->hGeneralPage,
                        IDC_SERVERCOMBO,
                        lpText);
    }

}


INT_PTR CALLBACK
GeneralDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    PINFO pInfo = (PINFO)GetWindowLongPtr(GetParent(hDlg),
                                          GWLP_USERDATA);

    switch (message)
    {
        case WM_INITDIALOG:
            pInfo->hGeneralPage = hDlg;
            GeneralOnInit(pInfo);
            return TRUE;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_SERVERCOMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        INT last, cur;
                        
                        cur = SendDlgItemMessage(hDlg,
                                                 IDC_SERVERCOMBO,
                                                 CB_GETCURSEL,
                                                 0,
                                                 0);
                        cur++;

                        last = SendDlgItemMessage(hDlg,
                                                  IDC_SERVERCOMBO,
                                                  CB_GETCOUNT,
                                                  0,
                                                  0);
                        if (cur == last)
                            MessageBox(hDlg, _T("SMB is not yet supported"), _T("RDP error"), MB_ICONERROR);
                    }
                    break;

                case IDC_SAVE:
                    SaveRdpSettingsToFile(NULL, pInfo->pRdpSettings);
                break;
            }

            break;
        }

        case WM_CLOSE:
        {
            if (pInfo->hLogon)
                DestroyIcon(pInfo->hLogon);

            if (pInfo->hConn)
                DestroyIcon(pInfo->hConn);

            break;
        }
    }

    return 0;
}


static PSETTINGS_ENTRY
GetPossibleSettings(IN LPCTSTR DeviceName,
                    OUT DWORD* pSettingsCount,
                    OUT PSETTINGS_ENTRY* CurrentSettings)
{
    DEVMODE devmode;
    DWORD NbSettings = 0;
    DWORD iMode = 0;
    DWORD dwFlags = 0;
    PSETTINGS_ENTRY Settings = NULL;
    HDC hDC;
    PSETTINGS_ENTRY Current;
    DWORD bpp, xres, yres, checkbpp;

    /* Get current settings */
    *CurrentSettings = NULL;
    hDC = CreateIC(NULL, DeviceName, NULL, NULL);
    bpp = GetDeviceCaps(hDC, PLANES);
    bpp *= GetDeviceCaps(hDC, BITSPIXEL);
    xres = GetDeviceCaps(hDC, HORZRES);
    yres = GetDeviceCaps(hDC, VERTRES);
    DeleteDC(hDC);

    /* List all settings */
    devmode.dmSize = (WORD)sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;

    if (!EnumDisplaySettingsEx(DeviceName, ENUM_CURRENT_SETTINGS, &devmode, dwFlags))
        return NULL;

    while (EnumDisplaySettingsEx(DeviceName, iMode, &devmode, dwFlags))
    {
        if (devmode.dmBitsPerPel==8 ||
            devmode.dmBitsPerPel==16 ||
            devmode.dmBitsPerPel==24 ||
            devmode.dmBitsPerPel==32)
        {
            checkbpp=1;
        }
        else
            checkbpp=0;

        if (devmode.dmPelsWidth < 640 ||
            devmode.dmPelsHeight < 480 || checkbpp == 0)
        {
            iMode++;
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
            while (Next != NULL &&
                   (Next->dmPelsWidth < Current->dmPelsWidth ||
                    (Next->dmPelsWidth == Current->dmPelsWidth && Next->dmPelsHeight < Current->dmPelsHeight) ||
                    (Next->dmPelsHeight == Current->dmPelsHeight &&
                     Next->dmPelsWidth == Current->dmPelsWidth &&
                     Next->dmBitsPerPel < Current->dmBitsPerPel )))
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
        iMode++;
    }

    *pSettingsCount = NbSettings;
    return Settings;
}


static BOOL
AddDisplayDevice(PINFO pInfo, PDISPLAY_DEVICE DisplayDevice)
{
    PDISPLAY_DEVICE_ENTRY newEntry = NULL;
    LPTSTR description = NULL;
    LPTSTR name = NULL;
    LPTSTR key = NULL;
    LPTSTR devid = NULL;
    DWORD descriptionSize, nameSize, keySize, devidSize;
    PSETTINGS_ENTRY Current;
    DWORD ResolutionsCount = 1;
    DWORD i;

    newEntry = HeapAlloc(GetProcessHeap(),
                         0,
                         sizeof(DISPLAY_DEVICE_ENTRY));
    if (!newEntry) goto ByeBye;
    ZeroMemory(newEntry, sizeof(DISPLAY_DEVICE_ENTRY));

    newEntry->Settings = GetPossibleSettings(DisplayDevice->DeviceName,
                                             &newEntry->SettingsCount,
                                             &newEntry->CurrentSettings);
    if (!newEntry->Settings) goto ByeBye;

    newEntry->InitialSettings.dmPelsWidth = newEntry->CurrentSettings->dmPelsWidth;
    newEntry->InitialSettings.dmPelsHeight = newEntry->CurrentSettings->dmPelsHeight;
    newEntry->InitialSettings.dmBitsPerPel = newEntry->CurrentSettings->dmBitsPerPel;

    /* Count different resolutions */
    for (Current = newEntry->Settings; Current != NULL; Current = Current->Flink)
    {
        if (Current->Flink != NULL &&
            ((Current->dmPelsWidth != Current->Flink->dmPelsWidth) &&
            (Current->dmPelsHeight != Current->Flink->dmPelsHeight)))
        {
            ResolutionsCount++;
        }
    }

    newEntry->Resolutions = HeapAlloc(GetProcessHeap(),
                                      0,
                                      ResolutionsCount * sizeof(RESOLUTION_INFO));
    if (!newEntry->Resolutions) goto ByeBye;

    newEntry->ResolutionsCount = ResolutionsCount;

    /* Fill resolutions infos */
    for (Current = newEntry->Settings, i = 0; Current != NULL; Current = Current->Flink)
    {
        if (Current->Flink == NULL ||
            (Current->Flink != NULL &&
            ((Current->dmPelsWidth != Current->Flink->dmPelsWidth) &&
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
    newEntry->Flink = pInfo->DisplayDeviceList;
    pInfo->DisplayDeviceList = newEntry;
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
    if (devid != NULL)
        HeapFree(GetProcessHeap(), 0, devid);
    return FALSE;
}


static VOID
OnResolutionChanged(PINFO pInfo, DWORD position)
{
    TCHAR Buffer[64];
    INT MaxSlider;

    MaxSlider = SendDlgItemMessageW(pInfo->hDisplayPage,
                                    IDC_GEOSLIDER,
                                    TBM_GETRANGEMAX,
                                    0,
                                    0);

    if (position == MaxSlider)
    {
        LoadString(hInst,
                   IDS_FULLSCREEN,
                   Buffer,
                   sizeof(Buffer) / sizeof(TCHAR));
    }
    else
    {
        TCHAR Pixel[64];

        if (LoadString(hInst,
                       IDS_PIXEL,
                       Pixel,
                       sizeof(Pixel) / sizeof(TCHAR)))
        {
            _stprintf(Buffer,
                      Pixel,
                      pInfo->DisplayDeviceList->Resolutions[position].dmPelsWidth,
                      pInfo->DisplayDeviceList->Resolutions[position].dmPelsHeight,
                      Pixel);
        }
    }

    SendDlgItemMessage(pInfo->hDisplayPage,
                       IDC_SETTINGS_RESOLUTION_TEXT,
                       WM_SETTEXT,
                       0,
                       (LPARAM)Buffer);

    /* save new settings */
    SetIntegerToSettings(pInfo->pRdpSettings,
                         L"desktopwidth",
                         pInfo->DisplayDeviceList->Resolutions[position].dmPelsWidth);
    SetIntegerToSettings(pInfo->pRdpSettings,
                         L"desktopheight",
                         pInfo->DisplayDeviceList->Resolutions[position].dmPelsHeight);

}


static VOID
FillResolutionsAndColors(PINFO pInfo)
{
    PSETTINGS_ENTRY Current;
    TCHAR Buffer[64];
    TCHAR Pixel[64];
    DWORD index, i, num;
    DWORD MaxBpp = 0;
    DWORD width, height;
    UINT types[4];

    pInfo->CurrentDisplayDevice = pInfo->DisplayDeviceList; /* Update global variable */

    /* find max bpp */
    SendDlgItemMessage(pInfo->hDisplayPage,
                       IDC_BPPCOMBO,
                       CB_RESETCONTENT,
                       0,
                       0);
    for (Current = pInfo->DisplayDeviceList->Settings; Current != NULL; Current = Current->Flink)
    {
        if (Current->dmBitsPerPel > MaxBpp)
            MaxBpp = Current->dmBitsPerPel;
    }
    switch (MaxBpp)
    {
        case 32:
        case 24: num = 4; break;
        case 16: num = 3; break;
        case 8:  num = 1; break;
    }

    types[0] = IDS_256COLORS;
    types[1] = IDS_HIGHCOLOR15;
    types[2] = IDS_HIGHCOLOR16;
    types[3] = IDS_HIGHCOLOR24;

    /* Fill color depths combo box */
    SendDlgItemMessage(pInfo->hDisplayPage,
                      IDC_BPPCOMBO,
                      CB_RESETCONTENT,
                      0,
                      0);

    for (i = 0, Current = pInfo->DisplayDeviceList->Settings;
         i <= num && Current != NULL;
         i++, Current = Current->Flink)
    {
        TCHAR Buffer[64];
        if (LoadString(hInst,
                       types[i],
                       Buffer,
                       sizeof(Buffer) / sizeof(TCHAR)))
        {
            index = (DWORD)SendDlgItemMessage(pInfo->hDisplayPage,
                                              IDC_BPPCOMBO,
                                              CB_FINDSTRINGEXACT,
                                              -1,
                                              (LPARAM)Buffer);
            if (index == (DWORD)CB_ERR)
            {
                index = (DWORD)SendDlgItemMessage(pInfo->hDisplayPage,
                                                  IDC_BPPCOMBO,
                                                  CB_ADDSTRING,
                                                  0,
                                                  (LPARAM)Buffer);
                SendDlgItemMessage(pInfo->hDisplayPage,
                                   IDC_BPPCOMBO,
                                   CB_SETITEMDATA,
                                   index,
                                   types[i]);
            }
        }
    }

    /* Fill resolutions slider */
    SendDlgItemMessage(pInfo->hDisplayPage,
                       IDC_GEOSLIDER,
                       TBM_CLEARTICS,
                       TRUE,
                       0);
    SendDlgItemMessage(pInfo->hDisplayPage,
                       IDC_GEOSLIDER,
                       TBM_SETRANGE,
                       TRUE,
                       MAKELONG(0, pInfo->DisplayDeviceList->ResolutionsCount)); //extra 1 for full screen

    width = GetIntegerFromSettings(pInfo->pRdpSettings, L"desktopwidth");
    height = GetIntegerFromSettings(pInfo->pRdpSettings, L"desktopheight");

    if (width && height)
    {
        for (index = 0; index < pInfo->CurrentDisplayDevice->ResolutionsCount; index++)
        {
            if (pInfo->CurrentDisplayDevice->Resolutions[index].dmPelsWidth == width &&
                pInfo->CurrentDisplayDevice->Resolutions[index].dmPelsHeight == height)
            {
                SendDlgItemMessage(pInfo->hDisplayPage,
                                   IDC_GEOSLIDER,
                                   TBM_SETPOS,
                                   TRUE,
                                   index);
                break;
            }
        }

        if (LoadString(hInst,
                       IDS_PIXEL,
                       Pixel,
                       sizeof(Pixel) / sizeof(TCHAR)))
        {
            _stprintf(Buffer,
                      Pixel,
                      width,
                      height,
                      Pixel);
            SendDlgItemMessage(pInfo->hDisplayPage,
                               IDC_SETTINGS_RESOLUTION_TEXT,
                               WM_SETTEXT,
                               0,
                               (LPARAM)Buffer);
        }
    }
}


static VOID
DisplayOnInit(PINFO pInfo)
{
    DISPLAY_DEVICE displayDevice;
    DWORD iDevNum = 0;
    BOOL GotDev = FALSE;

    SetWindowPos(pInfo->hDisplayPage,
                 NULL,
                 15,
                 110,
                 0,
                 0,
                 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

        pInfo->hRemote = LoadImage(hInst,
                                   MAKEINTRESOURCE(IDI_REMOTE),
                                   IMAGE_ICON,
                                   32,
                                   32,
                                   LR_DEFAULTCOLOR);
        if (pInfo->hRemote)
        {
            SendDlgItemMessage(pInfo->hDisplayPage,
                               IDC_REMICON,
                               STM_SETICON,
                               (WPARAM)pInfo->hRemote,
                               0);
        }

        pInfo->hColor = LoadImage(hInst,
                                  MAKEINTRESOURCE(IDI_COLORS),
                                  IMAGE_ICON,
                                  32,
                                  32,
                                  LR_DEFAULTCOLOR);
        if (pInfo->hColor)
        {
            SendDlgItemMessage(pInfo->hDisplayPage,
                               IDC_COLORSICON,
                               STM_SETICON,
                               (WPARAM)pInfo->hColor,
                               0);
        }

        pInfo->hSpectrum = LoadImage(hInst,
                                     MAKEINTRESOURCE(IDB_SPECT),
                                     IMAGE_BITMAP,
                                     0,
                                     0,
                                     LR_DEFAULTCOLOR);
        if (pInfo->hSpectrum)
        {
            GetObject(pInfo->hSpectrum,
                      sizeof(BITMAP),
                      &pInfo->bitmap);
        }

        /* Get video cards list */
        displayDevice.cb = (DWORD)sizeof(DISPLAY_DEVICE);
        while (EnumDisplayDevices(NULL, iDevNum, &displayDevice, 0x1))
        {
            if ((displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0)
            {
                if (AddDisplayDevice(pInfo, &displayDevice))
                    GotDev = TRUE;
            }
            iDevNum++;
        }

        if (GotDev)
        {
            INT bpp, num, i;
            BOOL bSet = FALSE;

            FillResolutionsAndColors(pInfo);

             /* set color combo */
            bpp = GetIntegerFromSettings(pInfo->pRdpSettings, L"session bpp");

            num = SendDlgItemMessage(pInfo->hDisplayPage,
                                     IDC_BPPCOMBO,
                                     CB_GETCOUNT,
                                     0,
                                     0);
            for (i = 0; i < num; i++)
            {
                INT data = SendDlgItemMessage(pInfo->hDisplayPage,
                                              IDC_BPPCOMBO,
                                              CB_GETITEMDATA,
                                              i,
                                              0);
                if (data == bpp)
                {
                    SendDlgItemMessage(pInfo->hDisplayPage,
                                       IDC_BPPCOMBO,
                                       CB_SETCURSEL,
                                       i,
                                       0);
                    bSet = TRUE;
                    break;
                }
            }

            if (!bSet)
            {
                SendDlgItemMessage(pInfo->hDisplayPage,
                                   IDC_BPPCOMBO,
                                   CB_SETCURSEL,
                                   num - 1,
                                   0);
            }
        }
}


INT_PTR CALLBACK
DisplayDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    PINFO pInfo = (PINFO)GetWindowLongPtr(GetParent(hDlg),
                                          GWLP_USERDATA);

    switch (message)
    {
        case WM_INITDIALOG:
            pInfo->hDisplayPage = hDlg;
            DisplayOnInit(pInfo);
            return TRUE;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
            if(lpDrawItem->CtlID == IDC_COLORIMAGE)
            {
                HDC hdcMem;
                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    SelectObject(hdcMem, pInfo->hSpectrum);
                    StretchBlt(lpDrawItem->hDC,
                               lpDrawItem->rcItem.left,
                               lpDrawItem->rcItem.top,
                               lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                               lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                               hdcMem,
                               0,
                               0,
                               pInfo->bitmap.bmWidth,
                               pInfo->bitmap.bmHeight,
                               SRCCOPY);
                    DeleteDC(hdcMem);
                }
            }
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
                    DWORD newPosition = (DWORD)SendDlgItemMessage(hDlg, IDC_GEOSLIDER, TBM_GETPOS, 0, 0);
                    OnResolutionChanged(pInfo, newPosition);
                    break;
                }

                case TB_THUMBTRACK:
                    OnResolutionChanged(pInfo, HIWORD(wParam));
                    break;
            }
            break;
        }

        case WM_CLOSE:
        {
            if (pInfo->hRemote)
                DestroyIcon(pInfo->hRemote);

            if (pInfo->hColor)
                DestroyIcon(pInfo->hColor);

            if (pInfo->hSpectrum)
                DeleteObject(pInfo->hSpectrum);

            break;
        }

        break;
    }
    return 0;
}



static BOOL
OnMainCreate(HWND hwnd,
             PRDPSETTINGS pRdpSettings)
{
    PINFO pInfo;
    TCITEM item;
    BOOL bRet = FALSE;

    pInfo = HeapAlloc(GetProcessHeap(),
                      HEAP_ZERO_MEMORY,
                      sizeof(INFO));
    if (pInfo)
    {
        SetWindowLongPtr(hwnd,
                         GWLP_USERDATA,
                         (LONG_PTR)pInfo);

        /* add main settings pointer */
        pInfo->pRdpSettings = pRdpSettings;

        pInfo->hHeader = (HBITMAP)LoadImage(hInst,
                                            MAKEINTRESOURCE(IDB_HEADER),
                                            IMAGE_BITMAP,
                                            0,
                                            0,
                                            LR_DEFAULTCOLOR);
        if (pInfo->hHeader)
        {
            GetObject(pInfo->hHeader, sizeof(BITMAP), &pInfo->headerbitmap);
        }

        pInfo->hTab = GetDlgItem(hwnd, IDC_TAB);
        if (pInfo->hTab)
        {
            if (CreateDialog(hInst,
                             MAKEINTRESOURCE(IDD_GENERAL),
                             hwnd,
                             (DLGPROC)GeneralDlgProc))
            {
                char str[256];
                LoadString(hInst, IDS_TAB_GENERAL, str, 256);
                ZeroMemory(&item, sizeof(TCITEM));
                item.mask = TCIF_TEXT;
                item.pszText = str;
                item.cchTextMax = 256;
                (void)TabCtrl_InsertItem(pInfo->hTab, 0, &item);
            }

            if (CreateDialog(hInst,
                             MAKEINTRESOURCE(IDD_DISPLAY),
                             hwnd,
                             (DLGPROC)DisplayDlgProc))
            {
                char str[256];
                LoadString(hInst, IDS_TAB_DISPLAY, str, 256);
                ZeroMemory(&item, sizeof(TCITEM));
                item.mask = TCIF_TEXT;
                item.pszText = str;
                item.cchTextMax = 256;
                (void)TabCtrl_InsertItem(pInfo->hTab, 1, &item);
            }

            OnTabWndSelChange(pInfo);
        }
    }

    return bRet;
}


static BOOL CALLBACK
DlgProc(HWND hDlg,
        UINT Message,
        WPARAM wParam,
        LPARAM lParam)
{
    PINFO pInfo;

    /* Get the window context */
    pInfo = (PINFO)GetWindowLongPtr(hDlg,
                                    GWLP_USERDATA);
    if (pInfo == NULL && Message != WM_INITDIALOG)
    {
        goto HandleDefaultMessage;
    }

    switch(Message)
    {
        case WM_INITDIALOG:
            OnMainCreate(hDlg, (PRDPSETTINGS)lParam);
        break;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                if (pInfo)
                {
                    HeapFree(GetProcessHeap(),
                             0,
                             pInfo);
                }

                EndDialog(hDlg, LOWORD(wParam));
            }

            switch(LOWORD(wParam))
            {

            }

            break;
        }

        case WM_NOTIFY:
        {
            INT idctrl;
            LPNMHDR pnmh;
            idctrl = (int)wParam;
            pnmh = (LPNMHDR)lParam;
            if (//(pnmh->hwndFrom == pInfo->hSelf) &&
                (pnmh->idFrom == IDC_TAB) &&
                (pnmh->code == TCN_SELCHANGE))
            {
                OnTabWndSelChange(pInfo);
            }

            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc, hdcMem;

            hdc = BeginPaint(hDlg, &ps);
            if (hdc != NULL)
            {
                HDC hdcMem = CreateCompatibleDC(hdc);
                if (hdcMem)
                {
                    SelectObject(hdcMem, pInfo->hHeader);
                    BitBlt(hdc,
                           0,
                           0,
                           pInfo->headerbitmap.bmWidth,
                           pInfo->headerbitmap.bmHeight,
                           hdcMem,
                           0,
                           0,
                           SRCCOPY);
                    DeleteDC(hdcMem);
                }

                EndPaint(hDlg, &ps);
            }

            break;
        }

        case WM_CLOSE:
        {
            if (pInfo)
            {
                HeapFree(GetProcessHeap(),
                         0,
                         pInfo);
            }

            EndDialog(hDlg, 0);
        }
        break;

HandleDefaultMessage:
        default:
            return FALSE;
    }

    return FALSE;
}

BOOL
OpenRDPConnectDialog(HINSTANCE hInstance,
                     PRDPSETTINGS pRdpSettings)
{
    INITCOMMONCONTROLSEX iccx;

    hInst = hInstance;

    iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&iccx);

    return (DialogBoxParam(hInst,
                           MAKEINTRESOURCE(IDD_CONNECTDIALOG),
                           NULL,
                           (DLGPROC)DlgProc,
                           pRdpSettings) == IDOK);
}
