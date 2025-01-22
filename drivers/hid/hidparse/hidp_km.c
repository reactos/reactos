/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser kernel mode
 * COPYRIGHT:   Copyright  Michael Martin <michael.martin@reactos.org>
 *              Copyright  Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#include "parser.h"
#include <hidparser.h>
#include <hidpmem.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
HidP_GetCollectionDescription(
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription
)
{
    return HidParser_GetCollectionDescription(ReportDesc, DescLength, PoolType, DeviceDescription);
}

VOID
NTAPI
HidP_FreeCollectionDescription(
    IN PHIDP_DEVICE_DESC DeviceDescription)
{
    ULONG index;

    // first free all context
    for (index = 0; index < DeviceDescription->CollectionDescLength; index++)
    {
        // free collection context
        FreeFunction(DeviceDescription->CollectionDesc[index].PreparsedData);
    }

    // now free collection description
    FreeFunction(DeviceDescription->CollectionDesc);

    // free report description
    FreeFunction(DeviceDescription->ReportIDs);
}

NTSTATUS
NTAPI
HidP_SysPowerEvent(
    IN PCHAR HidPacket,
    IN USHORT HidPacketLength,
    IN PHIDP_PREPARSED_DATA PreparsedData,
    OUT PULONG OutputBuffer)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidP_SysPowerCaps(
    IN PHIDP_PREPARSED_DATA PreparsedData,
    OUT PULONG OutputBuffer)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidP_TranslateUsageAndPagesToI8042ScanCodes(
    IN PUSAGE_AND_PAGE ChangedUsageList,
    IN ULONG UsageListLength,
    IN HIDP_KEYBOARD_DIRECTION KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE ModifierState,
    IN PHIDP_INSERT_SCANCODES InsertCodesProcedure,
    IN PVOID InsertCodesContext)
{
    ULONG index;
    NTSTATUS status = HIDP_STATUS_SUCCESS;

    for (index = 0; index < UsageListLength; index++)
    {
        // check current usage
        if (ChangedUsageList[index].UsagePage == HID_USAGE_PAGE_KEYBOARD)
        {
            // process keyboard usage
            status = HidParser_TranslateKbdUsage(ChangedUsageList[index].Usage, KeyAction, ModifierState, InsertCodesProcedure, InsertCodesContext);
        }
        else if (ChangedUsageList[index].UsagePage == HID_USAGE_PAGE_CONSUMER)
        {
            // process consumer usage
            status = HidParser_TranslateCustUsage(ChangedUsageList[index].Usage, KeyAction, ModifierState, InsertCodesProcedure, InsertCodesContext);
        }
        else
        {
            // invalid page / end of usage list page
            return HIDP_STATUS_I8042_TRANS_UNKNOWN;
        }

        // check status
        if (status != HIDP_STATUS_SUCCESS)
        {
            // failed
            return status;
        }
    }

    return status;
}

HIDAPI
NTSTATUS
NTAPI
HidP_UsageAndPageListDifference(
    IN PUSAGE_AND_PAGE PreviousUsageList,
    IN PUSAGE_AND_PAGE CurrentUsageList,
    OUT PUSAGE_AND_PAGE BreakUsageList,
    OUT PUSAGE_AND_PAGE MakeUsageList,
    IN ULONG UsageListLength)
{
    ULONG index;
    ULONG subIndex;
    ULONG breakUsageListIndex = 0;
    ULONG makeUsageListIndex = 0;
    ULONG bFound;
    PUSAGE_AND_PAGE currentUsage, usage;

    if (UsageListLength)
    {
        /* process removed usages */
        index = 0;
        do
        {
            /* get usage from current index */
            currentUsage = &PreviousUsageList[index];

            /* end of list reached? */
            if (currentUsage->Usage == 0 && currentUsage->UsagePage == 0)
                break;

            /* search in current list */
            subIndex = 0;
            bFound = FALSE;
            do
            {
                /* get usage */
                usage = &CurrentUsageList[subIndex];

                /* end of list reached? */
                if (usage->Usage == 0 && usage->UsagePage == 0)
                    break;

                /* does it match */
                if (usage->Usage == currentUsage->Usage && usage->UsagePage == currentUsage->UsagePage)
                {
                    /* found match */
                    bFound = TRUE;
                }

                /* move to next index */
                subIndex++;

            }
            while (subIndex < UsageListLength);

            if (!bFound)
            {
                /* store it in break usage list */
                BreakUsageList[breakUsageListIndex].Usage = currentUsage->Usage;
                BreakUsageList[breakUsageListIndex].UsagePage = currentUsage->UsagePage;
                breakUsageListIndex++;
            }

            /* move to next index */
            index++;

        }
        while (index < UsageListLength);

        /* process new usages */
        index = 0;
        do
        {
            /* get usage from current index */
            currentUsage = &CurrentUsageList[index];

            /* end of list reached? */
            if (currentUsage->Usage == 0 && currentUsage->UsagePage == 0)
                break;

            /* search in current list */
            subIndex = 0;
            bFound = FALSE;
            do
            {
                /* get usage */
                usage = &PreviousUsageList[subIndex];

                /* end of list reached? */
                if (usage->Usage == 0 && usage->UsagePage == 0)
                    break;

                /* does it match */
                if (usage->Usage == currentUsage->Usage && usage->UsagePage == currentUsage->UsagePage)
                {
                    /* found match */
                    bFound = TRUE;
                }

                /* move to next index */
                subIndex++;

            }
            while (subIndex < UsageListLength);

            if (!bFound)
            {
                /* store it in break usage list */
                MakeUsageList[makeUsageListIndex].Usage = currentUsage->Usage;
                MakeUsageList[makeUsageListIndex].UsagePage = currentUsage->UsagePage;
                makeUsageListIndex++;
            }

            /* move to next index */
            index++;
        }
        while (index < UsageListLength);
    }

    /* are there remaining free list entries */
    if (breakUsageListIndex < UsageListLength)
    {
        /* zero them */
        RtlZeroMemory(&BreakUsageList[breakUsageListIndex], (UsageListLength - breakUsageListIndex) * sizeof(USAGE_AND_PAGE));
    }

    /* are there remaining free list entries */
    if (makeUsageListIndex < UsageListLength)
    {
        /* zero them */
        RtlZeroMemory(&MakeUsageList[makeUsageListIndex], (UsageListLength - makeUsageListIndex) * sizeof(USAGE_AND_PAGE));
    }

    /* done */
    return HIDP_STATUS_SUCCESS;
}
