/*
 * ReactOS Hid User Library
 * Copyright (C) 2004-2005 ReactOS Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * PROJECT:         ReactOS Hid User Library
 * FILE:            lib/hid/hid.c
 * PURPOSE:         ReactOS Hid User Library
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      07/12/2004  Created
 */

#include "precomp.h"
#include <stdarg.h>

#include <winbase.h>

#define NDEBUG
#include <debug.h>
#include "hidp.h"

HINSTANCE hDllInstance;

/* device interface GUID for HIDClass devices */
const GUID HidClassGuid = {0x4D1E55B2, 0xF16F, 0x11CF, {0x88,0xCB,0x00,0x11,0x11,0x00,0x00,0x30}};

PVOID
NTAPI
AllocFunction(
    IN ULONG ItemSize)
{
    return LocalAlloc(LHND, ItemSize);
}

VOID
NTAPI
FreeFunction(
    IN PVOID Item)
{
    LocalFree((HLOCAL)Item);
}

VOID
NTAPI
ZeroFunction(
    IN PVOID Item,
    IN ULONG ItemSize)
{
    memset(Item, 0, ItemSize);
}

VOID
NTAPI
CopyFunction(
    IN PVOID Target,
    IN PVOID Source,
    IN ULONG Length)
{
    memcpy(Target, Source, Length);
}

VOID
__cdecl
DebugFunction(
    IN LPCSTR FormatStr, ...)
{
#if 0
    va_arg list;
    va_start(list, FormatStr);
    vDbgPrintEx(FormatStr, list);
    va_end(list);
#endif
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
{
  switch(dwReason)
  {
    case DLL_PROCESS_ATTACH:
      hDllInstance = hinstDLL;
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}


/*
 * HidD_FlushQueue							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_FlushQueue(IN HANDLE HidDeviceObject)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_FLUSH_QUEUE,
                         NULL, 0,
                         NULL, 0,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_FreePreparsedData						EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_FreePreparsedData(IN PHIDP_PREPARSED_DATA PreparsedData)
{
  return (LocalFree((HLOCAL)PreparsedData) == NULL);
}


/*
 * HidD_GetAttributes							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetAttributes(IN HANDLE HidDeviceObject,
                   OUT PHIDD_ATTRIBUTES Attributes)
{
  HID_COLLECTION_INFORMATION hci;
  DWORD RetLen;

  if(!DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_COLLECTION_INFORMATION,
                                       NULL, 0,
                                       &hci, sizeof(HID_COLLECTION_INFORMATION),
                                       &RetLen, NULL))
  {
    return FALSE;
  }

  /* copy the fields */
  Attributes->Size = sizeof(HIDD_ATTRIBUTES);
  Attributes->VendorID = hci.VendorID;
  Attributes->ProductID = hci.ProductID;
  Attributes->VersionNumber = hci.VersionNumber;

  return TRUE;
}


/*
 * HidD_GetFeature							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetFeature(IN HANDLE HidDeviceObject,
                OUT PVOID ReportBuffer,
                IN ULONG ReportBufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_FEATURE,
                         NULL, 0,
                         ReportBuffer, ReportBufferLength,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_GetHidGuid							EXPORTED
 *
 * @implemented
 */
HIDAPI
VOID WINAPI
HidD_GetHidGuid(OUT LPGUID HidGuid)
{
  *HidGuid = HidClassGuid;
}


/*
 * HidD_GetInputReport							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetInputReport(IN HANDLE HidDeviceObject,
                    IN OUT PVOID ReportBuffer,
                    IN ULONG ReportBufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_INPUT_REPORT,
                         NULL, 0,
                         ReportBuffer, ReportBufferLength,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_GetManufacturerString						EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetManufacturerString(IN HANDLE HidDeviceObject,
                           OUT PVOID Buffer,
                           IN ULONG BufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_MANUFACTURER_STRING,
                         NULL, 0,
                         Buffer, BufferLength,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_GetNumInputBuffers						EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetNumInputBuffers(IN HANDLE HidDeviceObject,
                        OUT PULONG NumberBuffers)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS,
                         NULL, 0,
                         NumberBuffers, sizeof(ULONG),
                         &RetLen, NULL) != 0;
}


/*
 * HidD_GetPhysicalDescriptor						EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetPhysicalDescriptor(IN HANDLE HidDeviceObject,
                           OUT PVOID Buffer,
                           IN ULONG BufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_GET_PHYSICAL_DESCRIPTOR,
                         NULL, 0,
                         Buffer, BufferLength,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_GetPreparsedData						EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetPreparsedData(IN HANDLE HidDeviceObject,
                      OUT PHIDP_PREPARSED_DATA *PreparsedData)
{
  HID_COLLECTION_INFORMATION hci;
  DWORD RetLen;
  BOOLEAN Ret;

  if(PreparsedData == NULL)
  {
    return FALSE;
  }

  if(!DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_COLLECTION_INFORMATION,
                                       NULL, 0,
                                       &hci, sizeof(HID_COLLECTION_INFORMATION),
                                       &RetLen, NULL))
  {
    return FALSE;
  }

  *PreparsedData = LocalAlloc(LHND, hci.DescriptorSize);
  if(*PreparsedData == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  Ret = DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                        NULL, 0,
                        *PreparsedData, hci.DescriptorSize,
                        &RetLen, NULL) != 0;

  if(!Ret)
  {
    /* FIXME - Free the buffer in case we failed to get the descriptor? */
    LocalFree((HLOCAL)*PreparsedData);
  }
