/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            himem.h
 * PURPOSE:         DOS XMS Driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* DEFINITIONS ****************************************************************/

#define XMS_ADDRESS     0x110000
#define XMS_BLOCKS      0x3BC0
#define XMS_BLOCK_SIZE  1024
#define XMS_MAX_HANDLES 16

#define XMS_STATUS_SUCCESS         0x00
#define XMS_STATUS_NOT_IMPLEMENTED 0x80
#define XMS_STATUS_A20_ERROR       0x82
#define XMS_STATUS_HMA_IN_USE      0x91
#define XMS_STATUS_OUT_OF_MEMORY   0xA0
#define XMS_STATUS_OUT_OF_HANDLES  0xA1
#define XMS_STATUS_INVALID_HANDLE  0xA2
#define XMS_STATUS_BAD_SRC_HANDLE  0xA3
#define XMS_STATUS_BAD_DEST_HANDLE 0xA4
#define XMS_STATUS_BAD_SRC_OFFSET  0xA5
#define XMS_STATUS_BAD_DEST_OFFSET 0xA6
#define XMS_STATUS_NOT_LOCKED      0xAA
#define XMS_STATUS_LOCKED          0xAB
#define XMS_STATUS_LOCK_OVERFLOW   0xAC
#define XMS_STATUS_CANNOT_LOCK     0xAD

typedef struct _XMS_HANDLE
{
    BYTE Handle;
    BYTE LockCount;
    WORD Size;
    DWORD Address;
} XMS_HANDLE, *PXMS_HANDLE;

#pragma pack(push, 1)
typedef struct _XMS_COPY_DATA
{
    DWORD Count;
    WORD SourceHandle;
    DWORD SourceOffset;
    WORD DestHandle;
    DWORD DestOffset;
} XMS_COPY_DATA, *PXMS_COPY_DATA;
#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

BOOLEAN XmsGetDriverEntry(PDWORD Pointer);
VOID XmsInitialize(VOID);
VOID XmsCleanup(VOID);
