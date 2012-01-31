/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/hidparser/api.c
 * PURPOSE:     HID Parser
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */


#include "parser.h"

static ULONG KeyboardScanCodes[256] =
{
    0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
    50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
    4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
    27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
    65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
    105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
    72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
    191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
    115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,
    122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
    150,158,159,128,136,177,178,176,142,152,173,140
};

HIDPARSER_STATUS
HidParser_GetCollectionUsagePage(
    IN PVOID CollectionContext,
    OUT PUSHORT Usage,
    OUT PUSHORT UsagePage)
{
    PHID_COLLECTION Collection;

    //
    // find collection
    //
    Collection = HidParser_GetCollectionFromContext(CollectionContext);
    if (!Collection)
    {
        //
        // collection not found
        //
        return HIDPARSER_STATUS_COLLECTION_NOT_FOUND;
    }

    //
    // store result
    //
    *UsagePage = (Collection->Usage >> 16);
    *Usage = (Collection->Usage & 0xFFFF);
    return HIDPARSER_STATUS_SUCCESS;
}

ULONG
HidParser_GetReportLength(
    IN PVOID CollectionContext,
    IN UCHAR ReportType)
{
    PHID_REPORT Report;
    ULONG ReportLength;

    //
    // get first report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no report found
        //
        return 0;
    }

    //
    // get report length
    //
    ReportLength = Report->ReportSize;

    //
    // done
    //
    if (ReportLength)
    {
        //
        // byte aligned length
        //
        ASSERT(ReportLength % 8 == 0);
        return ReportLength / 8;
    }
    return ReportLength;
}

ULONG
HidParser_GetReportItemCountFromReportType(
    IN PVOID CollectionContext,
    IN UCHAR ReportType)
{
    PHID_REPORT Report;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return 0;
    }

    //
    // return report item count
    //
    return Report->ItemCount;
}


ULONG
HidParser_GetReportItemTypeCountFromReportType(
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN ULONG bData)
{
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return 0;
    }

    //
    // enumerate all items
    //
    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // check item type
        //
        if (Report->Items[Index].HasData && bData == TRUE)
        {
            //
            // found data item
            //
            ItemCount++;
        }
        else if (Report->Items[Index].HasData == FALSE && bData == FALSE)
        {
            //
            // found value item
            //
            ItemCount++;
        }
    }

    //
    // no report items
    //
    return ItemCount;
}


VOID
HidParser_InitParser(
    IN PHIDPARSER_ALLOC_FUNCTION AllocFunction,
    IN PHIDPARSER_FREE_FUNCTION FreeFunction,
    IN PHIDPARSER_ZERO_FUNCTION ZeroFunction,
    IN PHIDPARSER_COPY_FUNCTION CopyFunction,
    IN PHIDPARSER_DEBUG_FUNCTION DebugFunction,
    OUT PHID_PARSER Parser)
{
    Parser->Alloc = AllocFunction;
    Parser->Free = FreeFunction;
    Parser->Zero = ZeroFunction;
    Parser->Copy = CopyFunction;
    Parser->Debug = DebugFunction;
}

ULONG
HidParser_GetMaxUsageListLengthWithReportAndPage(
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USAGE  UsagePage  OPTIONAL)
{
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return 0;
    }

    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // check usage page
        //
        CurrentUsagePage = (Report->Items[Index].UsageMinimum >> 16);
        if (CurrentUsagePage == UsagePage && Report->Items[Index].HasData)
        {
            //
            // found item
            //
            ItemCount++;
        }
    }

    //
    // done
    //
    return ItemCount;
}

HIDPARSER_STATUS
HidParser_GetSpecificValueCapsWithReport(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USHORT UsagePage,
    IN USHORT Usage,
    OUT PHIDP_VALUE_CAPS  ValueCaps,
    IN OUT PULONG  ValueCapsLength)
{
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;
    USHORT CurrentUsage;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return HIDPARSER_STATUS_REPORT_NOT_FOUND;
    }

    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // check usage page
        //
        CurrentUsagePage = (Report->Items[Index].UsageMinimum >> 16);
        CurrentUsage = (Report->Items[Index].UsageMinimum & 0xFFFF);

        if ((Usage == CurrentUsage && UsagePage == CurrentUsagePage) || (Usage == 0 && UsagePage == CurrentUsagePage) || (Usage == CurrentUsage && UsagePage == 0) || (Usage == 0 && UsagePage == 0))
        {
            //
            // check if there is enough place for the caps
            //
            if (ItemCount < *ValueCapsLength)
            {
                //
                // zero caps
                //
                Parser->Zero(&ValueCaps[ItemCount], sizeof(HIDP_VALUE_CAPS));

                //
                // init caps
                //
                ValueCaps[ItemCount].UsagePage = CurrentUsagePage;
                ValueCaps[ItemCount].ReportID = Report->ReportID;
                ValueCaps[ItemCount].LogicalMin = Report->Items[Index].Minimum;
                ValueCaps[ItemCount].LogicalMax = Report->Items[Index].Maximum;
                ValueCaps[ItemCount].IsAbsolute = !Report->Items[Index].Relative;
                ValueCaps[ItemCount].BitSize = Report->Items[Index].BitCount;

                //
                // FIXME: FILLMEIN
                //
            }


            //
            // found item
            //
            ItemCount++;
        }
    }

    //
    // store result
    //
    *ValueCapsLength = ItemCount;

    if (ItemCount)
    {
        //
        // success
        //
        return HIDPARSER_STATUS_SUCCESS;
    }

    //
    // item not found
    //
    return HIDPARSER_STATUS_USAGE_NOT_FOUND;
}

