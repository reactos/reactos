/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/hidparser/hidparser.c
 * PURPOSE:     HID Parser
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "parser.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
TranslateHidParserStatus(
    IN HIDPARSER_STATUS Status)
{
    switch(Status)
    {
        case HIDPARSER_STATUS_INSUFFICIENT_RESOURCES:
             return HIDP_STATUS_INTERNAL_ERROR;
        case HIDPARSER_STATUS_NOT_IMPLEMENTED:
            return HIDP_STATUS_NOT_IMPLEMENTED;
        case HIDPARSER_STATUS_REPORT_NOT_FOUND:
            return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
        case HIDPARSER_STATUS_INVALID_REPORT_LENGTH:
            return HIDP_STATUS_INVALID_REPORT_LENGTH;
        case HIDPARSER_STATUS_INVALID_REPORT_TYPE:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
        case HIDPARSER_STATUS_BUFFER_TOO_SMALL:
            return HIDP_STATUS_BUFFER_TOO_SMALL;
        case HIDPARSER_STATUS_USAGE_NOT_FOUND:
            return HIDP_STATUS_USAGE_NOT_FOUND;
        case HIDPARSER_STATUS_I8042_TRANS_UNKNOWN:
            return HIDP_STATUS_I8042_TRANS_UNKNOWN;
        case HIDPARSER_STATUS_COLLECTION_NOT_FOUND:
            return HIDP_STATUS_NOT_IMPLEMENTED; //FIXME
        case HIDPARSER_STATUS_BAD_LOG_PHY_VALUES:
            return HIDP_STATUS_BAD_LOG_PHY_VALUES;
    }
    DPRINT1("TranslateHidParserStatus Status %ld not implemented\n", Status);
    return HIDP_STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidParser_GetCollectionDescription(
    IN PHID_PARSER Parser,
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription)
{
    HIDPARSER_STATUS ParserStatus;
    ULONG CollectionCount;
    ULONG Index;
    PVOID ParserContext;

    //
    // first parse the report descriptor
    //
    ParserStatus = HidParser_ParseReportDescriptor(Parser, ReportDesc, DescLength, &ParserContext);
    if (ParserStatus != HIDPARSER_STATUS_SUCCESS)
    {
        //
        // failed to parse report descriptor
        //
        Parser->Debug("[HIDPARSER] Failed to parse report descriptor with %x\n", ParserStatus);
        return ParserStatus;
    }

    //
    // get collection count
    //
    CollectionCount = HidParser_NumberOfTopCollections(ParserContext);
    if (CollectionCount == 0)
    {
        //
        // no top level collections found
        //
        ASSERT(FALSE);
        return STATUS_NO_DATA_DETECTED;
    }

    //
    // zero description
    //
    Parser->Zero(DeviceDescription, sizeof(HIDP_DEVICE_DESC));

    //
    // allocate collection
    //
    DeviceDescription->CollectionDesc = (PHIDP_COLLECTION_DESC)Parser->Alloc(sizeof(HIDP_COLLECTION_DESC) * CollectionCount);
    if (!DeviceDescription->CollectionDesc)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // allocate report description
    //
    DeviceDescription->ReportIDs = (PHIDP_REPORT_IDS)Parser->Alloc(sizeof(HIDP_REPORT_IDS) * CollectionCount);
    if (!DeviceDescription->ReportIDs)
    {
        //
        // no memory
        //
        Parser->Free(DeviceDescription->CollectionDesc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for(Index = 0; Index < CollectionCount; Index++)
    {
        //
        // set preparsed data length
        //
        DeviceDescription->CollectionDesc[Index].PreparsedDataLength = HidParser_GetContextSize(Parser, ParserContext, Index);
        ParserStatus = HidParser_BuildContext(Parser, ParserContext, Index, DeviceDescription->CollectionDesc[Index].PreparsedDataLength, (PVOID*)&DeviceDescription->CollectionDesc[Index].PreparsedData);
        if (ParserStatus != HIDPARSER_STATUS_SUCCESS)
        {
            //
            // no memory
            //
            Parser->Free(DeviceDescription->CollectionDesc);
            Parser->Free(DeviceDescription->ReportIDs);
            return ParserStatus;
        }

        //
        // init report description
        //
        DeviceDescription->ReportIDs[Index].CollectionNumber = Index + 1;
        DeviceDescription->ReportIDs[Index].ReportID = Index; //FIXME
        DeviceDescription->ReportIDs[Index].InputLength = HidParser_GetReportLength((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_INPUT);
        DeviceDescription->ReportIDs[Index].OutputLength = HidParser_GetReportLength((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_OUTPUT);
        DeviceDescription->ReportIDs[Index].FeatureLength = HidParser_GetReportLength((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_FEATURE);


        DeviceDescription->ReportIDs[Index].InputLength += (HidParser_UsesReportId((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_INPUT) ? 1 : 0);
        DeviceDescription->ReportIDs[Index].OutputLength += (HidParser_UsesReportId((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_OUTPUT) ? 1 : 0);
        DeviceDescription->ReportIDs[Index].FeatureLength += (HidParser_UsesReportId((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_FEATURE) ? 1 : 0);


        //
        // init collection description
        //
        DeviceDescription->CollectionDesc[Index].CollectionNumber = Index + 1;

        //
        // get collection usage page
        //
        ParserStatus = HidParser_GetCollectionUsagePage((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, &DeviceDescription->CollectionDesc[Index].Usage, &DeviceDescription->CollectionDesc[Index].UsagePage);
        if (ParserStatus != HIDPARSER_STATUS_SUCCESS)
        {
            // collection not found
            Parser->Free(DeviceDescription->CollectionDesc);
            Parser->Free(DeviceDescription->ReportIDs);
            return ParserStatus;
        }

        //
        // windows seems to prepend the report id, regardless if it is required
        //
        DeviceDescription->CollectionDesc[Index].CollectionNumber = Index + 1;
        DeviceDescription->CollectionDesc[Index].InputLength = DeviceDescription->ReportIDs[Index].InputLength;
        DeviceDescription->CollectionDesc[Index].OutputLength = DeviceDescription->ReportIDs[Index].OutputLength;
        DeviceDescription->CollectionDesc[Index].FeatureLength = DeviceDescription->ReportIDs[Index].FeatureLength;

        DeviceDescription->CollectionDesc[Index].InputLength += (HidParser_UsesReportId((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_INPUT) == FALSE ? 1 : 0);
        DeviceDescription->CollectionDesc[Index].OutputLength += (HidParser_UsesReportId((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_OUTPUT) == FALSE ? 1 : 0);
        DeviceDescription->CollectionDesc[Index].FeatureLength += (HidParser_UsesReportId((PVOID)DeviceDescription->CollectionDesc[Index].PreparsedData, HID_REPORT_TYPE_FEATURE) == FALSE ? 1 : 0);


    }

    //
    // store collection & report count
    //
    DeviceDescription->CollectionDescLength = CollectionCount;
    DeviceDescription->ReportIDsLength = CollectionCount;

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
NTAPI
HidParser_FreeCollectionDescription(
    IN PHID_PARSER Parser,
    IN PHIDP_DEVICE_DESC   DeviceDescription)
{
    ULONG Index;

    //
    // first free all context
    //
    for(Index = 0; Index < DeviceDescription->CollectionDescLength; Index++)
    {
        //
        // free collection context
        //
        Parser->Free(DeviceDescription->CollectionDesc[Index].PreparsedData);
    }

    //
    // now free collection description
    //
    Parser->Free(DeviceDescription->CollectionDesc);

    //
    // free report description
    //
    Parser->Free(DeviceDescription->ReportIDs);
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    OUT PHIDP_CAPS  Capabilities)
{
    //
    // zero capabilities
    //
    Parser->Zero(Capabilities, sizeof(HIDP_CAPS));

    //
    // init capabilities
    //
    HidParser_GetCollectionUsagePage(CollectionContext, &Capabilities->Usage, &Capabilities->UsagePage);
    Capabilities->InputReportByteLength = HidParser_GetReportLength(CollectionContext, HID_REPORT_TYPE_INPUT);
    Capabilities->OutputReportByteLength = HidParser_GetReportLength(CollectionContext, HID_REPORT_TYPE_OUTPUT);
    Capabilities->FeatureReportByteLength = HidParser_GetReportLength(CollectionContext, HID_REPORT_TYPE_FEATURE);

    //
    // always pre-prend report id
    //
    Capabilities->InputReportByteLength = (Capabilities->InputReportByteLength > 0 ? Capabilities->InputReportByteLength + 1 : 0);
    Capabilities->OutputReportByteLength = (Capabilities->OutputReportByteLength > 0 ? Capabilities->OutputReportByteLength + 1 : 0);
    Capabilities->FeatureReportByteLength = (Capabilities->FeatureReportByteLength > 0 ? Capabilities->FeatureReportByteLength + 1 : 0);

    //
    // get number of link collection nodes
    //
    Capabilities->NumberLinkCollectionNodes = HidParser_GetTotalCollectionCount(CollectionContext);

    //
    // get data indices
    //
    Capabilities->NumberInputDataIndices = HidParser_GetReportItemTypeCountFromReportType(CollectionContext, HID_REPORT_TYPE_INPUT, TRUE);
    Capabilities->NumberOutputDataIndices = HidParser_GetReportItemTypeCountFromReportType(CollectionContext, HID_REPORT_TYPE_OUTPUT, TRUE);
    Capabilities->NumberFeatureDataIndices = HidParser_GetReportItemTypeCountFromReportType(CollectionContext, HID_REPORT_TYPE_FEATURE, TRUE);

    //
    // get value caps
    //
    Capabilities->NumberInputValueCaps = HidParser_GetReportItemTypeCountFromReportType(CollectionContext, HID_REPORT_TYPE_INPUT, FALSE);
    Capabilities->NumberOutputValueCaps = HidParser_GetReportItemTypeCountFromReportType(CollectionContext, HID_REPORT_TYPE_OUTPUT, FALSE);
    Capabilities->NumberFeatureValueCaps = HidParser_GetReportItemTypeCountFromReportType(CollectionContext, HID_REPORT_TYPE_FEATURE, FALSE);


    //
    // get button caps
    //
    Capabilities->NumberInputButtonCaps = HidParser_GetReportItemCountFromReportType(CollectionContext, HID_REPORT_TYPE_INPUT);
    Capabilities->NumberOutputButtonCaps = HidParser_GetReportItemCountFromReportType(CollectionContext, HID_REPORT_TYPE_OUTPUT);
    Capabilities->NumberFeatureButtonCaps = HidParser_GetReportItemCountFromReportType(CollectionContext, HID_REPORT_TYPE_FEATURE);

    //
    // done
    //
    return HIDP_STATUS_SUCCESS;
}

HIDAPI
ULONG
NTAPI
HidParser_MaxUsageListLength(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage  OPTIONAL)
{
    //
    // FIXME test what should be returned when usage page is not defined
    //
    if (UsagePage == HID_USAGE_PAGE_UNDEFINED)
    {
        //
        // implement me
        //
        UNIMPLEMENTED;

        //
        // invalid report
        //
        return 0;
    }

    if (ReportType == HidP_Input)
    {
        //
        // input report
        //
        return HidParser_GetMaxUsageListLengthWithReportAndPage(CollectionContext, HID_REPORT_TYPE_INPUT, UsagePage);
    }
    else if (ReportType == HidP_Output)
    {
        //
        // input report
        //
        return HidParser_GetMaxUsageListLengthWithReportAndPage(CollectionContext, HID_REPORT_TYPE_OUTPUT, UsagePage);
    }
    else if (ReportType == HidP_Feature)
    {
        //
        // input report
        //
        return HidParser_GetMaxUsageListLengthWithReportAndPage(CollectionContext, HID_REPORT_TYPE_FEATURE, UsagePage);
    }
    else
    {
        //
        // invalid report type
        //
        return 0;
    }
}

#undef HidParser_GetButtonCaps

HIDAPI
NTSTATUS
NTAPI
HidParser_GetButtonCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE ReportType,
    IN PHIDP_BUTTON_CAPS ButtonCaps,
    IN PUSHORT ButtonCapsLength)
{
    return HidParser_GetSpecificButtonCaps(Parser, CollectionContext, ReportType, HID_USAGE_PAGE_UNDEFINED, HIDP_LINK_COLLECTION_UNSPECIFIED, HID_USAGE_PAGE_UNDEFINED, ButtonCaps, (PULONG)ButtonCapsLength);
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetSpecificValueCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PHIDP_VALUE_CAPS  ValueCaps,
    IN OUT PUSHORT  ValueCapsLength)
{
    HIDPARSER_STATUS ParserStatus;

    //
    // FIXME: implement searching in specific collection
    //
    ASSERT(LinkCollection == HIDP_LINK_COLLECTION_UNSPECIFIED);

    if (ReportType == HidP_Input)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetSpecificValueCapsWithReport(Parser, CollectionContext, HID_REPORT_TYPE_INPUT, UsagePage, Usage, ValueCaps, ValueCapsLength);
    }
    else if (ReportType == HidP_Output)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetSpecificValueCapsWithReport(Parser, CollectionContext, HID_REPORT_TYPE_OUTPUT, UsagePage, Usage, ValueCaps, ValueCapsLength);
    }
    else if (ReportType == HidP_Feature)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetSpecificValueCapsWithReport(Parser, CollectionContext, HID_REPORT_TYPE_FEATURE, UsagePage, Usage, ValueCaps, ValueCapsLength);
    }
    else
    {
        //
        // invalid report type
        //
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }


    if (ParserStatus == HIDPARSER_STATUS_SUCCESS)
    {
        //
        // success
        //
        return HIDP_STATUS_SUCCESS;
    }

    //
    // translate error
    //
    return TranslateHidParserStatus(ParserStatus);
}

HIDAPI
NTSTATUS
NTAPI
HidParser_UsageListDifference(
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
HidParser_GetUsages(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection  OPTIONAL,
    OUT USAGE  *UsageList,
    IN OUT PULONG UsageLength,
    IN PCHAR  Report,
    IN ULONG  ReportLength)
{
    HIDPARSER_STATUS ParserStatus;

    //
    // FIXME: implement searching in specific collection
    //
    ASSERT(LinkCollection == HIDP_LINK_COLLECTION_UNSPECIFIED);

    if (ReportType == HidP_Input)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetUsagesWithReport(Parser, CollectionContext, HID_REPORT_TYPE_INPUT, UsagePage, UsageList, UsageLength, Report, ReportLength);
    }
    else if (ReportType == HidP_Output)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetUsagesWithReport(Parser, CollectionContext, HID_REPORT_TYPE_OUTPUT, UsagePage, UsageList, UsageLength, Report, ReportLength);
    }
    else if (ReportType == HidP_Feature)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetUsagesWithReport(Parser, CollectionContext, HID_REPORT_TYPE_FEATURE, UsagePage, UsageList, UsageLength, Report, ReportLength);
    }
    else
    {
        //
        // invalid report type
        //
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (ParserStatus == HIDPARSER_STATUS_SUCCESS)
    {
        //
        // success
        //
        return HIDP_STATUS_SUCCESS;
    }

    //
    // translate error
    //
    return TranslateHidParserStatus(ParserStatus);
}

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
    IN ULONG  ReportLength)
{
    HIDPARSER_STATUS ParserStatus;

    //
    // FIXME: implement searching in specific collection
    //
    ASSERT(LinkCollection == HIDP_LINK_COLLECTION_UNSPECIFIED);

    if (ReportType == HidP_Input)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetScaledUsageValueWithReport(Parser, CollectionContext, HID_REPORT_TYPE_INPUT, UsagePage, Usage, UsageValue, Report, ReportLength);
    }
    else if (ReportType == HidP_Output)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetScaledUsageValueWithReport(Parser, CollectionContext, HID_REPORT_TYPE_OUTPUT, UsagePage, Usage, UsageValue, Report, ReportLength);
    }
    else if (ReportType == HidP_Feature)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetScaledUsageValueWithReport(Parser, CollectionContext, HID_REPORT_TYPE_FEATURE,  UsagePage, Usage, UsageValue, Report, ReportLength);
    }
    else
    {
        //
        // invalid report type
        //
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (ParserStatus == HIDPARSER_STATUS_SUCCESS)
    {
        //
        // success
        //
        return HIDP_STATUS_SUCCESS;
    }

    //
    // translate error
    //
    return TranslateHidParserStatus(ParserStatus);
}

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
   IN PVOID  InsertCodesContext)
{
    ULONG Index;
    HIDPARSER_STATUS Status = HIDPARSER_STATUS_SUCCESS;

    for(Index = 0; Index < UsageListLength; Index++)
    {
        //
        // check current usage
        //
        if (ChangedUsageList[Index].UsagePage == HID_USAGE_PAGE_KEYBOARD)
        {
            //
            // process keyboard usage
            //
            Status = HidParser_TranslateKbdUsage(Parser, ChangedUsageList[Index].Usage, KeyAction, ModifierState, InsertCodesProcedure, InsertCodesContext);
        }
        else if (ChangedUsageList[Index].UsagePage == HID_USAGE_PAGE_CONSUMER)
        {
            //
            // process consumer usage
            //
            Status = HidParser_TranslateCustUsage(Parser, ChangedUsageList[Index].Usage, KeyAction, ModifierState, InsertCodesProcedure, InsertCodesContext);
        }
        else
        {
            //
            // invalid page / end of usage list page
            //
            return HIDP_STATUS_I8042_TRANS_UNKNOWN;
        }

        //
        // check status
        //
        if (Status != HIDPARSER_STATUS_SUCCESS)
        {
            //
            // failed
            //
            return TranslateHidParserStatus(Status);
        }
    }

    if (Status != HIDPARSER_STATUS_SUCCESS)
    {
        //
        // failed
        //
        return TranslateHidParserStatus(Status);
    }

    //
    // done
    //
    return HIDP_STATUS_SUCCESS;
}


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
    IN ULONG  ReportLength)
{
    return HidParser_GetUsages(Parser, CollectionContext, ReportType, HID_USAGE_PAGE_UNDEFINED, LinkCollection, (PUSAGE)ButtonList, UsageLength, Report, ReportLength);
}

HIDAPI
NTSTATUS
NTAPI
HidParser_UsageAndPageListDifference(
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
HidParser_GetSpecificButtonCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USAGE  UsagePage,
    IN USHORT  LinkCollection,
    IN USAGE  Usage,
    OUT PHIDP_BUTTON_CAPS  ButtonCaps,
    IN OUT PULONG  ButtonCapsLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}


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
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetExtendedAttributes(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN USHORT  DataIndex,
    OUT PHIDP_EXTENDED_ATTRIBUTES  Attributes,
    IN OUT PULONG  LengthAttributes)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetLinkCollectionNodes(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    OUT PHIDP_LINK_COLLECTION_NODE  LinkCollectionNodes,
    IN OUT PULONG  LinkCollectionNodesLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

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
    IN ULONG  ReportLength)
{
    HIDPARSER_STATUS ParserStatus;

    //
    // FIXME: implement searching in specific collection
    //
    ASSERT(LinkCollection == HIDP_LINK_COLLECTION_UNSPECIFIED);

    if (ReportType == HidP_Input)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetUsageValueWithReport(Parser, CollectionContext, HID_REPORT_TYPE_INPUT, UsagePage, Usage, UsageValue, Report, ReportLength);
    }
    else if (ReportType == HidP_Output)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetUsageValueWithReport(Parser, CollectionContext, HID_REPORT_TYPE_OUTPUT, UsagePage, Usage, UsageValue, Report, ReportLength);
    }
    else if (ReportType == HidP_Feature)
    {
        //
        // input report
        //
        ParserStatus = HidParser_GetUsageValueWithReport(Parser, CollectionContext, HID_REPORT_TYPE_FEATURE,  UsagePage, Usage, UsageValue, Report, ReportLength);
    }
    else
    {
        //
        // invalid report type
        //
        return HIDP_STATUS_INVALID_REPORT_TYPE;
    }

    if (ParserStatus == HIDPARSER_STATUS_SUCCESS)
    {
        //
        // success
        //
        return HIDP_STATUS_SUCCESS;
    }

    //
    // translate error
    //
    return TranslateHidParserStatus(ParserStatus);
}

NTSTATUS
NTAPI
HidParser_SysPowerEvent(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN PCHAR HidPacket,
    IN USHORT HidPacketLength,
    OUT PULONG OutputBuffer)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidParser_SysPowerCaps (
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    OUT PULONG OutputBuffer)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

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
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

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
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_TranslateUsagesToI8042ScanCodes(
  IN PUSAGE  ChangedUsageList,
  IN ULONG  UsageListLength,
  IN HIDP_KEYBOARD_DIRECTION  KeyAction,
  IN OUT PHIDP_KEYBOARD_MODIFIER_STATE  ModifierState,
  IN PHIDP_INSERT_SCANCODES  InsertCodesProcedure,
  IN PVOID  InsertCodesContext)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

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
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

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
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

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
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

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
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

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
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
ULONG
NTAPI
HidParser_MaxDataListLength(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return 0;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_InitializeReportForID(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    IN HIDP_REPORT_TYPE  ReportType,
    IN UCHAR  ReportID,
    IN OUT PCHAR  Report,
    IN ULONG  ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

#undef HidParser_GetValueCaps

HIDAPI
NTSTATUS
NTAPI
HidParser_GetValueCaps(
    IN PHID_PARSER Parser,
    IN PVOID CollectionContext,
    HIDP_REPORT_TYPE ReportType,
    PHIDP_VALUE_CAPS ValueCaps,
    PULONG ValueCapsLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}
