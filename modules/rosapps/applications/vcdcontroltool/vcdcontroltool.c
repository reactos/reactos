/*
 * PROJECT:     ReactOS Virtual CD Control Tool
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        modules/rosapps/applications/vcdcontroltool/vcdcontroltool.c
 * PURPOSE:     main dialog implementation
 * COPYRIGHT:   Copyright 2018 Pierre Schweitzer
 *
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winsvc.h>
#include <winreg.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wchar.h>
#include <ndk/rtltypes.h>
#include <ndk/rtlfuncs.h>

#include <vcdioctl.h>
#define IOCTL_CDROM_BASE FILE_DEVICE_CD_ROM
#define IOCTL_CDROM_EJECT_MEDIA CTL_CODE(IOCTL_CDROM_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)

#include "resource.h"

HWND hWnd;
HWND hMountWnd;
HWND hDriverWnd;
HINSTANCE hInstance;
/* FIXME: to improve, ugly hack */
WCHAR wMountLetter;

static
HANDLE
OpenMaster(VOID)
{
    /* Just open the device */
    return CreateFile(L"\\\\.\\\\VirtualCdRom", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

static
HANDLE
OpenLetter(WCHAR Letter)
{
    WCHAR Device[255];

    /* Make name */
    wsprintf(Device, L"\\\\.\\%c:", Letter);

    /* And open */
    return CreateFile(Device, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

static
VOID
RefreshDevicesList(WCHAR Letter)
{
    HWND hListView;
    WCHAR szFormat[50];
    WCHAR szText[MAX_PATH + 50];
    WCHAR szImage[MAX_PATH];
    HANDLE hMaster, hLet;
    DWORD BytesRead, i;
    DRIVES_LIST Drives;
    BOOLEAN Res;
    IMAGE_PATH Image;
    LVITEMW lvItem;
    LRESULT lResult;
    INT iSelected;

    /* Get our list view */
    hListView = GetDlgItem(hWnd, IDC_MAINDEVICES);

    /* Purge it */
    SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);

    /* Now, query the driver for all the devices */
    hMaster = OpenMaster();
    if (hMaster != INVALID_HANDLE_VALUE)
    {
        Res = DeviceIoControl(hMaster, IOCTL_VCDROM_ENUMERATE_DRIVES, NULL, 0, &Drives, sizeof(Drives), &BytesRead, NULL);
        CloseHandle(hMaster);

        if (Res)
        {
            /* Loop to add all the devices to the list */
            iSelected = -1;
            for (i = 0; i < Drives.Count; ++i)
            {
                /* We'll query device one by one */
                hLet = OpenLetter(Drives.Drives[i]);
                if (hLet != INVALID_HANDLE_VALUE)
                {
                    /* Get info about the mounted image */
                    Res = DeviceIoControl(hLet, IOCTL_VCDROM_GET_IMAGE_PATH, NULL, 0, &Image, sizeof(Image), &BytesRead, NULL);
                    if (Res)
                    {
                        /* First of all, add our driver letter to the list */
                        ZeroMemory(&lvItem, sizeof(LVITEMW));
                        lvItem.mask = LVIF_TEXT;
                        lvItem.pszText = szText;
                        lvItem.iItem = i;
                        szText[0] = Drives.Drives[i];
                        szText[1] = L':';
                        szText[2] = 0;

                        /* If it worked, we'll complete with the info about the device:
                         * (mounted? which image?)
                         */
                        lResult = SendMessage(hListView, LVM_INSERTITEM, 0, (LPARAM)&lvItem);
                        if (lResult != -1)
                        {
                            /* If it matches arg, that's the letter to select at the end */
                            if (Drives.Drives[i] == Letter)
                            {
                                iSelected = lResult;
                            }

                            /* We'll fill second column with info */
                            lvItem.iSubItem = 1;

                            /* Gather the image path */
                            if (Image.Length != 0)
                            {
                                memcpy(szImage, Image.Path, Image.Length);
                                szImage[(Image.Length / sizeof(WCHAR))] = L'\0';
                            }

                            /* It's not mounted... */
                            if (Image.Mounted == 0)
                            {
                                /* If we don't have an image, set default text instead */
                                if (Image.Length == 0)
                                {
                                    szImage[0] = 0;
                                    LoadString(hInstance, IDS_NONE, szImage, sizeof(szImage) / sizeof(WCHAR));
                                    szImage[(sizeof(szImage) / sizeof(WCHAR)) - 1] = L'\0';
                                }

                                /* Display the last known image */
                                szFormat[0] = 0;
                                LoadString(hInstance, IDS_NOMOUNTED, szFormat, sizeof(szFormat) / sizeof(WCHAR));
                                szFormat[(sizeof(szFormat) / sizeof(WCHAR)) - 1] = L'\0';

                                swprintf(szText, szFormat, szImage);
                                lvItem.pszText = szText;
                            }
                            else
                            {
                                /* Mounted, just display the image path */
                                lvItem.pszText = szImage;
                            }

                            /* Set text */
                            SendMessage(hListView, LVM_SETITEM, lResult, (LPARAM)&lvItem);
                        }
                    }

                    /* Don't leak our device */
                    CloseHandle(hLet);
                }
            }

            /* If we had something to select, then just do it */
            if (iSelected != -1)
            {
                ZeroMemory(&lvItem, sizeof(LVITEMW));

                lvItem.mask = LVIF_STATE;
                lvItem.iItem = iSelected;
                lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
                lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
                SendMessage(hListView, LVM_SETITEMSTATE, iSelected, (LPARAM)&lvItem);
            }
        }
    }
}

VOID
SetServiceState(BOOLEAN Started)
{
    HWND hControl;

    /* If started, disable start button */
    hControl = GetDlgItem(hDriverWnd, IDC_DRIVERSTART);
    EnableWindow(hControl, !Started);

    /* If started, enable stop button */
    hControl = GetDlgItem(hDriverWnd, IDC_DRIVERSTOP);
    EnableWindow(hControl, Started);
}

INT_PTR
QueryDriverInfo(HWND hDlg)
{
    DWORD dwSize;
    SC_HANDLE hMgr, hSvc;
    LPQUERY_SERVICE_CONFIGW pConfig;
    WCHAR szText[2 * MAX_PATH];
    HWND hControl;
    SERVICE_STATUS Status;

    hDriverWnd = hDlg;

    /* Open service manager */
    hMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hMgr != NULL)
    {
        /* Open our service */
        hSvc = OpenService(hMgr, L"Vcdrom", SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
        if (hSvc != NULL)
        {
            /* Probe its config size */
            if (!QueryServiceConfig(hSvc, NULL, 0, &dwSize) &&
                GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                /* And get its config */
                pConfig = HeapAlloc(GetProcessHeap(), 0, dwSize);

                if (QueryServiceConfig(hSvc, pConfig, dwSize, &dwSize))
                {
                    /* Display name & driver */
                    wsprintf(szText, L"%s:\n(%s)", pConfig->lpDisplayName, pConfig->lpBinaryPathName);
                    hControl = GetDlgItem(hDriverWnd, IDC_DRIVERINFO);
                    SendMessage(hControl, WM_SETTEXT, 0, (LPARAM)szText);
                }

                HeapFree(GetProcessHeap(), 0, pConfig);
            }

            /* Get its status */
            if (QueryServiceStatus(hSvc, &Status))
            {
                if (Status.dwCurrentState != SERVICE_RUNNING &&
                    Status.dwCurrentState != SERVICE_START_PENDING)
                {
                    SetServiceState(FALSE);
                }
                else
                {
                    SetServiceState(TRUE);
                }
            }

            CloseServiceHandle(hSvc);
        }

        CloseServiceHandle(hMgr);
    }

    /* FIXME: we don't allow uninstall/install */
    {
        hControl = GetDlgItem(hDriverWnd, IDC_DRIVERINSTALL);
        EnableWindow(hControl, FALSE);
        hControl = GetDlgItem(hDriverWnd, IDC_DRIVERREMOVE);
        EnableWindow(hControl, FALSE);
    }

    /* Display our sub window */
    ShowWindow(hDlg, SW_SHOW);

    return TRUE;
}

static
VOID
StartDriver(VOID)
{
    SC_HANDLE hMgr, hSvc;

    /* Open the SC manager */
    hMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hMgr != NULL)
    {
        /* Open the service matching our driver */
        hSvc = OpenService(hMgr, L"Vcdrom", SERVICE_START);
        if (hSvc != NULL)
        {
            /* Start it */
            /* FIXME: improve */
            StartService(hSvc, 0, NULL);

            CloseServiceHandle(hSvc);

            /* Refresh the list in case there were persistent mounts */
            RefreshDevicesList(0);

            /* Update buttons */
            SetServiceState(TRUE);
        }

        CloseServiceHandle(hMgr);
    }
}

static
VOID
StopDriver(VOID)
{
    SC_HANDLE hMgr, hSvc;
    SERVICE_STATUS Status;

    /* Open the SC manager */
    hMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hMgr != NULL)
    {
        /* Open the service matching our driver */
        hSvc = OpenService(hMgr, L"Vcdrom", SERVICE_STOP);
        if (hSvc != NULL)
        {
            /* Stop it */
            /* FIXME: improve */
            ControlService(hSvc, SERVICE_CONTROL_STOP, &Status);

            CloseServiceHandle(hSvc);

            /* Refresh the list to clear it */
            RefreshDevicesList(0);

            /* Update buttons */
            SetServiceState(FALSE);
        }

        CloseServiceHandle(hMgr);
    }
}

static
INT_PTR
HandleDriverCommand(WPARAM wParam,
                    LPARAM lParam)
{
    WORD Msg;

    /* Dispatch the message for the controls we manage */
    Msg = LOWORD(wParam);
    switch (Msg)
    {
        case IDC_DRIVEROK:
            DestroyWindow(hDriverWnd);
            return TRUE;

        case IDC_DRIVERSTART:
            StartDriver();
            return TRUE;

        case IDC_DRIVERSTOP:
            StopDriver();
            return TRUE;
    }

    return FALSE;
}

static
INT_PTR
CALLBACK
DriverDialogProc(HWND hDlg,
                 UINT Message,
                 WPARAM wParam,
                 LPARAM lParam)
{
    /* Dispatch the message */
    switch (Message)
    {
        case WM_INITDIALOG:
            return QueryDriverInfo(hDlg);

        case WM_COMMAND:
            return HandleDriverCommand(wParam, lParam);

        case WM_CLOSE:
            return DestroyWindow(hDlg);
    }

    return FALSE;
}

static
VOID
DriverControl(VOID)
{
    /* Just create a new window with our driver control dialog */
    CreateDialogParamW(hInstance,
                       MAKEINTRESOURCE(IDD_DRIVERWINDOW),
                       NULL,
                       DriverDialogProc,
                       0);
}

static
INT_PTR
SetMountFileName(HWND hDlg,
                 LPARAM lParam)
{
    HWND hEditText;

    hMountWnd = hDlg;

    /* Set the file name that was passed when creating dialog */
    hEditText = GetDlgItem(hMountWnd, IDC_MOUNTIMAGE);
    SendMessage(hEditText, WM_SETTEXT, 0, lParam);

    /* Show our window */
    ShowWindow(hDlg, SW_SHOW);

    return TRUE;
}

FORCEINLINE
DWORD
Min(DWORD a, DWORD b)
{
    return (a > b ? b : a);
}

static
VOID
PerformMount(VOID)
{
    HWND hControl;
    WCHAR szFileName[MAX_PATH];
    MOUNT_PARAMETERS MountParams;
    UNICODE_STRING NtPathName;
    HANDLE hLet;
    DWORD BytesRead;
    BOOLEAN bPersist, Res;
    WCHAR szKeyName[256];
    HKEY hKey;

    /* Zero our input structure */
    ZeroMemory(&MountParams, sizeof(MOUNT_PARAMETERS));

    /* Do we have to suppress UDF? */
    hControl = GetDlgItem(hMountWnd, IDC_MOUNTUDF);
    if (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        MountParams.Flags |= MOUNT_FLAG_SUPP_UDF;
    }

    /* Do we have to suppress Joliet? */
    hControl = GetDlgItem(hMountWnd, IDC_MOUNTJOLIET);
    if (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        MountParams.Flags |= MOUNT_FLAG_SUPP_JOLIET;
    }

    /* Should the mount be persistent? */
    hControl = GetDlgItem(hMountWnd, IDC_MOUNTPERSIST);
    bPersist = (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED);

    /* Get the file name */
    hControl = GetDlgItem(hMountWnd, IDC_MOUNTIMAGE);
    GetWindowText(hControl, szFileName, sizeof(szFileName) / sizeof(WCHAR));

    /* Get NT path for the driver */
    if (RtlDosPathNameToNtPathName_U(szFileName, &NtPathName, NULL, NULL))
    {
        /* Copy it in the parameter structure */
        wcsncpy(MountParams.Path, NtPathName.Buffer, 255);
        MountParams.Length = Min(NtPathName.Length, 255 * sizeof(WCHAR));
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);

        /* Open the device */
        hLet = OpenLetter(wMountLetter);
        if (hLet != INVALID_HANDLE_VALUE)
        {
            /* And issue the mount IOCTL */
            Res = DeviceIoControl(hLet, IOCTL_VCDROM_MOUNT_IMAGE, &MountParams, sizeof(MountParams), NULL, 0, &BytesRead, NULL);

            CloseHandle(hLet);

            /* Refresh the list so that our mount appears */
            RefreshDevicesList(0);

            /* If mount succeed and has to persistent, write it to registry */
            if (Res && bPersist)
            {
                wsprintf(szKeyName, L"SYSTEM\\CurrentControlSet\\Services\\Vcdrom\\Parameters\\Device%c", wMountLetter);
                if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
                {
                    wcsncpy(szKeyName, MountParams.Path, MountParams.Length);
                    szKeyName[MountParams.Length / sizeof(WCHAR)] = 0;
                    RegSetValueExW(hKey, L"IMAGE", 0, REG_SZ, (BYTE *)szKeyName, MountParams.Length);

                    szKeyName[0] = wMountLetter;
                    szKeyName[1] = L':';
                    szKeyName[2] = 0;
                    RegSetValueExW(hKey, L"DRIVE", 0, REG_SZ, (BYTE *)szKeyName, 3 * sizeof(WCHAR));

                    RegCloseKey(hKey);
                }
            }
        }
    }

    DestroyWindow(hMountWnd);
}

