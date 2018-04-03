/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_HIDSDI_H
#define __WINE_HIDSDI_H

#ifndef WINE_NTSTATUS_DECLARED
#define WINE_NTSTATUS_DECLARED
typedef LONG NTSTATUS;
#endif

#include <hidusage.h>
#include <hidpi.h>

BOOLEAN WINAPI HidD_GetFeature(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength);
void WINAPI HidD_GetHidGuid(LPGUID guid);
BOOLEAN WINAPI HidD_GetInputReport(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength);
BOOLEAN WINAPI HidD_GetManufacturerString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength);
BOOLEAN WINAPI HidD_GetNumInputBuffers(HANDLE HidDeviceObject, ULONG *NumberBuffers);
BOOLEAN WINAPI HidD_GetProductString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength);
BOOLEAN WINAPI HidD_GetSerialNumberString(HANDLE HidDeviceObject, PVOID Buffer, ULONG BufferLength);
BOOLEAN WINAPI HidD_SetFeature(HANDLE HidDeviceObject, PVOID ReportBuffer, ULONG ReportBufferLength);
BOOLEAN WINAPI HidD_SetNumInputBuffers(HANDLE HidDeviceObject, ULONG NumberBuffers);
BOOLEAN WINAPI HidD_GetPreparsedData( HANDLE HidDeviceObject, PHIDP_PREPARSED_DATA *PreparsedData);
BOOLEAN WINAPI HidD_FreePreparsedData(PHIDP_PREPARSED_DATA PreparsedData);
BOOLEAN WINAPI HidD_GetAttributes(HANDLE HidDeviceObject, PHIDD_ATTRIBUTES Attr);
BOOLEAN WINAPI HidD_SetOutputReport(HANDLE HidDeviceObject, void *ReportBuffer, ULONG ReportBufferLength);

#endif  /* __WINE_HIDSDI_H */
