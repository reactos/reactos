/*
 * ReactOS VMware(r) driver installation utility
 * Copyright (C) 2004 ReactOS Team
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
*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VMware(r) driver installation utility
 * FILE:        subsys/system/vmwinst/vmwinst.c
 * PROGRAMMERS: Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *              Klemens Friedl (frik85@hotmail.com)
 */
#include <windows.h>
#include <commctrl.h>
#include <newdev.h>
#include <stdio.h>
#include <string.h>
#include "vmwinst.h"
#include <debug.h>

extern VOID CALLBACK InstallHinfSectionW(HWND hwnd, HINSTANCE ModuleHandle,
                                         PCWSTR CmdLineBuffer, INT nCmdShow);


HINSTANCE hAppInstance;
BOOL StartVMwConfigWizard, DriverFilesFound, ActivateVBE = FALSE, UninstallDriver = FALSE;

static WCHAR DestinationDriversPath[MAX_PATH+1];
static WCHAR CDDrive = L'\0';
static WCHAR PathToVideoDrivers60[MAX_PATH+1] = L"X:\\program files\\VMWare\\VMWare Tools\\Drivers\\video\\2k\\32bit\\";
static WCHAR PathToVideoDrivers55[MAX_PATH+1] = L"X:\\program files\\VMware\\VMware Tools\\Drivers\\video\\winnt2k\\32Bit\\";
static WCHAR PathToVideoDrivers45[MAX_PATH+1] = L"X:\\program files\\VMware\\VMware Tools\\Drivers\\video\\winnt2k\\";
static WCHAR PathToVideoDrivers40[MAX_PATH+1] = L"X:\\video\\winnt2k\\";
static WCHAR DestinationPath[MAX_PATH+1];
static WCHAR *vmx_fb = L"vmx_fb.dll";
static WCHAR *vmx_mode = L"vmx_mode.dll";
static WCHAR *vmx_mode_v6 = L"vmx mode.dll";
static WCHAR *vmx_svga = L"vmx_svga.sys";

static WCHAR *SrcPath = PathToVideoDrivers45;

static HANDLE hInstallationThread = NULL;
static HWND hInstallationNotifyWnd = NULL;
static LONG AbortInstall = 0;
#define WM_INSTABORT        (WM_USER + 2)
#define WM_INSTCOMPLETE     (WM_USER + 3)
#define WM_INSTSTATUSUPDATE (WM_USER + 4)

/* Helper functions */

LONG CALLBACK VectoredExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
    /* we're not running in VMware, just terminate the process */
    ExitProcess(ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION);
    return EXCEPTION_CONTINUE_EXECUTION;
}

BOOL
DetectVMware(int *Version)
{
    int magic, ver;

    magic = 0;
    ver = 0;

    /* Try using a VMware I/O port. If not running in VMware this'll throw an
    exception! */
#ifndef _MSC_VER
    __asm__ __volatile__("inl  %%dx, %%eax"
        : "=a" (ver), "=b" (magic)
        : "0" (0x564d5868), "d" (0x5658), "c" (0xa));
#else
#error PLEASE WRITE THIS IN ASSEMBLY
#endif


    if(magic == 0x564d5868)
    {
        *Version = ver;
        return TRUE;
    }

    return FALSE;
}

/* try to open the file */
static BOOL
FileExists(WCHAR *Path, WCHAR *File)
{
    WCHAR FileName[MAX_PATH + 1];
    HANDLE FileHandle;

    FileName[0] = L'\0';
    wcscat(FileName, Path);
    wcscat(FileName, File);

    FileHandle = CreateFile(FileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(FileHandle == INVALID_HANDLE_VALUE)
    {
        /* If it was a sharing violation the file must already exist */
        return GetLastError() == ERROR_SHARING_VIOLATION;
    }

    if(GetFileSize(FileHandle, NULL) <= 0)
    {
        CloseHandle(FileHandle);
        return FALSE;
    }

    CloseHandle(FileHandle);
    return TRUE;
}

static VOID
CenterWindow(HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent;
    RECT rcWindow;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);

    SetWindowPos(hWnd,
        HWND_TOP,
        ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
        ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
        0,
        0,
        SWP_NOSIZE);
}


