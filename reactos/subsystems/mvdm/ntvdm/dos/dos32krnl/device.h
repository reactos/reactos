/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            device.h
 * PURPOSE:         DOS Device Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _DEVICE_H_
#define _DEVICE_H_

/* DEFINITIONS ****************************************************************/

#define MAX_DEVICE_NAME 8
#define DEVICE_CODE_SIZE 10
#define DEVICE_PRIVATE_AREA(Driver) (Driver + sizeof(DOS_DRIVER) + DEVICE_CODE_SIZE)

#define BOP_DRV_STRATEGY  0x42
#define BOP_DRV_INTERRUPT 0x43

#define DOS_DEVATTR_STDIN       (1 << 0)
#define DOS_DEVATTR_STDOUT      (1 << 1)
#define DOS_DEVATTR_NUL         (1 << 2)
#define DOS_DEVATTR_CLOCK       (1 << 3)
#define DOS_DEVATTR_CON         (1 << 4)
#define DOS_DEVATTR_OPENCLOSE   (1 << 11)
#define DOS_DEVATTR_SPECIAL     (1 << 13)
#define DOS_DEVATTR_IOCTL       (1 << 14)
#define DOS_DEVATTR_CHARACTER   (1 << 15)

#define DOS_DEVCMD_INIT         0
#define DOS_DEVCMD_MEDIACHK     1
#define DOS_DEVCMD_BUILDBPB     2
#define DOS_DEVCMD_IOCTL_READ   3
#define DOS_DEVCMD_READ         4
#define DOS_DEVCMD_PEEK         5
#define DOS_DEVCMD_INSTAT       6
#define DOS_DEVCMD_FLUSH_INPUT  7
#define DOS_DEVCMD_WRITE        8
#define DOS_DEVCMD_WRITE_VERIFY 9
#define DOS_DEVCMD_OUTSTAT      10
#define DOS_DEVCMD_FLUSH_OUTPUT 11
#define DOS_DEVCMD_IOCTL_WRITE  12
#define DOS_DEVCMD_OPEN         13
#define DOS_DEVCMD_CLOSE        14
#define DOS_DEVCMD_REMOVABLE    15
#define DOS_DEVCMD_OUTPUT_BUSY  16

#define DOS_DEVSTAT_DONE    (1 << 8)
#define DOS_DEVSTAT_BUSY    (1 << 9)
#define DOS_DEVSTAT_ERROR   (1 << 15)

#define DOS_DEVERR_WRITE_PROTECT    0
#define DOS_DEVERR_UNKNOWN_UNIT     1
#define DOS_DEVERR_NOT_READY        2
#define DOS_DEVERR_UNKNOWN_COMMAND  3
#define DOS_DEVERR_BAD_DATA_CRC     4
#define DOS_DEVERR_BAD_REQUEST      5
#define DOS_DEVERR_INVALID_SEEK     6
#define DOS_DEVERR_UNKNOWN_MEDIUM   7
#define DOS_DEVERR_BAD_BLOCK        8
#define DOS_DEVERR_OUT_OF_PAPER     9
#define DOS_DEVERR_WRITE_FAULT      10
#define DOS_DEVERR_READ_FAULT       11
#define DOS_DEVERR_GENERAL          12
#define DOS_DEVERR_BAD_MEDIA_CHANGE 15

typedef struct _DOS_DEVICE_NODE DOS_DEVICE_NODE, *PDOS_DEVICE_NODE;

typedef WORD (NTAPI *PDOS_DEVICE_GENERIC_ROUTINE)(PDOS_DEVICE_NODE DeviceNode);

typedef WORD (NTAPI *PDOS_DEVICE_IO_ROUTINE)
(
    PDOS_DEVICE_NODE DeviceNode,
    DWORD Buffer,
    PWORD Length
);

typedef WORD (NTAPI *PDOS_DEVICE_PEEK_ROUTINE)
(
    PDOS_DEVICE_NODE DeviceNode,
    PBYTE Character
);

