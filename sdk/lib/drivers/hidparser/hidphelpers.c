/*
 * PROJECT:     ReactOS HID Parser Library
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     HID Parser helper functions
 * COPYRIGHT:   Copyright 2022 Roman Masanin <36927roma@gmail.com>
 */

#include "hidphelpers.h"

#define NDEBUG
#include <debug.h>

/* HidParser_GetValuesForCap
 * ValueCap - pointer to internal valueCap structure
 * Report - pointer to report
 * ReportLen - size of the report in bytes
 * Values - array where return will be stored
 * ValuesLen - size of Values in elements, must be non zero
 */
NTSTATUS
HidParser_GetValuesForCap(
    IN PHIDPARSER_VALUE_CAPS ValueCap,
    IN PUCHAR Report,
    IN ULONG ReportLen,
    OUT PULONG Values,
    IN USHORT ValuesLen)
{
    ULONG data;
    ULONG index;
    ULONG mask = ((1 << ValueCap->ReportSize) - 1);
    USHORT startByte = ValueCap->StartByte;
    USHORT startBit = ValueCap->StartBit;
    USHORT reportSizeBytes = (ValueCap->ReportSize - 1) / 8;
    USHORT reportCount = min(ValuesLen, ValueCap->ReportCount);

    ASSERT(reportCount != 0);

    if (Report[0] != ValueCap->ReportID)
    {
        return HIDP_STATUS_INCOMPATIBLE_REPORT_ID;
    }

    for (index = 0; index < reportCount; index++)
    {
        if (ReportLen < startByte + reportSizeBytes)
        {
            return HIDP_STATUS_INVALID_REPORT_LENGTH;
        }

        // Get data from the report
        data = 0;
        switch (reportSizeBytes)
        {
            case 3:
                data |= Report[startByte + 3];
                data = data << 8;
            case 2:
                data |= Report[startByte + 2];
                data = data << 8;
            case 1:
                data |= Report[startByte + 1];
                data = data << 8;
            case 0:
                data |= Report[startByte];
                break;

            default:
                // error, report size for the item cannot be bigger then 32 bytes.
                break;
        }

        // Store result
        Values[index] = (data >> startBit) & mask;

        // Update offsets
        startBit += ValueCap->ReportSize;
        startByte += startBit / 8;
        startBit %= 8;
    }

    return HIDP_STATUS_SUCCESS;
}

BOOLEAN
HidParser_CheckPreparsedMagic(IN PHIDPARSER_PREPARSED_DATA PreparsedData)
{
    if (PreparsedData == NULL)
    {
        return FALSE;
    }

    for (int i = 0; i < 8; i++)
    {
        if (PreparsedData->Magic[i] != PreparsedMagic[i])
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
HidParser_FilterValueCap(
    IN PHIDPARSER_VALUE_CAPS ValueCap,
    IN USAGE UsagePage,
    IN USAGE Usage,
    IN USHORT LinkCollection)
{
    if (UsagePage != HID_USAGE_PAGE_UNDEFINED)
    {
        if (UsagePage != ValueCap->UsagePage)
        {
            return FALSE;
        }
    }

    if (Usage != HID_USAGE_PAGE_UNDEFINED)
    {
        if (Usage > ValueCap->UsageMax || Usage < ValueCap->UsageMin)
        {
            return FALSE;
        }
    }

    if (LinkCollection != 0)
    {
        if (LinkCollection != ValueCap->LinkCollection)
        {
            return FALSE;
        }
    }

    return TRUE;
}

// TODO: propper round
LONG
HidParser_MapValue(LONG Value,
                   LONG LogicalMin,
                   LONG LogicalMax,
                   LONG PhysicalMin,
                   LONG PhysicalMax)
{
    ULONGLONG shiftedValue;
    ULONGLONG temp;

    LONG logicalRange;
    LONG physicalRange;
    ULONG shiftedResult;
    LONG result = 0;

    ASSERT(Value >= LogicalMin);
    ASSERT(LogicalMin <= LogicalMax);
    ASSERT(PhysicalMin <= PhysicalMax);

    logicalRange = LogicalMax - LogicalMin;
    physicalRange = PhysicalMax - PhysicalMin;

    shiftedValue = Value;
    shiftedValue -= LogicalMin;

    if (logicalRange / physicalRange > 3)
    {
        temp = physicalRange;
        temp *= shiftedValue;
        temp /= logicalRange;

        shiftedResult = temp;
    }
    else if (physicalRange - logicalRange < 0)
    {
        temp = logicalRange;
        temp -= physicalRange;
        temp *= shiftedValue;
        temp /= logicalRange;

        shiftedResult = shiftedValue - temp;
    }
    else
    {
        temp = physicalRange;
        temp -= logicalRange;
        temp *= shiftedValue;
        temp /= logicalRange;

        shiftedResult = shiftedValue + temp;
    }

    result = shiftedResult + PhysicalMin;

    return result;
}

