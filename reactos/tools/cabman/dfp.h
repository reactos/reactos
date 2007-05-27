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
	uint32_t DiskNumber;
	char Name[128];
} CABINET_NAME, *PCABINET_NAME;

typedef struct _DISK_NUMBER {
	struct _DISK_NUMBER *Next;
	uint32_t DiskNumber;
	uint32_t Number;
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
	uint32_t Load(char* FileName);
	uint32_t Parse();
	void SetFileRelativePath(char* Path);
	bool InfFileOnly;
	bool DontGenerateInf;
	char FileRelativePath[300];
private:
	/* Event handlers */
	virtual bool OnDiskLabel(uint32_t Number, char* Label);
	virtual bool OnCabinetName(uint32_t Number, char* Name);

	void WriteInfLine(char* InfLine);
	bool SetDiskName(PCABINET_NAME *List, uint32_t Number, char* String);
	bool GetDiskName(PCABINET_NAME *List, uint32_t Number, char* String);
	bool SetDiskNumber(PDISK_NUMBER *List, uint32_t Number, uint32_t Value);
	bool GetDiskNumber(PDISK_NUMBER *List, uint32_t Number, uint32_t* Value);
	bool DoDiskLabel(uint32_t Number, char* Label);
	void DoDiskLabelTemplate(char* Template);
	bool DoCabinetName(uint32_t Number, char* Name);
	void DoCabinetNameTemplate(char* Template);
	void DoInfFileName(char* InfFileName);
	uint32_t DoMaxDiskSize(bool NumberValid, uint32_t Number);
	uint32_t SetupNewDisk();
	uint32_t PerformSetCommand();
	uint32_t PerformNewCommand();
	uint32_t PerformInfBeginCommand();
	uint32_t PerformInfEndCommand();
	uint32_t PerformCommand();
	uint32_t PerformFileCopy();
	void SkipSpaces();
	bool IsNextToken(DFP_TOKEN Token, bool NoSpaces);
	bool ReadLine();
	void NextToken();
	/* Parsing */
	bool FileLoaded;
	FILEHANDLE FileHandle;
	char* FileBuffer;
	uint32_t FileBufferSize;
	uint32_t CurrentOffset;
	char Line[128];
	uint32_t LineLength;
	uint32_t CurrentLine;
	uint32_t CurrentChar;
	/* Token */
	DFP_TOKEN CurrentToken;
	uint32_t CurrentInteger;
	char CurrentString[256];

	/* State */
	bool CabinetCreated;
	bool DiskCreated;
	bool FolderCreated;
	/* Standard directive variable */
	bool Cabinet;
	uint32_t CabinetFileCountThreshold;
	PCABINET_NAME CabinetName;
	bool CabinetNameTemplateSet;
	char CabinetNameTemplate[128];
	bool InfFileNameSet;
	char InfFileName[256];
	bool Compress;
	uint32_t CompressionType;
	PCABINET_NAME DiskLabel;
	bool DiskLabelTemplateSet;
	char DiskLabelTemplate[128];
	uint32_t FolderFileCountThreshold;
	uint32_t FolderSizeThreshold;
	uint32_t MaxCabinetSize;
	uint32_t MaxDiskFileCount;
	PDISK_NUMBER MaxDiskSize;
	bool MaxDiskSizeAllSet;
	uint32_t MaxDiskSizeAll;
	uint32_t ReservePerCabinetSize;
	uint32_t ReservePerDataBlockSize;
	uint32_t ReservePerFolderSize;
	char SourceDir[256];
	FILEHANDLE InfFileHandle;
	bool InfModeEnabled;
};

#endif /* __DFP_H */

/* EOF */
