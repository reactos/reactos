/*
 * PROJECT:     ReactOS Printing Include files
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Undocumented APIs of the Spooler Router "spoolss.dll" and internally shared interfaces
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#ifndef _REACTOS_SPOOLSS_H
#define _REACTOS_SPOOLSS_H

#define RESETPRINTERDEFAULTDATATYPE 0x0001
#define RESETPRINTERDEFAULTDEVMODE  0x0002

#define PORT_IS_UNKNOWN  0
#define PORT_IS_LPT      1
#define PORT_IS_COM      2
#define PORT_IS_FILE     3
#define PORT_IS_FILENAME 4
#define PORT_IS_WINE     5
#define PORT_IS_UNIXNAME 5
#define PORT_IS_PIPE     6
#define PORT_IS_VNET     7
#define PORT_IS_XPS      8


// Constants
#define MAX_PRINTER_NAME        220

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

/** From MS-RPRN, 2.2.1.10.1 */
typedef struct _PRINTER_INFO_STRESS
{
    PWSTR pPrinterName;
    PWSTR pServerName;
    DWORD cJobs;
    DWORD cTotalJobs;
    DWORD cTotalBytes;
    SYSTEMTIME stUpTime;
    DWORD MaxcRef;
    DWORD cTotalPagesPrinted;
    DWORD dwGetVersion;
    DWORD fFreeBuild;
    DWORD cSpooling;
    DWORD cMaxSpooling;
    DWORD cRef;
    DWORD cErrorOutOfPaper;
    DWORD cErrorNotReady;
    DWORD cJobError;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwHighPartTotalBytes;
    DWORD cChangeID;
    DWORD dwLastError;
    DWORD Status;
    DWORD cEnumerateNetworkPrinters;
    DWORD cAddNetPrinters;
    USHORT wProcessorArchitecture;
    USHORT wProcessorLevel;
    DWORD cRefIC;
    DWORD dwReserved2;
    DWORD dwReserved3;
}
PRINTER_INFO_STRESS, *PPRINTER_INFO_STRESS;

typedef struct _FILE_INFO_1
{
    BOOL   bInheritHandle;
    HANDLE hSpoolFileHandle;
    DWORD  dwOptions;
} FILE_INFO_1, *PFILE_INFO_1;

BOOL WINAPI AddPortExW(LPWSTR, DWORD, LPBYTE, LPWSTR);
PVOID WINAPI AlignRpcPtr(PVOID pBuffer, PDWORD pcbBuffer);
PWSTR WINAPI AllocSplStr(PCWSTR pwszInput);
PVOID WINAPI DllAllocSplMem(DWORD dwBytes);
BOOL WINAPI DllFreeSplMem(PVOID pMem);
BOOL WINAPI DllFreeSplStr(PWSTR pwszString);
BOOL WINAPI InitializeRouter(HANDLE SpoolerStatusHandle);
PBYTE WINAPI PackStrings(PCWSTR* pSource, PBYTE pDest, const DWORD* DestOffsets, PBYTE pEnd);
PVOID WINAPI ReallocSplMem(PVOID pOldMem, DWORD cbOld, DWORD cbNew);
BOOL WINAPI ReallocSplStr(PWSTR* ppwszString, PCWSTR pwszInput);
BOOL WINAPI SeekPrinter(HANDLE hPrinter,LARGE_INTEGER liDistanceToMove,PLARGE_INTEGER pliNewPointer,DWORD dwMoveMethod,BOOL bWrite);
BOOL WINAPI SplInitializeWinSpoolDrv(PVOID* pTable);
BOOL WINAPI SpoolerInit(VOID);
PDWORD WINAPI UndoAlignRpcPtr(PVOID pDestinationBuffer, PVOID pSourceBuffer, DWORD cbBuffer, PDWORD pcbNeeded);
BOOL WINAPI SplGetSpoolFileInfo(HANDLE hPrinter,HANDLE hProcessHandle,DWORD Level,FILE_INFO_1 *pFileInfo,DWORD dwSize,DWORD* dwNeeded );
BOOL WINAPI SplCommitSpoolData(HANDLE hPrinter,HANDLE hProcessHandle,DWORD cbCommit,DWORD Level,FILE_INFO_1 *pFileInfo,DWORD dwSize,DWORD* dwNeeded);
BOOL WINAPI SplCloseSpoolFileHandle( HANDLE hPrinter );
BOOL WINAPI GetPrinterDriverExW(HANDLE hPrinter,LPWSTR pEnvironment,DWORD Level,LPBYTE pDriverInfo,DWORD cbBuf,LPDWORD pcbNeeded,DWORD dwClientMajorVersion,DWORD dwClientMinorVersion,PDWORD pdwServerMajorVersion,PDWORD pdwServerMinorVersion );

#endif
