/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser
 * COPYRIGHT:   Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

/* Purpose of this file is to parse the HID report descriptor into preparsed data
 * structure. The idea about this structure arragement is taken from wine, and extended
 * based on results of tests(see ).
 * TODO:
 * Delimeters
 * PoolType
 * DebugField
 */

#include "parser.h"
#include <hidpmem.h>

#define NDEBUG
#include <debug.h>

typedef struct _HIDPARSER_CONTEXT
{
    BOOLEAN IsDelimiterOpen;
    BOOLEAN IsDelimiterProcessed;

    LIST_ENTRY workingListHead;
    LIST_ENTRY outputListHead;
    LIST_ENTRY nodesListHead;

    PHIDPARSER_GLOBAL_STACK globalStack;

    PHIDPARSER_COLLECTION_STACK collectionStack;
    PHIDPARSER_REPORT_IDS_STACK reportIDsStack;

    PHIDPARSER_NODES_LIST currentNode;
    PHIDP_REPORT_IDS currentReport;

    INT32 nodeDepth;
    INT32 workingCount;

} HIDPARSER_CONTEXT, *PHIDPARSER_CONTEXT;

/* HidParser_CombinePreparsedData takes the ValueCaps, Nodes and ReportIDs
 *  to combine into actual preparsedData structure
 */
