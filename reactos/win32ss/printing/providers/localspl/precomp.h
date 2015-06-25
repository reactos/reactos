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
#include <stdlib.h>
#include <wchar.h>

#include <lmcons.h>
#include <rpc.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>
#include <ndk/rtlfuncs.h>

#define SKIPLIST_LEVELS 16
#include <skiplist.h>
#include <spoolss.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(localspl);

// Macros
#define IS_VALID_JOB_ID(ID)     (ID >= 1 && ID <= 99999)
#define IS_VALID_PRIORITY(P)    (P >= MIN_PRIORITY && P <= MAX_PRIORITY)

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
    LIST_ENTRY Entry;
    PWSTR pwszName;
    PDATATYPES_INFO_1W pDatatypesInfo1;
    DWORD dwDatatypeCount;
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
    // This sort key must be the first element for LookupElementSkiplist to work!
    PWSTR pwszPrinterName;

    DWORD dwAttributes;
    DWORD dwStatus;
    PWSTR pwszPrinterDriver;
    PWSTR pwszDescription;
    PWSTR pwszDefaultDatatype;
    DEVMODEW DefaultDevMode;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    SKIPLIST JobList;
}
LOCAL_PRINTER, *PLOCAL_PRINTER;

/**
 * Describes an entire print job associated to a specific printer through the Printer member.
 * Created with every valid call to LocalStartDocPrinter.
 */
typedef struct _LOCAL_JOB
{
    // This sort key must be the first element for LookupElementSkiplist to work!
    DWORD dwJobID;                              // Internal and external ID of this Job

    PLOCAL_PRINTER pPrinter;                    // Associated Printer to this Job
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;     // Associated Print Processor to this Job
    DWORD dwPriority;                           // Priority of this Job from MIN_PRIORITY to MAX_PRIORITY, default being DEF_PRIORITY
    SYSTEMTIME stSubmitted;                     // Time of the submission of this Job
    PWSTR pwszUserName;                         // User that submitted the Job
    PWSTR pwszNotifyName;                       // User that shall be notified about the status of the Job
    PWSTR pwszDocumentName;                     // Name of the Document that is printed
    PWSTR pwszDatatype;                         // Datatype of the Document
    PWSTR pwszOutputFile;                       // Output File to spool the Job to
    PWSTR pwszPrintProcessorParameters;         // Optional; Parameters for the chosen Print Processor
    PWSTR pwszStatus;                           // Optional; a Status Message for the Job
    DWORD dwTotalPages;                         // Total pages of the Document
    DWORD dwPagesPrinted;                       // Number of pages that have already been printed
    DWORD dwStartTime;                          // Earliest time in minutes since 12:00 AM UTC when this document can be printed
    DWORD dwUntilTime;                          // Latest time in minutes since 12:00 AM UTC when this document can be printed
    DWORD dwStatus;                             // JOB_STATUS_* flags of the Job
    PWSTR pwszMachineName;                      // Name of the machine that submitted the Job (prepended with two backslashes)
    DEVMODEW DevMode;                           // Associated Device Mode to this Job
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
    PLOCAL_PRINTER pPrinter;
    PLOCAL_JOB pStartedJob;
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
    PVOID pSpecificHandle;
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
    SYSTEMTIME stSubmitted;
    DWORD dwStartTime;
    DWORD dwUntilTime;
    DWORD dwUnknown6;
    DWORD dwTotalPages;
    DWORD cbSecurityDescriptor;
    DWORD offSecurityDescriptor;
    DWORD dwUnknown3;
    DWORD dwUnknown4;
    DWORD dwUnknown5;
    DWORD offMachineName;
    DWORD dwSPLSize;
}
SHD_HEADER, *PSHD_HEADER;


// jobs.c
extern SKIPLIST GlobalJobList;
BOOL GetNextJobID(PDWORD dwJobID);
void InitializeGlobalJobList();
void InitializePrinterJobList(PLOCAL_PRINTER pPrinter);
BOOL WINAPI LocalAddJob(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf, LPDWORD pcbNeeded);
BOOL WINAPI LocalEnumJobs(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pStart, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalGetJob(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pStart, DWORD cbBuf, LPDWORD pcbNeeded);
BOOL WINAPI LocalSetJob(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command);
PLOCAL_JOB ReadJobShadowFile(PCWSTR pwszFilePath);
BOOL WriteJobShadowFile(PCWSTR pwszFilePath, const PLOCAL_JOB pJob);

// main.c
extern const WCHAR wszCurrentEnvironment[];
extern const WCHAR wszDefaultDocumentName[];
extern const WCHAR* wszPrintProviderInfo[3];
extern WCHAR wszSpoolDirectory[MAX_PATH];
extern DWORD cchSpoolDirectory;

// printers.c
extern SKIPLIST PrinterList;
void InitializePrinterList();
BOOL WINAPI LocalEnumPrinters(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalOpenPrinter(PWSTR lpPrinterName, HANDLE* phPrinter, PPRINTER_DEFAULTSW pDefault);
DWORD WINAPI LocalStartDocPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo);
BOOL WINAPI LocalStartPagePrinter(HANDLE hPrinter);
BOOL WINAPI LocalWritePrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf, LPDWORD pcWritten);
BOOL WINAPI LocalEndPagePrinter(HANDLE hPrinter);
BOOL WINAPI LocalEndDocPrinter(HANDLE hPrinter);
BOOL WINAPI LocalClosePrinter(HANDLE hPrinter);

// printprocessors.c
BOOL FindDatatype(PLOCAL_PRINT_PROCESSOR pPrintProcessor, PWSTR pwszDatatype);
PLOCAL_PRINT_PROCESSOR FindPrintProcessor(PWSTR pwszName);
void InitializePrintProcessorList();
BOOL WINAPI LocalEnumPrintProcessorDatatypes(LPWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, LPBYTE pDatatypes, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalEnumPrintProcessors(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalGetPrintProcessorDirectory(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded);

// tools.c
PWSTR AllocAndRegQueryWSZ(HKEY hKey, PCWSTR pwszValueName);

#endif
