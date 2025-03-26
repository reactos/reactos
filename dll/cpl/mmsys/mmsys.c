/*
 *
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/mmsys.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 * UPDATE HISTORY:
 *      2005/11/23  Created
 */

#include "mmsys.h"

#include <winsvc.h>
#include <shlwapi.h>
#include <debug.h>

#include <swenum.h>
#include <newdev.h>
#include <initguid.h>
#include <devguid.h>

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

typedef struct
{
    LPWSTR LabelName;
    LPWSTR DefaultName;
    UINT LocalizedResId;
    LPWSTR FileName;
} EVENT_LABEL_ITEM;

typedef struct
{
    LPWSTR LabelName;
    LPWSTR DefaultName;
    UINT IconId;
} SYSTEM_SCHEME_ITEM;

static EVENT_LABEL_ITEM EventLabels[] =
{
    {
        L"WindowsLogon",
        L"ReactOS Logon",
        IDS_REACTOS_LOGON,
        L"ReactOS_Logon.wav"
    },
    {
        L"WindowsLogoff",
        L"ReactOS Logoff",
        IDS_REACTOS_LOGOFF,
        L"ReactOS_Logoff.wav"
    },
    {
        NULL,
        NULL,
        0,
        NULL
    }
};

static SYSTEM_SCHEME_ITEM SystemSchemes[] =
{
    {
        L".Default",
        L"ReactOS Standard",
        IDS_REACTOS_DEFAULT_SCHEME
    },
    {
        L".None",
        L"No Sounds",
        -1
    },
    {
        NULL,
        NULL
    }
};


HWND WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           HWPAGE_DISPLAYMODE DisplayMode);

typedef BOOL (WINAPI *UpdateDriverForPlugAndPlayDevicesProto)(
    _In_opt_ HWND hwndParent,
    _In_ LPCWSTR HardwareId,
    _In_ LPCWSTR FullInfPath,
    _In_ DWORD InstallFlags,
    _Out_opt_ PBOOL bRebootRequired);

#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesW"
#define NUM_APPLETS    (1)


HINSTANCE hApplet = 0;

/* Applets */
const APPLET Applets[NUM_APPLETS] =
{
    {IDI_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, MmSysApplet},
};


HRESULT WINAPI
DllCanUnloadNow(VOID)
{
    DPRINT1("DllCanUnloadNow() stubs\n");
    return S_OK;
}

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid,
                  REFIID riid,
                  LPVOID *ppv)
{
    DPRINT1("DllGetClassObject() stubs\n");
    return S_OK;
}


VOID WINAPI
ShowDriverSettingsAfterForkW(HWND hwnd,
                             HINSTANCE hInstance,
                             LPWSTR lpszCmd,
                             int nCmdShow)
{
    DPRINT1("ShowDriverSettingsAfterForkW() stubs\n");
}

VOID WINAPI
ShowDriverSettingsAfterForkA(HWND hwnd,
                             HINSTANCE hInstance,
                             LPSTR lpszCmd,
                             int nCmdShow)
{
    DPRINT1("ShowDriverSettingsAfterForkA() stubs\n");
}

VOID WINAPI
ShowDriverSettingsAfterFork(HWND hwnd,
                            HINSTANCE hInstance,
                            LPSTR lpszCmd,
                            int nCmdShow)
{
    DPRINT1("ShowDriverSettingsAfterFork() stubs\n");
}

BOOL WINAPI
ShowMMCPLPropertySheet(HWND hwnd,
                       LPCSTR pszPropSheet,
                       LPSTR pszName,
                       LPSTR pszCaption)
{
    DPRINT1("ShowMMCPLPropertySheet() stubs\n");
    return TRUE;
}

VOID WINAPI
mmseRunOnceW(HWND hwnd,
             HINSTANCE hInstance,
             LPWSTR lpszCmd,
             int nCmdShow)
{
    DPRINT1("mmseRunOnceW() stubs\n");
}

VOID WINAPI
mmseRunOnceA(HWND hwnd,
             HINSTANCE hInstance,
             LPSTR lpszCmd,
             int nCmdShow)
{
    DPRINT1("mmseRunOnceA() stubs\n");
}

VOID WINAPI
mmseRunOnce(HWND hwnd,
            HINSTANCE hInstance,
            LPSTR lpszCmd,
            int nCmdShow)
{
    DPRINT1("mmseRunOnce() stubs\n");
}

BOOL WINAPI
MediaPropPageProvider(LPVOID Info,
                      LPFNADDPROPSHEETPAGE PropSheetPage,
                      LPARAM lParam)
{
    DPRINT1("MediaPropPageProvider() stubs\n");
    return TRUE;
}

