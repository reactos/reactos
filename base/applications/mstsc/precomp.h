#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include "uimain.h"
#include "rdesktop.h"
#include "bsops.h"
#include "orders.h"
#include "resource.h"

//#include <stdio.h>

#ifndef __TODO_MSTSC_H
#define __TODO_MSTSC_H

#define IS_PERSISTENT(id) (id < 8 && g_pstcache_fd[id] > 0)

#define MAXKEY 256
#define MAXVALUE 256
#define NUM_SETTINGS 4
extern LPWSTR lpSettings[];

typedef struct _SETTINGS
{
    WCHAR Key[MAXKEY];
    WCHAR Type; // holds 'i' or 's'
    union {
        INT i;
        WCHAR s[MAXVALUE];
    } Value;
} SETTINGS, *PSETTINGS;

typedef struct _RDPSETTINGS
{
    PSETTINGS pSettings;
    INT NumSettings;
} RDPSETTINGS, *PRDPSETTINGS;

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
    LPWSTR DeviceDescription;
    LPWSTR DeviceName;
    LPWSTR DeviceKey;
    LPWSTR DeviceID;
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
    HICON hMstscSm;
    HICON hMstscLg;
    HICON hLogon;
    HICON hConn;
    HICON hRemote;
    HICON hColor;
    HBITMAP hSpectrum;
    BITMAP bitmap;
} INFO, *PINFO;

BOOL InitRdpSettings(PRDPSETTINGS pRdpSettings);
BOOL OpenRDPConnectDialog(HINSTANCE hInstance, PRDPSETTINGS pRdpSettings);
BOOL LoadRdpSettingsFromFile(PRDPSETTINGS pRdpSettings, LPWSTR lpFile);
BOOL SaveRdpSettingsToFile(LPWSTR lpFile, PRDPSETTINGS pRdpSettings);
INT GetIntegerFromSettings(PRDPSETTINGS pSettings, LPWSTR lpValue);
LPWSTR GetStringFromSettings(PRDPSETTINGS pSettings, LPWSTR lpValue);
BOOL SetIntegerToSettings(PRDPSETTINGS pRdpSettings, LPWSTR lpKey, INT Value);
BOOL SetStringToSettings(PRDPSETTINGS pRdpSettings, LPWSTR lpKey, LPWSTR lpValue);
VOID SaveAllSettings(PINFO pInfo);


#endif /* __TODO_MSTSC_H */
