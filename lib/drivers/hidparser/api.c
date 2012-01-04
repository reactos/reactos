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


ULONG
HidParser_NumberOfTopCollections(
    IN PHID_PARSER Parser)
{
    PHID_PARSER_CONTEXT ParserContext;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);
    ASSERT(ParserContext->RootCollection);
    ASSERT(ParserContext->RootCollection->NodeCount);

    //
    // number of top collections
    //
    return ParserContext->RootCollection->NodeCount;
}

PHID_COLLECTION
HidParser_GetCollection(
    IN PHID_PARSER Parser,
    IN ULONG CollectionNumber)
{
    PHID_PARSER_CONTEXT ParserContext;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);
    ASSERT(ParserContext->RootCollection);
    ASSERT(ParserContext->RootCollection->NodeCount);

    //
    // is collection index out of bounds
    //
    if (CollectionNumber < ParserContext->RootCollection->NodeCount)
    {
        //
        // valid collection
        //
        return ParserContext->RootCollection->Nodes[CollectionNumber];
    }

    //
    // no such collection
    //
    Parser->Debug("HIDPARSE] No such collection %lu\n", CollectionNumber);
    return NULL;
}

PHID_REPORT
HidParser_GetReportByType(
    IN PHID_PARSER Parser,
    IN ULONG ReportType)
{
    PHID_PARSER_CONTEXT ParserContext;
    ULONG Index;
    ULONG ReportCount = 0;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);
    for(Index = 0; Index < ParserContext->ReportCount; Index++)
    {
        //
        // check if the report type match
        //
        if (ParserContext->Reports[Index]->Type == ReportType)
        {
            //
            // found report
            //
            return ParserContext->Reports[Index];
        }
    }

    //
    // report not found
    //
    return NULL;
}


ULONG
HidParser_NumberOfReports(
    IN PHID_PARSER Parser,
    IN ULONG ReportType)
{
    PHID_PARSER_CONTEXT ParserContext;
    ULONG Index;
    ULONG ReportCount = 0;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);
    for(Index = 0; Index < ParserContext->ReportCount; Index++)
    {
        //
        // check if the report type match
        //
        if (ParserContext->Reports[Index]->Type == ReportType)
        {
            //
            // found report
            //
            ReportCount++;
        }
    }

    //
    // done
    //
    return ReportCount;
}

HIDPARSER_STATUS
HidParser_GetCollectionUsagePage(
    IN PHID_PARSER Parser,
    IN ULONG CollectionIndex,
    OUT PUSHORT Usage,
    OUT PUSHORT UsagePage)
{
    PHID_COLLECTION Collection;

    //
    // find collection
    //
    Collection = HidParser_GetCollection(Parser, CollectionIndex);
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
    IN PHID_PARSER Parser,
    IN ULONG ReportType)
{
    PHID_PARSER_CONTEXT ParserContext;
    PHID_REPORT Report;
    ULONG ReportLength;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);

    //
    // get first report
    //
    Report = HidParser_GetReportByType(Parser, ReportType);
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

UCHAR
HidParser_IsReportIDUsed(
    IN PHID_PARSER Parser)
{
    PHID_PARSER_CONTEXT ParserContext;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // return flag
    //
    return ParserContext->UseReportIDs;
}

ULONG
HidParser_GetReportItemCountFromReportType(
    IN PHID_PARSER Parser,
    IN ULONG ReportType)
{
    PHID_PARSER_CONTEXT ParserContext;
    PHID_REPORT Report;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);

    //
    // get report
    //
    Report = HidParser_GetReportByType(Parser, ReportType);
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
    IN PHID_PARSER Parser,
    IN ULONG ReportType,
    IN ULONG bData)
{
    PHID_PARSER_CONTEXT ParserContext;
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);

    //
    // get report
    //
    Report = HidParser_GetReportByType(Parser, ReportType);
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
        if (Report->Items[Index]->HasData && bData == TRUE)
        {
            //
            // found data item
            //
            ItemCount++;
        }
        else if (Report->Items[Index]->HasData == FALSE && bData == FALSE)
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

ULONG
HidParser_GetContextSize(
    IN PHID_PARSER Parser)
{
    //
    // FIXME the context must contain all parsed info
    //
    return sizeof(HID_PARSER_CONTEXT);
}

VOID
HidParser_FreeContext(
    IN PHID_PARSER Parser,
    IN PUCHAR Context,
    IN ULONG ContextLength)
{
    //
    // FIXME implement freeing of parsed info
    //
}

