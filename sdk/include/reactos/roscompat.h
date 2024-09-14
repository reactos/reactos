/*!
    \brief Header shared by ntdll/ldr and spec2def
*/
#pragma once

#ifdef ROSCOMPAT_HOST
#include "../host/pecoff.h"
#else
#endif

typedef enum _ROSCOMPAT_VERSION_BIT
{
    ROSCOMPAT_VERSION_BIT_NT4 = 0,
    ROSCOMPAT_VERSION_BIT_WIN2K = 1,
    ROSCOMPAT_VERSION_BIT_WINXP = 2,
    ROSCOMPAT_VERSION_BIT_WS03 = 3,
    ROSCOMPAT_VERSION_BIT_VISTA = 4,
    ROSCOMPAT_VERSION_BIT_WIN7 = 5,
    ROSCOMPAT_VERSION_BIT_WIN8 = 6,
    ROSCOMPAT_VERSION_BIT_WIN81 = 7,
    ROSCOMPAT_VERSION_BIT_WIN10 = 8,
} ROSCOMPAT_VERSION_BIT;

typedef unsigned short ROSCOMPAT_VERMASK;

#if !defined(_WINNT_) && !defined(ROSCOMPAT_HOST)
typedef struct _IMAGE_EXPORT_DIRECTORY
{
    unsigned long Characteristics;
    unsigned long TimeDateStamp;
    unsigned short MajorVersion;
    unsigned short MinorVersion;
    unsigned long Name;
    unsigned long Base;
    unsigned long NumberOfFunctions;
    unsigned long NumberOfNames;
    unsigned long AddressOfFunctions;
    unsigned long AddressOfNames;
    unsigned long AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;
#endif

typedef struct _ROSCOMPAT_DESCRIPTOR
{
    ROSCOMPAT_VERMASK* ExportMasks; // Array with size NumberOfExportNames
    unsigned long NumberOfOrdinals;
    unsigned long BaseOrdinal;
    unsigned long *NumberOfValidExports;
} ROSCOMPAT_DESCRIPTOR, * PROSCOMPAT_DESCRIPTOR;
