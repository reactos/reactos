#pragma once

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <winreg.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <limits.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dbt.h>
#include <regstr.h>

#include "resource.h"

/* Hotplug Flags */
#define HOTPLUG_DISPLAY_DEVICE_COMPONENTS 0x00000002

// Globals
extern HINSTANCE hApplet;

// defines
#define NUM_APPLETS    (1)

// global structures
typedef struct
{
    int idIcon;
    int idName;
    int idDescription;
    APPLET_PROC AppletProc;
}APPLET, *PAPPLET;

typedef struct _HOTPLUG_DATA
{
    HICON hIcon;
    HICON hIconSm;
    SP_CLASSIMAGELIST_DATA ImageListData;
    HMENU hPopupMenu;
    HWND hwndDeviceTree;
    DWORD dwFlags;
} HOTPLUG_DATA, *PHOTPLUG_DATA;

// eject.c
DEVINST
GetDeviceInstForRemoval(
    _In_ PHOTPLUG_DATA pHotplugData);

INT_PTR
CALLBACK
ConfirmRemovalDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam);

// enum.c
VOID
EnumHotpluggedDevices(
    _In_ PHOTPLUG_DATA pHotplugData);

VOID
CfmListEnumDevices(
    _In_ HWND hwndCfmDeviceList,
    _In_ PHOTPLUG_DATA pHotplugData);

// hotplug.c
LONG
APIENTRY
InitApplet(
    HWND hwnd,
    UINT uMsg,
    LPARAM wParam,
    LPARAM lParam);
