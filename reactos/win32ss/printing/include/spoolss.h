/*
 * PROJECT:     ReactOS Printing Include files
 * LICENSE:     GNU LGPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Undocumented APIs of the Spooler Router "spoolss.dll"
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#ifndef _REACTOS_SPOOLSS_H
#define _REACTOS_SPOOLSS_H

PBYTE WINAPI
PackStrings(PCWSTR* pSource, PBYTE pDest, PDWORD DestOffsets, PBYTE pEnd);

#endif