static
INT_PTR
HandleMountCommand(WPARAM wParam,
                   LPARAM lParam)
{
    WORD Msg;

    /* Dispatch the message for the controls we manage */
    Msg = LOWORD(wParam);
    switch (Msg)
    {
        case IDC_MOUNTCANCEL:
            DestroyWindow(hMountWnd);
            return TRUE;

        case IDC_MOUNTOK:
            PerformMount();
            return TRUE;
    }

    return FALSE;
}

static
INT_PTR
CALLBACK
MountDialogProc(HWND hDlg,
                UINT Message,
                WPARAM wParam,
                LPARAM lParam)
{
    /* Dispatch the message */
    switch (Message)
    {
        case WM_INITDIALOG:
            return SetMountFileName(hDlg, lParam);

        case WM_COMMAND:
            return HandleMountCommand(wParam, lParam);

        case WM_CLOSE:
            return DestroyWindow(hDlg);
    }

    return FALSE;
}

static
VOID
AddDrive(VOID)
{
    WCHAR Letter;
    BOOLEAN Res;
    DWORD BytesRead;
    HANDLE hMaster;

    /* Open the driver */
    hMaster = OpenMaster();
    if (hMaster != INVALID_HANDLE_VALUE)
    {
        /* Issue the create IOCTL */
        Res = DeviceIoControl(hMaster, IOCTL_VCDROM_CREATE_DRIVE, NULL, 0, &Letter, sizeof(WCHAR), &BytesRead, NULL);
        CloseHandle(hMaster);

        /* If it failed, reset the drive letter */
        if (!Res)
        {
            Letter = 0;
        }

        /* Refresh devices list. If it succeed, we pass the created drive letter
         * So that, user can directly click on "mount" to mount an image, without
         * needing to select appropriate device.
         */
        RefreshDevicesList(Letter);
    }
}

