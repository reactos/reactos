/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser common functions implementation
 * COPYRIGHT:   Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#include "hidparser.h"
#include "hidphelpers.h"
#include "hidpmem.h"

#define NDEBUG
#include <debug.h>

char PreparsedMagic[8] = {'H', 'i', 'd', 'P', ' ', 'K', 'D', 'R'};

HIDAPI
NTSTATUS
NTAPI
HidParser_GetCaps(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    OUT PHIDP_CAPS Capabilities)
{
    USHORT index;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    // zero capabilities
    ZeroFunction(Capabilities, sizeof(HIDP_CAPS));

    // init capabilities
    Capabilities->UsagePage = PreparsedData->UsagePage;
    Capabilities->Usage = PreparsedData->Usage;
    Capabilities->InputReportByteLength = PreparsedData->InputReportByteLength;
    Capabilities->OutputReportByteLength = PreparsedData->OutputReportByteLength;
    Capabilities->FeatureReportByteLength = PreparsedData->FeatureReportByteLength;
    Capabilities->NumberLinkCollectionNodes = PreparsedData->LinkCollectionCount;

    Capabilities->NumberInputValueCaps = 0;
    Capabilities->NumberInputButtonCaps = 0;
    Capabilities->NumberInputDataIndices = 0;
    Capabilities->NumberOutputValueCaps = 0;
    Capabilities->NumberOutputButtonCaps = 0;
    Capabilities->NumberOutputDataIndices = 0;
    Capabilities->NumberFeatureValueCaps = 0;
    Capabilities->NumberFeatureButtonCaps = 0;
    Capabilities->NumberFeatureDataIndices = 0;

    for (index = PreparsedData->InputCapsStart; index < PreparsedData->InputCapsEnd; index++)
    {
        if (PreparsedData->ValueCaps[index].Flags.IsButton)
        {
            Capabilities->NumberInputButtonCaps++;
        }
        else
        {
            Capabilities->NumberInputValueCaps++;
        }
        Capabilities->NumberInputDataIndices += PreparsedData->ValueCaps[index].DataIndexMax - PreparsedData->ValueCaps[index].DataIndexMin + 1;
    }

    for (index = PreparsedData->OutputCapsStart; index < PreparsedData->OutputCapsEnd; index++)
    {
        if (PreparsedData->ValueCaps[index].Flags.IsButton)
        {
            Capabilities->NumberOutputButtonCaps++;
        }
        else
        {
            Capabilities->NumberOutputValueCaps++;
        }
        Capabilities->NumberOutputDataIndices += PreparsedData->ValueCaps[index].DataIndexMax - PreparsedData->ValueCaps[index].DataIndexMin + 1;
    }

    for (index = PreparsedData->FeatureCapsStart; index < PreparsedData->FeatureCapsEnd; index++)
    {
        if (PreparsedData->ValueCaps[index].Flags.IsButton)
        {
            Capabilities->NumberFeatureButtonCaps++;
        }
        else
        {
            Capabilities->NumberFeatureValueCaps++;
        }
        Capabilities->NumberFeatureDataIndices += PreparsedData->ValueCaps[index].DataIndexMax - PreparsedData->ValueCaps[index].DataIndexMin + 1;
    }

    return HIDP_STATUS_SUCCESS;
}

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
    IN OUT PUSHORT ValueCapsLength)
{
    USHORT valueCapsLen;
    USHORT startIndex;
    USHORT endIndex;
    USHORT index;
    USHORT count = 0;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    valueCapsLen = *ValueCapsLength;
    *ValueCapsLength = 0;

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            // not documented but still returns
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    ZeroFunction(ValueCaps, valueCapsLen * sizeof(HIDP_VALUE_CAPS));

    for (index = startIndex; index < endIndex; index++)
    {
        if (TRUE == PreparsedData->ValueCaps[index].Flags.IsButton)
        {
            continue;
        }

        if (FALSE == HidParser_FilterValueCap(&PreparsedData->ValueCaps[index],
                                              UsagePage,
                                              Usage,
                                              LinkCollection))
        {
            continue;
        }

        // if we have space then write
        if (count < valueCapsLen)
        {
            ValueCaps[count].UsagePage = PreparsedData->ValueCaps[index].UsagePage;
            ValueCaps[count].BitField = PreparsedData->ValueCaps[index].BitField.Raw;
            ValueCaps[count].BitSize = PreparsedData->ValueCaps[index].ReportSize;
            ValueCaps[count].IsAbsolute = PreparsedData->ValueCaps[index].Flags.IsAbsolute;
            ValueCaps[count].IsDesignatorRange = PreparsedData->ValueCaps[index].Flags.IsDesignatorRange;
            ValueCaps[count].IsRange = PreparsedData->ValueCaps[index].Flags.IsRange;
            ValueCaps[count].IsStringRange = PreparsedData->ValueCaps[index].Flags.IsStringRange;
            ValueCaps[count].LinkCollection = PreparsedData->ValueCaps[index].LinkCollection;
            ValueCaps[count].LinkUsage = PreparsedData->ValueCaps[index].LinkUsage;
            ValueCaps[count].LinkUsagePage = PreparsedData->ValueCaps[index].LinkUsagePage;
            ValueCaps[count].ReportID = PreparsedData->ValueCaps[index].ReportID;
            ValueCaps[count].Units = PreparsedData->ValueCaps[index].Units;
            ValueCaps[count].UnitsExp = PreparsedData->ValueCaps[index].UnitsExp;

            ValueCaps[count].ReportCount = PreparsedData->ValueCaps[index].ReportCount;
            // TODO: implement
            ValueCaps[count].IsAlias = FALSE;
            ValueCaps[count].HasNull = PreparsedData->ValueCaps[index].Logical.Value.NullValue;
            ValueCaps[count].LogicalMax = PreparsedData->ValueCaps[index].Logical.Value.Max;
            ValueCaps[count].LogicalMin = PreparsedData->ValueCaps[index].Logical.Value.Min;
            ValueCaps[count].PhysicalMax = PreparsedData->ValueCaps[index].PhysicalMax;
            ValueCaps[count].PhysicalMin = PreparsedData->ValueCaps[index].PhysicalMin;

            if (ValueCaps[count].IsRange)
            {
                ValueCaps[count].Range.UsageMin = PreparsedData->ValueCaps[index].UsageMin;
                ValueCaps[count].Range.UsageMax = PreparsedData->ValueCaps[index].UsageMax;
                ValueCaps[count].Range.DataIndexMin = PreparsedData->ValueCaps[index].DataIndexMin;
                ValueCaps[count].Range.DataIndexMax = PreparsedData->ValueCaps[index].DataIndexMax;
            }
            else
            {
                ValueCaps[count].NotRange.Usage = PreparsedData->ValueCaps[index].UsageMin;
                ValueCaps[count].NotRange.DataIndex = PreparsedData->ValueCaps[index].DataIndexMin;
            }

            if (ValueCaps[count].IsStringRange)
            {
                ValueCaps[count].Range.StringMin = PreparsedData->ValueCaps[index].StringMin;
                ValueCaps[count].Range.StringMax = PreparsedData->ValueCaps[index].StringMax;
            }
            else
            {
                ValueCaps[count].NotRange.StringIndex = PreparsedData->ValueCaps[index].StringMin;
            }

            if (ValueCaps[count].IsDesignatorRange)
            {
                ValueCaps[count].Range.DesignatorMin = PreparsedData->ValueCaps[index].DesignatorMin;
                ValueCaps[count].Range.DesignatorMax = PreparsedData->ValueCaps[index].DesignatorMax;
            }
            else
            {
                ValueCaps[count].NotRange.DesignatorIndex = PreparsedData->ValueCaps[index].DesignatorMin;
            }
        }

        count++;
    }

    *ValueCapsLength = count;

    return HIDP_STATUS_SUCCESS;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetSpecificButtonCaps(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN USAGE Usage,
    OUT PHIDP_BUTTON_CAPS ButtonCaps,
    IN OUT PUSHORT ButtonCapsLength)
{
    USHORT buttonCapsLen;
    USHORT startIndex;
    USHORT endIndex;
    USHORT index;
    USHORT count = 0;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    buttonCapsLen = *ButtonCapsLength;
    *ButtonCapsLength = 0;

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            // not documented but still returns
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    ZeroFunction(ButtonCaps, buttonCapsLen * sizeof(HIDP_BUTTON_CAPS));

    for (index = startIndex; index < endIndex; index++)
    {
        if (FALSE == PreparsedData->ValueCaps[index].Flags.IsButton)
        {
            continue;
        }

        if (FALSE == HidParser_FilterValueCap(&PreparsedData->ValueCaps[index],
                                              UsagePage,
                                              Usage,
                                              LinkCollection))
        {
            continue;
        }

        // if we have space then write
        if (count < buttonCapsLen)
        {
            ButtonCaps[count].UsagePage = PreparsedData->ValueCaps[index].UsagePage;
            ButtonCaps[count].BitField = PreparsedData->ValueCaps[index].BitField.Raw;
            ButtonCaps[count].IsAbsolute = PreparsedData->ValueCaps[index].Flags.IsAbsolute;
            ButtonCaps[count].IsDesignatorRange = PreparsedData->ValueCaps[index].Flags.IsDesignatorRange;
            ButtonCaps[count].IsRange = PreparsedData->ValueCaps[index].Flags.IsRange;
            ButtonCaps[count].IsStringRange = PreparsedData->ValueCaps[index].Flags.IsStringRange;
            ButtonCaps[count].LinkCollection = PreparsedData->ValueCaps[index].LinkCollection;
            ButtonCaps[count].LinkUsage = PreparsedData->ValueCaps[index].LinkUsage;
            ButtonCaps[count].LinkUsagePage = PreparsedData->ValueCaps[index].LinkUsagePage;
            ButtonCaps[count].ReportID = PreparsedData->ValueCaps[index].ReportID;

            ButtonCaps[count].IsAlias = FALSE;

            // TO-DO check if needed
            if (ButtonCaps[count].IsRange)
            {
                ButtonCaps[count].Range.UsageMin = PreparsedData->ValueCaps[index].UsageMin;
                ButtonCaps[count].Range.UsageMax = PreparsedData->ValueCaps[index].UsageMax;
                ButtonCaps[count].Range.DataIndexMin = PreparsedData->ValueCaps[index].DataIndexMin;
                ButtonCaps[count].Range.DataIndexMax = PreparsedData->ValueCaps[index].DataIndexMax;
            }
            else
            {
                ButtonCaps[count].NotRange.Usage = PreparsedData->ValueCaps[index].UsageMin;
                ButtonCaps[count].NotRange.DataIndex = PreparsedData->ValueCaps[index].DataIndexMin;
            }
            if (ButtonCaps[count].IsStringRange)
            {
                ButtonCaps[count].Range.StringMin = PreparsedData->ValueCaps[index].StringMin;
                ButtonCaps[count].Range.StringMax = PreparsedData->ValueCaps[index].StringMax;
            }
            else
            {
                ButtonCaps[count].NotRange.StringIndex = PreparsedData->ValueCaps[index].StringMin;
            }
            if (ButtonCaps[count].IsDesignatorRange)
            {
                ButtonCaps[count].Range.DesignatorMin = PreparsedData->ValueCaps[index].DesignatorMin;
                ButtonCaps[count].Range.DesignatorMax = PreparsedData->ValueCaps[index].DesignatorMax;
            }
            else
            {
                ButtonCaps[count].NotRange.DesignatorIndex = PreparsedData->ValueCaps[index].DesignatorMin;
            }

        }

        count++;
    }

    *ButtonCapsLength = count;

    return HIDP_STATUS_SUCCESS;
}

HIDAPI
ULONG
NTAPI
HidParser_MaxUsageListLength(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage OPTIONAL)
{
    USHORT startIndex;
    USHORT endIndex;
    USHORT index;
    ULONG count = 0;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
    case HidP_Input:
        startIndex = PreparsedData->InputCapsStart;
        endIndex = PreparsedData->InputCapsEnd;
        break;
    case HidP_Output:
        startIndex = PreparsedData->OutputCapsStart;
        endIndex = PreparsedData->OutputCapsEnd;
        break;
    case HidP_Feature:
        startIndex = PreparsedData->FeatureCapsStart;
        endIndex = PreparsedData->FeatureCapsEnd;
        break;
    default:
        // not documented but still returns
        return HIDP_STATUS_INVALID_REPORT_TYPE;
        break;
    }

    for (index = startIndex; index < endIndex; index++)
    {
        // counting only buttons
        if (FALSE == PreparsedData->ValueCaps[index].Flags.IsButton)
        {
            continue;
        }

        // Arrays must be added only once
        if (TRUE == PreparsedData->ValueCaps[index].Flags.ArrayHasMore)
        {
            continue;
        }

        if (UsagePage == HID_USAGE_PAGE_UNDEFINED || UsagePage == PreparsedData->ValueCaps[index].UsagePage)
        {
            count += PreparsedData->ValueCaps[index].ReportCount;
        }
    }

    return count;
}

