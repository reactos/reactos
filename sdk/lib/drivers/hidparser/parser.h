#ifndef _HIDPARSER_H_
#define _HIDPARSER_H_

#include <wdm.h>
#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
#include <hidpddi.h>

#include "hidparser.h"
#include "hidp.h"

 /*
  * Copyright 2007, Haiku, Inc. All Rights Reserved.
  * Distributed under the terms of the MIT License.
  */

#define HID_REPORT_TYPE_ANY			0x07


#define ITEM_TYPE_MAIN						0x0
#define ITEM_TYPE_GLOBAL					0x1
#define ITEM_TYPE_LOCAL						0x2
#define ITEM_TYPE_LONG						0x3

#define ITEM_TAG_MAIN_INPUT					0x8
#define ITEM_TAG_MAIN_OUTPUT				0x9
#define ITEM_TAG_MAIN_FEATURE				0xb
#define ITEM_TAG_MAIN_COLLECTION			0xa
#define ITEM_TAG_MAIN_END_COLLECTION		0xc

#define ITEM_TAG_GLOBAL_USAGE_PAGE			0x0
#define ITEM_TAG_GLOBAL_LOGICAL_MINIMUM		0x1
#define ITEM_TAG_GLOBAL_LOGICAL_MAXIMUM		0x2
#define ITEM_TAG_GLOBAL_PHYSICAL_MINIMUM	0x3
#define ITEM_TAG_GLOBAL_PHYSICAL_MAXIMUM	0x4
#define ITEM_TAG_GLOBAL_UNIT_EXPONENT		0x5
#define ITEM_TAG_GLOBAL_UNIT				0x6
#define ITEM_TAG_GLOBAL_REPORT_SIZE			0x7
#define ITEM_TAG_GLOBAL_REPORT_ID			0x8
#define ITEM_TAG_GLOBAL_REPORT_COUNT		0x9
#define ITEM_TAG_GLOBAL_PUSH				0xa
#define ITEM_TAG_GLOBAL_POP					0xb

#define ITEM_TAG_LOCAL_USAGE				0x0
#define ITEM_TAG_LOCAL_USAGE_MINIMUM		0x1
#define ITEM_TAG_LOCAL_USAGE_MAXIMUM		0x2
#define ITEM_TAG_LOCAL_DESIGNATOR_INDEX		0x3
#define ITEM_TAG_LOCAL_DESIGNATOR_MINIMUM	0x4
#define ITEM_TAG_LOCAL_DESIGNATOR_MAXIMUM	0x5
#define ITEM_TAG_LOCAL_STRING_INDEX			0x7
#define ITEM_TAG_LOCAL_STRING_MINIMUM		0x8
#define ITEM_TAG_LOCAL_STRING_MAXIMUM		0x9
#define ITEM_TAG_LOCAL_DELIMITER			0xa

#define ITEM_TAG_LONG						0xf

#define COLLECTION_PHYSICAL					0x00
#define COLLECTION_APPLICATION				0x01
#define COLLECTION_LOGICAL					0x02
#define COLLECTION_REPORT					0x03
#define COLLECTION_NAMED_ARRAY				0x04
#define COLLECTION_USAGE_SWITCH				0x05
#define COLLECTION_USAGE_MODIFIER			0x06
#define COLLECTION_ALL						0xff

#define UNIT_SYSTEM							0x0
#define UNIT_LENGTH							0x1
#define UNIT_MASS							0x2
#define UNIT_TIME							0x3
#define UNIT_TEMPERATURE					0x4
#define UNIT_CURRENT						0x5
#define UNIT_LUMINOUS_INTENSITY				0x6

#define USAGE_PAGE_SHIFT					16
#define USAGE_PAGE_MASK						0xffff
#define USAGE_ID_SHIFT						0
#define USAGE_ID_MASK						0xffff

typedef struct
{
    UCHAR Size:2;
    UCHAR Type:2;
    UCHAR Tag:4;
}ITEM_PREFIX, *PITEM_PREFIX;