static
WCHAR
GetSelectedDriveLetter(VOID)
{
    INT iItem;
    HWND hListView;
    LVITEM lvItem;
    WCHAR szText[255];

    /* Get the select device in the list view */
    hListView = GetDlgItem(hWnd, IDC_MAINDEVICES);
    iItem = SendMessage(hListView, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
    /* If there's one... */
    if (iItem != -1)
    {
        ZeroMemory(&lvItem, sizeof(LVITEM));
        lvItem.pszText = szText;
        lvItem.cchTextMax = sizeof(szText) / sizeof(WCHAR);
        szText[0] = 0;

        /* Get the item text, it will be the drive letter */
        SendMessage(hListView, LVM_GETITEMTEXT, iItem, (LPARAM)&lvItem);
        return szText[0];
    }

    /* Nothing selected */
    return 0;
}

static
VOID
MountImage(VOID)
{
    WCHAR szFilter[255];
    WCHAR szFileName[MAX_PATH];
    OPENFILENAMEW ImageOpen;

    /* Get the selected drive letter
     * FIXME: I make it global, because I don't know how to pass
     * it properly to the later involved functions.
     * Feel free to improve (without breaking ;-))
     */
    wMountLetter = GetSelectedDriveLetter();
    /* We can only mount if we have a device */
    if (wMountLetter != 0)
    {
        /* First of all, we need an image to mount */
        ZeroMemory(&ImageOpen, sizeof(OPENFILENAMEW));

        ImageOpen.lStructSize = sizeof(ImageOpen);
        ImageOpen.hwndOwner = NULL;
        ImageOpen.lpstrFilter = szFilter;
        ImageOpen.lpstrFile = szFileName;
        ImageOpen.nMaxFile = MAX_PATH;
        ImageOpen.Flags = OFN_EXPLORER| OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

        /* Get our filter (only supported images) */
        szFileName[0] = 0;
        szFilter[0] = 0;
        LoadString(hInstance, IDS_FILTER, szFilter, sizeof(szFilter) / sizeof(WCHAR));
        szFilter[(sizeof(szFilter) / sizeof(WCHAR)) - 1] = L'\0';

        /* Get the image name */
        if (!GetOpenFileName(&ImageOpen))
        {
            /* The user canceled... */
            return;
        }

        /* Start the mount dialog, so that user can select mount options */
        CreateDialogParamW(hInstance,
                           MAKEINTRESOURCE(IDD_MOUNTWINDOW),
                           NULL,
                           MountDialogProc,
                           (LPARAM)szFileName);
    }
}

static
VOID
RemountImage(VOID)
{
    WCHAR Letter;
    HANDLE hLet;
    DWORD BytesRead;

    /* Get the select drive letter */
    Letter = GetSelectedDriveLetter();
    if (Letter != 0)
    {
        /* Open it */
        hLet = OpenLetter(Letter);
        if (hLet != INVALID_HANDLE_VALUE)
        {
            /* And ask the driver for a remount */
            DeviceIoControl(hLet, IOCTL_STORAGE_LOAD_MEDIA, NULL, 0, NULL, 0, &BytesRead, NULL);

            CloseHandle(hLet);

            /* Refresh the list, to display the fact the image is now mounted.
             * Make sure it's selected as it was previously selected.
             */
            RefreshDevicesList(Letter);
        }
    }
}

static
VOID
EjectDrive(VOID)
{
    WCHAR Letter;
    HANDLE hLet;
    DWORD BytesRead;

    /* Get the select drive letter */
    Letter = GetSelectedDriveLetter();
    if (Letter != 0)
    {
        /* Open it */
        hLet = OpenLetter(Letter);
        if (hLet != INVALID_HANDLE_VALUE)
        {
            /* And ask the driver for an ejection */
            DeviceIoControl(hLet, IOCTL_CDROM_EJECT_MEDIA, NULL, 0, NULL, 0, &BytesRead, NULL);

            CloseHandle(hLet);

            /* Refresh the list, to display the fact the image is now unmounted but
             * still known by the driver
             * Make sure it's selected as it was previously selected.
             */
            RefreshDevicesList(Letter);
        }
    }
}

static
VOID
RemoveDrive(VOID)
{
    WCHAR Letter;
    HANDLE hLet;
    DWORD BytesRead;

    /* Get the select drive letter */
    Letter = GetSelectedDriveLetter();
    if (Letter != 0)
    {
        /* Open it */
        hLet = OpenLetter(Letter);
        if (hLet != INVALID_HANDLE_VALUE)
        {
            /* And ask the driver for a deletion */
            DeviceIoControl(hLet, IOCTL_VCDROM_DELETE_DRIVE, NULL, 0, NULL, 0, &BytesRead, NULL);

            CloseHandle(hLet);

            /* Refresh the list, to make the device disappear */
            RefreshDevicesList(0);
        }
    }
}

static
INT_PTR
HandleCommand(WPARAM wParam,
              LPARAM lParam)
{
    WORD Msg;

    /* Dispatch the message for the controls we manage */
    Msg = LOWORD(wParam);
    switch (Msg)
    {
        case IDC_MAINCONTROL:
            DriverControl();
            return TRUE;

        case IDC_MAINOK:
            DestroyWindow(hWnd);
            return TRUE;

        case IDC_MAINADD:
            AddDrive();
            return TRUE;

        case IDC_MAINMOUNT:
            MountImage();
            return TRUE;

        case IDC_MAINREMOUNT:
            RemountImage();
            return TRUE;

        case IDC_MAINEJECT:
            EjectDrive();
            return TRUE;

        case IDC_MAINREMOVE:
            RemoveDrive();
            return TRUE;
    }

    return FALSE;
}

static
VOID ResetStats(VOID)
{
    HWND hEditText;
    static const WCHAR szText[] = { L'0', 0 };

    /* Simply set '0' in all the edittext controls we
     * manage regarding statistics.
     */
    hEditText = GetDlgItem(hWnd, IDC_MAINSECTORS);
    SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)szText);

    hEditText = GetDlgItem(hWnd, IDC_MAINSIZE);
    SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)szText);

    hEditText = GetDlgItem(hWnd, IDC_MAINFREE);
    SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)szText);

    hEditText = GetDlgItem(hWnd, IDC_MAINTOTAL);
    SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)szText);
}