/* Find the drive with the inserted VMware cd-rom */
static BOOL
IsVMwareCDInDrive(WCHAR *Drv)
{
    static WCHAR Drive[4] = L"X:\\";
    WCHAR Current;

    *Drv = L'\0';
    for(Current = 'C'; Current <= 'Z'; Current++)
    {
        Drive[0] = Current;
#if CHECKDRIVETYPE
        if(GetDriveType(Drive) == DRIVE_CDROM)
        {
#endif
            PathToVideoDrivers60[0] = Current;
            PathToVideoDrivers55[0] = Current;
            PathToVideoDrivers40[0] = Current;
            PathToVideoDrivers45[0] = Current;
            if(SetCurrentDirectory(PathToVideoDrivers60))
                SrcPath = PathToVideoDrivers60;
            else if(SetCurrentDirectory(PathToVideoDrivers55))
                SrcPath = PathToVideoDrivers55;
            else if(SetCurrentDirectory(PathToVideoDrivers45))
                SrcPath = PathToVideoDrivers45;
            else if(SetCurrentDirectory(PathToVideoDrivers40))
                SrcPath = PathToVideoDrivers40;
            else
            {
                SetCurrentDirectory(DestinationPath);
                continue;
            }

            if(FileExists(SrcPath, vmx_fb) &&
                (FileExists(SrcPath, vmx_mode) || FileExists(SrcPath, vmx_mode_v6)) &&
                FileExists(SrcPath, vmx_svga))
            {
                *Drv = Current;
                return TRUE;
            }
#if CHECKDRIVETYPE
        }
#endif
    }

    return FALSE;
}

static BOOL
LoadResolutionSettings(DWORD *ResX, DWORD *ResY, DWORD *ColDepth)
{
    HKEY hReg;
    DWORD Type, Size = sizeof(DWORD);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\vmx_svga\\Device0",
        0, KEY_QUERY_VALUE, &hReg) != ERROR_SUCCESS)
    {
        DEVMODE CurrentDevMode;

        /* If this key is absent, just get current settings */
        memset(&CurrentDevMode, 0, sizeof(CurrentDevMode));
        CurrentDevMode.dmSize = sizeof(CurrentDevMode);
        if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &CurrentDevMode) == TRUE)
        {
            *ColDepth = CurrentDevMode.dmBitsPerPel;
            *ResX = CurrentDevMode.dmPelsWidth;
            *ResY = CurrentDevMode.dmPelsHeight;

            return TRUE;
        }
    }

    if(RegQueryValueEx(hReg, L"DefaultSettings.BitsPerPel", 0, &Type, (BYTE*)ColDepth, &Size) != ERROR_SUCCESS ||
        Type != REG_DWORD)
    {
        *ColDepth = 8;
        Size = sizeof(DWORD);
    }

    if(RegQueryValueEx(hReg, L"DefaultSettings.XResolution", 0, &Type, (BYTE*)ResX, &Size) != ERROR_SUCCESS ||
        Type != REG_DWORD)
    {
        *ResX = 640;
        Size = sizeof(DWORD);
    }

    if(RegQueryValueEx(hReg, L"DefaultSettings.YResolution", 0, &Type, (BYTE*)ResY, &Size) != ERROR_SUCCESS ||
        Type != REG_DWORD)
    {
        *ResY = 480;
    }

    RegCloseKey(hReg);
    return TRUE;
}

static BOOL
IsVmwSVGAEnabled(VOID)
{
    HKEY hReg;
    DWORD Type, Size, Value;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\vmx_svga",
        0, KEY_QUERY_VALUE, &hReg) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    Size = sizeof(DWORD);
    if(RegQueryValueEx(hReg, L"Start", 0, &Type, (BYTE*)&Value, &Size) != ERROR_SUCCESS ||
        Type != REG_DWORD)
    {
        RegCloseKey(hReg);
        return FALSE;
    }

    RegCloseKey(hReg);
    return (Value == 1);
}



