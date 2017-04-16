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
#include <strsafe.h>
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
typedef LPMONITOREX(WINAPI *PInitializePrintMonitor)(PWSTR);
typedef LPMONITOR2(WINAPI *PInitializePrintMonitor2)(PMONITORINIT, PHANDLE);

// Forward declarations
typedef struct _LOCAL_HANDLE            LOCAL_HANDLE, *PLOCAL_HANDLE;
typedef struct _LOCAL_JOB               LOCAL_JOB, *PLOCAL_JOB;
typedef struct _LOCAL_PORT              LOCAL_PORT, *PLOCAL_PORT;
typedef struct _LOCAL_PORT_HANDLE       LOCAL_PORT_HANDLE, *PLOCAL_PORT_HANDLE;
typedef struct _LOCAL_PRINT_MONITOR     LOCAL_PRINT_MONITOR, *PLOCAL_PRINT_MONITOR;
typedef struct _LOCAL_PRINT_PROCESSOR   LOCAL_PRINT_PROCESSOR, *PLOCAL_PRINT_PROCESSOR;
typedef struct _LOCAL_PRINTER           LOCAL_PRINTER, *PLOCAL_PRINTER;
typedef struct _LOCAL_PRINTER_HANDLE    LOCAL_PRINTER_HANDLE, *PLOCAL_PRINTER_HANDLE;
typedef struct _LOCAL_XCV_HANDLE        LOCAL_XCV_HANDLE, *PLOCAL_XCV_HANDLE;
typedef struct _SHD_HEADER              SHD_HEADER, *PSHD_HEADER;

// Structures
/**
 * Describes a Print Monitor.
 */
struct _LOCAL_PRINT_MONITOR
{
    LIST_ENTRY Entry;
    PWSTR pwszName;                             /** Name of the Print Monitor as read from the registry. */
    PWSTR pwszFileName;                         /** DLL File Name of the Print Monitor. */
    BOOL bIsLevel2;                             /** Whether this Print Monitor supplies an InitializePrintMonitor2 API (preferred) instead of InitializePrintMonitor. */
    PVOID pMonitor;                             /** For bIsLevel2 == TRUE:  LPMONITOR2 pointer returned by InitializePrintMonitor2.
                                                    For bIsLevel2 == FALSE: LPMONITOREX pointer returned by InitializePrintMonitor. */
    HANDLE hMonitor;                            /** Only used when bIsLevel2 == TRUE: Handle returned by InitializePrintMonitor2. */
};

/**
 * Describes a Port handled by a Print Monitor.
 */
struct _LOCAL_PORT
{
    LIST_ENTRY Entry;
    PWSTR pwszName;                             /** The name of the port (including the trailing colon). */
    PLOCAL_PRINT_MONITOR pPrintMonitor;         /** The Print Monitor handling this port. */
    PLOCAL_JOB pNextJobToProcess;               /** The Print Job that will be processed by the next created Port handle. */
};

/**
 * Describes a Print Processor.
 */
struct _LOCAL_PRINT_PROCESSOR
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
};

/**
 * Describes a printer and manages its print jobs.
 * Created once for every printer at startup.
 */
struct _LOCAL_PRINTER
{
    // This sort key must be the first element for LookupElementSkiplist to work!
    PWSTR pwszPrinterName;

    DWORD dwAttributes;
    DWORD dwStatus;
    PWSTR pwszLocation;
    PWSTR pwszPrinterDriver;
    PWSTR pwszDescription;
    PWSTR pwszDefaultDatatype;
    PDEVMODEW pDefaultDevMode;
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;
    PLOCAL_PORT pPort;
    SKIPLIST JobList;
};

/**
 * Describes an entire print job associated to a specific printer through the Printer member.
 * Created with every valid call to LocalStartDocPrinter.
 */
struct _LOCAL_JOB
{
    // This sort key must be the first element for LookupElementSkiplist to work!
    DWORD dwJobID;                              /** Internal and external ID of this Job */

