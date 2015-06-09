/*
 * PROJECT:     ReactOS Printing Include files
 * LICENSE:     GNU LGPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Undocumented APIs of the Spooler Router "spoolss.dll"
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#ifndef _REACTOS_SPOOLSS_H
#define _REACTOS_SPOOLSS_H

PWSTR WINAPI AllocSplStr(PCWSTR pwszInput);
PVOID WINAPI DllAllocSplMem(DWORD dwBytes);
BOOL WINAPI DllFreeSplMem(PVOID pMem);
BOOL WINAPI DllFreeSplStr(PWSTR pwszString);
PBYTE WINAPI PackStrings(PCWSTR* pSource, PBYTE pDest, PDWORD DestOffsets, PBYTE pEnd);
PVOID WINAPI ReallocSplMem(PVOID pOldMem, DWORD cbOld, DWORD cbNew);
BOOL WINAPI ReallocSplStr(PWSTR* ppwszString, PCWSTR pwszInput);
BOOL WINAPI SplInitializeWinSpoolDrv(PVOID* pTable);

#endif
