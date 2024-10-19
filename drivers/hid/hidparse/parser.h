/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser internal structures
 * COPYRIGHT:   Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#ifndef _HIDPARSER_H_
#define _HIDPARSER_H_

#include <wdm.h>
#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
#include <hidpddi.h>
#include <preparsed.h>

/* HID report related structures */

typedef enum _ITEM_SIZE
{
    ITEM_SIZE_0 = 0,
    ITEM_SIZE_1 = 1,
    ITEM_SIZE_2 = 2,
    ITEM_SIZE_4 = 3
} ITEM_SIZE;

typedef enum _ITEM_TYPE
{
    ITEM_TYPE_MAIN = 0,
    ITEM_TYPE_GLOBAL = 1,
    ITEM_TYPE_LOCAL = 2,
    ITEM_TYPE_RESERVED = 3
} ITEM_TYPE;

typedef enum _MAIN_ITEM_TYPE
{
    MAIN_ITEM_TAG_INPUT = 0x8,
    MAIN_ITEM_TAG_OUTPUT = 0x9,
    MAIN_ITEM_TAG_FEATURE = 0xb,
    MAIN_ITEM_TAG_COLLECTION = 0xa,
    MAIN_ITEM_TAG_END_COLLECTION = 0xc,
    MAIN_ITEM_TAG_END_RESERVED1 = 0xd,
    MAIN_ITEM_TAG_END_RESERVED2 = 0xf
} MAIN_ITEM_TYPE;

typedef enum _GLOBAL_ITEM_TYPE
{
    GLOBAL_ITEM_TAG_USAGE_PAGE        = 0x0,
    GLOBAL_ITEM_TAG_LOGICAL_MINIMUM   = 0x1,
    GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM   = 0x2,
    GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM  = 0x3,
    GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM  = 0x4,
    GLOBAL_ITEM_TAG_UNIT_EXPONENT     = 0x5,
    GLOBAL_ITEM_TAG_UNIT              = 0x6,
    GLOBAL_ITEM_TAG_REPORT_SIZE       = 0x7,
    GLOBAL_ITEM_TAG_REPORT_ID         = 0x8,
    GLOBAL_ITEM_TAG_REPORT_COUNT      = 0x9,
    GLOBAL_ITEM_TAG_PUSH              = 0xa,
    GLOBAL_ITEM_TAG_POP               = 0xb
} GLOBAL_ITEM_TYPE;

typedef enum _LOCAL_ITEM_TYPE
{
    ITEM_TAG_LOCAL_USAGE              = 0x0,
    ITEM_TAG_LOCAL_USAGE_MINIMUM      = 0x1,
    ITEM_TAG_LOCAL_USAGE_MAXIMUM      = 0x2,
    ITEM_TAG_LOCAL_DESIGNATOR_INDEX   = 0x3,
    ITEM_TAG_LOCAL_DESIGNATOR_MINIMUM = 0x4,
    ITEM_TAG_LOCAL_DESIGNATOR_MAXIMUM = 0x5,
    ITEM_TAG_LOCAL_STRING_INDEX       = 0x7,
    ITEM_TAG_LOCAL_STRING_MINIMUM     = 0x8,
    ITEM_TAG_LOCAL_STRING_MAXIMUM     = 0x9,
    ITEM_TAG_LOCAL_DELIMITER          = 0xa
} LOCAL_ITEM_TYPE;

#define ITEM_TAG_LONG           0xf

#define REPORT_ITEM_PREFIX_SIZE 1

#include <pshpack1.h>
typedef struct _REPORT_ITEM
{
    union
    {
        struct
        {
            UCHAR Size : 2;
            UCHAR Type : 2;
            UCHAR Tag : 4;
        };
        UCHAR RawTag;
    };
    union
    {
        struct {
            UCHAR Value;
            UCHAR Unused[3];
        } Short8;
        struct {
            USHORT Value;
            USHORT Unused;
        } Short16;
        struct {
            ULONG Value;
        } Short32;
        struct {
            UCHAR LongDataSize;
            UCHAR LongItemTag;
            UCHAR Value[1];
        } Long;
    } Data;
} REPORT_ITEM, * PREPORT_ITEM;
#include <poppack.h>

/* Parser internal strustures */

typedef union _ITEM_DATA
{
    struct
    {
        UCHAR Value;
        UCHAR Value2;
        UCHAR Value3;
        UCHAR Value4;
    } UData8;
    struct
    {
        USHORT Value;
        USHORT Value2;
    } UData16;
    struct
    {
        ULONG Value;
    } UData32;
    struct
    {
        ULONG Offset;
        CHAR Value;
        CHAR Value2;
        CHAR Value3;
        CHAR Value4;
    } SData8;
    struct
    {
        ULONG Offset;
        SHORT Value;
        SHORT Value2;
    } SData16;
    struct
    {
        ULONG Offset;
        LONG Value;
    } SData32;
    struct
    {
        ULONG UData;
        LONG SData;
    } Raw;
} ITEM_DATA;

typedef struct _HIDPARSER_REPORT_IDS_STACK
{
    HIDP_REPORT_IDS Value;
    struct _HIDPARSER_REPORT_IDS_STACK* Next;
} HIDPARSER_REPORT_IDS_STACK, * PHIDPARSER_REPORT_IDS_STACK;

typedef struct _HIDPARSER_COLLECTION_STACK
{
    ULONG Index;
    ULONG PreparsedSize;
    PHIDPARSER_PREPARSED_DATA PreparsedData;
    struct _HIDPARSER_COLLECTION_STACK* Next;
} HIDPARSER_COLLECTION_STACK, * PHIDPARSER_COLLECTION_STACK;

typedef struct _HIDPARSER_GLOBAL_STACK {
    USHORT UsagePage;
    UCHAR ReportID;
    UCHAR UnknownGlobalCount;
    USHORT ReportCount;
    USHORT ReportSize;
    LONG LogicalMin;
    LONG LogicalMax;
    LONG PhysicalMin;
    LONG PhysicalMax;
    LONG Units;
    LONG UnitsExp;
    HIDP_UNKNOWN_TOKEN UnknownGlobals[4];
    struct _HIDPARSER_GLOBAL_STACK* Next;
} HIDPARSER_GLOBAL_STACK, * PHIDPARSER_GLOBAL_STACK;

typedef struct _HIDPARSER_VALUE_CAPS_LIST {
    LIST_ENTRY Entry;
    HIDPARSER_VALUE_CAPS Value;
    HIDP_REPORT_TYPE ReportType;
} HIDPARSER_VALUE_CAPS_LIST, * PHIDPARSER_VALUE_CAPS_LIST;

typedef struct _HIDPARSER_NODES_LIST {
    LIST_ENTRY Entry;
    HIDPARSER_LINK_COLLECTION_NODE Value;
    UINT32 Index;
    UINT32 Depth;
} HIDPARSER_NODES_LIST, * PHIDPARSER_NODES_LIST;

NTSTATUS
NTAPI
HidParser_GetCollectionDescription(
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription);

#endif /* _HIDPARSER_H_ */