static
INT_PTR
HandleNotify(LPARAM lParam)
{
    WCHAR Letter;
    LPNMHDR NmHdr;
    WCHAR szText[255];
    HWND hEditText;
    DWORD ClusterSector, SectorSize, FreeClusters, Clusters, Sectors;

    NmHdr = (LPNMHDR)lParam;

    /* We only want notifications on click on our devices list */
    if (NmHdr->code == NM_CLICK &&
        NmHdr->idFrom == IDC_MAINDEVICES)
    {
        /* Get the newly selected device */
        Letter = GetSelectedDriveLetter();
        if (Letter != 0)
        {
            /* Setup its name */
            szText[0] = Letter;
            szText[1] = L':';
            szText[2] = 0;

            /* And get its capacities */
            if (GetDiskFreeSpace(szText, &ClusterSector, &SectorSize, &FreeClusters, &Clusters))
            {
                /* Nota: the application returns the total amount of clusters and sectors
                 * So, compute it
                 */
                Sectors = ClusterSector * Clusters;

                /* And now, update statistics about the device */
                hEditText = GetDlgItem(hWnd, IDC_MAINSECTORS);
                wsprintf(szText, L"%ld", Sectors);
                SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)szText);

                hEditText = GetDlgItem(hWnd, IDC_MAINSIZE);
                wsprintf(szText, L"%ld", SectorSize);
                SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)szText);

                hEditText = GetDlgItem(hWnd, IDC_MAINFREE);
                wsprintf(szText, L"%ld", FreeClusters);
                SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)szText);

                hEditText = GetDlgItem(hWnd, IDC_MAINTOTAL);
                wsprintf(szText, L"%ld", Clusters);
                SendMessage(hEditText, WM_SETTEXT, 0, (LPARAM)szText);

                return TRUE;
            }
        }

        /* We failed somewhere, make sure we're at 0 */
        ResetStats();

        return TRUE;
    }

    return FALSE;
}

