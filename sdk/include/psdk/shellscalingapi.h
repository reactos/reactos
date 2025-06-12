/*
 * Copyright 2016 Sebastian Lackner
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SHELLSCALINGAPI_H
#define __WINE_SHELLSCALINGAPI_H

#include <shtypes.h>

typedef enum MONITOR_DPI_TYPE
{
    MDT_EFFECTIVE_DPI   = 0,
    MDT_ANGULAR_DPI     = 1,
    MDT_RAW_DPI         = 2,
    MDT_DEFAULT         = MDT_EFFECTIVE_DPI,
} MONITOR_DPI_TYPE;

typedef enum PROCESS_DPI_AWARENESS
{
    PROCESS_DPI_UNAWARE,
    PROCESS_SYSTEM_DPI_AWARE,
    PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef enum
{
    DEVICE_PRIMARY   = 0,
    DEVICE_IMMERSIVE = 1,
} DISPLAY_DEVICE_TYPE;

HRESULT WINAPI GetDpiForMonitor(HMONITOR,MONITOR_DPI_TYPE,UINT*,UINT*);
HRESULT WINAPI GetProcessDpiAwareness(HANDLE,PROCESS_DPI_AWARENESS*);
DEVICE_SCALE_FACTOR WINAPI GetScaleFactorForDevice(DISPLAY_DEVICE_TYPE device_type);
HRESULT WINAPI GetScaleFactorForMonitor(HMONITOR,DEVICE_SCALE_FACTOR*);
HRESULT WINAPI RegisterScaleChangeNotifications(DISPLAY_DEVICE_TYPE,HWND,UINT,DWORD*);
HRESULT WINAPI SetProcessDpiAwareness(PROCESS_DPI_AWARENESS);

#endif /* __WINE_SHELLSCALINGAPI_H */
