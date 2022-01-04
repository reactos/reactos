/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/dos.h
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

#define BIOS_CODE_SEGMENT   0x70
#define BIOS_DATA_SEGMENT   0x70
#define DOS_CODE_SEGMENT    0x80
#define DOS_DATA_SEGMENT    0xA5

#define DOS_DATA_OFFSET(x) FIELD_OFFSET(DOS_DATA, x)

#define SYSTEM_ENV_BLOCK    0x600   // FIXME: Should be dynamically initialized!

#define SYSTEM_PSP          0x0008

#define INVALID_DOS_HANDLE  0xFFFF
#define DOS_INPUT_HANDLE    0
#define DOS_OUTPUT_HANDLE   1
#define DOS_ERROR_HANDLE    2

#define DOS_SFT_SIZE        255    // Value of the 'FILES=' command; maximum 255
#define DOS_DIR_LENGTH      64
#define NUM_DRIVES          ('Z' - 'A' + 1)
#define DOS_CHAR_ATTRIBUTE  0x07

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

// http://www.ctyme.com/intr/rb-2983.htm
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
    DWORD FirstDpb;                             // 0x00
    DWORD FirstSft;                             // 0x04
    DWORD ActiveClock;                          // 0x08
    DWORD ActiveCon;                            // 0x0c
    BYTE Reserved0[6];                          // 0x10
    DWORD CurrentDirs;                          // 0x16
    BYTE Reserved1[6];                          // 0x1a
    BYTE NumBlockDevices;                       // 0x20
    BYTE NumLocalDrives;                        // 0x21 - Set by LASTDRIVE
    DOS_DRIVER NullDevice;                      // 0x22
    BYTE Reserved2;                             // 0x34
    WORD ProgramVersionTable;                   // 0x35
    DWORD SetVerTable;                          // 0x37
    WORD Reserved3[2];                          // 0x3b
    WORD BuffersNumber;                         // 0x3f - 'x' parameter in "BUFFERS=x,y" command
    WORD BuffersLookaheadNumber;                // 0x41 - 'y' parameter in "BUFFERS=x,y" command
    BYTE BootDrive;                             // 0x43
    BYTE UseDwordMoves;                         // 0x44
    WORD ExtMemSize;                            // 0x45
    BYTE Reserved4[0x1C];                       // 0x47
    BYTE UmbLinked;                             // 0x63 - 0/1: UMB chain (un)linked to MCB chain
    WORD Reserved5;                             // 0x64
    WORD UmbChainStart;                         // 0x66 - Segment of the first UMB MCB
    WORD MemAllocScanStart;                     // 0x68 - Segment where allocation scan starts
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

/**
 * @struct DOS_FIND_FILE_BLOCK
 * Data block returned in the DTA (Disk Transfer Area) by the
 * INT 21h, AH=4Eh "Find First File" and the INT 21h, AH=4Fh "Find Next File"
 * functions.
 *
 * @see demFileFindFirst(), demFileFindNext()
 **/
typedef struct _DOS_FIND_FILE_BLOCK
{
    /* The 21 first bytes (0x00 to 0x14 included) are reserved */
    CHAR DriveLetter;
    CHAR Pattern[11];
    UCHAR AttribMask;
    DWORD Unused;           // FIXME: We must NOT store a Win32 handle here!
    HANDLE SearchHandle;    // Instead we should use an ID and helpers to map it to Win32.

    /* The following part of the structure is documented */
    UCHAR Attributes;
    WORD FileTime;
    WORD FileDate;
    DWORD FileSize;
    _Null_terminated_ CHAR FileName[13];
} DOS_FIND_FILE_BLOCK, *PDOS_FIND_FILE_BLOCK;

// http://www.ctyme.com/intr/rb-3023.htm
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

/*
 * DOS kernel data structure
 */
