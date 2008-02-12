/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/adapter.c
 * PURPOSE:         d3d9.dll adapter info functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include "d3d9_common.h"
#include <d3d9.h>
#include <ddraw.h>
#include <strsafe.h>
#include <debug.h>
#include "adapter.h"

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
typedef BOOL (WINAPI *LPFN_DISABLEWOW64FSREDIRECTION) (PVOID*);
typedef BOOL (WINAPI *LPFN_REVERTWOW64FSREDIRECTION) (PVOID);


typedef struct _ADAPTERMONITOR
{
    LPCSTR lpszDeviceName;
    HMONITOR hMonitor;
} ADAPTERMONITOR, *LPADAPTERMONITOR;


static BOOL GetDriverName(LPDISPLAY_DEVICEA pDisplayDevice, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    HKEY hKey;
    BOOL bResult = FALSE;

    if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE, pDisplayDevice->DeviceKey + strlen("\\Registry\\Machine\\"), 0, KEY_QUERY_VALUE, &hKey))
    {
        DWORD DriverNameLength = MAX_DEVICE_IDENTIFIER_STRING - (DWORD)strlen(".dll");
        DWORD Type = 0;

        if (ERROR_SUCCESS == RegQueryValueExA(hKey, "InstalledDisplayDrivers", 0, &Type, (LPBYTE)pIdentifier->Driver, &DriverNameLength))
        {
            pIdentifier->Driver[DriverNameLength] = '\0';
            StringCbCatA(pIdentifier->Driver, MAX_DEVICE_IDENTIFIER_STRING, ".dll");
            bResult = TRUE;
        }

        RegCloseKey(hKey);
    }

    return bResult;
}

static void GetDriverVersion(LPDISPLAY_DEVICEA pDisplayDevice, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    HMODULE hModule;
    LPFN_ISWOW64PROCESS fnIsWow64Process;
    BOOL bIsWow64 = FALSE;
    PVOID OldWow64RedirectValue;
    UINT DriverFileSize;

    hModule = GetModuleHandleA("KERNEL32");
    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(hModule, "IsWow64Process");
    if (fnIsWow64Process)
    {
        fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
        if (bIsWow64)
        {
            LPFN_DISABLEWOW64FSREDIRECTION fnDisableWow64FsRedirection;
            fnDisableWow64FsRedirection = (LPFN_DISABLEWOW64FSREDIRECTION)GetProcAddress(hModule, "Wow64DisableWow64FsRedirection");
            fnDisableWow64FsRedirection(&OldWow64RedirectValue);
        }
    }

    DriverFileSize = GetFileVersionInfoSizeA(pIdentifier->Driver, NULL);
    if (DriverFileSize > 0)
    {
        VS_FIXEDFILEINFO* FixedFileInfo = NULL;
        LPVOID pBlock = LocalAlloc(LMEM_ZEROINIT, DriverFileSize);

        if (TRUE == GetFileVersionInfoA(pIdentifier->Driver, 0, DriverFileSize, pBlock))
        {
            if (TRUE == VerQueryValueA(pBlock, "\\", (LPVOID*)&FixedFileInfo, &DriverFileSize))
            {
                pIdentifier->DriverVersion.HighPart = FixedFileInfo->dwFileVersionMS;
                pIdentifier->DriverVersion.LowPart = FixedFileInfo->dwFileVersionLS;
            }
        }

        LocalFree(pBlock);
    }

    if (bIsWow64)
    {
        LPFN_REVERTWOW64FSREDIRECTION fnRevertWow64FsRedirection;
        fnRevertWow64FsRedirection = (LPFN_REVERTWOW64FSREDIRECTION)GetProcAddress(hModule, "Wow64RevertWow64FsRedirection");
        fnRevertWow64FsRedirection(&OldWow64RedirectValue);
    }
}


static void ParseField(LPCSTR lpszDeviceKey, LPDWORD pField, LPCSTR lpszSubString)
{
    const char* ResultStr;
    ResultStr = strstr(lpszDeviceKey, lpszSubString);
    if (ResultStr != NULL)
    {
        *pField = strtol(ResultStr + strlen(lpszSubString), NULL, 16);
    }
}

