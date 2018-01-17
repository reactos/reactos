/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling definitions
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */


#ifndef _MARSHALLING_H
#define _MARSHALLING_H

typedef struct _MARSHALLING_INFO
{
    DWORD dwOffset;             /** Byte offset of this element within the structure or MAXDWORD to indicate the end of the array */
    DWORD cbSize;               /** Total size of this element in bytes under Windows. Unused here, I don't know what we need this number for. */
    DWORD cbPerElementSize;     /** If this element is a structure itself, this field gives the size in bytes of each element of the structure.
                                Otherwise, this is the same as cbTotalSize. E.g. for SYSTEMTIME, cbSize would be 16 and cbPerElementSize would be 2.
                                Unused here, I don't know what we need this number for. */
    BOOL bAdjustAddress;        /** TRUE if MarshallDownStructure shall adjust the address of this element, FALSE if it shall leave this element untouched. */
}
MARSHALLING_INFO;

typedef struct _MARSHALLING
{
    DWORD cbStructureSize;
    MARSHALLING_INFO pInfo[];
}
MARSHALLING;

BOOL WINAPI MarshallDownStructure(PVOID pStructure, const MARSHALLING_INFO* pInfo, DWORD cbStructureSize, BOOL bSomeBoolean);
BOOL WINAPI MarshallDownStructuresArray(PVOID pStructuresArray, DWORD cElements, const MARSHALLING_INFO* pInfo, DWORD cbStructureSize, BOOL bSomeBoolean);
BOOL WINAPI MarshallUpStructure(DWORD cbSize, PVOID pStructure, const MARSHALLING_INFO* pInfo, DWORD cbStructureSize, BOOL bSomeBoolean);
BOOL WINAPI MarshallUpStructuresArray(DWORD cbSize, PVOID pStructuresArray, DWORD cElements, const MARSHALLING_INFO* pInfo, DWORD cbStructureSize, BOOL bSomeBoolean);

#endif
