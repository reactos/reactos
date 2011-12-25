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


VOID
NTAPI
HidP_FreeCollectionDescription (
    IN PHIDP_DEVICE_DESC   DeviceDescription)
{
    DPRINT1("HidP_FreeCollectionDescription DeviceDescription %p\n", DeviceDescription);

    //
    // free collection
    //
    ExFreePool(DeviceDescription->CollectionDesc);

    //
    // free report ids
    //
    ExFreePool(DeviceDescription->ReportIDs);
}

#undef HidP_GetButtonCaps

HIDAPI
NTSTATUS
NTAPI
HidP_GetButtonCaps(
    HIDP_REPORT_TYPE ReportType,
    PHIDP_BUTTON_CAPS ButtonCaps,
    PUSHORT ButtonCapsLength,
    PHIDP_PREPARSED_DATA PreparsedData)
{
    return HidP_GetSpecificButtonCaps(ReportType, HID_USAGE_PAGE_UNDEFINED, 0, 0, ButtonCaps, (PULONG)ButtonCapsLength, PreparsedData);
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetSpecificButtonCaps(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection,
  IN USAGE  Usage,
  OUT PHIDP_BUTTON_CAPS  ButtonCaps,
  IN OUT PULONG  ButtonCapsLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}


HIDAPI
NTSTATUS
NTAPI
HidP_GetCaps(
    IN PHIDP_PREPARSED_DATA  PreparsedData,
    OUT PHIDP_CAPS  Capabilities)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidP_GetCollectionDescription(
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription)
{
    UNIMPLEMENTED
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
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetExtendedAttributes(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USHORT  DataIndex,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  OUT PHIDP_EXTENDED_ATTRIBUTES  Attributes,
  IN OUT PULONG  LengthAttributes)
{
    UNIMPLEMENTED
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
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}



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
    ULONG Index, SubIndex, bFound, BreakUsageIndex = 0, MakeUsageIndex = 0;
    USAGE CurrentUsage, Usage;

    if (UsageListLength)
    {
        Index = 0;
        do
        {
            /* get current usage */
            CurrentUsage = PreviousUsageList[Index];

            /* is the end of list reached? */
            if (!CurrentUsage)
                break;

            /* start searching in current usage list */
            SubIndex = 0;
            bFound = FALSE;
            do
            {
                /* get usage of current list */
                Usage = CurrentUsageList[SubIndex];

                /* end of list reached? */
                if (!Usage)
                    break;

                /* check if it matches the current one */
                if (CurrentUsage == Usage)
                {
                    /* it does */
                    bFound = TRUE;
                    break;
                }

                /* move to next usage */
                SubIndex++;
            }while(SubIndex < UsageListLength);

            /* was the usage found ?*/
            if (!bFound)
            {
                /* store it in the break usage list */
                BreakUsageList[BreakUsageIndex] = CurrentUsage;
                BreakUsageIndex++;
            }

            /* move to next usage */
            Index++;

        }while(Index < UsageListLength);

        /* now process the new items */
        Index = 0;
        do
        {
            /* get current usage */
            CurrentUsage = CurrentUsageList[Index];

            /* is the end of list reached? */
            if (!CurrentUsage)
                break;

            /* start searching in current usage list */
            SubIndex = 0;
            bFound = FALSE;
            do
            {
                /* get usage of previous list */
                Usage = PreviousUsageList[SubIndex];

                /* end of list reached? */
                if (!Usage)
                    break;

                /* check if it matches the current one */
                if (CurrentUsage == Usage)
                {
                    /* it does */
                    bFound = TRUE;
                    break;
                }

                /* move to next usage */
                SubIndex++;
            }while(SubIndex < UsageListLength);

            /* was the usage found ?*/
            if (!bFound)
            {
                /* store it in the make usage list */
                MakeUsageList[MakeUsageIndex] = CurrentUsage;
                MakeUsageIndex++;
            }

            /* move to next usage */
            Index++;

        }while(Index < UsageListLength);
    }

    /* does the break list contain empty entries */
    if (BreakUsageIndex < UsageListLength)
    {
        /* zeroize entries */
        RtlZeroMemory(&BreakUsageList[BreakUsageIndex], sizeof(USAGE) * (UsageListLength - BreakUsageIndex));
    }

    /* does the make usage list contain empty entries */
    if (MakeUsageIndex < UsageListLength)
    {
        /* zeroize entries */
        RtlZeroMemory(&MakeUsageList[MakeUsageIndex], sizeof(USAGE) * (UsageListLength - MakeUsageIndex));
    }

    /* done */
    return HIDP_STATUS_SUCCESS;
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
  IN OUT PULONG  ValueCapsLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
ULONG
NTAPI
HidP_MaxUsageListLength(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage  OPTIONAL,
  IN PHIDP_PREPARSED_DATA  PreparsedData)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_GetUsages(
  IN HIDP_REPORT_TYPE  ReportType,
  IN USAGE  UsagePage,
  IN USHORT  LinkCollection  OPTIONAL,
  OUT USAGE  *UsageList,
  IN OUT ULONG  *UsageLength,
  IN PHIDP_PREPARSED_DATA  PreparsedData,
  IN PCHAR  Report,
  IN ULONG  ReportLength)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidP_SysPowerEvent (
    IN PCHAR HidPacket,
    IN USHORT HidPacketLength,
    IN PHIDP_PREPARSED_DATA Ppd,
    OUT PULONG OutputBuffer)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidP_SysPowerCaps (
    IN PHIDP_PREPARSED_DATA Ppd,
    OUT PULONG OutputBuffer)
{
    UNIMPLEMENTED
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
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
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
    return HidP_GetUsages(ReportType, HID_USAGE_PAGE_UNDEFINED, LinkCollection, (PUSAGE)ButtonList, UsageLength, PreparsedData, Report, ReportLength);
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
    ULONG Index, SubIndex, BreakUsageListIndex = 0, MakeUsageListIndex = 0, bFound;
    PUSAGE_AND_PAGE CurrentUsage, Usage;

    if (UsageListLength)
    {
        /* process removed usages */
        Index = 0;
        do
        {
            /* get usage from current index */
            CurrentUsage = &PreviousUsageList[Index];

            /* end of list reached? */
            if (CurrentUsage->Usage == 0 && CurrentUsage->UsagePage == 0)
                break;

            /* search in current list */
            SubIndex = 0;
            bFound = FALSE;
            do
            {
                /* get usage */
                Usage = &CurrentUsageList[SubIndex];

                /* end of list reached? */
                if (Usage->Usage == 0 && Usage->UsagePage == 0)
                    break;

                /* does it match */
                if (Usage->Usage == CurrentUsage->Usage && Usage->UsagePage == CurrentUsage->UsagePage)
                {
                    /* found match */
                    bFound = TRUE;
                }

                /* move to next index */
                SubIndex++;

            }while(SubIndex < UsageListLength);

            if (!bFound)
            {
                /* store it in break usage list */
                BreakUsageList[BreakUsageListIndex].Usage = CurrentUsage->Usage;
                BreakUsageList[BreakUsageListIndex].UsagePage = CurrentUsage->UsagePage;
                BreakUsageListIndex++;
            }

            /* move to next index */
            Index++;

        }while(Index < UsageListLength);

        /* process new usages */
        Index = 0;
        do
        {
            /* get usage from current index */
            CurrentUsage = &CurrentUsageList[Index];

            /* end of list reached? */
            if (CurrentUsage->Usage == 0 && CurrentUsage->UsagePage == 0)
                break;

            /* search in current list */
            SubIndex = 0;
            bFound = FALSE;
            do
            {
                /* get usage */
                Usage = &PreviousUsageList[SubIndex];

                /* end of list reached? */
                if (Usage->Usage == 0 && Usage->UsagePage == 0)
                    break;

                /* does it match */
                if (Usage->Usage == CurrentUsage->Usage && Usage->UsagePage == CurrentUsage->UsagePage)
                {
                    /* found match */
                    bFound = TRUE;
                }

                /* move to next index */
                SubIndex++;

            }while(SubIndex < UsageListLength);

            if (!bFound)
            {
                /* store it in break usage list */
                MakeUsageList[MakeUsageListIndex].Usage = CurrentUsage->Usage;
                MakeUsageList[MakeUsageListIndex].UsagePage = CurrentUsage->UsagePage;
                MakeUsageListIndex++;
            }

            /* move to next index */
            Index++;
        }while(Index < UsageListLength);
    }

    /* are there remaining free list entries */
    if (BreakUsageListIndex < UsageListLength)
    {
        /* zero them */
        RtlZeroMemory(&BreakUsageList[BreakUsageListIndex], (UsageListLength - BreakUsageListIndex) * sizeof(USAGE_AND_PAGE));
    }

    /* are there remaining free list entries */
    if (MakeUsageListIndex < UsageListLength)
    {
        /* zero them */
        RtlZeroMemory(&MakeUsageList[MakeUsageListIndex], (UsageListLength - MakeUsageListIndex) * sizeof(USAGE_AND_PAGE));
    }

    /* done */
    return HIDP_STATUS_SUCCESS;
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
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
  PULONG ValueCapsLength,
  PHIDP_PREPARSED_DATA PreparsedData)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegPath)
{

    DPRINT1("********* HID PARSE *********\n");
    return STATUS_SUCCESS;
}