#include <pshpack1.h>
typedef struct
{
    ITEM_PREFIX Prefix;

    union
    {
        UCHAR UData8[4];
        CHAR  SData8[4];
        USHORT UData16[2];
        SHORT SData16[2];
        ULONG UData32;
        LONG SData32;
    }Data;

}SHORT_ITEM, *PSHORT_ITEM;
#include <poppack.h>

typedef struct
{
    ITEM_PREFIX Prefix;
    UCHAR DataSize;
    UCHAR LongItemTag;
    UCHAR Data[0];

}LONG_ITEM,*PLONG_ITEM;


#define  LBITFIELD9(b1,b2,b3,b4,b5,b6,b7,b8,b9)    USHORT b9,b8,b7,b6,b5,b4,b3,b2,b1
typedef struct
{
    USHORT DataConstant:1;
    USHORT ArrayVariable:1;
    USHORT Relative:1;
    USHORT Wrap:1;
    USHORT NonLinear:1;
    USHORT NoPreferred:1;
    USHORT NullState:1;
    USHORT IsVolatile:1;
    USHORT BitsBytes:1;
    UCHAR    reserved[2];

}MAIN_ITEM_DATA, *PMAIN_ITEM_DATA;

typedef struct __GLOBAL_ITEM_STATE_
{
    USHORT UsagePage;
    ULONG  LogicalMinimum;
    ULONG  LogicialMaximum;
    ULONG  PhysicalMinimum;
    ULONG  PhysicalMaximum;
    UCHAR  UnitExponent;
    UCHAR  Unit;
    ULONG  ReportSize;
    ULONG  ReportCount;
    UCHAR  ReportId;
    struct __GLOBAL_ITEM_STATE__ * Next;
}GLOBAL_ITEM_STATE, *PGLOBAL_ITEM_STATE;


typedef struct usage_value
{
    union
    {
        struct {
            USHORT UsageId;
            USHORT UsagePage;
        }s;
        ULONG  Extended;
    }u;

   UCHAR IsExtended;
}USAGE_VALUE, *PUSAGE_VALUE;


typedef struct
{
    PUSAGE_VALUE    UsageStack;
    ULONG           UsageStackUsed;
    ULONG           UsageStackAllocated;

    USAGE_VALUE     UsageMinimum;
    USAGE_VALUE     UsageMaximum;

    UCHAR           UsageMinimumSet;
    UCHAR           UsageMaximumSet;

    ULONG           DesignatorIndex;
    UCHAR           DesignatorIndexSet;

    ULONG           DesignatorMinimum;
    ULONG           DesignatorMaximum;

    UCHAR           StringIndex;
    UCHAR           StringIndexSet;
    UCHAR           StringMinimum;
    UCHAR           StringMaximum;

}LOCAL_ITEM_STATE, *PLOCAL_ITEM_STATE;

typedef struct
{
    ULONG ByteOffset;
    UCHAR Shift;
    ULONG Mask;
    UCHAR BitCount;
    UCHAR HasData;
    UCHAR Array;
    UCHAR Relative;
    ULONG Minimum;
    ULONG Maximum;
    ULONG UsageMinimum;
    ULONG UsageMaximum;
    ULONG Data;
    UCHAR Valid;
}HID_REPORT_ITEM, *PHID_REPORT_ITEM;

struct _HID_REPORT;

typedef struct __HID_COLLECTION__
{
    UCHAR Type;
    ULONG Usage;
    UCHAR StringID;
    UCHAR PhysicalID;
    ULONG ReportCount;
    ULONG NodeCount;

    struct __HID_COLLECTION__ ** Nodes;
    struct __HID_COLLECTION__ * Root;
    struct _HID_REPORT ** Reports;

    ULONG Offsets[1];

}HID_COLLECTION, *PHID_COLLECTION;

typedef struct _HID_REPORT
{
    UCHAR Type;
    UCHAR ReportID;
    ULONG ReportSize;
    ULONG ItemCount;
    ULONG ItemAllocated;
    HID_REPORT_ITEM Items[1];
}HID_REPORT, *PHID_REPORT;