static void GetDeviceId(LPCSTR lpszDeviceKey, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    ParseField(lpszDeviceKey, &pIdentifier->VendorId, "VEN_");
    ParseField(lpszDeviceKey, &pIdentifier->DeviceId, "DEV_");
    ParseField(lpszDeviceKey, &pIdentifier->SubSysId, "SUBSYS_");
    ParseField(lpszDeviceKey, &pIdentifier->Revision, "REV_");
}

static void GenerateDeviceIdentifier(D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    DWORD* dwIdentifier = (DWORD*)&pIdentifier->DeviceIdentifier;

    pIdentifier->DeviceIdentifier = CLSID_DirectDraw;

    dwIdentifier[0] ^= pIdentifier->VendorId;
    dwIdentifier[1] ^= pIdentifier->DeviceId;
    dwIdentifier[2] ^= pIdentifier->SubSysId;
    dwIdentifier[3] ^= pIdentifier->Revision;
    dwIdentifier[2] ^= pIdentifier->DriverVersion.LowPart;
    dwIdentifier[3] ^= pIdentifier->DriverVersion.HighPart;
}

BOOL GetAdapterInfo(LPCSTR lpszDeviceName, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    DISPLAY_DEVICEA DisplayDevice;
    DWORD AdapterIndex;
    BOOL FoundDisplayDevice;

    memset(&DisplayDevice, 0, sizeof(DISPLAY_DEVICEA));
    DisplayDevice.cb = sizeof(DISPLAY_DEVICEA);   

    AdapterIndex = 0;
    FoundDisplayDevice = FALSE;
    while (EnumDisplayDevicesA(NULL, AdapterIndex, &DisplayDevice, 0) == TRUE)
    {
        if (_stricmp(lpszDeviceName, DisplayDevice.DeviceName) == 0)
        {
            FoundDisplayDevice = TRUE;
            break;
        }

        ++AdapterIndex;
    }

    /* No matching display device found? */
    if (FALSE == FoundDisplayDevice)
        return FALSE;

    lstrcpynA(pIdentifier->Description, DisplayDevice.DeviceString, MAX_DEVICE_IDENTIFIER_STRING);
    lstrcpynA(pIdentifier->DeviceName, DisplayDevice.DeviceName, CCHDEVICENAME);

    if (GetDriverName(&DisplayDevice, pIdentifier) == TRUE)
        GetDriverVersion(&DisplayDevice, pIdentifier);

    GetDeviceId(DisplayDevice.DeviceID, pIdentifier);

    GenerateDeviceIdentifier(pIdentifier);

    return TRUE;
}



static D3DFORMAT Get16BitD3DFormat(LPCSTR lpszDeviceName)
{
    HDC hDC;
    HBITMAP hBitmap;
    LPBITMAPINFO pBitmapInfo;
    D3DFORMAT Format = D3DFMT_R5G6B5;

    if (NULL == (hDC = CreateDCA(NULL, lpszDeviceName, NULL, NULL)))
    {
        return Format;
    }

    if (NULL == (hBitmap = CreateCompatibleBitmap(hDC, 1, 1)))
    {
        DeleteDC(hDC);
        return Format;
    }

    pBitmapInfo = LocalAlloc(LMEM_ZEROINIT, sizeof(BITMAPINFOHEADER) + 4 * sizeof(RGBQUAD));
    if (NULL == pBitmapInfo)
    {
        DeleteObject(hBitmap);
        DeleteDC(hDC);
        return Format;
    }

    pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    if (GetDIBits(hDC, hBitmap, 0, 0, NULL, pBitmapInfo, DIB_RGB_COLORS) > 0)
    {
        if (pBitmapInfo->bmiHeader.biCompression == BI_BITFIELDS)
        {
            if (GetDIBits(hDC, hBitmap, 0, pBitmapInfo->bmiHeader.biHeight, NULL, pBitmapInfo, DIB_RGB_COLORS) > 0)
            {
                /* Check if the green field is 6 bits long */
                if (*(DWORD*)(&pBitmapInfo->bmiColors[1]) == 0x000003E0)
                {
                    Format = D3DFMT_X1R5G5B5;
                }
            }
        }
    }

    LocalFree(pBitmapInfo);
    DeleteObject(hBitmap);
    DeleteDC(hDC);

    return Format;
}

