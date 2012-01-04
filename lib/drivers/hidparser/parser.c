/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/hidparser/parser.c
 * PURPOSE:     HID Parser
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */


#include "parser.h"

static UCHAR ItemSize[4] = { 0, 1, 2, 4 };

VOID
HidParser_DeleteReport(
    IN PHID_PARSER Parser,
    IN PHID_REPORT Report)
{
    //
    // not implemented
    //
}

VOID
HidParser_FreeCollection(
    IN PHID_PARSER Parser,
    IN PHID_COLLECTION Collection)
{
    //
    // not implemented
    //
}

HIDPARSER_STATUS
HidParser_AllocateCollection(
    IN PHID_PARSER Parser,
    IN PHID_COLLECTION ParentCollection,
    IN UCHAR Type,
    IN PLOCAL_ITEM_STATE LocalItemState,
    OUT PHID_COLLECTION * OutCollection)
{
    PHID_COLLECTION Collection;
    USAGE_VALUE UsageValue;

    //
    // first allocate the collection
    //
    Collection = (PHID_COLLECTION)Parser->Alloc(sizeof(HID_COLLECTION));
    if (!Collection)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init collection
    //
    Collection->Root = ParentCollection;
    Collection->Type = Type;
    Collection->StringID = LocalItemState->StringIndex;
    Collection->PhysicalID = LocalItemState->DesignatorIndex;

    //
    // set Usage
    //
    ASSERT(LocalItemState);
    ASSERT(LocalItemState->UsageStack);

    if (LocalItemState->UsageStackUsed > 0)
    {
        //
        // usage value from first local stack item
        //
        UsageValue.u.Extended = LocalItemState->UsageStack[0].u.Extended;
    }
    else if (LocalItemState->UsageMinimumSet)
    {
        //
        // use value from minimum
        //
        UsageValue.u.Extended = LocalItemState->UsageMinimum.u.Extended;
    }
    else if (LocalItemState->UsageMaximumSet)
    {
        //
        // use value from maximum
        //
        UsageValue.u.Extended = LocalItemState->UsageMaximum.u.Extended;
    }
    else if (Type == COLLECTION_LOGICAL)
    {
        //
        // root collection
        //
        UsageValue.u.Extended = 0;
    }
    else
    {
        //
        // no usage set
        //
        Parser->Debug("HIDPARSE] No usage set\n");
        UsageValue.u.Extended = 0;
    }

    //
    // store usage
    //
    Collection->Usage = UsageValue.u.Extended;

    //
    // store result
    //
    *OutCollection = Collection;

    //
    // done
    // 
    return HIDPARSER_STATUS_SUCCESS;
}


VOID
HidParser_ResetParser(
    OUT PHID_PARSER Parser)
{
    ULONG Index;
    PHID_PARSER_CONTEXT ParserContext;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;


    //
    // delete all reports
    //
    for(Index = 0; Index < ParserContext->ReportCount; Index++)
    {
        //
        // delete report
        //
        HidParser_DeleteReport(Parser, ParserContext->Reports[Index]);
    }

    if (ParserContext->ReportCount && ParserContext->Reports)
    {
        //
        // free report array
        //
       Parser->Free(ParserContext->Reports);
    }

    if (ParserContext->RootCollection)
    {
        //
        // delete root collection
        //
        HidParser_FreeCollection(Parser, ParserContext->RootCollection);
    }

    //
    // reinit parser
    //
    ParserContext->ReportCount = 0;
    ParserContext->Reports = NULL;
    ParserContext->RootCollection = NULL;
    ParserContext->UseReportIDs = FALSE;

    //
    // zero item states
    //
    Parser->Zero(&ParserContext->GlobalItemState, sizeof(GLOBAL_ITEM_STATE));
    Parser->Zero(&ParserContext->LocalItemState, sizeof(LOCAL_ITEM_STATE));
}

HIDPARSER_STATUS
HidParser_AddCollection(
    IN PHID_PARSER Parser,
    IN PHID_COLLECTION CurrentCollection,
    IN PHID_COLLECTION NewCollection)
{
    PHID_COLLECTION * NewAllocCollection;
    ULONG CollectionCount;

    //
    // increment collection array
    //
    CollectionCount = CurrentCollection->NodeCount + 1;

    //
    // allocate new collection
    //
    NewAllocCollection = (PHID_COLLECTION*)Parser->Alloc(sizeof(PHID_COLLECTION) * CollectionCount);
    if (!NewAllocCollection)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    if (CurrentCollection->NodeCount)
    {
        //
        // copy old array
        //
        Parser->Copy(NewAllocCollection, CurrentCollection->Nodes, CurrentCollection->NodeCount * sizeof(PHID_COLLECTION));

        //
        // delete old array
        //
        Parser->Free(CurrentCollection->Nodes);
    }

    //
    // insert new item
    //
    NewAllocCollection[CurrentCollection->NodeCount] = (struct __HID_COLLECTION__*)NewCollection;


    //
    // store new array
    //
    CurrentCollection->Nodes = NewAllocCollection;
    CurrentCollection->NodeCount++;

    //
    // done
    //
    return HIDPARSER_STATUS_SUCCESS;
}