struct _DOS_DEVICE_NODE
{
    LIST_ENTRY Entry;
    DWORD Driver;
    WORD DeviceAttributes;
    ANSI_STRING Name;
    CHAR NameBuffer[MAX_DEVICE_NAME];
    PDOS_DEVICE_IO_ROUTINE IoctlReadRoutine;
    PDOS_DEVICE_IO_ROUTINE ReadRoutine;
    PDOS_DEVICE_PEEK_ROUTINE PeekRoutine;
    PDOS_DEVICE_GENERIC_ROUTINE InputStatusRoutine;
    PDOS_DEVICE_GENERIC_ROUTINE FlushInputRoutine;
    PDOS_DEVICE_IO_ROUTINE IoctlWriteRoutine;
    PDOS_DEVICE_IO_ROUTINE WriteRoutine;
    PDOS_DEVICE_GENERIC_ROUTINE OutputStatusRoutine;
    PDOS_DEVICE_GENERIC_ROUTINE FlushOutputRoutine;
    PDOS_DEVICE_GENERIC_ROUTINE OpenRoutine;
    PDOS_DEVICE_GENERIC_ROUTINE CloseRoutine;
    PDOS_DEVICE_IO_ROUTINE OutputUntilBusyRoutine;
};

#pragma pack(push, 1)

typedef struct _DOS_DRIVER
{
    DWORD Link;
    WORD  DeviceAttributes;
    WORD  StrategyRoutine;
    WORD  InterruptRoutine;

    union
    {
        CHAR DeviceName[MAX_DEVICE_NAME]; // for character devices

        struct // for block devices
        {
            BYTE UnitCount;
            BYTE Reserved[MAX_DEVICE_NAME - 1];
        };
    };
} DOS_DRIVER, *PDOS_DRIVER;

typedef struct _DOS_REQUEST_HEADER
{
    IN  BYTE RequestLength;
    IN  BYTE UnitNumber OPTIONAL;
    IN  BYTE CommandCode;
    OUT WORD Status;

    BYTE Reserved[8];
} DOS_REQUEST_HEADER, *PDOS_REQUEST_HEADER;

typedef struct _DOS_INIT_REQUEST
{
    DOS_REQUEST_HEADER Header;

    OUT BYTE  UnitsInitialized;
    OUT DWORD ReturnBreakAddress;

    union
    {
        IN  DWORD DeviceString; // for character devices

        struct // for block devices
        {
            IN  BYTE FirstDriveLetter;
            OUT DWORD BpbPointer;
        };
    };

} DOS_INIT_REQUEST, *PDOS_INIT_REQUEST;

typedef struct _DOS_IOCTL_RW_REQUEST
{
    DOS_REQUEST_HEADER Header;

    IN     BYTE  MediaDescriptorByte OPTIONAL;
    IN     DWORD BufferPointer;
    IN OUT WORD  Length;
    IN     WORD  StartingBlock OPTIONAL;
} DOS_IOCTL_RW_REQUEST, *PDOS_IOCTL_RW_REQUEST;

typedef struct _DOS_RW_REQUEST
{
    DOS_REQUEST_HEADER Header;

    IN     BYTE  MediaDescriptorByte OPTIONAL;
    IN     DWORD BufferPointer;
    IN OUT WORD  Length;
    IN     WORD  StartingBlock  OPTIONAL;
    OUT    DWORD VolumeLabelPtr OPTIONAL;
} DOS_RW_REQUEST, *PDOS_RW_REQUEST;

typedef struct _DOS_PEEK_REQUEST
{
    DOS_REQUEST_HEADER Header;
    OUT BYTE Character;
} DOS_PEEK_REQUEST, *PDOS_PEEK_REQUEST;

typedef struct _DOS_OUTPUT_BUSY_REQUEST
{
    DOS_REQUEST_HEADER Header;

    IN     DWORD BufferPointer;
    IN OUT WORD  Length;
} DOS_OUTPUT_BUSY_REQUEST, *PDOS_OUTPUT_BUSY_REQUEST;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

PDOS_DEVICE_NODE DosGetDriverNode(DWORD Driver);
PDOS_DEVICE_NODE DosGetDevice(LPCSTR DeviceName);
PDOS_DEVICE_NODE DosCreateDevice(WORD Attributes, PCHAR DeviceName);
PDOS_DEVICE_NODE DosCreateDeviceEx
(
    WORD Attributes,
    PCHAR DeviceName,
    WORD PrivateDataSize
);
VOID DosDeleteDevice(PDOS_DEVICE_NODE DeviceNode);
VOID DeviceStrategyBop(VOID);
VOID DeviceInterruptBop(VOID);
DWORD DosLoadDriver(LPCSTR DriverFile);

#endif // _DEVICE_H_

/* EOF */
