/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engprint.c
 * PURPOSE:         Printing Support Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
EngCheckAbort(IN SURFOBJ* Surface)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngEnumForms(
  IN HANDLE  hPrinter,
  IN DWORD  Level,
  OUT LPBYTE  pForm,
  IN DWORD  cbBuf,
  OUT LPDWORD  pcbNeeded,
  OUT LPDWORD  pcReturned)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngGetForm(
  IN HANDLE  hPrinter,
  IN LPWSTR  pFormName,
  IN DWORD  Level,
  OUT LPBYTE  pForm,
  IN DWORD  cbBuf,
  OUT LPDWORD  pcbNeeded)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngGetPrinter(
  IN HANDLE  hPrinter,
  IN DWORD  dwLevel,
  OUT PBYTE  pPrinter,
  IN DWORD  cbBuf,
  OUT PDWORD  pcbNeeded)
{
    UNIMPLEMENTED;
	return FALSE;
}

DWORD
APIENTRY
EngGetPrinterData(
  IN HANDLE  hPrinter,
  IN PWSTR  pValueName,
  OUT PDWORD  pType,
  OUT PBYTE  pData,
  IN DWORD  nSize,
  OUT PDWORD  pcbNeeded)
{
    UNIMPLEMENTED;
	return 0;
}

DWORD
APIENTRY
EngSetPrinterData(
  IN HANDLE  hPrinter,
  IN LPWSTR  pType,
  IN DWORD  dwType,
  IN LPBYTE  lpbPrinterData,
  IN DWORD  cjPrinterData)
{
    UNIMPLEMENTED;
	return 0;
}

PWSTR
APIENTRY
EngGetPrinterDataFileName(IN HDEV hDev)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
EngGetPrinterDriver(IN HANDLE  hPrinter,
                    IN PWSTR  pEnvironment,
                    IN DWORD  dwLevel,
                    OUT BYTE  *lpbDrvInfo,
                    IN DWORD  cbBuf,
                    OUT DWORD  *pcbNeeded)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngMarkBandingSurface(IN HSURF hSurface)
{
    UNIMPLEMENTED;
	return FALSE;
}

BOOL
APIENTRY
EngWritePrinter(IN HANDLE  hPrinter,
                IN PVOID  pBuf,
                IN DWORD  cbBuf,
                OUT PDWORD  pcWritten)
{
    UNIMPLEMENTED;
	return FALSE;
}