HIDPARSER_STATUS
HidParser_FindReport(
    IN PHID_PARSER Parser,
    IN UCHAR ReportType,
    IN UCHAR ReportID,
    OUT PHID_REPORT *OutReport)
{
    PHID_PARSER_CONTEXT ParserContext;
    ULONG Index;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;
    ASSERT(ParserContext);

    for(Index = 0; Index < ParserContext->ReportCount; Index++)
    {
        if (ParserContext->Reports[Index]->Type == ReportType && ParserContext->Reports[Index]->ReportID == ReportID)
        {
            //
            // found report
            //
            *OutReport = ParserContext->Reports[Index];
            return HIDPARSER_STATUS_SUCCESS;
        }
    }

    //
    // no such report found
    //
    *OutReport = NULL;
    return HIDPARSER_STATUS_REPORT_NOT_FOUND;
}

HIDPARSER_STATUS
HidParser_AllocateReport(
    IN PHID_PARSER Parser,
    IN UCHAR ReportType,
    IN UCHAR ReportID,
    OUT PHID_REPORT *OutReport)
{
    PHID_REPORT Report;

    //
    // allocate report
    //
    Report = (PHID_REPORT)Parser->Alloc(sizeof(HID_REPORT));
    if (!Report)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init report
    //
    Report->ReportID = ReportID;
    Report->Type = ReportType;

    //
    // done
    //
    *OutReport = Report;
    return HIDPARSER_STATUS_SUCCESS;
}

HIDPARSER_STATUS
HidParser_AddReport(
    IN PHID_PARSER Parser,
    IN PHID_REPORT NewReport)
{
    PHID_REPORT * NewReportArray;
    PHID_PARSER_CONTEXT ParserContext;

    //
    // get parser context
    //
    ParserContext = (PHID_PARSER_CONTEXT)Parser->ParserContext;
    ASSERT(ParserContext);

    //
    // allocate new report array
    //
    NewReportArray = (PHID_REPORT*)Parser->Alloc(sizeof(PHID_REPORT) * (ParserContext->ReportCount + 1));
    if (!NewReportArray)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ParserContext->ReportCount)
    {
        //
        // copy old array contents
        //
        Parser->Copy(NewReportArray, ParserContext->Reports, sizeof(PHID_REPORT) * ParserContext->ReportCount);

        //
        // free old array
        //
        Parser->Free(ParserContext->Reports);
    }

    //
    // store result
    //
    NewReportArray[ParserContext->ReportCount] = NewReport;
    ParserContext->Reports = NewReportArray;
    ParserContext->ReportCount++;

    //
    // completed successfully
    //
    return HIDPARSER_STATUS_SUCCESS;
}

HIDPARSER_STATUS
HidParser_GetReport(
    IN PHID_PARSER Parser,
    IN UCHAR ReportType,
    IN UCHAR ReportID,
    IN UCHAR CreateIfNotExists,
    OUT PHID_REPORT *OutReport)
{
    HIDPARSER_STATUS Status;

    //
    // try finding existing report
    //
    Status = HidParser_FindReport(Parser, ReportType, ReportID, OutReport);
    if (Status == HIDPARSER_STATUS_SUCCESS || CreateIfNotExists == FALSE)
    {
        //
        // founed report
        //
        return Status;
    }

    //
    // allocate new report
    //
    Status = HidParser_AllocateReport(Parser, ReportType, ReportID, OutReport);
    if (Status != HIDPARSER_STATUS_SUCCESS)
    {
        //
        // failed to allocate report
        //
        return Status;
    }

    //
    // add report
    //
    Status = HidParser_AddReport(Parser, *OutReport);
    if (Status != HIDPARSER_STATUS_SUCCESS)
    {
        //
        // failed to allocate report
        //
        Parser->Free(*OutReport);
    }

    //
    // done
    //
    return Status;
}