VOID
InstallSystemSoundLabels(HKEY hKey)
{
    UINT i = 0;
    HKEY hSubKey;
    WCHAR Buffer[40];

    do
    {
        if (RegCreateKeyExW(hKey, EventLabels[i].LabelName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
        {
            RegSetValueExW(hSubKey, NULL, 0, REG_SZ, (LPBYTE)EventLabels[i].DefaultName, (wcslen(EventLabels[i].DefaultName) + 1) * sizeof(WCHAR));
            StringCchPrintfW(Buffer, _countof(Buffer), L"@mmsys.cpl,-%u", EventLabels[i].LocalizedResId);
            RegSetValueExW(hSubKey, L"DispFileName", 0, REG_SZ, (LPBYTE)Buffer, (wcslen(Buffer) + 1) * sizeof(WCHAR));

            RegCloseKey(hSubKey);
        }
        i++;
    } while (EventLabels[i].LabelName);
}

VOID
InstallSystemSoundSchemeNames(HKEY hKey)
{
    UINT i = 0;
    HKEY hSubKey;

    do
    {
        if (RegCreateKeyExW(hKey, SystemSchemes[i].LabelName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
        {
            RegSetValueExW(hSubKey, NULL, 0, REG_SZ, (LPBYTE)SystemSchemes[i].DefaultName, (wcslen(SystemSchemes[i].DefaultName) + 1) * sizeof(WCHAR));
            RegCloseKey(hSubKey);
        }
        i++;
    } while (SystemSchemes[i].LabelName);
}

VOID
InstallDefaultSystemSoundScheme(HKEY hRootKey)
{
    HKEY hKey, hSubKey;
    WCHAR Path[MAX_PATH];
    UINT i = 0;

    if (RegCreateKeyExW(hRootKey, L".Default", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;

    RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE)SystemSchemes[0].DefaultName, (wcslen(SystemSchemes[0].DefaultName) + 1) * sizeof(WCHAR));
    StringCchPrintfW(Path, _countof(Path), L"@mmsys.cpl,-%u", SystemSchemes[0].IconId);
    RegSetValueExW(hKey, L"DispFileName", 0, REG_SZ, (LPBYTE)Path, (wcslen(Path) + 1) * sizeof(WCHAR));

    do
    {
        if (RegCreateKeyExW(hKey, EventLabels[i].LabelName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
        {
            HKEY hScheme;

            StringCchPrintfW(Path, _countof(Path), L"%%SystemRoot%%\\media\\%s", EventLabels[i].FileName);
            if (RegCreateKeyExW(hSubKey, L".Current", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hScheme, NULL) == ERROR_SUCCESS)
            {
                RegSetValueExW(hScheme, NULL, 0, REG_EXPAND_SZ, (LPBYTE)Path, (wcslen(Path) + 1) * sizeof(WCHAR));
                RegCloseKey(hScheme);
            }

            if (RegCreateKeyExW(hSubKey, L".Default", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hScheme, NULL) == ERROR_SUCCESS)
            {
                RegSetValueExW(hScheme, NULL, 0, REG_EXPAND_SZ, (LPBYTE)Path, (wcslen(Path) + 1) * sizeof(WCHAR));
                RegCloseKey(hScheme);
            }
            RegCloseKey(hSubKey);
        }
        i++;
    } while (EventLabels[i].LabelName);

    RegCloseKey(hKey);
}


VOID
InstallSystemSoundScheme(VOID)
{
    HKEY hKey, hSubKey;
    DWORD dwDisposition;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"AppEvents", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;

    if (RegCreateKeyExW(hKey, L"EventLabels", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, NULL) == ERROR_SUCCESS)
    {
        InstallSystemSoundLabels(hSubKey);
        RegCloseKey(hSubKey);
    }

    if (RegCreateKeyExW(hKey, L"Schemes", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, &dwDisposition) == ERROR_SUCCESS)
    {
        HKEY hNames;

        if (RegCreateKeyExW(hSubKey, L"Names", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hNames, NULL) == ERROR_SUCCESS)
        {
            InstallSystemSoundSchemeNames(hNames);
            RegCloseKey(hNames);
        }

        if (RegCreateKeyExW(hSubKey, L"Apps", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hNames, NULL) == ERROR_SUCCESS)
        {
            InstallDefaultSystemSoundScheme(hNames);
            RegCloseKey(hNames);
            if (dwDisposition & REG_CREATED_NEW_KEY)
            {
                // FIXME
                RegSetValueExW(hSubKey, NULL, 0, REG_SZ, (LPBYTE)L".Default", sizeof(L".Default"));
            }
        }

        RegCloseKey(hSubKey);
    }

    RegCloseKey(hKey);
}

BOOL
IsSoftwareBusPnpEnumeratorInstalled()
{
    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    GUID SWBusGuid = {STATIC_BUSID_SoftwareDeviceEnumerator};
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData;

    hDevInfo = SetupDiGetClassDevsW(&SWBusGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (!hDevInfo)
    {
        // failed
        return FALSE;
    }

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &SWBusGuid, 0, &DeviceInterfaceData))
    {
        // failed
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return FALSE;
    }

    DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(), 0, (MAX_PATH * sizeof(WCHAR)) + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W));
    if (!DeviceInterfaceDetailData)
    {
        // failed
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return FALSE;
    }

    DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    if (!SetupDiGetDeviceInterfaceDetailW(hDevInfo, &DeviceInterfaceData, DeviceInterfaceDetailData, (MAX_PATH * sizeof(WCHAR)) + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W), NULL, NULL))
    {
        // failed
        HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return FALSE;
    }
    HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return TRUE;
}

DWORD
InstallSoftwareBusPnpEnumerator(LPCWSTR InfPath, LPCWSTR HardwareIdList)
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    WCHAR ClassName[50];
    int Result = 0;
    HMODULE hModule = NULL;
    UpdateDriverForPlugAndPlayDevicesProto UpdateProc;
    BOOL reboot = FALSE;
    DWORD flags = 0;

    if (!SetupDiGetINFClassW(InfPath, &ClassGUID, ClassName, _countof(ClassName), NULL))
    {
        return -1;
    }

    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID, NULL);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfoW(DeviceInfoSet, ClassName, &ClassGUID, NULL, NULL, DICD_GENERATE_ID, &DeviceInfoData))
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return -1;
    }

    if (!SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID, (LPBYTE)HardwareIdList, (wcslen(HardwareIdList) + 1 + 1) * sizeof(WCHAR)))
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return -1;
    }

    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, DeviceInfoSet, &DeviceInfoData))
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return -1;
    }

    if (GetFileAttributesW(InfPath) == INVALID_FILE_ATTRIBUTES)
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return -1;
    }

    flags |= INSTALLFLAG_FORCE;
    hModule = LoadLibraryW(L"newdev.dll");
    if (!hModule)
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return -1;
    }

    UpdateProc = (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(hModule, UPDATEDRIVERFORPLUGANDPLAYDEVICES);
    if (!UpdateProc)
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        FreeLibrary(hModule);
        return -1;
    }

    if (!UpdateProc(NULL, HardwareIdList, InfPath, flags, &reboot))
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        FreeLibrary(hModule);
        return -1;
    }

    FreeLibrary(hModule);
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return Result;
}

