/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 * 
 * GDI Driver support routines
 * (mostly swiped from Wine)
 * 
 */

#undef WIN32_LEAN_AND_MEAN
#define WIN32_NO_PEHDR

#include <ddk/ntddk.h>
#include <windows.h>
#include <win32k/driver.h>
#include <win32k/misc.h>
#include <wchar.h>
#include <ddk/winddi.h>
#include <ddk/ntapi.h>
#include <rosrtl/string.h>
#include <include/tags.h>

#define NDEBUG
#include <debug.h>

typedef struct _GRAPHICS_DRIVER
{
  PWSTR  Name;
  PGD_ENABLEDRIVER  EnableDriver;
  int  ReferenceCount;
  struct _GRAPHICS_DRIVER  *Next;
} GRAPHICS_DRIVER, *PGRAPHICS_DRIVER;

static PGRAPHICS_DRIVER  DriverList;
static PGRAPHICS_DRIVER  GenericDriver = 0;

BOOL DRIVER_RegisterDriver(LPCWSTR  Name, PGD_ENABLEDRIVER  EnableDriver)
{
  PGRAPHICS_DRIVER  Driver = ExAllocatePoolWithTag(PagedPool, sizeof(*Driver), TAG_DRIVER);
  DPRINT( "DRIVER_RegisterDriver( Name: %S )\n", Name );
  if (!Driver)  return  FALSE;
  Driver->ReferenceCount = 0;
  Driver->EnableDriver = EnableDriver;
  if (Name)
  {
    Driver->Name = ExAllocatePoolWithTag(PagedPool,
                                         (wcslen(Name) + 1) * sizeof(WCHAR),
                                         TAG_DRIVER);
    wcscpy(Driver->Name, Name);
    Driver->Next  = DriverList;
    DriverList = Driver;
    return  TRUE;
  }
  
  if (GenericDriver != NULL)
  {
    ExFreePool(Driver);
    return  FALSE;
  }
  
  GenericDriver = Driver;
  return  TRUE;
}

PGD_ENABLEDRIVER DRIVER_FindDDIDriver(LPCWSTR Name)
{
  static WCHAR DefaultPath[] = L"\\SystemRoot\\System32\\";
  static WCHAR DefaultExtension[] = L".DLL";
  SYSTEM_LOAD_IMAGE GdiDriverInfo;
  GRAPHICS_DRIVER *Driver = DriverList;
  NTSTATUS Status;
  WCHAR *FullName;
  LPCWSTR p;
  BOOL PathSeparatorFound;
  BOOL DotFound;
  UINT Size;

  DotFound = FALSE;  
  PathSeparatorFound = FALSE;
  p = Name;
  while (L'\0' != *p)
  {
    if (L'\\' == *p || L'/' == *p)
    {
      PathSeparatorFound = TRUE;
      DotFound = FALSE;
    }
    else if (L'.' == *p)
    {
      DotFound = TRUE;
    }
    p++;
  }

  Size = (wcslen(Name) + 1) * sizeof(WCHAR);
  if (! PathSeparatorFound)
  {
    Size += sizeof(DefaultPath) - sizeof(WCHAR);
  }
  if (! DotFound)
  {
    Size += sizeof(DefaultExtension) - sizeof(WCHAR);
  }
  FullName = ExAllocatePoolWithTag(PagedPool, Size, TAG_DRIVER);
  if (NULL == FullName)
  {
    DPRINT1("Out of memory\n");
    return NULL;
  }
  if (PathSeparatorFound)
  {
    FullName[0] = L'\0';
  }
  else
  {
    wcscpy(FullName, DefaultPath);
  }
  wcscat(FullName, Name);
  if (! DotFound)
  {
    wcscat(FullName, DefaultExtension);
  }

  /* First see if the driver hasn't already been loaded */
  while (Driver && FullName)
  {
    if (!_wcsicmp( Driver->Name, FullName)) 
    {
      return Driver->EnableDriver;
    }
    Driver = Driver->Next;
  }

  /* If not, then load it */
  RtlInitUnicodeString (&GdiDriverInfo.ModuleName, (LPWSTR)FullName);
  Status = ZwSetSystemInformation (SystemLoadImage, &GdiDriverInfo, sizeof(SYSTEM_LOAD_IMAGE));
  ExFreePool(FullName);
  if (!NT_SUCCESS(Status)) return NULL;

  DRIVER_RegisterDriver( L"DISPLAY", GdiDriverInfo.EntryPoint);
  return (PGD_ENABLEDRIVER)GdiDriverInfo.EntryPoint;
}