HIDPARSER_STATUS
HidParser_GetUsagesWithReport(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN UCHAR  ReportType,
    IN USAGE  UsagePage,
    OUT USAGE  *UsageList,
    IN OUT PULONG UsageLength,
    IN PCHAR  ReportDescriptor,
    IN ULONG  ReportDescriptorLength)
{
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;
    PHID_REPORT_ITEM ReportItem;
    UCHAR Activated;
    ULONG Data;
    PUSAGE_AND_PAGE UsageAndPage = NULL;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return HIDPARSER_STATUS_REPORT_NOT_FOUND;
    }

    if (Report->ReportSize / 8 != (ReportDescriptorLength - 1))
    {
        //
        // invalid report descriptor length
        //
        return HIDPARSER_STATUS_INVALID_REPORT_LENGTH;
    }

    //
    // cast to usage and page
    //
    if (UsagePage == HID_USAGE_PAGE_UNDEFINED)
    {
        //
        // the caller requested any set usages
        //
        UsageAndPage = (PUSAGE_AND_PAGE)UsageList;
    }

    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // get report item
        //
        ReportItem = &Report->Items[Index];

        //
        // does it have data
        //
        if (!ReportItem->HasData)
            continue;

        //
        // check usage page
        //
        CurrentUsagePage = (ReportItem->UsageMinimum >> 16);

        if (UsagePage != HID_USAGE_PAGE_UNDEFINED)
        {
            //
            // does usage match
            //
            if (UsagePage != CurrentUsagePage)
                continue;
        }

        //
        // check if the specified usage is activated
        //
        ASSERT(ReportItem->ByteOffset < ReportDescriptorLength);
        ASSERT(ReportItem->BitCount <= 8);

        //
        // one extra shift for skipping the prepended report id
        //
        Data = ReportDescriptor[ReportItem->ByteOffset + 1];

        //
        // shift data
        //
        Data >>= ReportItem->Shift;

        //
        // clear unwanted bits
        //
        Data &= ReportItem->Mask;

        //
        // is it activated
        //
        Activated = (Data != 0);

        if (!Activated)
            continue;

        //
        // is there enough space for the usage
        //
        if (ItemCount >= *UsageLength)
        {
            ItemCount++;
            continue;
        }

        if (UsagePage != HID_USAGE_PAGE_UNDEFINED)
        {
            //
            // store item
            //
            UsageList[ItemCount] = (ReportItem->UsageMinimum & 0xFFFF);
        }
        else
        {
            //
            // store usage and page
            //
            UsageAndPage[ItemCount].Usage = (ReportItem->UsageMinimum & 0xFFFF);
            UsageAndPage[ItemCount].UsagePage = CurrentUsagePage;
        }
        ItemCount++;
    }

    if (ItemCount > *UsageLength)
    {
        //
        // list too small
        //
        return HIDPARSER_STATUS_BUFFER_TOO_SMALL;
    }

    if (UsagePage == HID_USAGE_PAGE_UNDEFINED)
    {
        //
        // success, clear rest of array
        //
        Parser->Zero(&UsageAndPage[ItemCount], (*UsageLength - ItemCount) * sizeof(USAGE_AND_PAGE));
    }
    else
    {
        //
        // success, clear rest of array
        //
        Parser->Zero(&UsageList[ItemCount], (*UsageLength - ItemCount) * sizeof(USAGE));
    }


    //
    // store result size
    //
    *UsageLength = ItemCount;

    //
    // done
    //
    return HIDPARSER_STATUS_SUCCESS;
}

