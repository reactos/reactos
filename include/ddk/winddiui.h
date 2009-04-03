/*
 *  Header for the Device Driver Interface - User Interface library
 *
 *  Copyright 2007 Marcel Partap
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

#ifndef __WINE_WINDDIUI_H
#define __WINE_WINDDIUI_H

#include <ddk/compstui.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_EVENT_INITIALIZE 1
#define DRIVER_EVENT_DELETE 2

#define PRINTER_EVENT_ADD_CONNECTION 1
#define PRINTER_EVENT_DELETE_CONNECTION 2
#define PRINTER_EVENT_INITIALIZE 3
#define PRINTER_EVENT_DELETE 4
#define PRINTER_EVENT_CACHE_REFRESH 5
#define PRINTER_EVENT_CACHE_DELETE 6
#define PRINTER_EVENT_ATTRIBUTES_CHANGED 7

#define PRINTER_EVENT_FLAG_NO_UI 1

BOOL WINAPI DrvDriverEvent(DWORD, DWORD, LPBYTE, LPARAM);
BOOL WINAPI DrvPrinterEvent(LPWSTR, INT, DWORD, LPARAM);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __WINE_WINDDIUI_H */
