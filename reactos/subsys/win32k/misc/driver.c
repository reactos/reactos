/* $Id: driver.c,v 1.6 1999/12/09 02:45:05 rex Exp $
 * 
 * GDI Driver support routines
 * (mostly swiped from Wine)
 * 
 */

#undef WIN32_LEAN_AND_MEAN
#include <ddk/ntddk.h>
#include <windows.h>
#include <win32k/driver.h>
#include <wchar.h>

//#define NDEBUG
#include <internal/debug.h>

typedef struct _GRAPHICS_DRIVER
{
  PWSTR  Name;
  PGD_ENABLEDRIVER  EnableDriver;
  int  ReferenceCount;
  struct _GRAPHICS_DRIVER  *Next;
} GRAPHICS_DRIVER, *PGRAPHICS_DRIVER;

static PGRAPHICS_DRIVER  DriverList;
static PGRAPHICS_DRIVER  GenericDriver;

BOOL  DRIVER_RegisterDriver(LPCWSTR  Name, PGD_ENABLEDRIVER  EnableDriver)
{
  PGRAPHICS_DRIVER  Driver = ExAllocatePool(NonPagedPool, sizeof(*Driver));
  if (!Driver) 
    {
      return  FALSE;
    }
  Driver->ReferenceCount = 0;
  Driver->EnableDriver = EnableDriver;
  if (Name)
    {
      Driver->Name = ExAllocatePool(NonPagedPool, 
                                    (wcslen(Name) + 1) * sizeof(WCHAR));
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
  
  Driver->Name = NULL;
  GenericDriver = Driver;
  return  TRUE;
}

PGD_ENABLEDRIVER  DRIVER_FindDDIDriver(LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
    {
      if (!_wcsicmp( Driver->Name, Name)) 
        {
          return Driver->EnableDriver;
        }
      Driver = Driver->Next;
    }
  
  return  GenericDriver ? GenericDriver->EnableDriver : NULL;
}

BOOL  DRIVER_BuildDDIFunctions(PDRVENABLEDATA  DED, 
                               PDRIVER_FUNCTIONS  DF)
{
  UNIMPLEMENTED;
}

HANDLE  DRIVER_FindMPDriver(LPCWSTR  Name)
{
  PWSTR  lName;
  HANDLE  DriverHandle;
  NTSTATUS  Status;
  UNICODE_STRING  DeviceName;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  
  if (Name[0] != '\\')
    {
      lName = ExAllocatePool(NonPagedPool, wcslen(Name) * sizeof(WCHAR) + 
                             10 * sizeof(WCHAR));
      wcscpy(lName, L"\\Devices\\");
      wcscat(lName, Name);
    }
  else
    {
      lName = ExAllocatePool(NonPagedPool, wcslen(Name) * sizeof(WCHAR));
      wcscpy(lName, Name);
    }
  
  RtlInitUnicodeString(&DeviceName, lName);
  InitializeObjectAttributes(&ObjectAttributes,
                             &DeviceName,
                             0,
                             NULL,
                             NULL);
  Status = ZwOpenFile(&DriverHandle,
                      FILE_ALL_ACCESS, 
                      &ObjectAttributes, 
                      NULL, 
                      0, 
                      FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to open display device\n");
      DbgPrint("%08lx\n", Status);
      if (Name[0] != '\\')
        {
          ExFreePool(lName);
        }
      
      return  NULL;
    }

  if (Name[0] != '\\')
    {
      ExFreePool(lName);
    }

  return  DriverHandle;
}

BOOL  DRIVER_UnregisterDriver(LPCWSTR  Name)
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

INT  DRIVER_ReferenceDriver (LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
    {
      if (!_wcsicmp( Driver->Name, Name)) 
        {
          return ++Driver->ReferenceCount;
        }
      Driver = Driver->Next;
    }
  
  return  GenericDriver ? ++GenericDriver->ReferenceCount : 0;
}

INT  DRIVER_UnreferenceDriver (LPCWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
    {
      if (!_wcsicmp( Driver->Name, Name)) 
        {
          return --Driver->ReferenceCount;
        }
      Driver = Driver->Next;
    }
  
  return  GenericDriver ? --GenericDriver->ReferenceCount : 0;
}