static BOOL
SaveResolutionSettings(DWORD ResX, DWORD ResY, DWORD ColDepth)
{
    HKEY hReg;
    DWORD VFreq = 85;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\vmx_svga\\Device0",
        0, KEY_SET_VALUE, &hReg) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    if(RegSetValueEx(hReg, L"DefaultSettings.BitsPerPel", 0, REG_DWORD, (BYTE*)&ColDepth, sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hReg);
        return FALSE;
    }

    if(RegSetValueEx(hReg, L"DefaultSettings.XResolution", 0, REG_DWORD, (BYTE*)&ResX, sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hReg);
        return FALSE;
    }

    if(RegSetValueEx(hReg, L"DefaultSettings.YResolution", 0, REG_DWORD, (BYTE*)&ResY, sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hReg);
        return FALSE;
    }

    if(RegSetValueEx(hReg, L"DefaultSettings.VRefresh", 0, REG_DWORD, (BYTE*)&VFreq, sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hReg);
        return FALSE;
    }

    RegCloseKey(hReg);
    return TRUE;
}

static BOOL
EnableDriver(WCHAR *Key, BOOL Enable)
{
    DWORD Value;
    HKEY hReg;

    Value = (Enable ? 1 : 4);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0, KEY_SET_VALUE, &hReg) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    if(RegSetValueEx(hReg, L"Start", 0, REG_DWORD, (BYTE*)&Value, sizeof(DWORD)) != ERROR_SUCCESS)
    {
        RegCloseKey(hReg);
        return FALSE;
    }

    RegCloseKey(hReg);
    return TRUE;
}

/* Activate the vmware driver and deactivate the others */
static BOOL
EnableVmwareDriver(BOOL VBE, BOOL VGA, BOOL VMX)
{
    if(!EnableDriver(L"SYSTEM\\CurrentControlSet\\Services\\VBE", VBE))
    {
        return FALSE;
    }
    if(!EnableDriver(L"SYSTEM\\CurrentControlSet\\Services\\vga", VGA))
    {
        return FALSE;
    }
    if(!EnableDriver(L"SYSTEM\\CurrentControlSet\\Services\\vmx_svga", VMX))
    {
        return FALSE;
    }

    return TRUE;
}

/* GUI */


