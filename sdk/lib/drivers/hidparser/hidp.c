/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser
 * COPYRIGHT:   Copyright  Michael Martin <michael.martin@reactos.org>
 *              Copyright  Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#include "hidparser.h"

#define NDEBUG
#include <debug.h>

/* hidpi caps */

HIDAPI
NTSTATUS
NTAPI
HidP_GetCaps(
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    OUT PHIDP_CAPS  Capabilities)
{
    return HidParser_GetCaps(PreparsedData, Capabilities);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetValueCaps (
    HIDP_REPORT_TYPE ReportType,
    PHIDP_VALUE_CAPS ValueCaps,
    PUSHORT ValueCapsLength,
    PHIDP_PREPARSED_DATA PreparsedData)
{
    return HidParser_GetSpecificValueCaps(PreparsedData,
                                          ReportType,
                                          HID_USAGE_PAGE_UNDEFINED,
                                          HIDP_LINK_COLLECTION_UNSPECIFIED,
                                          HID_USAGE_PAGE_UNDEFINED,
                                          ValueCaps,
                                          ValueCapsLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetSpecificValueCaps (
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PHIDP_VALUE_CAPS  ValueCaps,
    IN OUT PUSHORT  ValueCapsLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    // get value caps
    return HidParser_GetSpecificValueCaps (PreparsedData, ReportType, UsagePage, LinkCollection, Usage, ValueCaps, ValueCapsLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetButtonCaps (
    HIDP_REPORT_TYPE ReportType,
    PHIDP_BUTTON_CAPS ButtonCaps,
    PUSHORT ButtonCapsLength,
    PHIDP_PREPARSED_DATA PreparsedData)
{
    return HidParser_GetSpecificButtonCaps(PreparsedData,
                                           ReportType,
                                           HID_USAGE_PAGE_UNDEFINED,
                                           0,
                                           HID_USAGE_PAGE_UNDEFINED,
                                           ButtonCaps,
                                           ButtonCapsLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetSpecificButtonCaps (
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN USAGE Usage,
    OUT PHIDP_BUTTON_CAPS ButtonCaps,
    IN OUT PUSHORT ButtonCapsLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    return HidParser_GetSpecificButtonCaps(PreparsedData, ReportType, UsagePage, LinkCollection, Usage, ButtonCaps, ButtonCapsLength);
}

HIDAPI
ULONG
NTAPI
HidP_MaxUsageListLength(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage  OPTIONAL,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    return HidParser_MaxUsageListLength(PreparsedData, ReportType, UsagePage);
}

HIDAPI
ULONG
NTAPI
HidP_MaxDataListLength (
    IN HIDP_REPORT_TYPE  ReportType,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    return HidParser_MaxDataListLength(PreparsedData, ReportType);
}

/* hidpi helpers */

HIDAPI
NTSTATUS
NTAPI
HidP_UsageListDifference(
    IN PUSAGE  PreviousUsageList,
    IN PUSAGE  CurrentUsageList,
    OUT PUSAGE  BreakUsageList,
    OUT PUSAGE  MakeUsageList,
    IN ULONG  UsageListLength)
{
    return HidParser_UsageListDifference(PreviousUsageList, CurrentUsageList, BreakUsageList, MakeUsageList, UsageListLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_TranslateUsagesToI8042ScanCodes (
    IN PUSAGE  ChangedUsageList,
    IN ULONG  UsageListLength,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext)
{
    UNIMPLEMENTED;
    ASSERT (FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

/* hidpi get */

HIDAPI
NTSTATUS
NTAPI
HidP_GetUsages(
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection  OPTIONAL,
    OUT PUSAGE UsageList,
    IN OUT PULONG UsageLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN PCHAR Report,
    IN ULONG ReportLength)
{
    if (UsagePage == HID_USAGE_PAGE_UNDEFINED)
    {
        // wierd and unsecure behavoiur
        return HidParser_GetUsagesEx(PreparsedData,
                                     ReportType,
                                     LinkCollection,
                                     (PUSAGE_AND_PAGE)UsageList,
                                     UsageLength,
                                     Report,
                                     ReportLength);
    }
    else
    {
        return HidParser_GetUsages(PreparsedData,
                                   ReportType,
                                   UsagePage,
                                   LinkCollection,
                                   UsageList,
                                   UsageLength,
                                   Report,
                                   ReportLength);
    }
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetUsagesEx(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT  LinkCollection,
    OUT PUSAGE_AND_PAGE  ButtonList,
    IN OUT ULONG  *UsageLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN PCHAR  Report,
    IN ULONG  ReportLength)
{
    return HidParser_GetUsagesEx(PreparsedData, ReportType, LinkCollection, ButtonList, UsageLength, Report, ReportLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetScaledUsageValue(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    IN USAGE  Usage,
    OUT PLONG  UsageValue,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN PCHAR  Report,
    IN ULONG  ReportLength)
{
    return HidParser_GetScaledUsageValue(PreparsedData, ReportType, UsagePage, LinkCollection, Usage, UsageValue, Report, ReportLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetUsageValue(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PULONG  UsageValue,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN PCHAR  Report,
    IN ULONG  ReportLength)
{
    return HidParser_GetUsageValue(PreparsedData, ReportType, UsagePage, LinkCollection, Usage, UsageValue, Report, ReportLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetData(
    IN HIDP_REPORT_TYPE  ReportType,
    OUT PHIDP_DATA  DataList,
    IN OUT PULONG  DataLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN PCHAR  Report,
    IN ULONG  ReportLength)
{
    return HidParser_GetData(PreparsedData,
                             ReportType,
                             DataList,
                             DataLength,
                             Report,
                             ReportLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetExtendedAttributes(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT DataIndex,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    OUT PHIDP_EXTENDED_ATTRIBUTES  Attributes,
    IN OUT PULONG  LengthAttributes)
{
    return HidParser_GetExtendedAttributes(PreparsedData,
                                           ReportType,
                                           DataIndex,
                                           Attributes,
                                           LengthAttributes);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetLinkCollectionNodes(
    OUT PHIDP_LINK_COLLECTION_NODE  LinkCollectionNodes,
    IN OUT PULONG  LinkCollectionNodesLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    return HidParser_GetLinkCollectionNodes(PreparsedData,
                                            LinkCollectionNodes,
                                            LinkCollectionNodesLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetUsageValueArray(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    IN USAGE  Usage,
    OUT PCHAR  UsageValue,
    IN USHORT  UsageValueByteLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN PCHAR  Report,
    IN ULONG  ReportLength)
{
    return HidParser_GetUsageValueArray(PreparsedData,
                                        ReportType,
                                        UsagePage,
                                        LinkCollection,
                                        Usage,
                                        UsageValue,
                                        UsageValueByteLength,
                                        Report,
                                        ReportLength);
}

/* hidpi set */

HIDAPI
NTSTATUS
NTAPI
HidP_UnsetUsages(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN PUSAGE  UsageList,
    IN OUT PULONG  UsageLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_SetUsages(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN PUSAGE  UsageList,
    IN OUT PULONG  UsageLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_SetUsageValueArray(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    IN USAGE  Usage,
    IN PCHAR  UsageValue,
    IN USHORT  UsageValueByteLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    OUT PCHAR  Report,
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_SetUsageValue(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    IN ULONG  UsageValue,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_SetScaledUsageValue(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    IN USAGE  Usage,
    IN LONG  UsageValue,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_SetData(
    IN HIDP_REPORT_TYPE  ReportType,
    IN PHIDP_DATA  DataList,
    IN OUT PULONG  DataLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_InitializeReportForID(
    IN HIDP_REPORT_TYPE  ReportType,
    IN UCHAR  ReportID,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}
