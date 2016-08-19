/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/hidparser/context.c
 * PURPOSE:     HID Parser
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "parser.h"

#define NDEBUG
#include <debug.h>

typedef struct
{
    ULONG Size;
    union
    {
        UCHAR RawData[1];
    };
}HID_COLLECTION_CONTEXT, *PHID_COLLECTION_CONTEXT;

ULONG
HidParser_CalculateCollectionSize(
    IN PHID_COLLECTION Collection)
{
    ULONG Size = 0, Index;

    Size = sizeof(HID_COLLECTION);

    //
    // add size required for the number of report items
    //
    for(Index = 0; Index < Collection->ReportCount; Index++)
    {
        //
        // get report size
        //
        ASSERT(Collection->Reports[Index]->ItemCount);
        Size += sizeof(HID_REPORT) + Collection->Reports[Index]->ItemCount * sizeof(HID_REPORT_ITEM);
    }

    //
    // calculate size for sub collections
    //
    for(Index = 0; Index < Collection->NodeCount; Index++)
    {
        Size += HidParser_CalculateCollectionSize(Collection->Nodes[Index]);
    }

    //
    // append size for the offset
    //
    Size += (Collection->ReportCount + Collection->NodeCount) * sizeof(ULONG);

    //
    // done
    //
    return Size;
}

ULONG
HidParser_CalculateContextSize(
    IN PHID_COLLECTION Collection)
{
    ULONG Size;

    //
    // minimum size is the size of the collection
    //
    Size = HidParser_CalculateCollectionSize(Collection);

    //
    // append collection context size
    //
    Size += sizeof(HID_COLLECTION_CONTEXT);
    return Size;
}

ULONG
HidParser_StoreCollection(
    IN PHID_PARSER Parser,
    IN PHID_COLLECTION Collection,
    IN PHID_COLLECTION_CONTEXT CollectionContext,
    IN ULONG CurrentOffset)
{
    ULONG Index;
    ULONG ReportSize;
    ULONG InitialOffset;
    ULONG CollectionSize;
    PHID_COLLECTION TargetCollection;

    //
    // backup initial offset
    //
    InitialOffset = CurrentOffset;

    //
    // get target collection
    //
    TargetCollection = (PHID_COLLECTION)(&CollectionContext->RawData[CurrentOffset]);

    //
    // first copy the collection details
    //
    Parser->Copy(TargetCollection, Collection, sizeof(HID_COLLECTION));

    //
    // calulcate collection size
    //
    CollectionSize = sizeof(HID_COLLECTION) + sizeof(ULONG) * (Collection->ReportCount + Collection->NodeCount);

    //
    // increase offset
    //
    CurrentOffset += CollectionSize;

    //
    // sanity check
    //
    ASSERT(CurrentOffset < CollectionContext->Size);

    //
    // first store the report items
    //
    for(Index = 0; Index < Collection->ReportCount; Index++)
    {
        //
        // calculate report size
        //
        ReportSize = sizeof(HID_REPORT) + Collection->Reports[Index]->ItemCount * sizeof(HID_REPORT_ITEM);

        //
        // sanity check
        //
        ASSERT(CurrentOffset + ReportSize < CollectionContext->Size);

        //
        // copy report item
        //
        Parser->Copy(&CollectionContext->RawData[CurrentOffset], Collection->Reports[Index], ReportSize);

        //
        // store offset to report item
        //
        TargetCollection->Offsets[Index] = CurrentOffset;

        //
        // move to next offset
        //
        CurrentOffset += ReportSize;
    }

    ASSERT(CurrentOffset <= CollectionContext->Size);

    //
    // now store the sub collections
    //
    for(Index = 0; Index < Collection->NodeCount; Index++)
    {
        //
        // store offset
        //
        TargetCollection->Offsets[Collection->ReportCount + Index] = CurrentOffset;

        //
        // store sub collections
        //
        CurrentOffset += HidParser_StoreCollection(Parser, Collection->Nodes[Index], CollectionContext, CurrentOffset);

        //
        // sanity check
        //
        ASSERT(CurrentOffset < CollectionContext->Size);
    }

    //
    // return size of collection
    //
    return CurrentOffset - InitialOffset;
}