NTSTATUS
HidParser_CombinePreparsedData(
    IN PLIST_ENTRY OutputListHead,
    IN PLIST_ENTRY NodesListTail,
    IN PHIDPARSER_REPORT_IDS_STACK ReportIDsStack,
    OUT PHIDPARSER_COLLECTION_STACK CurrentCollection)
{
    UINT32 valuesCount = 0, nodesCount = 0, dataCount = 0;
    USHORT inputReportMax = 0, outputReportMax = 0, featureReportMax = 0;
    PUCHAR currentOffset = 0;

    PLIST_ENTRY listTemp;
    PHIDPARSER_VALUE_CAPS_LIST outputListTmp = NULL;
    PHIDPARSER_NODES_LIST nodesListTemp = NULL;
    PHIDPARSER_REPORT_IDS_STACK reportIDsTemp;
    PHIDPARSER_PREPARSED_DATA preparsedData = NULL;

    if (NodesListTail == NULL)
    {
        return STATUS_COULD_NOT_INTERPRET;
    }

    for (listTemp = OutputListHead->Flink; listTemp != OutputListHead; listTemp = listTemp->Flink)
    {
        valuesCount++;
    }

    for (listTemp = NodesListTail->Flink; listTemp != NodesListTail; listTemp = listTemp->Flink)
    {
        nodesCount++;
    }

    preparsedData = AllocFunction(
        sizeof(HIDPARSER_PREPARSED_DATA) + sizeof(HIDPARSER_VALUE_CAPS) * (valuesCount - 1) +
        sizeof(HIDPARSER_LINK_COLLECTION_NODE) * nodesCount);
    if (preparsedData == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    currentOffset = (PUCHAR)preparsedData->ValueCaps;

    for (reportIDsTemp = ReportIDsStack; reportIDsTemp != NULL; reportIDsTemp = reportIDsTemp->Next)
    {
        if (reportIDsTemp->Value.CollectionNumber == CurrentCollection->Index)
        {
            if (reportIDsTemp->Value.InputLength > inputReportMax)
            {
                inputReportMax = reportIDsTemp->Value.InputLength;
            }
            if (reportIDsTemp->Value.OutputLength > outputReportMax)
            {
                outputReportMax = reportIDsTemp->Value.OutputLength;
            }
            if (reportIDsTemp->Value.FeatureLength > featureReportMax)
            {
                featureReportMax = reportIDsTemp->Value.FeatureLength;
            }
        }
    }
    inputReportMax = inputReportMax > 0 ? (inputReportMax / 8) + 1 : 0;
    outputReportMax = outputReportMax > 0 ? (outputReportMax / 8) + 1 : 0;
    featureReportMax = featureReportMax > 0 ? (featureReportMax / 8) + 1 : 0;

    preparsedData->CapsByteLength = 0;

    preparsedData->InputCapsStart = 0;
    preparsedData->InputCapsCount = 0;
    preparsedData->InputCapsEnd = 0;
    preparsedData->InputReportByteLength = inputReportMax;
    dataCount = 0;
    for (listTemp = OutputListHead->Flink; listTemp != OutputListHead; listTemp = listTemp->Flink)
    {
        outputListTmp = CONTAINING_RECORD(listTemp, HIDPARSER_VALUE_CAPS_LIST, Entry);

        if (outputListTmp->ReportType == HidP_Input)
        {
            outputListTmp->Value.DataIndexMin = dataCount;
            dataCount += outputListTmp->Value.UsageMax - outputListTmp->Value.UsageMin;
            outputListTmp->Value.DataIndexMax = dataCount;
            dataCount++;

            CopyFunction(currentOffset, &outputListTmp->Value, sizeof(HIDPARSER_VALUE_CAPS));

            preparsedData->InputCapsCount++;
            preparsedData->InputCapsEnd++;
            preparsedData->CapsByteLength += sizeof(HIDPARSER_VALUE_CAPS);
            currentOffset += sizeof(HIDPARSER_VALUE_CAPS);
        }
    }

    preparsedData->OutputCapsStart = preparsedData->InputCapsEnd;
    preparsedData->OutputCapsCount = 0;
    preparsedData->OutputCapsEnd = preparsedData->InputCapsEnd;
    preparsedData->OutputReportByteLength = outputReportMax;
    dataCount = 0;
    for (listTemp = OutputListHead->Flink; listTemp != OutputListHead; listTemp = listTemp->Flink)
    {
        outputListTmp = CONTAINING_RECORD(listTemp, HIDPARSER_VALUE_CAPS_LIST, Entry);

        if (outputListTmp->ReportType == HidP_Output)
        {
            outputListTmp->Value.DataIndexMin = dataCount;
            dataCount += outputListTmp->Value.UsageMax - outputListTmp->Value.UsageMin;
            outputListTmp->Value.DataIndexMax = dataCount;
            dataCount++;

            CopyFunction(currentOffset, &outputListTmp->Value, sizeof(HIDPARSER_VALUE_CAPS));

            preparsedData->OutputCapsCount++;
            preparsedData->OutputCapsEnd++;
            preparsedData->CapsByteLength += sizeof(HIDPARSER_VALUE_CAPS);
            currentOffset += sizeof(HIDPARSER_VALUE_CAPS);
        }
    }

    preparsedData->FeatureCapsStart = preparsedData->OutputCapsEnd;
    preparsedData->FeatureCapsCount = 0;
    preparsedData->FeatureCapsEnd = preparsedData->OutputCapsEnd;
    preparsedData->FeatureReportByteLength = featureReportMax;
    dataCount = 0;
    for (listTemp = OutputListHead->Flink; listTemp != OutputListHead; listTemp = listTemp->Flink)
    {
        outputListTmp = CONTAINING_RECORD(listTemp, HIDPARSER_VALUE_CAPS_LIST, Entry);

        if (outputListTmp->ReportType == HidP_Feature)
        {
            outputListTmp->Value.DataIndexMin = dataCount;
            dataCount += outputListTmp->Value.UsageMax - outputListTmp->Value.UsageMin;
            outputListTmp->Value.DataIndexMax = dataCount;
            dataCount++;

            CopyFunction(currentOffset, &outputListTmp->Value, sizeof(HIDPARSER_VALUE_CAPS));

            preparsedData->FeatureCapsCount++;
            preparsedData->FeatureCapsEnd++;
            preparsedData->CapsByteLength += sizeof(HIDPARSER_VALUE_CAPS);
            currentOffset += sizeof(HIDPARSER_VALUE_CAPS);
        }
    }

    preparsedData->LinkCollectionCount = 0;
    for (listTemp = NodesListTail->Blink; listTemp != NodesListTail; listTemp = listTemp->Blink)
    {
        nodesListTemp = CONTAINING_RECORD(listTemp, HIDPARSER_NODES_LIST, Entry);

        CopyFunction(currentOffset, &nodesListTemp->Value, sizeof(HIDPARSER_LINK_COLLECTION_NODE));

        preparsedData->LinkCollectionCount++;
        currentOffset += sizeof(HIDPARSER_LINK_COLLECTION_NODE);
    }

    nodesListTemp = CONTAINING_RECORD(NodesListTail->Blink, HIDPARSER_NODES_LIST, Entry);
    preparsedData->Usage = nodesListTemp->Value.LinkUsage;
    preparsedData->UsagePage = nodesListTemp->Value.LinkUsagePage;

    // return created data
    CopyFunction(preparsedData->Magic, PreparsedMagic, sizeof(CurrentCollection->PreparsedData->Magic));
    CurrentCollection->PreparsedData = preparsedData;
    CurrentCollection->PreparsedSize = sizeof(HIDPARSER_PREPARSED_DATA) + sizeof(HIDPARSER_VALUE_CAPS) * (valuesCount - 1) +
        sizeof(HIDPARSER_LINK_COLLECTION_NODE) * nodesCount;
    return STATUS_SUCCESS;
}

PHIDP_REPORT_IDS HidParser_GetCurrentReportIDS(
    IN PHIDPARSER_CONTEXT Context
)
{
    PHIDPARSER_REPORT_IDS_STACK reportIDsTemp;

    // process reports stack
    for (reportIDsTemp = Context->reportIDsStack; reportIDsTemp != NULL; reportIDsTemp = reportIDsTemp->Next)
    {
        if (reportIDsTemp->Value.CollectionNumber == Context->collectionStack->Index)
        {
            if (reportIDsTemp->Value.ReportID == Context->globalStack->ReportID)
            {
                break;
            }
        }
    }

    if (reportIDsTemp == NULL)
    {
        reportIDsTemp = AllocFunction(sizeof(HIDPARSER_REPORT_IDS_STACK));
        if (reportIDsTemp == NULL)
        {
            return NULL;
        }

        reportIDsTemp->Value.ReportID = Context->globalStack->ReportID;
        // actually hid spec does not define reports limit
        reportIDsTemp->Value.CollectionNumber = (UCHAR)Context->collectionStack->Index;

        // link report to stack
        reportIDsTemp->Next = Context->reportIDsStack;
        Context->reportIDsStack = reportIDsTemp;
    }

    return &reportIDsTemp->Value;
}

NTSTATUS
HidParser_ProcessReportTag(
    IN PHIDPARSER_CONTEXT Context,
    IN ULONG Data,
    IN HIDP_REPORT_TYPE ReportType,
    OUT USHORT* reportSizeBits
)
{
    PHIDPARSER_VALUE_CAPS_LIST workingListTemp;
    PLIST_ENTRY listTemp;
    HIDPARSER_ITEM_FLAGS flags;

    // we must have atleast 1 usages in stack
    ASSERT(Context->currentNode != NULL);

    // if usage was not setted up, and type is constant
    flags.Raw = Data;
    if (Context->workingCount < 1)
    {
        if (flags.Flags.IsConstant == TRUE)
        {
            // skip bits and exit
            *reportSizeBits += Context->globalStack->ReportSize * Context->globalStack->ReportCount;
            return STATUS_SUCCESS;
        }
        else
        {
            // variable with no usage
            ASSERT(FALSE);
        }
    }

    // process usages
    for (listTemp = Context->workingListHead.Blink; listTemp != &Context->workingListHead; listTemp = listTemp->Blink)
    {
        workingListTemp = CONTAINING_RECORD(listTemp, HIDPARSER_VALUE_CAPS_LIST, Entry);

        // copy common items
        workingListTemp->ReportType = ReportType;
        workingListTemp->Value.BitField.Raw = Data;
        // copy global items
        workingListTemp->Value.Flags.UnknownGlobalCount = Context->globalStack->UnknownGlobalCount;
        CopyFunction(workingListTemp->Value.UnknownGlobals, Context->globalStack->UnknownGlobals, sizeof(workingListTemp->Value.UnknownGlobals));
        workingListTemp->Value.Units = Context->globalStack->Units;
        workingListTemp->Value.UnitsExp = Context->globalStack->UnitsExp;
        workingListTemp->Value.ReportSize = Context->globalStack->ReportSize;
        workingListTemp->Value.ReportID = Context->globalStack->ReportID;
        workingListTemp->Value.ReportCount = Context->globalStack->ReportCount;
        if (workingListTemp->Value.UsagePage == 0)
        {
            workingListTemp->Value.UsagePage = Context->globalStack->UsagePage;
        }

        // setup node linkage
        workingListTemp->Value.LinkCollection = Context->currentNode->Index;
        workingListTemp->Value.LinkUsagePage = Context->currentNode->Value.LinkUsagePage;
        workingListTemp->Value.LinkUsage = Context->currentNode->Value.LinkUsage;

        // process size
        if (Context->workingCount > 1)
        {
            if (workingListTemp->Value.ReportCount > 1)
            {
                //ASSERT(workingListTemp->Value.ReportCount == workingCount);
                workingListTemp->Value.ReportCount = 1;
            }
        }
        workingListTemp->Value.StartByte = (*reportSizeBits / 8) + 1;
        workingListTemp->Value.StartBit = *reportSizeBits % 8;
        workingListTemp->Value.TotalBits =
            workingListTemp->Value.ReportSize * workingListTemp->Value.ReportCount;
        workingListTemp->Value.EndByte =
            ((*reportSizeBits + 7 + workingListTemp->Value.TotalBits) / 8) + 1;
        if (workingListTemp->Value.BitField.Flags.IsVariable == TRUE ||
            workingListTemp->Entry.Blink == &Context->workingListHead)
        {
            // always add for variable and add once for arrays
            *reportSizeBits += workingListTemp->Value.TotalBits;
        }

        // process flags
        if (workingListTemp->Value.BitField.Flags.IsVariable == FALSE)
        {
            workingListTemp->Value.Flags.IsButton = TRUE;
            workingListTemp->Value.Logical.Button.Min = Context->globalStack->LogicalMin;
            workingListTemp->Value.Logical.Button.Max = Context->globalStack->LogicalMax;
            // if it's usages array setup hasMore flag
            if (workingListTemp->Entry.Flink != &Context->workingListHead) // not last
            {
                workingListTemp->Value.Flags.ArrayHasMore = TRUE;
            }
        }
        else if (workingListTemp->Value.ReportSize == 1)
        {
            // variable with size 1 is button too
            workingListTemp->Value.Flags.IsButton = TRUE;
        }
        else
        {
            workingListTemp->Value.Logical.Value.Min = Context->globalStack->LogicalMin;
            workingListTemp->Value.Logical.Value.Max = Context->globalStack->LogicalMax;
            workingListTemp->Value.Logical.Value.NullValue = workingListTemp->Value.BitField.Flags.NullState;
            workingListTemp->Value.PhysicalMax = Context->globalStack->PhysicalMax;
            workingListTemp->Value.PhysicalMin = Context->globalStack->PhysicalMin;
        }

        workingListTemp->Value.Flags.IsAbsolute = !workingListTemp->Value.BitField.Flags.IsRelative;
        workingListTemp->Value.Flags.IsConstant = workingListTemp->Value.BitField.Flags.IsConstant;
    }

    listTemp = Context->workingListHead.Flink;
    RemoveEntryList(&Context->workingListHead);
    InitializeListHead(&Context->workingListHead);
    AppendTailList(&Context->outputListHead, listTemp);

    workingListTemp = AllocFunction(sizeof(HIDPARSER_VALUE_CAPS_LIST));
    if (workingListTemp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    InsertHeadList(&Context->workingListHead, &workingListTemp->Entry);

    return STATUS_SUCCESS;
}

NTSTATUS
HidParser_ProcessOpenCollectionTag(
    IN PHIDPARSER_CONTEXT Context,
    IN USHORT Data)
{
    PHIDPARSER_COLLECTION_STACK collectionTemp;
    PHIDPARSER_VALUE_CAPS_LIST workingListTemp;
    PLIST_ENTRY listTemp;
    PHIDPARSER_NODES_LIST nodesListTemp;

    // process new collection
    if (Context->nodeDepth == 0)
    {
        // allocate new collection
        collectionTemp = AllocFunction(sizeof(HIDPARSER_COLLECTION_STACK));
        if (collectionTemp == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // calculate collection index
        collectionTemp->Index = Context->collectionStack != NULL ? Context->collectionStack->Index + 1 : 0;

        // add new item to the stack
        collectionTemp->Next = Context->collectionStack;
        Context->collectionStack = collectionTemp;
    }

    Context->currentNode = AllocFunction(sizeof(HIDPARSER_NODES_LIST));
    if (Context->currentNode == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Get recent workingItem
    workingListTemp = CONTAINING_RECORD(Context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);

    if (workingListTemp->Value.UsagePage == 0)
    {
        workingListTemp->Value.UsagePage = Context->globalStack->UsagePage;
    }

    Context->currentNode->Value.LinkUsage = workingListTemp->Value.UsageMin;
    Context->currentNode->Value.LinkUsagePage = workingListTemp->Value.UsagePage;
    Context->currentNode->Value.CollectionType = Data;
    Context->currentNode->Value.Parent = 0;
    Context->currentNode->Value.NextSibling = 0;
    Context->currentNode->Depth = Context->nodeDepth + 1;
    if (IsListEmpty(&Context->nodesListHead))
    {
        Context->currentNode->Index = 0;
    }
    else
    {
        nodesListTemp = CONTAINING_RECORD(Context->nodesListHead.Flink, HIDPARSER_NODES_LIST, Entry);
        Context->currentNode->Index = nodesListTemp->Index + 1;
    }

    if (Context->nodeDepth > 0)
    {
        // find parent
        for (listTemp = Context->nodesListHead.Flink; listTemp != &Context->nodesListHead; listTemp = listTemp->Flink)
        {
            nodesListTemp = CONTAINING_RECORD(listTemp, HIDPARSER_NODES_LIST, Entry);

            if (nodesListTemp->Depth == Context->nodeDepth)
            {
                Context->currentNode->Value.Parent = nodesListTemp->Index;
                Context->currentNode->Value.NextSibling = nodesListTemp->Value.FirstChild;
                nodesListTemp->Value.NumberOfChildren++;
                nodesListTemp->Value.FirstChild = Context->currentNode->Index;
                break;
            }
        }
    }

    InsertHeadList(&Context->nodesListHead, &Context->currentNode->Entry);
    Context->nodeDepth++;

    return STATUS_SUCCESS;
}

NTSTATUS
HidParser_ProcessCloseCollectionTag(IN PHIDPARSER_CONTEXT Context)
{
    NTSTATUS status;
    PHIDPARSER_NODES_LIST nodesListTemp;
    PLIST_ENTRY listTemp;

    Context->nodeDepth--;
    if (Context->nodeDepth < 0)
    {
        DPRINT("[HIDPARSE] Collection was closed, but no open was associated with it");
        return STATUS_COULD_NOT_INTERPRET;
    }

    // the top level collection was closed, combine all information about it.
    if (Context->nodeDepth == 0)
    {
        // put everything toogether
        status = HidParser_CombinePreparsedData(&Context->outputListHead, &Context->nodesListHead, Context->reportIDsStack, Context->collectionStack);
        if (status != STATUS_SUCCESS)
        {
            return status;
        }
        ASSERT(Context->collectionStack->PreparsedData != NULL);

        // free output and nodes stacks
        while (Context->outputListHead.Flink != &Context->outputListHead)
        {
            listTemp = RemoveHeadList(&Context->outputListHead);
            FreeFunction(listTemp);
        }
        while (Context->nodesListHead.Flink != &Context->nodesListHead)
        {
            listTemp = RemoveHeadList(&Context->nodesListHead);
            FreeFunction(listTemp);
        }
    }
    else
    {
        // find parent
        Context->currentNode = NULL;
        for (listTemp = Context->nodesListHead.Flink; listTemp != &Context->nodesListHead; listTemp = listTemp->Flink)
        {
            nodesListTemp = CONTAINING_RECORD(listTemp, HIDPARSER_NODES_LIST, Entry);

            if (nodesListTemp->Depth == Context->nodeDepth)
            {
                Context->currentNode = nodesListTemp;
                break;
            }
        }
        if (Context->currentNode == NULL)
        {
            DPRINT("[HIDPARSE] Error, no parent found for node");
            ASSERT(FALSE);
        }
    }

    return STATUS_SUCCESS;
}

PHIDPARSER_CONTEXT HidParser_InitializeContext()
{
    PHIDPARSER_VALUE_CAPS_LIST workingListTemp;
    PHIDPARSER_CONTEXT context = AllocFunction(sizeof(HIDPARSER_CONTEXT));
    if (context == NULL)
    {
        return NULL;
    }

    InitializeListHead(&context->workingListHead);
    InitializeListHead(&context->outputListHead);
    InitializeListHead(&context->nodesListHead);

    context->globalStack = AllocFunction(sizeof(HIDPARSER_GLOBAL_STACK));
    if (context->globalStack == NULL)
    {
        return NULL;
    }

    workingListTemp = AllocFunction(sizeof(HIDPARSER_VALUE_CAPS_LIST));
    if (workingListTemp == NULL)
    {
        FreeFunction(context->globalStack);
        return NULL;
    };
    InsertHeadList(&context->workingListHead, &workingListTemp->Entry);

    context->collectionStack = NULL;
    context->reportIDsStack = NULL;
    context->currentNode = NULL;
    context->nodeDepth = 0;
    context->workingCount = 0;
    context->IsDelimiterOpen = FALSE;

    return context;
}

void HidParser_FreeContext(PHIDPARSER_CONTEXT Context)
{
    PLIST_ENTRY listTemp;
    PHIDPARSER_GLOBAL_STACK globalStackTemp;
    PHIDPARSER_COLLECTION_STACK collectionTemp;
    PHIDPARSER_REPORT_IDS_STACK reportIDsTemp;


    // free working stacks and lists
    while (Context->workingListHead.Flink != &Context->workingListHead)
    {
        listTemp = RemoveHeadList(&Context->workingListHead);
        FreeFunction(CONTAINING_RECORD(listTemp, HIDPARSER_VALUE_CAPS_LIST, Entry));
    }
    while (Context->outputListHead.Flink != &Context->outputListHead)
    {
        listTemp = RemoveHeadList(&Context->outputListHead);
        FreeFunction(CONTAINING_RECORD(listTemp, HIDPARSER_VALUE_CAPS_LIST, Entry));
    }
    while (Context->nodesListHead.Flink != &Context->nodesListHead)
    {
        listTemp = RemoveHeadList(&Context->nodesListHead);
        FreeFunction(CONTAINING_RECORD(listTemp, HIDPARSER_NODES_LIST, Entry));
    }
    while (Context->globalStack != NULL)
    {
        globalStackTemp = Context->globalStack->Next;
        FreeFunction(Context->globalStack);
        Context->globalStack = globalStackTemp;
    }

    while (Context->collectionStack != NULL)
    {
        collectionTemp = Context->collectionStack->Next;
        FreeFunction(Context->collectionStack);
        Context->collectionStack = collectionTemp;
    }
    while (Context->reportIDsStack != NULL)
    {
        reportIDsTemp = Context->reportIDsStack->Next;
        FreeFunction(Context->reportIDsStack);
        Context->reportIDsStack = reportIDsTemp;
    }
    FreeFunction(Context);
}


void HidParser_ResetWorkingItems(PHIDPARSER_CONTEXT Context)
{
    PLIST_ENTRY listTemp;
    PHIDPARSER_VALUE_CAPS_LIST workingListTemp;

// free all but one (to avoid allocation)
    while (Context->workingListHead.Flink != Context->workingListHead.Blink)
    {
        listTemp = RemoveHeadList(&Context->workingListHead);
        FreeFunction(listTemp);
    }
    // reset working item
    listTemp = RemoveHeadList(&Context->workingListHead);
    workingListTemp = CONTAINING_RECORD(listTemp, HIDPARSER_VALUE_CAPS_LIST, Entry);
    ZeroFunction(workingListTemp, sizeof(HIDPARSER_VALUE_CAPS_LIST));
    InsertHeadList(&Context->workingListHead, &workingListTemp->Entry);

    Context->workingCount = 0;
}

NTSTATUS HidParser_GlobalPush(PHIDPARSER_CONTEXT Context)
{
    PHIDPARSER_GLOBAL_STACK globalStackTemp;

    // allocate global item state
    globalStackTemp = (PHIDPARSER_GLOBAL_STACK)AllocFunction(sizeof(HIDPARSER_GLOBAL_STACK));
    if (globalStackTemp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy global item state
    CopyFunction(globalStackTemp, Context->globalStack, sizeof(HIDPARSER_GLOBAL_STACK));

    // store pushed item in link member
    globalStackTemp->Next = Context->globalStack;
    Context->globalStack = globalStackTemp;

    return STATUS_SUCCESS;
}

BOOLEAN HidParser_GlobalPop(PHIDPARSER_CONTEXT Context)
{
    PHIDPARSER_GLOBAL_STACK globalStackTemp;

    if (Context->globalStack->Next == NULL)
    {
        return FALSE;
    }

    // get link
    globalStackTemp = Context->globalStack->Next;

    // free item
    FreeFunction(Context->globalStack);

    // replace current item with linked one
    Context->globalStack = globalStackTemp;

    return TRUE;
}

BOOLEAN HidParser_ProcessDelimiter(PHIDPARSER_CONTEXT Context)
{
    if (Context->IsDelimiterOpen == TRUE)
    {
        if (Context->IsDelimiterProcessed == TRUE)
        {
            return FALSE;
        }
        else
        {
            Context->IsDelimiterProcessed = TRUE;
        }
    }

    return TRUE;
}

NTSTATUS HidParser_CreateWorkingItem(PHIDPARSER_CONTEXT Context)
{
    PHIDPARSER_VALUE_CAPS_LIST workingListTemp;

    // first element already allocated
    if (Context->workingCount > 0)
    {
        workingListTemp = AllocFunction(sizeof(HIDPARSER_VALUE_CAPS_LIST));
        if (workingListTemp == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        CopyFunction(&workingListTemp->Value,
                     &CONTAINING_RECORD(Context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry)->Value,
                     sizeof(HIDPARSER_VALUE_CAPS));
        InsertHeadList(&Context->workingListHead, &workingListTemp->Entry);
    }
    Context->workingCount++;

    return STATUS_SUCCESS;
}

NTSTATUS HidParser_ConstructDeviceDescription(
    PHIDPARSER_CONTEXT Context,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription)
{
    PHIDPARSER_COLLECTION_STACK collectionTemp;
    PHIDPARSER_REPORT_IDS_STACK reportIDsTemp;
    INT32 reportIndex;

    // Cunstruct the device description
    for (collectionTemp = Context->collectionStack; collectionTemp != NULL; collectionTemp = collectionTemp->Next)
    {
        DeviceDescription->CollectionDescLength++;
    }
    if (DeviceDescription->CollectionDescLength == 0)
    {
        // no top level collections found
        return STATUS_NO_DATA_DETECTED;
    }

    // allocate collection
    DeviceDescription->CollectionDesc =
        (PHIDP_COLLECTION_DESC)AllocFunction(sizeof(HIDP_COLLECTION_DESC) * DeviceDescription->CollectionDescLength);
    if (DeviceDescription->CollectionDesc == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy collections and free
    for (collectionTemp = Context->collectionStack; collectionTemp != NULL; collectionTemp = collectionTemp->Next)
    {
        ASSERT(collectionTemp->Index < DeviceDescription->CollectionDescLength);
        DeviceDescription->CollectionDesc[collectionTemp->Index].PreparsedData = collectionTemp->PreparsedData;
        DeviceDescription->CollectionDesc[collectionTemp->Index].PreparsedDataLength = collectionTemp->PreparsedSize;
        DeviceDescription->CollectionDesc[collectionTemp->Index].UsagePage = collectionTemp->PreparsedData->UsagePage;
        DeviceDescription->CollectionDesc[collectionTemp->Index].Usage = collectionTemp->PreparsedData->Usage;
        DeviceDescription->CollectionDesc[collectionTemp->Index].CollectionNumber = collectionTemp->Index;
        DeviceDescription->CollectionDesc[collectionTemp->Index].InputLength =
            collectionTemp->PreparsedData->InputReportByteLength;
        DeviceDescription->CollectionDesc[collectionTemp->Index].OutputLength =
            collectionTemp->PreparsedData->OutputReportByteLength;
        DeviceDescription->CollectionDesc[collectionTemp->Index].FeatureLength =
            collectionTemp->PreparsedData->FeatureReportByteLength;
    }

    for (reportIDsTemp = Context->reportIDsStack; reportIDsTemp != NULL; reportIDsTemp = reportIDsTemp->Next)
    {
        DeviceDescription->ReportIDsLength++;
    }

    // allocate report description
    DeviceDescription->ReportIDs =
        (PHIDP_REPORT_IDS)AllocFunction(sizeof(HIDP_REPORT_IDS) * DeviceDescription->ReportIDsLength);
    if (DeviceDescription->ReportIDs == NULL)
    {
        // no memory
        FreeFunction(DeviceDescription->CollectionDesc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy reports and free
    for (
        reportIDsTemp = Context->reportIDsStack, reportIndex = 0;
        reportIDsTemp != NULL;
        reportIDsTemp = reportIDsTemp->Next, reportIndex++)
    {
        reportIDsTemp->Value.FeatureLength /= 8;
        reportIDsTemp->Value.InputLength /= 8;
        reportIDsTemp->Value.OutputLength /= 8;

        //
        // if desctiptor has only one report with reportID == 0
        // report will not contain reportID at zero's byte
        //
        if (Context->reportIDsStack->Next != NULL || reportIDsTemp->Value.ReportID > 0)
        {
            if (reportIDsTemp->Value.FeatureLength > 0)
            {
                reportIDsTemp->Value.FeatureLength++;
            }
            if (reportIDsTemp->Value.InputLength > 0)
            {
                reportIDsTemp->Value.InputLength++;
            }
            if (reportIDsTemp->Value.OutputLength > 0)
            {
                reportIDsTemp->Value.OutputLength++;
            }
        }
        CopyFunction(&DeviceDescription->ReportIDs[reportIndex], &reportIDsTemp->Value, sizeof(HIDP_REPORT_IDS));
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HidParser_GetCollectionDescription(
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription)
{
    NTSTATUS status = STATUS_SUCCESS;

    PUCHAR reportOffset = ReportDesc;
    PUCHAR reportEnd = ReportDesc + DescLength;

    ULONG currentItemSize;
    PREPORT_ITEM currentItem;
    ITEM_DATA currentData;

    PHIDPARSER_VALUE_CAPS_LIST workingListTemp;

    PHIDPARSER_CONTEXT context;

    if (DeviceDescription == NULL || DescLength == 0 || ReportDesc >= reportEnd)
    {
        return STATUS_NO_DATA_DETECTED;
    }

    context = HidParser_InitializeContext();
    if (context == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ZeroFunction(DeviceDescription, sizeof(HIDP_DEVICE_DESC));

    do
    {
        // get current item
        currentItem = (PREPORT_ITEM)reportOffset;

        // get item size
        currentItemSize = 0;
        currentData.Raw.UData = 0;
        currentData.Raw.SData = 0;

        // get associated data
        switch (currentItem->Size)
        {
            case ITEM_SIZE_0:
                // Data is zero already
                break;
            case ITEM_SIZE_1:
                currentItemSize = 1;
                currentData.Raw.UData = currentItem->Data.Short8.Value;
                currentData.Raw.SData = (CHAR)currentItem->Data.Short8.Value;
                break;
            case ITEM_SIZE_2:
                currentItemSize = 2;
                currentData.Raw.UData = currentItem->Data.Short16.Value;
                currentData.Raw.SData = (SHORT)currentItem->Data.Short16.Value;
                break;
            case ITEM_SIZE_4:
                currentItemSize = 4;
                currentData.Raw.UData = currentItem->Data.Short32.Value;
                currentData.Raw.SData = (LONG)currentItem->Data.Short32.Value;
                break;
        }

        // lazy validate delimiter
        if (context->IsDelimiterOpen)
        {
            if ((LOCAL_ITEM_TYPE)currentItem->Tag != ITEM_TAG_LOCAL_USAGE &&
                (LOCAL_ITEM_TYPE)currentItem->Tag != ITEM_TAG_LOCAL_DELIMITER)
            {
                // TODO error
                ASSERT(FALSE);
            }
        }

        // handle items
        switch ((ITEM_TYPE)currentItem->Type)
        {
            case ITEM_TYPE_MAIN:
            {
                switch ((MAIN_ITEM_TYPE)currentItem->Tag)
                {
                    case MAIN_ITEM_TAG_COLLECTION:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_MAIN_COLLECTION 0x%x\n", currentData.UData16.Value);
                        status = HidParser_ProcessOpenCollectionTag(context, currentData.UData16.Value);
                        if (status != STATUS_SUCCESS)
                        {
                            goto exit;
                        }
                    }
                    break;
                    case MAIN_ITEM_TAG_END_COLLECTION:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_MAIN_END_COLLECTION 0x%x\n", currentData.UData16.Value);
                        status = HidParser_ProcessCloseCollectionTag(context);
                        if (status != STATUS_SUCCESS)
                        {
                            goto exit;
                        }
                    }
                    break;
                    case MAIN_ITEM_TAG_INPUT:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_MAIN_INPUT 0x%x\n", currentData.UData16.Value);
                        context->currentReport = HidParser_GetCurrentReportIDS(context);
                        if (context->currentReport == NULL)
                        {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            goto exit;
                        }
                        status = HidParser_ProcessReportTag(context, currentData.UData32.Value, HidP_Input, &context->currentReport->InputLength);
                        if (status != STATUS_SUCCESS)
                        {
                            goto exit;
                        }
                    }
                    break;
                    case MAIN_ITEM_TAG_OUTPUT:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_MAIN_OUTPUT 0x%x\n", currentData.UData16.Value);
                        context->currentReport = HidParser_GetCurrentReportIDS(context);
                        if (context->currentReport == NULL)
                        {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            goto exit;
                        }
                        HidParser_ProcessReportTag(context, currentData.UData32.Value, HidP_Output, &context->currentReport->OutputLength);
                        if (status != STATUS_SUCCESS)
                        {
                            goto exit;
                        }
                    }
                    break;
                    case MAIN_ITEM_TAG_FEATURE:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_MAIN_FEATURE 0x%x\n", currentData.UData16.Value);
                        context->currentReport = HidParser_GetCurrentReportIDS(context);
                        if (context->currentReport == NULL)
                        {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            goto exit;
                        }
                        status = HidParser_ProcessReportTag(context, currentData.UData32.Value, HidP_Feature, &context->currentReport->FeatureLength);
                        if (status != STATUS_SUCCESS)
                        {
                            goto exit;
                        }
                    }
                    break;
                    default:
                    {
                        DPRINT(
                            "[HIDPARSE] Unknown ReportType Tag %x Type %x Size %x CurrentItemSize %x\n",
                            currentItem->Tag, currentItem->Type, currentItem->Size, currentItemSize);
                        status = STATUS_ILLEGAL_INSTRUCTION;
                        goto exit;
                    }
                    break;
                }

                // main item should reset local items
                HidParser_ResetWorkingItems(context);
            }
            break;
            case ITEM_TYPE_GLOBAL:
            {
                switch (currentItem->Tag)
                {
                    case GLOBAL_ITEM_TAG_USAGE_PAGE:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_USAGE_PAGE 0x%x\n", currentData.UData16.Value);
                        context->globalStack->UsagePage = currentData.UData16.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_LOGICAL_MINIMUM:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_LOGICAL_MINIMUM 0x%x\n", currentData.SData32.Value);
                        context->globalStack->LogicalMin = currentData.SData32.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_LOCAL_MAXIMUM 0x%x\n", currentData.SData32.Value);
                        context->globalStack->LogicalMax = currentData.SData32.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_PHYSICAL_MINIMUM 0x%x\n", currentData.SData32.Value);
                        context->globalStack->PhysicalMin = currentData.SData32.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_PHYSICAL_MAXIMUM 0x%x\n", currentData.SData32.Value);
                        context->globalStack->PhysicalMax = currentData.SData32.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_UNIT_EXPONENT:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_UNIT_EXPONENT 0x%x\n", currentData.SData32.Value);
                        context->globalStack->UnitsExp = currentData.SData32.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_UNIT:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_UNIT 0x%x\n", currentData.SData32.Value);
                        context->globalStack->Units = currentData.UData32.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_REPORT_SIZE:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_SIZE 0x%x\n", currentData.UData16.Value);
                        context->globalStack->ReportSize = currentData.UData16.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_REPORT_ID:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_ID 0x%x\n", currentData.UData8.Value);
                        context->globalStack->ReportID = currentData.UData8.Value;
                        if (currentData.UData8.Value == 0)
                        {
                            status = HIDP_STATUS_INVALID_REPORT_TYPE;
                            goto exit;
                        }
                    }
                    break;
                    case GLOBAL_ITEM_TAG_REPORT_COUNT:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_REPORT_COUNT 0x%x\n", currentData.UData16.Value);
                        context->globalStack->ReportCount = currentData.UData16.Value;
                    }
                    break;
                    case GLOBAL_ITEM_TAG_PUSH:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_PUSH\n");
                        status = HidParser_GlobalPush(context);
                        if (status != STATUS_SUCCESS)
                        {
                            goto exit;
                        }
                    }
                    break;
                    case GLOBAL_ITEM_TAG_POP:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_POP\n");

                        if (HidParser_GlobalPop(context) == FALSE)
                        {
                            // pop without push
                            DPRINT("[HIDPARSE] ITEM_TAG_GLOBAL_POP Stack underflow!!!\n");
                            status = STATUS_COULD_NOT_INTERPRET;
                            goto exit;
                        }

                    }
                    break;
                    default:
                    {
                        if (context->globalStack->UnknownGlobalCount < 4)
                        {
                            context->globalStack->UnknownGlobalCount++;
                        }
                        context->globalStack->UnknownGlobals[context->globalStack->UnknownGlobalCount - 1].Token = currentItem->RawTag;
                        context->globalStack->UnknownGlobals[context->globalStack->UnknownGlobalCount - 1].BitField = currentData.UData32.Value;
                    }
                    break;
                }
            }
            break;
            case ITEM_TYPE_LOCAL:
            {
                switch ((LOCAL_ITEM_TYPE)currentItem->Tag)
                {
                    case ITEM_TAG_LOCAL_USAGE:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_USAGE Data 0x%x\n", currentData.UData16.Value);
                        if (HidParser_ProcessDelimiter(context) == FALSE)
                        {
                            DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_USAGE skipping delimiter usages\n");
                            break;
                        }
                        status = HidParser_CreateWorkingItem(context);
                        if (status != STATUS_SUCCESS)
                        {
                            goto exit;
                        }
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        workingListTemp->Value.UsageMin = currentData.UData16.Value;
                        workingListTemp->Value.UsageMax = currentData.UData16.Value;
                        if (currentItemSize == 4)
                        {
                            workingListTemp->Value.UsagePage = currentData.UData16.Value2;
                        }
                    }
                    break;
                    case ITEM_TAG_LOCAL_USAGE_MAXIMUM:
                    {
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_USAGE_MAXIMUM Data 0x%x\n", currentData.UData16.Value);
                        status = HidParser_CreateWorkingItem(context);
                        if (status != STATUS_SUCCESS)
                        {
                            goto exit;
                        }
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        workingListTemp->Value.Flags.IsRange = TRUE;
                        workingListTemp->Value.UsageMax = currentData.UData16.Value;
                        if (currentItemSize == 4)
                        {
                            workingListTemp->Value.UsagePage = currentData.UData16.Value2;
                        }
                    }
                    break;
                    case ITEM_TAG_LOCAL_USAGE_MINIMUM:
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_USAGE_MINIMUM 0x%x\n", currentData.UData16.Value);
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        // usage min alone cannot be used to define an usage, usage max is must be called anyway
                        workingListTemp->Value.UsageMin = currentData.UData16.Value;
                        if (currentItemSize == 4)
                        {
                            workingListTemp->Value.UsagePage = currentData.UData16.Value2;
                        }
                        break;
                    case ITEM_TAG_LOCAL_DESIGNATOR_INDEX:
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_DESIGNATOR_INDEX 0x%x\n", currentData.UData16.Value);
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        workingListTemp->Value.DesignatorMin = currentData.UData16.Value;
                        workingListTemp->Value.DesignatorMax = currentData.UData16.Value;
                        break;

                    case ITEM_TAG_LOCAL_DESIGNATOR_MINIMUM:
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_DESIGNATOR_MINIMUM 0x%x\n", currentData.UData16.Value);
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        workingListTemp->Value.DesignatorMin = currentData.UData16.Value;
                        break;

                    case ITEM_TAG_LOCAL_DESIGNATOR_MAXIMUM:
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_DESIGNATOR_MAXIMUM 0x%x\n", currentData.UData16.Value);
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        workingListTemp->Value.DesignatorMax = currentData.UData16.Value;
                        workingListTemp->Value.Flags.IsDesignatorRange = TRUE;
                        break;

                    case ITEM_TAG_LOCAL_STRING_INDEX:
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_STRING_INDEX 0x%x\n", currentData.UData16.Value);
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        workingListTemp->Value.StringMin = currentData.UData16.Value;
                        workingListTemp->Value.StringMax = currentData.UData16.Value;
                        break;

                    case ITEM_TAG_LOCAL_STRING_MINIMUM:
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_STRING_MINIMUM 0x%x\n", currentData.UData16.Value);
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        workingListTemp->Value.StringMin = currentData.UData16.Value;
                        break;

                    case ITEM_TAG_LOCAL_STRING_MAXIMUM:
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_STRING_MAXIMUM 0x%x\n", currentData.UData16.Value);
                        workingListTemp = CONTAINING_RECORD(context->workingListHead.Flink, HIDPARSER_VALUE_CAPS_LIST, Entry);
                        workingListTemp->Value.StringMax = currentData.UData16.Value;
                        workingListTemp->Value.Flags.IsStringRange = TRUE;
                        break;

                    case ITEM_TAG_LOCAL_DELIMITER:
                        DPRINT("[HIDPARSE] ITEM_TAG_LOCAL_DELIMITER 0x%x\n", currentData.UData16.Value);
                        context->IsDelimiterOpen = currentData.UData16.Value == 1 ? TRUE : FALSE;
                        context->IsDelimiterProcessed = FALSE;
                        break;

                    default:
                        DPRINT("Unknown Local Item Tag 0x%x\n", currentItem->Tag);
                        status = STATUS_ILLEGAL_INSTRUCTION;
                        goto exit;
                        break;
                }

                break;
            }
            case ITEM_TYPE_RESERVED:
            {
                switch (currentItem->Tag)
                {
                    case ITEM_TAG_LONG:
                        // due hid specefication size of long item must be 2
                        if (currentItemSize != 2)
                        {
                            DPRINT(
                                "ITEM_TYPE_LONG Sanity check failed, size is 0x%x but 0x2 expected\n",
                                currentItem->Data.Long.LongItemTag);
                        }
                        currentItemSize += currentItem->Data.Long.LongDataSize;
                        DPRINT("Unsupported ITEM_TYPE_LONG Tag %x\n", currentItem->Data.Long.LongItemTag);
                        // status = STATUS_COULD_NOT_INTERPRET;
                        // goto exit;
                        break;
                    default:
                        DPRINT("Unsupported ITEM_TYPE_RESERVED Tag %x\n", currentItem->Data.Long.LongItemTag);
                        status = STATUS_ILLEGAL_INSTRUCTION;
                        goto exit;
                        break;
                }
                break;
            }
        }

        // move to next item
        reportOffset += currentItemSize + REPORT_ITEM_PREFIX_SIZE;

    }
    while (reportOffset < reportEnd);

    // check if all collection's was closed
    if (context->nodeDepth > 0)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }


    status = HidParser_ConstructDeviceDescription(context, PoolType, DeviceDescription);

exit:
    HidParser_FreeContext(context);

    return status;
}