HIDPARSER_STATUS
HidParser_AllocateParser(
    IN PHIDPARSER_ALLOC_FUNCTION AllocFunction,
    IN PHIDPARSER_FREE_FUNCTION FreeFunction,
    IN PHIDPARSER_ZERO_FUNCTION ZeroFunction,
    IN PHIDPARSER_COPY_FUNCTION CopyFunction,
    IN PHIDPARSER_DEBUG_FUNCTION DebugFunction,
    OUT PHID_PARSER *OutParser)
{
    PHID_PARSER Parser;
    PHID_PARSER_CONTEXT ParserContext;

    //
    // allocate 
    //
    Parser = (PHID_PARSER)AllocFunction(sizeof(HID_PARSER));
    if (!Parser)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // allocate parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)AllocFunction(sizeof(HID_PARSER_CONTEXT));
    if (!ParserContext)
    {
        //
        // no memory
        //
        FreeFunction(Parser);
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // init parser
    //
    Parser->Alloc = AllocFunction;
    Parser->Free = FreeFunction;
    Parser->Zero = ZeroFunction;
    Parser->Copy = CopyFunction;
    Parser->Debug = DebugFunction;
    Parser->ParserContext = ParserContext;

    //
    // store result
    //
    *OutParser = Parser;
    //
    // success
    //
    return HIDPARSER_STATUS_SUCCESS;
}

VOID
HidParser_InitParser(
    IN PHIDPARSER_ALLOC_FUNCTION AllocFunction,
    IN PHIDPARSER_FREE_FUNCTION FreeFunction,
    IN PHIDPARSER_ZERO_FUNCTION ZeroFunction,
    IN PHIDPARSER_COPY_FUNCTION CopyFunction,
    IN PHIDPARSER_DEBUG_FUNCTION DebugFunction,
    IN PVOID ParserContext,
    OUT PHID_PARSER Parser)
{
    Parser->Alloc = AllocFunction;
    Parser->Free = FreeFunction;
    Parser->Zero = ZeroFunction;
    Parser->Copy = CopyFunction;
    Parser->Debug = DebugFunction;
    Parser->ParserContext = ParserContext;
}

ULONG
HidParser_GetCollectionCount(
    IN PHID_COLLECTION Collection)
{
    ULONG Index;
    ULONG Count = Collection->NodeCount;

    for(Index = 0; Index < Collection->NodeCount; Index++)
    {
        //
        // count collection for sub nodes
        //
        Count += HidParser_GetCollectionCount(Collection->Nodes[Index]);
    }

    //
    // done
    //
    return Count;
}

ULONG
HidParser_GetTotalCollectionCount(
    IN PHID_PARSER Parser)
{
    PHID_PARSER_CONTEXT ParserContext;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity check
    //
    ASSERT(ParserContext);
    ASSERT(ParserContext->RootCollection);

    //
    // count collections
    //
    return HidParser_GetCollectionCount(ParserContext->RootCollection);
}

ULONG
HidParser_GetMaxUsageListLengthWithReportAndPage(
    IN PHID_PARSER Parser,
    IN ULONG ReportType,
    IN USAGE  UsagePage  OPTIONAL)
{
    PHID_PARSER_CONTEXT ParserContext;
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);

    //
    // get report
    //
    Report = HidParser_GetReportByType(Parser, ReportType);
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
        CurrentUsagePage = (Report->Items[Index]->UsageMinimum >> 16);
        if (CurrentUsagePage == UsagePage && Report->Items[Index]->HasData)
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
    IN ULONG ReportType,
    IN USHORT UsagePage,
    IN USHORT Usage,
    OUT PHIDP_VALUE_CAPS  ValueCaps,
    IN OUT PULONG  ValueCapsLength)
{
    PHID_PARSER_CONTEXT ParserContext;
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;
    USHORT CurrentUsage;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);

    //
    // get report
    //
    Report = HidParser_GetReportByType(Parser, ReportType);
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
        CurrentUsagePage = (Report->Items[Index]->UsageMinimum >> 16);
        CurrentUsage = (Report->Items[Index]->UsageMinimum & 0xFFFF);

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
                ValueCaps[ItemCount].LogicalMin = Report->Items[Index]->Minimum;
                ValueCaps[ItemCount].LogicalMax = Report->Items[Index]->Maximum;
                ValueCaps[ItemCount].IsAbsolute = !Report->Items[Index]->Relative;
                ValueCaps[ItemCount].BitSize = Report->Items[Index]->BitCount;

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
    IN ULONG  ReportType,
    IN USAGE  UsagePage,
    OUT USAGE  *UsageList,
    IN OUT PULONG UsageLength,
    IN PCHAR  ReportDescriptor,
    IN ULONG  ReportDescriptorLength)
{
    PHID_PARSER_CONTEXT ParserContext;
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;
    PHID_REPORT_ITEM ReportItem;
    UCHAR Activated;
    ULONG Data;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);

    //
    // get report
    //
    Report = HidParser_GetReportByType(Parser, ReportType);
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
        ReportItem = Report->Items[Index];

        //
        // does it have data
        //
        if (!ReportItem->HasData)
            continue;

        //
        // check usage page
        //
        CurrentUsagePage = (ReportItem->UsageMinimum >> 16);

        //
        // does usage match
        //
        if (UsagePage != CurrentUsagePage)
            continue;

        //
        // check if the specified usage is activated
        //
        ASSERT(ReportItem->ByteOffset < ReportDescriptorLength);
        ASSERT(ReportItem->BitCount < 8);

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

        //
        // store item
        //
        UsageList[ItemCount] = (ReportItem->UsageMinimum & 0xFFFF);
        ItemCount++;
    }

    if (ItemCount > *UsageLength)
    {
        //
        // list too small
        //
        return HIDPARSER_STATUS_BUFFER_TOO_SMALL;
    }

    //
    // success, clear rest of array
    //
    Parser->Zero(&UsageList[ItemCount], (*UsageLength - ItemCount) * sizeof(USAGE));

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
    IN ULONG ReportType,
    IN USAGE UsagePage,
    IN USAGE  Usage,
    OUT PLONG UsageValue,
    IN PCHAR ReportDescriptor,
    IN ULONG ReportDescriptorLength)
{
    PHID_PARSER_CONTEXT ParserContext;
    ULONG Index;
    PHID_REPORT Report;
    ULONG ItemCount = 0;
    USHORT CurrentUsagePage;
    PHID_REPORT_ITEM ReportItem;
    ULONG Data;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;

    //
    // sanity checks
    //
    ASSERT(ParserContext);

    //
    // FIXME support multiple top collecions
    //
    ASSERT(ParserContext->RootCollection->NodeCount == 1);

    //
    // get report
    //
    Report = HidParser_GetReportByType(Parser, ReportType);
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
        ReportItem = Report->Items[Index];

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
