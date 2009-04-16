
#ifndef __WIN32K_DRIVER_H
#define __WIN32K_DRIVER_H

#include <winddi.h>







BOOL  DRIVER_RegisterDriver(LPCWSTR  Name, PFN_DrvEnableDriver  EnableDriver);
PFN_DrvEnableDriver DRIVER_FindExistingDDIDriver(LPCWSTR  Name);
PFN_DrvEnableDriver  DRIVER_FindDDIDriver(LPCWSTR  Name);
PFILE_OBJECT DRIVER_FindMPDriver(ULONG  DisplayNumber);
BOOL  DRIVER_BuildDDIFunctions(PDRVENABLEDATA  DED,
                               PDRIVER_FUNCTIONS  DF);
BOOL  DRIVER_UnregisterDriver(LPCWSTR  Name);
INT  DRIVER_ReferenceDriver (LPCWSTR  Name);
INT  DRIVER_UnreferenceDriver (LPCWSTR  Name);

#endif


