/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/hidparser/hidparser.c
 * PURPOSE:     HID Parser
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#pragma once

//
// function prototypes
//
typedef PVOID (NTAPI *PHIDPARSER_ALLOC_FUNCTION)(ULONG Size);
typedef VOID (NTAPI *PHIDPARSER_FREE_FUNCTION)(PVOID Item);
typedef VOID (NTAPI *PHIDPARSER_ZERO_FUNCTION)(PVOID Item, ULONG Size);
typedef VOID (NTAPI *PHIDPARSER_COPY_FUNCTION)(PVOID Target, PVOID Source, ULONG Size);
typedef VOID (__cdecl *PHIDPARSER_DEBUG_FUNCTION)(LPCSTR Src, ...);

//
// status code
//
typedef long HIDPARSER_STATUS;

//
// result codes
//
typedef enum
{
    HIDPARSER_STATUS_SUCCESS = 0,
    HIDPARSER_STATUS_INSUFFICIENT_RESOURCES = -1,
    HIDPARSER_STATUS_NOT_IMPLEMENTED = -2,
    HIDPARSER_STATUS_REPORT_NOT_FOUND = -3,
    HIDPARSER_STATUS_COLLECTION_NOT_FOUND = -4,
    HIDPARSER_STATUS_INVALID_REPORT_LENGTH = -5,
    HIDPARSER_STATUS_INVALID_REPORT_TYPE = -6,
    HIDPARSER_STATUS_BUFFER_TOO_SMALL = -7,
    HIDPARSER_STATUS_USAGE_NOT_FOUND = -8,
    HIDPARSER_STATUS_I8042_TRANS_UNKNOWN = -9,
    HIDPARSER_STATUS_BAD_LOG_PHY_VALUES = -10
}HIDPARSER_STATUS_CODES;

typedef struct
{
    //
    // size of struct
    //
    unsigned long Size;

    //
    // allocation function
    //
    PHIDPARSER_ALLOC_FUNCTION Alloc;

    //
    // free function
    //
    PHIDPARSER_FREE_FUNCTION Free;

    //
    // zero function
    //
    PHIDPARSER_ZERO_FUNCTION Zero;

    //
    // copy function
    //
    PHIDPARSER_COPY_FUNCTION Copy;

    //
    // debug function
    //
    PHIDPARSER_DEBUG_FUNCTION Debug;
}HID_PARSER, *PHID_PARSER;

VOID
HidParser_InitParser(
    IN PHIDPARSER_ALLOC_FUNCTION AllocFunction,
    IN PHIDPARSER_FREE_FUNCTION FreeFunction,
    IN PHIDPARSER_ZERO_FUNCTION ZeroFunction,
    IN PHIDPARSER_COPY_FUNCTION CopyFunction,
    IN PHIDPARSER_DEBUG_FUNCTION DebugFunction,
    OUT PHID_PARSER Parser);

NTSTATUS
NTAPI
HidParser_GetCollectionDescription(
    IN PHID_PARSER Parser,
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription);

VOID
NTAPI
HidParser_FreeCollectionDescription(
    IN PHID_PARSER Parser,
    IN PHIDP_DEVICE_DESC DeviceDescription);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    OUT PHIDP_CAPS  Capabilities);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetSpecificValueCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN USAGE Usage,
    OUT PHIDP_VALUE_CAPS ValueCaps,
    IN OUT PUSHORT ValueCapsLength);


HIDAPI
NTSTATUS
NTAPI
HidParser_GetButtonCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    HIDP_REPORT_TYPE ReportType,
    PHIDP_BUTTON_CAPS ButtonCaps,
    PUSHORT ButtonCapsLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetSpecificButtonCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PHIDP_BUTTON_CAPS  ButtonCaps,
    IN OUT PULONG  ButtonCapsLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetScaledUsageValue(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    IN USAGE  Usage,
    OUT PLONG  UsageValue,
    IN PCHAR  Report,
    IN ULONG  ReportLength);


