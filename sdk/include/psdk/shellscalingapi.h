/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     API definitions for api-ms-win-shcore-scaling-l1-1-1.dll
 * COPYRIGHT:   Copyright 2025 Carl Bialorucki (carl.bialorucki@reactos.org)
 */

#pragma once

#include <shtypes.h>

typedef enum
{
    MDT_EFFECTIVE_DPI,
    MDT_ANGULAR_DPI,
    MDT_RAW_DPI,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

typedef enum
{
    PROCESS_DPI_UNAWARE,
    PROCESS_SYSTEM_DPI_AWARE,
    PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef enum
{
    DEVICE_PRIMARY,
    DEVICE_IMMERSIVE,
} DISPLAY_DEVICE_TYPE;

typedef enum
{
    SCF_VALUE_NONE,
    SCF_SCALE,
    SCF_PHYSICAL,
} SCALE_CHANGE_FLAGS;

#if (NTDDI_VERSION >= NTDDI_WIN8)
DEVICE_SCALE_FACTOR WINAPI GetScaleFactorForDevice(_In_ DISPLAY_DEVICE_TYPE deviceType);
HRESULT WINAPI RegisterScaleChangeNotifications(_In_ DISPLAY_DEVICE_TYPE displayDevice, _In_ HWND hwndNotify, _In_ UINT uMsgNotify, _Out_ DWORD *pdwCookie);
HRESULT WINAPI RevokeScaleChangeNotifications(_In_ DISPLAY_DEVICE_TYPE displayDevice, _In_ DWORD dwCookie);
#endif // (NTDDI_VERSION >= NTDDI_WIN8)

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
HRESULT WINAPI GetScaleFactorForMonitor(_In_ HMONITOR hMon, _Out_ DEVICE_SCALE_FACTOR *pScale);
HRESULT WINAPI RegisterScaleChangeEvent(_In_ HANDLE hEvent, _Out_ DWORD_PTR *pdwCookie);
HRESULT WINAPI UnregisterScaleChangeEvent(_In_ DWORD_PTR dwCookie);
HRESULT WINAPI SetProcessDpiAwareness(_In_ PROCESS_DPI_AWARENESS value);
HRESULT WINAPI GetProcessDpiAwareness(_In_opt_ HANDLE hprocess, _Out_ PROCESS_DPI_AWARENESS *value);
HRESULT WINAPI GetDpiForMonitor(_In_ HMONITOR hmonitor, _In_ MONITOR_DPI_TYPE dpiType, _Out_ UINT *dpiX, _Out_ UINT *dpiY);
#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)

#if (NTDDI_VERSION >= NTDDI_WIN10)
typedef enum
{
    SHELL_UI_COMPONENT_TASKBARS,
    SHELL_UI_COMPONENT_NOTIFICATIONAREA,
    SHELL_UI_COMPONENT_DESKBAND,
} SHELL_UI_COMPONENT;

UINT WINAPI GetDpiForShellUIComponent(_In_ SHELL_UI_COMPONENT);
#endif // (NTDDI_VERSION >= NTDDI_WIN10)