DWORD
MMSYS_InstallDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData)
{
    UINT Length;
    WCHAR szBuffer[MAX_PATH];
    HINF hInf;
    PVOID Context;
    BOOL Result;
    SC_HANDLE hSCManager, hService;
    WCHAR WaveName[20];
    HKEY hKey;
    DWORD BufferSize;
    ULONG Index;

    if (!IsEqualIID(&pspDevInfoData->ClassGuid, &GUID_DEVCLASS_SOUND) &&
        !IsEqualIID(&pspDevInfoData->ClassGuid, &GUID_DEVCLASS_MEDIA))
        return ERROR_DI_DO_DEFAULT;

    Length = GetWindowsDirectoryW(szBuffer, _countof(szBuffer));
    if (!Length || Length >= _countof(szBuffer) - CONST_STR_LEN(L"\\inf\\audio.inf"))
    {
        return ERROR_GEN_FAILURE;
    }

    //PathCchAppend(szBuffer, _countof(szBuffer), L"inf\\audio.inf");
    StringCchCatW(szBuffer, _countof(szBuffer), L"\\inf\\audio.inf");

    hInf = SetupOpenInfFileW(szBuffer,
                             NULL,
                             INF_STYLE_WIN4,
                             NULL);

    if (hInf == INVALID_HANDLE_VALUE)
    {
        return ERROR_GEN_FAILURE;
    }

    Context = SetupInitDefaultQueueCallback(NULL);
    if (Context == NULL)
    {
        SetupCloseInfFile(hInf);
        return ERROR_GEN_FAILURE;
    }

    Result = SetupInstallFromInfSectionW(NULL,
                                         hInf,
                                         L"AUDIO_Inst.NT",
                                         SPINST_ALL,
                                         NULL,
                                         NULL,
                                         SP_COPY_NEWER,
                                         SetupDefaultQueueCallbackW,
                                         Context,
                                         NULL,
                                         NULL);

    if (Result)
    {
        Result = SetupInstallServicesFromInfSectionW(hInf,
                                                     L"Audio_Inst.NT.Services",
                                                     0);
    }

    SetupTermDefaultQueueCallback(Context);
    SetupCloseInfFile(hInf);

    if (!IsSoftwareBusPnpEnumeratorInstalled())
    {
        Length = GetWindowsDirectoryW(szBuffer, _countof(szBuffer));
        if (!Length || Length >= _countof(szBuffer) - CONST_STR_LEN(L"\\inf\\machine.inf"))
        {
            return ERROR_GEN_FAILURE;
        }

        //PathCchAppend(szBuffer, _countof(szBuffer), L"inf\\machine.inf");
        StringCchCatW(szBuffer, _countof(szBuffer), L"\\inf\\machine.inf");

        InstallSoftwareBusPnpEnumerator(szBuffer, L"ROOT\\SWENUM\0");
    }

    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        return ERROR_DI_DO_DEFAULT;
    }

    hService = OpenServiceW(hSCManager, L"AudioSrv", SERVICE_ALL_ACCESS);
    if (hService)
    {
        /* Make AudioSrv start automatically */
        ChangeServiceConfigW(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        StartServiceW(hService, 0, NULL);
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCManager);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        Length = GetSystemDirectoryW(szBuffer, _countof(szBuffer));
        if (!Length || Length >= _countof(szBuffer) - CONST_STR_LEN(L"\\wdmaud.drv"))
        {
            RegCloseKey(hKey);
            return ERROR_DI_DO_DEFAULT;
        }

        //PathCchAppend(szBuffer, _countof(szBuffer), L"wdmaud.drv");
        StringCchCatW(szBuffer, _countof(szBuffer), L"\\wdmaud.drv");

        for (Index = 1; Index <= 4; Index++)
        {
            StringCchPrintfW(WaveName, _countof(WaveName), L"wave%u", Index);
            if (RegQueryValueExW(hKey, WaveName, 0, NULL, NULL, &BufferSize) != ERROR_MORE_DATA)
            {
                /* Store new audio driver entry */
                RegSetValueExW(hKey, WaveName, 0, REG_SZ, (LPBYTE)szBuffer, (wcslen(szBuffer) + 1) * sizeof(WCHAR));
                break;
            }
            else
            {
                WCHAR Buffer[MAX_PATH];
                BufferSize = sizeof(Buffer);

                if (RegQueryValueExW(hKey, WaveName, 0, NULL, (LPBYTE)Buffer, &BufferSize) == ERROR_SUCCESS)
                {
                    /* Make sure the buffer is zero terminated */
                    Buffer[_countof(Buffer) - 1] = UNICODE_NULL;

                    if (!_wcsicmp(Buffer, szBuffer))
                    {
                        /* An entry already exists */
                        break;
                    }
                }
            }
        }
        RegCloseKey(hKey);
    }
    InstallSystemSoundScheme();

    return ERROR_DI_DO_DEFAULT;

}