HIDAPI
NTSTATUS
NTAPI
HidParser_GetData(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    OUT PHIDP_DATA  DataList,
    IN OUT PULONG  DataLength,
    IN PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetExtendedAttributes(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT  DataIndex,
    OUT PHIDP_EXTENDED_ATTRIBUTES  Attributes,
    IN OUT PULONG  LengthAttributes);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetLinkCollectionNodes(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    OUT PHIDP_LINK_COLLECTION_NODE  LinkCollectionNodes,
    IN OUT PULONG  LinkCollectionNodesLength);


HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsageValue(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PULONG  UsageValue,
    IN PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_UsageListDifference(
    IN PUSAGE  PreviousUsageList,
    IN PUSAGE  CurrentUsageList,
    OUT PUSAGE  BreakUsageList,
    OUT PUSAGE  MakeUsageList,
    IN ULONG  UsageListLength);


HIDAPI
ULONG
NTAPI
HidParser_MaxUsageListLength(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage  OPTIONAL);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsages(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    OUT USAGE  *UsageList,
    IN OUT ULONG  *UsageLength,
    IN PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsagesEx(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT  LinkCollection,
    OUT PUSAGE_AND_PAGE  ButtonList,
    IN OUT ULONG  *UsageLength,
    IN PCHAR  Report,
    IN ULONG  ReportLength);


NTSTATUS
NTAPI
HidParser_SysPowerEvent (
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN PCHAR HidPacket,
    IN USHORT HidPacketLength,
    OUT PULONG OutputBuffer);

NTSTATUS
NTAPI
HidParser_SysPowerCaps (
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    OUT PULONG OutputBuffer);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsageValueArray(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    IN USAGE  Usage,
    OUT PCHAR  UsageValue,
    IN USHORT  UsageValueByteLength,
    IN PCHAR  Report,
    IN ULONG  ReportLength);


HIDAPI
NTSTATUS
NTAPI
HidParser_UsageAndPageListDifference(
   IN PUSAGE_AND_PAGE  PreviousUsageList,
   IN PUSAGE_AND_PAGE  CurrentUsageList,
   OUT PUSAGE_AND_PAGE  BreakUsageList,
   OUT PUSAGE_AND_PAGE  MakeUsageList,
   IN ULONG  UsageListLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_UnsetUsages(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN PUSAGE  UsageList,
    IN OUT PULONG  UsageLength,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_TranslateUsagesToI8042ScanCodes(
  IN PUSAGE  ChangedUsageList,
  IN ULONG  UsageListLength,
  IN HIDP_KEYBOARD_DIRECTION  KeyAction,
  IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
  IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
  IN PVOID  InsertCodesContext);

HIDAPI
NTSTATUS
NTAPI
HidParser_TranslateUsageAndPagesToI8042ScanCodes(
   IN PHID_PARSER Parser,
   IN PUSAGE_AND_PAGE  ChangedUsageList,
   IN ULONG  UsageListLength,
   IN HIDP_KEYBOARD_DIRECTION  KeyAction,
   IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
   IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
   IN PVOID  InsertCodesContext);

HIDAPI
NTSTATUS
NTAPI
HidParser_SetUsages(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN PUSAGE  UsageList,
    IN OUT PULONG  UsageLength,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_SetUsageValueArray(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    IN USAGE  Usage,
    IN PCHAR  UsageValue,
    IN USHORT  UsageValueByteLength,
    OUT PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_SetUsageValue(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    IN ULONG  UsageValue,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_SetScaledUsageValue(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    IN USAGE  Usage,
    IN LONG  UsageValue,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_SetData(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN PHIDP_DATA  DataList,
    IN OUT PULONG  DataLength,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
ULONG
NTAPI
HidParser_MaxDataListLength(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType);

HIDAPI
NTSTATUS
NTAPI
HidParser_InitializeReportForID(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN UCHAR  ReportID,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

HIDPARSER_STATUS
HidParser_TranslateKbdUsage(
    IN PHID_PARSER Parser,
    IN USAGE Usage,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext);

HIDPARSER_STATUS
HidParser_TranslateCustUsage(
    IN PHID_PARSER Parser,
    IN USAGE Usage,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetValueCaps(
    PHID_PARSER Parser,
    IN PVOID CollectionContext,
    HIDP_REPORT_TYPE ReportType,
    PHIDP_VALUE_CAPS ValueCaps,
    PULONG ValueCapsLength);