HIDPARSER_STATUS
HidParser_GetScaledUsageValueWithReport(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN UCHAR ReportType,
    IN USAGE UsagePage,
    IN USAGE  Usage,
    OUT PLONG UsageValue,
    IN PCHAR ReportDescriptor,
    IN ULONG ReportDescriptorLength)
{
    ULONG Index;
    PHID_REPORT Report;
    USHORT CurrentUsagePage;
    PHID_REPORT_ITEM ReportItem;
    ULONG Data;

    //
    // get report
    //
    Report = HidParser_GetReportInCollection(CollectionContext, ReportType);
    if (!Report)
    {
        //
        // no such report
        //
        return HIDPARSER_STATUS_REPORT_NOT_FOUND;
    }

    if (Report->ReportSize / 8 != (ReportDescriptorLength - 1))
    {
        //
        // invalid report descriptor length
        //
        return HIDPARSER_STATUS_INVALID_REPORT_LENGTH;
    }

    for(Index = 0; Index < Report->ItemCount; Index++)
    {
        //
        // get report item
        //
        ReportItem = &Report->Items[Index];

        //
        // check usage page
        //
        CurrentUsagePage = (ReportItem->UsageMinimum >> 16);

        //
        // does usage page match
        //
        if (UsagePage != CurrentUsagePage)
            continue;

        //
        // does the usage match
        //
        if (Usage != (ReportItem->UsageMinimum & 0xFFFF))
            continue;

        //
        // check if the specified usage is activated
        //
        ASSERT(ReportItem->ByteOffset < ReportDescriptorLength);

        //
        // one extra shift for skipping the prepended report id
        //
        Data = 0;
        Parser->Copy(&Data, &ReportDescriptor[ReportItem->ByteOffset +1], min(sizeof(ULONG), ReportDescriptorLength - (ReportItem->ByteOffset + 1)));
        Data = ReportDescriptor[ReportItem->ByteOffset + 1];

        //
        // shift data
        //
        Data >>= ReportItem->Shift;

        //
        // clear unwanted bits
        //
        Data &= ReportItem->Mask;

        if (ReportItem->Minimum > ReportItem->Maximum)
        {
            //
            // logical boundaries are signed values
            //
            if ((Data & ~(ReportItem->Mask >> 1)) != 0)
            {
                Data |= ~ReportItem->Mask;
            }
        }

        //
        // store result
        //
        *UsageValue = Data;
        return HIDPARSER_STATUS_SUCCESS;
    }

    //
    // usage not found
    //
    return HIDPARSER_STATUS_USAGE_NOT_FOUND;
}

ULONG
HidParser_GetScanCode(
    IN USAGE Usage)
{
    if (Usage < sizeof(KeyboardScanCodes) / sizeof(KeyboardScanCodes[0]))
    {
        //
        // valid usage
        //
        return KeyboardScanCodes[Usage];
    }

    //
    // invalid usage
    //
    return 0;
}

VOID
HidParser_DispatchKey(
    IN PCHAR ScanCodes,
    IN HIDP_KEYBOARD_DIRECTION KeyAction,
    IN PHIDP_INSERT_SCANCODES InsertCodesProcedure,
    IN PVOID InsertCodesContext)
{
    ULONG Index;
    ULONG Length = 0;

    //
    // count code length
    //
    for(Index = 0; Index < sizeof(ULONG); Index++)
    {
        if (ScanCodes[Index] == 0)
        {
            //
            // last scan code
            //
            break;
        }

        //
        // is this a key break
        //
        if (KeyAction == HidP_Keyboard_Break)
        {
            //
            // add break
            //
            ScanCodes[Index] |= KEY_BREAK;
        }

        //
        // more scan counts
        //
        Length++;
    }

    if (Length > 0)
    {
         //
         // dispatch scan codes
         //
         InsertCodesProcedure(InsertCodesContext, ScanCodes, Length);
    }
}


HIDPARSER_STATUS
HidParser_TranslateUsage(
    IN PHID_PARSER Parser,
    IN USAGE Usage,
    IN HIDP_KEYBOARD_DIRECTION  KeyAction,
    IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
    IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
    IN PVOID  InsertCodesContext)
{
    ULONG ScanCode;

    //
    // get scan code
    //
    ScanCode = HidParser_GetScanCode(Usage);
    if (!ScanCode)
    {
        //
        // invalid lookup or no scan code available
        //
        return HIDPARSER_STATUS_I8042_TRANS_UNKNOWN;
    }

    //
    // FIXME: translate modifier states
    //

    HidParser_DispatchKey((PCHAR)&ScanCode, KeyAction, InsertCodesProcedure, InsertCodesContext);

    //
    // done
    //
    return HIDPARSER_STATUS_SUCCESS;
}