DWORD
MMSYS_RemoveDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData)
{
    return ERROR_DI_DO_DEFAULT;
}

DWORD
MMSYS_AllowInstallDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData)
{
    return ERROR_DI_DO_DEFAULT;
}

DWORD
MMSYS_SelectDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData)
{
    return ERROR_DI_DO_DEFAULT;
}

DWORD
MMSYS_DetectDevice(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData)
{
    return ERROR_DI_DO_DEFAULT;
}

DWORD
MMSYS_SelectBestCompatDRV(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pspDevInfoData)
{
    return ERROR_DI_DO_DEFAULT;
}

DWORD WINAPI
MediaClassInstaller(IN DI_FUNCTION diFunction,
                    IN HDEVINFO hDevInfo,
                    IN PSP_DEVINFO_DATA pspDevInfoData OPTIONAL)
{
    switch (diFunction)
    {
        case DIF_INSTALLDEVICE:
            return MMSYS_InstallDevice(hDevInfo, pspDevInfoData);
        case DIF_REMOVE:
            return MMSYS_RemoveDevice(hDevInfo, pspDevInfoData);
        case DIF_ALLOW_INSTALL:
            return MMSYS_AllowInstallDevice(hDevInfo, pspDevInfoData);
        case DIF_SELECTDEVICE:
            return MMSYS_SelectDevice(hDevInfo, pspDevInfoData);
        case DIF_DETECT:
            return MMSYS_DetectDevice(hDevInfo, pspDevInfoData);
        case DIF_SELECTBESTCOMPATDRV:
            return MMSYS_SelectBestCompatDRV(hDevInfo, pspDevInfoData);
        default:
            return ERROR_DI_DO_DEFAULT;
    }
}


