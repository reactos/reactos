/* $Id: driver.c,v 1.1 1999/06/15 02:26:43 rex Exp $
 * 
 * GDI Driver support routines
 * (mostly swiped from Wine)
 * 
 */

#include <win32k/driver.h>

typedef struct _GRAPHICS_DRIVER
{
  PWSTR  Name;
  PGD_ENABLEDRIVER  EnableDriver;
  struct _GRAPHICS_DRIVER  *Next;
} GRAPHICS_DRIVER, *PGRAPHICS_DRIVER;

static PGRAPHICS_DRIVER  DriverList;
static PGRAPHICS_DRIVER  GenericDriver;

BOOL  DRIVER_RegisterDriver(PWSTR  Name, PGD_ENABLEDRIVER  EnableDriver)
{
  PGRAPHICS_DRIVER  Driver = ExAllocatePool(NonPagedPool, sizeof(*Driver));
  if (!Driver) 
    {
      return  FALSE;
    }
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
  
  Driver->name = NULL;
  GenericDriver = Driver;
  return  TRUE;
}

PGD_ENABLEDRIVER  DRIVER_FindDriver(PWSTR  Name)
{
  GRAPHICS_DRIVER *Driver = DriverList;
  
  while (Driver && Name)
    {
      if (!wcsicmp( Driver->Name, Name)) 
        {
          return Driver->EnableDriver;
        }
      Driver = Driver->Next;
    }
  
  return  GenericDriver ? GenericDriver->EnableDriver : NULL;
}

BOOL  DRIVER_UnregisterDriver(PWSTR  Name)
{
  PGRAPHICS_DRIVER  Driver = NULL;
  
  if (Name)
    {
      if (DriverList != NULL)
        {
          if (!wcsicmp(DriverList->Name, Name))
            {
              Driver = DriverList;
              DriverList = DriverList->Next;
            }
          else
            {
              Driver = DriverList;
              while (Driver->Next && wcsicmp(Driver->Name, Name))
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