HIDPARSER_STATUS
HidParser_BuildCollectionContext(
    IN PHID_PARSER Parser,
    IN PHID_COLLECTION RootCollection,
    IN PVOID Context,
    IN ULONG ContextSize)
{
    PHID_COLLECTION_CONTEXT CollectionContext;
    ULONG CollectionSize;

    //
    // init context
    //
    CollectionContext = (PHID_COLLECTION_CONTEXT)Context;
    CollectionContext->Size = ContextSize;

    //
    // store collections
    //
    CollectionSize = HidParser_StoreCollection(Parser, RootCollection, CollectionContext, 0);

    //
    // sanity check
    //
    ASSERT(CollectionSize + sizeof(HID_COLLECTION_CONTEXT) == ContextSize);

    DPRINT("CollectionContext %p\n", CollectionContext);
    DPRINT("CollectionContext RawData %p\n", CollectionContext->RawData);
    DPRINT("CollectionContext Size %lu\n", CollectionContext->Size);

    //
    // done
    //
    return HIDPARSER_STATUS_SUCCESS;
}

PHID_REPORT
HidParser_SearchReportInCollection(
    IN PHID_COLLECTION_CONTEXT CollectionContext,
    IN PHID_COLLECTION Collection,
    IN UCHAR ReportType)
{
    ULONG Index;
    PHID_REPORT Report;
    PHID_COLLECTION SubCollection;

    //
    // search first in local array
    //
    for(Index = 0; Index < Collection->ReportCount; Index++)
    {
        //
        // get report
        //
        Report = (PHID_REPORT)(CollectionContext->RawData + Collection->Offsets[Index]);
        if (Report->Type == ReportType)
        {
            //
            // found report
            //
            return Report;
        }
    }

    //
    // now search in sub collections
    //
    for(Index = 0; Index < Collection->NodeCount; Index++)
    {
        //
        // get collection
        //
        SubCollection = (PHID_COLLECTION)(CollectionContext->RawData + Collection->Offsets[Collection->ReportCount + Index]);

        //
        // recursively search collection
        //
        Report = HidParser_SearchReportInCollection(CollectionContext, SubCollection, ReportType);
        if (Report)
        {
            //
            // found report
            //
            return Report;
        }
    }

    //
    // not found
    //
    return NULL;
}

PHID_REPORT
HidParser_GetReportInCollection(
    IN PVOID Context,
    IN UCHAR ReportType)
{
    PHID_COLLECTION_CONTEXT CollectionContext = (PHID_COLLECTION_CONTEXT)Context;

    //
    // done
    //
    return HidParser_SearchReportInCollection(CollectionContext, (PHID_COLLECTION)&CollectionContext->RawData, ReportType);
}

PHID_COLLECTION
HidParser_GetCollectionFromContext(
    IN PVOID Context)
{
    PHID_COLLECTION_CONTEXT CollectionContext = (PHID_COLLECTION_CONTEXT)Context;

    //
    // return root collection
    //
    return (PHID_COLLECTION)CollectionContext->RawData;
}

ULONG
HidParser_GetCollectionCount(
    IN PHID_COLLECTION_CONTEXT CollectionContext,
    IN PHID_COLLECTION Collection)
{
    ULONG Index;
    ULONG Count = Collection->NodeCount;
    PHID_COLLECTION SubCollection;

    for(Index = 0; Index < Collection->NodeCount; Index++)
    {
        //
        // get offset to sub collection
        //
        SubCollection = (PHID_COLLECTION)(CollectionContext->RawData + Collection->Offsets[Collection->ReportCount + Index]);

        //
        // count collection for sub nodes
        //
        Count += HidParser_GetCollectionCount(CollectionContext, SubCollection);
    }

    //
    // done
    //
    return Count;
}

ULONG
HidParser_GetTotalCollectionCount(
    IN PVOID Context)
{
    PHID_COLLECTION_CONTEXT CollectionContext;

    //
    // get parser context
    //
    CollectionContext = (PHID_COLLECTION_CONTEXT)Context;

    //
    // count collections
    //
    return HidParser_GetCollectionCount(CollectionContext, (PHID_COLLECTION)CollectionContext->RawData);
}