static
INT_PTR
CreateListViewColumns(HWND hDlg)
{
    WCHAR szText[255];
    LVCOLUMNW lvColumn;
    HWND hListView;

    hWnd = hDlg;
    hListView = GetDlgItem(hDlg, IDC_MAINDEVICES);

    /* Select the whole line, not just the first column */
    SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    /* Set up the first column */
    ZeroMemory(&lvColumn, sizeof(LVCOLUMNW));
    lvColumn.pszText = szText;
    lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = 100;
    szText[0] = 0;
    LoadString(hInstance, IDS_DRIVE, szText, sizeof(szText) / sizeof(WCHAR));
    szText[(sizeof(szText) / sizeof(WCHAR)) - 1] = L'\0';
    SendMessage(hListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvColumn);

    /* Set up the second column */
    szText[0] = 0;
    lvColumn.cx = 350;
    LoadString(hInstance, IDS_MAPPEDIMAGE, szText, sizeof(szText) / sizeof(WCHAR));
    szText[(sizeof(szText) / sizeof(WCHAR)) - 1] = L'\0';
    SendMessage(hListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvColumn);

    /* Make sure stats are at 0 */
    ResetStats();

    /* And populate our device list */
    RefreshDevicesList(0);

    return TRUE;
}

static
INT_PTR
CALLBACK
MainDialogProc(HWND hDlg,
               UINT Message,
               WPARAM wParam,
               LPARAM lParam)
{
    /* Dispatch the message */
    switch (Message)
    {
        case WM_INITDIALOG:
            return CreateListViewColumns(hDlg);

        case WM_COMMAND:
            return HandleCommand(wParam, lParam);

        case WM_NOTIFY:
            return HandleNotify(lParam);

        case WM_CLOSE:
            return DestroyWindow(hDlg);

        case WM_DESTROY:
            PostQuitMessage(0);
            return TRUE;
    }

    return FALSE;
}

INT
WINAPI
wWinMain(HINSTANCE hInst,
         HINSTANCE hPrev,
         LPWSTR Cmd,
         int iCmd)
{
    MSG Msg;

    hInstance = hInst;

    /* Just start our main window */
    hWnd = CreateDialogParamW(hInst,
                              MAKEINTRESOURCE(IDD_MAINWINDOW),
                              NULL,
                              MainDialogProc,
                              0);
    /* And dispatch messages in case of a success */
    if (hWnd != NULL)
    {
        while (GetMessageW(&Msg, NULL, 0, 0) != 0)
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }
    }

    return 0;
}
