/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/bios/bios.h
 * PURPOSE:         VDM BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _BIOS_H_
#define _BIOS_H_

/* INCLUDES *******************************************************************/

#include "kbdbios.h"
#include "vidbios.h"

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define BOP_RESET       0x00    // Windows NTVDM (SoftPC) BIOS calls BOP 0x00
                                // to let the virtual machine perform the POST.
#define BOP_EQUIPLIST   0x11
#define BOP_GETMEMSIZE  0x12




#define BDA_SEGMENT     0x40
#define BIOS_SEGMENT    0xF000

#pragma pack(push, 1)

/*
 * BIOS Data Area at 0040:XXXX
 *
 * See: http://www.techhelpmanual.com/93-rom_bios_variables.html
 * and: https://web.archive.org/web/20240119203029/http://www.bioscentral.com/misc/bda.htm
 * for more information.
 */
typedef struct
{
    WORD SerialPorts[4];                        // 0x00
    WORD ParallelPorts[3];                      // 0x08
    WORD EbdaSegment;                           // 0x0e - ParallelPort in PC/XT
    WORD EquipmentList;                         // 0x10
    BYTE Reserved0;                             // 0x12 - Errors in PCjr infrared keyboard link
    WORD MemorySize;                            // 0x13
    WORD Reserved1;                             // 0x15 - Scratch pad for manufacturing error tests
    WORD KeybdShiftFlags;                       // 0x17
    BYTE AlternateKeypad;                       // 0x19
    WORD KeybdBufferHead;                       // 0x1a
    WORD KeybdBufferTail;                       // 0x1c
    WORD KeybdBuffer[BIOS_KBD_BUFFER_SIZE];     // 0x1e
    BYTE DriveRecalibrate;                      // 0x3e
    BYTE DriveMotorStatus;                      // 0x3f
    BYTE MotorShutdownCounter;                  // 0x40
    BYTE LastDisketteOperation;                 // 0x41
    BYTE Reserved2[7];                          // 0x42
    BYTE VideoMode;                             // 0x49
    WORD ScreenColumns;                         // 0x4a
    WORD VideoPageSize;                         // 0x4c
    WORD VideoPageOffset;                       // 0x4e
    WORD CursorPosition[BIOS_MAX_PAGES];        // 0x50
    BYTE CursorEndLine;                         // 0x60
    BYTE CursorStartLine;                       // 0x61
    BYTE VideoPage;                             // 0x62
    WORD CrtBasePort;                           // 0x63
    BYTE CrtModeControl;                        // 0x65
    BYTE CrtColorPaletteMask;                   // 0x66

    union                                       // 0x67
    {
        BYTE  CassetteData[5];  // Cassette tape control (unused)
        DWORD ResumeEntryPoint; // CS:IP for 286 return from Protected Mode
    };

    DWORD TickCounter;                          // 0x6c
    BYTE MidnightPassed;                        // 0x70
    BYTE CtrlBreakFlag;                         // 0x71
    WORD SoftReset;                             // 0x72
    BYTE LastDiskOperation;                     // 0x74
    BYTE NumDisks;                              // 0x75
    BYTE DriveControlByte;                      // 0x76
    BYTE DiskPortOffset;                        // 0x77
    BYTE LptTimeOut[4];                         // 0x78
    BYTE ComTimeOut[4];                         // 0x7c
    WORD KeybdBufferStart;                      // 0x80
    WORD KeybdBufferEnd;                        // 0x82
    BYTE ScreenRows;                            // 0x84
    WORD CharacterHeight;                       // 0x85
    BYTE VGAOptions;                            // 0x87
    BYTE VGASwitches;                           // 0x88
    BYTE VGAFlags;                              // 0x89
    BYTE VGADccIDActive;                        // 0x8a
    DWORD Reserved3;                            // 0x8b
    BYTE Reserved4;                             // 0x8f
    BYTE FloppyDriveState[2];                   // 0x90
    BYTE Reserved6[2];                          // 0x92
    BYTE Reserved7[2];                          // 0x94
    BYTE KeybdStatusFlags;                      // 0x96
    BYTE KeybdLedFlags;                         // 0x97
    DWORD Reserved9;                            // 0x98
    DWORD Reserved10;                           // 0x9c
    DWORD Reserved11[2];                        // 0xa0
    DWORD EGAPtr;                               // 0xa8
    BYTE Reserved12[68];                        // 0xac
    BYTE Reserved13[16];                        // 0xf0
} BIOS_DATA_AREA, *PBIOS_DATA_AREA;
C_ASSERT(sizeof(BIOS_DATA_AREA) == 0x100);

/*
 * User Data Area at 0050:XXXX
 *
 * See: https://helppc.netcore2k.net/table/memory-map
 * for more information.
 */
typedef struct
{
    BYTE PrintScreen;                           // 0x00
    BYTE Basic0[3];                             // 0x01
    BYTE SingleDisketteFlag;                    // 0x04
    BYTE PostArea[10];                          // 0x05
    BYTE Basic1;                                // 0x0f
    WORD Basic2;                                // 0x10
    DWORD Basic3;                               // 0x12
    DWORD Basic4;                               // 0x16
    DWORD Basic5;                               // 0x1a
    WORD Reserved0;                             // 0x1e
    WORD DynStorage;                            // 0x20
    BYTE DisketteInitStorage[14];               // 0x22
    DWORD Reserved1;                            // 0x30
} USER_DATA_AREA, *PUSER_DATA_AREA;
C_ASSERT(sizeof(USER_DATA_AREA) == 0x34);

/*
 * BIOS Configuration Table at F000:E6F5 for 100% compatible BIOSes.
 *
 * See: http://www.ctyme.com/intr/rb-1594.htm
 * for more information.
 */
typedef struct _BIOS_CONFIG_TABLE
{
    WORD    Length;                             // 0x00 - Number of bytes following
    BYTE    Model;                              // 0x02
    BYTE    SubModel;                           // 0x03
    BYTE    Revision;                           // 0x04
    BYTE    Feature[5];                         // 0x05 -- 0x09
    // Other BIOSes may extend this table. We don't.
} BIOS_CONFIG_TABLE, *PBIOS_CONFIG_TABLE;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

extern PBIOS_DATA_AREA Bda;
extern PBIOS_CONFIG_TABLE Bct;

VOID WINAPI BiosEquipmentService(LPWORD Stack);
VOID WINAPI BiosGetMemorySize(LPWORD Stack);

BOOLEAN
BiosInitialize(IN LPCSTR BiosFileName,
               IN LPCSTR RomFiles OPTIONAL);

VOID
BiosCleanup(VOID);

#endif // _BIOS_H_

/* EOF */