#define BEGIN_FUNCTION_MAP() \
  ULONG i; \
  for (i = 0; i < DED->c; i++) \
  { \
    switch(DED->pdrvfn[i].iFunc) \
    {

#define DRIVER_FUNCTION(function) \
      case INDEX_Drv##function: \
        *(PVOID*)&DF->function = DED->pdrvfn[i].pfn; \
        break

#define END_FUNCTION_MAP() \
      default: \
        DPRINT1("Unsupported DDI function 0x%x\n", DED->pdrvfn[i].iFunc); \
        break; \
    } \
  }

BOOL DRIVER_BuildDDIFunctions(PDRVENABLEDATA  DED, 
                               PDRIVER_FUNCTIONS  DF)
{
  BEGIN_FUNCTION_MAP();

    DRIVER_FUNCTION(EnablePDEV);
    DRIVER_FUNCTION(CompletePDEV);
    DRIVER_FUNCTION(DisablePDEV);
    DRIVER_FUNCTION(EnableSurface);
    DRIVER_FUNCTION(DisableSurface);
    DRIVER_FUNCTION(AssertMode);
    DRIVER_FUNCTION(ResetPDEV);
    DRIVER_FUNCTION(CreateDeviceBitmap);
    DRIVER_FUNCTION(DeleteDeviceBitmap);
    DRIVER_FUNCTION(RealizeBrush);
    DRIVER_FUNCTION(DitherColor);
    DRIVER_FUNCTION(StrokePath);
    DRIVER_FUNCTION(FillPath);
    DRIVER_FUNCTION(StrokeAndFillPath);
    DRIVER_FUNCTION(Paint);
    DRIVER_FUNCTION(BitBlt);
    DRIVER_FUNCTION(TransparentBlt);
    DRIVER_FUNCTION(CopyBits);
    DRIVER_FUNCTION(StretchBlt);
    DRIVER_FUNCTION(SetPalette);
    DRIVER_FUNCTION(TextOut);
    DRIVER_FUNCTION(Escape);
    DRIVER_FUNCTION(DrawEscape);
    DRIVER_FUNCTION(QueryFont);
    DRIVER_FUNCTION(QueryFontTree);
    DRIVER_FUNCTION(QueryFontData);
    DRIVER_FUNCTION(SetPointerShape);
    DRIVER_FUNCTION(MovePointer);
    DRIVER_FUNCTION(LineTo);
    DRIVER_FUNCTION(SendPage);
    DRIVER_FUNCTION(StartPage);
    DRIVER_FUNCTION(EndDoc);
    DRIVER_FUNCTION(StartDoc);
    DRIVER_FUNCTION(GetGlyphMode);
    DRIVER_FUNCTION(Synchronize);
    DRIVER_FUNCTION(SaveScreenBits);
    DRIVER_FUNCTION(GetModes);
    DRIVER_FUNCTION(Free);
    DRIVER_FUNCTION(DestroyFont);
    DRIVER_FUNCTION(QueryFontCaps);
    DRIVER_FUNCTION(LoadFontFile);
    DRIVER_FUNCTION(UnloadFontFile);
    DRIVER_FUNCTION(FontManagement);
    DRIVER_FUNCTION(QueryTrueTypeTable);
    DRIVER_FUNCTION(QueryTrueTypeOutline);
    DRIVER_FUNCTION(GetTrueTypeFile);
    DRIVER_FUNCTION(QueryFontFile);
    DRIVER_FUNCTION(QueryAdvanceWidths);
    DRIVER_FUNCTION(SetPixelFormat);
    DRIVER_FUNCTION(DescribePixelFormat);
    DRIVER_FUNCTION(SwapBuffers);
    DRIVER_FUNCTION(StartBanding);
    DRIVER_FUNCTION(NextBand);
    DRIVER_FUNCTION(GetDirectDrawInfo);
    DRIVER_FUNCTION(EnableDirectDraw);
    DRIVER_FUNCTION(DisableDirectDraw);
    DRIVER_FUNCTION(QuerySpoolType);
    DRIVER_FUNCTION(GradientFill);
    DRIVER_FUNCTION(SynchronizeSurface);

  END_FUNCTION_MAP();

  return TRUE;
}