BOOL GetAdapterMode(LPCSTR lpszDeviceName, D3DDISPLAYMODE* pMode)
{
    DEVMODEA DevMode;
    
    memset(&DevMode, 0, sizeof(DEVMODEA));
    DevMode.dmSize = sizeof(DEVMODEA);
    if (FALSE == EnumDisplaySettingsA(lpszDeviceName, ENUM_CURRENT_SETTINGS, &DevMode))
        return FALSE;

    pMode->Width = DevMode.dmPelsWidth;
    pMode->Height = DevMode.dmPelsHeight;
    pMode->RefreshRate = DevMode.dmDisplayFrequency;
    
    switch (DevMode.dmBitsPerPel)
    {
    case 8:
        pMode->Format = D3DFMT_P8;
        break;

    case 16:
        pMode->Format = Get16BitD3DFormat(lpszDeviceName);
        break;

    case 24:
        pMode->Format = D3DFMT_R8G8B8;
        break;

    case 32:
        pMode->Format = D3DFMT_X8R8G8B8;
        break;

    default:
        pMode->Format = D3DFMT_UNKNOWN;
        break;
    }

    return TRUE;
}



static BOOL CALLBACK AdapterMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    MONITORINFOEXA MonitorInfoEx;
    LPADAPTERMONITOR lpAdapterMonitor = (LPADAPTERMONITOR)dwData;

    memset(&MonitorInfoEx, 0, sizeof(MONITORINFOEXA));
    MonitorInfoEx.cbSize = sizeof(MONITORINFOEXA);

    GetMonitorInfoA(hMonitor, (LPMONITORINFO)&MonitorInfoEx);

    if (_stricmp(lpAdapterMonitor->lpszDeviceName, MonitorInfoEx.szDevice) == 0)
    {
        lpAdapterMonitor->hMonitor = hMonitor;
        return FALSE;
    }

    return TRUE;
}

HMONITOR GetAdapterMonitor(LPCSTR lpszDeviceName)
{
    ADAPTERMONITOR AdapterMonitor;
    AdapterMonitor.lpszDeviceName = lpszDeviceName;
    AdapterMonitor.hMonitor = NULL;

    EnumDisplayMonitors(NULL, NULL, AdapterMonitorEnumProc, (LPARAM)&AdapterMonitor);

    return AdapterMonitor.hMonitor;
}



UINT GetDisplayFormatCount(D3DFORMAT Format, const D3DDISPLAYMODE* pSupportedDisplayModes, UINT NumDisplayModes)
{
    UINT DisplayModeIndex;
    UINT FormatIndex = 0;

    for (DisplayModeIndex = 0; DisplayModeIndex < NumDisplayModes; DisplayModeIndex++)
    {
        if (pSupportedDisplayModes[DisplayModeIndex].Format == Format)
        {
            ++FormatIndex;
        }
    }

    return FormatIndex;
}

const D3DDISPLAYMODE* FindDisplayFormat(D3DFORMAT Format, UINT ModeIndex, const D3DDISPLAYMODE* pSupportedDisplayModes, UINT NumDisplayModes)
{
    UINT DisplayModeIndex;
    UINT FormatIndex = 0;

    for (DisplayModeIndex = 0; DisplayModeIndex < NumDisplayModes; DisplayModeIndex++)
    {
        if (pSupportedDisplayModes[DisplayModeIndex].Format == Format)
        {
            if (ModeIndex == FormatIndex)
                return &pSupportedDisplayModes[DisplayModeIndex];

            ++FormatIndex;
        }
    }

    if (FormatIndex == 0)
    {
        DPRINT1("No modes with the specified format found");
    }
    else if (FormatIndex < ModeIndex)
    {
        DPRINT1("Invalid mode index");
    }

    return NULL;
}
