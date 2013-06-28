/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos.h
 * PURPOSE:         VDM DOS Kernel (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _DOS_H_
#define _DOS_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define DOS_VERSION MAKEWORD(6, 0)
#define DOS_CONFIG_PATH L"%SystemRoot%\\system32\\CONFIG.NT"
#define DOS_COMMAND_INTERPRETER L"%SystemRoot%\\system32\\COMMAND.COM /k %SystemRoot%\\system32\\AUTOEXEC.NT"
#define FIRST_MCB_SEGMENT 0x1000
#define USER_MEMORY_SIZE 0x8FFFF
#define SYSTEM_PSP 0x08
#define SYSTEM_ENV_BLOCK 0x800
#define SEGMENT_TO_MCB(seg) ((PDOS_MCB)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), 0)))
#define SEGMENT_TO_PSP(seg) ((PDOS_PSP)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), 0)))

#pragma pack(push, 1)

typedef struct _DOS_MCB
{
    CHAR BlockType;
    WORD OwnerPsp;
    WORD Size;
    BYTE Unused[3];
    CHAR Name[8];
} DOS_MCB, *PDOS_MCB;

typedef struct _DOS_FCB
{
    BYTE DriveNumber;
    CHAR FileName[8];
    CHAR FileExt[3];
    WORD BlockNumber;
    WORD RecordSize;
    DWORD FileSize;
    WORD LastWriteDate;
    WORD LastWriteTime;
    BYTE Reserved[8];
    BYTE BlockRecord;
    BYTE RecordNumber[3];
} DOS_FCB, *PDOS_FCB;

typedef struct _DOS_PSP
{
    BYTE Exit[2];
    WORD MemSize;
    BYTE Reserved0[6];
    DWORD TerminateAddress;
    DWORD BreakAddress;
    DWORD CriticalAddress;
    WORD ParentPsp;
    BYTE HandleTable[20];
    WORD EnvBlock;
    DWORD LastStack;
    WORD HandleTableSize;
    DWORD HandleTablePtr;
    DWORD PreviousPsp;
    DWORD Reserved1;
    WORD DosVersion;
    BYTE Reserved2[14];
    BYTE FarCall[3];
    BYTE Reserved3[9];
    DOS_FCB Fcb;
    BYTE CommandLineSize;
    CHAR CommandLine[127];
} DOS_PSP, *PDOS_PSP;

typedef struct _DOS_SFT_ENTRY
{
    WORD ReferenceCount;
    WORD Mode;
    BYTE Attribute;
    WORD DeviceInfo;
    DWORD DriveParamBlock;
    WORD FirstCluster;
    WORD FileTime;
    WORD FileDate;
    DWORD FileSize;
    DWORD CurrentOffset;
    WORD LastClusterAccessed;
    DWORD DirEntSector;
    BYTE DirEntryIndex;
    CHAR FileName[11];
    BYTE Reserved0[6];
    WORD OwnerPsp;
    BYTE Reserved1[8];
} DOS_SFT_ENTRY, *PDOS_SFT_ENTRY;

typedef struct _DOS_SFT
{
    DWORD NextTablePtr;
    WORD FileCount;
    DOS_SFT_ENTRY Entry[ANYSIZE_ARRAY];
} DOS_SFT, *PDOS_SFT;

typedef struct _DOS_INPUT_BUFFER
{
    BYTE MaxLength, Length;
    CHAR Buffer[ANYSIZE_ARRAY];
} DOS_INPUT_BUFFER, *PDOS_INPUT_BUFFER;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

WORD DosAllocateMemory(WORD Size, WORD *MaxAvailable);
BOOLEAN DosResizeMemory(WORD BlockData, WORD NewSize, WORD *MaxAvailable);
BOOLEAN DosFreeMemory(WORD BlockData);
VOID DosInitializePsp(WORD PspSegment, LPCSTR CommandLine, WORD ProgramSize, WORD Environment);
BOOLEAN DosCreateProcess(LPCSTR CommandLine, WORD EnvBlock);
VOID DosTerminateProcess(WORD Psp, BYTE ReturnCode);
CHAR DosReadCharacter();
VOID DosPrintCharacter(CHAR Character);
VOID DosInt20h(WORD CodeSegment);
VOID DosInt21h(WORD CodeSegment);
VOID DosBreakInterrupt();
BOOLEAN DosInitialize();

#endif

/* EOF */

