/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Precompiled Header for all source files
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#ifndef _PRECOMP_H
#define _PRECOMP_H

#define WIN32_NO_STATUS
#include <limits.h>
#include <wchar.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>
#include <ndk/rtlfuncs.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(localspl);

// Macros
#define IS_VALID_JOB_ID(ID)     (ID >= 1 && ID <= 99999)

// Constants
#define MAX_PRINTER_NAME        220
#define SHD_WIN2003_SIGNATURE   0x4968

// Function pointers
typedef BOOL (WINAPI *PClosePrintProcessor)(HANDLE);
typedef BOOL (WINAPI *PControlPrintProcessor)(HANDLE, DWORD);
typedef BOOL (WINAPI *PEnumPrintProcessorDatatypesW)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef DWORD (WINAPI *PGetPrintProcessorCapabilities)(LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD);
typedef HANDLE (WINAPI *POpenPrintProcessor)(LPWSTR, PPRINTPROCESSOROPENDATA);
typedef BOOL (WINAPI *PPrintDocumentOnPrintProcessor)(HANDLE, LPWSTR);

// Structures
/**
* Describes a Print Processor.
*/
typedef struct _LOCAL_PRINT_PROCESSOR
{
    PWSTR pwszName;
    RTL_GENERIC_TABLE DatatypeTable;
    PClosePrintProcessor pfnClosePrintProcessor;
    PControlPrintProcessor pfnControlPrintProcessor;
    PEnumPrintProcessorDatatypesW pfnEnumPrintProcessorDatatypesW;
    PGetPrintProcessorCapabilities pfnGetPrintProcessorCapabilities;
    POpenPrintProcessor pfnOpenPrintProcessor;
    PPrintDocumentOnPrintProcessor pfnPrintDocumentOnPrintProcessor;
}
LOCAL_PRINT_PROCESSOR, *PLOCAL_PRINT_PROCESSOR;

/**
 * Describes a printer and manages its print jobs.
 * Created once for every printer at startup.
 */
typedef struct _LOCAL_PRINTER
{
    PWSTR pwszPrinterName;
    PWSTR pwszDefaultDatatype;
    DEVMODEW DefaultDevMode;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    LIST_ENTRY JobQueue;
}
LOCAL_PRINTER, *PLOCAL_PRINTER;

/**
 * Describes an entire print job associated to a specific printer through the Printer member.
 * Created with every valid call to LocalStartDocPrinter.
 */
typedef struct _LOCAL_JOB
{
    LIST_ENTRY Entry;
    PLOCAL_PRINTER Printer;
    DWORD dwJobID;
    PWSTR pwszDocumentName;
    PWSTR pwszDatatype;
    PWSTR pwszOutputFile;
    DEVMODEW DevMode;
}
LOCAL_JOB, *PLOCAL_JOB;

/**
 * Describes a template for new print jobs for a specific printer.
 * Created with every valid call to LocalOpenPrinter.
 *
 * This is needed, because you can supply defaults in a LocalOpenPrinter call, which affect all subsequent print jobs
 * started with the same handle and a call to LocalStartDocPrinter.
 */
typedef struct _LOCAL_PRINTER_HANDLE
{
    PLOCAL_PRINTER Printer;
    PLOCAL_JOB StartedJob;
    PWSTR pwszDatatype;
    DEVMODEW DevMode;
}
LOCAL_PRINTER_HANDLE, *PLOCAL_PRINTER_HANDLE;

/**
 * Describes a handle returned by LocalOpenPrinter.
 * Suitable for all things that can be opened through LocalOpenPrinter.
 */
typedef struct _LOCAL_HANDLE
{
    enum { Printer, Monitor, Port } HandleType;
    PVOID SpecificHandle;
}
LOCAL_HANDLE, *PLOCAL_HANDLE;

/**
 * Describes the header of a print job serialized into a shadow file (.SHD)
 * Documented in http://www.undocprint.org/formats/winspool/shd
 * Compatible with Windows Server 2003
 */
typedef struct _SHD_HEADER
{
    DWORD dwSignature;
    DWORD cbHeader;
    WORD wStatus;
    WORD wUnknown1;
    DWORD dwJobID;
    DWORD dwPriority;
    DWORD offUserName;
    DWORD offNotifyName;
    DWORD offDocumentName;
    DWORD offPort;
    DWORD offPrinterName;
    DWORD offDriverName;
    DWORD offDevMode;
    DWORD offPrintProcessor;
    DWORD offDatatype;
    DWORD dwUnknown2;
    SYSTEMTIME stSubmitTime;
    DWORD dwStartTime;
    DWORD dwUntilTime;
    DWORD dwUnknown6;
    DWORD dwPageCount;
    DWORD cbSecurityDescriptor;
    DWORD offSecurityDescriptor;
    DWORD dwUnknown3;
    DWORD dwUnknown4;
    DWORD dwUnknown5;
    DWORD offComputerName;
    DWORD dwSPLSize;
}
SHD_HEADER, *PSHD_HEADER;


// jobs.c
PLOCAL_JOB ReadJobShadowFile(PCWSTR pwszFilePath);
BOOL WriteJobShadowFile(PCWSTR pwszFilePath, const PLOCAL_JOB pJob);

// main.c
extern const WCHAR wszCurrentEnvironment[];
extern HANDLE hProcessHeap;
extern WCHAR wszSpoolDirectory[MAX_PATH];
extern DWORD cchSpoolDirectory;

// printers.c
extern RTL_GENERIC_TABLE PrinterTable;
void InitializePrinterTable();
BOOL WINAPI LocalEnumPrinters(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalOpenPrinter(PWSTR lpPrinterName, HANDLE* phPrinter, PPRINTER_DEFAULTSW pDefault);
DWORD WINAPI LocalStartDocPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo);
BOOL WINAPI LocalStartPagePrinter(HANDLE hPrinter);
BOOL WINAPI LocalWritePrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf, LPDWORD pcWritten);
BOOL WINAPI LocalEndPagePrinter(HANDLE hPrinter);
BOOL WINAPI LocalEndDocPrinter(HANDLE hPrinter);
BOOL WINAPI LocalClosePrinter(HANDLE hPrinter);

// printprocessors.c
extern RTL_GENERIC_TABLE PrintProcessorTable;
void InitializePrintProcessorTable();
BOOL WINAPI LocalEnumPrintProcessorDatatypes(LPWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, LPBYTE pDatatypes, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalEnumPrintProcessors(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalGetPrintProcessorDirectory(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded);

// tools.c
PWSTR AllocAndRegQueryWSZ(HKEY hKey, PCWSTR pwszValueName);
PWSTR DuplicateStringW(PCWSTR pwszInput);
PVOID NTAPI GenericTableAllocateRoutine(PRTL_GENERIC_TABLE Table, CLONG ByteSize);
VOID NTAPI GenericTableFreeRoutine(PRTL_GENERIC_TABLE Table, PVOID Buffer);

#endif