/* Property page dialog callback */
static INT_PTR CALLBACK
PageWelcomeProc(
                HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam
                )
{
    LPNMHDR pnmh = (LPNMHDR)lParam;
    switch(uMsg)
    {
    case WM_NOTIFY:
        {
            HWND hwndControl;

            /* Center the wizard window */
            hwndControl = GetParent(hwndDlg);
            CenterWindow (hwndControl);

            switch(pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                    break;
                }
            case PSN_WIZNEXT:
                {
                    if(DriverFilesFound)
                    {
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_CONFIG);
                        return TRUE;
                    }
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

/* Property page dialog callback */
static INT_PTR CALLBACK
PageInsertDiscProc(
                   HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam
                   )
{
    switch(uMsg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch(pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;
            case PSN_WIZNEXT:
                SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_INSTALLING_VMWARE_TOOLS);
                break;
            }
            break;
        }
    }
    return FALSE;
}

static VOID
InstTerminateInstaller(BOOL Wait)
{
    if(hInstallationThread != NULL)
    {
        if(Wait)
        {
            InterlockedExchange((LONG*)&AbortInstall, 2);
            WaitForSingleObject(hInstallationThread, INFINITE);
        }
        else
        {
            InterlockedExchange((LONG*)&AbortInstall, 1);
        }
    }
}

static DWORD WINAPI
InstInstallationThread(LPVOID lpParameter)
{
    HANDLE hThread;
    WCHAR InfFileName[1024];
    BOOL DriveAvailable;
    int DrivesTested = 0;

    if(AbortInstall != 0) goto done;
    PostMessage(hInstallationNotifyWnd, WM_INSTSTATUSUPDATE, IDS_SEARCHINGFORCDROM, 0);

    while(AbortInstall == 0)
    {
        Sleep(500);
        DriveAvailable = IsVMwareCDInDrive(&CDDrive);
        if(DriveAvailable)
            break;
        if(DrivesTested++ > 20)
        {
            PostMessage(hInstallationNotifyWnd, WM_INSTABORT, IDS_FAILEDTOLOCATEDRIVERS, 0);
            goto cleanup;
        }
    }

    if(AbortInstall != 0) goto done;
    PostMessage(hInstallationNotifyWnd, WM_INSTSTATUSUPDATE, IDS_COPYINGFILES, 0);

    wcscpy(InfFileName, SrcPath);
    wcscat(InfFileName, L"vmx_svga.inf");
    DPRINT1("Calling UpdateDriverForPlugAndPlayDevices()\n");
    if (!UpdateDriverForPlugAndPlayDevices(
        hInstallationNotifyWnd,
        L"PCI\\VEN_15AD&DEV_0405&SUBSYS_040515AD&REV_00",
        InfFileName,
        0,
        NULL))
    {
        AbortInstall = 1;
    }
    else
    {
        AbortInstall = 0;
    }

done:
    switch(AbortInstall)
    {
    case 0:
        SendMessage(hInstallationNotifyWnd, WM_INSTCOMPLETE, 0, 0);
        break;
    case 1:
        SendMessage(hInstallationNotifyWnd, WM_INSTABORT, 0, 0);
        break;
    }

cleanup:
    hThread = InterlockedExchangePointer((PVOID*)&hInstallationThread, 0);
    if(hThread != NULL)
    {
        CloseHandle(hThread);
    }
    return 0;
}

static BOOL
InstStartInstallationThread(HWND hwndNotify)
{
    if(hInstallationThread == NULL)
    {
        DWORD ThreadId;
        hInstallationNotifyWnd = hwndNotify;
        AbortInstall = 0;
        hInstallationThread = CreateThread(NULL,
            0,
            InstInstallationThread,
            NULL,
            CREATE_SUSPENDED,
            &ThreadId);
        if(hInstallationThread == NULL)
        {
            return FALSE;
        }

        ResumeThread(hInstallationThread);
        return TRUE;
    }

    return FALSE;
}

/* Property page dialog callback */
static INT_PTR CALLBACK
PageInstallingProc(
                   HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam
                   )
{
    switch(uMsg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch(pnmh->code)
            {
            case PSN_SETACTIVE:
                SetDlgItemText(hwndDlg, IDC_INSTALLINGSTATUS, L"");
                SendDlgItemMessage(hwndDlg, IDC_INSTALLINGPROGRESS, PBM_SETMARQUEE, TRUE, 50);
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                InstStartInstallationThread(hwndDlg);
                break;
            case PSN_RESET:
                InstTerminateInstaller(TRUE);
                break;
            case PSN_WIZBACK:
                if(hInstallationThread != NULL)
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
                    InstTerminateInstaller(FALSE);
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, -1);
                    return -1;
                }
                else
                {
                    SendDlgItemMessage(hwndDlg, IDC_INSTALLINGPROGRESS, PBM_SETMARQUEE, FALSE, 0);
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_INSERT_VMWARE_TOOLS);
                }
                break;
            }
            break;
        }
    case WM_INSTABORT:
        /* go back in case we aborted the installation thread */
        SendDlgItemMessage(hwndDlg, IDC_INSTALLINGPROGRESS, PBM_SETMARQUEE, FALSE, 0);
        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_INSERT_VMWARE_TOOLS);
        if(wParam != 0)
        {
            WCHAR Msg[1024];
            LoadString(hAppInstance, wParam, Msg, sizeof(Msg) / sizeof(WCHAR));
            MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
        }
        break;
    case WM_INSTCOMPLETE:
        SendDlgItemMessage(hwndDlg, IDC_INSTALLINGPROGRESS, PBM_SETMARQUEE, FALSE, 0);
        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_CONFIG);
        break;
    case WM_INSTSTATUSUPDATE:
        {
            WCHAR Msg[1024];
            LoadString(hAppInstance, wParam, Msg, sizeof(Msg) / sizeof(WCHAR));
            SetDlgItemText(hwndDlg, IDC_INSTALLINGSTATUS, Msg);
            break;
        }
    }
    return FALSE;
}

