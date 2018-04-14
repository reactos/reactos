#ifndef _DESK_H
#define _DESK_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <commdlg.h>
#include <cpl.h>
#include <tchar.h>
#include <setupapi.h>
#include <shlobj.h>
#include <regstr.h>
#include <dll/desk/deskcplx.h>
#include <strsafe.h>
#include <gdiplus.h>

#include "appearance.h"
#include "preview.h"
#include "draw.h"
#include "monslctl.h"

#include "resource.h"

typedef struct _APPLET
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

typedef struct _DIBITMAP
{
    BITMAPINFO       *info;
    BYTE             *bits;
    UINT              width;
    UINT              height;
} DIBITMAP, *PDIBITMAP;

extern HINSTANCE hApplet;

HMENU
LoadPopupMenu(IN HINSTANCE hInstance,
              IN LPCTSTR lpMenuName);

PDIBITMAP DibLoadImage(LPTSTR lpFilename);
VOID DibFreeImage(PDIBITMAP lpBitmap);

INT AllocAndLoadString(LPTSTR *lpTarget,
                       HINSTANCE hInst,
                       UINT uID);

ULONG __cdecl DbgPrint(PCCH Format,...);

/*
 * The values in these macros are dependent on the
 * layout of the monitor image and they must be adjusted
 * if that image is changed.
 */
#define MONITOR_LEFT        20
#define MONITOR_TOP         8
#define MONITOR_RIGHT       140
#define MONITOR_BOTTOM      92

#define MONITOR_WIDTH       (MONITOR_RIGHT-MONITOR_LEFT)
#define MONITOR_HEIGHT      (MONITOR_BOTTOM-MONITOR_TOP)

#define MONITOR_ALPHA       0xFF00FF

#define MAX_DESK_PAGES        32
#define NUM_SPECTRUM_BITMAPS  3

/* This number must match DesktopIcons array size */
#define NUM_DESKTOP_ICONS  4

typedef struct
{
    BOOL bHideClassic;  /* Hide icon in Classic mode */
    BOOL bHideNewStart; /* Hide icon in Modern Start menu mode */
} HIDE_ICON;

typedef struct _DESKTOP_DATA
{
    BOOL bSettingsChanged;
    HIDE_ICON optIcons[NUM_DESKTOP_ICONS];
    BOOL bHideChanged[NUM_DESKTOP_ICONS];

    BOOL bLocalSettingsChanged;
    BOOL bLocalHideIcon[NUM_DESKTOP_ICONS];
    BOOL bLocalHideChanged[NUM_DESKTOP_ICONS];
} DESKTOP_DATA, *PDESKTOP_DATA;

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
    DWORD dmDisplayFrequency;
} SETTINGS_ENTRY, *PSETTINGS_ENTRY;

typedef struct _DISPLAY_DEVICE_ENTRY
{
    struct _DISPLAY_DEVICE_ENTRY *Flink;
    LPTSTR DeviceDescription;
    LPTSTR DeviceName;
    LPTSTR DeviceKey;
    LPTSTR DeviceID;
    DWORD DeviceStateFlags;
    PSETTINGS_ENTRY Settings; /* Sorted by increasing dmPelsHeight, BPP */
    DWORD SettingsCount;
    PRESOLUTION_INFO Resolutions;
    DWORD ResolutionsCount;
    PSETTINGS_ENTRY CurrentSettings; /* Points into Settings list */
    SETTINGS_ENTRY InitialSettings;
} DISPLAY_DEVICE_ENTRY, *PDISPLAY_DEVICE_ENTRY;

typedef struct _GLOBAL_DATA
{
    COLORREF desktop_color;
    LPCWSTR pwszFile;
    LPCWSTR pwszAction;
    HBITMAP hMonitorBitmap;
    LONG bmMonWidth;
    LONG bmMonHeight;
} GLOBAL_DATA, *PGLOBAL_DATA;

extern GLOBAL_DATA g_GlobalData;
extern HWND hCPLWindow;

BOOL
DisplayAdvancedSettings(HWND hWndParent, PDISPLAY_DEVICE_ENTRY DisplayDevice);

IDataObject *
CreateDevSettings(PDISPLAY_DEVICE_ENTRY DisplayDeviceInfo);

HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY,LPCWSTR,UINT,IDataObject*);

INT_PTR CALLBACK
AdvGeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL
SwitchDisplayMode(HWND hwndDlg, PWSTR DeviceName, PSETTINGS_ENTRY seInit, PSETTINGS_ENTRY seNew, OUT PLONG rc);

INT_PTR CALLBACK
DesktopPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

VOID
InitDesktopSettings(PDESKTOP_DATA pData);

BOOL
SaveDesktopSettings(PDESKTOP_DATA pData);

VOID
SetDesktopSettings(PDESKTOP_DATA pData);

LONG
RegLoadMUIStringW(IN HKEY hKey,
                  IN LPCWSTR pszValue  OPTIONAL,
                  OUT LPWSTR pszOutBuf,
                  IN DWORD cbOutBuf,
                  OUT LPDWORD pcbData OPTIONAL,
                  IN DWORD Flags,
                  IN LPCWSTR pszDirectory  OPTIONAL);

#endif /* _DESK_H */