HIDPARSER_STATUS
HidParser_ReserveCollectionItems(
    IN PHID_PARSER Parser,
    IN PHID_COLLECTION Collection,
    IN ULONG ReportCount)
{
    PHID_REPORT_ITEM * NewReportArray;

    if (Collection->ItemCount + ReportCount <= Collection->ItemCountAllocated)
    {
        //
        // enough space for the next items
        //
        return HIDPARSER_STATUS_SUCCESS;
    }

    //
    // allocate report array
    //
    NewReportArray = (PHID_REPORT_ITEM*)Parser->Alloc(sizeof(PHID_REPORT) * (Collection->ItemCountAllocated + ReportCount));
    if (!NewReportArray)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // are there any items
    //
    if (Collection->ItemCount)
    {
        //
        // copy old items
        //
        Parser->Copy(NewReportArray, Collection->Items, sizeof(PHID_REPORT_ITEM) * Collection->ItemCount);

        //
        // free old item
        //
        Parser->Free(Collection->Items);
    }

    //
    // replace array
    //
    Collection->Items = NewReportArray;
    Collection->ItemCountAllocated += ReportCount;

    //
    // completed sucessfully
    //
    return HIDPARSER_STATUS_SUCCESS;
}


HIDPARSER_STATUS
HidParser_ReserveReportItems(
    IN PHID_PARSER Parser,
    IN PHID_REPORT Report,
    IN ULONG ReportCount)
{
    PHID_REPORT_ITEM * NewReportArray;

    if (Report->ItemCount + ReportCount <= Report->ItemAllocated)
    {
        //
        // enough space for the next items
        //
        return HIDPARSER_STATUS_SUCCESS;
    }

    //
    // allocate report array
    //
    NewReportArray = (PHID_REPORT_ITEM*)Parser->Alloc(sizeof(PHID_REPORT_ITEM) * (Report->ItemAllocated + ReportCount));
    if (!NewReportArray)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // are there any items
    //
    if (Report->ItemCount)
    {
        //
        // copy old items
        //
        Parser->Copy(NewReportArray, Report->Items, sizeof(PHID_REPORT_ITEM) * Report->ItemCount);

        //
        // free old item
        //
        Parser->Free(Report->Items);
    }

    //
    // replace array
    //
    Report->Items = NewReportArray;
    Report->ItemAllocated += ReportCount;

    //
    // completed sucessfully
    //
    return HIDPARSER_STATUS_SUCCESS;
}

HIDPARSER_STATUS
HidParser_AllocateReportItem(
    IN PHID_PARSER Parser,
    OUT PHID_REPORT_ITEM * OutReportItem)
{
    PHID_REPORT_ITEM ReportItem;

    //
    // allocate report item
    //
    ReportItem = (PHID_REPORT_ITEM)Parser->Alloc(sizeof(HID_REPORT_ITEM));
    if (!ReportItem)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // store result
    //
    *OutReportItem = ReportItem;
    return HIDPARSER_STATUS_SUCCESS;
}

VOID
HidParser_AddReportItemToReport(
    IN OUT PHID_REPORT Report,
    IN PHID_REPORT_ITEM ReportItem)
{
    //
    // there should be space in item array
    //
    ASSERT(Report->ItemCount + 1 <= Report->ItemAllocated);

    //
    // store item
    //
    Report->Items[Report->ItemCount] = ReportItem;
    Report->ItemCount++;
}

VOID
HidParser_AddReportItemToCollection(
    IN OUT PHID_COLLECTION Collection,
    IN PHID_REPORT_ITEM ReportItem)
{
    //
    // there should be space in item array
    //
    ASSERT(Collection->ItemCount + 1 <= Collection->ItemCountAllocated);

    //
    // store item
    //
    Collection->Items[Collection->ItemCount] = ReportItem;
    Collection->ItemCount++;
}

VOID
HidParser_SignRange(
    IN ULONG Minimum,
    IN ULONG Maximum,
    OUT PULONG NewMinimum,
    OUT PULONG NewMaximum)
{
    ULONG Mask = 0x80000000;
    ULONG Index;

    for (Index = 0; Index < 4; Index++)
    {
        if (Minimum & Mask) 
        {
            Minimum |= Mask;
            if (Maximum & Mask)
                Maximum |= Mask;
            return;
        }

        Mask >>= 8;
        Mask |= 0xff000000;
    }

    *NewMinimum = Minimum;
    *NewMaximum = Maximum;
}

