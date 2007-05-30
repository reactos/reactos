/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/hardware.c
 * PURPOSE:     Hardware devices
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

typedef BOOL (STDCALL *PDEVMGREXEC)(HWND hWndParent, HINSTANCE hInst, PVOID Unknown, int nCmdShow);

BOOL LaunchDeviceManager(HWND hWndParent)
{
/* hack for ROS to start our devmgmt until we have mmc */
#ifdef __REACTOS__
    return ((INT)ShellExecuteW(NULL, L"open", L"devmgmt.exe", NULL, NULL, SW_SHOWNORMAL) > 32);
#else
    HMODULE hDll;
    PDEVMGREXEC DevMgrExec;
    BOOL Ret;

    hDll = LoadLibrary(_TEXT("devmgr.dll"));
    if(!hDll)
        return FALSE;

    DevMgrExec = (PDEVMGREXEC)GetProcAddress(hDll, "DeviceManager_ExecuteW");
    if(!DevMgrExec)
    {
        FreeLibrary(hDll);
        return FALSE;
    }

    /* run the Device Manager */
    Ret = DevMgrExec(hWndParent, hApplet, NULL /* ??? */, SW_SHOW);
    FreeLibrary(hDll);
    return Ret;
#endif /* __REACTOS__ */
}

/* Property page dialog callback */
INT_PTR CALLBACK
HardwarePageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_HARDWARE_DEVICE_MANAGER:
                {
                    if(!LaunchDeviceManager(hwndDlg))
                    {
                        /* FIXME */
                    }

                    return TRUE;
                }

                case IDC_HARDWARE_PROFILE:
                {
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_HARDWAREPROFILES),
                              hwndDlg,
                              (DLGPROC)HardProfDlgProc);

                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

