/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ems.h
 * PURPOSE:         Expanded Memory Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _EMS_H_
#define _EMS_H_

/* DEFINITIONS ****************************************************************/

#define EMS_INTERRUPT_NUM   0x67
#define EMS_SEGMENT         0xD000
#define EMS_MAX_HANDLES     16
#define EMS_TOTAL_PAGES     256
#define EMS_PAGE_BITS       14
#define EMS_PAGE_SIZE       (1 << EMS_PAGE_BITS)
#define EMS_ADDRESS         0xA00000
#define EMS_PHYSICAL_PAGES  4

#define EMS_STATUS_OK                   0x00
#define EMS_STATUS_INTERNAL_ERROR       0x80
#define EMS_STATUS_INVALID_HANDLE       0x83
#define EMS_STATUS_NO_MORE_HANDLES      0x85
#define EMS_STATUS_INSUFFICIENT_PAGES   0x88
#define EMS_STATUS_ZERO_PAGES           0x89
#define EMS_STATUS_INV_LOGICAL_PAGE     0x8A
#define EMS_STATUS_INV_PHYSICAL_PAGE    0x8B
#define EMS_STATUS_UNKNOWN_FUNCTION     0x8F

#define ARRAY_INDEX(ptr, array) ((ULONG)(((ULONG_PTR)(ptr) - (ULONG_PTR)(array)) / sizeof(*array)))

typedef struct _EMS_HANDLE
{
    BOOLEAN Allocated;
    USHORT PageCount;
    LIST_ENTRY PageList;
} EMS_HANDLE, *PEMS_HANDLE;

typedef struct _EMS_PAGE
{
    LIST_ENTRY Entry;
    USHORT Handle;
} EMS_PAGE, *PEMS_PAGE;

#pragma pack(push, 1)

typedef struct _EMS_COPY_DATA
{
    ULONG RegionLength;
    UCHAR SourceType;
    USHORT SourceHandle;
    USHORT SourceOffset;
    USHORT SourceSegment;
    UCHAR DestType;
    USHORT DestHandle;
    USHORT DestOffset;
    USHORT DestSegment;
} EMS_COPY_DATA, *PEMS_COPY_DATA;

#pragma pack(pop)

/* FUNCTIONS ******************************************************************/

VOID EmsInitialize(VOID);
VOID EmsCleanup(VOID);

#endif // _EMS_H_

/* EOF */
