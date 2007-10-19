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

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* #define TRACE_DRV_CALLS to get a log of all calls into the display driver. */
#undef TRACE_DRV_CALLS

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
    if (Driver->Name == NULL)
    {
        DPRINT1("Out of memory\n");
        ExFreePool(Driver);
        return  FALSE;
    }

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

PGD_ENABLEDRIVER DRIVER_FindExistingDDIDriver(LPCWSTR Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  while (Driver && Name)
  {
    if (!_wcsicmp(Driver->Name, Name))
    {
      return Driver->EnableDriver;
    }
    Driver = Driver->Next;
  }

  return NULL;
}

PGD_ENABLEDRIVER DRIVER_FindDDIDriver(LPCWSTR Name)
{
  static WCHAR DefaultPath[] = L"\\SystemRoot\\System32\\";
  static WCHAR DefaultExtension[] = L".DLL";
  SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
  GRAPHICS_DRIVER *Driver = DriverList;
  NTSTATUS Status;
  LPWSTR FullName;
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
  RtlInitUnicodeString (&GdiDriverInfo.DriverName, FullName);
  Status = ZwSetSystemInformation (SystemLoadGdiDriverInformation, &GdiDriverInfo, sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
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

#define END_FUNCTION_MAP() \
      default: \
        DPRINT1("Unsupported DDI function 0x%x\n", DED->pdrvfn[i].iFunc); \
        break; \
    } \
  }

#ifdef TRACE_DRV_CALLS

typedef struct _TRACEDRVINFO
  {
  unsigned Index;
  char *Name;
  PVOID DrvRoutine;
  }
TRACEDRVINFO, *PTRACEDRVINFO;

__asm__(
" .text\n"
"TraceDrv:\n"
" pushl %eax\n"
" call  _FindTraceInfo\n"
" add   $4,%esp\n"
" pushl %eax\n"
" pushl 4(%eax)\n"
" call  _DbgPrint\n"
" addl  $4,%esp\n"
" popl  %eax\n"
" mov   8(%eax),%eax\n"
" jmp   *%eax\n"
);

#define TRACEDRV_ROUTINE(function) \
unsigned TraceDrvIndex##function = INDEX_Drv##function; \
__asm__ ( \
" .text\n" \
"_Trace" #function ":\n" \
" movl _TraceDrvIndex" #function ",%eax\n" \
" jmp TraceDrv\n" \
); \
extern PVOID Trace##function;