    BOOL bAddedJob : 1;                         /** Whether AddJob has already been called on this Job. */
    HANDLE hPrintProcessor;                     /** Handle returned by OpenPrintProcessor while the Job is printing. */
    PLOCAL_PRINTER pPrinter;                    /** Associated Printer to this Job */
    PLOCAL_PRINT_PROCESSOR pPrintProcessor;     /** Associated Print Processor to this Job */
    DWORD dwPriority;                           /** Priority of this Job from MIN_PRIORITY to MAX_PRIORITY, default being DEF_PRIORITY */
    SYSTEMTIME stSubmitted;                     /** Time of the submission of this Job */
    PWSTR pwszUserName;                         /** Optional; User that submitted the Job */
    PWSTR pwszNotifyName;                       /** Optional; User that shall be notified about the status of the Job */
    PWSTR pwszDocumentName;                     /** Optional; Name of the Document that is printed */
    PWSTR pwszDatatype;                         /** Datatype of the Document */
    PWSTR pwszOutputFile;                       /** Output File to spool the Job to */
    PWSTR pwszPrintProcessorParameters;         /** Optional; Parameters for the chosen Print Processor */
    PWSTR pwszStatus;                           /** Optional; a Status Message for the Job */
    DWORD dwTotalPages;                         /** Total pages of the Document */
    DWORD dwPagesPrinted;                       /** Number of pages that have already been printed */
    DWORD dwStartTime;                          /** Earliest time in minutes since 12:00 AM UTC when this document can be printed */
    DWORD dwUntilTime;                          /** Latest time in minutes since 12:00 AM UTC when this document can be printed */
    DWORD dwStatus;                             /** JOB_STATUS_* flags of the Job */
    PWSTR pwszMachineName;                      /** Name of the machine that submitted the Job (prepended with two backslashes) */
    PDEVMODEW pDevMode;                         /** Associated Device Mode to this Job */
};

/**
 * Specific handle returned by LocalOpenPrinter for every valid call that opens a Printer or Print Job.
 */
struct _LOCAL_PRINTER_HANDLE
{
    BOOL bStartedDoc : 1;                       /** Whether StartDocPrinter has already been called. */
    HANDLE hSPLFile;                            /** Handle to an opened SPL file for Printer Job handles. */
    PLOCAL_PRINTER pPrinter;                    /** Printer associated with this handle. */
    PLOCAL_JOB pJob;                            /** Print Job associated with this handle. This can be the specified Job if this is a Print Job handle or the started job through LocalStartDocPrinter. */
    PWSTR pwszDatatype;                         /** Datatype used for newly started jobs. */
    PDEVMODEW pDevMode;                         /** DevMode used for newly started jobs. */
};

/**
 * Specific handle returned by LocalOpenPrinter for every valid call that opens a Port.
 */
struct _LOCAL_PORT_HANDLE
{
    HANDLE hPort;                               /** Handle returned by pfnOpenPort. */
    PLOCAL_PORT pPort;                          /** Port associated with this handle. */
};

/**
 * Specific handle returned by LocalOpenPrinter for every valid call that opens an XcvMonitor or XcvPort.
 */
struct _LOCAL_XCV_HANDLE
{
    HANDLE hXcv;                                /** Handle returned by pfnXcvOpenPort. */
    PLOCAL_PRINT_MONITOR pPrintMonitor;         /** Print Monitor associated with this handle. */
};

/**
 * Describes a handle returned by LocalOpenPrinter.
 * Suitable for all things that can be opened through LocalOpenPrinter.
 */
struct _LOCAL_HANDLE
{
    enum {
        HandleType_Port,                        /** pSpecificHandle is a PLOCAL_PORT_HANDLE. */
        HandleType_Printer,                     /** pSpecificHandle is a PLOCAL_PRINTER_HANDLE. */
        HandleType_Xcv                          /** pSpecificHandle is a PLOCAL_XCV_HANDLE. */
    }
    HandleType;
    PVOID pSpecificHandle;
};

