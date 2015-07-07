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
#define DOS_CODE_SEGMENT 0x70
#define DOS_DATA_SEGMENT 0xA0

#define DOS_DATA_OFFSET(x) FIELD_OFFSET(DOS_DATA, x)

#define INVALID_DOS_HANDLE  0xFFFF
#define DOS_INPUT_HANDLE    0
#define DOS_OUTPUT_HANDLE   1
#define DOS_ERROR_HANDLE    2

#define DOS_SFT_SIZE 255
#define UMB_START_SEGMENT 0xC000
#define UMB_END_SEGMENT 0xDFFF
#define DOS_ALLOC_HIGH 0x40
#define DOS_ALLOC_HIGH_LOW 0x80
#define DOS_DIR_LENGTH 64
#define NUM_DRIVES ('Z' - 'A' + 1)
#define DOS_CHAR_ATTRIBUTE 0x07

/* 16 MB of EMS memory */
#define EMS_TOTAL_PAGES 1024

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

typedef struct _DOS_SYSVARS
{
    DWORD OemHandler;
    WORD Int21hReturn;
    WORD ShareRetryCount;
    WORD ShareRetryDelay;
    DWORD DiskBuffer;
    WORD UnreadConInput;
    WORD FirstMcb;

    /* This is where the SYSVARS really start */
    DWORD FirstDpb;
    DWORD FirstSft;
    DWORD ActiveClock;
    DWORD ActiveCon;
    BYTE Reserved0[6];
    DWORD CurrentDirs;
    BYTE Reserved1[6];
    BYTE NumBlockDevices;
    BYTE NumLocalDrives; // Set by LASTDRIVE
    DOS_DRIVER NullDevice;
    BYTE NullDriverRoutine[7];
} DOS_SYSVARS, *PDOS_SYSVARS;

typedef struct _DOS_CLOCK_TRANSFER_RECORD
{
    WORD NumberOfDays;
    BYTE Minutes;
    BYTE Hours;
    BYTE Hundredths;
    BYTE Seconds;
} DOS_CLOCK_TRANSFER_RECORD, *PDOS_CLOCK_TRANSFER_RECORD;

typedef struct _DOS_INPUT_BUFFER
{
    BYTE MaxLength;
    BYTE Length;
    CHAR Buffer[ANYSIZE_ARRAY];
} DOS_INPUT_BUFFER, *PDOS_INPUT_BUFFER;

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

typedef struct _DOS_SDA
{
    BYTE PrinterEchoFlag;
    CHAR CurrentSwitchChar;
    BYTE AllocStrategy;
    BYTE Unused0[28];

    /* This is where the SDA really starts */
    BYTE ErrorMode;
    BYTE InDos;
    BYTE ErrorDrive;
    BYTE LastErrorLocus;
    WORD LastErrorCode;
    BYTE LastErrorAction;
    BYTE LastErrorClass;
    DWORD LastErrorPointer;
    DWORD DiskTransferArea;
    WORD CurrentPsp;
    WORD Int23StackPointer;
    WORD ErrorLevel;
    BYTE CurrentDrive;
    BYTE ExtendedBreakFlag;

    /* This part is only valid while in DOS */
    WORD LastAX;
    WORD NetworkPsp;
    WORD NetworkMachineNumber;
    WORD FirstFreeMcb;
    WORD BestFreeMcb;
    WORD LastFreeMcb;
    WORD MemorySize;
    WORD LastSearchDirEntry;
    BYTE Int24FailFlag;
    BYTE DirectoryFlag;
    BYTE CtrlBreakFlag;
    BYTE AllowFcbBlanks;
    BYTE Unused1;
    BYTE DayOfMonth;
    BYTE Month;
    WORD Year;
    WORD NumDays;
    BYTE DayOfWeek;
    BYTE ConsoleSwappedFlag;
    BYTE Int28CallOk;
    BYTE Int24AbortFlag;
    DOS_RW_REQUEST Request;
    DWORD DriverEntryPoint;
    BYTE Unused2[44];
    BYTE PspCopyType;
    BYTE Unused3;
    BYTE UserNumber[3];
    BYTE OemNumber;
    WORD ErrorCodeTable;
    DOS_CLOCK_TRANSFER_RECORD ClockTransferRecord;
    BYTE ByteBuffer;
    BYTE Unused4;
    CHAR FileNameBuffer[256];
    BYTE Unused5[53];
    CHAR CurrentDirectory[81];
    CHAR FcbFilename[12];
    CHAR FcbRenameDest[12];
    BYTE Unused6[8];
    BYTE ExtendedAttribute;
    BYTE FcbType;
    BYTE DirSearchAttributes;
    BYTE FileOpenMode;
    BYTE FileFound;
    BYTE DeviceNameFound;
    BYTE SpliceFlag;
    BYTE DosCallFlag;
    BYTE Unused7[5];
    BYTE InsertMode;
    BYTE ParsedFcbExists;
    BYTE VolumeIDFlag;
    BYTE TerminationType;
    BYTE CreateFileFlag;
    BYTE FileDeletedChar;
    DWORD CriticalErrorDpb;
    DWORD UserRegistersStack;
    WORD Int24StackPointer;
    BYTE Unused8[14];
    DWORD DeviceHeader;
    DWORD CurrentSft;
    DWORD CurrentDirPointer;
    DWORD CallerFcb;
    WORD SftNumber;
    WORD TempFileHandle;
    DWORD JftEntry;
    WORD FirstArgument;
    WORD SecondArgument;
    WORD LastComponent;
    WORD TransferOffset;
    BYTE Unused9[38];
    DWORD WorkingSft;
    WORD Int21CallerBX;
    WORD Int21CallerDS;
    WORD Unused10;
    DWORD PrevCallFrame;
} DOS_SDA, *PDOS_SDA;

typedef struct _DOS_DATA
{
    DOS_SYSVARS SysVars;
    DOS_SDA Sda;
    CHAR CurrentDirectories[NUM_DRIVES][DOS_DIR_LENGTH];
    BYTE Sft[ANYSIZE_ARRAY];
} DOS_DATA, *PDOS_DATA;

#pragma pack(pop)

/* VARIABLES ******************************************************************/

extern BOOLEAN DoEcho;
extern WORD DosErrorLevel;
extern WORD DosLastError;
extern PDOS_SYSVARS SysVars;
extern PDOS_SDA Sda;

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

BOOLEAN DosKRNLInitialize(VOID);

#endif // _DOS_H_

/* EOF */
