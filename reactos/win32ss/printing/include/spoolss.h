/*
 * PROJECT:     ReactOS Printing Include files
 * LICENSE:     GNU LGPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Undocumented APIs of the Spooler Router "spoolss.dll"
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#ifndef _REACTOS_SPOOLSS_H
#define _REACTOS_SPOOLSS_H

typedef struct _MARSHALL_DOWN_INFO
{
    DWORD dwOffset;             /** Byte offset of this element within the structure or MAXDWORD to indicate the end of the array */
    DWORD cbSize;               /** Total size of this element in bytes under Windows. Unused here, I don't know what we need this number for. */
    DWORD cbPerElementSize;     /** If this element is a structure itself, this field gives the size in bytes of each element of the structure.
                                    Otherwise, this is the same as cbTotalSize. E.g. for SYSTEMTIME, cbSize would be 16 and cbPerElementSize would be 2.
                                    Unused here, I don't know what we need this number for. */
    BOOL bAdjustAddress;        /** TRUE if MarshallDownStructure shall adjust the address of this element, FALSE if it shall leave this element untouched. */
}
MARSHALL_DOWN_INFO, *PMARSHALL_DOWN_INFO;

PVOID WINAPI AlignRpcPtr(PVOID pBuffer, PDWORD pcbBuffer);
PWSTR WINAPI AllocSplStr(PCWSTR pwszInput);
PVOID WINAPI DllAllocSplMem(DWORD dwBytes);
BOOL WINAPI DllFreeSplMem(PVOID pMem);
BOOL WINAPI DllFreeSplStr(PWSTR pwszString);
BOOL WINAPI InitializeRouter(HANDLE SpoolerStatusHandle);
BOOL WINAPI MarshallDownStructure(PVOID pStructure, PMARSHALL_DOWN_INFO pParameters, DWORD cbStructureSize, BOOL bSomeBoolean);
PBYTE WINAPI PackStrings(PWSTR* pSource, PBYTE pDest, const DWORD* DestOffsets, PBYTE pEnd);
PVOID WINAPI ReallocSplMem(PVOID pOldMem, DWORD cbOld, DWORD cbNew);
BOOL WINAPI ReallocSplStr(PWSTR* ppwszString, PCWSTR pwszInput);
BOOL WINAPI SplInitializeWinSpoolDrv(PVOID* pTable);
BOOL WINAPI SpoolerInit();
PDWORD WINAPI UndoAlignRpcPtr(PVOID pDestinationBuffer, PVOID pSourceBuffer, DWORD cbBuffer, PDWORD pcbNeeded);

#endif