/* Property page dialog callback */
static INT_PTR CALLBACK
PageInstallFailedProc(
                      HWND hwndDlg,
                      UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam
                      )
{
    switch(uMsg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch(pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
                break;
            }
            break;
        }
    }
    return FALSE;
}

static void
FillComboBox(HWND Dlg, int idComboBox, int From, int To)
{
    int i;
    WCHAR Text[256];

    for(i = From; i <= To; i++)
    {
        if(LoadString(hAppInstance, i, Text, 255) > 0)
        {
            SendDlgItemMessage(Dlg, idComboBox, CB_ADDSTRING, 0, (LPARAM)Text);
        }
    }
}

typedef struct
{
    int ControlID;
    int ResX;
    int ResY;
} MAPCTLRES;

/* Property page dialog callback */
static INT_PTR CALLBACK
PageConfigProc(
               HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam
               )
{
    LPNMHDR pnmh = (LPNMHDR)lParam;
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            DWORD ResX = 0, ResY = 0, ColDepth = 0;
            int cbSel;

            FillComboBox(hwndDlg, IDC_COLORQUALITY, 10001, 10003);
            if(LoadResolutionSettings(&ResX, &ResY, &ColDepth))
            {
                SendDlgItemMessage(hwndDlg, ResX + ResY, BM_SETCHECK, BST_CHECKED, 0);
                switch(ColDepth)
                {
                case 8:
                    cbSel = 0;
                    break;
                case 16:
                    cbSel = 1;
                    break;
                case 32:
                    cbSel = 2;
                    break;
                default:
                    cbSel = -1;
                    break;
                }
                SendDlgItemMessage(hwndDlg, IDC_COLORQUALITY, CB_SETCURSEL, cbSel, 0);
            }
            break;
        }
    case WM_NOTIFY:
        {
            HWND hwndControl;

            /* Center the wizard window */
            hwndControl = GetParent(hwndDlg);
            CenterWindow (hwndControl);

            switch(pnmh->code)
            {
            case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), ((StartVMwConfigWizard || DriverFilesFound) ? PSWIZB_FINISH | PSWIZB_BACK : PSWIZB_FINISH));
                    break;
                }
            case PSN_WIZBACK:
                {
                    if(StartVMwConfigWizard)
                    {
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_CHOOSEACTION);
                        return TRUE;
                    }
                    if(DriverFilesFound)
                    {
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_WELCOMEPAGE);
                        return TRUE;
                    }
                    break;
                }
            case PSN_WIZFINISH:
                {
                    DWORD rx = 800, ry = 600, cd = 32;
                    int i;
                    static MAPCTLRES Resolutions[11] = {
                        {540, 640, 480},
                        {1400, 800, 600},
                        {1792, 1024, 768},
                        {2016, 1152, 864},
                        {2240, 1280, 960},
                        {2304, 1280, 1024},
                        {2450, 1400, 1050},
                        {2800, 1600, 1200},
                        {3136, 1792, 1344},
                        {3248, 1856, 1392},
                        {3360, 1920, 1440}
                    };
                    for(i = 0; i < 11; i++)
                    {
                        if(SendDlgItemMessage(hwndDlg, Resolutions[i].ControlID, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            rx = Resolutions[i].ResX;
                            ry = Resolutions[i].ResY;
                            break;
                        }
                    }

                    switch(SendDlgItemMessage(hwndDlg, IDC_COLORQUALITY, CB_GETCURSEL, 0, 0))
                    {
                    case 0:
                        cd = 8;
                        break;
                    case 1:
                        cd = 16;
                        break;
                    case 2:
                        cd = 32;
                        break;
                    }

                    SaveResolutionSettings(rx, ry, cd);
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

/* Property page dialog callback */
static INT_PTR CALLBACK
PageChooseActionProc(
                     HWND hwndDlg,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam
                     )
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwndDlg, IDC_CONFIGSETTINGS, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch(pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                break;
            case PSN_WIZBACK:
                {
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_CHOOSEACTION);
                    return TRUE;
                }
            case PSN_WIZNEXT:
                {
                    static ULONG SelPage[4] = {IDD_CONFIG, IDD_SELECTDRIVER, IDD_SELECTDRIVER, IDD_CHOOSEACTION};
                    int i;

                    for(i = IDC_CONFIGSETTINGS; i <= IDC_UNINSTALL; i++)
                    {
                        if(SendDlgItemMessage(hwndDlg, i, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            break;
                        }
                    }

                    UninstallDriver = (i == IDC_UNINSTALL);

                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, SelPage[i - IDC_CONFIGSETTINGS]);
                    return TRUE;
                }
            }
            break;
        }
    }
    return FALSE;
}

