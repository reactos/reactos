/*
 * PROJECT:     ReactOS Local Port Monitor
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Precompiled Header for all source files
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#ifndef _PRECOMP_H
#define _PRECOMP_H

#define WIN32_NO_STATUS
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>
#include <winuser.h>
#include <ndk/rtlfuncs.h>

#include <spoolss.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(localmon);

#include "resource.h"

// Structures
/**
 * Describes the monitor handle returned by InitializePrintMonitor2.
 * Manages all available ports in this instance.
 */
typedef struct _LOCALMON_HANDLE
{
    CRITICAL_SECTION Section;       /** Critical Section for modifying or reading the ports. */
    LIST_ENTRY FilePorts;           /** Ports created when a document is printed on FILE: and the user entered a file name. */
    LIST_ENTRY RegistryPorts;       /** Valid ports loaded from the local registry. */
    LIST_ENTRY XcvHandles;          /** Xcv handles created with LocalmonXcvOpenPort. */
}
LOCALMON_HANDLE, *PLOCALMON_HANDLE;

/**
 * Describes the port handle returned by LocalmonOpenPort.
 * Manages a legacy port (COM/LPT) or virtual FILE: port for printing as well as its associated printer and job.
 */
typedef struct _LOCALMON_PORT
{
    LIST_ENTRY Entry;
    enum {
        PortType_Other = 0,         /** Any port that doesn't belong into the other categories (default). */
        PortType_FILE,              /** A port created when a document is printed on FILE: and the user entered a file name. */
        PortType_PhysicalCOM,       /** A physical serial port (COM) */
        PortType_PhysicalLPT        /** A physical parallel port (LPT) */
    }
    PortType;
    BOOL bStartedDoc;               /** Whether a document has been started with StartDocPort. */
    DWORD dwJobID;                  /** ID of the printing job we are processing (for later reporting progress using SetJobW). */
    HANDLE hFile;                   /** Handle to the opened port or INVALID_HANDLE_VALUE if it isn't currently opened. */
    HANDLE hPrinter;                /** Handle to the printer for the job on this port (for using SetJobW). */
    PLOCALMON_HANDLE pLocalmon;     /** Pointer to the parent LOCALMON_HANDLE structure. */
    PWSTR pwszMapping;              /** The current mapping of the DOS Device corresponding to this port at the time _CreateNonspooledPort has been called. */
    PWSTR pwszPortName;             /** The name of this port including the trailing colon. Empty for virtual file ports. */
}
LOCALMON_PORT, *PLOCALMON_PORT;

/**
 * Describes the Xcv handle returned by LocalmonXcvOpenPort.
 * Manages the required data for the Xcv* calls.
 */
typedef struct _LOCALMON_XCV
{
    LIST_ENTRY Entry;
    ACCESS_MASK GrantedAccess;
    PLOCALMON_HANDLE pLocalmon;
    PWSTR pwszObject;
}
LOCALMON_XCV, *PLOCALMON_XCV;

// main.c
extern DWORD cbLocalMonitor;
extern DWORD cbLocalPort;
extern PCWSTR pwszLocalMonitor;
extern PCWSTR pwszLocalPort;
void WINAPI LocalmonShutdown(HANDLE hMonitor);

// ports.c
BOOL WINAPI LocalmonClosePort(HANDLE hPort);
BOOL WINAPI LocalmonEndDocPort(HANDLE hPort);
BOOL WINAPI LocalmonEnumPorts(HANDLE hMonitor, PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned);
BOOL WINAPI LocalmonGetPrinterDataFromPort(HANDLE hPort, DWORD ControlID, PWSTR pValueName, PWSTR lpInBuffer, DWORD cbInBuffer, PWSTR lpOutBuffer, DWORD cbOutBuffer, PDWORD lpcbReturned);
BOOL WINAPI LocalmonOpenPort(HANDLE hMonitor, PWSTR pName, PHANDLE pHandle);
BOOL WINAPI LocalmonReadPort(HANDLE hPort, PBYTE pBuffer, DWORD cbBuffer, PDWORD pcbRead);
BOOL WINAPI LocalmonSetPortTimeOuts(HANDLE hPort, LPCOMMTIMEOUTS lpCTO, DWORD Reserved);
BOOL WINAPI LocalmonStartDocPort(HANDLE hPort, PWSTR pPrinterName, DWORD JobId, DWORD Level, PBYTE pDocInfo);
BOOL WINAPI LocalmonWritePort(HANDLE hPort, PBYTE pBuffer, DWORD cbBuf, PDWORD pcbWritten);

// tools.c
BOOL DoesPortExist(PCWSTR pwszPortName);
DWORD GetLPTTransmissionRetryTimeout();
DWORD GetPortNameWithoutColon(PCWSTR pwszPortName, PWSTR* ppwszPortNameWithoutColon);

// xcv.c
BOOL WINAPI LocalmonXcvClosePort(HANDLE hXcv);
DWORD WINAPI LocalmonXcvDataPort(HANDLE hXcv, PCWSTR pszDataName, PBYTE pInputData, DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded);
BOOL WINAPI LocalmonXcvOpenPort(HANDLE hMonitor, PCWSTR pszObject, ACCESS_MASK GrantedAccess, PHANDLE phXcv);

#endif
