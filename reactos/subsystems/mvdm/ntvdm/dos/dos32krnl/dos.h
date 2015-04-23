/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/dos.h
 * PURPOSE:         DOS32 Kernel
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _DOS_H_
#define _DOS_H_

/* INCLUDES *******************************************************************/

#include "device.h"

/**/ #include "int32.h" /**/

/* DEFINES ********************************************************************/

//
// We are DOS 5.00 (reported by INT 21h, AH=30h)
//    and DOS 5.50 (reported by INT 21h, AX=3306h) for Windows NT Compatibility
//
#define DOS_VERSION     MAKEWORD(5, 00)
#define NTDOS_VERSION   MAKEWORD(5, 50)

#define DOS_CONFIG_PATH L"%SystemRoot%\\system32\\CONFIG.NT"
#define DOS_COMMAND_INTERPRETER L"%SystemRoot%\\system32\\COMMAND.COM /k %SystemRoot%\\system32\\AUTOEXEC.NT"
#define FIRST_MCB_SEGMENT 0x1000
#define USER_MEMORY_SIZE (0x9FFE - FIRST_MCB_SEGMENT)
#define SYSTEM_PSP 0x08
#define SYSTEM_ENV_BLOCK 0x800

#define INVALID_DOS_HANDLE  0xFFFF
#define DOS_INPUT_HANDLE    0
#define DOS_OUTPUT_HANDLE   1
#define DOS_ERROR_HANDLE    2

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
#define DOS_PROGRAM_NAME_TAG 0x0001
#define DEFAULT_JFT_SIZE 20

/* 16 MB of EMS memory */
#define EMS_TOTAL_PAGES 1024

typedef enum
{
    DOS_LOAD_AND_EXECUTE = 0x00,
    DOS_LOAD_ONLY = 0x01,
    DOS_LOAD_OVERLAY = 0x03
} DOS_EXEC_TYPE;

typedef enum
{
    DOS_SFT_ENTRY_NONE,
    DOS_SFT_ENTRY_WIN32,
    DOS_SFT_ENTRY_DEVICE
} DOS_SFT_ENTRY_TYPE;

typedef struct _DOS_SFT_ENTRY
{
    DOS_SFT_ENTRY_TYPE Type;
    WORD RefCount;

    union
    {
        HANDLE Handle;
        PDOS_DEVICE_NODE DeviceNode;
    };
} DOS_SFT_ENTRY, *PDOS_SFT_ENTRY;

#pragma pack(push, 1)

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

typedef struct _DOS_FIND_FILE_BLOCK
{
    CHAR DriveLetter;
    CHAR Pattern[11];
    UCHAR AttribMask;
    DWORD Unused;
    HANDLE SearchHandle;

    /* The following part of the structure is documented */
    UCHAR Attributes;
    WORD FileTime;
    WORD FileDate;
    DWORD FileSize;
    CHAR FileName[13];
} DOS_FIND_FILE_BLOCK, *PDOS_FIND_FILE_BLOCK;

typedef struct _DOS_EXEC_PARAM_BLOCK
{
    /* Input variables */
    WORD Environment;
    DWORD CommandLine;
    DWORD FirstFcb;
    DWORD SecondFcb;

    /* Output variables */
    DWORD StackLocation;
    DWORD EntryPoint;
} DOS_EXEC_PARAM_BLOCK, *PDOS_EXEC_PARAM_BLOCK;

typedef struct _DOS_COUNTRY_CODE_BUFFER
{
    WORD TimeFormat;
    WORD CurrencySymbol;
    WORD ThousandSep;
    WORD DecimalSep;
} DOS_COUNTRY_CODE_BUFFER, *PDOS_COUNTRY_CODE_BUFFER;

#pragma pack(pop)

/* VARIABLES ******************************************************************/

extern BOOLEAN DoEcho;
extern WORD CurrentPsp;
extern WORD DosLastError;

/* FUNCTIONS ******************************************************************/

extern CALLBACK16 DosContext;
#define RegisterDosInt32(IntNumber, IntHandler) \
do { \
    DosContext.NextOffset += RegisterInt32(MAKELONG(DosContext.NextOffset,   \
                                                    DosContext.Segment),     \
                                           (IntNumber), (IntHandler), NULL); \
} while(0);

/*
 * DOS BIOS Functions
 * See bios.c
 */
CHAR DosReadCharacter(WORD FileHandle);
BOOLEAN DosCheckInput(VOID);
VOID DosPrintCharacter(WORD FileHandle, CHAR Character);

BOOLEAN DosBIOSInitialize(VOID);
VOID ConDrvInitialize(VOID);
VOID ConDrvCleanup(VOID);

/*
 * DOS Kernel Functions
 * See dos.c
 */
WORD DosOpenHandle(HANDLE Handle);
PDOS_SFT_ENTRY DosGetSftEntry(WORD DosHandle);

WORD DosCreateFileEx(LPWORD Handle,
                     LPWORD CreationStatus,
                     LPCSTR FilePath,
                     BYTE AccessShareModes,
                     WORD CreateActionFlags,
                     WORD Attributes);
WORD DosCreateFile(LPWORD Handle,
                   LPCSTR FilePath,
                   DWORD CreationDisposition,
                   WORD Attributes);
WORD DosOpenFile(LPWORD Handle,
                 LPCSTR FilePath,
                 BYTE AccessShareModes);
WORD DosReadFile(WORD FileHandle,
                 DWORD Buffer,
                 WORD Count,
                 LPWORD BytesRead);
WORD DosWriteFile(WORD FileHandle,
                  DWORD Buffer,
                  WORD Count,
                  LPWORD BytesWritten);
WORD DosSeekFile(WORD FileHandle,
                 LONG Offset,
                 BYTE Origin,
                 LPDWORD NewOffset);
BOOL DosFlushFileBuffers(WORD FileHandle);

VOID DosInitializePsp(
    WORD PspSegment,
    LPCSTR CommandLine,
    WORD ProgramSize,
    WORD Environment,
    DWORD ReturnAddress
);
DWORD DosLoadExecutable(
    IN DOS_EXEC_TYPE LoadType,
    IN LPCSTR ExecutablePath,
    IN LPCSTR CommandLine,
    IN LPCSTR Environment OPTIONAL,
    IN DWORD ReturnAddress OPTIONAL,
    OUT PDWORD StackLocation OPTIONAL,
    OUT PDWORD EntryPoint OPTIONAL
);
WORD DosCreateProcess(
    DOS_EXEC_TYPE LoadType,
    LPCSTR ProgramName,
    PDOS_EXEC_PARAM_BLOCK Parameters,
    DWORD ReturnAddress
);
DWORD DosStartProcess(
    IN LPCSTR ExecutablePath,
    IN LPCSTR CommandLine,
    IN LPCSTR Environment OPTIONAL
);
VOID DosTerminateProcess(WORD Psp, BYTE ReturnCode, WORD KeepResident);
BOOLEAN DosHandleIoctl(BYTE ControlCode, WORD FileHandle);

BOOLEAN DosKRNLInitialize(VOID);

#endif // _DOS_H_

/* EOF */