/* Property page dialog callback */
static INT_PTR CALLBACK
PageSelectDriverProc(
                     HWND hwndDlg,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam
                     )
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwndDlg, IDC_VBE, BM_SETCHECK, BST_CHECKED, 0);
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch(pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(GetParent(hwndDlg), (UninstallDriver ? PSWIZB_NEXT | PSWIZB_BACK : PSWIZB_BACK | PSWIZB_FINISH));
                break;
            case PSN_WIZBACK:
                {
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_CHOOSEACTION);
                    return TRUE;
                }
            case PSN_WIZNEXT:
                {
                    ActivateVBE = (SendDlgItemMessage(hwndDlg, IDC_VBE, BM_GETCHECK, 0, 0) == BST_CHECKED);

                    if(UninstallDriver)
                    {
                        return FALSE;
                    }
                    return TRUE;
                }
            case PSN_WIZFINISH:
                {
                    if(UninstallDriver)
                    {
                        return FALSE;
                    }
                    ActivateVBE = (SendDlgItemMessage(hwndDlg, IDC_VBE, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if(!EnableVmwareDriver(ActivateVBE,
                        TRUE,
                        FALSE))
                    {
                        WCHAR Msg[1024];
                        LoadString(hAppInstance, (ActivateVBE ? IDS_FAILEDTOSELVBEDRIVER : IDS_FAILEDTOSELVGADRIVER), Msg, sizeof(Msg) / sizeof(WCHAR));
                        MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
                        SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_SELECTDRIVER);
                        return TRUE;
                    }
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

static VOID
ShowUninstNotice(HWND Owner)
{
    WCHAR Msg[1024];
    LoadString(hAppInstance, IDS_UNINSTNOTICE, Msg, sizeof(Msg) / sizeof(WCHAR));
    MessageBox(Owner, Msg, NULL, MB_ICONINFORMATION);
}

static INT_PTR CALLBACK
PageDoUninstallProc(
                    HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam
                    )
{
    switch(uMsg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch(pnmh->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
                break;
            case PSN_WIZFINISH:
                {
                    if(UninstallDriver)
                    {
                        if(!EnableVmwareDriver(ActivateVBE,
                            TRUE,
                            FALSE))
                        {
                            WCHAR Msg[1024];
                            LoadString(hAppInstance, (ActivateVBE ? IDS_FAILEDTOSELVBEDRIVER : IDS_FAILEDTOSELVGADRIVER), Msg, sizeof(Msg) / sizeof(WCHAR));
                            MessageBox(GetParent(hwndDlg), Msg, NULL, MB_ICONWARNING);
                            SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, IDD_SELECTDRIVER);
                            return TRUE;
                        }
                        ShowUninstNotice(GetParent(hwndDlg));
                    }
                    return FALSE;
                }
            }
            break;
        }
    }
    return FALSE;
}

