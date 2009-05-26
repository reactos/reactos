/* Definitions for printing
 *
 * Copyright 1998 Huw Davies, Andreas Mohr
 *
 * Portions Copyright (c) 1999 Corel Corporation
 *                             (Paul Quinn, Albert Den Haan)
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
#ifndef __WINE_WINSPOOL_H
#define __WINE_WINSPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compatibility header
 */
  
#if !defined(_MSC_VER)
#include_next "winspool.h"
#endif


/* DECLARATIONS */
LONG WINAPI ExtDeviceMode( HWND hWnd, HANDLE hInst, LPDEVMODEA pDevModeOutput,
    LPSTR pDeviceName, LPSTR pPort, LPDEVMODEA pDevModeInput, LPSTR pProfile,
    DWORD fMode);

LPSTR WINAPI StartDocDlgA(HANDLE hPrinter, DOCINFOA *doc);
LPWSTR WINAPI StartDocDlgW(HANDLE hPrinter, DOCINFOW *doc);
#define StartDocDlg WINELIB_NAME_AW(StartDocDlg)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* __WINE_WINSPOOL_H */