/**
 * Describes the header of a print job serialized into a shadow file (.SHD)
 * Documented in http://www.undocprint.org/formats/winspool/shd
 * Compatible with Windows Server 2003
 */
struct _SHD_HEADER
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
    DWORD offPrintProcessorParameters;
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
};

// jobs.c
extern SKIPLIST GlobalJobList;
DWORD WINAPI CreateJob(PLOCAL_PRINTER_HANDLE pPrinterHandle);
void FreeJob(PLOCAL_JOB pJob);
DWORD GetJobFilePath(PCWSTR pwszExtension, DWORD dwJobID, PWSTR pwszOutput);
BOOL InitializeGlobalJobList();
void InitializePrinterJobList(PLOCAL_PRINTER pPrinter);
BOOL WINAPI LocalAddJob(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf, LPDWORD pcbNeeded);
BOOL WINAPI LocalEnumJobs(HANDLE hPrinter, DWORD FirstJob, DWORD NoJobs, DWORD Level, PBYTE pStart, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalGetJob(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pStart, DWORD cbBuf, LPDWORD pcbNeeded);
BOOL WINAPI LocalScheduleJob(HANDLE hPrinter, DWORD dwJobID);
BOOL WINAPI LocalSetJob(HANDLE hPrinter, DWORD JobId, DWORD Level, PBYTE pJobInfo, DWORD Command);
PLOCAL_JOB ReadJobShadowFile(PCWSTR pwszFilePath);
BOOL WriteJobShadowFile(PWSTR pwszFilePath, const PLOCAL_JOB pJob);

// main.c
extern const WCHAR wszCurrentEnvironment[];
extern const DWORD cbCurrentEnvironment;
extern const WCHAR wszDefaultDocumentName[];
extern PWSTR wszPrintProviderInfo[3];
extern WCHAR wszSpoolDirectory[MAX_PATH];
extern DWORD cchSpoolDirectory;

// monitors.c
extern LIST_ENTRY PrintMonitorList;
PLOCAL_PRINT_MONITOR FindPrintMonitor(PCWSTR pwszName);
BOOL InitializePrintMonitorList();
BOOL WINAPI LocalEnumMonitors(PWSTR pName, DWORD Level, PBYTE pMonitors, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned);

// ports.c
PLOCAL_PORT FindPort(PCWSTR pwszName);
BOOL InitializePortList();
BOOL WINAPI LocalEnumPorts(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned);

// printers.c
extern SKIPLIST PrinterList;
BOOL InitializePrinterList();
BOOL WINAPI LocalEnumPrinters(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalOpenPrinter(PWSTR lpPrinterName, HANDLE* phPrinter, PPRINTER_DEFAULTSW pDefault);
BOOL WINAPI LocalReadPrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pNoBytesRead);
DWORD WINAPI LocalStartDocPrinter(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo);
BOOL WINAPI LocalStartPagePrinter(HANDLE hPrinter);
BOOL WINAPI LocalWritePrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf, LPDWORD pcWritten);
BOOL WINAPI LocalEndPagePrinter(HANDLE hPrinter);
BOOL WINAPI LocalEndDocPrinter(HANDLE hPrinter);
BOOL WINAPI LocalClosePrinter(HANDLE hPrinter);

// printingthread.c
DWORD WINAPI PrintingThreadProc(PLOCAL_JOB pJob);

// printprocessors.c
BOOL FindDatatype(const PLOCAL_PRINT_PROCESSOR pPrintProcessor, PCWSTR pwszDatatype);
PLOCAL_PRINT_PROCESSOR FindPrintProcessor(PCWSTR pwszName);
BOOL InitializePrintProcessorList();
BOOL WINAPI LocalEnumPrintProcessorDatatypes(LPWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, LPBYTE pDatatypes, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalEnumPrintProcessors(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned);
BOOL WINAPI LocalGetPrintProcessorDirectory(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded);

// tools.c
PWSTR AllocAndRegQueryWSZ(HKEY hKey, PCWSTR pwszValueName);
PDEVMODEW DuplicateDevMode(PDEVMODEW pInput);

#endif
