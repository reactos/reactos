/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/hidparse/hidparse.c
 * PURPOSE:     HID Parser
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "hidparse.h"

#define NDEBUG
#include <debug.h>

PVOID
NTAPI
AllocFunction(
    IN ULONG ItemSize)
{
    PVOID Item = ExAllocatePoolWithTag(NonPagedPool, ItemSize, HIDPARSE_TAG);
    if (Item)
    {
        //
        // zero item
        //
        RtlZeroMemory(Item, ItemSize);
    }

    //
    // done
    //
    return Item;
}

VOID
NTAPI
FreeFunction(
    IN PVOID Item)
{
    //
    // free item
    //
    ExFreePoolWithTag(Item, HIDPARSE_TAG);
}

VOID
NTAPI
ZeroFunction(
    IN PVOID Item,
    IN ULONG ItemSize)
{
    //
    // zero item
    //
    RtlZeroMemory(Item, ItemSize);
}

VOID
NTAPI
CopyFunction(
    IN PVOID Target,
    IN PVOID Source,
    IN ULONG Length)
{
    //
    // copy item
    //
    RtlCopyMemory(Target, Source, Length);
}

VOID
__cdecl
DebugFunction(
    IN LPCSTR FormatStr, ...)
{
#if HID_DBG
    va_list args;
    char printbuffer[1024];

    va_start(args, FormatStr);
    vsprintf(printbuffer, FormatStr, args);
    va_end(args);

    DbgPrint(printbuffer);
#endif
}

VOID
NTAPI
HidP_FreeCollectionDescription(
    IN PHIDP_DEVICE_DESC   DeviceDescription)
{
    HID_PARSER Parser;

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);

    //
    // free collection
    //
    HidParser_FreeCollectionDescription(&Parser, DeviceDescription);
}


HIDAPI
NTSTATUS
NTAPI
HidP_GetCaps(
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    OUT PHIDP_CAPS  Capabilities)
{
    HID_PARSER Parser;

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);

    //
    // get caps
    //
    return HidParser_GetCaps(&Parser, PreparsedData, Capabilities);
}

NTSTATUS
TranslateStatusForUpperLayer(
    IN HIDPARSER_STATUS Status)
{
    //
    // now we are handling only this values, for others just return
    // status as it is.
    //
    switch (Status)
    {
    case HIDPARSER_STATUS_INSUFFICIENT_RESOURCES:
        return STATUS_INSUFFICIENT_RESOURCES;
    case HIDPARSER_STATUS_INVALID_REPORT_TYPE:
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    case HIDPARSER_STATUS_BUFFER_TOO_SMALL:
        return STATUS_BUFFER_TOO_SMALL;
    case HIDPARSER_STATUS_COLLECTION_NOT_FOUND:
        return STATUS_NO_DATA_DETECTED;
    default:
        return Status;
    }
}

NTSTATUS
NTAPI
HidP_GetCollectionDescription(
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription)
{
    HID_PARSER Parser;
    NTSTATUS Status;

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);

    //
    // get description;
    //
    Status = HidParser_GetCollectionDescription(&Parser, ReportDesc, DescLength, PoolType, DeviceDescription);
    return TranslateStatusForUpperLayer(Status);
}

HIDAPI
ULONG
NTAPI
HidP_MaxUsageListLength(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage  OPTIONAL,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    HID_PARSER Parser;

    //
    // sanity check
    //
    ASSERT(ReportType == HidP_Input || ReportType == HidP_Output || ReportType == HidP_Feature);

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);


    //
    // get usage length
    //
    return HidParser_MaxUsageListLength(&Parser, PreparsedData, ReportType, UsagePage);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetSpecificValueCaps(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PHIDP_VALUE_CAPS  ValueCaps,
    IN OUT PUSHORT  ValueCapsLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    HID_PARSER Parser;

    //
    // sanity check
    //
    ASSERT(ReportType == HidP_Input || ReportType == HidP_Output || ReportType == HidP_Feature);

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);

    //
    // get value caps
    //
    return HidParser_GetSpecificValueCaps(&Parser, PreparsedData, ReportType, UsagePage, LinkCollection, Usage, ValueCaps, ValueCapsLength);
}

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
    HID_PARSER Parser;

    //
    // sanity check
    //
    ASSERT(ReportType == HidP_Input || ReportType == HidP_Output || ReportType == HidP_Feature);

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);

    //
    // get usages
    //
    return HidParser_GetUsages(&Parser, PreparsedData, ReportType, UsagePage, LinkCollection, UsageList, UsageLength, Report, ReportLength);
}