HIDAPI
ULONG
NTAPI
HidParser_MaxDataListLength(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType)
{
    USHORT startIndex;
    USHORT endIndex;
    USHORT index;
    ULONG count = 0;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            // not documented but still returns
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    for (index = startIndex; index < endIndex; index++)
    {
        // Arrays must be added only once
        if (TRUE == PreparsedData->ValueCaps[index].Flags.ArrayHasMore)
        {
            continue;
        }

        count += PreparsedData->ValueCaps[index].ReportCount;
    }

    return count;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsages(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection OPTIONAL,
    OUT USAGE *UsageList,
    IN OUT PULONG UsageLength,
    IN PCHAR Report,
    IN ULONG ReportLength)
{
    USHORT arrayStart;
    PULONG dataPtr;
    ULONG stackBuffer[16];
    USHORT i, j;

    NTSTATUS status = HIDP_STATUS_SUCCESS;
    USHORT startIndex = 0;
    USHORT endIndex = 0;
    ULONG outputUsageLen = 0;
    ULONG inputUsageLen = 0;
    PULONG buffer = NULL;
    PHIDPARSER_VALUE_CAPS valueCap = NULL;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    inputUsageLen = *UsageLength;
    ZeroFunction(UsageList, inputUsageLen * sizeof(USAGE));
    *UsageLength = 0;

    for (i = startIndex; i < endIndex; i++)
    {
        if (FALSE == PreparsedData->ValueCaps[i].Flags.IsButton)
        {
            continue;
        }

        if (FALSE == HidParser_FilterValueCap(&PreparsedData->ValueCaps[i],
                                              UsagePage,
                                              HID_USAGE_PAGE_UNDEFINED,
                                              LinkCollection))
        {
            continue;
        }

        // Found
        valueCap = &PreparsedData->ValueCaps[i];

        // we have small buffer at the stack, if it too small, allocate new one
        if (valueCap->ReportCount < sizeof(stackBuffer) / sizeof(stackBuffer[0]))
        {
            dataPtr = stackBuffer;
            ZeroFunction(dataPtr, valueCap->ReportCount * sizeof(ULONG));
        }
        else
        {
            buffer = AllocFunction(sizeof(ULONG) * valueCap->ReportCount);
            ASSERT(buffer != NULL);
            dataPtr = buffer;
        }

        status = HidParser_GetValuesForCap(valueCap,
                                           (PUCHAR)Report,
                                           ReportLength,
                                           dataPtr,
                                           valueCap->ReportCount);

        if (status != HIDP_STATUS_SUCCESS)
        {
            if (buffer != NULL)
            {
                FreeFunction(buffer);
                buffer = NULL;
            }
            return status;
        }

        if (valueCap->Flags.ArrayHasMore) //array
        {
            arrayStart = i;
            while (valueCap->Flags.ArrayHasMore)
            {
                // skip array item
                i++;
            }
            // now i is index of last element of array;

            for (j = 0; j < valueCap->ReportCount; j++)
            {
                // TO-DO: clip
                if (dataPtr[j] > 0)
                {
                    if (outputUsageLen < inputUsageLen)
                    {
                        UsageList[outputUsageLen] = PreparsedData->ValueCaps[dataPtr[j] + arrayStart].UsageMin;
                    }
                    else
                    {
                        status = HIDP_STATUS_BUFFER_TOO_SMALL;
                    }
                    outputUsageLen++;
                }
            }
        }
        else if (valueCap->ReportSize == 1) // bitmap
        {
            for (j = 0; j < valueCap->ReportCount; j++)
            {
                if (dataPtr[j] == TRUE)
                {
                    if (outputUsageLen < inputUsageLen)
                    {
                        UsageList[outputUsageLen] = valueCap->UsageMin + j;
                    }
                    else
                    {
                        status = HIDP_STATUS_BUFFER_TOO_SMALL;
                    }
                    outputUsageLen++;
                }
            }
        }
        else // range
        {
            for (j = 0; j < valueCap->ReportCount; j++)
            {
                // TODO: clip
                if (dataPtr[j] > 0)
                {
                    if (outputUsageLen < inputUsageLen)
                    {
                        UsageList[outputUsageLen] = valueCap->UsageMin + dataPtr[j];
                    }
                    else
                    {
                        status = HIDP_STATUS_BUFFER_TOO_SMALL;
                    }
                    outputUsageLen++;
                }
            }
        }

        if (buffer != NULL)
        {
            FreeFunction(buffer);
            buffer = NULL;
        }

    }

    // valueCap not setted, mean usage not found
    if (valueCap == NULL)
    {
        status = HIDP_STATUS_USAGE_NOT_FOUND;
    }

    *UsageLength = outputUsageLen;

    return status;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetScaledUsageValue(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection OPTIONAL,
    IN USAGE Usage,
    OUT PLONG UsageValue,
    IN PCHAR Report,
    IN ULONG ReportLength)
{
    USHORT startIndex;
    USHORT endIndex;
    USHORT index;
    NTSTATUS status = HIDP_STATUS_SUCCESS;
    ULONG mask = 0xFFFFFFFF;
    PHIDPARSER_VALUE_CAPS valueCap = NULL;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    for (index = startIndex; index < endIndex; index++)
    {
        if (TRUE == PreparsedData->ValueCaps[index].Flags.IsButton)
        {
            continue;
        }

        if (TRUE == HidParser_FilterValueCap(&PreparsedData->ValueCaps[index],
                                             UsagePage,
                                             Usage,
                                             LinkCollection))
        {
            // Found
            valueCap = &PreparsedData->ValueCaps[index];
            break;
        }
    }

    if (valueCap == NULL)
    {
        return HIDP_STATUS_USAGE_NOT_FOUND;
    }

    // read one usage from the report
    status = HidParser_GetValuesForCap(valueCap,
                                       (PUCHAR)Report,
                                       ReportLength,
                                       (PULONG)UsageValue,
                                       1);
    if (status != HIDP_STATUS_SUCCESS)
    {
        return status;
    }

    if (valueCap->Logical.Value.Min > valueCap->Logical.Value.Max ||
        valueCap->PhysicalMin > valueCap->PhysicalMax)
    {
        return HIDP_STATUS_BAD_LOG_PHY_VALUES;
    }

    if (valueCap->Logical.Value.Min < 0)
    {
        // make signed
        mask = (mask << (valueCap->ReportSize - 1));
        if ((mask & *UsageValue) != 0)
        {
            *UsageValue |= mask;
        }
    }

    if (valueCap->Logical.Value.Min > (*UsageValue) ||
        valueCap->Logical.Value.Max < (*UsageValue))
    {
        if (valueCap->Logical.Value.NullValue > 0)
        {
            return HIDP_STATUS_NULL;
        }
        else
        {
            return HIDP_STATUS_VALUE_OUT_OF_RANGE;
        }
    }

    if (valueCap->PhysicalMax != valueCap->PhysicalMin)
    {
        // physical range is defined, remap the value
        *UsageValue = HidParser_MapValue(*UsageValue,
                                         valueCap->Logical.Value.Min,
                                         valueCap->Logical.Value.Max,
                                         valueCap->PhysicalMin,
                                         valueCap->PhysicalMax);
    }

    return HIDP_STATUS_SUCCESS;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsagesEx(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USHORT LinkCollection,
    OUT PUSAGE_AND_PAGE ButtonList,
    IN OUT ULONG  *UsageLength,
    IN PCHAR Report,
    IN ULONG ReportLength)
{
    USHORT arrayStart;
    PULONG dataPtr;
    ULONG stackBuffer[16];
    USHORT startIndex;
    USHORT endIndex;
    USHORT i, j;
    NTSTATUS status = HIDP_STATUS_SUCCESS;
    ULONG outputUsageLen = 0;
    ULONG inputUsageLen = 0;
    PULONG buffer = NULL;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    inputUsageLen = *UsageLength;
    ZeroFunction(ButtonList, inputUsageLen * sizeof(USAGE_AND_PAGE));
    *UsageLength = 0;

    for (i = startIndex; i < endIndex; i++)
    {
        if (FALSE == PreparsedData->ValueCaps[i].Flags.IsButton)
        {
            continue;
        }

        // we have small buffer at the stack, if it too small, allocate new one
        if (PreparsedData->ValueCaps[i].ReportCount < sizeof(stackBuffer) / sizeof(stackBuffer[0]))
        {
            dataPtr = stackBuffer;
            ZeroFunction(dataPtr, PreparsedData->ValueCaps[i].ReportCount * sizeof(ULONG));
        }
        else
        {
            buffer = AllocFunction(sizeof(ULONG) * PreparsedData->ValueCaps[i].ReportCount);
            ASSERT(buffer != NULL);
            dataPtr = buffer;
        }

        status = HidParser_GetValuesForCap(&PreparsedData->ValueCaps[i],
                                           (PUCHAR)Report,
                                           ReportLength,
                                           dataPtr,
                                           PreparsedData->ValueCaps[i].ReportCount);
        if (status != HIDP_STATUS_SUCCESS)
        {
            if (buffer != NULL)
            {
                FreeFunction(buffer);
                buffer = NULL;
            }
            return status;
        }

        if (PreparsedData->ValueCaps[i].Flags.ArrayHasMore)
        {
            arrayStart = i;
            while (PreparsedData->ValueCaps[i].Flags.ArrayHasMore)
            {
                // skip array item
                i++;
            }
            // now i is index of last element of array;

            for (j = 0; j < PreparsedData->ValueCaps[i].ReportCount; j++)
            {
                // TO-DO: clip
                if (dataPtr[j] > 0)
                {
                    if (outputUsageLen < inputUsageLen)
                    {
                        ButtonList[outputUsageLen].Usage = PreparsedData->ValueCaps[dataPtr[j] + arrayStart].UsageMin;
                        ButtonList[outputUsageLen].UsagePage = PreparsedData->ValueCaps[dataPtr[j] + arrayStart].UsagePage;
                    }
                    else
                    {
                        status = HIDP_STATUS_BUFFER_TOO_SMALL;
                    }
                    outputUsageLen++;
                }
            }
        }
        else if (PreparsedData->ValueCaps[i].ReportSize == 1) // bitmap
        {
            for (j = 0; j < PreparsedData->ValueCaps[i].ReportCount; j++)
            {
                if (dataPtr[j] == TRUE)
                {
                    if (outputUsageLen < inputUsageLen)
                    {
                        ButtonList[outputUsageLen].Usage = PreparsedData->ValueCaps[i].UsageMin + j;
                        ButtonList[outputUsageLen].UsagePage = PreparsedData->ValueCaps[i].UsagePage;
                    }
                    else
                    {
                        status = HIDP_STATUS_BUFFER_TOO_SMALL;
                    }
                    outputUsageLen++;
                }
            }
        }
        else // array of range
        {
            for (j = 0; j < PreparsedData->ValueCaps[i].ReportCount; j++)
            {
                // TODO: clip
                if (dataPtr[j] > 0)
                {
                    if (outputUsageLen < inputUsageLen)
                    {
                        ButtonList[outputUsageLen].Usage = PreparsedData->ValueCaps[i].UsageMin + dataPtr[j];
                        ButtonList[outputUsageLen].UsagePage = PreparsedData->ValueCaps[i].UsagePage;
                    }
                    else
                    {
                        status = HIDP_STATUS_BUFFER_TOO_SMALL;
                    }
                    outputUsageLen++;
                }
            }
        }

        if (buffer != NULL)
        {
            FreeFunction(buffer);
            buffer = NULL;
        }

    }

    *UsageLength = outputUsageLen;

    return status;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetData(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    OUT PHIDP_DATA DataList,
    IN OUT PULONG DataLength,
    IN PCHAR Report,
    IN ULONG ReportLength)
{
    USHORT arrayStart;
    PULONG dataPtr;
    ULONG stackBuffer[16];
    USHORT startIndex;
    USHORT endIndex;
    USHORT i, j;
    NTSTATUS status = HIDP_STATUS_SUCCESS;
    ULONG outputUsageLen = 0;
    ULONG inputUsageLen = 0;
    PULONG buffer = NULL;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    inputUsageLen = *DataLength;
    ZeroFunction(DataList, inputUsageLen * sizeof(HIDP_DATA));
    *DataLength = 0;

    if (startIndex == endIndex)
    {
        return HIDP_STATUS_REPORT_DOES_NOT_EXIST;
    }

    for (i = startIndex; i < endIndex; i++)
    {
        // we have small buffer at the stack, if it too small, allocate new one
        if (PreparsedData->ValueCaps[i].ReportCount < sizeof(stackBuffer) / sizeof(stackBuffer[0]))
        {
            dataPtr = stackBuffer;
            ZeroFunction(dataPtr, PreparsedData->ValueCaps[i].ReportCount * sizeof(ULONG));
        }
        else
        {
            buffer = AllocFunction(sizeof(ULONG) * PreparsedData->ValueCaps[i].ReportCount);
            ASSERT(buffer != NULL);
            dataPtr = buffer;
        }

        status = HidParser_GetValuesForCap(&PreparsedData->ValueCaps[i],
                                           (PUCHAR)Report,
                                           ReportLength,
                                           dataPtr,
                                           PreparsedData->ValueCaps[i].ReportCount);

        if (status != HIDP_STATUS_SUCCESS)
        {
            if (buffer != NULL)
            {
                FreeFunction(buffer);
                buffer = NULL;
            }
            return status;
        }

        if (PreparsedData->ValueCaps[i].Flags.IsButton == FALSE)
        {
            for (j = 0; j < PreparsedData->ValueCaps[i].ReportCount; j++)
            {
                if (outputUsageLen < inputUsageLen)
                {
                    DataList[outputUsageLen].DataIndex = PreparsedData->ValueCaps[i].DataIndexMin + j;
                    DataList[outputUsageLen].RawValue = dataPtr[j];
                }
                else
                {
                    status = HIDP_STATUS_BUFFER_TOO_SMALL;
                }
                outputUsageLen++;
            }
        }
        else
        {
            if (PreparsedData->ValueCaps[i].Flags.ArrayHasMore)
            {
                arrayStart = i;
                while (PreparsedData->ValueCaps[i].Flags.ArrayHasMore)
                {
                    // skip array item
                    i++;
                }
                // now i is index of last element of array;

                for (j = 0; j < PreparsedData->ValueCaps[i].ReportCount; j++)
                {
                    // TO-DO: clip
                    if (dataPtr[j] > 0)
                    {
                        if (outputUsageLen < inputUsageLen)
                        {
                            DataList[outputUsageLen].DataIndex = PreparsedData->ValueCaps[dataPtr[j] + arrayStart].DataIndexMin;
                            DataList[outputUsageLen].On = TRUE;
                        }
                        else
                        {
                            status = HIDP_STATUS_BUFFER_TOO_SMALL;
                        }
                        outputUsageLen++;
                    }
                }
            }
            else if (PreparsedData->ValueCaps[i].ReportSize == 1) // bitmap
            {
                for (j = 0; j < PreparsedData->ValueCaps[i].ReportCount; j++)
                {
                    if (dataPtr[j] == TRUE)
                    {
                        if (outputUsageLen < inputUsageLen)
                        {
                            DataList[outputUsageLen].DataIndex = PreparsedData->ValueCaps[i].DataIndexMin + j;
                            DataList[outputUsageLen].On = TRUE;
                        }
                        else
                        {
                            status = HIDP_STATUS_BUFFER_TOO_SMALL;
                        }
                        outputUsageLen++;
                    }
                }
            }
            else // array of range
            {
                for (j = 0; j < PreparsedData->ValueCaps[i].ReportCount; j++)
                {
                    // TODO: clip
                    if (dataPtr[j] > 0)
                    {
                        if (outputUsageLen < inputUsageLen)
                        {
                            DataList[outputUsageLen].DataIndex = PreparsedData->ValueCaps[i].DataIndexMin + dataPtr[j];
                            DataList[outputUsageLen].On = TRUE;
                        }
                        else
                        {
                            status = HIDP_STATUS_BUFFER_TOO_SMALL;
                        }
                        outputUsageLen++;
                    }
                }
            }
        }

        if (buffer != NULL)
        {
            FreeFunction(buffer);
            buffer = NULL;
        }

    }

    *DataLength = outputUsageLen;

    return status;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetExtendedAttributes(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USHORT DataIndex,
    OUT PHIDP_EXTENDED_ATTRIBUTES Attributes,
    IN OUT PULONG LengthAttributes)
{
    USHORT startIndex;
    USHORT endIndex;
    USHORT index;
    ULONG attributesSize;
    NTSTATUS status = HIDP_STATUS_SUCCESS;
    PHIDPARSER_VALUE_CAPS valueCap = NULL;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    attributesSize = *LengthAttributes;
    *LengthAttributes = sizeof(HIDP_EXTENDED_ATTRIBUTES);
    if (attributesSize < sizeof(HIDP_EXTENDED_ATTRIBUTES))
    {
        return HIDP_STATUS_BUFFER_TOO_SMALL;
    }

    ZeroFunction(Attributes, *LengthAttributes);

    for (index = startIndex; index < endIndex; index++)
    {
        if (PreparsedData->ValueCaps[index].DataIndexMin <= DataIndex &&
            PreparsedData->ValueCaps[index].DataIndexMax >= DataIndex)
        {
            valueCap = &PreparsedData->ValueCaps[index];
            break;
        }
    }

    if (valueCap == NULL)
    {
        return HIDP_STATUS_DATA_INDEX_NOT_FOUND;
    }

    if (valueCap->Flags.UnknownGlobalCount > 0)
    {
        Attributes->NumGlobalUnknowns = valueCap->Flags.UnknownGlobalCount;
        // calculate how much bytes left;
        attributesSize -= (sizeof(HIDP_EXTENDED_ATTRIBUTES) - sizeof(Attributes->Data));
        if (attributesSize < sizeof(HIDP_UNKNOWN_TOKEN) * Attributes->NumGlobalUnknowns)
        {
            // not enought
            status = HIDP_STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            // too many
            attributesSize = sizeof(HIDP_UNKNOWN_TOKEN) * Attributes->NumGlobalUnknowns;
        }
        CopyFunction(Attributes->Data, valueCap->UnknownGlobals, attributesSize);

        // update the returning length
        *LengthAttributes += sizeof(HIDP_UNKNOWN_TOKEN) * Attributes->NumGlobalUnknowns - sizeof(Attributes->Data);
    }

    return status;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetLinkCollectionNodes(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    OUT PHIDP_LINK_COLLECTION_NODE LinkCollectionNodes,
    IN OUT PULONG LinkCollectionNodesLength)
{
    USHORT listSize;
    USHORT index;
    PHIDPARSER_LINK_COLLECTION_NODE nodesArray;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        // not documented
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    listSize = *LinkCollectionNodesLength;
    *LinkCollectionNodesLength = PreparsedData->LinkCollectionCount;

    if (listSize < PreparsedData->LinkCollectionCount)
    {
        return HIDP_STATUS_BUFFER_TOO_SMALL;
    }

    nodesArray = (PHIDPARSER_LINK_COLLECTION_NODE)(((PUCHAR)PreparsedData) + PreparsedData->CapsByteLength);

    for (index = 0; index < PreparsedData->LinkCollectionCount; index++)
    {
        CopyFunction(&LinkCollectionNodes[index], &nodesArray[index], sizeof(HIDPARSER_LINK_COLLECTION_NODE));
    }

    return HIDP_STATUS_SUCCESS;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsageValue(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN USAGE Usage,
    OUT PULONG UsageValue,
    IN PCHAR Report,
    IN ULONG ReportLength)
{
    USHORT startIndex;
    USHORT endIndex;
    USHORT index;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
    case HidP_Input:
        startIndex = PreparsedData->InputCapsStart;
        endIndex = PreparsedData->InputCapsEnd;
        break;
    case HidP_Output:
        startIndex = PreparsedData->OutputCapsStart;
        endIndex = PreparsedData->OutputCapsEnd;
        break;
    case HidP_Feature:
        startIndex = PreparsedData->FeatureCapsStart;
        endIndex = PreparsedData->FeatureCapsEnd;
        break;
    default:
        return HIDP_STATUS_INVALID_REPORT_TYPE;
        break;
    }

    for (index = startIndex; index < endIndex; index++)
    {
        if (PreparsedData->ValueCaps[index].Flags.IsButton == TRUE)
        {
            continue;
        }

        if (FALSE == HidParser_FilterValueCap(&PreparsedData->ValueCaps[index],
                                              UsagePage,
                                              Usage,
                                              LinkCollection))
        {
            continue;
        }

        // read one usage from the report
        return HidParser_GetValuesForCap(&PreparsedData->ValueCaps[index],
                                         (PUCHAR)Report,
                                         ReportLength,
                                         UsageValue,
                                         1);
    }

    return HIDP_STATUS_USAGE_NOT_FOUND;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_GetUsageValueArray(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection OPTIONAL,
    IN USAGE Usage,
    OUT PCHAR UsageValue,
    IN USHORT UsageValueByteLength,
    IN PCHAR Report,
    IN ULONG ReportLength)
{
    NTSTATUS status = HIDP_STATUS_SUCCESS;
    USHORT startIndex = 0;
    USHORT endIndex = 0;
    PULONG buffer = NULL;
    PHIDPARSER_VALUE_CAPS valueCap = NULL;
    USHORT index;
    USHORT bitsWrited = 0;
    USHORT startBit = 0;
    USHORT startByte = 0;

    if (FALSE == HidParser_CheckPreparsedMagic(PreparsedData))
    {
        return HIDP_STATUS_INVALID_PREPARSED_DATA;
    }

    switch (ReportType)
    {
        case HidP_Input:
            startIndex = PreparsedData->InputCapsStart;
            endIndex = PreparsedData->InputCapsEnd;
            break;
        case HidP_Output:
            startIndex = PreparsedData->OutputCapsStart;
            endIndex = PreparsedData->OutputCapsEnd;
            break;
        case HidP_Feature:
            startIndex = PreparsedData->FeatureCapsStart;
            endIndex = PreparsedData->FeatureCapsEnd;
            break;
        default:
            return HIDP_STATUS_INVALID_REPORT_TYPE;
            break;
    }

    ZeroFunction(UsageValue, UsageValueByteLength);

    for (index = startIndex; index < endIndex; index++)
    {
        if (TRUE == PreparsedData->ValueCaps[index].Flags.IsButton)
        {
            continue;
        }

        if (FALSE == HidParser_FilterValueCap(&PreparsedData->ValueCaps[index],
                                              UsagePage,
                                              Usage,
                                              LinkCollection))
        {
            continue;
        }

        valueCap = &PreparsedData->ValueCaps[index];
        break;
    }

    if (valueCap == NULL)
    {
        return HIDP_STATUS_USAGE_NOT_FOUND;
    }

    buffer = AllocFunction(sizeof(ULONG) * valueCap->ReportCount);
    ASSERT(buffer != NULL);

    // read usage from the report
    status = HidParser_GetValuesForCap(valueCap,
                                       (PUCHAR)Report,
                                       ReportLength,
                                       buffer,
                                       valueCap->ReportCount);
    if (status != HIDP_STATUS_SUCCESS)
    {
        FreeFunction(buffer);
        return status;
    }

    if (valueCap->ReportCount < 2)
    {
        status = HIDP_STATUS_NOT_VALUE_ARRAY;
    }

    for (index = 0; index < valueCap->ReportCount; index++)
    {
        bitsWrited += valueCap->ReportSize;
        if (UsageValueByteLength < bitsWrited / 8)
        {
            FreeFunction(buffer);
            return HIDP_STATUS_BUFFER_TOO_SMALL;
        }

        switch ((valueCap->ReportSize - 1) / 8)
        {
            case 3:
                UsageValue[startByte + 4] |= buffer[index] >> (32 - startBit);
            case 2:
                UsageValue[startByte + 3] |= buffer[index] >> (24 - startBit);
            case 1:
                UsageValue[startByte + 2] |= buffer[index] >> (16 - startBit);
            case 0:
                UsageValue[startByte + 1] |= buffer[index] >> (8 - startBit);
                UsageValue[startByte] |= buffer[index] << startBit;
                break;
            default:
                break;
        }

        startBit = bitsWrited % 8;
        startByte += (valueCap->ReportSize - 1) / 8;
    }

    FreeFunction(buffer);
    return status;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_UnsetUsages(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN PUSAGE UsageList,
    IN OUT PULONG UsageLength,
    IN OUT PCHAR Report,
    IN ULONG ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_SetUsages(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN PUSAGE UsageList,
    IN OUT PULONG UsageLength,
    IN OUT PCHAR Report,
    IN ULONG ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_SetUsageValueArray(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection OPTIONAL,
    IN USAGE Usage,
    IN PCHAR UsageValue,
    IN USHORT UsageValueByteLength,
    OUT PCHAR Report,
    IN ULONG ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_SetUsageValue(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection,
    IN USAGE Usage,
    IN ULONG UsageValue,
    IN OUT PCHAR Report,
    IN ULONG ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_SetScaledUsageValue(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN USAGE UsagePage,
    IN USHORT LinkCollection OPTIONAL,
    IN USAGE Usage,
    IN LONG UsageValue,
    IN OUT PCHAR Report,
    IN ULONG ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_SetData(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN PHIDP_DATA DataList,
    IN OUT PULONG DataLength,
    IN OUT PCHAR Report,
    IN ULONG ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

HIDAPI
NTSTATUS
NTAPI
HidParser_InitializeReportForID(
    IN PHIDPARSER_PREPARSED_DATA PreparsedData,
    IN HIDP_REPORT_TYPE ReportType,
    IN UCHAR ReportID,
    IN OUT PCHAR Report,
    IN ULONG ReportLength)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}