static LONG
CreateWizard(VOID)
{
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpsp[8];
    PROPSHEETPAGE psp;
    WCHAR Caption[1024];

    LoadString(hAppInstance, IDS_WIZARD_NAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    /* Create the Welcome page */
    ZeroMemory (&psp, sizeof(PROPSHEETPAGE));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hAppInstance;
    psp.pfnDlgProc = PageWelcomeProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOMEPAGE);
    ahpsp[0] = CreatePropertySheetPage(&psp);

    /* Create the INSERT_VMWARE_TOOLS page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDD_INSERT_VMWARE_TOOLSTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDD_INSERT_VMWARE_TOOLSSUBTITLE);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_INSERT_VMWARE_TOOLS);
    psp.pfnDlgProc = PageInsertDiscProc;
    ahpsp[1] = CreatePropertySheetPage(&psp);

    /* Create the INSTALLING_VMWARE_TOOLS page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDD_INSTALLING_VMWARE_TOOLSTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDD_INSTALLING_VMWARE_TOOLSSUBTITLE);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_INSTALLING_VMWARE_TOOLS);
    psp.pfnDlgProc = PageInstallingProc;
    ahpsp[2] = CreatePropertySheetPage(&psp);

    /* Create the CONFIG page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDD_CONFIGTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDD_CONFIGSUBTITLE);
    psp.pfnDlgProc = PageConfigProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG);
    ahpsp[3] = CreatePropertySheetPage(&psp);

    /* Create the INSTALLATION_FAILED page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDD_INSTALLATION_FAILEDTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDD_INSTALLATION_FAILEDSUBTITLE);
    psp.pfnDlgProc = PageInstallFailedProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_INSTALLATION_FAILED);
    ahpsp[4] = CreatePropertySheetPage(&psp);

    /* Create the CHOOSEACTION page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDD_CHOOSEACTIONTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDD_CHOOSEACTIONSUBTITLE);
    psp.pfnDlgProc = PageChooseActionProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CHOOSEACTION);
    ahpsp[5] = CreatePropertySheetPage(&psp);

    /* Create the SELECTDRIVER page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDD_SELECTDRIVERTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDD_SELECTDRIVERSUBTITLE);
    psp.pfnDlgProc = PageSelectDriverProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SELECTDRIVER);
    ahpsp[6] = CreatePropertySheetPage(&psp);

    /* Create the DOUNINSTALL page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDD_DOUNINSTALLTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDD_DOUNINSTALLSUBTITLE);
    psp.pfnDlgProc = PageDoUninstallProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DOUNINSTALL);
    ahpsp[7] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hAppInstance;
    psh.hwndParent = NULL;
    psh.nPages = 7;
    psh.nStartPage = (StartVMwConfigWizard ? 5 : 0);
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Display the wizard */
    return (LONG)(PropertySheet(&psh) != -1);
}

int WINAPI
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpszCmdLine,
         int nCmdShow)
{

    PVOID ExceptionHandler;
    int Version;
    WCHAR *lc;

    hAppInstance = hInstance;

    /* Setup a vectored exception handler to protect the detection. Don't use SEH
    here so we notice the next time someone removes support for vectored
    exception handling from ros... */
    if (!(ExceptionHandler = AddVectoredExceptionHandler(0,
        VectoredExceptionHandler)))
    {
        return 1;
    }

    if(!DetectVMware(&Version))
    {
        return 1;
    }

    /* unregister the handler */
    RemoveVectoredExceptionHandler(ExceptionHandler);

    lc = DestinationPath;
    lc += GetSystemDirectory(DestinationPath, MAX_PATH) - 1;
    if(lc >= DestinationPath && *lc != L'\\')
    {
        wcscat(DestinationPath, L"\\");
    }
    DestinationDriversPath[0] = L'\0';
    wcscat(DestinationDriversPath, DestinationPath);
    wcscat(DestinationDriversPath, L"drivers\\");

    SetCurrentDirectory(DestinationPath);

    DriverFilesFound = FileExists(DestinationPath, vmx_fb) &&
        FileExists(DestinationPath, vmx_mode) &&
        FileExists(DestinationDriversPath, vmx_svga);

    StartVMwConfigWizard = DriverFilesFound && IsVmwSVGAEnabled();

    /* Show the wizard */
    CreateWizard();

    return 2;
}

