/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/dfp.h
 * PURPOSE:     Directive file parser header
 */
#ifndef __DFP_H
#define __DFP_H

#include "cabinet.h"

typedef struct _CABINET_NAME {
    struct _CABINET_NAME *Next;
    ULONG DiskNumber;
    TCHAR Name[128];
} CABINET_NAME, *PCABINET_NAME;

typedef struct _DISK_NUMBER {
    struct _DISK_NUMBER *Next;
    ULONG DiskNumber;
    ULONG Number;
} DISK_NUMBER, *PDISK_NUMBER;

typedef enum {
    TokenUnknown,
    TokenInteger,
    TokenIdentifier,
    TokenString,
    TokenSpace,
    TokenSemi,
    TokenEqual,
    TokenPeriod,
    TokenBackslash,
    TokenEnd,
} TOKEN;


typedef enum {
    stCabinetName,
    stCabinetNameTemplate,
    stDiskLabel,
    stDiskLabelTemplate,
    stMaxDiskSize
} SETTYPE;


typedef enum {
    ntDisk,
    ntCabinet,
    ntFolder,
} NEWTYPE;


/* Classes */

class CDFParser : public CCabinet {
public:
    CDFParser();
    virtual ~CDFParser();
    ULONG Load(LPTSTR FileName);
    ULONG Parse();
private:
    /* Event handlers */
    virtual BOOL OnDiskLabel(ULONG Number, LPTSTR Label);
    virtual BOOL OnCabinetName(ULONG Number, LPTSTR Name);

    BOOL SetDiskName(PCABINET_NAME *List, ULONG Number, LPTSTR String);
    BOOL GetDiskName(PCABINET_NAME *List, ULONG Number, LPTSTR String);
    BOOL SetDiskNumber(PDISK_NUMBER *List, ULONG Number, ULONG Value);
    BOOL GetDiskNumber(PDISK_NUMBER *List, ULONG Number, PULONG Value);
    BOOL DoDiskLabel(ULONG Number, LPTSTR Label);
    VOID DoDiskLabelTemplate(LPTSTR Template);
    BOOL DoCabinetName(ULONG Number, LPTSTR Name);
    VOID DoCabinetNameTemplate(LPTSTR Template);
    ULONG DoMaxDiskSize(BOOL NumberValid, ULONG Number);
    ULONG SetupNewDisk();
    ULONG PerformSetCommand();
    ULONG PerformNewCommand();
    ULONG PerformCommand();
    ULONG PerformFileCopy();
    VOID SkipSpaces();
    BOOL IsNextToken(TOKEN Token, BOOL NoSpaces);
    BOOL ReadLine();
    VOID NextToken();
    /* Parsing */
    BOOL FileLoaded;
    HANDLE FileHandle;
    PCHAR FileBuffer;
    DWORD FileBufferSize;
    ULONG CurrentOffset;
    TCHAR Line[128];
    ULONG LineLength;
    ULONG CurrentLine;
    ULONG CurrentChar;
    /* Token */
    TOKEN CurrentToken;
    ULONG CurrentInteger;
    TCHAR CurrentString[256];

    /* State */
    BOOL CabinetCreated;
    BOOL DiskCreated;
    BOOL FolderCreated;
    /* Standard directive variable */
    BOOL Cabinet;
    ULONG CabinetFileCountThreshold;
    PCABINET_NAME CabinetName;
    BOOL CabinetNameTemplateSet;
    TCHAR CabinetNameTemplate[128];
    BOOL Compress;
    ULONG CompressionType;
    PCABINET_NAME DiskLabel;
    BOOL DiskLabelTemplateSet;
    TCHAR DiskLabelTemplate[128];
    ULONG FolderFileCountThreshold;
    ULONG FolderSizeThreshold;
    ULONG MaxCabinetSize;
    ULONG MaxDiskFileCount;
    PDISK_NUMBER MaxDiskSize;
    BOOL MaxDiskSizeAllSet;
    ULONG MaxDiskSizeAll;
    ULONG ReservePerCabinetSize;
    ULONG ReservePerDataBlockSize;
    ULONG ReservePerFolderSize;
    TCHAR SourceDir[256];
};

#endif /* __DFP_H */

/* EOF */