typedef struct
{
    //
    // global item state
    //
    GLOBAL_ITEM_STATE GlobalItemState;

    //
    // local item state
    //
    LOCAL_ITEM_STATE LocalItemState;

    //
    // root collection
    //
    PHID_COLLECTION RootCollection;

    //
    // uses report ids
    //
    UCHAR UseReportIDs;

    //
    // collection index
    //
    ULONG CollectionIndex;

}HID_PARSER_CONTEXT, *PHID_PARSER_CONTEXT;

#define HID_REPORT_TYPE_INPUT		0x01
#define HID_REPORT_TYPE_OUTPUT		0x02
#define HID_REPORT_TYPE_FEATURE		0x04

ULONG
HidParser_UsesReportId(
    IN PVOID CollectionContext,
    IN UCHAR  ReportType);

NTSTATUS
HidParser_GetCollectionUsagePage(
    IN PVOID CollectionContext,
    OUT PUSHORT Usage,
    OUT PUSHORT UsagePage);

ULONG
HidParser_GetReportLength(
    IN PVOID CollectionContext,
    IN UCHAR ReportType);

ULONG
HidParser_GetReportItemCountFromReportType(
    IN PVOID CollectionContext,
    IN UCHAR ReportType);

ULONG
HidParser_GetReportItemTypeCountFromReportType(
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN ULONG bData);

ULONG
HidParser_GetMaxUsageListLengthWithReportAndPage(
    IN PVOID CollectionContext,
    IN UCHAR  ReportType,
    IN USAGE  UsagePage  OPTIONAL);

NTSTATUS
HidParser_GetSpecificValueCapsWithReport(
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USHORT UsagePage,
    IN USHORT Usage,
    OUT PHIDP_VALUE_CAPS  ValueCaps,
    IN OUT PUSHORT  ValueCapsLength);


NTSTATUS
HidParser_GetUsagesWithReport(
    IN PVOID CollectionContext,
    IN UCHAR  ReportType,
    IN USAGE  UsagePage,
    OUT USAGE  *UsageList,
    IN OUT PULONG UsageLength,
    IN PCHAR  ReportDescriptor,
    IN ULONG  ReportDescriptorLength);

NTSTATUS
HidParser_GetScaledUsageValueWithReport(
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USAGE UsagePage,
    IN USAGE  Usage,
    OUT PLONG UsageValue,
    IN PCHAR ReportDescriptor,
    IN ULONG ReportDescriptorLength);

NTSTATUS
HidParser_GetUsageValueWithReport(
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USAGE UsagePage,
    IN USAGE  Usage,
    OUT PULONG UsageValue,
    IN PCHAR ReportDescriptor,
    IN ULONG ReportDescriptorLength);

/* parser.c */

NTSTATUS
HidParser_BuildContext(
    IN PVOID ParserContext,
    IN ULONG CollectionIndex,
    IN ULONG ContextSize,
    OUT PVOID *CollectionContext);

ULONG
HidParser_CalculateContextSize(
    IN PHID_COLLECTION Collection);

NTSTATUS
HidParser_ParseReportDescriptor(
    PUCHAR Report,
    ULONG ReportSize,
    OUT PVOID *ParserContext);

ULONG
HidParser_NumberOfTopCollections(
    IN PVOID ParserContext);

ULONG
HidParser_GetContextSize(
    IN PVOID ParserContext,
    IN ULONG CollectionNumber);


/* context.c */

PHID_COLLECTION
HidParser_GetCollectionFromContext(
    IN PVOID Context);

ULONG
HidParser_GetTotalCollectionCount(
    IN PVOID CollectionContext);

NTSTATUS
HidParser_BuildCollectionContext(
    IN PHID_COLLECTION RootCollection,
    IN PVOID Context,
    IN ULONG ContextSize);

PHID_REPORT
HidParser_GetReportInCollection(
    IN PVOID Context,
    IN UCHAR ReportType);

#endif /* _HIDPARSER_H_ */
