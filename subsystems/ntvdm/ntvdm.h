/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ntvdm.h
 * PURPOSE:         Header file to define commonly used stuff
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <stdarg.h>

#define NDEBUG
#include <debug.h>

/* DEFINES ********************************************************************/

#define TO_LINEAR(seg, off) (((seg) << 4) + (off))
#define MAX_SEGMENT 0xFFFF
#define MAX_OFFSET 0xFFFF
#define MAX_ADDRESS TO_LINEAR(MAX_SEGMENT, MAX_OFFSET)
#define ROM_AREA_START 0xC0000
#define ROM_AREA_END 0xFFFFF
#define BIOS_PIC_MASTER_INT 0x08
#define BIOS_PIC_SLAVE_INT 0x70
#define BIOS_SEGMENT 0xF000
#define VIDEO_BIOS_INTERRUPT 0x10
#define SPECIAL_INT_NUM 0xFF
#define SEGMENT_TO_MCB(seg) ((PDOS_MCB)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), 0)))
#define SEGMENT_TO_PSP(seg) ((PDOS_PSP)((ULONG_PTR)BaseAddress + TO_LINEAR((seg), 0)))
#define STEPS_PER_CYCLE 256

/* DOS constants */
#define DOS_VERSION 0x0600
#define DOS_CONFIG_PATH L"%SystemRoot%\\system32\\CONFIG.NT"
#define DOS_COMMAND_INTERPRETER L"%SystemRoot%\\system32\\COMMAND.COM /k %SystemRoot%\\system32\\AUTOEXEC.NT"
#define FIRST_MCB_SEGMENT 0x1000
#define USER_MEMORY_SIZE 0x8FFFF
#define SYSTEM_PSP 0x08
#define SYSTEM_ENV_BLOCK 0x800

/* System console constants */
#define CONSOLE_FONT_HEIGHT 8
#define CONSOLE_VIDEO_MEM_START 0xB8000
#define CONSOLE_VIDEO_MEM_END 0xBFFFF

/* Programmable interval timer (PIT) */
#define PIT_CHANNELS 3
#define PIT_BASE_FREQUENCY 1193182LL
#define PIT_DATA_PORT(x) (0x40 + (x))
#define PIT_COMMAND_PORT 0x43

/* Programmable interrupt controller (PIC) */
#define PIC_MASTER_CMD 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD 0xA0
#define PIC_SLAVE_DATA 0xA1
#define PIC_ICW1 0x10
#define PIC_ICW1_ICW4 (1 << 0)
#define PIC_ICW1_SINGLE (1 << 1)
#define PIC_ICW4_8086 (1 << 0)
#define PIC_ICW4_AEOI (1 << 1)
#define PIC_OCW2_NUM_MASK 0x07
#define PIC_OCW2_EOI (1 << 5)
#define PIC_OCW2_SL (1 << 6)
#define PIC_OCW3 (1 << 3)
#define PIC_OCW3_READ_ISR 0x0B

/* 8042 PS/2 controller */
#define KEYBOARD_BUFFER_SIZE 32
#define PS2_DATA_PORT 0x60
#define PS2_CONTROL_PORT 0x64

#define EMULATOR_FLAG_CF (1 << 0)
#define EMULATOR_FLAG_PF (1 << 2)
#define EMULATOR_FLAG_AF (1 << 4)
#define EMULATOR_FLAG_ZF (1 << 6)
#define EMULATOR_FLAG_SF (1 << 7)
#define EMULATOR_FLAG_TF (1 << 8)
#define EMULATOR_FLAG_IF (1 << 9)
#define EMULATOR_FLAG_DF (1 << 10)
#define EMULATOR_FLAG_OF (1 << 11)
#define EMULATOR_FLAG_NT (1 << 14)
#define EMULATOR_FLAG_RF (1 << 16)
#define EMULATOR_FLAG_VM (1 << 17)
#define EMULATOR_FLAG_AC (1 << 18)
#define EMULATOR_FLAG_VIF (1 << 19)
#define EMULATOR_FLAG_VIP (1 << 20)
#define EMULATOR_FLAG_ID (1 << 21)

typedef enum
{
    EMULATOR_REG_AX,
    EMULATOR_REG_CX,
    EMULATOR_REG_DX,
    EMULATOR_REG_BX,
    EMULATOR_REG_SI,
    EMULATOR_REG_DI,
    EMULATOR_REG_SP,
    EMULATOR_REG_BP,
    EMULATOR_REG_ES,
    EMULATOR_REG_CS,
    EMULATOR_REG_SS,
    EMULATOR_REG_DS,
} EMULATOR_REGISTER;

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

extern LPVOID BaseAddress;
extern BOOLEAN VdmRunning;
extern LPCWSTR ExceptionName[];

VOID DisplayMessage(LPCWSTR Format, ...);
BOOLEAN BiosInitialize();
VOID BiosUpdateConsole(ULONG StartAddress, ULONG EndAddress);
VOID BiosPrintCharacter(CHAR Character, BYTE Attribute);
BOOLEAN DosInitialize();
WORD DosAllocateMemory(WORD Size);
BOOLEAN DosFreeMemory(WORD Segment);
WORD DosResizeMemory(WORD Segment, WORD NewSize);
BOOLEAN DosCreateProcess(LPCSTR CommandLine, WORD EnvBlock);
VOID DosInt20h(WORD CodeSegment);
VOID DosInt21h(WORD CodeSegment);
VOID DosBreakInterrupt();
VOID BiosVideoService();
VOID BiosHandleIrq(BYTE IrqNumber);
BYTE PicReadCommand(BYTE Port);
VOID PicWriteCommand(BYTE Port, BYTE Value);
BYTE PicReadData(BYTE Port);
VOID PicWriteData(BYTE Port, BYTE Value);
VOID PicInterruptRequest(BYTE Number);
VOID PitInitialize();
VOID PitWriteCommand(BYTE Value);
BYTE PitReadData(BYTE Channel);
VOID PitWriteData(BYTE Channel, BYTE Value);
VOID PitDecrementCount();
VOID CheckForInputEvents();
VOID EmulatorSetStack(WORD Segment, WORD Offset);
VOID EmulatorExecute(WORD Segment, WORD Offset);
VOID EmulatorInterrupt(BYTE Number);
ULONG EmulatorGetRegister(ULONG Register);
VOID EmulatorSetRegister(ULONG Register, ULONG Value);
VOID EmulatorSetFlag(ULONG Flag);
VOID EmulatorClearFlag(ULONG Flag);
BOOLEAN EmulatorGetFlag(ULONG Flag);
BOOLEAN EmulatorInitialize();
VOID EmulatorStep();
VOID EmulatorCleanup();

/* EOF */
