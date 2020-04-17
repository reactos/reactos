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

NTSTATUS
NTAPI
HidParser_GetCollectionDescription(
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription);

VOID
NTAPI
HidParser_FreeCollectionDescription(
    IN PHIDP_DEVICE_DESC DeviceDescription);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetCaps(
    IN PVOID CollectionContext,
    OUT PHIDP_CAPS  Capabilities);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetSpecificValueCaps(
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
    IN PVOID CollectionContext,
    HIDP_REPORT_TYPE ReportType,
    PHIDP_BUTTON_CAPS ButtonCaps,
    PUSHORT ButtonCapsLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetSpecificButtonCaps(
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
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT  DataIndex,
    OUT PHIDP_EXTENDED_ATTRIBUTES  Attributes,
    IN OUT PULONG  LengthAttributes);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetLinkCollectionNodes(
    IN PVOID CollectionContext,
    OUT PHIDP_LINK_COLLECTION_NODE  LinkCollectionNodes,
    IN OUT PULONG  LinkCollectionNodesLength);


HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsageValue(
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
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage  OPTIONAL);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsages(
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
    IN PVOID CollectionContext,
    IN PCHAR HidPacket,
    IN USHORT HidPacketLength,
    OUT PULONG OutputBuffer);

NTSTATUS
NTAPI
HidParser_SysPowerCaps (
    IN PVOID CollectionContext,
    OUT PULONG OutputBuffer);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsageValueArray(
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
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType);

HIDAPI
NTSTATUS
NTAPI
HidParser_InitializeReportForID(
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN UCHAR  ReportID,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

NTSTATUS
HidParser_TranslateKbdUsage(
    IN USAGE Usage,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext);

NTSTATUS
HidParser_TranslateCustUsage(
    IN USAGE Usage,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetValueCaps(
    IN PVOID CollectionContext,
    HIDP_REPORT_TYPE ReportType,
    PHIDP_VALUE_CAPS ValueCaps,
    PULONG ValueCapsLength);