/* Hardware property page dialog callback */
static INT_PTR CALLBACK
HardwareDlgProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            GUID Guids[2];
            Guids[0] = GUID_DEVCLASS_CDROM;
            Guids[1] = GUID_DEVCLASS_MEDIA;

            /* Create the hardware page */
            DeviceCreateHardwarePageEx(hwndDlg,
                                       Guids,
                                       _countof(Guids),
                                       HWPD_LARGELIST);
            break;
        }
    }

    return FALSE;
}

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_CPLICON));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

LONG APIENTRY
MmSysApplet(HWND hwnd,
            UINT uMsg,
            LPARAM wParam,
            LPARAM lParam)
{
    PROPSHEETPAGEW psp[5];
    PROPSHEETHEADERW psh; // = { 0 };
    INT nPage = 0;

    UNREFERENCED_PARAMETER(wParam);

    if (uMsg == CPL_STARTWPARMSW && lParam != 0)
        nPage = _wtoi((PWSTR)lParam);

    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_CPLICON);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPLNAME);
    psh.nPages = _countof(psp);
    psh.nStartPage = 0;
    psh.ppsp = psp;
    psh.pfnCallback = PropSheetProc;

    InitPropSheetPage(&psp[0], IDD_VOLUME, VolumeDlgProc);
    InitPropSheetPage(&psp[1], IDD_SOUNDS, SoundsDlgProc);
    InitPropSheetPage(&psp[2], IDD_AUDIO, AudioDlgProc);
    InitPropSheetPage(&psp[3], IDD_VOICE, VoiceDlgProc);
    InitPropSheetPage(&psp[4], IDD_HARDWARE, HardwareDlgProc);

    if (nPage != 0 && nPage <= psh.nPages)
        psh.nStartPage = nPage;

    return (LONG)(PropertySheetW(&psh) != -1);
}

VOID
InitPropSheetPage(PROPSHEETPAGEW *psp,
                  WORD idDlg,
                  DLGPROC DlgProc)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGEW));
    psp->dwSize = sizeof(PROPSHEETPAGEW);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCEW(idDlg);
    psp->pfnDlgProc = DlgProc;
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    UINT i = (UINT)lParam1;

    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            if (i < NUM_APPLETS)
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            else
            {
                return TRUE;
            }
            break;

        case CPL_DBLCLK:
            if (i < NUM_APPLETS)
                Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
            else
                return TRUE;
            break;

        case CPL_STARTWPARMSW:
            if (i < NUM_APPLETS)
                return Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
            break;
    }

    return FALSE;
}

VOID WINAPI
ShowAudioPropertySheet(HWND hwnd,
                       HINSTANCE hInstance,
                       LPWSTR lpszCmd,
                       int nCmdShow)
{
    PROPSHEETPAGEW psp[1];
    PROPSHEETHEADERW psh;

    DPRINT("ShowAudioPropertySheet()\n");

    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_CPLICON);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPLNAME);
    psh.nPages = _countof(psp);
    psh.nStartPage = 0;
    psh.ppsp = psp;
    psh.pfnCallback = PropSheetProc;

    InitPropSheetPage(&psp[0], IDD_AUDIO,AudioDlgProc);

    PropertySheetW(&psh);
}

VOID WINAPI
ShowFullControlPanel(HWND hwnd,
                     HINSTANCE hInstance,
                     LPSTR lpszCmd,
                     int nCmdShow)
{
    PROPSHEETPAGEW psp[5];
    PROPSHEETHEADERW psh;

    DPRINT("ShowFullControlPanel()\n");

    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_CPLICON);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPLNAME);
    psh.nPages = _countof(psp);
    psh.nStartPage = 0;
    psh.ppsp = psp;
    psh.pfnCallback = PropSheetProc;

    InitPropSheetPage(&psp[0], IDD_VOLUME, VolumeDlgProc);
    InitPropSheetPage(&psp[1], IDD_SOUNDS, SoundsDlgProc);
    InitPropSheetPage(&psp[2], IDD_AUDIO, AudioDlgProc);
    InitPropSheetPage(&psp[3], IDD_VOICE, VoiceDlgProc);
    InitPropSheetPage(&psp[4], IDD_HARDWARE, HardwareDlgProc);

    PropertySheetW(&psh);
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hApplet = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
