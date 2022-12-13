/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser
 * COPYRIGHT:   Copyright  Michael Martin <michael.martin@reactos.org>
 *              Copyright  Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#pragma once

#include "preparsed.h"

HIDAPI
NTSTATUS
NTAPI
HidParser_GetCaps(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    OUT PHIDP_CAPS  Capabilities);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetSpecificValueCaps(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN USAGE Usage,
    OUT PHIDP_VALUE_CAPS ValueCaps,
    IN OUT PUSHORT ValueCapsLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetSpecificButtonCaps(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PHIDP_BUTTON_CAPS  ButtonCaps,
    IN OUT PUSHORT  ButtonCapsLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetScaledUsageValue(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
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
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    OUT PHIDP_DATA  DataList,
    IN OUT PULONG  DataLength,
    IN PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetExtendedAttributes(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT  DataIndex,
    OUT PHIDP_EXTENDED_ATTRIBUTES  Attributes,
    IN OUT PULONG  LengthAttributes);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetLinkCollectionNodes(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    OUT PHIDP_LINK_COLLECTION_NODE  LinkCollectionNodes,
    IN OUT PULONG  LinkCollectionNodesLength);


HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsageValue(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PULONG  UsageValue,
    IN PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
ULONG
NTAPI
HidParser_MaxUsageListLength(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage  OPTIONAL);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsages(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    OUT USAGE  *UsageList,
    IN OUT PULONG  UsageLength,
    IN PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsagesEx(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT  LinkCollection,
    OUT PUSAGE_AND_PAGE  ButtonList,
    IN OUT ULONG  *UsageLength,
    IN PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsageValueArray(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
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
HidParser_UnsetUsages(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
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
HidParser_SetUsages(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
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
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
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
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
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
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
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
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    IN PHIDP_DATA  DataList,
    IN OUT PULONG  DataLength,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

HIDAPI
ULONG
NTAPI
HidParser_MaxDataListLength(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType);

HIDAPI
NTSTATUS
NTAPI
HidParser_InitializeReportForID(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE  ReportType,
    IN UCHAR  ReportID,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength);

/* api.c */

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
HidParser_UsageListDifference(
    IN PUSAGE  PreviousUsageList,
    IN PUSAGE  CurrentUsageList,
    OUT PUSAGE  BreakUsageList,
    OUT PUSAGE  MakeUsageList,
    IN ULONG  UsageListLength);