typedef struct _DOS_DATA
{
    DOS_SYSVARS SysVars;
    BYTE NullDriverRoutine[7];
    WORD DosVersion; // DOS version to report to programs (can be different from the true one)
    DOS_SDA Sda;
    CHAR CurrentDirectories[NUM_DRIVES][DOS_DIR_LENGTH];
    BYTE UnreadConInputBuffer[128];
    BYTE DosStack[384];
    BYTE Sft[ANYSIZE_ARRAY];
} DOS_DATA, *PDOS_DATA;

/*
 * DOS BIOS data structure at segment 70h
 */
typedef struct _BIOS_DATA
{
    BYTE StartupCode[20];                       // 0x00 - 20 bytes: large enough for now!

/*
 * INT 13h (BIOS Disk Services) handler chain support.
 *
 * RomBiosInt13: The original INT 13h vector (normally from ROM BIOS).
 * PrevInt13   : The previous INT 13h vector in the handler chain (initially
 *               initialized with the RomBiosInt13 value; each time some
 *               program calls INT 2Fh, AH=13h, PrevInt13 is updated).
 *
 * DOS hooks INT 13h with its own code, then (in normal circumstances) calls
 * PrevInt13, so that when a program calls INT 13h, the DOS hook is first called,
 * followed by the previous INT 13h (be it the original or some other hooked one).
 * DOS may call PrevInt13 directly in some internal operations too.
 * RomBiosInt13 is intended to be the original INT 13h vector that existed
 * before DOS was loaded. A particular version of PC-AT's IBM's ROM BIOS
 * (on systems with model byte FCh and BIOS date "01/10/84" only, see
 * http://www.ctyme.com/intr/rb-4453.htm for more details) had a bug on disk
 * reads so that it was patched by DOS, and therefore PrevInt13 was the fixed
 * INT 13 interrupt (for the other cases, a direct call to RomBiosInt13 is done).
 *
 * NOTE: For compatibility with some programs (including virii), PrevInt13 should
 * be at 0070:00B4, see for more details:
 * http://repo.hackerzvoice.net/depot_madchat/vxdevl/vdat/tuvd0001.htm
 * http://vxheaven.org/lib/vsm01.html
 */
    BYTE Padding0[0xB0 - /*FIELD_OFFSET(BIOS_DATA, StartupCode)*/ 20];
    DWORD RomBiosInt13;                         // 0xb0
    DWORD PrevInt13;                            // 0xb4
    BYTE Padding1[0x100 - 0xB8];                // 0xb8
} BIOS_DATA, *PBIOS_DATA;

C_ASSERT(sizeof(BIOS_DATA) == 0x100);

#pragma pack(pop)

/* VARIABLES ******************************************************************/

extern PBIOS_DATA BiosData;
extern PDOS_DATA DosData;
extern PDOS_SYSVARS SysVars;
extern PDOS_SDA Sda;

/* FUNCTIONS ******************************************************************/

extern CALLBACK16 DosContext;
#define RegisterDosInt32(IntNumber, IntHandler)             \
do { \
    ASSERT((0x20 <= IntNumber) && (IntNumber <= 0x2F));     \
    RegisterInt32(DosContext.TrampolineFarPtr +             \
                  DosContext.TrampolineSize   +             \
                  (IntNumber - 0x20) * Int16To32StubSize,   \
                  (IntNumber), (IntHandler), NULL);         \
} while(0);

VOID ConDrvInitialize(VOID);
VOID ConDrvCleanup(VOID);

/*
 * DOS BIOS Functions
 * See bios.c
 */
CHAR DosReadCharacter(WORD FileHandle, BOOLEAN Echo);
BOOLEAN DosCheckInput(VOID);
VOID DosPrintCharacter(WORD FileHandle, CHAR Character);

BOOLEAN DosBIOSInitialize(VOID);

BOOLEAN DosControlBreak(VOID);
VOID DosEchoCharacter(CHAR Character);

/*
 * DOS Kernel Functions
 * See dos.c
 */
BOOLEAN DosKRNLInitialize(VOID);

#endif // _DOS_H_

/* EOF */