typedef LONG VP_STATUS;
typedef VP_STATUS (STDCALL *PMP_DRIVERENTRY)(PVOID, PVOID);

PFILE_OBJECT DRIVER_FindMPDriver(ULONG DisplayNumber)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR DeviceNameBuffer[20];
  UNICODE_STRING DeviceName;
  IO_STATUS_BLOCK Iosb;
  HANDLE DisplayHandle;
  NTSTATUS Status;
  PFILE_OBJECT VideoFileObject;

  swprintf(DeviceNameBuffer, L"\\??\\DISPLAY%d", DisplayNumber + 1);
  RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
  InitializeObjectAttributes(&ObjectAttributes,
			     &DeviceName,
			     0,
			     NULL,
			     NULL);
  Status = ZwOpenFile(&DisplayHandle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      &Iosb,
		      0,
		      FILE_SYNCHRONOUS_IO_ALERT);
  if (NT_SUCCESS(Status))
    {
      Status = ObReferenceObjectByHandle(DisplayHandle,
                                         FILE_READ_DATA | FILE_WRITE_DATA,
                                         IoFileObjectType,
                                         KernelMode,
                                         (PVOID *)&VideoFileObject,
                                         NULL);
      ZwClose(DisplayHandle);
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Unable to connect to miniport (Status %lx)\n", Status);
      DPRINT1("Perhaps the miniport wasn't loaded?\n");
      return(NULL);
    }

  return VideoFileObject;
}


BOOL DRIVER_UnregisterDriver(LPCWSTR  Name)
{
  PGRAPHICS_DRIVER  Driver = NULL;
  
  if (Name)
  {
    if (DriverList != NULL)
    {
      if (!_wcsicmp(DriverList->Name, Name))
      {
        Driver = DriverList;
        DriverList = DriverList->Next;
      }
      else
      {
        Driver = DriverList;
        while (Driver->Next && _wcsicmp(Driver->Name, Name))
        {
          Driver = Driver->Next;
        }
      }
    }
  }
  else
  {    
    if (GenericDriver != NULL)
    {
      Driver = GenericDriver;
      GenericDriver = NULL;
    }
  }
  
  if (Driver != NULL)
  {
    ExFreePool(Driver->Name);
    ExFreePool(Driver);
      
    return  TRUE;
  }
  else
  {
    return  FALSE;
  }
}

INT DRIVER_ReferenceDriver (LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
  {
    DPRINT( "Comparing %S to %S\n", Driver->Name, Name );
    if (!_wcsicmp( Driver->Name, Name)) 
    {
      return ++Driver->ReferenceCount;
    }
    Driver = Driver->Next;
  }
  DPRINT( "Driver %S not found to reference, generic count: %d\n", Name, GenericDriver->ReferenceCount );
  assert( GenericDriver != 0 );
  return ++GenericDriver->ReferenceCount;
}

INT DRIVER_UnreferenceDriver (LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
  {
    DPRINT( "Comparing %S to %S\n", Driver->Name, Name );
    if (!_wcsicmp( Driver->Name, Name)) 
    {
      return --Driver->ReferenceCount;
    }
    Driver = Driver->Next;
  }
  DPRINT( "Driver '%S' not found to dereference, generic count: %d\n", Name, GenericDriver->ReferenceCount );
  assert( GenericDriver != 0 );
  return --GenericDriver->ReferenceCount;
}
/* EOF */
