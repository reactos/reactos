#pragma once

#include <winddi.h>

typedef struct _DRIVERS
{
	LIST_ENTRY ListEntry;
    PVOID SectionPointer;
    PVOID BaseAddress;
	UNICODE_STRING DriverName;
}DRIVERS, *PDRIVERS;

BOOL  DRIVER_RegisterDriver(LPCWSTR  Name, PFN_DrvEnableDriver  EnableDriver);
PFN_DrvEnableDriver DRIVER_FindExistingDDIDriver(LPCWSTR  Name);
PFN_DrvEnableDriver  DRIVER_FindDDIDriver(LPCWSTR  Name);
PFILE_OBJECT DRIVER_FindMPDriver(ULONG  DisplayNumber);
BOOL  DRIVER_BuildDDIFunctions(PDRVENABLEDATA  DED,
                               PDRIVER_FUNCTIONS  DF);
BOOL  DRIVER_UnregisterDriver(LPCWSTR  Name);
INT  DRIVER_ReferenceDriver (LPCWSTR  Name);
INT  DRIVER_UnreferenceDriver (LPCWSTR  Name);
