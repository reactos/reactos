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
#define DOS_DATA_SEGMENT 0xA0
#define MASTER_SFT_OFFSET 0x100

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
    BYTE NumLocalDrives;
    DOS_DRIVER NullDevice;
    BYTE NullDriverRoutine[7];
} DOS_SYSVARS, *PDOS_SYSVARS;

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
extern DWORD DiskTransferArea;
extern WORD DosErrorLevel;
extern WORD DosLastError;
extern PDOS_SYSVARS SysVars;

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
