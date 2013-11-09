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

//
// We are DOS 6.00
//
#define DOS_VERSION MAKEWORD(6, 0)

#define DOS_CONFIG_PATH L"%SystemRoot%\\system32\\CONFIG.NT"
#define DOS_COMMAND_INTERPRETER L"%SystemRoot%\\system32\\COMMAND.COM /k %SystemRoot%\\system32\\AUTOEXEC.NT"
#define FIRST_MCB_SEGMENT 0x1000
#define USER_MEMORY_SIZE 0x8FFE
#define SYSTEM_PSP 0x08
#define SYSTEM_ENV_BLOCK 0x800
#define INVALID_DOS_HANDLE 0xFFFF
#define DOS_INPUT_HANDLE 0
#define DOS_OUTPUT_HANDLE 1
#define DOS_ERROR_HANDLE 2
#define DOS_SFT_SIZE 255
#define SEGMENT_TO_MCB(seg) ((PDOS_MCB)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), 0)))
#define SEGMENT_TO_PSP(seg) ((PDOS_PSP)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), 0)))
#define UMB_START_SEGMENT 0xC000
#define UMB_END_SEGMENT 0xDFFF
#define DOS_ALLOC_HIGH 0x40
#define DOS_ALLOC_HIGH_LOW 0x80
#define DOS_CMDLINE_LENGTH 127
#define DOS_DIR_LENGTH 64
#define NUM_DRIVES ('Z' - 'A' + 1)
#define DOS_CHAR_ATTRIBUTE 0x07

enum DOS_ALLOC_STRATEGY
{
    DOS_ALLOC_FIRST_FIT,
    DOS_ALLOC_BEST_FIT,
    DOS_ALLOC_LAST_FIT
};

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
    WORD LastParagraph;
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
    CHAR CommandLine[DOS_CMDLINE_LENGTH];
} DOS_PSP, *PDOS_PSP;

typedef struct _DOS_INPUT_BUFFER
{
    BYTE MaxLength;
    BYTE Length;
    CHAR Buffer[ANYSIZE_ARRAY];
} DOS_INPUT_BUFFER, *PDOS_INPUT_BUFFER;

typedef struct _DOS_DRIVER_HEADER
{
    DWORD NextDriver;
    WORD Attributes;
    WORD StrategyEntry;
    WORD InterruptEntry;
    CHAR DeviceName[8];
} DOS_DRIVER_HEADER, *PDOS_DRIVER_HEADER;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

WORD DosAllocateMemory(WORD Size, WORD *MaxAvailable);
BOOLEAN DosResizeMemory(WORD BlockData, WORD NewSize, WORD *MaxAvailable);
BOOLEAN DosFreeMemory(WORD BlockData);
WORD DosCreateFile(LPWORD Handle, LPCSTR FilePath, WORD Attributes);
WORD DosOpenFile(LPWORD Handle, LPCSTR FilePath, BYTE AccessMode);
WORD DosReadFile(WORD FileHandle, LPVOID Buffer, WORD Count, LPWORD BytesRead);
WORD DosWriteFile(WORD FileHandle, LPVOID Buffer, WORD Count, LPWORD BytesWritten);
WORD DosSeekFile(WORD FileHandle, LONG Offset, BYTE Origin, LPDWORD NewOffset);
BOOLEAN DosDuplicateHandle(WORD OldHandle, WORD NewHandle);
BOOLEAN DosCloseHandle(WORD DosHandle);
VOID DosInitializePsp(WORD PspSegment, LPCSTR CommandLine, WORD ProgramSize, WORD Environment);
BOOLEAN DosCreateProcess(LPCSTR CommandLine, WORD EnvBlock);
VOID DosTerminateProcess(WORD Psp, BYTE ReturnCode);
CHAR DosReadCharacter(VOID);
VOID DosPrintCharacter(CHAR Character);
BOOLEAN DosHandleIoctl(BYTE ControlCode, WORD FileHandle);

VOID WINAPI DosInt20h(LPWORD Stack);
VOID WINAPI DosInt21h(LPWORD Stack);
VOID WINAPI DosBreakInterrupt(LPWORD Stack);
VOID WINAPI DosInt2Fh(LPWORD Stack);

BOOLEAN DosInitialize(VOID);

#endif // _DOS_H_

/* EOF */
