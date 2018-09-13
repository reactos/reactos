/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    modules.c

Abstract:

    Implements .

Author:

    Calin Negreanu (calinn)  20-Jan-1999

Revision History:

    <alias> <date> <comments>

--*/

#include "modules.h"

PBYTE
ShMapFileIntoMemory (
    IN      PCTSTR  FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle
    )
{
    PBYTE fileImage = NULL;

    //first thing. Try to open the file, read-only
    *FileHandle = CreateFile (
                        FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*FileHandle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    //now try to create a mapping object, read-only
    *MapHandle = CreateFileMapping (*FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);

    if (*MapHandle == NULL) {
        return NULL;
    }

    //one more thing to do: map view of file
    fileImage = MapViewOfFile (*MapHandle, FILE_MAP_READ, 0, 0, 0);

    return fileImage;
}

BOOL
ShUnmapFile (
    IN PBYTE  FileImage,
    IN HANDLE MapHandle,
    IN HANDLE FileHandle
    )
{
    BOOL result = TRUE;

    //if FileImage is a valid pointer then try to unmap file
    if (FileImage != NULL) {
        if (UnmapViewOfFile (FileImage) == 0) {
            result = FALSE;
        }
    }

    //if mapping object is valid then try to delete it
    if (MapHandle != NULL) {
        if (CloseHandle (MapHandle) == 0) {
            result = FALSE;
        }
    }

    //if file handle is valid then try to close the file
    if (FileHandle != INVALID_HANDLE_VALUE) {
        if (CloseHandle (FileHandle) == 0) {
            result = FALSE;
        }
    }

    return result;
}

#define MT_UNKNOWN_MODULE  0
#define MT_DOS_MODULE      1
#define MT_W16_MODULE      2
#define MT_W32_MODULE      3

DWORD
ShGetModuleType (
    IN      PBYTE MappedImage
    )
{
    DWORD result;
    PIMAGE_DOS_HEADER dh;
    PDWORD sign;
    PWORD signNE;

    dh = (PIMAGE_DOS_HEADER) MappedImage;

    if (dh->e_magic != IMAGE_DOS_SIGNATURE) {
        result = MT_UNKNOWN_MODULE;
    } else {
        result = MT_DOS_MODULE;
        sign = (PDWORD)(MappedImage + dh->e_lfanew);
        if (*sign == IMAGE_NT_SIGNATURE) {
            result = MT_W32_MODULE;
        }
        signNE = (PWORD)sign;
        if (*signNE == IMAGE_OS2_SIGNATURE) {
            result = MT_W16_MODULE;
        }
    }
    return result;
}

PIMAGE_NT_HEADERS
pShGetImageNtHeader (
    IN PVOID Base
    )
{
    PIMAGE_NT_HEADERS NtHeaders;

    if (Base != NULL && Base != (PVOID)-1) {
        if (((PIMAGE_DOS_HEADER)Base)->e_magic == IMAGE_DOS_SIGNATURE) {
            NtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);
            if (NtHeaders->Signature == IMAGE_NT_SIGNATURE) {
                return NtHeaders;
            }
        }
    }
    return NULL;
}

ULONG
ShGetPECheckSum (
    IN      PBYTE MappedImage
    )
{
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG result = 0;

    if (ShGetModuleType (MappedImage) != MT_W32_MODULE) {
        return 0;
    }
    NtHeaders = pShGetImageNtHeader(MappedImage);
    if (NtHeaders) {
        if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            result = ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.CheckSum;
        } else
        if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            result = ((PIMAGE_NT_HEADERS64)NtHeaders)->OptionalHeader.CheckSum;
        }
    }
    return result;
}

UINT
ShGetCheckSum (
    IN      ULONG ImageSize,
    IN      PBYTE MappedImage
    )
{
    INT    i,size     = 4096;
    DWORD  startAddr  = 512;
    UINT   checkSum   = 0;

    if (ImageSize < (ULONG)size) {
        //
        // File size is less than 4096. We set the start address to 0 and set the size for the checksum
        // to the actual file size.
        //
        startAddr = 0;
        size = ImageSize;
    }
    else
    if (startAddr + size > ImageSize) {
        //
        // File size is too small. We set the start address so that size of checksum can be 4096 bytes
        //
        startAddr = ImageSize - size;
    }
    if (size <= 3) {
        //
        // we need at least 3 bytes to be able to do something here.
        //
        return 0;
    }
    MappedImage = MappedImage + startAddr;

    for (i = 0; i<(size - 3); i+=4) {
        checkSum += *((PDWORD) (MappedImage + i));
        checkSum = _rotr (checkSum, 1);
    }
    return checkSum;
}

PTSTR
ShGet16ModuleDescription (
    IN      PBYTE MappedImage
    )
{
    CHAR a_result [512];  // we know for sure that this one cannot be larger (size is a byte)
    PTSTR result = NULL;
    PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_OS2_HEADER neHeader;
    PBYTE size;
#ifdef UNICODE
    WCHAR w_result [512];
    INT converted;
#endif

    if (ShGetModuleType (MappedImage) != MT_W16_MODULE) {
        return NULL;
    }
    dosHeader = (PIMAGE_DOS_HEADER) MappedImage;
    neHeader = (PIMAGE_OS2_HEADER) (MappedImage + dosHeader->e_lfanew);
    size = (PBYTE) (MappedImage + neHeader->ne_nrestab);
    if (*size == 0) {
        return NULL;
    }
    CopyMemory (a_result, MappedImage + neHeader->ne_nrestab + 1, *size);
    a_result [*size] = 0;
#ifdef UNICODE
    converted = MultiByteToWideChar (
                    CP_ACP,
                    0,
                    a_result,
                    *size,
                    w_result,
                    512
                    );
    if (!converted) {
        return NULL;
    }
    w_result [*size] = 0;
    result = HeapAlloc (GetProcessHeap (), 0, (converted + 1) * sizeof (WCHAR));
    lstrcpyW (result, w_result);
#else
    result = HeapAlloc (GetProcessHeap (), 0, (*size + 1) * sizeof (CHAR));
    lstrcpyA (result, a_result);
#endif
    return result;
}