#if 0
  else
  {
    /* should we truncate the memory in case RetLen < hci.DescriptorSize? */
  }
#endif

  return Ret;
}


/*
 * HidD_GetProductString						EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetProductString(IN HANDLE HidDeviceObject,
                      OUT PVOID Buffer,
                      IN ULONG BufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_PRODUCT_STRING,
                         NULL, 0,
                         Buffer, BufferLength,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_GetSerialNumberString						EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetSerialNumberString(IN HANDLE HidDeviceObject,
                           OUT PVOID Buffer,
                           IN ULONG BufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_SERIALNUMBER_STRING,
                         NULL, 0,
                         Buffer, BufferLength,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_Hello								EXPORTED
 *
 * Undocumented easter egg function. It fills the buffer with "Hello\n"
 * and returns number of bytes filled in (lstrlen(Buffer) + 1 == 7)
 *
 * Bugs: - doesn't check Buffer for NULL
 *       - always returns 7 even if BufferLength < 7 but doesn't produce a buffer overflow
 *
 * @implemented
 */
HIDAPI
ULONG WINAPI
HidD_Hello(OUT PCHAR Buffer,
           IN ULONG BufferLength)
{
  const CHAR HelloString[] = "Hello\n";

  if(BufferLength > 0)
  {
    memcpy(Buffer, HelloString, min(sizeof(HelloString), BufferLength));
  }

  return sizeof(HelloString);
}


/*
 * HidD_SetFeature							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_SetFeature(IN HANDLE HidDeviceObject,
                IN PVOID ReportBuffer,
                IN ULONG ReportBufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_SET_FEATURE,
                         ReportBuffer, ReportBufferLength,
                         NULL, 0,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_SetNumInputBuffers						EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_SetNumInputBuffers(IN HANDLE HidDeviceObject,
                        IN ULONG NumberBuffers)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS,
                         &NumberBuffers, sizeof(ULONG),
                         NULL, 0,
                         &RetLen, NULL) != 0;
}


/*
 * HidD_SetOutputReport							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_SetOutputReport(IN HANDLE HidDeviceObject,
                     IN PVOID ReportBuffer,
                     IN ULONG ReportBufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_SET_OUTPUT_REPORT,
                         ReportBuffer, ReportBufferLength,
                         NULL, 0,
                         &RetLen, NULL) != 0;
}

/*
 * HidD_GetIndexedString							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetIndexedString(IN HANDLE HidDeviceObject,
                      IN ULONG StringIndex,
                      OUT PVOID Buffer,
                      IN ULONG BufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_INDEXED_STRING,
                         &StringIndex, sizeof(ULONG),
                         Buffer, BufferLength,
                         &RetLen, NULL) != 0;
}

/*
 * HidD_GetMsGenreDescriptor							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetMsGenreDescriptor(IN HANDLE HidDeviceObject,
                          OUT PVOID Buffer,
                          IN ULONG BufferLength)
{
  DWORD RetLen;
  return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_MS_GENRE_DESCRIPTOR,
                         0, 0,
                         Buffer, BufferLength,
                         &RetLen, NULL) != 0;
}

/*
 * HidD_GetConfiguration							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_GetConfiguration(IN HANDLE HidDeviceObject,
                      OUT PHIDD_CONFIGURATION Configuration,
                      IN ULONG ConfigurationLength)
{

  // magic cookie
  Configuration->cookie = (PVOID)HidD_GetConfiguration;

  return DeviceIoControl(HidDeviceObject, IOCTL_HID_GET_DRIVER_CONFIG,
                         0, 0,
                         &Configuration->size, ConfigurationLength - sizeof(ULONG),
                         (PULONG)&Configuration->cookie, NULL) != 0;
}

/*
 * HidD_SetConfiguration							EXPORTED
 *
 * @implemented
 */
HIDAPI
BOOLEAN WINAPI
HidD_SetConfiguration(IN HANDLE HidDeviceObject,
                      IN PHIDD_CONFIGURATION Configuration,
                      IN ULONG ConfigurationLength)
{
    BOOLEAN Ret = FALSE;

    if (Configuration->cookie == (PVOID)HidD_GetConfiguration)
    {
        Ret = DeviceIoControl(HidDeviceObject, IOCTL_HID_SET_DRIVER_CONFIG,
                              0, 0,
                              (PVOID)&Configuration->size, ConfigurationLength - sizeof(ULONG),
                              (PULONG)&Configuration->cookie, NULL) != 0;
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return Ret;
}

/* EOF */