TRACEDRV_ROUTINE(EnablePDEV)
TRACEDRV_ROUTINE(CompletePDEV)
TRACEDRV_ROUTINE(DisablePDEV)
TRACEDRV_ROUTINE(EnableSurface)
TRACEDRV_ROUTINE(DisableSurface)
TRACEDRV_ROUTINE(AssertMode)
TRACEDRV_ROUTINE(Offset)
TRACEDRV_ROUTINE(ResetPDEV)
TRACEDRV_ROUTINE(DisableDriver)
TRACEDRV_ROUTINE(CreateDeviceBitmap)
TRACEDRV_ROUTINE(DeleteDeviceBitmap)
TRACEDRV_ROUTINE(RealizeBrush)
TRACEDRV_ROUTINE(DitherColor)
TRACEDRV_ROUTINE(StrokePath)
TRACEDRV_ROUTINE(FillPath)
TRACEDRV_ROUTINE(StrokeAndFillPath)
TRACEDRV_ROUTINE(Paint)
TRACEDRV_ROUTINE(BitBlt)
TRACEDRV_ROUTINE(TransparentBlt)
TRACEDRV_ROUTINE(CopyBits)
TRACEDRV_ROUTINE(StretchBlt)
TRACEDRV_ROUTINE(StretchBltROP)
TRACEDRV_ROUTINE(SetPalette)
TRACEDRV_ROUTINE(TextOut)
TRACEDRV_ROUTINE(Escape)
TRACEDRV_ROUTINE(DrawEscape)
TRACEDRV_ROUTINE(QueryFont)
TRACEDRV_ROUTINE(QueryFontTree)
TRACEDRV_ROUTINE(QueryFontData)
TRACEDRV_ROUTINE(SetPointerShape)
TRACEDRV_ROUTINE(MovePointer)
TRACEDRV_ROUTINE(LineTo)
TRACEDRV_ROUTINE(SendPage)
TRACEDRV_ROUTINE(StartPage)
TRACEDRV_ROUTINE(EndDoc)
TRACEDRV_ROUTINE(StartDoc)
TRACEDRV_ROUTINE(GetGlyphMode)
TRACEDRV_ROUTINE(Synchronize)
TRACEDRV_ROUTINE(SaveScreenBits)
TRACEDRV_ROUTINE(GetModes)
TRACEDRV_ROUTINE(Free)
TRACEDRV_ROUTINE(DestroyFont)
TRACEDRV_ROUTINE(QueryFontCaps)
TRACEDRV_ROUTINE(LoadFontFile)
TRACEDRV_ROUTINE(UnloadFontFile)
TRACEDRV_ROUTINE(FontManagement)
TRACEDRV_ROUTINE(QueryTrueTypeTable)
TRACEDRV_ROUTINE(QueryTrueTypeOutline)
TRACEDRV_ROUTINE(GetTrueTypeFile)
TRACEDRV_ROUTINE(QueryFontFile)
TRACEDRV_ROUTINE(QueryAdvanceWidths)
TRACEDRV_ROUTINE(SetPixelFormat)
TRACEDRV_ROUTINE(DescribePixelFormat)
TRACEDRV_ROUTINE(SwapBuffers)
TRACEDRV_ROUTINE(StartBanding)
TRACEDRV_ROUTINE(NextBand)
TRACEDRV_ROUTINE(GetDirectDrawInfo)
TRACEDRV_ROUTINE(EnableDirectDraw)
TRACEDRV_ROUTINE(DisableDirectDraw)
TRACEDRV_ROUTINE(QuerySpoolType)
TRACEDRV_ROUTINE(GradientFill)
TRACEDRV_ROUTINE(SynchronizeSurface)
TRACEDRV_ROUTINE(AlphaBlend)