HIDPARSER_STATUS
HidParser_InitReportItem(
    IN PHID_REPORT Report,
    IN PHID_REPORT_ITEM ReportItem,
    IN PGLOBAL_ITEM_STATE GlobalItemState,
    IN PLOCAL_ITEM_STATE LocalItemState,
    IN PMAIN_ITEM_DATA ItemData,
    IN ULONG ReportItemIndex)
{
    ULONG LogicalMinimum;
    ULONG LogicalMaximum;
    ULONG PhysicalMinimum;
    ULONG PhysicalMaximum;
    ULONG UsageMinimum;
    ULONG UsageMaximum;
    USAGE_VALUE UsageValue;

    //
    // get logical bounds
    //
    LogicalMinimum = GlobalItemState->LogicalMinimum;
    LogicalMaximum = GlobalItemState->LogicialMaximum;
    if (LogicalMinimum > LogicalMaximum)
    {
        //
        // make them signed
        //
        HidParser_SignRange(LogicalMinimum, LogicalMaximum, &LogicalMinimum, &LogicalMaximum);
    }
    //ASSERT(LogicalMinimum <= LogicalMaximum);

    //
    // get physical bounds
    //
    PhysicalMinimum = GlobalItemState->PhysicalMinimum;
    PhysicalMaximum = GlobalItemState->PhysicalMaximum;
    if (PhysicalMinimum > PhysicalMaximum)
    {
        //
        // make them signed
        //
        HidParser_SignRange(PhysicalMinimum, PhysicalMaximum, &PhysicalMinimum, &PhysicalMaximum);
    }
    //ASSERT(PhysicalMinimum <= PhysicalMaximum);

    //
    // get usage bounds
    //
    UsageMinimum = 0;
    UsageMaximum = 0;
    if (ItemData->ArrayVariable == FALSE)
    {
        //
        // get usage bounds
        //
        UsageMinimum = LocalItemState->UsageMinimum.u.Extended;
        UsageMaximum = LocalItemState->UsageMaximum.u.Extended;
    }
    else
    {
        //
        // get usage value from stack
        //
        if (ReportItemIndex < LocalItemState->UsageStackUsed)
        {
            //
            // use stack item
            //
            UsageValue = LocalItemState->UsageStack[ReportItemIndex];
        }
        else
        {
            //
            // get usage minimum from local state
            //
            UsageValue = LocalItemState->UsageMinimum;
            ASSERT(LocalItemState->UsageMinimumSet);
            ASSERT(LocalItemState->UsageMaximumSet);

            //
            // append item index
            //
            UsageValue.u.Extended += ReportItemIndex;

            if (UsageValue.u.Extended > LocalItemState->UsageMaximum.u.Extended)
            {
                //
                // maximum reached
                //
                UsageValue.u.Extended = LocalItemState->UsageMaximum.u.Extended;
            }
        }

        //
        // usage usage bounds
        //
        UsageMinimum = UsageMaximum = UsageValue.u.Extended;
    }

    //
    // now store all values
    //
    ReportItem->ByteOffset = (Report->ReportSize / 8);
    ReportItem->Shift = (Report->ReportSize % 8);
    ReportItem->Mask = ~(0xFFFFFFFF << GlobalItemState->ReportSize);
    ReportItem->BitCount = GlobalItemState->ReportSize;
    ReportItem->HasData = (ItemData->DataConstant == FALSE);
    ReportItem->Array = (ItemData->ArrayVariable == 0);
    ReportItem->Relative = (ItemData->Relative == TRUE);
    ReportItem->Minimum = LogicalMinimum;
    ReportItem->Maximum = LogicalMaximum;
    ReportItem->UsageMinimum = UsageMinimum;
    ReportItem->UsageMaximum = UsageMaximum;

    //
    // increment report size
    //
    Report->ReportSize += GlobalItemState->ReportSize;

    //
    // completed successfully
    //
    return HIDPARSER_STATUS_SUCCESS;
}

HIDPARSER_STATUS
HidParser_AddMainItem(
    IN PHID_PARSER Parser,
    IN PHID_REPORT Report,
    IN PGLOBAL_ITEM_STATE GlobalItemState,
    IN PLOCAL_ITEM_STATE LocalItemState,
    IN PMAIN_ITEM_DATA ItemData,
    IN PHID_COLLECTION Collection)
{
    PHID_REPORT_ITEM ReportItem;
    HIDPARSER_STATUS Status;
    ULONG Index;

    //
    // first grow report item array
    //
    Status = HidParser_ReserveReportItems(Parser, Report, GlobalItemState->ReportCount);
    if (Status != HIDPARSER_STATUS_SUCCESS)
    {
        //
        // failed to allocate memory
        //
        return Status;
    }

    //
    // grow collection item array
    //
    Status = HidParser_ReserveCollectionItems(Parser, Collection, GlobalItemState->ReportCount);
    if (Status != HIDPARSER_STATUS_SUCCESS)
    {
        //
        // failed to allocate memory
        //
        return Status;
    }



    for(Index = 0; Index < GlobalItemState->ReportCount; Index++)
    {
        //
        // create report item
        //
        Status = HidParser_AllocateReportItem(Parser, &ReportItem);
        if (Status != HIDPARSER_STATUS_SUCCESS)
        {
            //
            // failed to allocate memory
            //
            return Status;
        }

        Status = HidParser_InitReportItem(Report, ReportItem, GlobalItemState, LocalItemState, ItemData, Index);
        if (Status != HIDPARSER_STATUS_SUCCESS)
        {
            //
            // failed to init report item
            //
            return Status;
        }

        //
        // add report item
        //
        HidParser_AddReportItemToReport(Report, ReportItem);
        HidParser_AddReportItemToCollection(Collection, ReportItem);
    }

    //
    // done
    //
    return HIDPARSER_STATUS_SUCCESS;
}

