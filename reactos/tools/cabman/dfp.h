/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/dfp.h
 * PURPOSE:     Directive file parser header
 */
#ifndef __DFP_H
#define __DFP_H

#include "cabinet.h"

typedef struct _CABINET_NAME {
    struct _CABINET_NAME *Next;
    unsigned long DiskNumber;
    char Name[128];
} CABINET_NAME, *PCABINET_NAME;

typedef struct _DISK_NUMBER {
    struct _DISK_NUMBER *Next;
    unsigned long DiskNumber;
    unsigned long Number;
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
} DFP_TOKEN;


typedef enum {
    stCabinetName,
    stCabinetNameTemplate,
    stDiskLabel,
    stDiskLabelTemplate,
    stMaxDiskSize,
    stInfFileName
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
    unsigned long Load(char* FileName);
    unsigned long Parse();
    void SetFileRelativePath(char* Path);
	bool InfFileOnly;
    bool DontGenerateInf;
    char FileRelativePath[300];
private:
    /* Event handlers */
    virtual bool OnDiskLabel(unsigned long Number, char* Label);
    virtual bool OnCabinetName(unsigned long Number, char* Name);

    void WriteInfLine(char* InfLine);
    bool SetDiskName(PCABINET_NAME *List, unsigned long Number, char* String);
    bool GetDiskName(PCABINET_NAME *List, unsigned long Number, char* String);
    bool SetDiskNumber(PDISK_NUMBER *List, unsigned long Number, unsigned long Value);
    bool GetDiskNumber(PDISK_NUMBER *List, unsigned long Number, unsigned long* Value);
    bool DoDiskLabel(unsigned long Number, char* Label);
    void DoDiskLabelTemplate(char* Template);
    bool DoCabinetName(unsigned long Number, char* Name);
    void DoCabinetNameTemplate(char* Template);
    void DoInfFileName(char* InfFileName);
    unsigned long DoMaxDiskSize(bool NumberValid, unsigned long Number);
    unsigned long SetupNewDisk();
    unsigned long PerformSetCommand();
    unsigned long PerformNewCommand();
    unsigned long PerformInfBeginCommand();
    unsigned long PerformInfEndCommand();
    unsigned long PerformCommand();
    unsigned long PerformFileCopy();
    void SkipSpaces();
    bool IsNextToken(DFP_TOKEN Token, bool NoSpaces);
    bool ReadLine();
    void NextToken();
    /* Parsing */
    bool FileLoaded;
    FILEHANDLE FileHandle;
    char* FileBuffer;
    unsigned long FileBufferSize;
    unsigned long CurrentOffset;
    char Line[128];
    unsigned long LineLength;
    unsigned long CurrentLine;
    unsigned long CurrentChar;
    /* Token */
    DFP_TOKEN CurrentToken;
    unsigned long CurrentInteger;
    char CurrentString[256];

    /* State */
    bool CabinetCreated;
    bool DiskCreated;
    bool FolderCreated;
    /* Standard directive variable */
    bool Cabinet;
    unsigned long CabinetFileCountThreshold;
    PCABINET_NAME CabinetName;
    bool CabinetNameTemplateSet;
    char CabinetNameTemplate[128];
    bool InfFileNameSet;
    char InfFileName[256];
    bool Compress;
    unsigned long CompressionType;
    PCABINET_NAME DiskLabel;
    bool DiskLabelTemplateSet;
    char DiskLabelTemplate[128];
    unsigned long FolderFileCountThreshold;
    unsigned long FolderSizeThreshold;
    unsigned long MaxCabinetSize;
    unsigned long MaxDiskFileCount;
    PDISK_NUMBER MaxDiskSize;
    bool MaxDiskSizeAllSet;
    unsigned long MaxDiskSizeAll;
    unsigned long ReservePerCabinetSize;
    unsigned long ReservePerDataBlockSize;
    unsigned long ReservePerFolderSize;
    char SourceDir[256];
    FILEHANDLE InfFileHandle;
    bool InfModeEnabled;
};

#endif /* __DFP_H */

/* EOF */