#define TRACEDRVINFO_ENTRY(function) \
    { INDEX_Drv##function, "Drv" #function "\n", NULL }
static TRACEDRVINFO TraceDrvInfo[] =
  {
    TRACEDRVINFO_ENTRY(EnablePDEV),
    TRACEDRVINFO_ENTRY(CompletePDEV),
    TRACEDRVINFO_ENTRY(DisablePDEV),
    TRACEDRVINFO_ENTRY(EnableSurface),
    TRACEDRVINFO_ENTRY(DisableSurface),
    TRACEDRVINFO_ENTRY(AssertMode),
    TRACEDRVINFO_ENTRY(Offset),
    TRACEDRVINFO_ENTRY(ResetPDEV),
    TRACEDRVINFO_ENTRY(DisableDriver),
    TRACEDRVINFO_ENTRY(CreateDeviceBitmap),
    TRACEDRVINFO_ENTRY(DeleteDeviceBitmap),
    TRACEDRVINFO_ENTRY(RealizeBrush),
    TRACEDRVINFO_ENTRY(DitherColor),
    TRACEDRVINFO_ENTRY(StrokePath),
    TRACEDRVINFO_ENTRY(FillPath),
    TRACEDRVINFO_ENTRY(StrokeAndFillPath),
    TRACEDRVINFO_ENTRY(Paint),
    TRACEDRVINFO_ENTRY(BitBlt),
    TRACEDRVINFO_ENTRY(TransparentBlt),
    TRACEDRVINFO_ENTRY(CopyBits),
    TRACEDRVINFO_ENTRY(StretchBlt),
    TRACEDRVINFO_ENTRY(StretchBltROP),
    TRACEDRVINFO_ENTRY(SetPalette),
    TRACEDRVINFO_ENTRY(TextOut),
    TRACEDRVINFO_ENTRY(Escape),
    TRACEDRVINFO_ENTRY(DrawEscape),
    TRACEDRVINFO_ENTRY(QueryFont),
    TRACEDRVINFO_ENTRY(QueryFontTree),
    TRACEDRVINFO_ENTRY(QueryFontData),
    TRACEDRVINFO_ENTRY(SetPointerShape),
    TRACEDRVINFO_ENTRY(MovePointer),
    TRACEDRVINFO_ENTRY(LineTo),
    TRACEDRVINFO_ENTRY(SendPage),
    TRACEDRVINFO_ENTRY(StartPage),
    TRACEDRVINFO_ENTRY(EndDoc),
    TRACEDRVINFO_ENTRY(StartDoc),
    TRACEDRVINFO_ENTRY(GetGlyphMode),
    TRACEDRVINFO_ENTRY(Synchronize),
    TRACEDRVINFO_ENTRY(SaveScreenBits),
    TRACEDRVINFO_ENTRY(GetModes),
    TRACEDRVINFO_ENTRY(Free),
    TRACEDRVINFO_ENTRY(DestroyFont),
    TRACEDRVINFO_ENTRY(QueryFontCaps),
    TRACEDRVINFO_ENTRY(LoadFontFile),
    TRACEDRVINFO_ENTRY(UnloadFontFile),
    TRACEDRVINFO_ENTRY(FontManagement),
    TRACEDRVINFO_ENTRY(QueryTrueTypeTable),
    TRACEDRVINFO_ENTRY(QueryTrueTypeOutline),
    TRACEDRVINFO_ENTRY(GetTrueTypeFile),
    TRACEDRVINFO_ENTRY(QueryFontFile),
    TRACEDRVINFO_ENTRY(QueryAdvanceWidths),
    TRACEDRVINFO_ENTRY(SetPixelFormat),
    TRACEDRVINFO_ENTRY(DescribePixelFormat),
    TRACEDRVINFO_ENTRY(SwapBuffers),
    TRACEDRVINFO_ENTRY(StartBanding),
    TRACEDRVINFO_ENTRY(NextBand),
    TRACEDRVINFO_ENTRY(GetDirectDrawInfo),
    TRACEDRVINFO_ENTRY(EnableDirectDraw),
    TRACEDRVINFO_ENTRY(DisableDirectDraw),
    TRACEDRVINFO_ENTRY(QuerySpoolType),
    TRACEDRVINFO_ENTRY(GradientFill),
    TRACEDRVINFO_ENTRY(SynchronizeSurface),
    TRACEDRVINFO_ENTRY(AlphaBlend)
  };

PTRACEDRVINFO
FindTraceInfo(unsigned Index)
{
  unsigned i;

  for (i = 0; i < sizeof(TraceDrvInfo) / sizeof(TRACEDRVINFO); i++)
    {
      if (TraceDrvInfo[i].Index == Index)
        {
          return TraceDrvInfo + i;
        }
    }

  return NULL;
}

#define DRIVER_FUNCTION(function) \
      case INDEX_Drv##function: \
        FindTraceInfo(INDEX_Drv##function)->DrvRoutine = DED->pdrvfn[i].pfn; \
        *(PVOID*)&DF->function = &Trace##function; \
        break
#else
#define DRIVER_FUNCTION(function) \
      case INDEX_Drv##function: \
        *(PVOID*)&DF->function = DED->pdrvfn[i].pfn; \
        break
#endif

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
    DRIVER_FUNCTION(Offset);
    DRIVER_FUNCTION(ResetPDEV);
    DRIVER_FUNCTION(DisableDriver);
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
    DRIVER_FUNCTION(StretchBltROP);
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
    DRIVER_FUNCTION(AlphaBlend);

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
