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

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "precomp.h"

#include <winreg.h>
#include <commdlg.h>

#define MAX_KEY_NAME 255

HINSTANCE hInst;

static VOID ReLoadGeneralPage(PINFO pInfo);
static VOID ReLoadDisplayPage(PINFO pInfo);

static VOID
DoOpenFile(PINFO pInfo)
{
    OPENFILENAMEW ofn;
    WCHAR szFileName[MAX_PATH] = L"Default.rdp";
    static WCHAR szFilter[] = L"Remote Desktop Files (*.RDP)\0*.rdp\0";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize   = sizeof(OPENFILENAMEW);
    ofn.hwndOwner     = pInfo->hGeneralPage;
    ofn.nMaxFile      = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrDefExt   = L"RDP";
    ofn.lpstrFilter   = szFilter;
    ofn.lpstrFile     = szFileName;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn))
    {
        LoadRdpSettingsFromFile(pInfo->pRdpSettings, szFileName);
        ReLoadGeneralPage(pInfo);
        ReLoadDisplayPage(pInfo);
    }
}


static VOID
DoSaveAs(PINFO pInfo)
{
    OPENFILENAMEW ofn;
    WCHAR szFileName[MAX_PATH] = L"Default.rdp";
    static WCHAR szFilter[] = L"Remote Desktop Files (*.RDP)\0*.rdp\0";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize   = sizeof(OPENFILENAMEW);
    ofn.hwndOwner     = pInfo->hGeneralPage;
    ofn.nMaxFile      = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrDefExt   = L"RDP";
    ofn.lpstrFilter   = szFilter;
    ofn.lpstrFile     = szFileName;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn))
    {
        SaveAllSettings(pInfo);
        SaveRdpSettingsToFile(szFileName, pInfo->pRdpSettings);
    }
}


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
LoadUsernameHint(HWND hDlg, INT iCur)
{
    WCHAR szValue[MAXVALUE+1000];
    WCHAR szName[MAX_KEY_NAME];
    WCHAR szKeyName[] = L"Software\\Microsoft\\Terminal Server Client\\Servers";
    PWCHAR lpAddress;
    HKEY hKey;
    HKEY hSubKey;
    LONG lRet = ERROR_SUCCESS;
    INT iIndex = 0;
    DWORD dwSize = MAX_KEY_NAME;

    SendDlgItemMessageW(hDlg, IDC_SERVERCOMBO, CB_GETLBTEXT, (WPARAM)iCur, (LPARAM)szValue);

    /* remove possible port number */
    lpAddress = wcstok(szValue, L":");

    if (lpAddress == NULL)
        return;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      szKeyName,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {
        while (lRet == ERROR_SUCCESS)
        {
            dwSize = MAX_KEY_NAME;

            lRet = RegEnumKeyExW(hKey, iIndex, szName, &dwSize, NULL, NULL, NULL, NULL);

            if(lRet == ERROR_SUCCESS && wcscmp(szName, lpAddress) == 0)
            {
                if(RegOpenKeyExW(hKey, szName, 0, KEY_READ, &hSubKey) != ERROR_SUCCESS)
                    break;

                dwSize = MAXVALUE * sizeof(WCHAR);

                if(RegQueryValueExW(hKey, L"UsernameHint", 0,  NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
                {
                    SetDlgItemTextW(hDlg, IDC_NAMEEDIT, szValue);
                }

                RegCloseKey(hSubKey);
                break;
            }
            iIndex++;
        }
        RegCloseKey(hKey);
    }
}


static VOID
FillServerAddressCombo(PINFO pInfo)
{
    HKEY hKey;
    WCHAR KeyName[] = L"Software\\Microsoft\\Terminal Server Client\\Default";
    WCHAR Name[MAX_KEY_NAME];
    LONG ret = ERROR_SUCCESS;
    DWORD size;
    INT i = 0;
    BOOL found = FALSE;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      KeyName,
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {
        while (ret == ERROR_SUCCESS)
        {
            size = MAX_KEY_NAME;
            ret = RegEnumValueW(hKey,
                                i,
                                Name,
                                &size,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
            if (ret == ERROR_SUCCESS)
            {
                size = sizeof(Name);
                if (RegQueryValueExW(hKey,
                                     Name,
                                     0,
                                     NULL,
                                     NULL,
                                     &size) == ERROR_SUCCESS)
                {
                    LPWSTR lpAddress = HeapAlloc(GetProcessHeap(),
                                                 0,
                                                 size);
                    if (lpAddress)
                    {
                        if (RegQueryValueExW(hKey,
                                             Name,
                                             0,
                                             NULL,
                                             (LPBYTE)lpAddress,
                                             &size) == ERROR_SUCCESS)
                        {
                            SendDlgItemMessageW(pInfo->hGeneralPage,
                                                IDC_SERVERCOMBO,
                                                CB_ADDSTRING,
                                                0,
                                                (LPARAM)lpAddress);
                            found = TRUE;
                        }

                        HeapFree(GetProcessHeap(),
                                 0,
                                 lpAddress);
                    }
                }
            }

            i++;
        }
        RegCloseKey(hKey);
    }

    if (LoadStringW(hInst,
                    IDS_BROWSESERVER,
                    Name,
                    sizeof(Name) / sizeof(WCHAR)))
    {
        SendDlgItemMessageW(pInfo->hGeneralPage,
                            IDC_SERVERCOMBO,
                            CB_ADDSTRING,
                            0,
                            (LPARAM)Name);
    }

    if(found)
    {
        SendDlgItemMessageW(pInfo->hGeneralPage,
                            IDC_SERVERCOMBO,
                            CB_SETCURSEL,
                            0,
                            0);
        LoadUsernameHint(pInfo->hGeneralPage, 0);
    }

}


static VOID
ReLoadGeneralPage(PINFO pInfo)
{
    LPWSTR lpText;

    /* add file address */
    lpText = GetStringFromSettings(pInfo->pRdpSettings,
                                   L"full address");
    if (lpText)
    {
        SetDlgItemTextW(pInfo->hGeneralPage,
                        IDC_SERVERCOMBO,
                        lpText);
    }

    /* set user name */
    lpText = GetStringFromSettings(pInfo->pRdpSettings,
                                   L"username");
    if (lpText)
    {
        SetDlgItemTextW(pInfo->hGeneralPage,
                        IDC_NAMEEDIT,
                        lpText);
    }
}


static VOID
GeneralOnInit(HWND hwnd,
              PINFO pInfo)
{
    SetWindowLongPtrW(hwnd,
                      GWLP_USERDATA,
                      (LONG_PTR)pInfo);

    pInfo->hGeneralPage = hwnd;

    SetWindowPos(pInfo->hGeneralPage,
                 NULL,
                 2,
                 22,
                 0,
                 0,
                 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

    pInfo->hLogon = LoadImageW(hInst,
                               MAKEINTRESOURCEW(IDI_LOGON),
                               IMAGE_ICON,
                               32,
                               32,
                               LR_DEFAULTCOLOR);
    if (pInfo->hLogon)
    {
        SendDlgItemMessageW(pInfo->hGeneralPage,
                            IDC_LOGONICON,
                            STM_SETICON,
                            (WPARAM)pInfo->hLogon,
                            0);
    }

    pInfo->hConn = LoadImageW(hInst,
                              MAKEINTRESOURCEW(IDI_CONN),
                              IMAGE_ICON,
                              32,
                              32,
                              LR_DEFAULTCOLOR);
    if (pInfo->hConn)
    {
        SendDlgItemMessageW(pInfo->hGeneralPage,
                            IDC_CONNICON,
                            STM_SETICON,
                            (WPARAM)pInfo->hConn,
                            0);
    }

    FillServerAddressCombo(pInfo);
    ReLoadGeneralPage(pInfo);
}


INT_PTR CALLBACK
GeneralDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    PINFO pInfo = (PINFO)GetWindowLongPtrW(hDlg,
                                           GWLP_USERDATA);

    switch (message)
    {
        case WM_INITDIALOG:
            GeneralOnInit(hDlg, (PINFO)lParam);
            return TRUE;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_SERVERCOMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        INT last, cur;

                        cur = SendDlgItemMessageW(hDlg,
                                                  IDC_SERVERCOMBO,
                                                  CB_GETCURSEL,
                                                  0,
                                                  0);

                        last = SendDlgItemMessageW(hDlg,
                                                   IDC_SERVERCOMBO,
                                                   CB_GETCOUNT,
                                                   0,
                                                   0);
                        if ((cur + 1) == last)
                            MessageBoxW(hDlg, L"SMB is not yet supported", L"RDP error", MB_ICONERROR);
                        else
                        {
                            LoadUsernameHint(hDlg, cur);
                        }
                    }
                    break;

                case IDC_SAVE:
                    SaveAllSettings(pInfo);
                    SaveRdpSettingsToFile(NULL, pInfo->pRdpSettings);
                break;

                case IDC_SAVEAS:
                    DoSaveAs(pInfo);
                break;

                case IDC_OPEN:
                    DoOpenFile(pInfo);
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
GetPossibleSettings(IN LPCWSTR lpDeviceName,
                    OUT DWORD* pSettingsCount,
                    OUT PSETTINGS_ENTRY* CurrentSettings)
{
    DEVMODEW devmode;
    DWORD NbSettings = 0;
    DWORD iMode = 0;
    DWORD dwFlags = 0;
    PSETTINGS_ENTRY Settings = NULL;
    HDC hDC;
    PSETTINGS_ENTRY Current;
    DWORD bpp, xres, yres, checkbpp;

    /* Get current settings */
    *CurrentSettings = NULL;
    hDC = CreateICW(NULL, lpDeviceName, NULL, NULL);
    bpp = GetDeviceCaps(hDC, PLANES);
    bpp *= GetDeviceCaps(hDC, BITSPIXEL);
    xres = GetDeviceCaps(hDC, HORZRES);
    yres = GetDeviceCaps(hDC, VERTRES);
    DeleteDC(hDC);

    /* List all settings */
    devmode.dmSize = (WORD)sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;

    if (!EnumDisplaySettingsExW(lpDeviceName, ENUM_CURRENT_SETTINGS, &devmode, dwFlags))
        return NULL;

    while (EnumDisplaySettingsExW(lpDeviceName, iMode, &devmode, dwFlags))
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
AddDisplayDevice(PINFO pInfo, PDISPLAY_DEVICEW DisplayDevice)
{
    PDISPLAY_DEVICE_ENTRY newEntry = NULL;
    LPWSTR description = NULL;
    LPWSTR name = NULL;
    LPWSTR key = NULL;
    LPWSTR devid = NULL;
    SIZE_T descriptionSize, nameSize, keySize, devidSize;
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
                                      ResolutionsCount * (sizeof(RESOLUTION_INFO) + 1));
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

    /* fullscreen */
    newEntry->Resolutions[i].dmPelsWidth = GetSystemMetrics(SM_CXSCREEN);
    newEntry->Resolutions[i].dmPelsHeight = GetSystemMetrics(SM_CYSCREEN);

    descriptionSize = (wcslen(DisplayDevice->DeviceString) + 1) * sizeof(WCHAR);
    description = HeapAlloc(GetProcessHeap(), 0, descriptionSize);
    if (!description) goto ByeBye;

    nameSize = (wcslen(DisplayDevice->DeviceName) + 1) * sizeof(WCHAR);
    name = HeapAlloc(GetProcessHeap(), 0, nameSize);
    if (!name) goto ByeBye;

    keySize = (wcslen(DisplayDevice->DeviceKey) + 1) * sizeof(WCHAR);
    key = HeapAlloc(GetProcessHeap(), 0, keySize);
    if (!key) goto ByeBye;

    devidSize = (wcslen(DisplayDevice->DeviceID) + 1) * sizeof(WCHAR);
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
OnResolutionChanged(PINFO pInfo, INT position)
{
    WCHAR Buffer[64];
    INT MaxSlider;

    MaxSlider = SendDlgItemMessageW(pInfo->hDisplayPage,
                                    IDC_GEOSLIDER,
                                    TBM_GETRANGEMAX,
                                    0,
                                    0);

    if (position == MaxSlider)
    {
        LoadStringW(hInst,
                    IDS_FULLSCREEN,
                    Buffer,
                    sizeof(Buffer) / sizeof(WCHAR));
    }
    else
    {
        WCHAR Pixel[64];

        if (LoadStringW(hInst,
                        IDS_PIXEL,
                        Pixel,
                        sizeof(Pixel) / sizeof(WCHAR)))
        {
            swprintf(Buffer,
                     Pixel,
                     pInfo->DisplayDeviceList->Resolutions[position].dmPelsWidth,
                     pInfo->DisplayDeviceList->Resolutions[position].dmPelsHeight,
                     Pixel);
        }
    }

    SendDlgItemMessageW(pInfo->hDisplayPage,
                        IDC_SETTINGS_RESOLUTION_TEXT,
                        WM_SETTEXT,
                        0,
                        (LPARAM)Buffer);
}


static VOID
FillResolutionsAndColors(PINFO pInfo)
{
    PSETTINGS_ENTRY Current;
    DWORD index, i, num;
    DWORD MaxBpp = 0;
    UINT types[5];

    pInfo->CurrentDisplayDevice = pInfo->DisplayDeviceList; /* Update global variable */

    /* find max bpp */
    SendDlgItemMessageW(pInfo->hDisplayPage,
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
        case 32: num = 4; break;
        case 24: num = 3; break;
        case 16: num = 2; break;
        case 15: num = 1; break;
        case 8:  num = 0; break;
        default: num = 0; break;
    }

    types[0] = IDS_256COLORS;
    types[1] = IDS_HIGHCOLOR15;
    types[2] = IDS_HIGHCOLOR16;
    types[3] = IDS_HIGHCOLOR24;
    types[4] = IDS_HIGHCOLOR32;

    /* Fill color depths combo box */
    SendDlgItemMessageW(pInfo->hDisplayPage,
                        IDC_BPPCOMBO,
                        CB_RESETCONTENT,
                        0,
                        0);

    for (i = 0, Current = pInfo->DisplayDeviceList->Settings;
         i <= num && Current != NULL;
         i++, Current = Current->Flink)
    {
        WCHAR Buffer[64];
        if (LoadStringW(hInst,
                        types[i],
                        Buffer,
                        sizeof(Buffer) / sizeof(WCHAR)))
        {
            index = (DWORD)SendDlgItemMessageW(pInfo->hDisplayPage,
                                               IDC_BPPCOMBO,
                                               CB_FINDSTRINGEXACT,
                                               (WPARAM)-1,
                                               (LPARAM)Buffer);
            if (index == (DWORD)CB_ERR)
            {
                index = (DWORD)SendDlgItemMessageW(pInfo->hDisplayPage,
                                                   IDC_BPPCOMBO,
                                                   CB_ADDSTRING,
                                                   0,
                                                   (LPARAM)Buffer);
                SendDlgItemMessageW(pInfo->hDisplayPage,
                                    IDC_BPPCOMBO,
                                    CB_SETITEMDATA,
                                    index,
                                    types[i]);
            }
        }
    }

    /* Fill resolutions slider */
    SendDlgItemMessageW(pInfo->hDisplayPage,
                        IDC_GEOSLIDER,
                        TBM_CLEARTICS,
                        TRUE,
                        0);
    SendDlgItemMessageW(pInfo->hDisplayPage,
                        IDC_GEOSLIDER,
                        TBM_SETRANGE,
                        TRUE,
                        MAKELONG(0, pInfo->DisplayDeviceList->ResolutionsCount)); //extra 1 for full screen


}


static VOID
ReLoadDisplayPage(PINFO pInfo)
{
    DWORD index;
    INT width, height, pos = 0;
    INT bpp, num, i, screenmode;
    BOOL bSet = FALSE;

    /* get fullscreen info */
    screenmode = GetIntegerFromSettings(pInfo->pRdpSettings, L"screen mode id");

    /* set trackbar position */
    width = GetIntegerFromSettings(pInfo->pRdpSettings, L"desktopwidth");
    height = GetIntegerFromSettings(pInfo->pRdpSettings, L"desktopheight");

    if (width != -1 && height != -1)
    {
        if(screenmode == 2)
        {
            pos = SendDlgItemMessageW(pInfo->hDisplayPage,
                                      IDC_GEOSLIDER,
                                      TBM_GETRANGEMAX,
                                      0,
                                      0);
        }
        else
        {
            for (index = 0; index < pInfo->CurrentDisplayDevice->ResolutionsCount; index++)
            {
                if (pInfo->CurrentDisplayDevice->Resolutions[index].dmPelsWidth == width &&
                    pInfo->CurrentDisplayDevice->Resolutions[index].dmPelsHeight == height)
                {
                    pos = index;
                    break;
                }
            }
        }
    }

    /* set slider position */
    SendDlgItemMessageW(pInfo->hDisplayPage,
                        IDC_GEOSLIDER,
                        TBM_SETPOS,
                        TRUE,
                        pos);

    OnResolutionChanged(pInfo, pos);


     /* set color combo */
    bpp = GetIntegerFromSettings(pInfo->pRdpSettings, L"session bpp");

    num = SendDlgItemMessageW(pInfo->hDisplayPage,
                              IDC_BPPCOMBO,
                              CB_GETCOUNT,
                              0,
                              0);
    for (i = 0; i < num; i++)
    {
        INT data = SendDlgItemMessageW(pInfo->hDisplayPage,
                                       IDC_BPPCOMBO,
                                       CB_GETITEMDATA,
                                       i,
                                       0);
        if (data == bpp)
        {
            SendDlgItemMessageW(pInfo->hDisplayPage,
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
        SendDlgItemMessageW(pInfo->hDisplayPage,
                            IDC_BPPCOMBO,
                            CB_SETCURSEL,
                            num - 1,
                            0);
    }
}


static VOID
DisplayOnInit(HWND hwnd,
              PINFO pInfo)
{
    DISPLAY_DEVICEW displayDevice;
    DWORD iDevNum = 0;
    BOOL GotDev = FALSE;

    SetWindowLongPtrW(hwnd,
                      GWLP_USERDATA,
                      (LONG_PTR)pInfo);

    pInfo->hDisplayPage = hwnd;

    SetWindowPos(pInfo->hDisplayPage,
                 NULL,
                 2,
                 22,
                 0,
                 0,
                 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);

        pInfo->hRemote = LoadImageW(hInst,
                                    MAKEINTRESOURCEW(IDI_REMOTE),
                                    IMAGE_ICON,
                                    32,
                                    32,
                                    LR_DEFAULTCOLOR);
        if (pInfo->hRemote)
        {
            SendDlgItemMessageW(pInfo->hDisplayPage,
                                IDC_REMICON,
                                STM_SETICON,
                                (WPARAM)pInfo->hRemote,
                                0);
        }

        pInfo->hColor = LoadImageW(hInst,
                                   MAKEINTRESOURCEW(IDI_COLORS),
                                   IMAGE_ICON,
                                   32,
                                   32,
                                  LR_DEFAULTCOLOR);
        if (pInfo->hColor)
        {
            SendDlgItemMessageW(pInfo->hDisplayPage,
                                IDC_COLORSICON,
                                STM_SETICON,
                                (WPARAM)pInfo->hColor,
                                0);
        }

        pInfo->hSpectrum = LoadImageW(hInst,
                                      MAKEINTRESOURCEW(IDB_SPECT),
                                      IMAGE_BITMAP,
                                      0,
                                      0,
                                      LR_DEFAULTCOLOR);
        if (pInfo->hSpectrum)
        {
            GetObjectW(pInfo->hSpectrum,
                       sizeof(BITMAP),
                       &pInfo->bitmap);
        }

        /* Get video cards list */
        displayDevice.cb = (DWORD)sizeof(DISPLAY_DEVICE);
        while (EnumDisplayDevicesW(NULL, iDevNum, &displayDevice, 0x1))
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
            FillResolutionsAndColors(pInfo);
            ReLoadDisplayPage(pInfo);
        }
}


INT_PTR CALLBACK
DisplayDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    PINFO pInfo = (PINFO)GetWindowLongPtrW(hDlg,
                                           GWLP_USERDATA);

    switch (message)
    {
        case WM_INITDIALOG:
            DisplayOnInit(hDlg, (PINFO)lParam);
            return TRUE;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
            if(lpDrawItem->CtlID == IDC_COLORIMAGE)
            {
                HDC hdcMem;
                HBITMAP hSpecOld;
                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    hSpecOld = SelectObject(hdcMem, pInfo->hSpectrum);
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
                    SelectObject(hdcMem, hSpecOld);
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
                    INT newPosition = (DWORD)SendDlgItemMessageW(hDlg, IDC_GEOSLIDER, TBM_GETPOS, 0, 0);
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
    TCITEMW item;
    BOOL bRet = FALSE;

    pInfo = HeapAlloc(GetProcessHeap(),
                      HEAP_ZERO_MEMORY,
                      sizeof(INFO));
    if (pInfo)
    {
        SetWindowLongPtrW(hwnd,
                          GWLP_USERDATA,
                          (LONG_PTR)pInfo);

        pInfo->hSelf = hwnd;

        /* add main settings pointer */
        pInfo->pRdpSettings = pRdpSettings;

        /* set the dialog icons */
        pInfo->hMstscSm = LoadImageW(hInst,
                                   MAKEINTRESOURCEW(IDI_MSTSC),
                                   IMAGE_ICON,
                                   16,
                                   16,
                                   LR_DEFAULTCOLOR);
        if (pInfo->hMstscSm)
        {
            SendMessageW(hwnd,
                         WM_SETICON,
                         ICON_SMALL,
                        (WPARAM)pInfo->hMstscSm);
        }
        pInfo->hMstscLg = LoadImageW(hInst,
                                   MAKEINTRESOURCEW(IDI_MSTSC),
                                   IMAGE_ICON,
                                   32,
                                   32,
                                   LR_DEFAULTCOLOR);
        if (pInfo->hMstscLg)
        {
            SendMessageW(hwnd,
                         WM_SETICON,
                         ICON_BIG,
                        (WPARAM)pInfo->hMstscLg);
        }

        pInfo->hHeader = (HBITMAP)LoadImageW(hInst,
                                             MAKEINTRESOURCEW(IDB_HEADER),
                                             IMAGE_BITMAP,
                                             0,
                                             0,
                                             LR_DEFAULTCOLOR);
        if (pInfo->hHeader)
        {
            GetObjectW(pInfo->hHeader,
                       sizeof(BITMAP),
                       &pInfo->headerbitmap);
        }

        /* setup the tabs */
        pInfo->hTab = GetDlgItem(hwnd, IDC_TAB);
        if (pInfo->hTab)
        {
            if (CreateDialogParamW(hInst,
                                   MAKEINTRESOURCEW(IDD_GENERAL),
                                   pInfo->hTab,
                                   GeneralDlgProc,
                                   (LPARAM)pInfo))
            {
                WCHAR str[256];
                ZeroMemory(&item, sizeof(TCITEM));
                item.mask = TCIF_TEXT;
                if (LoadStringW(hInst, IDS_TAB_GENERAL, str, 256))
                    item.pszText = str;
                item.cchTextMax = 256;
                (void)TabCtrl_InsertItem(pInfo->hTab, 0, &item);
            }

            if (CreateDialogParamW(hInst,
                                   MAKEINTRESOURCEW(IDD_DISPLAY),
                                   pInfo->hTab,
                                   DisplayDlgProc,
                                   (LPARAM)pInfo))
            {
                WCHAR str[256];
                ZeroMemory(&item, sizeof(TCITEM));
                item.mask = TCIF_TEXT;
                if (LoadStringW(hInst, IDS_TAB_DISPLAY, str, 256))
                    item.pszText = str;
                item.cchTextMax = 256;
                (void)TabCtrl_InsertItem(pInfo->hTab, 1, &item);
            }

            OnTabWndSelChange(pInfo);
        }
    }

    return bRet;
}

static void Cleanup(PINFO pInfo)
{
    if (pInfo)
    {
        if (pInfo->hMstscSm)
            DestroyIcon(pInfo->hMstscSm);
        if (pInfo->hMstscLg)
            DestroyIcon(pInfo->hMstscLg);
        if (pInfo->hHeader)
            DeleteObject(pInfo->hHeader);
        if (pInfo->hSpectrum)
            DeleteObject(pInfo->hSpectrum);
        if (pInfo->hRemote)
            DestroyIcon(pInfo->hRemote);
        if (pInfo->hLogon)
            DestroyIcon(pInfo->hLogon);
        if (pInfo->hConn)
            DestroyIcon(pInfo->hConn);
        if (pInfo->hColor)
            DestroyIcon(pInfo->hColor);
        HeapFree(GetProcessHeap(),
                 0,
                 pInfo);
    }
}

static INT_PTR CALLBACK
DlgProc(HWND hDlg,
        UINT Message,
        WPARAM wParam,
        LPARAM lParam)
{
    PINFO pInfo;

    /* Get the window context */
    pInfo = (PINFO)GetWindowLongPtrW(hDlg,
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
                if (LOWORD(wParam) == IDOK )
                {
                    SaveAllSettings(pInfo);
                    SaveRdpSettingsToFile(NULL, pInfo->pRdpSettings);
                }
                Cleanup(pInfo);
                EndDialog(hDlg, LOWORD(wParam));
            }

            break;
        }

        case WM_NOTIFY:
        {
            //INT idctrl;
            LPNMHDR pnmh;
            //idctrl = (int)wParam;
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
            HDC hdc;

            hdc = BeginPaint(hDlg, &ps);
            if (hdc != NULL)
            {
                HDC hdcMem = CreateCompatibleDC(hdc);
                if (hdcMem)
                {
                    WCHAR szBuffer[32];
                    RECT bmpRc, txtRc;
                    LOGFONTW lf;
                    HFONT hFont, hFontOld;
                    HBITMAP hBmpOld;

                    GetClientRect(pInfo->hSelf, &bmpRc);

                    hBmpOld = SelectObject(hdcMem, pInfo->hHeader);
                    StretchBlt(hdc,
                               0,
                               0,
                               bmpRc.right,
                               pInfo->headerbitmap.bmHeight,
                               hdcMem,
                               0,
                               0,
                               pInfo->headerbitmap.bmWidth,
                               pInfo->headerbitmap.bmHeight,
                               SRCCOPY);

                    SelectObject(hdcMem, hBmpOld);
                    txtRc.left = bmpRc.right / 4;
                    txtRc.top = 10;
                    txtRc.right = bmpRc.right * 3 / 4;
                    txtRc.bottom = pInfo->headerbitmap.bmHeight / 2;

                    ZeroMemory(&lf, sizeof(LOGFONTW));

                    if (LoadStringW(hInst,
                                    IDS_HEADERTEXT1,
                                    szBuffer,
                                    sizeof(szBuffer) / sizeof(WCHAR)))
                    {
                        lf.lfHeight = 20;
                        lf.lfCharSet = OEM_CHARSET;
                        lf.lfQuality = DEFAULT_QUALITY;
                        lf.lfWeight = FW_MEDIUM;
                        wcscpy(lf.lfFaceName, L"Tahoma");

                        hFont = CreateFontIndirectW(&lf);
                        if (hFont)
                        {
                            hFontOld = SelectObject(hdc, hFont);

                            DPtoLP(hdc, (PPOINT)&txtRc, 2);
                            SetTextColor(hdc, RGB(255,255,255));
                            SetBkMode(hdc, TRANSPARENT);
                            DrawTextW(hdc,
                                      szBuffer,
                                      -1,
                                      &txtRc,
                                      DT_BOTTOM | DT_SINGLELINE | DT_NOCLIP | DT_CENTER); //DT_CENTER makes the text visible in RTL layouts...
                            SelectObject(hdc, hFontOld);
                            DeleteObject(hFont);
                        }
                    }

                    txtRc.left = bmpRc.right / 4;
                    txtRc.top = txtRc.bottom - 5;
#ifdef __REACTOS__
                    txtRc.right = bmpRc.right * 4 / 5;
#else
                    txtRc.right = bmpRc.right * 3 / 4;
#endif
                    txtRc.bottom = pInfo->headerbitmap.bmHeight * 9 / 10;

                    if (LoadStringW(hInst,
                                    IDS_HEADERTEXT2,
                                    szBuffer,
                                    sizeof(szBuffer) / sizeof(WCHAR)))
                    {
                        lf.lfHeight = 24;
                        lf.lfCharSet = OEM_CHARSET;
                        lf.lfQuality = DEFAULT_QUALITY;
                        lf.lfWeight = FW_EXTRABOLD;
                        wcscpy(lf.lfFaceName, L"Tahoma");

                        hFont = CreateFontIndirectW(&lf);
                        if (hFont)
                        {
                            hFontOld = SelectObject(hdc, hFont);

                            DPtoLP(hdc, (PPOINT)&txtRc, 2);
                            SetTextColor(hdc, RGB(255,255,255));
                            SetBkMode(hdc, TRANSPARENT);
                            DrawTextW(hdc,
                                      szBuffer,
                                      -1,
                                      &txtRc,
                                      DT_TOP | DT_SINGLELINE);
                            SelectObject(hdc, hFontOld);
                            DeleteObject(hFont);
                        }
                    }

                    DeleteDC(hdcMem);
                }

                EndPaint(hDlg, &ps);
            }

            break;
        }

        case WM_CLOSE:
        {
            Cleanup(pInfo);
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

    return (DialogBoxParamW(hInst,
                            MAKEINTRESOURCEW(IDD_CONNECTDIALOG),
                            NULL,
                            DlgProc,
                            (LPARAM)pRdpSettings) == IDOK);
}