HIDPARSER_STATUS
HidParser_ParseReportDescriptor(
    IN PHID_PARSER Parser,
    IN PUCHAR ReportDescriptor,
    IN ULONG ReportLength)
{
    PGLOBAL_ITEM_STATE LinkedGlobalItemState, NextLinkedGlobalItemState;
    ULONG Index;
    PUSAGE_VALUE NewUsageStack, UsageValue;
    HIDPARSER_STATUS Status;
    PHID_COLLECTION CurrentCollection, NewCollection;
    PUCHAR CurrentOffset, ReportEnd;
    PITEM_PREFIX CurrentItem;
    ULONG CurrentItemSize;
    PLONG_ITEM CurrentLongItem;
    PSHORT_ITEM CurrentShortItem;
    ULONG Data;
    UCHAR ReportType;
    PHID_REPORT Report;
    PMAIN_ITEM_DATA MainItemData;
    PHID_PARSER_CONTEXT ParserContext;

    //
    // reset parser
    //
    HidParser_ResetParser(Parser);

    //
    // get parser context
    //
    ParserContext =(PHID_PARSER_CONTEXT)Parser->ParserContext;
    ASSERT(ParserContext);

    //
    // allocate usage stack
    //
    ParserContext->LocalItemState.UsageStackAllocated = 10;
    ParserContext->LocalItemState.UsageStack = (PUSAGE_VALUE)Parser->Alloc(ParserContext->LocalItemState.UsageStackAllocated * sizeof(USAGE_VALUE));
    if (!ParserContext->LocalItemState.UsageStack)
    {
        //
        // no memory
        //
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // now allocate root collection
    //
    Status = HidParser_AllocateCollection(Parser, NULL, COLLECTION_LOGICAL, &ParserContext->LocalItemState, &ParserContext->RootCollection);
    if (Status != HIDPARSER_STATUS_SUCCESS)
    {
        //
        // no memory
        //
        Parser->Free(ParserContext->LocalItemState.UsageStack);
        ParserContext->LocalItemState.UsageStack = NULL;
        return HIDPARSER_STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // start parsing
    //
    CurrentCollection = ParserContext->RootCollection;
    CurrentOffset = ReportDescriptor;
    ReportEnd = ReportDescriptor + ReportLength;

    do
    {
        //
        // get current item
        //
        CurrentItem = (PITEM_PREFIX)CurrentOffset;

        //
        // get item size
        //
        CurrentItemSize = ItemSize[CurrentItem->Size];
        Data = 0;

        if (CurrentItem->Type == ITEM_TYPE_LONG)
        {
            //
            // increment item size with size of data item
            //
            CurrentLongItem = (PLONG_ITEM)CurrentItem;
            CurrentItemSize += CurrentLongItem->DataSize;
        }
        else
        {
            //
            // get short item
            //
            CurrentShortItem = (PSHORT_ITEM)CurrentItem;

            //
            // get associated data
            //
            //ASSERT(CurrentItemSize == 1 || CurrentItemSize == 2 || CurrentItemSize == 4);
            if (CurrentItemSize == 1)
                Data = CurrentShortItem->Data.UData8[0];
            else if (CurrentItemSize == 2)
                Data = CurrentShortItem->Data.UData16[0];
            else if (CurrentItemSize == 4)
                Data = CurrentShortItem->Data.UData32;
            else
            {
                //
                // invalid item size
                //
                //Parser->Debug("CurrentItem invalid item size %lu\n", CurrentItemSize);
            }

        }

        //
        // handle items
        //
        ASSERT(CurrentItem->Type >= ITEM_TYPE_MAIN && CurrentItem->Type <= ITEM_TYPE_LONG);
        switch(CurrentItem->Type)
        {
            case ITEM_TYPE_MAIN:
            {
                // preprocess the local state if relevant (usages for
                // collections and report items)
                if (CurrentItem->Tag != ITEM_TAG_MAIN_END_COLLECTION) 
                {
                    // make all usages extended for easier later processing
                    for (Index = 0; Index < ParserContext->LocalItemState.UsageStackUsed; Index++) 
                    {
                        //
                        // is it already extended
                        //
                        if (ParserContext->LocalItemState.UsageStack[Index].IsExtended)
                            continue;

                        //
                        // extend usage item
                        //
                        ParserContext->LocalItemState.UsageStack[Index].u.s.UsagePage = ParserContext->GlobalItemState.UsagePage;
                        ParserContext->LocalItemState.UsageStack[Index].IsExtended = TRUE;
                    }

                    if (!ParserContext->LocalItemState.UsageMinimum.IsExtended) {
                        // the specs say if one of them is extended they must
                        // both be extended, so if the minimum isn't, the
                        // maximum mustn't either.
                        ParserContext->LocalItemState.UsageMinimum.u.s.UsagePage
                            = ParserContext->LocalItemState.UsageMaximum.u.s.UsagePage
                                = ParserContext->GlobalItemState.UsagePage;
                        ParserContext->LocalItemState.UsageMinimum.IsExtended
                            = ParserContext->LocalItemState.UsageMaximum.IsExtended = TRUE;
                    }

                    //LocalItemState.usage_stack = usageStack;
                    //ParserContext->LocalItemState.UsageStackUsed = UsageStackUsed;
                }

                if (CurrentItem->Tag == ITEM_TAG_MAIN_COLLECTION) {

                    //
                    // allocate new collection
                    //
                    Status = HidParser_AllocateCollection(Parser, CurrentCollection, (UCHAR)Data, &ParserContext->LocalItemState, &NewCollection);
                    ASSERT(Status == HIDPARSER_STATUS_SUCCESS);

                    //
                    // add new collection to current collection
                    //
                    Status = HidParser_AddCollection(Parser, CurrentCollection, NewCollection);
                    ASSERT(Status == HIDPARSER_STATUS_SUCCESS);

                    //
                    // make new collection current
                    //
                    CurrentCollection = NewCollection;
                } 
                else if (CurrentItem->Tag == ITEM_TAG_MAIN_END_COLLECTION) 
                {
                    //
                    // assert on ending the root collection
                    //
                    ASSERT(CurrentCollection != ParserContext->RootCollection);

                    //
                    // use parent of current collection
                    //
                    CurrentCollection = CurrentCollection->Root;
                    ASSERT(CurrentCollection);
                } 
                else 
                {
                    ReportType = HID_REPORT_TYPE_ANY;

                    switch (CurrentItem->Tag) {
                        case ITEM_TAG_MAIN_INPUT:
                            ReportType = HID_REPORT_TYPE_INPUT;
                            break;

                        case ITEM_TAG_MAIN_OUTPUT:
                            ReportType = HID_REPORT_TYPE_OUTPUT;
                            break;

                        case ITEM_TAG_MAIN_FEATURE:
                            ReportType = HID_REPORT_TYPE_FEATURE;
                            break;

                        default:
                            Parser->Debug("[HIDPARSE] Unknown ReportType %x\n", CurrentItem->Tag);
                            ASSERT(FALSE);
                            break;
                    }

                    if (ReportType == HID_REPORT_TYPE_ANY)
                        break;

                    //
                    // get report
                    //
                    Status = HidParser_GetReport(Parser, ReportType, ParserContext->GlobalItemState.ReportId, TRUE, &Report);
                    ASSERT(Status == HIDPARSER_STATUS_SUCCESS);

                    // fill in a sensible default if the index isn't set
                    if (!ParserContext->LocalItemState.DesignatorIndexSet) {
                        ParserContext->LocalItemState.DesignatorIndex
                            = ParserContext->LocalItemState.DesignatorMinimum;
                    }

                    if (!ParserContext->LocalItemState.StringIndexSet)
                        ParserContext->LocalItemState.StringIndex = ParserContext->LocalItemState.StringMinimum;

                    //
                    // get main item data
                    //
                    MainItemData = (PMAIN_ITEM_DATA)&Data;

                    //
                    // add states & data to the report
                    //
                    Status = HidParser_AddMainItem(Parser, Report, &ParserContext->GlobalItemState, &ParserContext->LocalItemState, MainItemData, CurrentCollection);
                    ASSERT(Status == HIDPARSER_STATUS_SUCCESS);
                }

                //
                // backup stack
                //
                Index = ParserContext->LocalItemState.UsageStackAllocated;
                NewUsageStack = ParserContext->LocalItemState.UsageStack;

                //
                // reset the local item state and clear the usage stack
                //
                Parser->Zero(&ParserContext->LocalItemState, sizeof(LOCAL_ITEM_STATE));

                //
                // restore stack
                //
                ParserContext->LocalItemState.UsageStack = NewUsageStack;
                ParserContext->LocalItemState.UsageStackAllocated = Index;
                break;
            }
            case ITEM_TYPE_GLOBAL:
            {
                switch (CurrentItem->Tag) {
                    case ITEM_TAG_GLOBAL_USAGE_PAGE:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_USAGE_PAGE %x\n", Data);
                        ParserContext->GlobalItemState.UsagePage = Data;
                        break;
                    case ITEM_TAG_GLOBAL_LOGICAL_MINIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_LOGICAL_MINIMUM %x\n", Data);
                        ParserContext->GlobalItemState.LogicalMinimum = Data;
                        break;

                    case ITEM_TAG_GLOBAL_LOGICAL_MAXIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_LOCAL_MAXIMUM %x\n", Data);
                        ParserContext->GlobalItemState.LogicialMaximum = Data;
                        break;

                    case ITEM_TAG_GLOBAL_PHYSICAL_MINIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_PHYSICAL_MINIMUM %x\n", Data);
                        ParserContext->GlobalItemState.PhysicalMinimum = Data;
                        break;

                    case ITEM_TAG_GLOBAL_PHYSICAL_MAXIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_PHYSICAL_MAXIMUM %x\n", Data);
                        ParserContext->GlobalItemState.PhysicalMaximum = Data;
                        break;

                    case ITEM_TAG_GLOBAL_UNIT_EXPONENT:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_UNIT_EXPONENT %x\n", Data);
                        ParserContext->GlobalItemState.UnitExponent = Data;
                        break;

                    case ITEM_TAG_GLOBAL_UNIT:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_UNIT %x\n", Data);
                        ParserContext->GlobalItemState.Unit = Data;
                        break;

                    case ITEM_TAG_GLOBAL_REPORT_SIZE:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_SIZE %x\n", Data);
                        ParserContext->GlobalItemState.ReportSize = Data;
                        break;

                    case ITEM_TAG_GLOBAL_REPORT_ID:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_ID %x\n", Data);
                        ParserContext->GlobalItemState.ReportId = Data;
                        ParserContext->UseReportIDs = TRUE;
                        break;

                    case ITEM_TAG_GLOBAL_REPORT_COUNT:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_COUNT %x\n", Data);
                        ParserContext->GlobalItemState.ReportCount = Data;
                        break;

                    case ITEM_TAG_GLOBAL_PUSH:
                    {
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_PUSH\n");
                        //
                        // allocate global item state
                        //
                        LinkedGlobalItemState = (PGLOBAL_ITEM_STATE)Parser->Alloc(sizeof(GLOBAL_ITEM_STATE));
                        ASSERT(LinkedGlobalItemState);

                        //
                        // copy global item state
                        //
                        Parser->Copy(LinkedGlobalItemState, &ParserContext->GlobalItemState, sizeof(GLOBAL_ITEM_STATE));

                        //
                        // store pushed item in link member
                        //
                        ParserContext->GlobalItemState.Next = (struct __GLOBAL_ITEM_STATE__*)LinkedGlobalItemState;
                        break;
                    }
                    case ITEM_TAG_GLOBAL_POP:
                    {
                        Parser->Debug("[HIDPARSE] ITEM_TAG_GLOBAL_POP\n");
                        if (ParserContext->GlobalItemState.Next == NULL) 
                        {
                            //
                            // pop without push
                            //
                            ASSERT(FALSE);
                            break;
                        }

                        //
                        // get link
                        //
                        LinkedGlobalItemState = (PGLOBAL_ITEM_STATE)ParserContext->GlobalItemState.Next;

                        //
                        // replace current item with linked one
                        //
                        Parser->Copy(&ParserContext->GlobalItemState, LinkedGlobalItemState, sizeof(GLOBAL_ITEM_STATE));

                        //
                        // free item
                        //
                        Parser->Free(LinkedGlobalItemState);
                        break;
                    }

                    default:
                        //
                        // unknown  / unsupported tag
                        //
                        ASSERT(FALSE);
                        break;
                }

                break;
            }
            case ITEM_TYPE_LOCAL:
            {
                switch (CurrentItem->Tag) 
                {
                    case ITEM_TAG_LOCAL_USAGE:
                    {
                        if (ParserContext->LocalItemState.UsageStackUsed >= ParserContext->LocalItemState.UsageStackAllocated)
                        {
                            //
                            // increment stack size
                            //
                            ParserContext->LocalItemState.UsageStackAllocated += 10;

                            //
                            // build new usage stack
                            //
                            NewUsageStack = (PUSAGE_VALUE)Parser->Alloc(sizeof(USAGE_VALUE) * ParserContext->LocalItemState.UsageStackAllocated);
                            ASSERT(NewUsageStack);

                            //
                            // copy old usage stack
                            //
                            Parser->Copy(NewUsageStack, ParserContext->LocalItemState.UsageStack, sizeof(USAGE_VALUE) * (ParserContext->LocalItemState.UsageStackAllocated - 10));

                            //
                            // free old usage stack
                            //
                            Parser->Free(ParserContext->LocalItemState.UsageStack);

                            //
                            // replace with new usage stack
                            //
                            ParserContext->LocalItemState.UsageStack = NewUsageStack;
                        }

                        //
                        // get fresh usage value
                        //
                        UsageValue = &ParserContext->LocalItemState.UsageStack[ParserContext->LocalItemState.UsageStackUsed];

                        //
                        // init usage stack
                        //
                        UsageValue->IsExtended = CurrentItemSize == sizeof(ULONG);
                        UsageValue->u.Extended = Data;

                        //
                        // increment usage stack usage count
                        //
                        ParserContext->LocalItemState.UsageStackUsed++;
                        break;
                    }

                    case ITEM_TAG_LOCAL_USAGE_MINIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_LOCAL_USAGE_MINIMUM Data %x\n", Data);
                        ParserContext->LocalItemState.UsageMinimum.u.Extended = Data;
                        ParserContext->LocalItemState.UsageMinimum.IsExtended
                            = CurrentItemSize == sizeof(ULONG);
                        ParserContext->LocalItemState.UsageMinimumSet = TRUE;
                        break;

                    case ITEM_TAG_LOCAL_USAGE_MAXIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_LOCAL_USAGE_MAXIMUM Data %x\n", Data);
                        ParserContext->LocalItemState.UsageMaximum.u.Extended = Data;
                        ParserContext->LocalItemState.UsageMaximum.IsExtended
                            = CurrentItemSize == sizeof(ULONG);
                        ParserContext->LocalItemState.UsageMaximumSet = TRUE;
                        break;

                    case ITEM_TAG_LOCAL_DESIGNATOR_INDEX:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_LOCAL_DESIGNATOR_INDEX Data %x\n", Data);
                        ParserContext->LocalItemState.DesignatorIndex = Data;
                        ParserContext->LocalItemState.DesignatorIndexSet = TRUE;
                        break;

                    case ITEM_TAG_LOCAL_DESIGNATOR_MINIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_LOCAL_DESIGNATOR_MINIMUM Data %x\n", Data);
                        ParserContext->LocalItemState.DesignatorMinimum = Data;
                        break;

                    case ITEM_TAG_LOCAL_DESIGNATOR_MAXIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_LOCAL_DESIGNATOR_MAXIMUM Data %x\n", Data);
                        ParserContext->LocalItemState.DesignatorMaximum = Data;
                        break;

                    case ITEM_TAG_LOCAL_STRING_INDEX:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_LOCAL_STRING_INDEX Data %x\n", Data);
                        ParserContext->LocalItemState.StringIndex = Data;
                        ParserContext->LocalItemState.StringIndexSet = TRUE;
                        break;

                    case ITEM_TAG_LOCAL_STRING_MINIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_LOCAL_STRING_MINIMUM Data %x\n", Data);
                        ParserContext->LocalItemState.StringMinimum = Data;
                        break;

                    case ITEM_TAG_LOCAL_STRING_MAXIMUM:
                        Parser->Debug("[HIDPARSE] ITEM_TAG_LOCAL_STRING_MAXIMUM Data %x\n", Data);
                        ParserContext->LocalItemState.StringMaximum = Data;
                        break;

                    default:
                        Parser->Debug("Unknown Local Item Tag %x\n", CurrentItem->Tag);
                        ASSERT(FALSE);
                        break;
                }
                break;
            }

            case ITEM_TYPE_LONG:
            {
                CurrentLongItem = (PLONG_ITEM)CurrentItem;
                Parser->Debug("Unsupported ITEM_TYPE_LONG Tag %x\n", CurrentLongItem->LongItemTag);
                break;
            }
        }

        //
        // move to next item
        //
        CurrentOffset += CurrentItemSize + sizeof(ITEM_PREFIX);


    }while(CurrentOffset < ReportEnd);


    //
    // cleanup global stack
    //
    LinkedGlobalItemState = (PGLOBAL_ITEM_STATE)ParserContext->GlobalItemState.Next;
    while(LinkedGlobalItemState != NULL)
    {
        Parser->Debug("[HIDPARSE] Freeing GlobalState %p\n", LinkedGlobalItemState);
        //
        // free global item state
        //
        NextLinkedGlobalItemState = (PGLOBAL_ITEM_STATE)LinkedGlobalItemState->Next;

        //
        // free state
        //
        Parser->Free(LinkedGlobalItemState);

        //
        // move to next global state
        //
        LinkedGlobalItemState = NextLinkedGlobalItemState;
    }

    //
    // free usage stack
    //
    Parser->Free(ParserContext->LocalItemState.UsageStack);
    ParserContext->LocalItemState.UsageStack = NULL;

    //
    // done
    //
    return HIDPARSER_STATUS_SUCCESS;
}

