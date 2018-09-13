/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    version.h

Abstract:

    Declares the structures used for version checkings.

Author:

    Calin Negreanu (calinn) 01/20/1999

Revision History:

--*/

#pragma once

#include <windows.h>
#include <winnt.h>

PBYTE
ShMapFileIntoMemory (
    IN      PCTSTR  FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle
    );

BOOL
ShUnmapFile (
    IN PBYTE  FileImage,
    IN HANDLE MapHandle,
    IN HANDLE FileHandle
    );

DWORD
ShGetModuleType (
    IN      PBYTE MappedImage
    );

ULONG
ShGetPECheckSum (
    IN      PBYTE MappedImage
    );

UINT
ShGetCheckSum (
    IN      ULONG ImageSize,
    IN      PBYTE MappedImage
    );

PTSTR
ShGet16ModuleDescription (
    IN      PBYTE MappedImage
    );
