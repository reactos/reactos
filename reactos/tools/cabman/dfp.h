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
	ULONG DiskNumber;
	char Name[128];
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
	ULONG Load(char* FileName);
	ULONG Parse();
	void SetFileRelativePath(char* Path);
	bool InfFileOnly;
	bool DontGenerateInf;
	char FileRelativePath[300];
private:
	/* Event handlers */
	virtual bool OnDiskLabel(ULONG Number, char* Label);
	virtual bool OnCabinetName(ULONG Number, char* Name);

	void WriteInfLine(char* InfLine);
	bool SetDiskName(PCABINET_NAME *List, ULONG Number, char* String);
	bool GetDiskName(PCABINET_NAME *List, ULONG Number, char* String);
	bool SetDiskNumber(PDISK_NUMBER *List, ULONG Number, ULONG Value);
	bool GetDiskNumber(PDISK_NUMBER *List, ULONG Number, PULONG Value);
	bool DoDiskLabel(ULONG Number, char* Label);
	void DoDiskLabelTemplate(char* Template);
	bool DoCabinetName(ULONG Number, char* Name);
	void DoCabinetNameTemplate(char* Template);
	void DoInfFileName(char* InfFileName);
	ULONG DoMaxDiskSize(bool NumberValid, ULONG Number);
	ULONG SetupNewDisk();
	ULONG PerformSetCommand();
	ULONG PerformNewCommand();
	ULONG PerformInfBeginCommand();
	ULONG PerformInfEndCommand();
	ULONG PerformCommand();
	ULONG PerformFileCopy();
	void SkipSpaces();
	bool IsNextToken(DFP_TOKEN Token, bool NoSpaces);
	bool ReadLine();
	void NextToken();
	/* Parsing */
	bool FileLoaded;
	FILEHANDLE FileHandle;
	char* FileBuffer;
	ULONG FileBufferSize;
	ULONG CurrentOffset;
	char Line[128];
	ULONG LineLength;
	ULONG CurrentLine;
	ULONG CurrentChar;
	/* Token */
	DFP_TOKEN CurrentToken;
	ULONG CurrentInteger;
	char CurrentString[256];

	/* State */
	bool CabinetCreated;
	bool DiskCreated;
	bool FolderCreated;
	/* Standard directive variable */
	bool Cabinet;
	ULONG CabinetFileCountThreshold;
	PCABINET_NAME CabinetName;
	bool CabinetNameTemplateSet;
	char CabinetNameTemplate[128];
	bool InfFileNameSet;
	char InfFileName[256];
	bool Compress;
	ULONG CompressionType;
	PCABINET_NAME DiskLabel;
	bool DiskLabelTemplateSet;
	char DiskLabelTemplate[128];
	ULONG FolderFileCountThreshold;
	ULONG FolderSizeThreshold;
	ULONG MaxCabinetSize;
	ULONG MaxDiskFileCount;
	PDISK_NUMBER MaxDiskSize;
	bool MaxDiskSizeAllSet;
	ULONG MaxDiskSizeAll;
	ULONG ReservePerCabinetSize;
	ULONG ReservePerDataBlockSize;
	ULONG ReservePerFolderSize;
	char SourceDir[256];
	FILEHANDLE InfFileHandle;
	bool InfModeEnabled;
};

#endif /* __DFP_H */

/* EOF */