#undef HidP_GetButtonCaps

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
HidP_GetUsagesEx(
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT  LinkCollection,
    OUT PUSAGE_AND_PAGE  ButtonList,
    IN OUT ULONG  *UsageLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    IN PCHAR  Report,
    IN ULONG  ReportLength)
{
    return HidP_GetUsages(ReportType, HID_USAGE_PAGE_UNDEFINED, LinkCollection, &ButtonList->Usage, UsageLength, PreparsedData, Report, ReportLength);
}

HIDAPI
NTSTATUS
NTAPI
HidP_UsageAndPageListDifference(
    IN PUSAGE_AND_PAGE  PreviousUsageList,
    IN PUSAGE_AND_PAGE  CurrentUsageList,
    OUT PUSAGE_AND_PAGE  BreakUsageList,
    OUT PUSAGE_AND_PAGE  MakeUsageList,
    IN ULONG  UsageListLength)
{
    return HidParser_UsageAndPageListDifference(PreviousUsageList, CurrentUsageList, BreakUsageList, MakeUsageList, UsageListLength);
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
    HID_PARSER Parser;

    //
    // sanity check
    //
    ASSERT(ReportType == HidP_Input || ReportType == HidP_Output || ReportType == HidP_Feature);

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);

    //
    // get scaled usage value
    //
    return HidParser_GetScaledUsageValue(&Parser, PreparsedData, ReportType, UsagePage, LinkCollection, Usage, UsageValue, Report, ReportLength);
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
    HID_PARSER Parser;

    //
    // sanity check
    //
    ASSERT(ReportType == HidP_Input || ReportType == HidP_Output || ReportType == HidP_Feature);

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);

    //
    // get scaled usage value
    //
    return HidParser_GetUsageValue(&Parser, PreparsedData, ReportType, UsagePage, LinkCollection, Usage, UsageValue, Report, ReportLength);
}


HIDAPI
NTSTATUS
NTAPI
HidP_TranslateUsageAndPagesToI8042ScanCodes(
    IN PUSAGE_AND_PAGE  ChangedUsageList,
    IN ULONG  UsageListLength,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext)
{
    HID_PARSER Parser;

    //
    // init parser
    //
    HidParser_InitParser(AllocFunction, FreeFunction, ZeroFunction, CopyFunction, DebugFunction, &Parser);

    //
    // translate usage pages
    //
    return HidParser_TranslateUsageAndPagesToI8042ScanCodes(&Parser, ChangedUsageList, UsageListLength, KeyAction, ModifierState, InsertCodesProcedure, InsertCodesContext);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetButtonCaps(
    HIDP_REPORT_TYPE ReportType,
    PHIDP_BUTTON_CAPS ButtonCaps,
    PUSHORT ButtonCapsLength,
    PHIDP_PREPARSED_DATA PreparsedData)
{
    return HidP_GetSpecificButtonCaps(ReportType, HID_USAGE_PAGE_UNDEFINED, 0, 0, ButtonCaps, ButtonCapsLength, PreparsedData);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetSpecificButtonCaps(
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN USAGE Usage,
    OUT PHIDP_BUTTON_CAPS ButtonCaps,
    IN OUT PUSHORT ButtonCapsLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetLinkCollectionNodes(
    OUT PHIDP_LINK_COLLECTION_NODE  LinkCollectionNodes,
    IN OUT PULONG  LinkCollectionNodesLength,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidP_SysPowerEvent(
    IN PCHAR HidPacket,
    IN USHORT HidPacketLength,
    IN PHIDP_PREPARSED_DATA Ppd,
    OUT PULONG OutputBuffer)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidP_SysPowerCaps(
    IN PHIDP_PREPARSED_DATA Ppd,
    OUT PULONG OutputBuffer)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}


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
HidP_TranslateUsagesToI8042ScanCodes(
    IN PUSAGE  ChangedUsageList,
    IN ULONG  UsageListLength,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext)
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
ULONG
NTAPI
HidP_MaxDataListLength(
    IN HIDP_REPORT_TYPE  ReportType,
    IN PHIDP_PREPARSED_DATA  PreparsedData)
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

#undef HidP_GetValueCaps

HIDAPI
NTSTATUS
NTAPI
HidP_GetValueCaps(
    HIDP_REPORT_TYPE ReportType,
    PHIDP_VALUE_CAPS ValueCaps,
    PUSHORT ValueCapsLength,
    PHIDP_PREPARSED_DATA PreparsedData)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegPath)
{
    DPRINT("********* HID PARSE *********\n");
    return STATUS_SUCCESS;
}
