/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * FILE:        lib/drivers/hidparser/preparsed.h
 * PURPOSE:     HID Parser preparsedData definition
 * COPYRIGHT:   Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#include <ntdef.h>
#include <ntstatus.h>
#include <hidusage.h>
#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
#include <hidpi.h>

extern char PreparsedMagic[8];

#ifndef _HIDPREPARSED_H_
#define _HIDPREPARSED_H_

typedef union _HIDPARSER_ITEM_FLAGS
{
    struct
    {
        USHORT IsConstant : 1;
        USHORT IsVariable : 1;
        USHORT IsRelative : 1;
        USHORT Wrap : 1;
        USHORT IsNonLinear : 1;
        USHORT NoPreferred : 1;
        USHORT NullState : 1;
        USHORT IsVolatile : 1;
        USHORT BitsBytes : 1;
        UCHAR Reserved[2];
    } Flags;
    ULONG Raw;
} HIDPARSER_ITEM_FLAGS, * PHIDPARSER_ITEM_FLAGS;

typedef struct _HIDPARSER_VALUE_FLAG
{
    ULONG ArrayHasMore : 1;       // 0x1
    ULONG IsConstant : 1;         // 0x2
    ULONG IsButton : 1;           // 0x4
    ULONG IsAbsolute : 1;         // 0x8
    ULONG IsRange : 1;            // 0x10
    ULONG Unknown : 1;            // 0x20
    ULONG IsStringRange : 1;      // 0x40
    ULONG IsDesignatorRange : 1;  // 0x80
    ULONG Reserved : 20;          // 0x100-0x8000000
    ULONG UnknownGlobalCount : 3; // 0x10000000-0x40000000
} HIDPARSER_VALUE_FLAG, * PHIDPARSER_VALUE_FLAG;

typedef struct _HIDPARSER_LINK_COLLECTION_NODE
{
    USAGE  LinkUsage;
    USAGE  LinkUsagePage;
    USHORT Parent;
    USHORT NumberOfChildren;
    USHORT NextSibling;
    USHORT FirstChild;
    ULONG  CollectionType : 8;
    ULONG  IsAlias : 1;
    ULONG  Reserved : 23;
} HIDPARSER_LINK_COLLECTION_NODE, * PHIDPARSER_LINK_COLLECTION_NODE;

typedef struct _HIDPARSER_VALUE_CAPS
{
    USHORT UsagePage;
    UCHAR ReportID;
    UCHAR StartBit;
    USHORT ReportSize;
    USHORT ReportCount;
    USHORT StartByte;
    USHORT TotalBits;
    HIDPARSER_ITEM_FLAGS BitField;
    USHORT EndByte;
    USHORT LinkCollection;
    USAGE LinkUsagePage;
    USAGE LinkUsage;
    HIDPARSER_VALUE_FLAG Flags;
    HIDP_UNKNOWN_TOKEN UnknownGlobals[4];
    USAGE UsageMin;
    USAGE UsageMax;
    USHORT StringMin;
    USHORT StringMax;
    USHORT DesignatorMin;
    USHORT DesignatorMax;
    USHORT DataIndexMin;
    USHORT DataIndexMax;
    union _LOGICAL
    {
        struct _BUTTON
        {
            LONG Min;
            LONG Max;
        } Button;
        struct _VALUE
        {
            USHORT NullValue;
            USHORT Offset;
            LONG Min;
            LONG Max;
        } Value;
    } Logical;
    LONG PhysicalMin;
    LONG PhysicalMax;
    LONG Units;
    LONG UnitsExp;
} HIDPARSER_VALUE_CAPS, * PHIDPARSER_VALUE_CAPS;

typedef struct _HIDP_PREPARSED_DATA
{
    CHAR Magic[8];
    USAGE Usage;
    USAGE UsagePage;
    USHORT Reserved[2];
    USHORT InputCapsStart;
    USHORT InputCapsCount;
    USHORT InputCapsEnd;
    USHORT InputReportByteLength;
    USHORT OutputCapsStart;
    USHORT OutputCapsCount;
    USHORT OutputCapsEnd;
    USHORT OutputReportByteLength;
    USHORT FeatureCapsStart;
    USHORT FeatureCapsCount;
    USHORT FeatureCapsEnd;
    USHORT FeatureReportByteLength;
    USHORT CapsByteLength;
    USHORT LinkCollectionCount;
    HIDPARSER_VALUE_CAPS ValueCaps[1];
    /* HIDPARSER_LINK_COLLECTION_NODE nodes[1] */
} HIDPARSER_PREPARSED_DATA, * PHIDPARSER_PREPARSED_DATA;

#endif /* _HIDPREPARSED_H_ */
